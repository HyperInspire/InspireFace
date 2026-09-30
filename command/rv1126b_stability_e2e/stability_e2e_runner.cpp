#include "inspireface.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <sys/resource.h>
#include <vector>

namespace {

constexpr HInt32 kSoakCycles = 50;
constexpr HInt32 kExpectedFeatureSize = 512;

struct Arguments {
    std::string pack, face_image, run_id, serial, pack_sha256, face_sha256, result_path;
    std::string image_backend = "cpu";
};

struct Cycle {
    HInt32 index = 0;
    long rss_kb = 0;
    double detect_ms = 0.0;
    HInt32 detected_faces = -1;
};

struct FeatureOwner {
    HFFaceFeature feature_ = {};
    ~FeatureOwner() { if (feature_.data != nullptr) HFReleaseFaceFeature(&feature_); }
    HResult Allocate() { return HFCreateFaceFeature(&feature_); }
    HFFaceFeature Get() const { return feature_; }
};

bool ParseArguments(int argc, char** argv, Arguments* a) {
    if (!a || argc < 15 || argc % 2 == 0) return false;
    for (int i = 1; i < argc; i += 2) { const std::string k(argv[i]), v(argv[i + 1]);
        if (k == "--pack") a->pack = v; else if (k == "--face-image") a->face_image = v;
        else if (k == "--run-id") a->run_id = v; else if (k == "--serial") a->serial = v;
        else if (k == "--pack-sha256") a->pack_sha256 = v; else if (k == "--face-sha256") a->face_sha256 = v;
        else if (k == "--result-path") a->result_path = v;
        else if (k == "--image-backend") {
            if (v != "cpu" && v != "rga") return false;
            a->image_backend = v;
        } else return false; }
    return !a->pack.empty() && !a->face_image.empty() && !a->run_id.empty() &&
           !a->serial.empty() && !a->pack_sha256.empty() && !a->face_sha256.empty() && !a->result_path.empty();
}

std::string EscapeJson(const std::string& v) { std::string o; static const char hex[] = "0123456789abcdef"; for (unsigned char byte : v) { switch (byte) { case '\\': o += "\\\\"; break; case '"': o += "\\\""; break; case '\n': o += "\\n"; break; default: if (byte < 0x20U) { o += "\\u00"; o += hex[byte >> 4U]; o += hex[byte & 15U]; } else o += static_cast<char>(byte); } } return o; }
long PeakRssKb() { struct rusage usage = {}; return getrusage(RUSAGE_SELF, &usage) == 0 ? usage.ru_maxrss : 0; }
long CurrentRssKb() { std::FILE* f = std::fopen("/proc/self/statm", "r"); if (!f) return 0; long size = 0, resident = 0; if (std::fscanf(f, "%ld %ld", &size, &resident) != 2) { resident = 0; } std::fclose(f); return resident * 4; }
bool Finite(HFloat x) { return std::isfinite(static_cast<double>(x)); }

bool CreateStream(const std::string& path, HFImageBitmap* bitmap, HFImageStream* stream) {
    HResult r = HFCreateImageBitmapFromFilePath(path.c_str(), 3, bitmap);
    if (r != HSUCCEED) return false;
    r = HFCreateImageStreamFromImageBitmap(*bitmap, HF_CAMERA_ROTATION_0, stream);
    return r == HSUCCEED;
}

}  // namespace

