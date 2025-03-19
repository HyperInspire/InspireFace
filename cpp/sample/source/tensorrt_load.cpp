#include <inspirecv/inspirecv.h>
#include "inspireface/initialization_module/launch.h"
#include "inspireface/middleware/model_archive/inspire_archive.h"
#include "inspireface/track_module/face_detect/face_detect_adapt.h"
#include "inspireface/track_module/landmark/face_landmark_adapt.h"
#include "inspireface/track_module/quality/face_pose_quality_adapt.h"
#include "inspireface/recognition_module/extract/extract_adapt.h"

int main() {
    std::string archivePath = "test_res/pack/Megatron_TRT";
    INSPIRE_LAUNCH->Load(archivePath);
}