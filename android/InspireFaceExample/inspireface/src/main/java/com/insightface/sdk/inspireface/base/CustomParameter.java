package com.insightface.sdk.inspireface.base;


public class CustomParameter {
    public int enableRecognition = 0;
    public int enableLiveness = 0;
    public int enableIrLiveness = 0;
    public int enableMaskDetect = 0;
    public int enableFaceQuality = 0;
    public int enableFaceAttribute = 0;
    public int enableInteractionLiveness = 0;

    public CustomParameter() {}


    public CustomParameter enableFaceAttribute(boolean enable) {
        this.enableFaceAttribute = enable ? 1 : 0;
        return this;
    }

    public CustomParameter enableInteractionLiveness(boolean enable) {
        this.enableInteractionLiveness = enable ? 1 : 0;
        return this;
    }   
    
    public CustomParameter enableMaskDetect(boolean enable) {
        this.enableMaskDetect = enable ? 1 : 0;
        return this;
    }

    public CustomParameter enableFaceQuality(boolean enable) {
        this.enableFaceQuality = enable ? 1 : 0;
        return this;
    }

    public CustomParameter enableLiveness(boolean enable) {
        this.enableLiveness = enable ? 1 : 0;
        return this;
    }

    public CustomParameter enableRecognition(boolean enable) {
        this.enableRecognition = enable ? 1 : 0;
        return this;
    }

    public CustomParameter enableIrLiveness(boolean enable) {
        this.enableIrLiveness = enable ? 1 : 0;
        return this;
    }

    public CustomParameter setEnableInteractionLiveness(int enable) {
        this.enableInteractionLiveness = enable;
        return this;
    }
}
