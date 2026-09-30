[CmdletBinding()]
param(
    [string]$Serial = 'e3d7377f6fc6d325',
    [Parameter(Mandatory = $true)][string]$PackPath,
    [string]$SingleFaceImage = 'test_res/data/bulk/kun.jpg',
    [string]$MultiFaceImage = 'test_res/data/bulk/pedestrian.png',
    [string]$NoFaceImage = 'test_res/data/bulk/view.jpg',
    [string]$RunnerDirectory = '',
    [string]$ResultDirectory = ''
)

$ErrorActionPreference = 'Stop'
$ExpectedSerial = 'e3d7377f6fc6d325'
$RemoteBaseDirectory = '/userdata/inspireface-rv1126b/tracking-e2e'
$ExpectedScenarios = @('light_track_single', 'track_by_detect_single', 'light_track_multi', 'always_detect_multi', 'no_face', 'light_track_landmark')

function Get-Sha256 {
    param([string]$Path)
    return (Get-FileHash -Algorithm SHA256 -LiteralPath $Path).Hash.ToLowerInvariant()
}

function Invoke-Adb {
    param([string[]]$Arguments)
    & adb -s $Serial @Arguments
    if ($LASTEXITCODE -ne 0) { throw "adb failed ($LASTEXITCODE): $($Arguments -join ' ')" }
}

function Invoke-AdbAllowFailure {
    param([string[]]$Arguments)
    & adb -s $Serial @Arguments | Out-Host
    return [int]$LASTEXITCODE
}

function Get-AdbText {
    param([string[]]$Arguments)
    $text = & adb -s $Serial @Arguments
    if ($LASTEXITCODE -ne 0) { throw "adb failed ($LASTEXITCODE): $($Arguments -join ' ')" }
    return ($text | Out-String).Trim()
}

function Assert-Sha256 {
    param([string]$Value, [string]$Label)
    if ($Value -notmatch '^[0-9a-f]{64}$') { throw "invalid SHA-256 for $Label" }
}

function Test-FiniteNumber {
    param($Value)
    if ($null -eq $Value -or $Value -isnot [System.ValueType] -or $Value -is [bool]) { return $false }
    try { return [double]::IsFinite([double]$Value) } catch { return $false }
}

function Test-Integer {
    param($Value)
    if (!(Test-FiniteNumber $Value)) { return $false }
    return [Math]::Truncate([double]$Value) -eq [double]$Value
}

function Resolve-RepositoryPath {
    param([string]$Path, [string]$RepositoryPath)
    if ([System.IO.Path]::IsPathRooted($Path)) { return (Resolve-Path -LiteralPath $Path).Path }
    return (Resolve-Path -LiteralPath (Join-Path $RepositoryPath $Path)).Path
}

function Assert-RunnerResult {
    param($Result, $Trusted)
    foreach ($field in @('run_id', 'serial', 'pack_sha256', 'single_face_image_sha256', 'multi_face_image_sha256', 'no_face_image_sha256')) {
        if ($Result.$field -cne $Trusted[$field]) { throw "runner overwrote or mismatched trusted $field" }
    }
    $scenarios = @($Result.scenarios)
    if ($scenarios.Count -ne $ExpectedScenarios.Count -or @($scenarios.name | Select-Object -Unique).Count -ne $ExpectedScenarios.Count -or
        @($ExpectedScenarios | Where-Object { $_ -notin @($scenarios.name) }).Count -ne 0) { throw 'runner result must contain exactly the six tracking scenarios' }
    foreach ($scenario in $scenarios) {
        if ($scenario.status -cne 'success' -or $scenario.failure_stage -cne '' -or $scenario.hresult -ne 0 -or
            $scenario.all_finite -ne $true -or !(Test-FiniteNumber $scenario.peak_rss_kb) -or $scenario.peak_rss_kb -lt 0 -or
            @($scenario.latency_ms).Count -lt 1 -or
            @($scenario.latency_ms | Where-Object { !(Test-FiniteNumber $_) -or $_ -lt 0 }).Count -ne 0 -or
            !(Test-Integer $scenario.detected_faces) -or !(Test-Integer $scenario.frames) -or $scenario.frames -lt 1) {
            throw "runner scenario $($scenario.name) violates common schema"
        }
        if (($scenario.name -eq 'light_track_single' -or $scenario.name -eq 'track_by_detect_single') -and
            ($scenario.detected_faces -ne 1 -or !(Test-Integer $scenario.track_id) -or $scenario.track_id -lt 0 -or
             !(Test-Integer $scenario.track_count) -or $scenario.track_count -lt 1)) {
            throw "runner scenario $($scenario.name) violates tracking continuity gate"
        }
        if ($scenario.name -eq 'light_track_multi' -and
            ($scenario.detected_faces -lt 2 -or $scenario.unique_ids -ne $scenario.detected_faces)) {
            throw "runner scenario $($scenario.name) violates multi-face gate"
        }
        if ($scenario.name -eq 'always_detect_multi' -and $scenario.detected_faces -lt 2) {
            throw "runner scenario $($scenario.name) violates multi-face gate"
        }
        if ($scenario.name -eq 'no_face' -and $scenario.detected_faces -ne 0) { throw 'runner scenario no_face violates gate' }
        if ($scenario.name -eq 'light_track_landmark' -and
            ($scenario.detected_faces -lt 1 -or !(Test-Integer $scenario.dense_count) -or $scenario.dense_count -ne 106 -or
             !(Test-Integer $scenario.five_point_count) -or $scenario.five_point_count -ne 5)) {
            throw 'runner scenario light_track_landmark violates gate'
        }
    }
    return $scenarios
}

