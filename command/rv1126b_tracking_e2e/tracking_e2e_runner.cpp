#include "inspireface.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <set>
#include <string>
#include <sys/resource.h>
#include <vector>

namespace {

constexpr HInt32 kFramesTrackSingle = 30;
constexpr HInt32 kFramesTrackMulti = 10;
constexpr HInt32 kFramesLandmark = 5;
constexpr HInt32 kMinMultiFaces = 2;
constexpr HInt32 kMaxDetectFaces = 30;
constexpr HInt32 kTrackDetectInterval = 5;
constexpr HInt32 kExpectedFeaturelessDenseCount = 106;

struct Arguments {
    std::string pack, single_face_image, multi_face_image, no_face_image;
    std::string run_id, serial, pack_sha256, single_sha256, multi_sha256, no_face_sha256, result_path;
};

struct Scenario {
    std::string name, status = "failure", failure_stage = "setup";
    HResult hresult = HERR_INVALID_PARAM;
    bool all_finite = false;
    long peak_rss_kb = 0;
    std::vector<double> latency_ms;
    HInt32 detected_faces = -1, track_id = -1, track_count = 0, frames = 0;
    HInt32 unique_ids = 0, dense_count = 0, five_point_count = 0;
};

class LaunchOwner { public: ~LaunchOwner() { if (launched_) HFTerminateInspireFace(); } void MarkLaunched() { launched_ = true; } private: bool launched_ = false; };
class SessionOwner { public: ~SessionOwner() { if (h_) HFReleaseInspireFaceSession(h_); } PHFSession Out() { return &h_; } HFSession Get() const { return h_; } private: HFSession h_ = nullptr; };
class BitmapOwner { public: ~BitmapOwner() { if (bitmap_ != nullptr) HFReleaseImageBitmap(bitmap_); } PHFImageBitmap Out() { return &bitmap_; } HFImageBitmap Get() const { return bitmap_; } private: HFImageBitmap bitmap_ = nullptr; };
class StreamOwner { public: ~StreamOwner() { if (h_) HFReleaseImageStream(h_); } PHFImageStream Out() { return &h_; } HFImageStream Get() const { return h_; } private: HFImageStream h_ = nullptr; };

bool ParseArguments(int argc, char** argv, Arguments* a) {
    if (!a || argc != 23) return false;
    for (int i = 1; i < argc; i += 2) { const std::string k(argv[i]), v(argv[i + 1]);
        if (k == "--pack") a->pack = v; else if (k == "--single-face-image") a->single_face_image = v;
        else if (k == "--multi-face-image") a->multi_face_image = v; else if (k == "--no-face-image") a->no_face_image = v;
        else if (k == "--run-id") a->run_id = v; else if (k == "--serial") a->serial = v;
        else if (k == "--pack-sha256") a->pack_sha256 = v; else if (k == "--single-sha256") a->single_sha256 = v;
        else if (k == "--multi-sha256") a->multi_sha256 = v; else if (k == "--no-face-sha256") a->no_face_sha256 = v;
        else if (k == "--result-path") a->result_path = v; else return false; }
    return !a->pack.empty() && !a->single_face_image.empty() && !a->multi_face_image.empty() &&
           !a->no_face_image.empty() && !a->run_id.empty() && !a->serial.empty() && !a->pack_sha256.empty() &&
           !a->single_sha256.empty() && !a->multi_sha256.empty() && !a->no_face_sha256.empty() && !a->result_path.empty();
}

std::string EscapeJson(const std::string& v) { std::string o; static const char hex[] = "0123456789abcdef"; for (unsigned char byte : v) { switch (byte) { case '\\': o += "\\\\"; break; case '"': o += "\\\""; break; case '\n': o += "\\n"; break; case '\r': o += "\\r"; break; case '\t': o += "\\t"; break; default: if (byte < 0x20U) { o += "\\u00"; o += hex[byte >> 4U]; o += hex[byte & 15U]; } else o += static_cast<char>(byte); } } return o; }
long PeakRssKb() { struct rusage usage = {}; return getrusage(RUSAGE_SELF, &usage) == 0 ? usage.ru_maxrss : 0; }
bool Finite(HFloat x) { return std::isfinite(static_cast<double>(x)); }
void Fail(Scenario* s, const char* stage, HResult r) { s->status = "failure"; s->failure_stage = stage; s->hresult = r; s->all_finite = false; s->peak_rss_kb = PeakRssKb(); }

bool CreateStream(const std::string& path, BitmapOwner* b, StreamOwner* st, Scenario* s) {
    HResult r = HFCreateImageBitmapFromFilePath(path.c_str(), 3, b->Out());
    if (r != HSUCCEED) { Fail(s, "create_bitmap", r); return false; }
    r = HFCreateImageStreamFromImageBitmap(b->Get(), HF_CAMERA_ROTATION_0, st->Out());
    if (r != HSUCCEED) { Fail(s, "create_stream", r); return false; }
    return true;
}

bool CreateSession(HFDetectMode mode, HInt32 level, HInt32 maxFaces, HInt32 interval, SessionOwner* session, Scenario* s) {
    HFSessionCustomParameter parameter = {};
    parameter.enable_detect_mode_landmark = 1;
    HResult r = HFCreateInspireFaceSession(parameter, mode, maxFaces, level, 30, session->Out());
    if (r != HSUCCEED) { Fail(s, "create_session", r); return false; }
    if (mode == HF_DETECT_MODE_LIGHT_TRACK || mode == HF_DETECT_MODE_TRACK_BY_DETECTION) {
        r = HFSessionSetTrackPreviewSize(session->Get(), level);
        if (r != HSUCCEED) { Fail(s, "set_preview_size", r); return false; }
        r = HFSessionSetFilterMinimumFacePixelSize(session->Get(), 0);
        if (r != HSUCCEED) { Fail(s, "set_filter_min_face", r); return false; }
        r = HFSessionSetTrackModeDetectInterval(session->Get(), interval);
        if (r != HSUCCEED) { Fail(s, "set_detect_interval", r); return false; }
    }
    return true;
}

bool ValidateFrame(const HFMultipleFaceData& faces, std::set<HInt32>* unique_ids) {
    if (faces.detectedNum < 0 || faces.detectedNum > kMaxDetectFaces) return false;
    if (faces.detectedNum > 0 && (faces.rects == nullptr || faces.trackIds == nullptr || faces.trackCounts == nullptr ||
        faces.detConfidence == nullptr || faces.angles.roll == nullptr || faces.angles.yaw == nullptr ||
        faces.angles.pitch == nullptr || faces.tokens == nullptr)) return false;
    if (unique_ids) unique_ids->clear();
    for (HInt32 i = 0; i < faces.detectedNum; ++i) {
        if (faces.rects[i].width <= 0 || faces.rects[i].height <= 0 || faces.trackCounts[i] < 0 ||
            !Finite(faces.detConfidence[i]) || !Finite(faces.angles.roll[i]) ||
            !Finite(faces.angles.yaw[i]) || !Finite(faces.angles.pitch[i])) return false;
        if (faces.tokens[i].size <= 0 || faces.tokens[i].data == nullptr) return false;
        if (unique_ids && !unique_ids->insert(faces.trackIds[i]).second) return false;
    }
    return true;
}

// Feed the same static stream for N frames. Asserts that the leading face keeps
// the same track id and that its track count never decreases across frames.
Scenario RunTrackContinuity(const char* name, HFDetectMode mode, HInt32 level, HInt32 interval, HInt32 frames,
                            const std::string& image, HInt32 minFaces, HInt32 maxFaces) {
    Scenario s; s.name = name; s.frames = frames;
    SessionOwner session; BitmapOwner b; StreamOwner st;
    if (!CreateSession(mode, level, kMaxDetectFaces, interval, &session, &s)) return s;
    if (!CreateStream(image, &b, &st, &s)) return s;
    HInt32 first_track_id = -1, prev_track_count = 0;
    for (HInt32 i = 0; i < frames; ++i) {
        HFMultipleFaceData faces = {};
        const auto begin = std::chrono::steady_clock::now();
        HResult r = HFExecuteFaceTrack(session.Get(), st.Get(), &faces);
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count();
        if (r != HSUCCEED) { Fail(&s, "track", r); return s; }
        if (!std::isfinite(ms) || ms < 0.0) { Fail(&s, "latency", HERR_SESS_PIPELINE_FAILURE); return s; }
        s.latency_ms.push_back(ms);
        std::set<HInt32> ids;
        bool check_unique = (mode != HF_DETECT_MODE_ALWAYS_DETECT);
        if (!ValidateFrame(faces, check_unique ? &ids : nullptr)) {
            std::cerr << "DEBUG " << s.name << " validate detected=" << faces.detectedNum
                      << " rects=" << (faces.rects != nullptr) << " ids=" << (faces.trackIds != nullptr)
                      << " counts=" << (faces.trackCounts != nullptr) << " conf=" << (faces.detConfidence != nullptr)
                      << " roll=" << (faces.angles.roll != nullptr) << "\n";
            Fail(&s, "validate", HERR_SESS_PIPELINE_FAILURE); return s;
        }
        if (faces.detectedNum < minFaces || faces.detectedNum > maxFaces) {
            std::cerr << "DEBUG " << s.name << " frame=" << i << " detected=" << faces.detectedNum
                      << " min=" << minFaces << " max=" << maxFaces << "\n";
            Fail(&s, "face_count", HERR_SESS_PIPELINE_FAILURE); return s;
        }
        s.detected_faces = faces.detectedNum;
        s.unique_ids = check_unique ? static_cast<HInt32>(ids.size()) : 0;
        if (i == 0) { first_track_id = faces.trackIds[0]; s.track_id = first_track_id; }
        else {
            bool is_single_face = (maxFaces <= 1);
            if (is_single_face && mode != HF_DETECT_MODE_ALWAYS_DETECT && faces.trackIds[0] != first_track_id) { Fail(&s, "track_id_changed", HERR_SESS_PIPELINE_FAILURE); return s; }
            if (is_single_face && mode != HF_DETECT_MODE_ALWAYS_DETECT && faces.trackCounts[0] < prev_track_count) { Fail(&s, "track_count_decreased", HERR_SESS_PIPELINE_FAILURE); return s; }
        }
        prev_track_count = faces.trackCounts[0];
    }
    s.track_count = prev_track_count;
    s.status = "success"; s.failure_stage.clear(); s.hresult = HSUCCEED; s.all_finite = true;
    s.peak_rss_kb = PeakRssKb();
    return s;
}

Scenario RunNoFace(const std::string& image) {
    Scenario s; s.name = "no_face"; s.frames = 1;
    SessionOwner session; BitmapOwner b; StreamOwner st;
    if (!CreateSession(HF_DETECT_MODE_ALWAYS_DETECT, 320, 1, 1, &session, &s)) return s;
    if (!CreateStream(image, &b, &st, &s)) return s;
    HFMultipleFaceData faces = {};
    const auto begin = std::chrono::steady_clock::now();
    HResult r = HFExecuteFaceTrack(session.Get(), st.Get(), &faces);
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count();
    if (r != HSUCCEED) { Fail(&s, "track", r); return s; }
    if (!std::isfinite(ms) || ms < 0.0) { Fail(&s, "latency", HERR_SESS_PIPELINE_FAILURE); return s; }
    s.latency_ms.push_back(ms);
    if (faces.detectedNum != 0) { Fail(&s, "face_count", HERR_SESS_PIPELINE_FAILURE); return s; }
    s.detected_faces = 0;
    s.status = "success"; s.failure_stage.clear(); s.hresult = HSUCCEED; s.all_finite = true;
    s.peak_rss_kb = PeakRssKb();
    return s;
}

Scenario RunTrackLandmark(const std::string& image) {
    Scenario s; s.name = "light_track_landmark"; s.frames = kFramesLandmark;
    SessionOwner session; BitmapOwner b; StreamOwner st;
    if (!CreateSession(HF_DETECT_MODE_LIGHT_TRACK, 320, 1, kTrackDetectInterval, &session, &s)) return s;
    if (!CreateStream(image, &b, &st, &s)) return s;
    for (HInt32 i = 0; i < kFramesLandmark; ++i) {
        HFMultipleFaceData faces = {};
        const auto begin = std::chrono::steady_clock::now();
        HResult r = HFExecuteFaceTrack(session.Get(), st.Get(), &faces);
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count();
        if (r != HSUCCEED) { Fail(&s, "track", r); return s; }
        if (!std::isfinite(ms) || ms < 0.0) { Fail(&s, "latency", HERR_SESS_PIPELINE_FAILURE); return s; }
        s.latency_ms.push_back(ms);
        if (!ValidateFrame(faces, nullptr) || faces.detectedNum < 1) { Fail(&s, "validate", HERR_SESS_PIPELINE_FAILURE); return s; }
        s.detected_faces = faces.detectedNum;
    }
    HFMultipleFaceData faces = {};
    HResult r = HFExecuteFaceTrack(session.Get(), st.Get(), &faces);
    if (r != HSUCCEED || !ValidateFrame(faces, nullptr) || faces.detectedNum < 1) { Fail(&s, "track_final", HERR_SESS_PIPELINE_FAILURE); return s; }
    HInt32 n = 0;
    r = HFGetNumOfFaceDenseLandmark(&n);
    if (r != HSUCCEED || n != kExpectedFeaturelessDenseCount) { Fail(&s, "dense_count", r); return s; }
    std::vector<HPoint2f> dense(static_cast<size_t>(n));
    HPoint2f five[5] = {};
    r = HFGetFaceDenseLandmarkFromFaceToken(faces.tokens[0], dense.data(), n);
    if (r == HSUCCEED) r = HFGetFaceFiveKeyPointsFromFaceToken(faces.tokens[0], five, 5);
    if (r != HSUCCEED) { Fail(&s, "landmark", r); return s; }
    for (const auto& p : dense) if (!Finite(p.x) || !Finite(p.y)) { Fail(&s, "dense_finite", HERR_SESS_PIPELINE_FAILURE); return s; }
    for (const auto& p : five) if (!Finite(p.x) || !Finite(p.y)) { Fail(&s, "five_finite", HERR_SESS_PIPELINE_FAILURE); return s; }
    s.dense_count = n; s.five_point_count = 5;
    s.status = "success"; s.failure_stage.clear(); s.hresult = HSUCCEED; s.all_finite = true;
    s.peak_rss_kb = PeakRssKb();
    return s;
}

std::vector<Scenario> RunAll(const Arguments& a) {
    const char* names[] = {"light_track_single", "track_by_detect_single", "light_track_multi",
                           "always_detect_multi", "no_face", "light_track_landmark"};
    std::vector<Scenario> rows;
    HFResourcePackInfo info = {}; info.structSize = sizeof(info); info.structVersion = HF_RESOURCE_PACK_INFO_VERSION;
    HResult r = HFValidateResourcePack(a.pack.c_str(), &info);
    if (r != HSUCCEED) { for (const char* n : names) { Scenario s; s.name = n; Fail(&s, "validate_pack", r); rows.push_back(s); } return rows; }
    LaunchOwner launch;
    r = HFLaunchInspireFace(a.pack.c_str());
    if (r != HSUCCEED) { for (const char* n : names) { Scenario s; s.name = n; Fail(&s, "launch", r); rows.push_back(s); } return rows; }
    launch.MarkLaunched();
    rows.push_back(RunTrackContinuity("light_track_single", HF_DETECT_MODE_LIGHT_TRACK, 320, kTrackDetectInterval, kFramesTrackSingle, a.single_face_image, 1, 1));
    rows.push_back(RunTrackContinuity("track_by_detect_single", HF_DETECT_MODE_TRACK_BY_DETECTION, 320, 1, kFramesTrackSingle, a.single_face_image, 1, 1));
    rows.push_back(RunTrackContinuity("light_track_multi", HF_DETECT_MODE_LIGHT_TRACK, 640, 1, kFramesTrackMulti, a.multi_face_image, kMinMultiFaces, kMaxDetectFaces));
    rows.push_back(RunTrackContinuity("always_detect_multi", HF_DETECT_MODE_ALWAYS_DETECT, 640, 1, kFramesTrackMulti, a.multi_face_image, kMinMultiFaces, kMaxDetectFaces));
    rows.push_back(RunNoFace(a.no_face_image));
    rows.push_back(RunTrackLandmark(a.single_face_image));
    return rows;
}

std::string ScenarioJson(const Scenario& s) {
    std::string lat;
    for (size_t i = 0; i < s.latency_ms.size(); ++i) { if (i) lat += ','; lat += std::to_string(s.latency_ms[i]); }
    return "{\"name\":\"" + EscapeJson(s.name) + "\",\"status\":\"" + EscapeJson(s.status) +
           "\",\"failure_stage\":\"" + EscapeJson(s.failure_stage) + "\",\"hresult\":" + std::to_string(static_cast<int>(s.hresult)) +
           ",\"all_finite\":" + (s.all_finite ? "true" : "false") + ",\"peak_rss_kb\":" + std::to_string(s.peak_rss_kb) +
           ",\"latency_ms\":[" + lat + "],\"detected_faces\":" + std::to_string(s.detected_faces) +
           ",\"track_id\":" + std::to_string(s.track_id) + ",\"track_count\":" + std::to_string(s.track_count) +
           ",\"frames\":" + std::to_string(s.frames) + ",\"unique_ids\":" + std::to_string(s.unique_ids) +
           ",\"dense_count\":" + std::to_string(s.dense_count) + ",\"five_point_count\":" + std::to_string(s.five_point_count) + "}";
}

std::string BuildReport(const Arguments& a, const std::vector<Scenario>& rows) {
    std::string body;
    for (size_t i = 0; i < rows.size(); ++i) { if (i) body += ','; body += ScenarioJson(rows[i]); }
    return "{\"run_id\":\"" + EscapeJson(a.run_id) + "\",\"serial\":\"" + EscapeJson(a.serial) +
           "\",\"pack_sha256\":\"" + EscapeJson(a.pack_sha256) + "\",\"single_face_image_sha256\":\"" + EscapeJson(a.single_sha256) +
           "\",\"multi_face_image_sha256\":\"" + EscapeJson(a.multi_sha256) + "\",\"no_face_image_sha256\":\"" + EscapeJson(a.no_face_sha256) +
           "\",\"scenarios\":[" + body + "]}\n";
}

bool WriteReport(const std::string& path, const std::string& text) {
    const std::string temp = path + ".partial";
    std::FILE* f = std::fopen(temp.c_str(), "wb");
    if (!f) return false;
    const bool wrote = std::fwrite(text.data(), 1, text.size(), f) == text.size();
    const bool closed = std::fclose(f) == 0;
    const bool ok = wrote && closed && std::rename(temp.c_str(), path.c_str()) == 0;
    if (!ok) std::remove(temp.c_str());
    return ok;
}

}  // namespace

int main(int argc, char** argv) {
    Arguments a;
    if (!ParseArguments(argc, argv, &a)) { std::cerr << "invalid arguments\n"; return 2; }
    const auto rows = RunAll(a);
    if (!WriteReport(a.result_path, BuildReport(a, rows))) { std::cerr << "cannot atomically publish result file\n"; return 2; }
    for (const auto& s : rows) if (s.status != "success") return 1;
    return 0;
}
