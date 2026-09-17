"""Pure, strict mapping from RV1126B conversion records to archive metadata."""

from __future__ import annotations

import math
import pathlib
from collections.abc import Mapping, Sequence
from typing import Any

from command.rv1126b_models.records import EXPECTED_RECORD_IDS


class PackContractError(ValueError):
    """Raised when an inventory contract cannot be represented safely."""


_PIPELINE_IDS = (
    "landmark", "rnet", "recognition", "liveness", "mask", "quality", "emotion", "attitude",
)
_DETECTORS = {
    "scrfd_2_5g": ("scrfd_2_5g_160", "scrfd_2_5g_320", "scrfd_2_5g_640"),
    "scrfd_500m": ("scrfd_500m_160", "scrfd_500m_320", "scrfd_500m_640"),
}
_KEY_BY_ID = {
    "landmark": "landmark", "rnet": "refine_net", "recognition": "feature",
    "liveness": "rgb_anti_spoofing", "mask": "mask_detect", "quality": "pose_quality",
    "emotion": "face_emotion", "attitude": "face_attribute",
}
_SIMILARITY = {"threshold": 0.32, "middle_score": 0.6, "steepness": 10.0, "output_min": 0.02, "output_max": 1.0}
_MODEL_FIELDS = frozenset({
    "name", "fullname", "model_type", "infer_engine", "infer_device", "infer_backend",
    "input_channel", "input_image_channel", "nchw", "swap_color", "data_type",
    "input_tensor_type", "output_tensor_type", "threads", "input_layer", "outputs_layers",
    "input_size", "mean", "norm",
})


def selected_model_ids(detector_family: str = "scrfd_2_5g") -> tuple[str, ...]:
    """Return the explicit, stable resource-pack profile for one detector family."""
    try:
        return _DETECTORS[detector_family] + _PIPELINE_IDS
    except KeyError as error:
        raise PackContractError(f"unsupported detector family: {detector_family}") from error


def _require_record(record: Mapping[str, object], archive_name: str) -> dict:
    try:
        input_spec = record["input"]
        preprocess = record["preprocess"]
        outputs = record["outputs"]
    except KeyError as error:
        raise PackContractError(f"record is missing {error.args[0]}") from error
    if not isinstance(input_spec, Mapping) or not isinstance(preprocess, Mapping) or not isinstance(outputs, list):
        raise PackContractError("record tensor/preprocess contract is malformed")
    layout = input_spec.get("layout")
    shape = input_spec.get("shape")
    if layout not in {"NHWC", "NCHW"}:
        raise PackContractError("input layout cannot be represented by the archive")
    if not isinstance(shape, list) or len(shape) != 4 or not all(type(value) is int and value > 0 for value in shape):
        raise PackContractError("input shape cannot be represented by the archive")
    if shape[0] != 1:
        raise PackContractError("archive image inputs require batch size one")
    height, width, channel = (shape[1], shape[2], shape[3]) if layout == "NHWC" else (shape[2], shape[3], shape[1])
    if channel != 3:
        raise PackContractError("only three-channel image inputs are archive-compatible")
    if input_spec.get("dtype") != "uint8" or not isinstance(input_spec.get("name"), str) or not input_spec["name"]:
        raise PackContractError("input tensor type/name cannot be represented by the archive")
    resize = preprocess.get("resize")
    mean, std = preprocess.get("mean"), preprocess.get("std")
    if not isinstance(resize, Mapping) or resize.get("width") != width or resize.get("height") != height:
        raise PackContractError("input shape and preprocessing resize disagree")
    if preprocess.get("color_order") not in {"BGR", "RGB"}:
        raise PackContractError("preprocessing color order cannot be represented by the archive")
    if not isinstance(mean, list) or not isinstance(std, list) or len(mean) != 3 or len(std) != 3:
        raise PackContractError("preprocessing mean/std cannot be represented by the archive")
    if not all(isinstance(value, (int, float)) and math.isfinite(value) for value in mean + std) or any(value == 0 for value in std):
        raise PackContractError("preprocessing mean/std must be finite with nonzero std")
    if not outputs or not all(isinstance(output, Mapping) for output in outputs):
        raise PackContractError("outputs cannot be represented by the archive")
    names = [output.get("name") for output in outputs]
    if not all(isinstance(name, str) and name for name in names) or len(set(names)) != len(names):
        raise PackContractError("output names cannot be represented by the archive")
    if any(
        output.get("dtype") != "float32"
        or not isinstance(output.get("semantic"), str)
        or not output["semantic"]
        or not isinstance(output.get("shape"), list)
        or not output["shape"]
        or not all(isinstance(value, int) and value > 0 for value in output["shape"])
        for output in outputs
    ):
        raise PackContractError("output tensor type cannot be represented by the archive")
    if not isinstance(archive_name, str) or not archive_name or archive_name == "__inspire__" or pathlib.PurePosixPath(archive_name).name != archive_name or "." in archive_name:
        raise PackContractError("archive member name must be a safe extensionless basename")
    return {
        "name": archive_name,
        "fullname": archive_name,
        "model_type": "RKNN", "infer_engine": "RKNN", "infer_device": "RKNPU", "infer_backend": "RKNPU",
        "input_channel": channel, "input_image_channel": channel, "nchw": layout == "NCHW",
        "swap_color": preprocess["color_order"] == "RGB", "data_type": "image",
        "input_tensor_type": input_spec["dtype"], "output_tensor_type": "float32", "threads": 1,
        "input_layer": input_spec["name"], "outputs_layers": names, "input_size": [width, height],
        # The loader applies (pixel / 255 - mean) / norm. Inventory mean/std are in pixel units.
        "mean": [float(value) / 255.0 for value in mean], "norm": [float(value) / 255.0 for value in std],
    }


