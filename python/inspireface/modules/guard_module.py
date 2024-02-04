from .base import *
from .typedef import *
from typing import List


class FaceGuardModule(object):

    def __init__(self, engine: InspireFaceEngine):
        self.engine = engine
        if not self.engine.check():
            raise NotImplemented("The Engine is not instantiated")
        if not self.engine.param.enable_recognition:
            raise NotImplemented("The Engine's face recognition module is not enabled")

    def pipeline(self, image, faces: List[FaceInformation], option: int) -> List[FaceExtended]:
        if isinstance(image, np.ndarray):
            stream = CameraStream(image)
        elif isinstance(image, CameraStream):
            stream = image
        else:
            raise NotImplemented(f"The {type(image)} type is not supported")
        tokens = [face.token for face in faces]

        HF_FaceBasicToken_Array = HF_FaceBasicToken * len(tokens)
        tokens_array = HF_FaceBasicToken_Array(*tokens)
        tokens_ptr = cast(tokens_array, Ptr_HF_FaceBasicToken)

        multi_faces = HF_MultipleFaceData()
        multi_faces.detectedNum = len(tokens)
        multi_faces.tokens = tokens_ptr

        ret = HF_MultipleFacePipelineProcessOptional(self.engine.handle, stream.handle, Ptr_HF_MultipleFaceData(multi_faces), option)
        print(f"ret = {ret}")
        mask_results = HF_FaceMaskConfidence()
        HF_GetFaceMaskConfidence(self.engine.handle, Ptr_HF_FaceMaskConfidence(mask_results))
        print(mask_results.confidence[0])

        return []