"""Create the sole raw UINT8/NHWC input accepted by the RKNN2 parity runner.

Unlike the conversion-era board bytes, this deliberately stops after resize and
declared color conversion.  RKNN's binding performs normalization/quantization
with ``pass_through=0`` for both the reference and Nano paths.
"""

from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import sys

import numpy as np
from PIL import Image


def _sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _load_record(path: Path) -> dict:
    record = json.loads(path.read_text(encoding="utf-8-sig"))
    try:
        input_spec, preprocess = record["input"], record["preprocess"]
        shape, resize = input_spec["shape"], preprocess["resize"]
    except (KeyError, TypeError) as error:
        raise ValueError("conversion record lacks image contract") from error
    if (
        input_spec.get("dtype") != "uint8" or input_spec.get("layout") != "NHWC"
        or not isinstance(shape, list) or len(shape) != 4 or shape[0] != 1 or shape[3] != 3
        or not all(type(value) is int and value > 0 for value in shape)
        or resize.get("width") != shape[2] or resize.get("height") != shape[1]
        or preprocess.get("color_order") not in {"BGR", "RGB"}
    ):
        raise ValueError("record is not an unambiguous uint8 NHWC image contract")
    return record


def build_raw_input(record_path: Path, image_path: Path, destination: Path) -> dict:
    """Write raw resized/color-ordered uint8 bytes and self-verifying provenance."""
    record = _load_record(record_path)
    input_spec, preprocess = record["input"], record["preprocess"]
    resize = preprocess["resize"]
    try:
        with Image.open(image_path) as source:
            # Pillow exposes decoded pixels as RGB.  Convert only after the
            # deterministic bilinear resize, so provenance names the exact stage.
            image = source.convert("RGB").resize((resize["width"], resize["height"]), Image.Resampling.BILINEAR)
    except (OSError, ValueError) as error:
        raise ValueError(f"unreadable source image: {image_path}") from error
    image = np.asarray(image)
    if preprocess["color_order"] == "BGR":
        image = image[:, :, ::-1]
    values = np.ascontiguousarray(image, dtype=np.uint8)
    expected = tuple(input_spec["shape"][1:])
    if values.shape != expected:
        raise ValueError("raw image dimensions do not match declared NHWC contract")
    payload = values.tobytes()
    destination.mkdir(parents=True, exist_ok=True)
    input_path = destination / "input_0.bin"
    partial = destination / "input_0.partial"
    partial.write_bytes(payload)
    os.replace(partial, input_path)
    provenance = {
        "schema_version": 1,
        "model_id": record["model_id"],
        "source_image": str(image_path),
        "source_image_sha256": _sha256(image_path.read_bytes()),
        "input_sha256": _sha256(payload),
        "dtype": "uint8",
        "layout": "NHWC",
        "shape": input_spec["shape"],
        "color_order": preprocess["color_order"],
        "resize": {"width": resize["width"], "height": resize["height"]},
        "preprocess_stage": "raw_uint8_after_resize_and_color",
        "pass_through": 0,
    }
    provenance_path = destination / "input_provenance.json"
    provenance_partial = destination / "input_provenance.partial"
    provenance_partial.write_text(json.dumps(provenance, sort_keys=True, indent=2) + "\n", encoding="utf-8")
    os.replace(provenance_partial, provenance_path)
    return provenance


def main() -> int:
    if len(sys.argv) != 4:
        raise SystemExit("usage: prepare_raw_inputs.py SIDECAR IMAGE DESTINATION")
    build_raw_input(*(Path(value) for value in sys.argv[1:]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
