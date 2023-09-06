#ifndef _LOG_UTILS_H_
#define _LOG_UTILS_H_

#include <stdio.h>
#include <string.h>

//#define DEBUG

#define __FILENAME__ (strrchr(__FILE__, '/') + 1)

//#ifdef DEBUG

#ifdef ANDROID

#include <android/log.h>
#define TAG "SolexPoseEngineSDK-Native" // 这个是自定义的LOG的标识
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,TAG ,__VA_ARGS__) // 定义LOGD类型
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG ,__VA_ARGS__) // 定义LOGI类型
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,TAG ,__VA_ARGS__) // 定义LOGW类型
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG ,__VA_ARGS__) // 定义LOGE类型
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL,TAG ,__VA_ARGS__) // 定义LOGF类型

#else
#define LOGD(format, ...)                                                      \
  printf("[%s][%s][%d]: " format "\n", __FILENAME__, __FUNCTION__, __LINE__,   \
         ##__VA_ARGS__)

#define LOGE(format, ...)                                                      \
  printf("\033[1;31m[%s][%s][%d]: ERROR: " format "\033[0m\n", __FILENAME__, __FUNCTION__, __LINE__,   \
         ##__VA_ARGS__)


#endif //ANDROID

//#else
//#define LOGD(format, ...)
//#endif

#endif // _LOG_UTILS_H_
