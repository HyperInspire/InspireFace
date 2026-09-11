# RV1126B Full Model Adaptation Design

## Goal

Adapt the complete InspireFace model set and runtime path to RV1126B Linux ARMHF using RKNN Toolkit2 and Runtime 2.3.2, then demonstrate the supported InspireFace features end to end on the connected board.

## Scope

The input model collection is `inspireface-3588`. It contains ONNX sources for SCRFD 500M and 2.5G at five input sizes each, landmark, RNet, face recognition, liveness, mask, quality, emotion, and attitude models.

The work includes model inventory, conversion, calibration provenance, board-level model validation, resource-pack assembly, InspireFace integration, functional tests, accuracy comparison where reference data exists, performance measurement, and documentation. It does not claim production accuracy for models converted with provisional calibration data.

## Toolchain and Target

- RKNN Toolkit2: exactly 2.3.2
- RKNN Runtime: 2.3.2 or newer, tested against board runtime 2.3.2
- Target platform: `rv1126b`
- Board userspace: Linux ARMv7 hard-float
- RKNPU driver observed on the test board: 0.9.8
- Existing RK3588 RKNN files are references only; every deployable RV1126B model is rebuilt from ONNX.

## Per-model Conversion Records

Each model is converted independently from its own ONNX file and its own calibration dataset. A machine-readable record describes the result of each conversion; it does not provide shared preprocessing or calibration configuration. Each record contains:

- stable model identifier and InspireFace feature
- ONNX source path and generated RKNN path
- input name, shape, layout, dtype, and color order
- preprocessing mean, standard deviation, resize, and crop behavior
- expected output names, shapes, and semantic ordering
- quantization mode and calibration dataset path
- calibration status: `verified` or `provisional`
- provenance and reason for the status
- conversion tool/runtime versions and source checksum
- validation state, board latency, and comparison metrics when available

Conversion scripts must not silently infer preprocessing from filenames. For each model, its original conversion script, ONNX metadata, model-specific calibration list, and current InspireFace preprocessing implementation determine the configuration. No preprocessing or calibration setting is copied from another model merely because both process faces.

## Calibration Policy

The attitude, emotion, and landmark datasets whose referenced files exist are `verified` inputs for conversion. Their labels are irrelevant to post-training quantization, but their images must match the model input domain.

For liveness, mask, quality, recognition, RNet, and SCRFD entries whose original dataset lists point to missing files, a separate deterministic provisional calibration set is generated for each model from the available face images in the supplied collection. Every set is processed with that model's own input size, crop/resize behavior, color order, mean, and standard deviation. The record retains every source checksum. Random tensors are not used.

Every model produced from a provisional set is marked `provisional` in its conversion record and the generated summary. Provisional models may be used to validate compatibility, graph execution, output contracts, resource-pack wiring, and performance. They must not be used for final accuracy acceptance. When the original calibration images become available, only that model is reconverted with its own restored dataset.

## Independent Conversion Workflow

Each model has an independent conversion invocation and model-specific configuration. An optional outer command may invoke them in sequence and collect results, but it must not change or normalize their parameters. Each invocation performs these stages:

1. Validate source files and calibration references.
2. Inspect the ONNX input/output contract and compare it with that model's conversion record.
3. Materialize an absolute-path calibration list in the artifact directory.
4. Configure RKNN with that model's preprocessing and the `rv1126b` target.
5. Build a quantized RKNN model and save conversion logs.
6. Emit checksums and calibration-status metadata beside the model.
7. Refuse to overwrite a verified artifact with a provisional one unless explicitly requested.

A failed model does not hide successful conversions. The batch report records success or the precise failed stage for every entry and exits nonzero when any selected entry fails.

## Standalone Board Validation

A generic ARMHF runner loads each RKNN model and validates its declared input/output contract. Test inputs are generated from available images using that model's recorded preprocessing. For every model it records:

- runtime and driver versions
- model initialization result
- actual input/output tensor attributes
- inference status and repeated-run latency
- output byte counts, finite-value checks, and output checksums
- ONNX-versus-RKNN numeric metrics when the same reference input can be evaluated

Task-specific decoding is added for SCRFD and landmarks so structural output checks are meaningful. Classification and embedding models compare raw outputs first; semantic acceptance is deferred for provisional calibration models.

## InspireFace Resource-Pack Integration

The generated RV1126B models are assembled into a dedicated resource pack without modifying the supplied source collection. Model identifiers and configuration fields remain compatible with the current InspireFace archive loader. Pack construction validates that every referenced model exists, has the intended checksum, and carries its calibration status in the build report.

Integration proceeds feature by feature:

1. SCRFD detection and RNet refinement, if enabled by the selected pipeline.
2. Landmark alignment.
3. Face recognition embedding and similarity.
4. Liveness.
5. Mask, quality, emotion, and attitude attributes.

The SDK must expose the existing public API; RV1126B support is selected through build/resource configuration rather than new feature-specific public interfaces.

## End-to-End Acceptance

The connected RV1126B must run an ARMHF sample using the generated pack and process still images through the complete enabled pipeline. Acceptance requires:

- no model initialization, tensor-contract, allocation, or inference errors
- valid face boxes and landmarks on known face images
- stable finite outputs across repeated sessions
- recognition embeddings with expected dimension; repeated runs on the same input must have cosine similarity of at least 0.9999
- callable liveness and attribute modules with correctly shaped outputs
- latency and memory results recorded per stage
- graceful behavior for a no-face input

For verified-calibration models, ONNX/RKNN comparisons and task-level results are recorded. For provisional models, successful execution is a compatibility result only, and the final report lists them under “requires recalibration and accuracy regression.”

## Testing

Host-side tests cover per-model record validation, dataset path resolution, calibration-status propagation, preprocessing, batch failure reporting, and pack completeness. RKNN conversion smoke tests run in the pinned Docker image. ARMHF build and ELF/ABI checks run before deployment. Board tests cover every converted model and then the complete InspireFace sample.

Existing RK356X, RK3588, RV1106, and RV1109/RV1126 configuration behavior must remain unchanged. Git-ignored artifacts contain source copies, calibration material, generated models, logs, packs, and board outputs; reusable scripts, tests, conversion-record schema, and documentation are committed.

## Error Handling and Safety

- Missing or inconsistent ONNX metadata is a hard conversion failure.
- Missing verified calibration images are a hard failure; they are never silently downgraded.
- Provisional calibration is opt-in for each individual model and visibly reported.
- Board deployment uses a validated absolute directory under `/userdata`.
- Original model files and user datasets are read-only inputs.
- No branch is pushed, merged, or published without explicit user instruction.

## Deliverables

- reviewed per-model conversion records and validation schema
- reproducible Toolkit2 2.3.2 conversion tooling
- verified and provisional RV1126B RKNN artifacts with reports
- generic standalone board validator plus task-specific checks
- RV1126B InspireFace resource pack
- ARMHF end-to-end sample results from the connected board
- recalibration backlog identifying every provisional model
- developer documentation for reproduction and later accuracy regression
