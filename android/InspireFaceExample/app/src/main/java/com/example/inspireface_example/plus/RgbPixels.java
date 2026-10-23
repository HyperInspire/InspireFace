package com.example.inspireface_example.plus;

/** BT.601 limited-range conversion, matching the reference Camera2 RGB pipeline. */
public final class RgbPixels {
    private RgbPixels() {}

    public static byte[] cropNv21(byte[] nv21, FaceFrame frame) {
        int w = frame.width, h = frame.height;
        if ((w & 1) != 0 || (h & 1) != 0 || nv21.length < w * h * 3 / 2 || !frame.valid()) {
            throw new IllegalArgumentException("Invalid upright NV21 frame");
        }
        int[] pixels = new int[w * h];
        for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {
            int yy = Math.max(0, (nv21[y * w + x] & 255) - 16);
            int uv = w * h + (y / 2) * w + (x & ~1);
            int v = (nv21[uv] & 255) - 128, u = (nv21[uv + 1] & 255) - 128;
            int r = clip((298 * yy + 409 * v + 128) >> 8);
            int g = clip((298 * yy - 100 * u - 208 * v + 128) >> 8);
            int b = clip((298 * yy + 516 * u + 128) >> 8);
            pixels[y * w + x] = 0xff000000 | (r << 16) | (g << 8) | b;
        }
        return fromArgb(EyeCrop320.crop(pixels, w, h, frame.ax, frame.ay, frame.bx, frame.by));
    }

    public static byte[] fromArgb(int[] pixels) {
        if (pixels.length != 320 * 320) throw new IllegalArgumentException("Expected 320x320 pixels");
        byte[] bytes = new byte[307200];
        int i = 0;
        for (int pixel : pixels) {
            bytes[i++] = (byte) (pixel >>> 16);
            bytes[i++] = (byte) (pixel >>> 8);
            bytes[i++] = (byte) pixel;
        }
        return bytes;
    }

    private static int clip(int v) { return Math.max(0, Math.min(255, v)); }
}
