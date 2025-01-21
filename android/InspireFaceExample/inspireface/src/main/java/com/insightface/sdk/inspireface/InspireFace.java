package com.insightface.sdk.inspireface;

import android.content.Context;
import android.graphics.Bitmap;

import com.insightface.sdk.inspireface.base.CustomParameter;
import com.insightface.sdk.inspireface.base.FaceAttributeResult;
import com.insightface.sdk.inspireface.base.FaceBasicToken;
import com.insightface.sdk.inspireface.base.FaceFeature;
import com.insightface.sdk.inspireface.base.FaceFeatureIdentity;
import com.insightface.sdk.inspireface.base.FaceInteractionState;
import com.insightface.sdk.inspireface.base.FaceInteractionsActions;
import com.insightface.sdk.inspireface.base.FaceMaskConfidence;
import com.insightface.sdk.inspireface.base.FaceQualityConfidence;
import com.insightface.sdk.inspireface.base.FeatureHubConfiguration;
import com.insightface.sdk.inspireface.base.ImageStream;
import com.insightface.sdk.inspireface.base.InspireFaceVersion;
import com.insightface.sdk.inspireface.base.MultipleFaceData;
import com.insightface.sdk.inspireface.base.Point2f;
import com.insightface.sdk.inspireface.base.RGBLivenessConfidence;
import com.insightface.sdk.inspireface.base.SearchTopKResults;
import com.insightface.sdk.inspireface.base.Session;
import com.insightface.sdk.inspireface.base.SimilarityConverterConfig;
import com.insightface.sdk.inspireface.base.TypeDefine;
import com.insightface.sdk.inspireface.utils.SDKUtils;

public class InspireFace extends TypeDefine {
    static {
        System.loadLibrary("InspireFace");
    }

    /**
     * Copy resource file to application directory
     * @param context Context object
     * @return resource file path
     */
    public static String copyResourceFileToApplicationDir(Context context) {
        String dest = context.getExternalFilesDir(null).getAbsolutePath() + "/inspireface";
        String resourceFolderPath = context.getExternalFilesDir(null).getAbsolutePath() + "/inspireface";
        SDKUtils.copyFilesFromAssets(context, "inspireface", resourceFolderPath);
        return dest;
    }

    /**
     * Create FeatureHubConfiguration object
     * @return FeatureHubConfiguration object
     */
    public static FeatureHubConfiguration CreateFeatureHubConfiguration() {
        return new FeatureHubConfiguration();
    }

    /**
     * Create FeatureHubConfiguration object
     * @param primaryKeyMode Primary key mode(The id increment mode is recommended)
     * @param enablePersistence Flag to enable or disable the use of the database
     * @param persistenceDbPath Path to the database file
     * @param searchThreshold Threshold for face search
     * @param searchMode Mode of face search
     * @return FeatureHubConfiguration object
     */
    public static FeatureHubConfiguration CreateFeatureHubConfiguration(int primaryKeyMode, boolean enablePersistence, String persistenceDbPath, float searchThreshold, int searchMode) {
        FeatureHubConfiguration configuration = new FeatureHubConfiguration();
        configuration.primaryKeyMode = primaryKeyMode;
        configuration.enablePersistence = enablePersistence ? 1 : 0;
        configuration.persistenceDbPath = persistenceDbPath;
        configuration.searchThreshold = searchThreshold;
        configuration.searchMode = searchMode;
        return configuration;
    }

    /**
     * Create CustomParameter object
     * @return CustomParameter object
     */
    public static CustomParameter CreateCustomParameter() {
        return new CustomParameter();
    }

    /**
     * Create CustomParameter object
     * @param enableRecognition Flag to enable or disable face recognition
     * @param enableLiveness Flag to enable or disable liveness detection
     * @param enableIrLiveness Flag to enable or disable IR liveness detection
     * @param enableMaskDetect Flag to enable or disable mask detection
     * @param enableFaceQuality Flag to enable or disable face quality detection
     * @param enableFaceAttribute Flag to enable or disable face attribute detection
     * @param enableInteractionLiveness Flag to enable or disable interaction liveness detection
     * @return CustomParameter object
     */
    public static CustomParameter CreateCustomParameter(boolean enableRecognition, boolean enableLiveness, boolean enableIrLiveness, boolean enableMaskDetect, boolean enableFaceQuality, boolean enableFaceAttribute, boolean enableInteractionLiveness) {
        CustomParameter parameter = new CustomParameter();
        parameter.enableRecognition = enableRecognition ? 1 : 0;
        parameter.enableLiveness = enableLiveness ? 1 : 0;
        parameter.enableIrLiveness = enableIrLiveness ? 1 : 0;
        parameter.enableMaskDetect = enableMaskDetect ? 1 : 0;
        parameter.enableFaceQuality = enableFaceQuality ? 1 : 0;
        parameter.enableFaceAttribute = enableFaceAttribute ? 1 : 0;
        parameter.enableInteractionLiveness = enableInteractionLiveness ? 1 : 0;
        return parameter;
    }

