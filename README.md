# InspireFace
[![GitHub release](https://img.shields.io/github/v/release/HyperInspire/InspireFace.svg?style=for-the-badge&color=blue)](https://github.com/HyperInspire/InspireFace/releases/latest)
[![build](https://img.shields.io/github/actions/workflow/status/HyperInspire/InspireFace/release-sdks.yaml?&style=for-the-badge&label=build)](https://img.shields.io/github/actions/workflow/status/HyperInspire/InspireFace/release-sdks.yaml?&style=for-the-badge&label=build)

InspireFace is a cross-platform face recognition SDK developed in C/C++, supporting multiple operating systems and various backend types for inference, such as CPU, GPU, and NPU.

If you require further information on tracking development branches, CI/CD processes, or downloading pre-compiled libraries, please visit our [development repository](https://github.com/HyperInspire/InspireFace).

Please contact [contact@insightface.ai](mailto:contact@insightface.ai?subject=InspireFace) for commercial support, including obtaining and integrating higher accuracy models, as well as custom development.

## 1. Preparation
### 1.1. Downloading 3rdparty Files

After pulling the project to your local machine, you need to execute the command to fetch or update the submodules, retrieving the latest submodules online to your local environment.

```Bash
# Go to the project root directory
cd InspireFace/
# Pull the submodule project
git submodule update --init --recursive
```

### 1.2. Downloading Pack Files
You can download the pack files containing models and configurations needed for compilation from [Google Drive](https://drive.google.com/drive/folders/1krmv9Pj0XEZXR1GRPHjW_Sl7t4l0dNSS?usp=sharing) and extract them to any location.

### 1.3. Installing OpenCV
If you intend to use the SDK locally or on a server, ensure that OpenCV is installed on the host device beforehand to enable successful linking during the compilation process. For cross-compilation targets like Android or ARM embedded boards, you can use the pre-compiled OpenCV libraries provided by **3rdparty/inspireface-precompile/opencv/**.

### 1.4. Installing MNN
The '3rdparty' directory already includes the MNN library and specifies a particular version as the stable version. If you need to enable or disable additional configuration options during compilation, you can refer to the CMake Options provided by MNN. If you need to use your own precompiled version, feel free to replace it.

### 1.5. Requirements

- CMake (version 3.10 or higher)
- OpenCV (version 4.20 or higher)
    - Use the specific OpenCV-SDK supported by each target platform such as Android, iOS, and Linux.
- NDK (version 16 or higher, only required for Android)
- MNN (version 1.4.0 or higher)
- C++ Compiler
    - Either GCC or Clang can be used (macOS does not require additional installation as Xcode is included)
        - Recommended GCC version is 4.9 or higher
            - Note that in some distributions, GCC (GNU C Compiler) and G++ (GNU C++ Compiler) are installed separately.
            - For instance, on Ubuntu, you need to install both gcc and g++
        - Recommended Clang version is 3.9 or higher
    - arm-linux-gnueabihf (for RV1109/RV1126)
        - Prepare the cross-compilation toolchain in advance, such as gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf
- CUDA (version 10.1 or higher)
    - GPU-based inference requires installing NVIDIA's CUDA dependencies on the device.
- RKNN
    - Adjust and select versions currently supported for specific requirements.

## 2. Compilation
CMake option are used to control the various details of the compilation phase. Please select according to your actual requirements. [Parameter table](doc/CMake-Option.md).

### 2.1. Local Compilation
Make sure OpenCV is installed, you can begin the compilation process.  If you are using macOS or Linux, you can quickly compile using the shell scripts provided in the **command/** folder at the project root:
```bash
cd InspireFace/
# Execute the local compilation script
bash command/build.sh
```
After compilation, you can find the local file in the build directory, which contains the compilation results. The install directory structure is as follows:
```bash
inspireface-linux
   ├── include
   │   ├── herror.h
   │   └── inspireface.h
   └── lib
       └── libInspireFace.so
```

- **libInspireFace.so**：Compiled dynamic linking library.
- **inspireface.h**：Header file definition.
- **herror.h**：Reference error number definition.
### 2.2. Cross Compilation
Cross compilation requires you to prepare the target platform's cross-compilation toolchain on the host machine in advance. Here, compiling for Rockchip's embedded devices RV1109/RV1126 is used as an example:
```bash
# Set the path for the cross-compilation toolchain
export ARM_CROSS_COMPILE_TOOLCHAIN=YOUR_DIR/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf
# Execute the cross-compilation script for RV1109/RV1126
bash command/build_cross_rv1109rv1126_armhf.sh
```
After the compilation is complete, you can find the compiled results in the **build/inspireface-linux-armv7-rv1109rv1126-armhf** directory.
### 2.3. Supported Platforms and Architectures
We have completed the adaptation and testing of the software across various operating systems and CPU architectures. This includes compatibility verification for platforms such as Linux, macOS, iOS, and Android, as well as testing for specific hardware support to ensure stable operation in diverse environments.

| **No.** | **Operating System** | **CPU Architecture** | **Special Device Support** | **Adapted** | **Passed Tests** |
| ------- | -------------------- | --------------------- | -------------------------- | ----------- | ---------------- |
| 1       | **Linux**            | ARMv7                 | -                          | [![build](https://img.shields.io/github/actions/workflow/status/HyperInspire/InspireFace/release-sdks.yaml?&style=for-the-badge&label=build)](https://github.com/HyperInspire/InspireFace/actions/workflows/release-sdks.yaml) | ![test](https://img.shields.io/badge/OFFLINE-PASSING-blue?style=for-the-badge) |
| 2       |                      | ARMv8                 | -                          | [![build](https://img.shields.io/github/actions/workflow/status/HyperInspire/InspireFace/release-sdks.yaml?&style=for-the-badge&label=build)](https://github.com/HyperInspire/InspireFace/actions/workflows/release-sdks.yaml) | ![test](https://img.shields.io/badge/OFFLINE-PASSING-blue?style=for-the-badge) |
| 3       |                      | x86/x86_64            | -                          | [![build](https://img.shields.io/github/actions/workflow/status/HyperInspire/InspireFace/release-sdks.yaml?&style=for-the-badge&label=build)](https://github.com/HyperInspire/InspireFace/actions/workflows/release-sdks.yaml) | [![test](https://img.shields.io/github/actions/workflow/status/HyperInspire/InspireFace/test_ubuntu_x86_Pikachu.yaml?style=for-the-badge&label=Test&color=blue)](https://github.com/HyperInspire/InspireFace/actions/workflows/test_ubuntu_x86_Pikachu.yaml) |
| 4       |                      | ARMv7                 | RV1109RV1126               | [![build](https://img.shields.io/github/actions/workflow/status/HyperInspire/InspireFace/release-sdks.yaml?&style=for-the-badge&label=build)](https://github.com/HyperInspire/InspireFace/actions/workflows/release-sdks.yaml) | ![test](https://img.shields.io/badge/OFFLINE-PASSING-blue?style=for-the-badge) |
| 5       |                      | x86/x86_64            | CUDA                       | ![build](https://img.shields.io/badge/OFFLINE-PASSING-green?style=for-the-badge) |  |
| 6       | **macOS**            | Intel x86             | -                          | ![build](https://img.shields.io/badge/OFFLINE-PASSING-green?style=for-the-badge) | ![test](https://img.shields.io/badge/OFFLINE-PASSING-blue?style=for-the-badge) |
| 7       |                      | Apple Silicon         | -                          | ![build](https://img.shields.io/badge/OFFLINE-PASSING-green?style=for-the-badge) | ![test](https://img.shields.io/badge/OFFLINE-PASSING-blue?style=for-the-badge) |
| 8       | **iOS**              | ARM                   | -                          |  |  |
| 9       | **Android**          | ARMv7                 | -                          |  |  |
| 10      |                      | ARMv8                 | -                          |  |  |

- Complete compilation scripts and successful compilation.
- Pass unit tests on physical devices.
- Meet all performance benchmarks in tests.

#### 2.4. Multi-platform compilation using Docker

We offer a method for rapid multi-platform compilation using Docker, provided that Docker is installed beforehand, and the appropriate commands are executed:
```Bash
# Build x86 Ubuntu18.04
docker-compose up build-ubuntu18

# Build armv7 cross-compile
build-cross-armv7-armhf

# Build armv7 with support RV1109RV1126 device NPU cross-complie
docker-compose up build-cross-rv1109rv1126-armhf

# Build all
docker-compose up
```

## 3. Example
### 3.1. C/C++ Sample
To integrate InspireFace into a C/C++ project, you simply need to link the InspireFace library and include the appropriate header files. Below is a basic example demonstrating face detection:

```cpp
HResult ret;
// The resource file must be loaded before it can be used
ret = HFLaunchInspireFace(packPath);
if (ret != HSUCCEED) {
    std::cout << "Load Resource error: " << ret << std::endl;
    return ret;
}

// Enable the functions in the pipeline: mask detection, live detection, and face quality detection
HOption option = HF_ENABLE_QUALITY | HF_ENABLE_MASK_DETECT | HF_ENABLE_LIVENESS;
// Non-video or frame sequence mode uses IMAGE-MODE, which is always face detection without tracking
HFDetectMode detMode = HF_DETECT_MODE_IMAGE;
// Maximum number of faces detected
HInt32 maxDetectNum = 5;
// Handle of the current face SDK algorithm context
HFSession session = {0};
ret = HFCreateInspireFaceSessionOptional(option, detMode, maxDetectNum, &session);
if (ret != HSUCCEED) {
    std::cout << "Create FaceContext error: " << ret << std::endl;
    return ret;
}

// Load a image
cv::Mat image = cv::imread(sourcePath);
if (image.empty()) {
    std::cout << "The source entered is not a picture or read error." << std::endl;
    return 1;
}
// Prepare an image parameter structure for configuration
HFImageData imageParam = {0};
imageParam.data = image.data;       // Data buffer
imageParam.width = image.cols;      // Target view width
imageParam.height = image.rows;      // Target view width
imageParam.rotation = HF_CAMERA_ROTATION_0;      // Data source rotate
imageParam.format = HF_STREAM_BGR;      // Data source format

// Create an image data stream
HFImageStream imageHandle = {0};
ret = HFCreateImageStream(&imageParam, &imageHandle);
if (ret != HSUCCEED) {
    std::cout << "Create ImageStream error: " << ret << std::endl;
    return ret;
}

// Execute HF_FaceContextRunFaceTrack captures face information in an image
HFMultipleFaceData multipleFaceData = {0};
ret = HFExecuteFaceTrack(session, imageHandle, &multipleFaceData);
if (ret != HSUCCEED) {
    std::cout << "Execute HFExecuteFaceTrack error: " << ret << std::endl;
    return ret;
}
// Print the number of faces detected
auto faceNum = multipleFaceData.detectedNum;
std::cout << "Num of face: " << faceNum << std::endl;

ret = HFReleaseImageStream(imageHandle);
if (ret != HSUCCEED) {
    printf("Release image stream error: %lu\n", ret);
}
// The memory must be freed at the end of the program
ret = HFReleaseInspireFaceSession(session);
if (ret != HSUCCEED) {
    printf("Release session error: %lu\n", ret);
    return ret;
}
```
For more examples, you can refer to the **cpp/sample** sub-project located in the root directory. You can compile these sample executables by enabling the **ISF_BUILD_WITH_SAMPLE** option during the compilation process.

**Note**: For each error code feedback, you can click on this [link](doc/Error-Feedback-Codes.md) to view detailed explanations.

### 3.2. Python Native Sample
We provide a Python API that allows for more efficient use of the InspireFace library. After compiling the dynamic link library, you need to either symlink or copy it to the **python/inspireface/modules/core** directory within the root directory. You can then start testing by navigating to the **[python/](python/)** directory. Your Python environment will need to have some dependencies installed:

- python >= 3.7
- opencv-python
- loguru
- tqdm
- numpy
- ctypes
```bash
# Use a symbolic link
ln -s YOUR_BUILD_DIR/install/InspireFace/lib/libInspireFace.so python/inspireface/modules/core
# Navigate to the sub-project directory
cd python

```

Import inspireface for a quick facial detection example:
```python
import cv2
import inspireface as ifac
from inspireface.param import *

# Step 1: Initialize the SDK and load the algorithm resource files.
resource_path = "pack/Pikachu"
ret = ifac.launch(resource_path)
assert ret, "Launch failure. Please ensure the resource path is correct."

# Optional features, loaded during session creation based on the modules specified.
opt = HF_ENABLE_NONE
session = ifac.InspireFaceSession(opt, HF_DETECT_MODE_IMAGE)

# Load the image using OpenCV.
image = cv2.imread(image_path)
assert image is not None, "Please check that the image path is correct."

# Perform face detection on the image.
faces = session.face_detection(image)
print(f"face detection: {len(faces)} found")

# Copy the image for drawing the bounding boxes.
draw = image.copy()
for idx, face in enumerate(faces):
    print(f"{'==' * 20}")
    print(f"idx: {idx}")
    # Print Euler angles of the face.
    print(f"roll: {face.roll}, yaw: {face.yaw}, pitch: {face.pitch}")
    # Draw bounding box around the detected face.
    x1, y1, x2, y2 = face.location
    cv2.rectangle(draw, (x1, y1), (x2, y2), (0, 0, 255), 2)
```
In the project, more usage examples are provided:

- sample_face_detection.py: Facial detection example
- sample_face_recognition.py: Facial recognition example
- sample_face_track_from_video.py: Facial tracking from video stream example

## 4. Test
In the project, there is a subproject called cpp/test. To compile it, you need to enable the ISF_BUILD_WITH_TEST switch, which will allow you to compile executable programs for testing.

```bash
cmake -DISF_BUILD_WITH_TEST=ON ..
```
If you need to run test cases, you will need to download the required [resource files](https://drive.google.com/file/d/1i4uC-dZTQxdVgn2rP0ZdfJTMkJIXgYY4/view?usp=sharing), which are **test_res** and **pack** respectively. Unzip the pack file into the test_res folder. The directory structure of test_res should be prepared as follows before testing:

```bash

test_res
├── data
├── images
├── pack		<- unzip pack.zip
├── save
├── valid_lfw_funneled.txt
├── video
└── video_frames

```
After compilation, you can find the executable program "Test" in **YOUR_BUILD_FOLDER/test**. The program accepts two optional parameters:

- **test_dir**：Path to the test resource files
- **pack**：Name of the model to be tested
```bash
./Test --test_dir PATH/test_res --pack Pikachu
```
During the process of building the test program using CMake, it will involve selecting CMake parameters. For specific details, you can refer to the parameter configuration table.

**Note**: If you want to view the benchmark test report, you can click on the [link](doc/Benchmark-Remark(Updating).md).

## 5. Function Support
The following functionalities and technologies are currently supported.

| Index | Function | Adaptation | Note |
| -- | --- | --- | --- |
| 1 | Face Detection | ![Static Badge](https://img.shields.io/badge/STABLE-blue?style=for-the-badge) | SCRFD |
| 2 | Facial Landmark Detection | ![Static Badge](https://img.shields.io/badge/STABLE-blue?style=for-the-badge) | HyperLandmark |
| 3 | Face Recognition | ![Static Badge](https://img.shields.io/badge/STABLE-blue?style=for-the-badge) | ArcFace |
| 4 | Face Tracking | ![Static Badge](https://img.shields.io/badge/STABLE-blue?style=for-the-badge) |  |
| 5 | Mask Detection | ![Static Badge](https://img.shields.io/badge/STABLE-blue?style=for-the-badge) |  |
| 6 | Silent Liveness Detection | ![Static Badge](https://img.shields.io/badge/STABLE-blue?style=for-the-badge) | MiniVision |
| 7 | Face Quality Detection | ![Static Badge](https://img.shields.io/badge/STABLE-blue?style=for-the-badge) |  |
| 8 | Face Pose Estimation | ![Static Badge](https://img.shields.io/badge/STABLE-blue?style=for-the-badge) |  |
| 9 | Age Prediction | ![Static Badge](https://img.shields.io/badge/PENDING-yellow?style=for-the-badge) |  |
| 10 | Cooperative Liveness Detection | ![Static Badge](https://img.shields.io/badge/PENDING-yellow?style=for-the-badge) |  |


## 6. Models Pack List

For different scenarios, we currently provide several Packs, each containing multiple models and configurations.

| Name | Supported Devices | Note | Link |
| --- | --- | --- | --- |
| Pikachu | CPU | Lightweight edge-side model | [GDrive](https://drive.google.com/file/d/1i4uC-dZTQxdVgn2rP0ZdfJTMkJIXgYY4/view?usp=drive_link) |
| Megatron | CPU, GPU | Local or server-side model | [GDrive](https://drive.google.com/file/d/1i4uC-dZTQxdVgn2rP0ZdfJTMkJIXgYY4/view?usp=drive_link) |
| Gundam-RV1109 | RKNPU | Supports RK1109 and RK1126 | [GDrive](https://drive.google.com/file/d/1i4uC-dZTQxdVgn2rP0ZdfJTMkJIXgYY4/view?usp=drive_link) |

