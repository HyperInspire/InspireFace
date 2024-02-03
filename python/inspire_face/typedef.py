import numpy as np
from dataclasses import dataclass
from typing import Tuple
from .core import *


@dataclass
class InspireFaceCustomParameter(object):
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
class InspireFaceDatabaseConfiguration:
    enable_use_db: bool
    db_path: str

    def to_ctypes(self):
        return HF_DatabaseConfiguration(
            enableUseDb=int(self.enable_use_db),
            dbPath=self.db_path.encode('utf-8')
        )


@dataclass
class FaceInformation(object):
    track_id: int
    top_left: Tuple
    bottom_right: Tuple
    roll: float
    yaw: float
    pitch: float
    _token: HF_FaceBasicToken
    _feature: np.array

    @property
    def token(self):
        return self._token

    @property
    def feature(self):
        return self._feature
