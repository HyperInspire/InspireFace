package com.example.inspireface_example.view;

import android.graphics.Color;
import android.os.SystemClock;
import android.util.SparseIntArray;

import androidx.annotation.Nullable;

import com.insightface.sdk.inspireface.InspireFace;
import com.insightface.sdk.inspireface.base.FaceRect;
import com.insightface.sdk.inspireface.base.ImageStream;
import com.insightface.sdk.inspireface.base.MultipleFaceData;
import com.insightface.sdk.inspireface.base.Point2f;
import com.insightface.sdk.inspireface.base.Session;

/** Runs configurable multi-face tracking and submits one batched GL frame per camera frame. */
public final class FaceTrackingAnalyzer extends UprightFaceCameraAnalyzer {

    public interface Listener {
        void onSessionReady();

        /** Throttled callback on the camera analysis executor. */
        void onStats(double fps, long latencyMs);

        void onSessionError();
    }

    private static final long REPORT_INTERVAL_MS = 300L;
    private static final int[] TRACK_COLORS = {
            0xFF00E5A0, 0xFFFFC400, 0xFF40C4FF, 0xFFFF6E9C, 0xFFC6A5FF,
            0xFFFF8A65, 0xFF76FF03, 0xFF18FFFF, 0xFFFFD180, 0xFFEA80FC
    };

    private final FaceTrackingGlView overlay;
    private final TrackIdOverlayView trackIdOverlay;
    private final int detectMode;
    private final int detectPixelLevel;
    private final int maxFaces;
    private final int minimumFacePixelSize;
    private final Listener listener;
    private final float[] pointVertices = new float[
            FaceTrackingGlView.MAX_FACES * FaceTrackingGlView.LANDMARKS_PER_FACE
                    * FaceTrackingGlView.FLOATS_PER_VERTEX];
    private final float[] boxVertices = new float[
            FaceTrackingGlView.MAX_FACES * FaceTrackingGlView.BOX_VERTICES_PER_FACE
                    * FaceTrackingGlView.FLOATS_PER_VERTEX];
    private final int[] labelTrackIds = new int[FaceTrackingGlView.MAX_FACES];
    private final int[] labelColors = new int[FaceTrackingGlView.MAX_FACES];
    private final float[] labelRects = new float[FaceTrackingGlView.MAX_FACES * 4];
    private final SparseIntArray colorByTrackId = new SparseIntArray();
    private final TrackBoxSmoother boxSmoother = new TrackBoxSmoother();
    private final float[] smoothedRect = new float[4];
    private int nextColorOrdinal;

    private volatile boolean mirrored = true;
    private long fpsWindowStart;
    private int fpsWindowFrames;
    private double fps;
    private double latencyEmaMs;
    private long lastReport;

    public FaceTrackingAnalyzer(FaceTrackingGlView overlay,
                                TrackIdOverlayView trackIdOverlay,
                                int detectMode, int detectPixelLevel,
                                int maxFaces, int minimumFacePixelSize,
                                Listener listener) {
        this.overlay = overlay;
        this.trackIdOverlay = trackIdOverlay;
        this.detectMode = detectMode;
        this.detectPixelLevel = detectPixelLevel;
        this.maxFaces = maxFaces;
        this.minimumFacePixelSize = minimumFacePixelSize;
        this.listener = listener;
    }

    @Override
    protected Session createSession() {
        return FaceEngine.createFaceTrackingSession(
                detectMode, detectPixelLevel, maxFaces, minimumFacePixelSize);
    }

    @Override
    protected void onSessionReady() {
        listener.onSessionReady();
    }

    @Override
    protected void onFaces(Session session, ImageStream stream,
                           @Nullable MultipleFaceData faces, byte[] uprightNv21,
                           int uprightWidth, int uprightHeight, long frameStart) {
        int detected = faces == null ? 0
                : Math.min(faces.detectedNum, FaceTrackingGlView.MAX_FACES);
        if (detected <= 0) {
            overlay.clearTracking();
            trackIdOverlay.clearTracking();
            reportStats(frameStart);
            return;
        }

        int pointCount = 0;
        int boxCount = 0;
        int labelCount = 0;
        boxSmoother.beginFrame();
        for (int i = 0; i < detected; i++) {
            int sdkTrackId = faces.trackIds != null && i < faces.trackIds.length
                    ? faces.trackIds[i] : -1;
            int colorKey = sdkTrackId >= 0 ? sdkTrackId : Integer.MIN_VALUE + i;
            int color = colorForTrackId(colorKey);
            Point2f[] landmarks = faces.tokens != null && i < faces.tokens.length
                    && faces.tokens[i] != null
                    ? InspireFace.GetFaceDenseLandmarkFromFaceToken(faces.tokens[i]) : null;
            if (faces.rects != null && i < faces.rects.length && faces.rects[i] != null) {
                FaceRect rect = faces.rects[i];
                boxSmoother.smooth(colorKey, rect.x, rect.y,
                        rect.x + rect.width, rect.y + rect.height, smoothedRect);
                boxCount = appendBrackets(
                        boxVertices, boxCount, smoothedRect[0], smoothedRect[1],
                        smoothedRect[2], smoothedRect[3], color);
                if (sdkTrackId >= 0 && labelCount < FaceTrackingGlView.MAX_FACES) {
                    labelTrackIds[labelCount] = sdkTrackId;
                    labelColors[labelCount] = color;
                    int offset = labelCount * 4;
                    labelRects[offset] = smoothedRect[0];
                    labelRects[offset + 1] = smoothedRect[1];
                    labelRects[offset + 2] = smoothedRect[2];
                    labelRects[offset + 3] = smoothedRect[3];
                    labelCount++;
                }
            }
            if (landmarks == null || landmarks.length == 0) {
                continue;
            }
            for (Point2f point : landmarks) {
                if (point == null || pointCount >= FaceTrackingGlView.MAX_FACES
                        * FaceTrackingGlView.LANDMARKS_PER_FACE) {
                    continue;
                }
                appendVertex(pointVertices, pointCount++, point.x, point.y, color);
            }
        }

        overlay.submit(pointVertices, pointCount, boxVertices, boxCount,
                uprightWidth, uprightHeight, mirrored);
        trackIdOverlay.submit(labelTrackIds, labelRects, labelColors, labelCount,
                uprightWidth, uprightHeight, mirrored);
        reportStats(frameStart);
    }

