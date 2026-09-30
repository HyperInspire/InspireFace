#include "inspireface.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <sys/resource.h>
#include <vector>

namespace {
constexpr int kWarmupIterations = 2;
constexpr int kMeasuredIterations = 10;

struct Arguments {
    std::string pack, face_image, run_id, serial, pack_sha256, face_sha256, result_path;
};

struct PipelineCase {
    std::string name;
    HOption option_flag;
    std::string status = "failure";
    std::string failure_stage = "setup";
    HResult hresult = HERR_INVALID_PARAM;
    bool all_finite = false;
    long peak_rss_kb = 0;
    std::vector<double> latency_ms;
    HInt32 detected_faces = -1;
    double value_0 = 0.0;
    int int_value_0 = -1;
    std::string note;
};

class LaunchOwner {
public:
    ~LaunchOwner() { if (launched_) HFTerminateInspireFace(); }
    void MarkLaunched() { launched_ = true; }
private:
    bool launched_ = false;
};

class SessionOwner {
public:
    ~SessionOwner() { if (h_) HFReleaseInspireFaceSession(h_); }
    PHFSession Out() { return &h_; }
    HFSession Get() const { return h_; }
private:
    HFSession h_ = nullptr;
};

class BitmapOwner {
public:
    ~BitmapOwner() { if (bitmap_ != nullptr) HFReleaseImageBitmap(bitmap_); }
    PHFImageBitmap Out() { return &bitmap_; }
    HFImageBitmap Get() const { return bitmap_; }
private:
    HFImageBitmap bitmap_ = nullptr;
};

class StreamOwner {
public:
    ~StreamOwner() { if (h_) HFReleaseImageStream(h_); }
    PHFImageStream Out() { return &h_; }
    HFImageStream Get() const { return h_; }
private:
    HFImageStream h_ = nullptr;
};

bool ParseArguments(int argc, char** argv, Arguments* a) {
    if (!a || argc != 15) return false;
    for (int i = 1; i < argc; i += 2) {
        const std::string k(argv[i]), v(argv[i + 1]);
        if (k == "--pack") a->pack = v;
        else if (k == "--face-image") a->face_image = v;
        else if (k == "--run-id") a->run_id = v;
        else if (k == "--serial") a->serial = v;
        else if (k == "--pack-sha256") a->pack_sha256 = v;
        else if (k == "--face-sha256") a->face_sha256 = v;
        else if (k == "--result-path") a->result_path = v;
        else return false;
    }
    return !a->pack.empty() && !a->face_image.empty() && !a->run_id.empty()
        && !a->serial.empty() && !a->pack_sha256.empty() && !a->face_sha256.empty()
        && !a->result_path.empty();
}

std::string EscapeJson(const std::string& v) {
    std::string o;
    static const char hex[] = "0123456789abcdef";
    for (unsigned char byte : v) {
        switch (byte) {
            case '\\': o += "\\\\"; break;
            case '"': o += "\\\""; break;
            case '\n': o += "\\n"; break;
            case '\r': o += "\\r"; break;
            case '\t': o += "\\t"; break;
            default:
                if (byte < 0x20U) {
                    o += "\\u00";
                    o += hex[byte >> 4U];
                    o += hex[byte & 15U];
                } else {
                    o += static_cast<char>(byte);
                }
        }
    }
    return o;
}

long PeakRssKb() {
    struct rusage usage = {};
    return getrusage(RUSAGE_SELF, &usage) == 0 ? usage.ru_maxrss : 0;
}

bool Finite(HFloat x) { return std::isfinite(static_cast<double>(x)); }

void Fail(PipelineCase* s, const char* stage, HResult r) {
    s->status = "failure";
    s->failure_stage = stage;
    s->hresult = r;
    s->all_finite = false;
    s->peak_rss_kb = PeakRssKb();
}

bool CreateStream(const std::string& path, BitmapOwner* b, StreamOwner* st, PipelineCase* s) {
    HResult r = HFCreateImageBitmapFromFilePath(path.c_str(), 3, b->Out());
    if (r != HSUCCEED) { Fail(s, "create_bitmap", r); return false; }
    r = HFCreateImageStreamFromImageBitmap(b->Get(), HF_CAMERA_ROTATION_0, st->Out());
    if (r != HSUCCEED) { Fail(s, "create_stream", r); return false; }
    return true;
}

bool CreateSession(HFSessionCustomParameter param, HInt32 level, SessionOwner* session, PipelineCase* s) {
    HResult r = HFCreateInspireFaceSession(param, HF_DETECT_MODE_ALWAYS_DETECT, 1, level, -1, session->Out());
    if (r != HSUCCEED) { Fail(s, "create_session", r); return false; }
    return true;
}

template <typename Operation>
void Measure(PipelineCase* s, Operation op) {
    for (int i = 0; i < kWarmupIterations; ++i) {
        HResult r = op();
        if (r != HSUCCEED) { Fail(s, "warmup", r); return; }
    }
    for (int i = 0; i < kMeasuredIterations; ++i) {
        const auto begin = std::chrono::steady_clock::now();
        HResult r = op();
        const double ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - begin).count();
        if (r != HSUCCEED) { Fail(s, "measure", r); return; }
        if (!std::isfinite(ms) || ms < 0.0) { Fail(s, "latency", HERR_SESS_PIPELINE_FAILURE); return; }
        s->latency_ms.push_back(ms);
    }
    s->status = "success";
    s->failure_stage.clear();
    s->hresult = HSUCCEED;
    s->all_finite = true;
    s->peak_rss_kb = PeakRssKb();
}

