param(
    [Parameter(Mandatory = $true)][string]$Serial,
    [string]$Artifacts = "build/rv1126b-landmark-artifacts",
    [string]$RemoteDirectory = "/userdata/inspireface-rv1126b-landmark"
)

$ErrorActionPreference = "Stop"
if ($RemoteDirectory -notmatch '^/[A-Za-z0-9._/-]+$' -or
    (($RemoteDirectory -split '/') -contains '..')) {
    throw "RemoteDirectory must be a safe absolute path without '..': $RemoteDirectory"
}
$files = @("rknn_landmark_runner", "landmark_rv1126b.rknn", "input_bgr_u8.bin")
foreach ($name in $files) {
    $path = Join-Path $Artifacts $name
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Missing artifact: $path"
    }
}

& adb -s $Serial shell "mkdir -p '$RemoteDirectory'"
if ($LASTEXITCODE -ne 0) { throw "Could not create board test directory" }
foreach ($name in $files) {
    & adb -s $Serial push (Join-Path $Artifacts $name) "$RemoteDirectory/$name"
    if ($LASTEXITCODE -ne 0) { throw "Could not push $name" }
}
& adb -s $Serial shell "chmod 755 '$RemoteDirectory/rknn_landmark_runner' && cd '$RemoteDirectory' && LD_LIBRARY_PATH=/oem/usr/lib ./rknn_landmark_runner landmark_rv1126b.rknn input_bgr_u8.bin rknn_output_f32.bin"
if ($LASTEXITCODE -ne 0) { throw "Board inference failed" }
& adb -s $Serial pull "$RemoteDirectory/rknn_output_f32.bin" (Join-Path $Artifacts "rknn_output_f32.bin")
if ($LASTEXITCODE -ne 0) { throw "Could not pull board output" }
