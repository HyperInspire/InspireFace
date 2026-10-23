package com.example.inspireface_example.view;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNotSame;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import com.example.inspireface_example.face.FaceAttributeProcessor;
import com.example.inspireface_example.plus.EyeCrop320;
import com.example.inspireface_example.plus.RgbPixels;
import com.insightface.sdk.inspireface.FaceCapture;
import com.insightface.sdk.inspireface.FaceDetectionSnapshot;
import com.insightface.sdk.inspireface.InspireFace;
import com.insightface.sdk.inspireface.base.CustomParameter;
import com.insightface.sdk.inspireface.base.FaceCaptureConfig;
import com.insightface.sdk.inspireface.base.FaceCaptureProgress;
import com.insightface.sdk.inspireface.base.FaceCaptureResult;
import com.insightface.sdk.inspireface.base.FaceFeature;
import com.insightface.sdk.inspireface.base.ImageStream;
import com.insightface.sdk.inspireface.base.InspireFaceVersion;
import com.insightface.sdk.inspireface.base.MultipleFaceData;
import com.insightface.sdk.inspireface.base.Point2f;
import com.insightface.sdk.inspireface.base.Session;
import com.insightface.sdk.inspireface.jni.Native;
import com.insightface.sdk.inspireface.jni.NativeConstants;
import com.insightface.sdk.inspireface.jni.NativeTypes.HFImageBitmapData;
import com.insightface.sdk.inspireface.jni.NativeTypes.HFImageData;
import com.insightface.sdk.inspireface.jni.NativeTypes.HFInspireFaceVersion;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.InputStream;
import java.lang.reflect.Field;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/** Exercises the complete 1.2.4.post1 AAR, its portable JNI, and both bundled model packs. */
@RunWith(AndroidJUnit4.class)
public final class Sdk124SmokeTest {
    private final Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    @Before public void prepareRuntime() throws Exception { SdkTestRuntime.resetWhenIdle(); }
    @After public void releaseRuntime() throws Exception { SdkTestRuntime.resetWhenIdle(); }

    @Test
    public void packagedFacadeAndPortableJniUse124ApiLevel2() {
        InspireFaceVersion version = InspireFace.QueryInspireFaceVersion();
        assertNotNull(version);
        assertEquals(1, version.major);
        assertEquals(2, version.minor);
        assertEquals(4, version.patch);
        // post1 is the Android package revision; the native core still reports 1.2.4.
        HFInspireFaceVersion portableVersion = new HFInspireFaceVersion();
        assertNativeSuccess(Native.HFQueryInspireFaceVersion(portableVersion));
        assertEquals(version.major, portableVersion.major);
        assertEquals(version.minor, portableVersion.minor);
        assertEquals(version.patch, portableVersion.patch);
        int[] apiLevel = new int[1];
        assertNativeSuccess(Native.HFQueryCAPILevel(apiLevel));
        assertEquals(2, apiLevel[0]);
        assertEquals(apiLevel[0], InspireFace.QueryCAPILevel());
        assertTrue(InspireFace.QueryInspireFaceComponentVersions().length() > 0);
        assertTrue(InspireFace.QueryInspireFaceDiagnosticInformation().length() > 0);
    }

    @Test
    public void bundledModelsSupportAppSessionsAndPipelines() throws Exception {
        Bitmap bitmap = loadSampleFace();
        try {
            for (String model : new String[]{"Pikachu", "Megatron"}) {
                assertTrue("Launch " + model, InspireFace.GlobalLaunch(context, model));
                try {
                    checkSession(FaceEngine.createDetectionSession(320, 3, 0), bitmap, false, false);
                    checkSession(FaceEngine.createFaceTrackingSession(
                            InspireFace.DETECT_MODE_LIGHT_TRACK, 320, 3, 0), bitmap, false, false);
                    checkSession(FaceEngine.createFaceTrackingSession(
                            InspireFace.DETECT_MODE_TRACK_BY_DETECTION, 320, 3, 0), bitmap, false, false);
                    checkSession(FaceEngine.createRecognitionSession(), bitmap, true, false);
                    checkSession(FaceEngine.createVideoRecognitionSession(), bitmap, true, false);
                    checkSession(FaceEngine.createPreviewSession(), bitmap, false, true);
                    checkPlusPreprocessing(bitmap);

                    Session attributes = FaceEngine.createAttributeSession();
                    assertNotNull("Attribute session", attributes);
                    try {
                        FaceAttributeProcessor.Result result = FaceAttributeProcessor.analyze(attributes, bitmap);
                        assertEquals(FaceAttributeProcessor.Status.READY, result.status);
                        assertTrue(result.attributes.length > 0);
                    } finally {
                        FaceEngine.releaseSession(attributes);
                    }
                } finally {
                    assertTrue(InspireFace.GlobalTerminate());
                }
            }
        } finally {
            bitmap.recycle();
        }
    }

