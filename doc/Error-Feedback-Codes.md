# Error Feedback Codes

During the use of InspireFace, some error feedback codes may be generated. Here is a table of error feedback codes.

- As of **June 15, 2025**, the error code definitions have been restructured. Some legacy codes from historical versions have been removed, and a more streamlined version has been reorganized and consolidated.

 | Index | Name | Code | Comment | 
 | --- | --- | --- | --- | 
 | 1 | HSUCCEED | 0 | Success | 
 | 2 | HERR_UNKNOWN | 1 | Unknown error (1) | 
 | 3 | HERR_INVALID_PARAM | 2 | Invalid parameter (2) | 
 | 4 | HERR_INVALID_IMAGE_STREAM_HANDLE | 3 | Invalid image stream handle (3) | 
 | 5 | HERR_INVALID_CONTEXT_HANDLE | 4 | Invalid context handle (4) | 
 | 6 | HERR_INVALID_FACE_TOKEN | 5 | Invalid face token (5) | 
 | 7 | HERR_INVALID_FACE_FEATURE | 6 | Invalid face feature (6) | 
 | 8 | HERR_INVALID_FACE_LIST | 7 | Invalid face feature list (7) | 
 | 9 | HERR_INVALID_BUFFER_SIZE | 8 | Invalid copy token (8) | 
 | 10 | HERR_INVALID_IMAGE_STREAM_PARAM | 9 | Invalid image param (9) | 
 | 11 | HERR_INVALID_SERIALIZATION_FAILED | 10 | Invalid face serialization failed (10) | 
 | 12 | HERR_INVALID_DETECTION_INPUT | 11 | Failed to modify detector input size (11) | 
 | 13 | HERR_INVALID_IMAGE_BITMAP_HANDLE | 12 | Invalid image bitmap handle (12) | 
 | 14 | HERR_SESS_FUNCTION_UNUSABLE | 101 | Function not usable (101) | 
 | 15 | HERR_SESS_TRACKER_FAILURE | 102 | Tracker module not initialized (102) | 
 | 16 | HERR_SESS_PIPELINE_FAILURE | 103 | Pipeline module not initialized (103) | 
 | 17 | HERR_SESS_INVALID_RESOURCE | 104 | Invalid static resource (104) | 
 | 18 | HERR_SESS_LANDMARK_NUM_NOT_MATCH | 105 | The number of input landmark points does not match (105) | 
 | 19 | HERR_SESS_LANDMARK_NOT_ENABLE | 106 | The landmark model is not enabled (106) | 
 | 20 | HERR_SESS_KEY_POINT_NUM_NOT_MATCH | 107 | The number of input key points does not match (107) | 
 | 21 | HERR_SESS_REC_EXTRACT_FAILURE | 108 | Face feature extraction not registered (108) | 
 | 22 | HERR_SESS_REC_CONTRAST_FEAT_ERR | 109 | Incorrect length of feature vector for comparison (109) | 
 | 23 | HERR_SESS_FACE_DATA_ERROR | 110 | Face data parsing (110) | 
 | 24 | HERR_SESS_FACE_REC_OPTION_ERROR | 111 | An optional parameter is incorrect (111) | 
 | 25 | HERR_FT_HUB_DISABLE | 201 | FeatureHub is disabled (201) | 
 | 26 | HERR_FT_HUB_INSERT_FAILURE | 202 | Data insertion error (202) | 
 | 27 | HERR_FT_HUB_NOT_FOUND_FEATURE | 203 | Get face feature error (203) | 
 | 28 | HERR_ARCHIVE_LOAD_FAILURE | 251 | Archive load failure (251) | 
 | 29 | HERR_ARCHIVE_LOAD_MODEL_FAILURE | 252 | Model load failure (252) | 
 | 30 | HERR_ARCHIVE_FILE_FORMAT_ERROR | 253 | The archive format is incorrect (253) | 
 | 31 | HERR_ARCHIVE_REPETITION_LOAD | 254 | Do not reload the model (254) | 
 | 32 | HERR_ARCHIVE_NOT_LOAD | 255 | Model not loaded (255) | 
 | 33 | HERR_DEVICE_CUDA_NOT_SUPPORT | 301 | CUDA not supported (301) | 
 | 34 | HERR_DEVICE_CUDA_TENSORRT_NOT_SUPPORT | 302 | CUDA TensorRT not supported (302) | 
 | 35 | HERR_DEVICE_CUDA_UNKNOWN_ERROR | 303 | CUDA unknown error (303) | 
 | 36 | HERR_DEVICE_CUDA_DISABLE | 304 | CUDA support is disabled (304) | 
 | 37 | HERR_EXTENSION_ERROR | 351 | Extension module error (351) | 
 | 38 | HERR_EXTENSION_MLMODEL_LOAD_FAILED | 352 | MLModel load failed (352) | 
 | 39 | HERR_EXTENSION_HETERO_MODEL_TAG_ERROR | 353 | Incorrect heterogeneous model tag (353) | 
 | 40 | HERR_EXTENSION_HETERO_REC_HEAD_CONFIG_ERROR | 354 | Rec head config error (354) | 
 | 41 | HERR_EXTENSION_HETERO_MODEL_NOT_MATCH | 355 | Heterogeneous model dimensions do not match (355) | 
 | 42 | HERR_EXTENSION_HETERO_MODEL_NOT_LOADED | 356 | Heterogeneous model dimensions not loaded (356) | 
