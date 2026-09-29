package com.insightface.sdk.inspireface.jni;

import java.io.File;

/** Loads the JNI library for the running JVM architecture, not the OS architecture. */
final class NativeLibrary {
    private NativeLibrary() {}
    static void load() {
        String path = System.getProperty("inspireface.native.path");
        if (path == null || path.isEmpty()) {
            System.loadLibrary("InspireFaceJNI");
        } else {
            File library = new File(path);
            if (!library.isAbsolute()) {
                throw new IllegalArgumentException("inspireface.native.path must be an absolute library path");
            }
            System.load(library.getPath());
        }
    }
}
