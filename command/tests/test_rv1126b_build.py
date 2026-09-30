"""Configure the real cross build without needing a connected board.

Requires CMake, aarch64-linux-gnu-gcc/g++, and the project's 3rdparty tree.
Set RKNN_RUNTIME_DIR to the official Toolkit2 2.3.2+ Linux/librknn_api directory.
"""

import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]


class RV1126BBuildTest(unittest.TestCase):
    def test_uses_rknn2_and_selected_runtime(self):
        runtime = Path(os.environ["RKNN_RUNTIME_DIR"]).resolve()
        with tempfile.TemporaryDirectory(prefix="rv1126b-") as directory:
            build = Path(directory)
            result = subprocess.run(
                ["cmake", "-G", "Unix Makefiles", "-S", str(ROOT), "-B", str(build),
                 "-DCMAKE_SYSTEM_NAME=Linux", "-DCMAKE_SYSTEM_PROCESSOR=armv7",
                 "-DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc",
                 "-DCMAKE_CXX_COMPILER=arm-linux-gnueabihf-g++",
                 "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
                 "-DISF_BUILD_LINUX_ARM7=ON", "-DISF_ENABLE_RKNN=ON",
                 "-DISF_RK_DEVICE_TYPE=RV1126B", "-DISF_RK_COMPILER_TYPE=armhf",
                 "-DISF_RKNN_RUNTIME_DIR=" + str(runtime), "-DISF_ENABLE_RGA=ON",
                 "-DISF_BUILD_WITH_SAMPLE=OFF", "-DISF_BUILD_WITH_TEST=OFF"],
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
            )
            self.assertEqual(result.returncode, 0, result.stdout)
            commands = json.loads((build / "compile_commands.json").read_text())
            adapter = next(item["command"] for item in commands
                           if item["file"].endswith("inference_wrapper_rknn_adapter_nano.cpp"))
            self.assertIn("-DINFERENCE_WRAPPER_ENABLE_RKNN2", adapter)
            self.assertIn("-DISF_RKNPU_RV1126B", adapter)
            self.assertNotIn("-DISF_RKNPU_RV1106", adapter)
            self.assertIn(str(runtime / "include"), adapter)
            link = (build / "cpp/inspireface/CMakeFiles/InspireFace.dir/link.txt").read_text()
            self.assertIn(str(runtime / "armhf"), link)
            self.assertNotIn("librknnmrt.a", link)

    def test_existing_rknn2_devices_keep_bundled_runtime(self):
        for device in ("RK356X", "RK3588"):
            with self.subTest(device=device), tempfile.TemporaryDirectory() as directory:
                result = subprocess.run(
                    ["cmake", "-G", "Unix Makefiles", "-S", str(ROOT), "-B", directory,
                     "-DCMAKE_SYSTEM_NAME=Linux", "-DCMAKE_SYSTEM_PROCESSOR=aarch64",
                     "-DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc",
                     "-DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++",
                     "-DISF_BUILD_LINUX_AARCH64=ON", "-DISF_ENABLE_RKNN=ON",
                     "-DISF_RK_DEVICE_TYPE=" + device, "-DISF_RK_COMPILER_TYPE=aarch64",
                     "-DISF_ENABLE_RGA=ON", "-DISF_BUILD_WITH_SAMPLE=OFF",
                     "-DISF_BUILD_WITH_TEST=OFF"],
                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
                )
                self.assertEqual(result.returncode, 0, result.stdout)
                link = (Path(directory) / "cpp/inspireface/CMakeFiles/InspireFace.dir/link.txt").read_text()
                self.assertIn("inspireface-precompile-lite/rknn/rknpu2/runtime/Linux/librknn_api/aarch64", link)

    def test_rv1126b_does_not_silently_use_old_bundled_runtime(self):
        with tempfile.TemporaryDirectory() as directory:
            result = subprocess.run(
                ["cmake", "-S", str(ROOT), "-B", directory,
                 "-DCMAKE_SYSTEM_NAME=Linux", "-DCMAKE_SYSTEM_PROCESSOR=armv7",
                 "-DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc",
                 "-DCMAKE_CXX_COMPILER=arm-linux-gnueabihf-g++",
                 "-DISF_BUILD_LINUX_ARM7=ON",
                 "-DISF_ENABLE_RKNN=ON", "-DISF_RK_DEVICE_TYPE=RV1126B",
                 "-DISF_RK_COMPILER_TYPE=armhf"],
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("Set ISF_RKNN_RUNTIME_DIR", result.stdout)

    def test_rejects_non_linux_target(self):
        with tempfile.TemporaryDirectory() as directory:
            result = subprocess.run(
                ["cmake", "-S", str(ROOT), "-B", directory,
                 "-DCMAKE_SYSTEM_NAME=FreeBSD", "-DCMAKE_SYSTEM_PROCESSOR=armv7",
                 "-DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc",
                 "-DCMAKE_CXX_COMPILER=arm-linux-gnueabihf-g++",
                 "-DISF_ENABLE_RKNN=ON", "-DISF_RK_DEVICE_TYPE=RV1126B",
                 "-DISF_RK_COMPILER_TYPE=armhf",
                 "-DISF_RKNN_RUNTIME_DIR=" + os.environ["RKNN_RUNTIME_DIR"]],
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
            )
            self.assertIn("RV1126B currently requires a Linux (non-Android) toolchain", result.stdout)
            self.assertNotEqual(result.returncode, 0)

    def test_rejects_runtime_older_than_2_3(self):
        old_runtime = ROOT / "3rdparty/inspireface-precompile-lite/rknn/rknpu2/runtime/Linux/librknn_api"
        with tempfile.TemporaryDirectory() as directory:
            result = subprocess.run(
                ["cmake", "-S", str(ROOT), "-B", directory,
                 "-DCMAKE_SYSTEM_NAME=Linux", "-DCMAKE_SYSTEM_PROCESSOR=armv7",
                 "-DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc",
                 "-DCMAKE_CXX_COMPILER=arm-linux-gnueabihf-g++",
                 "-DISF_BUILD_LINUX_ARM7=ON", "-DISF_ENABLE_RKNN=ON",
                 "-DISF_RK_DEVICE_TYPE=RV1126B", "-DISF_RK_COMPILER_TYPE=armhf",
                 "-DISF_RKNN_RUNTIME_DIR=" + str(old_runtime)],
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("requires RKNN Runtime 2.3.2 or newer", result.stdout)

    def test_explicit_runtime_version_supports_library_without_embedded_version_string(self):
        source_runtime = Path(os.environ["RKNN_RUNTIME_DIR"]).resolve()
        with tempfile.TemporaryDirectory() as runtime_directory, tempfile.TemporaryDirectory() as build_directory:
            runtime = Path(runtime_directory)
            (runtime / "include").mkdir()
            (runtime / "armhf").mkdir()
            shutil.copyfile(source_runtime / "include/rknn_api.h", runtime / "include/rknn_api.h")
            (runtime / "armhf/librknnrt.so").write_bytes(b"no embedded version marker")
            result = subprocess.run(
                ["cmake", "-G", "Unix Makefiles", "-S", str(ROOT), "-B", build_directory,
                 "-DCMAKE_SYSTEM_NAME=Linux", "-DCMAKE_SYSTEM_PROCESSOR=armv7",
                 "-DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc",
                 "-DCMAKE_CXX_COMPILER=arm-linux-gnueabihf-g++",
                 "-DISF_BUILD_LINUX_ARM7=ON", "-DISF_ENABLE_RKNN=ON",
                 "-DISF_RK_DEVICE_TYPE=RV1126B", "-DISF_RK_COMPILER_TYPE=armhf",
                 "-DISF_RKNN_RUNTIME_DIR=" + str(runtime), "-DISF_RKNN_RUNTIME_VERSION=2.3.2",
                 "-DISF_BUILD_WITH_SAMPLE=OFF", "-DISF_BUILD_WITH_TEST=OFF"],
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
            )
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn("RV1126B RKNN Runtime: 2.3.2", result.stdout)


if __name__ == "__main__":
    unittest.main()
