"""Stream and validate a checksum-gated RV1126B InspireFace resource pack."""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import re
import sys
import tarfile
from collections.abc import Mapping
from typing import Any

from command.rv1126b_models.records import load_inventory
from command.rv1126b_pack.contracts import PackContractError, build_manifest, selected_model_ids, validate_manifest


class PackValidationError(ValueError):
    """Raised when an archive or its build report is incomplete or inconsistent."""


_SHA256_RE = re.compile(r"^[0-9a-f]{64}$")
_CALIBRATION_STATUSES = frozenset({"verified", "provisional"})
_DEFAULT_INVENTORY = pathlib.Path(__file__).parents[1] / "rv1126b_models" / "model_inventory.json"
_TRUSTED_ARCHIVE_NAMES = {
    "scrfd_2_5g_160": "face_detect_160", "scrfd_2_5g_320": "face_detect_320",
    "scrfd_2_5g_640": "face_detect_640", "landmark": "landmark", "rnet": "refine_net",
    "recognition": "feature", "liveness": "rgb_anti_spoofing", "mask": "mask_detect",
    "quality": "pose_quality", "emotion": "face_emotion", "attitude": "face_attribute",
}


def _sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _read_report(path: pathlib.Path) -> dict[str, Any]:
    try:
        report = json.loads(path.read_text(encoding="utf-8-sig"))
    except (OSError, json.JSONDecodeError) as error:
        raise PackValidationError(f"invalid build report: {path}") from error
    if not isinstance(report, dict):
        raise PackValidationError("build report must be an object")
    return report


def _is_sha256(value: Any) -> bool:
    return isinstance(value, str) and _SHA256_RE.fullmatch(value) is not None


def _trusted_contract(inventory_path: pathlib.Path) -> tuple[dict[str, dict[str, str]], dict[str, Mapping[str, object]]]:
    """Derive the sole accepted member and calibration mapping from trusted inputs."""
    try:
        records = load_inventory(inventory_path)
        manifest = build_manifest(records, _TRUSTED_ARCHIVE_NAMES)
    except (OSError, ValueError, PackContractError) as error:
        raise PackValidationError(f"invalid trusted inventory: {inventory_path}") from error
    records_by_id = {record["id"]: record for record in records}
    members = {
        name: {"id": model_id, "calibration_status": records_by_id[model_id]["calibration_status"]}
        for model_id, name in _TRUSTED_ARCHIVE_NAMES.items()
    }
    sections = _manifest_model_sections(manifest)
    if set(members) != set(sections):
        raise PackValidationError("trusted inventory manifest mapping is invalid")
    return members, sections


def _validate_report(report: Mapping[str, Any], trusted_members: Mapping[str, Mapping[str, str]]) -> dict[str, dict[str, str]]:
    expected_ids = list(selected_model_ids())
    if report.get("schema_version") != 1 or report.get("target") != "rv1126b":
        raise PackValidationError("build report identity is invalid")
    if report.get("detector_family") != "scrfd_2_5g" or report.get("selected_model_ids") != expected_ids:
        raise PackValidationError("build report selected model IDs are invalid")
    if report.get("selected_model_count") != len(expected_ids) or report.get("conversion_engineering_gate") is not True:
        raise PackValidationError("build report engineering gate is invalid")
    if not _is_sha256(report.get("pack_sha256")) or not _is_sha256(report.get("manifest_sha256")):
        raise PackValidationError("build report hashes are invalid")
    entries = report.get("members")
    if not isinstance(entries, list) or len(entries) != len(expected_ids):
        raise PackValidationError("build report members are invalid")
    members: dict[str, dict[str, str]] = {}
    ids = set()
    for entry in entries:
        if not isinstance(entry, dict):
            raise PackValidationError("build report member is invalid")
        model_id, name = entry.get("id"), entry.get("member")
        status, digest = entry.get("calibration_status"), entry.get("sha256")
        if (
            not isinstance(model_id, str) or model_id not in expected_ids or model_id in ids
            or not isinstance(name, str) or name not in trusted_members or name in members
            or not _is_sha256(digest)
        ):
            raise PackValidationError("build report member contract is invalid")
        if model_id != trusted_members[name]["id"]:
            raise PackValidationError("build report model/member mapping is invalid")
        if status not in _CALIBRATION_STATUSES:
            raise PackValidationError("build report calibration status is invalid")
        if status != trusted_members[name]["calibration_status"]:
            raise PackValidationError("build report calibration status does not match trusted inventory")
        ids.add(model_id)
        members[name] = {"id": model_id, "sha256": digest, "calibration_status": status}
    if ids != set(expected_ids):
        raise PackValidationError("build report member IDs do not match selection")
    if entries != sorted(entries, key=lambda entry: entry["member"]):
        raise PackValidationError("build report members are not sorted")
    verified = sorted(entry["id"] for entry in trusted_members.values() if entry["calibration_status"] == "verified")
    provisional = sorted(entry["id"] for entry in trusted_members.values() if entry["calibration_status"] == "provisional")
    if report.get("verified") != verified or report.get("requires_recalibration") != provisional:
        raise PackValidationError("build report calibration status lists are invalid")
    return members


