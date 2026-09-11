"""Materialize native input bytes from a calibration image and board queries.

Run in the pinned image with /artifacts and /source mounted as for conversion.
The current conversion records describe one image input; reject other records
explicitly. The C++ executable itself accepts arbitrary queried input counts.
"""
import hashlib
import json
from pathlib import Path
import sys
import cv2
import numpy as np


def main():
    record_path, contract_path, destination = map(Path, sys.argv[1:])
    record = json.loads(record_path.read_text(encoding="utf-8-sig"))
    contract = json.loads(contract_path.read_text(encoding="utf-8-sig"))
    if len(contract["inputs"]) != 1:
        raise ValueError("conversion record only declares one image input")
    calibration = Path("/artifacts/calibration") / record["model_id"]
    dataset = calibration / "dataset.txt"
    manifest = calibration / "calibration.json"
    for path, key in ((dataset, "dataset_sha256"), (manifest, "calibration_manifest_sha256")):
        if hashlib.sha256(path.read_bytes()).hexdigest() != record[key]:
            raise ValueError("calibration provenance hash mismatch: " + str(path))
    image_path = Path(dataset.read_text().splitlines()[0])
    resolved = image_path.resolve()
    if not any(root in resolved.parents for root in (Path("/source"), Path("/artifacts/calibration"))):
        raise ValueError("calibration image outside mounted roots")
    image = cv2.imread(str(image_path))
    if image is None:
        raise ValueError("unreadable calibration image")
    preprocess = record["preprocess"]
    size = preprocess["resize"]
    image = cv2.resize(image, (size["width"], size["height"]))
    if preprocess["color_order"] == "RGB":
        image = image[:, :, ::-1]
    elif preprocess["color_order"] != "BGR":
        raise ValueError("unsupported color order")
    values = (image.astype(np.float32) - np.array(preprocess["mean"], dtype=np.float32)) / np.array(preprocess["std"], dtype=np.float32)
    attr = contract["inputs"][0]
    if attr["layout"] == "NCHW":
        values = values.transpose(2, 0, 1)[None]
    elif attr["layout"] == "NHWC":
        values = values[None]
    else:
        raise ValueError("unsupported queried input layout")
    if list(values.shape) != attr["dims"]:
        raise ValueError("calibration dimensions do not match native tensor")
    types = {"FP32": "<f4", "FP16": "<f2", "INT8": "i1", "UINT8": "u1", "INT16": "<i2", "UINT16": "<u2", "INT32": "<i4", "UINT32": "<u4"}
    dtype = np.dtype(types[attr["dtype"]])
    if attr["qnt_type"] == "AFFINE":
        if attr["scale"] <= 0:
            raise ValueError("invalid affine scale")
        values = np.rint(values / attr["scale"] + attr["zp"])
    elif attr["qnt_type"] == "DFP":
        values = np.rint(values * (2.0 ** attr["fl"]))
    elif attr["qnt_type"] != "NONE":
        raise ValueError("unsupported quantization")
    if np.issubdtype(dtype, np.integer):
        limits = np.iinfo(dtype)
        values = np.clip(values, limits.min, limits.max)
    if not np.isfinite(values).all():
        raise ValueError("non-finite input")
    data = np.ascontiguousarray(values, dtype=dtype).tobytes()
    if len(data) != attr["size"]:
        raise ValueError("native byte size mismatch")
    (destination / "input_0.bin").write_bytes(data)
    provenance = {"image": str(image_path), "image_sha256": hashlib.sha256(image_path.read_bytes()).hexdigest(), "input_sha256": hashlib.sha256(data).hexdigest(), "pass_through": 1, "normalization": "record mean/std then queried native quantization", "bytes": len(data)}
    (destination / "input_provenance.json").write_text(json.dumps(provenance, indent=2) + "\n")


if __name__ == "__main__":
    main()