    @Test
    public void captureAndSnapshotExtensionsMatchNativeLibrary() throws Exception {
        assertTrue(InspireFace.GlobalLaunch(context, "Pikachu"));
        try {
            Bitmap bitmap = loadSampleFace();
            try {
                Session session = FaceEngine.createPreviewSession();
                assertNotNull(session);
                try {
                    ImageStream stream = InspireFace.CreateImageStreamFromBitmap(bitmap, InspireFace.CAMERA_ROTATION_0);
                    assertNotNull(stream);
                    try {
                        FaceCaptureConfig config = FaceCapture.defaultConfig();
                        // Exercise capture ownership deterministically, without pose/quality policy.
                        config.filterMask = FaceCapture.FILTER_NONE;
                        config.minTrackCount = 0;
                        config.stableDurationMs = 0;
                        config.collectDurationMs = 0;
                        config.minCandidateIntervalMs = 0;
                        try (FaceCapture capture = FaceCapture.create(session, config);
                             FaceDetectionSnapshot snapshot = FaceDetectionSnapshot.create(session, stream)) {
                            MultipleFaceData faces = snapshot.getFaces();
                            assertFaceArrays(faces);
                            MultipleFaceData copied = snapshot.getFaces();
                            assertNotSame(faces.tokens[0].data, copied.tokens[0].data);
                            assertNotSame(faces.trackCounts, copied.trackCounts);
                            assertArrayEquals(faces.tokens[0].data, copied.tokens[0].data);
                            assertArrayEquals(faces.trackCounts, copied.trackCounts);
                            FaceCaptureProgress progress = capture.update(stream, snapshot, 1L, 1L);
                            assertEquals(1L, progress.frameId);
                            assertEquals(1L, progress.timestampMs);
                            assertEquals(FaceCapture.REJECT_NONE, progress.rejectReasons);
                            assertEquals(FaceCapture.STATE_FINISHED, capture.finish().state);
                            FaceCaptureResult[] results = capture.getResults();
                            assertEquals(1, results.length);
                            assertEquals(1L, results[0].frameId);
                            assertTrue(results[0].token.length > 0);
                            byte[] ownedToken = results[0].token.clone();
                            capture.reset();
                            assertEquals(0, capture.getResults().length);
                            assertArrayEquals(ownedToken, results[0].token);
                            snapshot.close();
                            snapshot.close();
                            assertTrue(snapshot.isClosed());
                            assertThrows(IllegalStateException.class, snapshot::getFaces);
                            assertEquals(106, InspireFace.GetFaceDenseLandmarkFromFaceToken(
                                    faces.tokens[0]).length);
                            capture.update(stream, 2L, 2L);
                            capture.finish();
                            assertEquals(1, capture.getResults().length);
                            assertEquals(2L, capture.getResults()[0].frameId);
                            capture.close();
                            capture.close();
                            assertTrue(capture.isClosed());
                            assertThrows(IllegalStateException.class, capture::getResults);
                        }
                    } finally {
                        InspireFace.ReleaseImageStream(stream);
                    }
                } finally {
                    FaceEngine.releaseSession(session);
                }
            } finally {
                bitmap.recycle();
            }
        } finally {
            assertTrue(InspireFace.GlobalTerminate());
        }
    }

    @Test
    public void portableJniRespectsBufferPositionAndOwnedImageLifetime() {
        HFImageData image = new HFImageData();
        image.width = 4;
        image.height = 4;
        image.format = NativeConstants.HF_STREAM_RGB;
        image.data = ByteBuffer.allocateDirect(8 + 4 * 4 * 3).order(ByteOrder.nativeOrder());
        image.data.position(8);
        for (int i = 0; i < 16; i++) image.data.put(new byte[]{(byte) 255, 0, 0});
        image.data.position(8);
        long[] stream = new long[1];
        long[] bitmap = new long[1];
        long[] ownedStream = new long[1];
        long[] decoded = new long[1];
        try {
            assertNativeSuccess(Native.HFCreateImageStream(image, stream));
            assertNativeSuccess(Native.HFCreateImageBitmapFromImageStreamProcess(
                    stream[0], bitmap, 0, 1f));
            assertNativeSuccess(Native.HFCreateImageStreamFromImageBitmap(bitmap[0], 0, ownedStream));
            assertNativeSuccess(Native.HFReleaseImageBitmap(bitmap[0]));
            bitmap[0] = 0L;
            assertNativeSuccess(Native.HFReleaseImageStream(stream[0]));
            stream[0] = 0L;
            assertNativeSuccess(Native.HFCreateImageBitmapFromImageStreamProcess(
                    ownedStream[0], decoded, 0, 1f));
            HFImageBitmapData pixels = new HFImageBitmapData();
            assertNativeSuccess(Native.HFImageBitmapGetData(decoded[0], pixels));
            assertEquals(4, pixels.width);
            assertEquals(4, pixels.height);
            assertEquals(3, pixels.channels);
            assertTrue(pixels.data.isDirect());
            // Decoded native image bitmaps use BGR, independent of input RGB layout.
            assertEquals(0, pixels.data.get(0) & 255);
            assertEquals(0, pixels.data.get(1) & 255);
            assertEquals(255, pixels.data.get(2) & 255);
        } finally {
            if (decoded[0] != 0L) assertNativeSuccess(Native.HFReleaseImageBitmap(decoded[0]));
            if (ownedStream[0] != 0L) assertNativeSuccess(Native.HFReleaseImageStream(ownedStream[0]));
            if (bitmap[0] != 0L) assertNativeSuccess(Native.HFReleaseImageBitmap(bitmap[0]));
            if (stream[0] != 0L) assertNativeSuccess(Native.HFReleaseImageStream(stream[0]));
        }
    }

