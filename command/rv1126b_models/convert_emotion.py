"""Independent emotion conversion for RV1126B.
"""

import pathlib

from .calibration import prepare_calibration
from .common import (assert_toolkit_version, require_ok, select_cli_record, sha256,
                     validate_conversion, validate_onnx_metadata, write_result)


def convert(record: dict, dataset: pathlib.Path, output: pathlib.Path) -> dict:
    version = assert_toolkit_version()
    source = validate_conversion(record, "emotion", "emotion", dataset, output)
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
        require_ok(rknn.load_onnx(model=str(source)), "load_onnx")
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
    record, source_root, artifacts = select_cli_record("emotion")
    dataset = prepare_calibration(record, source_root, artifacts / "calibration")
    result = convert(record, dataset, artifacts / record["output"])
    print(f'{result["model_id"]}: {result["conversion_status"]} ({result["calibration_status"]})')


if __name__ == "__main__":
    main()
