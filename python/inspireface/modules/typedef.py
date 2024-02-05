import numpy as np
from dataclasses import dataclass
from typing import Tuple
from inspireface.modules.core import *

ENABLE_FACE_RECOGNITION = HF_ENABLE_FACE_RECOGNITION
ENABLE_LIVENESS = HF_ENABLE_LIVENESS
ENABLE_IR_LIVENESS = HF_ENABLE_IR_LIVENESS
ENABLE_MASK_DETECT = HF_ENABLE_MASK_DETECT
ENABLE_AGE_PREDICT = HF_ENABLE_AGE_PREDICT
ENABLE_GENDER_PREDICT = HF_ENABLE_GENDER_PREDICT
ENABLE_QUALITY = HF_ENABLE_QUALITY
ENABLE_INTERACTION = HF_ENABLE_INTERACTION

DETECT_MODE_IMAGE = HF_DETECT_MODE_IMAGE
DETECT_MODE_VIDEO = HF_DETECT_MODE_VIDEO


@dataclass
class EngineCustomParameter:
    enable_recognition = False
    enable_liveness = False
    enable_ir_liveness = False
    enable_mask_detect = False
    enable_age = False
    enable_gender = False
    enable_face_quality = False
    enable_interaction_liveness = False

    def dump(self):
        custom_param = HF_ContextCustomParameter(
            enable_recognition=int(self.enable_recognition),
            enable_liveness=int(self.enable_liveness),
            enable_ir_liveness=int(self.enable_ir_liveness),
            enable_mask_detect=int(self.enable_mask_detect),
            enable_age=int(self.enable_age),
            enable_gender=int(self.enable_gender),
            enable_face_quality=int(self.enable_face_quality),
            enable_interaction_liveness=int(self.enable_interaction_liveness)
        )

        return custom_param


@dataclass
class DatabaseConfiguration:
    enable_use_db: bool
    db_path: str

    def to_ctypes(self):
        return HF_DatabaseConfiguration(
            enableUseDb=int(self.enable_use_db),
            dbPath=String(bytes(self.db_path, encoding="utf8"))
        )


class FaceInformation:

    def __init__(self,
                 track_id: int,
                 top_left: Tuple,
                 bottom_right: Tuple,
                 roll: float,
                 yaw: float,
                 pitch: float,
                 _token: HF_FaceBasicToken,
                 _feature: np.array = None):
        self.track_id = track_id
        self.top_left = top_left
        self.bottom_right = bottom_right
        self.roll = roll
        self.yaw = yaw
        self.pitch = pitch
        self._feature = _feature
        self.extend = None # Expand some face information

        # copy token
        token_size = HInt32()
        HF_GetFaceBasicTokenSize(HPInt32(token_size))
        buffer_size = token_size.value
        self.buffer = create_string_buffer(buffer_size)
        ret = HF_CopyFaceBasicToken(_token, self.buffer, token_size)
        if ret != 0:
            raise Exception("Failed to copy face basic token")

        self._token = HF_FaceBasicToken()
        self._token.size = buffer_size
        self._token.data = cast(addressof(self.buffer), c_void_p)

    @property
    def token(self):
        return self._token

    @property
    def feature(self):
        return self._feature


@dataclass
class FaceExtended:
    rgb_liveness_confidence: float
    mask_confidence: float
    quality_confidence: float


class FaceIdentity(object):

    def __init__(self, data, custom_id: int, tag: str):
        if isinstance(data, np.ndarray):
            self.feature = data
        elif isinstance(data, FaceInformation):
            self.feature = data.feature
        else:
            raise Exception(f"This type of input is not supported: {type(data)}")
        if self.feature is None:
            raise Exception("Not Feature")
        self.custom_id = custom_id
        self.tag = tag

    @staticmethod
    def from_ctypes(raw_identity: HF_FaceFeatureIdentity):
        feature_size = raw_identity.feature.contents.size
        feature_data_ptr = raw_identity.feature.contents.data
        feature_data = np.ctypeslib.as_array(cast(feature_data_ptr, HPFloat), (feature_size,))
        custom_id = raw_identity.customId
        tag = raw_identity.tag.data.decode('utf-8')

        return FaceIdentity(data=feature_data, custom_id=custom_id, tag=tag)

    def to_ctypes(self):
        feature = HF_FaceFeature()
        data_ptr = self.feature.ctypes.data_as(HPFloat)
        feature.size = HInt32(self.feature.size)
        feature.data = data_ptr
        return HF_FaceFeatureIdentity(
            customId=self.custom_id,
            tag=String(bytes(self.tag, encoding="utf8")),
            feature=Ptr_HF_FaceFeature(feature)
        )


@dataclass
class SearchResult:
    confidence: float
    similar_identity: FaceIdentity
