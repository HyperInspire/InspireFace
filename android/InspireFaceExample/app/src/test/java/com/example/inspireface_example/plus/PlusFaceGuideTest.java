package com.example.inspireface_example.plus;

import org.junit.Test;
import static org.junit.Assert.*;

public class PlusFaceGuideTest {
    @Test public void acceptsNaturalOffsetAndOvalWhoseBoxCornersCrossCircle() {
        FaceGuideGeometry g = new FaceGuideGeometry(360, 400);
        // The old center tolerance rejected this small shift.
        assertEquals(FaceFrame.Issue.NONE, g.evaluate(640, 480, 280, 130, 460, 350));
        // The actual oval fits even though the rectangular detector corners do not.
        assertEquals(FaceFrame.Issue.NONE, g.evaluate(640, 480, 190, 90, 450, 390));
        assertEquals(FaceFrame.Issue.OUTSIDE_GUIDE, g.evaluate(640, 480, 380, 130, 560, 350));
    }

    @Test public void distanceFeedbackChangesGraduallyTowardComfortableRange() {
        FaceGuideGeometry g = new FaceGuideGeometry(400, 400);
        FaceGuideGeometry.Feedback far = feedback(g, .30f);
        FaceGuideGeometry.Feedback closer = feedback(g, .40f);
        FaceGuideGeometry.Feedback comfortable = feedback(g, .55f);
        FaceGuideGeometry.Feedback near = feedback(g, .70f);
        FaceGuideGeometry.Feedback tooNear = feedback(g, .85f);
        assertEquals(FaceFrame.Issue.TOO_FAR, far.issue);
        assertTrue(far.distance < closer.distance);
        assertTrue(closer.distance < comfortable.distance);
        assertEquals(0, comfortable.distance, .001f);
        assertTrue(near.distance > comfortable.distance);
        assertTrue(tooNear.distance > near.distance);
        assertEquals(FaceFrame.Issue.TOO_CLOSE, tooNear.issue);
    }

    private FaceGuideGeometry.Feedback feedback(FaceGuideGeometry g, float imageFraction) {
        float halfHeight = 200 * imageFraction, halfWidth = halfHeight * .7f;
        return g.measure(400, 400, 200 - halfWidth, 200 - halfHeight,
                200 + halfWidth, 200 + halfHeight, false);
    }

    @Test public void boundaryHysteresisAllowsSmallJitterButNotClearDeparture() {
        FaceGuideGeometry g = new FaceGuideGeometry(400, 400);
        float dx = g.radius * .45f;
        assertEquals(FaceFrame.Issue.OUTSIDE_GUIDE, g.measure(400, 400, 145 + dx, 110, 255 + dx, 290, false).issue);
        assertEquals(FaceFrame.Issue.NONE, g.measure(400, 400, 145 + dx, 110, 255 + dx, 290, true).issue);
        dx = g.radius * .6f;
        assertEquals(FaceFrame.Issue.OUTSIDE_GUIDE, g.measure(400, 400, 145 + dx, 110, 255 + dx, 290, true).issue);
    }

    @Test public void largerHandheldJitterCanBecomeReady() {
        PlusFaceStability gate = new PlusFaceStability();
        for (int i = 0; i <= 12; i++) {
            long now = PlusCaptureTest.START + i * 50_000_000L;
            float shift = i % 2 == 0 ? 0 : 30;
            gate.observe(new FaceFrame(i, now, now, 640, 480, 90, 7,
                    270 + shift, 200, 370 + shift, 200, true, FaceFrame.Issue.NONE,
                    i % 2 == 0 ? 0 : 8, 0, 0), now);
        }
        assertTrue(gate.ready(PlusCaptureTest.START + 600_000_000L));
    }

    @Test public void centeredFaceFitsSameCircleInPortraitAndLandscape() {
        for (FaceGuideGeometry g : new FaceGuideGeometry[]{new FaceGuideGeometry(360, 400), new FaceGuideGeometry(400, 240)}) {
            assertEquals(FaceFrame.Issue.NONE, g.evaluate(640, 480, 230, 130, 410, 350));
            assertEquals(FaceFrame.Issue.TOO_FAR, g.evaluate(640, 480, 295, 200, 345, 280));
            assertEquals(FaceFrame.Issue.TOO_CLOSE, g.evaluate(640, 480, 170, 50, 470, 430));
        }
    }
    @Test public void frontMirrorAndCenterCropGiveSymmetricPlacementDecisions() {
        FaceGuideGeometry g = new FaceGuideGeometry(360, 400);
        assertEquals(FaceFrame.Issue.OUTSIDE_GUIDE, g.evaluate(640, 480, 310, 130, 490, 350));
        assertEquals(FaceFrame.Issue.OUTSIDE_GUIDE, g.evaluate(640, 480, 150, 130, 330, 350));
        assertEquals(FaceFrame.Issue.METADATA, new FaceGuideGeometry(0, 0).evaluate(640, 480, 230, 130, 410, 350));
    }
    @Test public void steadyFreshFramesFillRingAndAStaleFaceCannotStart() {
        PlusFaceStability gate = new PlusFaceStability();
        long time = PlusCaptureTest.START;
        for (int i = 0; i <= 12; i++) {
            long now = time + i * 50_000_000L;
            float progress = gate.observe(PlusCaptureTest.frame(i, now, true, FaceFrame.Issue.NONE), now);
            if (i < 12) assertTrue(progress < 1);
        }
        assertTrue(gate.ready(time + 600_000_000L));
        assertFalse(gate.ready(time + 1_200_000_000L));
        long now = time + 1_500_000_000L;
        assertEquals(0, gate.observe(PlusCaptureTest.frame(17, now, true, FaceFrame.Issue.NONE), now), 0);
    }
    @Test public void lossAndGradualDriftResetStabilityWithoutAutostarting() {
        PlusFaceStability gate = new PlusFaceStability();
        for (int i = 0; i <= 25; i++) {
            long now = PlusCaptureTest.START + i * 50_000_000L;
            FaceFrame f = new FaceFrame(i, now, now, 640, 480, 90, 7,
                    270 + i * 4, 200, 370 + i * 4, 200, true, FaceFrame.Issue.NONE);
            gate.observe(f, now); assertFalse(gate.ready(now));
        }
        assertEquals(0, gate.observe(PlusCaptureTest.frame(26, PlusCaptureTest.START + 1_300_000_000L,
                true, FaceFrame.Issue.NO_FACE), PlusCaptureTest.START + 1_300_000_000L), 0);
    }

