// Created by tunm on 2024/04/17.

#ifndef INSPIREFACE_LAUNCH_H
#define INSPIREFACE_LAUNCH_H
#include "middleware/model_archive/inspire_archive.h"
#include <mutex>

#ifndef INSPIRE_API
#define INSPIRE_API
#endif

#define INSPIRE_LAUNCH inspire::Launch::GetInstance()

namespace inspire {

class INSPIRE_API Launch {
public:
    Launch(const Launch&) = delete;
    Launch& operator=(const Launch&) = delete;

    static std::shared_ptr<Launch> GetInstance();

    int32_t Load(const std::string &path);
    InspireArchive& getMArchive();

    bool isMLoad() const;

private:
    Launch() : m_load_(false) {}

    static std::mutex mutex_;                         ///< Mutex lock
    static std::shared_ptr<Launch> instance_;     ///< FeatureHub Instance

    InspireArchive m_archive_;
    bool m_load_;
};


}   // namespace inspire

#endif //INSPIREFACE_LAUNCH_H
