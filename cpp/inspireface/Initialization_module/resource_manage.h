// Created by tunm on 2024/07/16.
#pragma once
#ifndef INSPIREFACE_RESOURCE_MANAGE_H
#define INSPIREFACE_RESOURCE_MANAGE_H
#include <mutex>
#include <unordered_map>
#include <memory>
#include <iomanip>  // For std::setw and std::left

#ifndef INSPIRE_API
#define INSPIRE_API
#endif

#define RESOURCE_MANAGE inspire::ResourceManager::getInstance()

namespace inspire {

class ResourceManager {
private:
    // Private static instance pointer
    static std::unique_ptr<ResourceManager> instance;
    static std::mutex mutex;

    // Use hash tables to store session and image stream handles
    std::unordered_map<long, bool> sessionMap;
    std::unordered_map<long, bool> streamMap;

    // The private constructor guarantees singletons
    ResourceManager() {}

public:
    // Remove copy constructors and assignment operators
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    // Method of obtaining singleton instance
    static ResourceManager* getInstance() {
        std::lock_guard<std::mutex> lock(mutex);
        if (!instance) {
            instance.reset(new ResourceManager());
        }
        return instance.get();
    }

    // Method of obtaining singleton instance
    void createSession(long handle) {
        std::lock_guard<std::mutex> lock(mutex);
        sessionMap[handle] = false;  // false indicates that it is not released
    }

    // Release session
    bool releaseSession(long handle) {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = sessionMap.find(handle);
        if (it != sessionMap.end() && !it->second) {
            it->second = true;  // Mark as released
            return true;
        }
        return false;  // Release failed, possibly because the handle could not be found or was
                       // released
    }

    // Create and record image streams
    void createStream(long handle) {
        std::lock_guard<std::mutex> lock(mutex);
        streamMap[handle] = false;  // false indicates that it is not released
    }

    // Release image stream
    bool releaseStream(long handle) {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = streamMap.find(handle);
        if (it != streamMap.end() && !it->second) {
            it->second = true;  // Mark as released
            return true;
        }
        return false;  // Release failed, possibly because the handle could not be found or was
                       // released
    }
};

}  // namespace inspire

#endif  // INSPIREFACE_RESOURCE_MANAGE_H