package com.example.inspireface_example.view;

import android.content.res.ColorStateList;
import android.graphics.Color;
import android.os.Bundle;
import com.example.inspireface_example.ui.UiMotion;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.view.View;
import android.view.WindowManager;
import android.widget.TextView;

import androidx.appcompat.app.AlertDialog;
import com.example.inspireface_example.ui.UiActivity;
import androidx.camera.camera2.interop.ExperimentalCamera2Interop;
import androidx.camera.view.PreviewView;
import androidx.core.graphics.Insets;
import androidx.core.graphics.ColorUtils;
import androidx.core.content.ContextCompat;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.lifecycle.ViewModel;
import androidx.lifecycle.ViewModelProvider;

import com.example.inspireface_example.BuildConfig;
import com.example.inspireface_example.R;
import com.example.inspireface_example.permission.CameraPermissionCoordinator;
import com.example.inspireface_example.plus.CapturePacket;
import com.example.inspireface_example.plus.FaceFrame;
import com.example.inspireface_example.plus.PlusFaceStability;
import com.example.inspireface_example.plus.PlusCaptureCoordinator;
import com.example.inspireface_example.plus.PlusCaptureCoordinator.State;
import com.example.inspireface_example.plus.PlusLivenessClient;
import com.google.android.material.button.MaterialButton;
import com.google.android.material.card.MaterialCardView;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import com.google.android.material.progressindicator.LinearProgressIndicator;

import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import okhttp3.Request;

/** Button-driven PLUS recording. All UI transitions belong to one capture generation. */
@androidx.annotation.OptIn(markerClass = ExperimentalCamera2Interop.class)
public final class PlusLivenessActivity extends UiActivity {
    public static final String EXTRA_COLOR = "plus_color";
    private static final int BACKGROUND = Color.WHITE;
    private static final int[] COLORS = { Color.WHITE, Color.RED, Color.GREEN, Color.BLUE };
    private static final int[] COLOR_NAMES = { R.string.plus_white, R.string.plus_red, R.string.plus_green, R.string.plus_blue };
    private final Handler timers = new Handler(Looper.getMainLooper());
    private final ExecutorService analysisWorker = Executors.newSingleThreadExecutor();
    private final ExecutorService packetWorker = Executors.newSingleThreadExecutor();
    private final PlusCaptureCoordinator coordinator = new PlusCaptureCoordinator();
    private final PlusFaceStability stability = new PlusFaceStability();
    private final PlusLivenessClient client = new PlusLivenessClient();
    private CameraPermissionCoordinator permission;
    private ConsentState consent;
    private AlertDialog consentDialog;
    private boolean captureUiReady;
    private PlusCameraController camera;
    private PlusFaceAnalyzer analyzer;
    private PreviewView preview;
    private PlusFaceGuideView guide;
    private View root;
    private TextView status, detail, guideHint;
    private MaterialButton start, retry;
    private LinearProgressIndicator progress;
    private boolean color, resumed, startingEngine, cameraReady, flashSupported, transitionScheduled;
    private boolean lightChanged, recovering;
    private float originalBrightness;
    private int cameraEpoch, networkAttempt, phase = -1;
    private long phaseShownNs, lastFaceReceived;
    private FaceFrame lastFace;
    private CapturePacket frozenPacket;
    private Request frozenRequest;

    @Override protected void onCreate(Bundle state) {
        super.onCreate(state);
        color = getIntent().getBooleanExtra(EXTRA_COLOR, true);
        consent = new ViewModelProvider(this).get(ConsentState.class);
        WindowCompat.setDecorFitsSystemWindows(getWindow(), false);
        WindowCompat.getInsetsController(getWindow(), getWindow().getDecorView())
                .setAppearanceLightNavigationBars(true);
        WindowCompat.getInsetsController(getWindow(), getWindow().getDecorView())
                .setAppearanceLightStatusBars(true);
        // Register the permission launcher before STARTED, but do not request access yet.
        permission = new CameraPermissionCoordinator(this, new CameraPermissionCoordinator.Listener() {
            @Override public void onCameraPermissionGranted() { startEngine(); }
            @Override public void onCameraPermissionBlocked(boolean settings) {
                if (!captureUiReady) return;
                status.setText(R.string.msg_permission_required);
                detail.setText(settings ? R.string.camera_permission_settings_hint : R.string.camera_permission_retry_hint);
                updateControls();
            }
        });
        if (consent.granted) showCapturePage();
        else showConsentDialog();
    }

