"""Tests for per-model RV1126B calibration material preparation."""

from __future__ import annotations

import copy
import hashlib
import json
import pathlib
import sys
import tempfile
import types
import unittest
from unittest.mock import patch

import cv2
import numpy as np


ROOT = pathlib.Path(__file__).resolve().parents[2]
MODEL_DIR = ROOT / "command" / "rv1126b_models"
sys.path.insert(0, str(MODEL_DIR))

from calibration import prepare_calibration  # noqa: E402
from records import load_inventory  # noqa: E402


class CalibrationPreparationTests(unittest.TestCase):
    def test_provisional_read_and_write_failures_are_reported(self):
        for failure in ("read", "write"):
            with self.subTest(failure=failure), tempfile.TemporaryDirectory() as directory:
                root = pathlib.Path(directory)
                source = root / "source"
                self._write_image(source / "image.png", (20, 30, 40))
                fake_cv2 = types.SimpleNamespace(
                    IMREAD_COLOR=1, INTER_LINEAR=1,
                    imread=lambda *a: None if failure == "read" else object(),
                    resize=lambda *a, **k: object(),
                    imwrite=lambda *a: False,
                )
                with patch.dict(sys.modules, {"cv2": fake_cv2}):
                    with self.assertRaisesRegex(ValueError, failure):
                        prepare_calibration(self._record("mask"), source, root / "artifacts")

    @staticmethod
    def _write_image(path: pathlib.Path, color: tuple[int, int, int]) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        image = np.empty((10, 14, 3), dtype=np.uint8)
        image[:] = color[::-1]
        if not cv2.imwrite(str(path), image):
            raise ValueError(f"cannot write test fixture: {path}")

    @staticmethod
    def _checksum(path: pathlib.Path) -> str:
        return hashlib.sha256(path.read_bytes()).hexdigest()

    def _record(self, model_id: str) -> dict:
        return copy.deepcopy(next(record for record in load_inventory(MODEL_DIR / "model_inventory.json")
                                  if record["id"] == model_id))

    def test_verified_list_names_missing_image(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            source = root / "source"
            self._write_image(source / "lists" / "present.png", (20, 30, 40))
            dataset = source / "lists" / "verified.txt"
            dataset.parent.mkdir(parents=True, exist_ok=True)
            dataset.write_text("present.png\nmissing.png\n", encoding="utf-8")
            record = self._record("attitude")
            record["calibration"] = {"path": "lists/verified.txt", "reason": "test"}

            with self.assertRaisesRegex(FileNotFoundError, "missing.png"):
                prepare_calibration(record, source, root / "artifacts")

    def test_provisional_models_have_distinct_model_specific_dataset_paths(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            source = root / "source"
            self._write_image(source / "faces" / "face.png", (20, 30, 40))
            artifact_root = root / "artifacts"

            mask_dataset = prepare_calibration(self._record("mask"), source, artifact_root)
            recognition_dataset = prepare_calibration(self._record("recognition"), source, artifact_root)

            self.assertIn("mask", mask_dataset.as_posix())
            self.assertIn("recognition", recognition_dataset.as_posix())
            self.assertNotEqual(mask_dataset, recognition_dataset)
            self.assertEqual(cv2.imread(mask_dataset.read_text().strip()).shape, (96, 96, 3))
            self.assertEqual(cv2.imread(recognition_dataset.read_text().strip()).shape, (112, 112, 3))

    def test_provisional_models_dispatch_their_declared_spatial_operations(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            source = root / "source"
            self._write_image(source / "faces" / "face.png", (20, 30, 40))

            expected_operations = {
                "mask": "caller_supplied_face_crop_resize",
                "recognition": "aligned_face_crop_resize",
                "rnet": "candidate_face_crop_resize",
                "scrfd_500m_160": "full_frame_resize",
            }
            for model_id, expected_operation in expected_operations.items():
                dataset = prepare_calibration(self._record(model_id), source, root / "artifacts")
                manifest = json.loads((dataset.parent / "calibration.json").read_text(encoding="utf-8"))
                self.assertEqual(manifest["spatial_operation"], expected_operation)

    def test_unknown_provisional_spatial_preparation_fails(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            source = root / "source"
            self._write_image(source / "faces" / "face.png", (20, 30, 40))
            record = self._record("mask")
            record["preprocess"]["crop"] = "unrecognized crop contract"

            with self.assertRaisesRegex(ValueError, "unknown spatial preparation"):
                prepare_calibration(record, source, root / "artifacts")

    def test_empty_verified_list_fails(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            source = root / "source"
            empty_list = source / "lists" / "verified.txt"
            empty_list.parent.mkdir(parents=True, exist_ok=True)
            empty_list.write_text("\n", encoding="utf-8")
            record = self._record("emotion")
            record["calibration"] = {"path": "lists/verified.txt", "reason": "test"}

            with self.assertRaisesRegex(ValueError, "empty"):
                prepare_calibration(record, source, root / "artifacts")

    def test_provisional_manifest_is_deterministic_and_preserves_source(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            source = root / "source"
            first = source / "faces" / "z.png"
            second = source / "faces" / "nested" / "a.png"
            self._write_image(first, (10, 20, 30))
            self._write_image(second, (40, 50, 60))
            source_checksums = {path: self._checksum(path) for path in (first, second)}
            record = self._record("quality")

            artifact_root = root / "artifacts"
            first_dataset = prepare_calibration(record, source, artifact_root)
            first_dataset_text = first_dataset.read_text(encoding="utf-8")
            first_manifest_text = (first_dataset.parent / "calibration.json").read_text(encoding="utf-8")
            second_dataset = prepare_calibration(record, source, artifact_root)

            first_manifest = json.loads((first_dataset.parent / "calibration.json").read_text(encoding="utf-8"))
            second_manifest = json.loads((second_dataset.parent / "calibration.json").read_text(encoding="utf-8"))
            self.assertEqual(first_dataset_text, second_dataset.read_text(encoding="utf-8"))
            self.assertEqual(first_manifest_text, (second_dataset.parent / "calibration.json").read_text(encoding="utf-8"))
            self.assertEqual(first_manifest, second_manifest)
            self.assertEqual(first_manifest["status"], "provisional")
            self.assertEqual(first_manifest["model_id"], "quality")
            self.assertEqual(first_manifest["original_missing_list"], "face_quality/data.txt")
            self.assertEqual(first_manifest["input_dimensions"], {"width": 96, "height": 96})
            self.assertIn("source_checksums", first_manifest)
            self.assertIn("generated_checksums", first_manifest)
            self.assertIn("spatial_operation", first_manifest)
            self.assertEqual(source_checksums, {path: self._checksum(path) for path in (first, second)})

    def test_dry_preparation_covers_the_full_inventory_with_expected_status_counts(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            source = root / "source"
            self._write_image(source / "shared" / "face.png", (1, 2, 3))
            self._write_image(source / "shared" / "face-two.png", (4, 5, 6))
            records = load_inventory(MODEL_DIR / "model_inventory.json")
            for record in records:
                if record["calibration_status"] == "verified":
                    dataset = source / record["calibration"]["path"]
                    dataset.parent.mkdir(parents=True, exist_ok=True)
                    dataset.write_text("../shared/face.png\n", encoding="utf-8")

            datasets = [prepare_calibration(record, source, root / "artifacts") for record in records]
            manifests = [path.parent / "calibration.json" for path in datasets]
            statuses = [json.loads(path.read_text(encoding="utf-8"))["status"]
                        for path in manifests if path.exists()]

            self.assertEqual(len(datasets), 18)
            self.assertEqual(statuses.count("verified"), 3)
            self.assertEqual(statuses.count("provisional"), 15)
            self.assertTrue(all(path.is_absolute() for dataset in datasets
                                for path in map(pathlib.Path, dataset.read_text(encoding="utf-8").splitlines())))


if __name__ == "__main__":
    unittest.main(verbosity=2)
