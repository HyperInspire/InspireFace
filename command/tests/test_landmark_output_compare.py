import importlib.util
from pathlib import Path
import tempfile
import unittest

import numpy as np


MODULE_PATH = Path(__file__).resolve().parents[1] / "rv1126b_landmark" / "compare_outputs.py"


class LandmarkOutputCompareTest(unittest.TestCase):
    def test_reports_pixel_errors_for_106_normalized_points(self):
        spec = importlib.util.spec_from_file_location("compare_outputs", MODULE_PATH)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        reference = np.zeros(212, dtype=np.float32)
        actual = reference.copy()
        actual[0] = 1.0 / 112.0
        metrics = module.compare_landmarks(reference, actual, input_size=112)
        self.assertEqual(metrics["point_count"], 106)
        self.assertAlmostEqual(metrics["mean_absolute_error_px"], 1.0 / 212.0, places=6)
        self.assertAlmostEqual(metrics["max_point_error_px"], 1.0, places=6)

    def test_rejects_mismatched_output_shapes(self):
        spec = importlib.util.spec_from_file_location("compare_outputs", MODULE_PATH)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        with self.assertRaisesRegex(ValueError, "same number"):
            module.compare_landmarks(np.zeros(212), np.zeros(210), input_size=112)


if __name__ == "__main__":
    unittest.main()
