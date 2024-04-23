import cv2
import numpy as np

from inspireface.modules.base import InspireFaceEngine, EngineCustomParameter, FaceTrackerModule, \
    FaceRecognitionModule


class QuickComparison(object):

    def __init__(self, path: str, threshold: float = 0.48):
        param = EngineCustomParameter()
        param.enable_recognition = True
        self.engine = InspireFaceEngine(path, param=param)
        self.tracker = FaceTrackerModule(self.engine)
        self.recognition = FaceRecognitionModule(self.engine)
        self.threshold = threshold
        self.faces_set_1 = None
        self.faces_set_2 = None

    def setup(self, image1: np.ndarray, image2: np.ndarray) -> bool:
        images = [image1, image2]
        self.faces_set_1 = list()
        self.faces_set_2 = list()
        for idx, img in enumerate(images):
            results = self.tracker.execute(img)
            if len(results) > 0:
                for info in results:
                    self.recognition.extract_feature(img, info)
            else:
                return False

            if idx == 0:
                self.faces_set_1 = results
            else:
                self.faces_set_2 = results

        return True

    def comp(self) -> float:
        """
        Cross-compare one by one, keep the value with the highest score and return it, calling self.recognition.face_comparison1v1(info1, info2)
        :return: Maximum matching score
        """
        max_score = 0.0  # The maximum initial fraction is 0

        # Each face in faces_set_1 is traversed and compared with each face in faces_set_2
        for face1 in self.faces_set_1:
            for face2 in self.faces_set_2:
                # Use self.recognition.face_comparison1v1(info1, info2) Compare faces
                score = self.recognition.face_comparison1v1(face1, face2)
                # Update maximum score
                if score > max_score:
                    max_score = score

        return max_score

    def match(self) -> bool:
        return self.comp() > self.threshold


if __name__ == "__main__":
    path = "/Users/tunm/work/HyperFace/test_res/model_zip/Pikachu-t1"
    quick = QuickComparison(path, threshold=0.47)
    image1 = cv2.imread("/Users/tunm/Downloads/lfw_funneled/Eliane_Karp/Eliane_Karp_0001.jpg")
    image2 = cv2.imread("/Users/tunm/Downloads/lfw_funneled/Eliane_Karp/Eliane_Karp_0002.jpg")
    quick.setup(image1, image2)
    print(quick.comp())
