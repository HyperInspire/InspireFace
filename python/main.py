import cv2
from inspire_face.instantiation import CameraStream, InspireFaceEngine, InspireFaceCustomParameter


if __name__ == '__main__':

    # image = cv2.imread("/Users/tunm/work/HyperFace/test_res/images/kun.jpg")
    # stream = CameraStream(image)
    # stream.debug_show()

    param = InspireFaceCustomParameter()
    param.enable_recognition = True
    engine = InspireFaceEngine("/Users/tunm/work/HyperFace/test_res/model_zip/T1", param=param)