def _validate_member_name(name: str) -> None:
    path = pathlib.PurePosixPath(name)
    if (
        not name or "\\" in name or path.is_absolute() or ".." in path.parts
        or len(path.parts) != 1 or path.name != name
    ):
        raise PackValidationError(f"invalid archive member path: {name}")


def _stream_member_hash(handle: Any) -> str:
    digest = hashlib.sha256()
    for chunk in iter(lambda: handle.read(1024 * 1024), b""):
        digest.update(chunk)
    return digest.hexdigest()


def _parse_manifest(raw: bytes) -> Mapping[str, object]:
    try:
        # The builder writes canonical JSON, a strict YAML subset accepted by yaml-cpp.
        manifest = json.loads(raw.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        raise PackValidationError("invalid manifest YAML") from error
    try:
        validate_manifest(manifest)
    except PackContractError as error:
        raise PackValidationError(f"invalid manifest contract: {error}") from error
    return manifest


def _manifest_model_sections(manifest: Mapping[str, object]) -> dict[str, Mapping[str, object]]:
    return {
        section["name"]: section
        for section in manifest.values()
        if isinstance(section, Mapping) and isinstance(section.get("name"), str)
    }


def validate_resource_pack(
    pack_path: pathlib.Path, report_path: pathlib.Path, *, inventory_path: pathlib.Path | None = None,
) -> dict[str, Any]:
    """Validate a pack without extracting any archive member to disk."""
    pack_path, report_path = pathlib.Path(pack_path), pathlib.Path(report_path)
    trusted_members, trusted_sections = _trusted_contract(pathlib.Path(inventory_path or _DEFAULT_INVENTORY))
    report = _read_report(report_path)
    report_members = _validate_report(report, trusted_members)
    expected_names = {"__inspire__", *report_members}
    seen: set[str] = set()
    actual_hashes: dict[str, str] = {}
    manifest_raw: bytes | None = None
    try:
        with tarfile.open(pack_path, "r|*") as archive:
            for member in archive:
                name = member.name
                _validate_member_name(name)
                if name in seen:
                    raise PackValidationError(f"duplicate archive member: {name}")
                seen.add(name)
                if not member.isreg():
                    raise PackValidationError(f"archive member is not a regular file: {name}")
                if name not in expected_names:
                    raise PackValidationError(f"unexpected archive member: {name}")
                handle = archive.extractfile(member)
                if handle is None:
                    raise PackValidationError(f"cannot stream archive member: {name}")
                if name == "__inspire__":
                    manifest_raw = handle.read()
                else:
                    actual_hashes[name] = _stream_member_hash(handle)
    except PackValidationError:
        raise
    except (OSError, tarfile.TarError) as error:
        raise PackValidationError(f"cannot read resource pack: {pack_path}") from error
    if seen != expected_names:
        missing = sorted(expected_names.difference(seen))
        extra = sorted(seen.difference(expected_names))
        name = (missing or extra)[0]
        raise PackValidationError(f"archive member set does not match report: {name}")
    if manifest_raw is None:
        raise PackValidationError("archive is missing __inspire__ manifest")
    if hashlib.sha256(manifest_raw).hexdigest() != report["manifest_sha256"]:
        raise PackValidationError("manifest hash does not match build report")
    manifest = _parse_manifest(manifest_raw)
    if _manifest_model_sections(manifest) != trusted_sections:
        raise PackValidationError("manifest model/member mapping does not match trusted inventory")
    for name, entry in report_members.items():
        if actual_hashes.get(name) != entry["sha256"]:
            raise PackValidationError(f"model member hash does not match report: {name}")
    pack_hash = _sha256(pack_path)
    if pack_hash != report["pack_sha256"]:
        raise PackValidationError("pack hash does not match build report")
    return {"valid": True, "pack_sha256": pack_hash, "model_count": len(report_members)}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pack_path", type=pathlib.Path)
    parser.add_argument("report_path", type=pathlib.Path)
    parser.add_argument("--inventory", type=pathlib.Path, default=_DEFAULT_INVENTORY)
    args = parser.parse_args(argv)
    try:
        result = validate_resource_pack(args.pack_path, args.report_path, inventory_path=args.inventory)
    except PackValidationError as error:
        print(f"resource-pack validation failed: {error}", file=sys.stderr)
        return 1
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
