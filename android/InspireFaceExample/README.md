# InspireFace Android Example

A CameraX-based InspireFace Android SDK (1.2.4.post1) example. The launcher is a square-grid
feature menu with a global model selector (`Pikachu` / `Megatron`). The selected model is
loaded when a feature page opens.

## Try the Android app

<p>
  <a href="http://fir.tunm.top/pro/pz7b3dgv">
    <img src="docs/images/inspireface-android-example-app-download.png" width="220" alt="Download the InspireFace Android example">
  </a>
</p>

<p>
  <strong>Download the Android example and try it now.</strong><br>
  Scan the QR code or <strong><a href="http://fir.tunm.top/pro/pz7b3dgv">Download App</a></strong>.
</p>

The home menu starts with **Anti-fraud**, followed by **Face analysis** and **Face
recognition**. It contains eight local demo pages and two InspireFacePlus verification pages:

- **Silent liveness (RGB anti-spoofing)**: streams a per-frame liveness score for the current face, averages it over a sliding window, and labels the face real/spoof against a threshold.
- **Action liveness (cooperative)**: generates a random challenge sequence (blink / head shake / mouth open / head raise), prompts each action in turn, and detects completion with per-action timeouts and face-loss failure handling.
- **Passive-RGB Liveness · PLUS** and **Color liveness · PLUS**: Button-driven cloud verification with a top-right PLUS badge. Local InspireFace selects faces and supplies eye landmarks. Color captures white/red/green/blue; passive RGB captures 20 consecutive valid frames. Each completed round uploads once to InspireFacePlus. Shield-lock and multicolor-flare icons come from [Google Material Symbols](https://github.com/google/material-design-icons); their license and attribution are bundled in `app/src/main/assets/licenses/`.
- **Pose recognition**: displays whatever action you perform — the latest action in large text, with a fading row of smaller history entries below (at most 6 shown; rising-edge debounced, so a held pose is recorded once).
- **Face 1:1**: selects two local images, detects and numbers every face, selects face 1 by default, and lets you tap any A/B face box to immediately rerun the comparison. The circular gauge shows the converted similarity percentage and the SDK-recommended threshold verdict.
- **Face management**: searches, adds, renames, replaces, and deletes identities in the currently selected model library. Enrollment supports either gallery multi-face selection or an automatic camera flow that tracks face 1, waits for 1 stable second, then fills a 2-second red/yellow/green ring. Motion immediately resets and hides the ring.
- **Face recognition**: switches between Photo input and Video stream tabs. Photo mode detects and numbers faces, searches a single face immediately, and reruns the search when a numbered face is tapped. Its collapsed-by-default settings panel sits below the photo picker; detection input px, maximum face count, and minimum face px rebuild the Session and persist locally. Video mode reuses the CameraX tracking pipeline, tracks only face 0, searches after roughly one stable second, renders the result at the bottom, and supports front/rear cameras.
- **Face tracking**: detects and numbers every image face, then displays the selected face's SDK-native 106 dense landmarks. Medium and large faces are annotated directly; genuinely small faces use a bottom-right 148dp magnifier cropped to 2.4× the detected box. The Video tracking tab renders four-corner boxes and 106 points with OpenGL, adds color-matched Track ID badges, and applies a lightweight three-frame median filter to reduce box jitter.
- **Face attributes**: analyzes a selected image face for mask state, age bracket, image quality, expression state, ethnicity, gender and left/right eye state. Tapping another numbered face updates the result immediately, and small faces reuse the expanded bottom-right crop magnifier.

Debug aids on the local liveness camera pages:

- **Euler angles** switch: live Yaw / Pitch / Roll readout for the first tracked face (~10 Hz). SDK 1.2.4 fixes the old JNI per-face angle indexing bug.
- **Landmarks** rendering is currently hidden; `LandmarkGlView` remains available as an OpenGL ES 2.0 overlay for a future menu entry or debug switch.

On local camera pages, a **Flip camera** chip switches between the front and back lens at runtime. No SDK-side
changes are needed for that: every frame is pre-rotated by its own `rotationDegrees`
before being handed to InspireFace as an upright `CAMERA_ROTATION_0` buffer, so the new
lens's sensor orientation is absorbed per frame — only the display mirroring flips, and
the mode state machine restarts.

Language: the first launch defaults to **English** regardless of the system language. The
language chip switches between English and Chinese, and Android 13+ also exposes both in
the system per-app language settings. The selected language persists across launches.

## PLUS configuration and verification

Copy `plus.local.properties.example` to `plus.local.properties` and set the business
`token` locally. The file is ignored by Git. The API base is configured as
`https://api.inspirehub.cc`; PLUS requests require HTTPS and cleartext traffic is
disabled. Build-time injection keeps credentials out of source
control but does not make a shared token secret inside an APK.

Both PLUS entries first show a separate face-data consent dialog in the selected
app language (English or Chinese). Camera permission requests, SDK analysis and camera
preview start only after **Agree and enter**. Declining or going back closes the page.
Consent lasts for that visit, survives rotation, and is not stored as a permanent
preference; reopening either PLUS feature requires agreement again. Entering does not
start a capture or upload: each round still requires the Start button.
The short notice describes cloud uploads, potential image retention and the limits of
cancelling an already-submitted request. It does not display the API address or link
to developer documentation.

All features share a white interface with soft gray surfaces and a restrained green
accent, including home, photo analysis, comparison, recognition, the face library,
local camera screens and dialogs. The light theme also applies in Android dark mode.
Navigation and button feedback use short eased transitions; result labels and settings
cards animate only when their state changes and respect system animation settings.
Local camera screens have dedicated portrait and landscape arrangements, with matching
preview and overlay bounds.

PLUS adds a compact consent dialog and a circular guide with a quiet breathing ring
and eased progress. Entrance animations never resize the camera viewport or change
capture timing. WRGB illumination updates all light surfaces immediately and suspends
label motion until normal lighting returns.

Both PLUS pages require the front camera. Color additionally requires a REALTIME
sensor clock plus exposure and white-balance locks; unsupported cameras display a
message instead of submitting an unverified light sequence. The screen warms up in
white, waits for convergence, locks AE/AWB, then captures WRGB with a settling guard.
An animated circular guide checks framing and accumulates 600 ms of comfortable alignment
before enabling Start. Brief jitter pauses the ring for up to 300 ms, preserving its
progress; Start requires a fresh valid frame even if the ring was already full.
The guide allows small offsets and uses the face oval instead of detector-box corners.
Subtle blue (far), amber (near) and mint (comfortable) colors ease between positions;
separate entry/exit tolerances prevent boundary flicker.
Brief borderline motion or blur pauses sampling for up to 350 ms and can recover in
the same round. These frames are skipped; passive RGB capture restarts its contiguous
segment internally without requiring another button press.
Both modes check every analyzed frame during preparation and capture:
face loss, multiple faces, leaving the circle, excessive motion or head rotation stop
the round and discard its images. The camera stays open; once steady again, the user
can tap Recapture. The app never automatically starts or resumes a stopped round.
Leaving the foreground or cancelling discards unsubmitted images and cancels local
waiting. An already accepted server request may still finish. Images remain in memory
and are not written to a capture archive.

Run `./gradlew :app:testDebugUnitTest` for preprocessing, capture-state and HTTP
contract tests. To explicitly run the live test with synthetic pixels:

```sh
INSPIREFACE_PLUS_LIVE_TEST=1 ./gradlew :app:testDebugUnitTest --tests '*PlusLiveContractTest'
```

The live test consumes one verification per module, then replays each unchanged
request to check server idempotency. It verifies transport and model output structure,
not real-world liveness accuracy. If Gradle reports this opt-in test as up-to-date after
changing only the environment, use `--rerun-tasks`.

See the [InspireFacePlus API documentation](https://api.inspirehub.cc/docs) for the
service protocol. Local migration notes and generated verification reports are ignored
by Git.

## Architecture

```
CameraX ImageAnalysis (YUV_420_888, 640x480, KEEP_ONLY_LATEST)
   └─ UprightFaceCameraAnalyzer (single-threaded analysis executor)
        ├─ Nv21Converter.convert()        YUV_420_888 → tight NV21 (VU interleave probed once, then bulk-copied)
        ├─ Nv21Converter.rotateUpright()  Java-side rotation to upright
        ├─ CreateImageStreamFromByteBuffer(nv21, CAMERA_ROTATION_0)
        ├─ ExecuteFaceTrack               LIGHT_TRACK mode
        ├─ FaceAnalyzer / EnrollmentFaceAnalyzer
        │    └─ mode pipeline or first-face stability state machine
        └─ ReleaseImageStream             released within the same frame
```

- `HomeActivity` — square-grid feature menu and global model selection
- `FaceCompareActivity` — local image decoding, face feature extraction and 1:1 comparison
- `FaceManagementActivity` — CRUD UI for model-isolated identities, crops and FeatureHub data
- `FaceRecognitionActivity` / `StillImageSessionSettings` — photo multi-face selection, model-scoped 1:N search and persisted Session parameters
- `view/RecognitionFaceAnalyzer` — video face-0 stability gate, feature extraction and model-library search
- `FaceDetectionActivity` / `widget/FaceLandmarkOverlayView` — image multi-face detection, 106-point overlay and small-face magnifier
- `FaceAttributeActivity` / `face/FaceAttributeProcessor` — selectable still-image mask, quality, demographic and interaction attributes
- `view/FaceCaptureActivity` / `EnrollmentFaceAnalyzer` — first-face stable camera enrollment and automatic capture
- `view/CameraPreviewController` — reusable CameraX preview, 4:3 analysis, lens fallback and front/rear switching
- `view/UprightFaceCameraAnalyzer` — shared YUV→upright NV21, face tracking and native stream/session lifecycle
- `face/FaceImageProcessor` / `face/FaceCropUtils` / `widget/FaceImageOverlayView` — shared multi-face extraction, expanded crop and tappable numbered boxes
- `face/FaceRepository` — model-scoped persistent FeatureHub, crop files and metadata
- `view/LivenessActivity` — shared CameraX screen and the silent-liveness entry
- `view/ActionLivenessActivity` / `PoseActivity` — dedicated routes that select their fixed controller mode
- `view/FaceAnalyzer` — liveness-mode pipeline, performance stats and debug readouts
- `view/LivenessController` — the state machines for all three modes (tunables live at the top of this class)
- `view/Nv21Converter` — fast YUV→NV21 conversion + NV21 rotation
- `view/FaceOverlayView` — face bracket overlay (center-crop mapping + front mirror)
- `view/LandmarkGlView` — OpenGL landmark overlay
- `view/FaceEngine` — model-aware GlobalLaunch/GlobalTerminate and session creation
- `FaceModelPrefs` / `LocalePrefs` / `App` — persisted global model and per-app language

## Model-isolated face storage

Pikachu and Megatron never share face features, crop images, metadata, or ID sequences.
The app stores them under separate app-private paths:

```text
files/face_hub/Pikachu/features.db
files/face_hub/Pikachu/crops/
shared_prefs/face_records_Pikachu.xml

files/face_hub/Megatron/features.db
files/face_hub/Megatron/crops/
shared_prefs/face_records_Megatron.xml
```

FeatureHub uses manual primary keys and persistent storage. Switching the global model
therefore opens a different native database as well as a different crop/metadata set.

## Key design decisions (verified against SDK source)

1. **Pre-rotate NV21 on the Java side and always pass `CAMERA_ROTATION_0`.**
   This preserves the coordinate convention shared by detection, liveness, enrollment
   crops and overlays, and the workaround for older SDK liveness crop handling.
   Every SDK output coordinate lands directly in display orientation, so
   the overlays only need the front-camera mirror. Also note the SDK's rotation
   constants are the *opposite* of Android's `rotationDegrees` (Android 90 → SDK
   ROTATION_270); pre-rotation avoids that trap too.

2. **Action liveness configuration**:
   - `DETECT_MODE_LIGHT_TRACK` for continuous tracking and temporal action history;
   - `enableInteractionLiveness` at session creation;
   - `enableFacePose` at session creation (loads the pose model — without pose or quality
     enabled, yaw/pitch stay 0 and shake/head-raise can never trigger).

3. **Action flag semantics** (SDK-internal 10-frame sliding window + rules):
   blink is a one-call pulse (the window resets after it); shake latches while both yaw
   extremes sit in the rolling window (~10 calls); mouth-open/head-raise are
   level-triggered. The controller therefore uses **edge gates**: each challenge step
   must observe the flag at 0 before a 1 counts, and the SDK's `normal` flag (warm-up
   indicator, also raised for ~9 calls after every blink-induced reset) quarantines the
   placeholder zeros so a pose held through a natural blink is neither double-counted
   (pose mode) nor accepted as fresh (action mode).

