//
// Created by Tunm-Air13 on 2023/9/15.
//

#include "FacePoseQuality.h"
#include "utils.h"

namespace hyper {


std::vector<float> FacePoseQuality::operator()(const Matrix &bgr_affine) {
    return std::vector<float>();
}

cv::Mat FacePoseQuality::ComputeCropMatrix(const cv::Rect2f &rect) {
    float x = rect.x;
    float y = rect.y;
    float w = rect.width;
    float h = rect.height;
    float cx = x + w / 2;
    float cy = y + h / 2;
    float length = std::max(w, h) * 1.5 / 2;
    float x1 = cx - length;
    float y1 = cy - length;
    float x2 = cx + length;
    float y2 = cy + length;
    cv::Rect2f padding_rect(x1, y1, x2 - x1, y2 - y1);
    std::vector<cv::Point2f> rect_pts = Rect2Points(padding_rect);
    rect_pts.erase(rect_pts.end() - 1);
    std::vector<cv::Point2f> dst_pts = {{0, 0}, {INPUT_WIDTH, INPUT_HEIGHT}, {INPUT_WIDTH, INPUT_HEIGHT}};
    cv::Mat m = cv::getAffineTransform(rect_pts, dst_pts);

    return m;
}


}   // namespace hyper