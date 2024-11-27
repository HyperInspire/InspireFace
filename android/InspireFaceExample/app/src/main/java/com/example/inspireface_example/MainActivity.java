package com.example.inspireface_example;

import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.util.Log;

import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

import com.insightface.sdk.inspireface.InspireFace;
import com.insightface.sdk.inspireface.base.CustomParameter;
import com.insightface.sdk.inspireface.base.FaceFeature;
import com.insightface.sdk.inspireface.base.FaceFeatureIdentity;
import com.insightface.sdk.inspireface.base.FeatureHubConfiguration;
import com.insightface.sdk.inspireface.base.ImageStream;
import com.insightface.sdk.inspireface.base.MultipleFaceData;
import com.insightface.sdk.inspireface.base.Point2f;
import com.insightface.sdk.inspireface.base.SearchTopKResults;
import com.insightface.sdk.inspireface.base.Session;
import com.insightface.sdk.inspireface.utils.SDKUtils;

import java.io.IOException;
import java.io.InputStream;

public class MainActivity extends AppCompatActivity {

    private final String TAG = "InspireFace";

    private Bitmap getImageFromAssetsFile(Context context, String fileName) {
        Bitmap image = null;
        AssetManager am = context.getResources().getAssets();
        try {
            InputStream is = am.open(fileName);
            image = BitmapFactory.decodeStream(is);
            is.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
        return image;
    }

    void test() {
        String dbPath = "/storage/emulated/0/Android/data/com.example.inspireface_example/files/f.db";
        FeatureHubConfiguration configuration = new FeatureHubConfiguration();
        configuration.setEnablePersistence(true).setPersistenceDbPath(dbPath).setSearchThreshold(0.42f).setSearchMode(0).setPrimaryKeyMode(0);
        boolean enableStatus = InspireFace.FeatureHubDataEnable(configuration);
        Log.d(TAG, "Enable feature hub data status: " + enableStatus);
        InspireFace.FeatureHubFaceSearchThresholdSetting(0.42f);

        String folder = InspireFace.copyResourceFileToApplicationDir(this);
        Log.d(TAG, "Path: "+ folder);
        boolean launchStatus = InspireFace.GlobalLaunch(folder + "/Pikachu");
        Log.d(TAG, "Launch status: " + launchStatus);
        if (!launchStatus) {
            Log.e(TAG, "Failed to launch InspireFace");
            return;
        }
        CustomParameter parameter = new CustomParameter();
        parameter.enableRecognition(true).enableFaceQuality(true);
        Session session = InspireFace.CreateSession(parameter, 0, 10, -1, -1);
        Log.i(TAG, "session handle: " + session.handle);
        InspireFace.SetTrackPreviewSize(session, 320);
        InspireFace.SetFaceDetectThreshold(session, 0.5f);
        InspireFace.SetFilterMinimumFacePixelSize(session, 0);

        Bitmap img = getImageFromAssetsFile(this, "inspireface/kun.jpg");
        ImageStream stream = InspireFace.CreateImageStreamFromBitmap(img, 0);
        Log.i(TAG, "stream handle: " + stream.handle);
        InspireFace.WriteImageStreamToFile(stream, "/storage/emulated/0/Android/data/com.example.inspireface_example/files/out.jpg");

        MultipleFaceData multipleFaceData = InspireFace.ExecuteFaceTrack(session, stream);
        Log.i(TAG, "Face num: " + multipleFaceData.detectedNum);

        if (multipleFaceData.detectedNum > 0) {
            Point2f[] lmk = InspireFace.GetFaceDenseLandmarkFromFaceToken(multipleFaceData.tokens[0]);
            for (Point2f p : lmk) {
                Log.i(TAG, p.x + ", " + p.y);
            }
            FaceFeature feature = InspireFace.ExtractFaceFeature(session, stream, multipleFaceData.tokens[0]);
            Log.i(TAG, "Feature size: " + feature.data.length);
            String strFt = "";
            for (int i = 0; i < feature.data.length; i++) {
                strFt = strFt + feature.data[i] + ", ";
            }
            Log.i(TAG, strFt);

            for (int i = 0; i < 10; i++) {
                FaceFeatureIdentity identity = FaceFeatureIdentity.create(-1, feature);
                boolean succ = InspireFace.FeatureHubInsertFeature(identity);
                if (succ) {
                    Log.i(TAG, "Allocation ID: " + identity.id);
                }
            }

            FaceFeatureIdentity searched = InspireFace.FeatureHubFaceSearch(feature);
            Log.i(TAG, "Searched id: " + searched.id + ", Confidence: " + searched.searchConfidence);

            SearchTopKResults topKResults = InspireFace.FeatureHubFaceSearchTopK(feature, 10);
            for (int i = 0; i < topKResults.num; i++) {
                Log.i(TAG, "TopK id: " + topKResults.ids[i] + ", Confidence: " + topKResults.confidence[i]);
            }

            FaceFeature newFeature = new FaceFeature();
            Log.i(TAG, "Feature length: " + InspireFace.GetFeatureLength());
            newFeature.data = new float[InspireFace.GetFeatureLength()];
            FaceFeatureIdentity identity = FaceFeatureIdentity.create(8, newFeature);
            boolean updateSucc = InspireFace.FeatureHubFaceUpdate(identity);
            if (updateSucc) {
                Log.i(TAG, "Update feature success: " + 8);
            }
            boolean removeSucc = InspireFace.FeatureHubFaceRemove(4);
            if (removeSucc) {
                Log.i(TAG, "Remove feature success: " + 4);
            }
            SearchTopKResults topkAgn = InspireFace.FeatureHubFaceSearchTopK(feature, 10);
            for (int i = 0; i < topkAgn.num; i++) {
                Log.i(TAG, "Agn TopK id: " + topkAgn.ids[i] + ", Confidence: " + topKResults.confidence[i]);
            }

            FaceFeatureIdentity queryIdentity = InspireFace.FeatureHubGetFaceIdentity(4);
            if (queryIdentity != null) {
                Log.e(TAG, "query id: " + queryIdentity.id);
            }
            queryIdentity = InspireFace.FeatureHubGetFaceIdentity(2);
            if (queryIdentity != null) {
                strFt = "";
                for (int i = 0; i < queryIdentity.feature.data.length; i++) {
                    strFt = strFt + queryIdentity.feature.data[i] + ", ";
                }
                Log.i(TAG, "query id: " + queryIdentity.id);
                Log.i(TAG, strFt);

                float comp = InspireFace.FaceComparison(queryIdentity.feature, feature);
                Log.i(TAG, "Comparison: " + comp);
            }

        }

        int count = InspireFace.FeatureHubGetFaceCount();
        Log.i(TAG, "Face count: " + count);

        Bitmap crop = InspireFace.GetFaceAlignmentImage(session, stream, multipleFaceData.tokens[0]);
        try {
            SDKUtils.saveBitmap("/storage/emulated/0/Android/data/com.example.inspireface_example/files/", "crop", crop);
        } catch (IOException e) {
            throw new RuntimeException(e);
        }

        InspireFace.ReleaseImageStream(stream);
        InspireFace.ReleaseSession(session);


        InspireFace.FeatureHubDataDisable();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        EdgeToEdge.enable(this);
        setContentView(R.layout.activity_main);
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main), (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });

        //

        test();
    }
}