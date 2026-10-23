package com.example.inspireface_example.plus;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.List;

import okhttp3.HttpUrl;
import okhttp3.MediaType;
import okhttp3.MultipartBody;
import okhttp3.Request;
import okhttp3.RequestBody;

/** Frozen multipart request. Keep this same object for retries of one recording. */
public final class CapturePacket {
    private static final String[] PHASES = {"white", "red", "green", "blue"};
    public final String captureId, module, mode;
    private final MultipartBody body;

    public CapturePacket(String captureId, boolean color, List<PlusCaptureCoordinator.Sample> samples)
            throws JSONException {
        if (captureId == null || captureId.isEmpty() || captureId.length() > 128
                || samples.size() != (color ? 4 : 20)) throw new IllegalArgumentException("Incomplete recording");
        this.captureId = captureId; module = color ? "color_liveness" : "rgb_liveness";
        mode = color ? "color" : "rgb_sequence";
        JSONArray frames = new JSONArray();
        MultipartBody.Builder multipart = new MultipartBody.Builder().setType(MultipartBody.FORM);
        FaceFrame previous = null, first = null;
        for (int i = 0; i < samples.size(); i++) {
            PlusCaptureCoordinator.Sample sample = samples.get(i);
            FaceFrame f = sample.frame;
            if (!f.valid() || (color ? sample.phase != i : sample.phase != -1)) {
                throw new IllegalArgumentException("Invalid phase or face");
            }
            if (previous != null && (f.sourceIndex <= previous.sourceIndex
                    || f.timestampNs / 1000 <= previous.timestampNs / 1000
                    || (!color && f.sourceIndex != previous.sourceIndex + 1)
                    || !f.sameGeometry(color ? first : previous, color))) {
                throw new IllegalArgumentException("Invalid continuity or geometry");
            }
            if (first == null) first = f;
            previous = f;
            byte[] pixels = sample.pixels();
            String part = "frame_" + i + ".rgb";
            JSONObject row = new JSONObject().put("source_index", f.sourceIndex)
                    .put("timestamp_us", f.timestampNs / 1000).put("status", "ok")
                    .put("source_size", new JSONArray().put(f.width).put(f.height))
                    .put("eyes_xy", new JSONArray()
                            .put(new JSONArray().put((double) f.ax).put((double) f.ay))
                            .put(new JSONArray().put((double) f.bx).put((double) f.by)))
                    .put("image", new JSONObject().put("part", part)
                            .put("encoding", "rgb8").put("sha256", sha256(pixels)));
            if (color) row.put("phase", PHASES[i]);
            frames.put(row);
            multipart.addFormDataPart(part, part,
                    RequestBody.create(MediaType.get("application/octet-stream"), pixels));
        }
        JSONObject metadata = new JSONObject().put("protocol_version", 1).put("capture_id", captureId)
                .put("mode", mode).put("capture_profile", color ? "color_wrgb_v1" : "rgb_contiguous_v1")
                .put("preprocess_profile", "eye_crop_320_v1").put("complete", true).put("frames", frames);
        multipart.addFormDataPart("metadata", metadata.toString());
        body = multipart.build();
    }

    public Request request(String base, String token) {
        if (token == null || token.trim().isEmpty()) throw new IllegalArgumentException("Missing service configuration");
        HttpUrl url = HttpUrl.parse(base.replaceAll("/+$", "") + "/v1/modules/" + module + "/verify");
        if (url == null || !url.isHttps()) {
            throw new IllegalArgumentException("Use an HTTPS InspireFacePlus endpoint");
        }
        return new Request.Builder().url(url).header("Authorization", "Bearer " + token.trim())
                .header("Idempotency-Key", captureId).post(body).build();
    }

    public static String sha256(byte[] bytes) {
        try {
            byte[] digest = MessageDigest.getInstance("SHA-256").digest(bytes);
            char[] alphabet = "0123456789abcdef".toCharArray();
            StringBuilder result = new StringBuilder(64);
            for (byte b : digest) result.append(alphabet[(b & 255) >>> 4]).append(alphabet[b & 15]);
            return result.toString();
        } catch (NoSuchAlgorithmException impossible) { throw new AssertionError(impossible); }
    }
}
