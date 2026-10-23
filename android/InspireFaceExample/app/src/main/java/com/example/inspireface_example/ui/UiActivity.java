package com.example.inspireface_example.ui;

import android.animation.AnimatorInflater;
import android.os.Bundle;
import android.os.Build;
import android.view.View;
import android.view.ViewGroup;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.view.WindowCompat;

import com.example.inspireface_example.R;

import java.util.ArrayList;
import java.util.List;

/** Common system chrome and subtle entrance/press feedback for every feature. */
public abstract class UiActivity extends AppCompatActivity {
    private final List<View> entranceViews = new ArrayList<>();

    @Override protected void onPostCreate(Bundle state) {
        super.onPostCreate(state);
        if (Build.VERSION.SDK_INT >= 29) getWindow().getDecorView().setForceDarkAllowed(false);
        WindowCompat.getInsetsController(getWindow(), getWindow().getDecorView())
                .setAppearanceLightStatusBars(true);
        WindowCompat.getInsetsController(getWindow(), getWindow().getDecorView())
                .setAppearanceLightNavigationBars(true);
        bindMotion(findViewById(android.R.id.content), state == null);
    }

    private void bindMotion(View view, boolean enter) {
        if (view.isClickable() && view.getStateListAnimator() == null) {
            view.setStateListAnimator(AnimatorInflater.loadStateListAnimator(
                    this, R.animator.plus_button_press));
        }
        if (enter && "motion_enter".equals(view.getTag())) {
            entranceViews.add(view);
            UiMotion.enter(view, Math.min(entranceViews.size() - 1, 3) * 35L);
        }
        if (view instanceof ViewGroup) {
            ViewGroup group = (ViewGroup) view;
            for (int i = 0; i < group.getChildCount(); i++) bindMotion(group.getChildAt(i), enter);
        }
    }

    @Override protected void onStop() {
        for (View view : entranceViews) UiMotion.finish(view);
        entranceViews.clear();
        super.onStop();
    }
}
