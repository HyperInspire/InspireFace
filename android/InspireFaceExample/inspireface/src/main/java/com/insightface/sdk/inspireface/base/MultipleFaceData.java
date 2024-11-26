package com.insightface.sdk.inspireface.base;

public class MultipleFaceData {
    public int detectedNum;
    public FaceRect[] rects;
    public int[] trackIds;
    public float[] detConfidence;
    public FaceEulerAngle[] angles;
    public FaceBasicToken[] tokens;

    MultipleFaceData() {}
}