4. **Image streams own their input data in 1.2.4.** The JNI copies bitmap/byte-array
   pixels into a native buffer retained until `ReleaseImageStream`. The app still uses
   create → track → pipeline → release within one frame and reuses its Java buffers.

5. **Silent liveness**: every tracked frame must fit a centered 2.7× face-width/height
   context region inside the upright source image, with a one-pixel edge inset. This
   matches SDK 1.2.4's RGB crop scale, without its near-edge shrinking or shifting.
   Faces too close to fit prompt the user to move back; off-center faces prompt recentering.
   The 18% minimum face-width gate is temporarily disabled for distant-face accuracy
   testing (`SILENT_MIN_FACE_SIZE_CHECK_ENABLED = false`). Detected small faces can
   reach scoring once the context fits; action liveness still requires the 18% minimum.
   Rejected frames clear the score window and cached verdict before pipeline throttling,
   so scoring resumes with fresh samples after repositioning. This gate applies only to
   local silent liveness (`SilentLivenessFraming.CONTEXT_SCALE`). Valid frames use a
   single-frame score with the author-encoded 0.88 decision boundary;
   every pipeline call converts the full frame internally (a known SDK hot
   spot), so the pipeline runs every 2nd frame with an 8-sample sliding average — same
   accuracy, half the cost.

