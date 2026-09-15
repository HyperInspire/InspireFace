[CmdletBinding()]
param(
    [string]$Serial = 'e3d7377f6fc6d325',
    [Parameter(Mandatory = $true)][string]$PackPath,
    [string]$FaceImage = 'test_res/data/bulk/kun.jpg',
    [ValidateSet('cpu', 'rga')][string]$ImageBackend = 'cpu',
    [string]$RunnerDirectory = '',
    [string]$ResultDirectory = ''
)
$ErrorActionPreference = 'Stop'
$ExpectedSerial = 'e3d7377f6fc6d325'
$RemoteBaseDirectory = '/userdata/inspireface-rv1126b/concurrent-e2e'
function Get-Sha256 { param([string]$Path) return (Get-FileHash -Algorithm SHA256 -LiteralPath $Path).Hash.ToLowerInvariant() }
function Invoke-Adb { param([string[]]$Arguments) & adb -s $Serial @Arguments; if ($LASTEXITCODE -ne 0) { throw "adb failed ($LASTEXITCODE)" } }
function Invoke-AdbAllowFailure { param([string[]]$Arguments) & adb -s $Serial @Arguments | Out-Host; return [int]$LASTEXITCODE }
if ($Serial -cne $ExpectedSerial) { throw "concurrent E2E is pinned to serial $ExpectedSerial" }
$Repository = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
if ([string]::IsNullOrWhiteSpace($RunnerDirectory)) { $RunnerDirectory = Join-Path $Repository 'build/rv1126b-concurrent-e2e' }
if ([string]::IsNullOrWhiteSpace($ResultDirectory)) { $ResultDirectory = Join-Path $RunnerDirectory 'board' }
$PackPath = (Resolve-Path -LiteralPath $PackPath).Path
$FaceImage = (Resolve-Path -LiteralPath (Join-Path $Repository $FaceImage)).Path
$RunnerDirectory = (Resolve-Path -LiteralPath $RunnerDirectory).Path
New-Item -ItemType Directory -Force -Path $ResultDirectory | Out-Null
$ResultDirectory = (Resolve-Path -LiteralPath $ResultDirectory).Path
foreach ($name in @('concurrent_e2e_runner', 'libInspireFace.so', 'librknnrt.so')) {
    if (!(Test-Path -LiteralPath (Join-Path $RunnerDirectory $name) -PathType Leaf)) { throw "runner deployment missing $name" }
}
$packHash = Get-Sha256 $PackPath; $faceHash = Get-Sha256 $FaceImage
$RunId = 'concurrent-e2e-' + [DateTime]::UtcNow.ToString('yyyyMMddTHHmmss') + '-' + [Guid]::NewGuid().ToString('N').Substring(0, 12)
$RemoteRunDirectory = "$RemoteBaseDirectory/$RunId"
$RunDirectory = Join-Path $ResultDirectory $RunId
New-Item -ItemType Directory -Force -Path $RunDirectory | Out-Null
try {
    Invoke-Adb @('get-state')
    Invoke-Adb @('shell', 'mkdir', '-p', $RemoteRunDirectory)
    Invoke-Adb @('push', (Join-Path $RunnerDirectory 'concurrent_e2e_runner'), "$RemoteRunDirectory/concurrent_e2e_runner")
    Invoke-Adb @('push', (Join-Path $RunnerDirectory 'libInspireFace.so'), "$RemoteRunDirectory/libInspireFace.so")
    Invoke-Adb @('push', (Join-Path $RunnerDirectory 'librknnrt.so'), "$RemoteRunDirectory/librknnrt.so")
    Invoke-Adb @('push', $PackPath, "$RemoteRunDirectory/pack")
    Invoke-Adb @('push', $FaceImage, "$RemoteRunDirectory/face-image")
    $runScript = "#!/bin/sh`nset -eu`ncd $RemoteRunDirectory`nexport LD_LIBRARY_PATH=$RemoteRunDirectory`nexec ./concurrent_e2e_runner `"`$@`"`n"
    $runScriptPath = Join-Path $RunDirectory 'run-concurrent-e2e.sh'
    [System.IO.File]::WriteAllText($runScriptPath, $runScript.Replace("`r`n", "`n"), [System.Text.UTF8Encoding]::new($false))
    Invoke-Adb @('push', $runScriptPath, "$RemoteRunDirectory/run-concurrent-e2e.sh")
    Invoke-Adb @('shell', 'chmod', '755', "$RemoteRunDirectory/concurrent_e2e_runner", "$RemoteRunDirectory/run-concurrent-e2e.sh")
    $exit = Invoke-AdbAllowFailure @('shell', 'sh', "$RemoteRunDirectory/run-concurrent-e2e.sh", '--pack', "$RemoteRunDirectory/pack", '--face-image', "$RemoteRunDirectory/face-image", '--run-id', $RunId, '--serial', $Serial, '--pack-sha256', $packHash, '--face-sha256', $faceHash, '--result-path', "$RemoteRunDirectory/result.json", '--image-backend', $ImageBackend)
    Invoke-Adb @('pull', "$RemoteRunDirectory/result.json", (Join-Path $RunDirectory 'result.json'))
    $result = Get-Content -Raw -LiteralPath (Join-Path $RunDirectory 'result.json') | ConvertFrom-Json
    if ($result.status -cne 'success') { throw "concurrent runner failed" }
    if ($result.run_id -cne $RunId -or $result.serial -cne $Serial -or $result.pack_sha256 -cne $packHash -or $result.image_backend -cne $ImageBackend) { throw 'runner evidence mismatch' }
    if ($exit -ne 0) { throw "runner returned $exit despite success evidence" }
    Write-Host "concurrent e2e passed ($($result.image_backend)): $($result.thread_count) threads x $($result.frames_per_thread) frames, wall=$([math]::Round($result.wall_ms,1))ms"
    $result.threads | ForEach-Object { Write-Host ("  thread {0}: ok={1} mean={2}ms min={3}ms max={4}ms" -f $_.index, $_.ok, [math]::Round($_.mean_ms,1), [math]::Round($_.min_ms,1), [math]::Round($_.max_ms,1)) }
} finally {
    try { Invoke-Adb @('shell', 'rm', '-rf', $RemoteRunDirectory) } catch { }
}
