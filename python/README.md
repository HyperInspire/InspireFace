# InspireFace Python API

InspireFace provides an easy-to-use Python API that wraps the underlying dynamic link library through ctypes. You can install the latest release version via pip or configure it using the project's self-compiled dynamic library.

## Quick Installation

### Install via pip (Recommended)

```bash
pip install inspireface
```

### Manual Installation

1. Copy the compiled dynamic library to the specified directory:
```bash
# Copy the compiled dynamic library to the corresponding system architecture directory
cp YOUR_BUILD_DIR/libInspireFace.so inspireface/modules/core/SYSTEM/CORE_ARCH/
```

2. Install the Python package and its declared dependencies:
```bash
pip install .
```

## Quick Start

Here's a simple example showing how to use InspireFace for face detection and landmark drawing:

```python
import cv2
import inspireface as isf

isf.launch()
try:
    with isf.InspireFaceSession(
        param=isf.HF_ENABLE_NONE,
        detect_mode=isf.HF_DETECT_MODE_ALWAYS_DETECT,
        auto_launch=False,
    ) as session:
        session.set_detection_confidence_threshold(0.5)

        image = cv2.imread("path/to/your/image.jpg")
        if image is None:
            raise FileNotFoundError("Unable to read the input image")

        faces = session.face_detection(image)
        print(f"Detected {len(faces)} faces")

        draw = image.copy()
        for face in faces:
            x1, y1, x2, y2 = face.location
            center = ((x1 + x2) / 2, (y1 + y2) / 2)
            size = (x2 - x1, y2 - y1)
            rect = (center, size, face.roll)
            box = cv2.boxPoints(rect).astype(int)
            cv2.drawContours(draw, [box], 0, (100, 180, 29), 2)

            landmarks = session.get_face_dense_landmark(face)
            for x, y in landmarks.astype(int):
                cv2.circle(draw, (x, y), 0, (220, 100, 0), 2)
finally:
    if isf.query_launch_status():
        isf.terminate()
```

## More Examples

The project provides multiple example files demonstrating different features:

- `sample_face_detection.py`: Basic face detection
- `sample_face_track_from_video.py`: Video face tracking
- `sample_face_recognition.py`: Face recognition
- `sample_face_comparison.py`: Face comparison
- `sample_feature_hub.py`: Feature extraction
- `sample_system_resource_statistics.py`: System resource statistics

## Running Tests

The comprehensive Python suite shares the same `test_res` fixture tree as the C++ API tests:

```bash
python -m sample_testcase.run --native-lib ../build/lib/libInspireFace.so
```

## Notes

1. Ensure that OpenCV and other necessary dependencies are installed on your system
2. Make sure the dynamic library is correctly installed before use
3. Python 3.7 or higher is recommended
4. The default version is CPU, if you want to use the GPU, CoreML, or NPU backend version, you can refer to the [documentation](https://doc.inspireface.online/guides/python-rockchip-device.html) to replace the so and make a Python installation package
