"""Contracts for the RV1126B public-C-API E2E runner's base evidence."""

import base64
import json
import math
from pathlib import Path
import re
import shutil
import subprocess
import sys
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
        if not isinstance(latency, list) or len(latency) > 10 or any(not _is_finite_number(value) or value < 0 for value in latency):
            return False
        for field in ("detected_faces", "dense_count", "five_point_count", "feature_size"):
            if field in scenario and (not isinstance(scenario[field], int) or isinstance(scenario[field], bool)):
                return False
        if "similarity" in scenario and not _is_finite_number(scenario["similarity"]):
            return False
        if status == "success":
            if len(latency) != 10 or not isinstance(scenario.get("detected_faces"), int) or isinstance(scenario["detected_faces"], bool):
                return False
            if name.startswith("detect_") and scenario["detected_faces"] < 1:
                return False
            if name == "no_face" and scenario["detected_faces"] != 0:
                return False
            if name == "landmark" and (scenario["detected_faces"] < 1 or scenario.get("dense_count") != 106 or scenario.get("five_point_count") != 5):
                return False
            if name == "recognition" and (scenario["detected_faces"] < 1 or scenario.get("feature_size") != 512 or not _is_finite_number(scenario.get("similarity")) or scenario["similarity"] < 0.9999):
                return False
        names.append(name)
    return len(names) == len(set(names)) and set(names) == _BASE_SCENARIOS


