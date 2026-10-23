package com.example.inspireface_example.sdk;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertSame;

import com.insightface.sdk.inspireface.base.CustomParameter;
import com.insightface.sdk.inspireface.base.FaceBasicToken;
import com.insightface.sdk.inspireface.base.FaceFeature;
import com.insightface.sdk.inspireface.base.FaceFeatureIdentity;
import com.insightface.sdk.inspireface.base.MultipleFaceData;
import com.insightface.sdk.inspireface.base.SearchTopKResults;

import org.junit.Test;

/** Verifies the consumed AAR's facade data contract without loading Android native libraries. */
public final class JniDataContractTest {
    @Test
    public void faceTokensAndTrackCountsMatchNativeArrayContract() throws Exception {
        assertEquals(byte[].class, FaceBasicToken.class.getField("data").getType());
        assertEquals(int.class, FaceBasicToken.class.getField("size").getType());
        assertEquals(FaceBasicToken[].class, MultipleFaceData.class.getField("tokens").getType());
        assertEquals(int[].class, MultipleFaceData.class.getField("trackIds").getType());
        assertEquals(int[].class, MultipleFaceData.class.getField("trackCounts").getType());
        FaceBasicToken.class.getConstructor();
        MultipleFaceData.class.getConstructor();
    }

    @Test
    public void sessionAndPipelineOptionsMatchNativeIntegerContract() throws Exception {
        for (String field : new String[]{
                "enableRecognition", "enableLiveness", "enableIrLiveness", "enableMaskDetect",
                "enableFaceQuality", "enableFaceAttribute", "enableInteractionLiveness",
                "enableDetectModeLandmark", "enableFacePose", "enableFaceEmotion"}) {
            assertEquals(field, int.class, CustomParameter.class.getField(field).getType());
        }
    }

    @Test
    public void legacyLandmarkOptionIsIndependentOfPoseAndEmotion() {
        CustomParameter options = new CustomParameter().enableFacePose(true).enableFaceEmotion(true);
        options.enableDetectModeLandmark(true).enableDetectModeLandmark(false);
        assertEquals(0, options.enableDetectModeLandmark);
        assertEquals(1, options.enableFacePose);
        assertEquals(1, options.enableFaceEmotion);
    }

    @Test
    public void featureHubPreservesLongIdentityAndNoMatchSentinel() throws Exception {
        assertEquals(long.class, FaceFeatureIdentity.class.getField("id").getType());
        assertEquals(long[].class, SearchTopKResults.class.getField("ids").getType());
        assertEquals(-1L, new FaceFeatureIdentity().id);
        long id = (1L << 40) + 23;
        FaceFeature feature = new FaceFeature();
        FaceFeatureIdentity identity = FaceFeatureIdentity.create(id, feature);
        assertEquals(id, identity.id);
        assertSame(feature, identity.feature);
    }
}
