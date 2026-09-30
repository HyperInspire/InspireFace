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
$RemoteBaseDirectory = '/userdata/inspireface-rv1126b/public-api-e2e'
$ExpectedCoreScenarios = @('detect_160', 'detect_320', 'detect_640', 'no_face', 'landmark', 'recognition')

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
    # Display diagnostics without adding stdout to the function's return value.
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
    foreach ($field in @('run_id', 'serial', 'pack_sha256', 'face_image_sha256', 'no_face_image_sha256')) {
        if ($Result.$field -cne $Trusted[$field]) { throw "runner overwrote or mismatched trusted $field" }
    }
    $scenarios = @($Result.scenarios)
    if ($scenarios.Count -ne $ExpectedCoreScenarios.Count -or @($scenarios.name | Select-Object -Unique).Count -ne $ExpectedCoreScenarios.Count -or
        @($ExpectedCoreScenarios | Where-Object { $_ -notin @($scenarios.name) }).Count -ne 0) { throw 'runner result must contain exactly the six core scenarios' }
    foreach ($scenario in $scenarios) {
        if ($scenario.status -cne 'success' -or $scenario.failure_stage -cne '' -or $scenario.hresult -ne 0 -or
            $scenario.all_finite -ne $true -or !(Test-FiniteNumber $scenario.peak_rss_kb) -or $scenario.peak_rss_kb -lt 0 -or @($scenario.latency_ms).Count -ne 10 -or
            @($scenario.latency_ms | Where-Object { !(Test-FiniteNumber $_) -or $_ -lt 0 }).Count -ne 0 -or !(Test-Integer $scenario.detected_faces)) { throw "runner scenario $($scenario.name) violates common schema" }
        if (($scenario.name -like 'detect_*' -and $scenario.detected_faces -lt 1) -or ($scenario.name -eq 'no_face' -and $scenario.detected_faces -ne 0) -or
            ($scenario.name -eq 'landmark' -and ($scenario.detected_faces -lt 1 -or !(Test-Integer $scenario.dense_count) -or $scenario.dense_count -ne 106 -or !(Test-Integer $scenario.five_point_count) -or $scenario.five_point_count -ne 5)) -or
            ($scenario.name -eq 'recognition' -and ($scenario.detected_faces -lt 1 -or !(Test-Integer $scenario.feature_size) -or $scenario.feature_size -ne 512 -or !(Test-FiniteNumber $scenario.similarity) -or $scenario.similarity -lt 0.9999))) { throw "runner scenario $($scenario.name) violates core gate" }
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

if ($Serial -cne $ExpectedSerial) { throw "E2E is pinned to serial $ExpectedSerial" }
if ($RemoteBaseDirectory -cne '/userdata/inspireface-rv1126b/public-api-e2e') { throw 'unsafe remote base directory' }

$Repository = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
if ([string]::IsNullOrWhiteSpace($RunnerDirectory)) { $RunnerDirectory = Join-Path $Repository 'build/rv1126b-public-api-e2e' }
if ([string]::IsNullOrWhiteSpace($ResultDirectory)) { $ResultDirectory = Join-Path $RunnerDirectory 'board' }
$PackPath = (Resolve-Path -LiteralPath $PackPath).Path
$FaceImage = Resolve-RepositoryPath $FaceImage $Repository
$NoFaceImage = Resolve-RepositoryPath $NoFaceImage $Repository
$RunnerDirectory = (Resolve-Path -LiteralPath $RunnerDirectory).Path
New-Item -ItemType Directory -Force -Path $ResultDirectory | Out-Null
$ResultDirectory = (Resolve-Path -LiteralPath $ResultDirectory).Path

foreach ($name in @('capi_e2e_runner', 'libInspireFace.so', 'librknnrt.so')) {
    if (!(Test-Path -LiteralPath (Join-Path $RunnerDirectory $name) -PathType Leaf)) { throw "runner deployment missing $name" }
}
$packHash = Get-Sha256 $PackPath
$faceHash = Get-Sha256 $FaceImage
$noFaceHash = Get-Sha256 $NoFaceImage
Assert-Sha256 $packHash 'pack'; Assert-Sha256 $faceHash 'face image'; Assert-Sha256 $noFaceHash 'no-face image'
$RoundtripFiles = @(
    [ordered]@{ name = 'capi_e2e_runner'; local = (Join-Path $RunnerDirectory 'capi_e2e_runner'); sha256 = (Get-Sha256 (Join-Path $RunnerDirectory 'capi_e2e_runner')) },
    [ordered]@{ name = 'libInspireFace.so'; local = (Join-Path $RunnerDirectory 'libInspireFace.so'); sha256 = (Get-Sha256 (Join-Path $RunnerDirectory 'libInspireFace.so')) },
    [ordered]@{ name = 'librknnrt.so'; local = (Join-Path $RunnerDirectory 'librknnrt.so'); sha256 = (Get-Sha256 (Join-Path $RunnerDirectory 'librknnrt.so')) },
    [ordered]@{ name = 'pack'; local = $PackPath; sha256 = $packHash },
    [ordered]@{ name = 'face-image'; local = $FaceImage; sha256 = $faceHash },
    [ordered]@{ name = 'no-face-image'; local = $NoFaceImage; sha256 = $noFaceHash }
)
$RunId = 'capi-e2e-' + [DateTime]::UtcNow.ToString('yyyyMMddTHHmmss') + '-' + [Guid]::NewGuid().ToString('N').Substring(0, 12)
$RemoteRunDirectory = "$RemoteBaseDirectory/$RunId"
$RunDirectory = Join-Path $ResultDirectory $RunId
New-Item -ItemType Directory -Force -Path $RunDirectory | Out-Null
$trusted = [ordered]@{
    run_id = $RunId; serial = $Serial; pack_sha256 = $packHash
    face_image_sha256 = $faceHash; no_face_image_sha256 = $noFaceHash
}
$row = [ordered]@{ run_id = $RunId; serial = $Serial; pack_sha256 = $packHash; face_image_sha256 = $faceHash; no_face_image_sha256 = $noFaceHash; status = 'failed'; failure_stage = 'host'; error = ''; scenarios = @() }
$aggregatePath = Join-Path $ResultDirectory 'capi-e2e.json'
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
    $runScript = "#!/bin/sh`nset -eu`ncd $RemoteRunDirectory`nexport LD_LIBRARY_PATH=$RemoteRunDirectory`nexec ./capi_e2e_runner `"`$@`"`n"
    $runScriptPath = Join-Path $RunDirectory 'run-e2e.sh'
    [System.IO.File]::WriteAllText($runScriptPath, $runScript.Replace("`r`n", "`n"), [System.Text.UTF8Encoding]::new($false))
    Invoke-Adb @('push', $runScriptPath, "$RemoteRunDirectory/run-e2e.sh")
    Invoke-Adb @('shell', 'chmod', '755', "$RemoteRunDirectory/capi_e2e_runner", "$RemoteRunDirectory/run-e2e.sh")
    $runnerExit = Invoke-AdbAllowFailure @('shell', 'sh', "$RemoteRunDirectory/run-e2e.sh", '--pack', "$RemoteRunDirectory/pack", '--face-image', "$RemoteRunDirectory/face-image", '--no-face-image', "$RemoteRunDirectory/no-face-image", '--run-id', $RunId, '--serial', $Serial, '--pack-sha256', $packHash, '--face-sha256', $faceHash, '--no-face-sha256', $noFaceHash, '--result-path', "$RemoteRunDirectory/result.json")
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
    $document = [ordered]@{ schema_version = 1; run_id = $RunId; serial = $Serial; pack_sha256 = $packHash; face_image_sha256 = $faceHash; no_face_image_sha256 = $noFaceHash; status = $row.status; scenario = [pscustomobject]$row }
    Write-Aggregate $aggregatePath $document
}

if ($row.status -ne 'success') { throw $row.error }
