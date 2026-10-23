package com.example.inspireface_example.view;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.content.ContextWrapper;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import com.example.inspireface_example.FaceModelPrefs;
import com.example.inspireface_example.face.FaceRepository;
import com.insightface.sdk.inspireface.InspireFace;
import com.insightface.sdk.inspireface.base.FaceFeature;
import com.insightface.sdk.inspireface.base.FaceFeatureIdentity;
import com.insightface.sdk.inspireface.base.ImageStream;
import com.insightface.sdk.inspireface.base.MultipleFaceData;
import com.insightface.sdk.inspireface.base.Session;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.File;
import java.io.InputStream;
import java.util.HashSet;
import java.util.Set;
import java.util.UUID;

/** Real app repository/JNI round trips; all database, preferences and crops are isolated. */
@RunWith(AndroidJUnit4.class)
public final class FaceRepositorySdkTest {
    @Before public void prepareRuntime() throws Exception { SdkTestRuntime.resetWhenIdle(); }
    @After public void releaseRuntime() throws Exception { SdkTestRuntime.resetWhenIdle(); }

    @Test
    public void bothModelsPersistSearchUpdateAndDeleteLongIds() throws Exception {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        IsolatedStorage storage = new IsolatedStorage(context);
        Bitmap bitmap = null;
        try {
            try (InputStream input = context.getAssets().open("inspireface/kun.jpg")) {
                BitmapFactory.Options options = new BitmapFactory.Options();
                options.inPreferredConfig = Bitmap.Config.ARGB_8888;
                bitmap = BitmapFactory.decodeStream(input, null, options);
            }
            assertNotNull(bitmap);
            for (FaceModelPrefs.Model model : FaceModelPrefs.Model.values()) {
                assertTrue("Launch " + model.sdkName(), InspireFace.GlobalLaunch(context, model.sdkName()));
                try {
                    checkRepository(storage, model, extractFeature(bitmap), bitmap);
                } finally {
                    assertTrue(InspireFace.GlobalTerminate());
                }
            }
        } finally {
            if (bitmap != null) bitmap.recycle();
            storage.clear();
        }
    }

    private static void checkRepository(Context context, FaceModelPrefs.Model model,
                                        FaceFeature feature, Bitmap bitmap) {
        long id = (1L << 40) + 23;
        // Seed only this test's metadata to force the app ID generator across the int boundary.
        assertTrue(context.getSharedPreferences("face_records_" + model.sdkName(), Context.MODE_PRIVATE)
                .edit().putLong("next_id", id).commit());
        FaceRepository repository = new FaceRepository(context, model);
        try {
            assertTrue(repository.open());
            assertFalse(repository.search(feature).matched);
            FaceFeatureIdentity missing = InspireFace.FeatureHubFaceSearch(feature);
            assertNotNull("Empty search is a successful no-match result", missing);
            assertEquals(-1L, missing.id);
            assertNull(missing.feature);

            FaceRepository.InsertResult inserted = repository.insert("SDK post1", feature, bitmap);
            assertTrue(inserted.success);
            assertNotNull(inserted.record);
            assertEquals(id, inserted.record.id);
            assertTrue(new File(inserted.record.cropPath).isFile());
            assertArrayEquals(new long[]{id}, InspireFace.FeatureHubGetExistingIds());
            assertEquals(id, InspireFace.FeatureHubFaceSearch(feature).id);
            FaceRepository.SearchResult match = repository.search(feature);
            assertTrue("App TopK search must join 64-bit IDs to metadata", match.matched);
            assertEquals(id, match.record.id);
            assertEquals(1f, match.confidence, 0.001f);

            assertTrue(repository.update(id, "Updated SDK post1", feature, bitmap));
            assertEquals(id, InspireFace.FeatureHubGetFaceIdentity(id).id);
            repository.close();
            assertTrue(repository.databaseFile().isFile());
            repository = new FaceRepository(context, model);
            assertTrue(repository.open());
            assertEquals(1, InspireFace.FeatureHubGetFaceCount());
            match = repository.search(feature);
            assertTrue("Persistent features must survive disable/re-enable", match.matched);
            assertEquals(id, match.record.id);
            assertEquals("Updated SDK post1", match.record.name);
            assertEquals(1, repository.query(Long.toString(id)).size());

            String cropPath = match.record.cropPath;
            assertTrue(repository.delete(id));
            assertNull(repository.get(id));
            assertFalse(new File(cropPath).exists());
            assertEquals(0, InspireFace.FeatureHubGetFaceCount());
            assertEquals(0, InspireFace.FeatureHubGetExistingIds().length);
            assertFalse(repository.search(feature).matched);
        } finally {
            repository.close();
        }
    }

    private static FaceFeature extractFeature(Bitmap bitmap) {
        Session session = FaceEngine.createRecognitionSession();
        assertNotNull(session);
        try {
            ImageStream stream = InspireFace.CreateImageStreamFromBitmap(bitmap, InspireFace.CAMERA_ROTATION_0);
            assertNotNull(stream);
            try {
                MultipleFaceData faces = InspireFace.ExecuteFaceTrack(session, stream);
                assertNotNull(faces);
                assertTrue(faces.detectedNum > 0);
                FaceFeature feature = InspireFace.ExtractFaceFeature(session, stream, faces.tokens[0]);
                assertNotNull(feature);
                assertEquals(InspireFace.GetFeatureLength(), feature.data.length);
                return feature;
            } finally {
                InspireFace.ReleaseImageStream(stream);
            }
        } finally {
            FaceEngine.releaseSession(session);
        }
    }

    private static final class IsolatedStorage extends ContextWrapper {
        private final String prefix = "sdk-post1-" + UUID.randomUUID() + "-";
        private final Set<String> preferences = new HashSet<>();
        private final File directory;

        IsolatedStorage(Context context) {
            super(context);
            directory = new File(context.getCacheDir(), prefix);
            assertTrue(directory.mkdirs());
        }

        @Override public Context getApplicationContext() { return this; }
        @Override public File getFilesDir() { return directory; }

        @Override public SharedPreferences getSharedPreferences(String name, int mode) {
            String isolatedName = prefix + name;
            preferences.add(isolatedName);
            return getBaseContext().getSharedPreferences(isolatedName, mode);
        }

        void clear() {
            for (String name : preferences) getBaseContext().deleteSharedPreferences(name);
            delete(directory);
        }

        private static void delete(File file) {
            File[] children = file.listFiles();
            if (children != null) for (File child : children) delete(child);
            assertTrue("Remove test storage " + file, !file.exists() || file.delete());
        }
    }
}
