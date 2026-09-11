"""Contract tests for the RV1126B resource-pack manifest mapping."""

from __future__ import annotations

import copy
import pathlib
import unittest

from command.rv1126b_models.records import load_inventory
from command.rv1126b_pack.contracts import (
    PackContractError,
    build_manifest,
    selected_model_ids,
    validate_manifest,
)


ROOT = pathlib.Path(__file__).resolve().parents[2]
INVENTORY = ROOT / "command" / "rv1126b_models" / "model_inventory.json"
EXPECTED_IDS = (
    "scrfd_2_5g_160", "scrfd_2_5g_320", "scrfd_2_5g_640",
    "landmark", "rnet", "recognition", "liveness", "mask",
    "quality", "emotion", "attitude",
)
KEY_BY_ID = {
    "scrfd_2_5g_160": "face_detect_160",
    "scrfd_2_5g_320": "face_detect_320",
    "scrfd_2_5g_640": "face_detect_640",
    "landmark": "landmark",
    "rnet": "refine_net",
    "recognition": "feature",
    "liveness": "rgb_anti_spoofing",
    "mask": "mask_detect",
    "quality": "pose_quality",
    "emotion": "face_emotion",
    "attitude": "face_attribute",
}


class RV1126BPackContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.records = load_inventory(INVENTORY)
        cls.names = {model_id: KEY_BY_ID[model_id] for model_id in EXPECTED_IDS}

    def test_default_profile_selects_exactly_eleven_models(self):
        self.assertEqual(selected_model_ids(), EXPECTED_IDS)

    def test_manifest_uses_current_archive_keys_and_per_model_contracts(self):
        manifest = build_manifest(self.records, self.names)
        self.assertEqual(manifest["face_detect_pixel_list"], [160, 320, 640])
        self.assertEqual(manifest["face_detect_model_list"], ["face_detect_160", "face_detect_320", "face_detect_640"])
        self.assertEqual(manifest["landmark"]["infer_engine"], "RKNN")
        self.assertEqual(manifest["feature"]["input_size"], [112, 112])
        self.assertEqual(manifest["rgb_anti_spoofing"]["input_size"], [80, 80])
        self.assertEqual(manifest["face_attribute"]["outputs_layers"], ["547", "548", "549"])
        self.assertEqual(manifest["refine_net"]["input_layer"], "input_1:0")
        self.assertEqual(manifest["refine_net"]["outputs_layers"], ["conv5-1/BiasAdd:0", "conv5-2/BiasAdd:0"])
        self.assertTrue(manifest["feature"]["swap_color"])
        self.assertFalse(manifest["face_emotion"]["swap_color"])
        self.assertFalse(manifest["landmark"]["nchw"])
        self.assertEqual(manifest["feature"]["mean"], [0.5, 0.5, 0.5])
        self.assertEqual(manifest["feature"]["norm"], [0.5, 0.5, 0.5])
        self.assertEqual(manifest["rgb_anti_spoofing"]["norm"], [1 / 255, 1 / 255, 1 / 255])
        for model_id, archive_key in KEY_BY_ID.items():
            section = manifest[archive_key]
            self.assertEqual(section["fullname"], self.names[model_id])
            self.assertEqual(section["input_tensor_type"], "uint8")
            self.assertEqual(section["output_tensor_type"], "float32")
            self.assertEqual(section["model_type"], "RKNN")
            self.assertEqual(section["infer_device"], "RKNPU")
            self.assertEqual(section["infer_backend"], "RKNPU")
        validate_manifest(manifest)

    def test_rejects_ambiguous_or_incomplete_selected_contracts(self):
        missing = [record for record in self.records if record["id"] != "emotion"]
        with self.assertRaisesRegex(PackContractError, "selected IDs"):
            build_manifest(missing, self.names)

        duplicate = copy.deepcopy(self.records)
        duplicate.append(copy.deepcopy(next(record for record in self.records if record["id"] == "emotion")))
        with self.assertRaisesRegex(PackContractError, "duplicate"):
            build_manifest(duplicate, self.names)

        extra = copy.deepcopy(self.records)
        extra.append(copy.deepcopy(extra[0]))
        extra[-1]["id"] = "unknown"
        with self.assertRaisesRegex(PackContractError, "extra"):
            build_manifest(extra, self.names)

        duplicate_names = dict(self.names)
        duplicate_names["emotion"] = duplicate_names["attitude"]
        with self.assertRaisesRegex(PackContractError, "unique"):
            build_manifest(self.records, duplicate_names)

        unsupported = copy.deepcopy(self.records)
        next(record for record in unsupported if record["id"] == "emotion")["input"]["layout"] = "NHWC4"
        with self.assertRaisesRegex(PackContractError, "layout"):
            build_manifest(unsupported, self.names)


if __name__ == "__main__":
    unittest.main()
