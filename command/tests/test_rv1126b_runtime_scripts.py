"""Static contracts for the RV1126B RKNN2 runtime-parity tooling."""

from pathlib import Path
import hashlib
import json
import os
import re
import shutil
import subprocess
import tempfile
import unittest

from command.rv1126b_pack.contracts import selected_model_ids


ROOT = Path(__file__).resolve().parents[2]
RUNTIME = ROOT / "command" / "rv1126b_runtime"
RUNNER = RUNTIME / "rknn2_parity_runner.cpp"
BUILD = RUNTIME / "build_parity_runner.sh"
BOARD = RUNTIME / "run_board_parity.ps1"
RAW_INPUTS = RUNTIME / "prepare_raw_inputs.py"


def load_selected_ids() -> list[str]:
    """The runtime matrix is the immutable default resource-pack selection."""
    return list(selected_model_ids())


class RV1126BRuntimeParityScriptTests(unittest.TestCase):
    def test_runner_executes_both_reference_and_production_paths(self):
        text = RUNNER.read_text(encoding="utf-8")
        self.assertIn("rknn_inputs_set", text)
        self.assertIn("rknn_outputs_get", text)
        self.assertIn("InferenceWrapperRKNNAdapter", text)
        self.assertIn("want_float = 1", text)
        self.assertIn("kWarmupIterations = 10", text)
        self.assertIn("kMeasuredIterations = 10", text)
        self.assertIn("max_abs", text)
        self.assertIn("mean_abs", text)
        self.assertIn("cosine", text)
        self.assertIn("failure_stage", text)
        self.assertIn("RKNN_QUERY_NATIVE_NHWC_OUTPUT_ATTR", text)

    def test_runner_records_contracts_and_required_model_shapes(self):
        text = RUNNER.read_text(encoding="utf-8")
        for required in (
            "normal_inputs", "normal_outputs", "native_outputs", "binding_inputs",
            "logical_dims", "qnt_type", "scale", "zp", "rnet_stride", "scrfd_output_count",
            "attitude_output_count", "model_sha256", "input_sha256", "all_finite",
        ):
            self.assertIn(required, text)

    def test_board_matrix_is_exactly_the_pack_selection(self):
        text = BOARD.read_text(encoding="utf-8")
        start = text.index("$ExpectedIds = @(")
        end = text.index(")", start)
        actual = re.findall(r"'([a-z0-9_]+)'", text[start:end])
        self.assertEqual(actual, list(selected_model_ids()))

    def test_cross_build_reuses_armhf_sdk_and_checks_elf_dependencies(self):
        text = BUILD.read_text(encoding="utf-8")
        for required in (
            "set -euo pipefail", "build_cross_rv1126b_armhf.sh", "SDK_INSTALL_DIR",
            "RKNN_RUNTIME_DIR", "arm-linux-gnueabihf", "hard-float ABI", "readelf",
            "libInspireFace.so", "librknnrt.so", "rknn2_parity_runner.cpp",
            "inference_wrapper_rknn_adapter_nano.cpp", "rknn_include", "-Wno-unknown-pragmas",
        ):
            self.assertIn(required, text)

    def test_board_script_is_hash_gated_safe_and_atomic(self):
        text = BOARD.read_text(encoding="utf-8")
        for required in (
            "e3d7377f6fc6d325", "-s $Serial", "/userdata/inspireface-rv1126b/runtime-parity",
            "command.rv1126b_pack.validate_pack", "Get-FileHash", "'readlink', '-f'",
            "'rm', '-rf', $RemoteDirectory", "$RunId", ".partial", "Move-Item",
            "input_sha256", "model_sha256", "ConvertTo-Json", "failure_stage",
            "continue", "Replace(\"`r`n\", \"`n\")",
            "runtime_version", "driver_version", "peak_rss_kb", "Test-RuntimeVersion",
        ):
            self.assertIn(required, text)
        self.assertNotIn("shell -c", text)

    def test_runner_uses_result_file_and_rejects_too_old_runtime(self):
        text = RUNNER.read_text(encoding="utf-8")
        self.assertIn("--result", text)
        self.assertIn("WriteReport", text)
        self.assertNotIn("std::cout << report", text)
        self.assertIn("RuntimeAtLeast", text)
        self.assertIn("2, 3, 2", text)
        self.assertIn("peak_rss_kb", text)


