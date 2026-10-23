package com.example.inspireface_example.view;

import com.example.inspireface_example.ui.UiMotion;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import android.os.SystemClock;
import android.util.AttributeSet;
import android.view.View;

import com.example.inspireface_example.plus.FaceFrame;
import com.example.inspireface_example.plus.FaceGuideGeometry;

/** Circular camera aperture, breathing readiness halo and animated capture progress. */
public final class PlusFaceGuideView extends View {
    public enum Mode { ALIGN, HOLD, READY, CAPTURE, VERIFY, WARNING, SUCCESS, FAILURE }
    private static final int BACKGROUND = Color.WHITE;
    private static final int MINT = 0xff4a987e, FAR = 0xff819db5, NEAR = 0xffb69a71, NEUTRAL = 0xffa4b5ad;
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Path mask = new Path(), silhouette = new Path(), symbol = new Path();
    private final RectF ring = new RectF();
    private volatile FaceGuideGeometry geometry = new FaceGuideGeometry(0, 0);
    private Mode mode = Mode.ALIGN;
    private float targetProgress, drawnProgress, resultReveal;
    private boolean facePresent, animating;
    private int illumination = Color.TRANSPARENT;
    private long lastDraw;
    private int drawnTint = NEUTRAL;
    private FaceGuideGeometry.Feedback feedback;
    private FaceFrame.Issue faceIssue = FaceFrame.Issue.NO_FACE;

