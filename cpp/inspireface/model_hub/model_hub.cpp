//
// Created by tunm on 2024/4/17.
//

#include "model_hub.h"
#include "log.h"
#include "herror.h"


namespace inspire {

std::mutex ModelHub::mutex_;
std::shared_ptr<ModelHub> ModelHub::instance_ = nullptr;

InspireArchive& ModelHub::getMArchive() {
    return m_archive_;
}

std::shared_ptr<ModelHub> ModelHub::GetInstance() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!instance_) {
        instance_ = std::shared_ptr<ModelHub>(new ModelHub());
    }
    return instance_;
}

int32_t ModelHub::Load(const std::string &path) {
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

bool ModelHub::isMLoad() const {
    return m_load_;
}

}   // namespace inspire