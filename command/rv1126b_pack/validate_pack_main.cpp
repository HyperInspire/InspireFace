#include "inspireface.h"

#include <cstdio>
#include <cstring>
#include <sys/resource.h>

namespace {

constexpr const char* kExpectedTag = "Gundam_RV1126B";
void PrintJsonString(const char* value) {
    std::putchar('"');
    for (const unsigned char* character = reinterpret_cast<const unsigned char*>(value); *character != '\0'; ++character) {
        switch (*character) {
            case '"': std::fputs("\\\"", stdout); break;
            case '\\': std::fputs("\\\\", stdout); break;
            case '\n': std::fputs("\\n", stdout); break;
            case '\r': std::fputs("\\r", stdout); break;
            case '\t': std::fputs("\\t", stdout); break;
            default:
                if (*character < 0x20) {
                    std::printf("\\u%04x", static_cast<unsigned int>(*character));
                } else {
                    std::putchar(*character);
                }
        }
    }
    std::putchar('"');
}

void PrintJson(const char* status, HFStatus sdk_status, const HFResourcePackInfo& info, const char* error) {
    rusage usage = {};
    getrusage(RUSAGE_SELF, &usage);
    std::fputs("{\"status\":", stdout);
    PrintJsonString(status);
    std::printf(",\"sdk_status\":%d,\"archive_file_count\":%u,\"model_count\":%u,\"tag\":",
                static_cast<int>(sdk_status), info.archiveFileCount, info.modelCount);
    PrintJsonString(info.tag);
    std::fputs(",\"version\":", stdout);
    PrintJsonString(info.version);
    std::fputs(",\"major\":", stdout);
    PrintJsonString(info.major);
    std::fputs(",\"release_date\":", stdout);
    PrintJsonString(info.releaseDate);
    std::printf(",\"peak_rss_kb\":%ld,\"error\":", usage.ru_maxrss);
    PrintJsonString(error);
    std::fputs("}\n", stdout);
}

}  // namespace

int main(int argc, char** argv) {
    HFResourcePackInfo info = {};
    info.structSize = sizeof(info);
    info.structVersion = HF_RESOURCE_PACK_INFO_VERSION;
    if (argc != 2) {
        PrintJson("failed", HERR_INVALID_PARAM, info, "usage: validate_pack_main PACK_PATH");
        return 1;
    }

    // The loader logs its banner to stdout by default; reserve stdout for the result JSON.
    HFSetLogLevel(HF_LOG_NONE);
    const HFStatus status = HFValidateResourcePack(argv[1], &info);
    if (status != HSUCCEED) {
        PrintJson("failed", status, info, "HFValidateResourcePack failed");
        return 1;
    }
    if (std::strcmp(info.tag, kExpectedTag) != 0) {
        PrintJson("failed", status, info, "unexpected resource-pack tag");
        return 1;
    }
    if (info.modelCount != 11) {
        PrintJson("failed", status, info, "unexpected resource-pack model count");
        return 1;
    }
    if (info.archiveFileCount != 12 || std::strcmp(info.version, "4.0") != 0 || std::strcmp(info.major, "t4") != 0) {
        PrintJson("failed", status, info, "unexpected resource-pack archive count/version/major");
        return 1;
    }
    PrintJson("success", status, info, "");
    return 0;
}
