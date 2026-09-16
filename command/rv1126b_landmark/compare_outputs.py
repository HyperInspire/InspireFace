#!/usr/bin/env python3
import argparse
import json
from pathlib import Path

import cv2
import numpy as np


def compare_landmarks(reference, actual, input_size):
    reference = np.asarray(reference, dtype=np.float32).reshape(-1)
    actual = np.asarray(actual, dtype=np.float32).reshape(-1)
    if reference.size != actual.size:
        raise ValueError("Landmark outputs must contain the same number of values")
    if reference.size == 0 or reference.size % 2:
        raise ValueError("Landmark output must contain non-empty x/y pairs")
    delta_px = (actual - reference).reshape(-1, 2) * float(input_size)
    point_error = np.linalg.norm(delta_px, axis=1)
    return {
        "point_count": int(point_error.size),
        "mean_absolute_error_px": float(np.abs(delta_px).mean()),
        "root_mean_square_error_px": float(np.sqrt(np.square(delta_px).mean())),
        "mean_point_error_px": float(point_error.mean()),
        "max_point_error_px": float(point_error.max()),
    }


def draw_comparison(image_path, reference, actual, output_path, input_size):
    image = cv2.imread(str(image_path))
    if image is None:
        raise ValueError("Could not read image: {}".format(image_path))
    image = cv2.resize(image, (input_size, input_size))
    for point in np.asarray(reference).reshape(-1, 2) * input_size:
        cv2.circle(image, tuple(np.rint(point).astype(int)), 1, (0, 255, 0), -1)
    for point in np.asarray(actual).reshape(-1, 2) * input_size:
        cv2.circle(image, tuple(np.rint(point).astype(int)), 1, (0, 0, 255), -1)
    if not cv2.imwrite(str(output_path), image):
        raise RuntimeError("Could not write image: {}".format(output_path))


def main():
    parser = argparse.ArgumentParser(description="Compare ONNX and RV1126B landmark outputs")
    parser.add_argument("reference", type=Path)
    parser.add_argument("actual", type=Path)
    parser.add_argument("--image", type=Path)
    parser.add_argument("--overlay", type=Path)
    parser.add_argument("--json", type=Path)
    parser.add_argument("--input-size", type=int, default=112)
    args = parser.parse_args()
    reference = np.fromfile(args.reference, dtype=np.float32)
    actual = np.fromfile(args.actual, dtype=np.float32)
    metrics = compare_landmarks(reference, actual, args.input_size)
    if args.image and args.overlay:
        draw_comparison(args.image, reference, actual, args.overlay, args.input_size)
    if args.json:
        args.json.write_text(json.dumps(metrics, indent=2) + "\n")
    print(json.dumps(metrics, indent=2))


if __name__ == "__main__":
    main()
