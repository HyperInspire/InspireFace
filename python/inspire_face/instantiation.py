import cv2
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
        self.handle = HImageHandle()
        ret = HF_CreateImageStream(Ptr_HF_ImageData(image_struct), self.handle)
        if ret != 0:
            raise Exception("Error")

    def __del__(self):
        ret = HF_ReleaseImageStream(self.handle)
        if ret != 0:
            raise Exception("Error")

    def debug_show(self):
        HF_DeBugImageStreamImShow(self.handle)


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
                 max_detect_num: int = 1):
        bundle_path = String(bytes(bundle_file, encoding="utf8"))
        self.handle = HContextHandle()
        ret = HF_CreateFaceContextFromResourceFile(bundle_path, param.dump(), detect_mode, max_detect_num, self.handle)
        if ret != 0:
            raise Exception(f"Create engine error: {ret}")

    def __del__(self):
        ret = HF_ReleaseFaceContext(self.handle)
        if ret != 0:
            raise Exception(f"Release engine error: {ret}")
