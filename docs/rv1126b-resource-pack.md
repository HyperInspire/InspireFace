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

The resource-pack loader run recorded below independently confirmed runtime
`2.3.2 (429f97ae6b@2025-04-09T09:10:37)` and driver `0.9.8` on the same serial.

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
copying the validator plus its two required shared libraries. Header checks use
`readelf -h` for ELF32, ARM, EABI5 and hard-float flags; attribute checks use
`readelf -A` for `Tag_ABI_VFP_args: VFP registers`. The runtime version is parsed
as three integers: 2.3.0/2.3.1 fail, while 2.3.2 and 2.4.0 pass.

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

The script pushes a fixed, LF-only `run.sh` and invokes it as separate ADB
arguments (`shell`, `sh`, the fixed script path), without nested `sh -c`
quoting. Both the pack and deployed runtime library are pulled into the unique
run directory for host SHA-256 verification. The runtime version is read from
the returned library bytes, and the driver is read from the board's version
file. Neither `sha256sum` nor `strings` is required on the board.

The atomic local result is written to
`build/rv1126b-pack/board-validation/board-result.json`. It records the pack
SHA-256, serial, SDK validation result, archive/model counts, tag/version/major
/ release date, RKNN runtime string, driver string, process exit status, and
peak RSS. Loader success proves archive compatibility only; the eight
provisional artifacts still require recalibration and later inference/accuracy
regression.

Every invocation has a GUID `run_id` and a separate `<ResultDirectory>/<run_id>`
directory containing its script, raw loader result, exit status, runtime/driver
evidence, and final result. The latest result is marked `running` before
preflight; any failure then writes `status: failed` with an error and this
run ID, so an old success cannot stand in for a failed run. Empty runtime,
driver, or exit-status evidence fails validation. The host checks SDK/process
status, all counts, tag, version and major independently of the C++ checks.

## Verified run on 2026-09-12

The corrected validator was cross-built in the pinned `inspireface-rv1126b`
Docker image using the existing `build/rv1126b-sdk-armhf/InspireFace` SDK.
The repository was mounted read-only at `/code`, the SDK read-only at `/sdk`,
and the ignored validator output directory at `/out`:

```sh
SDK_INSTALL_DIR=/sdk bash /code/command/rv1126b_pack/build_validator.sh /out
```

All three outputs passed the ELF/ABI checks and the required shared-library
dependency checks. Windows pack/inventory regressions ran 44 tests: 42 passed
and two Linux-only build/C++ behavior tests were skipped. Those two tests passed
separately in the pinned Docker image. Fake ADB tests execute the PowerShell
script, capture actual argument arrays and exercise successful deployment,
offline/hash/metadata/runtime/driver failures, and stale-result replacement.
Linux tests execute the build script with controlled tool outputs and compile
the real C++ validator against a stub public API, including its stdout banner.

This Codex host had a local ACL mismatch: the sandbox-created pack report was
readable during ordinary host validation but not by the desktop account used
for ADB. The verified pack/report were copied, without changing their contents,
into `build/rv1126b-pack-validator`, where new files inherit that directory's
readable ACL. Normal users whose pack/report already have readable permissions
can use the default command above. The successful command in this environment
was:

```powershell
$env:ANDROID_SDK_HOME = (Resolve-Path build/adb-home).Path
$env:ANDROID_USER_HOME = $env:ANDROID_SDK_HOME
& ./command/rv1126b_pack/run_board_pack_validation.ps1 `
  -PackPath build/rv1126b-pack-validator/Gundam_RV1126B `
  -BuildReport build/rv1126b-pack-validator/Gundam_RV1126B.report.json `
  -ResultDirectory build/rv1126b-pack-validator/board-validation
```

No HOME variable or original model artifact was changed. Initial failed runs
(the ACL boundary, missing board `strings`, and an SDK banner preceding JSON)
retain their own failure artifacts. The final run exited 0, cleaned only the
resolved fixed board directory, and wrote this result:

```json
{
  "status": "success",
  "sdk_status": 0,
  "archive_file_count": 12,
  "model_count": 11,
  "tag": "Gundam_RV1126B",
  "version": "4.0",
  "major": "t4",
  "release_date": "unknown",
  "peak_rss_kb": 37848,
  "error": "",
  "process_exit_status": 0,
  "runtime": "librknnrt version: 2.3.2 (429f97ae6b@2025-04-09T09:10:37)",
  "run_id": "6ab31601a23e47b7bc01689f8e90dd6f",
  "driver": "RKNPU driver: v0.9.8",
  "serial": "e3d7377f6fc6d325",
  "pack_sha256": "9ec91a21617c00b6b194b37570ee5883764dca169ee1552ac4219b944f4d28d7"
}
```

Raw evidence is under
`build/rv1126b-pack-validator/board-validation/6ab31601a23e47b7bc01689f8e90dd6f`.
Verified selected models remain attitude, emotion and landmark. Selected
provisional models remain liveness, mask, quality, recognition, rnet,
scrfd_2_5g_160, scrfd_2_5g_320 and scrfd_2_5g_640. Loader success does not promote
any calibration status or establish inference/accuracy acceptance.
