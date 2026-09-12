# RV1126B RKNN2 Production Runtime Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the actual InspireFace RKNN2 Nano inference path enforce the converted model contracts and prove on RV1126B that its zero-copy/native-output results match the standard RKNN reference path for all 11 packed models.

**Architecture:** Harden only the RKNPU2 Nano wrapper selected on RV1126B; the RKNPU1 adapter remains untouched. The wrapper will retain normal input/output attributes as the semantic contract, use native attributes only for allocation/layout conversion, and reject mismatched caller metadata before binding memory. A board parity runner executes the standard `inputs_set/outputs_get(want_float=1)` reference and the production Nano path on identical prepared bytes, then emits model-by-model numerical and latency evidence.

**Tech Stack:** C++14, RKNN Runtime 2.3.2, existing `InferenceWrapperRKNNAdapter`/`RKNNAdapterNano`, Python unittest contract tests, ARM Linux EABI5 hard-float toolchain, PowerShell, ADB.

**Spec:** `docs/superpowers/specs/2026-09-11-rv1126b-full-model-adaptation-design.md`

## Global Constraints

- RV1126B uses `INFERENCE_WRAPPER_ENABLE_RKNN2` and `inference_wrapper_rknn_adapter_nano.*`; do not modify the RKNPU1 `customized/rknn_adapter.h` path for this work.
- RKNN Toolkit2 is exactly 2.3.2; runtime is 2.3.2 or newer and the tested board runtime is 2.3.2 with driver 0.9.8.
- Public InspireFace APIs and resource-pack schema remain unchanged.
- Pack inputs are uint8/NHWC; queried model inputs are int8/NHWC/affine and must use RKNN conversion mode (`pass_through=0`), not an ad-hoc host subtraction.
- Semantic output names, order, dimensions, and float results come from normal output attributes; native attributes exist only for zero-copy allocation and storage-layout conversion.
- All packed models request float32 outputs from the wrapper. Unsupported output declarations or quantization modes fail explicitly.
- Generated runners, model copies, inputs, and JSON evidence remain git-ignored. Scripts, tests, adapter changes, and documentation are committed.
- Original models and datasets are read-only. Do not push, merge, publish, or create a PR.

## File Structure

- `cpp/inspireface/middleware/inference_wrapper/customized/rknn_adapter_nano.h`: own and expose validated normal/native tensor contracts; copy padded inputs and decode native outputs safely.
- `cpp/inspireface/middleware/inference_wrapper/inference_wrapper_rknn_adapter_nano.cpp`: bind caller names/types/shapes to queried contracts and expose logical float outputs.
- `cpp/sample/benchmark/rknn2_adapter_contract_guard.cpp`: fake-runtime unit/contract coverage for validation, stride, quantization, finiteness, and failures.
- `command/rv1126b_runtime/rknn2_parity_runner.cpp`: real-runtime standard-versus-Nano comparison executable.
- `command/rv1126b_runtime/build_parity_runner.sh`: ARMHF build and ELF/dependency checks.
- `command/rv1126b_runtime/run_board_parity.ps1`: guarded 11-model deployment, execution, and result collection.
- `command/tests/test_rv1126b_runtime_scripts.py`: build/deployment/parity schema tests.
- `docs/rv1126b-rknn2-runtime.md`: reproducible commands, queried contracts, results, and remaining end-to-end boundary.

---

### Task 1: Bind Caller Input and Output Metadata to RKNN Contracts

**Files:**
- Modify: `cpp/inspireface/middleware/inference_wrapper/customized/rknn_adapter_nano.h`
- Modify: `cpp/inspireface/middleware/inference_wrapper/inference_wrapper_rknn_adapter_nano.cpp`
- Modify: `cpp/sample/benchmark/rknn2_adapter_contract_guard.cpp`

**Interfaces:**
- Consumes: normal `RKNN_QUERY_INPUT_ATTR` and `RKNN_QUERY_OUTPUT_ATTR`, native `RKNN_QUERY_NATIVE_NHWC_OUTPUT_ATTR`, and `InputTensorInfo`/`OutputTensorInfo` from AnyNet.
- Produces: const accessors for normal input/output attributes and native output attributes; `PreProcess` succeeds only for exactly one matching uint8 input; `Process` succeeds only for exact ordered names and float32 output declarations.

- [ ] **Step 1: Add failing fake-runtime contract cases**

```cpp
passed = passed && RejectInput({1, 111, 112, 3});
passed = passed && RejectInput({2, 112, 112, 3});
passed = passed && RejectInputName("wrong");
passed = passed && RejectSecondInput();
passed = passed && RejectOutputNameSwap();
passed = passed && RejectOutputType(TensorInfo::TensorTypeInt8);
```

The fake queried contract must include distinct names and logical dimensions. Assert every rejection occurs before `rknn_set_io_mem` or `rknn_run`, clears output pointers, and leaves no stale successful result.

- [ ] **Step 2: Run the RKNN2 guard and confirm the new cases fail**

