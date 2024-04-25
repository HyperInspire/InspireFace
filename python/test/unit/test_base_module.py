# from test import *
# import unittest
# import inspireface as isf
# import cv2
#
#
# class EngineCase(unittest.TestCase):
#
#     def setUp(self) -> None:
#         """Shared area for priority execution"""
#         pass
#
#     def test_create_engine(self):
#         option_param = isf.SessionCustomParameter()
#         max_num_of_tracked_faces = 3
#         engine = isf.create_engine(bundle_file=TEST_MODEL_PATH,
#                                    param=option_param,
#                                    db_configuration=None,
#                                    detect_mode=isf.DETECT_MODE_IMAGE,
#                                    max_detect_num=max_num_of_tracked_faces)
#         self.assertEqual(True, engine.check())
#
#     def test_release_engine(self):
#         option_param = isf.SessionCustomParameter()
#         max_num_of_tracked_faces = 3
#         engine = isf.create_engine(bundle_file=TEST_MODEL_PATH,
#                                    param=option_param,
#                                    db_configuration=None,
#                                    detect_mode=isf.DETECT_MODE_IMAGE,
#                                    max_detect_num=max_num_of_tracked_faces)
#         engine.release()
#         self.assertEqual(False, engine.check())
#
#
# class CameraStreamCase(unittest.TestCase):
#     def setUp(self) -> None:
#         """Shared area for priority execution"""
#         pass
#
#     def test_image_codec(self) -> None:
#         image = cv2.imread(get_test_data("bulk/kun.jpg"))
#         self.assertIsNotNone(image)
#
#     def test_stream_rotation(self) -> None:
#         # Prepare material
#         engine = isf.create_engine(bundle_file=TEST_MODEL_PATH, param=isf.SessionCustomParameter(), )
#         self.assertEqual(True, engine.check(), "Failed to create engine.")
#         # Use tracker module
#         tracker = isf.FaceTrackerModule(engine)
#         self.assertIsNotNone(tracker)
#         # Prepare rotation images
#         rotation_images_filenames = ["rotate/rot_0.jpg", "rotate/rot_90.jpg", "rotate/rot_180.jpg","rotate/rot_270.jpg"]
#         rotation_images = [cv2.imread(get_test_data(path)) for path in rotation_images_filenames]
#         self.assertEqual(True, all(isinstance(item, np.ndarray) for item in rotation_images))
#
#         # Detecting face images without rotation
#         rot_0 = rotation_images[0]
#         h, w, _ = rot_0.shape
#         self.assertIsNotNone(rot_0, "Image is empty")
#         rot_0_faces = tracker.execute(image=rot_0)
#         self.assertEqual(True, len(rot_0_faces) > 0)
#         rot_0_face_box = (rot_0_faces[0].top_left, rot_0_faces[0].bottom_right)
#         num_of_faces = len(rot_0_faces)
#
#         # Detect images with other rotation angles
#         rotation_tags = [isf.ROTATION_90, isf.ROTATION_180, isf.ROTATION_270]
#         streams = [isf.CameraStream.load_from_cv_image(img, rotation=rotation_tags[idx]) for idx, img in enumerate(rotation_images[1:])]
#         results = [tracker.execute(stream) for stream in streams]
#         # No matter how many degrees the image is rotated, the same number of faces should be detected
#         self.assertEqual(True, all(len(item) == num_of_faces for item in results))
#         # Select all the first face box
#         rot_other_faces_boxes = [(face[0].top_left, face[0].bottom_right) for face in results]
#         # We need to restore the rotated face box
#         restored_boxes = [restore_rotated_box(w, h, rot_other_faces_boxes[idx], rotation_tags[idx]) for idx, box in enumerate(rot_other_faces_boxes)]
#         # IoU is performed with the face box of the original image to calculate the overlap
#         iou_results = [calculate_overlap(box, rot_0_face_box) for box in restored_boxes]
#         # The face box position of all rotated images is detected to be consistent with that of the original image
#         self.assertEqual(all(0.95 < iou < 1.0 for iou in iou_results), True)
#
#
# if __name__ == '__main__':
#     unittest.main()
