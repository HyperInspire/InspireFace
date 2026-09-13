[CmdletBinding()]
param(
    [string]$Serial = 'e3d7377f6fc6d325',
    [Parameter(Mandatory = $true)][string]$PackPath,
    [string]$BuildReport = "$PackPath.report.json",
    [Parameter(Mandatory = $true)][string]$ArtifactRoot,
    [Parameter(Mandatory = $true)][string]$RawInputDirectory,
    [string]$RunnerDirectory = (Join-Path $PSScriptRoot '../../build/rv1126b-runtime-parity'),
    [string]$ResultDirectory = (Join-Path $PSScriptRoot '../../build/rv1126b-runtime-parity/board')
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$RemoteDirectory = '/userdata/inspireface-rv1126b/runtime-parity'
$ExpectedIds = @(
    'scrfd_2_5g_160', 'scrfd_2_5g_320', 'scrfd_2_5g_640', 'landmark', 'rnet',
    'recognition', 'liveness', 'mask', 'quality', 'emotion', 'attitude'
)
# This is the frozen command.rv1126b_pack.contracts.selected_model_ids() profile.
$selected_model_ids = $ExpectedIds

function Invoke-Adb {
    param([string[]]$Arguments)
    & adb -s $Serial @Arguments
    if ($LASTEXITCODE -ne 0) { throw "ADB failed ($LASTEXITCODE): $($Arguments -join ' ')" }
}
function Get-AdbText {
    param([string[]]$Arguments)
    $answer = & adb -s $Serial @Arguments
    if ($LASTEXITCODE -ne 0) { throw "ADB failed ($LASTEXITCODE): $($Arguments -join ' ')" }
    return ($answer | Out-String).Trim()
}
function Get-Sha256 {
    param([string]$Path)
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}
function Assert-ExactIds {
    param([object[]]$Ids, [string]$Label)
    if ($Ids.Count -ne 11 -or @(Compare-Object -CaseSensitive $ExpectedIds @($Ids)).Count) {
        throw "$Label does not contain the exact selected 11 model IDs"
    }
}
function Get-PreparedInputPath {
    param([string]$Id)
    return (Join-Path $RawInputDirectory "$Id/input_0.bin")
}
function Test-RuntimeVersion {
    param([string]$Version)
    if ($Version -notmatch '^([0-9]+)\.([0-9]+)\.([0-9]+)(?:\D|$)') { return $false }
    $major, $minor, $patch = [int]$Matches[1], [int]$Matches[2], [int]$Matches[3]
    return $major -gt 2 -or ($major -eq 2 -and ($minor -gt 3 -or ($minor -eq 3 -and $patch -ge 2)))
}
function Assert-RawProvenance {
    param([string]$Id, $Provenance, [string]$InputPath, $Evidence)
    if ($Provenance.model_id -cne $Id -or $Provenance.dtype -cne 'uint8' -or $Provenance.layout -cne 'NHWC' -or
        $Provenance.pass_through -ne 0 -or $Provenance.preprocess_stage -cne 'raw_uint8_after_resize_and_color' -or
        $Provenance.input_sha256 -cne (Get-Sha256 $InputPath)) { throw "raw input provenance is invalid: $Id" }
    if (@($Provenance.shape).Count -ne 4 -or (@($Provenance.shape) -join ',') -cne (@($Evidence.input.shape) -join ',') -or
        $Provenance.color_order -cne $Evidence.preprocess.color_order -or
        $Provenance.resize.width -ne $Evidence.preprocess.resize.width -or $Provenance.resize.height -ne $Evidence.preprocess.resize.height) {
        throw "raw input contract does not match conversion evidence: $Id"
    }
}
function Assert-RunnerResult {
    param([string]$Id, $Result, [string]$ModelHash, [string]$InputHash, $Evidence)
    if ($Result.status -cne 'success' -or $Result.model_id -cne $Id -or $Result.model_sha256 -cne $ModelHash -or $Result.input_sha256 -cne $InputHash -or
        !(Test-RuntimeVersion $Result.runtime_version) -or [string]::IsNullOrWhiteSpace($Result.driver_version) -or
        $Result.all_finite -ne $true -or @($Result.reference_latency_ms).Count -ne 10 -or @($Result.production_latency_ms).Count -ne 10 -or
        $Result.peak_rss_kb -le 0 -or @($Result.outputs).Count -ne @($Evidence.outputs).Count -or @($Result.native_outputs).Count -ne @($Evidence.outputs).Count -or
        @($Result.reference_latency_ms | Where-Object { ![double]::IsFinite([double]$_) -or $_ -le 0 }).Count -ne 0 -or
        @($Result.production_latency_ms | Where-Object { ![double]::IsFinite([double]$_) -or $_ -le 0 }).Count -ne 0) { throw "runner evidence is incomplete: $Id" }
    $integrity = $Result.diagnostics.input_integrity
    if ($null -eq $integrity -or $integrity.initial_sha256 -cne $InputHash -or
        $integrity.reference_before_sha256 -cne $InputHash -or $integrity.reference_after_sha256 -cne $InputHash -or
        $integrity.production_before_sha256 -cne $InputHash -or $integrity.production_after_sha256 -cne $InputHash -or
        $integrity.reference_unchanged -ne $true -or $integrity.production_unchanged -ne $true -or
        @($Result.diagnostics.output_diagnostics).Count -ne @($Evidence.outputs).Count) { throw "runner input-integrity diagnostics failed: $Id" }
    for ($index = 0; $index -lt @($Evidence.outputs).Count; ++$index) {
        $expected, $actual, $native = $Evidence.outputs[$index], $Result.outputs[$index], $Result.native_outputs[$index]
        $diagnostic = $Result.diagnostics.output_diagnostics[$index]
        $max_envelope = [Math]::Max([double]$diagnostic.reference_self_repeat.max_abs, [double]$diagnostic.production_self_repeat.max_abs) + 1e-5
        $cosine_envelope = [Math]::Min([double]$diagnostic.reference_self_repeat.cosine, [double]$diagnostic.production_self_repeat.cosine) - 1e-6
        if ($actual.name -cne $expected.name -or (@($actual.logical_dims) -join ',') -cne (@($expected.shape) -join ',') -or $actual.type -cne 'FP32' -or
            $actual.logical_type -cne 'FP32' -or $actual.native_type -cne $native.type -or $actual.native_qnt_type -cne $native.qnt_type -or
            [double]$actual.native_scale -ne [double]$native.scale -or [int]$actual.native_zp -ne [int]$native.zp -or
            $actual.finite -ne $true -or $diagnostic.index -ne $index -or
            $diagnostic.reference_self_repeat.finite -ne $true -or $diagnostic.production_self_repeat.finite -ne $true -or
            $diagnostic.reference_raw_vs_float.finite -ne $true -or $diagnostic.production_native_vs_logical.finite -ne $true -or
            ![double]::IsFinite([double]$diagnostic.reference_self_repeat.max_abs) -or ![double]::IsFinite([double]$diagnostic.reference_self_repeat.cosine) -or
            ![double]::IsFinite([double]$diagnostic.production_self_repeat.max_abs) -or ![double]::IsFinite([double]$diagnostic.production_self_repeat.cosine) -or
            ![double]::IsFinite([double]$diagnostic.reference_raw_vs_float.max_abs) -or ![double]::IsFinite([double]$diagnostic.reference_raw_vs_float.cosine) -or
            ![double]::IsFinite([double]$diagnostic.production_native_vs_logical.max_abs) -or ![double]::IsFinite([double]$diagnostic.production_native_vs_logical.cosine) -or
            ![double]::IsFinite([double]$actual.max_abs) -or ![double]::IsFinite([double]$actual.cosine) -or
            $diagnostic.reference_raw_vs_float.max_abs -gt 1e-5 -or $diagnostic.reference_raw_vs_float.cosine -lt 0.999999 -or
            $diagnostic.production_native_vs_logical.max_abs -gt 1e-5 -or $diagnostic.production_native_vs_logical.cosine -lt 0.999999 -or
            $actual.max_abs -gt $max_envelope -or $actual.cosine -lt $cosine_envelope) { throw "runner output contract/parity failed: $Id/$index" }
    }
}

$RunId = 'runtime-' + [DateTime]::UtcNow.ToString('yyyyMMddTHHmmss') + '-' + [Guid]::NewGuid().ToString('N').Substring(0, 12)
$remoteReady = $false
$rows = [System.Collections.Generic.List[object]]::new()
$packHash = ''

New-Item -ItemType Directory -Force -Path $ResultDirectory | Out-Null
$ResultDirectory = (Resolve-Path -LiteralPath $ResultDirectory).Path
$RunDirectory = Join-Path $ResultDirectory $RunId
New-Item -ItemType Directory -Path $RunDirectory | Out-Null
$ResultPath = Join-Path $ResultDirectory 'runtime-parity.json'
$PartialPath = Join-Path $ResultDirectory "$RunId.partial"
function Write-Aggregate {
    param([string]$Status, [string]$Error = '')
    $document = [ordered]@{
        schema_version = 1; status = $Status; error = $Error; run_id = $RunId; serial = $Serial
        pack_sha256 = $packHash; selected_model_ids = $ExpectedIds; selected_model_count = $ExpectedIds.Count
        models = $rows.ToArray()
    }
    $document | ConvertTo-Json -Depth 32 | Set-Content -LiteralPath (Join-Path $RunDirectory 'runtime-parity.json') -Encoding utf8
    $document | ConvertTo-Json -Depth 32 | Set-Content -LiteralPath $PartialPath -Encoding utf8
    Move-Item -LiteralPath $PartialPath -Destination $ResultPath -Force
}

Write-Aggregate 'running'
try {
    if ($Serial -cne 'e3d7377f6fc6d325') { throw 'Parity is pinned to serial e3d7377f6fc6d325' }
    if ($RemoteDirectory -cne '/userdata/inspireface-rv1126b/runtime-parity') { throw 'Unsafe fixed remote directory' }
    $Repository = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
    $PackPath = (Resolve-Path -LiteralPath $PackPath).Path
    $BuildReport = (Resolve-Path -LiteralPath $BuildReport).Path
    $ArtifactRoot = (Resolve-Path -LiteralPath $ArtifactRoot).Path
    $RawInputDirectory = (Resolve-Path -LiteralPath $RawInputDirectory).Path
    $RunnerDirectory = (Resolve-Path -LiteralPath $RunnerDirectory).Path
    foreach ($name in @('rknn2_parity_runner', 'libInspireFace.so', 'librknnrt.so')) {
        if (!(Test-Path -LiteralPath (Join-Path $RunnerDirectory $name) -PathType Leaf)) { throw "Runner deployment is missing $name" }
    }
    Push-Location $Repository
    try {
        & python -B -m command.rv1126b_pack.validate_pack $PackPath $BuildReport
        if ($LASTEXITCODE -ne 0) { throw 'Host resource-pack gate failed; refusing board deployment' }
    } finally { Pop-Location }
    $packHash = Get-Sha256 $PackPath
    $report = Get-Content -Raw -LiteralPath $BuildReport | ConvertFrom-Json
    if ($report.pack_sha256 -cne $packHash -or $report.conversion_engineering_gate -ne $true) { throw 'Pack/report hash or engineering gate failed' }
    Assert-ExactIds @($report.selected_model_ids) 'trusted pack report selected_model_ids'
    if ($report.selected_model_count -ne 11 -or @($report.members).Count -ne 11) { throw 'trusted pack report count is invalid' }
    # The report is the deployment authority only after its exact frozen set has passed the host gate.
    $SelectedIds = @($report.selected_model_ids)
    $members = @{}
    $evidences = @{}
    foreach ($member in $report.members) {
        if ($members.ContainsKey($member.id)) { throw "duplicate pack report member: $($member.id)" }
        $members[$member.id] = $member
    }
    Assert-ExactIds @($members.Keys) 'trusted pack report members'
    foreach ($id in $SelectedIds) {
        $model = Join-Path $ArtifactRoot "models/${id}_rv1126b.rknn"
        $sidecar = [System.IO.Path]::ChangeExtension($model, '.json')
        if (!(Test-Path -LiteralPath $model -PathType Leaf) -or !(Test-Path -LiteralPath $sidecar -PathType Leaf)) { throw "missing model or conversion evidence: $id" }
        $evidence = Get-Content -Raw -LiteralPath $sidecar | ConvertFrom-Json
        $modelHash = Get-Sha256 $model
        if ($evidence.model_id -cne $id -or $evidence.conversion_status -cne 'success' -or $evidence.output_sha256 -cne $modelHash -or $members[$id].sha256 -cne $modelHash) {
            throw "model/evidence hash gate failed: $id"
        }
        $input = Get-PreparedInputPath $id
        if (!(Test-Path -LiteralPath $input -PathType Leaf) -or (Get-Item -LiteralPath $input).Length -le 0) { throw "missing prepared uint8 input: $id" }
        $provenancePath = Join-Path (Split-Path -Parent $input) 'input_provenance.json'
        if (!(Test-Path -LiteralPath $provenancePath -PathType Leaf)) { throw "missing prepared input evidence: $id" }
        $provenance = Get-Content -Raw -LiteralPath $provenancePath | ConvertFrom-Json
        Assert-RawProvenance $id $provenance $input $evidence
        $evidences[$id] = $evidence
    }

    Invoke-Adb @('get-state')
    Invoke-Adb @('shell', 'mkdir', '-p', '/userdata/inspireface-rv1126b')
    if ((Get-AdbText @('shell', 'readlink', '-f', '/userdata/inspireface-rv1126b')) -cne '/userdata/inspireface-rv1126b') { throw 'Remote parent resolved outside /userdata' }
    Invoke-Adb @('shell', 'mkdir', '-p', $RemoteDirectory)
    if ((Get-AdbText @('shell', 'readlink', '-f', $RemoteDirectory)) -cne $RemoteDirectory) { throw 'Remote parity directory did not resolve to its fixed path' }
    $remoteReady = $true
    Invoke-Adb @('shell', 'rm', '-rf', $RemoteDirectory)
    Invoke-Adb @('shell', 'mkdir', '-p', $RemoteDirectory)
    foreach ($name in @('rknn2_parity_runner', 'libInspireFace.so', 'librknnrt.so')) { Invoke-Adb @('push', (Join-Path $RunnerDirectory $name), "$RemoteDirectory/$name") }
    Invoke-Adb @('shell', 'chmod', '755', "$RemoteDirectory/rknn2_parity_runner")
    $runScript = @'
#!/bin/sh
set -u
cd /userdata/inspireface-rv1126b/runtime-parity || exit 125
export LD_LIBRARY_PATH=/userdata/inspireface-rv1126b/runtime-parity
model_dir=$1
shift
exec ./rknn2_parity_runner "$@"
'@
    $runScriptPath = Join-Path $RunDirectory 'run-one.sh'
    [System.IO.File]::WriteAllText($runScriptPath, $runScript.Replace("`r`n", "`n") + "`n", [System.Text.UTF8Encoding]::new($false))
    Invoke-Adb @('push', $runScriptPath, "$RemoteDirectory/run-one.sh")
    Invoke-Adb @('shell', 'chmod', '755', "$RemoteDirectory/run-one.sh")

    foreach ($id in $SelectedIds) {
        $local = Join-Path $RunDirectory $id
        New-Item -ItemType Directory -Path $local | Out-Null
        $remoteModel = "$RemoteDirectory/$id"
        $model = Join-Path $ArtifactRoot "models/${id}_rv1126b.rknn"
        $input = Get-PreparedInputPath $id
        $evidence = $evidences[$id]
        $row = [ordered]@{ model_id = $id; status = 'failed'; failure_stage = 'deploy'; error = ''; run_id = $RunId; serial = $Serial; pack_sha256 = $packHash; model_sha256 = ''; input_sha256 = '' }
        try {
            $row.model_sha256 = Get-Sha256 $model; $row.input_sha256 = Get-Sha256 $input
            Invoke-Adb @('shell', 'mkdir', '-p', $remoteModel)
            Invoke-Adb @('push', $model, "$remoteModel/model.rknn")
            Invoke-Adb @('push', $input, "$remoteModel/input_0.bin")
            foreach ($pair in @(@('model.rknn', $row.model_sha256), @('input_0.bin', $row.input_sha256))) {
                $roundtrip = Join-Path $local "$($pair[0]).roundtrip.partial"
                Invoke-Adb @('pull', "$remoteModel/$($pair[0])", $roundtrip)
                if ((Get-Sha256 $roundtrip) -cne $pair[1]) { throw "remote $($pair[0]) hash mismatch" }
                Remove-Item -LiteralPath $roundtrip -Force
            }
            $runFailure = $null
            try {
                # The LF script is invoked by sh as an argument vector, never as a command string.
                Invoke-Adb @('shell', 'sh', "$RemoteDirectory/run-one.sh", $remoteModel, '--id', $id, '--model', "$remoteModel/model.rknn", '--input', "$remoteModel/input_0.bin", '--result', "$remoteModel/result.json", '--model-sha256', $row.model_sha256, '--input-sha256', $row.input_sha256)
            } catch { $runFailure = $_ }
            Invoke-Adb @('pull', "$remoteModel/result.json", (Join-Path $local 'result.json'))
            $native = Get-Content -Raw -LiteralPath (Join-Path $local 'result.json') | ConvertFrom-Json
            foreach ($property in $native.PSObject.Properties) { $row[$property.Name] = $property.Value }
            if ($runFailure) { throw $runFailure }
            Assert-RunnerResult $id $native $row.model_sha256 $row.input_sha256 $evidence
            $row.accepted = $true
            $row.status = 'success'
        } catch {
            $row.status = 'failed'
            if (!$row.failure_stage) { $row.failure_stage = 'host_or_board' }
            $row.error = $_.Exception.Message
        }
        $rows.Add([pscustomobject]$row)
        Write-Aggregate ($(if ($rows.Count -eq 11 -and !($rows | Where-Object { $_.status -ne 'success' })) { 'success' } else { 'running' }))
        Write-Output "$id : $($row.status) $($row.failure_stage)"
        continue
    }
    Invoke-Adb @('shell', 'rm', '-rf', $RemoteDirectory)
    $remoteReady = $false
    if ($rows.Count -ne 11 -or ($rows | Where-Object { $_.status -ne 'success' })) { throw 'one or more runtime parity rows failed' }
    Write-Aggregate 'success'
} catch {
    Write-Aggregate 'failed' $_.Exception.Message
    throw
} finally {
    if ($remoteReady) { try { Invoke-Adb @('shell', 'rm', '-rf', $RemoteDirectory) } catch { Write-Warning "Board cleanup failed: $_" } }
}