    private void showConsentDialog() {
        setContentView(R.layout.activity_plus_consent);
        applyInsets(findViewById(R.id.plusConsentRoot));
        String feature = getString(color ? R.string.home_color_plus_title : R.string.home_silent_plus_title);
        ((TextView) findViewById(R.id.plusConsentFeature)).setText(feature);
        consentDialog = new MaterialAlertDialogBuilder(this, R.style.ThemeOverlay_InspireFaceExample_PlusConsent)
                .setTitle(R.string.plus_consent_title)
                .setMessage(getString(R.string.plus_consent_message, feature))
                .setNegativeButton(R.string.plus_consent_decline, (dialog, which) -> finish())
                .setPositiveButton(R.string.plus_consent_agree, (dialog, which) -> {
                    if (isFinishing() || consent.granted) return;
                    dialog.dismiss(); consentDialog = null;
                    consent.granted = true;
                    showCapturePage();
                })
                .setOnCancelListener(dialog -> finish())
                .show();
    }

    private void showCapturePage() {
        if (!consent.granted || captureUiReady) return;
        setContentView(R.layout.activity_plus_liveness);
        root = findViewById(R.id.plusRoot);
        preview = findViewById(R.id.plusPreview); guide = findViewById(R.id.plusGuide);
        guideHint = findViewById(R.id.plusGuideHint);
        status = findViewById(R.id.plusStatus); detail = findViewById(R.id.plusDetail);
        start = findViewById(R.id.plusStart); retry = findViewById(R.id.plusRetry);
        progress = findViewById(R.id.plusProgress);
        ((TextView) findViewById(R.id.plusTitle)).setText(color ? R.string.home_color_plus_title : R.string.home_silent_plus_title);
        findViewById(R.id.plusBack).setOnClickListener(v -> finish());
        start.setOnClickListener(v -> {
            if (coordinator.isRunning()) cancelRound(true);
            else startRound();
        });
        retry.setOnClickListener(v -> submit(coordinator.generation()));
        applyInsets(root);
        captureUiReady = true;
        guide.setAnimating(resumed);
        UiMotion.enter(findViewById(R.id.plusHeader), 0);
        UiMotion.enter(findViewById(R.id.plusControls), 45);
        permission.bindRecoveryButton(findViewById(R.id.plusPermission));
        updateControls();
        permission.requestAccess();
    }

    @Override protected void onResume() {
        super.onResume(); resumed = true;
        if (!consent.granted || !captureUiReady) return;
        guide.setAnimating(true);
        permission.onResume();
        if (permission.hasPermission()) startEngine();
    }

    private void startEngine() {
        if (!consent.granted || !captureUiReady || !resumed || startingEngine || camera != null) return;
        startingEngine = true;
        int epoch = ++cameraEpoch;
        status.setText(R.string.msg_initializing);
        analysisWorker.execute(() -> {
            boolean launched = FaceEngine.ensureLaunched(this);
            runOnUiThread(() -> {
                if (!resumed || isDestroyed() || epoch != cameraEpoch) return;
                startingEngine = false;
                if (!launched) { status.setText(R.string.msg_engine_failed); return; }
                camera = new PlusCameraController(this, this, preview, analysisWorker,
                        new PlusCameraController.Listener() {
                            @Override public void onReady(boolean supported) {
                                if (!resumed || epoch != cameraEpoch) return;
                                cameraReady = true; flashSupported = supported;
                                coordinator.ready(); stability.reset(); recovering = false;
                                status.setText(R.string.plus_ready);
                                detail.setText(color && !supported ? R.string.plus_flash_unsupported : R.string.plus_align);
                                if (BuildConfig.PLUS_TOKEN.isEmpty()) detail.setText(R.string.plus_config_missing);
                                updateControls();
                            }
                            @Override public void onError() { cameraError(epoch); }
                        });
                analyzer = new PlusFaceAnalyzer(camera, coordinator, guide, new PlusFaceAnalyzer.Listener() {
                    @Override public void onFrame(long generation, FaceFrame face) {
                        runOnUiThread(() -> {
                            if (resumed && epoch == cameraEpoch && generation == coordinator.generation()) onFace(generation, face);
                        });
                    }
                    @Override public void onEngineError() { runOnUiThread(() -> cameraError(epoch)); }
                }, color);
                camera.start(analyzer);
            });
        });
    }

