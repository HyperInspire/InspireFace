package com.example.inspireface_example.plus;

import org.junit.Test;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Arrays;
import java.util.Properties;
import static org.junit.Assert.*;

public class PlusCaptureTest {
    static final long START = 1_000_000_000L;
    static FaceFrame frame(long index, long time, boolean locked, FaceFrame.Issue issue) {
        return new FaceFrame(index, time, time, 640, 480, 90, 7,
                270, 200, 370, 200, locked, issue);
    }
    static PlusCaptureCoordinator completeColor() {
        PlusCaptureCoordinator c = new PlusCaptureCoordinator();
        long t = c.start(true, START, -1);
        for (int p = 0; p < 4; p++) {
            long displayed = START + p * 1_000_000_000L;
            assertTrue(c.beginPhase(t, p, displayed));
            assertTrue(c.offer(t, frame(p * 5, displayed + 200_000_000L, true, FaceFrame.Issue.NONE), () -> new byte[307200]));
        }
        return c;
    }

    @Test public void cropMatchesIndependentReferenceFixtures() throws Exception {
        Properties golden = new Properties();
        try (InputStream input = getClass().getResourceAsStream("/eye_crop_golden.properties")) { golden.load(input); }
        int[] pixels = new int[480 * 360];
        for (int y = 0; y < 360; y++) for (int x = 0; x < 480; x++) {
            pixels[y * 480 + x] = 0xff000000 | (((x * 13 + y * 7 + x * y) % 256) << 16)
                    | (((x * 3 + y * 19) % 256) << 8) | ((x * 23 + y * 11) % 256);
        }
        assertEquals(golden.getProperty("eye_crop"), hash(EyeCrop320.crop(pixels, 480, 360, 170, 120, 240, 130)));
        for (int angle : new int[] {0, -17, 90, 183}) {
            assertEquals(golden.getProperty("affine_" + angle), hash(EyeCrop320.affine(pixels,
                    480, 360, (double) .85f, (double) .85f, angle, 240, 160, 160, 128)));
        }
    }
    private static String hash(int[] pixels) {
        ByteBuffer data = ByteBuffer.allocate(pixels.length * 4).order(ByteOrder.LITTLE_ENDIAN);
        for (int p : pixels) data.putInt(p);
        return CapturePacket.sha256(data.array());
    }
    @Test public void rgb8PreservesChannelOrderAndUsesFullBytes() {
        int[] pixels = new int[102400]; Arrays.fill(pixels, 0xff1280ef);
        byte[] rgb = RgbPixels.fromArgb(pixels);
        assertEquals(307200, rgb.length);
        assertArrayEquals(new byte[] {0x12, (byte) 0x80, (byte) 0xef}, Arrays.copyOf(rgb, 3));
        byte[] nv21 = new byte[640 * 480 * 3 / 2];
        Arrays.fill(nv21, 0, 640 * 480, (byte) 126);
        Arrays.fill(nv21, 640 * 480, nv21.length, (byte) 128);
        byte[] crop = RgbPixels.cropNv21(nv21, frame(1, START, true, FaceFrame.Issue.NONE));
        int center = (160 * 320 + 160) * 3;
        assertArrayEquals(new byte[] {(byte) 128, (byte) 128, (byte) 128}, Arrays.copyOfRange(crop, center, center + 3));
    }
    @Test public void flashRejectsOldFramesAndUnconfirmedLocksAndDuplicates() {
        PlusCaptureCoordinator c = new PlusCaptureCoordinator();
        long t = c.start(true, START, -1);
        assertFalse(c.offer(t, frame(1, START, true, FaceFrame.Issue.NONE), () -> { fail(); return null; }));
        assertTrue(c.beginPhase(t, 0, START));
        assertFalse(c.offer(t, frame(2, START + 179_999_999, true, FaceFrame.Issue.NONE), () -> { fail(); return null; }));
        assertFalse(c.offer(t, frame(3, START + 200_000_000, false, FaceFrame.Issue.NONE), () -> { fail(); return null; }));
        assertTrue(c.offer(t, frame(4, START + 201_000_000, true, FaceFrame.Issue.NONE), () -> new byte[307200]));
        assertFalse(c.offer(t, frame(5, START + 300_000_000, true, FaceFrame.Issue.NONE), () -> { fail(); return null; }));
        assertFalse(c.beginPhase(t, 2, START + 500_000_000));
        assertEquals(1, c.count());
    }
    @Test public void rgbRequiresTwentyActuallyContinuousFrames() {
        PlusCaptureCoordinator c = new PlusCaptureCoordinator();
        long t = c.start(false, START, -1);
        for (int i = 0; i < 5; i++) c.offer(t, frame(i, START + i * 1_000_000L, false, FaceFrame.Issue.NONE), () -> new byte[307200]);
        assertEquals(5, c.count());
        // Source frame 5 was dropped. The next segment keeps its real indices.
        for (int i = 6; i < 26; i++) c.offer(t, frame(i, START + i * 1_000_000L, false, FaceFrame.Issue.NONE), () -> new byte[307200]);
        assertEquals(PlusCaptureCoordinator.State.ENCODING, c.state());
        assertEquals(20, c.snapshot(t).size());
        assertEquals(6, c.snapshot(t).get(0).frame.sourceIndex);
        assertEquals(25, c.snapshot(t).get(19).frame.sourceIndex);
    }
    @Test public void invalidFaceAbortsRgbRoundAndRequiresANewStart() {
        PlusCaptureCoordinator c = new PlusCaptureCoordinator();
        long t = c.start(false, START, -1);
        c.offer(t, frame(1, START + 1_000_000, false, FaceFrame.Issue.NONE), () -> new byte[307200]);
        c.offer(t, frame(2, START + 2_000_000, false, FaceFrame.Issue.MULTIPLE), () -> { fail(); return null; });
        assertEquals(0, c.count());
        assertEquals(PlusCaptureCoordinator.State.ERROR, c.state());
        assertEquals(PlusCaptureCoordinator.Failure.MULTIPLE_FACES, c.failure());
        assertFalse(c.uploading(t));
        assertFalse(c.offer(t, frame(3, START + 3_000_000, false, FaceFrame.Issue.NONE), () -> { fail(); return null; }));
        String id = c.captureId();
        c.start(false, START + 4_000_000, -1);
        assertNotEquals(id, c.captureId());
    }

