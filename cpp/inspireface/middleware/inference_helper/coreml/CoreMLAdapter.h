/**
 * Created by Jingyu Yan
 * @date 2025-02-01
 */

#ifndef INSPIRE_FACE_COREML_ADAPTER_H
#define INSPIRE_FACE_COREML_ADAPTER_H
#include <string>
#include <vector>

#ifdef __OBJC__
@class MLModel;
@protocol MLFeatureProvider;
#else
typedef void MLModel;
typedef void MLFeatureProvider;
#endif

#define COREML_HSUCCEED 0
#define COREML_HFAIL -1

/**
 * @brief CoreML adapter for inference
 */
class CoreMLAdapter {
public:
    enum class InferenceMode { CPU, GPU, ANE };
    CoreMLAdapter();
    ~CoreMLAdapter();

    int32_t readFromFile(const std::string &modelPath);
    static CoreMLAdapter readNetFrom(const std::string &modelPath);
    static CoreMLAdapter readNetFromBin(const std::vector<char> &model_data);

    std::vector<std::string> getInputNames() const;
    std::vector<std::string> getOutputNames() const;
    std::vector<int> getInputShapeByName(const std::string &name);
    std::vector<int> getOutputShapeByName(const std::string &name);

    void setInput(const char *inputName, const char *data);
    void forward();
    const char *getOutput(const char *nodeName);
    void setInferenceMode(InferenceMode mode);
    void printModelInfo() const;

private:
    class Impl;
    Impl *pImpl;
};

#endif  // INSPIRE_FACE_COREML_ADAPTER_H
