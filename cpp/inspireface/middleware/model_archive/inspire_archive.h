//
// Created by tunm on 2024/3/30.
//

#ifndef MODELLOADERTAR_INSPIREARCHIVE_H
#define MODELLOADERTAR_INSPIREARCHIVE_H
#include "sample_archive.h"
#include "inspire_model/inspire_model.h"
#include "yaml-cpp/yaml.h"
#include "fstream"

namespace inspire {

enum {
    MISS_MANIFEST = -11,
    FORMAT_ERROR = -12,
    NOT_MATCH_MODEL = -13,
    ERROR_MODEL_BUFFER = -14,
};

class INSPIRE_API InspireArchive: SimpleArchive {
public:
    InspireArchive() : SimpleArchive() {
        m_status_ = loadManifestFile();
    }

    explicit InspireArchive(const std::string& archiveFile) : SimpleArchive(archiveFile) {
        m_status_ = loadManifestFile();
    }

    int32_t QueryStatus() const {
        return m_status_;
    }

    int32_t LoadModel(const std::string &name, InspireModel &model) {
        if (m_config_[name]) {
            auto ret = model.Reset(m_config_[name]);
            if (ret != 0) {
                return ret;
            }
            auto &buffer = GetFileContent(model.name);
            if (buffer.empty()) {
                return ERROR_MODEL_BUFFER;
            }
            model.SetBuffer(buffer, buffer.size());
            return SARC_SUCCESS;
        } else {
            return NOT_MATCH_MODEL;
        }
    }

private:

    int32_t loadManifestFile() {
        if (QueryLoadStatus() == SARC_SUCCESS) {
            auto configBuffer = GetFileContent(MANIFEST_FILE);
            if (configBuffer.empty()) {
                return MISS_MANIFEST;
            }
            m_config_ = YAML::Load(configBuffer.data());
            if (!m_config_["tag"] || !m_config_["version"]) {
                return FORMAT_ERROR;
            }
        }
        return 0;
    }

private:
    YAML::Node m_config_;

    int32_t m_status_;

    const std::string MANIFEST_FILE = "__inspire__";

};

}   // namespace inspire

#endif //MODELLOADERTAR_INSPIREARCHIVE_H