    @Test public void losingFaceBetweenLightPhasesAbortsImmediately() {
        PlusCaptureCoordinator c = new PlusCaptureCoordinator();
        long t = c.start(true, START, -1);
        c.beginPhase(t, 0, START);
        c.offer(t, frame(1, START + 200_000_000, true, FaceFrame.Issue.NONE), () -> new byte[307200]);
        // White is already filled; the next color has not started yet.
        assertFalse(c.offer(t, frame(2, START + 250_000_000, true, FaceFrame.Issue.NO_FACE), () -> { fail(); return null; }));
        assertEquals(PlusCaptureCoordinator.Failure.FACE_LOST, c.failure());
        assertEquals(0, c.count());
        assertFalse(c.beginPhase(t, 1, START + 500_000_000));
        assertFalse(c.uploading(t));
    }

    @Test public void guardAlsoChecksWarmupAndBeforeSettling() {
        for (boolean warmup : new boolean[]{true, false}) {
            PlusCaptureCoordinator c = new PlusCaptureCoordinator();
            long t = c.start(true, START, -1);
            if (!warmup) c.beginPhase(t, 0, START);
            assertFalse(c.offer(t, frame(1, START + 10_000_000, false, FaceFrame.Issue.OUTSIDE_GUIDE), () -> { fail(); return null; }));
            assertEquals(PlusCaptureCoordinator.Failure.OUTSIDE_GUIDE, c.pendingFailure());
            assertTrue(c.isRunning());
            assertFalse(c.offer(t, frame(2, START + 410_000_000, false, FaceFrame.Issue.OUTSIDE_GUIDE), () -> { fail(); return null; }));
            assertEquals(PlusCaptureCoordinator.Failure.OUTSIDE_GUIDE, c.failure());
            assertFalse(c.uploading(t));
        }
    }

