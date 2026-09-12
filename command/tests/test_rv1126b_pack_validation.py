"""Tamper-resistance tests for RV1126B resource-pack validation."""

from __future__ import annotations

import copy
import hashlib
import io
import json
import pathlib
import tarfile
import tempfile
import unittest
from unittest import mock

from command.rv1126b_models.records import load_inventory
from command.rv1126b_pack import build_pack, validate_pack
from command.rv1126b_pack.contracts import selected_model_ids


ROOT = pathlib.Path(__file__).resolve().parents[2]
INVENTORY = ROOT / "command" / "rv1126b_models" / "model_inventory.json"


class RV1126BPackValidationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = pathlib.Path(self.temp.name)
        self.inventory = self.root / "inventory.json"
        self.inventory.write_bytes(INVENTORY.read_bytes())
        self.models = self.root / "models"
        self.evidence = self.root / "evidence"
        self.pack = self.root / "Gundam_RV1126B"
        self.report = self.pack.with_name(self.pack.name + ".report.json")
        self.records = load_inventory(self.inventory)
        self.selected = selected_model_ids()
        self.by_id = {record["id"]: record for record in self.records}
        self.models.mkdir()
        self.evidence.mkdir()
        self._write_models()
        self._build()

    def tearDown(self):
        self.temp.cleanup()

    def _write_models(self):
        for model_id in self.selected:
            record = self.by_id[model_id]
            model = self.models / pathlib.PurePosixPath(record["output"]).name
            model.write_bytes(f"rknn:{model_id}".encode("ascii"))
            model.with_suffix(".json").write_text(json.dumps({
                "model_id": model_id,
                "conversion_status": "success",
                "target": "rv1126b",
                "toolkit_version": "2.3.2",
                "output_sha256": hashlib.sha256(model.read_bytes()).hexdigest(),
                "calibration_status": record["calibration_status"],
                "source": record["source"],
                "source_sha256": record["source_sha256"],
                "input": record["input"],
                "outputs": record["outputs"],
                "preprocess": record["preprocess"],
            }), encoding="utf-8")

    def _conversion_report(self):
        models = []
        for record in self.records:
            model = self.models / pathlib.PurePosixPath(record["output"]).name
            digest = hashlib.sha256(model.read_bytes()).hexdigest() if model.is_file() else "0" * 64
            models.append({"id": record["id"], "failure_stage": None,
                           "model": {"sha256": digest, "record_sha256": digest},
                           "calibration": {"status": record["calibration_status"]}})
        return {"models": models}

    def _build(self):
        with mock.patch.object(build_pack, "build_report", return_value=self._conversion_report()), \
             mock.patch.object(build_pack, "can_start_pack_integration", return_value=True):
            build_pack.build_resource_pack(self.inventory, self.models, self.evidence, self.pack)

    def _report_json(self):
        return json.loads(self.report.read_text(encoding="utf-8"))

    def _write_report(self, report):
        self.report.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    def _rewrite_archive(self, changes=None, *, remove=(), extra=(), duplicate=None, special=None):
        changes = changes or {}
        remove = set(remove)
        members = []
        with tarfile.open(self.pack, "r") as archive:
            for member in archive:
                if member.name not in remove:
                    data = archive.extractfile(member).read()
                    members.append((member.name, changes.get(member.name, data), tarfile.REGTYPE))
        members.extend(extra)
        if duplicate is not None:
            members.append((duplicate, b"duplicate", tarfile.REGTYPE))
        if special is not None:
            members.append(special)
        with tarfile.open(self.pack, "w", format=tarfile.USTAR_FORMAT) as archive:
            for name, data, member_type in members:
                info = tarfile.TarInfo(name)
                info.type = member_type
                info.mode = 0o644
                if member_type == tarfile.REGTYPE:
                    info.size = len(data)
                    archive.addfile(info, io.BytesIO(data))
                else:
                    info.linkname = "target"
                    archive.addfile(info)

    def test_valid_pack_matches_report_and_manifest(self):
        result = validate_pack.validate_resource_pack(self.pack, self.report)
        self.assertTrue(result["valid"])
        self.assertEqual(result["model_count"], 11)
        self.assertEqual(result["pack_sha256"], self._report_json()["pack_sha256"])

    def test_tampered_member_fails_with_member_name(self):
        self._rewrite_archive({"feature": b"tampered"})
        with self.assertRaisesRegex(validate_pack.PackValidationError, "feature"):
            validate_pack.validate_resource_pack(self.pack, self.report)

    def test_rejects_traversal_duplicate_nonregular_and_unexpected_members(self):
        cases = (
            ("traversal", {"extra": [("../escape", b"x", tarfile.REGTYPE)]}, "escape"),
            ("absolute", {"extra": [("/escape", b"x", tarfile.REGTYPE)]}, "escape"),
            ("duplicate", {"duplicate": "feature"}, "duplicate"),
            ("directory", {"special": ("directory", b"", tarfile.DIRTYPE)}, "directory"),
            ("symlink", {"special": ("link", b"", tarfile.SYMTYPE)}, "link"),
            ("hardlink", {"special": ("hard", b"", tarfile.LNKTYPE)}, "hard"),
            ("extra", {"extra": [("unexpected", b"x", tarfile.REGTYPE)]}, "unexpected"),
        )
        for name, kwargs, message in cases:
            with self.subTest(name=name):
                self._build()
                self._rewrite_archive(**kwargs)
                with self.assertRaisesRegex(validate_pack.PackValidationError, message):
                    validate_pack.validate_resource_pack(self.pack, self.report)

    def test_rejects_missing_member_malformed_manifest_and_report_mismatches(self):
        self._rewrite_archive(remove=("feature",))
        with self.assertRaisesRegex(validate_pack.PackValidationError, "feature"):
            validate_pack.validate_resource_pack(self.pack, self.report)

        self._build()
        self._rewrite_archive({"__inspire__": b"tag: Broken\n"})
        with self.assertRaisesRegex(validate_pack.PackValidationError, "manifest"):
            validate_pack.validate_resource_pack(self.pack, self.report)

        self._build()
        report = self._report_json()
        report["pack_sha256"] = "0" * 64
        self._write_report(report)
        with self.assertRaisesRegex(validate_pack.PackValidationError, "pack hash"):
            validate_pack.validate_resource_pack(self.pack, self.report)

        self._build()
        report = self._report_json()
        next(member for member in report["members"] if member["member"] == "feature")["sha256"] = "0" * 64
        self._write_report(report)
        with self.assertRaisesRegex(validate_pack.PackValidationError, "feature"):
            validate_pack.validate_resource_pack(self.pack, self.report)

        self._build()
        report = self._report_json()
        del next(member for member in report["members"] if member["member"] == "feature")["calibration_status"]
        self._write_report(report)
        with self.assertRaisesRegex(validate_pack.PackValidationError, "calibration"):
            validate_pack.validate_resource_pack(self.pack, self.report)

    def test_cli_returns_nonzero_for_invalid_pack(self):
        self._rewrite_archive({"feature": b"tampered"})
        self.assertEqual(validate_pack.main([str(self.pack), str(self.report)]), 1)


if __name__ == "__main__":
    unittest.main()
