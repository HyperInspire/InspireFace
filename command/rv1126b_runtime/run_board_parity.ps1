[CmdletBinding()]
param(
    [string]$Serial = 'e3d7377f6fc6d325',
    [Parameter(Mandatory = $true)][string]$PackPath,
    [string]$BuildReport = "$PackPath.report.json",
    [Parameter(Mandatory = $true)][string]$ArtifactRoot,
    [Parameter(Mandatory = $true)][string]$InputDirectory,
    [Parameter(Mandatory = $true)][ValidatePattern('^run-[A-Za-z0-9T-]+$')][string]$InputRunId,
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
    return (Join-Path $InputDirectory "$Id/$InputRunId/input_0.bin")
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
    $InputDirectory = (Resolve-Path -LiteralPath $InputDirectory).Path
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
        if ($provenance.input_sha256 -cne (Get-Sha256 $input)) { throw "prepared input evidence hash failed: $id" }
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
./rknn2_parity_runner "$@" > "$model_dir/result.partial.json"
status=$?
mv "$model_dir/result.partial.json" "$model_dir/result.json"
exit "$status"
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
                Invoke-Adb @('shell', 'sh', "$RemoteDirectory/run-one.sh", $remoteModel, '--id', $id, '--model', "$remoteModel/model.rknn", '--input', "$remoteModel/input_0.bin", '--model-sha256', $row.model_sha256, '--input-sha256', $row.input_sha256)
            } catch { $runFailure = $_ }
            Invoke-Adb @('pull', "$remoteModel/result.json", (Join-Path $local 'result.json'))
            $native = Get-Content -Raw -LiteralPath (Join-Path $local 'result.json') | ConvertFrom-Json
            foreach ($property in $native.PSObject.Properties) { $row[$property.Name] = $property.Value }
            if ($runFailure) { throw $runFailure }
            if ($native.status -ne 'success' -or $native.accepted -ne $true) { throw "runner parity failed: $($native.failure_stage) $($native.error)" }
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
