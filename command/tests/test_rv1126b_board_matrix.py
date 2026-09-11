"""Real-board acceptance tests; missing board evidence is a failure, never a skip."""
import hashlib
import json
import math
import os
from pathlib import Path
import re
import struct
import unittest

ROOT = Path(__file__).resolve().parents[2]
ARTIFACTS = Path(os.environ.get("RV1126B_ARTIFACTS", ROOT / "build/rv1126b-models"))


class BoardMatrixTests(unittest.TestCase):
    def test_all_converted_models_have_successful_finite_board_outputs(self):
        records = [json.loads(p.read_text()) for p in (ARTIFACTS / "models").glob("*.json")]
        expected = {r["model_id"] for r in records}
        self.assertEqual(len(expected), 18, "need all converted artifacts")
        artifacts = list((ARTIFACTS / "models").glob("*.rknn"))
        self.assertEqual({p.stem for p in artifacts}, {model_id + "_rv1126b" for model_id in expected})
        for record in records:
            model = ARTIFACTS / "models" / (record["model_id"] + "_rv1126b.rknn")
            self.assertEqual(hashlib.sha256(model.read_bytes()).hexdigest(), record["output_sha256"])
        result_path = ARTIFACTS / "board/results.json"
        self.assertTrue(result_path.is_file(), "real board results are required")
        results = json.loads(result_path.read_text(encoding="utf-8-sig"))
        self.assertEqual(len(results), len(expected))
        self.assertEqual({r["model_id"] for r in results}, expected)
        by_id = {r["model_id"]: r for r in records}
        for result in results:
            with self.subTest(model=result["model_id"]):
                self.assertEqual(result["status"], "success", result.get("error"))
                self.assertEqual(result["model_sha256"], by_id[result["model_id"]]["output_sha256"])
                self.assertRegex(result["api_version"], r"^2\.3\.2(?:\D|$)")
                self.assertEqual(result["driver_version"], "0.9.8")
                self.assertEqual(len(result["latency_ms"]), 10)
                self.assertTrue(all(math.isfinite(t) and t > 0 for t in result["latency_ms"]))
                self.assertTrue(result["inputs"])
                self.assertTrue(result["outputs"])
                for tensor in result["inputs"] + result["outputs"]:
                    for field in ("dtype", "layout", "dims", "n_elems", "size", "qnt_type", "zp", "scale", "fl"):
                        self.assertIn(field, tensor)
                    self.assertGreater(tensor["n_elems"], 0)
                for output in result["outputs"]:
                    data = (ARTIFACTS / "board" / result["model_id"] / output["file"]).read_bytes()
                    self.assertEqual(len(data), output["n_elems"] * 4)
                    self.assertEqual(hashlib.sha256(data).hexdigest(), output["sha256"])
                    self.assertTrue(output["finite"])
                    self.assertTrue(all(math.isfinite(x[0]) for x in struct.iter_unpack("<f", data)))

    def test_runner_uses_queried_counts_and_tensor_contracts(self):
        source = ROOT / "command/rv1126b_models/rknn_contract_runner.cpp"
        self.assertTrue(source.is_file(), "generic runner must exist")
        code = source.read_text()
        self.assertNotIn("212", code)
        self.assertNotRegex(code, r"n_(?:input|output)\s*!=\s*1\b")
        for count in ("n_input", "n_output"):
            self.assertRegex(code, r"for\s*\([^;]+;[^;]+<\s*count\." + count)
        for query in ("RKNN_QUERY_INPUT_ATTR", "RKNN_QUERY_OUTPUT_ATTR", "RKNN_QUERY_SDK_VERSION"):
            self.assertIn(query, code)


if __name__ == "__main__":
    unittest.main()
