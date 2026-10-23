package com.example.inspireface_example.view;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeFalse;

import android.app.KeyguardManager;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.os.SystemClock;
import androidx.camera.view.PreviewView;
import androidx.lifecycle.Lifecycle;
import androidx.test.core.app.ActivityScenario;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.example.inspireface_example.R;
import org.junit.Test;
import org.junit.runner.RunWith;
import java.io.File;
import java.io.FileOutputStream;
import java.util.concurrent.atomic.AtomicBoolean;

/** Opens the real camera and verifies foreground recovery; never presses Start or uploads. */
@RunWith(AndroidJUnit4.class)
public final class PlusLivenessUiTest {
    @Test public void circlePageReopensCameraAfterBackgrounding() throws Exception {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        KeyguardManager keyguard = context.getSystemService(KeyguardManager.class);
        assumeFalse("Unlock the device before testing the foreground camera page", keyguard.isKeyguardLocked());
        Intent intent = new Intent(context, PlusLivenessActivity.class).putExtra(PlusLivenessActivity.EXTRA_COLOR, true);
        try (ActivityScenario<PlusLivenessActivity> scenario = ActivityScenario.launch(intent)) {
            scenario.onActivity(activity -> PlusConsentUiTest.consentDialog(activity)
                    .getButton(androidx.appcompat.app.AlertDialog.BUTTON_POSITIVE).performClick());
            awaitPreview(scenario);
            scenario.onActivity(activity -> {
                PlusFaceGuideView guide = activity.findViewById(R.id.plusGuide);
                assertTrue(guide.getWidth() > 0 && guide.getHeight() > 0);
                assertNotNull(activity.findViewById(R.id.plusGuideHint));
            });
            saveScreenshot(context, "plus-circle-portrait.png");
            scenario.moveToState(Lifecycle.State.CREATED);
            scenario.moveToState(Lifecycle.State.RESUMED);
            awaitPreview(scenario);
            saveScreenshot(context, "plus-circle-resumed.png");
        }
    }

    private void awaitPreview(ActivityScenario<PlusLivenessActivity> scenario) {
        AtomicBoolean streaming = new AtomicBoolean();
        long deadline = SystemClock.elapsedRealtime() + 15000;
        do {
            scenario.onActivity(activity -> {
                PreviewView preview = activity.findViewById(R.id.plusPreview);
                streaming.set(preview.getPreviewStreamState().getValue() == PreviewView.StreamState.STREAMING);
            });
            if (streaming.get()) return;
            SystemClock.sleep(100);
        } while (SystemClock.elapsedRealtime() < deadline);
        assertTrue("Front camera should stream after resume", streaming.get());
    }

    private void saveScreenshot(Context context, String name) throws Exception {
        Bitmap screenshot = InstrumentationRegistry.getInstrumentation().getUiAutomation().takeScreenshot();
        assertNotNull(screenshot);
        File directory = new File(context.getFilesDir(), "ui-checks");
        assertTrue(directory.isDirectory() || directory.mkdirs());
        try (FileOutputStream output = new FileOutputStream(new File(directory, name))) {
            assertTrue(screenshot.compress(Bitmap.CompressFormat.PNG, 100, output));
        } finally { screenshot.recycle(); }
    }
}