// --- Liveness ---
PipelineCase RunLiveness(const std::string& image) {
    PipelineCase s;
    s.name = "liveness";
    s.option_flag = HF_ENABLE_LIVENESS;
    HFSessionCustomParameter param = {};
    param.enable_liveness = 1;
    SessionOwner session;
    BitmapOwner b;
    StreamOwner st;
    if (!CreateSession(param, 320, &session, &s) ||
        !CreateStream(image, &b, &st, &s)) return s;
    Measure(&s, [&]() -> HResult {
        HFMultipleFaceData faces = {};
        HResult r = HFExecuteFaceTrack(session.Get(), st.Get(), &faces);
        if (r != HSUCCEED) return r;
        s.detected_faces = faces.detectedNum;
        if (faces.detectedNum < 1) return HERR_SESS_PIPELINE_FAILURE;
        HFSessionCustomParameter pipeParam = {};
        pipeParam.enable_liveness = 1;
        r = HFMultipleFacePipelineProcess(session.Get(), st.Get(), &faces, pipeParam);
        if (r != HSUCCEED) return r;
        HFRGBLivenessConfidence conf = {};
        r = HFGetRGBLivenessConfidence(session.Get(), &conf);
        if (r != HSUCCEED) return r;
        if (conf.num < 1 || !conf.confidence) return HERR_SESS_PIPELINE_FAILURE;
        if (!Finite(conf.confidence[0])) return HERR_SESS_PIPELINE_FAILURE;
        s.value_0 = conf.confidence[0];
        return HSUCCEED;
    });
    return s;
}

// --- Mask ---
PipelineCase RunMask(const std::string& image) {
    PipelineCase s;
    s.name = "mask";
    s.option_flag = HF_ENABLE_MASK_DETECT;
    HFSessionCustomParameter param = {};
    param.enable_mask_detect = 1;
    SessionOwner session;
    BitmapOwner b;
    StreamOwner st;
    if (!CreateSession(param, 320, &session, &s) ||
        !CreateStream(image, &b, &st, &s)) return s;
    Measure(&s, [&]() -> HResult {
        HFMultipleFaceData faces = {};
        HResult r = HFExecuteFaceTrack(session.Get(), st.Get(), &faces);
        if (r != HSUCCEED) return r;
        s.detected_faces = faces.detectedNum;
        if (faces.detectedNum < 1) return HERR_SESS_PIPELINE_FAILURE;
        HFSessionCustomParameter pipeParam = {};
        pipeParam.enable_mask_detect = 1;
        r = HFMultipleFacePipelineProcess(session.Get(), st.Get(), &faces, pipeParam);
        if (r != HSUCCEED) return r;
        HFFaceMaskConfidence conf = {};
        r = HFGetFaceMaskConfidence(session.Get(), &conf);
        if (r != HSUCCEED) return r;
        if (conf.num < 1 || !conf.confidence) return HERR_SESS_PIPELINE_FAILURE;
        if (!Finite(conf.confidence[0])) return HERR_SESS_PIPELINE_FAILURE;
        s.value_0 = conf.confidence[0];
        return HSUCCEED;
    });
    return s;
}

