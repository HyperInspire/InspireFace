/**
 * @author Jingyu Yan
 * @date 2024-10-01
 */
#include <iostream>
#include "inspireface/c_api/inspireface.h"

int main(int argc, char* argv[]) {
    // Check whether the number of parameters is correct
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: " << argv[0] << " <pack_path> <source_path> [rotation]\n";
        return 1;
    }

    auto packPath = argv[1];
    auto sourcePath = argv[2];
    int rotation = 0;

    // If rotation is provided, check and set the value
    if (argc == 4) {
        rotation = std::atoi(argv[3]);
        if (rotation != 0 && rotation != 90 && rotation != 180 && rotation != 270) {
            std::cerr << "Invalid rotation value. Allowed values are 0, 90, 180, 270.\n";
            return 1;
        }
    }
    HFRotation rotation_enum;
    // Set rotation based on input parameter
    switch (rotation) {
        case 90:
            rotation_enum = HF_CAMERA_ROTATION_90;
            break;
        case 180:
            rotation_enum = HF_CAMERA_ROTATION_180;
            break;
        case 270:
            rotation_enum = HF_CAMERA_ROTATION_270;
            break;
        case 0:
        default:
            rotation_enum = HF_CAMERA_ROTATION_0;
            break;
    }

    std::cout << "Pack file Path: " << packPath << std::endl;
    std::cout << "Source file Path: " << sourcePath << std::endl;
    std::cout << "Rotation: " << rotation << std::endl;

    HFSetLogLevel(HF_LOG_INFO);

    HResult ret;
    // The resource file must be loaded before it can be used
    ret = HFLaunchInspireFace(packPath);
    if (ret != HSUCCEED) {
        std::cout << "Load Resource error: " << ret << std::endl;
        return ret;
    }

    // Enable the functions in the pipeline: mask detection, live detection, and face quality
    // detection
    HOption option = HF_ENABLE_QUALITY | HF_ENABLE_MASK_DETECT | HF_ENABLE_LIVENESS | HF_ENABLE_DETECT_MODE_LANDMARK;
    // Non-video or frame sequence mode uses IMAGE-MODE, which is always face detection without
    // tracking
    HFDetectMode detMode = HF_DETECT_MODE_ALWAYS_DETECT;
    // Maximum number of faces detected
    HInt32 maxDetectNum = 20;
    // Face detection image input level
    HInt32 detectPixelLevel = 160;
    // Handle of the current face SDK algorithm context
    HFSession session = {0};
    ret = HFCreateInspireFaceSessionOptional(option, detMode, maxDetectNum, detectPixelLevel, -1, &session);
    if (ret != HSUCCEED) {
        std::cout << "Create FaceContext error: " << ret << std::endl;
        return ret;
    }

    HFSessionSetTrackPreviewSize(session, detectPixelLevel);
    HFSessionSetFilterMinimumFacePixelSize(session, 4);

    // Load a image
    HFImageBitmap image;
    ret = HFCreateImageBitmapFromFilePath(sourcePath, 3, &image);
    if (ret != HSUCCEED) {
        std::cout << "The source entered is not a picture or read error." << std::endl;
        return ret;
    }
    // Prepare an image parameter structure for configuration
    HFImageStream imageHandle = {0};
    ret = HFCreateImageStreamFromImageBitmap(image, rotation_enum, &imageHandle);
    if (ret != HSUCCEED) {
        std::cout << "Create ImageStream error: " << ret << std::endl;
        return ret;
    }

    // Execute HF_FaceContextRunFaceTrack captures face information in an image
    HFMultipleFaceData multipleFaceData = {0};
    ret = HFExecuteFaceTrack(session, imageHandle, &multipleFaceData);
    if (ret != HSUCCEED) {
        std::cout << "Execute HFExecuteFaceTrack error: " << ret << std::endl;
        return ret;
    }

    // Print the number of faces detected
    auto faceNum = multipleFaceData.detectedNum;
    std::cout << "Num of face: " << faceNum << std::endl;

    // Copy a new image to draw
    HFImageBitmap drawImage = {0};
    ret = HFImageBitmapCopy(image, &drawImage);
    if (ret != HSUCCEED) {
        std::cout << "Copy ImageBitmap error: " << ret << std::endl;
        return ret;
    }
    HFImageBitmapData data;
    ret = HFImageBitmapGetData(drawImage, &data);
    if (ret != HSUCCEED) {
        std::cout << "Get ImageBitmap data error: " << ret << std::endl;
        return ret;
    }
    for (int index = 0; index < faceNum; ++index) {
        std::cout << "========================================" << std::endl;
        std::cout << "Token size: " << multipleFaceData.tokens[index].size << std::endl;
        std::cout << "Process face index: " << index << std::endl;
        std::cout << "DetConfidence: " << multipleFaceData.detConfidence[index] << std::endl;
        HFImageBitmapDrawRect(drawImage, multipleFaceData.rects[index], {0, 100, 255}, 4);

        // Print FaceID, In IMAGE-MODE it is changing, in VIDEO-MODE it is fixed, but it may be lost
        std::cout << "FaceID: " << multipleFaceData.trackIds[index] << std::endl;

        // Print Head euler angle, It can often be used to judge the quality of a face by the Angle
        // of the head
        std::cout << "Roll: " << multipleFaceData.angles.roll[index] << ", Yaw: " << multipleFaceData.angles.yaw[index]
                  << ", Pitch: " << multipleFaceData.angles.pitch[index] << std::endl;

        HInt32 numOfLmk;
        HFGetNumOfFaceDenseLandmark(&numOfLmk);
        HPoint2f denseLandmarkPoints[numOfLmk];
        ret = HFGetFaceDenseLandmarkFromFaceToken(multipleFaceData.tokens[index], denseLandmarkPoints, numOfLmk);
        if (ret != HSUCCEED) {
            std::cerr << "HFGetFaceDenseLandmarkFromFaceToken error!!" << std::endl;
            return -1;
        }
        for (size_t i = 0; i < numOfLmk; i++) {
            HFImageBitmapDrawCircleF(drawImage, {denseLandmarkPoints[i].x, denseLandmarkPoints[i].y}, 0, {100, 100, 0}, 1);
        }
        auto& rt = multipleFaceData.rects[index];
        float area = ((float)(rt.height * rt.width)) / (data.width * data.height);
        std::cout << "area: " << area << std::endl;

        HPoint2f fiveKeyPoints[5];
        ret = HFGetFaceFiveKeyPointsFromFaceToken(multipleFaceData.tokens[index], fiveKeyPoints, 5);
        if (ret != HSUCCEED) {
            std::cerr << "HFGetFaceFiveKeyPointsFromFaceToken error!!" << std::endl;
            return -1;
        }
        for (size_t i = 0; i < 5; i++) {
            HFImageBitmapDrawCircleF(drawImage, {fiveKeyPoints[i].x, fiveKeyPoints[i].y}, 0, {0, 0, 232}, 2);
        }
    }
    HFImageBitmapWriteToFile(drawImage, "draw_detected.jpg");
    std::cout << "Write to file success: " << "draw_detected.jpg" << std::endl;

    // Run pipeline function
    // Select the pipeline function that you want to execute, provided that it is already enabled
    // when FaceContext is created!
    auto pipelineOption = HF_ENABLE_QUALITY | HF_ENABLE_MASK_DETECT | HF_ENABLE_LIVENESS;
    // In this loop, all faces are processed
    ret = HFMultipleFacePipelineProcessOptional(session, imageHandle, &multipleFaceData, pipelineOption);
    if (ret != HSUCCEED) {
        std::cout << "Execute Pipeline error: " << ret << std::endl;
        return ret;
    }

    // Get mask detection results from the pipeline cache
    HFFaceMaskConfidence maskConfidence = {0};
    ret = HFGetFaceMaskConfidence(session, &maskConfidence);
    if (ret != HSUCCEED) {
        std::cout << "Get mask detect result error: " << ret << std::endl;
        return -1;
    }

    // Get face quality results from the pipeline cache
    HFFaceQualityConfidence qualityConfidence = {0};
    ret = HFGetFaceQualityConfidence(session, &qualityConfidence);
    if (ret != HSUCCEED) {
        std::cout << "Get face quality result error: " << ret << std::endl;
        return -1;
    }

    for (int index = 0; index < faceNum; ++index) {
        std::cout << "========================================" << std::endl;
        std::cout << "Process face index from pipeline: " << index << std::endl;
        std::cout << "Mask detect result: " << maskConfidence.confidence[index] << std::endl;
        std::cout << "Quality predict result: " << qualityConfidence.confidence[index] << std::endl;
        // We set the threshold of wearing a mask as 0.85. If it exceeds the threshold, it will be
        // judged as wearing a mask. The threshold can be adjusted according to the scene
        if (maskConfidence.confidence[index] > 0.85) {
            std::cout << "Mask" << std::endl;
        } else {
            std::cout << "Non Mask" << std::endl;
        }
    }

    ret = HFReleaseImageStream(imageHandle);
    if (ret != HSUCCEED) {
        printf("Release image stream error: %lu\n", ret);
    }
    // The memory must be freed at the end of the program
    ret = HFReleaseInspireFaceSession(session);
    if (ret != HSUCCEED) {
        printf("Release session error: %lu\n", ret);
        return ret;
    }

    ret = HFReleaseImageBitmap(image);
    if (ret != HSUCCEED) {
        printf("Release image bitmap error: %lu\n", ret);
        return ret;
    }

    ret = HFReleaseImageBitmap(drawImage);
    if (ret != HSUCCEED) {
        printf("Release draw image bitmap error: %lu\n", ret);
        return ret;
    }

    return 0;
}
