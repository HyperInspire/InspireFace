#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <inspirecv/inspirecv.h>
#include <inspireface/include/inspireface/session.h>
#include <inspireface/include/inspireface/launch.h>
#include <inspireface/middleware/thread/resource_pool.h>
#include <thread>

int main(int argc, char** argv) {
    std::string model_path = argv[1];
    std::string image_path = argv[2];
    int loop = 100000;
    int thread_num = 4;
    INSPIREFACE_CONTEXT->Load(model_path);
    inspirecv::Image image = inspirecv::Image::Create(image_path);
    inspirecv::FrameProcess process =
      inspirecv::FrameProcess::Create(image.Data(), image.Height(), image.Width(), inspirecv::BGR, inspirecv::ROTATION_0);

    inspire::parallel::ResourcePool<inspire::Session> sessionPool(thread_num, [](inspire::Session& session) {

    });

    for (int i = 0; i < thread_num; ++i) {
        inspire::CustomPipelineParameter param;
        param.enable_recognition = true;
        param.enable_liveness = true;
        param.enable_mask_detect = true;
        param.enable_face_attribute = true;
        param.enable_face_quality = true;
        inspire::Session session = inspire::Session::Create(inspire::DetectModuleMode::DETECT_MODE_ALWAYS_DETECT, 1, param);
        sessionPool.AddResource(std::move(session));
    }

    std::vector<std::thread> threads;
    int tasksPerThread = loop / thread_num;
    int remainingTasks = loop % thread_num;

    for (int i = 0; i < thread_num; ++i) {
        int taskCount = tasksPerThread + (i < remainingTasks ? 1 : 0);
        threads.emplace_back([&, taskCount]() {
            for (int j = 0; j < taskCount; ++j) {
                auto sessionGuard = sessionPool.AcquireResource();
                std::vector<inspire::FaceTrackWrap> results;
                int32_t ret;
                ret = sessionGuard->FaceDetectAndTrack(process, results);
                if (ret != 0) {
                    std::cerr << "FaceDetectAndTrack failed" << std::endl;
                    return ret;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }
    return 0;
}
