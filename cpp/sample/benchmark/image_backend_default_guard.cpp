#include <iostream>
#include <launch.h>

int main() {
    const auto launch = inspire::Launch::GetInstance();
    const auto initial = launch->GetImageProcessingBackend();
    launch->SwitchImageProcessingBackend(inspire::Launch::IMAGE_PROCESSING_RGA);
    const auto explicit_rga = launch->GetImageProcessingBackend();
    launch->SwitchImageProcessingBackend(inspire::Launch::IMAGE_PROCESSING_CPU);
    const auto explicit_cpu = launch->GetImageProcessingBackend();
    std::cout << "initial=" << initial << ",explicit_rga=" << explicit_rga
              << ",explicit_cpu=" << explicit_cpu << '\n';
    const auto expected_initial =
#if defined(ISF_ENABLE_RGA) && !defined(ISF_RKNPU_RV1106) && !defined(ISF_RKNPU_RV1126B)
      inspire::Launch::IMAGE_PROCESSING_RGA;
#else
      inspire::Launch::IMAGE_PROCESSING_CPU;
#endif
    if (initial != expected_initial || explicit_cpu != inspire::Launch::IMAGE_PROCESSING_CPU) {
        return 1;
    }
#if defined(ISF_ENABLE_RGA)
    if (explicit_rga != inspire::Launch::IMAGE_PROCESSING_RGA || launch->GetRockchipDmaHeapPath().empty()) {
        return 2;
    }
#if defined(ISF_RKNPU_RV1126B)
    if (launch->GetRockchipDmaHeapPath() != "/dev/dma_heap/system-uncached") {
        return 3;
    }
#elif !defined(ISF_RKNPU_RV1106)
    if (launch->GetRockchipDmaHeapPath() != "/dev/dma_heap/dma32-uncached") {
        return 3;
    }
#endif
#else
    if (explicit_rga != inspire::Launch::IMAGE_PROCESSING_CPU) {
        return 2;
    }
#endif
    return 0;
}