function Merge-RunnerEvidence {
    param([System.Collections.IDictionary]$Row, $Scenarios)
    $Row['scenarios'] = @($Scenarios)
}

function Write-Aggregate {
    param([string]$Path, $Document)
    $partial = "$Path.partial"
    $Document | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $partial -Encoding utf8
    Move-Item -LiteralPath $partial -Destination $Path -Force
}

if ($Serial -cne $ExpectedSerial) { throw "tracking E2E is pinned to serial $ExpectedSerial" }
if ($RemoteBaseDirectory -cne '/userdata/inspireface-rv1126b/tracking-e2e') { throw 'unsafe remote base directory' }

$Repository = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
if ([string]::IsNullOrWhiteSpace($RunnerDirectory)) { $RunnerDirectory = Join-Path $Repository 'build/rv1126b-tracking-e2e' }
if ([string]::IsNullOrWhiteSpace($ResultDirectory)) { $ResultDirectory = Join-Path $RunnerDirectory 'board' }
$PackPath = (Resolve-Path -LiteralPath $PackPath).Path
$SingleFaceImage = Resolve-RepositoryPath $SingleFaceImage $Repository
$MultiFaceImage = Resolve-RepositoryPath $MultiFaceImage $Repository
$NoFaceImage = Resolve-RepositoryPath $NoFaceImage $Repository
$RunnerDirectory = (Resolve-Path -LiteralPath $RunnerDirectory).Path
New-Item -ItemType Directory -Force -Path $ResultDirectory | Out-Null
$ResultDirectory = (Resolve-Path -LiteralPath $ResultDirectory).Path

foreach ($name in @('tracking_e2e_runner', 'libInspireFace.so', 'librknnrt.so')) {
    if (!(Test-Path -LiteralPath (Join-Path $RunnerDirectory $name) -PathType Leaf)) { throw "runner deployment missing $name" }
}
$packHash = Get-Sha256 $PackPath
$singleHash = Get-Sha256 $SingleFaceImage
$multiHash = Get-Sha256 $MultiFaceImage
$noFaceHash = Get-Sha256 $NoFaceImage
Assert-Sha256 $packHash 'pack'; Assert-Sha256 $singleHash 'single-face image'; Assert-Sha256 $multiHash 'multi-face image'; Assert-Sha256 $noFaceHash 'no-face image'
$RoundtripFiles = @(
    [ordered]@{ name = 'tracking_e2e_runner'; local = (Join-Path $RunnerDirectory 'tracking_e2e_runner'); sha256 = (Get-Sha256 (Join-Path $RunnerDirectory 'tracking_e2e_runner')) },
    [ordered]@{ name = 'libInspireFace.so'; local = (Join-Path $RunnerDirectory 'libInspireFace.so'); sha256 = (Get-Sha256 (Join-Path $RunnerDirectory 'libInspireFace.so')) },
    [ordered]@{ name = 'librknnrt.so'; local = (Join-Path $RunnerDirectory 'librknnrt.so'); sha256 = (Get-Sha256 (Join-Path $RunnerDirectory 'librknnrt.so')) },
    [ordered]@{ name = 'pack'; local = $PackPath; sha256 = $packHash },
    [ordered]@{ name = 'single-face'; local = $SingleFaceImage; sha256 = $singleHash },
    [ordered]@{ name = 'multi-face'; local = $MultiFaceImage; sha256 = $multiHash },
    [ordered]@{ name = 'no-face'; local = $NoFaceImage; sha256 = $noFaceHash }
)
$RunId = 'tracking-e2e-' + [DateTime]::UtcNow.ToString('yyyyMMddTHHmmss') + '-' + [Guid]::NewGuid().ToString('N').Substring(0, 12)
$RemoteRunDirectory = "$RemoteBaseDirectory/$RunId"
$RunDirectory = Join-Path $ResultDirectory $RunId
New-Item -ItemType Directory -Force -Path $RunDirectory | Out-Null
$trusted = [ordered]@{
    run_id = $RunId; serial = $Serial; pack_sha256 = $packHash
    single_face_image_sha256 = $singleHash; multi_face_image_sha256 = $multiHash; no_face_image_sha256 = $noFaceHash
}
$row = [ordered]@{ run_id = $RunId; serial = $Serial; pack_sha256 = $packHash; single_face_image_sha256 = $singleHash; multi_face_image_sha256 = $multiHash; no_face_image_sha256 = $noFaceHash; status = 'failed'; failure_stage = 'host'; error = ''; scenarios = @() }
$aggregatePath = Join-Path $ResultDirectory 'tracking-e2e.json'
$remoteReady = $false

