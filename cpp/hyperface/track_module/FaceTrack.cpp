//
// Created by tunm on 2023/8/29.
//

#include "FaceTrack.h"
#include "config.h"
#include "log.h"
#include "model_index.h"
#include "landmark/mean_shape.h"

namespace hyper {

FaceTrack::FaceTrack(int max_detected_faces):max_detected_faces_(max_detected_faces) {

}



void FaceTrack::SparseLandmarkPredict(const cv::Mat &raw_face_crop, std::vector<cv::Point2f> &landmarks_output,
                                         float &score, float size) {
//    LOGD("ready to landmark predict");
    landmarks_output.resize(FaceLandmark::NUM_OF_LANDMARK);
    std::vector<float> lmk_out = (*m_landmark_predictor_)(raw_face_crop);
    for (int i = 0; i < FaceLandmark::NUM_OF_LANDMARK; ++i) {
        float x = lmk_out[i * 2 + 0] * 112;
        float y = lmk_out[i * 2 + 1] * 112;
        landmarks_output[i] = cv::Point2f(x, y);
    }
    score = (*m_refine_net_)(raw_face_crop);

//    LOGD("predict ok ,score: %f", score);

}

bool FaceTrack::TrackFace(CameraStream &image, FaceObject &face) {
    if (face.GetConfidence() < 0.1) {
        face.DisableTracking();
//        LOGD("flag disable TrackFace");
        return false;
    }
//    LOGD("start track one");
    cv::Mat affine;
    std::vector<cv::Point2f> landmark_back;

    face.IncrementTrackingCount();

    float score;
    if (face.TrackingState() == DETECT) {
        cv::Rect rect_square = face.GetRectSquare(0.0);
        std::vector<cv::Point2f> rect_pts = Rect2Points(rect_square);
        cv::Mat rotation_mode_affine = image.GetAffineMatrix();
        assert(rotation_mode_affine.rows == 2);
        assert(rotation_mode_affine.cols == 3);
        cv::Mat rotation_mode_affine_inv;
        cv::invertAffineTransform(rotation_mode_affine, rotation_mode_affine_inv);
        rotation_mode_affine_inv = rotation_mode_affine;
        std::vector<cv::Point2f> camera_pts =
                ApplyTransformToPoints(rect_pts, rotation_mode_affine_inv);
        camera_pts.erase(camera_pts.end() - 1);
        std::vector<cv::Point2f> dst_pts = {{0,   0},
                                            {112, 0},
                                            {112, 112}};
        assert(dst_pts.size() == camera_pts.size());
        affine = cv::getAffineTransform(camera_pts, dst_pts);
        // affine = GetRectSquareAffine(rect_square);
        face.setTransMatrix(affine);
    }

    affine = face.getTransMatrix();
//    cout << affine << endl;
    cv::Mat crop;
//    LOGD("get affine crop ok");
    double time1 = (double) cv::getTickCount();
    crop = image.GetAffineRGBImage(affine, 112, 112);

    // pose
    auto pose = (*m_pose_net_)(crop);
    face.setPoseEulerAngle(pose);

    cv::Mat affine_inv;
    cv::invertAffineTransform(affine, affine_inv);
    double _diff =
            (((double) cv::getTickCount() - time1) / cv::getTickFrequency()) * 1000;
//    LOGD("affine cost %f", _diff);

    std::vector<cv::Point2f> landmark_rawout;
    std::vector<float> bbox;
    auto timeStart = (double) cv::getTickCount();
    SparseLandmarkPredict(crop, landmark_rawout, score, 112);
    vector<cv::Point2f> lmk_5 = {landmark_rawout[FaceLandmark::LEFT_EYE_CENTER],
                                 landmark_rawout[FaceLandmark::RIGHT_EYE_CENTER],
                                 landmark_rawout[FaceLandmark::NOSE_CORNER],
                                 landmark_rawout[FaceLandmark::MOUTH_LEFT_CORNER],
                                 landmark_rawout[FaceLandmark::MOUTH_RIGHT_CORNER]};
    face.setAlignMeanSquareError(lmk_5);
    double nTime =
            (((double) cv::getTickCount() - timeStart) / cv::getTickFrequency()) *
            1000;
//    LOGD("sparse cost %f", nTime);

    landmark_back.resize(landmark_rawout.size());
    landmark_back = ApplyTransformToPoints(landmark_rawout, affine_inv);
    int MODE = 1;


    if (MODE > 0) {
        if (face.TrackingState() == DETECT) {
            face.ReadyTracking();
        } else if (face.TrackingState() == READY ||
                   face.TrackingState() == TRACKING) {
            cv::Mat trans_m(2, 3, CV_64F);
            cv::Mat tmp = face.getTransMatrix();
            std::vector<cv::Point2f> inside_points = landmark_rawout;
            //      std::vector<cv::Point2f> inside_points =
            //      ApplyTransformToPoints(face.GetLanmdark(), tmp);
            cv::Mat _affine(2, 3, CV_64F);
            std::vector<cv::Point2f> mean_shape_(FaceLandmark::NUM_OF_LANDMARK);
            for (int k = 0; k < FaceLandmark::NUM_OF_LANDMARK; k++) {
                mean_shape_[k].x = mean_shape[k * 2];
                mean_shape_[k].y = mean_shape[k * 2 + 1];
            }
            SimilarityTransformEstimate(inside_points, mean_shape_, _affine);
            inside_points = ApplyTransformToPoints(inside_points, _affine);
            // RotPoints(inside_points,7.5);
            inside_points = FixPointsMeanshape(inside_points, mean_shape_);
            // inside_points = FixPoints(inside_points);

            //      std::vector<cv::Point2f> proxy_back(5);
            //      std::vector<cv::Point2f> proxy_inside(5);
            //      std::vector<int> index{72 ,105, 69 , 45 ,50};
            //      for(int i =0 ; i < index.size();i++){
            //          proxy_back[i] = inside_points[index[i]];
            //          proxy_inside[i] = landmark_back[index[i]];
            //      }
            //
            // SimilarityTransformEstimate(proxy_back, proxy_inside, trans_m);
            SimilarityTransformEstimate(landmark_back, inside_points, trans_m);
            face.setTransMatrix(trans_m);
            face.EnableTracking();
//            LOGD("ready face TrackFace state %d  ", face.TrackingState());

        }
    }

    // realloc many times
    face.SetLandmark(landmark_back, true);
    face.SetConfidence(score);
    face.UpdateFaceAction();
    return true;
}

void FaceTrack::UpdateStream(CameraStream &image, bool is_detect) {
    auto timeStart = (double) cv::getTickCount();
    detection_index_ += 1;
    if (is_detect)
        trackingFace.clear();
//    LOGD("%d, %d", detection_index_, detection_interval_);
    if (detection_index_ % detection_interval_ == 0 || is_detect) {
        cv::Mat image_detect = image.GetPreviewImage(true);
        nms();
        for (auto const &face: trackingFace) {
            cv::Rect m_mask_rect = face.GetRectSquare();
            std::vector<cv::Point2f> pts = Rect2Points(m_mask_rect);
            cv::Mat rotation_mode_affine = image.GetAffineMatrix();
            cv::Mat rotation_mode_affine_inv;
            cv::invertAffineTransform(rotation_mode_affine, rotation_mode_affine_inv);
            std::vector<cv::Point2f> affine_pts =
                    ApplyTransformToPoints(pts, rotation_mode_affine_inv);
            cv::Rect mask_rect = cv::boundingRect(affine_pts);
            BlackingTrackingRegion(image_detect, mask_rect);
        }
        // do detection in thread
//        LOGD("detect scaled rows: %d cols: %d", image_detect.rows,
//             image_detect.cols);
        auto timeStart = (double) cv::getTickCount();
        DetectFace(image_detect, image.GetPreviewScale());
        det_use_time_ = ((double) cv::getTickCount() - timeStart) / cv::getTickFrequency() * 1000;
//        LOGD("detect track");
    }

    if (!candidate_faces_.empty()) {
//        LOGD("push track face");
        for (int i = 0; i < candidate_faces_.size(); i++) {
            trackingFace.push_back(candidate_faces_[i]);
        }
        candidate_faces_.clear();
    }

    for (vector<FaceObject>::iterator iter = trackingFace.begin();
         iter != trackingFace.end();) {
        if (!TrackFace(image, *iter)) {
            iter = trackingFace.erase(iter);
        } else {
            iter++;
        }
    }
    track_total_use_time_ = ((double) cv::getTickCount() - timeStart) / cv::getTickFrequency() * 1000;

}

void FaceTrack::nms(float th) {
    std::sort(trackingFace.begin(), trackingFace.end(), [](FaceObject a, FaceObject b) { return a.confidence_ > b.confidence_; });
    std::vector<float> area(trackingFace.size());
    for (int i = 0; i < int(trackingFace.size()); ++i) {
        area[i] = trackingFace.at(i).getBbox().area();
    }
    for (int i = 0; i < int(trackingFace.size()); ++i) {
        for (int j = i + 1; j < int(trackingFace.size());) {
            float xx1 = (std::max)(trackingFace[i].getBbox().x, trackingFace[j].getBbox().x);
            float yy1 = (std::max)(trackingFace[i].getBbox().y, trackingFace[j].getBbox().y);
            float xx2 = (std::min)(trackingFace[i].getBbox().x + trackingFace[i].getBbox().width, trackingFace[j].getBbox().x + trackingFace[j].getBbox().width);
            float yy2 = (std::min)(trackingFace[i].getBbox().y + trackingFace[i].getBbox().height, trackingFace[j].getBbox().y + trackingFace[j].getBbox().height);
            float w = (std::max)(float(0), xx2 - xx1 + 1);
            float h = (std::max)(float(0), yy2 - yy1 + 1);
            float inter = w * h;
            float ovr = inter / (area[i] + area[j] - inter);
            if (ovr >= th) {
                trackingFace.erase(trackingFace.begin() + j);
                area.erase(area.begin() + j);
            } else {
                j++;
            }
        }
    }
}

void FaceTrack::BlackingTrackingRegion(cv::Mat &image, cv::Rect &rect_mask) {
    int height = image.rows;
    int width = image.cols;
    cv::Rect safe_rect = ComputeSafeRect(rect_mask, height, width);
    cv::Mat subImage = image(safe_rect);
    subImage.setTo(0);
}

void FaceTrack::DetectFace(const cv::Mat &input, float scale) {
    std::vector<FaceLoc> boxes = (*m_face_detector_)(input);
    std::vector<cv::Rect> bbox;
    bbox.resize(boxes.size());
    for (int i = 0; i < boxes.size(); i++) {
        bbox[i] = cv::Rect(cv::Point(static_cast<int>(boxes[i].x1), static_cast<int>(boxes[i].y1)),
                           cv::Point(static_cast<int>(boxes[i].x2), static_cast<int>(boxes[i].y2)));
        tracking_idx_ = tracking_idx_ + 1;
        FaceObject faceinfo(tracking_idx_, bbox[i], FaceLandmark::NUM_OF_LANDMARK);
        faceinfo.detect_bbox_ = bbox[i];

        // 控制检测到的人脸数量不超过最大限制
        if (candidate_faces_.size() < max_detected_faces_) {
            candidate_faces_.push_back(faceinfo);
        } else {
            // 如果超过了最大限制，您可以选择丢弃当前检测到的人脸或根据策略来选择要丢弃的人脸
            // 例如，可以比较人脸置信度，丢弃置信度较低的人脸
            // 这里以直接丢弃最后一个人脸为例
            candidate_faces_.pop_back();
        }
    }
}

int FaceTrack::Configuration(ModelLoader &loader) {
    InitDetectModel(loader.ReadModel(ModelIndex::_00_fdet_160));
    InitLandmarkModel(loader.ReadModel(ModelIndex::_01_lmk));
    InitRNetModel(loader.ReadModel(ModelIndex::_04_refine_net));
    InitFacePoseModel(loader.ReadModel(ModelIndex::_02_pose_fp16));
    return 0;
}

int FaceTrack::InitLandmarkModel(Model *model) {
    Parameter param;
    param.set<int>("model_index", ModelIndex::_01_lmk);
    param.set<string>("input_layer", "input_1");
    param.set<vector<string>>("outputs_layers", {"prelu1/add", });
    param.set<vector<int>>("input_size", {112, 112});
    param.set<vector<float>>("mean", {127.5f, 127.5f, 127.5f});
    param.set<vector<float>>("norm", {0.0078125f, 0.0078125f, 0.0078125f});

    m_landmark_predictor_ = std::make_shared<FaceLandmark>(112);
    m_landmark_predictor_->LoadParam(param, model);

    return 0;
}

int FaceTrack::InitDetectModel(Model *model) {
    Parameter param;
    param.set<int>("model_index", ModelIndex::_00_fdet_160);
    param.set<string>("input_layer", "input.1");
    param.set<vector<string>>("outputs_layers", {"443", "468", "493", "446", "471", "496", "449", "474", "499"});
    param.set<vector<int>>("input_size", {160, 160});
    param.set<vector<float>>("mean", {127.5f, 127.5f, 127.5f});
    param.set<vector<float>>("norm", {0.0078125f, 0.0078125f, 0.0078125f});

    m_face_detector_ = std::make_shared<FaceDetect>(160);
    m_face_detector_->LoadParam(param, model);

    return 0;
}

int FaceTrack::InitRNetModel(Model *model) {
    Parameter param;
    param.set<int>("model_index", ModelIndex::_04_refine_net);
    param.set<string>("input_layer", "data");
    param.set<vector<string>>("outputs_layers", {"prob1", "conv5-2"});
    param.set<vector<int>>("input_size", {24, 24});
    param.set<vector<float>>("mean", {127.5f, 127.5f, 127.5f});
    param.set<vector<float>>("norm", {0.0078125f, 0.0078125f, 0.0078125f});
    param.set<bool>("swap_color", true);        // RGB mode

    m_refine_net_ = std::make_shared<RNet>();
    m_refine_net_->LoadParam(param, model);

    return 0;
}

int FaceTrack::InitFacePoseModel(Model *model) {
    Parameter param;
    param.set<int>("model_index", ModelIndex::_02_pose_fp16);
    param.set<string>("input_layer", "data");
    param.set<vector<string>>("outputs_layers", {"ip3_pose", });
    param.set<vector<int>>("input_size", {112, 112});
    param.set<vector<float>>("mean", {0.0f, 0.0f, 0.0f});
    param.set<vector<float>>("norm", {1.0f, 1.0f, 1.0f});
    param.set<int>("input_channel", 1);        // Input Gray
    param.set<int>("input_image_channel", 1);        // BGR 2 Gray

    m_pose_net_ = std::make_shared<FacePose>();
    m_pose_net_->LoadParam(param, model);

    return 0;
}

double FaceTrack::GetTrackTotalUseTime() const {
    return track_total_use_time_;
}


}   // namespace hyper
