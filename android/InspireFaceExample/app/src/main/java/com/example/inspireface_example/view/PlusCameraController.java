package com.example.inspireface_example.view;

import android.content.Context;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.CaptureResult;
import android.hardware.camera2.TotalCaptureResult;
import android.util.Range;
import android.util.Size;

import androidx.annotation.NonNull;
import androidx.camera.camera2.interop.Camera2CameraControl;
import androidx.camera.camera2.interop.Camera2CameraInfo;
import androidx.camera.camera2.interop.Camera2Interop;
import androidx.camera.camera2.interop.CaptureRequestOptions;
import androidx.camera.camera2.interop.ExperimentalCamera2Interop;
import androidx.camera.core.Camera;
import androidx.camera.core.CameraInfo;
import androidx.camera.core.CameraSelector;
import androidx.camera.core.ImageAnalysis;
import androidx.camera.core.Preview;
import androidx.camera.core.resolutionselector.AspectRatioStrategy;
import androidx.camera.core.resolutionselector.ResolutionSelector;
import androidx.camera.core.resolutionselector.ResolutionStrategy;
import androidx.camera.lifecycle.ProcessCameraProvider;
import androidx.camera.view.PreviewView;
import androidx.core.content.ContextCompat;
import androidx.lifecycle.LifecycleOwner;

import com.google.common.util.concurrent.ListenableFuture;

import java.util.LinkedHashMap;
import java.util.List;
import java.util.concurrent.Executor;

/** Front-only CameraX session with exposure metadata; owns only its own use cases. */
@androidx.annotation.OptIn(markerClass = ExperimentalCamera2Interop.class)
final class PlusCameraController {
    interface Listener {
        void onReady(boolean flashSupported);
        void onError();
    }
    static final class Metadata {
        final long index, timestampNs;
        final boolean realtime, locksConfirmed, exposureSettled;
        Metadata(TotalCaptureResult result, boolean realtime) {
            index = result.getFrameNumber();
            Long timestamp = result.get(CaptureResult.SENSOR_TIMESTAMP);
            timestampNs = timestamp == null ? -1 : timestamp;
            this.realtime = realtime;
            locksConfirmed = Boolean.TRUE.equals(result.get(CaptureResult.CONTROL_AE_LOCK))
                    && Boolean.TRUE.equals(result.get(CaptureResult.CONTROL_AWB_LOCK));
            Integer ae = result.get(CaptureResult.CONTROL_AE_STATE), awb = result.get(CaptureResult.CONTROL_AWB_STATE);
            exposureSettled = (Integer.valueOf(CaptureResult.CONTROL_AE_STATE_CONVERGED).equals(ae)
                    || Integer.valueOf(CaptureResult.CONTROL_AE_STATE_LOCKED).equals(ae))
                    && (Integer.valueOf(CaptureResult.CONTROL_AWB_STATE_CONVERGED).equals(awb)
                    || Integer.valueOf(CaptureResult.CONTROL_AWB_STATE_LOCKED).equals(awb));
        }
    }
    private final Context context;
    private final LifecycleOwner owner;
    private final PreviewView view;
    private final Executor executor;
    private final Listener listener;
    private final LinkedHashMap<Long, Metadata> metadata = new LinkedHashMap<>();
    private ProcessCameraProvider provider;
    private Camera camera;
    private Preview preview;
    private ImageAnalysis analysis;
    private volatile boolean closed = true;
    private volatile int generation;
    private boolean realtime, aeAvailable, awbAvailable;
    private long latestIndex = -1;
    private volatile Metadata latestMetadata;

