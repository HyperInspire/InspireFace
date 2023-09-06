//
// Created by Tunm-Air13 on 2023/9/6.
//

#include "FaceLandmark.h"

namespace hyper {


std::vector<cv::Point2f> FaceLandmark::operator()(const Matrix &bgr_affine) {
    return std::vector<cv::Point2f>();
}

FaceLandmark::FaceLandmark(int input_size):
    m_input_size(input_size){

}

const int FaceLandmark::getInputSize() const {
    return m_input_size;
}


}   //  namespace hyper