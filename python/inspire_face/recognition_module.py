from .tracker_module import *
from .base import *
from .typedef import *


class FaceRecognitionModule(object):

    def __init__(self, engine: InspireFaceEngine):
        self.engine = engine
        if not self.engine.check():
            raise NotImplemented("The Engine is not instantiated")
        if not self.engine.param.enable_recognition:
            raise NotImplemented("The Engine's face recognition module is not enabled")

    def extract_feature_from_token(self, stream: CameraStream, token: HF_FaceBasicToken) -> np.ndarray:
        feature_length = HInt32()
        ret = HF_GetFeatureLength(self.engine.handle, byref(feature_length))
        if ret != 0:
            raise Exception("error")
        features = (ctypes.c_float * feature_length.value)()
        ret = HF_FaceFeatureExtractCpy(self.engine.handle, stream.handle, token, features)
        if ret != 0:
            raise Exception(f"Extract error: {ret}")
        features_np = np.ctypeslib.as_array(features).copy()

        return features_np

    def extract_feature(self, image, face_information: FaceInformation):
        if isinstance(image, np.ndarray):
            stream = CameraStream(image)
        elif isinstance(image, CameraStream):
            stream = image
        else:
            raise NotImplemented(f"The {type(image)} type is not supported")
        embedded = self.extract_feature_from_token(stream, face_information.token)
        face_information._feature = embedded

    def face_comparison1v1(self, face1: FaceInformation, face2: FaceInformation) -> float:
        if face1.feature is None or face2.feature is None:
            logger.error("The compared FaceInformation object has no face features loaded.")
            return -1
        faces = [face1, face2]
        feats = list()
        for face in faces:
            feature = HF_FaceFeature()
            data_ptr = face.feature.ctypes.data_as(HPFloat)
            feature.size = ctypes.c_int32(face.feature.size)
            feature.data = data_ptr
            feats.append(feature)

        comparison_result = HFloat()
        ret = HF_FaceComparison1v1(self.engine.handle, feats[0], feats[1], HPFloat(comparison_result))
        if ret != 0:
            raise Exception(f"Comparison error: {ret}")

        return comparison_result.value
