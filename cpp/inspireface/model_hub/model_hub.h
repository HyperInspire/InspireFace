// Created by tunm on 2024/04/17.

#ifndef INSPIREFACE_MODEL_HUB_H
#define INSPIREFACE_MODEL_HUB_H
#include "middleware/model_archive/inspire_archive.h"
#include <mutex>

#ifndef INSPIRE_API
#define INSPIRE_API
#endif

#define MODEL_HUB inspire::ModelHub::GetInstance()

namespace inspire {

class INSPIRE_API ModelHub {
public:
    ModelHub(const ModelHub&) = delete;
    ModelHub& operator=(const ModelHub&) = delete;

    static std::shared_ptr<ModelHub> GetInstance();

    int32_t Load(const std::string &path);
    InspireArchive& getMArchive();

    bool isMLoad() const;

private:
    ModelHub() : m_load_(false) {}

    static std::mutex mutex_;                         ///< Mutex lock
    static std::shared_ptr<ModelHub> instance_;     ///< FeatureHub Instance

    InspireArchive m_archive_;
    bool m_load_;
};


}   // namespace inspire

#endif //INSPIREFACE_MODEL_HUB_H
