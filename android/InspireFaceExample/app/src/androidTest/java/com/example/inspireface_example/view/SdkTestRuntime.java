package com.example.inspireface_example.view;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.os.SystemClock;

import androidx.test.platform.app.InstrumentationRegistry;

import com.insightface.sdk.inspireface.InspireFace;
import com.insightface.sdk.inspireface.jni.Native;
import com.insightface.sdk.inspireface.jni.NativeConstants;

import java.lang.reflect.Field;

/** Isolates tests that own GlobalLaunch/Terminate from Activity tests using FaceEngine. */
final class SdkTestRuntime {
    private SdkTestRuntime() {}

    static void resetWhenIdle() throws Exception {
        // ActivityScenario closes the UI synchronously; analyzer cleanup runs on its executor.
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
        long deadline = SystemClock.elapsedRealtime() + 5_000;
        while (true) {
            synchronized (FaceEngine.class) {
                int appSessions = field("activeSessions").getInt(null);
                int[] nativeSessions = new int[1];
                int[] nativeStreams = new int[1];
                assertEquals(NativeConstants.HSUCCEED,
                        Native.HFDeBugGetUnreleasedSessionsCount(nativeSessions));
                assertEquals(NativeConstants.HSUCCEED,
                        Native.HFDeBugGetUnreleasedStreamsCount(nativeStreams));
                if (appSessions == 0 && nativeSessions[0] == 0 && nativeStreams[0] == 0) {
                    if (InspireFace.QueryLaunchStatus()) assertTrue(InspireFace.GlobalTerminate());
                    // Direct SDK tests do not use ensureLaunched, so clear only its launch cache.
                    // Never zero counters or forcibly release another test's live resources.
                    field("launched").setBoolean(null, false);
                    field("launchedModel").set(null, null);
                    return;
                }
                if (SystemClock.elapsedRealtime() >= deadline) {
                    assertEquals("App sessions must finish before resetting SDK", 0, appSessions);
                    assertEquals("Native sessions leaked across tests", 0, nativeSessions[0]);
                    assertEquals("Native image streams leaked across tests", 0, nativeStreams[0]);
                }
            }
            SystemClock.sleep(50);
        }
    }

    private static Field field(String name) throws NoSuchFieldException {
        Field field = FaceEngine.class.getDeclaredField(name);
        field.setAccessible(true);
        return field;
    }
}