    /**
     * Global launch
     * @param resourcePath
     * @return true if success, false otherwise
     */
    public static native boolean GlobalLaunch(String resourcePath);

    /**
     * Global terminate
     * @return true if success, false otherwise
     */
    public static native boolean GlobalTerminate();

    /**
     * Create session
     * @param customParameter Custom parameter object
     * @param detectMode Detect mode
     * @param maxDetectFaceNum Maximum number of detected faces
     * @param detectPixelLevel Detect pixel level
     * @param trackByDetectModeFPS Track by detect mode FPS
     * @return Session object
     */
    public static native Session CreateSession(CustomParameter customParameter, int detectMode, int maxDetectFaceNum, int detectPixelLevel, int trackByDetectModeFPS);

    /**
     * Release session
     * @param session Session object
     */
    public static native void ReleaseSession(Session session);

    /**
     * Create ImageStream from Bitmap
     * @param bitmap Bitmap object
     * @param rotation Rotation
     * @return ImageStream object
     */
    public static native ImageStream CreateImageStreamFromBitmap(Bitmap bitmap, int rotation);

    /**
     * Create ImageStream from ByteBuffer
     * @param buffer ByteBuffer object
     * @param width Image width
     * @param height Image height
     * @param format Image format
     * @param rotation Image rotation
     * @return ImageStream object
     */
    public static native ImageStream CreateImageStreamFromByteBuffer(byte[] buffer, int width, int height, int format, int rotation);

    /**
     * Write ImageStream to file
     * @param imageStream ImageStream object
     * @param filePath Write sdcard path
     */
    public static native void WriteImageStreamToFile(ImageStream imageStream, String filePath);

    /**
     * Release ImageStream
     * @param imageStream ImageStream object
     */
    public static native void ReleaseImageStream(ImageStream imageStream);

    /**
     * Execute face track
     * @param session Session object
     * @param imageStream ImageStream object
     * @return MultipleFaceData object
     */
    public static native MultipleFaceData ExecuteFaceTrack(Session session, ImageStream imageStream);
    
    /**
     * Get face dense landmark from face token
     * @param token FaceBasicToken object
     * @return Point2f array
     */
    public static native Point2f[] GetFaceDenseLandmarkFromFaceToken(FaceBasicToken token);

    /**
     * Extract face feature
     * @param session
     * @param imageStream ImageStream object
     * @param token FaceBasicToken object
     * @return FaceFeature object
     */
    public static native FaceFeature ExtractFaceFeature(Session session, ImageStream imageStream, FaceBasicToken token);

    /**
     * Get face alignment image
     * @param session
     * @param imageStream ImageStream object
     * @param token FaceBasicToken object
     * @return Bitmap object
     */
    public static native Bitmap GetFaceAlignmentImage(Session session, ImageStream imageStream, FaceBasicToken token);

    /**
     * Set track preview size
     * @param session Session object
     * @param previewSize The size of the preview for detection and tracking
     */
    public static native void SetTrackPreviewSize(Session session, int previewSize);

    /**
     * Set filter minimum face pixel size
     * @param session Session object
     * @param minSize Minimum face pixel size
     */
    public static native void SetFilterMinimumFacePixelSize(Session session, int minSize);

    /**
     * Set face detect threshold
     * @param session Session object
     * @param threshold Face detection threshold, not tracking threshold
     */
    public static native void SetFaceDetectThreshold(Session session, float threshold);

    /**
     * Set track mode smooth ratio, default value is 0.025
     * @param session Session object
     * @param ratio Smooth ratio
     */
    public static native void SetTrackModeSmoothRatio(Session session, float ratio);

    /**
     * Set track mode num smooth cache frame, default value is 15
     * @param session Session object
     * @param num Num smooth cache frame
     */
    public static native void SetTrackModeNumSmoothCacheFrame(Session session, int num);

    /**
     * Set track mode detect interval, default value is 20
     * @param session Session object
     * @param interval Detect interval
     */
    public static native void SetTrackModeDetectInterval(Session session, int interval);

    /**
     * Enable feature hub data
     * @param configuration FeatureHubConfiguration object
     * @return true if success, false otherwise
     */
    public static native boolean FeatureHubDataEnable(FeatureHubConfiguration configuration);

