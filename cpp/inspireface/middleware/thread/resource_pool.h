#ifndef RESOURCE_POOL_H
#define RESOURCE_POOL_H

#include <iostream>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <memory>

namespace inspire {
namespace parallel {

template <typename Resource>
class ResourcePool {
public:
    class ResourceGuard {
    public:
        ResourceGuard(Resource& resource, ResourcePool& pool) : m_resource(resource), m_pool(pool), m_valid(true) {}

        // Move constructor
        ResourceGuard(ResourceGuard&& other) noexcept : m_resource(other.m_resource), m_pool(other.m_pool), m_valid(other.m_valid) {
            other.m_valid = false;
        }

        // Disable copy
        ResourceGuard(const ResourceGuard&) = delete;
        ResourceGuard& operator=(const ResourceGuard&) = delete;

        ~ResourceGuard() {
            if (m_valid) {
                m_pool.returnResource(std::move(m_resource));
            }
        }

        Resource* operator->() {
            return &m_resource;
        }
        Resource& operator*() {
            return m_resource;
        }

    private:
        Resource& m_resource;
        ResourcePool& m_pool;
        bool m_valid;
    };

    explicit ResourcePool(size_t size) {
        m_resources.reserve(size);
    }

    void addResource(Resource&& resource) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_resources.push_back(std::move(resource));
        m_available_resources.push(&m_resources.back());
        m_cv.notify_one();
    }

    // Acquire resource (blocking mode)
    ResourceGuard acquireResource() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this] { return !m_available_resources.empty(); });

        Resource* resource = m_available_resources.front();
        m_available_resources.pop();

        return ResourceGuard(*resource, *this);
    }

    // Try to acquire resource (non-blocking mode), returns nullptr if no resource available
    std::unique_ptr<ResourceGuard> tryAcquireResource() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_available_resources.empty()) {
            return nullptr;
        }

        Resource* resource = m_available_resources.front();
        m_available_resources.pop();

        return std::unique_ptr<ResourceGuard>(new ResourceGuard(*resource, *this));
    }

    // Acquire resource with timeout, returns nullptr if timeout
    std::unique_ptr<ResourceGuard> acquireResource(std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_cv.wait_for(lock, timeout, [this] { return !m_available_resources.empty(); })) {
            return nullptr;
        }

        Resource* resource = m_available_resources.front();
        m_available_resources.pop();

        return std::unique_ptr<ResourceGuard>(new ResourceGuard(*resource, *this));
    }

    size_t availableCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_available_resources.size();
    }

    size_t totalCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_resources.size();
    }

private:
    void returnResource(Resource&& resource) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& stored_resource : m_resources) {
            if (&stored_resource == &resource) {
                m_available_resources.push(&stored_resource);
                m_cv.notify_one();
                break;
            }
        }
    }

private:
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::vector<Resource> m_resources;            // Store actual resources
    std::queue<Resource*> m_available_resources;  // Queue of available resources

    friend class ResourceGuard;
};

}  // namespace parallel
}  // namespace inspire

#endif  // RESOURCE_POOL_H