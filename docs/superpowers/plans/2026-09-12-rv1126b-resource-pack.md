# RV1126B Resource Pack Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a reproducible, checksum-verified InspireFace resource pack from the validated RV1126B RKNN artifacts and prove that the existing ARMHF archive loader accepts it on the connected board.

**Architecture:** Add a focused Python pack module that derives every model entry from `model_inventory.json`, stages only an explicit 2.5G detector profile plus the eight pipeline models, and emits the existing `__inspire__` YAML contract. A separate verifier checks the tar member set, model hashes, calibration status, and conversion gate before packaging; the existing C++ loader remains the authority for final on-board compatibility. Runtime inference and end-to-end feature behavior are deliberately deferred to the next plan after this pack can load successfully.

**Tech Stack:** Python 3 standard library, PyYAML, unittest, existing `tools/inspire_archive` tar format, CMake/C++14, RKNN Runtime 2.3.2, ADB, RV1126B ARMv7 hard-float.

**Spec:** `docs/superpowers/specs/2026-09-11-rv1126b-full-model-adaptation-design.md`

## Global Constraints

- RKNN Toolkit2 is exactly 2.3.2; the board runtime is 2.3.2 or newer and the observed driver is 0.9.8.
- Target platform is `rv1126b`; board userspace is Linux ARMv7 hard-float.
- Existing RK3588/RV1109 packs are read-only references; every RV1126B RKNN member comes from the independently converted ONNX artifact recorded by the inventory.
- The default pack uses SCRFD 2.5G at 160, 320, and 640 pixels; other converted detector variants remain standalone artifacts and are not silently substituted.
- Every referenced model path and SHA-256 must match the successful conversion sidecar and the passing conversion report before packing.
- Calibration status remains visible in the build report: 3 verified models and 8 selected provisional models; provisional artifacts are engineering-only and do not satisfy accuracy release gates.
- Model identifiers and YAML fields remain compatible with the current `InspireArchive` loader; no new public SDK API is introduced.
- Generated models, staging directories, packs, and board output remain git-ignored; scripts, tests, schemas, and documentation are committed.
- Original model sources and datasets are read-only. Do not push, merge, publish, or create a PR.

## File Structure

- `command/rv1126b_pack/contracts.py`: pure inventory-to-pack mapping, manifest construction, and validation helpers.
- `command/rv1126b_pack/build_pack.py`: CLI orchestration for evidence validation, staging, tar creation, and build report output.
- `command/rv1126b_pack/validate_pack.py`: deterministic structural and checksum verifier for an existing pack.
- `command/rv1126b_pack/validate_pack_main.cpp`: small existing-loader smoke executable used on ARMHF.
- `command/rv1126b_pack/build_validator.sh`: cross-build entry point using the existing RV1126B CMake/toolchain configuration.
- `command/rv1126b_pack/run_board_pack_validation.ps1`: guarded ADB deployment and result collection under `/userdata/inspireface-rv1126b/pack-validation`.
- `command/tests/test_rv1126b_pack_contracts.py`: manifest mapping and schema tests.
- `command/tests/test_rv1126b_pack_build.py`: evidence, staging, archive, and failure-path tests.
- `command/tests/test_rv1126b_pack_validation.py`: tamper and pack-completeness tests.
- `docs/rv1126b-resource-pack.md`: reproduction commands, selected members, hashes/statuses, board loader evidence, and accuracy limitations.

---

### Task 1: Freeze the Inventory-to-Manifest Contract

**Files:**
- Create: `command/rv1126b_pack/__init__.py`
- Create: `command/rv1126b_pack/contracts.py`
- Create: `command/tests/test_rv1126b_pack_contracts.py`

**Interfaces:**
- Consumes: `records.load_inventory(path: pathlib.Path) -> list[dict]` and the committed 18-record inventory.
- Produces: `selected_model_ids(detector_family: str = "scrfd_2_5g") -> tuple[str, ...]`, `build_manifest(records: Sequence[Mapping[str, object]], artifact_names: Mapping[str, str]) -> dict`, and `validate_manifest(manifest: Mapping[str, object]) -> None`.

- [ ] **Step 1: Write failing contract tests**

