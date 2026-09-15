#include "inspireface.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <sys/resource.h>
#include <thread>
#include <vector>

namespace {

constexpr int kThreadCount = 4;
constexpr int kFramesPerThread = 20;

struct Arguments {
    std::string pack, face_image, run_id, serial, pack_sha256, face_sha256, result_path;
    std::string image_backend = "cpu";
};

struct ThreadResult {
    int index = 0;
    bool ok = false;
    int completed_frames = 0;
    double mean_ms = 0.0;
    double min_ms = 0.0;
    double max_ms = 0.0;
    HInt32 detected_faces = -1;
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

std::string EscapeJson(const std::string& v) { std::string o; static const char hex[] = "0123456789abcdef"; for (unsigned char byte : v) { switch (byte) { case '\\': o += "\\\\"; break; case '"': o += "\\\""; break; default: if (byte < 0x20U) { o += "\\u00"; o += hex[byte >> 4U]; o += hex[byte & 15U]; } else o += static_cast<char>(byte); } } return o; }
bool Finite(HFloat x) { return std::isfinite(static_cast<double>(x)); }

void ThreadWorker(int index, const std::string& face_image, std::atomic<int>* /*failures*/, ThreadResult* out) {
    out->index = index;
    HFImageBitmap bitmap = nullptr; HFImageStream stream = nullptr;
    HResult r = HFCreateImageBitmapFromFilePath(face_image.c_str(), 3, &bitmap);
    if (r != HSUCCEED) return;
    r = HFCreateImageStreamFromImageBitmap(bitmap, HF_CAMERA_ROTATION_0, &stream);
    if (r != HSUCCEED) { HFReleaseImageBitmap(bitmap); return; }
    HFSession session = nullptr;
    r = HFCreateInspireFaceSessionOptional(HF_ENABLE_NONE, HF_DETECT_MODE_ALWAYS_DETECT, 1, 320, -1, &session);
    if (r != HSUCCEED || session == nullptr) { HFReleaseImageStream(stream); HFReleaseImageBitmap(bitmap); return; }
    std::vector<double> samples;
    for (int i = 0; i < kFramesPerThread; ++i) {
        HFMultipleFaceData faces = {};
        const auto begin = std::chrono::steady_clock::now();
        r = HFExecuteFaceTrack(session, stream, &faces);
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count();
        if (r != HSUCCEED || !std::isfinite(ms) || ms < 0.0 || faces.detectedNum < 1 ||
            !Finite(faces.detConfidence[0])) { HFReleaseInspireFaceSession(session); HFReleaseImageStream(stream); HFReleaseImageBitmap(bitmap); return; }
        samples.push_back(ms);
        out->detected_faces = faces.detectedNum;
    }
    HFReleaseInspireFaceSession(session);
    HFReleaseImageStream(stream);
    HFReleaseImageBitmap(bitmap);
    if (samples.size() != static_cast<size_t>(kFramesPerThread)) return;
    double sum = 0.0; out->min_ms = samples[0]; out->max_ms = samples[0];
    for (double s : samples) { sum += s; if (s < out->min_ms) out->min_ms = s; if (s > out->max_ms) out->max_ms = s; }
    out->mean_ms = sum / samples.size();
    out->completed_frames = static_cast<int>(samples.size());
    out->ok = true;
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

    std::vector<ThreadResult> results(kThreadCount);
    std::atomic<int> failures(0);
    std::vector<std::thread> threads;
    const auto begin = std::chrono::steady_clock::now();
    for (int i = 0; i < kThreadCount; ++i) threads.emplace_back(ThreadWorker, i, std::cref(a.face_image), &failures, &results[i]);
    for (auto& t : threads) t.join();
    const double wall_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count();
    HFTerminateInspireFace();

    bool all_ok = true;
    std::string rows_json;
    for (int i = 0; i < kThreadCount; ++i) {
        if (!results[i].ok) all_ok = false;
        if (i) rows_json += ',';
        rows_json += "{\"index\":" + std::to_string(i) +
            ",\"ok\":" + (results[i].ok ? "true" : "false") +
            ",\"completed_frames\":" + std::to_string(results[i].completed_frames) +
            ",\"mean_ms\":" + std::to_string(results[i].mean_ms) +
            ",\"min_ms\":" + std::to_string(results[i].min_ms) +
            ",\"max_ms\":" + std::to_string(results[i].max_ms) +
            ",\"detected_faces\":" + std::to_string(results[i].detected_faces) + "}";
    }
    std::string status = all_ok ? "success" : "failure";
    std::string body = "{\"run_id\":\"" + EscapeJson(a.run_id) + "\",\"serial\":\"" + EscapeJson(a.serial) +
        "\",\"pack_sha256\":\"" + EscapeJson(a.pack_sha256) + "\",\"face_image_sha256\":\"" + EscapeJson(a.face_sha256) +
        "\",\"image_backend\":\"" + EscapeJson(a.image_backend) +
        "\",\"status\":\"" + status + "\",\"thread_count\":" + std::to_string(kThreadCount) +
        ",\"frames_per_thread\":" + std::to_string(kFramesPerThread) +
        ",\"wall_ms\":" + std::to_string(wall_ms) +
        ",\"peak_rss_kb\":" + std::to_string([]{ struct rusage u = {}; return getrusage(RUSAGE_SELF, &u) == 0 ? u.ru_maxrss : 0; }()) +
        ",\"threads\":[" + rows_json + "]}\n";
    const std::string temp = a.result_path + ".partial";
    std::FILE* f = std::fopen(temp.c_str(), "wb");
    if (!f) { std::cerr << "cannot open result file\n"; return 2; }
    std::fwrite(body.data(), 1, body.size(), f);
    std::fclose(f);
    if (std::rename(temp.c_str(), a.result_path.c_str()) != 0) { std::remove(temp.c_str()); std::cerr << "cannot publish result\n"; return 2; }
    std::cerr << "all_ok=" << all_ok << " wall_ms=" << wall_ms << "\n";
    return all_ok ? 0 : 1;
}
