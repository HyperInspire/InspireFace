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

    def pipeline_to_information(self, image, faces: List[FaceInformation], option: int):
        extends = self.pipeline(image, faces, option)
        for idx, face in enumerate(faces):
            face.extend = extends[idx]

    def pipeline(self, image, faces: List[FaceInformation], option: int) -> List[FaceExtended]:
        if isinstance(image, np.ndarray):
            stream = CameraStream.load_from_cv_image(image)
        elif isinstance(image, CameraStream):
            stream = image
        else:
            error = f"The {type(image)} type is not supported"
            raise NotImplemented(error)
        tokens = [face.token for face in faces]

        HF_FaceBasicToken_Array = HF_FaceBasicToken * len(tokens)
        tokens_array = HF_FaceBasicToken_Array(*tokens)
        tokens_ptr = cast(tokens_array, Ptr_HF_FaceBasicToken)

        multi_faces = HF_MultipleFaceData()
        multi_faces.detectedNum = len(tokens)
        multi_faces.tokens = tokens_ptr

        ret = HF_MultipleFacePipelineProcessOptional(self.engine.handle, stream.handle,
                                                     Ptr_HF_MultipleFaceData(multi_faces), option)
        if ret != 0:
            error = f"Execution pipeline error: {ret}"
            raise Exception(error)

        extends = list()
        for idx in range(len(faces)):
            ext = FaceExtended(-1, -1, -1)
            extends.append(ext)

        if option & ENABLE_MASK_DETECT:
            if not self.engine.param.enable_mask_detect:
                raise Exception("Mask detection is not enabled when creating engine")
            mask_results = HF_FaceMaskConfidence()
            ret = HF_GetFaceMaskConfidence(self.engine.handle, Ptr_HF_FaceMaskConfidence(mask_results))
            if ret != 0:
                error = f"Get mask result error: {ret}"
                raise Exception(error)
            for idx in range(mask_results.num):
                extends[idx].mask_confidence = mask_results.confidence[idx]

        if option & ENABLE_LIVENESS:
            if not self.engine.param.enable_liveness:
                raise Exception("RGB Liveness detection is not enabled when creating engine")
            liveness_results = HF_RGBLivenessConfidence()
            ret = HF_GetRGBLivenessConfidence(self.engine.handle, Ptr_HF_RGBLivenessConfidence(liveness_results))

            if ret != 0:
                error = f"Get liveness result error: {ret}"
                raise Exception(error)
            for idx in range(liveness_results.num):
                extends[idx].rgb_liveness_confidence = liveness_results.confidence[idx]

        if option & ENABLE_QUALITY:
            if not self.engine.param.enable_face_quality:
                raise Exception("Face quality detection is not enabled when creating engine")
            for idx in range(len(tokens)):
                quality = HFloat()
                ret = HF_FaceQualityDetect(self.engine.handle, faces[idx].token, HPFloat(quality))
                if ret != 0:
                    error = f"Get quality result error: {ret}"
                    raise Exception(error)
                extends[idx].quality_confidence = quality.value

        return extends