// --- Quality ---
PipelineCase RunQuality(const std::string& image) {
    PipelineCase s;
    s.name = "quality";
    s.option_flag = HF_ENABLE_QUALITY;
    HFSessionCustomParameter param = {};
    param.enable_face_quality = 1;
    SessionOwner session;
    BitmapOwner b;
    StreamOwner st;
    if (!CreateSession(param, 320, &session, &s) ||
        !CreateStream(image, &b, &st, &s)) return s;
    Measure(&s, [&]() -> HResult {
        HFMultipleFaceData faces = {};
        HResult r = HFExecuteFaceTrack(session.Get(), st.Get(), &faces);
        if (r != HSUCCEED) return r;
        s.detected_faces = faces.detectedNum;
        if (faces.detectedNum < 1) return HERR_SESS_PIPELINE_FAILURE;
        HFFaceQualityConfidence conf = {};
        r = HFGetFaceQualityConfidence(session.Get(), &conf);
        if (r != HSUCCEED) return r;
        if (conf.num < 1 || !conf.confidence) return HERR_SESS_PIPELINE_FAILURE;
        if (!Finite(conf.confidence[0])) return HERR_SESS_PIPELINE_FAILURE;
        s.value_0 = conf.confidence[0];
        return HSUCCEED;
    });
    return s;
}

// --- Attribute ---
PipelineCase RunAttribute(const std::string& image) {
    PipelineCase s;
    s.name = "attribute";
    s.option_flag = HF_ENABLE_FACE_ATTRIBUTE;
    HFSessionCustomParameter param = {};
    param.enable_face_attribute = 1;
    SessionOwner session;
    BitmapOwner b;
    StreamOwner st;
    if (!CreateSession(param, 320, &session, &s) ||
        !CreateStream(image, &b, &st, &s)) return s;
    Measure(&s, [&]() -> HResult {
        HFMultipleFaceData faces = {};
        HResult r = HFExecuteFaceTrack(session.Get(), st.Get(), &faces);
        if (r != HSUCCEED) return r;
        s.detected_faces = faces.detectedNum;
        if (faces.detectedNum < 1) return HERR_SESS_PIPELINE_FAILURE;
        HFSessionCustomParameter pipeParam = {};
        pipeParam.enable_face_attribute = 1;
        r = HFMultipleFacePipelineProcess(session.Get(), st.Get(), &faces, pipeParam);
        if (r != HSUCCEED) return r;
        HFFaceAttributeResult attr = {};
        r = HFGetFaceAttributeResult(session.Get(), &attr);
        if (r != HSUCCEED) return r;
        if (attr.num < 1 || !attr.gender || !attr.ageBracket || !attr.race)
            return HERR_SESS_PIPELINE_FAILURE;
        s.int_value_0 = attr.gender[0];
        s.value_0 = static_cast<double>(attr.ageBracket[0]);
        return HSUCCEED;
    });
    return s;
}

// --- Emotion ---
PipelineCase RunEmotion(const std::string& image) {
    PipelineCase s;
    s.name = "emotion";
    s.option_flag = HF_ENABLE_FACE_EMOTION;
    HFSessionCustomParameter param = {};
    param.enable_face_emotion = 1;
    SessionOwner session;
    BitmapOwner b;
    StreamOwner st;
    if (!CreateSession(param, 320, &session, &s) ||
        !CreateStream(image, &b, &st, &s)) return s;
    Measure(&s, [&]() -> HResult {
        HFMultipleFaceData faces = {};
        HResult r = HFExecuteFaceTrack(session.Get(), st.Get(), &faces);
        if (r != HSUCCEED) return r;
        s.detected_faces = faces.detectedNum;
        if (faces.detectedNum < 1) return HERR_SESS_PIPELINE_FAILURE;
        HFSessionCustomParameter pipeParam = {};
        pipeParam.enable_face_emotion = 1;
        r = HFMultipleFacePipelineProcess(session.Get(), st.Get(), &faces, pipeParam);
        if (r != HSUCCEED) return r;
        HFFaceEmotionResult emo = {};
        r = HFGetFaceEmotionResult(session.Get(), &emo);
        if (r != HSUCCEED) return r;
        if (emo.num < 1 || !emo.emotion) return HERR_SESS_PIPELINE_FAILURE;
        s.int_value_0 = emo.emotion[0];
        return HSUCCEED;
    });
    return s;
}

