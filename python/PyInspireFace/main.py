import cv2
import inspireface as isf

if __name__ == '__main__':

    face_image_1 = cv2.imread("/Users/tunm/work/HyperFace/test_res/images/kun.jpg")
    stream1 = isf.CameraStream.load_from_cv_image(face_image_1)
    face_image_2 = cv2.imread("/Users/tunm/work/HyperFace/test_res/images/face_comp.jpeg")
    stream2 = isf.CameraStream.load_from_cv_image(face_image_2)

    param = isf.SessionCustomParameter()
    param.enable_recognition = True
    param.enable_liveness = True
    param.enable_mask_detect = True
    param.enable_face_quality = True

    db_config = isf.DatabaseConfiguration(enable_use_db=True, db_path="./")

    engine = isf.create_engine("/Users/tunm/work/HyperFace/test_res/pack/Pikachu-t1", param=param,
                           db_configuration=db_config)
    tracker = isf.FaceTrackerModule(engine)
    recognition = isf.FaceRecognitionModule(engine)
    guard = isf.FaceGuardModule(engine)

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

    guard.pipeline_to_information(stream1, faces_information1, isf.ENABLE_QUALITY|isf.ENABLE_MASK_DETECT|isf.ENABLE_LIVENESS)

    print(faces_information1[0].extend)