package com.example.inspireface_example.plus;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.IOException;
import java.util.concurrent.TimeUnit;

import okhttp3.Call;
import okhttp3.Callback;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.Response;

/** One cancellable call. Transport errors never become a non-live verdict. */
public final class PlusLivenessClient {
    public enum Error { NETWORK, AUTH, FORBIDDEN, QUOTA, BUSY, CONFLICT, INPUT, SERVER, RESPONSE }
    public interface Listener {
        void onResult(Result result);
        void onError(Error error, boolean retryable, int retryAfterSeconds);
    }
    public static final class Result {
        public final boolean retry, alive;
        public final double score;
        public final int windowFrames;
        public final String requestId;
        Result(boolean retry, boolean alive, double score, int windowFrames, String requestId) {
            this.retry = retry; this.alive = alive; this.score = score;
            this.windowFrames = windowFrames; this.requestId = requestId;
        }
    }
    private final OkHttpClient client;
    private Call current;

    public PlusLivenessClient() {
        this(new OkHttpClient.Builder().connectTimeout(15, TimeUnit.SECONDS)
                .writeTimeout(30, TimeUnit.SECONDS).readTimeout(60, TimeUnit.SECONDS)
                .callTimeout(90, TimeUnit.SECONDS).retryOnConnectionFailure(false)
                .followRedirects(false).followSslRedirects(false).build());
    }
    public PlusLivenessClient(OkHttpClient client) { this.client = client; }

    public synchronized void submit(Request frozen, CapturePacket packet, Listener listener) {
        cancel();
        current = client.newCall(frozen);
        current.enqueue(new Callback() {
            @Override public void onFailure(Call call, IOException e) {
                if (!call.isCanceled()) listener.onError(Error.NETWORK, true, 1);
            }
            @Override public void onResponse(Call call, Response response) {
                try (Response closed = response) {
                    if (call.isCanceled()) return;
                    // A normal response is a small result JSON; cap unexpected bodies.
                    String text = closed.peekBody(256 * 1024).string();
                    if (!closed.isSuccessful()) {
                        String code = "";
                        try { code = new JSONObject(text).getJSONObject("error").optString("code"); }
                        catch (JSONException ignored) { }
                        Error error = classify(closed.code(), code);
                        boolean retry = error == Error.BUSY || (error == Error.SERVER
                                && !"settlement_failed".equals(code));
                        int wait = 2;
                        try { wait = Math.max(1, Integer.parseInt(closed.header("Retry-After", "2"))); }
                        catch (NumberFormatException ignored) { }
                        listener.onError(error, retry, wait);
                        return;
                    }
                    listener.onResult(parseResult(text, packet));
                } catch (IOException e) {
                    if (!call.isCanceled()) listener.onError(Error.NETWORK, true, 1);
                } catch (JSONException | IllegalArgumentException e) {
                    // An uncertain response can be retrieved again with the same key.
                    listener.onError(Error.RESPONSE, true, 2);
                }
            }
        });
    }

    public static Result parseResult(String text, CapturePacket packet) throws JSONException {
        JSONObject root = new JSONObject(text);
        if (!packet.module.equals(root.getString("module_id"))) throw new JSONException("Wrong module");
        JSONArray results = root.getJSONArray("results");
        if (results.length() != 1) throw new JSONException("Expected one capture result");
        JSONObject row = results.getJSONObject(0);
        if (!packet.captureId.equals(row.getString("capture_id")) || !packet.mode.equals(row.getString("mode"))) {
            throw new JSONException("Wrong capture");
        }
        String state = row.getString("status");
        if ("retry".equals(state)) {
            if (!row.has("score") || !row.has("alive") || !row.isNull("score") || !row.isNull("alive")) {
                throw new JSONException("Invalid retry result");
            }
            return new Result(true, false, Double.NaN, row.optInt("final_window_frames", 0), root.optString("request_id"));
        }
        if (!"ok".equals(state) || !(row.get("alive") instanceof Boolean)) throw new JSONException("Invalid verdict");
        double score = row.getDouble("score");
        if (!Double.isFinite(score) || score < 0 || score > 1) throw new JSONException("Invalid score");
        return new Result(false, row.getBoolean("alive"), score,
                row.optInt("final_window_frames", 0), root.optString("request_id"));
    }

    private static Error classify(int status, String code) {
        if (status == 401) return Error.AUTH;
        if (status == 403 || status == 404) return Error.FORBIDDEN;
        if (status == 429) return Error.QUOTA;
        if (status == 409) return "request_in_progress".equals(code) ? Error.BUSY : Error.CONFLICT;
        if (status == 503) return "module_disabled".equals(code) ? Error.FORBIDDEN : Error.BUSY;
        if (status >= 500) return Error.SERVER;
        return Error.INPUT;
    }
    public synchronized void cancel() {
        if (current != null) { current.cancel(); current = null; }
    }
}
