package com.example.inspireface_example.view;

/**
 * Allocation-free per-frame median smoothing for tracked face boxes.
 *
 * <p>Each Track ID keeps three samples for left/top/right/bottom. A three-value median rejects
 * single-frame coordinate spikes while adding at most one frame of visual latency.</p>
 */
final class TrackBoxSmoother {

    private static final int WINDOW_SIZE = 3;
    private static final int MAX_TRACK_STATES = FaceTrackingGlView.MAX_FACES * 3;

    private final State[] states = new State[MAX_TRACK_STATES];
    private long frameIndex;

    void beginFrame() {
        frameIndex++;
    }

    void smooth(int trackId, float left, float top, float right, float bottom,
                float[] output) {
        if (output == null || output.length < 4) {
            throw new IllegalArgumentException("output must contain at least four values");
        }
        State state = stateFor(trackId);
        state.lastSeenFrame = frameIndex;
        int sample = state.nextSample;
        state.left[sample] = left;
        state.top[sample] = top;
        state.right[sample] = right;
        state.bottom[sample] = bottom;
        state.nextSample = (sample + 1) % WINDOW_SIZE;
        state.sampleCount = Math.min(WINDOW_SIZE, state.sampleCount + 1);

        output[0] = filtered(state.left, state.sampleCount);
        output[1] = filtered(state.top, state.sampleCount);
        output[2] = filtered(state.right, state.sampleCount);
        output[3] = filtered(state.bottom, state.sampleCount);
    }

    void clear() {
        for (int i = 0; i < states.length; i++) {
            if (states[i] != null) {
                states[i].assigned = false;
            }
        }
        frameIndex = 0L;
    }

    private State stateFor(int trackId) {
        State available = null;
        State oldest = null;
        for (State state : states) {
            if (state == null) {
                continue;
            }
            if (state.assigned && state.trackId == trackId) {
                return state;
            }
            if (!state.assigned && available == null) {
                available = state;
            }
            if (state.assigned
                    && (oldest == null || state.lastSeenFrame < oldest.lastSeenFrame)) {
                oldest = state;
            }
        }
        if (available == null) {
            for (int i = 0; i < states.length; i++) {
                if (states[i] == null) {
                    available = new State();
                    states[i] = available;
                    break;
                }
            }
        }
        if (available == null) {
            available = oldest;
        }
        available.reset(trackId, frameIndex);
        return available;
    }

    private static float filtered(float[] values, int count) {
        if (count <= 1) {
            return values[0];
        }
        if (count == 2) {
            return (values[0] + values[1]) * 0.5f;
        }
        float a = values[0];
        float b = values[1];
        float c = values[2];
        return a + b + c - Math.min(a, Math.min(b, c)) - Math.max(a, Math.max(b, c));
    }

    private static final class State {
        final float[] left = new float[WINDOW_SIZE];
        final float[] top = new float[WINDOW_SIZE];
        final float[] right = new float[WINDOW_SIZE];
        final float[] bottom = new float[WINDOW_SIZE];
        boolean assigned;
        int trackId;
        int nextSample;
        int sampleCount;
        long lastSeenFrame;

        void reset(int id, long currentFrame) {
            assigned = true;
            trackId = id;
            nextSample = 0;
            sampleCount = 0;
            lastSeenFrame = currentFrame;
        }
    }
}
