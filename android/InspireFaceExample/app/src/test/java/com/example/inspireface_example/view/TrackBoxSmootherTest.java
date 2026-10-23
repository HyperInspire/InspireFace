package com.example.inspireface_example.view;

import static org.junit.Assert.assertArrayEquals;

import org.junit.Test;

public class TrackBoxSmootherTest {

    @Test
    public void medianOfThreeRejectsSingleFrameBoxSpike() {
        TrackBoxSmoother smoother = new TrackBoxSmoother();
        float[] output = new float[4];

        smoother.beginFrame();
        smoother.smooth(7, 10f, 20f, 110f, 140f, output);
        smoother.beginFrame();
        smoother.smooth(7, 12f, 22f, 112f, 142f, output);
        smoother.beginFrame();
        smoother.smooth(7, 80f, 90f, 180f, 210f, output);

        assertArrayEquals(new float[]{12f, 22f, 112f, 142f}, output, 0.001f);
    }

    @Test
    public void historiesAreIsolatedByTrackId() {
        TrackBoxSmoother smoother = new TrackBoxSmoother();
        float[] first = new float[4];
        float[] second = new float[4];

        smoother.beginFrame();
        smoother.smooth(1, 10f, 10f, 50f, 50f, first);
        smoother.smooth(2, 200f, 200f, 260f, 260f, second);
        smoother.beginFrame();
        smoother.smooth(1, 12f, 12f, 52f, 52f, first);
        smoother.smooth(2, 204f, 204f, 264f, 264f, second);

        assertArrayEquals(new float[]{11f, 11f, 51f, 51f}, first, 0.001f);
        assertArrayEquals(new float[]{202f, 202f, 262f, 262f}, second, 0.001f);
    }
}