try {
    Invoke-Adb @('get-state')
    Invoke-Adb @('shell', 'mkdir', '-p', '/userdata/inspireface-rv1126b')
    if ((Get-AdbText @('shell', 'readlink', '-f', '/userdata/inspireface-rv1126b')) -cne '/userdata/inspireface-rv1126b') { throw 'remote parent resolved outside /userdata' }
    Invoke-Adb @('shell', 'mkdir', '-p', $RemoteBaseDirectory)
    if ((Get-AdbText @('shell', 'readlink', '-f', $RemoteBaseDirectory)) -cne $RemoteBaseDirectory) { throw 'remote base did not resolve safely' }
    Invoke-Adb @('shell', 'mkdir', '-p', $RemoteRunDirectory)
    $resolvedRun = Get-AdbText @('shell', 'readlink', '-f', $RemoteRunDirectory)
    if ($resolvedRun -cne $RemoteRunDirectory) { throw 'remote run directory did not resolve safely' }
    $remoteReady = $true
    foreach ($file in $RoundtripFiles) { Invoke-Adb @('push', $file.local, "$RemoteRunDirectory/$($file.name)") }
    foreach ($file in $RoundtripFiles) {
        $roundtrip = Join-Path $RunDirectory "$($file.name).roundtrip.partial"
        Invoke-Adb @('pull', "$RemoteRunDirectory/$($file.name)", $roundtrip)
        if ((Get-Sha256 $roundtrip) -cne $file.sha256) { throw "remote $($file.name) roundtrip hash mismatch" }
        Remove-Item -LiteralPath $roundtrip -Force
    }
    $runScript = "#!/bin/sh`nset -eu`ncd $RemoteRunDirectory`nexport LD_LIBRARY_PATH=$RemoteRunDirectory`nexec ./tracking_e2e_runner `"`$@`"`n"
    $runScriptPath = Join-Path $RunDirectory 'run-tracking-e2e.sh'
    [System.IO.File]::WriteAllText($runScriptPath, $runScript.Replace("`r`n", "`n"), [System.Text.UTF8Encoding]::new($false))
    Invoke-Adb @('push', $runScriptPath, "$RemoteRunDirectory/run-tracking-e2e.sh")
    Invoke-Adb @('shell', 'chmod', '755', "$RemoteRunDirectory/tracking_e2e_runner", "$RemoteRunDirectory/run-tracking-e2e.sh")
    $runnerExit = Invoke-AdbAllowFailure @('shell', 'sh', "$RemoteRunDirectory/run-tracking-e2e.sh", '--pack', "$RemoteRunDirectory/pack", '--single-face-image', "$RemoteRunDirectory/single-face", '--multi-face-image', "$RemoteRunDirectory/multi-face", '--no-face-image', "$RemoteRunDirectory/no-face", '--run-id', $RunId, '--serial', $Serial, '--pack-sha256', $packHash, '--single-sha256', $singleHash, '--multi-sha256', $multiHash, '--no-face-sha256', $noFaceHash, '--result-path', "$RemoteRunDirectory/result.json")
    Invoke-Adb @('pull', "$RemoteRunDirectory/result.json", (Join-Path $RunDirectory 'result.json'))
    $native = Get-Content -Raw -LiteralPath (Join-Path $RunDirectory 'result.json') | ConvertFrom-Json
    $scenarios = Assert-RunnerResult $native $trusted
    if ($runnerExit -ne 0) { throw "runner returned $runnerExit despite success evidence" }
    Merge-RunnerEvidence $row $scenarios
    $row.status = 'success'; $row.failure_stage = ''; $row.error = ''
} catch {
    $row.status = 'failed'; $row.error = $_.Exception.Message
    if ([string]::IsNullOrWhiteSpace($row.failure_stage)) { $row.failure_stage = 'host_or_board' }
} finally {
    if ($remoteReady) { try { Invoke-Adb @('shell', 'rm', '-rf', $RemoteRunDirectory) } catch { Write-Warning "board cleanup failed: $_" } }
    $document = [ordered]@{ schema_version = 1; run_id = $RunId; serial = $Serial; pack_sha256 = $packHash; single_face_image_sha256 = $singleHash; multi_face_image_sha256 = $multiHash; no_face_image_sha256 = $noFaceHash; status = $row.status; scenario = [pscustomobject]$row }
    Write-Aggregate $aggregatePath $document
}

if ($row.status -ne 'success') { throw $row.error }
