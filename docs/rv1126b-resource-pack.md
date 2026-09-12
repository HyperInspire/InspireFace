# RV1126B resource-pack loader validation

This procedure validates only archive loading and metadata on the RV1126B
ARMHF board. It does not run inference and is not an accuracy or calibration
acceptance test.

## Inputs and limits

The default pack is `Gundam_RV1126B`, built from SCRFD 2.5G at 160, 320, and
640 pixels plus landmark, RNet, recognition, liveness, mask, quality, emotion,
and attitude (11 members). The build report preserves three verified selected
models (`attitude`, `emotion`, `landmark`) and eight provisional models that
require recalibration before an accuracy release.

The current generated engineering pack is
`build/rv1126b-pack/Gundam_RV1126B`, with SHA-256
`9ec91a21617c00b6b194b37570ee5883764dca169ee1552ac4219b944f4d28d7`.
Its adjacent report is the source of the selected-member and calibration
summary; always re-run the host verifier before deployment.

The preceding standalone board matrix for this same serial recorded RKNN
runtime `2.3.2 (429f97ae6b@2025-04-09T09:10:37)` and driver `0.9.8`. This is
deployment context only, not resource-pack loader evidence: the command below
creates the separate loader JSON and records its own RSS and process status.

## Reproduce

Use the official RKNN Toolkit2 2.3.2 `librknn_api` directory and its ARMHF
runtime. The build script invokes the existing RV1126B SDK build, compiles a
small executable that calls only public `HFValidateResourcePack`, and rejects
non-ARM EABI5 hard-float ELF output or missing `libInspireFace.so` /
`librknnrt.so` dependencies.

```sh
export RKNN_RUNTIME_DIR=/path/to/rknpu2/runtime/Linux/librknn_api
bash command/rv1126b_pack/build_validator.sh
```

When an already-built RV1126B SDK is available, set `SDK_INSTALL_DIR` to its
`InspireFace` install directory instead. The script still checks its embedded
RKNN runtime is 2.3.2 or newer and ARM EABI5 hard-float before compiling and
copying the validator plus its two required shared libraries.

On Windows, deploy only after the Task 3 host validation succeeds. The script
is intentionally pinned to serial `e3d7377f6fc6d325`, verifies the resolved
remote directory, and removes only
`/userdata/inspireface-rv1126b/pack-validation`.

```powershell
pwsh command/rv1126b_pack/run_board_pack_validation.ps1 `
  -Serial e3d7377f6fc6d325 `
  -PackPath build/rv1126b-pack/Gundam_RV1126B `
  -BuildReport build/rv1126b-pack/Gundam_RV1126B.report.json
```

The atomic local result is written to
`build/rv1126b-pack/board-validation/board-result.json`. It records the pack
SHA-256, serial, SDK validation result, archive/model counts, tag/version/major
/ release date, RKNN runtime string, driver string, process exit status, and
peak RSS. Loader success proves archive compatibility only; the eight
provisional artifacts still require recalibration and later inference/accuracy
regression.
