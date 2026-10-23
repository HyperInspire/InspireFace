package com.example.inspireface_example.view;

import androidx.annotation.NonNull;
import androidx.camera.camera2.interop.ExperimentalCamera2Interop;
import androidx.camera.core.ImageAnalysis;
import androidx.camera.core.ImageProxy;

import com.example.inspireface_example.plus.FaceFrame;
import com.example.inspireface_example.plus.FaceGuideGeometry;
import com.example.inspireface_example.plus.PlusCaptureCoordinator;
import com.example.inspireface_example.plus.RgbPixels;
import com.insightface.sdk.inspireface.InspireFace;
import com.insightface.sdk.inspireface.base.CustomParameter;
import com.insightface.sdk.inspireface.base.FaceEulerAngle;
import com.insightface.sdk.inspireface.base.FaceQualityConfidence;
import com.insightface.sdk.inspireface.base.ImageStream;
import com.insightface.sdk.inspireface.base.MultipleFaceData;
import com.insightface.sdk.inspireface.base.Point2f;
import com.insightface.sdk.inspireface.base.Session;

/** Native objects stay on the analyzer worker and never enter the HTTP request. */
@androidx.annotation.OptIn(markerClass = ExperimentalCamera2Interop.class)
final class PlusFaceAnalyzer implements ImageAnalysis.Analyzer {
    interface Listener { void onFrame(long generation, FaceFrame face); void onEngineError(); }
    private final Nv21Converter converter = new Nv21Converter();
    private final PlusCameraController camera;
    private final PlusCaptureCoordinator coordinator;
    private final PlusFaceGuideView guide;
    private final Listener listener;
    private final boolean color;
    private final CustomParameter qualityParameter = InspireFace.CreateCustomParameter().enableFaceQuality(true);
    private Session session;
    private int alignedTrackId = -1;
    private long alignedTimestampNs = -1;
    private volatile boolean released;

    PlusFaceAnalyzer(PlusCameraController camera, PlusCaptureCoordinator coordinator,
                     PlusFaceGuideView guide, Listener listener, boolean color) {
        this.camera = camera; this.coordinator = coordinator; this.guide = guide;
        this.listener = listener; this.color = color;
    }
    @Override public void analyze(@NonNull ImageProxy image) {
        long generation = coordinator.generation();
        byte[] nv21;
        int width = image.getWidth(), height = image.getHeight();
        int rotation = image.getImageInfo().getRotationDegrees();
        long timestamp = image.getImageInfo().getTimestamp();
        try {
            if (released) return;
            nv21 = converter.convert(image);
        } catch (RuntimeException e) { listener.onEngineError(); return; }
        finally { image.close(); }
        if (released) return;
        try {
            byte[] upright = converter.rotateUpright(nv21, width, height, rotation);
            int w = rotation % 180 == 0 ? width : height, h = rotation % 180 == 0 ? height : width;
            if (session == null) session = FaceEngine.createPlusSession();
            if (session == null) { released = true; listener.onEngineError(); return; }
            try (ImageStream stream = InspireFace.CreateImageStreamFromByteBuffer(upright, w, h,
                    InspireFace.STREAM_YUV_NV21, InspireFace.CAMERA_ROTATION_0)) {
                if (stream == null) { listener.onEngineError(); return; }
                MultipleFaceData faces = InspireFace.ExecuteFaceTrack(session, stream);
                PlusCameraController.Metadata meta = camera.metadata(timestamp);
                FaceFrame observation = observe(faces, stream, meta, timestamp, w, h, rotation);
                coordinator.offer(generation, observation, () -> RgbPixels.cropNv21(upright, observation));
                listener.onFrame(generation, observation);
            }
        } catch (RuntimeException e) { listener.onEngineError(); }
    }

