package com.insightface.sdk.inspireface;

import android.content.Context;
import android.graphics.Bitmap;
import android.util.Log;

import androidx.test.platform.app.InstrumentationRegistry;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;

import static org.junit.Assert.*;

import com.insightface.sdk.inspireface.base.CustomParameter;
import com.insightface.sdk.inspireface.base.ImageStream;
import com.insightface.sdk.inspireface.base.MultipleFaceData;
import com.insightface.sdk.inspireface.base.Session;
import com.insightface.sdk.inspireface.utils.SDKUtils;

import java.io.File;

/**
 * Instrumented test, which will execute on an Android device.
 *
 * @see <a href="http://d.android.com/tools/testing">Testing documentation</a>
 */
@RunWith(AndroidJUnit4.class)
public class ExampleInstrumentedTest {

    private Context context;
    private static final String TAG = "InspireFace-Test";
    private static String inspireFaceFolder;

    @BeforeClass
    public static void setUpClass() {
        Log.d(TAG, "Ready to start testing...");
        Context ctx = InstrumentationRegistry.getInstrumentation().getTargetContext();
        inspireFaceFolder = InspireFace.copyResourceFileToApplicationDir(ctx);
        assertTrue(new File(inspireFaceFolder).exists());
    }

    @Before
    public void setUp() {
        // Context of the app under test.
        context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        assertEquals("com.insightface.sdk.inspireface.test", context.getPackageName());
        String modelPath = inspireFaceFolder + "/Pikachu";
        assertTrue(new File(modelPath).exists());
        boolean succeed = InspireFace.GlobalLaunch(modelPath);
        assertTrue(succeed);
    }

    @Test
    public void testBasicFunction() {
        assertTrue("Basic test failed", true);
    }

    @Test
    public void sessionBasic() {
        // Create Session
        Session session = InspireFace.CreateSession(new CustomParameter(), InspireFace.DETECT_MODE_ALWAYS_DETECT, 10, 320, -1);
        assertNotEquals(session, null);
        InspireFace.ReleaseSession(session);
    }

    @Test
    public void imageStreamBasic() {
        // Prepare the bitmap image
        Bitmap image = SDKUtils.getImageFromAssetsFile(context, "inspireface/kun.jpg");
        assertNotNull(image);
        // Using an invalid bitmap
        ImageStream stream = InspireFace.CreateImageStreamFromBitmap(null, InspireFace.CAMERA_ROTATION_0);
        assertNull(stream);
        // Use a valid bitmap
        stream = InspireFace.CreateImageStreamFromBitmap(image, InspireFace.CAMERA_ROTATION_0);
        assertNotNull(stream);
        InspireFace.ReleaseImageStream(stream);
    }

    @Test
    public void faceTrack() {
        // Prepare the bitmap image
        Bitmap image = SDKUtils.getImageFromAssetsFile(context, "inspireface/kun.jpg");
        assertNotNull(image);
        ImageStream stream = InspireFace.CreateImageStreamFromBitmap(image, InspireFace.CAMERA_ROTATION_0);
        assertNotNull(stream);

        Session session = InspireFace.CreateSession(new CustomParameter(), InspireFace.DETECT_MODE_ALWAYS_DETECT, 10, 320, -1);

        MultipleFaceData results = InspireFace.ExecuteFaceTrack(session, stream);
        assertNotNull(results);

        assertTrue(results.detectedNum > 0);
        assertEquals(-1, results.trackIds[0]);
        assertTrue(0 < results.detConfidence[0] && results.detConfidence[0] < 1);
        assertTrue(results.angles[0].roll >= -180 && results.angles[0].roll <= 180);
        assertTrue(results.angles[0].yaw >= -180 && results.angles[0].yaw <= 180);
        assertTrue(results.angles[0].pitch >= -180 && results.angles[0].pitch <= 180);

        InspireFace.ReleaseSession(session);
        InspireFace.ReleaseImageStream(stream);
    }

    @Test
    public void faceFeature() {

    }

    @After
    public void tearDown() {
        boolean succeed = InspireFace.GlobalTerminate();
        assertTrue(succeed);
    }



}