import cv2
import inspireface as hf

ret = hf.launch("/Users/tunm/work/InspireFace/test_res/pack/Pikachu")
assert ret

config = hf.FeatureHubConfiguration(
    feature_block_num=10,
    enable_use_db=True,
    db_path="./",
    search_threshold=0.48,
    search_mode=hf.HF_SEARCH_MODE_EXHAUSTIVE,
)
hf.feature_hub_enable(config)

session_opt = hf.HF_ENABLE_FACE_RECOGNITION | hf.HF_ENABLE_MASK_DETECT | hf.HF_ENABLE_QUALITY | hf.HF_ENABLE_LIVENESS
session = hf.InspireFaceSession(session_opt)

image = cv2.imread("test/data/bulk/kun.jpg")
stream = hf.ImageStream.load_from_cv_image(image)

result = session.face_detection(stream)


if len(result) > 0:
    print(result[0].location)
    feature = session.face_feature_extract(image, result[0])
    identity = hf.FaceIdentity(feature, 1, "KUN")
    ret = hf.feature_hub_face_insert(identity)
    assert ret
    idg = hf.feature_hub_get_face_identity(1)
    assert idg != None

extends = session.face_pipeline(image, result, hf.HF_ENABLE_MASK_DETECT | hf.HF_ENABLE_QUALITY | hf.HF_ENABLE_LIVENESS)

print(extends[0])