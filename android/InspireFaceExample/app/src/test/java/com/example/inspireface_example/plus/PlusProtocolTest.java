package com.example.inspireface_example.plus;

import org.json.JSONObject;
import org.junit.Test;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;
import okhttp3.MultipartBody;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.mockwebserver.MockResponse;
import okhttp3.mockwebserver.MockWebServer;
import okhttp3.mockwebserver.RecordedRequest;
import okio.Buffer;
import static org.junit.Assert.*;

public class PlusProtocolTest {
    private CapturePacket packet() throws Exception {
        PlusCaptureCoordinator c = PlusCaptureTest.completeColor();
        return new CapturePacket(c.captureId(), true, c.snapshot(c.generation()));
    }
    private String response(CapturePacket packet, boolean retry, boolean alive) throws Exception {
        JSONObject row = new JSONObject().put("capture_id", packet.captureId).put("mode", packet.mode)
                .put("status", retry ? "retry" : "ok").put("alive", retry ? JSONObject.NULL : alive)
                .put("score", retry ? JSONObject.NULL : .42);
        return new JSONObject().put("module_id", packet.module).put("request_id", "test-request")
                .put("results", new org.json.JSONArray().put(row)).toString();
    }
    @Test public void multipartContainsExactProtocolAndHashesOfUploadedBytes() throws Exception {
        CapturePacket packet = packet();
        Request request = packet.request("https://api.inspirehub.cc/", "test-credential");
        assertEquals("https://api.inspirehub.cc/v1/modules/color_liveness/verify", request.url().toString());
        assertEquals(packet.captureId, request.header("Idempotency-Key"));
        MultipartBody body = (MultipartBody) request.body();
        assertEquals(5, body.parts().size());
        MultipartBody.Part meta = body.part(4);
        assertEquals("form-data; name=\"metadata\"", meta.headers().get("Content-Disposition"));
        Buffer text = new Buffer(); meta.body().writeTo(text);
        JSONObject json = new JSONObject(text.readUtf8());
        assertEquals("color_wrgb_v1", json.getString("capture_profile"));
        assertEquals("eye_crop_320_v1", json.getString("preprocess_profile"));
        assertEquals(7, json.length());
        String[] phases = {"white", "red", "green", "blue"};
        for (int i = 0; i < 4; i++) {
            JSONObject row = json.getJSONArray("frames").getJSONObject(i);
            assertEquals(phases[i], row.getString("phase"));
            assertEquals(i * 5, row.getInt("source_index"));
            assertEquals(640, row.getJSONArray("source_size").getInt(0));
            assertEquals(270., row.getJSONArray("eyes_xy").getJSONArray(0).getDouble(0), 0);
            assertFalse(row.has("display_time_us"));
            Buffer pixels = new Buffer(); body.part(i).body().writeTo(pixels);
            byte[] bytes = pixels.readByteArray(); assertEquals(307200, bytes.length);
            assertEquals(CapturePacket.sha256(bytes), row.getJSONObject("image").getString("sha256"));
        }
    }
    @Test public void http200IsNotAutomaticallyALiveVerdict() throws Exception {
        CapturePacket p = packet();
        PlusLivenessClient.Result failed = PlusLivenessClient.parseResult(response(p, false, false), p);
        assertFalse(failed.alive); assertFalse(failed.retry);
        PlusLivenessClient.Result retry = PlusLivenessClient.parseResult(response(p, true, false), p);
        assertTrue(retry.retry); assertTrue(Double.isNaN(retry.score));
    }
    @Test public void silentRequestUsesNewDomainAndExistingModulePath() throws Exception {
        PlusCaptureCoordinator c = new PlusCaptureCoordinator();
        long token = c.start(false, PlusCaptureTest.START, -1);
        for (int i = 0; i < 20; i++) {
            assertTrue(c.offer(token, PlusCaptureTest.frame(i, PlusCaptureTest.START + i * 50_000_000L,
                    false, FaceFrame.Issue.NONE), () -> new byte[307200]));
        }
        CapturePacket packet = new CapturePacket(c.captureId(), false, c.snapshot(token));
        assertEquals("https://api.inspirehub.cc/v1/modules/rgb_liveness/verify",
                packet.request("https://api.inspirehub.cc", "test-credential").url().toString());
    }
    @Test public void rejectsCleartextEndpoints() throws Exception {
        CapturePacket packet = packet();
        for (String base : new String[]{"http://api.inspirehub.cc", "http://84.247.156.1:8000", "http://api.inspirehub.cc:8000",
                "http://example.com", "http://api.inspirehub.cc.example.com"}) {
            try { packet.request(base, "test-credential"); fail("HTTP endpoints must be rejected"); }
            catch (IllegalArgumentException expected) { }
        }
        assertTrue(packet.request("https://api.inspirehub.cc", "test-credential").url().isHttps());
    }
    @Test public void rejectsAnotherRoundsResult() throws Exception {
        CapturePacket p = packet();
        try { PlusLivenessClient.parseResult(response(packet(), false, true), p); fail("must match capture ID"); }
        catch (org.json.JSONException expected) { }
    }
    @Test public void retrySubmitsIdenticalBytesAndKey() throws Exception {
        try (MockWebServer server = new MockWebServer()) {
            CapturePacket p = packet();
            server.enqueue(new MockResponse().setResponseCode(503).setHeader("Retry-After", "3")
                    .setBody("{\"error\":{\"code\":\"server_busy\"}}"));
            server.enqueue(new MockResponse().setBody(response(p, false, true)));
            Request frozen = p.request("https://api.inspirehub.cc", "test-credential")
                    .newBuilder().url(server.url("/verify")).build();
            PlusLivenessClient client = new PlusLivenessClient(new OkHttpClient());
            CountDownLatch first = new CountDownLatch(1);
            AtomicReference<PlusLivenessClient.Error> error = new AtomicReference<>();
            client.submit(frozen, p, new PlusLivenessClient.Listener() {
                public void onResult(PlusLivenessClient.Result r) { first.countDown(); }
                public void onError(PlusLivenessClient.Error e, boolean retryable, int wait) {
                    error.set(e); if (retryable && wait == 3) first.countDown();
                }
            });
            assertTrue(first.await(5, TimeUnit.SECONDS)); assertEquals(PlusLivenessClient.Error.BUSY, error.get());
            CountDownLatch second = new CountDownLatch(1);
            client.submit(frozen, p, new PlusLivenessClient.Listener() {
                public void onResult(PlusLivenessClient.Result r) { if (r.alive) second.countDown(); }
                public void onError(PlusLivenessClient.Error e, boolean retryable, int wait) { }
            });
            assertTrue(second.await(5, TimeUnit.SECONDS));
            RecordedRequest a = server.takeRequest(1, TimeUnit.SECONDS), b = server.takeRequest(1, TimeUnit.SECONDS);
            assertNotNull(a); assertNotNull(b);
            assertEquals(a.getHeader("Idempotency-Key"), b.getHeader("Idempotency-Key"));
            assertEquals(a.getHeader("Content-Type"), b.getHeader("Content-Type"));
            assertArrayEquals(a.getBody().readByteArray(), b.getBody().readByteArray());
            client.cancel();
        }
    }
}
