/**
 * @author Jingyu Yan
 * @date 2024-11-26
 */

#ifdef ANDROID

#include <jni.h>
#include <string>
#include <android/log.h>
#include <stdlib.h>
#include <android/bitmap.h>
#include "../common/common.h"
#include "c_api/inspireface.h"
#include "log.h"

#define INSPIRE_FACE_JNI(sig) Java_com_insightface_sdk_inspireface_##sig

extern "C" {

JNIEXPORT jboolean INSPIRE_FACE_JNI(InspireFace_GlobalLaunch)(JNIEnv *env, jobject thiz, jstring resourcePath) {
    std::string path = jstring2str(env, resourcePath);
    auto result = HFLaunchInspireFace(path.c_str());
    if (result != 0) {
        INSPIRE_LOGE("Failed to launch InspireFace, error code: %d", result);
        return false;
    }
    return true;
}

JNIEXPORT jboolean INSPIRE_FACE_JNI(InspireFace_GlobalTerminate)(JNIEnv *env, jobject thiz) {
    auto result = HFTerminateInspireFace();
    if (result != 0) {
        INSPIRE_LOGE("Failed to terminate InspireFace, error code: %d", result);
        return false;
    }
    return true;
}

JNIEXPORT jobject INSPIRE_FACE_JNI(InspireFace_CreateSession)(JNIEnv *env, jobject thiz, jobject customParameter) {
    // Get CustomParameter class and fields
    jclass customParamClass = env->GetObjectClass(customParameter);
    jfieldID enableRecognitionField = env->GetFieldID(customParamClass, "enableRecognition", "I");
    jfieldID enableLivenessField = env->GetFieldID(customParamClass, "enableLiveness", "I");
    jfieldID enableIrLivenessField = env->GetFieldID(customParamClass, "enableIrLiveness", "I");
    jfieldID enableMaskDetectField = env->GetFieldID(customParamClass, "enableMaskDetect", "I");
    jfieldID enableFaceQualityField = env->GetFieldID(customParamClass, "enableFaceQuality", "I");
    jfieldID enableFaceAttributeField = env->GetFieldID(customParamClass, "enableFaceAttribute", "I");
    jfieldID enableInteractionLivenessField = env->GetFieldID(customParamClass, "enableInteractionLiveness", "I");

    // Create HFSessionCustomParameter struct
    HFSessionCustomParameter parameter;
    parameter.enable_recognition = env->GetIntField(customParameter, enableRecognitionField);
    parameter.enable_liveness = env->GetIntField(customParameter, enableLivenessField);
    parameter.enable_ir_liveness = env->GetIntField(customParameter, enableIrLivenessField);
    parameter.enable_mask_detect = env->GetIntField(customParameter, enableMaskDetectField);
    parameter.enable_face_quality = env->GetIntField(customParameter, enableFaceQualityField);
    parameter.enable_face_attribute = env->GetIntField(customParameter, enableFaceAttributeField);
    parameter.enable_interaction_liveness = env->GetIntField(customParameter, enableInteractionLivenessField);

    // Create session
    HFSession handle;
    HResult result = HFCreateInspireFaceSession(parameter, HF_DETECT_MODE_LIGHT_TRACK, 1, -1, -1, &handle);

    if (result != 0) {
        INSPIRE_LOGE("Failed to create session, error code: %d", result);
        return nullptr;
    }

    // Create Session object
    jclass sessionClass = env->FindClass("com/insightface/sdk/inspireface/base/Session");
    jmethodID constructor = env->GetMethodID(sessionClass, "<init>", "()V");
    jobject session = env->NewObject(sessionClass, constructor);

    // Set handle
    jfieldID handleField = env->GetFieldID(sessionClass, "handle", "J");
    env->SetLongField(session, handleField, (jlong)handle);

    return session;
}

JNIEXPORT jobject INSPIRE_FACE_JNI(InspireFace_ExecuteFaceTrack)(JNIEnv *env, jobject thiz, jobject session, jobject streamHandle) {
    // Get session handle
    jclass sessionClass = env->GetObjectClass(session);
    jfieldID handleField = env->GetFieldID(sessionClass, "handle", "J");
    HFSession sessionHandle = (HFSession)env->GetLongField(session, handleField);

    // Get stream handle
    jclass streamClass = env->GetObjectClass(streamHandle);
    jfieldID streamHandleField = env->GetFieldID(streamClass, "handle", "J");
    HFImageStream imageStreamHandle = (HFImageStream)env->GetLongField(streamHandle, streamHandleField);

    // Execute face track
    HFMultipleFaceData results;
    HResult result = HFExecuteFaceTrack(sessionHandle, imageStreamHandle, &results);
    if (result != 0) {
        INSPIRE_LOGE("Failed to execute face track, error code: %d", result);
        return nullptr;
    }

    // Create MultipleFaceData object
    jclass multipleFaceDataClass = env->FindClass("com/insightface/sdk/inspireface/base/MultipleFaceData");
    jmethodID constructor = env->GetMethodID(multipleFaceDataClass, "<init>", "()V");
    jobject multipleFaceData = env->NewObject(multipleFaceDataClass, constructor);

    // Set detected number
    jfieldID detectedNumField = env->GetFieldID(multipleFaceDataClass, "detectedNum", "I");
    env->SetIntField(multipleFaceData, detectedNumField, results.detectedNum);

    if (results.detectedNum > 0) {
        // Create and set face rects array
        jclass faceRectClass = env->FindClass("com/insightface/sdk/inspireface/base/FaceRect");
        jobjectArray rectArray = env->NewObjectArray(results.detectedNum, faceRectClass, nullptr);
        jmethodID rectConstructor = env->GetMethodID(faceRectClass, "<init>", "()V");
        jfieldID xField = env->GetFieldID(faceRectClass, "x", "I");
        jfieldID yField = env->GetFieldID(faceRectClass, "y", "I");
        jfieldID widthField = env->GetFieldID(faceRectClass, "width", "I");
        jfieldID heightField = env->GetFieldID(faceRectClass, "height", "I");

        // Create and set track IDs array
        jintArray trackIdsArray = env->NewIntArray(results.detectedNum);
        env->SetIntArrayRegion(trackIdsArray, 0, results.detectedNum, results.trackIds);

        // Create and set detection confidence array
        jfloatArray detConfArray = env->NewFloatArray(results.detectedNum);
        env->SetFloatArrayRegion(detConfArray, 0, results.detectedNum, results.detConfidence);

        // Create and set angles array
        jclass angleClass = env->FindClass("com/insightface/sdk/inspireface/base/FaceEulerAngle");
        jobjectArray angleArray = env->NewObjectArray(results.detectedNum, angleClass, nullptr);
        jmethodID angleConstructor = env->GetMethodID(angleClass, "<init>", "()V");
        jfieldID rollField = env->GetFieldID(angleClass, "roll", "F");
        jfieldID yawField = env->GetFieldID(angleClass, "yaw", "F");
        jfieldID pitchField = env->GetFieldID(angleClass, "pitch", "F");

        // Create and set tokens array
        jclass tokenClass = env->FindClass("com/insightface/sdk/inspireface/base/FaceBasicToken");
        jobjectArray tokenArray = env->NewObjectArray(results.detectedNum, tokenClass, nullptr);
        jmethodID tokenConstructor = env->GetMethodID(tokenClass, "<init>", "()V");
        jfieldID tokenHandleField = env->GetFieldID(tokenClass, "handle", "J");
        jfieldID sizeField = env->GetFieldID(tokenClass, "size", "I");

        for (int i = 0; i < results.detectedNum; i++) {
            // Set face rect
            jobject rect = env->NewObject(faceRectClass, rectConstructor);
            env->SetIntField(rect, xField, results.rects[i].x);
            env->SetIntField(rect, yField, results.rects[i].y);
            env->SetIntField(rect, widthField, results.rects[i].width);
            env->SetIntField(rect, heightField, results.rects[i].height);
            env->SetObjectArrayElement(rectArray, i, rect);

            // Set angle
            jobject angle = env->NewObject(angleClass, angleConstructor);
            env->SetFloatField(angle, rollField, *results.angles.roll);
            env->SetFloatField(angle, yawField, *results.angles.yaw);
            env->SetFloatField(angle, pitchField, *results.angles.pitch);
            env->SetObjectArrayElement(angleArray, i, angle);

            // Set token
            jobject token = env->NewObject(tokenClass, tokenConstructor);
            env->SetLongField(token, tokenHandleField, (jlong)results.tokens[i].data);
            env->SetIntField(token, sizeField, results.tokens[i].size);
            env->SetObjectArrayElement(tokenArray, i, token);
        }

        // Set arrays to MultipleFaceData
        jfieldID rectsField = env->GetFieldID(multipleFaceDataClass, "rects", "[Lcom/insightface/sdk/inspireface/base/FaceRect;");
        jfieldID trackIdsField = env->GetFieldID(multipleFaceDataClass, "trackIds", "[I");
        jfieldID detConfidenceField = env->GetFieldID(multipleFaceDataClass, "detConfidence", "[F");
        jfieldID anglesField = env->GetFieldID(multipleFaceDataClass, "angles", "[Lcom/insightface/sdk/inspireface/base/FaceEulerAngle;");
        jfieldID tokensField = env->GetFieldID(multipleFaceDataClass, "tokens", "[Lcom/insightface/sdk/inspireface/base/FaceBasicToken;");

        env->SetObjectField(multipleFaceData, rectsField, rectArray);
        env->SetObjectField(multipleFaceData, trackIdsField, trackIdsArray);
        env->SetObjectField(multipleFaceData, detConfidenceField, detConfArray);
        env->SetObjectField(multipleFaceData, anglesField, angleArray);
        env->SetObjectField(multipleFaceData, tokensField, tokenArray);
    }

    return multipleFaceData;
}

}  // extern "C"

#endif
