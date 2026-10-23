package com.example.inspireface_example.plus;

/** Immutable observation in upright, unmirrored source coordinates. */
public final class FaceFrame {
    public enum Issue { NONE, NO_FACE, MULTIPLE, POSE, DISTANCE, EYES, QUALITY, METADATA, OUTSIDE_GUIDE, TOO_CLOSE, TOO_FAR }
    public final long sourceIndex, timestampNs, captureElapsedNs;
    public final int width, height, rotation, trackId;
    public final float ax, ay, bx, by;
    public final float yaw, pitch, roll;
    public final boolean locksConfirmed;
    public final Issue issue;
    public final FaceGuideGeometry.Feedback guide;

    public FaceFrame(long index, long timestampNs, long elapsedNs, int width, int height,
                     int rotation, int trackId, float ax, float ay, float bx, float by,
                     boolean locksConfirmed, Issue issue) {
        this(index, timestampNs, elapsedNs, width, height, rotation, trackId,
                ax, ay, bx, by, locksConfirmed, issue, 0, 0, 0);
    }

    public FaceFrame(long index, long timestampNs, long elapsedNs, int width, int height,
                     int rotation, int trackId, float ax, float ay, float bx, float by,
                     boolean locksConfirmed, Issue issue, float yaw, float pitch, float roll) {
        this(index, timestampNs, elapsedNs, width, height, rotation, trackId,
                ax, ay, bx, by, locksConfirmed, issue, yaw, pitch, roll, null);
    }

    public FaceFrame(long index, long timestampNs, long elapsedNs, int width, int height,
                     int rotation, int trackId, float ax, float ay, float bx, float by,
                     boolean locksConfirmed, Issue issue, float yaw, float pitch, float roll,
                     FaceGuideGeometry.Feedback guide) {
        this.sourceIndex = index; this.timestampNs = timestampNs; captureElapsedNs = elapsedNs;
        this.width = width; this.height = height; this.rotation = rotation; this.trackId = trackId;
        this.ax = ax; this.ay = ay; this.bx = bx; this.by = by;
        this.locksConfirmed = locksConfirmed; this.issue = issue;
        this.yaw = yaw; this.pitch = pitch; this.roll = roll;
        this.guide = guide;
    }

    public boolean valid() {
        return issue == Issue.NONE && sourceIndex >= 0 && timestampNs >= 0 && width >= 2 && height >= 2
                && Float.isFinite(ax) && Float.isFinite(ay) && Float.isFinite(bx) && Float.isFinite(by)
                && ax >= 0 && ay >= 0 && bx < width && by < height && ay < height && by >= 0
                && bx - ax >= 2;
    }

    public boolean frontal() {
        return Float.isFinite(yaw) && Float.isFinite(pitch) && Float.isFinite(roll)
                && Math.abs(yaw) <= 20 && Math.abs(pitch) <= 18 && Math.abs(roll) <= 15;
    }

    public boolean poseWithin(FaceFrame previous, float degrees) {
        return Math.abs(yaw - previous.yaw) <= degrees && Math.abs(pitch - previous.pitch) <= degrees
                && Math.abs(roll - previous.roll) <= degrees;
    }

    public boolean positionWithin(FaceFrame previous, double movement, double scaleChange) {
        if (width != previous.width || height != previous.height || rotation != previous.rotation
                || trackId != previous.trackId) return false;
        double distance = Math.hypot(previous.bx - previous.ax, previous.by - previous.ay);
        double ratio = Math.hypot(bx - ax, by - ay) / distance;
        return ratio >= 1 - scaleChange && ratio <= 1 + scaleChange
                && Math.hypot((ax + bx - previous.ax - previous.bx) * .5,
                (ay + by - previous.ay - previous.by) * .5) <= distance * movement;
    }

    public boolean sameGeometry(FaceFrame previous, boolean color) {
        if (width != previous.width || height != previous.height || rotation != previous.rotation
                || trackId != previous.trackId) return false;
        double oldDistance = Math.hypot(previous.bx - previous.ax, previous.by - previous.ay);
        double ratio = Math.hypot(bx - ax, by - ay) / oldDistance;
        double movement = Math.hypot((ax + bx - previous.ax - previous.bx) * .5,
                (ay + by - previous.ay - previous.by) * .5) / oldDistance;
        return ratio >= (color ? .75 : .65) && ratio <= (color ? 1.3 : 1.5)
                && movement <= (color ? .45 : .6);
    }
}
