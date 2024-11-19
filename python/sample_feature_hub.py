import os
import cv2
import inspireface as ifac
from inspireface.param import *
import numpy as np
import os

def case_feature_hub():
    db_path = "test.db"
    # Configure the feature management system.
    feature_hub_config = ifac.FeatureHubConfiguration(
        primary_key_mode=ifac.HF_PK_AUTO_INCREMENT,
        enable_persistence=True,
        persistence_db_path=db_path,
        search_threshold=0.48,
        search_mode=HF_SEARCH_MODE_EAGER,
    )
    ret = ifac.feature_hub_enable(feature_hub_config)
    assert ret, "Failed to enable FeatureHub."
    print(ifac.feature_hub_get_face_count())
    for i in range(10):
        feature = ifac.FaceIdentity(np.random.rand(512), -1)
        ifac.feature_hub_face_insert(feature)
    print(ifac.feature_hub_get_face_count())

    assert os.path.exists(db_path), "FeatureHub database file not found."


if __name__ == "__main__":
    case_feature_hub()