    @Override
    protected void onSessionError() {
        overlay.clearTracking();
        trackIdOverlay.clearTracking();
        boxSmoother.clear();
        listener.onSessionError();
    }

    public void setMirrored(boolean mirrored) {
        this.mirrored = mirrored;
        overlay.clearTracking();
        trackIdOverlay.clearTracking();
        boxSmoother.clear();
    }

    public void clearTracking() {
        overlay.clearTracking();
        trackIdOverlay.clearTracking();
        boxSmoother.clear();
    }

    private void reportStats(long frameStart) {
        long now = SystemClock.elapsedRealtime();
        long latency = Math.max(0L, now - frameStart);
        latencyEmaMs = latencyEmaMs == 0.0
                ? latency : latencyEmaMs * 0.85 + latency * 0.15;
        fpsWindowFrames++;
        if (fpsWindowStart == 0L) {
            fpsWindowStart = now;
        } else if (now - fpsWindowStart >= 1_000L) {
            fps = fpsWindowFrames * 1_000.0 / (now - fpsWindowStart);
            fpsWindowFrames = 0;
            fpsWindowStart = now;
        }
        if (now - lastReport >= REPORT_INTERVAL_MS) {
            lastReport = now;
            listener.onStats(fps, Math.round(latencyEmaMs));
        }
    }

    /** Keeps a Track ID's color stable for the lifetime of this camera Session. */
    private int colorForTrackId(int trackId) {
        int existing = colorByTrackId.indexOfKey(trackId);
        if (existing >= 0) {
            return colorByTrackId.valueAt(existing);
        }
        int color;
        int attempts = 0;
        do {
            color = colorForOrdinal(nextColorOrdinal++);
            attempts++;
        } while (colorByTrackId.indexOfValue(color) >= 0 && attempts < 720);
        colorByTrackId.put(trackId, color);
        return color;
    }

    private static int colorForOrdinal(int ordinal) {
        if (ordinal < TRACK_COLORS.length) {
            return TRACK_COLORS[ordinal];
        }
        // The golden-angle sequence keeps later IDs visually separated without a small palette.
        float hue = (17f + ordinal * 137.50776f) % 360f;
        float saturation = 0.68f + (ordinal % 3) * 0.08f;
        return Color.HSVToColor(new float[]{hue, saturation, 1f});
    }

    private static int appendBrackets(float[] target, int vertexCount,
                                      float left, float top, float right, float bottom,
                                      int color) {
        float length = Math.min(right - left, bottom - top) * 0.22f;

        vertexCount = appendLine(target, vertexCount,
                left, top + length, left, top, color);
        vertexCount = appendLine(target, vertexCount,
                left, top, left + length, top, color);
        vertexCount = appendLine(target, vertexCount,
                right - length, top, right, top, color);
        vertexCount = appendLine(target, vertexCount,
                right, top, right, top + length, color);
        vertexCount = appendLine(target, vertexCount,
                left, bottom - length, left, bottom, color);
        vertexCount = appendLine(target, vertexCount,
                left, bottom, left + length, bottom, color);
        vertexCount = appendLine(target, vertexCount,
                right - length, bottom, right, bottom, color);
        vertexCount = appendLine(target, vertexCount,
                right, bottom, right, bottom - length, color);
        return vertexCount;
    }

    private static int appendLine(float[] target, int vertexCount,
                                  float x0, float y0, float x1, float y1, int color) {
        appendVertex(target, vertexCount++, x0, y0, color);
        appendVertex(target, vertexCount++, x1, y1, color);
        return vertexCount;
    }

    private static void appendVertex(float[] target, int vertexIndex,
                                     float x, float y, int color) {
        int offset = vertexIndex * FaceTrackingGlView.FLOATS_PER_VERTEX;
        target[offset] = x;
        target[offset + 1] = y;
        target[offset + 2] = Color.red(color) / 255f;
        target[offset + 3] = Color.green(color) / 255f;
        target[offset + 4] = Color.blue(color) / 255f;
        target[offset + 5] = Color.alpha(color) / 255f;
    }
}