    private void cameraError(int epoch) {
        if (!resumed || epoch != cameraEpoch) return;
        cancelRound(false); cameraReady = false;
        status.setText(R.string.plus_error_title); detail.setText(R.string.plus_camera_error);
        stopCamera(); updateControls();
    }

    private void onFace(long token, FaceFrame face) {
        lastFace = face; lastFaceReceived = SystemClock.elapsedRealtimeNanos();
        guide.setAlignment(face);
        State state = coordinator.state();
        boolean hasFace = face.trackId >= 0;
        if (state == State.ERROR && coordinator.failure() != PlusCaptureCoordinator.Failure.NONE) {
            captureFailed(failureHint(coordinator.failure()));
            return;
        }
        if (!coordinator.isRunning()) {
            float steady = stability.observe(face, lastFaceReceived);
            boolean ready = stability.ready(lastFaceReceived);
            if ((state == State.READY || state == State.CANCELLED) && cameraReady
                    && (!color || flashSupported) && !BuildConfig.PLUS_TOKEN.isEmpty()) {
                status.setText(ready ? (recovering ? R.string.plus_ready_again : R.string.plus_ready)
                        : recovering ? R.string.plus_interrupted : R.string.plus_ready);
                detail.setText(ready ? R.string.plus_positioned : face.valid() ? R.string.plus_stabilize : faceHint(face));
                guide.setGuideState(ready ? PlusFaceGuideView.Mode.READY
                        : face.valid() || steady > 0 ? PlusFaceGuideView.Mode.HOLD
                        : recovering ? PlusFaceGuideView.Mode.WARNING : PlusFaceGuideView.Mode.ALIGN, steady, hasFace);
            }
            guideHint.setText(ready ? R.string.plus_stable_ready : face.valid() ? R.string.plus_stabilize : faceHint(face));
            updateControls();
            return;
        }
        if (state == State.PREPARING) {
            guide.setGuideState(PlusFaceGuideView.Mode.CAPTURE, 0, hasFace);
            guideHint.setText(R.string.plus_hold);
            detail.setText(R.string.plus_hold);
        } else if (state == State.CAPTURING || state == State.ENCODING) {
            int count = coordinator.count();
            float fraction = count / (float) (color ? 4 : 20);
            progress.setProgressCompat(Math.round(fraction * 100), true);
            guide.setGuideState(PlusFaceGuideView.Mode.CAPTURE, fraction, hasFace);
            guideHint.setText(R.string.plus_keep_stable);
            if (!color) {
                status.setText(R.string.plus_rgb_progress);
                detail.setText(R.string.plus_rgb_hold);
                if (state == State.ENCODING && !transitionScheduled) {
                    transitionScheduled = true; encodeAndSubmit(token);
                }
            } else {
                detail.setText(R.string.plus_hold);
                if (count > phase && phase >= 0 && !transitionScheduled) {
                    transitionScheduled = true;
                    long delayMs = Math.max(0, 500 - (SystemClock.elapsedRealtimeNanos() - phaseShownNs) / 1_000_000);
                    timers.postDelayed(() -> {
                        if (!current(token)) return;
                        if (count == 4) encodeAndSubmit(token);
                        else showPhase(token, count);
                    }, delayMs);
                }
            }
        }
        if ((state == State.PREPARING || state == State.CAPTURING)
                && coordinator.pendingFailure() != PlusCaptureCoordinator.Failure.NONE) {
            guideHint.setText(R.string.plus_adjusting);
            detail.setText(R.string.plus_adjusting);
        }
    }

