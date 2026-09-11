"""Prepare independent calibration inputs for RV1126B model conversions.

Verified model records keep their supplied image lists.  Provisional records
use deterministic, model-specific image material generated from available
source images; numeric normalization remains the conversion tool's job.
"""

from __future__ import annotations

import hashlib
import json
import pathlib
from typing import Iterable

IMAGE_SUFFIXES = frozenset({".bmp", ".jpeg", ".jpg", ".png", ".webp"})
SPATIAL_PREPARATION_BY_MODEL = {
    "liveness": (
        "caller_supplied_face_crop; raw pixels required by original converter",
        "caller_supplied_face_crop_resize",
    ),
    "mask": (
        "caller_supplied_face_crop; convert BGR to RGB",
        "caller_supplied_face_crop_resize",
    ),
    "quality": (
        "caller_supplied_face_crop; convert BGR to RGB; raw pixels required by original converter",
        "caller_supplied_face_crop_resize",
    ),
    "recognition": (
        "aligned face crop; convert BGR to RGB; original converter sets quant_img_RGB2BGR",
        "aligned_face_crop_resize",
    ),
    "rnet": ("candidate face crop", "candidate_face_crop_resize"),
    "scrfd_500m_160": (
        "full frame resize; original list converter sets quant_img_RGB2BGR",
        "full_frame_resize",
    ),
    "scrfd_500m_192": (
        "full frame resize; original list converter sets quant_img_RGB2BGR",
        "full_frame_resize",
    ),
    "scrfd_500m_256": (
        "full frame resize; original list converter sets quant_img_RGB2BGR",
        "full_frame_resize",
    ),
    "scrfd_500m_320": (
        "full frame resize; original list converter sets quant_img_RGB2BGR",
        "full_frame_resize",
    ),
    "scrfd_500m_640": (
        "full frame resize; original list converter sets quant_img_RGB2BGR",
        "full_frame_resize",
    ),
    "scrfd_2_5g_160": (
        "full frame resize; original list converter sets quant_img_RGB2BGR",
        "full_frame_resize",
    ),
    "scrfd_2_5g_192": (
        "full frame resize; original list converter sets quant_img_RGB2BGR",
        "full_frame_resize",
    ),
    "scrfd_2_5g_256": (
        "full frame resize; original list converter sets quant_img_RGB2BGR",
        "full_frame_resize",
    ),
    "scrfd_2_5g_320": (
        "full frame resize; original list converter sets quant_img_RGB2BGR",
        "full_frame_resize",
    ),
    "scrfd_2_5g_640": (
        "full frame resize; original list converter sets quant_img_RGB2BGR",
        "full_frame_resize",
    ),
}


def _sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _input_dimensions(record: dict) -> tuple[int, int]:
    shape = record["input"]["shape"]
    if record["input"]["layout"] == "NHWC":
        return shape[2], shape[1]
    return shape[3], shape[2]


def _artifact_directory(record: dict, artifact_root: pathlib.Path) -> pathlib.Path:
    return artifact_root.resolve() / record["id"]


def _write_dataset(dataset: pathlib.Path, images: Iterable[pathlib.Path]) -> pathlib.Path:
    dataset.parent.mkdir(parents=True, exist_ok=True)
    dataset.write_text(
        "".join(f"{image.resolve()}\n" for image in images),
        encoding="utf-8",
    )
    return dataset.resolve()


