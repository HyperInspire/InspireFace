# RV1126B RKNN2 runtime-parity evidence

## Accepted board run

The production RKNN2 Nano adapter and the standard RKNN path both passed for
the frozen 11-model RV1126B pack.

| Field | Value |
| --- | --- |
| Run ID | `runtime-20260913T135617-b8a988db804a` |
| Board serial | `e3d7377f6fc6d325` |
| Pack SHA-256 | `9ec91a21617c00b6b194b37570ee5883764dca169ee1552ac4219b944f4d28d7` |
| Result | 11/11 rows `success`, finite, and accepted |
| RKNN runtime | `2.3.2 (429f97ae6b@2025-04-09T09:10:37)` |
| RKNN driver | `0.9.8` |
| Evidence | `build/rv1126b-runtime-parity/board/runtime-20260913T135617-b8a988db804a/runtime-parity.json` (ignored generated artifact) |

All rows use the verified pack identity and exact per-model/per-input hashes.
The report also records RNet `width=24, w_stride=32`, nine SCRFD outputs, and
three attitude outputs.

## Measured production path

These values were read from each row's `latency_summary_ms.production_mean` and
`peak_rss_kb` in the accepted report. RSS is Linux `getrusage(RUSAGE_SELF)`
`ru_maxrss`, in KiB.

| Model | Production mean (ms) | Peak RSS (KiB) |
| --- | ---: | ---: |
| `scrfd_2_5g_160` | 8.18 | 10,176 |
| `scrfd_2_5g_320` | 29.68 | 11,772 |
| `scrfd_2_5g_640` | 110.47 | 18,104 |
| `landmark` | 2.72 | 8,372 |
| `rnet` | 0.38 | 8,004 |
| `recognition` | 9.48 | 31,208 |
| `liveness` | 1.79 | 8,492 |
| `mask` | 2.01 | 8,144 |
| `quality` | 1.97 | 8,368 |
| `emotion` | 2.82 | 8,872 |
| `attitude` | 3.01 | 8,684 |

Each result contains ten warmups and ten measured runs for both paths. The
reported latency intentionally includes raw-output retrieval, native-to-logical
diagnostic conversion, and SHA-256 diagnostic hashing; it excludes the separate
input-integrity hashes. It is therefore a parity-evidence cost, not a clean
application inference benchmark.

## Reproduction

Use a Linux ARMHF cross-build host and the official Toolkit2 runtime/header.
`RKNN_RUNTIME_DIR` must name that Toolkit2 directory; `SDK_INSTALL_DIR` is
optional when the standard RV1126B SDK build location is used.

```bash
export RKNN_RUNTIME_DIR=/opt/rknn-toolkit2/Linux/librknn_api
bash command/rv1126b_runtime/build_parity_runner.sh build/rv1126b-runtime-parity
```

Generate an exact raw UINT8/NHWC input for every selected model. The accepted
run used `build/rv1126b-models/calibration/liveness/generated/0000.png`
(`934a2615500ca0e80fb36e63bf453ebf495562bc8fec81095384f1ce7e28f9a8`) as
the source image; the script writes per-model provenance and the immutable input
hash used by the board gate.

```powershell
$ids = 'scrfd_2_5g_160','scrfd_2_5g_320','scrfd_2_5g_640','landmark','rnet',
  'recognition','liveness','mask','quality','emotion','attitude'
$image = 'build/rv1126b-models/calibration/liveness/generated/0000.png'
foreach ($id in $ids) {
  python -B command/rv1126b_runtime/prepare_raw_inputs.py `
    "build/rv1126b-runtime-deploy/models/${id}_rv1126b.json" $image `
    "build/rv1126b-runtime-inputs/$id"
}
```

The board launcher first runs the trusted host pack validator, verifies pack,
model, conversion-sidecar, provenance, and round-trip ADB hashes, then deploys
each model independently to the fixed resolved remote directory.

```powershell
pwsh -NoProfile -File command/rv1126b_runtime/run_board_parity.ps1 `
  -Serial e3d7377f6fc6d325 `
  -PackPath build/rv1126b-runtime-deploy/Gundam_RV1126B `
  -BuildReport build/rv1126b-runtime-deploy/Gundam_RV1126B.report.json `
  -ArtifactRoot build/rv1126b-runtime-deploy `
  -RawInputDirectory build/rv1126b-runtime-inputs `
  -RunnerDirectory build/rv1126b-runtime-parity `
  -ResultDirectory build/rv1126b-runtime-parity/board
```

## Acceptance gate

The engineering-parity gate is true only if all 11 rows pass their input and
output contracts, have ten finite latency samples in each path, and publish
finite diagnostics. For every output:

- Same-run checks are strict: reference raw-versus-float and production native
  storage decoded independently versus the wrapper's logical FP32 view require
  `max_abs <= 1e-5` and cosine `>= 0.999999`.
- Reference and production self-repeat, plus the cross-path observation, must
  be finite and stay below the independent hard bounds `max_abs <= 1.0` and
  cosine `>= 0.90`.
- The cross-path observation is additionally constrained by the measured
  nondeterminism envelope: `max_abs <= max(reference_self, production_self) +
  1e-5` and cosine `>= min(reference_self, production_self) - 1e-6`.

The hard bounds prevent a large self-repeat drift from expanding the envelope
without limit. They accommodate the observed SCRFD-320 run-to-run behavior but
do not relax either same-run conversion check.

This is an engineering compatibility result, not calibration acceptance. Eight
models remain provisional and require a real calibration set before promotion:
`scrfd_2_5g_160`, `scrfd_2_5g_320`, `scrfd_2_5g_640`, `rnet`, `recognition`,
`liveness`, `mask`, and `quality`.
