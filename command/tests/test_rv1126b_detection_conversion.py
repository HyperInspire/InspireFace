"""Landmark/SCRFD conversion contracts, with the Linux Toolkit at the boundary."""

import importlib.util
import json
import sys
import types
import unittest
from unittest.mock import patch

import test_rv1126b_converter_contracts as helpers

FakeRKNN, INVENTORY, ROOT = helpers.FakeRKNN, helpers.INVENTORY, helpers.ROOT


class DetectionConversionTests(unittest.TestCase):
    # Reuse only the existing boundary fixtures, not its seven-family test cases.
    setUp = helpers.ConverterContracts.setUp
    module = helpers.ConverterContracts.module
    fixture = helpers.ConverterContracts.fixture
    session = helpers.ConverterContracts.session
    convert = helpers.ConverterContracts.convert

    def test_ten_scrfd_graphs_have_independent_dataset_and_ordered_outputs(self):
        ids = {f"scrfd_{family}_{size}" for family in ("500m", "2_5g")
               for size in (160, 192, 256, 320, 640)}
        self.assertEqual({r["id"] for r in INVENTORY if r["family"] == "scrfd"}, ids)
        names = ["score_8", "score_16", "score_32", "bbox_8", "bbox_16", "bbox_32",
                 "kps_8", "kps_16", "kps_32"]
        for model_id in sorted(ids):
            record, dataset, output = self.fixture(model_id)
            result = self.convert("scrfd", record, dataset, output)
            instance = FakeRKNN.instances[-1]
            self.assertEqual(instance.calls[0], ("config", {
                "mean_values": [[127.5, 127.5, 127.5]], "std_values": [[127.5, 127.5, 127.5]],
                "quant_img_RGB2BGR": True, "target_platform": "rv1126b"}))
            self.assertEqual(instance.calls[1], ("load_onnx", {
                "model": str(self.root / record["source"]), "outputs": names}))
            self.assertEqual(instance.calls[2], ("build", {"do_quantization": True, "dataset": str(dataset)}))
            self.assertEqual(result["calibration_status"], "provisional")
            size = int(model_id.rsplit("_", 1)[1])
            self.assertEqual(result["input"]["shape"], [1, size, size, 3])
            self.assertEqual([o["name"] for o in result["onnx_metadata"]["outputs"]], names)
            self.assertTrue(instance.released)
            self.assertEqual(result, json.loads(output.with_suffix(".json").read_text()))
            self.assertEqual(result["source_sha256"], record["source_sha256"])
            self.assertEqual(result["conversion_status"], "success")
            self.assertEqual(result["toolkit_version"], "2.3.2")
        self.assertEqual(len(FakeRKNN.instances), 10)
        self.assertEqual(len({i.calls[2][1]["dataset"] for i in FakeRKNN.instances}), 10)

    def test_landmark_keeps_verified_bgr_112_contract_and_dataset(self):
        record, dataset, output = self.fixture("landmark")
        self.assertEqual(record["calibration"]["path"], "landmark/quant_v2_dataset.txt")
        result = self.convert("landmark", record, dataset, output)
        instance = FakeRKNN.instances[-1]
        self.assertEqual(instance.calls[0], ("config", {
            "mean_values": [[0, 0, 0]], "std_values": [[255, 255, 255]],
            "quant_img_RGB2BGR": True, "target_platform": "rv1126b"}))
        self.assertEqual(instance.calls[2][1]["dataset"], str(dataset))
        self.assertEqual(result["input"]["shape"], [1, 112, 112, 3])
        self.assertEqual(result["outputs"][0]["shape"], [1, 212])
        self.assertEqual(result["calibration_status"], "verified")
        self.assertTrue(instance.released)
        from rv1126b_models.common import sha256
        for field, path in (("source_sha256", self.root / record["source"]),
                            ("dataset_sha256", dataset), ("output_sha256", output),
                            ("calibration_manifest_sha256", dataset.with_name("calibration.json"))):
            self.assertEqual(result[field], sha256(path))

    def test_unknown_scrfd_ids_and_wrong_contracts_fail_before_rknn(self):
        for change in ("id", "dimensions", "order", "semantic", "output_shape"):
            record, dataset, output = self.fixture("scrfd_500m_160")
            if change == "id": record["id"] = "scrfd_unknown_160"
            if change == "dimensions": record["input"]["shape"] = [1, 192, 192, 3]
            if change == "order": record["outputs"].reverse()
            if change == "semantic": record["outputs"][0]["semantic"] = "bbox_stride_8"
            if change == "output_shape": record["outputs"][0]["shape"] = [1, 200, 1]
            with self.subTest(change=change), self.assertRaises(ValueError):
                self.convert("scrfd", record, dataset, output)
        self.assertEqual(FakeRKNN.instances, [])

    def test_landmark_rejects_another_dataset_and_changed_preprocessing(self):
        for change in ("dataset", "color", "size"):
            record, dataset, output = self.fixture("landmark")
            if change == "dataset": record["calibration"]["path"] = "face_emotion/datasets.txt"
            if change == "color": record["preprocess"]["color_order"] = "RGB"
            if change == "size": record["input"]["shape"] = [1, 96, 96, 3]
            with self.subTest(change=change), self.assertRaises(ValueError):
                self.convert("landmark", record, dataset, output)
        self.assertEqual(FakeRKNN.instances, [])

    def test_unsupported_runtime_dtype_and_landmark_resize_are_rejected(self):
        for family, model_id, change in (("landmark", "landmark", "dtype"),
                                         ("scrfd", "scrfd_500m_160", "dtype"),
                                         ("landmark", "landmark", "resize")):
            record, dataset, output = self.fixture(model_id)
            if change == "dtype": record["input"]["dtype"] = "int8"
            if change == "resize": record["preprocess"]["resize"]["width"] = 96
            with self.subTest(family=family, change=change), self.assertRaises(ValueError):
                self.convert(family, record, dataset, output)

    def test_failed_calls_release_and_record_failure_for_both_converters(self):
        for family, model_id in (("landmark", "landmark"), ("scrfd", "scrfd_2_5g_640")):
            self.module(family)
            for stage in ("config", "load_onnx", "build", "export_rknn"):
                FakeRKNN.fail_stage = stage
                args = self.fixture(model_id)
                with self.subTest(family=family, stage=stage), self.assertRaisesRegex(RuntimeError, stage):
                    self.convert(family, *args)
                self.assertTrue(FakeRKNN.instances[-1].released)
                result = json.loads(args[-1].with_suffix(".json").read_text())
                self.assertEqual(result["conversion_status"], "failed")
                self.assertIsNone(result["output_sha256"])

    def test_scrfd_cli_selects_exactly_one_record(self):
        module = self.module("scrfd")
        _, dataset, _ = self.fixture("scrfd_500m_192")
        args = ["convert_scrfd", "--model-id", "scrfd_500m_192", "--inventory",
                str(ROOT / "command/rv1126b_models/model_inventory.json"),
                "--source-root", str(self.root), "--artifacts", str(self.root / "cli")]
        with patch.object(sys, "argv", args), patch.object(module, "prepare_calibration", return_value=dataset) as prepare, \
                patch.object(module, "convert", return_value={"model_id": "scrfd_500m_192",
                    "conversion_status": "success", "calibration_status": "provisional"}) as convert:
            module.main()
        self.assertEqual(prepare.call_count, 1)
        self.assertEqual(convert.call_count, 1)
        self.assertEqual(convert.call_args.args[0]["id"], "scrfd_500m_192")
        self.assertEqual(convert.call_args.args[1], dataset)
        self.assertEqual(convert.call_args.args[2].name, "scrfd_500m_192_rv1126b.rknn")

    def test_legacy_landmark_preserves_comparison_artifacts_while_delegating(self):
        import cv2
        import numpy as np
        module_path = ROOT / "command/rv1126b_landmark/prepare_model.py"
        spec = importlib.util.spec_from_file_location("legacy_landmark", module_path)
        module = importlib.util.module_from_spec(spec)
        record, _, _ = self.fixture("landmark")
        source = self.root / "landmark"
        image = np.full((112, 112, 3), [10, 20, 30], dtype=np.uint8)
        cv2.imwrite(str(source / "crop.png"), image)
        (source / "quant_v2_dataset.txt").write_text("crop.png\n")
        reference = np.arange(212, dtype=np.float32)[None, :]
        feeds = []
        session = self.session(record)
        session.run = lambda _, feed: (feeds.append(feed) or [reference])
        with patch.dict(sys.modules, {"onnxruntime": types.SimpleNamespace(InferenceSession=lambda *a, **k: session)}):
            spec.loader.exec_module(module)
            self.assertTrue(hasattr(module, "convert_landmark"), "legacy script must delegate family conversion")
            output = self.root / "legacy"
            with patch.object(sys, "argv", ["prepare_model", str(source), str(output)]), \
                    patch.object(module.convert_landmark, "convert", return_value={}) as convert:
                module.main()
        self.assertEqual(convert.call_count, 1)
        self.assertEqual(convert.call_args.args[0]["id"], "landmark")
        self.assertEqual(convert.call_args.args[1].parent.name, "landmark")
        self.assertEqual(convert.call_args.args[2], output / "landmark_rv1126b.rknn")
        self.assertEqual((output / "input_bgr_u8.bin").read_bytes(), image.tobytes())
        self.assertEqual((output / "onnx_output_f32.bin").read_bytes(), reference.tobytes())
        self.assertTrue((output / "input.png").is_file())
        self.assertEqual((output / "quant_dataset_absolute.txt").read_text().splitlines(), [str(source / "crop.png")])
        np.testing.assert_array_equal(next(iter(feeds[0].values())), image.astype(np.float32).transpose(2, 0, 1)[None] / 255)


if __name__ == "__main__":
    unittest.main()
