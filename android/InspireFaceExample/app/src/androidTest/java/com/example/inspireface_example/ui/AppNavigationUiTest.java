package com.example.inspireface_example.ui;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeFalse;

import android.app.Activity;
import android.app.KeyguardManager;
import android.content.Context;
import android.graphics.Bitmap;
import android.os.SystemClock;
import android.view.View;
import android.view.WindowManager;

import androidx.test.core.app.ActivityScenario;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import androidx.test.runner.lifecycle.ActivityLifecycleMonitorRegistry;
import androidx.test.runner.lifecycle.Stage;

import com.example.inspireface_example.HomeActivity;
import com.example.inspireface_example.R;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.File;
import java.io.FileOutputStream;
import java.util.concurrent.atomic.AtomicReference;

/** Real launcher routes and back navigation; no enrollment, capture submission or cloud calls. */
@RunWith(AndroidJUnit4.class)
public final class AppNavigationUiTest {
    @Test public void localFeaturesOpenFromHomeAndReturn() throws Exception {
        Context app = InstrumentationRegistry.getInstrumentation().getTargetContext();
        assumeFalse("Unlock the device for navigation checks", app.getSystemService(KeyguardManager.class).isKeyguardLocked());
        int[] entries = {R.id.cardCompare, R.id.cardAttribute, R.id.cardDetection,
                R.id.cardRecognition, R.id.cardManagement, R.id.cardSilent,
                R.id.cardAction, R.id.cardPose};
        String[] screens = {"FaceCompareActivity", "FaceAttributeActivity", "FaceDetectionActivity",
                "FaceRecognitionActivity", "FaceManagementActivity", "LivenessActivity",
                "ActionLivenessActivity", "PoseActivity"};
        try (ActivityScenario<HomeActivity> home = ActivityScenario.launch(HomeActivity.class)) {
            home.onActivity(a -> a.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON));
            SystemClock.sleep(350);
            screenshot(app, "home");
            for (int i = 0; i < entries.length; i++) {
                int entry = entries[i];
                home.onActivity(a -> assertTrue(a.findViewById(entry).performClick()));
                Activity feature = awaitActivity(screens[i]);
                InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
                    feature.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
                    assertNotNull(feature.findViewById(R.id.btnBack));
                    View settings = feature.findViewById(R.id.sessionSettingsHeader);
                    if (settings != null) settings.performClick();
                });
                SystemClock.sleep(700);
                screenshot(app, screens[i]);
                InstrumentationRegistry.getInstrumentation().runOnMainSync(() ->
                        assertTrue(feature.findViewById(R.id.btnBack).performClick()));
                awaitActivity("HomeActivity");
            }
        }
    }

    private static Activity awaitActivity(String simpleName) {
        AtomicReference<Activity> current = new AtomicReference<>();
        long deadline = SystemClock.elapsedRealtime() + 10000;
        while (SystemClock.elapsedRealtime() < deadline) {
            InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
                for (Activity a : ActivityLifecycleMonitorRegistry.getInstance().getActivitiesInStage(Stage.RESUMED)) {
                    if (a.getClass().getSimpleName().equals(simpleName)) current.set(a);
                }
            });
            if (current.get() != null) return current.get();
            SystemClock.sleep(50);
        }
        throw new AssertionError("Page did not resume: " + simpleName);
    }

    private static void screenshot(Context context, String name) throws Exception {
        Bitmap bitmap = InstrumentationRegistry.getInstrumentation().getUiAutomation().takeScreenshot();
        assertNotNull(bitmap);
        File directory = new File(context.getFilesDir(), "ui-checks/navigation");
        assertTrue(directory.isDirectory() || directory.mkdirs());
        try (FileOutputStream output = new FileOutputStream(new File(directory, name + ".png"))) {
            assertTrue(bitmap.compress(Bitmap.CompressFormat.PNG, 100, output));
        } finally { bitmap.recycle(); }
    }
}