Run the existing pinned guard build command used by `command/tests/test_rv1126b_build.py`.

Expected: FAIL because the current wrapper validates only capacity/count/index.

- [ ] **Step 3: Preserve normal and native attributes with explicit roles**

Keep the normal input and output attribute arrays unchanged after query. Create separate binding attributes with `type=RKNN_TENSOR_UINT8`, `fmt=RKNN_TENSOR_NHWC`, and `pass_through=0` for input memory registration. Expose normal attributes for semantic validation and native attributes for allocation/copy only. Do not derive names or logical shapes from native storage layout.

- [ ] **Step 4: Implement exact wrapper validation**

Require one input; name equality; batch 1; exact height/width/channel; NHWC; caller uint8; and non-null data. Require output count, ordered names, and float32 declarations to equal normal queried outputs. On any mismatch, return `WrapperError` before execution and null every output pointer.

- [ ] **Step 5: Verify focused and legacy guards**

Run the RKNN2 adapter guard, output lifecycle guard, AnyNet initialization guard, and existing RV1126B build-script tests.

Expected: all PASS.

- [ ] **Step 6: Commit**

```bash
git add cpp/inspireface/middleware/inference_wrapper/customized/rknn_adapter_nano.h cpp/inspireface/middleware/inference_wrapper/inference_wrapper_rknn_adapter_nano.cpp cpp/sample/benchmark/rknn2_adapter_contract_guard.cpp
git commit -m "fix: bind RKNN2 runtime tensor contracts"
```

### Task 2: Harden Stride Copy and Native Output Conversion

**Files:**
- Modify: `cpp/inspireface/middleware/inference_wrapper/customized/rknn_adapter_nano.h`
- Modify: `cpp/inspireface/middleware/inference_wrapper/inference_wrapper_rknn_adapter_nano.cpp`
- Modify: `cpp/sample/benchmark/rknn2_adapter_contract_guard.cpp`

**Interfaces:**
- Consumes: Task 1 normal/native attributes and validated uint8 input.
- Produces: deterministic zero-filled input padding; float output vectors in normal-query logical order/dimensions; explicit failure for unsupported or inconsistent native contracts.

- [ ] **Step 1: Add failing stride and conversion tests**

```cpp
passed = passed && CopyRNetRows(24, 24, 3, 32);
passed = passed && RepeatedRNetCopyClearsPadding();
passed = passed && RejectDimensionProductMismatch();
passed = passed && RejectNonAffineIntegerOutput();
passed = passed && RejectNonFiniteScaleOrResult();
passed = passed && RejectUnsupportedFp16Output();
passed = passed && ClearResultsAfterRunFailure();
```

The RNet fixture must assert logical size 1728, storage size 2304, 72 copied bytes plus 24 zero padding bytes per row, including after a second smaller-valued input.

- [ ] **Step 2: Run the guard and confirm failures**

Expected: at least the dimension-product, quantization-mode, finite-value, and stale-result cases FAIL on the current implementation.

- [ ] **Step 3: Implement checked input copy**

Validate `n_elems == batch*height*width*channel`, `size` and `size_with_stride` against tensor element size, and width stride against logical width. Zero the complete input memory before every copy, then copy exactly one logical row at a time. Reject any unsupported input storage layout or arithmetic overflow.

- [ ] **Step 4: Implement checked output conversion**

Use native attributes for source stride/layout and normal attributes for destination dimensions. Permit float32 with `QNT_NONE` and int8/uint8 only with `QNT_AFFINE_ASYMMETRIC`, finite scale, valid zero point, and sufficient memory. Convert NC1HWC2/NHWC/NCHW into the normal logical order; require every produced float to be finite. Clear internal output vectors before a run and leave them unavailable on any failure.

- [ ] **Step 5: Run focused and full adapter regressions**

Run all guards from Task 1 plus 256 repeated process iterations and allocation/query/set-io/run failure injection.

Expected: all PASS with no leak and no output exposure after failure.

- [ ] **Step 6: Commit**

```bash
git add cpp/inspireface/middleware/inference_wrapper/customized/rknn_adapter_nano.h cpp/inspireface/middleware/inference_wrapper/inference_wrapper_rknn_adapter_nano.cpp cpp/sample/benchmark/rknn2_adapter_contract_guard.cpp
git commit -m "fix: harden RKNN2 native tensor conversion"
```

### Task 3: Compare the Production Nano Path with the Standard RKNN Path on Board

**Files:**
- Create: `command/rv1126b_runtime/rknn2_parity_runner.cpp`
- Create: `command/rv1126b_runtime/build_parity_runner.sh`
- Create: `command/rv1126b_runtime/run_board_parity.ps1`
- Create: `command/tests/test_rv1126b_runtime_scripts.py`

**Interfaces:**
- Consumes: 11 packed RKNN files, their existing prepared uint8 board inputs, inventory/sidecars, and Task 2 production wrapper.
- Produces: one JSON report with normal/native/binding attributes, per-output numerical comparison, repeated latency, finite checks, and failure stage for every selected model.

