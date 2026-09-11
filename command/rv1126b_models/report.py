"""Build a deterministic RV1126B conversion evidence report and integration gate.

The report deliberately separates compatibility from accuracy readiness: a
successful board matrix proves that these artifacts execute, but it does not
promote a provisional calibration dataset to an accepted calibration.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import pathlib
import statistics
import struct
from typing import Any


EXPECTED_IDS = (
    "attitude", "emotion", "landmark", "liveness", "mask", "quality", "recognition", "rnet",
    "scrfd_2_5g_160", "scrfd_2_5g_192", "scrfd_2_5g_256", "scrfd_2_5g_320", "scrfd_2_5g_640",
    "scrfd_500m_160", "scrfd_500m_192", "scrfd_500m_256", "scrfd_500m_320", "scrfd_500m_640",
)
REQUIRES_RECALIBRATION = (
    "liveness", "mask", "quality", "recognition", "rnet", "scrfd_2_5g_160", "scrfd_2_5g_192",
    "scrfd_2_5g_256", "scrfd_2_5g_320", "scrfd_2_5g_640", "scrfd_500m_160", "scrfd_500m_192",
    "scrfd_500m_256", "scrfd_500m_320", "scrfd_500m_640",
)
TARGET = "rv1126b"
TOOLKIT_VERSION = "2.3.2"
DRIVER_VERSION = "0.9.8"
LIMITATIONS = (
    "Preflight failures may leave stale conversion sidecars.",
    "Direct API calibration validation does not bind every dataset entry to manifest hashes.",
    "Liveness and quality raw-pixel normalization remains provisional.",
    "Emotion and liveness conversion logs contain quantization outliers requiring accuracy regression.",
    "Recognition conversion logs contain compiler diagnostics requiring accuracy regression.",
    "The runner's version-prefix check and defensive dimension serialization have minor known issues.",
)


def _read_json(path: pathlib.Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def _sha256(path: pathlib.Path) -> str | None:
    if not path.is_file():
        return None
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _inventory() -> list[dict]:
    path = pathlib.Path(__file__).with_name("model_inventory.json")
    records = _read_json(path)
    if not isinstance(records, list):
        raise ValueError("RV1126B inventory must be a list")
    return sorted(records, key=lambda record: record["id"])


def _error(stage: str, message: str) -> dict:
    return {"stage": stage, "message": message}


def _latency_stats(values: Any) -> dict:
    if not isinstance(values, list) or not values or not all(isinstance(value, (int, float)) and math.isfinite(value) and value > 0 for value in values):
        return {"count": 0, "min": None, "max": None, "mean": None, "median": None}
    values = [float(value) for value in values]
    return {
        "count": len(values), "min": min(values), "max": max(values),
        "mean": statistics.fmean(values), "median": statistics.median(values),
    }


def _output_evidence(artifact_root: pathlib.Path, model_id: str, outputs: Any) -> tuple[dict, str | None]:
    if not isinstance(outputs, list) or not outputs:
        return {"count": 0, "sha256": [], "all_finite": False}, "missing board outputs"
    evidence = []
    all_finite = True
    for output in outputs:
        if not isinstance(output, dict):
            return {"count": 0, "sha256": [], "all_finite": False}, "invalid board output record"
        path = artifact_root / "board" / model_id / str(output.get("file", ""))
        raw = path.read_bytes() if path.is_file() else b""
        n_elems = output.get("n_elems")
        actual_sha = hashlib.sha256(raw).hexdigest() if raw else None
        finite = bool(output.get("finite")) and len(raw) == (n_elems * 4 if isinstance(n_elems, int) else -1)
        if finite:
            finite = all(math.isfinite(value[0]) for value in struct.iter_unpack("<f", raw))
        all_finite = all_finite and finite and actual_sha == output.get("sha256")
        evidence.append({
            "index": output.get("index"), "name": output.get("name"), "n_elems": n_elems,
            "sha256": actual_sha, "reported_sha256": output.get("sha256"), "finite": finite,
        })
    return {"count": len(evidence), "sha256": evidence, "all_finite": all_finite}, None


def _model_report(artifact_root: pathlib.Path, inventory: dict, sidecar: dict | None, board: dict | None) -> dict:
    model_id = inventory["id"]
    source_path = artifact_root / "source" / pathlib.PurePosixPath(inventory["source"])
    model_path = artifact_root / "models" / f"{model_id}_rv1126b.rknn"
    calibration_dir = artifact_root / "calibration" / model_id
    source_sha = _sha256(source_path)
    model_sha = _sha256(model_path)
    dataset_sha = _sha256(calibration_dir / "dataset.txt")
    manifest_sha = _sha256(calibration_dir / "calibration.json")
    sidecar = sidecar if isinstance(sidecar, dict) else {}
    board = board if isinstance(board, dict) else {}
    failure = None
    if not sidecar:
        failure = _error("conversion", "conversion sidecar is missing")
    elif sidecar.get("conversion_status") != "success":
        failure = _error("conversion", "conversion did not succeed")
    elif not model_sha or sidecar.get("output_sha256") != model_sha:
        failure = _error("model_hash", "RKNN artifact hash does not match conversion sidecar")
    elif source_sha != inventory.get("source_sha256") or sidecar.get("source_sha256") != source_sha:
        failure = _error("source_hash", "source ONNX hash does not match inventory and conversion sidecar")
    elif dataset_sha != sidecar.get("dataset_sha256") or manifest_sha != sidecar.get("calibration_manifest_sha256"):
        failure = _error("calibration_hash", "calibration artifact hashes do not match conversion sidecar")
    elif not board:
        failure = _error("board", "board result is missing")
    outputs, output_error = _output_evidence(artifact_root, model_id, board.get("outputs"))
    if failure is None and output_error:
        failure = _error("board_output", output_error)
    if failure is None and not outputs["all_finite"]:
        failure = _error("board_output", "board output hash, size, or finiteness check failed")
    return {
        "id": model_id,
        "family": inventory["family"],
        "source": {"path": inventory["source"], "sha256": source_sha, "inventory_sha256": inventory["source_sha256"]},
        "model": {
            "path": inventory["output"], "sha256": model_sha, "record_sha256": sidecar.get("output_sha256"),
            "conversion_status": sidecar.get("conversion_status"), "target": sidecar.get("target"),
            "toolkit_version": sidecar.get("toolkit_version"),
        },
        "calibration": {
            "status": inventory["calibration_status"], "dataset_sha256": dataset_sha,
            "record_dataset_sha256": sidecar.get("dataset_sha256"), "manifest_sha256": manifest_sha,
            "record_manifest_sha256": sidecar.get("calibration_manifest_sha256"),
        },
        "native_tensor_contracts": {"inputs": board.get("inputs", []), "outputs": board.get("outputs", [])},
        "outputs": outputs,
        "latency_ms": _latency_stats(board.get("latency_ms")),
        "board": {
            "status": board.get("status"), "model_sha256": board.get("model_sha256"),
            "api_version": board.get("api_version"), "driver_version": board.get("driver_version"),
            "run_id": board.get("run_id"), "serial": board.get("serial"),
        },
        "failure_stage": failure,
    }


def build_report(artifact_root: pathlib.Path) -> dict:
    """Read conversion and board artifacts and return a stable evidence report."""
    artifact_root = pathlib.Path(artifact_root)
    inventory = _inventory()
    if tuple(record["id"] for record in inventory) != EXPECTED_IDS:
        raise ValueError("RV1126B inventory does not contain the exact expected model IDs")
    sidecars: dict[str, dict] = {}
    for path in (artifact_root / "models").glob("*_rv1126b.json") if (artifact_root / "models").is_dir() else ():
        data = _read_json(path)
        if isinstance(data, dict) and isinstance(data.get("model_id"), str):
            sidecars[data["model_id"]] = data
    board_results: dict[str, dict] = {}
    board_path = artifact_root / "board" / "results.json"
    if board_path.is_file():
        data = _read_json(board_path)
        if isinstance(data, list):
            board_results = {item.get("model_id"): item for item in data if isinstance(item, dict) and isinstance(item.get("model_id"), str)}
    models = [_model_report(artifact_root, record, sidecars.get(record["id"]), board_results.get(record["id"])) for record in inventory]
    report = {
        "schema_version": 1, "target": TARGET, "toolkit_version": TOOLKIT_VERSION,
        "model_ids": list(EXPECTED_IDS), "requires_recalibration": list(REQUIRES_RECALIBRATION),
        "status_counts": {"verified": 3, "provisional": 15}, "models": models,
        "limitations": list(LIMITATIONS), "accuracy_release_ready": False,
        "accuracy_release_reason": "15 models require restored calibration and accuracy regression.",
    }
    report["can_start_pack_integration"] = can_start_pack_integration(report)
    return report


def can_start_pack_integration(report: dict) -> bool:
    """Return true only when the complete artifact/board compatibility gate passes."""
    if not isinstance(report, dict) or report.get("target") != TARGET or report.get("toolkit_version") != TOOLKIT_VERSION:
        return False
    models = report.get("models")
    if (
        not isinstance(models, list)
        or len(models) != len(EXPECTED_IDS)
        or any(not isinstance(model, dict) for model in models)
        or [model.get("id") for model in models] != list(EXPECTED_IDS)
    ):
        return False
    if report.get("model_ids") != list(EXPECTED_IDS):
        return False
    for model in models:
        if model.get("failure_stage") is not None:
            return False
        conversion, board, outputs, latency = model.get("model"), model.get("board"), model.get("outputs"), model.get("latency_ms")
        if not isinstance(conversion, dict) or conversion.get("conversion_status") != "success":
            return False
        if conversion.get("target") != TARGET or conversion.get("toolkit_version") != TOOLKIT_VERSION:
            return False
        if not conversion.get("sha256") or conversion.get("sha256") != conversion.get("record_sha256"):
            return False
        if not isinstance(board, dict) or board.get("status") != "success" or board.get("model_sha256") != conversion.get("sha256"):
            return False
        if not isinstance(board.get("api_version"), str) or not board["api_version"].startswith(TOOLKIT_VERSION):
            return False
        if board.get("driver_version") != DRIVER_VERSION:
            return False
        if not isinstance(outputs, dict) or not outputs.get("all_finite") or outputs.get("count", 0) <= 0:
            return False
        if not isinstance(latency, dict) or latency.get("count") != 10 or not all(isinstance(latency.get(key), (int, float)) and math.isfinite(latency[key]) and latency[key] > 0 for key in ("min", "max", "mean", "median")):
            return False
    return True


def render_json(report: dict) -> str:
    return json.dumps(report, indent=2, sort_keys=True, allow_nan=False) + "\n"


def render_markdown(report: dict) -> str:
    gate = "PASS" if report["can_start_pack_integration"] else "FAIL"
    lines = [
        "# RV1126B model conversion status", "",
        f"Resource-pack integration gate: **{gate}**. Accuracy/release readiness: **NO**.", "",
        "The successful board matrix is compatibility evidence, not accuracy acceptance.", "",
        "## Status", "",
        "| Model | Calibration | Board | Outputs | Latency mean (ms) | Failure stage |",
        "|---|---|---|---:|---:|---|",
    ]
    for model in report["models"]:
        failure = model["failure_stage"]
        stage = "" if failure is None else failure["stage"]
        lines.append(
            f"| {model['id']} | {model['calibration']['status']} | {model['board']['status']} | "
            f"{model['outputs']['count']} | {model['latency_ms']['mean'] or 0:.6f} | {stage} |"
        )
    lines += [
        "", "## Calibration and release", "",
        "Each provisional model requires restored calibration and accuracy regression before release: "
        + ", ".join(report["requires_recalibration"]) + ".",
        "", "Only attitude, emotion, and landmark have verified calibration status. Provisional status never disappears merely because actual board runtime succeeds.",
        "", "## Evidence and known limitations", "",
        "Each JSON model entry records source/model/calibration hashes, target Toolkit, native queried tensor contracts, board API/driver, output hashes and finiteness, latency statistics, and any failure stage.",
    ]
    lines.extend(f"- {limitation}" for limitation in report["limitations"])
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifacts", type=pathlib.Path, default=pathlib.Path("build/rv1126b-models"))
    parser.add_argument("--json", type=pathlib.Path, default=None)
    parser.add_argument("--markdown", type=pathlib.Path, default=None)
    args = parser.parse_args()
    report = build_report(args.artifacts)
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(render_json(report), encoding="utf-8")
    if args.markdown:
        args.markdown.parent.mkdir(parents=True, exist_ok=True)
        args.markdown.write_text(render_markdown(report), encoding="utf-8")
    if not args.json and not args.markdown:
        print(render_json(report), end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
