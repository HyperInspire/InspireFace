"""Contract tests for immutable RV1126B model conversion records."""

import copy
import pathlib
import sys
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
