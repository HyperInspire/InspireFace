"""Contracts for the RV1126B public-C-API E2E runner's base evidence."""

import math
from pathlib import Path
import re
from typing import Any, Dict, Mapping
import unittest


ROOT = Path(__file__).resolve().parents[2]
RUNNER = ROOT / "command" / "rv1126b_public_api_e2e" / "capi_e2e_runner.cpp"
BUILD = ROOT / "command" / "rv1126b_public_api_e2e" / "build_capi_e2e_runner.sh"
BOARD = ROOT / "command" / "rv1126b_public_api_e2e" / "run_board_capi_e2e.ps1"

_IDENTITY_FIELDS = (
    "run_id",
    "serial",
    "pack_sha256",
    "face_image_sha256",
    "no_face_image_sha256",
)
_BASE_SCENARIOS = {"detect_160", "detect_320", "detect_640", "no_face", "landmark", "recognition"}


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
        if status == "success" and (scenario["hresult"] != 0 or scenario["all_finite"] is not True):
            return False
        if status == "failure" and (scenario["hresult"] == 0 or scenario["all_finite"] is not False):
            return False
        if not _is_finite_number(scenario.get("peak_rss_kb")) or scenario["peak_rss_kb"] < 0:
            return False
        latency = scenario.get("latency_ms")
        if not isinstance(latency, list) or len(latency) != 10 or any(not _is_finite_number(value) or value < 0 for value in latency):
            return False
        if not isinstance(scenario.get("detected_faces"), int) or isinstance(scenario["detected_faces"], bool):
            return False
        if name.startswith("detect_") and scenario["detected_faces"] < 1:
            return False
        if name == "no_face" and scenario["detected_faces"] != 0:
            return False
        if name == "landmark" and (scenario.get("dense_count") != 106 or scenario.get("five_point_count") != 5):
            return False
        if name == "recognition" and (scenario.get("feature_size") != 512 or not _is_finite_number(scenario.get("similarity")) or scenario["similarity"] < 0.9999):
            return False
        names.append(name)
    return len(names) == len(set(names)) and set(names) == _BASE_SCENARIOS


