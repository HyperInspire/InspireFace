import logging
from typing import List
import cv2
import os
import numpy as np
import tqdm

from evaluation_tools import QuickComparison, FaceInformation


def draw_face_boxes(image, faces: List[FaceInformation]):
    for face in faces:
        cv2.rectangle(image, face.top_left, face.bottom_right, (0, 0, 255), 2)


def read_pairs(pairs_filename):
    """Read the pairs.txt file and return a list of image pairs"""
    pairs = []
    with open(pairs_filename, 'r') as f:
        for line in f.readlines()[1:]:  # Skip the first line
            pair = line.strip().split()
            pairs.append(pair)
    return pairs


def test_lfw(lfw_dir, pairs, quick, cache_file="lfw_cache.npy", visual=True):
    """Load and display image pairs, while printing whether they should match, and calculate accuracy"""
    # Check whether the cache file exists
    if os.path.exists(cache_file):
        print("Loading results from cache")
        cache = np.load(cache_file, allow_pickle=True)
        similarities = cache[0]
        labels = cache[1]
    else:
        similarities = []
        labels = []

        for pair in tqdm.tqdm(pairs):
            if len(pair) == 3:
                # matching pair
                person, img_num1, img_num2 = pair
                img_path1 = os.path.join(lfw_dir, person, f"{person}_{img_num1.zfill(4)}.jpg")
                img_path2 = os.path.join(lfw_dir, person, f"{person}_{img_num2.zfill(4)}.jpg")
                match = True
            else:
                # Mismatched pair
                person1, img_num1, person2, img_num2 = pair
                img_path1 = os.path.join(lfw_dir, person1, f"{person1}_{img_num1.zfill(4)}.jpg")
                img_path2 = os.path.join(lfw_dir, person2, f"{person2}_{img_num2.zfill(4)}.jpg")
                match = False

            # Load image
            img1 = cv2.imread(img_path1)
            img2 = cv2.imread(img_path2)

            if not quick.setup(img1, img2):
                logging.warning("not detect face")
                continue

            # Record cosine similarity and label
            cosine_similarity = quick.comp()
            similarities.append(cosine_similarity)
            labels.append(match)

            # If visual is True, visualize
            if visual:
                draw_face_boxes(img1, quick.faces_set_1)
                draw_face_boxes(img2, quick.faces_set_2)

                # display image
                cv2.imshow(f"Image 1 (Match: {match})", img1)
                cv2.imshow(f"Image 2 (Match: {match})", img2)
                print(f"Should match: {match}, Cosine Similarity: {cosine_similarity}")

                if cv2.waitKey(0) & 0xFF == ord('q'):
                    break

            if visual:
                cv2.destroyAllWindows()

        similarities = np.array(similarities)
        labels = np.array(labels)
        # 保存结果到缓存文件
        np.save(cache_file, [similarities, labels])

    # Find the optimal threshold
    best_threshold, best_accuracy = find_best_threshold(similarities, labels)
    print(f"Best Threshold: {best_threshold:.2f}, Best Accuracy: {best_accuracy:.3f}")


def find_best_threshold(similarities, labels):
    thresholds = np.arange(0, 1, 0.01)
    best_threshold = best_accuracy = 0

    for threshold in thresholds:
        predictions = (similarities > threshold)
        accuracy = np.mean((predictions == labels).astype(int))
        if accuracy > best_accuracy:
            best_accuracy = accuracy
            best_threshold = threshold

    return best_threshold, best_accuracy


if __name__ == "__main__":
    lfw_dir = "/Users/tunm/datasets/lfw_funneled"  # Change the path to your LFW funneled folder
    pairs_filename = "/Users/tunm/datasets/lfw_funneled/pairs.txt"  # Change to your pairs.txt file path

    path = "/Users/tunm/work/HyperFace/test_res/pack/Optimus-t1"
    quick = QuickComparison(path)

    pairs = read_pairs(pairs_filename)
    test_lfw(lfw_dir, pairs, quick, visual=False)  # You can turn off visualization by setting visual to False