    @Test
    public void repeatedSessionReleasePreservesOtherLiveSessions() throws Exception {
        assertTrue(InspireFace.GlobalLaunch(context, "Pikachu"));
        Session first = null;
        Session second = null;
        int appCountBefore = activeSessionCount();
        int nativeCountBefore = nativeSessionCount();
        try {
            first = FaceEngine.createTrackingSession();
            second = FaceEngine.createTrackingSession();
            assertNotNull(first);
            assertNotNull(second);
            assertEquals(appCountBefore + 2, activeSessionCount());
            assertEquals(nativeCountBefore + 2, nativeSessionCount());
            FaceEngine.releaseSession(null);
            assertEquals(appCountBefore + 2, activeSessionCount());
            FaceEngine.releaseSession(first);
            assertTrue(first.isClosed());
            FaceEngine.releaseSession(first);
            assertEquals("Double cleanup must still block switching an active model",
                    appCountBefore + 1, activeSessionCount());
            assertEquals(nativeCountBefore + 1, nativeSessionCount());
            assertTrue("The other session remains open", second.handle != 0L);
        } finally {
            try {
                FaceEngine.releaseSession(first);
                FaceEngine.releaseSession(second);
                assertEquals(appCountBefore, activeSessionCount());
                assertEquals(nativeCountBefore, nativeSessionCount());
            } finally {
                assertTrue(InspireFace.GlobalTerminate());
            }
        }
    }

    private Bitmap loadSampleFace() throws Exception {
        try (InputStream input = context.getAssets().open("inspireface/kun.jpg")) {
            BitmapFactory.Options options = new BitmapFactory.Options();
            options.inPreferredConfig = Bitmap.Config.ARGB_8888;
            Bitmap bitmap = BitmapFactory.decodeStream(input, null, options);
            assertNotNull("Bundled SDK sample image", bitmap);
            return bitmap;
        }
    }

    /** Uses only the bundled image locally; this test never contacts the PLUS service. */
    private void checkPlusPreprocessing(Bitmap bitmap) {
        Session session = FaceEngine.createPlusSession();
        assertNotNull(session);
        try {
            ImageStream stream = InspireFace.CreateImageStreamFromBitmap(bitmap, InspireFace.CAMERA_ROTATION_0);
            assertNotNull(stream);
            try {
                MultipleFaceData faces = InspireFace.ExecuteFaceTrack(session, stream);
                assertFaceArrays(faces);
                Point2f[] points = InspireFace.GetFaceDenseLandmarkFromFaceToken(faces.tokens[0]);
                assertEquals(106, points.length);
                Point2f a = points[67], b = points[68];
                if (a.x > b.x) { Point2f swap = a; a = b; b = swap; }
                assertTrue(a.x >= 0 && b.x < bitmap.getWidth() && b.x - a.x >= 2);
                assertTrue(a.y >= 0 && a.y < bitmap.getHeight() && b.y >= 0 && b.y < bitmap.getHeight());
                assertTrue(InspireFace.MultipleFacePipelineProcess(session, stream, faces,
                        InspireFace.CreateCustomParameter().enableFaceQuality(true)));
                assertTrue(InspireFace.GetFaceQualityConfidence(session).num > 0);
                int[] pixels = new int[bitmap.getWidth() * bitmap.getHeight()];
                bitmap.getPixels(pixels, 0, bitmap.getWidth(), 0, 0, bitmap.getWidth(), bitmap.getHeight());
                assertEquals(307200, RgbPixels.fromArgb(EyeCrop320.crop(pixels,
                        bitmap.getWidth(), bitmap.getHeight(), a.x, a.y, b.x, b.y)).length);
                MultipleFaceData next = InspireFace.ExecuteFaceTrack(session, stream);
                assertFaceArrays(next);
                assertEquals("A steady face must keep its identity", faces.trackIds[0], next.trackIds[0]);
                assertTrue("Tracking count must advance", next.trackCounts[0] > faces.trackCounts[0]);
            } finally { InspireFace.ReleaseImageStream(stream); }
            Bitmap blank = Bitmap.createBitmap(bitmap.getWidth(), bitmap.getHeight(), Bitmap.Config.ARGB_8888);
            try {
                ImageStream empty = InspireFace.CreateImageStreamFromBitmap(blank, InspireFace.CAMERA_ROTATION_0);
                assertNotNull(empty);
                try {
                    MultipleFaceData lost = InspireFace.ExecuteFaceTrack(session, empty);
                    assertNotNull(lost);
                    assertEquals("A missing face must be reported on the next frame", 0, lost.detectedNum);
                } finally { InspireFace.ReleaseImageStream(empty); }
            } finally { blank.recycle(); }
        } finally { FaceEngine.releaseSession(session); }
    }

