#include "inspireface.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <string>
#include <vector>

namespace {
constexpr int kIterations = 5;

struct Arguments {
    std::string pack, face_image, result_path;
};

bool ParseArguments(int argc, char** argv, Arguments* a) {
    if (!a || argc != 7) return false;
    for (int i = 1; i < argc; i += 2) {
        const std::string k(argv[i]), v(argv[i + 1]);
        if (k == "--pack") a->pack = v;
        else if (k == "--face-image") a->face_image = v;
        else if (k == "--result-path") a->result_path = v;
        else return false;
    }
    return !a->pack.empty() && !a->face_image.empty() && !a->result_path.empty();
}
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

struct ProbeCase {
    std::string label;
    HResult hresult = HERR_INVALID_PARAM;
    HInt32 detected_faces = -1;
    bool all_finite = false;
    int failure_count = 0;
    std::vector<double> latency_ms;
};

std::string EscapeJson(const std::string& v) {
    std::string o;
    for (unsigned char byte : v) {
        if (byte == '"') o += "\\\"";
        else if (byte == '\\') o += "\\\\";
        else o += static_cast<char>(byte);
    }
    return o;
}

bool RunDetect(HFSession session, HFImageStream stream, ProbeCase* c) {
    HFMultipleFaceData faces = {};
    HResult r = HFExecuteFaceTrack(session, stream, &faces);
    c->hresult = r;
    if (r != HSUCCEED) { c->detected_faces = -1; return false; }
    c->detected_faces = faces.detectedNum;
    c->all_finite = true;
    for (HInt32 i = 0; i < faces.detectedNum; ++i) {
        if (!std::isfinite(faces.detConfidence[i])) { c->all_finite = false; }
    }
    return true;
}

void Measure(HFSession session, HFImageStream stream, ProbeCase* c) {
    for (int i = 0; i < 2; ++i) RunDetect(session, stream, c);  // warmup
    for (int i = 0; i < kIterations; ++i) {
        const auto begin = std::chrono::steady_clock::now();
        const bool ok = RunDetect(session, stream, c);
        const double ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - begin).count();
        if (!ok) ++c->failure_count;
        else c->latency_ms.push_back(ms);
    }
}

std::string CaseJson(const ProbeCase& c) {
    std::string lat;
    for (size_t i = 0; i < c.latency_ms.size(); ++i) {
        if (i) lat += ',';
        lat += std::to_string(c.latency_ms[i]);
    }
    return "{\"label\":\"" + EscapeJson(c.label)
        + "\",\"hresult\":" + std::to_string(static_cast<int>(c.hresult))
        + ",\"detected_faces\":" + std::to_string(c.detected_faces)
        + ",\"all_finite\":" + (c.all_finite ? "true" : "false")
        + ",\"failure_count\":" + std::to_string(c.failure_count)
        + ",\"latency_ms\":[" + lat + "]}";
}

}  // namespace

int main(int argc, char** argv) {
    Arguments a;
    if (!ParseArguments(argc, argv, &a)) { std::cerr << "invalid arguments\n"; return 2; }

    std::vector<ProbeCase> cases;

    LaunchOwner launch;
    HResult r = HFLaunchInspireFace(a.pack.c_str());
    if (r != HSUCCEED) { std::cerr << "launch failed: " << r << "\n"; return 2; }
    launch.MarkLaunched();

    // Query the default configured heap path for the report.
    char default_path[256] = {};
    HFQueryExpansiveHardwareRockchipDmaHeapPathWithSize(default_path, sizeof(default_path));

    HFSessionCustomParameter param = {};
    SessionOwner session;
    BitmapOwner b;
    StreamOwner st;
    r = HFCreateInspireFaceSession(param, HF_DETECT_MODE_ALWAYS_DETECT, 1, 320, -1, session.Out());
    if (r != HSUCCEED) { std::cerr << "create session failed: " << r << "\n"; return 2; }
    r = HFCreateImageBitmapFromFilePath(a.face_image.c_str(), 3, b.Out());
    if (r != HSUCCEED) { std::cerr << "create bitmap failed: " << r << "\n"; return 2; }
    r = HFCreateImageStreamFromImageBitmap(b.Get(), HF_CAMERA_ROTATION_0, st.Out());
    if (r != HSUCCEED) { std::cerr << "create stream failed: " << r << "\n"; return 2; }

    // Case 1: RGA backend with the compiled-in default heap path (expected to fail on RV1126B).
    HFSwitchImageProcessingBackend(HF_IMAGE_PROCESSING_RGA);
    ProbeCase c1;
    c1.label = "rga_default_heap";
    Measure(session.Get(), st.Get(), &c1);
    cases.push_back(c1);

    // Case 2: RGA backend with the heap path corrected to the board's device.
    HResult set_heap = HFSetExpansiveHardwareRockchipDmaHeapPath("/dev/dma_heap/system-uncached");
    ProbeCase c2;
    c2.label = set_heap == HSUCCEED ? "rga_corrected_heap" : "rga_set_heap_failed";
    if (set_heap == HSUCCEED) {
        Measure(session.Get(), st.Get(), &c2);
    } else {
        c2.hresult = set_heap;
    }
    cases.push_back(c2);

    // Emit report.
    std::string body;
    for (size_t i = 0; i < cases.size(); ++i) {
        if (i) body += ',';
        body += CaseJson(cases[i]);
    }
    char query_path[256] = {};
    HFQueryExpansiveHardwareRockchipDmaHeapPathWithSize(query_path, sizeof(query_path));
    std::string report = "{\"default_heap_path\":\"" + EscapeJson(default_path)
        + "\",\"configured_heap_path\":\"" + EscapeJson(query_path)
        + "\",\"cases\":[" + body + "]}\n";
    const std::string temp = a.result_path + ".partial";
    std::FILE* f = std::fopen(temp.c_str(), "wb");
    bool ok = false;
    if (f) {
        ok = std::fwrite(report.data(), 1, report.size(), f) == report.size();
        ok = ok && std::fclose(f) == 0;
        ok = ok && std::rename(temp.c_str(), a.result_path.c_str()) == 0;
    }
    if (!ok) { std::cerr << "cannot publish result\n"; return 2; }

    // Exit 0 if the corrected heap case detected a face.
    return (cases.size() == 2 && cases[1].detected_faces >= 1) ? 0 : 1;
}

