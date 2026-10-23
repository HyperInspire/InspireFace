package com.example.inspireface_example.plus;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.UUID;

/** Serializes round ownership across the UI, camera worker and network callbacks. */
public final class PlusCaptureCoordinator {
    public enum State { READY, PREPARING, CAPTURING, ENCODING, UPLOADING, RESULT, RETRY, ERROR, CANCELLED }
    public enum Failure { NONE, GEOMETRY, PROCESSING, FACE_LOST, MULTIPLE_FACES, POSE, OUTSIDE_GUIDE, QUALITY }
    public interface Cropper { byte[] crop(); }
    public static final long SETTLE_NS = 180_000_000L;
    private static final long RECOVERY_NS = 350_000_000L;
    private long generation, startedNs, minimumIndex, phaseStartNs = Long.MAX_VALUE;
    private String captureId;
    private boolean color;
    private int phase = -1;
    private State state = State.READY;
    private Failure failure = Failure.NONE;
    private FaceFrame anchor;
    private FaceFrame roundAnchor;
    private long lastObservedNs = -1;
    private long unsettledSinceNs = -1;
    private Failure pendingFailure = Failure.NONE;
    private final List<Sample> samples = new ArrayList<>();

    public static final class Sample {
        public final FaceFrame frame;
        public final int phase;
        private final byte[] pixels;
        public Sample(FaceFrame frame, int phase, byte[] pixels) {
            if (pixels.length != 307200) throw new IllegalArgumentException("Invalid RGB8 size");
            this.frame = frame; this.phase = phase; this.pixels = pixels.clone();
        }
        public byte[] pixels() { return pixels.clone(); }
    }

    public synchronized long start(boolean color, long nowNs, long minimumIndex) {
        return start(color, nowNs, minimumIndex, null);
    }

    public synchronized long start(boolean color, long nowNs, long minimumIndex, FaceFrame initialFace) {
        if (isRunning()) throw new IllegalStateException("Round already active");
        generation++;
        captureId = UUID.randomUUID().toString();
        this.color = color; startedNs = nowNs; this.minimumIndex = minimumIndex;
        phase = color ? -1 : 0; phaseStartNs = Long.MAX_VALUE;
        samples.clear(); anchor = null; failure = Failure.NONE;
        roundAnchor = initialFace; lastObservedNs = -1;
        clearPendingFailure();
        state = color ? State.PREPARING : State.CAPTURING;
        return generation;
    }

    public synchronized boolean beginPhase(long token, int next, long displayedNs) {
        if (token != generation || !color || next != samples.size() || next < 0 || next > 3
                || (state != State.PREPARING && state != State.CAPTURING)
                || displayedNs < startedNs || next != phase + 1) return false;
        phase = next; phaseStartNs = displayedNs; state = State.CAPTURING;
        return true;
    }

    /** Cropper executes only for a selected frame, on the analyzer's single worker. */
    public synchronized boolean offer(long token, FaceFrame f, Cropper cropper) {
        if (token != generation || (state != State.CAPTURING && state != State.PREPARING)
                || (f.captureElapsedNs >= 0 && f.captureElapsedNs < startedNs)) return false;
        // Guard every fresh observation, including warm-up, settle time and already-filled phases.
        if (f.sourceIndex >= 0 && f.sourceIndex <= minimumIndex) return false;
        if (f.timestampNs >= 0 && f.timestampNs <= lastObservedNs) return false;
        if (f.timestampNs >= 0) lastObservedNs = f.timestampNs;
        if (roundAnchor != null && (f.width != roundAnchor.width || f.height != roundAnchor.height
                || f.rotation != roundAnchor.rotation || (f.trackId >= 0 && f.trackId != roundAnchor.trackId))) {
            return fail(Failure.GEOMETRY);
        }
        if (f.issue == FaceFrame.Issue.METADATA) {
            if (!color) { samples.clear(); anchor = null; }
            return false;
        }
        if (!f.valid()) {
            switch (f.issue) {
                case NO_FACE: return fail(Failure.FACE_LOST);
                case MULTIPLE: return fail(Failure.MULTIPLE_FACES);
                case POSE: return severePose(f) ? fail(Failure.POSE) : waitForRecovery(Failure.POSE, f);
                case OUTSIDE_GUIDE: case TOO_CLOSE: case TOO_FAR: case DISTANCE:
                    return waitForRecovery(Failure.OUTSIDE_GUIDE, f);
                case QUALITY: case EYES: return waitForRecovery(Failure.QUALITY, f);
                default: return fail(Failure.QUALITY);
            }
        }
        if (severePose(f)) return fail(Failure.POSE);
        if (!f.frontal() || (roundAnchor != null && !f.poseWithin(roundAnchor, 14)))
            return waitForRecovery(Failure.POSE, f);
        if (roundAnchor != null && !f.positionWithin(roundAnchor, .85, .50)) return fail(Failure.GEOMETRY);
        if (roundAnchor != null && !f.positionWithin(roundAnchor, .45, .25))
            return waitForRecovery(Failure.GEOMETRY, f);
        if (color && anchor != null && !f.sameGeometry(anchor, true))
            return waitForRecovery(Failure.GEOMETRY, f);
        clearPendingFailure();
        if (roundAnchor == null) roundAnchor = f;
        if (state == State.PREPARING) return false;
        if (color) {
            if (samples.size() != phase || f.captureElapsedNs < 0 || !f.locksConfirmed
                    || f.captureElapsedNs < phaseStartNs
                    || f.captureElapsedNs - phaseStartNs < SETTLE_NS) return false;
        } else if (anchor != null && (f.sourceIndex != anchor.sourceIndex + 1
                || !f.sameGeometry(anchor, false))) {
            samples.clear(); anchor = null;
        }
        if (!samples.isEmpty()) {
            FaceFrame last = samples.get(samples.size() - 1).frame;
            if (f.sourceIndex <= last.sourceIndex || f.timestampNs / 1000 <= last.timestampNs / 1000) return false;
        }
        try {
            samples.add(new Sample(f, color ? phase : -1, cropper.crop()));
        } catch (RuntimeException e) {
            return fail(Failure.PROCESSING);
        }
        if (anchor == null || !color) anchor = f;
        if (samples.size() == (color ? 4 : 20)) state = State.ENCODING;
        return true;
    }

