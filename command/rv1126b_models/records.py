"""Strict validation for immutable RV1126B model conversion records."""

import json
import pathlib
import re
from typing import Any


CALIBRATION_STATUSES = frozenset({"verified", "provisional"})
TOOLKIT_VERSION = "2.3.2"
EXPECTED_STATUS_BY_FAMILY = {
    "attitude": "verified",
    "emotion": "verified",
    "landmark": "verified",
    "liveness": "provisional",
    "mask": "provisional",
    "quality": "provisional",
    "recognition": "provisional",
    "rnet": "provisional",
    "scrfd": "provisional",
}
REQUIRED_SCRFD_IDS = frozenset(
    {
        "scrfd_500m_160",
        "scrfd_500m_192",
        "scrfd_500m_256",
        "scrfd_500m_320",
        "scrfd_500m_640",
        "scrfd_2_5g_160",
        "scrfd_2_5g_192",
        "scrfd_2_5g_256",
        "scrfd_2_5g_320",
        "scrfd_2_5g_640",
    }
)
EXPECTED_RECORD_IDS = REQUIRED_SCRFD_IDS | frozenset(
    {"attitude", "emotion", "liveness", "mask", "quality", "recognition", "landmark", "rnet"}
)
REQUIRED_TOP_LEVEL_FIELDS = frozenset(
    {
        "id",
        "family",
        "source",
        "source_sha256",
        "output",
        "target",
        "toolkit_version",
        "input",
        "outputs",
        "preprocess",
        "calibration",
        "calibration_status",
    }
)
REQUIRED_PREPROCESS_FIELDS = frozenset({"color_order", "mean", "std", "resize", "crop"})
SHA256_RE = re.compile(r"^[0-9a-f]{64}$")


def _require_mapping(value: Any, field: str) -> dict:
    if not isinstance(value, dict):
        raise ValueError(f"{field} must be an object")
    return value


def _require_nonempty_string(value: Any, field: str) -> str:
    if not isinstance(value, str) or not value.strip():
        raise ValueError(f"{field} must be a non-empty string")
    return value


def _require_relative_path(value: Any, field: str, suffix: str) -> str:
    raw_path = _require_nonempty_string(value, field)
    path = pathlib.PurePosixPath(raw_path)
    windows_path = pathlib.PureWindowsPath(raw_path)
    if (
        path.is_absolute()
        or windows_path.is_absolute()
        or windows_path.drive
        or ".." in path.parts
        or "\\" in raw_path
        or path.suffix != suffix
    ):
        raise ValueError(f"{field} must be a relative {suffix} path")
    return str(path)


def _validate_input(record: dict) -> None:
    input_record = _require_mapping(record["input"], "input")
    for field in ("name", "shape", "layout", "dtype"):
        if field not in input_record:
            raise ValueError(f"input.{field} is required")
    _require_nonempty_string(input_record["name"], "input.name")
    shape = input_record["shape"]
    if not isinstance(shape, list) or len(shape) != 4 or not all(isinstance(item, int) and item > 0 for item in shape):
        raise ValueError("input.shape must contain four positive integers")
    if input_record["layout"] not in {"NHWC", "NCHW"}:
        raise ValueError("input.layout must be NHWC or NCHW")
    _require_nonempty_string(input_record["dtype"], "input.dtype")


def _validate_outputs(record: dict) -> None:
    outputs = record["outputs"]
    if not isinstance(outputs, list) or not outputs:
        raise ValueError("outputs must be a non-empty list")
    seen_names = set()
    for index, output in enumerate(outputs):
        prefix = f"outputs[{index}]"
        output = _require_mapping(output, prefix)
        for field in ("name", "shape", "dtype", "semantic"):
            if field not in output:
                raise ValueError(f"{prefix}.{field} is required")
        name = _require_nonempty_string(output["name"], f"{prefix}.name")
        if name in seen_names:
            raise ValueError(f"outputs contains duplicate name: {name}")
        seen_names.add(name)
        shape = output["shape"]
        if not isinstance(shape, list) or not shape or not all(isinstance(item, int) and item > 0 for item in shape):
            raise ValueError(f"{prefix}.shape must contain positive integers")
        _require_nonempty_string(output["dtype"], f"{prefix}.dtype")
        _require_nonempty_string(output["semantic"], f"{prefix}.semantic")


