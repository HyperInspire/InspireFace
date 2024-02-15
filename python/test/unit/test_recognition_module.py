import unittest
from test import *
import inspireface as isf
import cv2


class FaceRecognitionBaseCase(unittest.TestCase):
    """
    This case is mainly used to test the basic functions of face recognition.
    """

    def setUp(self) -> None:
        # Prepare material
        track_mode = isf.DETECT_MODE_IMAGE
        param = isf.EngineCustomParameter()
        param.enable_recognition = True
        self.engine = isf.create_engine(bundle_file=TEST_MODEL_PATH, param=param,
                                   detect_mode=track_mode)
        self.assertEqual(True, self.engine.check(), "Failed to create engine.")
        # Create a tracker
        self.tracker = isf.FaceTrackerModule(self.engine)
        # Create a recognition
        self.recognition = isf.FaceRecognitionModule(self.engine)

    def test_face_feature_extraction(self):
        self.tracker.set_track_mode(mode=isf.DETECT_MODE_IMAGE)
        # Prepare a image
        image = cv2.imread(get_test_data("bulk/kun.jpg"))
        self.assertIsNotNone(image)
        # Face detection
        faces = self.tracker.execute(image)
        # "kun.jpg" has only one face
        self.assertEqual(len(faces), 1)
        face = faces[0]
        box = (face.top_left, face.bottom_right)
        expect_box = ((98, 146), (233, 272))
        # Calculate the location of the detected box and the expected box
        iou = calculate_overlap(box, expect_box)
        self.assertAlmostEqual(iou, 1.0, places=3)
        self.assertIsNone(face.feature)

        # Extract feature
        self.recognition.extract_feature(image, face)
        self.assertIsNotNone(face.feature)
        self.assertEqual(TEST_MODEL_FACE_FEATURE_LENGTH, face.feature.size)


@optional(ENABLE_BENCHMARK_TEST, "All benchmark related tests have been closed.")
class FaceRecognitionFeatureExtractCase(unittest.TestCase):
    benchmark_results = list()
    loop = 1

    @classmethod
    def setUpClass(cls):
        cls.benchmark_results = []

    def setUp(self) -> None:
        # Prepare image
        image = cv2.imread(get_test_data("bulk/kun.jpg"))
        self.stream = isf.CameraStream.load_from_cv_image(image)
        self.assertIsNotNone(self.stream)
        # Prepare material
        track_mode = isf.DETECT_MODE_IMAGE
        param = isf.EngineCustomParameter()
        param.enable_recognition = True
        self.engine = isf.create_engine(bundle_file=TEST_MODEL_PATH, param=param,
                                   detect_mode=track_mode)
        self.assertEqual(True, self.engine.check(), "Failed to create engine.")
        # Use tracker module
        self.tracker = isf.FaceTrackerModule(self.engine)
        self.assertIsNotNone(self.tracker)
        # Prepare a face
        faces = self.tracker.execute(self.stream)
        # "kun.jpg" has only one face
        self.assertEqual(len(faces), 1)
        self.face = faces[0]
        box = (self.face.top_left, self.face.bottom_right)
        expect_box = ((98, 146), (233, 272))
        # Calculate the location of the detected box and the expected box
        iou = calculate_overlap(box, expect_box)
        self.assertAlmostEqual(iou, 1.0, places=3)
        self.assertIsNone(self.face.feature)

        # Create a recognition
        self.recognition = isf.FaceRecognitionModule(self.engine)

    @benchmark(test_name="Feature Extract", loop=1000)
    def test_benchmark_face_detect(self):
        self.tracker.set_track_mode(isf.DETECT_MODE_IMAGE)
        for _ in range(self.loop):
            self.recognition.extract_feature(self.stream, self.face)
            self.assertIsNotNone(self.face.feature)
            self.assertEqual(TEST_MODEL_FACE_FEATURE_LENGTH, self.face.feature.size)


    @classmethod
    def tearDownClass(cls):
        print_benchmark_table(cls.benchmark_results)


if __name__ == '__main__':
    unittest.main()
