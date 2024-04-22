//
// Created by tunm on 2024/4/17.
//

#include "launch.h"
#include "log.h"
#include "herror.h"


namespace inspire {

std::mutex Launch::mutex_;
std::shared_ptr<Launch> Launch::instance_ = nullptr;

InspireArchive& Launch::getMArchive() {
    return m_archive_;
}

std::shared_ptr<Launch> Launch::GetInstance() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!instance_) {
        instance_ = std::shared_ptr<Launch>(new Launch());
    }
    return instance_;
}

int32_t Launch::Load(const std::string &path) {
    if (!m_load_) {
        m_archive_.ReLoad(path);
        if (m_archive_.QueryStatus() == SARC_SUCCESS) {
            m_load_ = true;
            return HSUCCEED;
        } else {
            return HERR_ARCHIVE_LOAD_MODEL_FAILURE;
        }
    } else {
        INSPIRE_LOGE("Do not reload the model.");
        return HERR_ARCHIVE_REPETITION_LOAD;
    }
}

bool Launch::isMLoad() const {
    return m_load_;
}

}   // namespace inspire