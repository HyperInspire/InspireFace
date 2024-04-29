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
                logger.error(f"Release ImageStream error: {ret}")

    def __del__(self):
        self.release()

    def debug_show(self):
        HFDeBugImageStreamImShow(self._handle)

    @property
    def handle(self):
        return self._handle


# == Session API ==

@dataclass
class FaceExtended:
    rgb_liveness_confidence: float
    mask_confidence: float
    quality_confidence: float


class FaceInformation:

    def __init__(self,
                 track_id: int,
                 location: Tuple,
                 roll: float,
                 yaw: float,
                 pitch: float,
                 _token: HFFaceBasicToken,
                 _feature: np.array = None):
        self.track_id = track_id
        self.location = location
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
            logger.error("Failed to copy face basic token")

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

    def __init__(self, param, detect_mode: int = HF_DETECT_MODE_IMAGE,
                 max_detect_num: int = 10):
        self.multiple_faces = None
        self._sess = HFSession()
        self.param = param
        if isinstance(self.param, SessionCustomParameter):
            ret = HFCreateInspireFaceSession(self.param._c_struct(), detect_mode, max_detect_num, self._sess)
        elif isinstance(self.param, int):
            ret = HFCreateInspireFaceSessionOptional(self.param, detect_mode, max_detect_num, self._sess)
        else:
            raise NotImplemented("")
        if ret != 0:
            st = f"Create session error: {ret}"
            raise Exception(st)

    def face_detection(self, image) -> List[FaceInformation]:
        stream = self._get_image_stream(image)
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
                    location=(top_left[0], top_left[1], bottom_right[0], bottom_right[1]),
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
        ret = HFSessionSetFaceTrackMode(self._sess, mode)
        if ret != 0:
            logger.error(f"set track model error: {ret}")

    def set_track_preview_size(self, size=192):
        ret = HFSessionSetTrackPreviewSize(self._sess, size)
        if ret != 0:
            logger.error(f"set track preview size error: {ret}")


    def face_pipeline(self, image, faces: List[FaceInformation], exec_param) -> List[FaceExtended]:
        stream = self._get_image_stream(image)
        fn, pm, flag = self._get_processing_function_and_param(exec_param)
        tokens = [face._token for face in faces]
        tokens_array = (HFFaceBasicToken * len(tokens))(*tokens)
        tokens_ptr = cast(tokens_array, PHFFaceBasicToken)

        multi_faces = HFMultipleFaceData()
        multi_faces.detectedNum = len(tokens)
        multi_faces.tokens = tokens_ptr
        ret = fn(self._sess, stream.handle, PHFMultipleFaceData(multi_faces), pm)

        if ret != 0:
            logger.error(f"face pipeline error: {ret}")
            return []

        extends = [FaceExtended(-1.0, -1.0, -1.0) for _ in range(len(faces))]
        self._update_mask_confidence(exec_param, flag, extends)
        self._update_rgb_liveness_confidence(exec_param, flag, extends)
        self._update_face_quality_confidence(exec_param, flag, extends)

        return extends


    def face_feature_extract(self, image, face_information: FaceInformation):
        stream = self._get_image_stream(image)
        feature_length = HInt32()
        HFGetFeatureLength(byref(feature_length))

        feature = np.zeros((feature_length.value,), dtype=np.float32)

        ret = HFFaceFeatureExtractCpy(self._sess, stream.handle, face_information._token,
                                      feature.ctypes.data_as(ctypes.POINTER(HFloat)))

        if ret != 0:
            logger.error(f"face feature extract error: {ret}")
            return None

        return feature


    @staticmethod
    def _get_image_stream(image):
        if isinstance(image, np.ndarray):
            return ImageStream.load_from_cv_image(image)
        elif isinstance(image, ImageStream):
            return image
        else:
            raise NotImplemented("Place check input type.")

    @staticmethod
    def _get_processing_function_and_param(exec_param):
        if isinstance(exec_param, SessionCustomParameter):
            return HFMultipleFacePipelineProcess, exec_param._c_struct(), "object"
        elif isinstance(exec_param, int):
            return HFMultipleFacePipelineProcessOptional, exec_param, "bitmask"
        else:
            raise NotImplemented("Unsupported parameter type")

    def _update_mask_confidence(self, exec_param, flag, extends):
        if (flag == "object" and exec_param.enable_mask_detect) or (
                flag == "bitmask" and exec_param & HF_ENABLE_MASK_DETECT):
            mask_results = HFFaceMaskConfidence()
            ret = HFGetFaceMaskConfidence(self._sess, PHFFaceMaskConfidence(mask_results))
            if ret == 0:
                for i in range(mask_results.num):
                    extends[i].mask_confidence = mask_results.confidence[i]
            else:
                logger.error(f"Get mask result error: {ret}")

    def _update_rgb_liveness_confidence(self, exec_param, flag, extends: List[FaceExtended]):
        if (flag == "object" and exec_param.enable_liveness) or (
                flag == "bitmask" and exec_param & HF_ENABLE_LIVENESS):
            liveness_results = HFRGBLivenessConfidence()
            ret = HFGetRGBLivenessConfidence(self._sess, PHFRGBLivenessConfidence(liveness_results))
            if ret == 0:
                for i in range(liveness_results.num):
                    extends[i].rgb_liveness_confidence = liveness_results.confidence[i]
            else:
                logger.error(f"Get rgb liveness result error: {ret}")

    def _update_face_quality_confidence(self, exec_param, flag, extends: List[FaceExtended]):
        if (flag == "object" and exec_param.enable_face_quality) or (
                flag == "bitmask" and exec_param & HF_ENABLE_QUALITY):
            quality_results = HFFaceQualityConfidence()
            ret = HFGetFaceQualityConfidence(self._sess, PHFFaceQualityConfidence(quality_results))
            if ret == 0:
                for i in range(quality_results.num):
                    extends[i].quality_confidence = quality_results.confidence[i]
            else:
                logger.error(f"Get quality result error: {ret}")

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


