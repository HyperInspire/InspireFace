"""Contract tests for immutable RV1126B model conversion records."""

import copy
import json
import pathlib
import sys
import tempfile
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]
MODEL_DIR = ROOT / "command" / "rv1126b_models"
INVENTORY = MODEL_DIR / "model_inventory.json"
sys.path.insert(0, str(MODEL_DIR))

from records import load_inventory, validate_record  # noqa: E402


class ModelRecordTests(unittest.TestCase):
    def test_inventory_contains_all_eighteen_onnx_models(self):
        records = load_inventory(INVENTORY)
        self.assertEqual(len(records), 18)
        self.assertEqual(len({item["id"] for item in records}), 18)

    def test_each_record_has_model_specific_conversion_fields(self):
        required = {
            "id",
            "family",
            "source",
            "source_sha256",
            "output",
            "target",
            "toolkit_version",
            "input",
            "outputs",
            "preprocess",
            "calibration",
            "calibration_status",
        }
        for record in load_inventory(INVENTORY):
            self.assertTrue(required.issubset(record))
            validate_record(record)

    def test_validation_rejects_missing_model_specific_preprocessing(self):
        record = copy.deepcopy(load_inventory(INVENTORY)[0])
        del record["preprocess"]["mean"]
        with self.assertRaisesRegex(ValueError, "preprocess.mean"):
            validate_record(record)

    def test_validation_rejects_invalid_target_and_status(self):
        record = copy.deepcopy(load_inventory(INVENTORY)[0])
        record["target"] = "rk3588"
        with self.assertRaisesRegex(ValueError, "target"):
            validate_record(record)

        record = copy.deepcopy(load_inventory(INVENTORY)[0])
        record["calibration_status"] = "assumed"
        with self.assertRaisesRegex(ValueError, "calibration_status"):
            validate_record(record)

    def test_validation_requires_pinned_toolkit_version(self):
        record = copy.deepcopy(load_inventory(INVENTORY)[0])
        del record["toolkit_version"]
        with self.assertRaisesRegex(ValueError, "toolkit_version"):
            validate_record(record)

        record = copy.deepcopy(load_inventory(INVENTORY)[0])
        record["toolkit_version"] = "2.3.1"
        with self.assertRaisesRegex(ValueError, "toolkit_version"):
            validate_record(record)

    def test_validation_rejects_windows_absolute_and_drive_qualified_paths(self):
        for invalid_path in (
            "/models/model.onnx",
            "D:/models/model.onnx",
            "D:models/model.onnx",
            "\\\\server\\share\\model.onnx",
        ):
            record = copy.deepcopy(load_inventory(INVENTORY)[0])
            record["source"] = invalid_path
            with self.assertRaisesRegex(ValueError, "source"):
                validate_record(record)

    def test_load_inventory_requires_exact_model_count_and_stable_scrfd_ids(self):
        records = load_inventory(INVENTORY)
        with tempfile.TemporaryDirectory() as directory:
            invalid_inventory = pathlib.Path(directory) / "inventory.json"
            invalid_inventory.write_text(json.dumps(records[:-1]), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "exactly 18"):
                load_inventory(invalid_inventory)

            malformed = copy.deepcopy(records)
            malformed[-1]["id"] = "scrfd_2_5g_unstable"
            invalid_inventory.write_text(json.dumps(malformed), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "SCRFD"):
                load_inventory(invalid_inventory)

    def test_validation_enforces_inventory_calibration_policy_by_family(self):
        expected_statuses = {
            "attitude": "verified",
            "emotion": "verified",
            "landmark": "verified",
            "liveness": "provisional",
            "mask": "provisional",
            "quality": "provisional",
            "recognition": "provisional",
            "rnet": "provisional",
            "scrfd": "provisional",
        }
        for record in load_inventory(INVENTORY):
            self.assertEqual(record["calibration_status"], expected_statuses[record["family"]])

        for record_id, invalid_status in (("attitude", "provisional"), ("mask", "verified")):
            record = next(item for item in load_inventory(INVENTORY) if item["id"] == record_id)
            record = copy.deepcopy(record)
            record["calibration_status"] = invalid_status
            with self.assertRaisesRegex(ValueError, "calibration_status"):
                validate_record(record)

    def test_validation_rejects_incomplete_tensor_contracts(self):
        record = copy.deepcopy(load_inventory(INVENTORY)[0])
        del record["input"]["layout"]
        with self.assertRaisesRegex(ValueError, "input.layout"):
            validate_record(record)

        record = copy.deepcopy(load_inventory(INVENTORY)[0])
        record["outputs"] = []
        with self.assertRaisesRegex(ValueError, "outputs"):
            validate_record(record)


if __name__ == "__main__":
    unittest.main(verbosity=2)
