# RV1126B Per-model Conversion Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Convert every supplied InspireFace ONNX model independently for RV1126B, preserve each model's own calibration and preprocessing, and validate every generated RKNN graph on the connected ARMHF board.

**Architecture:** A checked-in model inventory records immutable facts and conversion results, while each model family keeps an independent Python converter with its own RKNN configuration. A small shared module handles only file validation, checksums, result serialization, and calibration-list materialization; it never supplies model preprocessing. Conversion artifacts remain under ignored `build/`, and a generic ARMHF runner verifies tensor contracts before resource-pack integration begins.

**Tech Stack:** Python 3.8, ONNX Runtime, RKNN Toolkit2 2.3.2, C++14, RKNN Runtime C API 2.3.2, CMake, Docker, ADB, unittest.

**Spec:** `docs/superpowers/specs/2026-09-11-rv1126b-full-model-adaptation-design.md`

## Global Constraints

- Convert every deployable RV1126B model from ONNX; do not reuse RK3588 RKNN binaries.
- Target `rv1126b` with RKNN Toolkit2 exactly 2.3.2 and RKNN Runtime 2.3.2 or newer.
- Target the observed Linux ARMv7 hard-float board and retain compatibility with RKNPU driver 0.9.8.
- Each model uses only its own input shape, layout, color order, mean, standard deviation, and calibration list.
- Attitude, emotion, and landmark calibration status is `verified` only after every referenced image is found.
- Liveness, mask, quality, recognition, RNet, and SCRFD use independent temporary datasets and status `provisional` until their original calibration images are restored.
- Provisional results validate compatibility and performance only, never final accuracy.
- Original files under `D:/WeChat/Data/xwechat_files/wxid_f9m0atq6asbc22_4b32/msg/file/2026-09/inspireface-3588` are read-only.
- Generated models, source copies, calibration images, logs, and board output stay under ignored `build/rv1126b-models/`.
- Do not push, merge, or publish without explicit user instruction.

---

### Task 1: Immutable model inventory and record validation

**Files:**
- Create: `command/rv1126b_models/model_inventory.json`
- Create: `command/rv1126b_models/records.py`
- Create: `command/tests/test_rv1126b_model_records.py`

**Interfaces:**
- Consumes: supplied model tree and original per-family conversion scripts.
- Produces: `load_inventory(path: Path) -> list[dict]`, `validate_record(record: dict) -> None`, and one inventory entry per ONNX model.

- [ ] **Step 1: Write failing inventory validation tests**

```python
def test_inventory_contains_all_eighteen_onnx_models(self):
    records = load_inventory(INVENTORY)
    self.assertEqual(len(records), 18)
    self.assertEqual(len({item["id"] for item in records}), 18)

def test_each_record_has_model_specific_conversion_fields(self):
    required = {"id", "family", "source", "output", "input", "outputs",
                "preprocess", "calibration", "calibration_status"}
    for record in load_inventory(INVENTORY):
        self.assertTrue(required.issubset(record))
        validate_record(record)
```

- [ ] **Step 2: Run the tests and verify failure**

Run: `python3 command/tests/test_rv1126b_model_records.py`

Expected: FAIL because `records.py` and the inventory do not exist.

- [ ] **Step 3: Implement strict record loading and validation**

Implement `records.py` so `validate_record` rejects an unknown status, non-`rv1126b` target, missing input shape/layout/dtype, empty output list, or absent model-specific preprocessing keys. It must not insert default mean, standard deviation, color order, or input size.

- [ ] **Step 4: Populate all 18 records from ONNX inspection and original scripts**

Record the 10 SCRFD variants separately. Record the remaining attitude, emotion, liveness, mask, quality, recognition, landmark, and RNet models separately. Add `source_sha256` after copying each ONNX file to `build/rv1126b-models/source/`; do not edit the external source tree.

- [ ] **Step 5: Run the inventory tests**

Run: `python3 command/tests/test_rv1126b_model_records.py`

