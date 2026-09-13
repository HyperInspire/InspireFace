#include "inspireface.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

namespace {

struct Arguments {
    std::string pack;
    std::string face_image;
    std::string no_face_image;
    std::string run_id;
    std::string serial;
    std::string pack_sha256;
    std::string face_sha256;
    std::string no_face_sha256;
    std::string result_path;
};

struct Scenario {
    const char* status = "failure";
    const char* failure_stage = "arguments";
    HResult hresult = HERR_INVALID_PARAM;
    bool all_finite = false;
    long peak_rss_kb = 0;
};

class LaunchOwner {
 public:
    ~LaunchOwner() {
        if (launched_) {
            HFTerminateInspireFace();
        }
    }
    void MarkLaunched() { launched_ = true; }

 private:
    bool launched_ = false;
};

class SessionOwner {
 public:
    ~SessionOwner() {
        if (handle_ != nullptr) {
            HFReleaseInspireFaceSession(handle_);
        }
    }
    PHFSession Out() { return &handle_; }
    HFSession Get() const { return handle_; }

 private:
    HFSession handle_ = nullptr;
};

class BitmapOwner {
 public:
    ~BitmapOwner() {
        if (bitmap_ != nullptr) {
            HFReleaseImageBitmap(bitmap_);
        }
    }
    PHFImageBitmap Out() { return &bitmap_; }
    HFImageBitmap Get() const { return bitmap_; }

 private:
    HFImageBitmap bitmap_ = nullptr;
};

class StreamOwner {
 public:
    ~StreamOwner() {
        if (handle_ != nullptr) {
            HFReleaseImageStream(handle_);
        }
    }
    PHFImageStream Out() { return &handle_; }
    HFImageStream Get() const { return handle_; }

 private:
    HFImageStream handle_ = nullptr;
};

class FeatureOwner {
 public:
    ~FeatureOwner() {
        if (feature_.data != nullptr) {
            HFReleaseFaceFeature(&feature_);
        }
    }
    HResult Allocate() {
        return HFCreateFaceFeature(&feature_);
    }
    HFFaceFeature Get() const { return feature_; }

