"""Tests for checksum-gated RV1126B resource-pack construction."""

from __future__ import annotations

import copy
import hashlib
import json
import pathlib
import tarfile
import tempfile
import unittest
from unittest import mock

from command.rv1126b_models.records import load_inventory
from command.rv1126b_pack import build_pack
from command.rv1126b_pack.contracts import selected_model_ids


ROOT = pathlib.Path(__file__).resolve().parents[2]
INVENTORY = ROOT / "command" / "rv1126b_models" / "model_inventory.json"


class RV1126BPackBuildTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = pathlib.Path(self.temp.name)
        self.models = self.root / "models"
        self.evidence = self.root / "evidence"
        self.output = self.root / "pack" / "Gundam_RV1126B"
        self.records = load_inventory(INVENTORY)
        self.selected = selected_model_ids()
        self.by_id = {record["id"]: record for record in self.records}
        self.models.mkdir()
        self.evidence.mkdir()
        self._write_selected_artifacts()
        self.report = self._report()

    def tearDown(self):
        self.temp.cleanup()

    def _write_selected_artifacts(self):
        for model_id in self.selected:
            model = self.models / pathlib.PurePosixPath(self.by_id[model_id]["output"]).name
            model.write_bytes(f"rknn:{model_id}".encode("ascii"))
            digest = hashlib.sha256(model.read_bytes()).hexdigest()
            model.with_suffix(".json").write_text(json.dumps({
                "model_id": model_id,
                "conversion_status": "success",
                "target": "rv1126b",
                "toolkit_version": "2.3.2",
                "output_sha256": digest,
                "calibration_status": self.by_id[model_id]["calibration_status"],
            }), encoding="utf-8")

    def _report(self):
        models = []
        for record in self.records:
            model_id = record["id"]
            model_path = self.models / pathlib.PurePosixPath(record["output"]).name
            digest = hashlib.sha256(model_path.read_bytes()).hexdigest() if model_path.is_file() else "0" * 64
            models.append({
                "id": model_id,
                "failure_stage": None,
                "model": {"sha256": digest, "record_sha256": digest, "conversion_status": "success"},
                "calibration": {"status": record["calibration_status"]},
            })
        return {"models": models, "can_start_pack_integration": True}

    def _build(self, *, report=None, gate=True, **kwargs):
        with mock.patch.object(build_pack, "build_report", return_value=report or self.report), \
             mock.patch.object(build_pack, "can_start_pack_integration", return_value=gate):
            return build_pack.build_resource_pack(INVENTORY, self.models, self.evidence, self.output, **kwargs)

    def test_builds_deterministic_pack_and_sorted_report_from_exact_evidence(self):
        first = self._build()
        first_bytes = self.output.read_bytes()
        second = self._build()
        self.assertEqual(first_bytes, self.output.read_bytes())
        self.assertEqual(first["selected_model_count"], 11)
        self.assertEqual(first["verified"], ["attitude", "emotion", "landmark"])
        self.assertEqual(len(first["requires_recalibration"]), 8)
        self.assertEqual(hashlib.sha256(first_bytes).hexdigest(), second["pack_sha256"])
        self.assertEqual(first["artifact_root"], str(self.models))
        self.assertEqual(first["evidence_root"], str(self.evidence))
        report_path = self.output.with_name(self.output.name + ".report.json")
        self.assertEqual(json.loads(report_path.read_text(encoding="utf-8")), first)
        self.assertEqual(first["members"], sorted(first["members"], key=lambda entry: entry["member"]))
        with tarfile.open(self.output) as archive:
            self.assertEqual(archive.getnames(), ["__inspire__"] + sorted(entry["member"] for entry in first["members"]))
            manifest = json.loads(archive.extractfile("__inspire__").read())
        self.assertEqual(manifest["tag"], "Gundam_RV1126B")

    def test_rejects_model_or_evidence_hash_mismatch_without_partial_pack(self):
        model = self.models / pathlib.PurePosixPath(self.by_id["recognition"]["output"]).name
        model.write_bytes(b"tampered")
        with self.assertRaisesRegex(build_pack.PackBuildError, "hash"):
            self._build()
        self.assertFalse(self.output.exists())
        self.assertFalse(self.output.with_name(self.output.name + ".report.json").exists())

    def test_rejects_false_conversion_gate_and_cleans_stale_outputs(self):
        self._build()
        with self.assertRaisesRegex(build_pack.PackBuildError, "gate"):
            self._build(gate=False)
        self.assertFalse(self.output.exists())
        self.assertFalse(self.output.with_name(self.output.name + ".report.json").exists())

    def test_rejects_missing_sidecar_model_and_lost_calibration_status(self):
        sidecar = self.models / "emotion_rv1126b.json"
        sidecar.unlink()
        with self.assertRaisesRegex(build_pack.PackBuildError, "sidecar"):
            self._build()

        self._write_selected_artifacts()
        model = self.models / "emotion_rv1126b.rknn"
        model.unlink()
        with self.assertRaisesRegex(build_pack.PackBuildError, "model"):
            self._build()

        self._write_selected_artifacts()
        sidecar = self.models / "emotion_rv1126b.json"
        data = json.loads(sidecar.read_text(encoding="utf-8"))
        data["calibration_status"] = "provisional"
        sidecar.write_text(json.dumps(data), encoding="utf-8")
        with self.assertRaisesRegex(build_pack.PackBuildError, "calibration"):
            self._build()

    def test_rejects_duplicate_tar_names_and_unknown_detector_family(self):
        names = {model_id: "same" for model_id in self.selected}
        with mock.patch.object(build_pack, "_archive_names", return_value=names):
            with self.assertRaisesRegex(build_pack.PackBuildError, "unique"):
                self._build()
        with self.assertRaisesRegex(build_pack.PackBuildError, "detector family"):
            self._build(detector_family="unknown")

    def test_cli_returns_nonzero_when_build_gate_fails(self):
        with mock.patch.object(build_pack, "build_resource_pack", side_effect=build_pack.PackBuildError("gate failed")):
            self.assertEqual(build_pack.main(["--inventory", str(INVENTORY), "--artifact-root", str(self.models),
                                              "--evidence-root", str(self.evidence), "--output", str(self.output)]), 1)


if __name__ == "__main__":
    unittest.main()
