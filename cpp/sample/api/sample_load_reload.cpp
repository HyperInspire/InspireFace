#include <iostream>
#include "inspireface/c_api/inspireface.h"

int main() {
    std::string resourcePath = "test_res/pack/Pikachu";
    HResult ret = HFReloadInspireFace(resourcePath.c_str());
    if (ret != HSUCCEED) {
        std::cerr << "Failed to launch InspireFace: " << ret << std::endl;
        return 1;
    }

    // Switch to another resource
    ret = HFReloadInspireFace("test_res/pack/Megatron");
    if (ret != HSUCCEED) {
        std::cerr << "Failed to reload InspireFace: " << ret << std::endl;
        return 1;
    }

    return 0;
}