```python
def test_default_profile_selects_exactly_eleven_models(self):
    self.assertEqual(selected_model_ids(), (
        "scrfd_2_5g_160", "scrfd_2_5g_320", "scrfd_2_5g_640",
        "landmark", "rnet", "recognition", "liveness", "mask",
        "quality", "emotion", "attitude",
    ))

def test_manifest_uses_current_archive_keys_and_per_model_contracts(self):
    manifest = build_manifest(self.records, self.names)
    self.assertEqual(manifest["face_detect_pixel_list"], [160, 320, 640])
    self.assertEqual(manifest["face_detect_model_list"], ["face_detect_160", "face_detect_320", "face_detect_640"])
    self.assertEqual(manifest["landmark"]["infer_engine"], "RKNN")
    self.assertEqual(manifest["feature"]["input_size"], [112, 112])
    self.assertEqual(manifest["rgb_anti_spoofing"]["input_size"], [80, 80])
    self.assertEqual(manifest["face_attribute"]["outputs_layers"], ["547", "548", "549"])
```

Also assert exact key mappings (`rnet -> refine_net`, `recognition -> feature`, `quality -> pose_quality`, `liveness -> rgb_anti_spoofing`, `mask -> mask_detect`, `emotion -> face_emotion`, `attitude -> face_attribute`), input tensor type/layout/color conversion, output order, unique archive names, and rejection of missing/duplicate/extra selected IDs.

- [ ] **Step 2: Run tests and confirm the module is missing**

Run: `python -B -m unittest command.tests.test_rv1126b_pack_contracts -v`

Expected: FAIL because `command.rv1126b_pack.contracts` does not exist.

- [ ] **Step 3: Implement the pure manifest builder**

Use an explicit immutable key map and derive `input_layer`, `outputs_layers`, `input_size`, `nchw`, `swap_color`, `mean`, and `norm` from each record. Emit `model_type`, `infer_engine`, `infer_device`, and `infer_backend` as the current loader-compatible RKNN values. Preserve the official similarity-converter values `0.32/0.6/10.0/0.02/1.0`; reject any inventory shape, output ordering, or preprocessing value that cannot be represented without guessing.

- [ ] **Step 4: Run focused and inventory regression tests**

Run: `python -B -m unittest command.tests.test_rv1126b_pack_contracts command.tests.test_rv1126b_model_records -v`

Expected: all tests PASS.

- [ ] **Step 5: Commit**

```bash
git add command/rv1126b_pack/__init__.py command/rv1126b_pack/contracts.py command/tests/test_rv1126b_pack_contracts.py
git commit -m "feat: define RV1126B resource pack contract"
```

### Task 2: Build a Checksum-Gated Pack

**Files:**
- Create: `command/rv1126b_pack/build_pack.py`
- Create: `command/tests/test_rv1126b_pack_build.py`
- Modify: `.gitignore`

**Interfaces:**
- Consumes: Task 1 `selected_model_ids`, `build_manifest`, `validate_manifest`; conversion `report.build_report(...)`; each selected sidecar's `output_sha256`, `calibration_status`, and output path.
- Produces: `build_resource_pack(inventory_path: Path, artifact_root: Path, evidence_root: Path, output_path: Path, *, detector_family: str = "scrfd_2_5g") -> dict` and CLI exit `0` only when the engineering gate is true and the pack/report were written atomically.

- [ ] **Step 1: Write failing build tests**

```python
def test_builds_deterministic_pack_and_report_from_exact_evidence(self):
    report = build_resource_pack(self.inventory, self.models, self.evidence, self.output)
    self.assertEqual(report["selected_model_count"], 11)
    self.assertEqual(report["verified"], ["attitude", "emotion", "landmark"])
    self.assertEqual(len(report["requires_recalibration"]), 8)
    self.assertEqual(hashlib.sha256(self.output.read_bytes()).hexdigest(), report["pack_sha256"])

def test_rejects_model_or_evidence_hash_mismatch_without_partial_pack(self):
    self.tamper_selected_model("recognition")
    with self.assertRaises(PackBuildError):
        build_resource_pack(self.inventory, self.models, self.evidence, self.output)
    self.assertFalse(self.output.exists())
```

