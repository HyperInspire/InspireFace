package com.insightface.sdk.inspireface.base;

public class FaceFeatureIdentity {
    public long id = -1;
    public FaceFeature feature;
    public float searchConfidence = -1.0f;


    public static FaceFeatureIdentity create(long id, FaceFeature feature) {
        FaceFeatureIdentity identity = new FaceFeatureIdentity();
        identity.id = id;
        identity.feature = feature;
        return identity;
    }
}
