#include "inspireface.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <sys/resource.h>
#include <vector>

namespace {

constexpr HFUInt32 kOutputCount = 1;
constexpr int kUpdateFrames = 30;

struct Arguments {
    std::string pack, face_image, no_face_image, run_id, serial, pack_sha256, face_sha256, no_face_sha256, result_path;
};

bool ParseArguments(int argc, char** argv, Arguments* a) {
    if (!a || argc != 19) return false;
    for (int i = 1; i < argc; i += 2) { const std::string k(argv[i]), v(argv[i + 1]);
        if (k == "--pack") a->pack = v; else if (k == "--face-image") a->face_image = v;
        else if (k == "--no-face-image") a->no_face_image = v; else if (k == "--run-id") a->run_id = v;
        else if (k == "--serial") a->serial = v; else if (k == "--pack-sha256") a->pack_sha256 = v;
        else if (k == "--face-sha256") a->face_sha256 = v; else if (k == "--no-face-sha256") a->no_face_sha256 = v;
        else if (k == "--result-path") a->result_path = v; else return false; }
    return !a->pack.empty() && !a->face_image.empty() && !a->no_face_image.empty() && !a->run_id.empty() &&
           !a->serial.empty() && !a->pack_sha256.empty() && !a->face_sha256.empty() && !a->no_face_sha256.empty() && !a->result_path.empty();
}

std::string EscapeJson(const std::string& v) { std::string o; static const char hex[] = "0123456789abcdef"; for (unsigned char byte : v) { switch (byte) { case '\\': o += "\\\\"; break; case '"': o += "\\\""; break; default: if (byte < 0x20U) { o += "\\u00"; o += hex[byte >> 4U]; o += hex[byte & 15U]; } else o += static_cast<char>(byte); } } return o; }

struct BitmapOwner { HFImageBitmap b = nullptr; ~BitmapOwner() { if (b) HFReleaseImageBitmap(b); } };
struct StreamOwner { HFImageStream s = nullptr; ~StreamOwner() { if (s) HFReleaseImageStream(s); } };

bool CreateStream(const std::string& path, BitmapOwner* bm, StreamOwner* st) {
    if (HFCreateImageBitmapFromFilePath(path.c_str(), 3, &bm->b) != HSUCCEED) return false;
    return HFCreateImageStreamFromImageBitmap(bm->b, HF_CAMERA_ROTATION_0, &st->s) == HSUCCEED;
}

HFFaceCaptureConfig RelaxedConfig() {
    HFFaceCaptureConfig config = {};
    HFGetDefaultFaceCaptureConfig(&config);
    config.filterMask = HF_CAPTURE_FILTER_FACE_COUNT | HF_CAPTURE_FILTER_FACE_SIZE |
                        HF_CAPTURE_FILTER_FACE_POSITION | HF_CAPTURE_FILTER_FACE_BOUNDARY;
    config.outputCount = kOutputCount;
    config.stableDurationMs = 0;
    config.collectDurationMs = 0;
    config.maxCollectDurationMs = 100000000;
    config.minCandidateIntervalMs = 0;
    config.minFaceWidthRatio = 0.0f;
    config.maxFaceWidthRatio = 1.0f;
    config.maxCenterOffsetX = 1.0f;
    config.maxCenterOffsetY = 1.0f;
    config.boundaryMarginRatio = 0.0f;
    return config;
}

}  // namespace

