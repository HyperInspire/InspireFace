import os
import cv2
import inspireface as ifac
from inspireface.param import *
import numpy as np

def case_feature_hub():
    # Configure the feature management system.
    feature_hub_config = ifac.FeatureHubConfiguration(
        feature_block_num=10,
        enable_use_db=True,
        db_path="test.db",
        search_threshold=0.48,
        search_mode=HF_SEARCH_MODE_EAGER,
    )
    ret = ifac.feature_hub_enable(feature_hub_config)
    assert ret, "Failed to enable FeatureHub."
    print(ifac.feature_hub_get_face_count())
    for i in range(10):
        feature = ifac.FaceIdentity(np.random.rand(512), i, "test")
        ifac.feature_hub_face_insert(feature)
    print(ifac.feature_hub_get_face_count())


if __name__ == "__main__":
    case_feature_hub()