    private int failureHint(PlusCaptureCoordinator.Failure failure) {
        switch (failure) {
            case FACE_LOST: return R.string.plus_face_lost;
            case MULTIPLE_FACES: return R.string.plus_multiple;
            case POSE: return R.string.plus_pose_changed;
            case OUTSIDE_GUIDE: return R.string.plus_outside_guide;
            case QUALITY: return R.string.plus_quality;
            case GEOMETRY: return R.string.plus_moved;
            default: return R.string.plus_processing_error;
        }
    }

    private int faceHint(FaceFrame f) {
        switch (f.issue) {
            case MULTIPLE: return R.string.plus_multiple;
            case POSE: return R.string.plus_pose;
            case DISTANCE:
                if (f.guide != null && f.guide.issue == FaceFrame.Issue.TOO_CLOSE) return R.string.plus_move_away;
                if (f.guide != null && f.guide.issue == FaceFrame.Issue.TOO_FAR) return R.string.plus_move_closer;
                return R.string.plus_distance;
            case OUTSIDE_GUIDE: return R.string.plus_outside_guide;
            case TOO_CLOSE: return R.string.plus_move_away;
            case TOO_FAR: return R.string.plus_move_closer;
            case QUALITY: return R.string.plus_quality;
            case METADATA: return R.string.plus_metadata;
            case EYES: case NONE: return R.string.plus_eyes;
            default: return R.string.plus_align;
        }
    }

    private void startRound() {
        if (!consent.granted || !resumed || !cameraReady || (color && !flashSupported) || BuildConfig.PLUS_TOKEN.isEmpty()) return;
        if (lastFace == null || !lastFace.valid() || !stability.ready(SystemClock.elapsedRealtimeNanos())) {
            detail.setText(lastFace == null ? R.string.plus_align : lastFace.valid() ? R.string.plus_stabilize : faceHint(lastFace));
            updateControls(); return;
        }
        // Finish decorative motion before the first illumination or sampled frame.
        finishEntrance();
        client.cancel(); networkAttempt++;
        frozenPacket = null; frozenRequest = null; timers.removeCallbacksAndMessages(null);
        long token = coordinator.start(color, SystemClock.elapsedRealtimeNanos(), camera.startBarrier(), lastFace);
        recovering = false; stability.reset();
        guide.setGuideState(PlusFaceGuideView.Mode.CAPTURE, 0, true);
        guideHint.setText(R.string.plus_keep_stable);
        phase = -1; transitionScheduled = false;
        progress.setIndeterminate(false); progress.setProgressCompat(0, false);
        retry.setVisibility(View.GONE);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        updateControls();
        if (color) {
            originalBrightness = getWindow().getAttributes().screenBrightness; lightChanged = true;
            WindowManager.LayoutParams attributes = getWindow().getAttributes();
            attributes.screenBrightness = 1f; getWindow().setAttributes(attributes);
            applyIllumination(Color.WHITE); status.setText(R.string.plus_warming); detail.setText(R.string.plus_hold);
            camera.setLocks(false, () -> {
                if (!current(token)) return;
                timers.postDelayed(() -> {
                    if (!current(token)) return;
                    lockAfterSettling(token, SystemClock.elapsedRealtime() + 3500);
                }, 1200);
            }, () -> { if (current(token)) captureFailed(R.string.plus_camera_error); });
        } else {
            status.setText(R.string.plus_rgb_progress); detail.setText(R.string.plus_rgb_hold);
        }
        timers.postDelayed(() -> {
            State state = coordinator.state();
            if (current(token) && (state == State.PREPARING || state == State.CAPTURING)) captureFailed(R.string.plus_timeout);
        }, color ? 20000 : 30000);
        watchCapture(token);
    }

    private void lockAfterSettling(long token, long deadlineMs) {
        if (!current(token)) return;
        if (camera.exposureSettled()) {
            camera.setLocks(true, () -> { if (current(token)) showPhase(token, 0); },
                    () -> { if (current(token)) captureFailed(R.string.plus_camera_error); });
        } else if (SystemClock.elapsedRealtime() >= deadlineMs) captureFailed(R.string.plus_timeout);
        else timers.postDelayed(() -> lockAfterSettling(token, deadlineMs), 100);
    }