def accepts_success_report(report: Mapping[str, Any], trusted: Mapping[str, str]) -> bool:
    return validate_report(report, trusted) and all(
        scenario["status"] == "success" for scenario in report["scenarios"]
    )


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
    def test_adb_runner_logs_do_not_contaminate_exit_status_or_success_gate(self) -> None:
        host = shutil.which("pwsh")
        if host is None:
            self.skipTest("PowerShell 7 is required for the host launcher probe")
        for exit_code in (0, 7):
            with self.subTest(exit_code=exit_code):
                probe = """$ErrorActionPreference = 'Stop'
$tokens = $null; $parseErrors = $null
$ast = [System.Management.Automation.Language.Parser]::ParseFile('{source_path}', [ref]$tokens, [ref]$parseErrors)
if ($parseErrors.Count) {{ throw 'Launcher parsing failed' }}
$function = $ast.Find({{ param($node)
    $node -is [System.Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq 'Invoke-AdbAllowFailure'
}}, $true)
. ([scriptblock]::Create($function.Extent.Text))
function adb {{
    & '{python}' -c 'import sys; print("native stdout diagnostic"); print("native stderr diagnostic", file=sys.stderr); sys.exit(int(sys.argv[1]))' {exit_code}
}}
$Serial = 'e3d7377f6fc6d325'
$runnerExit = Invoke-AdbAllowFailure @('shell', 'sh', '/fixed/run-e2e.sh')
$nativeExit = $LASTEXITCODE
$row = @{{ status = 'failed' }}
# Execute the production exit-status gate and its success publication, rather
# than replacing the launcher check with a test-only comparison.
$gate = $ast.Find({{ param($node)
    $node -is [System.Management.Automation.Language.IfStatementAst] -and $node.Clauses[0].Item1.Extent.Text -eq '$runnerExit -ne 0'
}}, $true)
$publish = $ast.Find({{ param($node)
    $node -is [System.Management.Automation.Language.AssignmentStatementAst] -and $node.Left.Extent.Text -eq '$row.status' -and $node.Right.Extent.Text -eq "'success'"
}}, $true)
if ($null -eq $gate -or $null -eq $publish) {{ throw 'Missing launcher success gate' }}
try {{
    . ([scriptblock]::Create($gate.Extent.Text))
    . ([scriptblock]::Create($publish.Extent.Text))
}} catch {{ $row.status = 'failed' }}
$outcome = @{{ scalar_integer = ($runnerExit -is [int]); value = $runnerExit; native_exit = $nativeExit; status = $row.status }}
'PROBE_RESULT:' + ($outcome | ConvertTo-Json -Compress)
""".format(source_path=str(BOARD).replace("'", "''"), python=sys.executable.replace("'", "''"), exit_code=exit_code)
                encoded = base64.b64encode(probe.encode("utf-16-le")).decode("ascii")
                completed = subprocess.run(
                    [host, "-NoProfile", "-NonInteractive", "-EncodedCommand", encoded],
                    capture_output=True, text=True, encoding="utf-8", check=False, timeout=30,
                )
                self.assertEqual(completed.returncode, 0, completed.stdout + completed.stderr)
                results = [line[len("PROBE_RESULT:"):] for line in completed.stdout.splitlines() if line.startswith("PROBE_RESULT:")]
                self.assertEqual(len(results), 1, completed.stdout)
                outcome = json.loads(results[0])
                self.assertTrue(outcome["scalar_integer"], outcome)
                self.assertEqual(outcome["value"], exit_code)
                self.assertEqual(outcome["native_exit"], exit_code)
                self.assertEqual(outcome["status"], "success" if exit_code == 0 else "failed")
                self.assertIn("native stdout diagnostic", completed.stdout)
                self.assertIn("native stderr diagnostic", completed.stderr)
                self.assertNotIn("native stdout diagnostic", completed.stderr)
                self.assertNotIn("native stderr diagnostic", completed.stdout)

    def _host_gate_accepts(self, report: Mapping[str, Any], rss_expression: str = "") -> bool:
        host = shutil.which("pwsh")
        if host is None:
            self.skipTest("PowerShell 7 is required by the host gate's Double.IsFinite")
        # Load the production functions through the parser, without running deployment.
        # A harness/parser failure must fail the test, never count as gate rejection.
        probe = """$ErrorActionPreference = 'Stop'
$tokens = $null; $parseErrors = $null
$ast = [System.Management.Automation.Language.Parser]::ParseFile('{source_path}', [ref]$tokens, [ref]$parseErrors)
if ($parseErrors.Count -ne 0) {{ throw 'Host script failed PowerShell parsing' }}
foreach ($name in @('Test-FiniteNumber', 'Test-Integer', 'Assert-RunnerResult')) {{
    $definition = $ast.Find({{ param($node)
        $node -is [System.Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq $name
    }}, $true)
    if ($null -eq $definition) {{ throw "Missing production function $name" }}
    . ([scriptblock]::Create($definition.Extent.Text))
}}
$scenarioDefinition = $ast.Find({{ param($node)
    $node -is [System.Management.Automation.Language.AssignmentStatementAst] -and $node.Left.Extent.Text -eq '$ExpectedCoreScenarios'
}}, $true)
if ($null -eq $scenarioDefinition) {{ throw 'Missing production scenario list' }}
. ([scriptblock]::Create($scenarioDefinition.Extent.Text))
$trusted = [ordered]@{{ run_id = 'run-001'; serial = 'e3d7377f6fc6d325'; pack_sha256 = ('a' * 64); face_image_sha256 = ('b' * 64); no_face_image_sha256 = ('c' * 64) }}
$result = @'
{report}
'@ | ConvertFrom-Json
{rss_override}
try {{
    Assert-RunnerResult $result $trusted | Out-Null
    @{{ accepted = $true; error = '' }} | ConvertTo-Json -Compress
}} catch {{
    @{{ accepted = $false; error = $_.Exception.Message }} | ConvertTo-Json -Compress
}}
""".format(
            source_path=str(BOARD).replace("'", "''"), report=json.dumps(report, allow_nan=False),
            rss_override=("$result.scenarios[0].peak_rss_kb = " + rss_expression) if rss_expression else "",
        )
        encoded = base64.b64encode(probe.encode("utf-16-le")).decode("ascii")
        completed = subprocess.run(
            [host, "-NoProfile", "-NonInteractive", "-EncodedCommand", encoded],
            capture_output=True, text=True, encoding="utf-8", check=False, timeout=30,
        )
        self.assertEqual(completed.returncode, 0, completed.stdout + completed.stderr)
        outcome = json.loads(completed.stdout)
        if not outcome["accepted"]:
            self.assertRegex(outcome["error"], r"runner scenario \w+ violates (common schema|core gate)")
        return outcome["accepted"]

    def test_host_gate_accepts_valid_report(self) -> None:
        self.assertTrue(self._host_gate_accepts(valid_report()))

    def test_host_gate_rejects_missing_landmark_or_recognition_face_and_nonfinite_rss(self) -> None:
        for index in (4, 5):
            with self.subTest(scenario=valid_report()["scenarios"][index]["name"]):
                report = valid_report()
                report["scenarios"][index]["detected_faces"] = 0
                self.assertFalse(self._host_gate_accepts(report))
        # JSON cannot represent these numeric values. Inject actual Double values
        # after parsing so rejection proves the gate ran, not that parsing failed.
        for expression in ("[double]::NaN", "[double]::PositiveInfinity", "[double]::NegativeInfinity"):
            with self.subTest(rss=expression):
                self.assertFalse(self._host_gate_accepts(valid_report(), expression))
        for value in ("1", True):
            with self.subTest(rss=value):
                report = valid_report()
                report["scenarios"][0]["peak_rss_kb"] = value
                self.assertFalse(self._host_gate_accepts(report))

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

    def test_success_schema_requires_face_for_landmark_and_recognition_and_finite_rss(self) -> None:
        for index in (4, 5):
            report = valid_report()
            report["scenarios"][index]["detected_faces"] = 0
            trusted = {field: report[field] for field in _IDENTITY_FIELDS}
            self.assertFalse(validate_report(report, trusted))
        for value in (float("nan"), float("inf")):
            report = valid_report()
            report["scenarios"][0]["peak_rss_kb"] = value
            trusted = {field: report[field] for field in _IDENTITY_FIELDS}
            self.assertFalse(validate_report(report, trusted))

    def test_schema_allows_structurally_valid_failure_but_not_success_acceptance(self) -> None:
        report = valid_report()
        for scenario in report["scenarios"]:
            scenario.update(status="failure", failure_stage="launch", hresult=7,
                            all_finite=False, latency_ms=[])
            for field in ("detected_faces", "dense_count", "five_point_count", "feature_size", "similarity"):
                scenario.pop(field, None)
        trusted = {field: report[field] for field in _IDENTITY_FIELDS}
        self.assertTrue(validate_report(report, trusted))
        self.assertFalse(accepts_success_report(report, trusted))

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

    def test_launcher_accepts_external_pack_and_keeps_runner_identity_untrusted(self) -> None:
        text = BOARD.read_text(encoding="utf-8")
        for required in (
            "e3d7377f6fc6d325", "[Parameter(Mandatory = $true)][string]$PackPath",
            "Get-FileHash", "Resolve-Path", "readlink", "-f",
            "/userdata/inspireface-rv1126b/public-api-e2e", "roundtrip", ".partial",
            "Move-Item", "Assert-RunnerResult", "Merge-RunnerEvidence", "run_id",
            "face_image_sha256", "no_face_image_sha256",
            "detect_160", "detect_320", "detect_640", "no_face", "landmark", "recognition",
            "dense_count", "five_point_count", "feature_size", "0.9999",
            "Test-FiniteNumber", "Test-Integer",
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
