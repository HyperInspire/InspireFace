import cv2
from inspire_face.base import CameraStream, create_engine, InspireFaceCustomParameter
from inspire_face.tracker_module import FaceTrackerModule
from inspire_face.recognition_module import FaceRecognitionModule

if __name__ == '__main__':

    face_image_1 = cv2.imread("/Users/tunm/work/HyperFace/test_res/images/kun.jpg")
    stream1 = CameraStream(face_image_1)
    face_image_2 = cv2.imread("/Users/tunm/work/HyperFace/test_res/images/face_comp.jpeg")
    stream2 = CameraStream(face_image_2)

    param = InspireFaceCustomParameter()
    param.enable_recognition = True
    engine = create_engine("/Users/tunm/work/HyperFace/test_res/model_zip/Optimus-t1", param=param)
    tracker = FaceTrackerModule(engine)
    recognition = FaceRecognitionModule(engine)

    # 对比
    faces_information1 = tracker.execute(stream1)

    v1 = None
    if len(faces_information1) > 0:
        item = faces_information1[0]
        recognition.extract_feature(stream1, item)
        v1 = item

    faces_information2 = tracker.execute(stream2)

    v2 = None
    if len(faces_information2) > 0:
        item = faces_information2[0]
        recognition.extract_feature(stream2, item)
        v2 = item

    res = recognition.face_comparison1v1(v1, v2)
    print(res)