    @Test public void persistentRelativeHeadTurnStillAborts() {
        for (boolean color : new boolean[]{true, false}) {
            PlusCaptureCoordinator c = new PlusCaptureCoordinator();
            long t = c.start(color, START, -1, frame(0, START - 1, true, FaceFrame.Issue.NONE));
            FaceFrame turned = new FaceFrame(1, START + 1, START + 1, 640, 480, 90, 7,
                    270, 200, 370, 200, true, FaceFrame.Issue.NONE, 18, 0, 0);
            assertTrue(turned.frontal());
            assertFalse(c.offer(t, turned, () -> { fail(); return null; }));
            assertEquals(PlusCaptureCoordinator.Failure.POSE, c.pendingFailure());
            assertTrue(c.isRunning());
            turned = new FaceFrame(2, START + 400_000_000, START + 400_000_000, 640, 480, 90, 7,
                    270, 200, 370, 200, true, FaceFrame.Issue.NONE, 18, 0, 0);
            assertFalse(c.offer(t, turned, () -> { fail(); return null; }));
            assertEquals(PlusCaptureCoordinator.Failure.POSE, c.failure());
        }
    }

    @Test public void smallInFrameMotionCompletesBothModesAndBuildsAValidPacket() throws Exception {
        for (boolean color : new boolean[]{true, false}) {
            PlusCaptureCoordinator c = new PlusCaptureCoordinator();
            long token = c.start(color, START, -1, frame(0, START - 1, true, FaceFrame.Issue.NONE));
            for (int i = 0; i < (color ? 4 : 20); i++) {
                long time = START + i * 500_000_000L;
                if (color) assertTrue(c.beginPhase(token, i, time));
                // A 40%-of-eye-distance shift, 22% size change and 12-degree turn used to abort.
                FaceFrame moved = new FaceFrame(i + 1, time + 200_000_000, time + 200_000_000,
                        640, 480, 90, 7, 299, 200, 421, 200, true, FaceFrame.Issue.NONE, 12, 0, 0);
                assertTrue(c.offer(token, moved, () -> new byte[307200]));
            }
            assertEquals(PlusCaptureCoordinator.State.ENCODING, c.state());
            assertNotNull(new CapturePacket(c.captureId(), color, c.snapshot(token)));
        }
    }

    @Test public void briefBlurKeepsColorRoundAndRecoversWithoutAnotherStart() throws Exception {
        PlusCaptureCoordinator c = new PlusCaptureCoordinator();
        long token = c.start(true, START, -1);
        String id = c.captureId();
        c.beginPhase(token, 0, START);
        assertTrue(c.offer(token, frame(1, START + 200_000_000, true, FaceFrame.Issue.NONE), () -> new byte[307200]));
        assertFalse(c.offer(token, frame(2, START + 250_000_000, true, FaceFrame.Issue.QUALITY), () -> { fail(); return null; }));
        assertEquals(1, c.count());
        assertEquals(PlusCaptureCoordinator.Failure.QUALITY, c.pendingFailure());
        assertFalse(c.offer(token, frame(3, START + 400_000_000, true, FaceFrame.Issue.NONE), () -> { fail(); return null; }));
        assertEquals(PlusCaptureCoordinator.Failure.NONE, c.pendingFailure());
        for (int p = 1; p < 4; p++) {
            long time = START + p * 500_000_000L;
            assertTrue(c.beginPhase(token, p, time));
            assertTrue(c.offer(token, frame(p + 3, time + 200_000_000, true, FaceFrame.Issue.NONE), () -> new byte[307200]));
        }
        assertEquals(id, c.captureId());
        assertNotNull(new CapturePacket(id, true, c.snapshot(token)));
    }

    @Test public void silentRecoversFromBriefBlurWithOneNewContiguousSegment() throws Exception {
        PlusCaptureCoordinator c = new PlusCaptureCoordinator();
        long token = c.start(false, START, -1);
        String id = c.captureId();
        assertTrue(c.offer(token, frame(1, START + 50_000_000, false, FaceFrame.Issue.NONE), () -> new byte[307200]));
        assertFalse(c.offer(token, frame(2, START + 100_000_000, false, FaceFrame.Issue.QUALITY), () -> { fail(); return null; }));
        assertTrue(c.isRunning());
        assertEquals(0, c.count());
        for (int i = 3; i <= 22; i++) {
            assertTrue(c.offer(token, frame(i, START + i * 50_000_000L, false, FaceFrame.Issue.NONE), () -> new byte[307200]));
        }
        assertEquals(id, c.captureId());
        assertEquals(3, c.snapshot(token).get(0).frame.sourceIndex);
        assertNotNull(new CapturePacket(id, false, c.snapshot(token)));
    }

