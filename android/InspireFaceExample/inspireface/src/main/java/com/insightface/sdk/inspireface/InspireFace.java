package com.insightface.sdk.inspireface;

import android.content.Context;
import android.graphics.Bitmap;

import com.insightface.sdk.inspireface.base.CustomParameter;
import com.insightface.sdk.inspireface.base.FaceBasicToken;
import com.insightface.sdk.inspireface.base.FaceFeature;
import com.insightface.sdk.inspireface.base.FaceFeatureIdentity;
import com.insightface.sdk.inspireface.base.FeatureHubConfiguration;
import com.insightface.sdk.inspireface.base.ImageStream;
import com.insightface.sdk.inspireface.base.MultipleFaceData;
import com.insightface.sdk.inspireface.base.Point2f;
import com.insightface.sdk.inspireface.base.SearchTopKResults;
import com.insightface.sdk.inspireface.base.Session;
import com.insightface.sdk.inspireface.utils.SDKUtils;

public class InspireFace {


    static {
        System.loadLibrary("InspireFace");
    }

    public static String copyResourceFileToApplicationDir(Context context) {
        String dest = context.getExternalFilesDir(null).getAbsolutePath() + "/inspireface";
        String resourceFolderPath = context.getExternalFilesDir(null).getAbsolutePath() + "/inspireface";
        SDKUtils.copyFilesFromAssets(context, "inspireface", resourceFolderPath);
        return dest;
    }

    public static native boolean GlobalLaunch(String resourcePath);

    public static native boolean GlobalTerminate();

    public static native Session CreateSession(CustomParameter customParameter, int detectMode, int maxDetectFaceNum, int detectPixelLevel, int trackByDetectModeFPS);

    public static native void ReleaseSession(Session session);

    public static native ImageStream CreateImageStreamFromBitmap(Bitmap bitmap, int rotation);

    public static native ImageStream CreateImageStreamFromBuffer(byte[] buffer, int width, int height, int rotation);

    public static native void WriteImageStreamToFile(ImageStream imageStream, String filePath);

    public static native void ReleaseImageStream(ImageStream imageStream);

    public static native MultipleFaceData ExecuteFaceTrack(Session session, ImageStream imageStream);

    public static native Point2f[] GetFaceDenseLandmarkFromFaceToken(FaceBasicToken token);

    public static native FaceFeature ExtractFaceFeature(Session session, ImageStream imageStream, FaceBasicToken token);

    public static native Bitmap GetFaceAlignmentImage(Session session, ImageStream imageStream, FaceBasicToken token);

    public static native void SetTrackPreviewSize(Session session, int previewSize);

    public static native void SetFilterMinimumFacePixelSize(Session session, int minSize);

    public static native void SetFaceDetectThreshold(Session session, float threshold);

    public static native boolean FeatureHubDataEnable(FeatureHubConfiguration configuration);

    public static native boolean FeatureHubDataDisable();
    
    public static native boolean FeatureHubInsertFeature(FaceFeatureIdentity featureIdentity);

    public static native FaceFeatureIdentity FeatureHubFaceSearch(FaceFeature searchFeature);

    public static native SearchTopKResults FeatureHubFaceSearchTopK(FaceFeature searchFeature, int topK);

    public static native boolean FeatureHubFaceRemove(int id);

    public static native boolean FeatureHubFaceUpdate(FaceFeatureIdentity featureIdentity);

    public static native FaceFeatureIdentity FeatureHubGetFaceIdentity(int id);

    public static native int FeatureHubGetFaceCount();

    public static native void FeatureHubFaceSearchThresholdSetting(float threshold);

    public static native int GetFeatureLength();

    public static native float FaceComparison(FaceFeature feature1, FaceFeature feature2);
}
