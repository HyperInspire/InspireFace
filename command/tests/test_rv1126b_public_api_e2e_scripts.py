"""Contracts for the RV1126B public-C-API E2E runner's base evidence."""

import math
from pathlib import Path
import re
from typing import Any, Dict, Mapping
import unittest


ROOT = Path(__file__).resolve().parents[2]
RUNNER = ROOT / "command" / "rv1126b_public_api_e2e" / "capi_e2e_runner.cpp"

_IDENTITY_FIELDS = (
    "run_id",
    "serial",
    "pack_sha256",
    "face_image_sha256",
    "no_face_image_sha256",
)
_BASE_SCENARIOS = {"bootstrap"}


def _is_finite_number(value: Any) -> bool:
    return isinstance(value, (int, float)) and not isinstance(value, bool) and math.isfinite(value)


def _is_sha256(value: Any) -> bool:
    return isinstance(value, str) and re.fullmatch(r"[0-9a-f]{64}", value) is not None


def validate_report(report: Mapping[str, Any], trusted: Mapping[str, str]) -> bool:
    """Accept only a complete base runner report bound to host-held identity."""
    if not isinstance(report, Mapping) or not isinstance(trusted, Mapping):
        return False
    if any(not isinstance(report.get(field), str) or not report[field] or report[field] != trusted.get(field)
           for field in _IDENTITY_FIELDS):
        return False
    if any(not _is_sha256(report[field]) for field in _IDENTITY_FIELDS if field.endswith("sha256")):
        return False
    scenarios = report.get("scenarios")
    if not isinstance(scenarios, list) or not scenarios:
        return False
    names = []
    for scenario in scenarios:
        if not isinstance(scenario, Mapping):
            return False
        name, status = scenario.get("name"), scenario.get("status")
        if not isinstance(name, str) or name not in _BASE_SCENARIOS or status not in {"success", "failure"}:
            return False
        if not isinstance(scenario.get("hresult"), int) or isinstance(scenario["hresult"], bool):
            return False
        failure_stage = scenario.get("failure_stage")
        if not isinstance(failure_stage, str) or (status == "failure") != bool(failure_stage):
            return False
        if not isinstance(scenario.get("all_finite"), bool):
            return False
        if not _is_finite_number(scenario.get("peak_rss_kb")) or scenario["peak_rss_kb"] < 0:
            return False
        latency = scenario.get("latency_ms")
        if not isinstance(latency, list) or any(not _is_finite_number(value) or value < 0 for value in latency):
            return False
        names.append(name)
    return len(names) == len(set(names)) and set(names) == _BASE_SCENARIOS


def valid_report() -> Dict[str, Any]:
    return {
        "run_id": "run-001",
        "serial": "e3d7377f6fc6d325",
        "pack_sha256": "a" * 64,
        "face_image_sha256": "b" * 64,
        "no_face_image_sha256": "c" * 64,
        "scenarios": [{
            "name": "bootstrap", "status": "success", "failure_stage": "",
            "hresult": 0, "all_finite": True, "peak_rss_kb": 0, "latency_ms": [],
        }],
    }


class PublicApiE2EBaseContractTests(unittest.TestCase):
    def test_valid_report_is_bound_to_independent_trusted_identity(self) -> None:
        report = valid_report()
        trusted = {field: report[field] for field in _IDENTITY_FIELDS}
        self.assertTrue(validate_report(report, trusted))
        report["pack_sha256"] = "d" * 64
        self.assertFalse(validate_report(report, trusted))

    def test_schema_rejects_non_sha256_identity_values(self) -> None:
        report = valid_report()
        report["face_image_sha256"] = "not-a-sha256"
        trusted = {field: report[field] for field in _IDENTITY_FIELDS}
        self.assertFalse(validate_report(report, trusted))

    def test_schema_rejects_duplicate_or_unknown_scenarios(self) -> None:
        report = valid_report()
        report["scenarios"].append(dict(report["scenarios"][0]))
        trusted = {field: report[field] for field in _IDENTITY_FIELDS}
        self.assertFalse(validate_report(report, trusted))
        report = valid_report()
        report["scenarios"][0]["name"] = "unexpected"
        trusted = {field: report[field] for field in _IDENTITY_FIELDS}
        self.assertFalse(validate_report(report, trusted))

    def test_schema_requires_nonempty_stage_for_failure_and_finite_metrics(self) -> None:
        report = valid_report()
        report["scenarios"][0].update(status="failure", failure_stage="")
        trusted = {field: report[field] for field in _IDENTITY_FIELDS}
        self.assertFalse(validate_report(report, trusted))
        report = valid_report()
        report["scenarios"][0]["latency_ms"] = [float("nan")]
        trusted = {field: report[field] for field in _IDENTITY_FIELDS}
        self.assertFalse(validate_report(report, trusted))

    def test_runner_uses_public_api_raii_owned_feature_and_atomic_result_file(self) -> None:
        text = RUNNER.read_text(encoding="utf-8")
        for required in (
            '#include "inspireface.h"', "HFValidateResourcePack", "HFLaunchInspireFace",
            "HFCreateImageBitmapFromFilePath", "HFCreateImageStreamFromImageBitmap",
            "HFExecuteFaceTrack", "HFFaceFeatureExtractTo", "HFCreateFaceFeature",
            "HFReleaseFaceFeature", "HFReleaseImageBitmap", "HFReleaseImageStream",
            "HFReleaseInspireFaceSession", "HERR_INVALID_PARAM", "std::cerr", "std::rename", "--result-path",
        ):
            self.assertIn(required, text)
        for forbidden in ('#include "face_session.h"', "FaceTrackModule", "std::cout << report"):
            self.assertNotIn(forbidden, text)
        self.assertRegex(text, r"HResult hresult = HERR_INVALID_PARAM;")


if __name__ == "__main__":
    unittest.main()
