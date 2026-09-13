# RV1126B Public C API End-to-End Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (- [ ]) syntax for tracking.

**Goal:** Establish a hash-bound RV1126B public C API engineering gate for the accepted Gundam_RV1126B pack, without claiming model accuracy or calibration.

**Architecture:** One compact ARMHF C++14 executable uses only the public InspireFace C header, executes named scenarios, and atomically writes a JSON result file. A PowerShell board launcher holds trusted pack/image identities outside parsed evidence, validates them before ADB deployment and again after collection. Profile limitations are explicit expected-failure scenarios.

**Tech Stack:** C++14, public C API, libInspireFace.so, ARMHF EABI5 hard-float, RKNN Runtime 2.3.2, Python unittest, PowerShell, ADB, SHA-256.

**Spec:** docs/rv1126b-runtime-parity.md, command/rv1126b_pack/contracts.py, cpp/inspireface/c_api/inspireface.h, and generated Task3 report build/rv1126b-runtime-parity/board/runtime-20260913T135617-b8a988db804a/runtime-parity.json.

## Global Constraints

- Prerequisite is Task3 11/11 success with run runtime-20260913T135617-b8a988db804a, serial e3d7377f6fc6d325, pack SHA-256 9ec91a21617c00b6b194b37570ee5883764dca169ee1552ac4219b944f4d28d7, runtime 2.3.2, and driver 0.9.8.
- Runner links/calls only cpp/inspireface/c_api/inspireface.h. It must not use FaceSession, archive, module, or adapter internals.
- Positive fixture is test_res/data/bulk/kun.jpg. No-face fixture is test_res/data/crop/no_face.png. SHA-256 each on host, after remote ADB round-trip, and in returned evidence. If kun.jpg proves multi-face or unusable, select only another existing positive fixture under test_res/data, record path/SHA, and never download a replacement.
- Generated runners, copied models/libraries/images, and board evidence stay ignored. Commit source, tests, scripts, and documentation only.
- Eight models remain provisional: the three SCRFDs, rnet, recognition, liveness, mask, and quality. E2E enables labelled calibration work only and never promotes their status.
- RGB liveness is execution/finite-output evidence only: existing test_face_pipeline.cpp skips correctness on RKNN2.
- Expected profile failures are IR liveness returning HERR_UNSUPPORTED, interaction requiring absent blink_predict, and non-default landmark engines requiring absent members.
- Use ARMHF EABI5 hard-float, explicit serial e3d7377f6fc6d325, resolved remote paths strictly under /userdata/inspireface-rv1126b/public-api-e2e, LF remote scripts, unique run IDs, and cleanup only of the exact run directory.
- This plan does not convert models, rebuild pack, modify runtime adapter, use a network asset, merge, or edit calibration status.

## File Structure

- command/rv1126b_public_api_e2e/capi_e2e_runner.cpp — public C API runner and atomic JSON writer.
- command/rv1126b_public_api_e2e/build_capi_e2e_runner.sh — narrow ARMHF build and ELF/dependency guard.
- command/rv1126b_public_api_e2e/run_board_capi_e2e.ps1 — trusted host gate, safe ADB deploy, atomic result collection.
- command/tests/test_rv1126b_public_api_e2e_scripts.py — source/schema/build/deployment and malicious-report tests.
- docs/rv1126b-public-api-e2e.md — accepted evidence, commands, profile matrix, calibration boundary.
- docs/rv1126b-runtime-parity.md — latency-scope correction.

---

### Task 1: Public C API Runner and TDD Contract

**Files:**
- Create: command/rv1126b_public_api_e2e/capi_e2e_runner.cpp
- Create: command/tests/test_rv1126b_public_api_e2e_scripts.py

**Interfaces:**
- Consumes CLI arguments: pack, face-image, no-face-image, run-id, serial, pack-sha256, face-sha256, no-face-sha256, result-path.
- Produces one JSON document with all immutable identity fields, unique named rows, status, failure_stage, HResult, finite summaries, timing samples, and RSS.
- Uses HFValidateResourcePack, HFLaunchInspireFace, image bitmap/stream APIs, HFExecuteFaceTrack, landmark APIs, HFFaceFeatureExtractTo, HFFaceComparison, and HFMultipleFacePipelineProcessOptional.

- [ ] **Step 1: Write failing source/schema tests**

    def test_runner_uses_public_api_and_result_path(self):
        text = RUNNER.read_text(encoding="utf-8")
        self.assertIn('#include "inspireface.h"', text)
        self.assertIn("HFValidateResourcePack", text)
        self.assertIn("HFExecuteFaceTrack", text)
        self.assertIn("--result-path", text)
        self.assertNotIn("FaceSession", text)

    def test_failure_needs_stage_and_identity_is_exact(self):
        report = valid_report()
        report["scenarios"][0].update(status="failure", failure_stage="")
        self.assertFalse(validate_report(report, trusted()))