def valid_report() -> Dict[str, Any]:
    report: Dict[str, Any] = {
        "run_id": "run-001",
        "serial": "e3d7377f6fc6d325",
        "pack_sha256": "a" * 64,
        "face_image_sha256": "b" * 64,
        "no_face_image_sha256": "c" * 64,
        "scenarios": [
            {"name": "detect_160", "detected_faces": 1},
            {"name": "detect_320", "detected_faces": 1},
            {"name": "detect_640", "detected_faces": 1},
            {"name": "no_face", "detected_faces": 0},
            {"name": "landmark", "detected_faces": 1, "dense_count": 106, "five_point_count": 5},
            {"name": "recognition", "detected_faces": 1, "feature_size": 512, "similarity": 0.9999},
        ],
    }
    for scenario in report["scenarios"]:
        scenario.update(status="success", failure_stage="", hresult=0, all_finite=True,
                        peak_rss_kb=1, latency_ms=[1.0] * 10)
        scenario.setdefault("dense_count", 0)
        scenario.setdefault("five_point_count", 0)
        scenario.setdefault("feature_size", 0)
        scenario.setdefault("similarity", 0.0)
    return report


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

    def test_schema_rejects_status_hresult_and_finiteness_contradictions(self) -> None:
        report = valid_report()
        trusted = {field: report[field] for field in _IDENTITY_FIELDS}
        report["scenarios"][0]["hresult"] = 7
        self.assertFalse(validate_report(report, trusted))

    def test_core_schema_rejects_wrong_counts_feature_or_latency_sample_count(self) -> None:
        report = valid_report()
        report["scenarios"][4]["dense_count"] = 105
        trusted = {field: report[field] for field in _IDENTITY_FIELDS}
        self.assertFalse(validate_report(report, trusted))
        report = valid_report()
        report["scenarios"][5]["similarity"] = 0.9998
        trusted = {field: report[field] for field in _IDENTITY_FIELDS}
        self.assertFalse(validate_report(report, trusted))
        report = valid_report()
        report["scenarios"][0]["latency_ms"] = [1.0] * 9
        trusted = {field: report[field] for field in _IDENTITY_FIELDS}
        self.assertFalse(validate_report(report, trusted))
        report = valid_report()
        trusted = {field: report[field] for field in _IDENTITY_FIELDS}
        report["scenarios"][0]["all_finite"] = False
        self.assertFalse(validate_report(report, trusted))
        report = valid_report()
        report["scenarios"][0].update(status="failure", failure_stage="track", hresult=0, all_finite=False)
        trusted = {field: report[field] for field in _IDENTITY_FIELDS}
        self.assertFalse(validate_report(report, trusted))
        report = valid_report()
        report["scenarios"][0].update(status="failure", failure_stage="track", hresult=7, all_finite=True)
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

    def test_atomic_writer_closes_the_partial_file_even_when_write_fails(self) -> None:
        text = RUNNER.read_text(encoding="utf-8")
        self.assertIn("const bool wrote =", text)
        self.assertIn("const bool closed = std::fclose", text)

    def test_runner_escapes_all_json_control_bytes_and_releases_only_live_handles(self) -> None:
        text = RUNNER.read_text(encoding="utf-8")
        self.assertIn("byte < 0x20U", text)
        self.assertIn("\\\\u00", text)
        self.assertIn("bitmap_ != nullptr", text)
        self.assertIn("feature_.data != nullptr", text)
        self.assertNotIn("bool valid_", text)
        self.assertNotIn("bool allocated_", text)

    def test_runner_records_exact_core_scenarios_with_warmups_and_public_api_measurements(self) -> None:
        text = RUNNER.read_text(encoding="utf-8")
        for required in (
            'kWarmupIterations = 2', 'kMeasuredIterations = 10', 'detect_160', 'detect_320',
            'detect_640', 'no_face', 'landmark', 'recognition', 'HFGetFaceDenseLandmarkFromFaceToken',
            'HFGetFaceFiveKeyPointsFromFaceToken', 'HFQuerySupportedPixelLevelsForFaceDetection',
            'getrusage(RUSAGE_SELF', 'FACE_FEATURE_SIZE', '0.9999', 'std::chrono',
        ):
            self.assertIn(required, text)

    def test_build_links_only_public_sdk_and_proves_armhf_abi(self) -> None:
        text = BUILD.read_text(encoding="utf-8")
        for required in (
            "set -euo pipefail", "RKNN_RUNTIME_DIR", "SDK_INSTALL_DIR",
            "build_cross_rv1126b_armhf.sh", "inspireface.h", "libInspireFace.so",
            "librknnrt.so", "arm-linux-gnueabihf", "readelf", "Tag_ABI_VFP_args",
            "hard-float ABI", "capi_e2e_runner.cpp", "-lInspireFace", "-lrknnrt",
            "-Wl,--no-as-needed -lrknnrt -Wl,--as-needed",
        ):
            self.assertIn(required, text)
        self.assertNotIn("inference_wrapper_rknn_adapter_nano.cpp", text)
        self.assertNotIn("rknn_adapter_nano.h", text)

    def test_launcher_gates_task3_hashes_and_keeps_report_identity_untrusted(self) -> None:
        text = BOARD.read_text(encoding="utf-8")
        for required in (
            "e3d7377f6fc6d325", "runtime-20260913T135617-b8a988db804a",
            "9ec91a21617c00b6b194b37570ee5883764dca169ee1552ac4219b944f4d28d7",
            "Test-Task3Prerequisite", "Get-FileHash", "Resolve-Path", "readlink", "-f",
            "/userdata/inspireface-rv1126b/public-api-e2e", "roundtrip", ".partial",
            "Move-Item", "Assert-RunnerResult", "Merge-RunnerEvidence", "run_id",
            "face_image_sha256", "no_face_image_sha256",
            "detect_160", "detect_320", "detect_640", "no_face", "landmark", "recognition",
            "dense_count", "five_point_count", "feature_size", "0.9999",
        ):
            self.assertIn(required, text)
        merge = text[text.index("$ExpectedCoreScenarios"):text.index("function Assert-RunnerResult")]
        for forbidden in ("'run_id'", "'serial'", "'pack_sha256'", "'face_image_sha256'", "'no_face_image_sha256'"):
            self.assertNotIn(forbidden, merge)

    def test_launcher_uses_argument_vector_and_restricts_cleanup_to_resolved_run_directory(self) -> None:
        text = BOARD.read_text(encoding="utf-8")
        self.assertIn("& adb -s $Serial @Arguments", text)
        self.assertIn("'rm', '-rf', $RemoteRunDirectory", text)
        self.assertIn("$resolvedRun -cne $RemoteRunDirectory", text)
        self.assertIn("IsPathRooted", text)
        self.assertNotIn("shell -c", text)

    def test_launcher_roundtrips_every_deployed_payload(self) -> None:
        text = BOARD.read_text(encoding="utf-8")
        self.assertIn("$RoundtripFiles", text)
        for name in ("capi_e2e_runner", "libInspireFace.so", "librknnrt.so", "pack", "face-image", "no-face-image"):
            self.assertIn(f"'{name}'", text)
        self.assertIn("Get-Sha256 $roundtrip", text)


if __name__ == "__main__":
    unittest.main()
