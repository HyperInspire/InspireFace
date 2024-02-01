import cv2
import numpy as np

from inspire_face.instantiation import InspireFaceEngine, InspireFaceCustomParameter, FaceTracker, \
    FaceRecognition, FaceInformation


class QuickComparison(object):

    def __init__(self, path: str, threshold: float = 0.48):
        param = InspireFaceCustomParameter()
        param.enable_recognition = True
        self.engine = InspireFaceEngine(path, param=param)
        self.tracker = FaceTracker(self.engine)
        self.recognition = FaceRecognition(self.engine)
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
        逐个交叉对比，保留分数最大的值进行返回，调用self.recognition.face_comparison1v1(info1, info2)
        :return: 最大的匹配分数
        """
        max_score = 0.0  # 初始化最大分数为0

        # 遍历faces_set_1中的每个人脸与faces_set_2中的每个人脸进行比较
        for face1 in self.faces_set_1:
            for face2 in self.faces_set_2:
                # 使用self.recognition.face_comparison1v1(info1, info2)进行人脸比较
                score = self.recognition.face_comparison1v1(face1, face2)
                # 更新最大分数
                if score > max_score:
                    max_score = score

        return max_score

    def match(self) -> bool:
        return self.comp() > self.threshold


if __name__ == "__main__":
    path = "/Users/tunm/work/HyperFace/test_res/model_zip/T1"
    quick = QuickComparison(path, threshold=0.47)
    image1 = cv2.imread("/Users/tunm/Downloads/lfw_funneled/Eliane_Karp/Eliane_Karp_0001.jpg")
    image2 = cv2.imread("/Users/tunm/Downloads/lfw_funneled/Eliane_Karp/Eliane_Karp_0002.jpg")
    quick.setup(image1, image2)
    print(quick.comp())