Expected: PASS with 18 unique, valid records.

- [ ] **Step 6: Commit**

```bash
git add command/rv1126b_models/model_inventory.json command/rv1126b_models/records.py command/tests/test_rv1126b_model_records.py
git commit -m "feat: inventory RV1126B model conversions"
```

### Task 2: Per-model calibration preparation and visible provisional status

**Files:**
- Create: `command/rv1126b_models/calibration.py`
- Create: `command/tests/test_rv1126b_calibration.py`
- Modify: `command/rv1126b_models/model_inventory.json`

**Interfaces:**
- Consumes: `calibration.path`, `calibration_status`, input shape, and the supplied image tree.
- Produces: `prepare_calibration(record: dict, source_root: Path, artifact_root: Path) -> Path` and an absolute dataset file unique to the model ID.

- [ ] **Step 1: Write failing verified-dataset tests**

```python
def test_verified_dataset_requires_every_original_image(self):
    record = make_record(status="verified", lines=["images/a.jpg", "images/missing.jpg"])
    with self.assertRaisesRegex(FileNotFoundError, "missing.jpg"):
        prepare_calibration(record, self.source, self.artifacts)
```

- [ ] **Step 2: Write failing provisional-isolation tests**

```python
def test_provisional_dataset_is_separate_for_each_model(self):
    first = prepare_calibration(make_record("mask", [1, 96, 96, 3]), self.source, self.artifacts)
    second = prepare_calibration(make_record("recognition", [1, 112, 112, 3]), self.source, self.artifacts)
    self.assertNotEqual(first, second)
    self.assertIn("mask", str(first))
    self.assertIn("recognition", str(second))
```

- [ ] **Step 3: Run the calibration tests and verify failure**

Run: `python3 command/tests/test_rv1126b_calibration.py`

Expected: FAIL because `prepare_calibration` is undefined.

- [ ] **Step 4: Implement verified calibration resolution**

Resolve every original list entry relative to that list file. Write absolute paths to `build/rv1126b-models/calibration/{model_id}/dataset.txt`, where `{model_id}` is the validated inventory ID. Refuse missing files and empty lists; never downgrade `verified` to `provisional` automatically.

- [ ] **Step 5: Implement independent provisional datasets**

For each provisional record, deterministically select available face images by sorted source path and SHA-256. Materialize images in the model's own calibration directory using only that model's declared spatial preparation. Write `calibration.json` containing `status: provisional`, model ID, original missing-list path, selected source hashes, and generated image hashes.

- [ ] **Step 6: Run calibration tests and inspect generated records**

Run: `python3 command/tests/test_rv1126b_calibration.py`

Expected: PASS; no two model IDs share a generated dataset file, and all missing original sets remain visibly provisional.

- [ ] **Step 7: Commit**

```bash
git add command/rv1126b_models/calibration.py command/rv1126b_models/model_inventory.json command/tests/test_rv1126b_calibration.py
git commit -m "feat: prepare per-model RV1126B calibration sets"
```

### Task 3: Independent converters for attribute and embedding models

**Files:**
- Create: `command/rv1126b_models/common.py`
- Create: `command/rv1126b_models/convert_attitude.py`
- Create: `command/rv1126b_models/convert_emotion.py`
- Create: `command/rv1126b_models/convert_liveness.py`
- Create: `command/rv1126b_models/convert_mask.py`
- Create: `command/rv1126b_models/convert_quality.py`
- Create: `command/rv1126b_models/convert_recognition.py`
- Create: `command/rv1126b_models/convert_rnet.py`
- Create: `command/tests/test_rv1126b_converter_contracts.py`

**Interfaces:**
- Consumes: one validated model record and that model's absolute dataset file.
- Produces: `convert(record: dict, dataset: Path, output: Path) -> dict` in each family module and a result JSON beside each RKNN file.

- [ ] **Step 1: Write failing converter ownership tests**