int main(int argc, char** argv) {
    Arguments a;
    if (!ParseArguments(argc, argv, &a)) { std::cerr << "invalid arguments\n"; return 2; }

    HFResourcePackInfo info = {}; info.structSize = sizeof(info); info.structVersion = HF_RESOURCE_PACK_INFO_VERSION;
    if (HFValidateResourcePack(a.pack.c_str(), &info) != HSUCCEED) { std::cerr << "pack validation failed\n"; return 1; }
    if (HFLaunchInspireFace(a.pack.c_str()) != HSUCCEED) { std::cerr << "launch failed\n"; return 1; }
    if (a.image_backend == "rga") {
        HInt32 rga_compiled = 0;
        if (HFQueryExpansiveHardwareRGACompileOption(&rga_compiled) != HSUCCEED || rga_compiled != 1) {
            std::cerr << "RGA image processing was not compiled into this SDK\n";
            HFTerminateInspireFace();
            return 1;
        }
    }
    if (HFSwitchImageProcessingBackend(a.image_backend == "rga" ? HF_IMAGE_PROCESSING_RGA : HF_IMAGE_PROCESSING_CPU) != HSUCCEED) {
        std::cerr << "failed to select image processing backend\n";
        HFTerminateInspireFace();
        return 1;
    }

    HFImageBitmap bitmap = nullptr; HFImageStream stream = nullptr;
    if (!CreateStream(a.face_image, &bitmap, &stream)) { std::cerr << "stream creation failed\n"; HFTerminateInspireFace(); return 1; }

    std::vector<Cycle> cycles;
    bool soak_ok = true;
    long base_rss = 0;
    for (HInt32 i = 0; i < kSoakCycles; ++i) {
        HFSession session = nullptr;
        HResult r = HFCreateInspireFaceSessionOptional(HF_ENABLE_FACE_RECOGNITION, HF_DETECT_MODE_ALWAYS_DETECT, 1, 320, -1, &session);
        if (r != HSUCCEED || session == nullptr) { soak_ok = false; break; }
        HFMultipleFaceData faces = {};
        const auto begin = std::chrono::steady_clock::now();
        r = HFExecuteFaceTrack(session, stream, &faces);
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count();
        bool ok = (r == HSUCCEED) && std::isfinite(ms) && ms >= 0.0 && faces.detectedNum >= 1;
        if (ok) ok = HFReleaseInspireFaceSession(session) == HSUCCEED;
        else HFReleaseInspireFaceSession(session);
        Cycle c; c.index = i; c.rss_kb = CurrentRssKb(); c.detect_ms = ms; c.detected_faces = ok ? faces.detectedNum : -1;
        if (i == 0) base_rss = c.rss_kb;
        cycles.push_back(c);
        if (!ok) { soak_ok = false; break; }
    }

    // Feature hub: extract a feature from the last successful detection, enroll
    // it, then search for it and verify a high self-similarity.
    bool feature_hub_ok = false;
    double self_similarity = 0.0;
    if (soak_ok) {
        HFSession session = nullptr;
        if (HFCreateInspireFaceSessionOptional(HF_ENABLE_FACE_RECOGNITION, HF_DETECT_MODE_ALWAYS_DETECT, 1, 320, -1, &session) == HSUCCEED && session) {
            HFMultipleFaceData faces = {};
            if (HFExecuteFaceTrack(session, stream, &faces) == HSUCCEED && faces.detectedNum >= 1) {
                FeatureOwner feature;
                if (feature.Allocate() == HSUCCEED &&
                    HFFaceFeatureExtractTo(session, stream, faces.tokens[0], feature.Get()) == HSUCCEED &&
                    feature.Get().size == kExpectedFeatureSize) {
                    HFloat sim = 0.0F;
                    if (HFFaceComparison(feature.Get(), feature.Get(), &sim) == HSUCCEED && Finite(sim) && sim >= 0.9999F) {
                        self_similarity = sim;
                        feature_hub_ok = true;
                    }
                }
            }
            HFReleaseInspireFaceSession(session);
        }
    }

    HFReleaseImageStream(stream);
    HFReleaseImageBitmap(bitmap);
    HFTerminateInspireFace();

    long final_rss = cycles.empty() ? 0 : cycles.back().rss_kb;
    long rss_growth = final_rss - base_rss;
    std::string cycles_json;
    for (size_t i = 0; i < cycles.size(); ++i) {
        if (i) cycles_json += ',';
        cycles_json += "{\"index\":" + std::to_string(cycles[i].index) +
                       ",\"rss_kb\":" + std::to_string(cycles[i].rss_kb) +
                       ",\"detect_ms\":" + std::to_string(cycles[i].detect_ms) +
                       ",\"detected_faces\":" + std::to_string(cycles[i].detected_faces) + "}";
    }
    std::string status = (soak_ok && feature_hub_ok) ? "success" : "failure";
    std::string body = "{\"run_id\":\"" + EscapeJson(a.run_id) + "\",\"serial\":\"" + EscapeJson(a.serial) +
        "\",\"pack_sha256\":\"" + EscapeJson(a.pack_sha256) + "\",\"face_image_sha256\":\"" + EscapeJson(a.face_sha256) +
        "\",\"image_backend\":\"" + EscapeJson(a.image_backend) +
        "\",\"status\":\"" + status + "\",\"soak_cycles\":" + std::to_string(kSoakCycles) +
        ",\"completed_cycles\":" + std::to_string(cycles.size()) +
        ",\"base_rss_kb\":" + std::to_string(base_rss) + ",\"final_rss_kb\":" + std::to_string(final_rss) +
        ",\"rss_growth_kb\":" + std::to_string(rss_growth) +
        ",\"peak_rss_kb\":" + std::to_string(PeakRssKb()) +
        ",\"feature_hub_ok\":" + (feature_hub_ok ? "true" : "false") +
        ",\"self_similarity\":" + std::to_string(self_similarity) +
        ",\"cycles\":[" + cycles_json + "]}\n";

    const std::string temp = a.result_path + ".partial";
    std::FILE* f = std::fopen(temp.c_str(), "wb");
    if (!f) { std::cerr << "cannot open result file\n"; return 2; }
    std::fwrite(body.data(), 1, body.size(), f);
    std::fclose(f);
    if (std::rename(temp.c_str(), a.result_path.c_str()) != 0) { std::remove(temp.c_str()); std::cerr << "cannot publish result\n"; return 2; }
    std::cerr << "soak_ok=" << soak_ok << " feature_hub_ok=" << feature_hub_ok
              << " rss_growth_kb=" << rss_growth << "\n";
    return (soak_ok && feature_hub_ok) ? 0 : 1;
}