    private void showPhase(long token, int next) {
        if (!current(token)) return;
        phase = next; transitionScheduled = false;
        applyIllumination(COLORS[next]);
        status.setText(getString(R.string.plus_phase, next + 1, getString(COLOR_NAMES[next])));
        // Two vsyncs after invalidation give a conservative local display boundary.
        // This estimate is not uploaded as a claimed hardware display timestamp.
        root.postOnAnimation(() -> root.postOnAnimation(() -> {
            if (!current(token) || phase != next) return;
            phaseShownNs = SystemClock.elapsedRealtimeNanos();
            if (!coordinator.beginPhase(token, next, phaseShownNs)) return;
            timers.postDelayed(() -> {
                if (current(token) && phase == next && coordinator.count() <= next) captureFailed(R.string.plus_timeout);
            }, 3500);
        }));
    }

    private void encodeAndSubmit(long token) {
        if (!current(token) || coordinator.state() != State.ENCODING) return;
        timers.removeCallbacksAndMessages(null); restoreLight();
        status.setText(R.string.plus_encoding); detail.setText(R.string.plus_upload_detail);
        guide.setGuideState(PlusFaceGuideView.Mode.VERIFY, 1, true);
        guideHint.setText(R.string.plus_capture_done);
        progress.setIndeterminate(true);
        List<PlusCaptureCoordinator.Sample> samples = coordinator.snapshot(token);
        String id = coordinator.captureId();
        packetWorker.execute(() -> {
            try {
                CapturePacket packet = new CapturePacket(id, color, samples);
                Request request = packet.request(BuildConfig.PLUS_API_BASE, BuildConfig.PLUS_TOKEN);
                runOnUiThread(() -> {
                    if (!current(token)) return;
                    frozenPacket = packet; frozenRequest = request; submit(token);
                });
            } catch (Exception e) {
                runOnUiThread(() -> { if (current(token)) captureFailed(R.string.plus_processing_error); });
            }
        });
    }

