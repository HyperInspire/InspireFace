package com.example.inspireface_example.plus;

/** Accumulates 600 ms of comfortable alignment, preserving progress through brief jitter. */
public final class PlusFaceStability {
    public static final long HOLD_NS = 600_000_000L;
    private static final long MAX_GAP_NS = 300_000_000L;
    private static final long JITTER_GRACE_NS = 300_000_000L;
    private FaceFrame anchor, previous;
    private long settledNs, receivedNs, interruptedSinceNs = -1;
    private boolean currentAccepted;

    public float observe(FaceFrame frame, long nowNs) {
        if (frame.timestampNs < 0 || frame.issue == FaceFrame.Issue.NO_FACE
                || frame.issue == FaceFrame.Issue.MULTIPLE || frame.trackId < 0) {
            reset(); return 0;
        }
        if (previous != null && (frame.timestampNs <= previous.timestampNs
                || (frame.sourceIndex >= 0 && previous.sourceIndex >= 0 && frame.sourceIndex <= previous.sourceIndex))) {
            return progress(); // Duplicate callbacks neither fill the ring nor refresh readiness.
        }
        if (anchor == null || previous == null || nowNs < receivedNs || nowNs - receivedNs > MAX_GAP_NS
                || frame.timestampNs - previous.timestampNs > MAX_GAP_NS
                || frame.trackId != anchor.trackId || frame.width != anchor.width
                || frame.height != anchor.height || frame.rotation != anchor.rotation) {
            return restart(frame, nowNs);
        }
        if ((Float.isFinite(frame.yaw) && Math.abs(frame.yaw) > 35)
                || (Float.isFinite(frame.pitch) && Math.abs(frame.pitch) > 30)
                || (Float.isFinite(frame.roll) && Math.abs(frame.roll) > 28)
                || (frame.valid() && !frame.positionWithin(anchor, .75, .45))
                || (interruptedSinceNs >= 0 && nowNs - interruptedSinceNs >= JITTER_GRACE_NS)) {
            return restart(frame, nowNs);
        }

        // Use both clocks' elapsed intervals so delayed camera callbacks cannot fast-forward readiness.
        long interval = Math.min(frame.timestampNs - previous.timestampNs, nowNs - receivedNs);
        boolean accepted = frame.valid() && frame.frontal()
                && frame.positionWithin(anchor, .35, .22) && frame.poseWithin(anchor, 10);
        if (accepted) {
            if (currentAccepted) settledNs = Math.min(HOLD_NS, settledNs + interval);
            interruptedSinceNs = -1;
        } else if (interruptedSinceNs < 0) interruptedSinceNs = nowNs;
        // An invalid or unsettled current frame never enables Start, even with a previously full ring.
        currentAccepted = accepted;
        previous = frame; receivedNs = nowNs;
        return progress();
    }
    private float restart(FaceFrame frame, long nowNs) {
        reset();
        previous = frame; receivedNs = nowNs;
        currentAccepted = frame.valid() && frame.frontal();
        anchor = currentAccepted ? frame : null;
        return 0;
    }
    private float progress() { return settledNs / (float) HOLD_NS; }
    public boolean ready(long nowNs) {
        return currentAccepted && settledNs >= HOLD_NS && nowNs >= receivedNs && nowNs - receivedNs <= MAX_GAP_NS;
    }
    public void reset() {
        anchor = null; previous = null; settledNs = 0; receivedNs = 0;
        interruptedSinceNs = -1; currentAccepted = false;
    }
}
