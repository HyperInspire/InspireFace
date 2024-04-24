import cv2
from inspireface.modules import inspire_face as isf

ret = isf.launch_inspireface("/Users/tunm/work/InspireFace/test_res/pack/Pikachu")
assert ret == 0

config = isf.FeatureHubConfiguration(
    feature_block_num=10,
    enable_use_db=True,
    db_path="./",
    search_threshold=0.48,
    search_mode=isf.HF_SEARCH_MODE_EXHAUSTIVE,
)
isf.feature_hub_enable(config)

session_param = isf.SessionCustomParameter()
session_param.enable_recognition = True
session = isf.InspireFaceSession(session_param)

image = cv2.imread("test/data/bulk/kun.jpg")
stream = isf.ImageStream.load_from_cv_image(image)

result = session.face_detection(stream)

if len(result) > 0:
    feature = session.face_feature_extract(image, result[0])
    print(feature)