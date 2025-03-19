#include <inspirecv/inspirecv.h>
#include "inspireface/initialization_module/launch.h"
#include "inspireface/middleware/model_archive/inspire_archive.h"
#include "inspireface/track_module/face_detect/face_detect_adapt.h"
#include "inspireface/track_module/landmark/face_landmark_adapt.h"
#include "inspireface/track_module/quality/face_pose_quality_adapt.h"
#include "inspireface/recognition_module/extract/extract_adapt.h"

void test_face_detect() {
    inspire::InspireModel model;
    INSPIRE_LAUNCH->getMArchive().LoadModel("face_detect_160", model);
    auto input_size = 160;
    inspire::FaceDetectAdapt faceDetectAdapt(input_size);
    faceDetectAdapt.loadData(model, model.modelType);
    inspirecv::Image image = inspirecv::Image::Create("test_res/data/bulk/kun.jpg");
    inspire::FaceLocList faces;
    inspirecv::TimeSpend timeSpend("Face Detect@" + std::to_string(input_size));
    timeSpend.Start();
    for (int i = 0; i < 10; i++) {
        faces = faceDetectAdapt(image);
    }
    timeSpend.Stop();
    std::cout << timeSpend << std::endl;
    std::cout << "faces size: " << faces.size() << std::endl;
    for (auto &face : faces) {
        inspirecv::Rect2i rect = inspirecv::Rect2i::Create(face.x1, face.y1, face.x2 - face.x1, face.y2 - face.y1);
        image.DrawRect(rect, {0, 0, 255});
    }
    image.Write("im.jpg");
}

int main() {
    std::string archivePath = "test_res/pack/Megatron_TRT";
    INSPIRE_LAUNCH->Load(archivePath);
    test_face_detect();
    return 0;
}