# == Global API ==

def launch(resource_path: str) -> bool:
    path_c = String(bytes(resource_path, encoding="utf8"))
    ret = HFLaunchInspireFace(path_c)
    if ret != 0:
        if ret == 1363:
            logger.warning("Duplicate loading was found")
            return True
        else:
            logger.error(f"Launch InspireFace failure: {ret}")
            return False
    return True


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


def feature_hub_enable(config: FeatureHubConfiguration) -> bool:
    ret = HFFeatureHubDataEnable(config._c_struct())
    if ret != 0:
        logger.error(f"FeatureHub enable failure: {ret}")
        return False
    return True


def feature_hub_disable() -> bool:
    ret = HFFeatureHubDataDisable()
    if ret != 0:
        logger.error(f"FeatureHub disable failure: {ret}")
        return False
    return True


def feature_comparison(feature1: np.ndarray, feature2: np.ndarray) -> float:
    faces = [feature1, feature2]
    feats = list()
    for face in faces:
        feature = HFFaceFeature()
        data_ptr = face.ctypes.data_as(HPFloat)
        feature.size = HInt32(face.size)
        feature.data = data_ptr
        feats.append(feature)

    comparison_result = HFloat()
    ret = HFFaceComparison(feats[0], feats[1], HPFloat(comparison_result))
    if ret != 0:
        logger.error(f"Comparison error: {ret}")
        return -1.0

    return float(comparison_result.value)


class FaceIdentity(object):

    def __init__(self, data: np.ndarray, custom_id: int, tag: str):
        self.feature = data
        self.custom_id = custom_id
        self.tag = tag

    @staticmethod
    def from_ctypes(raw_identity: HFFaceFeatureIdentity):
        feature_size = raw_identity.feature.contents.size
        feature_data_ptr = raw_identity.feature.contents.data
        feature_data = np.ctypeslib.as_array(cast(feature_data_ptr, HPFloat), (feature_size,))
        custom_id = raw_identity.customId
        tag = raw_identity.tag.data.decode('utf-8')

        return FaceIdentity(data=feature_data, custom_id=custom_id, tag=tag)

    def _c_struct(self):
        feature = HFFaceFeature()
        data_ptr = self.feature.ctypes.data_as(HPFloat)
        feature.size = HInt32(self.feature.size)
        feature.data = data_ptr
        return HFFaceFeatureIdentity(
            customId=self.custom_id,
            tag=String(bytes(self.tag, encoding="utf8")),
            feature=PHFFaceFeature(feature)
        )