```python
def test_every_non_detector_family_owns_its_rknn_config(self):
    for module_name in MODULES:
        source = (CONVERTER_DIR / module_name).read_text()
        self.assertIn("RKNN(", source)
        self.assertIn("target_platform=\"rv1126b\"", source)
        self.assertNotIn("default_preprocess", source)
```

- [ ] **Step 2: Run the contract test and verify failure**

Run: `python3 command/tests/test_rv1126b_converter_contracts.py`

Expected: FAIL because the converter modules do not exist.

- [ ] **Step 3: Implement common mechanics only**

`common.py` may provide `require_ok`, SHA-256 calculation, JSON result writing, and model-path checks. It must not define RKNN mean, standard deviation, color order, input size, output names, or quantization configuration.

- [ ] **Step 4: Implement each family converter with its own settings**

Copy the model-specific preprocessing semantics from its original script and confirm input/output shapes with ONNX Runtime. Each module constructs its own `RKNN`, calls its own `rknn.config(..., target_platform="rv1126b")`, loads only its ONNX source, builds with only its dataset, exports its model, and writes status/toolkit/source/calibration hashes to its result JSON.

- [ ] **Step 5: Run contract and dry-run tests**

Run: `python3 command/tests/test_rv1126b_converter_contracts.py`

Expected: PASS and demonstrate that changing recognition preprocessing cannot alter mask or liveness configuration.

- [ ] **Step 6: Convert the seven model families in the pinned container**

Run these commands separately from the repository root:

```bash
docker run --rm -v "$PWD:/workspace" inspireface-rv1126b python3 command/rv1126b_models/convert_attitude.py --inventory command/rv1126b_models/model_inventory.json --artifacts build/rv1126b-models
docker run --rm -v "$PWD:/workspace" inspireface-rv1126b python3 command/rv1126b_models/convert_emotion.py --inventory command/rv1126b_models/model_inventory.json --artifacts build/rv1126b-models
docker run --rm -v "$PWD:/workspace" inspireface-rv1126b python3 command/rv1126b_models/convert_liveness.py --inventory command/rv1126b_models/model_inventory.json --artifacts build/rv1126b-models
docker run --rm -v "$PWD:/workspace" inspireface-rv1126b python3 command/rv1126b_models/convert_mask.py --inventory command/rv1126b_models/model_inventory.json --artifacts build/rv1126b-models
docker run --rm -v "$PWD:/workspace" inspireface-rv1126b python3 command/rv1126b_models/convert_quality.py --inventory command/rv1126b_models/model_inventory.json --artifacts build/rv1126b-models
docker run --rm -v "$PWD:/workspace" inspireface-rv1126b python3 command/rv1126b_models/convert_recognition.py --inventory command/rv1126b_models/model_inventory.json --artifacts build/rv1126b-models
docker run --rm -v "$PWD:/workspace" inspireface-rv1126b python3 command/rv1126b_models/convert_rnet.py --inventory command/rv1126b_models/model_inventory.json --artifacts build/rv1126b-models
```

Expected: seven `.rknn` files and seven result JSON files; attitude/emotion are verified, while liveness/mask/quality/recognition/RNet are provisional.

- [ ] **Step 7: Commit**

```bash
git add command/rv1126b_models command/tests/test_rv1126b_converter_contracts.py
git commit -m "feat: convert RV1126B face attribute models"
```

### Task 4: Landmark and SCRFD converters

**Files:**
- Create: `command/rv1126b_models/convert_landmark.py`
- Create: `command/rv1126b_models/convert_scrfd.py`
- Create: `command/tests/test_rv1126b_detection_conversion.py`
- Modify: `command/rv1126b_landmark/prepare_model.py`

**Interfaces:**
- Consumes: landmark's verified 300-image dataset and one independent SCRFD record/dataset per input size.
- Produces: one landmark RKNN and ten SCRFD RKNN files with independent conversion records.

- [ ] **Step 1: Write failing SCRFD enumeration and landmark-delegation tests**

