"""Conversion boundaries tested without requiring the Linux-only Toolkit."""

import copy
import hashlib
import importlib
import json
import pathlib
import sys
import tempfile
import types
import unittest
from unittest.mock import patch

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "command"))
FAMILIES = ("attitude", "emotion", "liveness", "mask", "quality", "recognition", "rnet")
INVENTORY = json.loads((ROOT / "command/rv1126b_models/model_inventory.json").read_text())


class FakeRKNN:
    instances = []
    fail_stage = None

    def __init__(self, **kwargs):
        self.calls = []
        self.released = False
        self.instances.append(self)

    def config(self, **kwargs):
        self.calls.append(("config", copy.deepcopy(kwargs)))
        return -1 if self.fail_stage == "config" else 0

    def load_onnx(self, **kwargs):
        self.calls.append(("load_onnx", kwargs))
        return -1 if self.fail_stage == "load_onnx" else 0

    def build(self, **kwargs):
        self.calls.append(("build", kwargs))
        return -1 if self.fail_stage == "build" else 0

    def export_rknn(self, path):
        self.calls.append(("export_rknn", path))
        if self.fail_stage == "export_rknn":
            return -1
        pathlib.Path(path).write_bytes(b"fake-rknn")
        return 0

    def release(self):
        self.released = True


