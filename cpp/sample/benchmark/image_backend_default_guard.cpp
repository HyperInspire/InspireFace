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
    if (initial != inspire::Launch::IMAGE_PROCESSING_CPU || explicit_cpu != inspire::Launch::IMAGE_PROCESSING_CPU) {
        return 1;
    }
#if defined(ISF_ENABLE_RGA)
    if (explicit_rga != inspire::Launch::IMAGE_PROCESSING_RGA || launch->GetRockchipDmaHeapPath().empty()) {
        return 2;
    }
#else
    if (explicit_rga != inspire::Launch::IMAGE_PROCESSING_CPU) {
        return 2;
    }
#endif
    return 0;
}