def _validate_preprocess(record: dict) -> None:
    preprocess = _require_mapping(record["preprocess"], "preprocess")
    missing = REQUIRED_PREPROCESS_FIELDS.difference(preprocess)
    if missing:
        raise ValueError(f"preprocess.{sorted(missing)[0]} is required")
    if preprocess["color_order"] not in {"BGR", "RGB"}:
        raise ValueError("preprocess.color_order must be BGR or RGB")
    for field in ("mean", "std"):
        values = preprocess[field]
        if not isinstance(values, list) or len(values) != 3 or not all(isinstance(item, (int, float)) for item in values):
            raise ValueError(f"preprocess.{field} must contain three numeric values")
    resize = _require_mapping(preprocess["resize"], "preprocess.resize")
    for field in ("width", "height"):
        if not isinstance(resize.get(field), int) or resize[field] <= 0:
            raise ValueError(f"preprocess.resize.{field} must be a positive integer")
    _require_nonempty_string(preprocess["crop"], "preprocess.crop")


def _validate_calibration(record: dict) -> None:
    calibration = _require_mapping(record["calibration"], "calibration")
    for field in ("path", "reason"):
        if field not in calibration:
            raise ValueError(f"calibration.{field} is required")
    _require_relative_path(calibration["path"], "calibration.path", ".txt")
    _require_nonempty_string(calibration["reason"], "calibration.reason")
    if record["calibration_status"] not in CALIBRATION_STATUSES:
        raise ValueError("calibration_status must be verified or provisional")
    expected_status = EXPECTED_STATUS_BY_FAMILY.get(record["family"])
    if expected_status is None:
        raise ValueError(f"family is not supported by this inventory: {record['family']}")
    if record["calibration_status"] != expected_status:
        raise ValueError(
            f"calibration_status for {record['family']} must be {expected_status}"
        )


def validate_record(record: dict) -> None:
    """Raise ValueError unless *record* fully declares one model's contract."""
    if not isinstance(record, dict):
        raise ValueError("record must be an object")
    missing = REQUIRED_TOP_LEVEL_FIELDS.difference(record)
    if missing:
        raise ValueError(f"record is missing required field: {sorted(missing)[0]}")
    _require_nonempty_string(record["id"], "id")
    _require_nonempty_string(record["family"], "family")
    _require_relative_path(record["source"], "source", ".onnx")
    if not isinstance(record["source_sha256"], str) or not SHA256_RE.fullmatch(record["source_sha256"]):
        raise ValueError("source_sha256 must be a lowercase SHA-256 digest")
    _require_relative_path(record["output"], "output", ".rknn")
    if record["target"] != "rv1126b":
        raise ValueError("target must be rv1126b")
    if record["toolkit_version"] != TOOLKIT_VERSION:
        raise ValueError(f"toolkit_version must be {TOOLKIT_VERSION}")
    _validate_input(record)
    _validate_outputs(record)
    _validate_preprocess(record)
    _validate_calibration(record)


def load_inventory(path: pathlib.Path) -> list[dict]:
    """Load and strictly validate the immutable RV1126B model inventory."""
    try:
        with pathlib.Path(path).open(encoding="utf-8") as handle:
            records = json.load(handle)
    except json.JSONDecodeError as error:
        raise ValueError(f"invalid inventory JSON: {error}") from error
    if not isinstance(records, list):
        raise ValueError("inventory must be a list")
    if len(records) != 18:
        raise ValueError("inventory must contain exactly 18 records")
    ids = set()
    for record in records:
        validate_record(record)
        if record["id"] in ids:
            raise ValueError(f"inventory contains duplicate id: {record['id']}")
        ids.add(record["id"])
    scrfd_ids = {record["id"] for record in records if record["family"] == "scrfd"}
    if scrfd_ids != REQUIRED_SCRFD_IDS:
        raise ValueError("inventory SCRFD IDs must match the required stable set")
    if ids != EXPECTED_RECORD_IDS:
        raise ValueError("inventory IDs must match the required 18-model set")
    return records
