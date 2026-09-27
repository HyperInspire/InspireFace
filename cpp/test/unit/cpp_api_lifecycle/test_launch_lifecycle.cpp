#include <memory>
#include <string>
#include <vector>

#include <inspireface/include/inspireface/inspireface.hpp>

#include "settings/test_settings.h"

namespace {

class CppLaunchReset {
public:
    CppLaunchReset() {
        inspire::Launch::GetInstance()->Unload();
    }
    ~CppLaunchReset() {
        inspire::Launch::GetInstance()->Unload();
    }
};

}  // namespace

TEST_CASE("C++ component versions are available before launch", "[cpp_api][contract][lifecycle][metadata]") {
    CppLaunchReset reset;
    const auto launch = inspire::Launch::GetInstance();
    REQUIRE_FALSE(launch->isMLoad());

    const auto mnn_version = inspire::GetComponentVersion(inspire::ComponentType::MNN);
    CHECK(mnn_version.IsVersionKnown());
    CHECK(inspire::GetComponentVersionsString().find("inspireface=") == 0);
    CHECK_FALSE(inspire::GetDiagnosticInfo().empty());
    CHECK_FALSE(launch->isMLoad());
}

TEST_CASE("C++ Launch rejects unavailable archives without changing state", "[cpp_api][contract][lifecycle][boundary]") {
    CppLaunchReset reset;
    const auto launch = inspire::Launch::GetInstance();
    REQUIRE_FALSE(launch->isMLoad());
    CHECK_THROWS_AS(launch->getMArchive(), std::runtime_error);

    const std::string missing = GET_RUNTIME_FULLPATH_NAME + ".missing";
    CHECK(launch->Load(missing) == HERR_ARCHIVE_LOAD_FAILURE);
    CHECK_FALSE(launch->isMLoad());
    CHECK(launch->Reload(missing) == HERR_ARCHIVE_LOAD_FAILURE);
    CHECK_FALSE(launch->isMLoad());
}

TEST_CASE("C++ Session created before launch preserves its configuration failure", "[cpp_api][contract][lifecycle][session]") {
    CppLaunchReset reset;
    inspire::CustomPipelineParameter parameter;
    auto session = inspire::Session::Create(inspire::DETECT_MODE_ALWAYS_DETECT, 1, parameter);
    const auto image = inspirecv::Image::Create(GET_DATA("data/bulk/kun.jpg"));
    REQUIRE(!image.Empty());
    auto process = inspirecv::FrameProcess::Create(image, inspirecv::BGR, inspirecv::ROTATION_0);
    std::vector<inspire::FaceTrackWrap> faces(1);
    CHECK(session.FaceDetectAndTrack(process, faces) == HERR_ARCHIVE_NOT_LOAD);
    CHECK(faces.empty());
}

TEST_CASE("C++ Launch load and unload are idempotent", "[cpp_api][contract][lifecycle]") {
    CppLaunchReset reset;
    const auto launch = inspire::Launch::GetInstance();
    REQUIRE(launch->Load(GET_RUNTIME_FULLPATH_NAME) == HSUCCEED);
    REQUIRE(launch->isMLoad());
    CHECK_NOTHROW(launch->getMArchive());
    CHECK(launch->Load(GET_RUNTIME_FULLPATH_NAME) == HSUCCEED);
    launch->SwitchLandmarkEngine(inspire::Launch::LANDMARK_HYPLMV2_0_25);

    launch->Unload();
    CHECK_FALSE(launch->isMLoad());
    CHECK_THROWS_AS(launch->getMArchive(), std::runtime_error);
    CHECK_NOTHROW(launch->Unload());
}

TEST_CASE("C++ Launch reload initializes and failed reload preserves live sessions", "[cpp_api][contract][lifecycle]") {
    CppLaunchReset reset;
    const auto launch = inspire::Launch::GetInstance();
    REQUIRE(launch->Reload(GET_RUNTIME_FULLPATH_NAME) == HSUCCEED);
    REQUIRE(launch->isMLoad());

    inspire::CustomPipelineParameter parameter;
    auto session = inspire::Session::Create(inspire::DETECT_MODE_ALWAYS_DETECT, 1, parameter);
    const std::string missing = GET_RUNTIME_FULLPATH_NAME + ".missing";
    CHECK(launch->Reload(missing) == HERR_ARCHIVE_LOAD_FAILURE);
    CHECK(launch->isMLoad());

    const auto image = inspirecv::Image::Create(GET_DATA("data/bulk/kun.jpg"));
    REQUIRE(!image.Empty());
    auto process = inspirecv::FrameProcess::Create(image, inspirecv::BGR, inspirecv::ROTATION_0);
    std::vector<inspire::FaceTrackWrap> faces;
    REQUIRE(session.FaceDetectAndTrack(process, faces) == HSUCCEED);
    CHECK(faces.size() == 1);
}

TEST_CASE("C++ Launch extension configuration accepts an existing directory", "[cpp_api][contract][lifecycle][configuration]") {
    CppLaunchReset reset;
    const auto launch = inspire::Launch::GetInstance();
    launch->ConfigurationExtensionPath(GET_DIR);
    CHECK(launch->GetExtensionPath() == GET_DIR);

    const auto build_extension_path = &inspire::Launch::BuildAppleExtensionPath;
    CHECK(build_extension_path != nullptr);
#if defined(ISF_ENABLE_APPLE_EXTENSION)
    REQUIRE(launch->Load(GET_RUNTIME_FULLPATH_NAME) == HSUCCEED);
    launch->BuildAppleExtensionPath(GET_RUNTIME_FULLPATH_NAME);
    CHECK_FALSE(launch->GetExtensionPath().empty());
#endif
}