std::vector<PipelineCase> RunAll(const Arguments& a) {
    std::vector<PipelineCase> rows;
    HFResourcePackInfo info = {};
    info.structSize = sizeof(info);
    info.structVersion = HF_RESOURCE_PACK_INFO_VERSION;
    HResult r = HFValidateResourcePack(a.pack.c_str(), &info);
    if (r != HSUCCEED) {
        const char* names[] = {"liveness", "mask", "quality", "attribute", "emotion"};
        for (const char* n : names) {
            PipelineCase s; s.name = n; Fail(&s, "validate_pack", r); rows.push_back(s);
        }
        return rows;
    }
    LaunchOwner launch;
    r = HFLaunchInspireFace(a.pack.c_str());
    if (r != HSUCCEED) {
        const char* names[] = {"liveness", "mask", "quality", "attribute", "emotion"};
        for (const char* n : names) {
            PipelineCase s; s.name = n; Fail(&s, "launch", r); rows.push_back(s);
        }
        return rows;
    }
    launch.MarkLaunched();
    // Explicitly switch to CPU image processing backend
    // (library silently defaults to RGA when compiled with ISF_ENABLE_RGA=ON,
    //  but RGA may be unavailable on some board configurations)
    HFSwitchImageProcessingBackend(HF_IMAGE_PROCESSING_CPU);

    rows.push_back(RunLiveness(a.face_image));
    rows.push_back(RunMask(a.face_image));
    rows.push_back(RunQuality(a.face_image));
    rows.push_back(RunAttribute(a.face_image));
    rows.push_back(RunEmotion(a.face_image));
    return rows;
}

std::string CaseJson(const PipelineCase& s) {
    std::string lat;
    for (size_t i = 0; i < s.latency_ms.size(); ++i) {
        if (i) lat += ',';
        lat += std::to_string(s.latency_ms[i]);
    }
    return "{\"name\":\"" + EscapeJson(s.name)
        + "\",\"status\":\"" + EscapeJson(s.status)
        + "\",\"failure_stage\":\"" + EscapeJson(s.failure_stage)
        + "\",\"hresult\":" + std::to_string(static_cast<int>(s.hresult))
        + ",\"all_finite\":" + (s.all_finite ? "true" : "false")
        + ",\"peak_rss_kb\":" + std::to_string(s.peak_rss_kb)
        + ",\"latency_ms\":[" + lat + "]"
        + ",\"detected_faces\":" + std::to_string(s.detected_faces)
        + ",\"value_0\":" + std::to_string(s.value_0)
        + ",\"int_value_0\":" + std::to_string(s.int_value_0)
        + "}";
}

std::string BuildReport(const Arguments& a, const std::vector<PipelineCase>& rows) {
    std::string body;
    for (size_t i = 0; i < rows.size(); ++i) {
        if (i) body += ',';
        body += CaseJson(rows[i]);
    }
    return "{\"run_id\":\"" + EscapeJson(a.run_id)
        + "\",\"serial\":\"" + EscapeJson(a.serial)
        + "\",\"pack_sha256\":\"" + EscapeJson(a.pack_sha256)
        + "\",\"face_image_sha256\":\"" + EscapeJson(a.face_sha256)
        + "\",\"scenarios\":[" + body + "]}\n";
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
    if (!ParseArguments(argc, argv, &a)) {
        std::cerr << "invalid arguments\n";
        return 2;
    }
    const auto rows = RunAll(a);
    if (!WriteReport(a.result_path, BuildReport(a, rows))) {
        std::cerr << "cannot atomically publish result file\n";
        return 2;
    }
    for (const auto& s : rows) {
        if (s.status != "success") return 1;
    }
    return 0;
}
