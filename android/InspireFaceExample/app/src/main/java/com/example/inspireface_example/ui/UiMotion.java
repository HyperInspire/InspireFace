package com.example.inspireface_example.ui;

import android.animation.ValueAnimator;
import android.os.Build;
import android.provider.Settings;
import android.transition.AutoTransition;
import android.transition.TransitionManager;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Interpolator;
import android.view.animation.PathInterpolator;

/** Shared timings for interface chrome; capture geometry and frame timing stay untouched. */
public final class UiMotion {
    public static final Interpolator EASE = new PathInterpolator(.2f, 0f, 0f, 1f);
    private UiMotion() { }

    public static boolean enabled(View view) {
        if (Build.VERSION.SDK_INT >= 26) return ValueAnimator.areAnimatorsEnabled();
        return Settings.Global.getFloat(view.getContext().getContentResolver(),
                Settings.Global.ANIMATOR_DURATION_SCALE, 1f) > 0f;
    }

    public static void enter(View view, long delayMs) {
        if (!enabled(view)) return;
        view.setAlpha(0f);
        view.setTranslationY(8f * view.getResources().getDisplayMetrics().density);
        view.animate().alpha(1f).translationY(0f).setStartDelay(delayMs)
                .setDuration(260).setInterpolator(EASE).start();
    }

    public static void finish(View view) {
        view.animate().cancel();
        view.setAlpha(1f);
        view.setTranslationY(0f);
    }

    public static void reveal(View view) {
        boolean wasHidden = view.getVisibility() != View.VISIBLE;
        view.setVisibility(View.VISIBLE);
        if (wasHidden && view.isAttachedToWindow()) enter(view, 0);
    }

    /** Only used inside settings cards, never on a preview or a capture ancestor. */
    public static void expand(View content, boolean expanded) {
        int visibility = expanded ? View.VISIBLE : View.GONE;
        if (content.getVisibility() == visibility) return;
        if (content.isLaidOut() && enabled(content) && content.getParent() instanceof ViewGroup) {
            ViewGroup parent = (ViewGroup) content.getParent();
            TransitionManager.endTransitions(parent);
            AutoTransition transition = new AutoTransition();
            transition.setDuration(180).setInterpolator(EASE);
            TransitionManager.beginDelayedTransition(parent, transition);
        }
        content.setVisibility(visibility);
    }
}
