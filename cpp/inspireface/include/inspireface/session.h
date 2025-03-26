#ifndef INSPIRE_FACE_SESSION_H
#define INSPIRE_FACE_SESSION_H
#include <memory>
#include <data_type.h>
#include <frame_process.h>
#include <face_data_type.h>

namespace inspire {

class INSPIRE_API Session {
public:
    static Session Create(DetectModuleMode detect_mode, int32_t max_detect_face, CustomPipelineParameter param, int32_t detect_level_px = -1,
                          int32_t track_by_detect_mode_fps = -1);

    Session();
    ~Session();

    Session(Session&&) noexcept;
    Session& operator=(Session&&) noexcept;

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    void SetTrackPreviewSize(int32_t preview_size);

    void SetFilterMinimumFacePixelSize(int32_t min_face_pixel_size);

    void SetFaceDetectThreshold(int32_t threshold);

    void SetTrackModeSmoothRatio(int32_t smooth_ratio);

    void SetTrackModeNumSmoothCacheFrame(int32_t num_smooth_cache_frame);

    void SetTrackModeDetectInterval(int32_t detect_interval);

    int32_t FaceDetectAndTrack(inspirecv::FrameProcess& process, std::vector<HyperFaceData>& results);

    int32_t FaceFeatureExtract(inspirecv::FrameProcess& process, HyperFaceData& data, FaceEmbedding& embedding, bool normalize = true);

    void GetFaceAlignmentImage(inspirecv::FrameProcess& process, HyperFaceData& data, inspirecv::Image& wrapped);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}  // namespace inspire

#endif  // INSPIRE_FACE_SESSION_H