"""Conversion mechanics only; each converter owns its model configuration."""

import argparse
import hashlib
import importlib.metadata
import json
import pathlib

from .records import TOOLKIT_VERSION, load_inventory, validate_record


def sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with pathlib.Path(path).open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def require_ok(code, operation: str) -> None:
    if code != 0:
        raise RuntimeError(f"{operation} failed with code {code}")


def assert_toolkit_version() -> str:
    installed = importlib.metadata.version("rknn-toolkit2")
    if installed != TOOLKIT_VERSION:
        raise ValueError(f"Toolkit2 {TOOLKIT_VERSION} required, found {installed}")
    return installed


def validate_conversion(record: dict, family: str, model_id: str,
                        dataset: pathlib.Path, output: pathlib.Path) -> pathlib.Path:
    validate_record(record)
    if record["family"] != family or record["id"] != model_id:
        raise ValueError(f"converter requires family={family}, id={model_id}")
    source_root = pathlib.Path(record.get("_source_root", ".")).resolve()
    source = (source_root / record["source"]).resolve()
    source.relative_to(source_root)
    if sha256(source) != record["source_sha256"]:
        raise ValueError(f"source SHA-256 mismatch: {source}")
    dataset = pathlib.Path(dataset)
    if not dataset.is_absolute() or dataset.parent.name != model_id:
        raise ValueError("dataset must be absolute and in its own model directory")
    entries = [line.strip() for line in dataset.read_text(encoding="utf-8").splitlines() if line.strip()]
    if not entries or any(not pathlib.Path(p).is_absolute() or not pathlib.Path(p).is_file() for p in entries):
        raise ValueError("dataset requires existing absolute image paths")
    manifest = json.loads(dataset.with_name("calibration.json").read_text(encoding="utf-8"))
    if manifest["model_id"] != model_id or manifest["status"] != record["calibration_status"]:
        raise ValueError("calibration manifest does not match record")
    if pathlib.Path(output).name != pathlib.PurePosixPath(record["output"]).name:
        raise ValueError("output name does not match record")
    return source


def validate_onnx_metadata(record: dict, source: pathlib.Path) -> dict:
    import onnxruntime as ort

    session = ort.InferenceSession(str(source), providers=["CPUExecutionProvider"])
    def metadata(nodes):
        return [{"name": node.name, "shape": list(node.shape),
                 "dtype": "float32" if node.type == "tensor(float)" else node.type} for node in nodes]
    observed = {"inputs": metadata(session.get_inputs()), "outputs": metadata(session.get_outputs())}
    # The inspected source contract is float32; input.dtype describes the
    # separate RKNN runtime contract, not the pre-normalized ONNX tensor.
    expected_input = [{"name": record["input"]["name"],
                       "shape": record["input"]["onnx_shape"], "dtype": "float32"}]
    expected_outputs = [{key: out[key] for key in ("name", "shape", "dtype")} for out in record["outputs"]]
    if observed["inputs"] != expected_input or observed["outputs"] != expected_outputs:
        raise ValueError(f"ONNX metadata differs from record: {observed}")
    return observed


def write_result(output: pathlib.Path, result: dict) -> None:
    path = pathlib.Path(output).with_suffix(".json")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def select_cli_record(model_id: str):
    parser = argparse.ArgumentParser(description=f"Convert {model_id} for RV1126B")
    parser.add_argument("--inventory", type=pathlib.Path, required=True)
    parser.add_argument("--source-root", type=pathlib.Path, required=True)
    parser.add_argument("--artifacts", type=pathlib.Path, required=True)
    args = parser.parse_args()
    record = next(r for r in load_inventory(args.inventory) if r["id"] == model_id)
    record["_source_root"] = str(args.source_root.resolve())
    return record, args.source_root.resolve(), args.artifacts.resolve()