class ConverterContracts(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = pathlib.Path(self.temp.name)
        FakeRKNN.instances = []
        FakeRKNN.fail_stage = None
        self.modules = patch.dict(sys.modules, {
            "rknn": types.ModuleType("rknn"),
            "rknn.api": types.SimpleNamespace(RKNN=FakeRKNN),
        })
        self.modules.start()
        self.addCleanup(self.modules.stop)

    def module(self, family):
        path = ROOT / "command/rv1126b_models" / f"convert_{family}.py"
        self.assertTrue(path.is_file(), f"missing independent converter: {family}")
        return importlib.import_module(f"rv1126b_models.convert_{family}")

    def fixture(self, family):
        record = copy.deepcopy(next(r for r in INVENTORY if r["id"] == family))
        source = self.root / record["source"]
        source.parent.mkdir(parents=True, exist_ok=True)
        source.write_bytes(b"source-onnx")
        record["source_sha256"] = hashlib.sha256(b"source-onnx").hexdigest()
        record["_source_root"] = str(self.root)
        directory = self.root / "calibration" / family
        directory.mkdir(parents=True, exist_ok=True)
        sample = directory / "image.png"
        sample.write_bytes(b"sample")
        dataset = directory / "dataset.txt"
        dataset.write_text(str(sample.resolve()) + "\n", encoding="utf-8")
        (directory / "calibration.json").write_text(json.dumps({
            "model_id": family, "status": record["calibration_status"],
        }))
        output = self.root / record["output"]
        return record, dataset, output

    def session(self, record, wrong_shape=False, wrong_dtype=False):
        inp = types.SimpleNamespace(name=record["input"]["name"],
            shape=[9, 3, 1, 1] if wrong_shape else record["input"]["onnx_shape"],
            type="tensor(uint8)" if wrong_dtype else "tensor(float)")
        outputs = [types.SimpleNamespace(name=o["name"], shape=o["shape"], type="tensor(float)")
                   for o in record["outputs"]]
        return types.SimpleNamespace(get_inputs=lambda: [inp], get_outputs=lambda: outputs)

    def convert(self, family, record, dataset, output, **metadata):
        module = self.module(family)
        with patch("importlib.metadata.version", return_value="2.3.2"), patch.dict(sys.modules, {
            "onnxruntime": types.SimpleNamespace(InferenceSession=lambda *a, **k: self.session(record, **metadata))
        }):
            return module.convert(record, dataset, output)

    def test_every_converter_owns_rknn_configuration(self):
        for family in FAMILIES:
            with self.subTest(family=family):
                self.module(family)
                source = (ROOT / "command/rv1126b_models" / f"convert_{family}.py").read_text()
                for required in ('RKNN(', 'rknn.config(', 'target_platform="rv1126b"'):
                    self.assertIn(required, source)
                self.assertNotIn("default_preprocess", source)
                self.assertNotIn("init_runtime", source)
        common = ROOT / "command/rv1126b_models/common.py"
        self.assertTrue(common.is_file())
        for forbidden in ("mean_values", "std_values", "quant_img_RGB2BGR", "quantized_dtype", "RKNN("):
            self.assertNotIn(forbidden, common.read_text())

    def test_each_record_controls_config_and_results(self):
        for family in FAMILIES:
            with self.subTest(family=family):
                record, dataset, output = self.fixture(family)
                result = self.convert(family, record, dataset, output)
                instance = FakeRKNN.instances[-1]
                self.assertEqual(instance.calls[0], ("config", {
                    "mean_values": [record["preprocess"]["mean"]],
                    "std_values": [record["preprocess"]["std"]],
                    "quant_img_RGB2BGR": record["preprocess"]["color_order"] == "BGR",
                    "target_platform": "rv1126b",
                }))
                self.assertEqual(instance.calls[1], ("load_onnx", {"model": str(self.root / record["source"])}))
                self.assertEqual(instance.calls[2], ("build", {"do_quantization": True, "dataset": str(dataset)}))
                self.assertTrue(instance.released)
                self.assertEqual(result, json.loads(output.with_suffix(".json").read_text()))
                self.assertEqual(result["calibration_status"], record["calibration_status"])
                self.assertEqual(result["source_sha256"], record["source_sha256"])
                self.assertEqual(result["dataset_sha256"], hashlib.sha256(dataset.read_bytes()).hexdigest())
                self.assertEqual(result["output_sha256"], hashlib.sha256(b"fake-rknn").hexdigest())
                self.assertEqual(result["conversion_status"], "success")
                self.assertEqual(result["toolkit_version"], "2.3.2")
                self.assertEqual(result["onnx_metadata"]["inputs"][0]["dtype"], "float32")
                self.assertEqual(result["input"]["dtype"], "uint8")

    def test_recognition_changes_do_not_leak(self):
        record, dataset, output = self.fixture("recognition")
        record["preprocess"].update(mean=[11, 12, 13], std=[2, 3, 4], color_order="BGR")
        self.convert("recognition", record, dataset, output)
        for family in ("mask", "liveness"):
            rec, data, out = self.fixture(family)
            self.convert(family, rec, data, out)
        self.assertEqual(FakeRKNN.instances[0].calls[0][1]["mean_values"], [[11, 12, 13]])
        self.assertEqual(FakeRKNN.instances[1].calls[0][1]["std_values"], [[255, 255, 255]])
        self.assertEqual(FakeRKNN.instances[2].calls[0][1]["std_values"], [[1, 1, 1]])

    def test_wrong_toolkit_rejected_before_creation(self):
        module = self.module("mask")
        args = self.fixture("mask")
        with patch("importlib.metadata.version", return_value="2.3.0"):
            with self.assertRaisesRegex(ValueError, "2.3.2"):
                module.convert(*args)
        self.assertEqual(FakeRKNN.instances, [])

    def test_source_and_metadata_mismatches_rejected(self):
        for change in ("hash", "shape", "dtype", "family", "dataset"):
            with self.subTest(change=change):
                record, dataset, output = self.fixture("mask")
                if change == "hash": record["source_sha256"] = "0" * 64
                if change == "family": record["family"] = "liveness"
                if change == "dataset": dataset.write_text("relative.png\n")
                with self.assertRaises(ValueError):
                    self.convert("mask", record, dataset, output,
                        wrong_shape=change == "shape", wrong_dtype=change == "dtype")
        self.assertEqual(FakeRKNN.instances, [])

    def test_failures_release_and_record_failure(self):
        for stage in ("config", "load_onnx", "build", "export_rknn"):
            with self.subTest(stage=stage):
                record, dataset, output = self.fixture("mask")
                FakeRKNN.fail_stage = stage
                with self.assertRaisesRegex(RuntimeError, stage):
                    self.convert("mask", record, dataset, output)
                self.assertTrue(FakeRKNN.instances[-1].released)
                self.assertEqual(json.loads(output.with_suffix(".json").read_text())["conversion_status"], "failed")


if __name__ == "__main__":
    unittest.main()