Add probes for duplicate/unknown names, missing image hashes, stdout JSON pollution, non-finite numbers, and a successful row with failure_stage.

- [ ] **Step 2: Run RED test**

Run: python -B -m unittest command.tests.test_rv1126b_public_api_e2e_scripts -v

Expected: FAIL because runner/parser do not exist.

- [ ] **Step 3: Implement minimal public runner**

Add local RAII release helpers for session/bitmap/stream. Reject empty identities before launch and write diagnostics only to stderr. Validate pack before launch; write temp JSON then rename. A failed API call writes a row containing name, failure status, nonempty failure_stage, and HResult. Consume face tokens before another tracking call and use caller-owned buffers through HFFaceFeatureExtractTo.

- [ ] **Step 4: Implement host schema function**

Implement validate_report(report, trusted) returning bool in the Python module. Independently compare run_id, serial, pack SHA, face SHA, and no-face SHA. Reject duplicate rows, missing fields, unknown statuses, non-finite values, and invalid status/failure-stage pairs. Parsed evidence cannot overwrite trusted data.

- [ ] **Step 5: Verify focused/public C API tests**

Run:

    python -B -m unittest command.tests.test_rv1126b_public_api_e2e_scripts -v
    ctest --test-dir build -R "c_api_parity|face_feature_contract|face_token_contract|pipeline_contract" --output-on-failure

Expected: all available tests PASS.

- [ ] **Step 6: Commit Task 1**

    git add command/rv1126b_public_api_e2e/capi_e2e_runner.cpp command/tests/test_rv1126b_public_api_e2e_scripts.py
    git commit -m "test: add RV1126B public C API e2e runner"

### Task 2: ARMHF Build and Guarded Board Launcher

**Files:**
- Create: command/rv1126b_public_api_e2e/build_capi_e2e_runner.sh
- Create: command/rv1126b_public_api_e2e/run_board_capi_e2e.ps1
- Modify: command/tests/test_rv1126b_public_api_e2e_scripts.py

**Interfaces:**
- Consumes RKNN_RUNTIME_DIR, optional SDK_INSTALL_DIR, pack, Task3 report, both fixture paths, output dir, and explicit serial.
- Produces ARM ELF32 EABI5 hard-float runner and ignored build/rv1126b-public-api-e2e/board/run-id evidence directory.

- [ ] **Step 1: Write failing build/deployment tests**

    def test_build_uses_public_sdk_headers_and_armhf_guard(self):
        text = BUILD.read_text(encoding="utf-8")
        self.assertIn("RKNN_RUNTIME_DIR", text)
        self.assertIn("inspireface.h", text)
        self.assertIn("readelf", text)
        self.assertIn("Tag_ABI_VFP_args", text)

    def test_launcher_binds_identity_before_report_parse(self):
        text = LAUNCHER.read_text(encoding="utf-8")
        self.assertIn("e3d7377f6fc6d325", text)
        self.assertIn("Resolve-Path", text)
        self.assertIn("Get-FileHash", text)
        self.assertIn("public-api-e2e", text)

Add malicious reports that replace run ID, serial, pack SHA, face SHA, or no-face SHA. The host gate rejects each before checking row status.

- [ ] **Step 2: Run RED test**

Run: python -B -m unittest command.tests.test_rv1126b_public_api_e2e_scripts -v

Expected: FAIL because shell/PowerShell scripts do not exist.

- [ ] **Step 3: Implement narrow build**

Follow command/build_cross_rv1126b_armhf.sh compiler/sysroot conventions. Resolve public include, libInspireFace.so, Toolkit2 include, and librknnrt.so; reject missing paths. Compile runner only with C++14 and normal warnings. Require file and readelf output proving ELF32 ARM, EABI5 hard-float, libInspireFace.so and librknnrt.so dependencies.

- [ ] **Step 4: Implement safe deploy and host gate**

Before ADB writes require pinned Task3 identity and all 11 success rows. Hash pack/images. Deploy runner, required libraries, pack, and images only to /userdata/inspireface-rv1126b/public-api-e2e/run-id using serial. Check remote hashes, execute runner to an in-directory JSON file, pull into a temporary local path, rehash/parse, validate immutable trusted fields, atomically publish evidence, and delete only resolved run directory.

- [ ] **Step 5: Verify without board execution**

Run:

    python -B -m unittest command.tests.test_rv1126b_public_api_e2e_scripts -v
    bash command/rv1126b_public_api_e2e/build_capi_e2e_runner.sh build/rv1126b-public-api-e2e
    git diff --check

Expected: tests and ELF/dependency checks PASS; no ADB run in this task.

