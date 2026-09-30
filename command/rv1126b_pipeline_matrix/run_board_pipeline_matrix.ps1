[CmdletBinding()]
param(
    [string]$Serial = 'e3d7377f6fc6d325',
    [Parameter(Mandatory = $true)][string]$PackPath,
    [string]$FaceImage = 'test_res/data/bulk/kun.jpg',
    [string]$RunnerDirectory = '',
    [string]$ResultDirectory = ''
)

$ErrorActionPreference = 'Stop'
$ExpectedSerial = 'e3d7377f6fc6d325'
$RemoteBaseDirectory = '/userdata/inspireface-rv1126b/pipeline-matrix'
$ExpectedScenarios = @('liveness', 'mask', 'quality', 'attribute', 'emotion')

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
    try {
        $d = [double]$Value
        return !([double]::IsNaN($d) -or [double]::IsInfinity($d))
    } catch { return $false }
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
    foreach ($field in @('run_id', 'serial', 'pack_sha256', 'face_image_sha256')) {
        if ($Result.$field -cne $Trusted[$field]) { throw "runner overwrote or mismatched trusted $field" }
    }
    $scenarios = @($Result.scenarios)
    if ($scenarios.Count -ne $ExpectedScenarios.Count -or
        @($ExpectedScenarios | Where-Object { $_ -notin @($scenarios.name) }).Count -ne 0) { throw 'runner result must contain the five pipeline scenarios' }
    foreach ($scenario in $scenarios) {
        if ($scenario.status -cne 'success' -or $scenario.failure_stage -cne '' -or $scenario.hresult -ne 0 -or
            $scenario.all_finite -ne $true -or !(Test-FiniteNumber $scenario.peak_rss_kb) -or $scenario.peak_rss_kb -lt 0 -or
            @($scenario.latency_ms).Count -ne 10 -or
            @($scenario.latency_ms | Where-Object { !(Test-FiniteNumber $_) -or $_ -lt 0 }).Count -ne 0 -or
            !(Test-Integer $scenario.detected_faces) -or $scenario.detected_faces -lt 1) { throw "runner scenario $($scenario.name) violates common schema" }
        switch ($scenario.name) {
            'liveness' { if (!(Test-FiniteNumber $scenario.value_0) -or $scenario.value_0 -lt 0.5) { throw 'liveness gate failed' } }
            'mask' { if (!(Test-FiniteNumber $scenario.value_0)) { throw 'mask gate failed' } }
            'quality' { if (!(Test-FiniteNumber $scenario.value_0) -or $scenario.value_0 -lt 0.5) { throw 'quality gate failed' } }
            'attribute' { if (!(Test-Integer $scenario.int_value_0)) { throw 'attribute gate failed' } }
            'emotion' { if (!(Test-Integer $scenario.int_value_0)) { throw 'emotion gate failed' } }
        }
    }
    return $scenarios
}

if ($Serial -cne $ExpectedSerial) { throw "pipeline matrix is pinned to serial $ExpectedSerial" }
if ($RemoteBaseDirectory -cne '/userdata/inspireface-rv1126b/pipeline-matrix') { throw 'unsafe remote base directory' }

$Repository = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
if ([string]::IsNullOrWhiteSpace($RunnerDirectory)) { $RunnerDirectory = Join-Path $Repository 'build/rv1126b-pipeline-matrix' }
if ([string]::IsNullOrWhiteSpace($ResultDirectory)) { $ResultDirectory = Join-Path $RunnerDirectory 'board' }
$PackPath = (Resolve-Path -LiteralPath $PackPath).Path
$FaceImage = Resolve-RepositoryPath $FaceImage $Repository
$RunnerDirectory = (Resolve-Path -LiteralPath $RunnerDirectory).Path
New-Item -ItemType Directory -Force -Path $ResultDirectory | Out-Null
$ResultDirectory = (Resolve-Path -LiteralPath $ResultDirectory).Path

