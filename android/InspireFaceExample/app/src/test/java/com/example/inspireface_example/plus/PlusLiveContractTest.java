package com.example.inspireface_example.plus;

import org.junit.Assume;
import org.junit.Test;
import java.io.File;
import java.io.FileInputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.Properties;
import java.util.UUID;
import java.util.concurrent.TimeUnit;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.Response;
import static org.junit.Assert.*;

/** Explicit opt-in: two metered captures of synthetic pixels, never a face/photo fixture. */
public class PlusLiveContractTest {
    @Test public void syntheticColorAndRgbRoundTripAndIdempotentReplay() throws Exception {
        Assume.assumeTrue("1".equals(System.getenv("INSPIREFACE_PLUS_LIVE_TEST")));
        Properties config = new Properties();
        File configFile = new File("../plus.local.properties");
        if (!configFile.isFile()) configFile = new File("plus.local.properties");
        try (FileInputStream input = new FileInputStream(configFile)) { config.load(input); }
        OkHttpClient http = new OkHttpClient.Builder().callTimeout(90, TimeUnit.SECONDS)
                .retryOnConnectionFailure(false).followRedirects(false).build();
        for (boolean color : new boolean[] {true, false}) {
            List<PlusCaptureCoordinator.Sample> samples = new ArrayList<>();
            for (int i = 0; i < (color ? 4 : 20); i++) {
                int[] source = new int[640 * 480];
                int phase = color ? i : 0;
                for (int y = 0; y < 480; y++) for (int x = 0; x < 640; x++) {
                    source[y * 640 + x] = 0xff000000 | (((x + phase * 23) & 255) << 16)
                            | (((y + phase * 17) & 255) << 8) | ((x + y + phase * 11) & 255);
                }
                FaceFrame info = PlusCaptureTest.frame(i, PlusCaptureTest.START + i * 500_000_000L, true, FaceFrame.Issue.NONE);
                byte[] crop = RgbPixels.fromArgb(EyeCrop320.crop(source, 640, 480, info.ax, info.ay, info.bx, info.by));
                samples.add(new PlusCaptureCoordinator.Sample(info, color ? i : -1, crop));
            }
            CapturePacket packet = new CapturePacket(UUID.randomUUID().toString(), color, samples);
            Request request = packet.request(config.getProperty("apiBase"), config.getProperty("token"));
            String original;
            try (Response response = http.newCall(request).execute()) {
                assertEquals("Verify HTTP status", 200, response.code());
                original = response.body().string();
                PlusLivenessClient.Result result = PlusLivenessClient.parseResult(original, packet);
                assertFalse("Complete synthetic request should return a model verdict", result.retry);
                assertTrue(Double.isFinite(result.score));
            }
            try (Response replay = http.newCall(request).execute()) {
                assertEquals(200, replay.code());
                assertEquals("true", replay.header("Idempotency-Replayed"));
                assertEquals("Replay must return saved result without another inference", original, replay.body().string());
            }
        }
    }
}