int main(int argc, char** argv) {
    Arguments a;
    if (!ParseArguments(argc, argv, &a)) { std::cerr << "invalid arguments\n"; return 2; }

    HFResourcePackInfo info = {}; info.structSize = sizeof(info); info.structVersion = HF_RESOURCE_PACK_INFO_VERSION;
    if (HFValidateResourcePack(a.pack.c_str(), &info) != HSUCCEED) { std::cerr << "pack validation failed\n"; return 1; }
    if (HFLaunchInspireFace(a.pack.c_str()) != HSUCCEED) { std::cerr << "launch failed\n"; return 1; }

    bool face_ok = false, no_face_ok = false;
    int face_final_state = -1, no_face_final_state = -1;
    int face_results_collected = 0;
    double face_update_ms = 0.0;

    // Face capture with a face image: state should progress beyond IDLE.
    {
        HFSession session = nullptr;
        BitmapOwner bm; StreamOwner st;
        if (HFCreateInspireFaceSessionOptional(HF_ENABLE_NONE, HF_DETECT_MODE_ALWAYS_DETECT, 16, -1, -1, &session) == HSUCCEED && session && CreateStream(a.face_image, &bm, &st)) {
            HFFaceCaptureConfig config = RelaxedConfig();
            HFFaceCaptureSession capture = nullptr;
            if (HFCreateFaceCaptureSession(session, &config, &capture) == HSUCCEED && capture) {
                for (int i = 0; i < kUpdateFrames; ++i) {
                    HFFaceCaptureProgress progress = {};
                    const auto begin = std::chrono::steady_clock::now();
                    HResult r = HFUpdateFaceCaptureSession(capture, st.s, 1000 + i * 33, 1000 + i * 33, &progress);
                    face_update_ms += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count();
                    if (r != HSUCCEED) break;
                    face_final_state = progress.state;
                    if (progress.state == HF_CAPTURE_STATE_READY || progress.state == HF_CAPTURE_STATE_FINISHED) {
                        HFFaceCaptureResult results[1] = {};
                        HFUInt32 count = 0;
                        HFFaceCaptureProgress finish_progress = {};
                        if (HFFinishFaceCaptureSession(capture, &finish_progress) == HSUCCEED &&
                            HFGetFaceCaptureResults(capture, results, 1, &count) == HSUCCEED) {
                            face_results_collected = static_cast<int>(count);
                        }
                        break;
                    }
                }
                HFReleaseFaceCaptureSession(capture);
            }
            HFReleaseInspireFaceSession(session);
        }
        face_ok = (face_final_state >= HF_CAPTURE_STATE_STABILIZING);
    }

    // No-face image: state should stay IDLE or go to TRACK_LOST.
    {
        HFSession session = nullptr;
        BitmapOwner bm; StreamOwner st;
        if (HFCreateInspireFaceSessionOptional(HF_ENABLE_NONE, HF_DETECT_MODE_ALWAYS_DETECT, 16, -1, -1, &session) == HSUCCEED && session && CreateStream(a.no_face_image, &bm, &st)) {
            HFFaceCaptureConfig config = RelaxedConfig();
            HFFaceCaptureSession capture = nullptr;
            if (HFCreateFaceCaptureSession(session, &config, &capture) == HSUCCEED && capture) {
                for (int i = 0; i < kUpdateFrames; ++i) {
                    HFFaceCaptureProgress progress = {};
                    if (HFUpdateFaceCaptureSession(capture, st.s, 1000 + i * 33, 1000 + i * 33, &progress) != HSUCCEED) break;
                    no_face_final_state = progress.state;
                }
                HFReleaseFaceCaptureSession(capture);
            }
            HFReleaseInspireFaceSession(session);
        }
        no_face_ok = (no_face_final_state == HF_CAPTURE_STATE_IDLE || no_face_final_state == HF_CAPTURE_STATE_TRACK_LOST);
    }

    HFTerminateInspireFace();

    bool all_ok = face_ok && no_face_ok;
    std::string status = all_ok ? "success" : "failure";
    std::string body = "{\"run_id\":\"" + EscapeJson(a.run_id) + "\",\"serial\":\"" + EscapeJson(a.serial) +
        "\",\"pack_sha256\":\"" + EscapeJson(a.pack_sha256) + "\",\"face_image_sha256\":\"" + EscapeJson(a.face_sha256) +
        "\",\"no_face_image_sha256\":\"" + EscapeJson(a.no_face_sha256) +
        "\",\"status\":\"" + status +
        "\",\"face_ok\":" + (face_ok ? "true" : "false") +
        ",\"face_final_state\":" + std::to_string(face_final_state) +
        ",\"face_results_collected\":" + std::to_string(face_results_collected) +
        ",\"face_avg_update_ms\":" + std::to_string(face_update_ms / kUpdateFrames) +
        ",\"no_face_ok\":" + (no_face_ok ? "true" : "false") +
        ",\"no_face_final_state\":" + std::to_string(no_face_final_state) + "}\n";
    const std::string temp = a.result_path + ".partial";
    std::FILE* f = std::fopen(temp.c_str(), "wb");
    if (!f) { std::cerr << "cannot open result file\n"; return 2; }
    std::fwrite(body.data(), 1, body.size(), f);
    std::fclose(f);
    if (std::rename(temp.c_str(), a.result_path.c_str()) != 0) { std::remove(temp.c_str()); return 2; }
    std::cerr << "face_ok=" << face_ok << " state=" << face_final_state << " no_face_ok=" << no_face_ok << " state=" << no_face_final_state << "\n";
    return all_ok ? 0 : 1;
}
