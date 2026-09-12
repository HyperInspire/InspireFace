"""Build a deterministic, conversion-gated RV1126B InspireFace resource pack."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import pathlib
import shutil
import tarfile
import tempfile
from collections.abc import Mapping
from typing import Any

from command.rv1126b_models.records import load_inventory
from command.rv1126b_models.report import build_report, can_start_pack_integration
from command.rv1126b_pack.contracts import PackContractError, build_manifest, selected_model_ids


class PackBuildError(ValueError):
    """Raised when conversion evidence cannot safely produce a resource pack."""


_ARCHIVE_NAMES = {
    "scrfd_2_5g_160": "face_detect_160",
    "scrfd_2_5g_320": "face_detect_320",
    "scrfd_2_5g_640": "face_detect_640",
    "landmark": "landmark",
    "rnet": "refine_net",
    "recognition": "feature",
    "liveness": "rgb_anti_spoofing",
    "mask": "mask_detect",
    "quality": "pose_quality",
    "emotion": "face_emotion",
    "attitude": "face_attribute",
}


def _sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _read_json(path: pathlib.Path, label: str) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8-sig"))
    except (OSError, json.JSONDecodeError) as error:
        raise PackBuildError(f"invalid {label}: {path}") from error
    if not isinstance(value, dict):
        raise PackBuildError(f"{label} must be a JSON object: {path}")
    return value


def _report_path(output_path: pathlib.Path) -> pathlib.Path:
    return output_path.with_name(output_path.name + ".report.json")


def _remove_outputs(output_path: pathlib.Path) -> None:
    for path in (output_path, _report_path(output_path)):
        if path.is_dir():
            raise PackBuildError(f"output target must be a file: {path}")
        if path.exists() or path.is_symlink():
            path.unlink()


def _archive_names(model_ids: tuple[str, ...]) -> dict[str, str]:
    try:
        names = {model_id: _ARCHIVE_NAMES[model_id] for model_id in model_ids}
    except KeyError as error:
        raise PackBuildError(f"selected model cannot be assigned an archive member: {error.args[0]}") from error
    if len(names) != len(set(names.values())):
        raise PackBuildError("archive member names must be unique")
    if "__inspire__" in names.values():
        raise PackBuildError("archive member name __inspire__ is reserved")
    return names


def _model_path(artifact_root: pathlib.Path, record: Mapping[str, Any]) -> pathlib.Path:
    output = pathlib.PurePosixPath(str(record["output"]))
    candidates = (artifact_root / pathlib.Path(*output.parts), artifact_root / output.name)
    existing = []
    for candidate in candidates:
        if candidate.is_file() and candidate not in existing:
            existing.append(candidate)
    if not existing:
        raise PackBuildError(f"missing selected model: {record['id']}")
    if len(existing) != 1:
        raise PackBuildError(f"ambiguous selected model path: {record['id']}")
    return existing[0]


def _report_models(report: Mapping[str, Any]) -> dict[str, Mapping[str, Any]]:
    models = report.get("models")
    if not isinstance(models, list):
        raise PackBuildError("conversion report is missing models")
    result: dict[str, Mapping[str, Any]] = {}
    for model in models:
        if not isinstance(model, Mapping) or not isinstance(model.get("id"), str):
            raise PackBuildError("conversion report contains an invalid model")
        model_id = model["id"]
        if model_id in result:
            raise PackBuildError(f"conversion report has duplicate model ID: {model_id}")
        result[model_id] = model
    return result


def _validate_selected_evidence(
    records: list[dict], artifact_root: pathlib.Path, evidence_root: pathlib.Path, model_ids: tuple[str, ...]
) -> tuple[dict[str, str], list[dict[str, str]]]:
    report = build_report(evidence_root)
    if not can_start_pack_integration(report):
        raise PackBuildError("conversion engineering gate is not satisfied")
    report_models = _report_models(report)
    by_id = {record["id"]: record for record in records}
    members: list[dict[str, str]] = []
    model_paths: dict[str, str] = {}
    for model_id in model_ids:
        record = by_id.get(model_id)
        report_model = report_models.get(model_id)
        if record is None or report_model is None:
            raise PackBuildError(f"conversion report is missing selected model: {model_id}")
        if report_model.get("failure_stage") is not None:
            raise PackBuildError(f"selected model has failed evidence: {model_id}")
        model_path = _model_path(artifact_root, record)
        sidecar_path = model_path.with_suffix(".json")
        if not sidecar_path.is_file():
            raise PackBuildError(f"missing selected conversion sidecar: {model_id}")
        sidecar = _read_json(sidecar_path, "conversion sidecar")
        actual_hash = _sha256(model_path)
        conversion = report_model.get("model")
        calibration = report_model.get("calibration")
        if not isinstance(conversion, Mapping) or not isinstance(calibration, Mapping):
            raise PackBuildError(f"conversion report is incomplete for: {model_id}")
        if (
            sidecar.get("model_id") != model_id
            or sidecar.get("conversion_status") != "success"
            or sidecar.get("target") != "rv1126b"
            or sidecar.get("toolkit_version") != "2.3.2"
            or sidecar.get("output_sha256") != actual_hash
            or conversion.get("sha256") != actual_hash
            or conversion.get("record_sha256") != actual_hash
        ):
            raise PackBuildError(f"selected model hash/status evidence does not match: {model_id}")
        expected_status = record["calibration_status"]
        if sidecar.get("calibration_status") != expected_status or calibration.get("status") != expected_status:
            raise PackBuildError(f"selected model calibration status does not match: {model_id}")
        model_paths[model_id] = str(model_path)
        members.append({
            "id": model_id,
            "member": "",  # Filled after archive names are validated.
            "sha256": actual_hash,
            "calibration_status": expected_status,
        })
    return model_paths, members


def _write_tar(staging: pathlib.Path, pack_path: pathlib.Path, members: list[str]) -> None:
    with tarfile.open(pack_path, "w", format=tarfile.USTAR_FORMAT) as archive:
        for name in members:
            source = staging / name
            info = tarfile.TarInfo(name)
            info.size = source.stat().st_size
            info.mode = 0o644
            info.mtime = 0
            info.uid = 0
            info.gid = 0
            info.uname = ""
            info.gname = ""
            with source.open("rb") as handle:
                archive.addfile(info, handle)


def _render_json(value: Mapping[str, Any]) -> str:
    return json.dumps(value, indent=2, sort_keys=True, allow_nan=False) + "\n"


def build_resource_pack(
    inventory_path: pathlib.Path,
    artifact_root: pathlib.Path,
    evidence_root: pathlib.Path,
    output_path: pathlib.Path,
    *,
    detector_family: str = "scrfd_2_5g",
) -> dict:
    """Build an atomically replaced pack after validating every selected artifact."""
    inventory_path = pathlib.Path(inventory_path)
    artifact_root = pathlib.Path(artifact_root)
    evidence_root = pathlib.Path(evidence_root)
    output_path = pathlib.Path(output_path)
    _remove_outputs(output_path)
    try:
        model_ids = selected_model_ids(detector_family)
    except PackContractError as error:
        raise PackBuildError(str(error)) from error
    if detector_family != "scrfd_2_5g":
        raise PackBuildError("detector family is not representable by the frozen default manifest")
    try:
        records = load_inventory(inventory_path)
        names = _archive_names(model_ids)
        model_paths, members = _validate_selected_evidence(records, artifact_root, evidence_root, model_ids)
        manifest = build_manifest(records, names)
    except (OSError, PackContractError, ValueError) as error:
        if isinstance(error, PackBuildError):
            raise
        raise PackBuildError(str(error)) from error

    for entry in members:
        entry["member"] = names[entry["id"]]
    members.sort(key=lambda entry: entry["member"])
    output_path.parent.mkdir(parents=True, exist_ok=True)
    staging = pathlib.Path(tempfile.mkdtemp(prefix=f".{output_path.name}.", dir=output_path.parent))
    temp_pack = staging / "pack.tar"
    temp_report = staging / "report.json"
    try:
        # JSON is a YAML subset, so this remains consumable by the archive's YAML loader.
        (staging / "__inspire__").write_bytes(_render_json(manifest).encode("utf-8"))
        for model_id, member_name in names.items():
            shutil.copyfile(model_paths[model_id], staging / member_name)
        tar_members = ["__inspire__"] + sorted(names.values())
        _write_tar(staging, temp_pack, tar_members)
        pack_sha = _sha256(temp_pack)
        verified = sorted(entry["id"] for entry in members if entry["calibration_status"] == "verified")
        provisional = sorted(entry["id"] for entry in members if entry["calibration_status"] == "provisional")
        result: dict[str, Any] = {
            "schema_version": 1,
            "target": "rv1126b",
            "detector_family": detector_family,
            "selected_model_ids": list(model_ids),
            "selected_model_count": len(model_ids),
            "artifact_root": str(artifact_root),
            "evidence_root": str(evidence_root),
            "manifest_sha256": hashlib.sha256((staging / "__inspire__").read_bytes()).hexdigest(),
            "pack_sha256": pack_sha,
            "members": members,
            "verified": verified,
            "requires_recalibration": provisional,
            "conversion_engineering_gate": True,
        }
        temp_report.write_bytes(_render_json(result).encode("utf-8"))
        os.replace(temp_pack, output_path)
        os.replace(temp_report, _report_path(output_path))
        return result
    except Exception:
        _remove_outputs(output_path)
        raise
    finally:
        shutil.rmtree(staging, ignore_errors=True)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inventory", type=pathlib.Path, required=True)
    parser.add_argument("--artifact-root", type=pathlib.Path, required=True)
    parser.add_argument("--evidence-root", type=pathlib.Path, required=True)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    parser.add_argument("--detector-family", default="scrfd_2_5g")
    args = parser.parse_args(argv)
    try:
        build_resource_pack(args.inventory, args.artifact_root, args.evidence_root, args.output,
                            detector_family=args.detector_family)
    except PackBuildError as error:
        print(f"resource-pack build failed: {error}", file=__import__("sys").stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
