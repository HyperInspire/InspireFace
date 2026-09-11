# RV1126B model conversion status

Resource-pack integration gate: **PASS**. Accuracy/release readiness: **NO**.

The successful board matrix is compatibility evidence, not accuracy acceptance.

## Status

| Model | Calibration | Board | Outputs | Latency mean (ms) | Failure stage |
|---|---|---|---:|---:|---|
| attitude | verified | success | 3 | 0.758158 |  |
| emotion | verified | success | 1 | 0.722621 |  |
| landmark | verified | success | 1 | 0.616888 |  |
| liveness | provisional | success | 1 | 0.614192 |  |
| mask | provisional | success | 1 | 0.534275 |  |
| quality | provisional | success | 1 | 0.484196 |  |
| recognition | provisional | success | 1 | 6.574271 |  |
| rnet | provisional | success | 2 | 0.314183 |  |
| scrfd_2_5g_160 | provisional | success | 9 | 2.406296 |  |
| scrfd_2_5g_192 | provisional | success | 9 | 2.901629 |  |
| scrfd_2_5g_256 | provisional | success | 9 | 3.952988 |  |
| scrfd_2_5g_320 | provisional | success | 9 | 5.171962 |  |
| scrfd_2_5g_640 | provisional | success | 9 | 15.350729 |  |
| scrfd_500m_160 | provisional | success | 9 | 2.638000 |  |
| scrfd_500m_192 | provisional | success | 9 | 2.482417 |  |
| scrfd_500m_256 | provisional | success | 9 | 3.428704 |  |
| scrfd_500m_320 | provisional | success | 9 | 3.617938 |  |
| scrfd_500m_640 | provisional | success | 9 | 10.057863 |  |

## Calibration and release

Each provisional model requires restored calibration and accuracy regression before release: liveness, mask, quality, recognition, rnet, scrfd_2_5g_160, scrfd_2_5g_192, scrfd_2_5g_256, scrfd_2_5g_320, scrfd_2_5g_640, scrfd_500m_160, scrfd_500m_192, scrfd_500m_256, scrfd_500m_320, scrfd_500m_640.

Only attitude, emotion, and landmark have verified calibration status. Provisional status never disappears merely because actual board runtime succeeds.

## Evidence and known limitations

Each JSON model entry records source/model/calibration hashes, target Toolkit, native queried tensor contracts, board API/driver, output hashes and finiteness, latency statistics, and any failure stage.
- Preflight failures may leave stale conversion sidecars.
- Direct API calibration validation does not bind every dataset entry to manifest hashes.
- Liveness and quality raw-pixel normalization remains provisional.
- Emotion and liveness conversion logs contain quantization outliers requiring accuracy regression.
- Recognition conversion logs contain compiler diagnostics requiring accuracy regression.
- The runner's version-prefix check and defensive dimension serialization have minor known issues.
