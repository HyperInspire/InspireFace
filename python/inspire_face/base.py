from loguru import logger
from .typedef import *


def to_dict(func):
    def wrapper(*args, **kwargs):
        struct = func(*args, **kwargs)
        return get_dict(struct)

    return wrapper


def get_dict(struct):
    result = {}
    for field, _ in struct._fields_:
        value = getattr(struct, field)
        if (type(value) not in [int, float, bool]) and not bool(value):
            value = None
        elif hasattr(value, "_length_") and hasattr(value, "_type_"):
            value = list(value)
        elif hasattr(value, "_fields_"):
            value = get_dict(value)
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

    def release(self):
        if self._handle is not None:
            ret = HF_ReleaseImageStream(self._handle)
            if ret != 0:
                raise Exception("Error")

    def __del__(self):
        self.release()

    def debug_show(self):
        HF_DeBugImageStreamImShow(self._handle)

    @property
    def handle(self):
        return self._handle


class InspireFaceEngine(object):

    def __init__(self, bundle_file: str, param: InspireFaceCustomParameter, detect_mode: int = HF_DETECT_MODE_IMAGE,
                 max_detect_num: int = 10):
        bundle_path = String(bytes(bundle_file, encoding="utf8"))
        self._handle = HContextHandle()
        self.param = param
        ret = HF_CreateFaceContextFromResourceFile(bundle_path, param.dump(), detect_mode, max_detect_num, self._handle)
        if ret != 0:
            raise Exception(f"Create engine error: {ret}")

    def release(self):
        if self._handle is not None:
            ret = HF_ReleaseFaceContext(self._handle)
            self._handle = None
            if ret != 0:
                raise Exception(f"Release engine error: {ret}")

    def __del__(self):
        self.release()

    @property
    def handle(self):
        return self._handle

    def check(self):
        return self._handle is not None


def create_engine(*args, **kwargs) -> InspireFaceEngine:
    return InspireFaceEngine(*args, **kwargs)
