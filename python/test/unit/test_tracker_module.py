import time
import unittest
from test import *
import inspireface as isf
import cv2


@unittest.skipUnless(ENABLE_BENCHMARK_TEST, "All benchmark related tests have been closed.")
class TrackerBenchmarkCase(unittest.TestCase):
    def test_benchmark_tracker(self):
        # Prepare material
        track_mode = isf.DETECT_MODE_VIDEO  # Use video mode
        engine = isf.create_engine(bundle_file=TEST_MODEL_PATH, param=isf.EngineCustomParameter(), detect_mode=track_mode)
        self.assertEqual(True, engine.check(), "Failed to create engine.")
        # Use tracker module
        tracker = isf.FaceTrackerModule(engine)
        self.assertIsNotNone(tracker)
        # Prepare rotation images
        image = cv2.imread(get_test_data("bulk/kun.jpg"))
        self.assertIsNotNone(image)

        loop = 1000
        cost_total = time.time()
        for _ in range(loop):
            tracker.execute(image)
        cost_total = time.time() - cost_total
        print_benchmark_table([("Face track", loop, cost_total)])


if __name__ == '__main__':
    unittest.main()