6. **Dense landmarks are enabled in every detection mode in 1.2.4.** Still images use
   `ALWAYS_DETECT`; video supports `LIGHT_TRACK` and `TRACK_BY_DETECTION` directly.
   The old landmark compatibility bridge has been removed: its `0x200` flag now means
   face pose, not landmark detection. Per-face Euler angles are also fixed upstream.

## SDK 1.2.4.post1 integration

The app consumes the complete SDK AAR: Java APIs, all three native ABIs, Pikachu/Megatron
model packs and JNI consumer rules come from the same package. The former
`inspireface-sdk` shim and its 1.2.0 source/asset dependencies have been removed.
The Android artifact version is **1.2.4.post1**; the native core still reports **1.2.4**,
with **C API level 2**. `FaceEngine` logs all of these and the dependency source.

The default dependency source is JitPack (`inspirefaceSdkSource=jitpack` in
`gradle.properties`), using the published coordinate
`com.github.HyperInspire:inspireface-android-sdk:v1.2.4.post1`. The leading `v` is part
of the published version. See the [JitPack release](https://jitpack.io/#HyperInspire/inspireface-android-sdk/v1.2.4.post1).
JitPack mode never includes or falls back to a local AAR.

```sh
./gradlew --refresh-dependencies \
  :app:assembleDebug :app:assembleRelease :app:testDebugUnitTest :app:lintDebug \
  :app:assembleDebugAndroidTest
# Run on a connected Android device (includes local inference; no PLUS cloud requests):
./gradlew :app:connectedDebugAndroidTest
./gradlew :app:dependencyInsight \
  --dependency inspireface-android-sdk --configuration debugRuntimeClasspath
```

To explicitly test a local SDK build, build `:inspireface:assembleRelease` in the SDK
repository, then copy `sdk.local.properties.example` to `sdk.local.properties` and set
`aarPath` to its `inspireface/build/outputs/aar/inspireface-release.aar`. This
machine-specific file is ignored by Git. Alternatively, pass
`-PinspirefaceSdkAar=/absolute/path/to/the.aar` together with
`-PinspirefaceSdkSource=local`. The AAR is consumed directly; rebuild it in the SDK
repository after SDK changes.

```sh
./gradlew -PinspirefaceSdkSource=local :app:assembleDebug :app:testDebugUnitTest
```

See [the migration analysis and verification record](docs/sdk-1.2.4.post1-upgrade.md)
for API changes, model compatibility and test scope.

## Tunables

At the top of `LivenessController`:

| Parameter | Default | Meaning |
|---|---|---|
| `RGB_LIVENESS_THRESHOLD` | 0.88 | silent-liveness real/spoof boundary |
| `SILENT_MIN_FACE_SIZE_CHECK_ENABLED` | false | enable the 18% minimum width gate in silent mode; disabled for distant-face testing |
| `SCORE_WINDOW` | 8 | sliding average window for scores |
| `SILENT_PIPELINE_INTERVAL` | 2 | run the anti-spoofing pipeline every N frames |
| `ACTIONS_PER_RUN` | 3 | challenge actions per round |
| `ACTION_TIMEOUT_MS` | 8000 | per-action timeout |
| `MIN_FACE_WIDTH_RATIO` | 0.18 | minimum face width for action mode and silent mode when its size gate is enabled |
| `POSE_HISTORY_MAX` | 6 | pose-mode entries shown (1 large + 5 history) |

## Requirements

- JDK 17 (required by AGP 8.6.1; Android Studio's embedded JDK works)
- Android Studio Ladybug+ — the Gradle 8.7 wrapper is committed, no local Gradle needed
- Network access to `google()`, `mavenCentral()` and `jitpack.io` on first sync.
  Explicit local SDK mode requires the complete release AAR.
- An ARM or x86_64 Android device/emulator running Android 7.0 / API 24 or newer. The app compiles and targets
  Android 15 / API 35; Android has no declared upper install limit.
- The native SDK ships `arm64-v8a`, `armeabi-v7a` and `x86_64`; 32-bit x86 is unsupported.
  Its libraries were built with NDK r28b and have 16 KB LOAD-segment alignment. The app
  uses prebuilt libraries and no longer needs a local NDK to build a compatibility bridge.

`local.properties` is intentionally not committed; Android Studio regenerates it, or set
`ANDROID_HOME` for command-line builds.

## Running

The first feature launch is slower while the bundled model packs are unpacked from assets.

```bash
./gradlew :app:installDebug
```

This project now covers liveness, pose, 1:1 comparison, face management, and FeatureHub
1:N photo search. For other SDK capabilities, continue with the upstream
[InspireFace](https://github.com/HyperInspire/InspireFace) Android example.
