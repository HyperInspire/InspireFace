#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <inspirecv/inspirecv.h>
#include <inspireface/include/inspireface/session.h>
#include <inspireface/include/inspireface/launch.h>

int main(int argc, char** argv) {
    std::string model_path = argv[1];
    std::string image_path = argv[2];
    INSPIREFACE_CONTEXT->Load(model_path);
    inspirecv::Image image = inspirecv::Image::Create(image_path);
    inspirecv::FrameProcess process =
      inspirecv::FrameProcess::Create(image.Data(), image.Height(), image.Width(), inspirecv::BGR, inspirecv::ROTATION_0);

    inspire::CustomPipelineParameter param;
    param.enable_recognition = true;
    param.enable_liveness = true;
    param.enable_mask_detect = true;
    param.enable_face_attribute = true;
    param.enable_face_quality = true;
    inspire::Session session = inspire::Session::Create(inspire::DetectModuleMode::DETECT_MODE_ALWAYS_DETECT, 1, param);

    std::vector<inspire::HyperFaceData> results;
    int32_t ret;
    ret = session.FaceDetectAndTrack(process, results);
    if (ret != 0) {
        std::cerr << "FaceDetectAndTrack failed" << std::endl;
        return ret;
    }
    for (auto& result : results) {
        std::cout << "result: " << result.trackId << std::endl;
        std::cout << "quality: " << result.quality[0] << ", " << result.quality[1] << ", " << result.quality[2] << ", " << result.quality[3] << ", "
                  << result.quality[4] << std::endl;
        inspirecv::Rect2i rect = inspirecv::Rect2i::Create(result.rect.x, result.rect.y, result.rect.width, result.rect.height);
        image.DrawRect(rect, inspirecv::Color::Red);
    }
    image.Write("result.jpg");

    inspirecv::Image wrapped;
    session.GetFaceAlignmentImage(process, results[0], wrapped);
    wrapped.Write("wrapped.jpg");
}
