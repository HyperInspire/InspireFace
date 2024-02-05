from .base import *
from .typedef import *
from typing import List


class FaceTrackerModule(object):

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

    def set_track_preview_size(self, size=192):
        HF_FaceContextSetTrackPreviewSize(self.engine.handle, size)

    def get_faces_boundary_boxes(self) -> List:
        num_of_faces = self.multiple_faces.detectedNum
        rects_ptr = self.multiple_faces.rects
        rects = [(rects_ptr[i].x, rects_ptr[i].y, rects_ptr[i].width, rects_ptr[i].height) for i in range(num_of_faces)]

        return rects

    def get_faces_track_ids(self) -> List:
        num_of_faces = self.multiple_faces.detectedNum
        track_ids_ptr = self.multiple_faces.trackIds
        track_ids = [track_ids_ptr[i] for i in range(num_of_faces)]

        return track_ids

    def get_faces_euler_angle(self) -> List:
        num_of_faces = self.multiple_faces.detectedNum
        euler_angle = self.multiple_faces.angles
        angles = [(euler_angle.roll[i], euler_angle.yaw[i], euler_angle.pitch[i]) for i in range(num_of_faces)]

        return angles

    def get_faces_tokens(self) -> List[HF_FaceBasicToken]:
        num_of_faces = self.multiple_faces.detectedNum
        tokens_ptr = self.multiple_faces.tokens
        tokens = [tokens_ptr[i] for i in range(num_of_faces)]

        return tokens
