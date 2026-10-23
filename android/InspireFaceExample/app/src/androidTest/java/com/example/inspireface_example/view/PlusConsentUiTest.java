package com.example.inspireface_example.view;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeFalse;
import static org.junit.Assume.assumeTrue;

import android.Manifest;
import android.app.KeyguardManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.os.SystemClock;
import android.widget.TextView;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatDelegate;
import androidx.core.content.ContextCompat;
import androidx.core.os.LocaleListCompat;
import androidx.lifecycle.Lifecycle;
import androidx.test.core.app.ActivityScenario;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import com.example.inspireface_example.R;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.File;
import java.io.FileOutputStream;
import java.lang.reflect.Field;

/** Exercises the consent dialog, including already-granted Android camera permission. */
@RunWith(AndroidJUnit4.class)
public final class PlusConsentUiTest {
    private final Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();

    @Before public void requireUnlockedDevice() {
        assumeFalse("Unlock the device to test the consent page",
                context.getSystemService(KeyguardManager.class).isKeyguardLocked());
    }

    @Test public void bothLanguagesAndModesRequireConsentAfterRecreation() throws Exception {
        LocaleListCompat original = AppCompatDelegate.getApplicationLocales();
        try {
            for (String language : new String[]{"en", "zh"}) {
                setLocales(LocaleListCompat.forLanguageTags(language));
                for (boolean color : new boolean[]{false, true}) {
                    try (ActivityScenario<PlusLivenessActivity> scenario = launch(color)) {
                        scenario.onActivity(activity -> {
                            assertEquals(language, activity.getResources().getConfiguration().getLocales().get(0).getLanguage());
                            TextView feature = activity.findViewById(R.id.plusConsentFeature);
                            assertEquals(activity.getString(color ? R.string.home_color_plus_title
                                    : R.string.home_silent_plus_title), feature.getText().toString());
                            assertConsentOnly(activity);
                        });
                        scenario.recreate();
                        scenario.onActivity(PlusConsentUiTest::assertConsentOnly);
                        if (color) saveScreenshot("plus-consent-" + language + ".png");
                        scenario.onActivity(activity -> consentDialog(activity)
                                .getButton(AlertDialog.BUTTON_NEGATIVE).performClick());
                        awaitDestroyed(scenario);
                    }
                }
            }
        } finally {
            setLocales(original);
        }
    }

    @Test public void consentIsRequiredAgainAfterLeavingEvenWhenCameraPermissionExists() {
        assumeTrue("Grant Android camera permission before testing the accepted page",
                ContextCompat.checkSelfPermission(context, Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED);
        for (boolean color : new boolean[]{false, true}) {
            try (ActivityScenario<PlusLivenessActivity> scenario = launch(color)) {
                scenario.onActivity(activity -> {
                    assertConsentOnly(activity);
                    consentDialog(activity).getButton(AlertDialog.BUTTON_POSITIVE).performClick();
                });
                // AlertDialog dispatches button callbacks through its main-thread Handler.
                InstrumentationRegistry.getInstrumentation().waitForIdleSync();
                scenario.onActivity(activity -> {
                    assertNotNull(activity.findViewById(R.id.plusPreview));
                    assertNull(activity.findViewById(R.id.plusConsentRoot));
                });
                scenario.recreate();
                scenario.onActivity(activity -> {
                    assertNotNull(activity.findViewById(R.id.plusPreview));
                    assertNull(activity.findViewById(R.id.plusConsentRoot));
                    activity.findViewById(R.id.plusBack).performClick();
                });
                awaitDestroyed(scenario);
            }
            try (ActivityScenario<PlusLivenessActivity> scenario = launch(color)) {
                scenario.onActivity(activity -> {
                    assertConsentOnly(activity);
                    consentDialog(activity).cancel();
                });
                awaitDestroyed(scenario);
            }
        }
    }

    private static void awaitDestroyed(ActivityScenario<?> scenario) {
        long deadline = SystemClock.elapsedRealtime() + 5000;
        while (scenario.getState() != Lifecycle.State.DESTROYED
                && SystemClock.elapsedRealtime() < deadline) SystemClock.sleep(50);
        assertEquals(Lifecycle.State.DESTROYED, scenario.getState());
    }

    private ActivityScenario<PlusLivenessActivity> launch(boolean color) {
        return ActivityScenario.launch(new Intent(context, PlusLivenessActivity.class)
                .putExtra(PlusLivenessActivity.EXTRA_COLOR, color));
    }

    private static void assertConsentOnly(PlusLivenessActivity activity) {
        assertNotNull(activity.findViewById(R.id.plusConsentRoot));
        AlertDialog dialog = consentDialog(activity);
        assertTrue(dialog.isShowing());
        assertTrue(dialog.getButton(AlertDialog.BUTTON_POSITIVE).isEnabled());
        assertEquals(activity.getString(R.string.plus_consent_agree),
                dialog.getButton(AlertDialog.BUTTON_POSITIVE).getText().toString());
        assertEquals(activity.getString(R.string.plus_consent_decline),
                dialog.getButton(AlertDialog.BUTTON_NEGATIVE).getText().toString());
        String message = ((TextView) dialog.findViewById(android.R.id.message)).getText().toString();
        assertTrue(message.contains("InspireFacePlus"));
        assertTrue("Developer addresses must not appear in consent", !message.contains("http")
                && !message.contains("api.inspirehub.cc") && !message.contains("/docs"));
        assertNull(activity.findViewById(R.id.plusPreview));
        assertNull(activity.findViewById(R.id.plusStart));
        // Check resources as well as the visible page: permission alone must not start capture.
        for (String name : new String[]{"camera", "analyzer", "frozenRequest"}) {
            try {
                Field field = PlusLivenessActivity.class.getDeclaredField(name);
                field.setAccessible(true);
                assertNull("Consent must precede " + name, field.get(activity));
            } catch (ReflectiveOperationException e) {
                throw new AssertionError(e);
            }
        }
    }

    static AlertDialog consentDialog(PlusLivenessActivity activity) {
        try {
            Field field = PlusLivenessActivity.class.getDeclaredField("consentDialog");
            field.setAccessible(true);
            AlertDialog dialog = (AlertDialog) field.get(activity);
            assertNotNull(dialog);
            return dialog;
        } catch (ReflectiveOperationException e) {
            throw new AssertionError(e);
        }
    }

    private void setLocales(LocaleListCompat locales) {
        InstrumentationRegistry.getInstrumentation().runOnMainSync(
                () -> AppCompatDelegate.setApplicationLocales(locales));
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
    }

    private void saveScreenshot(String name) throws Exception {
        Bitmap bitmap = InstrumentationRegistry.getInstrumentation().getUiAutomation().takeScreenshot();
        assertNotNull(bitmap);
        File directory = new File(context.getFilesDir(), "ui-checks");
        assertTrue(directory.isDirectory() || directory.mkdirs());
        try (FileOutputStream output = new FileOutputStream(new File(directory, name))) {
            assertTrue(bitmap.compress(Bitmap.CompressFormat.PNG, 100, output));
        } finally {
            bitmap.recycle();
        }
    }
}
