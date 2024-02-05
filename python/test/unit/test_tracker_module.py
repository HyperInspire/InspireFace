import time
import unittest
from test import *
import inspireface as isf
import cv2

benchmark_results = []


class FaceTrackerCase(unittest.TestCase):
    pass


@optional(ENABLE_BENCHMARK_TEST, "All benchmark related tests have been closed.")
class FaceTrackerBenchmarkCase(unittest.TestCase):
    benchmark_results = list()
    loop = 1

    @classmethod
    def setUpClass(cls):
        cls.benchmark_results = []

    def setUp(self) -> None:
        # Prepare rotation images
        self.image = cv2.imread(get_test_data("bulk/kun.jpg"))
        self.assertIsNotNone(self.image)

    @benchmark(test_name="Face track", loop=1000)
    def test_benchmark_face_track(self):
        # Prepare material
        track_mode = isf.DETECT_MODE_VIDEO  # Use video mode
        self.engine = isf.create_engine(bundle_file=TEST_MODEL_PATH, param=isf.EngineCustomParameter(),
                                        detect_mode=track_mode)
        self.assertEqual(True, self.engine.check(), "Failed to create engine.")
        # Use tracker module
        self.tracker = isf.FaceTrackerModule(self.engine)
        self.assertIsNotNone(self.tracker)
        for _ in range(self.loop):
            self.tracker.execute(self.image)

    @benchmark(test_name="Face detection", loop=1000)
    def test_benchmark_face_detect(self):
        # Prepare material
        track_mode = isf.DETECT_MODE_IMAGE  # Use image mode
        self.engine = isf.create_engine(bundle_file=TEST_MODEL_PATH, param=isf.EngineCustomParameter(),
                                        detect_mode=track_mode)
        self.assertEqual(True, self.engine.check(), "Failed to create engine.")
        # Use tracker module
        self.tracker = isf.FaceTrackerModule(self.engine)
        self.assertIsNotNone(self.tracker)
        for _ in range(self.loop):
            self.tracker.execute(self.image)

    @classmethod
    def tearDownClass(cls):
        print_benchmark_table(cls.benchmark_results)


if __name__ == '__main__':
    unittest.main()