    private void checkSession(Session session, Bitmap bitmap, boolean recognition, boolean liveness) {
        assertNotNull("CreateSession JNI and model compatibility", session);
        try {
            ImageStream stream = InspireFace.CreateImageStreamFromBitmap(bitmap, InspireFace.CAMERA_ROTATION_0);
            assertNotNull(stream);
            try {
                MultipleFaceData faces = InspireFace.ExecuteFaceTrack(session, stream);
                assertNotNull(faces);
                assertTrue("Detect the bundled sample face", faces.detectedNum > 0);
                assertFaceArrays(faces);
                assertNotNull("Owned token payload from JNI", faces.tokens[0].data);
                assertTrue(faces.tokens[0].size > 0);
                assertEquals(faces.tokens[0].size, faces.tokens[0].data.length);
                Point2f[] landmarks = InspireFace.GetFaceDenseLandmarkFromFaceToken(faces.tokens[0]);
                assertNotNull(landmarks);
                assertEquals(106, landmarks.length);
                if (recognition) {
                    FaceFeature feature = InspireFace.ExtractFaceFeature(session, stream, faces.tokens[0]);
                    assertNotNull(feature);
                    assertEquals(InspireFace.GetFeatureLength(), feature.data.length);
                    assertTrue(InspireFace.FaceComparison(feature, feature) > 0.99f);
                }
                if (liveness) {
                    CustomParameter pipeline = InspireFace.CreateCustomParameter()
                            .enableLiveness(true).enableFaceQuality(true).enableInteractionLiveness(true);
                    assertTrue("Pipeline JNI parameter layout", InspireFace.MultipleFacePipelineProcess(
                            session, stream, faces, pipeline));
                    assertNotNull(InspireFace.GetRGBLivenessConfidence(session));
                    assertNotNull(InspireFace.GetFaceInteractionActionsResult(session));
                }
                byte[] token = faces.tokens[0].data.clone();
                int[] trackCounts = faces.trackCounts.clone();
                assertFaceArrays(InspireFace.ExecuteFaceTrack(session, stream));
                assertArrayEquals("Facade tokens survive later tracking calls", token, faces.tokens[0].data);
                assertArrayEquals("Facade counts are independent copies", trackCounts, faces.trackCounts);
            } finally {
                InspireFace.ReleaseImageStream(stream);
            }
        } finally {
            FaceEngine.releaseSession(session);
        }
    }

    private static void assertFaceArrays(MultipleFaceData faces) {
        assertNotNull(faces);
        assertTrue("Detect the bundled sample face", faces.detectedNum > 0);
        assertEquals(faces.detectedNum, faces.rects.length);
        assertEquals(faces.detectedNum, faces.trackIds.length);
        assertEquals(faces.detectedNum, faces.trackCounts.length);
        assertEquals(faces.detectedNum, faces.tokens.length);
    }

    private static void assertNativeSuccess(long status) {
        assertEquals("Portable JNI status", NativeConstants.HSUCCEED, status);
    }

    private static int activeSessionCount() throws Exception {
        // Check the app's model-switch guard without changing persistent model selection.
        Field field = FaceEngine.class.getDeclaredField("activeSessions");
        field.setAccessible(true);
        return field.getInt(null);
    }

    private static int nativeSessionCount() {
        int[] count = new int[1];
        assertNativeSuccess(Native.HFDeBugGetUnreleasedSessionsCount(count));
        return count[0];
    }
}