    /** Borderline motion/blur pauses sampling; it does not immediately discard the round. */
    private boolean waitForRecovery(Failure reason, FaceFrame frame) {
        if (frame.timestampNs < 0) return fail(reason);
        if (unsettledSinceNs < 0) unsettledSinceNs = frame.timestampNs;
        pendingFailure = reason;
        // Passive RGB liveness requires one truly consecutive sequence, never stitched segments.
        if (!color) { samples.clear(); anchor = null; }
        if (frame.timestampNs - unsettledSinceNs >= RECOVERY_NS) return fail(reason);
        return false;
    }

    private boolean severePose(FaceFrame frame) {
        return (Float.isFinite(frame.yaw) && Math.abs(frame.yaw) > 35)
                || (Float.isFinite(frame.pitch) && Math.abs(frame.pitch) > 30)
                || (Float.isFinite(frame.roll) && Math.abs(frame.roll) > 28)
                || (roundAnchor != null && Float.isFinite(frame.yaw) && Float.isFinite(frame.pitch)
                && Float.isFinite(frame.roll) && !frame.poseWithin(roundAnchor, 25));
    }

    private void clearPendingFailure() { unsettledSinceNs = -1; pendingFailure = Failure.NONE; }

    private boolean fail(Failure reason) {
        state = State.ERROR; failure = reason; samples.clear(); anchor = null; roundAnchor = null;
        clearPendingFailure();
        return false;
    }

    public synchronized List<Sample> snapshot(long token) {
        if (token != generation || state != State.ENCODING) throw new IllegalStateException("Incomplete round");
        return Collections.unmodifiableList(new ArrayList<>(samples));
    }
    public synchronized boolean uploading(long token) {
        if (token != generation || (state != State.ENCODING && state != State.ERROR) || failure != Failure.NONE) return false;
        state = State.UPLOADING; samples.clear(); anchor = null; roundAnchor = null; return true;
    }
    public synchronized boolean finish(long token, State result) {
        if (token != generation || state != State.UPLOADING) return false;
        if (result != State.RESULT && result != State.RETRY && result != State.ERROR) return false;
        state = result; return true;
    }
    public synchronized void cancel() {
        generation++; state = State.CANCELLED; samples.clear(); anchor = null; roundAnchor = null;
        clearPendingFailure();
    }
    public synchronized void ready() {
        if (!isRunning()) { state = State.READY; failure = Failure.NONE; }
    }
    public synchronized boolean isRunning() {
        return state == State.PREPARING || state == State.CAPTURING || state == State.ENCODING || state == State.UPLOADING;
    }
    public synchronized long generation() { return generation; }
    public synchronized State state() { return state; }
    public synchronized Failure failure() { return failure; }
    public synchronized Failure pendingFailure() { return pendingFailure; }
    public synchronized int count() { return samples.size(); }
    public synchronized String captureId() { return captureId; }
}