Also cover a false conversion gate, missing sidecar/model, provisional-status loss, duplicate tar names, unknown detector family, stale output cleanup, and byte-for-byte deterministic rebuilds.

- [ ] **Step 2: Run tests and verify they fail**

Run: `python -B -m unittest command.tests.test_rv1126b_pack_build -v`

Expected: FAIL because `build_resource_pack` is undefined.

- [ ] **Step 3: Implement atomic staging and archive creation**

Validate all evidence before copying. Stage `__inspire__` plus 11 extensionless model members in a temporary directory adjacent to the requested output, write a sorted JSON build report, normalize tar metadata (`mtime=0`, uid/gid=0, stable mode and member order), then `os.replace` both final files. Never modify the source collection or model artifact directory.

- [ ] **Step 4: Run focused tests and the full conversion regression**

Run: `python -B -m unittest command.tests.test_rv1126b_pack_build command.tests.test_rv1126b_conversion_report command.tests.test_rv1126b_converter_contracts command.tests.test_rv1126b_detection_conversion command.tests.test_rv1126b_model_records -v`

Expected: all tests PASS.

- [ ] **Step 5: Build the real engineering pack**

Run:

```powershell
python -B -m command.rv1126b_pack.build_pack --inventory command/rv1126b_models/model_inventory.json --artifact-root artifacts/rv1126b-models --evidence-root artifacts/rv1126b-board --output artifacts/rv1126b-pack/Gundam_RV1126B
```

Expected: exit 0; 11 model members plus `__inspire__`; report lists 3 verified and 8 provisional selected models and records all hashes.

- [ ] **Step 6: Commit**

```bash
git add .gitignore command/rv1126b_pack/build_pack.py command/tests/test_rv1126b_pack_build.py
git commit -m "feat: build checksum-gated RV1126B pack"
```

### Task 3: Validate Archive Completeness and Tamper Resistance

**Files:**
- Create: `command/rv1126b_pack/validate_pack.py`
- Create: `command/tests/test_rv1126b_pack_validation.py`

**Interfaces:**
- Consumes: Task 2 pack path and adjacent `<pack>.report.json`.
- Produces: `validate_resource_pack(pack_path: Path, report_path: Path) -> dict` with exact member/hash/status checks and CLI exit 0/1.

- [ ] **Step 1: Write failing verifier tests**

```python
def test_valid_pack_matches_report_and_manifest(self):
    result = validate_resource_pack(self.pack, self.report)
    self.assertTrue(result["valid"])
    self.assertEqual(result["model_count"], 11)

def test_tampered_member_fails_with_member_name(self):
    self.rewrite_member("feature", b"tampered")
    with self.assertRaisesRegex(PackValidationError, "feature"):
        validate_resource_pack(self.pack, self.report)
```

Also cover path traversal, duplicate members, missing/extra members, malformed YAML, report-pack hash mismatch, wrong model hash, missing calibration status, and nonzero CLI exit.

- [ ] **Step 2: Run tests and verify they fail**

Run: `python -B -m unittest command.tests.test_rv1126b_pack_validation -v`

Expected: FAIL because `validate_resource_pack` is undefined.

- [ ] **Step 3: Implement streaming validation**

Read the tar without extracting it; reject non-regular members, absolute paths, `..`, duplicates, and any member set differing from the report. Parse `__inspire__`, call Task 1 `validate_manifest`, stream SHA-256 for every model, and compare the final pack SHA-256 and calibration lists to the report.

- [ ] **Step 4: Validate unit fixtures and the real pack**

Run:

```powershell
python -B -m unittest command.tests.test_rv1126b_pack_validation command.tests.test_rv1126b_pack_build -v
python -B -m command.rv1126b_pack.validate_pack artifacts/rv1126b-pack/Gundam_RV1126B artifacts/rv1126b-pack/Gundam_RV1126B.report.json
```

Expected: all tests PASS and real-pack CLI exits 0.

- [ ] **Step 5: Commit**

```bash
git add command/rv1126b_pack/validate_pack.py command/tests/test_rv1126b_pack_validation.py
git commit -m "test: verify RV1126B pack integrity"
```

### Task 4: Prove the Existing Loader Accepts the Pack on RV1126B