- [ ] **Step 6: Commit Task 2**

    git add command/rv1126b_public_api_e2e/build_capi_e2e_runner.sh command/rv1126b_public_api_e2e/run_board_capi_e2e.ps1 command/tests/test_rv1126b_public_api_e2e_scripts.py
    git commit -m "build: add guarded RV1126B public API launcher"

### Task 3: Real Board Core Detection, Landmark, Feature, and No-Face Gate

**Files:**
- Modify: command/rv1126b_public_api_e2e/capi_e2e_runner.cpp
- Modify: command/rv1126b_public_api_e2e/run_board_capi_e2e.ps1
- Modify: command/tests/test_rv1126b_public_api_e2e_scripts.py

**Interfaces:**
- Produces exactly detect_160, detect_320, detect_640, no_face, landmark, and recognition rows. Each has two unreported warmups, ten finite samples, and non-negative RSS KiB.

- [ ] **Step 1: Write failing core-gate tests**

    def test_core_gate_requires_counts_dims_and_similarity(self):
        report = valid_core_report()
        self.assertTrue(core_gate(report, trusted()))
        report["scenarios_by_name"]["landmark"]["dense_count"] = 105
        self.assertFalse(core_gate(report, trusted()))
        report = valid_core_report()
        report["scenarios_by_name"]["no_face"]["detected_faces"] = 1
        self.assertFalse(core_gate(report, trusted()))

Require positive detection at each level, no-face count zero, 106 dense/5 sparse finite points, 512 finite feature values, same-image cosine at least 0.9999, ten finite samples, and RSS at least zero.

- [ ] **Step 2: Run RED test**

Run: python -B -m unittest command.tests.test_rv1126b_public_api_e2e_scripts -v

Expected: FAIL because core rows/gate are absent.

- [ ] **Step 3: Implement core scenarios**

Create default-landmark detection sessions for every supported 160/320/640 level returned by HFQuerySupportedPixelLevelsForFaceDetection. Use positive image for detector rows and fixed no-face image for no_face. For first positive token call 106- and 5-point APIs. Create recognition session, extract two owned 512-value features from same image/token, compare with HFFaceComparison, reject non-finite or result below 0.9999.

- [ ] **Step 4: Implement timing/RSS and host core gate**

Record getrusage(RUSAGE_SELF).ru_maxrss in KiB. Host requires exact scenario names, trusted identities, successful rows, counts/dimensions/finite data, and ten samples. Timed scope includes stream creation and public operation, excluding JSON serialization.

- [ ] **Step 5: Execute board core gate**

Run command/rv1126b_public_api_e2e/run_board_capi_e2e.ps1 with serial e3d7377f6fc6d325, pack build/rv1126b-runtime-deploy/Gundam_RV1126B, pinned Task3 report, face image test_res/data/bulk/kun.jpg, and no-face image test_res/data/crop/no_face.png.

Expected: all six core rows are hash-bound and accepted. On unusable kun.jpg, stop, select/record existing repository fixture, then rerun.

- [ ] **Step 6: Verify and commit Task 3**

Run focused tests, available C API contracts, host gate against pulled report, and git diff --check.

    git add command/rv1126b_public_api_e2e/capi_e2e_runner.cpp command/rv1126b_public_api_e2e/run_board_capi_e2e.ps1 command/tests/test_rv1126b_public_api_e2e_scripts.py
    git commit -m "test: gate RV1126B public API core flows"

### Task 4: Optional Pipeline and Expected Unsupported Profile Matrix

**Files:**
- Modify: command/rv1126b_public_api_e2e/capi_e2e_runner.cpp
- Modify: command/rv1126b_public_api_e2e/run_board_capi_e2e.ps1
- Modify: command/tests/test_rv1126b_public_api_e2e_scripts.py

**Interfaces:**
- Produces successful pipeline_mask, pipeline_quality_pose, pipeline_attribute, pipeline_emotion, pipeline_liveness_execution rows and expected-failure unsupported_ir_liveness, unsupported_interaction, unsupported_alternate_landmark rows.

- [ ] **Step 1: Write failing matrix tests**

    def test_optional_matrix_distinguishes_execution_from_accuracy(self):
        report = valid_optional_report()
        self.assertTrue(optional_gate(report, trusted()))
        report["scenarios_by_name"]["unsupported_ir_liveness"]["hresult"] = 0
        self.assertFalse(optional_gate(report, trusted()))
        report = valid_optional_report()
        report["scenarios_by_name"]["pipeline_liveness_execution"]["accuracy_claimed"] = True
        self.assertFalse(optional_gate(report, trusted()))

Require supported output counts equal input faces and outputs finite. Expected failures need nonempty reason and expected result category.

