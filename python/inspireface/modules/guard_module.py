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
        multi_faces = HF_MultipleFaceData()
        multi_faces.detectedNum = len(faces)
        tokens_pointers = (Ptr_HF_FaceBasicToken * len(faces))()
        for i, token in enumerate(tokens):
            tokens_pointers[i] = Ptr_HF_FaceBasicToken(token)

        multi_faces.tokens = Ptr_HF_FaceBasicToken(tokens_pointers)
        ret = HF_MultipleFacePipelineProcessOptional(self.engine.handle, stream.handle, multi_faces, option)
        print(f"ret = {ret}")
        mask_results = HF_FaceMaskConfidence()
        HF_GetFaceMaskConfidence(self.engine.handle, Ptr_HF_FaceMaskConfidence(mask_results))
        print(mask_results.confidence[0])

        return []