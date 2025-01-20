/**
 * Created by Jingyu Yan
 * @date 2024-10-01
 */

#ifndef MODELLOADERTAR_INSPIREARCHIVE_H
#define MODELLOADERTAR_INSPIREARCHIVE_H
#include "simple_archive.h"
#include "inspire_model/inspire_model.h"
#include "yaml-cpp/yaml.h"
#include "fstream"
#include "recognition_module/similarity_converter.h"

namespace inspire {

enum {
    MISS_MANIFEST = -11,
    FORMAT_ERROR = -12,
    NOT_MATCH_MODEL = -13,
    ERROR_MODEL_BUFFER = -14,
    NOT_READ = -15,
};

class INSPIRE_API InspireArchive : SimpleArchive {
public:
    InspireArchive() : SimpleArchive() {
        m_status_ = NOT_READ;
    }

    explicit InspireArchive(const std::string &archiveFile) : SimpleArchive(archiveFile) {
        m_status_ = QueryStatus();
        if (m_status_ == SARC_SUCCESS) {
            m_status_ = loadManifestFile();
        }
    }

    int32_t ReLoad(const std::string &archiveFile) {
        auto ret = Reset(archiveFile);
        if (ret != SARC_SUCCESS) {
            Close();
            m_status_ = ret;
            return ret;
        }
        m_status_ = loadManifestFile();
        return m_status_;
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

    void PublicPrintSubFiles() {
        PrintSubFiles();
    }

    void Release() {
        m_status_ = NOT_READ;
        Close();
    }

private:
    int32_t loadManifestFile() {
        if (QueryLoadStatus() == SARC_SUCCESS) {
            auto configBuffer = GetFileContent(MANIFEST_FILE);
            configBuffer.push_back('\0');
            if (configBuffer.empty()) {
                return MISS_MANIFEST;
            }
            m_config_ = YAML::Load(configBuffer.data());
            if (!m_config_["tag"] || !m_config_["version"]) {
                return FORMAT_ERROR;
            }
            m_tag_ = m_config_["tag"].as<std::string>();
            m_version_ = m_config_["version"].as<std::string>();
            INSPIRE_LOGI("== %s %s ==", m_tag_.c_str(), m_version_.c_str());
            // Load similarity converter config
            if (m_config_["similarity_converter"]) {
                SimilarityConverterConfig config;
                config.threshold = m_config_["similarity_converter"]["threshold"].as<double>();
                config.middleScore = m_config_["similarity_converter"]["middle_score"].as<double>();
                config.steepness = m_config_["similarity_converter"]["steepness"].as<double>();
                config.outputMin = m_config_["similarity_converter"]["output_min"].as<double>();
                config.outputMax = m_config_["similarity_converter"]["output_max"].as<double>();
                SIMILARITY_CONVERTER_UPDATE_CONFIG(config);
                INSPIRE_LOGI(
                  "Successfully loaded similarity converter config: \n \t threshold: %f \n \t middle_score: %f \n \t steepness: %f \n \t output_min: "
                  "%f \n \t output_max: %f",
                  config.threshold, config.middleScore, config.steepness, config.outputMin, config.outputMax);
            } else {
                INSPIRE_LOGW("No similarity converter config found, use default config: ");
                auto config = SIMILARITY_CONVERTER_GET_CONFIG();
                INSPIRE_LOGI("threshold: %f \n \t middle_score: %f \n \t steepness: %f \n \t output_min: %f \n \t output_max: %f", config.threshold,
                             config.middleScore, config.steepness, config.outputMin, config.outputMax);
            }
        }
        return 0;
    }

private:
    YAML::Node m_config_;

    int32_t m_status_;

    const std::string MANIFEST_FILE = "__inspire__";

    std::string m_tag_;
    std::string m_version_;
};

}  // namespace inspire

#endif  // MODELLOADERTAR_INSPIREARCHIVE_H
