"""Acceptance tests for the deterministic RV1126B conversion report."""

from __future__ import annotations

import copy
import importlib.util
import json
import os
import pathlib
import shutil
import tempfile
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]
ARTIFACTS = pathlib.Path(os.environ.get("RV1126B_ARTIFACTS", ROOT / "build" / "rv1126b-models"))
MODULE_PATH = ROOT / "command" / "rv1126b_models" / "report.py"

SPEC = importlib.util.spec_from_file_location("rv1126b_conversion_report", MODULE_PATH)
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(MODULE)


EXPECTED_IDS = [
    "attitude", "emotion", "landmark", "liveness", "mask", "quality", "recognition", "rnet",
    "scrfd_2_5g_160", "scrfd_2_5g_192", "scrfd_2_5g_256", "scrfd_2_5g_320", "scrfd_2_5g_640",
    "scrfd_500m_160", "scrfd_500m_192", "scrfd_500m_256", "scrfd_500m_320", "scrfd_500m_640",
]
REQUIRES_RECALIBRATION = [
    "liveness", "mask", "quality", "recognition", "rnet", "scrfd_2_5g_160", "scrfd_2_5g_192",
    "scrfd_2_5g_256", "scrfd_2_5g_320", "scrfd_2_5g_640", "scrfd_500m_160", "scrfd_500m_192",
    "scrfd_500m_256", "scrfd_500m_320", "scrfd_500m_640",
]


class ConversionReportTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = MODULE.build_report(ARTIFACTS)

    def test_exact_inventory_and_provisional_set(self):
        self.assertEqual(self.report["model_ids"], EXPECTED_IDS)
        self.assertEqual(self.report["requires_recalibration"], REQUIRES_RECALIBRATION)
        self.assertEqual(self.report["status_counts"], {"verified": 3, "provisional": 15})

    def test_report_is_deterministic_and_complete(self):
        second = MODULE.build_report(ARTIFACTS)
        self.assertEqual(self.report, second)
        for model in self.report["models"]:
            self.assertEqual(set(model), {
                "id", "family", "source", "model", "calibration", "native_tensor_contracts",
                "outputs", "latency_ms", "board", "failure_stage",
            })
            self.assertIn("sha256", model["source"])
            self.assertIn("sha256", model["model"])
            self.assertIn("dataset_sha256", model["calibration"])
            self.assertIn("manifest_sha256", model["calibration"])
            self.assertIn("toolkit_version", model["model"])
            self.assertIn("api_version", model["board"])
            self.assertIn("driver_version", model["board"])
            self.assertEqual(model["outputs"]["count"], len(model["outputs"]["sha256"]))
            self.assertEqual(model["outputs"]["count"], len(model["native_tensor_contracts"]["outputs"]))
            self.assertEqual(model["latency_ms"]["count"], 10)

    def test_gate_accepts_only_the_complete_validated_report(self):
        self.assertTrue(MODULE.can_start_pack_integration(self.report))
        cases = []
        missing_conversion = copy.deepcopy(self.report)
        missing_conversion["models"][0]["model"]["conversion_status"] = "missing"
        cases.append(missing_conversion)
        failed_board = copy.deepcopy(self.report)
        failed_board["models"][0]["board"]["status"] = "failed"
        cases.append(failed_board)
        hash_mismatch = copy.deepcopy(self.report)
        hash_mismatch["models"][0]["model"]["sha256"] = "0" * 64
        cases.append(hash_mismatch)
        wrong_target = copy.deepcopy(self.report)
        wrong_target["models"][0]["model"]["toolkit_version"] = "2.3.1"
        cases.append(wrong_target)
        wrong_platform = copy.deepcopy(self.report)
        wrong_platform["models"][0]["model"]["target"] = "rk3588"
        cases.append(wrong_platform)
        wrong_api = copy.deepcopy(self.report)
        wrong_api["models"][0]["board"]["api_version"] = "2.3.1"
        cases.append(wrong_api)
        nonfinite = copy.deepcopy(self.report)
        nonfinite["models"][0]["outputs"]["all_finite"] = False
        cases.append(nonfinite)
        source_hash = copy.deepcopy(self.report)
        source_hash["models"][0]["source"]["sha256"] = "0" * 64
        cases.append(source_hash)
        calibration_hash = copy.deepcopy(self.report)
        calibration_hash["models"][0]["calibration"]["dataset_sha256"] = "0" * 64
        cases.append(calibration_hash)
        output_hash = copy.deepcopy(self.report)
        output_hash["models"][0]["outputs"]["sha256"][0]["sha256"] = "0" * 64
        cases.append(output_hash)
        missing_contract = copy.deepcopy(self.report)
        missing_contract["models"][0]["native_tensor_contracts"]["outputs"].pop()
        cases.append(missing_contract)
        changed_contract = copy.deepcopy(self.report)
        changed_contract["models"][0]["native_tensor_contracts"]["inputs"][0]["dims"] = [1, 1, 1, 1]
        cases.append(changed_contract)
        bad_api_suffix = copy.deepcopy(self.report)
        bad_api_suffix["models"][0]["board"]["api_version"] = "2.3.20"
        cases.append(bad_api_suffix)
        bad_api_text = copy.deepcopy(self.report)
        bad_api_text["models"][0]["board"]["api_version"] = "2.3.2-not-a-version"
        cases.append(bad_api_text)
        unexpected = copy.deepcopy(self.report)
        unexpected["models"].append(copy.deepcopy(unexpected["models"][0]))
        unexpected["models"][-1]["id"] = "unexpected"
        unexpected["model_ids"].append("unexpected")
        cases.append(unexpected)
        for report in cases:
            with self.subTest(report=report["models"][0]["id"]):
                self.assertFalse(MODULE.can_start_pack_integration(report))

    def test_ingestion_rejects_unknown_and_duplicate_evidence_ids(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            models = root / "models"
            board = root / "board"
            models.mkdir()
            board.mkdir()
            for sidecar in (ARTIFACTS / "models").glob("*_rv1126b.json"):
                shutil.copy2(sidecar, models / sidecar.name)
            shutil.copy2(ARTIFACTS / "board" / "results.json", board / "results.json")

            duplicate = json.loads((models / "attitude_rv1126b.json").read_text(encoding="utf-8-sig"))
            (models / "duplicate_rv1126b.json").write_text(json.dumps(duplicate), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "duplicate conversion evidence ID"):
                MODULE.build_report(root)
            (models / "duplicate_rv1126b.json").unlink()

            unknown = json.loads((models / "attitude_rv1126b.json").read_text(encoding="utf-8-sig"))
            unknown["model_id"] = "unexpected"
            (models / "unexpected_rv1126b.json").write_text(json.dumps(unknown), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "conversion evidence IDs"):
                MODULE.build_report(root)
            (models / "unexpected_rv1126b.json").unlink()

            results = json.loads((board / "results.json").read_text(encoding="utf-8-sig"))
            results.append(copy.deepcopy(results[0]))
            (board / "results.json").write_text(json.dumps(results), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "duplicate board evidence ID"):
                MODULE.build_report(root)

    def test_report_marks_board_failures_at_the_specific_stage(self):
        inventory = MODULE._inventory()[0]
        sidecar = json.loads((ARTIFACTS / "models" / "attitude_rv1126b.json").read_text(encoding="utf-8-sig"))
        board = json.loads((ARTIFACTS / "board" / "results.json").read_text(encoding="utf-8-sig"))[0]
        board["status"] = "failed"
        board["error"] = "forced board failure"
        report = MODULE._model_report(ARTIFACTS, inventory, sidecar, board)
        self.assertEqual(report["failure_stage"], {
            "stage": "board", "message": "forced board failure",
        })

    def test_report_marks_hash_version_and_latency_failures_at_specific_stages(self):
        inventory = MODULE._inventory()[0]
        sidecar = json.loads((ARTIFACTS / "models" / "attitude_rv1126b.json").read_text(encoding="utf-8-sig"))
        board = json.loads((ARTIFACTS / "board" / "results.json").read_text(encoding="utf-8-sig"))[0]
        cases = []
        bad_hash = copy.deepcopy(sidecar)
        bad_hash["output_sha256"] = "0" * 64
        cases.append((bad_hash, board, "model_hash"))
        bad_toolkit = copy.deepcopy(sidecar)
        bad_toolkit["toolkit_version"] = "2.3.1"
        cases.append((bad_toolkit, board, "toolkit"))
        bad_api = copy.deepcopy(board)
        bad_api["api_version"] = "2.3.20"
        cases.append((sidecar, bad_api, "api"))
        bad_driver = copy.deepcopy(board)
        bad_driver["driver_version"] = "0.9.7"
        cases.append((sidecar, bad_driver, "driver"))
        bad_latency = copy.deepcopy(board)
        bad_latency["latency_ms"] = []
        cases.append((sidecar, bad_latency, "latency"))
        for selected_sidecar, selected_board, stage in cases:
            with self.subTest(stage=stage):
                report = MODULE._model_report(ARTIFACTS, inventory, selected_sidecar, selected_board)
                self.assertEqual(report["failure_stage"]["stage"], stage)

    def test_json_and_markdown_are_stable(self):
        rendered = MODULE.render_markdown(self.report)
        self.assertIn("requires restored calibration and accuracy regression", rendered)
        self.assertIn("compatibility evidence, not accuracy acceptance", rendered)
        self.assertEqual(json.loads(MODULE.render_json(self.report)), self.report)


if __name__ == "__main__":
    unittest.main()
