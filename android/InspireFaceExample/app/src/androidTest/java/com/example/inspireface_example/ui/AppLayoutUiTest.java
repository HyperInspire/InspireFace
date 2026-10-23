package com.example.inspireface_example.ui;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import androidx.appcompat.view.ContextThemeWrapper;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import com.example.inspireface_example.R;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.File;
import java.io.FileOutputStream;
import java.util.Locale;

/** Exercises actual Android inflation, including landscape and localized long labels. */
@RunWith(AndroidJUnit4.class)
public final class AppLayoutUiTest {
    @Test public void featureLayoutsInflateInBothLanguagesOrientationsAndSystemThemes() {
        Context app = InstrumentationRegistry.getInstrumentation().getTargetContext();
        int[] layouts = {R.layout.activity_home, R.layout.activity_face_detection,
                R.layout.activity_face_attribute, R.layout.activity_face_compare,
                R.layout.activity_face_recognition, R.layout.activity_face_management,
                R.layout.activity_liveness, R.layout.activity_face_capture,
                R.layout.activity_plus_liveness, R.layout.activity_plus_consent,
                R.layout.dialog_face_editor};
        for (String language : new String[]{"en", "zh"}) {
            for (boolean landscape : new boolean[]{false, true}) {
                for (boolean night : new boolean[]{false, true}) {
                    Configuration config = new Configuration(app.getResources().getConfiguration());
                    config.setLocale(Locale.forLanguageTag(language));
                    config.orientation = landscape ? Configuration.ORIENTATION_LANDSCAPE
                            : Configuration.ORIENTATION_PORTRAIT;
                    config.uiMode = (config.uiMode & ~Configuration.UI_MODE_NIGHT_MASK)
                            | (night ? Configuration.UI_MODE_NIGHT_YES : Configuration.UI_MODE_NIGHT_NO);
                    // Compact phones expose layout problems hidden by a large unfolded display.
                    config.screenWidthDp = landscape ? 800 : 360;
                    config.screenHeightDp = landscape ? 360 : 800;
                    Context themed = new ContextThemeWrapper(app.createConfigurationContext(config),
                            R.style.Theme_InspireFaceExample);
                    for (int layout : layouts) {
                        InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
                            FrameLayout host = new FrameLayout(themed);
                            View view = LayoutInflater.from(themed).inflate(layout, host, false);
                            host.addView(view);
                            float density = themed.getResources().getDisplayMetrics().density;
                            int width = Math.round(config.screenWidthDp * density);
                            int height = Math.round(config.screenHeightDp * density);
                            host.measure(View.MeasureSpec.makeMeasureSpec(width, View.MeasureSpec.EXACTLY),
                                    View.MeasureSpec.makeMeasureSpec(height, View.MeasureSpec.EXACTLY));
                            host.layout(0, 0, width, height);
                            assertTrue(view.getMeasuredWidth() > 0);
                            if (layout == R.layout.activity_home) {
                                View left = view.findViewById(R.id.cardSilent);
                                View right = view.findViewById(R.id.cardAction);
                                assertTrue("Menu tiles must not overflow or overlap", left.getLeft() >= 0
                                        && left.getRight() <= right.getLeft()
                                        && right.getRight() <= ((View) right.getParent()).getWidth());
                                assertTrue(view.findViewById(R.id.cardSilentPlus).isClickable());
                                assertTrue(view.findViewById(R.id.cardColorPlus).isClickable());
                            }
                            if (layout == R.layout.activity_liveness || layout == R.layout.activity_face_capture) {
                                ViewGroup viewport = view.findViewById(R.id.localCameraViewport);
                                assertTrue("Camera viewport needs usable height", viewport.getHeight() > 80 * density);
                            }
                            if (layout == R.layout.activity_plus_consent) {
                                assertNotNull(view.findViewById(R.id.plusConsentFeature));
                            }
                            if (!night) savePreview(host, app, app.getResources().getResourceEntryName(layout)
                                    + "-" + language + (landscape ? "-land" : "") + ".png");
                        });
                    }
                }
            }
        }
    }

    private static void savePreview(View view, Context context, String name) {
        Bitmap bitmap = Bitmap.createBitmap(view.getWidth(), view.getHeight(), Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(bitmap);
        canvas.drawColor(android.graphics.Color.WHITE);
        view.draw(canvas);
        File directory = new File(context.getFilesDir(), "ui-checks/layouts");
        assertTrue(directory.isDirectory() || directory.mkdirs());
        try (FileOutputStream output = new FileOutputStream(new File(directory, name))) {
            bitmap.compress(Bitmap.CompressFormat.PNG, 100, output);
        } catch (Exception e) {
            throw new AssertionError(e);
        } finally {
            bitmap.recycle();
        }
    }
}