    private FaceFrame observe(MultipleFaceData faces, ImageStream stream, PlusCameraController.Metadata meta,
                              long timestamp, int w, int h, int rotation) {
        FaceFrame.Issue issue = FaceFrame.Issue.NONE;
        int id = -1;
        float ax = 0, ay = 0, bx = 0, by = 0;
        float yaw = Float.NaN, pitch = Float.NaN, roll = Float.NaN;
        FaceGuideGeometry.Feedback feedback = null;
        if (faces == null || faces.detectedNum == 0) issue = FaceFrame.Issue.NO_FACE;
        else if (faces.detectedNum != 1) issue = FaceFrame.Issue.MULTIPLE;
        else {
            id = faces.trackIds[0];
            Point2f[] points = InspireFace.GetFaceDenseLandmarkFromFaceToken(faces.tokens[0]);
            if (points == null || points.length != 106) issue = FaceFrame.Issue.EYES;
            else {
                Point2f a = points[67], b = points[68];
                if (a.x > b.x) { Point2f t = a; a = b; b = t; }
                ax = a.x; ay = a.y; bx = b.x; by = b.y;
                double distance = Math.hypot(bx - ax, by - ay);
                FaceEulerAngle angle = faces.angles == null ? null : faces.angles[0];
                if (angle != null) { yaw = angle.yaw; pitch = angle.pitch; roll = angle.roll; }
                if (angle == null || !Float.isFinite(angle.yaw) || !Float.isFinite(angle.pitch)
                        || !Float.isFinite(angle.roll) || Math.abs(angle.yaw) > 20
                        || Math.abs(angle.pitch) > 18 || Math.abs(angle.roll) > 15) issue = FaceFrame.Issue.POSE;
                else if (distance < w * .075) issue = FaceFrame.Issue.TOO_FAR;
                else if (distance > w * .42) issue = FaceFrame.Issue.TOO_CLOSE;
                if (faces.rects != null && faces.rects.length > 0) {
                    boolean wasAligned = alignedTrackId == id && timestamp >= alignedTimestampNs
                            && timestamp - alignedTimestampNs < 300_000_000L;
                    feedback = guide.measureFace(w, h, faces.rects[0].x, faces.rects[0].y,
                            faces.rects[0].x + faces.rects[0].width, faces.rects[0].y + faces.rects[0].height, wasAligned);
                    if (issue == FaceFrame.Issue.NONE) issue = feedback.issue;
                }
                // Framing owns position guidance. Avoid an extra, conflicting eye-center box.
                // Natural-light quality is used before flash and for RGB, not colored illumination.
                if (issue == FaceFrame.Issue.NONE && (!color
                        || coordinator.state() != PlusCaptureCoordinator.State.CAPTURING || coordinator.count() == 0)) {
                    boolean ran = InspireFace.MultipleFacePipelineProcess(session, stream, faces, qualityParameter);
                    FaceQualityConfidence quality = ran ? InspireFace.GetFaceQualityConfidence(session) : null;
                    if (quality == null || quality.num < 1 || quality.confidence == null
                            || !Float.isFinite(quality.confidence[0]) || quality.confidence[0] < .3f) issue = FaceFrame.Issue.QUALITY;
                }
            }
        }
        if (meta == null && issue == FaceFrame.Issue.NONE) issue = FaceFrame.Issue.METADATA;
        // A brief quality/pose fluctuation should not also reset the spatial exit margin.
        alignedTrackId = feedback != null && feedback.issue == FaceFrame.Issue.NONE ? id : -1;
        alignedTimestampNs = timestamp;
        return new FaceFrame(meta == null ? -1 : meta.index, meta == null ? timestamp : meta.timestampNs,
                meta != null && meta.realtime ? meta.timestampNs : -1, w, h, rotation, id,
                ax, ay, bx, by, meta != null && meta.locksConfirmed, issue, yaw, pitch, roll, feedback);
    }
    void stopAccepting() { released = true; }
    /** Queue after pending analyze work, on its same executor. */
    void release() {
        released = true;
        if (session != null) { FaceEngine.releaseSession(session); session = null; }
    }
}
