//
// Created by Tunm-Air13 on 2023/5/6.
//

#pragma once
#ifndef BIGGUYSMAIN_FACE_MODULE_FACE_DETECT_FACEDETECT_H
#define BIGGUYSMAIN_FACE_MODULE_FACE_DETECT_FACEDETECT_H
#include "../../DataType.h"
#include "middleware/AnyNet.h"

namespace hyper {

class HYPER_API FaceDetect: public AnyNet {
public:

    explicit FaceDetect(int input_size = 160, float nms_threshold = 0.5f, float = 0.5f);

    /**
     * Detect faces for one image
     * @param bgr image
     * @return detect results
     * */
    FaceLocList operator()(const Matrix& bgr);


private:

    /** Non-maximum suppression */
    static void _nms(FaceLocList &input_faces, float nms_threshold);

    /** The detection anchor frame is generated according to stride */
    void _generate_anchors(int stride, int input_size, int num_anchors, vector<float> &anchors);

    /** Network post-processing decoding */
    void _decode(const std::vector<float> &cls_pred, const std::vector<float> &box_pred, const std::vector<float>& lmk_pred, int stride, std::vector<FaceLoc> &results);

private:
    float m_nms_threshold_;
    float m_cls_threshold_;
    int m_input_size_;

};


/** Sort FaceLoc in descending order of area */
bool SortBoxSize(const FaceLoc &a, const FaceLoc &b);

}


#endif //BIGGUYSMAIN_FACE_MODULE_FACE_DETECT_FACEDETECT_H