@unittest.skipUnless(os.name == "posix", "Linux cross-build argv behavior")
class ParityBuildArgumentTests(unittest.TestCase):
    def test_sdk_install_still_compiles_with_explicit_rknn_header_and_only_known_warning_waived(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk, runtime, tools, output = root / "sdk", root / "runtime", root / "tools", root / "out"
            for path in (sdk / "include", sdk / "lib", runtime / "include", runtime / "armhf", tools):
                path.mkdir(parents=True, exist_ok=True)
            (sdk / "include" / "inspireface.h").write_text("fixture")
            (runtime / "include" / "rknn_api.h").write_text("fixture")
            for library in (sdk / "lib" / "libInspireFace.so", sdk / "lib" / "librknnrt.so", runtime / "armhf" / "librknnrt.so"):
                library.write_text("fixture")
            compiler = tools / "arm-linux-gnueabihf-g++"
            compiler.write_text("#!/bin/sh\nif [ \"$1\" = -dumpmachine ]; then echo arm-linux-gnueabihf; exit 0; fi\nprintf '%s\\n' \"$@\" > \"$FAKE_ARGS\"\nwhile [ $# -gt 0 ]; do [ \"$1\" = -o ] && { : > \"$2\"; exit 0; }; shift; done\n")
            readelf = tools / "arm-linux-gnueabihf-readelf"
            readelf.write_text("#!/bin/sh\ncase \"$1\" in -h) printf 'Class: ELF32\\nMachine: ARM\\nFlags: Version5 EABI, hard-float ABI\\n';; -A) echo 'Tag_ABI_VFP_args: VFP registers';; -d) echo 'Shared library: [libInspireFace.so]'; echo 'Shared library: [librknnrt.so]';; esac\n")
            file_tool = tools / "file"
            file_tool.write_text("#!/bin/sh\nexit 0\n")
            for tool in (compiler, readelf, file_tool):
                tool.chmod(0o755)
            args = root / "args.txt"
            env = dict(os.environ, PATH=str(tools) + os.pathsep + os.environ["PATH"], SDK_INSTALL_DIR=str(sdk), RKNN_RUNTIME_DIR=str(runtime), FAKE_ARGS=str(args))
            completed = subprocess.run(["bash", str(BUILD), str(output)], env=env, capture_output=True, text=True)
            self.assertEqual(completed.returncode, 0, completed.stdout + completed.stderr)
            argv = args.read_text()
            self.assertIn("-I" + str(runtime / "include"), argv)
            self.assertIn("-Werror", argv)
            self.assertIn("-Wno-unknown-pragmas", argv)
            self.assertIn("inference_wrapper_rknn_adapter_nano.cpp", argv)


class RawUint8InputTests(unittest.TestCase):
    def test_generator_emits_raw_nhwc_provenance_before_normalization(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            image = root / "image.ppm"
            image.write_bytes(b"P6\n2 1\n255\n\x01\x02\x03\x04\x05\x06")
            sidecar = root / "model.json"
            sidecar.write_text(json.dumps({
                "model_id": "unit", "input": {"dtype": "uint8", "layout": "NHWC", "shape": [1, 1, 2, 3]},
                "preprocess": {"color_order": "BGR", "resize": {"width": 2, "height": 1}, "mean": [1, 2, 3], "std": [4, 5, 6]},
            }))
            output = root / "out"
            result = subprocess.run(
                ["python", "-B", str(RAW_INPUTS), str(sidecar), str(image), str(output)],
                capture_output=True, text=True,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            payload = (output / "input_0.bin").read_bytes()
            provenance = json.loads((output / "input_provenance.json").read_text())
            self.assertEqual(provenance["dtype"], "uint8")
            self.assertEqual(provenance["layout"], "NHWC")
            self.assertEqual(provenance["preprocess_stage"], "raw_uint8_after_resize_and_color")
            self.assertEqual(provenance["pass_through"], 0)
            self.assertEqual(provenance["input_sha256"], hashlib.sha256(payload).hexdigest())
            self.assertEqual(len(payload), 6)


if __name__ == "__main__":
    unittest.main()
