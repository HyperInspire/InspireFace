//
// Created by tunm on 2023/10/3.
//

#ifndef HYPERFACEREPO_INSPIREFACE_H
#define HYPERFACEREPO_INSPIREFACE_H

#include <stdint.h>
#include "intypedef.h"
#include "herror.h"

#if defined(_WIN32)
#ifdef HYPER_BUILD_SHARED_LIB
#define HYPER_CAPI_EXPORT __declspec(dllexport)
#else
#define HYPER_CAPI_EXPORT
#endif
#else
#define HYPER_CAPI_EXPORT __attribute__((visibility("default")))
#endif // _WIN32


#ifdef __cplusplus
extern "C" {
#endif

#define HF_ENABLE_NONE                      0x00000000  ///< Flag to enable no features.
#define HF_ENABLE_FACE_RECOGNITION          0x00000002  ///< Flag to enable face recognition feature.
#define HF_ENABLE_LIVENESS                  0x00000004  ///< Flag to enable RGB liveness detection feature.
#define HF_ENABLE_IR_LIVENESS               0x00000008  ///< Flag to enable IR (Infrared) liveness detection feature.
#define HF_ENABLE_MASK_DETECT               0x00000010  ///< Flag to enable mask detection feature.
#define HF_ENABLE_AGE_PREDICT               0x00000020  ///< Flag to enable age prediction feature.
#define HF_ENABLE_GENDER_PREDICT            0x00000040  ///< Flag to enable gender prediction feature.
#define HF_ENABLE_QUALITY                   0x00000080  ///< Flag to enable face quality assessment feature.
#define HF_ENABLE_INTERACTION               0x00000100  ///< Flag to enable interaction feature.


/**
 * Camera stream format.
 * Contains several common camera stream formats available in the market.
 */
typedef enum HF_ImageFormat {
    STREAM_RGB = 0,             ///< Image in RGB format.
    STREAM_BGR = 1,             ///< Image in BGR format (Opencv Mat default).
    STREAM_RGBA = 2,            ///< Image in RGB format with alpha channel.
    STREAM_BGRA = 3,            ///< Image in BGR format with alpha channel.
    STREAM_YUV_NV12 = 4,        ///< Image in YUV NV12 format.
    STREAM_YUV_NV21 = 5,        ///< Image in YUV NV21 format.
} HF_ImageFormat;


/**
 * Camera picture rotation mode.
 * To accommodate the rotation of certain devices, four image rotation modes are provided.
 */
typedef enum HF_Rotation {
    CAMERA_ROTATION_0 = 0,      ///< 0 degree rotation.
    CAMERA_ROTATION_90 = 1,     ///< 90 degree rotation.
    CAMERA_ROTATION_180 = 2,    ///< 180 degree rotation.
    CAMERA_ROTATION_270 = 3,    ///< 270 degree rotation.
} HF_Rotation;


/**
 * Image Buffer Data structure.
 * Defines the structure for image data stream.
 */
typedef struct HF_ImageData {
    uint8_t *data;              ///< Pointer to the image data stream.
    HInt32 width;               ///< Width of the image.
    HInt32 height;              ///< Height of the image.
    HF_ImageFormat format;      ///< Format of the image, indicating the data stream format to be parsed.
    HF_Rotation rotation;       ///< Rotation angle of the image.
} HF_ImageData, *Ptr_HF_ImageData;


/**
 * @brief Create a data buffer stream instantiation object.
 *
 * This function is used to create an instance of a data buffer stream with the given image data.
 *
 * @param data Pointer to the image buffer data structure.
 * @param handle Pointer to the stream handle that will be returned.
 * @return HResult indicating the success or failure of the operation.
 */
HYPER_CAPI_EXPORT extern HResult HF_CreateImageStream(Ptr_HF_ImageData data, HFImageStream *handle);


/**
 * @brief Release the instantiated DataBuffer object.
 *
 * This function is used to release the DataBuffer object that has been previously instantiated.
 *
 * @param streamHandle Pointer to the DataBuffer handle representing the camera stream component.
 * @return HResult indicating the success or failure of the operation.
 */
HYPER_CAPI_EXPORT extern HResult HF_ReleaseImageStream(HFImageStream streamHandle);

/************************************************************************
* Resource Function
************************************************************************/

/**
 * @brief Launch InspireFace SDK
 * Start the InspireFace SDK at the initialization stage of your program, as it is global and designed to be used only once.
 * It serves as a prerequisite for other function interfaces, so it is essential to ensure it is initialized before calling any other APIs.
 * @param resourcePath Initializes the path to the resource file that needs to be loaded
 * @return HResult indicating the success or failure of the operation.
 * */
HYPER_CAPI_EXPORT extern HResult HF_LaunchInspireFace(HPath resourcePath);

/************************************************************************
* FaceContext
************************************************************************/

/**
 * @brief Struct for custom parameters in face recognition context.
 *
 * This struct holds various flags to enable or disable specific features
 * in the face recognition context, such as face recognition, liveness detection,
 * mask detection, age and gender prediction, etc.
 */
typedef struct HF_SessionCustomParameter {
    HInt32 enable_recognition;               ///< Enable face recognition feature.
    HInt32 enable_liveness;                  ///< Enable RGB liveness detection feature.
    HInt32 enable_ir_liveness;               ///< Enable IR liveness detection feature.
    HInt32 enable_mask_detect;               ///< Enable mask detection feature.
    HInt32 enable_age;                       ///< Enable age prediction feature.
    HInt32 enable_gender;                    ///< Enable gender prediction feature.
    HInt32 enable_face_quality;              ///< Enable face quality detection feature.
    HInt32 enable_interaction_liveness;      ///< Enable interaction for liveness detection feature.
} HF_SessionCustomParameter, *PHF_SessionCustomParameter;


/**
 * @brief Enumeration for face detection modes.
 */
typedef enum HF_DetectMode {
    HF_DETECT_MODE_IMAGE,   ///< Image detection mode, always detect.
    HF_DETECT_MODE_VIDEO,   ///< Video detection mode, face tracking.
} HF_DetectMode;

/**
 * @brief Create a session from a resource file.
 *
 * @param parameter Custom parameters for session.
 * @param detectMode Detection mode to be used.
 * @param maxDetectFaceNum Maximum number of faces to detect.
 * @param handle Pointer to the context handle that will be returned.
 * @return HResult indicating the success or failure of the operation.
 */
HYPER_CAPI_EXPORT extern HResult HF_CreateInspireFaceSession(
        HF_SessionCustomParameter parameter,
        HF_DetectMode detectMode,
        HInt32 maxDetectFaceNum,
        HFSession *handle
);

/**
 * @brief Create a session from a resource file with additional options.
 *
 * @param customOption Custom option for additional configuration.
 * @param detectMode Detection mode to be used.
 * @param maxDetectFaceNum Maximum number of faces to detect.
 * @param handle Pointer to the context handle that will be returned.
 * @return HResult indicating the success or failure of the operation.
 */
HYPER_CAPI_EXPORT extern HResult HF_CreateInspireFaceSessionOptional(
        HInt32 customOption,
        HF_DetectMode detectMode,
        HInt32 maxDetectFaceNum,
        HFSession *handle
);

/**
 * @brief Release the session.
 *
 * @param handle Handle to the session to be released.
 * @return HResult indicating the success or failure of the operation.
 */
HYPER_CAPI_EXPORT extern HResult HF_ReleaseInspireFaceSession(HFSession handle);

/**
 * @brief Struct representing a basic token for face data.
 *
 * This struct holds the size and data pointer for a basic token associated with face data.
 */
typedef struct HF_FaceBasicToken {
    HInt32 size;            ///< Size of the token.
    HPVoid data;            ///< Pointer to the token data.
} HF_FaceBasicToken, *PHF_FaceBasicToken;

/**
 * @brief Struct for face Euler angles.
 *
 * This struct represents the Euler angles (roll, yaw, pitch) for face orientation.
 */
typedef struct HF_FaceEulerAngle {
    HFloat *roll;       ///< Roll angle of the face.
    HFloat *yaw;        ///< Yaw angle of the face.
    HFloat *pitch;      ///< Pitch angle of the face.
} HF_FaceEulerAngle;

/**
 * @brief Struct for holding data of multiple detected faces.
 *
 * This struct stores the data related to multiple faces detected, including the number of faces,
 * their bounding rectangles, track IDs, angles, and tokens.
 */
typedef struct HF_MultipleFaceData {
    HInt32 detectedNum;                             ///< Number of faces detected.
    HFaceRect *rects;                               ///< Array of bounding rectangles for each face.
    HInt32 *trackIds;                               ///< Array of track IDs for each face.
    HF_FaceEulerAngle angles;                       ///< Euler angles for each face.
    PHF_FaceBasicToken tokens;                   ///< Tokens associated with each face.
} HF_MultipleFaceData, *PHF_MultipleFaceData;

/**
 * @brief Set the track preview size in the session, it works with face detection and tracking algorithms.
 * Default preview size is 192(px).
 *
 * @param session Handle to the session.
 * @param previewSize The size of the preview for tracking.
 * @return HResult indicating the success or failure of the operation.
 */
HYPER_CAPI_EXPORT extern HResult HF_SessionSetTrackPreviewSize(HFSession session, HInt32 previewSize);

/**
 * @brief Set the face track mode in the session.
 *
 * @param session Handle to the session.
 * @param detectMode The mode of the detection mode for tracking.
 * @return HResult indicating the success or failure of the operation.
 */
HYPER_CAPI_EXPORT extern HResult HF_SessionSetFaceTrackMode(HFSession session, HF_DetectMode detectMode);

/**
 * @brief Set the face detect threshold in the session.
 *
 * @param session Handle to the session.
 * @param detectMode The mode of the detection mode for tracking.
 * @return HResult indicating the success or failure of the operation.
 */
HYPER_CAPI_EXPORT extern HResult HF_SessionSetFaceDetectThreshold(HFSession session, HFloat threshold);

/**
 * @brief Run face tracking in the session.
 *
 * @param session Handle to the session.
 * @param streamHandle Handle to the data buffer representing the camera stream component.
 * @param results Pointer to the structure where the results will be stored.
 * @return HResult indicating the success or failure of the operation.
 */
HYPER_CAPI_EXPORT extern HResult HF_ExecuteFaceTrack(HFSession session, HFImageStream streamHandle, PHF_MultipleFaceData results);

/**
 * @brief Copies the data from a HF_FaceBasicToken to a specified buffer.
 *
 * This function copies the data pointed to by the HF_FaceBasicToken's data field
 * into a user-provided buffer. The caller is responsible for ensuring that the buffer
 * is large enough to hold the data being copied.
 *
 * @param token The HF_FaceBasicToken containing the data to be copied.
 * @param buffer The buffer where the data will be copied to.
 * @param bufferSize The size of the buffer provided by the caller. Must be large enough
 *        to hold the data pointed to by the token's data field.
 * @return HResult indicating the success or failure of the operation. Returns HSUCCEED
 *         if the operation was successful, or an error code if the buffer was too small
 *         or if any other error occurred.
 */
HYPER_CAPI_EXPORT extern HResult HF_CopyFaceBasicToken(HF_FaceBasicToken token, HPBuffer buffer, HInt32 bufferSize);

/**
 * @brief Retrieves the size of the data contained in a HF_FaceBasicToken.
 *
 * This function is used to query the size of the data that a HF_FaceBasicToken is
 * expected to contain. This is useful for allocating a buffer of appropriate size
 * before copying data from a HF_FaceBasicToken.
 *
 * @param bufferSize Pointer to an integer where the size of the data will be stored.
 *        On successful completion, this will contain the size of the data in bytes.
 * @return HResult indicating the success or failure of the operation. Returns HSUCCEED
 *         if the operation was successful, or an error code if it failed.
 */
HYPER_CAPI_EXPORT extern HResult HF_GetFaceBasicTokenSize(HPInt32 bufferSize);

/************************************************************************
* Face Recognition
************************************************************************/

/**
 * @brief Struct representing a face feature.
 *
 * This struct holds the data related to a face feature, including size and actual feature data.
 */
typedef struct HF_FaceFeature {
    HInt32 size;            ///< Size of the feature data.
    HPFloat data;           ///< Pointer to the feature data.
} HF_FaceFeature, *Ptr_HF_FaceFeature;

/**
 * @brief Extract a face feature from a given face.
 *
 * @param session Handle to the session.
 * @param streamHandle Handle to the data buffer representing the camera stream component.
 * @param singleFace Basic token representing a single face.
 * @param feature Pointer to the extracted face feature.
 * @return HResult indicating the success or failure of the operation.
 */
HYPER_CAPI_EXPORT extern HResult
HF_FaceFeatureExtract(HFSession session, HFImageStream streamHandle, HF_FaceBasicToken singleFace, Ptr_HF_FaceFeature feature);

/**
 * @brief Extract a face feature from a given face and copy it to the provided feature buffer.
 *
 * @param session Handle to the session.
 * @param streamHandle Handle to the data buffer representing the camera stream component.
 * @param singleFace Basic token representing a single face.
 * @param feature Pointer to the buffer where the extracted feature will be copied.
 * @return HResult indicating the success or failure of the operation.
 */
HYPER_CAPI_EXPORT extern HResult
HF_FaceFeatureExtractCpy(HFSession session, HFImageStream streamHandle, HF_FaceBasicToken singleFace, HPFloat feature);

/************************************************************************
* Feature Hub
************************************************************************/

/**
 * @brief Select the search mode in the process of face recognition search,
 * and different modes will affect the execution efficiency and results
 * */
typedef enum HF_SearchMode {
    HF_SEARCH_MODE_EAGER = 0,     // Eager mode: Stops when a vector meets the threshold.
    HF_SEARCH_MODE_EXHAUSTIVE,    // Exhaustive mode: Searches until the best match is found.
} HF_SearchMode;

/**
 * @brief Struct for database configuration.
 *
 * This struct holds the configuration settings for using a database in the face recognition context.
 */
typedef struct HF_FeatureHubConfiguration {
    HInt32 featureBlockNum;             ///< The order of magnitude of face feature database is N * 512, and 20 is recommended by default
    HInt32 enablePersistence;           ///< Flag to enable or disable the use of the database.
    HString dbPath;                     ///< Path to the database file.
    float searchThreshold;              ///< Threshold for face search
    HF_SearchMode searchMode;           ///< Mode of face search
} HF_FeatureHubConfiguration;


/**
 * @brief A lightweight face feature vector management.
 * @details FeatureHub is a built-in global lightweight face feature vector management functionality provided in the InspireFace-SDK.
 * It supports basic face feature search, deletion, and modification functions, and offers two optional data storage modes:
 * an in-memory model and a persistence model. If you have simple storage needs, you can enable it.
 *
 * @param configuration FeatureHub configuration details.
 * @return HResult indicating the success or failure of the operation.
 */
HYPER_CAPI_EXPORT extern HResult HF_FeatureHubDataEnable(HF_FeatureHubConfiguration configuration);

/**
 * @brief Disable the global FeatureHub feature, and you can enable it again if needed.
 * @return HResult indicating the success or failure of the operation.
 * */
HYPER_CAPI_EXPORT extern HResult HF_FeatureHubDataDisable();


/**
 * @brief Struct representing the identity of a face feature.
 *
 * This struct associates a custom identifier and a tag with a specific face feature.
 */
typedef struct HF_FaceFeatureIdentity {
    HInt32 customId;             ///< Custom identifier for the face feature.
    HString tag;                 ///< Tag associated with the face feature.
    Ptr_HF_FaceFeature feature;  ///< Pointer to the face feature.
} HF_FaceFeatureIdentity, *Ptr_HF_FaceFeatureIdentity;

/**
 * @brief Set the face recognition search threshold.
 *
 * This function sets the threshold for face recognition, which determines the sensitivity
 * of the recognition process. A lower threshold may yield more matches but with less confidence.
 *
 * @param threshold The threshold value to set for face recognition (default is 0.48, suitable for access control scenarios).
 * @return HResult indicating the success or failure of the operation.
 */
HYPER_CAPI_EXPORT extern HResult HF_FeatureHubFaceSearchThresholdSetting(float threshold);

/**
 * @brief Perform a one-to-one comparison of two face features.
 *
 * @param session Handle to the session.
 * @param feature1 The first face feature for comparison.
 * @param feature2 The second face feature for comparison.
 * @param result Pointer to the floating-point value where the comparison result will be stored.
 * @return HResult indicating the success or failure of the operation.
 */
HYPER_CAPI_EXPORT extern HResult HF_FaceComparison1v1(HF_FaceFeature feature1, HF_FaceFeature feature2, HPFloat result);

/**
 * @brief Get the length of the face feature.
 *
 * @param num Pointer to an integer where the length of the feature will be stored.
 * @return HResult indicating the success or failure of the operation.
 */
HYPER_CAPI_EXPORT extern HResult HF_GetFeatureLength(HPInt32 num);


/**
 * @brief Insert a face feature identity into the features group.
 *
 * @param featureIdentity The face feature identity to be inserted.
 * @return HResult indicating the success or failure of the operation.
 */
HYPER_CAPI_EXPORT extern HResult HF_FeatureHubInsertFeature(HF_FaceFeatureIdentity featureIdentity);

/**
 * @brief Search for the most similar face feature in the features group.
 *
 * @param searchFeature The face feature to be searched.
 * @param confidence Pointer to a floating-point value where the confidence level of the match will be stored.
 * @param mostSimilar Pointer to the most similar face feature identity found.
 * @return HResult indicating the success or failure of the operation.
 */
HYPER_CAPI_EXPORT extern HResult HF_FeatureHubFaceSearch(HF_FaceFeature searchFeature, HPFloat confidence, Ptr_HF_FaceFeatureIdentity mostSimilar);

/**
 * @brief Remove a face feature from the features group based on custom ID.
 *
 * @param customId The custom ID of the feature to be removed.
 * @return HResult indicating the success or failure of the operation.
 */
HYPER_CAPI_EXPORT extern HResult HF_FeatureHubFaceRemove(HInt32 customId);

/**
 * @brief Update a face feature identity in the features group.
 *
 * @param featureIdentity The face feature identity to be updated.
 * @return HResult indicating the success or failure of the operation.
 */
HYPER_CAPI_EXPORT extern HResult HF_FeatureHubFaceUpdate(HF_FaceFeatureIdentity featureIdentity);

/**
 * @brief Retrieve a face feature identity from the features group based on custom ID.
 *
 * @param customId The custom ID of the feature.
 * @param identity Pointer to the face feature identity to be retrieved.
 * @return HResult indicating the success or failure of the operation.
 */
HYPER_CAPI_EXPORT extern HResult HF_FeatureHubGetFaceIdentity(HInt32 customId, Ptr_HF_FaceFeatureIdentity identity);

/**
 * @brief Get the count of face features in the features group.
 *
 * @param count Pointer to an integer where the count of features will be stored.
 * @return HResult indicating the success or failure of the operation.
 */
HYPER_CAPI_EXPORT extern HResult HF_FeatureHubGetFaceCount(HInt32 *count);

/**
 * @brief View the face database table.
 *
 * @return HResult indicating the success or failure of the operation.
 */
HYPER_CAPI_EXPORT extern HResult HF_FeatureHubViewDBTable();

/************************************************************************
* Face Pipeline
************************************************************************/

/**
 * @brief Process multiple faces in a pipeline.
 *
 * This function processes multiple faces detected in an image or video frame, applying
 * various face recognition and analysis features as specified in the parameters.
 *
 * @param session Handle to the session.
 * @param streamHandle Handle to the data buffer representing the camera stream component.
 * @param faces Pointer to the structure containing data of multiple detected faces.
 * @param parameter Custom parameters for processing the faces.
 * @return HResult indicating the success or failure of the operation.
 */
HYPER_CAPI_EXPORT extern HResult
HF_MultipleFacePipelineProcess(HFSession session, HFImageStream streamHandle, PHF_MultipleFaceData faces,
                               HF_SessionCustomParameter parameter);

/**
 * @brief Process multiple faces in a pipeline with an optional custom option.
 *
 * Similar to HF_MultipleFacePipelineProcess, but allows for additional custom options
 * to modify the face processing behavior.
 *
 * @param session Handle to the session.
 * @param streamHandle Handle to the data buffer representing the camera stream component.
 * @param faces Pointer to the structure containing data of multiple detected faces.
 * @param customOption An integer representing a custom option for processing.
 * @return HResult indicating the success or failure of the operation.
 */
HYPER_CAPI_EXPORT extern HResult
HF_MultipleFacePipelineProcessOptional(HFSession session, HFImageStream streamHandle,
                                       PHF_MultipleFaceData faces, HInt32 customOption);

/**
 * @brief Struct representing RGB liveness confidence.
 *
 * This struct holds the number of faces and the confidence level of liveness detection
 * for each face, using RGB analysis.
 */
typedef struct HF_RGBLivenessConfidence {
    HInt32 num;        ///< Number of faces detected.
    HPFloat confidence; ///< Confidence level of RGB liveness detection for each face.
} HF_RGBLivenessConfidence, *PHF_RGBLivenessConfidence;

/**
 * @brief Get the RGB liveness confidence.
 *
 * This function retrieves the confidence level of RGB liveness detection for faces detected
 * in the current context.
 *
 * @param session Handle to the session.
 * @param confidence Pointer to the structure where RGB liveness confidence data will be stored.
 * @return HResult indicating the success or failure of the operation.
 */
HYPER_CAPI_EXPORT extern HResult
HF_GetRGBLivenessConfidence(HFSession session, PHF_RGBLivenessConfidence confidence);

/**
 * @brief Struct representing face mask confidence.
 *
 * This struct holds the number of faces and the confidence level of mask detection
 * for each face.
 */
typedef struct HF_FaceMaskConfidence {
    HInt32 num;         ///< Number of faces detected.
    HPFloat confidence; ///< Confidence level of mask detection for each face.
} HF_FaceMaskConfidence, *PHF_FaceMaskConfidence;

/**
 * @brief Get the face mask confidence.
 *
 * This function retrieves the confidence level of mask detection for faces detected
 * in the current context.
 *
 * @param session Handle to the session.
 * @param confidence Pointer to the structure where face mask confidence data will be stored.
 * @return HResult indicating the success or failure of the operation.
 */
HYPER_CAPI_EXPORT extern HResult HF_GetFaceMaskConfidence(HFSession session, PHF_FaceMaskConfidence confidence);

/**
 * @brief Struct representing face quality predict confidence.
 *
 * This struct holds the number of faces and the confidence level of face quality predict
 * for each face.
 */
typedef struct HF_FaceQualityConfidence {
    HInt32 num;         ///< Number of faces detected.
    HPFloat confidence; ///< Confidence level of face quality predict for each face.
} HF_FaceQualityConfidence, *PHF_FaceQualityConfidence;

/**
 * @brief Get the face quality predict confidence.
 *
 * This function retrieves the confidence level of face quality predict for faces detected
 * in the current context.
 *
 * @param session Handle to the session.
 * @param confidence Pointer to the structure where face mask confidence data will be stored.
 * @return HResult indicating the success or failure of the operation.
 */
HYPER_CAPI_EXPORT extern HResult HF_GetFaceQualityConfidence(HFSession session, PHF_FaceQualityConfidence confidence);

/**
 * @brief Detect the quality of a face in an image.
 *
 * This function assesses the quality of a detected face, such as its clarity and visibility.
 *
 * @param session Handle to the session.
 * @param singleFace A token representing a single face.
 * @param confidence Pointer to a floating-point value where the quality confidence will be stored.
 * @return HResult indicating the success or failure of the operation.
 */
HYPER_CAPI_EXPORT extern HResult
HF_FaceQualityDetect(HFSession session, HF_FaceBasicToken singleFace, HFloat *confidence);

/************************************************************************
* System Function
************************************************************************/

/**
 * @brief Structure representing the version information of the InspireFace library.
 */
typedef struct HF_InspireFaceVersion {
    int major;     ///< Major version number.
    int minor;     ///< Minor version number.
    int patch;     ///< Patch version number.
} HF_InspireFaceVersion, *PHF_InspireFaceVersion;

/**
 * @brief Function to query the version information of the InspireFace library.
 *
 * This function retrieves the version information of the InspireFace library.
 *
 * @param version Pointer to the structure where the version information will be stored.
 * @return HResult indicating the success or failure of the operation.
 */
HYPER_CAPI_EXPORT extern HResult HF_QueryInspireFaceVersion(PHF_InspireFaceVersion version);

/**
 * @brief SDK built-in log level mode
 * */
typedef enum HF_LogLevel {
    HF_LOG_NONE = 0,   // No logging, disables all log output
    HF_LOG_DEBUG,      // Debug level for detailed system information mostly useful for developers
    HF_LOG_INFO,       // Information level for general system information about operational status
    HF_LOG_WARN,       // Warning level for non-critical issues that might need attention
    HF_LOG_ERROR,      // Error level for error events that might still allow the application to continue running
    HF_LOG_FATAL       // Fatal level for severe error events that will presumably lead the application to abort
} HF_LogLevel;

/**
 * @brief Set the log level built into the SDK.The default is HF LOG DEBUG
 * */
HYPER_CAPI_EXPORT extern HResult HF_SetLogLevel(HF_LogLevel level);

/**
 * @brief Disable the log function. Like HF_SetLogLevel(HF_LOG_NONE)
 * */
HYPER_CAPI_EXPORT extern HResult HF_LogDisable();

/********************************DEBUG Utils****************************************/

/**
 * @brief Display an image stream for debugging purposes.
 *
 * This function is used for debugging, allowing the visualization of the image stream
 * as it is being processed. It can be useful to understand the data being received
 * from the camera or image source.
 *
 * @param streamHandle Handle to the data buffer representing the camera stream component.
 */
HYPER_CAPI_EXPORT extern void HF_DeBugImageStreamImShow(HFImageStream streamHandle);


#ifdef __cplusplus
}
#endif


#endif //HYPERFACEREPO_INSPIREFACE_H
