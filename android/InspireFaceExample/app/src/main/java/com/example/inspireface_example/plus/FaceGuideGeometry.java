package com.example.inspireface_example.plus;

/** Same circle and FILL_CENTER transform used by the view and by the capture gate. */
public final class FaceGuideGeometry {
    /** Signed distance: negative is too far, positive is too close; zero is comfortable. */
    public static final class Feedback {
        public final FaceFrame.Issue issue;
        public final float distance, offset;
        private Feedback(FaceFrame.Issue issue, float distance, float offset) {
            this.issue = issue; this.distance = distance; this.offset = offset;
        }
    }
    public final float centerX, centerY, radius;
    private final int viewWidth, viewHeight;

    public FaceGuideGeometry(int width, int height) {
        viewWidth = width; viewHeight = height;
        centerX = width * .5f; centerY = height * .5f;
        radius = Math.min(width, height) * .405f;
    }

    public FaceFrame.Issue evaluate(int sourceWidth, int sourceHeight,
                                    float left, float top, float right, float bottom) {
        return measure(sourceWidth, sourceHeight, left, top, right, bottom, false).issue;
    }

    public Feedback measure(int sourceWidth, int sourceHeight,
                            float left, float top, float right, float bottom, boolean previouslyAligned) {
        if (radius <= 0 || sourceWidth <= 0 || sourceHeight <= 0)
            return new Feedback(FaceFrame.Issue.METADATA, 0, 0);
        if (!Float.isFinite(left) || !Float.isFinite(top) || !Float.isFinite(right) || !Float.isFinite(bottom)
                || right <= left || bottom <= top) return new Feedback(FaceFrame.Issue.EYES, 0, 0);
        float scale = Math.max(viewWidth / (float) sourceWidth, viewHeight / (float) sourceHeight);
        float dx = (viewWidth - sourceWidth * scale) * .5f;
        float dy = (viewHeight - sourceHeight * scale) * .5f;
        // Front-preview mirroring changes the sign of x about the circle center only.
        float l = left * scale + dx - centerX, r = right * scale + dx - centerX;
        float t = top * scale + dy - centerY, b = bottom * scale + dy - centerY;
        float w = (r - l) / radius, h = (b - t) / radius;
        float cx = (l + r) * .5f / radius, cy = (t + b) * .5f / radius;
        float centerDistance = (float) Math.hypot(cx, cy);
        float far = clamp(Math.max((1.12f - h) / .30f, (.66f - w) / .18f));
        float near = clamp(Math.max((h - 1.55f) / .41f, (w - 1.20f) / .55f));
        float distance = near > far ? near : -far;
        float offset = clamp((centerDistance - .16f) / .32f);

        // A small exit margin prevents accepted faces flickering at the entry boundary.
        FaceFrame.Issue issue = FaceFrame.Issue.NONE;
        if (h < (previouslyAligned ? .78f : .82f) || w < (previouslyAligned ? .46f : .48f))
            issue = FaceFrame.Issue.TOO_FAR;
        else if (h > (previouslyAligned ? 2.06f : 1.96f) || w > (previouslyAligned ? 1.82f : 1.75f))
            issue = FaceFrame.Issue.TOO_CLOSE;
        else if (centerDistance > (previouslyAligned ? .48f : .42f))
            issue = FaceFrame.Issue.OUTSIDE_GUIDE;
        else {
            // Approximate the face by its oval, not the empty corners of its detector box.
            // Slight contact with the visual circle is fine; clearly leaving it is not.
            double limit = previouslyAligned ? 1.20 : 1.15;
            for (int i = 0; i < 32; i++) {
                double angle = i * Math.PI / 16;
                if (Math.hypot(cx + w * .5 * Math.cos(angle), cy + h * .5 * Math.sin(angle)) > limit) {
                    issue = FaceFrame.Issue.OUTSIDE_GUIDE;
                    break;
                }
            }
        }
        return new Feedback(issue, distance, offset);
    }
    private static float clamp(float value) { return Math.max(0, Math.min(1, value)); }
}