    @Test public void changingWarningReasonCannotProlongTheRecoveryWindow() {
        PlusCaptureCoordinator c = new PlusCaptureCoordinator();
        long token = c.start(false, START, -1);
        c.offer(token, frame(1, START + 100_000_000, false, FaceFrame.Issue.QUALITY), () -> { fail(); return null; });
        c.offer(token, frame(2, START + 250_000_000, false, FaceFrame.Issue.OUTSIDE_GUIDE), () -> { fail(); return null; });
        assertTrue(c.isRunning());
        c.offer(token, frame(3, START + 500_000_000, false, FaceFrame.Issue.QUALITY), () -> { fail(); return null; });
        assertEquals(PlusCaptureCoordinator.Failure.QUALITY, c.failure());
        assertFalse(c.uploading(token));
    }

    @Test public void recoveryCannotHideFaceLossAnotherPersonOrALargeTurn() {
        for (int scenario = 0; scenario < 3; scenario++) {
            PlusCaptureCoordinator c = new PlusCaptureCoordinator();
            long token = c.start(true, START, -1, frame(0, START - 1, true, FaceFrame.Issue.NONE));
            c.offer(token, frame(1, START + 100_000_000, true, FaceFrame.Issue.QUALITY), () -> { fail(); return null; });
            FaceFrame f = scenario == 0 ? frame(2, START + 150_000_000, true, FaceFrame.Issue.NO_FACE)
                    : new FaceFrame(2, START + 150_000_000, START + 150_000_000, 640, 480, 90,
                    scenario == 1 ? 99 : 7, 270, 200, 370, 200, true,
                    scenario == 2 ? FaceFrame.Issue.POSE : FaceFrame.Issue.NONE, scenario == 2 ? 40 : 0, 0, 0);
            c.offer(token, f, () -> { fail(); return null; });
            assertEquals(PlusCaptureCoordinator.State.ERROR, c.state());
            assertFalse(c.uploading(token));
        }
    }

    @Test public void faceMovementAfterCaptureDoesNotInvalidateCompletedPacket() {
        PlusCaptureCoordinator c = completeColor();
        long t = c.generation();
        assertFalse(c.offer(t, frame(99, START + 9_000_000_000L, true, FaceFrame.Issue.NO_FACE), () -> { fail(); return null; }));
        assertEquals(4, c.snapshot(t).size());
        assertTrue(c.uploading(t));
        assertTrue(c.finish(t, PlusCaptureCoordinator.State.RESULT));
    }
    @Test public void changingPersonAbortsColorInsteadOfMixingFaces() {
        PlusCaptureCoordinator c = new PlusCaptureCoordinator();
        long t = c.start(true, START, -1);
        c.beginPhase(t, 0, START);
        c.offer(t, frame(1, START + 200_000_000, true, FaceFrame.Issue.NONE), () -> new byte[307200]);
        c.beginPhase(t, 1, START + 500_000_000);
        FaceFrame other = new FaceFrame(2, START + 700_000_000, START + 700_000_000,
                640, 480, 90, 99, 270, 200, 370, 200, true, FaceFrame.Issue.NONE);
        assertFalse(c.offer(t, other, () -> { fail(); return null; }));
        assertEquals(PlusCaptureCoordinator.Failure.GEOMETRY, c.failure());
        assertEquals(0, c.count());
    }
    @Test public void cancelInvalidatesEveryAsyncStageAndNewRoundHasNewId() {
        for (int stage = 0; stage < 4; stage++) {
            PlusCaptureCoordinator c = stage >= 2 ? completeColor() : new PlusCaptureCoordinator();
            long t = stage >= 2 ? c.generation() : c.start(true, START, -1);
            if (stage == 1) c.beginPhase(t, 0, START);
            if (stage == 3) assertTrue(c.uploading(t));
            String oldId = c.captureId();
            c.cancel();
            assertFalse(c.beginPhase(t, 0, START));
            assertFalse(c.offer(t, frame(1, START + 200_000_000, true, FaceFrame.Issue.NONE), () -> { fail(); return null; }));
            assertFalse(c.uploading(t));
            assertFalse(c.finish(t, PlusCaptureCoordinator.State.RESULT));
            c.start(false, START + 2_000_000_000L, -1);
            assertNotEquals(oldId, c.captureId());
            assertEquals(0, c.count());
        }
    }
}
