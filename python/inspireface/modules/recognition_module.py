from .base import *
from .typedef import *


class FaceRecognitionModule(object):

    def __init__(self, engine: InspireFaceEngine, search_threshold=0.42):
        self.engine = engine
        if not self.engine.check():
            raise NotImplemented("The Engine is not instantiated")
        if not self.engine.param.enable_recognition:
            raise NotImplemented("The Engine's face recognition module is not enabled")
        self.search_threshold = search_threshold
        self.set_search_threshold(self.search_threshold)

    def set_search_threshold(self, search_threshold: float):
        self.search_threshold = search_threshold
        ret = HF_FaceRecognitionThresholdSetting(self.engine.handle, search_threshold)
        if ret != 0:
            raise Exception(f"Threshold setting error: {ret}")

    def get_current_search_threshold(self) -> float:
        return self.search_threshold

    def extract_feature_from_token(self, stream: CameraStream, token: HF_FaceBasicToken) -> np.ndarray:
        feature_length = HInt32()
        ret = HF_GetFeatureLength(self.engine.handle, byref(feature_length))
        if ret != 0:
            raise Exception("error")
        feature = (HFloat * feature_length.value)()
        ret = HF_FaceFeatureExtractCpy(self.engine.handle, stream.handle, token, feature)
        if ret != 0:
            raise Exception(f"Extract error: {ret}")
        features_np = np.ctypeslib.as_array(feature).copy()

        return features_np

    def extract_feature(self, image, face_information: FaceInformation):
        if isinstance(image, np.ndarray):
            stream = CameraStream.load_from_cv_image(image)
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
            feature.size = HInt32(face.feature.size)
            feature.data = data_ptr
            feats.append(feature)

        comparison_result = HFloat()
        ret = HF_FaceComparison1v1(self.engine.handle, feats[0], feats[1], HPFloat(comparison_result))
        if ret != 0:
            raise Exception(f"Comparison error: {ret}")

        return comparison_result.value

    def face_register(self, face_identity: FaceIdentity):
        ret = HF_FeaturesGroupInsertFeature(self.engine.handle, face_identity.to_ctypes())
        if ret != 0:
            raise Exception(f"Failed to register face: {ret}")

    def face_search(self, data) -> SearchResult:
        if isinstance(data, np.ndarray):
            vector = data
        elif isinstance(data, FaceInformation):
            vector = data.feature
        else:
            raise Exception(f"This type of input is not supported: {type(data)}")
        feature = HF_FaceFeature(size=HInt32(vector.size), data=vector.ctypes.data_as(HPFloat))
        confidence = HFloat()
        most_similar = HF_FaceFeatureIdentity()
        ret = HF_FeaturesGroupFeatureSearch(self.engine.handle, feature, HPFloat(confidence),
                                            Ptr_HF_FaceFeatureIdentity(most_similar))
        if ret != 0:
            raise Exception(f"Failed to search face: {ret}")
        if most_similar.customId != -1:
            search_identity = FaceIdentity.from_ctypes(most_similar)
            search_result = SearchResult(confidence=confidence.value, similar_identity=search_identity, )
        else:
            none = FaceIdentity(np.zeros(0), most_similar.customId, "None")
            search_result = SearchResult(confidence=confidence.value, similar_identity=none, )

        return search_result

    def face_remove(self, custom_id: int):
        ret = HF_FeaturesGroupFeatureRemove(self.engine.handle, custom_id)
        if ret != 0:
            raise Exception(f"Failed to remove face: {ret}")

    def face_update(self, face_identity: FaceIdentity):
        ret = HF_FeaturesGroupFeatureUpdate(self.engine.handle, face_identity.to_ctypes())
        if ret != 0:
            raise Exception(f"Failed to update face: {ret}")

    def get_face_identity(self, custom_id: int) -> FaceIdentity:
        identify = HF_FaceFeatureIdentity()
        ret = HF_FeaturesGroupGetFeatureIdentity(self.engine.handle, custom_id, Ptr_HF_FaceFeatureIdentity(identify))
        if ret != 0:
            raise Exception(f"Failed to get face: {ret}")

        return FaceIdentity.from_ctypes(identify)

    def get_face_count(self) -> int:
        count = HInt32()
        ret = HF_FeatureGroupGetCount(self.engine.handle, HPInt32(count))
        if ret != 0:
            raise Exception(f"Failed to get count: {ret}")

        return count.value

    def view_table_in_terminal(self):
        ret = HF_ViewFaceDBTable(self.engine.handle)
        if ret != 0:
            raise Exception(f"Failed to view table: {ret}")

