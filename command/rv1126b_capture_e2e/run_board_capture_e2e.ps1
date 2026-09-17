[CmdletBinding()]
param(
    [string]$Serial = 'e3d7377f6fc6d325',
    [Parameter(Mandatory = $true)][string]$PackPath,
    [string]$FaceImage = 'test_res/data/bulk/kun.jpg',
    [string]$NoFaceImage = 'test_res/data/crop/no_face.png',
    [string]$RunnerDirectory = '',
    [string]$ResultDirectory = ''
)
$ErrorActionPreference = 'Stop'
$ExpectedSerial = 'e3d7377f6fc6d325'
$RemoteBaseDirectory = '/userdata/inspireface-rv1126b/capture-e2e'
function Get-Sha256 { param([string]$Path) return (Get-FileHash -Algorithm SHA256 -LiteralPath $Path).Hash.ToLowerInvariant() }
function Invoke-Adb { param([string[]]$Arguments) & adb -s $Serial @Arguments; if ($LASTEXITCODE -ne 0) { throw "adb failed ($LASTEXITCODE)" } }
function Invoke-AdbAllowFailure { param([string[]]$Arguments) & adb -s $Serial @Arguments | Out-Host; return [int]$LASTEXITCODE }
if ($Serial -cne $ExpectedSerial) { throw "capture E2E is pinned to serial $ExpectedSerial" }
$Repository = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
if ([string]::IsNullOrWhiteSpace($RunnerDirectory)) { $RunnerDirectory = Join-Path $Repository 'build/rv1126b-capture-e2e' }
if ([string]::IsNullOrWhiteSpace($ResultDirectory)) { $ResultDirectory = Join-Path $RunnerDirectory 'board' }
$PackPath = (Resolve-Path -LiteralPath $PackPath).Path
$FaceImage = (Resolve-Path -LiteralPath (Join-Path $Repository $FaceImage)).Path
$NoFaceImage = (Resolve-Path -LiteralPath (Join-Path $Repository $NoFaceImage)).Path
$RunnerDirectory = (Resolve-Path -LiteralPath $RunnerDirectory).Path
New-Item -ItemType Directory -Force -Path $ResultDirectory | Out-Null
$ResultDirectory = (Resolve-Path -LiteralPath $ResultDirectory).Path
foreach ($name in @('capture_e2e_runner', 'libInspireFace.so', 'librknnrt.so')) {
    if (!(Test-Path -LiteralPath (Join-Path $RunnerDirectory $name) -PathType Leaf)) { throw "runner deployment missing $name" }
}
$packHash = Get-Sha256 $PackPath; $faceHash = Get-Sha256 $FaceImage; $noFaceHash = Get-Sha256 $NoFaceImage
$RunId = 'capture-e2e-' + [DateTime]::UtcNow.ToString('yyyyMMddTHHmmss') + '-' + [Guid]::NewGuid().ToString('N').Substring(0, 12)
$RemoteRunDirectory = "$RemoteBaseDirectory/$RunId"
$RunDirectory = Join-Path $ResultDirectory $RunId
New-Item -ItemType Directory -Force -Path $RunDirectory | Out-Null
try {
    Invoke-Adb @('get-state')
    Invoke-Adb @('shell', 'mkdir', '-p', $RemoteRunDirectory)
    Invoke-Adb @('push', (Join-Path $RunnerDirectory 'capture_e2e_runner'), "$RemoteRunDirectory/capture_e2e_runner")
    Invoke-Adb @('push', (Join-Path $RunnerDirectory 'libInspireFace.so'), "$RemoteRunDirectory/libInspireFace.so")
    Invoke-Adb @('push', (Join-Path $RunnerDirectory 'librknnrt.so'), "$RemoteRunDirectory/librknnrt.so")
    Invoke-Adb @('push', $PackPath, "$RemoteRunDirectory/pack")
    Invoke-Adb @('push', $FaceImage, "$RemoteRunDirectory/face-image")
    Invoke-Adb @('push', $NoFaceImage, "$RemoteRunDirectory/no-face-image")
    $runScript = "#!/bin/sh`nset -eu`ncd $RemoteRunDirectory`nexport LD_LIBRARY_PATH=$RemoteRunDirectory`nexec ./capture_e2e_runner `"`$@`"`n"
    $runScriptPath = Join-Path $RunDirectory 'run-capture-e2e.sh'
    [System.IO.File]::WriteAllText($runScriptPath, $runScript.Replace("`r`n", "`n"), [System.Text.UTF8Encoding]::new($false))
    Invoke-Adb @('push', $runScriptPath, "$RemoteRunDirectory/run-capture-e2e.sh")
    Invoke-Adb @('shell', 'chmod', '755', "$RemoteRunDirectory/capture_e2e_runner", "$RemoteRunDirectory/run-capture-e2e.sh")
    $exit = Invoke-AdbAllowFailure @('shell', 'sh', "$RemoteRunDirectory/run-capture-e2e.sh", '--pack', "$RemoteRunDirectory/pack", '--face-image', "$RemoteRunDirectory/face-image", '--no-face-image', "$RemoteRunDirectory/no-face-image", '--run-id', $RunId, '--serial', $Serial, '--pack-sha256', $packHash, '--face-sha256', $faceHash, '--no-face-sha256', $noFaceHash, '--result-path', "$RemoteRunDirectory/result.json")
    Invoke-Adb @('pull', "$RemoteRunDirectory/result.json", (Join-Path $RunDirectory 'result.json'))
    $result = Get-Content -Raw -LiteralPath (Join-Path $RunDirectory 'result.json') | ConvertFrom-Json
    if ($result.status -cne 'success') { throw "capture runner failed: face_ok=$($result.face_ok) state=$($result.face_final_state) no_face_ok=$($result.no_face_ok) state=$($result.no_face_final_state)" }
    if ($result.run_id -cne $RunId -or $result.serial -cne $Serial -or $result.pack_sha256 -cne $packHash) { throw 'runner evidence mismatch' }
    if ($exit -ne 0) { throw "runner returned $exit despite success evidence" }
    Write-Host "capture e2e passed: face_state=$($result.face_final_state) results=$($result.face_results_collected) avg=$([math]::Round($result.face_avg_update_ms,1))ms no_face_state=$($result.no_face_final_state)"
} finally {
    try { Invoke-Adb @('shell', 'rm', '-rf', $RemoteRunDirectory) } catch { }
}
