import cv2
import numpy as np

import inspireface as isf

print(isf.__version__)

if __name__ == '__main__':
    param = isf.SessionCustomParameter()
    param.enable_recognition = True

    # db_config = isf.DatabaseConfiguration(enable_use_db=True, db_path="./")
    db_config = None

    engine = isf.create_engine("/Users/tunm/work/HyperFace/test_res/pack/Optimus-t1", param=param,
                           db_configuration=db_config

   )
    tracker = isf.FaceTrackerModule(engine)
    recognition = isf.FaceRecognitionModule(engine)

    face_image_1 = cv2.imread("/Users/tunm/work/HyperFace/test_res/images/kun.jpg")
    stream1 = isf.CameraStream.load_from_cv_image(face_image_1)

    face_image_2 = cv2.imread("/Users/tunm/work/HyperFace/test_res/images/kunkun.jpg")
    stream2 = isf.CameraStream.load_from_cv_image(face_image_2)

    faces_information1 = tracker.execute(stream1)
    v1 = None
    if len(faces_information1) > 0:
        item = faces_information1[0]
        recognition.extract_feature(stream1, item)
        v1 = item

        identify = isf.FaceIdentity(item, custom_id=1, tag="kun")
        recognition.face_register(identify)

    faces_information2 = tracker.execute(stream2)
    v2 = None
    if len(faces_information2) > 0:
        item = faces_information2[0]
        recognition.extract_feature(stream2, item)
        v2 = item
        print("search")
        searched = recognition.face_search(v2)
        print(searched.confidence)
        print(searched.similar_identity.tag)
        print(searched.similar_identity.feature)
        print(np.dot(v2.feature, searched.similar_identity.feature.T))

    face = recognition.get_face_identity(custom_id=1)
    print(face.feature)

    print(recognition.get_face_count())
    # recognition.view_table_in_terminal()