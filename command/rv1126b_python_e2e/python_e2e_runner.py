#!/usr/bin/env python3
"""Minimal RV1126B Python E2E that exercises the generated ctypes bindings."""

import argparse
import ctypes
import json
import os
from pathlib import Path
import sys

HSUCCEED = 0


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--pack", required=True)
    parser.add_argument("--face-image", required=True)
    parser.add_argument("--library", required=True)
    parser.add_argument("--result-path", required=True)
    parser.add_argument(
        "--image-backend",
        choices=("cpu", "rga"),
        default="cpu",
    )
    return parser.parse_args()


def load_native(library: Path) -> object:
    os.environ["INSPIREFACE_LIBRARY_PATH"] = str(library.resolve())
    package_root = Path(__file__).resolve().parent
    sys.path.insert(0, str(package_root))
    from native_pkg import native  # noqa: PLC0415

    return native


def write_result(path: str, document: dict) -> None:
    temporary = path + ".partial"
    with open(temporary, "w", encoding="utf-8") as handle:
        json.dump(document, handle, separators=(",", ":"))
        handle.write("\n")
    os.replace(temporary, path)


def main() -> int:
    args = parse_arguments()
    document = {
        "status": "failure",
        "failure_stage": "load",
        "image_backend": args.image_backend,
        "detected_faces": -1,
        "face_confidence": None,
        "python_version": sys.version,
    }
    session = None
    stream = None
    bitmap = None
    launched = False
    try:
        document["failure_stage"] = "load_library"
        library = Path(args.library).resolve()
        if not library.is_file():
            raise RuntimeError("native library not found: {}".format(library))
        native = load_native(library)

        document["failure_stage"] = "validate_pack"
        pack_info = native.HFResourcePackInfo()
        pack_info.structSize = ctypes.sizeof(pack_info)
        pack_info.structVersion = native.HF_RESOURCE_PACK_INFO_VERSION
        if (
            native.HFValidateResourcePack(
                str(Path(args.pack).resolve()), ctypes.byref(pack_info)
            )
            != HSUCCEED
        ):
            raise RuntimeError("resource pack validation failed")

        document["failure_stage"] = "launch"
        if native.HFLaunchInspireFace(str(Path(args.pack).resolve())) != HSUCCEED:
            raise RuntimeError("launch failed")
        launched = True

        document["failure_stage"] = "select_backend"
        backend = (
            native.HF_IMAGE_PROCESSING_RGA
            if args.image_backend == "rga"
            else native.HF_IMAGE_PROCESSING_CPU
        )
        if args.image_backend == "rga":
            compiled = native.HInt32(0)
            if (
                native.HFQueryExpansiveHardwareRGACompileOption(
                    ctypes.byref(compiled)
                )
                != HSUCCEED
                or compiled.value != 1
            ):
                raise RuntimeError("RGA was not compiled into this SDK")
        if native.HFSwitchImageProcessingBackend(backend) != HSUCCEED:
            raise RuntimeError("failed to select image processing backend")

        document["failure_stage"] = "create_bitmap"
        bitmap = native.HFImageBitmap()
        if (
            native.HFCreateImageBitmapFromFilePath(
                str(Path(args.face_image).resolve()), 3, ctypes.byref(bitmap)
            )
            != HSUCCEED
        ):
            raise RuntimeError("bitmap creation failed")

        document["failure_stage"] = "create_stream"
        stream = native.HFImageStream()
        if (
            native.HFCreateImageStreamFromImageBitmap(
                bitmap, native.HF_CAMERA_ROTATION_0, ctypes.byref(stream)
            )
            != HSUCCEED
        ):
            raise RuntimeError("stream creation failed")

        document["failure_stage"] = "create_session"
        session = native.HFSession()
        parameter = native.HFSessionCustomParameter()
        if (
            native.HFCreateInspireFaceSessionOptional(
                native.HF_ENABLE_NONE,
                native.HF_DETECT_MODE_ALWAYS_DETECT,
                1,
                320,
                -1,
                ctypes.byref(session),
            )
            != HSUCCEED
        ):
            raise RuntimeError("session creation failed")

        document["failure_stage"] = "detect"
        faces = native.HFMultipleFaceData()
        if native.HFExecuteFaceTrack(session, stream, ctypes.byref(faces)) != HSUCCEED:
            raise RuntimeError("face detection failed")

        if faces.detectedNum < 1:
            raise RuntimeError("no face detected")
        document["failure_stage"] = ""
        document["status"] = "success"
        document["detected_faces"] = int(faces.detectedNum)
        document["face_confidence"] = float(faces.detConfidence[0])
        document["bbox"] = [
            float(faces.rects[0].x),
            float(faces.rects[0].y),
            float(faces.rects[0].width),
            float(faces.rects[0].height),
        ]
    except Exception as error:
        document["error"] = str(error)
    finally:
        if session is not None:
            native.HFReleaseInspireFaceSession(session)
        if stream is not None:
            native.HFReleaseImageStream(stream)
        if bitmap is not None:
            native.HFReleaseImageBitmap(bitmap)
        if launched:
            native.HFTerminateInspireFace()

    write_result(args.result_path, document)
    print(json.dumps(document, ensure_ascii=False))
    return 0 if document["status"] == "success" else 1


if __name__ == "__main__":
    raise SystemExit(main())
