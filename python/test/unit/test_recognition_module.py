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
        pass

    def test_face_feature_extraction(self):
        track_mode = isf.DETECT_MODE_IMAGE
        param = isf.EngineCustomParameter()
        param.enable_recognition = True
        engine = isf.create_engine(bundle_file=TEST_MODEL_PATH, param=param,
                                        detect_mode=track_mode)
        self.assertEqual(True, engine.check(), "Failed to create engine.")
        recognition = isf.FaceRecognitionModule(engine)


if __name__ == '__main__':
    unittest.main()
