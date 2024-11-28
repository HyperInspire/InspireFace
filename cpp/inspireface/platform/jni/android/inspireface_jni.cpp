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
#include "herror.h"

#define INSPIRE_FACE_JNI(sig) Java_com_insightface_sdk_inspireface_##sig

extern "C" {

JNIEXPORT jboolean INSPIRE_FACE_JNI(InspireFace_GlobalLaunch)(JNIEnv *env, jobject thiz, jstring resourcePath) {
    std::string path = jstring2str(env, resourcePath);
    auto result = HFLaunchInspireFace(path.c_str());
    if (result != HSUCCEED) {
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

JNIEXPORT jobject INSPIRE_FACE_JNI(InspireFace_CreateSession)(JNIEnv *env, jobject thiz, jobject customParameter, jint detectMode,
                                                              jint maxDetectFaceNum, jint detectPixelLevel, jint trackByDetectModeFPS) {
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
    HResult result =
      HFCreateInspireFaceSession(parameter, (HFDetectMode)detectMode, maxDetectFaceNum, detectPixelLevel, trackByDetectModeFPS, &handle);

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

JNIEXPORT void INSPIRE_FACE_JNI(InspireFace_ReleaseSession)(JNIEnv *env, jobject thiz, jobject session) {
    jclass sessionClass = env->GetObjectClass(session);
    jfieldID handleField = env->GetFieldID(sessionClass, "handle", "J");
    HFSession handle = (HFSession)env->GetLongField(session, handleField);
    auto result = HFReleaseInspireFaceSession(handle);
    if (result != HSUCCEED) {
        INSPIRE_LOGE("Failed to release session, error code: %d", result);
    }
}

JNIEXPORT jobject INSPIRE_FACE_JNI(InspireFace_CreateImageStreamFromBitmap)(JNIEnv *env, jobject thiz, jobject bitmap, jint rotation) {
    AndroidBitmapInfo info;
    void *pixels = nullptr;
    HFImageFormat format = HF_STREAM_RGB;
    if (AndroidBitmap_getInfo(env, bitmap, &info) < 0) {
        INSPIRE_LOGE("Failed to get bitmap info");
        return nullptr;
    }
    if (AndroidBitmap_lockPixels(env, bitmap, &pixels) < 0) {
        INSPIRE_LOGE("Failed to lock pixels");
        return nullptr;
    }
    if (info.format == ANDROID_BITMAP_FORMAT_RGBA_8888) {
        format = HF_STREAM_RGBA;
    } else if (info.format == ANDROID_BITMAP_FORMAT_RGB_565) {
        format = HF_STREAM_RGB;
    } else {
        AndroidBitmap_unlockPixels(env, bitmap);
        INSPIRE_LOGE("Unsupported bitmap format: %d", info.format);
        return nullptr;
    }

    HFImageData imageData;
    imageData.data = (uint8_t *)pixels;
    imageData.width = info.width;
    imageData.height = info.height;
    imageData.format = (HFImageFormat)format;
    imageData.rotation = (HFRotation)rotation;

    HFImageStream streamHandle;
    auto result = HFCreateImageStream(&imageData, &streamHandle);
    if (result != HSUCCEED) {
        INSPIRE_LOGE("Failed to create image stream, error code: %d", result);
        return nullptr;
    }
    AndroidBitmap_unlockPixels(env, bitmap);

    jclass streamClass = env->FindClass("com/insightface/sdk/inspireface/base/ImageStream");
    jmethodID constructor = env->GetMethodID(streamClass, "<init>", "()V");
    jfieldID streamHandleField = env->GetFieldID(streamClass, "handle", "J");
    jobject imageStreamObj = env->NewObject(streamClass, constructor);
    env->SetLongField(imageStreamObj, streamHandleField, (jlong)streamHandle);

    return imageStreamObj;
}

JNIEXPORT jobject INSPIRE_FACE_JNI(InspireFace_CreateImageStreamFromByteBuffer)(JNIEnv *env, jobject thiz, jbyteArray data, jint width, jint height,
                                                                                jint format, jint rotation) {
    // Convert jbyteArray to byte*
    uint8_t *buf = (uint8_t *)env->GetByteArrayElements(data, 0);
    HFImageData imageData;
    imageData.data = buf;
    imageData.width = width;
    imageData.height = height;
    imageData.format = (HFImageFormat)format;
    imageData.rotation = (HFRotation)rotation;

    HFImageStream streamHandle;
    auto result = HFCreateImageStream(&imageData, &streamHandle);
    if (result != HSUCCEED) {
        INSPIRE_LOGE("Failed to create image stream, error code: %d", result);
        return nullptr;
    }

    jclass streamClass = env->FindClass("com/insightface/sdk/inspireface/base/ImageStream");
    jmethodID constructor = env->GetMethodID(streamClass, "<init>", "()V");
    jfieldID streamHandleField = env->GetFieldID(streamClass, "handle", "J");
    jobject imageStreamObj = env->NewObject(streamClass, constructor);
    env->SetLongField(imageStreamObj, streamHandleField, (jlong)streamHandle);

    return imageStreamObj;
}

JNIEXPORT void INSPIRE_FACE_JNI(InspireFace_WriteImageStreamToFile)(JNIEnv *env, jobject thiz, jobject imageStream, jstring filePath) {
    jclass streamClass = env->GetObjectClass(imageStream);
    jfieldID streamHandleField = env->GetFieldID(streamClass, "handle", "J");
    HFImageStream streamHandle = (HFImageStream)env->GetLongField(imageStream, streamHandleField);

    std::string path = jstring2str(env, filePath);
    HFDeBugImageStreamDecodeSave(streamHandle, path.c_str());
}

JNIEXPORT void INSPIRE_FACE_JNI(InspireFace_ReleaseImageStream)(JNIEnv *env, jobject thiz, jobject imageStream) {
    jclass streamClass = env->GetObjectClass(imageStream);
    jfieldID streamHandleField = env->GetFieldID(streamClass, "handle", "J");
    HFImageStream streamHandle = (HFImageStream)env->GetLongField(imageStream, streamHandleField);
    auto result = HFReleaseImageStream(streamHandle);
    if (result != HSUCCEED) {
        INSPIRE_LOGE("Failed to release image stream, error code: %d", result);
    }
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

JNIEXPORT jobjectArray INSPIRE_FACE_JNI(InspireFace_GetFaceDenseLandmarkFromFaceToken)(JNIEnv *env, jobject thiz, jobject token) {
    // Get token handle and size from FaceBasicToken object
    jclass tokenClass = env->GetObjectClass(token);
    jfieldID handleField = env->GetFieldID(tokenClass, "handle", "J");
    jfieldID sizeField = env->GetFieldID(tokenClass, "size", "I");
    jlong handle = env->GetLongField(token, handleField);
    jint size = env->GetIntField(token, sizeField);

    // Get number of landmarks
    int32_t numLandmarks = 0;
    HFGetNumOfFaceDenseLandmark(&numLandmarks);

    // Allocate memory for landmarks
    HPoint2f *landmarks = new HPoint2f[numLandmarks];

    // Create face token struct
    HFFaceBasicToken faceToken;
    faceToken.size = size;
    faceToken.data = reinterpret_cast<void *>(handle);

    // Get landmarks from token
    HResult result = HFGetFaceDenseLandmarkFromFaceToken(faceToken, landmarks, numLandmarks);

    if (result != HSUCCEED) {
        INSPIRE_LOGE("Failed to get face dense landmark from face token, error code: %d", result);
        delete[] landmarks;
        return nullptr;
    }

    // Create Point2f array to return
    jclass point2fClass = env->FindClass("com/insightface/sdk/inspireface/base/Point2f");
    jobjectArray landmarkArray = env->NewObjectArray(numLandmarks, point2fClass, nullptr);
    jmethodID constructor = env->GetMethodID(point2fClass, "<init>", "()V");
    jfieldID xField = env->GetFieldID(point2fClass, "x", "F");
    jfieldID yField = env->GetFieldID(point2fClass, "y", "F");

    // Fill array with landmark points
    for (int i = 0; i < numLandmarks; i++) {
        jobject point = env->NewObject(point2fClass, constructor);
        env->SetFloatField(point, xField, landmarks[i].x);
        env->SetFloatField(point, yField, landmarks[i].y);
        env->SetObjectArrayElement(landmarkArray, i, point);
    }

    delete[] landmarks;
    return landmarkArray;
}

JNIEXPORT jobject INSPIRE_FACE_JNI(InspireFace_ExtractFaceFeature)(JNIEnv *env, jobject thiz, jobject session, jobject streamHandle, jobject token) {
    // Get session handle
    jclass sessionClass = env->GetObjectClass(session);
    jfieldID sessionHandleField = env->GetFieldID(sessionClass, "handle", "J");
    jlong sessionHandle = env->GetLongField(session, sessionHandleField);

    // Get stream handle
    jclass streamClass = env->GetObjectClass(streamHandle);
    jfieldID streamHandleField = env->GetFieldID(streamClass, "handle", "J");
    jlong streamHandleValue = env->GetLongField(streamHandle, streamHandleField);

    // Get token handle and size
    jclass tokenClass = env->GetObjectClass(token);
    jfieldID tokenHandleField = env->GetFieldID(tokenClass, "handle", "J");
    jfieldID tokenSizeField = env->GetFieldID(tokenClass, "size", "I");
    jlong tokenHandle = env->GetLongField(token, tokenHandleField);
    jint tokenSize = env->GetIntField(token, tokenSizeField);

    // Create face token struct
    HFFaceBasicToken faceToken;
    faceToken.size = tokenSize;
    faceToken.data = reinterpret_cast<void *>(tokenHandle);

    // Extract face feature
    HFFaceFeature feature;
    HResult result =
      HFFaceFeatureExtract(reinterpret_cast<HFSession>(sessionHandle), reinterpret_cast<HFImageStream>(streamHandleValue), faceToken, &feature);

    if (result != HSUCCEED) {
        INSPIRE_LOGE("Failed to extract face feature, error code: %d", result);
        return nullptr;
    }

    // Create FaceFeature object to return
    jclass faceFeatureClass = env->FindClass("com/insightface/sdk/inspireface/base/FaceFeature");
    jobject faceFeature = env->NewObject(faceFeatureClass, env->GetMethodID(faceFeatureClass, "<init>", "()V"));

    // Create float array and copy feature data
    jfloatArray dataArray = env->NewFloatArray(feature.size);
    env->SetFloatArrayRegion(dataArray, 0, feature.size, reinterpret_cast<jfloat *>(feature.data));

    // Set data field in FaceFeature object
    jfieldID dataField = env->GetFieldID(faceFeatureClass, "data", "[F");
    env->SetObjectField(faceFeature, dataField, dataArray);

    return faceFeature;
}

JNIEXPORT jobject INSPIRE_FACE_JNI(InspireFace_GetFaceAlignmentImage)(JNIEnv *env, jobject thiz, jobject session, jobject streamHandle,
                                                                      jobject token) {
    // Get session handle
    jclass sessionClass = env->GetObjectClass(session);
    jfieldID sessionHandleField = env->GetFieldID(sessionClass, "handle", "J");
    jlong sessionHandle = env->GetLongField(session, sessionHandleField);

    // Get stream handle
    jclass streamClass = env->GetObjectClass(streamHandle);
    jfieldID streamHandleField = env->GetFieldID(streamClass, "handle", "J");
    jlong streamHandleValue = env->GetLongField(streamHandle, streamHandleField);

    // Get token handle and size
    jclass tokenClass = env->GetObjectClass(token);
    jfieldID tokenHandleField = env->GetFieldID(tokenClass, "handle", "J");
    jfieldID tokenSizeField = env->GetFieldID(tokenClass, "size", "I");
    jlong tokenHandle = env->GetLongField(token, tokenHandleField);
    jint tokenSize = env->GetIntField(token, tokenSizeField);

    // Create face token struct
    HFFaceBasicToken faceToken;
    faceToken.size = tokenSize;
    faceToken.data = reinterpret_cast<void *>(tokenHandle);

    // Get face alignment image
    HFImageBitmap imageBitmap;
    HResult result = HFFaceGetFaceAlignmentImage(reinterpret_cast<HFSession>(sessionHandle), reinterpret_cast<HFImageStream>(streamHandleValue),
                                                 faceToken, &imageBitmap);

    if (result != HSUCCEED) {
        INSPIRE_LOGE("Failed to get face alignment image, error code: %d", result);
        return nullptr;
    }

    // Get image bitmap data
    HFImageBitmapData bitmapData;
    result = HFImageBitmapGetData(imageBitmap, &bitmapData);
    if (result != HSUCCEED) {
        INSPIRE_LOGE("Failed to get image bitmap data, error code: %d", result);
        HFReleaseImageBitmap(imageBitmap);
        return nullptr;
    }

    // Create Android Bitmap
    jclass bitmapClass = env->FindClass("android/graphics/Bitmap");
    jmethodID createBitmapMethod =
      env->GetStaticMethodID(bitmapClass, "createBitmap", "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");

    // Get Bitmap.Config.ARGB_8888
    jclass bitmapConfigClass = env->FindClass("android/graphics/Bitmap$Config");
    jfieldID argb8888FieldID = env->GetStaticFieldID(bitmapConfigClass, "ARGB_8888", "Landroid/graphics/Bitmap$Config;");
    jobject argb8888Obj = env->GetStaticObjectField(bitmapConfigClass, argb8888FieldID);

    // Create bitmap with alignment image dimensions
    jobject bitmap = env->CallStaticObjectMethod(bitmapClass, createBitmapMethod, bitmapData.width, bitmapData.height, argb8888Obj);

    // Copy pixels to bitmap
    AndroidBitmapInfo bitmapInfo;
    void *pixels;
    if (AndroidBitmap_getInfo(env, bitmap, &bitmapInfo) < 0 || AndroidBitmap_lockPixels(env, bitmap, &pixels) < 0) {
        INSPIRE_LOGE("Failed to get bitmap info or lock pixels, error code: %d", result);
        HFReleaseImageBitmap(imageBitmap);
        return nullptr;
    }

    // Convert BGR to ARGB
    uint8_t *src = (uint8_t *)bitmapData.data;
    uint32_t *dst = (uint32_t *)pixels;
    for (int i = 0; i < bitmapData.width * bitmapData.height; i++) {
        // BGR to ARGB conversion
        uint8_t r = src[i * 3 + 2];
        uint8_t g = src[i * 3 + 1];
        uint8_t b = src[i * 3];
        dst[i] = (0xFF << 24) | (b << 16) | (g << 8) | r;
    }

    AndroidBitmap_unlockPixels(env, bitmap);
    HFReleaseImageBitmap(imageBitmap);

    return bitmap;
}

JNIEXPORT void INSPIRE_FACE_JNI(InspireFace_SetTrackPreviewSize)(JNIEnv *env, jobject thiz, jobject session, jint previewSize) {
    jclass sessionClass = env->GetObjectClass(session);
    jfieldID sessionHandleField = env->GetFieldID(sessionClass, "handle", "J");
    jlong sessionHandle = env->GetLongField(session, sessionHandleField);
    auto result = HFSessionSetTrackPreviewSize((HFSession)sessionHandle, previewSize);
    if (result != HSUCCEED) {
        INSPIRE_LOGE("Failed to set track preview size, error code: %d", result);
    }
}

JNIEXPORT void INSPIRE_FACE_JNI(InspireFace_SetFilterMinimumFacePixelSize)(JNIEnv *env, jobject thiz, jobject session, jint minSize) {
    jclass sessionClass = env->GetObjectClass(session);
    jfieldID sessionHandleField = env->GetFieldID(sessionClass, "handle", "J");
    jlong sessionHandle = env->GetLongField(session, sessionHandleField);
    auto result = HFSessionSetFilterMinimumFacePixelSize((HFSession)sessionHandle, minSize);
    if (result != HSUCCEED) {
        INSPIRE_LOGE("Failed to set filter minimum face pixel size, error code: %d", result);
    }
}

JNIEXPORT void INSPIRE_FACE_JNI(InspireFace_SetFaceDetectThreshold)(JNIEnv *env, jobject thiz, jobject session, jfloat threshold) {
    jclass sessionClass = env->GetObjectClass(session);
    jfieldID sessionHandleField = env->GetFieldID(sessionClass, "handle", "J");
    jlong sessionHandle = env->GetLongField(session, sessionHandleField);
    auto result = HFSessionSetFaceDetectThreshold((HFSession)sessionHandle, threshold);
    if (result != HSUCCEED) {
        INSPIRE_LOGE("Failed to set face detect threshold, error code: %d", result);
    }
}
JNIEXPORT jboolean INSPIRE_FACE_JNI(InspireFace_FeatureHubDataEnable)(JNIEnv *env, jobject thiz, jobject configuration) {
    jclass configClass = env->GetObjectClass(configuration);

    jfieldID primaryKeyModeField = env->GetFieldID(configClass, "primaryKeyMode", "I");
    jfieldID enablePersistenceField = env->GetFieldID(configClass, "enablePersistence", "I");
    jfieldID persistenceDbPathField = env->GetFieldID(configClass, "persistenceDbPath", "Ljava/lang/String;");
    jfieldID searchThresholdField = env->GetFieldID(configClass, "searchThreshold", "F");
    jfieldID searchModeField = env->GetFieldID(configClass, "searchMode", "I");
    HFFeatureHubConfiguration config;
    config.primaryKeyMode = (HFPKMode)env->GetIntField(configuration, primaryKeyModeField);
    config.enablePersistence = env->GetIntField(configuration, enablePersistenceField);

    // Add null check for dbPath
    jstring dbPath = (jstring)env->GetObjectField(configuration, persistenceDbPathField);
    if (dbPath != nullptr) {
        const char *nativeDbPath = env->GetStringUTFChars(dbPath, nullptr);
        if (nativeDbPath != nullptr) {
            config.persistenceDbPath = const_cast<char *>(nativeDbPath);
        } else {
            config.persistenceDbPath[0] = '\0';
        }
    } else {
        config.persistenceDbPath[0] = '\0';
    }

    config.searchThreshold = env->GetFloatField(configuration, searchThresholdField);
    config.searchMode = (HFSearchMode)env->GetIntField(configuration, searchModeField);

    // Remove debug logs that might interfere with error handling
    auto result = HFFeatureHubDataEnable(config);
    if (result != HSUCCEED) {
        INSPIRE_LOGE("Failed to enable feature hub data, error code: %d", result);
        return false;
    }

    return true;
}

JNIEXPORT jboolean INSPIRE_FACE_JNI(InspireFace_FeatureHubDataDisable)(JNIEnv *env, jobject thiz) {
    auto result = HFFeatureHubDataDisable();
    if (result != HSUCCEED) {
        INSPIRE_LOGE("Failed to disable feature hub data, error code: %d", result);
        return false;
    }

    return true;
}

JNIEXPORT jboolean INSPIRE_FACE_JNI(InspireFace_FeatureHubInsertFeature)(JNIEnv *env, jobject thiz, jobject newIdentity) {
    // Get FaceFeatureIdentity class and fields
    jclass identityClass = env->GetObjectClass(newIdentity);
    jfieldID idField = env->GetFieldID(identityClass, "id", "I");
    jfieldID featureField = env->GetFieldID(identityClass, "feature", "Lcom/insightface/sdk/inspireface/base/FaceFeature;");

    // Get feature object
    jobject featureObj = env->GetObjectField(newIdentity, featureField);
    if (featureObj == nullptr) {
        INSPIRE_LOGE("Feature object is null");
        return false;
    }

    // Get FaceFeature class and fields
    jclass featureClass = env->GetObjectClass(featureObj);
    jfieldID dataField = env->GetFieldID(featureClass, "data", "[F");

    // Get feature data array
    jfloatArray dataArray = (jfloatArray)env->GetObjectField(featureObj, dataField);
    if (dataArray == nullptr) {
        INSPIRE_LOGE("Feature data array is null");
        return false;
    }

    // Get feature data
    jsize length = env->GetArrayLength(dataArray);
    jfloat *data = env->GetFloatArrayElements(dataArray, nullptr);

    // Create HFFaceFeature
    HFFaceFeature feature;
    feature.data = data;
    feature.size = length;

    // Create HFFaceFeatureIdentity
    HFFaceFeatureIdentity identity;
    identity.id = env->GetIntField(newIdentity, idField);
    identity.feature = &feature;

    // Insert feature and get allocated ID
    int32_t allocId = -1;
    auto result = HFFeatureHubInsertFeature(identity, &allocId);

    // Release array elements
    env->ReleaseFloatArrayElements(dataArray, data, 0);

    if (result != HSUCCEED) {
        INSPIRE_LOGE("Failed to insert feature, error code: %d", result);
        return false;
    }

    // Update ID field with allocated ID
    env->SetIntField(newIdentity, idField, allocId);
    return true;
}

JNIEXPORT jobject INSPIRE_FACE_JNI(InspireFace_FeatureHubFaceSearch)(JNIEnv *env, jobject thiz, jobject searchFeature, jfloat confidence) {
    // Get FaceFeature class and fields
    jclass featureClass = env->GetObjectClass(searchFeature);
    jfieldID dataField = env->GetFieldID(featureClass, "data", "[F");

    // Get feature data array
    jfloatArray dataArray = (jfloatArray)env->GetObjectField(searchFeature, dataField);
    if (dataArray == nullptr) {
        INSPIRE_LOGE("Feature data array is null");
        return nullptr;
    }

    // Get feature data
    jsize length = env->GetArrayLength(dataArray);
    jfloat *data = env->GetFloatArrayElements(dataArray, nullptr);

    // Create HFFaceFeature
    HFFaceFeature feature;
    feature.data = data;
    feature.size = length;

    // Create HFFaceFeatureIdentity to store search result
    float searchConfidence = 0.0f;
    HFFaceFeatureIdentity mostSimilar;
    auto result = HFFeatureHubFaceSearch(feature, &searchConfidence, &mostSimilar);

    // Release array elements
    env->ReleaseFloatArrayElements(dataArray, data, 0);

    if (result != HSUCCEED) {
        INSPIRE_LOGE("Failed to search feature, error code: %d", result);
        return nullptr;
    }

    // Create FaceFeatureIdentity object to return
    jclass identityClass = env->FindClass("com/insightface/sdk/inspireface/base/FaceFeatureIdentity");
    jmethodID constructor = env->GetMethodID(identityClass, "<init>", "()V");
    jobject identityObj = env->NewObject(identityClass, constructor);

    // Set id field
    jfieldID idField = env->GetFieldID(identityClass, "id", "I");
    env->SetIntField(identityObj, idField, mostSimilar.id);

    // Create FaceFeature object and set data
    jclass faceFeatureClass = env->FindClass("com/insightface/sdk/inspireface/base/FaceFeature");
    jmethodID featureConstructor = env->GetMethodID(faceFeatureClass, "<init>", "()V");
    jobject featureObj = env->NewObject(faceFeatureClass, featureConstructor);

    // Create and set float array for feature data
    jfloatArray featureArray = env->NewFloatArray(mostSimilar.feature->size);
    env->SetFloatArrayRegion(featureArray, 0, mostSimilar.feature->size, mostSimilar.feature->data);
    env->SetObjectField(featureObj, dataField, featureArray);

    // Set feature field in identity object
    jfieldID featureField = env->GetFieldID(identityClass, "feature", "Lcom/insightface/sdk/inspireface/base/FaceFeature;");
    env->SetObjectField(identityObj, featureField, featureObj);

    // Set searchConfidence field
    jfieldID searchConfidenceField = env->GetFieldID(identityClass, "searchConfidence", "F");
    env->SetFloatField(identityObj, searchConfidenceField, searchConfidence);

    return identityObj;
}

JNIEXPORT jobject INSPIRE_FACE_JNI(InspireFace_FeatureHubFaceSearchTopK)(JNIEnv *env, jobject thiz, jobject searchFeature, jint topK) {
    // Get feature data array
    jclass faceFeatureClass = env->GetObjectClass(searchFeature);
    jfieldID dataField = env->GetFieldID(faceFeatureClass, "data", "[F");
    jfloatArray dataArray = (jfloatArray)env->GetObjectField(searchFeature, dataField);
    jfloat *data = env->GetFloatArrayElements(dataArray, 0);
    jsize length = env->GetArrayLength(dataArray);

    // Create HFFaceFeature
    HFFaceFeature feature;
    feature.data = data;
    feature.size = length;

    // Create HFSearchTopKResults to store search results
    HFSearchTopKResults results;
    auto result = HFFeatureHubFaceSearchTopK(feature, topK, &results);

    // Release array elements
    env->ReleaseFloatArrayElements(dataArray, data, 0);

    if (result != HSUCCEED) {
        INSPIRE_LOGE("Failed to search top-k features, error code: %d", result);
        return nullptr;
    }

    // Create SearchTopKResults object to return
    jclass searchResultsClass = env->FindClass("com/insightface/sdk/inspireface/base/SearchTopKResults");
    jmethodID constructor = env->GetMethodID(searchResultsClass, "<init>", "()V");
    jobject searchResultsObj = env->NewObject(searchResultsClass, constructor);

    // Set num field
    jfieldID numField = env->GetFieldID(searchResultsClass, "num", "I");
    env->SetIntField(searchResultsObj, numField, results.size);

    // Create and set confidence array
    jfloatArray confidenceArray = env->NewFloatArray(results.size);
    env->SetFloatArrayRegion(confidenceArray, 0, results.size, results.confidence);
    jfieldID confidenceField = env->GetFieldID(searchResultsClass, "confidence", "[F");
    env->SetObjectField(searchResultsObj, confidenceField, confidenceArray);

    // Create and set ids array
    jintArray idsArray = env->NewIntArray(results.size);
    env->SetIntArrayRegion(idsArray, 0, results.size, results.ids);
    jfieldID idsField = env->GetFieldID(searchResultsClass, "ids", "[I");
    env->SetObjectField(searchResultsObj, idsField, idsArray);

    return searchResultsObj;
}

JNIEXPORT jboolean INSPIRE_FACE_JNI(InspireFace_FeatureHubFaceRemove)(JNIEnv *env, jobject thiz, jint id) {
    auto result = HFFeatureHubFaceRemove(id);
    if (result != HSUCCEED) {
        INSPIRE_LOGE("Failed to remove feature, error code: %d", result);
        return false;
    }
    return true;
}

JNIEXPORT jboolean INSPIRE_FACE_JNI(InspireFace_FeatureHubFaceUpdate)(JNIEnv *env, jobject thiz, jobject featureIdentity) {
    // Get FaceFeatureIdentity class and fields
    jclass identityClass = env->GetObjectClass(featureIdentity);
    jfieldID idField = env->GetFieldID(identityClass, "id", "I");
    jfieldID featureField = env->GetFieldID(identityClass, "feature", "Lcom/insightface/sdk/inspireface/base/FaceFeature;");

    // Get id value
    jint id = env->GetIntField(featureIdentity, idField);

    // Get feature object
    jobject featureObj = env->GetObjectField(featureIdentity, featureField);
    jclass featureClass = env->GetObjectClass(featureObj);
    jfieldID dataField = env->GetFieldID(featureClass, "data", "[F");

    // Get feature data array
    jfloatArray dataArray = (jfloatArray)env->GetObjectField(featureObj, dataField);
    jsize length = env->GetArrayLength(dataArray);
    jfloat *data = env->GetFloatArrayElements(dataArray, nullptr);

    // Create HFFaceFeature
    HFFaceFeature feature;
    feature.data = data;
    feature.size = length;

    // Create HFFaceFeatureIdentity
    HFFaceFeatureIdentity identity;
    identity.id = id;
    identity.feature = &feature;

    // Update feature
    auto result = HFFeatureHubFaceUpdate(identity);

    // Release array elements
    env->ReleaseFloatArrayElements(dataArray, data, 0);

    if (result != HSUCCEED) {
        INSPIRE_LOGE("Failed to update feature, error code: %d", result);
        return false;
    }
    return true;
}

JNIEXPORT jobject INSPIRE_FACE_JNI(InspireFace_FeatureHubGetFaceIdentity)(JNIEnv *env, jobject thiz, jint id) {
    // Create HFFaceFeatureIdentity
    HFFaceFeatureIdentity identity;
    auto result = HFFeatureHubGetFaceIdentity(id, &identity);
    if (result != HSUCCEED) {
        INSPIRE_LOGE("Failed to get face identity, error code: %d", result);
        return nullptr;
    }

    // Create FaceFeature object
    jclass featureClass = env->FindClass("com/insightface/sdk/inspireface/base/FaceFeature");
    jmethodID featureConstructor = env->GetMethodID(featureClass, "<init>", "()V");
    jobject featureObj = env->NewObject(featureClass, featureConstructor);

    // Set feature data array
    jfieldID dataField = env->GetFieldID(featureClass, "data", "[F");
    jfloatArray dataArray = env->NewFloatArray(identity.feature->size);
    env->SetFloatArrayRegion(dataArray, 0, identity.feature->size, identity.feature->data);
    env->SetObjectField(featureObj, dataField, dataArray);

    // Create FaceFeatureIdentity object
    jclass identityClass = env->FindClass("com/insightface/sdk/inspireface/base/FaceFeatureIdentity");
    jmethodID identityConstructor = env->GetMethodID(identityClass, "<init>", "()V");
    jobject identityObj = env->NewObject(identityClass, identityConstructor);

    // Set id and feature fields
    jfieldID idField = env->GetFieldID(identityClass, "id", "I");
    jfieldID featureField = env->GetFieldID(identityClass, "feature", "Lcom/insightface/sdk/inspireface/base/FaceFeature;");
    env->SetIntField(identityObj, idField, identity.id);
    env->SetObjectField(identityObj, featureField, featureObj);

    return identityObj;
}

JNIEXPORT jint INSPIRE_FACE_JNI(InspireFace_FeatureHubGetFaceCount)(JNIEnv *env, jobject thiz) {
    HInt32 count;
    auto result = HFFeatureHubGetFaceCount(&count);
    if (result != HSUCCEED) {
        INSPIRE_LOGE("Failed to get face count, error code: %d", result);
        return -1;
    }
    return count;
}

JNIEXPORT void INSPIRE_FACE_JNI(InspireFace_FeatureHubFaceSearchThresholdSetting)(JNIEnv *env, jobject thiz, jfloat threshold) {
    auto result = HFFeatureHubFaceSearchThresholdSetting(threshold);
    if (result != HSUCCEED) {
        INSPIRE_LOGE("Failed to set face search threshold, error code: %d", result);
    }
}

JNIEXPORT jint INSPIRE_FACE_JNI(InspireFace_GetFeatureLength)(JNIEnv *env, jobject thiz) {
    HInt32 length;
    auto result = HFGetFeatureLength(&length);
    return length;
}

JNIEXPORT jfloat INSPIRE_FACE_JNI(InspireFace_FaceComparison)(JNIEnv *env, jobject thiz, jobject feature1, jobject feature2) {
    if (feature1 == nullptr || feature2 == nullptr) {
        return -1.0f;
    }

    // Get FaceFeature class and data field
    jclass featureClass = env->FindClass("com/insightface/sdk/inspireface/base/FaceFeature");
    jfieldID dataField = env->GetFieldID(featureClass, "data", "[F");

    // Get float arrays from FaceFeature objects
    jfloatArray data1Array = (jfloatArray)env->GetObjectField(feature1, dataField);
    jfloatArray data2Array = (jfloatArray)env->GetObjectField(feature2, dataField);

    if (data1Array == nullptr || data2Array == nullptr) {
        return -1.0f;
    }

    // Get array lengths
    jsize len1 = env->GetArrayLength(data1Array);
    jsize len2 = env->GetArrayLength(data2Array);

    if (len1 != len2) {
        return -1.0f;
    }

    // Get float data
    jfloat *data1 = env->GetFloatArrayElements(data1Array, nullptr);
    jfloat *data2 = env->GetFloatArrayElements(data2Array, nullptr);

    // Create HFFaceFeature structs
    HFFaceFeature ft1, ft2;
    ft1.data = data1;
    ft1.size = len1;
    ft2.data = data2;
    ft2.size = len2;

    // Compare features
    float compareResult;
    HResult ret = HFFaceComparison(ft1, ft2, &compareResult);

    // Release arrays
    env->ReleaseFloatArrayElements(data1Array, data1, 0);
    env->ReleaseFloatArrayElements(data2Array, data2, 0);

    if (ret != HSUCCEED) {
        return -1.0f;
    }

    return compareResult;
}

JNIEXPORT jboolean INSPIRE_FACE_JNI(InspireFace_MultipleFacePipelineProcess)(JNIEnv *env, jobject thiz, jobject session, jobject streamHandle,
                                                                             jobject faces, jobject parameter) {
    // Get session handle
    jclass sessionClass = env->FindClass("com/insightface/sdk/inspireface/base/Session");
    jfieldID handleField = env->GetFieldID(sessionClass, "handle", "J");
    jlong sessionHandle = env->GetLongField(session, handleField);

    // Get stream handle
    jclass streamClass = env->FindClass("com/insightface/sdk/inspireface/base/ImageStream");
    handleField = env->GetFieldID(streamClass, "handle", "J");
    jlong streamHandleValue = env->GetLongField(streamHandle, handleField);

    // Get faces data
    jclass facesClass = env->FindClass("com/insightface/sdk/inspireface/base/MultipleFaceData");
    jfieldID numField = env->GetFieldID(facesClass, "detectedNum", "I");
    jfieldID tokensField = env->GetFieldID(facesClass, "tokens", "[Lcom/insightface/sdk/inspireface/base/FaceBasicToken;");

    jint detectedNum = env->GetIntField(faces, numField);
    jobjectArray tokenArray = (jobjectArray)env->GetObjectField(faces, tokensField);

    // Create HFMultipleFaceData struct
    HFMultipleFaceData faceData;
    faceData.detectedNum = detectedNum;

    // Get token data
    HFFaceBasicToken *tokens = nullptr;
    if (detectedNum > 0) {
        jclass tokenClass = env->FindClass("com/insightface/sdk/inspireface/base/FaceBasicToken");
        jfieldID handleTokenField = env->GetFieldID(tokenClass, "handle", "J");
        jfieldID sizeField = env->GetFieldID(tokenClass, "size", "I");

        tokens = new HFFaceBasicToken[detectedNum];
        for (int i = 0; i < detectedNum; i++) {
            jobject token = env->GetObjectArrayElement(tokenArray, i);
            tokens[i].data = (void *)env->GetLongField(token, handleTokenField);
            tokens[i].size = env->GetIntField(token, sizeField);
            env->DeleteLocalRef(token);
        }
        faceData.tokens = tokens;
    } else {
        faceData.tokens = nullptr;
    }

    // Get custom parameter fields
    jclass paramClass = env->FindClass("com/insightface/sdk/inspireface/base/CustomParameter");
    jfieldID enableRecognitionField = env->GetFieldID(paramClass, "enableRecognition", "I");
    jfieldID enableLivenessField = env->GetFieldID(paramClass, "enableLiveness", "I");
    jfieldID enableIrLivenessField = env->GetFieldID(paramClass, "enableIrLiveness", "I");
    jfieldID enableMaskDetectField = env->GetFieldID(paramClass, "enableMaskDetect", "I");
    jfieldID enableFaceQualityField = env->GetFieldID(paramClass, "enableFaceQuality", "I");
    jfieldID enableFaceAttributeField = env->GetFieldID(paramClass, "enableFaceAttribute", "I");
    jfieldID enableInteractionLivenessField = env->GetFieldID(paramClass, "enableInteractionLiveness", "I");

    // Get parameter values
    HFSessionCustomParameter customParam;
    customParam.enable_recognition = env->GetIntField(parameter, enableRecognitionField);
    customParam.enable_liveness = env->GetIntField(parameter, enableLivenessField);
    customParam.enable_ir_liveness = env->GetIntField(parameter, enableIrLivenessField);
    customParam.enable_mask_detect = env->GetIntField(parameter, enableMaskDetectField);
    customParam.enable_face_quality = env->GetIntField(parameter, enableFaceQualityField);
    customParam.enable_face_attribute = env->GetIntField(parameter, enableFaceAttributeField);
    customParam.enable_interaction_liveness = env->GetIntField(parameter, enableInteractionLivenessField);

    // Call native function
    HResult ret = HFMultipleFacePipelineProcess((HFSession)sessionHandle, (HFImageStream)streamHandleValue, &faceData, customParam);

    // Clean up allocated memory
    if (tokens != nullptr) {
        delete[] tokens;
    }

    if (ret != HSUCCEED) {
        INSPIRE_LOGE("Failed to process multiple faces, error code: %d", ret);
        return false;
    }

    return true;
}

JNIEXPORT jobject INSPIRE_FACE_JNI(InspireFace_GetRGBLivenessConfidence)(JNIEnv *env, jobject thiz, jobject session) {
    // Get session handle
    jclass sessionClass = env->FindClass("com/insightface/sdk/inspireface/base/Session");
    jfieldID handleField = env->GetFieldID(sessionClass, "handle", "J");
    jlong sessionHandle = env->GetLongField(session, handleField);

    // Create native confidence struct
    HFRGBLivenessConfidence confidence;
    HResult ret = HFGetRGBLivenessConfidence((HFSession)sessionHandle, &confidence);

    if (ret != HSUCCEED) {
        INSPIRE_LOGE("Failed to get RGB liveness confidence, error code: %d", ret);
        return nullptr;
    }

    // Create Java RGBLivenessConfidence object
    jclass confidenceClass = env->FindClass("com/insightface/sdk/inspireface/base/RGBLivenessConfidence");
    jobject confidenceObj = env->AllocObject(confidenceClass);

    // Set num field
    jfieldID numField = env->GetFieldID(confidenceClass, "num", "I");
    env->SetIntField(confidenceObj, numField, confidence.num);

    // Set confidence array field
    jfieldID confidenceField = env->GetFieldID(confidenceClass, "confidence", "[F");
    jfloatArray confidenceArray = env->NewFloatArray(confidence.num);
    float *confidencePtr = confidence.confidence;
    env->SetFloatArrayRegion(confidenceArray, 0, confidence.num, confidencePtr);
    env->SetObjectField(confidenceObj, confidenceField, confidenceArray);

    return confidenceObj;
}

JNIEXPORT jobject INSPIRE_FACE_JNI(InspireFace_GetFaceQualityConfidence)(JNIEnv *env, jobject thiz, jobject session) {
    // Get session handle
    jclass sessionClass = env->FindClass("com/insightface/sdk/inspireface/base/Session");
    jfieldID handleField = env->GetFieldID(sessionClass, "handle", "J");
    jlong sessionHandle = env->GetLongField(session, handleField);

    // Create native confidence struct
    HFFaceQualityConfidence confidence;
    HResult ret = HFGetFaceQualityConfidence((HFSession)sessionHandle, &confidence);

    if (ret != HSUCCEED) {
        INSPIRE_LOGE("Failed to get face quality confidence, error code: %d", ret);
        return nullptr;
    }

    // Create Java FaceQualityConfidence object
    jclass confidenceClass = env->FindClass("com/insightface/sdk/inspireface/base/FaceQualityConfidence");
    jobject confidenceObj = env->AllocObject(confidenceClass);

    // Set num field
    jfieldID numField = env->GetFieldID(confidenceClass, "num", "I");
    env->SetIntField(confidenceObj, numField, confidence.num);

    // Set confidence array field
    jfieldID confidenceField = env->GetFieldID(confidenceClass, "confidence", "[F");
    jfloatArray confidenceArray = env->NewFloatArray(confidence.num);
    float *confidencePtr = confidence.confidence;
    env->SetFloatArrayRegion(confidenceArray, 0, confidence.num, confidencePtr);
    env->SetObjectField(confidenceObj, confidenceField, confidenceArray);

    return confidenceObj;
}

JNIEXPORT jobject INSPIRE_FACE_JNI(InspireFace_GetFaceMaskConfidence)(JNIEnv *env, jobject thiz, jobject session) {
    // Get session handle
    jclass sessionClass = env->FindClass("com/insightface/sdk/inspireface/base/Session");
    jfieldID handleField = env->GetFieldID(sessionClass, "handle", "J");
    jlong sessionHandle = env->GetLongField(session, handleField);

    // Create native confidence struct
    HFFaceMaskConfidence confidence;
    HResult ret = HFGetFaceMaskConfidence((HFSession)sessionHandle, &confidence);

    if (ret != HSUCCEED) {
        INSPIRE_LOGE("Failed to get face mask confidence, error code: %d", ret);
        return nullptr;
    }

    // Create Java FaceMaskConfidence object
    jclass confidenceClass = env->FindClass("com/insightface/sdk/inspireface/base/FaceMaskConfidence");
    jobject confidenceObj = env->AllocObject(confidenceClass);

    // Set num field
    jfieldID numField = env->GetFieldID(confidenceClass, "num", "I");
    env->SetIntField(confidenceObj, numField, confidence.num);

    // Set confidence array field
    jfieldID confidenceField = env->GetFieldID(confidenceClass, "confidence", "[F");
    jfloatArray confidenceArray = env->NewFloatArray(confidence.num);
    float *confidencePtr = confidence.confidence;
    env->SetFloatArrayRegion(confidenceArray, 0, confidence.num, confidencePtr);
    env->SetObjectField(confidenceObj, confidenceField, confidenceArray);

    return confidenceObj;
}

JNIEXPORT jobject INSPIRE_FACE_JNI(InspireFace_GetFaceInteractionStateResult)(JNIEnv *env, jobject thiz, jobject session) {
    // Get session handle
    jclass sessionClass = env->FindClass("com/insightface/sdk/inspireface/base/Session");
    jfieldID handleField = env->GetFieldID(sessionClass, "handle", "J");
    jlong sessionHandle = env->GetLongField(session, handleField);

    // Create native state struct
    HFFaceInteractionState state;
    HResult ret = HFGetFaceInteractionStateResult((HFSession)sessionHandle, &state);

    if (ret != HSUCCEED) {
        INSPIRE_LOGE("Failed to get face interaction state result, error code: %d", ret);
        return nullptr;
    }

    // Create Java FaceInteractionState object
    jclass stateClass = env->FindClass("com/insightface/sdk/inspireface/base/FaceInteractionState");
    jobject stateObj = env->AllocObject(stateClass);

    // Set num field
    jfieldID numField = env->GetFieldID(stateClass, "num", "I");
    env->SetIntField(stateObj, numField, state.num);

    // Set leftEyeStatusConfidence array field
    jfieldID leftEyeField = env->GetFieldID(stateClass, "leftEyeStatusConfidence", "[F");
    jfloatArray leftEyeArray = env->NewFloatArray(state.num);
    float *leftEyePtr = state.leftEyeStatusConfidence;
    env->SetFloatArrayRegion(leftEyeArray, 0, state.num, leftEyePtr);
    env->SetObjectField(stateObj, leftEyeField, leftEyeArray);

    // Set rightEyeStatusConfidence array field
    jfieldID rightEyeField = env->GetFieldID(stateClass, "rightEyeStatusConfidence", "[F");
    jfloatArray rightEyeArray = env->NewFloatArray(state.num);
    float *rightEyePtr = state.rightEyeStatusConfidence;
    env->SetFloatArrayRegion(rightEyeArray, 0, state.num, rightEyePtr);
    env->SetObjectField(stateObj, rightEyeField, rightEyeArray);

    return stateObj;
}

JNIEXPORT jobject INSPIRE_FACE_JNI(InspireFace_GetFaceInteractionActionsResult)(JNIEnv *env, jobject thiz, jobject session) {
    // Get session handle
    jclass sessionClass = env->FindClass("com/insightface/sdk/inspireface/base/Session");
    jfieldID handleField = env->GetFieldID(sessionClass, "handle", "J");
    jlong sessionHandle = env->GetLongField(session, handleField);

    // Create native actions struct
    HFFaceInteractionsActions actions;
    HResult ret = HFGetFaceInteractionActionsResult((HFSession)sessionHandle, &actions);

    if (ret != HSUCCEED) {
        INSPIRE_LOGE("Failed to get face interaction actions result, error code: %d", ret);
        return nullptr;
    }

    // Create Java FaceInteractionsActions object
    jclass actionsClass = env->FindClass("com/insightface/sdk/inspireface/base/FaceInteractionsActions");
    jobject actionsObj = env->AllocObject(actionsClass);

    // Set num field
    jfieldID numField = env->GetFieldID(actionsClass, "num", "I");
    env->SetIntField(actionsObj, numField, actions.num);

    // Set normal array field
    jfieldID normalField = env->GetFieldID(actionsClass, "normal", "[I");
    jintArray normalArray = env->NewIntArray(actions.num);
    int *normalPtr = actions.normal;
    env->SetIntArrayRegion(normalArray, 0, actions.num, normalPtr);
    env->SetObjectField(actionsObj, normalField, normalArray);

    // Set shake array field
    jfieldID shakeField = env->GetFieldID(actionsClass, "shake", "[I");
    jintArray shakeArray = env->NewIntArray(actions.num);
    int *shakePtr = actions.shake;
    env->SetIntArrayRegion(shakeArray, 0, actions.num, shakePtr);
    env->SetObjectField(actionsObj, shakeField, shakeArray);

    // Set jawOpen array field
    jfieldID jawOpenField = env->GetFieldID(actionsClass, "jawOpen", "[I");
    jintArray jawOpenArray = env->NewIntArray(actions.num);
    int *jawOpenPtr = actions.jawOpen;
    env->SetIntArrayRegion(jawOpenArray, 0, actions.num, jawOpenPtr);
    env->SetObjectField(actionsObj, jawOpenField, jawOpenArray);

    // Set headRaise array field
    jfieldID headRaiseField = env->GetFieldID(actionsClass, "headRaise", "[I");
    jintArray headRaiseArray = env->NewIntArray(actions.num);
    int *headRaisePtr = actions.headRaise;
    env->SetIntArrayRegion(headRaiseArray, 0, actions.num, headRaisePtr);
    env->SetObjectField(actionsObj, headRaiseField, headRaiseArray);

    // Set blink array field
    jfieldID blinkField = env->GetFieldID(actionsClass, "blink", "[I");
    jintArray blinkArray = env->NewIntArray(actions.num);
    int *blinkPtr = actions.blink;
    env->SetIntArrayRegion(blinkArray, 0, actions.num, blinkPtr);
    env->SetObjectField(actionsObj, blinkField, blinkArray);

    return actionsObj;
}

JNIEXPORT jobject INSPIRE_FACE_JNI(InspireFace_GetFaceAttributeResult)(JNIEnv *env, jobject thiz, jobject session) {
    // Validate input parameters
    if (!env || !session) {
        INSPIRE_LOGE("Invalid input parameters");
        return nullptr;
    }

    // Get session handle
    jclass sessionClass = env->GetObjectClass(session);
    jfieldID handleField = env->GetFieldID(sessionClass, "handle", "J");
    jlong sessionHandle = env->GetLongField(session, handleField);
    if (!sessionHandle) {
        INSPIRE_LOGE("Invalid session handle");
        return nullptr;
    }

    // Get face attribute results
    HFFaceAttributeResult results = {};
    HResult ret = HFGetFaceAttributeResult((HFSession)sessionHandle, &results);
    if (ret != HSUCCEED) {
        INSPIRE_LOGE("Failed to get face attribute result, error code: %d", ret);
        return nullptr;
    }

    // Create Java FaceAttributeResult object
    jclass attributeClass = env->FindClass("com/insightface/sdk/inspireface/base/FaceAttributeResult");
    if (!attributeClass) {
        INSPIRE_LOGE("Failed to find FaceAttributeResult class");
        return nullptr;
    }

    jmethodID constructor = env->GetMethodID(attributeClass, "<init>", "()V");
    jobject attributeObj = env->NewObject(attributeClass, constructor);
    if (!attributeObj) {
        INSPIRE_LOGE("Failed to create FaceAttributeResult object");
        return nullptr;
    }

    // Set fields
    jfieldID numField = env->GetFieldID(attributeClass, "num", "I");
    jfieldID raceField = env->GetFieldID(attributeClass, "race", "[I");
    jfieldID genderField = env->GetFieldID(attributeClass, "gender", "[I");
    jfieldID ageBracketField = env->GetFieldID(attributeClass, "ageBracket", "[I");

    if (!numField || !raceField || !genderField || !ageBracketField) {
        INSPIRE_LOGE("Failed to get field IDs");
        return nullptr;
    }

    // Set num
    env->SetIntField(attributeObj, numField, results.num);

    // Set arrays
    jintArray raceArray = env->NewIntArray(results.num);
    jintArray genderArray = env->NewIntArray(results.num);
    jintArray ageBracketArray = env->NewIntArray(results.num);

    if (!raceArray || !genderArray || !ageBracketArray) {
        INSPIRE_LOGE("Failed to create arrays");
        return nullptr;
    }

    env->SetIntArrayRegion(raceArray, 0, results.num, results.race);
    env->SetIntArrayRegion(genderArray, 0, results.num, results.gender);
    env->SetIntArrayRegion(ageBracketArray, 0, results.num, results.ageBracket);

    env->SetObjectField(attributeObj, raceField, raceArray);
    env->SetObjectField(attributeObj, genderField, genderArray);
    env->SetObjectField(attributeObj, ageBracketField, ageBracketArray);

    return attributeObj;
}
JNIEXPORT jobject INSPIRE_FACE_JNI(InspireFace_QueryInspireFaceVersion)(JNIEnv *env, jobject thiz) {
    // Get version info
    HFInspireFaceVersion versionInfo;
    HFQueryInspireFaceVersion(&versionInfo);

    // Create new InspireFaceVersion object
    jclass versionClass = env->FindClass("com/insightface/sdk/inspireface/base/InspireFaceVersion");
    if (!versionClass) {
        INSPIRE_LOGE("Failed to find InspireFaceVersion class");
        return nullptr;
    }

    jmethodID constructor = env->GetMethodID(versionClass, "<init>", "()V");
    jobject version = env->NewObject(versionClass, constructor);
    if (!version) {
        INSPIRE_LOGE("Failed to create InspireFaceVersion object");
        return nullptr;
    }

    // Get field IDs
    jfieldID majorField = env->GetFieldID(versionClass, "major", "I");
    jfieldID minorField = env->GetFieldID(versionClass, "minor", "I");
    jfieldID patchField = env->GetFieldID(versionClass, "patch", "I");
    jfieldID infoField = env->GetFieldID(versionClass, "information", "Ljava/lang/String;");

    if (!majorField || !minorField || !patchField || !infoField) {
        INSPIRE_LOGE("Failed to get InspireFaceVersion field IDs");
        return nullptr;
    }

    // Set version fields
    env->SetIntField(version, majorField, versionInfo.major);
    env->SetIntField(version, minorField, versionInfo.minor);
    env->SetIntField(version, patchField, versionInfo.patch);

    // Get extended information
    HFInspireFaceExtendedInformation extendedInfo;
    HFQueryInspireFaceExtendedInformation(&extendedInfo);

    // Convert C string to Java string and set information field
    jstring infoString = env->NewStringUTF(extendedInfo.information);
    env->SetObjectField(version, infoField, infoString);

    return version;
}

JNIEXPORT void INSPIRE_FACE_JNI(InspireFace_SetLogLevel)(JNIEnv *env, jobject thiz, jint level) {
    HFSetLogLevel((HFLogLevel)level);
}

}  // extern "C"

#endif
