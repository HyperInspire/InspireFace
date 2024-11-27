package com.insightface.sdk.inspireface.base;

public class FaceFeatureIdentity {
    public int id = -1;
    public FaceFeature feature;
    public float searchConfidence = -1.0f;


    public static FaceFeatureIdentity create(int id, FaceFeature feature) {
        FaceFeatureIdentity identity = new FaceFeatureIdentity();
        identity.id = id;
        identity.feature = feature;
        return identity;
    }
}