def _write_manifest(directory: pathlib.Path, contents: dict) -> None:
    (directory / "calibration.json").write_text(
        json.dumps(contents, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )


def _read_verified_list(record: dict, source_root: pathlib.Path) -> list[pathlib.Path]:
    original_list = source_root / pathlib.PurePosixPath(record["calibration"]["path"])
    if not original_list.is_file():
        raise FileNotFoundError(f"verified calibration list is missing: {original_list}")
    entries = [line.strip() for line in original_list.read_text(encoding="utf-8").splitlines() if line.strip()]
    if not entries:
        raise ValueError(f"verified calibration list is empty: {original_list}")

    images = []
    for entry in entries:
        image = pathlib.Path(entry)
        if not image.is_absolute():
            image = original_list.parent / image
        if not image.is_file():
            raise FileNotFoundError(f"verified calibration image is missing: {entry}")
        images.append(image.resolve())
    return images


def _candidate_images(source_root: pathlib.Path, artifact_root: pathlib.Path) -> list[tuple[pathlib.Path, str]]:
    source_root = source_root.resolve()
    artifact_root = artifact_root.resolve()
    candidates = []
    for path in source_root.rglob("*"):
        if not path.is_file() or path.suffix.lower() not in IMAGE_SUFFIXES:
            continue
        try:
            path.resolve().relative_to(artifact_root)
            continue
        except ValueError:
            pass
        candidates.append((path.resolve(), _sha256(path)))
    candidates.sort(key=lambda item: (item[0].relative_to(source_root).as_posix().casefold(), item[1]))
    return candidates


def _spatial_operation(record: dict) -> str:
    try:
        declared_crop, operation = SPATIAL_PREPARATION_BY_MODEL[record["id"]]
    except KeyError as error:
        raise ValueError(f"unknown spatial preparation for provisional model: {record['id']}") from error
    if record["preprocess"]["crop"] != declared_crop:
        raise ValueError(f"unknown spatial preparation for provisional model: {record['id']}")
    return operation


def _prepare_image(source: pathlib.Path, destination: pathlib.Path, width: int, height: int, spatial_operation: str) -> None:
    import cv2

    if spatial_operation not in {operation for _, operation in SPATIAL_PREPARATION_BY_MODEL.values()}:
        raise ValueError(f"unknown spatial preparation: {spatial_operation}")
    # OpenCV honors EXIF orientation on ordinary color reads. Its BGR decode
    # and encode preserve file colors; Toolkit owns numeric/color preprocessing.
    image = cv2.imread(str(source), cv2.IMREAD_COLOR)
    if image is None:
        raise ValueError(f"cannot read calibration image: {source}")
    prepared = cv2.resize(image, (width, height), interpolation=cv2.INTER_LINEAR)
    if not cv2.imwrite(str(destination), prepared):
        raise ValueError(f"cannot write calibration image: {destination}")


def _prepare_verified(record: dict, source_root: pathlib.Path, artifact_directory: pathlib.Path) -> pathlib.Path:
    images = _read_verified_list(record, source_root)
    dataset = _write_dataset(artifact_directory / "dataset.txt", images)
    width, height = _input_dimensions(record)
    _write_manifest(
        artifact_directory,
        {
            "input_dimensions": {"height": height, "width": width},
            "model_id": record["id"],
            "original_list": record["calibration"]["path"],
            "source_checksums": {
                str(image.relative_to(source_root.resolve()).as_posix()): _sha256(image) for image in images
            },
            "spatial_operation": record["preprocess"]["crop"],
            "status": "verified",
        },
    )
    return dataset


def _prepare_provisional(record: dict, source_root: pathlib.Path, artifact_directory: pathlib.Path) -> pathlib.Path:
    candidates = _candidate_images(source_root, artifact_directory.parent)
    if not candidates:
        raise ValueError(f"no source images available for provisional calibration: {record['id']}")
    width, height = _input_dimensions(record)
    spatial_operation = _spatial_operation(record)
    generated_directory = artifact_directory / "generated"
    generated_directory.mkdir(parents=True, exist_ok=True)
    generated = []
    source_checksums = {}
    generated_checksums = {}
    for index, (source, source_checksum) in enumerate(candidates):
        generated_image = generated_directory / f"{index:04d}.png"
        _prepare_image(source, generated_image, width, height, spatial_operation)
        generated.append(generated_image)
        relative_source = source.relative_to(source_root.resolve()).as_posix()
        relative_generated = generated_image.relative_to(artifact_directory).as_posix()
        source_checksums[relative_source] = source_checksum
        generated_checksums[relative_generated] = _sha256(generated_image)
    dataset = _write_dataset(artifact_directory / "dataset.txt", generated)
    _write_manifest(
        artifact_directory,
        {
            "generated_checksums": generated_checksums,
            "input_dimensions": {"height": height, "width": width},
            "model_id": record["id"],
            "original_missing_list": record["calibration"]["path"],
            "source_checksums": source_checksums,
            "spatial_operation": spatial_operation,
            "status": "provisional",
        },
    )
    return dataset


def prepare_calibration(record: dict, source_root: pathlib.Path, artifact_root: pathlib.Path) -> pathlib.Path:
    """Create a model-local absolute dataset list and calibration record."""
    source_root = pathlib.Path(source_root).resolve()
    artifact_directory = _artifact_directory(record, pathlib.Path(artifact_root))
    artifact_directory.mkdir(parents=True, exist_ok=True)
    if record["calibration_status"] == "verified":
        return _prepare_verified(record, source_root, artifact_directory)
    if record["calibration_status"] == "provisional":
        return _prepare_provisional(record, source_root, artifact_directory)
    raise ValueError(f"unsupported calibration status: {record['calibration_status']}")
