package com.insightface.sdk.inspireface.base;

public class TypeDefine {
    /** Camera rotation 0 degrees */
    public static final int CAMERA_ROTATION_0 = 0;
    /** Camera rotation 90 degrees */
    public static final int CAMERA_ROTATION_90 = 1;
    /** Camera rotation 180 degrees */
    public static final int CAMERA_ROTATION_180 = 2;
    /** Camera rotation 270 degrees */
    public static final int CAMERA_ROTATION_270 = 3;

    /** Image in RGB format */
    public static final int STREAM_RGB = 0;
    /** Image in BGR format (Opencv Mat default) */
    public static final int STREAM_BGR = 1;
    /** Image in RGB format with alpha channel */
    public static final int STREAM_RGBA = 2;
    /** Image in BGR format with alpha channel */
    public static final int STREAM_BGRA = 3;
    /** Image in YUV NV12 format */
    public static final int STREAM_YUV_NV12 = 4;
    /** Image in YUV NV21 format */
    public static final int STREAM_YUV_NV21 = 5;

    /** Image detection mode, always detect, applicable to images. */
    public static final int DETECT_MODE_ALWAYS_DETECT = 0;
    /** Video detection mode, face tracking, applicable to video streaming, front camera. */
    public static final int DETECT_MODE_LIGHT_TRACK = 1;
    /** Video detection mode, face tracking, applicable to high resolution, monitoring, capturing */
    public static final int DETECT_MODE_TRACK_BY_DETECTION = 2;

    /** Eager mode: Stops when a vector meets the threshold. */
    public static final int SEARCH_MODE_EAGER = 0;
    /** Exhaustive mode: Searches until the best match is found. */
    public static final int SEARCH_MODE_EXHAUSTIVE = 1;

    /** Auto-increment mode for primary key. */
    public static final int PK_AUTO_INCREMENT = 0;
    /** Manual input mode for primary key. */
    public static final int PK_MANUAL_INPUT = 1;

    /** No logging, disables all log output */
    public static final int LOG_NONE = 0;
    /** Debug level for detailed system information mostly useful for developers */
    public static final int LOG_DEBUG = 1;
    /** Information level for general system information about operational status */
    public static final int LOG_INFO = 2;
    /** Warning level for non-critical issues that might need attention */
    public static final int LOG_WARN = 3;
    /** Error level for error events that might still allow the application to continue running */
    public static final int LOG_FATAL = 5;
}
