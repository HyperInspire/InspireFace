package com.example.inspireface_example.view;

import android.content.Context;
import android.util.Log;

import com.example.inspireface_example.BuildConfig;
import com.example.inspireface_example.DetectorDefaults;
import com.example.inspireface_example.FaceModelPrefs;
import com.insightface.sdk.inspireface.InspireFace;
import com.insightface.sdk.inspireface.base.CustomParameter;
import com.insightface.sdk.inspireface.base.InspireFaceVersion;
import com.insightface.sdk.inspireface.base.Session;

/**
 * Process-wide InspireFace lifecycle: one GlobalLaunch, session factory tuned for
 * low-latency front-camera preview.
 */
public final class FaceEngine {

    private static final String TAG = "FaceEngine";
    private static final long SESSION_RELEASE_WAIT_MS = 2_000;

    /** Max faces tracked per frame — enough to notice "more than one face" and stay cheap. */
    private static final int PREVIEW_MAX_FACES = 3;
    private static final int IMAGE_MAX_FACES = 10;

    /**
     * Long-edge size the tracker scales frames to before detection. Matches the default
     * 320 detect level; in LIGHT_TRACK mode detection is amortized (~1 in 20 frames) so
     * this costs little.
     */
    private static final int TRACK_PREVIEW_SIZE = 320;

    private static volatile boolean launched;
    private static String launchedModel;
    private static int activeSessions;

    private FaceEngine() {
    }

    /**
     * Loads the globally selected model. Changing the home-screen selection terminates the
     * previous model before the next feature screen launches the new one.
     */
    public static synchronized boolean ensureLaunched(Context context) {
        String requestedModel = FaceModelPrefs.get(context).sdkName();
        if (launched && requestedModel.equals(launchedModel)) {
            return true;
        }
        if (launched) {
            // Back navigation can reveal the home page a fraction before the previous
            // analysis executor releases its session. Briefly wait for that hand-off;
            // wait() releases this monitor so releaseSession can make progress.
            long deadline = android.os.SystemClock.elapsedRealtime() + SESSION_RELEASE_WAIT_MS;
            while (activeSessions > 0) {
                long remaining = deadline - android.os.SystemClock.elapsedRealtime();
                if (remaining <= 0) {
                    Log.e(TAG, "Cannot switch model while " + activeSessions
                            + " session(s) are active");
                    return false;
                }
                try {
                    FaceEngine.class.wait(remaining);
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    return false;
                }
            }
            if (!InspireFace.GlobalTerminate()) {
                Log.e(TAG, "GlobalTerminate failed for " + launchedModel);
                return false;
            }
            launched = false;
            launchedModel = null;
        }
        if (!launched) {
            InspireFaceVersion version = InspireFace.QueryInspireFaceVersion();
            Log.i(TAG, "Android SDK " + BuildConfig.INSPIREFACE_SDK_VERSION
                    + " (" + BuildConfig.INSPIREFACE_SDK_SOURCE + "), native "
                    + version.major + "." + version.minor + "." + version.patch
                    + ", C API " + InspireFace.QueryCAPILevel());
            launched = Boolean.TRUE.equals(
                    InspireFace.GlobalLaunch(context.getApplicationContext(), requestedModel));
            launchedModel = launched ? requestedModel : null;
            Log.i(TAG, "GlobalLaunch(" + requestedModel + ") -> " + launched);
        }
        return launched;
    }

    /**
     * Session for continuous video tracking with both liveness models loaded. All calls on
     * the returned session must stay on a single thread.
     */
    static synchronized Session createPreviewSession() {
        // Explicitly enable 1.2.4 pose estimation for yaw/pitch and head actions.
        CustomParameter parameter = InspireFace.CreateCustomParameter()
                .enableLiveness(true)
                .enableInteractionLiveness(true)
                .enableFacePose(true)
                .enableFaceQuality(true);
        Session session = InspireFace.CreateSession(
                parameter, InspireFace.DETECT_MODE_LIGHT_TRACK, PREVIEW_MAX_FACES,
                DetectorDefaults.INPUT_PX, -1);
        if (session == null) {
            return null;
        }
        activeSessions++;
        InspireFace.SetTrackPreviewSize(session, TRACK_PREVIEW_SIZE);
        InspireFace.SetFaceDetectThreshold(session, 0.5f);
        InspireFace.SetFilterMinimumFacePixelSize(session, 0);
        return session;
    }

    /** Local face analysis for cloud PLUS capture; liveness inference stays on the service. */
    static synchronized Session createPlusSession() {
        // Re-detect on every analyzed frame, keeping stable IDs. A light tracker may
        // otherwise hide a lost face or a second person between detector intervals.
        return createTrackingSession(InspireFace.CreateCustomParameter()
                .enableFacePose(true).enableFaceQuality(true), PREVIEW_MAX_FACES, 0,
                InspireFace.DETECT_MODE_TRACK_BY_DETECTION, 15);
    }

    /** Lightweight session for camera enrollment: tracking only, no liveness models. */
    static synchronized Session createTrackingSession() {
        return createTrackingSession(
                InspireFace.CreateCustomParameter(), PREVIEW_MAX_FACES, 0);
    }

    /** First-face camera tracking with feature extraction enabled for live 1:N search. */
    static synchronized Session createVideoRecognitionSession() {
        return createTrackingSession(
                InspireFace.CreateCustomParameter().enableRecognition(true), 1, 24);
    }

