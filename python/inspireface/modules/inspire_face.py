import cv2
import numpy as np
from .core import *
from typing import Tuple, List
from dataclasses import dataclass
from loguru import logger

class ImageStream(object):

    @staticmethod
    def load_from_cv_image(image: np.ndarray, stream_format=HF_STREAM_BGR, rotation=HF_CAMERA_ROTATION_0):
        h, w, c = image.shape
        if c != 3 and c != 4:
            raise Exception("Thr channel must be 3 or 4.")

        return ImageStream(image, w, h, stream_format, rotation)

    @staticmethod
    def load_from_ndarray(data: np.ndarray, width: int, height: int, stream_format: int, rotation: int):
        return ImageStream(data, width, height, stream_format, rotation)

    @staticmethod
    def load_from_buffer(data, width: int, height: int, stream_format: int, rotation: int):
        return ImageStream(data, width, height, stream_format, rotation)

    def __init__(self, data, width: int, height: int, stream_format: int, rotation: int, ):
        self.rotate = rotation
        self.data_format = stream_format
        if isinstance(data, np.ndarray):
            data_ptr = ctypes.cast(data.ctypes.data, ctypes.POINTER(ctypes.c_uint8))
        else:
            data_ptr = ctypes.cast(data, ctypes.POINTER(ctypes.c_uint8))
        image_struct = HFImageData()
        image_struct.data = data_ptr
        image_struct.width = width
        image_struct.height = height
        image_struct.format = self.data_format
        image_struct.rotation = self.rotate
        self._handle = HFImageStream()
        ret = HFCreateImageStream(PHFImageData(image_struct), self._handle)
        if ret != 0:
            raise Exception("Error")

    def release(self):
        if self._handle is not None:
            ret = HFReleaseImageStream(self._handle)
            if ret != 0:
                raise Exception("Error")

    def __del__(self):
        self.release()

    def debug_show(self):
        HFDeBugImageStreamImShow(self._handle)

    @property
    def handle(self):
        return self._handle

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


@dataclass
class SessionCustomParameter:
    enable_recognition = False
    enable_liveness = False
    enable_ir_liveness = False
    enable_mask_detect = False
    enable_age = False
    enable_gender = False
    enable_face_quality = False
    enable_interaction_liveness = False


    def _c_struct(self):
        custom_param = HFSessionCustomParameter(
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

class InspireFaceSession(object):

    def __init__(self, param: SessionCustomParameter, detect_mode: int = HF_DETECT_MODE_IMAGE, max_detect_num: int = 10):
        self.multiple_faces = None
        self._sess = HFSession()
        self.param = param
        ret = HFCreateInspireFaceSession(param._c_struct(), detect_mode, max_detect_num, self._sess)
        if ret != 0:
            raise Exception(f"Create session error: {ret}")

    def face_detection(self, image) -> List[FaceInformation]:
        if isinstance(image, np.ndarray):
            stream = ImageStream.load_from_cv_image(image)
        elif isinstance(image, ImageStream):
            stream = image
        else:
            raise NotImplemented("Place check input type.")
        self.multiple_faces = HFMultipleFaceData()
        ret = HFExecuteFaceTrack(self._sess, stream.handle,
                                 PHFMultipleFaceData(self.multiple_faces))
        if ret != 0:
            logger.error(f"Face detection error: ", {ret})
            return []

        if self.multiple_faces.detectedNum > 0:
            boxes = self._get_faces_boundary_boxes()
            track_ids = self._get_faces_track_ids()
            euler_angle = self._get_faces_euler_angle()
            tokens = self._get_faces_tokens()

            infos = list()
            for idx in range(self.multiple_faces.detectedNum):
                top_left = (boxes[idx][0], boxes[idx][1])
                bottom_right = (boxes[idx][0] + boxes[idx][2], boxes[idx][1] + boxes[idx][3])
                roll = euler_angle[idx][0]
                yaw = euler_angle[idx][1]
                pitch = euler_angle[idx][2]
                track_id = track_ids[idx]
                _token = tokens[idx]

                info = FaceInformation(
                    top_left=top_left,
                    bottom_right=bottom_right,
                    roll=roll,
                    yaw=yaw,
                    pitch=pitch,
                    track_id=track_id,
                    _token=_token,
                )
                infos.append(info)

            return infos
        else:
            return []

    def set_track_mode(self, mode: int):
        ret = HFExecuteFaceTrack(self._sess, mode)
        if ret != 0:
            logger.error(f"set track model error: {ret}")

    def set_track_preview_size(self, size=192):
        ret = HFSessionSetTrackPreviewSize(self._sess, size)
        if ret != 0:
            logger.error(f"set track preview size error: {ret}")

    def face_feature_extract(self, image, face_information: FaceInformation) :
        if isinstance(image, np.ndarray):
            stream = ImageStream.load_from_cv_image(image)
        elif isinstance(image, ImageStream):
            stream = image
        else:
            raise NotImplemented("Place check input type.")
        feature_length = HInt32()
        HFGetFeatureLength(byref(feature_length))

        feature = np.zeros((feature_length.value, ), dtype=np.float32)

        ret = HFFaceFeatureExtractCpy(self._sess, stream.handle, face_information._token, feature.ctypes.data_as(ctypes.POINTER(HFloat)))

        if ret != 0:
            logger.error(f"face feature extract error: {ret}")
            return None

        return feature


    def _get_faces_boundary_boxes(self) -> List:
        num_of_faces = self.multiple_faces.detectedNum
        rects_ptr = self.multiple_faces.rects
        rects = [(rects_ptr[i].x, rects_ptr[i].y, rects_ptr[i].width, rects_ptr[i].height) for i in range(num_of_faces)]

        return rects

    def _get_faces_track_ids(self) -> List:
        num_of_faces = self.multiple_faces.detectedNum
        track_ids_ptr = self.multiple_faces.trackIds
        track_ids = [track_ids_ptr[i] for i in range(num_of_faces)]

        return track_ids

    def _get_faces_euler_angle(self) -> List:
        num_of_faces = self.multiple_faces.detectedNum
        euler_angle = self.multiple_faces.angles
        angles = [(euler_angle.roll[i], euler_angle.yaw[i], euler_angle.pitch[i]) for i in range(num_of_faces)]

        return angles

    def _get_faces_tokens(self) -> List[HFFaceBasicToken]:
        num_of_faces = self.multiple_faces.detectedNum
        tokens_ptr = self.multiple_faces.tokens
        tokens = [tokens_ptr[i] for i in range(num_of_faces)]

        return tokens




def launch_inspireface(resorece_path: str) -> int:
    path_c = String(bytes(resorece_path, encoding="utf8"))
    ret = HFLaunchInspireFace(path_c)
    return ret

@dataclass
class FeatureHubConfiguration:
    feature_block_num: int
    enable_use_db: bool
    db_path: str
    search_threshold: float
    search_mode: HF_SEARCH_MODE_EAGER

    def _c_struct(self):
        return HFFeatureHubConfiguration(
            enableUseDb=int(self.enable_use_db),
            dbPath=String(bytes(self.db_path, encoding="utf8")),
            featureBlockNum=self.feature_block_num,
            searchThreshold=self.search_threshold,
            searchMode=self.search_mode
        )

def feature_hub_enable(config: FeatureHubConfiguration) -> int:
    ret = HFFeatureHubDataEnable(config._c_struct())
    return ret

