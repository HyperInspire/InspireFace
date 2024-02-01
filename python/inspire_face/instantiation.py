import numpy as np
from .inspire_face import *
from loguru import logger
from dataclasses import dataclass
from typing import List, Tuple, Generator, Dict


def struct_to_dict_decorator(func):
    def wrapper(*args, **kwargs):
        struct = func(*args, **kwargs)
        return getdict(struct)

    return wrapper


def getdict(struct):
    result = {}
    for field, _ in struct._fields_:
        value = getattr(struct, field)
        if (type(value) not in [int, float, bool]) and not bool(value):
            value = None
        elif hasattr(value, "_length_") and hasattr(value, "_type_"):
            value = list(value)
        elif hasattr(value, "_fields_"):
            value = getdict(value)
        result[field] = value
    return result


class CameraStream(object):
    def __init__(self, image, stream_format=STREAM_BGR, rotation=CAMERA_ROTATION_0):
        try:
            self.height, self.width, self.channel = image.shape
        except AttributeError as err:
            logger.error(err)
            logger.error("The image data entered is incorrect. Please check whether empty data is entered!")

        self.rotate = rotation
        self.data_format = stream_format
        image_data_ptr = ctypes.cast(image.ctypes.data, ctypes.POINTER(ctypes.c_uint8))
        image_struct = HF_ImageData()
        image_struct.data = image_data_ptr
        image_struct.width = image.shape[1]
        image_struct.height = image.shape[0]
        image_struct.format = self.data_format
        image_struct.rotation = self.rotate
        self._handle = HImageHandle()
        ret = HF_CreateImageStream(Ptr_HF_ImageData(image_struct), self._handle)
        if ret != 0:
            raise Exception("Error")

    def __del__(self):
        ret = HF_ReleaseImageStream(self._handle)
        if ret != 0:
            raise Exception("Error")

    def debug_show(self):
        HF_DeBugImageStreamImShow(self._handle)

    @property
    def handle(self):
        return self._handle


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


class InspireFaceEngine(object):

    def __init__(self, bundle_file: str, param: InspireFaceCustomParameter, detect_mode: int = HF_DETECT_MODE_IMAGE,
                 max_detect_num: int = 10):
        bundle_path = String(bytes(bundle_file, encoding="utf8"))
        self._handle = HContextHandle()
        self.param = param
        ret = HF_CreateFaceContextFromResourceFile(bundle_path, param.dump(), detect_mode, max_detect_num, self._handle)
        if ret != 0:
            raise Exception(f"Create engine error: {ret}")

    def __del__(self):
        ret = HF_ReleaseFaceContext(self._handle)
        self._handle = None
        if ret != 0:
            raise Exception(f"Release engine error: {ret}")

    @property
    def handle(self):
        return self._handle

    def check(self):
        return self._handle is not None


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


class FaceTracker(object):

    def __init__(self, engine: InspireFaceEngine):
        self.engine = engine
        self.multiple_faces = None
        if not self.engine.check():
            raise NotImplemented("The Engine is not instantiated")

    def execute(self, image) -> List[FaceInformation]:
        if isinstance(image, np.ndarray):
            num_of_faces = self.track(image)
        elif isinstance(image, CameraStream):
            num_of_faces = self.track_from_stream(image)
        else:
            raise NotImplemented(f"The {type(image)} type is not supported")
        boxes = self.get_faces_boundary_boxes()
        track_ids = self.get_faces_track_ids()
        euler_angle = self.get_faces_euler_angle()
        tokens = self.get_faces_tokens()

        infos = list()
        for idx in range(num_of_faces):
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
                _feature=None
            )
            infos.append(info)

        return infos

    def track_from_stream(self, stream: CameraStream) -> int:
        self.multiple_faces = HF_MultipleFaceData()
        ret = HF_FaceContextRunFaceTrack(self.engine.handle, stream.handle,
                                         Ptr_HF_MultipleFaceData(self.multiple_faces))
        if ret != 0:
            raise Exception("An error occurred tracking faces")

        return self.multiple_faces.detectedNum

    def track(self, image: np.ndarray) -> int:
        stream = CameraStream(image)
        return self.track_from_stream(stream)

    def get_faces_boundary_boxes(self):
        num_of_faces = self.multiple_faces.detectedNum
        rects_ptr = self.multiple_faces.rects
        rects = [(rects_ptr[i].x, rects_ptr[i].y, rects_ptr[i].width, rects_ptr[i].height) for i in range(num_of_faces)]

        return rects

    def get_faces_track_ids(self):
        num_of_faces = self.multiple_faces.detectedNum
        track_ids_ptr = self.multiple_faces.trackIds
        track_ids = [track_ids_ptr[i] for i in range(num_of_faces)]

        return track_ids

    def get_faces_euler_angle(self):
        num_of_faces = self.multiple_faces.detectedNum
        euler_angle = self.multiple_faces.angles
        angles = [(euler_angle.roll[i], euler_angle.yaw[i], euler_angle.pitch[i]) for i in range(num_of_faces)]

        return angles

    def get_faces_tokens(self):
        num_of_faces = self.multiple_faces.detectedNum
        tokens_ptr = self.multiple_faces.tokens
        tokens = [tokens_ptr[i] for i in range(num_of_faces)]

        return tokens


@dataclass
class InspireFaceDatabaseConfiguration:
    enable_use_db: bool
    db_path: str

    def to_ctypes(self):
        # 转换 Python 类的字段到 ctypes 结构体
        return HF_DatabaseConfiguration(
            enableUseDb=int(self.enable_use_db),
            dbPath=self.db_path.encode('utf-8')  # 假设 C 字符串是 utf-8 编码
        )


class FaceRecognition(object):

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
