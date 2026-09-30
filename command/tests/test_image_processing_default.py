"""Run the real Launch implementation without model or accelerator dependencies."""

import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
LAUNCH = ROOT / "cpp/inspireface/runtime_module/launch.cpp"
GUARD = ROOT / "cpp/sample/benchmark/image_backend_default_guard.cpp"

# Only unrelated archive, logging and platform services are stubbed. Launch's
# actual constructor, singleton, backend getter/setter and public header compile
# unchanged, so enabling RGA must not silently choose it for new consumers.
STUBS = {
    "data_type.h": "#pragma once\n#include <vector>\n#define INSPIRE_API_EXPORT\n",
    "log.h": "#pragma once\n#define INSPIRE_LOGI(...)\n#define INSPIRE_LOGW(...)\n#define INSPIRE_LOGE(...)\n",
    "isf_check.h": "#pragma once\n#define INSPIREFACE_CHECK_MSG(...)\n",
    "meta.h": "#pragma once\n",
    "herror.h": """#pragma once
constexpr int HSUCCEED=0, HERR_ARCHIVE_LOAD_FAILURE=1, HERR_ARCHIVE_LOAD_MODEL_FAILURE=2,
HERR_ARCHIVE_NOT_LOAD=3, HERR_INVALID_PARAM=4;
""",
    "middleware/inference_wrapper/inference_wrapper.h": """#pragma once
class InferenceWrapper { public: enum SpecialBackend { COREML_CPU, COREML_GPU, COREML_ANE }; };
""",
    "image_process/nexus_processor/rga/dma_alloc.h": """#pragma once
#define RV1106_CMA_HEAP_PATH "/dev/dma_heap/rv1106"
#define DMA_HEAP_UNCACHE_PATH "/dev/dma_heap/system-uncached"
#define DMA_HEAP_DMA32_UNCACHE_PATCH "/dev/dma_heap/dma32-uncached"
""",
    "middleware/system.h": """#pragma once
#include <string>
namespace inspire { namespace os {
inline std::string Basename(const std::string& p) { return p; }
inline std::string Dirname(const std::string& p) { return p; }
inline std::string PathJoin(const std::string& a, const std::string& b) { return a+b; }
inline bool IsExists(const std::string&) { return false; }
inline bool IsDir(const std::string&) { return false; }
} }
""",
    "middleware/model_archive/inspire_archive.h": """#pragma once
#include <stdexcept>
#include <string>
#include <vector>
namespace inspire {
constexpr int SARC_SUCCESS = 0;
struct SimilarityConverterConfig { double threshold = 0.32; };
class SimilarityConverter { public:
static SimilarityConverter& getInstance() { static SimilarityConverter c; return c; }
bool updateConfigAndRecommendedThreshold(const SimilarityConverterConfig&, float) { return true; }
};
class InspireArchive { public:
void ReLoad(const std::string&) {}
int QueryStatus() const { return SARC_SUCCESS; }
std::vector<int> GetFaceDetectPixelList() const { return {160,320,640}; }
std::vector<std::string> GetFaceDetectModelList() const { return {"160","320","640"}; }
SimilarityConverterConfig GetSimilarityConverterConfig() const { return {}; }
bool SwitchLandmarkEngine(const char*) { return true; }
};
}
""",
}


def compiler_environment():
    for name in ("g++", "clang++"):
        compiler = shutil.which(name)
        if compiler:
            return compiler, dict(os.environ), False
    if os.name == "nt":
        vswhere = Path(os.environ.get("ProgramFiles(x86)", "C:/Program Files (x86)")) / "Microsoft Visual Studio/Installer/vswhere.exe"
        if vswhere.is_file():
            installation = subprocess.check_output([
                str(vswhere), "-latest", "-products", "*", "-requires",
                "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath",
            ], text=True).strip()
            if installation:
                directory = Path(installation)
                vcvars = directory / "VC/Auxiliary/Build/vcvars64.bat"
                compiler = sorted(directory.glob("VC/Tools/MSVC/*/bin/Hostx64/x64/cl.exe"))[-1]
                output = subprocess.check_output('call "{}" >nul && set'.format(vcvars), shell=True, text=True)
                env = dict(os.environ)
                env.update(line.split("=", 1) for line in output.splitlines() if "=" in line and not line.startswith("="))
                return str(compiler), env, True
    raise unittest.SkipTest("A C++14 compiler is required for the Launch backend regression")


class ImageProcessingDefaultTests(unittest.TestCase):
    def test_cpu_default_and_explicit_rga_selection_across_builds(self):
        compiler, env, msvc = compiler_environment()
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            shutil.copyfile(ROOT / "cpp/inspireface/include/inspireface/launch.h", root / "launch.h")
            for name, content in STUBS.items():
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(content, encoding="utf-8")
            for definitions in ((), ("ISF_ENABLE_RGA",), ("ISF_ENABLE_RGA", "ISF_RKNPU_RV1106"),
                                ("ISF_ENABLE_RGA", "ISF_RKNPU_RV1126B")):
                with self.subTest(definitions=definitions):
                    binary = root / ("guard.exe" if msvc else "guard")
                    if msvc:
                        command = [compiler, "/nologo", "/std:c++14", "/EHsc", "/I" + str(root), "/Fe:" + str(binary)]
                        command += ["/D" + definition for definition in definitions]
                    else:
                        command = [compiler, "-std=c++14", "-pthread", "-I", str(root), "-o", str(binary)]
                        command += ["-D" + definition for definition in definitions]
                    compiled = subprocess.run(command + [str(LAUNCH), str(GUARD)], cwd=root, env=env, capture_output=True, text=True)
                    self.assertEqual(compiled.returncode, 0, compiled.stdout + compiled.stderr)
                    result = subprocess.run([str(binary)], cwd=root, env=env, capture_output=True, text=True)
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