    PlusCameraController(Context context, LifecycleOwner owner, PreviewView view,
                         Executor executor, Listener listener) {
        this.context = context; this.owner = owner; this.view = view;
        this.executor = executor; this.listener = listener;
    }
    void start(ImageAnalysis.Analyzer analyzer) {
        closed = false;
        int token = ++generation;
        ListenableFuture<ProcessCameraProvider> future = ProcessCameraProvider.getInstance(context);
        future.addListener(() -> {
            if (closed || token != generation) return;
            try {
                provider = future.get();
                List<CameraInfo> front = CameraSelector.DEFAULT_FRONT_CAMERA.filter(provider.getAvailableCameraInfos());
                if (front.isEmpty()) throw new IllegalStateException("Front camera required");
                Camera2CameraInfo info = Camera2CameraInfo.from(front.get(0));
                realtime = Integer.valueOf(CameraCharacteristics.SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME)
                        .equals(info.getCameraCharacteristic(CameraCharacteristics.SENSOR_INFO_TIMESTAMP_SOURCE));
                aeAvailable = Boolean.TRUE.equals(info.getCameraCharacteristic(CameraCharacteristics.CONTROL_AE_LOCK_AVAILABLE));
                awbAvailable = Boolean.TRUE.equals(info.getCameraCharacteristic(CameraCharacteristics.CONTROL_AWB_LOCK_AVAILABLE));
                ResolutionSelector resolution = new ResolutionSelector.Builder()
                        .setAspectRatioStrategy(AspectRatioStrategy.RATIO_4_3_FALLBACK_AUTO_STRATEGY)
                        .setResolutionStrategy(new ResolutionStrategy(new Size(640, 480),
                                ResolutionStrategy.FALLBACK_RULE_CLOSEST_HIGHER_THEN_LOWER)).build();
                analysis = new ImageAnalysis.Builder().setResolutionSelector(resolution)
                        .setBackpressureStrategy(ImageAnalysis.STRATEGY_BLOCK_PRODUCER)
                        .setImageQueueDepth(4).build();
                analysis.setAnalyzer(executor, analyzer);
                Preview.Builder builder = new Preview.Builder().setResolutionSelector(resolution);
                Camera2Interop.Extender<Preview> interop = new Camera2Interop.Extender<>(builder);
                interop.setCaptureRequestOption(CaptureRequest.CONTROL_AE_MODE, CaptureRequest.CONTROL_AE_MODE_ON);
                interop.setCaptureRequestOption(CaptureRequest.CONTROL_AWB_MODE, CaptureRequest.CONTROL_AWB_MODE_AUTO);
                Range<Integer>[] rates = info.getCameraCharacteristic(CameraCharacteristics.CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES);
                Range<Integer> chosen = null;
                if (rates != null) for (Range<Integer> rate : rates) {
                    if (rate.getUpper() < 10) continue;
                    if (chosen == null || fpsCost(rate) < fpsCost(chosen)) chosen = rate;
                }
                if (chosen != null) interop.setCaptureRequestOption(CaptureRequest.CONTROL_AE_TARGET_FPS_RANGE, chosen);
                interop.setSessionCaptureCallback(new CameraCaptureSession.CaptureCallback() {
                    @Override public void onCaptureCompleted(@NonNull CameraCaptureSession session,
                            @NonNull CaptureRequest request, @NonNull TotalCaptureResult result) {
                        if (closed || token != generation) return;
                        Metadata entry = new Metadata(result, realtime);
                        if (entry.timestampNs < 0) return;
                        synchronized (metadata) {
                            metadata.put(entry.timestampNs, entry);
                            latestIndex = Math.max(latestIndex, entry.index);
                            if (latestMetadata == null || entry.index > latestMetadata.index) latestMetadata = entry;
                            while (metadata.size() > 64) metadata.remove(metadata.keySet().iterator().next());
                        }
                    }
                });
                preview = builder.build();
                preview.setSurfaceProvider(view.getSurfaceProvider());
                camera = provider.bindToLifecycle(owner, CameraSelector.DEFAULT_FRONT_CAMERA, preview, analysis);
                camera.getCameraInfo().getCameraState().observe(owner, state -> {
                    if (!closed && token == generation && state.getError() != null) listener.onError();
                });
                listener.onReady(realtime && aeAvailable && awbAvailable);
            } catch (Exception e) { if (!closed && token == generation) listener.onError(); }
        }, ContextCompat.getMainExecutor(context));
    }
    private static int fpsCost(Range<Integer> r) {
        return Math.abs(r.getUpper() - 15) * 10 + Math.abs(r.getLower() - r.getUpper());
    }
    Metadata metadata(long timestamp) {
        synchronized (metadata) { return metadata.get(timestamp); }
    }
    long startBarrier() {
        synchronized (metadata) { return realtime ? -1 : latestIndex + 4; }
    }
    boolean exposureSettled() {
        Metadata latest = latestMetadata;
        return !closed && latest != null && latest.realtime && latest.exposureSettled
                && android.os.SystemClock.elapsedRealtimeNanos() - latest.timestampNs < 500_000_000L;
    }
    void setLocks(boolean lock, Runnable done, Runnable failed) {
        if (camera == null || closed) { failed.run(); return; }
        CaptureRequestOptions.Builder builder = new CaptureRequestOptions.Builder();
        if (aeAvailable) builder.setCaptureRequestOption(CaptureRequest.CONTROL_AE_LOCK, lock);
        if (awbAvailable) builder.setCaptureRequestOption(CaptureRequest.CONTROL_AWB_LOCK, lock);
        int token = generation;
        ListenableFuture<Void> future = Camera2CameraControl.from(camera.getCameraControl())
                .setCaptureRequestOptions(builder.build());
        future.addListener(() -> {
            if (closed || token != generation) return;
            try { future.get(); done.run(); }
            catch (Exception e) { failed.run(); }
        }, ContextCompat.getMainExecutor(context));
    }
    void stop() {
        closed = true; generation++;
        if (analysis != null) analysis.clearAnalyzer();
        if (provider != null && preview != null && analysis != null) provider.unbind(preview, analysis);
        if (camera != null) camera.getCameraInfo().getCameraState().removeObservers(owner);
        camera = null;
        synchronized (metadata) { metadata.clear(); latestIndex = -1; latestMetadata = null; }
    }
}
