import cv2
import numpy as np
from .core import *
from typing import Tuple

class FaceInformation:

    def __init__(self,
                 track_id: int,
                 top_left: Tuple,
                 bottom_right: Tuple,
                 roll: float,
                 yaw: float,
                 pitch: float,
                 _token: HFFaceBasicToken,
                 _feature: np.array = None):
        self.track_id = track_id
        self.top_left = top_left
        self.bottom_right = bottom_right
        self.roll = roll
        self.yaw = yaw
        self.pitch = pitch

        # copy token
        token_size = HInt32()
        HFGetFaceBasicTokenSize(HPInt32(token_size))
        buffer_size = token_size.value
        self.buffer = create_string_buffer(buffer_size)
        ret = HFCopyFaceBasicToken(_token, self.buffer, token_size)
        if ret != 0:
            raise Exception("Failed to copy face basic token")

        self._token = HFFaceBasicToken()
        self._token.size = buffer_size
        self._token.data = cast(addressof(self.buffer), c_void_p)


class InspireFaceSession(object):

    def __init__(self):
        self.multiple_faces = None
        pass

    def face_detection(self):
        pass

    # def