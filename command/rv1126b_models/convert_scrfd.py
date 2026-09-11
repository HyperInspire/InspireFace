"""Convert exactly one supplied SCRFD graph using its own provisional dataset."""

import argparse
import pathlib

from .calibration import prepare_calibration
from .common import (assert_toolkit_version, require_ok, sha256,
                     validate_conversion, validate_onnx_metadata, write_result)
from .records import REQUIRED_SCRFD_IDS, load_inventory


def convert(record: dict, dataset: pathlib.Path, output: pathlib.Path) -> dict:
    version = assert_toolkit_version()
    if record["id"] not in REQUIRED_SCRFD_IDS:
        raise ValueError(f'unknown SCRFD model ID: {record["id"]}')
    source = validate_conversion(record, "scrfd", record["id"], dataset, output)
    size = int(record["id"].rsplit("_", 1)[1])
    expected_outputs = [
        {"name": f"{name}_{stride}", "shape": [1, 2 * (size // stride) ** 2, width],
         "dtype": "float32", "semantic": f"{semantic}_stride_{stride}"}
        for name, semantic, width in (("score", "score", 1), ("bbox", "bbox", 4), ("kps", "keypoints", 10))
        for stride in (8, 16, 32)
    ]
    if (record["input"]["shape"] != [1, size, size, 3]
            or record["input"]["onnx_shape"] != [1, 3, size, size]
            or record["input"]["layout"] != "NHWC"
            or record["input"]["dtype"] != "uint8"
            or record["preprocess"]["resize"]["width"] != size
            or record["preprocess"]["resize"]["height"] != size
            or record["outputs"] != expected_outputs):
        raise ValueError("SCRFD requires its declared input size and nine ordered score/bbox/keypoint outputs")
    observed = validate_onnx_metadata(record, source)
    dataset, output = pathlib.Path(dataset), pathlib.Path(output)
    result = {
        "model_id": record["id"], "source": record["source"],
        "source_sha256": sha256(source), "dataset_sha256": sha256(dataset),
        "calibration_status": record["calibration_status"],
        "calibration_manifest_sha256": sha256(dataset.with_name("calibration.json")),
        "toolkit_version": version, "target": record["target"],
        "input": record["input"], "outputs": record["outputs"],
        "preprocess": record["preprocess"], "onnx_metadata": observed,
        "conversion_status": "failed", "output_sha256": None,
    }
    from rknn.api import RKNN

    rknn = RKNN(verbose=False)
    try:
        require_ok(rknn.config(
            mean_values=[record["preprocess"]["mean"]],
            std_values=[record["preprocess"]["std"]],
            quant_img_RGB2BGR=record["preprocess"]["color_order"] == "BGR",
            target_platform="rv1126b",
        ), "config")
        require_ok(rknn.load_onnx(model=str(source),
                                 outputs=[out["name"] for out in observed["outputs"]]), "load_onnx")
        require_ok(rknn.build(do_quantization=True, dataset=str(dataset)), "build")
        output.parent.mkdir(parents=True, exist_ok=True)
        require_ok(rknn.export_rknn(str(output)), "export_rknn")
        if not output.is_file() or output.stat().st_size == 0:
            raise RuntimeError("export_rknn produced no model")
        result.update(conversion_status="success", output_sha256=sha256(output))
    except Exception as error:
        result["error"] = str(error)
        raise
    finally:
        try:
            rknn.release()
        finally:
            write_result(output, result)
    return result


def main():
    parser = argparse.ArgumentParser(description="Convert one SCRFD graph for RV1126B")
    parser.add_argument("--model-id", required=True, choices=sorted(REQUIRED_SCRFD_IDS))
    parser.add_argument("--inventory", type=pathlib.Path, required=True)
    parser.add_argument("--source-root", type=pathlib.Path, required=True)
    parser.add_argument("--artifacts", type=pathlib.Path, required=True)
    args = parser.parse_args()
    record = next(r for r in load_inventory(args.inventory) if r["id"] == args.model_id)
    source_root, artifacts = args.source_root.resolve(), args.artifacts.resolve()
    record["_source_root"] = str(source_root)
    dataset = prepare_calibration(record, source_root, artifacts / "calibration")
    result = convert(record, dataset, artifacts / record["output"])
    print(f'{result["model_id"]}: {result["conversion_status"]} ({result["calibration_status"]})')


if __name__ == "__main__":
    main()
