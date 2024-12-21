#ifndef INSPIRE_FACE_NEXUS_IMAGE_PROCESSOR_H
#define INSPIRE_FACE_NEXUS_IMAGE_PROCESSOR_H

#include <iostream>
#include <vector>
#include <inspirecv/inspirecv.h>
#include <memory>

namespace inspire {

namespace nexus {

class ImageProcessor {
public:
    static std::unique_ptr<ImageProcessor> Create();

public:
    virtual ~ImageProcessor() = 0;
    virtual int32_t Resize(const uint8_t* src_data, int src_width, int src_height, int channels, uint8_t** dst_data, int dst_width,
                           int dst_height) = 0;

    virtual int32_t SwapColor(const uint8_t* src_data, int src_width, int src_height, int channels, uint8_t** dst_data) = 0;

    virtual int32_t Padding(const uint8_t* src_data, int src_width, int src_height, int channels, int top, int bottom, int left, int right,
                            uint8_t** dst_data, int& dst_width, int& dst_height) = 0;

    virtual int32_t ResizeAndPadding(const uint8_t* src_data, int src_width, int src_height, int channels, int dst_width, int dst_height,
                                     uint8_t** dst_data, float& scale) = 0;

    virtual int32_t MarkDone() = 0;

    virtual void DumpCacheStatus() const = 0;

};  // class ImageProcessor

}  // namespace nexus

}  // namespace inspire

#endif  // INSPIRE_FACE_NEXUS_IMAGE_PROCESSOR_H
