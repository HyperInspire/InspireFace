#ifndef LOG_MANAGER_H
#define LOG_MANAGER_H

#include <mutex>
#include <string>

// Macro to extract the filename from the full path
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#ifdef ANDROID
// Android platform log macros
#define LOGD(...) LogManager::getInstance()->logAndroid(LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGI(...) LogManager::getInstance()->logAndroid(LOG_INFO, TAG, __VA_ARGS__)
#define LOGW(...) LogManager::getInstance()->logAndroid(LOG_WARN, TAG, __VA_ARGS__)
#define LOGE(...) LogManager::getInstance()->logAndroid(LOG_ERROR, TAG, __VA_ARGS__)
#define LOGF(...) LogManager::getInstance()->logAndroid(LOG_FATAL, TAG, __VA_ARGS__)
#else
// Standard platform log macros
#define LOGD(...) LogManager::getInstance()->logStandard(LOG_DEBUG, __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOGI(...) LogManager::getInstance()->logStandard(LOG_INFO, __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOGW(...) LogManager::getInstance()->logStandard(LOG_WARN, __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOGE(...) LogManager::getInstance()->logStandard(LOG_ERROR, __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOGF(...) LogManager::getInstance()->logStandard(LOG_FATAL, __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__)
#endif

// Macro to set the global log level
#define SET_LOG_LEVEL(level) LogManager::getInstance()->setLogLevel(level)

// Log levels
enum LogLevel {
    LOG_NONE = 0,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL
};

class LogManager {
private:
    LogLevel currentLevel;
    static LogManager* instance;
    static std::mutex mutex;

    // Private constructor
    LogManager() : currentLevel(LOG_DEBUG) {}  // Default log level is DEBUG

public:
    // Disable copy construction and assignment
    LogManager(const LogManager&) = delete;
    LogManager& operator=(const LogManager&) = delete;

    // Get the singleton instance
    static LogManager* getInstance() {
        std::lock_guard<std::mutex> lock(mutex);
        if (instance == nullptr) {
            instance = new LogManager();
        }
        return instance;
    }

    // Set the log level
    void setLogLevel(LogLevel level) {
        currentLevel = level;
    }

    // Get the current log level
    LogLevel getLogLevel() const {
        return currentLevel;
    }

#ifdef ANDROID
    // Method for logging on the Android platform
    void logAndroid(LogLevel level, const char* tag, const char* format, ...) const {
        if (level < currentLevel) return;

        int androidLevel;
        switch (level) {
            case LOG_DEBUG: androidLevel = ANDROID_LOG_DEBUG; break;
            case LOG_INFO: androidLevel = ANDROID_LOG_INFO; break;
            case LOG_WARN: androidLevel = ANDROID_LOG_WARN; break;
            case LOG_ERROR: androidLevel = ANDROID_LOG_ERROR; break;
            case LOG_FATAL: androidLevel = ANDROID_LOG_FATAL; break;
            default: androidLevel = ANDROID_LOG_DEFAULT;
        }

        va_list args;
        va_start(args, format);
        __android_log_vprint(androidLevel, tag, format, args);
        va_end(args);
    }
#else
    // Method for standard platform logging
    void logStandard(LogLevel level, const char* filename, const char* function, int line, const char* format, ...) const {
        if (level < currentLevel) return;

        printf("[%s][%s][%d]: ", filename, function, line);
        if (level == LOG_ERROR || level == LOG_FATAL) {
            printf("\033[1;31m");  // Red color
        } else if (level == LOG_WARN) {
            printf("\033[1;33m");  // Yellow color
        }
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        if (level == LOG_ERROR || level == LOG_WARN || level == LOG_FATAL) {
            printf("\033[0m");  // Reset color
        }
        printf("\n");
    }
#endif

};

#endif // LOG_MANAGER_H
