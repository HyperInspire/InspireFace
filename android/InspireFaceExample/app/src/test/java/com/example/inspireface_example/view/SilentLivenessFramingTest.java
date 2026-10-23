package com.example.inspireface_example.view;

import org.junit.Test;

import static com.example.inspireface_example.view.SilentLivenessFraming.Issue.NEAR_EDGE;
import static com.example.inspireface_example.view.SilentLivenessFraming.Issue.NONE;
import static com.example.inspireface_example.view.SilentLivenessFraming.Issue.TOO_CLOSE;
import static org.junit.Assert.assertEquals;

public class SilentLivenessFramingTest {
    @Test
    public void centeredFaceWithContextFitsPortraitAndLandscape() {
        assertEquals(NONE, evaluate(180, 240, 120, 160, 480, 640));
        assertEquals(NONE, evaluate(260, 160, 120, 160, 640, 480));
    }

    @Test
    public void visibleFaceIsRejectedWhenContextCrossesAnyEdge() {
        assertEquals(NEAR_EDGE, evaluate(84, 250, 100, 100, 480, 640));
        assertEquals(NEAR_EDGE, evaluate(296, 250, 100, 100, 480, 640));
        assertEquals(NEAR_EDGE, evaluate(190, 84, 100, 100, 480, 640));
        assertEquals(NEAR_EDGE, evaluate(190, 456, 100, 100, 480, 640));
    }

    @Test
    public void contextTouchingTheImageEdgeStillNeedsAnInset() {
        assertEquals(NEAR_EDGE, evaluate(85, 250, 100, 100, 480, 640));
        assertEquals(NEAR_EDGE, evaluate(295, 250, 100, 100, 480, 640));
        assertEquals(NEAR_EDGE, evaluate(190, 85, 100, 100, 480, 640));
        assertEquals(NEAR_EDGE, evaluate(190, 455, 100, 100, 480, 640));
        assertEquals(NONE, evaluate(86, 250, 100, 100, 480, 640));
        assertEquals(NONE, evaluate(294, 250, 100, 100, 480, 640));
        assertEquals(NONE, evaluate(190, 86, 100, 100, 480, 640));
        assertEquals(NONE, evaluate(190, 454, 100, 100, 480, 640));
    }

    @Test
    public void smallerContextDoesNotHideSdkCropOverflow() {
        // A 2.2x expansion would leave 15px at the left edge; the SDK's 2.7x crosses it.
        assertEquals(NEAR_EDGE, evaluate(75, 250, 100, 100, 480, 640));
    }

    @Test
    public void contextTooLargeToFitNeedsDistanceInsteadOfRecentering() {
        assertEquals(TOO_CLOSE, evaluate(151, 240, 178, 160, 480, 640));
        assertEquals(TOO_CLOSE, evaluate(260, 130, 120, 220, 640, 480));
        assertEquals(TOO_CLOSE, evaluate(0, 0, 220, 220, 480, 640));
        assertEquals(NONE, evaluate(152, 240, 176, 160, 480, 640));
    }

    @Test
    public void uprightRotationUsesBothImageDimensions() {
        // Rotate the frame and box by 90 degrees: the geometry decision stays the same.
        assertEquals(NEAR_EDGE, evaluate(180, 80, 120, 160, 480, 640));
        assertEquals(NEAR_EDGE, evaluate(400, 180, 160, 120, 640, 480));
        assertEquals(NONE, evaluate(180, 140, 120, 160, 480, 640));
        assertEquals(NONE, evaluate(340, 180, 160, 120, 640, 480));
    }

    @Test
    public void invalidAndOutOfFrameGeometryCannotPass() {
        assertEquals(NEAR_EDGE, evaluate(190, 250, 0, 100, 480, 640));
        assertEquals(NEAR_EDGE, evaluate(190, 250, 100, -1, 480, 640));
        assertEquals(NEAR_EDGE, evaluate(190, 250, 100, 100, 0, 640));
        assertEquals(NEAR_EDGE, evaluate(190, 250, 100, 100, 480, -1));
        assertEquals(NEAR_EDGE, evaluate(-5, 250, 100, 100, 480, 640));
        assertEquals(NEAR_EDGE, evaluate(Integer.MAX_VALUE, 250, 100, 100, 480, 640));
    }

    private static SilentLivenessFraming.Issue evaluate(int x, int y, int width, int height,
                                                        int frameWidth, int frameHeight) {
        return SilentLivenessFraming.evaluate(x, y, width, height, frameWidth, frameHeight);
    }
}
