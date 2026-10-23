package com.example.inspireface_example.view;

import android.content.Context;
import com.example.inspireface_example.ui.UiMotion;
import android.os.SystemClock;
import android.text.TextUtils;
import android.util.AttributeSet;

import androidx.appcompat.widget.AppCompatTextView;

/** Updates text immediately, with a short fade only for distinct, infrequent changes. */
public final class AnimatedLabelView extends AppCompatTextView {
    private boolean initialized, animateChanges = true;
    private long lastChange;

    public AnimatedLabelView(Context context, AttributeSet attrs) {
        super(context, attrs);
        initialized = true;
    }

    @Override public void setText(CharSequence text, BufferType type) {
        if (initialized && TextUtils.equals(getText(), text)) return;
        super.setText(text, type);
        if (!initialized) return;
        long now = SystemClock.uptimeMillis();
        boolean animate = animateChanges && isAttachedToWindow() && isShown()
                && UiMotion.enabled(this) && now - lastChange >= 240;
        lastChange = now;
        UiMotion.finish(this);
        if (animate) {
            setAlpha(.65f);
            setTranslationY(2f * getResources().getDisplayMetrics().density);
            animate().alpha(1f).translationY(0f).setStartDelay(0)
                    .setDuration(180).setInterpolator(UiMotion.EASE).start();
        }
    }

    public void setAnimateChanges(boolean enabled) {
        animateChanges = enabled;
        if (!enabled) UiMotion.finish(this);
    }

    @Override protected void onDetachedFromWindow() {
        UiMotion.finish(this);
        super.onDetachedFromWindow();
    }
}
