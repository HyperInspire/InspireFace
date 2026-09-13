"""Static contracts for the RV1126B RKNN2 runtime-parity tooling."""

from pathlib import Path
import unittest

from command.rv1126b_pack.contracts import selected_model_ids


ROOT = Path(__file__).resolve().parents[2]
RUNTIME = ROOT / "command" / "rv1126b_runtime"
RUNNER = RUNTIME / "rknn2_parity_runner.cpp"
BUILD = RUNTIME / "build_parity_runner.sh"
BOARD = RUNTIME / "run_board_parity.ps1"


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
        self.assertEqual(load_selected_ids(), list(selected_model_ids()))
        text = BOARD.read_text(encoding="utf-8")
        self.assertIn("selected_model_ids", text)
        self.assertIn("selected_model_count", text)
        self.assertIn("11", text)

    def test_cross_build_reuses_armhf_sdk_and_checks_elf_dependencies(self):
        text = BUILD.read_text(encoding="utf-8")
        for required in (
            "set -euo pipefail", "build_cross_rv1126b_armhf.sh", "SDK_INSTALL_DIR",
            "RKNN_RUNTIME_DIR", "arm-linux-gnueabihf", "hard-float ABI", "readelf",
            "libInspireFace.so", "librknnrt.so", "rknn2_parity_runner.cpp",
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
        ):
            self.assertIn(required, text)
        self.assertNotIn("shell -c", text)


if __name__ == "__main__":
    unittest.main()
