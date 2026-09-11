[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][ValidatePattern('^[A-Za-z0-9_.:-]+$')][string]$Serial,
    [string]$RemoteDirectory = '/userdata/inspireface-rv1126b-matrix',
    [string]$Artifacts = (Join-Path $PSScriptRoot '../../build/rv1126b-models'),
    [string]$DockerImage = 'inspireface-rv1126b'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ($RemoteDirectory -notmatch '^/userdata/[A-Za-z0-9_-]+(?:/[A-Za-z0-9_-]+)*$') {
    throw 'RemoteDirectory must be an absolute named subdirectory of /userdata with safe path components'
}
$Artifacts = (Resolve-Path -LiteralPath $Artifacts).Path
$repository = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
$binary = Join-Path $Artifacts 'rknn_contract_runner'
if (!(Test-Path -LiteralPath $binary -PathType Leaf)) { throw 'Build rknn_contract_runner first' }
$inventory = Get-Content -Raw -LiteralPath (Join-Path $PSScriptRoot 'model_inventory.json') | ConvertFrom-Json
$ids = @($inventory | ForEach-Object { $_.id } | Sort-Object)
$sidecars = @(Get-ChildItem -LiteralPath (Join-Path $Artifacts 'models') -Filter '*.json')
$records = @($sidecars | ForEach-Object { Get-Content -Raw -LiteralPath $_.FullName | ConvertFrom-Json })
$recordIds = @($records | ForEach-Object { $_.model_id } | Sort-Object)
if ($ids.Count -ne 18 -or $recordIds.Count -ne 18 -or @(Compare-Object $ids $recordIds).Count) { throw 'Exact 18-model inventory/record match required' }
$modelNames = @(Get-ChildItem -LiteralPath (Join-Path $Artifacts 'models') -Filter '*.rknn' | ForEach-Object { $_.BaseName } | Sort-Object)
if (@(Compare-Object @($ids | ForEach-Object { "${_}_rv1126b" }) $modelNames).Count) { throw 'Unexpected/missing RKNN artifacts' }
foreach ($record in $records) {
    if ($record.model_id -notmatch '^[a-z0-9_]+$' -or $record.conversion_status -ne 'success') { throw 'Invalid conversion record' }
    $model = Join-Path $Artifacts "models/$($record.model_id)_rv1126b.rknn"
    if ((Get-Item -LiteralPath $model).Length -le 0 -or (Get-FileHash -LiteralPath $model -Algorithm SHA256).Hash.ToLowerInvariant() -ne $record.output_sha256) { throw "Model hash mismatch: $($record.model_id)" }
}
function Invoke-Adb {
    param([string[]]$Arguments, [string]$Log)
    & adb -s $Serial @Arguments > $Log 2>&1
    if ($LASTEXITCODE -ne 0) { throw "ADB failed ($LASTEXITCODE): $($Arguments[0]); see $Log" }
}
$board = Join-Path $Artifacts 'board'
New-Item -ItemType Directory -Force -Path $board | Out-Null
$runId = 'run-' + [DateTime]::UtcNow.ToString('yyyyMMddTHHmmss') + '-' + [Guid]::NewGuid().ToString('N').Substring(0, 8)
$remote = "$RemoteDirectory/$runId"
Invoke-Adb -Arguments @('get-state') -Log (Join-Path $board "$runId-device.log")
Invoke-Adb -Arguments @('shell', 'mkdir', '-p', $remote) -Log (Join-Path $board "$runId-mkdir.log")
Invoke-Adb -Arguments @('push', $binary, "$remote/runner") -Log (Join-Path $board "$runId-push.log")
Invoke-Adb -Arguments @('shell', 'chmod', '755', "$remote/runner") -Log (Join-Path $board "$runId-chmod.log")
$results = [System.Collections.Generic.List[object]]::new()
foreach ($id in $ids) {
    $local = Join-Path $board "$id/$runId"
    New-Item -ItemType Directory -Force -Path $local | Out-Null
    $modelRemote = "$remote/$id"
    $record = $records | Where-Object { $_.model_id -eq $id }
    $result = [ordered]@{ model_id = $id; status = 'failed'; error = ''; serial = $Serial; run_id = $runId; model_sha256 = $record.output_sha256 }
    try {
        Invoke-Adb -Arguments @('shell', 'mkdir', '-p', $modelRemote) -Log (Join-Path $local 'mkdir.log')
        Invoke-Adb -Arguments @('push', (Join-Path $Artifacts "models/${id}_rv1126b.rknn"), "$modelRemote/model.rknn") -Log (Join-Path $local 'push-model.log')
        $queryError = $null
        try { Invoke-Adb -Arguments @('shell', "$remote/runner", "$modelRemote/model.rknn", $modelRemote, '--query') -Log (Join-Path $local 'query.log') } catch { $queryError = $_ }
        Invoke-Adb -Arguments @('pull', "$modelRemote/contract.json", (Join-Path $local 'contract.json')) -Log (Join-Path $local 'pull-contract.log')
        $contract = Get-Content -Raw -LiteralPath (Join-Path $local 'contract.json') | ConvertFrom-Json
        foreach ($property in $contract.PSObject.Properties) { $result[$property.Name] = $property.Value }
        if ($queryError) { throw $queryError }
        # Some older adbd builds return zero even when the remote process fails.
        if ($contract.status -ne 'queried') { throw "Runner query failed: $($contract.error)" }
        $containerLocal = "/artifacts/board/$id/$runId"
        & docker run --rm --mount "type=bind,source=$repository,target=/work,readonly" --mount "type=bind,source=$Artifacts,target=/artifacts" --mount "type=bind,source=$Artifacts/source,target=/source,readonly" $DockerImage python /work/command/rv1126b_models/prepare_board_inputs.py "/artifacts/models/${id}_rv1126b.json" "$containerLocal/contract.json" $containerLocal > (Join-Path $local 'prepare-input.log') 2>&1
        if ($LASTEXITCODE -ne 0) { throw "Input generation failed; see $local/prepare-input.log" }
        $inputArguments = @()
        foreach ($inputTensor in $contract.inputs) {
            $name = "input_$($inputTensor.index).bin"
            Invoke-Adb -Arguments @('push', (Join-Path $local $name), "$modelRemote/$name") -Log (Join-Path $local "push-$name.log")
            $inputArguments += "$modelRemote/$name"
        }
        $runError = $null
        try { Invoke-Adb -Arguments (@('shell', "$remote/runner", "$modelRemote/model.rknn", $modelRemote) + $inputArguments) -Log (Join-Path $local 'run.log') } catch { $runError = $_ }
        Invoke-Adb -Arguments @('pull', "$modelRemote/result.json", (Join-Path $local 'result.json')) -Log (Join-Path $local 'pull-result.log')
        $native = Get-Content -Raw -LiteralPath (Join-Path $local 'result.json') | ConvertFrom-Json
        foreach ($property in $native.PSObject.Properties) { $result[$property.Name] = $property.Value }
        if ($runError) { throw $runError }
        if ($native.status -ne 'success') { throw "Runner failed: $($native.error)" }
        foreach ($output in $native.outputs) {
            $name = "output_$($output.index).f32"
            Invoke-Adb -Arguments @('pull', "$modelRemote/$name", (Join-Path $local $name)) -Log (Join-Path $local "pull-$name.log")
            if ((Get-Item -LiteralPath (Join-Path $local $name)).Length -ne [long]$output.n_elems * 4) { throw 'Output size mismatch' }
            $output.file = "$runId/$name"
            $output | Add-Member -NotePropertyName sha256 -NotePropertyValue (Get-FileHash -LiteralPath (Join-Path $local $name) -Algorithm SHA256).Hash.ToLowerInvariant()
        }
        $result['input_provenance'] = Get-Content -Raw -LiteralPath (Join-Path $local 'input_provenance.json') | ConvertFrom-Json
    } catch {
        $priorError = $result['error']
        $result['status'] = 'failed'
        $result['error'] = "$priorError $($_.Exception.Message)".Trim()
    }
    $results.Add($result)
    $results.ToArray() | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath (Join-Path $board 'results.json') -Encoding utf8
    Write-Output "$id : $($result['status']) $($result['error'])"
}
if (@($results | Where-Object { $_['status'] -ne 'success' }).Count) { exit 1 }
exit 0