    /**
     * Disable feature hub data
     * @return true if success, false otherwise
     */
    public static native boolean FeatureHubDataDisable();
    
    /**
     * Insert feature to feature hub
     * @param featureIdentity FaceFeatureIdentity object
     * @return true if success, false otherwise
     */
    public static native boolean FeatureHubInsertFeature(FaceFeatureIdentity featureIdentity);

    /**
     * Search face feature from feature hub
     * @param searchFeature FaceFeature object
     * @return FaceFeatureIdentity object
     */
    public static native FaceFeatureIdentity FeatureHubFaceSearch(FaceFeature searchFeature);

    /**
     * Search face feature from feature hub
     * @param searchFeature FaceFeature object
     * @param topK Top K results
     * @return SearchTopKResults object
     */
    public static native SearchTopKResults FeatureHubFaceSearchTopK(FaceFeature searchFeature, int topK);

    /**
     * Remove face feature from feature hub
     * @param id Face identity
     * @return true if success, false otherwise
     */
    public static native boolean FeatureHubFaceRemove(long id);

    /**
     * Update face feature in feature hub
     * @param featureIdentity FaceFeatureIdentity object
     * @return true if success, false otherwise
     */
    public static native boolean FeatureHubFaceUpdate(FaceFeatureIdentity featureIdentity);

    /**
     * Get face identity from feature hub
     * @param id Face identity
     * @return FaceFeatureIdentity object
     */
    public static native FaceFeatureIdentity FeatureHubGetFaceIdentity(long id);

    /**
     * Get face count in feature hub
     * @return Face count
     */
    public static native int FeatureHubGetFaceCount();

    /**
     * Set face search threshold in feature hub
     * @param threshold
     */
    public static native void FeatureHubFaceSearchThresholdSetting(float threshold);

    /**
     * Get feature length
     * @return Feature length, default is 512
     */
    public static native int GetFeatureLength();


    /**
     * Get cosine similarity converter
     * @return SimilarityConverterConfig object
     */
    public static native SimilarityConverterConfig GetCosineSimilarityConverter();

    /**
     * Update cosine similarity converter
     * @param config SimilarityConverterConfig object
     */
    public static native void UpdateCosineSimilarityConverter(SimilarityConverterConfig config);

    /**
     * Convert cosine similarity to percentage
     * @param similarity Cosine similarity
     * @return Percentage similarity
     */
    public static native float CosineSimilarityConvertToPercentage(float similarity);

    /**
     * Face comparison
     * @param feature1 FaceFeature object
     * @param feature2 FaceFeature object
     * @return Comparison result
     */
    public static native float FaceComparison(FaceFeature feature1, FaceFeature feature2);

    /**
     * Multiple face pipeline process
     * @param session Session object
     * @param imageStream ImageStream object
     * @param faces MultipleFaceData object
     * @param customParameter CustomParameter object
     * @return true if success, false otherwise
     */
    public static native boolean MultipleFacePipelineProcess(Session session, ImageStream imageStream, MultipleFaceData faces, CustomParameter customParameter);

    /**
     * Get FaceQualityConfidence
     * @param session Session object
     * @return FaceQualityConfidence object
     */
    public static native FaceQualityConfidence GetFaceQualityConfidence(Session session);
    
    /**
     * Get RGBLivenessConfidence
     * @param session Session object
     * @return RGBLivenessConfidence object
     */
    public static native RGBLivenessConfidence GetRGBLivenessConfidence(Session session);

    /**
     * Get FaceQualityConfidence
     * @param session Session object
     * @return FaceQualityConfidence object
     */

    public static native FaceMaskConfidence GetFaceMaskConfidence(Session session);

    /**
     * Get FaceInteractionState
     * @param session Session object
     * @return FaceInteractionState object
     */
    public static native FaceInteractionState GetFaceInteractionStateResult(Session session);

    /**
     * Get FaceInteractionActionsResult
     * @param session Session object
     * @return FaceInteractionsActions object
     */
    public static native FaceInteractionsActions GetFaceInteractionActionsResult(Session session);

    /**
     * Get FaceAttributeResult
     * @param session Session object
     * @return FaceAttributeResult object
     */
    public static native FaceAttributeResult GetFaceAttributeResult(Session session);

    /**
     * Query InspireFace version
     * @return InspireFaceVersion object
     */
    public static native InspireFaceVersion QueryInspireFaceVersion();

    /**
     * Set log level
     * @param level
     */
    public static native void SetLogLevel(int level);
}
