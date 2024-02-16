from test.test_settings import *
import inspireface as isf
import numpy as np
import time
from functools import wraps
import cv2
from itertools import cycle
from tqdm import tqdm

from unittest import skipUnless as optional


def title(name: str = None):
    print("--" * 35)
    print(f" InspireFace Version: {isf.__version__}")
    if name is not None:
        print(f" {name}")
    print("--" * 35)


def get_test_data(path: str) -> str:
    return os.path.join(PYTHON_TEST_DATA_FOLDER, path)


def calculate_overlap(box1, box2):
    """
    Calculate the overlap ratio between two rectangular boxes.
    Parameters:
    - box1: The first rectangle, format ((x1, y1), (x2, y2)), where (x1, y1) is the top left coordinate, and (x2, y2) is the bottom right coordinate.
    - box2: The second rectangle, format the same as box1.

    Returns:
    - The overlap ratio, 0 if the rectangles do not overlap.
    """
    # Unpack rectangle coordinates
    (x1_box1, y1_box1), (x2_box1, y2_box1) = box1
    (x1_box2, y1_box2), (x2_box2, y2_box2) = box2

    # Calculate the coordinates of the intersection rectangle
    x_overlap = max(0, min(x2_box1, x2_box2) - max(x1_box1, x1_box2))
    y_overlap = max(0, min(y2_box1, y2_box2) - max(y1_box1, y1_box2))

    # Calculate the area of the intersection
    overlap_area = x_overlap * y_overlap

    # Calculate the area of each rectangle
    box1_area = (x2_box1 - x1_box1) * (y2_box1 - y1_box1)
    box2_area = (x2_box2 - x1_box2) * (y2_box2 - y1_box2)

    # Calculate the total area
    total_area = box1_area + box2_area - overlap_area

    # Calculate the overlap ratio
    overlap_ratio = overlap_area / total_area if total_area > 0 else 0

    return overlap_ratio


def restore_rotated_box(original_width, original_height, box, rotation):
    """
    Restore the coordinates of a rotated face box based on the original image width, height, and rotation angle.

    Parameters:
    - original_width: The width of the original image.
    - original_height: The height of the original image.
    - box: The coordinates of the rotated box, format ((x1, y1), (x2, y2)).
    - rotation: The rotation angle, represented by 0, 1, 2, 3 for 0, 90, 180, 270 degrees respectively.

    Returns:
    - The restored box coordinates, format same as box.
    """
    # For 90 or 270 degrees rotation, the image width and height are swapped
    if rotation == 1 or rotation == 3:
        width, height = original_height, original_width
    else:
        width, height = original_width, original_height

    (x1, y1), (x2, y2) = box

    if rotation == 0:  # No transformation needed for 0 degrees
        restored_box = box
    elif rotation == 1:  # 90 degrees rotation
        restored_box = ((y1, width - x2), (y2, width - x1))
    elif rotation == 2:  # 180 degrees rotation
        restored_box = ((width - x2, height - y2), (width - x1, height - y1))
    elif rotation == 3:  # 270 degrees rotation
        restored_box = ((height - y2, x1), (height - y1, x2))
    else:
        raise ValueError("Rotation must be 0, 1, 2, or 3 representing 0, 90, 180, 270 degrees.")

    return restored_box


def read_binary_file_to_ndarray(file_path, width, height):
    nv21_size = width * height * 3 // 2  # NV21 size calculation

    try:
        with open(file_path, 'rb') as file:
            file_data = file.read()  # Read the entire file

            if len(file_data) != nv21_size:
                print(f"Expected file size is {nv21_size}, but got {len(file_data)}")
                return None

            # Assuming the file data is a complete NV21 frame
            data = np.frombuffer(file_data, dtype=np.uint8)
            return data
    except FileNotFoundError:
        print(f"File '{file_path}' not found.")
        return None
    except Exception as e:
        print(f"An error occurred while reading the file: {str(e)}")
        return None


def print_benchmark_table(benchmark_results):
    print("\n")
    header_format = "{:<20} | {:<10} | {:<15} | {:<15}"
    row_format = "{:<20} | {:<10} | {:>10.2f} ms | {:>10.4f} ms"
    print(header_format.format('Benchmark', 'Loops', 'Total Time', 'Avg Time'))
    print("-" * 70)  # 调整分割线长度以匹配标题长度

    for name, loops, total_time in benchmark_results:
        avg_time = total_time / loops
        print(row_format.format(name, loops, total_time * 1000, avg_time * 1000))


def benchmark(test_name, loop):
    def benchmark_decorator(func):
        @wraps(func)
        def wrapper(self, *args, **kwargs):
            # 在测试对象上设置loop属性
            setattr(self, 'loop', loop)

            start_time = time.time()
            try:
                result = func(self, *args, **kwargs)
            finally:
                end_time = time.time()
                cost_total = end_time - start_time
                self.__class__.benchmark_results.append((test_name, loop, cost_total))

            # 测试完成后，删除loop属性，防止影响其他测试
            delattr(self, 'loop')
            return result

        return wrapper

    return benchmark_decorator


def read_video_generator(video_path):
    cap = cv2.VideoCapture(video_path)

    if not cap.isOpened():
        raise IOError(f"Cannot open video {video_path}")

    while True:
        ret, frame = cap.read()
        if not ret:
            break
        yield frame

    cap.release()


def lfw_generator(directory_path):
    for root, dirs, files in os.walk(directory_path):
        for file_name in files:
            # 确保只处理以'0001.jpg'结尾的JPG图片
            if file_name.endswith('0001.jpg'):
                # 提取人名为目录名的最后一部分
                name = os.path.basename(root)
                image_path = os.path.join(root, file_name)
                image = cv2.imread(image_path)
                assert image is not None, "Error of image data."

                yield image, name


def batch_import_lfw_faces(lfw_path, engine: isf.InspireFaceEngine, num_of_faces: int):
    tracker = isf.FaceTrackerModule(engine)
    tracker.set_track_mode(isf.DETECT_MODE_IMAGE)
    recognition = isf.FaceRecognitionModule(engine)
    generator = lfw_generator(lfw_path)
    registered_faces = 0

    # 使用tqdm包装生成器，未知的总数使用total=None，tqdm将以未知总量模式运行
    for image, name in tqdm(generator, total=num_of_faces, desc="Registering faces"):
        # 执行人脸检测
        faces_info = tracker.execute(image)
        if len(faces_info) == 0:
            continue

        # 从检测到的第一个人脸提取特征
        first_face_info = faces_info[0]
        recognition.extract_feature(image, first_face_info)

        # 使用提取的特征进行人脸注册
        if first_face_info._feature is not None:
            face_identity = isf.FaceIdentity(data=first_face_info, tag=name, custom_id=registered_faces)
            recognition.face_register(face_identity)
            registered_faces += 1
            if registered_faces >= num_of_faces:
                break

    print(f"Completed. Total faces registered: {registered_faces}")