def build_manifest(records: Sequence[Mapping[str, object]], artifact_names: Mapping[str, str]) -> dict:
    """Build the default archive manifest without inferring any model contract."""
    expected_ids = selected_model_ids()
    by_id: dict[str, Mapping[str, object]] = {}
    for record in records:
        if not isinstance(record, Mapping) or not isinstance(record.get("id"), str):
            raise PackContractError("record is missing a stable ID")
        model_id = record["id"]
        if model_id in by_id:
            raise PackContractError(f"duplicate record ID: {model_id}")
        by_id[model_id] = record
    unknown = set(by_id).difference(EXPECTED_RECORD_IDS)
    if unknown:
        raise PackContractError(f"extra record ID: {sorted(unknown)[0]}")
    missing = set(expected_ids).difference(by_id)
    if missing:
        raise PackContractError(f"selected IDs are missing: {sorted(missing)[0]}")
    if set(artifact_names) != set(expected_ids):
        raise PackContractError("artifact names must cover exactly the selected IDs")
    if not all(isinstance(name, str) for name in artifact_names.values()):
        raise PackContractError("archive member names must be strings")
    if len(set(artifact_names.values())) != len(artifact_names):
        raise PackContractError("archive member names must be unique")

    detector_ids = expected_ids[:3]
    sections = {model_id: _require_record(by_id[model_id], artifact_names[model_id]) for model_id in expected_ids}
    manifest: dict[str, Any] = {
        "tag": "Gundam_RV1126B", "version": "4.0", "major": "t4",
        "similarity_converter": dict(_SIMILARITY),
        "face_detect_pixel_list": [sections[model_id]["input_size"][0] for model_id in detector_ids],
        "face_detect_model_list": [f"face_detect_{sections[model_id]['input_size'][0]}" for model_id in detector_ids],
    }
    for model_id in expected_ids:
        key = _KEY_BY_ID.get(model_id)
        if key is None:
            key = f"face_detect_{sections[model_id]['input_size'][0]}"
        manifest[key] = sections[model_id]
    validate_manifest(manifest)
    return manifest


def validate_manifest(manifest: Mapping[str, object]) -> None:
    """Reject manifest structures that the current archive loader cannot consume safely."""
    if not isinstance(manifest, Mapping) or manifest.get("tag") != "Gundam_RV1126B" or manifest.get("version") != "4.0" or manifest.get("major") != "t4":
        raise PackContractError("manifest identity is invalid")
    if manifest.get("similarity_converter") != _SIMILARITY:
        raise PackContractError("manifest similarity converter is invalid")
    pixels, detector_keys = manifest.get("face_detect_pixel_list"), manifest.get("face_detect_model_list")
    if pixels != [160, 320, 640] or detector_keys != ["face_detect_160", "face_detect_320", "face_detect_640"]:
        raise PackContractError("manifest detector profile is invalid")
    model_keys = set(detector_keys) | set(_KEY_BY_ID.values())
    if set(manifest).difference({"tag", "version", "major", "similarity_converter", "face_detect_pixel_list", "face_detect_model_list", *model_keys}):
        raise PackContractError("manifest contains unsupported fields")
    names = []
    for key in model_keys:
        section = manifest.get(key)
        if not isinstance(section, Mapping) or set(section) != _MODEL_FIELDS:
            raise PackContractError(f"manifest model section is invalid: {key}")
        if section["model_type"] != "RKNN" or section["infer_engine"] != "RKNN" or section["infer_device"] != "RKNPU" or section["infer_backend"] != "RKNPU":
            raise PackContractError(f"manifest inference contract is invalid: {key}")
        if section["input_tensor_type"] != "uint8" or section["output_tensor_type"] != "float32" or not isinstance(section["nchw"], bool) or not isinstance(section["swap_color"], bool):
            raise PackContractError(f"manifest tensor contract is invalid: {key}")
        input_size = section["input_size"]
        if not isinstance(input_size, list) or len(input_size) != 2 or not all(type(value) is int and value > 0 for value in input_size):
            raise PackContractError(f"manifest input_size must contain two positive integers: {key}")
        outputs_layers = section["outputs_layers"]
        if not isinstance(outputs_layers, list) or not outputs_layers or not all(isinstance(value, str) and value.strip() for value in outputs_layers):
            raise PackContractError(f"manifest outputs_layers must contain nonempty strings: {key}")
        for field in ("mean", "norm"):
            values = section[field]
            if not isinstance(values, list) or len(values) != 3 or not all(type(value) in (int, float) and math.isfinite(value) for value in values):
                raise PackContractError(f"manifest {field} must contain three finite numbers: {key}")
        if any(value == 0 for value in section["norm"]):
            raise PackContractError(f"manifest norm must contain nonzero values: {key}")
        if type(section["threads"]) is not int or section["threads"] <= 0:
            raise PackContractError(f"manifest threads must be a positive integer: {key}")
        if not isinstance(section["input_layer"], str) or not section["input_layer"].strip():
            raise PackContractError(f"manifest input_layer must be a nonempty string: {key}")
        if section["name"] != section["fullname"] or not isinstance(section["name"], str) or not section["name"]:
            raise PackContractError(f"manifest archive member is invalid: {key}")
        if section["name"] == "__inspire__":
            raise PackContractError(f"manifest archive member name is reserved: {key}")
        names.append(section["name"])
    if len(names) != len(set(names)):
        raise PackContractError("manifest archive member names must be unique")
