import cv2
import inspireface as ifac
from inspireface.param import *

ret = ifac.launch("/Users/tunm/work/InspireFace/test_res/pack/Pikachu")
assert ret

config = ifac.FeatureHubConfiguration(
    feature_block_num=10,
    enable_use_db=True,
    db_path="./",
    search_threshold=0.48,
    search_mode=HF_SEARCH_MODE_EXHAUSTIVE,
)
ifac.feature_hub_enable(config)

session_opt = HF_ENABLE_FACE_RECOGNITION | HF_ENABLE_MASK_DETECT | HF_ENABLE_QUALITY | HF_ENABLE_LIVENESS
session = ifac.InspireFaceSession(session_opt)

image = cv2.imread("test/data/bulk/kun.jpg")
stream = ifac.ImageStream.load_from_cv_image(image)

result = session.face_detection(stream)


if len(result) > 0:
    print(result[0].location)
    feature = session.face_feature_extract(image, result[0])
    identity = ifac.FaceIdentity(feature, 1, "KUN")
    ret = ifac.feature_hub_face_insert(identity)
    assert ret
    idg = ifac.feature_hub_get_face_identity(1)
    assert idg != None

extends = session.face_pipeline(image, result, HF_ENABLE_MASK_DETECT | HF_ENABLE_QUALITY | HF_ENABLE_LIVENESS)

print(extends[0])