- [ ] **Step 1: Add failing source/script contract tests**

```python
def test_runner_executes_both_reference_and_production_paths(self):
    text = RUNNER.read_text()
    self.assertIn("rknn_inputs_set", text)
    self.assertIn("rknn_outputs_get", text)
    self.assertIn("InferenceWrapperRKNNAdapter", text)

def test_board_matrix_is_exactly_the_pack_selection(self):
    self.assertEqual(load_selected_ids(), list(selected_model_ids()))
```

Also require explicit serial, fixed resolved `/userdata/inspireface-rv1126b/runtime-parity`, host pack/evidence gates, LF remote script, round-trip input/model hash checks, per-model continuation after failure, unique run ID, and atomic local result.

- [ ] **Step 2: Run tests and confirm files are missing**

Run: `python -B -m unittest command.tests.test_rv1126b_runtime_scripts -v`

Expected: FAIL because the runtime parity files do not exist.

- [ ] **Step 3: Implement the parity runner**

For each invocation load one model and one exact prepared uint8 input. Run ten warm production iterations and ten measured reference/production pairs. Record all normal input/output attributes, native output attributes, binding attributes, output count/name/logical shape/type/qnt/scale/zp, finite status, SHA-256-equivalent byte digest supplied by the launcher, mean/median/p95 latency, `max_abs`, `mean_abs`, and cosine for each flattened output. Require identical output count and logical element counts; acceptance is `max_abs <= 1e-5` and cosine `>= 0.999999` for nonzero outputs.

- [ ] **Step 4: Implement cross-build and guarded deployment**

Build against the same installed ARMHF SDK/runtime as the loader validator; assert ELF32 ARM EABI5 hard-float and `libInspireFace.so`/`librknnrt.so` dependencies. The PowerShell launcher derives the exact 11-model selection from the trusted pack report, deploys one model/input at a time, collects a row even on failure, and cleans only the resolved fixed directory.

- [ ] **Step 5: Run all 11 models on `e3d7377f6fc6d325`**

Expected: every row initializes and runs both paths, every output is finite, RNet records `width=24/w_stride=32`, SCRFD records nine ordered outputs, attitude records three ordered outputs, and the public engineering gate is true only when all per-output parity thresholds pass.

- [ ] **Step 6: Run host regressions and commit**

Run all RV1126B Python tests, all RKNN adapter guards, cross-build/ELF checks, and `git diff --check`.

```bash
git add command/rv1126b_runtime command/tests/test_rv1126b_runtime_scripts.py
git commit -m "test: compare RV1126B RKNN2 production runtime"
```

### Task 4: Publish Runtime Evidence and the End-to-End Entry Gate

**Files:**
- Create: `docs/rv1126b-rknn2-runtime.md`
- Modify: `docs/rv1126b-model-status.md`

**Interfaces:**
- Consumes: Task 3 JSON and the accepted pack report.
- Produces: a reproducible runtime status document and an explicit boolean gate for starting the public API end-to-end plan.

- [ ] **Step 1: Add a report completeness test**

Extend `command/tests/test_rv1126b_runtime_scripts.py` to require 11 unique IDs, exact pack/model/input hashes, runtime 2.3.2, driver 0.9.8, both-path attributes, every expected output, finite values, latency samples, parity metrics, and nonempty failure stages when any row fails.

- [ ] **Step 2: Generate the final runtime report from real evidence**

The document must state the commit, run ID, serial, pack SHA-256, selected models, normal/native/binding tensor contracts, RNet stride result, per-output max/mean absolute error and cosine, latency median/p95, RSS, and all remaining provisional calibration restrictions.

- [ ] **Step 3: Define the next-stage gate**

Set `can_start_end_to_end=true` only when all 11 production Nano runs and standard-path comparisons pass. This gate is engineering compatibility only; it must not change any provisional model to verified or claim accuracy acceptance.

- [ ] **Step 4: Verify and commit**

Run the runtime report test, all RV1126B tests, adapter guards, and `git diff --check`.

```bash
git add docs/rv1126b-rknn2-runtime.md docs/rv1126b-model-status.md command/tests/test_rv1126b_runtime_scripts.py
git commit -m "docs: report RV1126B RKNN2 runtime parity"
```

## Self-Review

- Spec coverage: this plan covers the actual RKNN2 production backend, input/output contracts, image byte handoff, native-output postprocessing, ARMHF build, real-board parity, latency, and failure reporting. Public API detection/landmark/recognition/pipeline semantics remain intentionally assigned to the following end-to-end plan after the production-runtime gate passes.
- Placeholder scan: no TBD/TODO or unspecified error-handling steps remain; every task provides concrete files, contracts, tests, commands, acceptance criteria, and a commit boundary.
- Type consistency: Task 1 exposes queried normal/native attributes; Task 2 consumes them for storage conversion; Task 3 compares the same production float outputs with standard RKNN float outputs; Task 4 consumes only Task 3's fixed 11-row JSON schema.