- [ ] **Step 2: Run RED test**

Run: python -B -m unittest command.tests.test_rv1126b_public_api_e2e_scripts -v

Expected: FAIL because optional and expected-failure rows are absent.

- [ ] **Step 3: Implement supported pipeline scenarios**

Create session with recognition, liveness, mask, quality, pose, attribute, and emotion enabled before tracking. Call HFMultipleFacePipelineProcessOptional and public getters. Require finite/count-bound outputs. Liveness writes accuracy_claimed false and proves only execution/finite score.

- [ ] **Step 4: Implement expected profile failures**

Request IR liveness and require HERR_UNSUPPORTED. Request interaction and require failed load labelled missing_pack_member with blink_predict. Select a documented non-default landmark engine before session creation and require failed load labelled unsupported_pack_landmark_engine. Do not change library or pack behavior.

- [ ] **Step 5: Board matrix and regressions**

Run Task3 launcher command. Run focused tests, available test_face_pipeline, test_c_api_v2_contract, feature/token contracts, and git diff --check.

Expected: supported rows pass, all known limitations match expected failure, and surprise success/failure makes host gate false.

- [ ] **Step 6: Commit Task 4**

    git add command/rv1126b_public_api_e2e/capi_e2e_runner.cpp command/rv1126b_public_api_e2e/run_board_capi_e2e.ps1 command/tests/test_rv1126b_public_api_e2e_scripts.py
    git commit -m "test: cover RV1126B optional public API profile"

### Task 5: Evidence, Documentation, and Latency Scope Correction

**Files:**
- Create: docs/rv1126b-public-api-e2e.md
- Modify: docs/rv1126b-runtime-parity.md
- Modify: command/tests/test_rv1126b_public_api_e2e_scripts.py

**Interfaces:**
- Produces can_start_public_api_calibration and can_claim_public_api_accuracy. Former is true only after all engineering rows succeed; latter remains false for provisional models.

- [ ] **Step 1: Write failing evidence/document tests**

    def test_evidence_never_promotes_provisional_models(self):
        gate = evidence_gate(valid_optional_report(), trusted())
        self.assertTrue(gate["can_start_public_api_calibration"])
        self.assertFalse(gate["can_claim_public_api_accuracy"])

    def test_runtime_latency_scope_includes_input_integrity_hashes(self):
        self.assertIn("input-integrity SHA-256",
                      RUNTIME_DOC.read_text(encoding="utf-8"))

Require document values come from result JSON: commit, run/serial, full pack/image/result hashes, runtime/driver, actual fixture, scenario status, mean/p95/RSS, and unsupported matrix.

- [ ] **Step 2: Run RED test**

Run: python -B -m unittest command.tests.test_rv1126b_public_api_e2e_scripts -v

Expected: FAIL because evidence documentation/latency wording are incomplete.

- [ ] **Step 3: Write accepted evidence documentation**

Document cross-build, hash/deploy, board commands, source commit, Task3 prerequisite, scenario measures read from JSON, supported matrix, expected failures, and calibration boundary. State no detector/recognition/liveness/mask/quality accuracy is claimed.

- [ ] **Step 4: Correct Task3 latency wording**

Change docs/rv1126b-runtime-parity.md so parity latency explicitly includes input-integrity SHA-256 checks, raw-output retrieval, native-to-logical conversion, and diagnostic SHA-256 hashing. Retain evidence-cost rather than clean benchmark warning.

- [ ] **Step 5: Verify and commit Task 5**

Run:

    python -B -m unittest command.tests.test_rv1126b_public_api_e2e_scripts -v
    pwsh -NoProfile -File command/rv1126b_public_api_e2e/run_board_capi_e2e.ps1 -ValidateOnly -ResultRoot build/rv1126b-public-api-e2e/board
    git diff --check

Expected: only pulled/hash-bound evidence passes; calibration-start gate true only after all engineering rows; accuracy gate false.

    git add docs/rv1126b-public-api-e2e.md docs/rv1126b-runtime-parity.md command/tests/test_rv1126b_public_api_e2e_scripts.py
    git commit -m "docs: record RV1126B public API e2e evidence"

## Self-Review

- Task 1 gives public API/report contract; Task 2 gives ARMHF/reproducible guarded deployment; Task 3 gates three detector levels, default landmark, feature, and no-face; Task 4 covers optional pipeline and known limitations; Task 5 preserves evidence and calibration boundary.
- Known differences are explicit profile facts: IR is engine-unsupported, interaction needs blink_predict absent from the pack, and alternate landmarks lack pack members. Default 11-member mapping remains the supported path.
- Trust fields remain consistent across all tasks: run_id, serial, pack SHA-256, face SHA-256, and no-face SHA-256 are never accepted from evidence without independent comparison.
