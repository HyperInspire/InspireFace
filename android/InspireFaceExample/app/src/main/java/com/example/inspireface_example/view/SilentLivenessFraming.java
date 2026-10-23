package com.example.inspireface_example.view;

/** Geometry gate for local RGB liveness, in the upright SDK image coordinates. */
final class SilentLivenessFraming {
    /**
     * Matches InspireFace 1.2.4 RGB liveness's GetNewBox(..., 2.7f) context crop.
     * Require it to fit about the face center without the SDK shrinking or shifting it.
     */
    static final float CONTEXT_SCALE = 2.7f;
    /** Keep one source pixel inside each edge to avoid rounding into out-of-frame padding. */
    private static final double EDGE_INSET_PX = 1.0;

    enum Issue { NONE, TOO_CLOSE, NEAR_EDGE }

    private SilentLivenessFraming() {}

    static Issue evaluate(int x, int y, int width, int height,
                          int frameWidth, int frameHeight) {
        if (width <= 0 || height <= 0 || frameWidth <= 0 || frameHeight <= 0) {
            return Issue.NEAR_EDGE;
        }
        double contextWidth = width * CONTEXT_SCALE;
        double contextHeight = height * CONTEXT_SCALE;
        // If centering cannot fit the context, the user needs to move farther away first.
        if (contextWidth > frameWidth - 2 * EDGE_INSET_PX
                || contextHeight > frameHeight - 2 * EDGE_INSET_PX) {
            return Issue.TOO_CLOSE;
        }
        double centerX = x + width * 0.5;
        double centerY = y + height * 0.5;
        if (centerX - contextWidth * 0.5 < EDGE_INSET_PX
                || centerY - contextHeight * 0.5 < EDGE_INSET_PX
                || centerX + contextWidth * 0.5 > frameWidth - EDGE_INSET_PX
                || centerY + contextHeight * 0.5 > frameHeight - EDGE_INSET_PX) {
            return Issue.NEAR_EDGE;
        }
        return Issue.NONE;
    }
}
