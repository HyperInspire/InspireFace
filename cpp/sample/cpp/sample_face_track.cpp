//
// Created by Tunm-Air13 on 2024/4/17.
//
#include <iostream>
#include "opencv2/opencv.hpp"
#include "inspireface/c_api/inspireface.h"

int main(int argc, char* argv[]) {
    // Check whether the number of parameters is correct
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <pack_path> <source_path>\n";
        return 1;
    }

    auto packPath = argv[1];
    auto sourcePath = argv[2];

    std::cout << "Pack file Path: " << packPath << std::endl;
    std::cout << "Source file Path: " << sourcePath << std::endl;

    HResult ret;
    // The resource file must be loaded before it can be used
    ret = HF_LaunchInspireFace(packPath);
    if (ret != HSUCCEED) {
        std::cout << "Load Resource error: " << ret << std::endl;
        return ret;
    }

    // Enable the functions in the pipeline: mask detection, live detection, and face quality detection
    HInt32 option = HF_ENABLE_QUALITY | HF_ENABLE_MASK_DETECT | HF_ENABLE_LIVENESS;
    // Non-video or frame sequence mode uses IMAGE-MODE, which is always face detection without tracking
    HF_DetectMode detMode = HF_DETECT_MODE_IMAGE;
    // Maximum number of faces detected
    HInt32 maxDetectNum = 5;
    // Handle of the current face SDK algorithm context
    HContextHandle ctxHandle = {0};
    ret = HF_CreateFaceContextFromResourceFileOptional(option, detMode, maxDetectNum, &ctxHandle);
    if (ret != HSUCCEED) {
        std::cout << "Create FaceContext error: " << ret << std::endl;
        return ret;
    }

    // Load a image
    cv::Mat image = cv::imread(sourcePath);
    if (image.empty()) {
        std::cout << "The source entered is not a picture or read error." << std::endl;
        return 1;
    }
    // Prepare an image parameter structure for configuration
    HF_ImageData imageParam = {0};
    imageParam.data = image.data;       // Data buffer
    imageParam.width = image.cols;      // Target view width
    imageParam.height = image.rows;      // Target view width
    imageParam.rotation = CAMERA_ROTATION_0;      // Data source rotate
    imageParam.format = STREAM_BGR;      // Data source format

    // Create an image data stream
    HImageHandle imageHandle = {0};
    ret = HF_CreateImageStream(&imageParam, &imageHandle);
    if (ret != HSUCCEED) {
        std::cout << "Create ImageStream error: " << ret << std::endl;
        return ret;
    }

    // Execute HF_FaceContextRunFaceTrack captures face information in an image
    HF_MultipleFaceData multipleFaceData = {0};
    ret = HF_FaceContextRunFaceTrack(ctxHandle, imageHandle, &multipleFaceData);
    if (ret != HSUCCEED) {
        std::cout << "Execute HF_FaceContextRunFaceTrack error: " << ret << std::endl;
        return ret;
    }
    // Print the number of faces detected
    auto faceNum = multipleFaceData.detectedNum;
    std::cout << "Num of face: " << faceNum << std::endl;

    // Copy a new image to draw
    cv::Mat draw = image.clone();
    for (int index = 0; index < faceNum; ++index) {
        std::cout << "========================================" << std::endl;
        std::cout << "Process face index: " << index << std::endl;
        // Use OpenCV's Rect to receive face bounding boxes
        cv::Rect rect = cv::Rect(multipleFaceData.rects[index].x, multipleFaceData.rects[index].y,
                                 multipleFaceData.rects[index].width, multipleFaceData.rects[index].height);
        cv::rectangle(draw, rect, cv::Scalar(0, 100, 255), 1);

        // Print FaceID, In IMAGE-MODE it is changing, in VIDEO-MODE it is fixed, but it may be lost
        std::cout << "FaceID: " << multipleFaceData.trackIds[index] << std::endl;

        // Print Head euler angle, It can often be used to judge the quality of a face by the Angle of the head
        std::cout << "Roll: " << multipleFaceData.angles.roll[index]
                    << ", Yaw: " << multipleFaceData.angles.roll[index]
                    << ", Pitch: " << multipleFaceData.angles.pitch[index] << std::endl;

    }
    cv::imwrite("draw_detected.jpg", draw);

    // Run pipeline function
    // Select the pipeline function that you want to execute, provided that it is already enabled when FaceContext is created!
    auto pipelineOption = HF_ENABLE_QUALITY | HF_ENABLE_MASK_DETECT | HF_ENABLE_LIVENESS;
    // In this loop, all faces are processed
    ret = HF_MultipleFacePipelineProcessOptional(ctxHandle, imageHandle, &multipleFaceData, pipelineOption);
    if (ret != HSUCCEED) {
        std::cout << "Execute Pipeline error: " << ret << std::endl;
        return ret;
    }

    // Get mask detection results from the pipeline cache
    HF_FaceMaskConfidence maskConfidence = {0};
    ret = HF_GetFaceMaskConfidence(ctxHandle, &maskConfidence);
    if (ret != HSUCCEED) {
        std::cout << "Get mask detect result error: " << ret << std::endl;
        return -1;
    }


    for (int index = 0; index < faceNum; ++index) {
        std::cout << "========================================" << std::endl;
        std::cout << "Process face index from pipeline: " << index << std::endl;
        std::cout << "Mask detect result: " << maskConfidence.confidence[index] << std::endl;
        // We set the threshold of wearing a mask as 0.85. If it exceeds the threshold, it will be judged as wearing a mask.
        // The threshold can be adjusted according to the scene
        if (maskConfidence.confidence[0] > 0.85) {
            std::cout << "Mask" << std::endl;
        } else {
            std::cout << "Non Mask" << std::endl;
        }
        // Get face quality result from the pipeline cache
        HFloat quality;
        ret = HF_FaceQualityDetect(ctxHandle, multipleFaceData.tokens[index], &quality);
        if (ret != HSUCCEED) {
            std::cout << "Get quality error" << std::endl;
        }
        std::cout << "Face quality: " << quality << std::endl;
        //
    }

    // The memory must be freed at the end of the program
    ret = HF_ReleaseFaceContext(ctxHandle);
    if (ret != HSUCCEED) {
        printf("Release FaceContext error: %lu\n", ret);
        return ret;
    }

    return 0;
}