```python
def test_scrfd_records_are_converted_independently(self):
    records = [r for r in load_inventory(INVENTORY) if r["family"] == "scrfd"]
    self.assertEqual({tuple(r["input"]["shape"][1:3]) for r in records},
                     {(160, 160), (192, 192), (256, 256), (320, 320), (640, 640)})
    self.assertEqual(len(records), 10)

def test_landmark_converter_uses_only_landmark_dataset(self):
    self.assertEqual(landmark_record["calibration"]["path"], "landmark/quant_v2_dataset.txt")
```

- [ ] **Step 2: Run tests and verify failure**

Run: `python3 command/tests/test_rv1126b_detection_conversion.py`

Expected: FAIL until all SCRFD entries and converter interfaces exist.

- [ ] **Step 3: Move reusable landmark conversion behind the family interface**

Retain the already verified BGR, 112x112, mean 0, standard deviation 255 behavior. Make `prepare_model.py` call `convert_landmark.convert(...)` without changing its existing board-comparison outputs.

- [ ] **Step 4: Implement one-record-at-a-time SCRFD conversion**

`convert_scrfd.py --model-id MODEL_ID` loads exactly one ONNX graph, uses that record's input dimensions and its own provisional dataset, and writes exactly one RV1126B RKNN model. The ten explicit IDs are `scrfd_500m_160`, `scrfd_500m_192`, `scrfd_500m_256`, `scrfd_500m_320`, `scrfd_500m_640`, `scrfd_2_5g_160`, `scrfd_2_5g_192`, `scrfd_2_5g_256`, `scrfd_2_5g_320`, and `scrfd_2_5g_640`. It must preserve the nine-output score/bbox/keypoint ordering discovered from ONNX rather than relying on directory order.

- [ ] **Step 5: Run tests and convert landmark plus ten SCRFD graphs**

Run: `python3 command/tests/test_rv1126b_detection_conversion.py`

Then invoke landmark once and SCRFD separately for all ten IDs inside `inspireface-rv1126b`.

Expected: PASS; landmark result is verified and all ten SCRFD results are provisional.

- [ ] **Step 6: Commit**

```bash
git add command/rv1126b_models/convert_landmark.py command/rv1126b_models/convert_scrfd.py command/rv1126b_landmark/prepare_model.py command/tests/test_rv1126b_detection_conversion.py
git commit -m "feat: convert RV1126B landmark and SCRFD models"
```

### Task 5: Generic ARMHF tensor-contract runner and board matrix

**Files:**
- Create: `command/rv1126b_models/rknn_contract_runner.cpp`
- Create: `command/rv1126b_models/build_contract_runner.sh`
- Create: `command/rv1126b_models/run_board_matrix.ps1`
- Create: `command/tests/test_rv1126b_board_matrix.py`

**Interfaces:**
- Consumes: one RKNN path, binary input tensors, and expected tensor metadata from the conversion result.
- Produces: one board JSON result per model containing actual tensor attributes, runtime/driver versions, run latency, finite-value status, and output files.

- [ ] **Step 1: Write failing matrix completeness tests**

```python
def test_board_matrix_requires_one_result_per_converted_model(self):
    converted = {p.stem for p in ARTIFACTS.glob("models/*.rknn")}
    validated = {p.stem for p in ARTIFACTS.glob("board/*.json")}
    self.assertEqual(validated, converted)
```

- [ ] **Step 2: Run tests and verify failure**

Run: `python3 command/tests/test_rv1126b_board_matrix.py`

Expected: FAIL because board result files are absent.

- [ ] **Step 3: Implement the generic C++ runner**

Query all RKNN input/output attributes, allocate buffers by queried byte size, accept multiple binary inputs, request float outputs, reject non-finite output values, run one warm-up plus ten timed iterations, and emit JSON without hard-coding landmark's 212-value output.

- [ ] **Step 4: Build and verify ARMHF ABI**

Run: `bash command/rv1126b_models/build_contract_runner.sh build/rv1126b-models/rknn_contract_runner`