    @Test public void briefMotionOrBlurPausesProgressThenContinuesFromThere() {
        for (FaceFrame.Issue issue : new FaceFrame.Issue[]{FaceFrame.Issue.NONE, FaceFrame.Issue.QUALITY}) {
            PlusFaceStability gate = new PlusFaceStability();
            float before = fill(gate, 400);
            assertTrue(before > .5f);
            long wobble = PlusCaptureTest.START + 450_000_000L;
            FaceFrame f = new FaceFrame(9, wobble, wobble, 640, 480, 90, 7,
                    310, 200, 410, 200, true, issue);
            assertEquals(before, gate.observe(f, wobble), 0);
            assertFalse(gate.ready(wobble));
            long recovered = PlusCaptureTest.START + 500_000_000L;
            assertEquals(before, gate.observe(PlusCaptureTest.frame(10, recovered, true, FaceFrame.Issue.NONE), recovered), 0);
            for (int i = 11; i <= 14; i++) {
                long now = PlusCaptureTest.START + i * 50_000_000L;
                gate.observe(PlusCaptureTest.frame(i, now, true, FaceFrame.Issue.NONE), now);
            }
            assertTrue(gate.ready(PlusCaptureTest.START + 700_000_000L));
        }
    }

    @Test public void fullRingSurvivesBriefBlurButCannotStartOnTheBlurredFrame() {
        PlusFaceStability gate = new PlusFaceStability();
        assertEquals(1, fill(gate, 600), 0);
        long blurred = PlusCaptureTest.START + 650_000_000L;
        assertEquals(1, gate.observe(PlusCaptureTest.frame(13, blurred, true, FaceFrame.Issue.QUALITY), blurred), 0);
        assertFalse(gate.ready(blurred));
        long recovered = PlusCaptureTest.START + 700_000_000L;
        assertEquals(1, gate.observe(PlusCaptureTest.frame(14, recovered, true, FaceFrame.Issue.NONE), recovered), 0);
        assertTrue(gate.ready(recovered));
    }

    @Test public void persistentBlurDoesNotKeepProgressIndefinitely() {
        PlusFaceStability gate = new PlusFaceStability();
        float before = fill(gate, 400);
        for (int i = 9; i < 15; i++) {
            long now = PlusCaptureTest.START + i * 50_000_000L;
            assertEquals(before, gate.observe(PlusCaptureTest.frame(i, now, true, FaceFrame.Issue.QUALITY), now), 0);
        }
        long expired = PlusCaptureTest.START + 750_000_000L;
        assertEquals(0, gate.observe(PlusCaptureTest.frame(15, expired, true, FaceFrame.Issue.QUALITY), expired), 0);
        assertFalse(gate.ready(expired));
    }

    @Test public void lostFaceAnotherPersonAndLargeDisplacementImmediatelyClearReadiness() {
        for (int scenario = 0; scenario < 3; scenario++) {
            PlusFaceStability gate = new PlusFaceStability();
            fill(gate, 600);
            long now = PlusCaptureTest.START + 650_000_000L;
            float shift = scenario == 2 ? 90 : 0;
            FaceFrame f = new FaceFrame(13, now, now, 640, 480, 90, scenario == 1 ? 99 : 7,
                    270 + shift, 200, 370 + shift, 200, true,
                    scenario == 0 ? FaceFrame.Issue.NO_FACE : FaceFrame.Issue.NONE);
            assertEquals(0, gate.observe(f, now), 0);
            assertFalse(gate.ready(now));
        }
    }

    @Test public void duplicatesAndBackloggedFramesCannotFillOrRefreshReadiness() {
        PlusFaceStability gate = new PlusFaceStability();
        fill(gate, 600);
        FaceFrame duplicate = PlusCaptureTest.frame(12, PlusCaptureTest.START + 600_000_000L, true, FaceFrame.Issue.NONE);
        gate.observe(duplicate, PlusCaptureTest.START + 1_000_000_000L);
        assertFalse(gate.ready(PlusCaptureTest.START + 1_000_000_000L));
        gate.reset();
        for (int i = 0; i <= 12; i++) {
            long received = PlusCaptureTest.START + i * 1_000_000L;
            float progress = gate.observe(PlusCaptureTest.frame(i, PlusCaptureTest.START + i * 50_000_000L,
                    true, FaceFrame.Issue.NONE), received);
            assertTrue(progress < .1f);
            assertFalse(gate.ready(received));
        }
    }

    private float fill(PlusFaceStability gate, int durationMs) {
        float progress = 0;
        for (int millis = 0; millis <= durationMs; millis += 50) {
            long now = PlusCaptureTest.START + millis * 1_000_000L;
            progress = gate.observe(PlusCaptureTest.frame(millis / 50, now, true, FaceFrame.Issue.NONE), now);
        }
        return progress;
    }
}