**Files:**
- Create: `command/rv1126b_pack/validate_pack_main.cpp`
- Create: `command/rv1126b_pack/build_validator.sh`
- Create: `command/rv1126b_pack/run_board_pack_validation.ps1`
- Create: `command/tests/test_rv1126b_pack_board_scripts.py`
- Create: `docs/rv1126b-resource-pack.md`

**Interfaces:**
- Consumes: Task 3 validated pack, existing `HFValidateResourcePack`, the pinned ARMHF/RKNN build inputs, and ADB device `e3d7377f6fc6d325`.
- Produces: board JSON containing pack SHA-256, SDK status, archive/model counts, tag/version/major/release, runtime/driver strings, process exit status, and peak RSS; returns nonzero for any loader or metadata mismatch.

- [ ] **Step 1: Write failing script-contract tests**

```python
def test_board_script_uses_serial_and_validated_userdata_directory(self):
    text = BOARD_SCRIPT.read_text(encoding="utf-8")
    self.assertIn("-s $Serial", text)
    self.assertIn("/userdata/inspireface-rv1126b/pack-validation", text)
    self.assertNotIn("adb push $PackPath /userdata/", text)

def test_validator_calls_public_pack_validation_api(self):
    text = VALIDATOR_SOURCE.read_text(encoding="utf-8")
    self.assertIn("HFValidateResourcePack", text)
    self.assertIn("HF_RESOURCE_PACK_INFO_VERSION", text)
```

Also require `set -euo pipefail`, ARMHF ELF checks, explicit serial, remote-path validation, atomic result pull, cleanup limited to the validated directory, and JSON error output.

- [ ] **Step 2: Run tests and verify they fail**

Run: `python -B -m unittest command.tests.test_rv1126b_pack_board_scripts -v`

Expected: FAIL because the board validator files do not exist.

- [ ] **Step 3: Implement and cross-build the loader smoke test**

The C++ executable calls only the existing public validator, verifies expected tag `Gundam_RV1126B` and model count 11, and prints one JSON object. The build script reuses the established RV1126B ARMHF toolchain and external RKNN 2.3.2 runtime, then checks `file`/`readelf` for ARM EABI5 hard-float and required dynamic libraries.

- [ ] **Step 4: Deploy and run on the connected board**

Run:

```powershell
pwsh command/rv1126b_pack/run_board_pack_validation.ps1 -Serial e3d7377f6fc6d325 -PackPath artifacts/rv1126b-pack/Gundam_RV1126B -BuildReport artifacts/rv1126b-pack/Gundam_RV1126B.report.json
```

Expected: exit 0; loader status success; tag/version/counts and pack hash match the host report; collected result is under the git-ignored artifact directory.

- [ ] **Step 5: Run regressions and document evidence**

Run: `python -B -m unittest discover -s command/tests -p "test_rv1126b*.py" -v` and the existing legacy RKNN guard tests used by the conversion plan.

Document exact commands, hashes, board/runtime/driver versions, selected models, 3 verified statuses, 8 provisional selected statuses, and the explicit boundary that loader success is not inference or accuracy acceptance.

- [ ] **Step 6: Commit**

```bash
git add command/rv1126b_pack/validate_pack_main.cpp command/rv1126b_pack/build_validator.sh command/rv1126b_pack/run_board_pack_validation.ps1 command/tests/test_rv1126b_pack_board_scripts.py docs/rv1126b-resource-pack.md
git commit -m "test: validate RV1126B resource pack on board"
```

## Self-Review

- Spec coverage: this plan covers dedicated pack assembly, current-loader compatibility, completeness/hash/status validation, generated-artifact isolation, ARMHF loader validation, and documentation. Runtime inference, preprocessing/postprocessing, end-to-end APIs, performance, and accuracy comparisons remain intentionally assigned to the next implementation plan after Task 4 passes.
- Placeholder scan: no TBD/TODO/deferred implementation placeholders remain; every task names concrete files, interfaces, tests, commands, expected results, and commit boundaries.
- Type consistency: Task 2 consumes Task 1's exact tuple/dict validation interfaces; Task 3 consumes Task 2's pack/report pair; Task 4 consumes the same validated pair and emits board evidence without redefining the manifest contract.
