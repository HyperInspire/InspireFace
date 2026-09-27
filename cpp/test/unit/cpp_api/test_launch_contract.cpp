#include <string>
#include <vector>

#include <inspireface/include/inspireface/inspireface.hpp>

#include "settings/test_settings.h"

namespace {

class LaunchConfigurationReset {
public:
    explicit LaunchConfigurationReset(const std::shared_ptr<inspire::Launch>& launch)
    : launch_(launch),
      rockchip_path_(launch->GetRockchipDmaHeapPath()),
      coreml_mode_(launch->GetGlobalCoreMLInferenceMode()),
      cuda_device_id_(launch->GetCudaDeviceId()),
      detect_pixels_(launch->GetFaceDetectPixelList()),
      detect_models_(launch->GetFaceDetectModelList()),
      image_backend_(launch->GetImageProcessingBackend()),
      aligned_width_(launch->GetImageProcessAlignedWidth()) {}

    ~LaunchConfigurationReset() {
        launch_->SetRockchipDmaHeapPath(rockchip_path_);
        launch_->SetGlobalCoreMLInferenceMode(coreml_mode_);
        launch_->SetCudaDeviceId(cuda_device_id_);
        launch_->SetFaceDetectPixelList(detect_pixels_);
        launch_->SetFaceDetectModelList(detect_models_);
        launch_->SwitchImageProcessingBackend(image_backend_);
        launch_->SetImageProcessAlignedWidth(aligned_width_);
    }

private:
    std::shared_ptr<inspire::Launch> launch_;
    std::string rockchip_path_;
    inspire::Launch::NNInferenceBackend coreml_mode_;
    int32_t cuda_device_id_;
    std::vector<int32_t> detect_pixels_;
    std::vector<std::string> detect_models_;
    inspire::Launch::ImageProcessingBackend image_backend_;
    int32_t aligned_width_;
};

}  // namespace

TEST_CASE("C++ Launch singleton exposes a stable loaded resource", "[cpp_api][contract][launch]") {
    const auto first = inspire::Launch::GetInstance();
    const auto second = inspire::Launch::GetInstance();
    REQUIRE(first);
    CHECK(first == second);
    REQUIRE(first->isMLoad());
    CHECK_NOTHROW(first->getMArchive());

    CHECK(first->Load(GET_RUNTIME_FULLPATH_NAME) == HSUCCEED);
    CHECK(first->isMLoad());
}

TEST_CASE("C++ Launch failed reload preserves the active resource", "[cpp_api][contract][launch][boundary]") {
    const auto launch = inspire::Launch::GetInstance();
    REQUIRE(launch->isMLoad());
    const auto pixels = launch->GetFaceDetectPixelList();
    const auto models = launch->GetFaceDetectModelList();

    CHECK(launch->Reload(GET_DATA("missing/does-not-exist.pack")) == HERR_ARCHIVE_LOAD_FAILURE);
    CHECK(launch->isMLoad());
    CHECK(launch->GetFaceDetectPixelList() == pixels);
    CHECK(launch->GetFaceDetectModelList() == models);
    CHECK_NOTHROW(launch->getMArchive());
}

TEST_CASE("C++ Launch configuration getters round-trip and restore global state", "[cpp_api][contract][launch][configuration]") {
    const auto launch = inspire::Launch::GetInstance();
    LaunchConfigurationReset reset(launch);

    launch->SetRockchipDmaHeapPath("/contract/dma_heap");
    CHECK(launch->GetRockchipDmaHeapPath() == "/contract/dma_heap");

    launch->SetGlobalCoreMLInferenceMode(inspire::Launch::NN_INFERENCE_CPU);
    CHECK(launch->GetGlobalCoreMLInferenceMode() == inspire::Launch::NN_INFERENCE_CPU);
    launch->SetGlobalCoreMLInferenceMode(inspire::Launch::NN_INFERENCE_COREML_GPU);
    CHECK(launch->GetGlobalCoreMLInferenceMode() == inspire::Launch::NN_INFERENCE_COREML_GPU);
    launch->SetGlobalCoreMLInferenceMode(inspire::Launch::NN_INFERENCE_COREML_ANE);
    CHECK(launch->GetGlobalCoreMLInferenceMode() == inspire::Launch::NN_INFERENCE_COREML_ANE);

    launch->SetCudaDeviceId(7);
    CHECK(launch->GetCudaDeviceId() == 7);

    const std::vector<int32_t> pixels = {128, 256, 512};
    const std::vector<std::string> models = {"detect_128", "detect_256", "detect_512"};
    launch->SetFaceDetectPixelList(pixels);
    launch->SetFaceDetectModelList(models);
    CHECK(launch->GetFaceDetectPixelList() == pixels);
    CHECK(launch->GetFaceDetectModelList() == models);

    launch->SwitchImageProcessingBackend(inspire::Launch::IMAGE_PROCESSING_CPU);
    CHECK(launch->GetImageProcessingBackend() == inspire::Launch::IMAGE_PROCESSING_CPU);
    launch->SetImageProcessAlignedWidth(16);
    CHECK(launch->GetImageProcessAlignedWidth() == 16);
}