Run: `file build/rv1126b-models/rknn_contract_runner`

Expected: ELF 32-bit ARM, EABI5, hard-float executable linked to `librknnrt.so` with `/oem/usr/lib` runtime search path.

- [ ] **Step 5: Deploy every model to the connected board**

Run: `powershell -File command/rv1126b_models/run_board_matrix.ps1 -Serial e3d7377f6fc6d325 -Artifacts build/rv1126b-models -RemoteDirectory /userdata/inspireface-rv1126b-models`

Expected: one successful JSON and output set per converted model, with API 2.3.2, driver 0.9.8, finite outputs, and no tensor-contract mismatch.

- [ ] **Step 6: Run matrix completeness tests**

Run: `python3 command/tests/test_rv1126b_board_matrix.py`

Expected: PASS for all 18 generated models.

- [ ] **Step 7: Commit**

```bash
git add command/rv1126b_models command/tests/test_rv1126b_board_matrix.py
git commit -m "test: validate RV1126B model matrix on board"
```

### Task 6: Conversion report and resource-pack integration gate

**Files:**
- Create: `command/rv1126b_models/report.py`
- Create: `command/tests/test_rv1126b_conversion_report.py`
- Create: `docs/rv1126b-model-status.md`

**Interfaces:**
- Consumes: all conversion, calibration, and board result JSON files.
- Produces: `build/rv1126b-models/conversion-report.json`, `docs/rv1126b-model-status.md`, and `can_start_pack_integration(report: dict) -> bool`.

- [ ] **Step 1: Write failing status and gate tests**

```python
def test_report_keeps_provisional_models_visible(self):
    report = build_report(self.results)
    self.assertEqual(set(report["requires_recalibration"]), EXPECTED_PROVISIONAL_IDS)

def test_pack_gate_requires_every_model_to_pass_board_contract(self):
    report = build_report(self.results_without_one_board_result)
    self.assertFalse(can_start_pack_integration(report))
```

- [ ] **Step 2: Run tests and verify failure**

Run: `python3 command/tests/test_rv1126b_conversion_report.py`

Expected: FAIL because report generation is undefined.

- [ ] **Step 3: Implement deterministic report generation**

Sort records by model ID, include source/model/calibration hashes, toolkit/runtime/driver versions, calibration status, tensor attributes, latency statistics, and failure stage. `requires_recalibration` must list every provisional ID even when its board test passes.

- [ ] **Step 4: Implement the pack-integration gate**

Return true only when all 18 expected conversions succeeded and every generated model passed its board tensor-contract test. Provisional status does not block engineering integration, but it remains a release/accuracy warning.

- [ ] **Step 5: Generate and verify the report**

Run: `python3 command/rv1126b_models/report.py --artifacts build/rv1126b-models --markdown docs/rv1126b-model-status.md`

Run: `python3 command/tests/test_rv1126b_conversion_report.py`

Expected: PASS and `can_start_pack_integration: true`; the Markdown report clearly separates verified from provisional models.

- [ ] **Step 6: Run the complete model-conversion verification suite**

Run: `python3 -m unittest discover -s command/tests -p 'test_rv1126b_*.py' -v`

Run: `git diff --check`

Expected: all tests pass and no whitespace errors.

- [ ] **Step 7: Commit**

```bash
git add command/rv1126b_models/report.py command/tests/test_rv1126b_conversion_report.py docs/rv1126b-model-status.md
git commit -m "docs: report RV1126B model conversion status"
```

## Follow-on Plan Boundary

After `can_start_pack_integration` becomes true, write a second implementation plan for resource-pack construction and the full InspireFace board pipeline. That plan must use the actual converted tensor contracts and must cover archive configuration, RKNN backend dispatch, detection decoding, landmark alignment, recognition, liveness, attributes, no-face behavior, repeated lifecycle, memory, and end-to-end latency. Do not guess those interfaces before Task 5 records them from the board.