 private:
    HFFaceFeature feature_ = {};
};

bool IsNonEmpty(const std::string& value) {
    return !value.empty();
}

bool ParseArguments(int argc, char** argv, Arguments* arguments) {
    if (arguments == nullptr || argc != 19) {
        return false;
    }
    for (int index = 1; index < argc; index += 2) {
        const std::string key(argv[index]);
        const std::string value(argv[index + 1]);
        if (key == "--pack") arguments->pack = value;
        else if (key == "--face-image") arguments->face_image = value;
        else if (key == "--no-face-image") arguments->no_face_image = value;
        else if (key == "--run-id") arguments->run_id = value;
        else if (key == "--serial") arguments->serial = value;
        else if (key == "--pack-sha256") arguments->pack_sha256 = value;
        else if (key == "--face-sha256") arguments->face_sha256 = value;
        else if (key == "--no-face-sha256") arguments->no_face_sha256 = value;
        else if (key == "--result-path") arguments->result_path = value;
        else return false;
    }
    return IsNonEmpty(arguments->pack) && IsNonEmpty(arguments->face_image) &&
           IsNonEmpty(arguments->no_face_image) && IsNonEmpty(arguments->run_id) &&
           IsNonEmpty(arguments->serial) && IsNonEmpty(arguments->pack_sha256) &&
           IsNonEmpty(arguments->face_sha256) && IsNonEmpty(arguments->no_face_sha256) &&
           IsNonEmpty(arguments->result_path);
}

std::string EscapeJson(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    static const char hex[] = "0123456789abcdef";
    for (unsigned char byte : value) {
        switch (byte) {
            case '\\': escaped += "\\\\"; break;
            case '"': escaped += "\\\""; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default:
                if (byte < 0x20U) {
                    escaped += "\\u00";
                    escaped += hex[(byte >> 4U) & 0x0fU];
                    escaped += hex[byte & 0x0fU];
                } else {
                    escaped += static_cast<char>(byte);
                }
                break;
        }
    }
    return escaped;
}

std::string BuildReport(const Arguments& arguments, const Scenario& scenario) {
    return "{\"run_id\":\"" + EscapeJson(arguments.run_id) +
           "\",\"serial\":\"" + EscapeJson(arguments.serial) +
           "\",\"pack_sha256\":\"" + EscapeJson(arguments.pack_sha256) +
           "\",\"face_image_sha256\":\"" + EscapeJson(arguments.face_sha256) +
           "\",\"no_face_image_sha256\":\"" + EscapeJson(arguments.no_face_sha256) +
           "\",\"scenarios\":[{\"name\":\"bootstrap\",\"status\":\"" +
           scenario.status + "\",\"failure_stage\":\"" + scenario.failure_stage +
           "\",\"hresult\":" + std::to_string(static_cast<int>(scenario.hresult)) +
           ",\"all_finite\":" + (scenario.all_finite ? "true" : "false") +
           ",\"peak_rss_kb\":" + std::to_string(scenario.peak_rss_kb) +
           ",\"latency_ms\":[]}] }\n";
}

bool WriteReport(const std::string& result_path, const std::string& report) {
    const std::string temporary_path = result_path + ".partial";
    std::FILE* file = std::fopen(temporary_path.c_str(), "wb");
    if (file == nullptr) {
        std::cerr << "cannot open temporary result file\n";
        return false;
    }
    const bool wrote = std::fwrite(report.data(), 1, report.size(), file) == report.size();
    const bool closed = std::fclose(file) == 0;
    if (!wrote || !closed || std::rename(temporary_path.c_str(), result_path.c_str()) != 0) {
        std::remove(temporary_path.c_str());
        std::cerr << "cannot atomically publish result file\n";
        return false;
    }
    return true;
}

void RunBootstrap(const Arguments& arguments, Scenario* scenario) {
    HFResourcePackInfo info = {};
    info.structSize = sizeof(info);
    info.structVersion = HF_RESOURCE_PACK_INFO_VERSION;
    HFStatus status = HFValidateResourcePack(arguments.pack.c_str(), &info);
    if (status != HSUCCEED) {
        scenario->failure_stage = "validate_pack";
        scenario->hresult = static_cast<HResult>(status);
        return;
    }
    LaunchOwner launch;
    HResult result = HFLaunchInspireFace(arguments.pack.c_str());
    if (result != HSUCCEED) {
        scenario->failure_stage = "launch";
        scenario->hresult = result;
        return;
    }
    launch.MarkLaunched();

    SessionOwner session;
    result = HFCreateInspireFaceSessionOptional(HF_ENABLE_FACE_RECOGNITION,
                                                HF_DETECT_MODE_ALWAYS_DETECT,
                                                1, -1, -1, session.Out());
    if (result != HSUCCEED) {
        scenario->failure_stage = "create_session";
        scenario->hresult = result;
        return;
    }
    BitmapOwner bitmap;
    result = HFCreateImageBitmapFromFilePath(arguments.face_image.c_str(), 3, bitmap.Out());
    if (result != HSUCCEED) {
        scenario->failure_stage = "create_bitmap";
        scenario->hresult = result;
        return;
    }
    StreamOwner stream;
    result = HFCreateImageStreamFromImageBitmap(bitmap.Get(), HF_CAMERA_ROTATION_0, stream.Out());
    if (result != HSUCCEED) {
        scenario->failure_stage = "create_stream";
        scenario->hresult = result;
        return;
    }
    HFMultipleFaceData faces = {};
    result = HFExecuteFaceTrack(session.Get(), stream.Get(), &faces);
    if (result != HSUCCEED || faces.detectedNum < 1) {
        scenario->failure_stage = result == HSUCCEED ? "detect_face" : "track";
        scenario->hresult = result == HSUCCEED ? HERR_SESS_PIPELINE_FAILURE : result;
        return;
    }
    FeatureOwner first_feature;
    FeatureOwner second_feature;
    result = first_feature.Allocate();
    if (result == HSUCCEED) result = second_feature.Allocate();
    if (result == HSUCCEED) result = HFFaceFeatureExtractTo(session.Get(), stream.Get(), faces.tokens[0], first_feature.Get());
    if (result == HSUCCEED) result = HFFaceFeatureExtractTo(session.Get(), stream.Get(), faces.tokens[0], second_feature.Get());
    HFloat similarity = 0.0F;
    if (result == HSUCCEED) result = HFFaceComparison(first_feature.Get(), second_feature.Get(), &similarity);
    if (result != HSUCCEED || !std::isfinite(similarity)) {
        scenario->failure_stage = result == HSUCCEED ? "feature_finite" : "feature";
        scenario->hresult = result == HSUCCEED ? HERR_SESS_PIPELINE_FAILURE : result;
        return;
    }
    scenario->status = "success";
    scenario->failure_stage = "";
    scenario->hresult = HSUCCEED;
    scenario->all_finite = true;
}

}  // namespace

int main(int argc, char** argv) {
    Arguments arguments;
    Scenario scenario;
    if (!ParseArguments(argc, argv, &arguments)) {
        std::cerr << "invalid arguments\n";
        return 2;
    }
    RunBootstrap(arguments, &scenario);
    if (!WriteReport(arguments.result_path, BuildReport(arguments, scenario))) {
        return 2;
    }
    return scenario.status[0] == 's' ? 0 : 1;
}
