[CmdletBinding()]
param(
    [string]$Serial = 'e3d7377f6fc6d325',
    [Parameter(Mandatory = $true)][string]$PackPath,
    [string]$BuildReport = "$PackPath.report.json",
    [string]$ValidatorDirectory = (Join-Path $PSScriptRoot '../../build/rv1126b-pack-validator'),
    [string]$Inventory = (Join-Path $PSScriptRoot '../rv1126b_models/model_inventory.json'),
    [string]$ResultDirectory = (Join-Path $PSScriptRoot '../../build/rv1126b-pack/board-validation')
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$RemoteDirectory = '/userdata/inspireface-rv1126b/pack-validation'
$RunId = [Guid]::NewGuid().ToString('N')
New-Item -ItemType Directory -Force -Path $ResultDirectory | Out-Null
$ResultDirectory = (Resolve-Path -LiteralPath $ResultDirectory).Path
$RunDirectory = Join-Path $ResultDirectory $RunId
New-Item -ItemType Directory -Path $RunDirectory | Out-Null
$resultPath = Join-Path $ResultDirectory 'board-result.json'
$partial = Join-Path $ResultDirectory "$RunId.partial"
$remoteReady = $false
$hostHash = ''

function Write-Result {
    param($Record)
    $json = $Record | ConvertTo-Json -Depth 8
    $json | Set-Content -LiteralPath (Join-Path $RunDirectory 'board-result.json') -Encoding utf8
    $json | Set-Content -LiteralPath $partial -Encoding utf8
    Move-Item -LiteralPath $partial -Destination $resultPath -Force
}
function Invoke-Adb {
    param([string[]]$Arguments)
    & adb -s $Serial @Arguments
    if ($LASTEXITCODE -ne 0) { throw "ADB failed ($LASTEXITCODE): $($Arguments -join ' ')" }
}
function Get-AdbText {
    param([string[]]$Arguments)
    $result = & adb -s $Serial @Arguments
    if ($LASTEXITCODE -ne 0) { throw "ADB failed ($LASTEXITCODE): $($Arguments -join ' ')" }
    return ($result | Out-String).Trim()
}
function Get-RepositoryRelativePath {
    param([string]$Path, [string]$Label)
    $relative = [System.IO.Path]::GetRelativePath($Repository, $Path)
    if ([System.IO.Path]::IsPathRooted($relative) -or $relative -eq '..' -or $relative.StartsWith("..$([System.IO.Path]::DirectorySeparatorChar)")) {
        throw "$Label must be within the repository for host validation"
    }
    return $relative.Replace('\', '/')
}

# Replace the previous success before any preflight can fail. Each run also keeps its own artifact.
Write-Result @{ status = 'running'; run_id = $RunId; serial = $Serial }
try {
    if ($Serial -ne 'e3d7377f6fc6d325') { throw 'Validation is pinned to serial e3d7377f6fc6d325' }
    if ($RemoteDirectory -cne '/userdata/inspireface-rv1126b/pack-validation') { throw 'Unsafe fixed remote directory' }
    $Repository = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
    $PackPath = (Resolve-Path -LiteralPath $PackPath).Path
    $BuildReport = (Resolve-Path -LiteralPath $BuildReport).Path
    $ValidatorDirectory = (Resolve-Path -LiteralPath $ValidatorDirectory).Path
    $Inventory = (Resolve-Path -LiteralPath $Inventory).Path
    $PythonPackPath = Get-RepositoryRelativePath $PackPath 'PackPath'
    $PythonBuildReport = Get-RepositoryRelativePath $BuildReport 'BuildReport'
    $PythonInventory = Get-RepositoryRelativePath $Inventory 'Inventory'
    foreach ($name in @('validate_pack_main', 'libInspireFace.so', 'librknnrt.so')) {
        if (!(Test-Path -LiteralPath (Join-Path $ValidatorDirectory $name) -PathType Leaf)) { throw "Validator deployment is missing $name" }
    }
    Push-Location $Repository
    try {
        & python -B -m command.rv1126b_pack.validate_pack $PythonPackPath $PythonBuildReport --inventory $PythonInventory
        if ($LASTEXITCODE -ne 0) { throw 'Host Task 3 resource-pack validation failed; refusing board deployment' }
    } finally { Pop-Location }
    $hostReport = Get-Content -Raw -LiteralPath $BuildReport | ConvertFrom-Json
    $hostHash = (Get-FileHash -LiteralPath $PackPath -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($hostHash -ne $hostReport.pack_sha256) { throw 'Host pack SHA-256 does not match the build report' }

    Invoke-Adb @('get-state')
    Invoke-Adb @('shell', 'mkdir', '-p', '/userdata/inspireface-rv1126b')
    if ((Get-AdbText @('shell', 'readlink', '-f', '/userdata/inspireface-rv1126b')) -cne '/userdata/inspireface-rv1126b') {
        throw 'Board parent path resolved outside /userdata/inspireface-rv1126b'
    }
    Invoke-Adb @('shell', 'mkdir', '-p', $RemoteDirectory)
    if ((Get-AdbText @('shell', 'readlink', '-f', $RemoteDirectory)) -cne $RemoteDirectory) { throw 'Board validation path resolved outside the fixed directory' }
    $remoteReady = $true
    Invoke-Adb @('shell', 'rm', '-rf', $RemoteDirectory)
    Invoke-Adb @('shell', 'mkdir', '-p', $RemoteDirectory)

    $runScript = @'
#!/bin/sh
cd /userdata/inspireface-rv1126b/pack-validation || exit 1
export LD_LIBRARY_PATH=/userdata/inspireface-rv1126b/pack-validation
./validate_pack_main ./pack > result.json
validator_status=$?
printf '%s\n' "$validator_status" > exit-status.txt
(cat /sys/kernel/debug/rknpu/version 2>/dev/null || cat /proc/rknn/version 2>/dev/null) > driver.txt
exit 0
'@
    $scriptPath = Join-Path $RunDirectory 'run.sh'
    [System.IO.File]::WriteAllText($scriptPath, $runScript.Replace("`r`n", "`n") + "`n", [System.Text.UTF8Encoding]::new($false))
    Invoke-Adb @('push', $PackPath, "$RemoteDirectory/pack")
    foreach ($name in @('validate_pack_main', 'libInspireFace.so', 'librknnrt.so')) {
        Invoke-Adb @('push', (Join-Path $ValidatorDirectory $name), "$RemoteDirectory/$name")
    }
    Invoke-Adb @('push', $scriptPath, "$RemoteDirectory/run.sh")
    Invoke-Adb @('shell', 'chmod', '755', "$RemoteDirectory/validate_pack_main")
    $roundTripPack = Join-Path $RunDirectory 'pack-roundtrip.partial'
    Invoke-Adb @('pull', "$RemoteDirectory/pack", $roundTripPack)
    $remoteHash = (Get-FileHash -LiteralPath $roundTripPack -Algorithm SHA256).Hash.ToLowerInvariant()
    Remove-Item -LiteralPath $roundTripPack -Force
    if ($remoteHash -ne $hostHash) { throw 'Board pack SHA-256 does not match the host build report' }
    # Inspect bytes returned from the deployed runtime; minimal board images need no strings utility.
    $roundTripRuntime = Join-Path $RunDirectory 'runtime-roundtrip.partial'
    Invoke-Adb @('pull', "$RemoteDirectory/librknnrt.so", $roundTripRuntime)
    $runtimeHash = (Get-FileHash -LiteralPath $roundTripRuntime -Algorithm SHA256).Hash
    if ($runtimeHash -ne (Get-FileHash -LiteralPath (Join-Path $ValidatorDirectory 'librknnrt.so') -Algorithm SHA256).Hash) {
        throw 'Board runtime SHA-256 does not match the deployed library'
    }
    $runtimeBytes = [System.Text.Encoding]::ASCII.GetString([System.IO.File]::ReadAllBytes($roundTripRuntime))
    $runtimeMatch = [regex]::Match($runtimeBytes, 'librknnrt version: ([0-9]+\.[0-9]+\.[0-9]+)[^\x00\r\n]*')
    $runtime = $runtimeMatch.Value.Trim()
    Remove-Item -LiteralPath $roundTripRuntime -Force
    if (!$runtime) { throw 'Board runtime evidence is empty' }
    $runtime | Set-Content -LiteralPath (Join-Path $RunDirectory 'runtime.txt') -Encoding utf8
    Invoke-Adb @('shell', 'sh', "$RemoteDirectory/run.sh")
    foreach ($name in @('result.json', 'exit-status.txt', 'driver.txt')) {
        Invoke-Adb @('pull', "$RemoteDirectory/$name", (Join-Path $RunDirectory $name))
    }
    $exitText = (Get-Content -Raw -LiteralPath (Join-Path $RunDirectory 'exit-status.txt') | Out-String).Trim()
    if ($exitText -notmatch '^[0-9]+$') { throw 'Board exit status evidence is empty or malformed' }
    $boardExit = [int]$exitText
    $driver = (Get-Content -Raw -LiteralPath (Join-Path $RunDirectory 'driver.txt') | Out-String).Trim()
    if (!$runtime -or !$driver) { throw 'Board runtime/driver evidence is empty' }
    $boardResult = Get-Content -Raw -LiteralPath (Join-Path $RunDirectory 'result.json') | ConvertFrom-Json
    if ($boardExit -ne 0 -or $boardResult.status -ne 'success' -or $boardResult.sdk_status -ne 0 -or
        $boardResult.archive_file_count -ne 12 -or $boardResult.model_count -ne 11 -or
        $boardResult.tag -cne 'Gundam_RV1126B' -or $boardResult.version -cne '4.0' -or $boardResult.major -cne 't4') {
        throw 'Board loader result does not match the expected pack metadata or exit status'
    }
    foreach ($entry in @{ run_id=$RunId; serial=$Serial; pack_sha256=$remoteHash; runtime=$runtime; driver=$driver; process_exit_status=$boardExit }.GetEnumerator()) {
        $boardResult | Add-Member -NotePropertyName $entry.Key -NotePropertyValue $entry.Value
    }
    Invoke-Adb @('shell', 'rm', '-rf', $RemoteDirectory)
    $remoteReady = $false
    Write-Result $boardResult
} catch {
    Write-Result @{ status='failed'; run_id=$RunId; serial=$Serial; pack_sha256=$hostHash; error=$_.Exception.Message }
    throw
} finally {
    if ($remoteReady) {
        try { Invoke-Adb @('shell', 'rm', '-rf', $RemoteDirectory) } catch { Write-Warning "Board cleanup failed: $_" }
    }
}