    private static Session createTrackingSession(
            CustomParameter parameter, int maxFaces, int minimumFacePixelSize) {
        return createTrackingSession(parameter, maxFaces, minimumFacePixelSize,
                InspireFace.DETECT_MODE_LIGHT_TRACK, -1);
    }

    private static Session createTrackingSession(
            CustomParameter parameter, int maxFaces, int minimumFacePixelSize,
            int detectMode, int fps) {
        Session session = InspireFace.CreateSession(
                parameter, detectMode, maxFaces, DetectorDefaults.INPUT_PX, fps);
        if (session == null) {
            return null;
        }
        activeSessions++;
        InspireFace.SetTrackPreviewSize(session, TRACK_PREVIEW_SIZE);
        InspireFace.SetFaceDetectThreshold(session, 0.5f);
        InspireFace.SetFilterMinimumFacePixelSize(session, minimumFacePixelSize);
        return session;
    }

    /** Session for still-image detection and feature extraction. */
    public static synchronized Session createRecognitionSession() {
        return createRecognitionSession(DetectorDefaults.INPUT_PX, IMAGE_MAX_FACES, 0);
    }

    /** Still-image Session with every attribute model used by the attribute demo. */
    public static synchronized Session createAttributeSession() {
        CustomParameter parameter = InspireFace.CreateCustomParameter()
                .enableMaskDetect(true)
                .enableFaceQuality(true)
                .enableFaceAttribute(true)
                .enableInteractionLiveness(true);
        return createStillImageSession(
                parameter, DetectorDefaults.INPUT_PX, IMAGE_MAX_FACES, 0);
    }

    /** Configurable still-image session used by the recognition parameter demo. */
    public static synchronized Session createRecognitionSession(
            int detectPixelLevel, int maxDetectFaceNum, int minimumFacePixelSize) {
        int safeMaxFaces = Math.max(1, maxDetectFaceNum);
        int safeMinimumFacePixelSize = Math.max(0, minimumFacePixelSize);
        return createStillImageSession(
                InspireFace.CreateCustomParameter().enableRecognition(true),
                detectPixelLevel, safeMaxFaces, safeMinimumFacePixelSize);
    }

    /** Detection-only still-image Session used by the landmark demo. */
    public static synchronized Session createDetectionSession(
            int detectPixelLevel, int maxDetectFaceNum, int minimumFacePixelSize) {
        // 1.2.4 computes dense landmarks in every detection mode.
        return createStillImageSession(InspireFace.CreateCustomParameter(),
                detectPixelLevel, maxDetectFaceNum, minimumFacePixelSize);
    }

    /**
     * Configurable continuous tracker with deterministic 106-point tokens in both tracking
     * modes. InspireFace 1.2.4 enables landmarks internally; no option bit is required.
     */
    public static synchronized Session createFaceTrackingSession(
            int detectMode, int detectPixelLevel,
            int maxDetectFaceNum, int minimumFacePixelSize) {
        int safeMode = detectMode == InspireFace.DETECT_MODE_TRACK_BY_DETECTION
                ? detectMode : InspireFace.DETECT_MODE_LIGHT_TRACK;
        int safeMaxFaces = Math.max(1, Math.min(maxDetectFaceNum, IMAGE_MAX_FACES));
        int safeMinimumFacePixelSize = Math.max(0, minimumFacePixelSize);
        int previewSize = detectPixelLevel > 0 ? detectPixelLevel : TRACK_PREVIEW_SIZE;
        Session session = InspireFace.CreateSession(InspireFace.CreateCustomParameter(),
                safeMode, safeMaxFaces, detectPixelLevel, 30);
        if (session == null) {
            return null;
        }
        activeSessions++;
        InspireFace.SetTrackPreviewSize(session, previewSize);
        InspireFace.SetFaceDetectThreshold(session, 0.5f);
        InspireFace.SetFilterMinimumFacePixelSize(session, safeMinimumFacePixelSize);
        return session;
    }

    private static Session createStillImageSession(
            CustomParameter parameter, int detectPixelLevel,
            int maxDetectFaceNum, int minimumFacePixelSize) {
        int safeMaxFaces = Math.max(1, maxDetectFaceNum);
        int safeMinimumFacePixelSize = Math.max(0, minimumFacePixelSize);
        int previewSize = detectPixelLevel > 0
                ? detectPixelLevel : DetectorDefaults.INPUT_PX;
        Session session = InspireFace.CreateSession(
                parameter, InspireFace.DETECT_MODE_ALWAYS_DETECT,
                safeMaxFaces, detectPixelLevel, -1);
        if (session == null) {
            return null;
        }
        activeSessions++;
        InspireFace.SetTrackPreviewSize(session, previewSize);
        InspireFace.SetFaceDetectThreshold(session, 0.5f);
        InspireFace.SetFilterMinimumFacePixelSize(session, safeMinimumFacePixelSize);
        return session;
    }

    public static synchronized void releaseSession(Session session) {
        // 1.2.4.post1 clears the handle on close. Match its idempotent release contract
        // so repeated cleanup cannot make an unrelated live session disappear from the count.
        if (session == null || session.isClosed()) {
            return;
        }
        session.close();
        if (activeSessions > 0) {
            activeSessions--;
        }
        FaceEngine.class.notifyAll();
    }
}