foreach ($name in @('pipeline_matrix_runner', 'libInspireFace.so', 'librknnrt.so')) {
    if (!(Test-Path -LiteralPath (Join-Path $RunnerDirectory $name) -PathType Leaf)) { throw "runner deployment missing $name" }
}
$packHash = Get-Sha256 $PackPath
$faceHash = Get-Sha256 $FaceImage
Assert-Sha256 $packHash 'pack'; Assert-Sha256 $faceHash 'face image'
$RoundtripFiles = @(
    [ordered]@{ name = 'pipeline_matrix_runner'; local = (Join-Path $RunnerDirectory 'pipeline_matrix_runner'); sha256 = (Get-Sha256 (Join-Path $RunnerDirectory 'pipeline_matrix_runner')) },
    [ordered]@{ name = 'libInspireFace.so'; local = (Join-Path $RunnerDirectory 'libInspireFace.so'); sha256 = (Get-Sha256 (Join-Path $RunnerDirectory 'libInspireFace.so')) },
    [ordered]@{ name = 'librknnrt.so'; local = (Join-Path $RunnerDirectory 'librknnrt.so'); sha256 = (Get-Sha256 (Join-Path $RunnerDirectory 'librknnrt.so')) },
    [ordered]@{ name = 'pack'; local = $PackPath; sha256 = $packHash },
    [ordered]@{ name = 'face-image'; local = $FaceImage; sha256 = $faceHash }
)
$RunId = 'pipe-matrix-' + [DateTime]::UtcNow.ToString('yyyyMMddTHHmmss') + '-' + [Guid]::NewGuid().ToString('N').Substring(0, 12)
$RemoteRunDirectory = "$RemoteBaseDirectory/$RunId"
$RunDirectory = Join-Path $ResultDirectory $RunId
New-Item -ItemType Directory -Force -Path $RunDirectory | Out-Null
$trusted = [ordered]@{
    run_id = $RunId; serial = $Serial; pack_sha256 = $packHash; face_image_sha256 = $faceHash
}
$row = [ordered]@{ run_id = $RunId; serial = $Serial; pack_sha256 = $packHash; face_image_sha256 = $faceHash; status = 'failed'; failure_stage = 'host'; error = ''; scenarios = @() }
$aggregatePath = Join-Path $ResultDirectory 'pipeline-matrix.json'
$remoteReady = $false

try {
    Invoke-Adb @('get-state')
    Invoke-Adb @('shell', 'mkdir', '-p', '/userdata/inspireface-rv1126b')
    Invoke-Adb @('shell', 'mkdir', '-p', $RemoteBaseDirectory)
    Invoke-Adb @('shell', 'mkdir', '-p', $RemoteRunDirectory)
    $remoteReady = $true
    foreach ($file in $RoundtripFiles) {
        $remoteName = $file.name
        if ($file.name -eq 'pipeline_matrix_runner') { $remoteName = 'runner' }
        if ($file.name -eq 'pack') { $remoteName = 'pack.ispack' }
        Invoke-Adb @('push', $file.local, "$RemoteRunDirectory/$remoteName")
    }
    foreach ($file in $RoundtripFiles) {
        $remoteName = $file.name
        if ($file.name -eq 'pipeline_matrix_runner') { $remoteName = 'runner' }
        if ($file.name -eq 'pack') { $remoteName = 'pack.ispack' }
        $roundtrip = Join-Path $RunDirectory "$($file.name).roundtrip.partial"
        Invoke-Adb @('pull', "$RemoteRunDirectory/$remoteName", $roundtrip)
        if ((Get-Sha256 $roundtrip) -cne $file.sha256) { throw "remote $($file.name) roundtrip hash mismatch" }
        Remove-Item -LiteralPath $roundtrip -Force
    }
    $runScript = "#!/bin/sh`nset -eu`ncd `"$RemoteRunDirectory`"`nexport LD_LIBRARY_PATH=`"$RemoteRunDirectory`"`nexec ./runner `"`$@`"`n"
    $runScriptPath = Join-Path $RunDirectory 'run-pipe.sh'
    [System.IO.File]::WriteAllText($runScriptPath, $runScript.Replace("`r`n", "`n"), [System.Text.UTF8Encoding]::new($false))
    Invoke-Adb @('push', $runScriptPath, "$RemoteRunDirectory/run-pipe.sh")
    Invoke-Adb @('shell', 'chmod', '755', "$RemoteRunDirectory/runner", "$RemoteRunDirectory/run-pipe.sh")
    $runnerExit = Invoke-AdbAllowFailure @('shell', 'sh', "$RemoteRunDirectory/run-pipe.sh", '--pack', 'pack.ispack', '--face-image', 'face-image', '--run-id', $RunId, '--serial', $Serial, '--pack-sha256', $packHash, '--face-sha256', $faceHash, '--result-path', 'result.json')
    Invoke-Adb @('pull', "$RemoteRunDirectory/result.json", (Join-Path $RunDirectory 'result.json'))
    $native = Get-Content -Raw -LiteralPath (Join-Path $RunDirectory 'result.json') | ConvertFrom-Json
    $scenarios = Assert-RunnerResult $native $trusted
    if ($runnerExit -ne 0) { throw "runner returned $runnerExit despite success evidence" }
    $row.scenarios = @($scenarios)
    $row.status = 'success'; $row.failure_stage = ''; $row.error = ''
} catch {
    $row.status = 'failed'; $row.error = $_.Exception.Message
    if ([string]::IsNullOrWhiteSpace($row.failure_stage)) { $row.failure_stage = 'host_or_board' }
} finally {
    if ($remoteReady) { try { Invoke-Adb @('shell', 'rm', '-rf', $RemoteRunDirectory) } catch { Write-Warning "board cleanup failed: $_" } }
    $document = [ordered]@{ schema_version = 1; run_id = $RunId; serial = $Serial; pack_sha256 = $packHash; face_image_sha256 = $faceHash; status = $row.status; scenarios = $row.scenarios }
    $partial = "$aggregatePath.partial"
    $document | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $partial -Encoding utf8
    Move-Item -LiteralPath $partial -Destination $aggregatePath -Force
}

if ($row.status -ne 'success') { throw $row.error }



