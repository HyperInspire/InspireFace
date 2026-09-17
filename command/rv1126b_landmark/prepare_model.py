#!/usr/bin/env python3
import argparse
import copy
import sys
from pathlib import Path

import cv2
import numpy as np
import onnxruntime as ort

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from rv1126b_models import convert_landmark
from rv1126b_models.calibration import prepare_calibration
from rv1126b_models.records import load_inventory


def make_absolute_dataset(dataset, output):
    entries = []
    for raw_line in dataset.read_text(encoding="utf-8").splitlines():
        entry = raw_line.strip()
        if not entry:
            continue
        image_path = (dataset.parent / entry).resolve()
        if not image_path.is_file():
            raise FileNotFoundError("Quantization image does not exist: {}".format(image_path))
        entries.append(str(image_path))
    if not entries:
        raise RuntimeError("Quantization dataset is empty: {}".format(dataset))
    output.write_text("\n".join(entries) + "\n", encoding="utf-8")
    return output


def main():
    parser = argparse.ArgumentParser(description="Convert and prepare the 106-point RV1126B landmark test")
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    model = args.source / "models/_01_hyplmkv2_0.25_112x.onnx"
    image_path = args.source / "crop.png"
    dataset = args.source / "quant_v2_dataset.txt"
    rknn_path = args.output / "landmark_rv1126b.rknn"

    image = cv2.imread(str(image_path))
    if image is None:
        raise RuntimeError("Could not read {}".format(image_path))
    image = cv2.resize(image, (112, 112))
    image.astype(np.uint8).tofile(args.output / "input_bgr_u8.bin")
    cv2.imwrite(str(args.output / "input.png"), image)

    session = ort.InferenceSession(str(model), providers=["CPUExecutionProvider"])
    nchw = image.astype(np.float32).transpose(2, 0, 1)[None, ...] / 255.0
    reference = session.run(None, {session.get_inputs()[0].name: nchw})[0].reshape(-1).astype(np.float32)
    if reference.size != 212:
        raise RuntimeError("Expected 212 ONNX outputs, got {}".format(reference.size))
    reference.tofile(args.output / "onnx_output_f32.bin")

    inventory = Path(__file__).resolve().parents[1] / "rv1126b_models/model_inventory.json"
    record = next(r for r in load_inventory(inventory) if r["id"] == "landmark")
    # The legacy CLI accepts the landmark directory itself, under any name.
    record["_source_root"] = str(args.source.resolve())
    record["source"] = "models/_01_hyplmkv2_0.25_112x.onnx"
    calibration_record = copy.deepcopy(record)
    calibration_record["calibration"]["path"] = "quant_v2_dataset.txt"
    absolute_dataset = prepare_calibration(calibration_record, args.source, args.output / "calibration")
    make_absolute_dataset(dataset, args.output / "quant_dataset_absolute.txt")
    convert_landmark.convert(record, absolute_dataset, rknn_path)
    print("Prepared {} values and {}".format(reference.size, rknn_path))


if __name__ == "__main__":
    main()
