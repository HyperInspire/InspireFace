/**
 * Created by Jingyu Yan
 * @date 2024-10-01
 */

#include "launch.h"
#include "log.h"
#include "herror.h"

namespace inspire {

std::mutex Launch::mutex_;
std::shared_ptr<Launch> Launch::instance_ = nullptr;

InspireArchive& Launch::getMArchive() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!m_archive_) {
        throw std::runtime_error("Archive not initialized");
    }
    return *m_archive_;
}

std::shared_ptr<Launch> Launch::GetInstance() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!instance_) {
        instance_ = std::shared_ptr<Launch>(new Launch());
    }
    return instance_;
}

int32_t Launch::Load(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!m_load_) {
        try {
            m_archive_ = std::make_unique<InspireArchive>();
            m_archive_->ReLoad(path);

            if (m_archive_->QueryStatus() == SARC_SUCCESS) {
                m_load_ = true;
                INSPIRE_LOGI("Successfully loaded resources");
                return HSUCCEED;
            } else {
                m_archive_.reset();
                INSPIRE_LOGE("Failed to load resources");
                return HERR_ARCHIVE_LOAD_MODEL_FAILURE;
            }
        } catch (const std::exception& e) {
            m_archive_.reset();
            INSPIRE_LOGE("Exception during resource loading: %s", e.what());
            return HERR_ARCHIVE_LOAD_MODEL_FAILURE;
        }
    } else {
        INSPIRE_LOGW("There is no need to call launch more than once, as subsequent calls will not affect the initialization.");
        return HSUCCEED;
    }
}

int32_t Launch::Reload(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    try {
        // Clean up existing archive if it exists
        if (m_archive_) {
            m_archive_.reset();
            m_load_ = false;
        }

        // Create and load new archive
        m_archive_ = std::make_unique<InspireArchive>();
        m_archive_->ReLoad(path);

        if (m_archive_->QueryStatus() == SARC_SUCCESS) {
            m_load_ = true;
            INSPIRE_LOGI("Successfully reloaded resources");
            return HSUCCEED;
        } else {
            m_archive_.reset();
            INSPIRE_LOGE("Failed to reload resources");
            return HERR_ARCHIVE_LOAD_MODEL_FAILURE;
        }
    } catch (const std::exception& e) {
        m_archive_.reset();
        INSPIRE_LOGE("Exception during resource reloading: %s", e.what());
        return HERR_ARCHIVE_LOAD_MODEL_FAILURE;
    }
}

bool Launch::isMLoad() const {
    return m_load_;
}

void Launch::Unload() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (m_load_) {
        m_archive_.reset();
        m_load_ = false;
        INSPIRE_LOGI("All resources have been successfully unloaded and system is reset.");
    } else {
        INSPIRE_LOGW("Unload called but system was not loaded.");
    }
}

void Launch::SetRockchipDmaHeapPath(const std::string& path) {
    m_rockchip_dma_heap_path_ = path;
}

std::string Launch::GetRockchipDmaHeapPath() const {
    return m_rockchip_dma_heap_path_;
}

}  // namespace inspire