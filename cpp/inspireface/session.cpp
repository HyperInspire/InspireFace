#include <memory>
#include "session.h"
#include "engine/face_session.h"
#include "recognition_module/dest_const.h"

namespace inspire {

class Session::Impl {
public:
    Impl() : m_face_session_(std::make_unique<FaceSession>()) {}

    int32_t Configure(DetectModuleMode detect_mode, int32_t max_detect_face, CustomPipelineParameter param, int32_t detect_level_px,
                      int32_t track_by_detect_mode_fps) {
        return m_face_session_->Configuration(detect_mode, max_detect_face, param, detect_level_px, track_by_detect_mode_fps);
    }

    ~Impl() = default;

    void SetTrackPreviewSize(int32_t preview_size) {
        m_face_session_->SetTrackPreviewSize(preview_size);
    }

    void SetFilterMinimumFacePixelSize(int32_t min_face_pixel_size) {
        m_face_session_->SetTrackFaceMinimumSize(min_face_pixel_size);
    }

    void SetFaceDetectThreshold(int32_t threshold) {
        m_face_session_->SetFaceDetectThreshold(threshold);
    }

    void SetTrackModeSmoothRatio(int32_t smooth_ratio) {
        m_face_session_->SetTrackModeSmoothRatio(smooth_ratio);
    }

    void SetTrackModeNumSmoothCacheFrame(int32_t num_smooth_cache_frame) {
        m_face_session_->SetTrackModeNumSmoothCacheFrame(num_smooth_cache_frame);
    }

    void SetTrackModeDetectInterval(int32_t detect_interval) {
        m_face_session_->SetTrackModeDetectInterval(detect_interval);
    }

    int32_t FaceDetectAndTrack(inspirecv::FrameProcess& process, std::vector<HyperFaceData>& results) {
        int32_t ret = m_face_session_->FaceDetectAndTrack(process);
        if (ret < 0) {
            return ret;
        }

        const auto& face_data = m_face_session_->GetDetectCache();
        for (const auto& data : face_data) {
            HyperFaceData hyper_face_data;
            RunDeserializeHyperFaceData(data, hyper_face_data);
            results.emplace_back(hyper_face_data);
        }

        return ret;
    }

    int32_t FaceFeatureExtract(inspirecv::FrameProcess& process, HyperFaceData& data, FaceEmbedding& embedding, bool normalize) {
        int32_t ret = m_face_session_->FaceFeatureExtract(process, data, normalize);
        if (ret < 0) {
            return ret;
        }
        embedding.isNormal = normalize;
        embedding.embedding = m_face_session_->GetFaceFeatureCache();
        embedding.norm = m_face_session_->GetFaceFeatureNormCache();

        return ret;
    }

    void GetFaceAlignmentImage(inspirecv::FrameProcess& process, HyperFaceData& data, inspirecv::Image& wrapped) {
        std::vector<inspirecv::Point2f> pointsFive;
        for (const auto& p : data.keyPoints) {
            pointsFive.push_back(inspirecv::Point2f(p.x, p.y));
        }
        auto trans = inspirecv::SimilarityTransformEstimateUmeyama(SIMILARITY_TRANSFORM_DEST, pointsFive);
        wrapped = process.ExecuteImageAffineProcessing(trans, FACE_CROP_SIZE, FACE_CROP_SIZE);
    }

    std::unique_ptr<FaceSession> m_face_session_;
};

Session::Session() : pImpl(std::make_unique<Impl>()) {}

Session::~Session() = default;

Session::Session(Session&&) noexcept = default;

Session& Session::operator=(Session&&) noexcept = default;

Session Session::Create(DetectModuleMode detect_mode, int32_t max_detect_face, CustomPipelineParameter param, int32_t detect_level_px,
                        int32_t track_by_detect_mode_fps) {
    Session session;
    session.pImpl->Configure(detect_mode, max_detect_face, param, detect_level_px, track_by_detect_mode_fps);
    return session;
}

void Session::SetTrackPreviewSize(int32_t preview_size) {
    pImpl->SetTrackPreviewSize(preview_size);
}

void Session::SetFilterMinimumFacePixelSize(int32_t min_face_pixel_size) {
    pImpl->SetFilterMinimumFacePixelSize(min_face_pixel_size);
}

void Session::SetFaceDetectThreshold(int32_t threshold) {
    pImpl->SetFaceDetectThreshold(threshold);
}

void Session::SetTrackModeSmoothRatio(int32_t smooth_ratio) {
    pImpl->SetTrackModeSmoothRatio(smooth_ratio);
}

void Session::SetTrackModeNumSmoothCacheFrame(int32_t num_smooth_cache_frame) {
    pImpl->SetTrackModeNumSmoothCacheFrame(num_smooth_cache_frame);
}

void Session::SetTrackModeDetectInterval(int32_t detect_interval) {
    pImpl->SetTrackModeDetectInterval(detect_interval);
}

int32_t Session::FaceDetectAndTrack(inspirecv::FrameProcess& process, std::vector<HyperFaceData>& results) {
    return pImpl->FaceDetectAndTrack(process, results);
}

int32_t Session::FaceFeatureExtract(inspirecv::FrameProcess& process, HyperFaceData& data, FaceEmbedding& embedding, bool normalize) {
    return pImpl->FaceFeatureExtract(process, data, embedding, normalize);
}

void Session::GetFaceAlignmentImage(inspirecv::FrameProcess& process, HyperFaceData& data, inspirecv::Image& wrapped) {
    pImpl->GetFaceAlignmentImage(process, data, wrapped);
}

}  // namespace inspire