def feature_hub_set_search_threshold(threshold: float):
    HFFeatureHubFaceSearchThresholdSetting(threshold)


def feature_hub_face_insert(face_identity: FaceIdentity) -> bool:
    ret = HFFeatureHubInsertFeature(face_identity._c_struct())
    if ret != 0:
        logger.error(f"Failed to insert face feature data into FeatureHub")
        return False
    return True


@dataclass
class SearchResult:
    confidence: float
    similar_identity: FaceIdentity


def feature_hub_face_search(data: np.ndarray) -> SearchResult:
    feature = HFFaceFeature(size=HInt32(data.size), data=data.ctypes.data_as(HPFloat))
    confidence = HFloat()
    most_similar = HFFaceFeatureIdentity()
    ret = HFFeatureHubFaceSearch(feature, HPFloat(confidence), PHFFaceFeatureIdentity(most_similar))
    if ret != 0:
        logger.error(f"Failed to search face: {ret}")
        return SearchResult(confidence=-1, similar_identity=FaceIdentity(np.zeros(0), most_similar.customId, "None"))
    if most_similar.customId != -1:
        search_identity = FaceIdentity.from_ctypes(most_similar)
        search_result = SearchResult(confidence=confidence.value, similar_identity=search_identity, )
    else:
        none = FaceIdentity(np.zeros(0), most_similar.customId, "None")
        search_result = SearchResult(confidence=confidence.value, similar_identity=none, )

    return search_result


def feature_hub_face_search_top_k(data: np.ndarray, top_k: int) -> List[Tuple]:
    feature = HFFaceFeature(size=HInt32(data.size), data=data.ctypes.data_as(HPFloat))
    results = HFSearchTopKResults()
    ret = HFFeatureHubFaceSearchTopK(feature, top_k, PHFSearchTopKResults(results))
    outputs = list()
    if ret == 0:
        num = results.size.value
        for idx in range(num):
            confidence = results.confidence[idx].value
            customId = results.customIds[idx].value
            outputs.append((confidence, customId))

    return outputs

def feature_hub_face_update(face_identity: FaceIdentity) -> bool:
    ret = HFFeatureHubFaceUpdate(face_identity._c_struct())
    if ret != 0:
        logger.error(f"Failed to update face feature data to FeatureHub")
        return False
    return True


def feature_hub_face_remove(custom_id: int) -> bool:
    ret = HFFeatureHubFaceRemove(custom_id)
    if ret != 0:
        logger.error(f"Failed to update face feature data to FeatureHub")
        return False
    return True


def feature_hub_get_face_identity(custom_id: int):
    identify = HFFaceFeatureIdentity()
    ret = HFFeatureHubGetFaceIdentity(custom_id, PHFFaceFeatureIdentity(identify))
    if ret != 0:
        logger.error("Get face identity errors from FeatureHub")
        return None

    return FaceIdentity.from_ctypes(identify)


def feature_hub_get_face_count() -> int:
    count = HInt32()
    ret = HFFeatureHubGetFaceCount(HPInt32(count))
    if ret != 0:
        logger.error(f"Failed to get count: {ret}")

    return int(count.value)


def view_table_in_terminal():
    ret = HFFeatureHubViewDBTable()
    if ret != 0:
        logger.error(f"Failed to view DB: {ret}")


def version() -> str:
    ver = HFInspireFaceVersion()
    HFQueryInspireFaceVersion(PHFInspireFaceVersion(ver))
    st = f"{ver.major}.{ver.minor}.{ver.patch}"
    return st

def set_logging_level(level: int) -> None:
    HFSetLogLevel(level)

def disable_logging() -> None:
    HFLogDisable()