    public PlusFaceGuideView(Context context) { this(context, null); }
    public PlusFaceGuideView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setImportantForAccessibility(IMPORTANT_FOR_ACCESSIBILITY_NO);
    }

    /** Only immutable layout geometry crosses onto the analyzer thread. */
    FaceGuideGeometry.Feedback measureFace(int width, int height, float left, float top, float right, float bottom,
                                          boolean previouslyAligned) {
        return geometry.measure(width, height, left, top, right, bottom, previouslyAligned);
    }

    public void setAlignment(FaceFrame frame) {
        feedback = frame.guide; faceIssue = frame.issue;
        invalidate();
    }

    public void setGuideState(Mode next, float progress, boolean hasFace) {
        boolean adjustingAlignment = alignmentMode(mode) && alignmentMode(next);
        if ((next != mode && !adjustingAlignment) || progress < targetProgress) drawnProgress = 0;
        if (next != mode) resultReveal = 0;
        mode = next; facePresent = hasFace;
        if (!hasFace) { feedback = null; faceIssue = FaceFrame.Issue.NO_FACE; }
        targetProgress = Math.max(0, Math.min(1, progress));
        invalidate();
    }
    private static boolean alignmentMode(Mode mode) {
        return mode == Mode.ALIGN || mode == Mode.HOLD || mode == Mode.READY || mode == Mode.WARNING;
    }
    public void setIllumination(int color) { illumination = color; invalidate(); }
    public void setAnimating(boolean value) { animating = value; lastDraw = 0; invalidate(); }

    @Override protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);
        FaceGuideGeometry g = new FaceGuideGeometry(w, h);
        geometry = g;
        ring.set(g.centerX - g.radius, g.centerY - g.radius, g.centerX + g.radius, g.centerY + g.radius);
        mask.reset(); mask.setFillType(Path.FillType.EVEN_ODD);
        mask.addRect(0, 0, w, h, Path.Direction.CW);
        mask.addCircle(g.centerX, g.centerY, g.radius, Path.Direction.CW);
        silhouette.reset();
        float cx = g.centerX, cy = g.centerY, r = g.radius;
        silhouette.addOval(cx - r * .28f, cy - r * .47f, cx + r * .28f, cy + r * .22f, Path.Direction.CW);
        silhouette.moveTo(cx - r * .55f, cy + r * .65f);
        silhouette.cubicTo(cx - r * .49f, cy + r * .18f, cx + r * .49f, cy + r * .18f, cx + r * .55f, cy + r * .65f);
    }

    @Override protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        FaceGuideGeometry g = geometry;
        if (g.radius <= 0) return;
        long now = SystemClock.uptimeMillis();
        boolean motion = animating && isShown() && getWindowVisibility() == VISIBLE
                && UiMotion.enabled(this);
        float dt = lastDraw == 0 ? 16 : Math.min(100, now - lastDraw);
        lastDraw = now;
        drawnProgress = motion ? drawnProgress + (targetProgress - drawnProgress) * (1 - (float) Math.exp(-dt / 95f)) : targetProgress;
        resultReveal = motion ? Math.min(1f, resultReveal + dt / 260f) : 1f;
        float pulse = motion ? (float) (.5 + .5 * Math.sin(now * Math.PI / 1400)) : .5f;
        boolean flashing = illumination != Color.TRANSPARENT;
        int targetTint = tint();
        int nextTint = motion ? blend(drawnTint, targetTint, 1 - (float) Math.exp(-dt / 220f)) : targetTint;
        drawnTint = nextTint == drawnTint ? targetTint : nextTint;
        int color = drawnTint;
        if (flashing) color = illumination == Color.WHITE || illumination == Color.GREEN ? 0xff233247 : Color.WHITE;

        paint.setShader(null); paint.setStyle(Paint.Style.FILL);
        paint.setColor(flashing ? illumination : BACKGROUND);
        canvas.drawPath(mask, paint);

        // Two quiet halo bands keep the aperture crisp without a software blur layer.
        paint.setStyle(Paint.Style.STROKE);
        if (!flashing) for (int i = 2; i >= 1; i--) {
            paint.setStrokeWidth(dp(4 + i * 6 + pulse * 1.2f));
            paint.setColor(alpha(color, (int) (7 + pulse * 3)));
            canvas.drawCircle(g.centerX, g.centerY, g.radius, paint);
        }
        paint.setStrokeWidth(dp(1));
        for (int i = 0; i < 4; i++) {
            double angle = Math.toRadians(i * 90);
            float length = dp(4);
            float outer = g.radius * 1.10f;
            paint.setColor(alpha(color, 70));
            canvas.drawLine(g.centerX + (float) Math.cos(angle) * (outer - length),
                    g.centerY + (float) Math.sin(angle) * (outer - length),
                    g.centerX + (float) Math.cos(angle) * outer,
                    g.centerY + (float) Math.sin(angle) * outer, paint);
        }
        paint.setStrokeWidth(dp(1.5f)); paint.setColor(alpha(color, facePresent ? 100 : 65));
        canvas.drawOval(ring, paint);
        paint.setStrokeCap(Paint.Cap.ROUND); paint.setStrokeWidth(dp(2.5f));
        paint.setColor(color);
        if (mode == Mode.VERIFY) {
            canvas.drawArc(ring, motion ? (now % 1800) / 5f - 90 : -90, 95, false, paint);
            canvas.drawArc(ring, motion ? (now % 1800) / 5f + 90 : 90, 35, false, paint);
        } else if (mode == Mode.READY || mode == Mode.SUCCESS || mode == Mode.FAILURE) {
            canvas.drawOval(ring, paint);
        } else if (drawnProgress > .001f) canvas.drawArc(ring, -90, drawnProgress * 360, false, paint);
        else {
            // Quiet paired corner arcs orient the face without suggesting capture has started.
            canvas.drawArc(ring, -110, 40, false, paint);
            canvas.drawArc(ring, 70, 40, false, paint);
        }
        paint.setShader(null); paint.setStrokeCap(Paint.Cap.BUTT);
        if (!facePresent && mode != Mode.SUCCESS && mode != Mode.FAILURE && mode != Mode.VERIFY) {
            paint.setStrokeWidth(dp(1.5f)); paint.setColor(alpha(color, 105));
            canvas.drawPath(silhouette, paint);
        }
        boolean result = mode == Mode.SUCCESS || mode == Mode.FAILURE;
        if (result) drawResult(canvas, g, mode == Mode.SUCCESS, color);
        boolean continuous = !flashing && (alignmentMode(mode) || mode == Mode.VERIFY);
        boolean settling = Math.abs(targetProgress - drawnProgress) > .001f || drawnTint != targetTint
                || (result && resultReveal < 1);
        if (motion && (continuous || settling)) postInvalidateOnAnimation();
    }

    private void drawResult(Canvas canvas, FaceGuideGeometry g, boolean success, int color) {
        float radius = Math.min(dp(22), g.radius * .2f);
        float cx = g.centerX, cy = g.centerY + g.radius * .68f;
        canvas.save();
        float scale = .88f + .12f * resultReveal;
        canvas.scale(scale, scale, cx, cy);
        paint.setStyle(Paint.Style.FILL); paint.setColor(BACKGROUND);
        canvas.drawCircle(cx, cy, radius, paint);
        paint.setStyle(Paint.Style.STROKE); paint.setColor(alpha(color, Math.round(255 * resultReveal))); paint.setStrokeWidth(dp(2.5f));
        paint.setStrokeCap(Paint.Cap.ROUND);
        symbol.reset();
        if (success) {
            symbol.moveTo(cx - radius * .42f, cy); symbol.lineTo(cx - radius * .08f, cy + radius * .3f);
            symbol.lineTo(cx + radius * .45f, cy - radius * .3f);
        } else {
            symbol.moveTo(cx - radius * .3f, cy - radius * .3f); symbol.lineTo(cx + radius * .3f, cy + radius * .3f);
            symbol.moveTo(cx + radius * .3f, cy - radius * .3f); symbol.lineTo(cx - radius * .3f, cy + radius * .3f);
        }
        canvas.drawPath(symbol, paint); paint.setStrokeCap(Paint.Cap.BUTT);
        canvas.restore();
    }
    private int tint() {
        switch (mode) {
            case FAILURE: return 0xffc97474;
            case VERIFY: return 0xff648ca8;
            case READY: case SUCCESS: case CAPTURE: return MINT;
            default:
                if (!facePresent) return NEUTRAL;
                if (faceIssue == FaceFrame.Issue.POSE || faceIssue == FaceFrame.Issue.QUALITY
                        || faceIssue == FaceFrame.Issue.EYES || faceIssue == FaceFrame.Issue.MULTIPLE) return NEAR;
                if (feedback == null || faceIssue == FaceFrame.Issue.METADATA) return NEUTRAL;
                if (faceIssue == FaceFrame.Issue.TOO_FAR && feedback.issue != FaceFrame.Issue.TOO_FAR) return FAR;
                if (faceIssue == FaceFrame.Issue.TOO_CLOSE && feedback.issue != FaceFrame.Issue.TOO_CLOSE) return NEAR;
                int distanceTint = blend(MINT, feedback.distance < 0 ? FAR : NEAR, Math.abs(feedback.distance));
                return blend(distanceTint, NEAR, feedback.offset * .7f);
        }
    }
    private static int blend(int from, int to, float amount) {
        return Color.rgb(Math.round(Color.red(from) + (Color.red(to) - Color.red(from)) * amount),
                Math.round(Color.green(from) + (Color.green(to) - Color.green(from)) * amount),
                Math.round(Color.blue(from) + (Color.blue(to) - Color.blue(from)) * amount));
    }
    private static int alpha(int color, int alpha) { return (color & 0x00ffffff) | (alpha << 24); }
    private float dp(float value) { return value * getResources().getDisplayMetrics().density; }
}