    private void submit(long token) {
        if (!resumed || token != coordinator.generation() || frozenRequest == null || frozenPacket == null
                || !coordinator.uploading(token)) return;
        retry.setVisibility(View.GONE); progress.setIndeterminate(true);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        status.setText(R.string.plus_uploading); detail.setText(R.string.plus_upload_detail); updateControls();
        guide.setGuideState(PlusFaceGuideView.Mode.VERIFY, 1, true);
        int attempt = ++networkAttempt;
        client.submit(frozenRequest, frozenPacket, new PlusLivenessClient.Listener() {
            @Override public void onResult(PlusLivenessClient.Result result) {
                runOnUiThread(() -> {
                    if (!current(token) || attempt != networkAttempt
                            || !coordinator.finish(token, result.retry ? State.RETRY : State.RESULT)) return;
                    frozenPacket = null; frozenRequest = null;
                    stability.reset();
                    guide.setGuideState(result.retry ? PlusFaceGuideView.Mode.WARNING
                            : result.alive ? PlusFaceGuideView.Mode.SUCCESS : PlusFaceGuideView.Mode.FAILURE, 1, true);
                    progress.setIndeterminate(false); progress.setProgressCompat(100, false);
                    status.setText(result.retry ? R.string.plus_result_retry : result.alive ? R.string.plus_pass : R.string.plus_fail);
                    detail.setText(result.retry ? getString(R.string.plus_retry_detail) : getString(R.string.plus_score, result.score));
                    if (BuildConfig.DEBUG) {
                        String requestId = result.requestId.replaceAll("[^a-zA-Z0-9_-]", "");
                        android.util.Log.i("InspireFacePlus", "request_id="
                                + requestId.substring(0, Math.min(96, requestId.length()))
                                + " final_window_frames=" + result.windowFrames);
                    }
                    getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON); updateControls();
                });
            }
            @Override public void onError(PlusLivenessClient.Error error, boolean canRetry, int waitSeconds) {
                runOnUiThread(() -> {
                    if (!current(token) || attempt != networkAttempt || !coordinator.finish(token, State.ERROR)) return;
                    progress.setIndeterminate(false); status.setText(R.string.plus_error_title); detail.setText(errorHint(error));
                    guide.setGuideState(PlusFaceGuideView.Mode.WARNING, 0, true); stability.reset();
                    getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
                    if (canRetry) {
                        retry.setVisibility(View.VISIBLE); retry.setEnabled(false); retry.setText(R.string.plus_retry_wait);
                        timers.postDelayed(() -> {
                            if (resumed && token == coordinator.generation() && attempt == networkAttempt && coordinator.state() == State.ERROR) {
                                retry.setEnabled(true); retry.setText(R.string.plus_retry_upload);
                            }
                        }, waitSeconds * 1000L);
                    } else { frozenPacket = null; frozenRequest = null; }
                    updateControls();
                });
            }
        });
    }

    private int errorHint(PlusLivenessClient.Error error) {
        switch (error) {
            case AUTH: return R.string.plus_error_auth;
            case FORBIDDEN: return R.string.plus_error_forbidden;
            case QUOTA: return R.string.plus_error_quota;
            case BUSY: return R.string.plus_error_busy;
            case CONFLICT: return R.string.plus_error_conflict;
            case INPUT: return R.string.plus_error_input;
            case SERVER: return R.string.plus_error_server;
            case RESPONSE: return R.string.plus_error_response;
            default: return R.string.plus_error_network;
        }
    }
    private boolean current(long token) { return resumed && !isDestroyed() && token == coordinator.generation() && coordinator.isRunning(); }

    private void captureFailed(int message) {
        cancelRound(false); recovering = true;
        status.setText(R.string.plus_interrupted); detail.setText(message);
        guideHint.setText(R.string.plus_realign);
        guide.setGuideState(PlusFaceGuideView.Mode.WARNING, 0, lastFace != null && lastFace.trackId >= 0);
    }
    private void cancelRound(boolean show) {
        boolean uploaded = frozenRequest != null;
        coordinator.cancel(); networkAttempt++; client.cancel(); timers.removeCallbacksAndMessages(null);
        frozenPacket = null; frozenRequest = null; transitionScheduled = false;
        stability.reset(); recovering = false;
        guide.setGuideState(PlusFaceGuideView.Mode.ALIGN, 0, false);
        guideHint.setText(R.string.plus_align);
        restoreLight(); progress.setIndeterminate(false); progress.setProgressCompat(0, false);
        retry.setVisibility(View.GONE); getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        if (show) {
            status.setText(R.string.plus_cancelled);
            detail.setText(uploaded ? R.string.plus_cancel_upload : R.string.plus_cancel_detail);
        }
        updateControls();
    }
    private void restoreLight() {
        if (!lightChanged) return;
        lightChanged = false;
        applyIllumination(Color.TRANSPARENT);
        WindowManager.LayoutParams attributes = getWindow().getAttributes();
        attributes.screenBrightness = originalBrightness; getWindow().setAttributes(attributes);
        if (camera != null) camera.setLocks(false, () -> {}, () -> {});
    }
    private void applyIllumination(int color) {
        boolean neutral = color == Color.TRANSPARENT;
        boolean darkText = neutral || color == Color.WHITE || color == Color.GREEN;
        root.setBackgroundColor(neutral ? BACKGROUND : color);
        guide.setIllumination(color);
        WindowCompat.getInsetsController(getWindow(), getWindow().getDecorView()).setAppearanceLightStatusBars(darkText);
        WindowCompat.getInsetsController(getWindow(), getWindow().getDecorView()).setAppearanceLightNavigationBars(darkText);
        ((TextView) findViewById(R.id.plusTitle)).setTextColor(darkText ? getColor(R.color.plus_text) : Color.WHITE);
        int foreground = darkText ? getColor(R.color.plus_text) : Color.WHITE;
        TextView back = findViewById(R.id.plusBack), badge = findViewById(R.id.plusBadge);
        back.setTextColor(foreground);
        badge.setTextColor(neutral ? getColor(R.color.plus_badge_text) : foreground);
        // White cards must not wash out red/green/blue illumination. Change all surfaces
        // synchronously, before the two-vsync capture barrier; never crossfade WRGB colors.
        ColorStateList phaseSurface = neutral ? null : ColorStateList.valueOf(ColorUtils.blendARGB(color, Color.BLACK, .06f));
        back.setBackgroundTintList(phaseSurface);
        badge.setBackgroundTintList(phaseSurface);
        MaterialCardView panel = findViewById(R.id.plusStatusCard);
        panel.setCardBackgroundColor(neutral ? getColor(R.color.plus_surface) : color);
        panel.setStrokeColor(neutral ? getColor(R.color.plus_border) : ColorUtils.blendARGB(color, Color.BLACK, .12f));
        status.setTextColor(foreground);
        detail.setTextColor(neutral ? getColor(R.color.plus_text_secondary) : foreground);
        progress.setIndicatorColor(neutral ? getColor(R.color.plus_accent) : foreground);
        progress.setTrackColor(neutral ? getColor(R.color.plus_progress_track) : ColorUtils.blendARGB(color, Color.BLACK, .12f));
        start.setBackgroundTintList(neutral ? ContextCompat.getColorStateList(this, R.color.plus_start_background) : phaseSurface);
        start.setTextColor(neutral ? ContextCompat.getColorStateList(this, R.color.plus_start_text) : ColorStateList.valueOf(foreground));
        for (int id : new int[]{R.id.plusBrand, R.id.plusNotice, R.id.plusGuideHint}) {
            ((TextView) findViewById(id)).setTextColor(darkText ? getColor(R.color.plus_text_secondary) : Color.WHITE);
        }
        for (TextView label : new TextView[]{status, detail, guideHint}) {
            ((AnimatedLabelView) label).setAnimateChanges(neutral);
        }
    }

    private void watchCapture(long token) {
        if (!current(token) || (coordinator.state() != State.PREPARING && coordinator.state() != State.CAPTURING)) return;
        if (SystemClock.elapsedRealtimeNanos() - lastFaceReceived > 800_000_000L) {
            captureFailed(R.string.plus_frame_lost); return;
        }
        timers.postDelayed(() -> watchCapture(token), 200);
    }

    private void updateControls() {
        boolean active = coordinator.isRunning();
        int caption = active ? R.string.plus_cancel : coordinator.state() == State.READY ? R.string.plus_start : R.string.plus_again;
        String text = getString(caption);
        if (!text.contentEquals(start.getText())) start.setText(text);
        start.setActivated(active);
        start.setEnabled(resumed && (active || (cameraReady && (!color || flashSupported)
                && !BuildConfig.PLUS_TOKEN.isEmpty() && stability.ready(SystemClock.elapsedRealtimeNanos()))));
    }
    private void stopCamera() {
        cameraEpoch++; startingEngine = false; cameraReady = false; lastFace = null;
        if (analyzer != null) {
            PlusFaceAnalyzer old = analyzer; old.stopAccepting(); analysisWorker.execute(old::release); analyzer = null;
        }
        if (camera != null) { camera.stop(); camera = null; }
        stability.reset();
        guide.setGuideState(PlusFaceGuideView.Mode.ALIGN, 0, false);
    }
    @Override protected void onPause() {
        resumed = false;
        if (captureUiReady) {
            finishEntrance();
            guide.setAnimating(false); cancelRound(false); stopCamera();
        }
        super.onPause();
    }

    private void finishEntrance() {
        UiMotion.finish(findViewById(R.id.plusHeader));
        UiMotion.finish(findViewById(R.id.plusControls));
    }
    @Override protected void onDestroy() {
        if (consentDialog != null) { consentDialog.dismiss(); consentDialog = null; }
        permission.close(); timers.removeCallbacksAndMessages(null); client.cancel();
        analysisWorker.shutdown(); packetWorker.shutdown(); super.onDestroy();
    }

    private void applyInsets(View page) {
        int left = page.getPaddingLeft(), top = page.getPaddingTop();
        int right = page.getPaddingRight(), bottom = page.getPaddingBottom();
        ViewCompat.setOnApplyWindowInsetsListener(page, (v, insets) -> {
            Insets bars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(left + bars.left, top + bars.top, right + bars.right, bottom + bars.bottom);
            return insets;
        });
        ViewCompat.requestApplyInsets(page);
    }

    /** Per-visit consent survives rotation, but is never persisted or accepted from an Intent. */
    public static final class ConsentState extends ViewModel {
        boolean granted;
    }
}
