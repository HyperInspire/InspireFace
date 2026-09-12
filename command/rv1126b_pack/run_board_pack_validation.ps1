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
$ExpectedSerial = 'e3d7377f6fc6d325'
$RemoteDirectory = '/userdata/inspireface-rv1126b/pack-validation'
if ($Serial -ne $ExpectedSerial) { throw "This validation is pinned to board serial $ExpectedSerial" }
if ($RemoteDirectory -notmatch '^/userdata/inspireface-rv1126b/pack-validation$') { throw 'Unsafe fixed remote directory' }

$Repository = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
$PackPath = (Resolve-Path -LiteralPath $PackPath).Path
$BuildReport = (Resolve-Path -LiteralPath $BuildReport).Path
$ValidatorDirectory = (Resolve-Path -LiteralPath $ValidatorDirectory).Path
$Inventory = (Resolve-Path -LiteralPath $Inventory).Path
function Get-RepositoryRelativePath {
    param([string]$Path, [string]$Label)
    $relative = [System.IO.Path]::GetRelativePath($Repository, $Path)
    if ([System.IO.Path]::IsPathRooted($relative) -or $relative -eq '..' -or $relative.StartsWith("..$([System.IO.Path]::DirectorySeparatorChar)")) {
        throw "$Label must be within the repository for host validation"
    }
    return $relative.Replace('\', '/')
}
$PythonPackPath = Get-RepositoryRelativePath -Path $PackPath -Label 'PackPath'
$PythonBuildReport = Get-RepositoryRelativePath -Path $BuildReport -Label 'BuildReport'
$PythonInventory = Get-RepositoryRelativePath -Path $Inventory -Label 'Inventory'
foreach ($name in @('validate_pack_main', 'libInspireFace.so', 'librknnrt.so')) {
    if (!(Test-Path -LiteralPath (Join-Path $ValidatorDirectory $name) -PathType Leaf)) {
        throw "Validator deployment is missing $name; run build_validator.sh first"
    }
}

Push-Location $Repository
try {
    # Native argument encoding can corrupt the repository's non-ASCII absolute path on Windows.
    & python -B -m command.rv1126b_pack.validate_pack $PythonPackPath $PythonBuildReport --inventory $PythonInventory
    if ($LASTEXITCODE -ne 0) { throw 'Host Task 3 resource-pack validation failed; refusing board deployment' }
} finally {
    Pop-Location
}
$hostReport = Get-Content -Raw -LiteralPath $BuildReport | ConvertFrom-Json
$hostHash = (Get-FileHash -LiteralPath $PackPath -Algorithm SHA256).Hash.ToLowerInvariant()
if ($hostHash -ne $hostReport.pack_sha256) { throw 'Host pack SHA-256 does not match the build report' }

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

New-Item -ItemType Directory -Force -Path $ResultDirectory | Out-Null
$ResultDirectory = (Resolve-Path -LiteralPath $ResultDirectory).Path
$partial = Join-Path $ResultDirectory 'board-result.json.partial'
$resultPath = Join-Path $ResultDirectory 'board-result.json'
Remove-Item -LiteralPath $partial -Force -ErrorAction SilentlyContinue

Invoke-Adb -Arguments @('get-state')
Invoke-Adb -Arguments @('shell', 'mkdir', '-p', '/userdata/inspireface-rv1126b')
if ((Get-AdbText -Arguments @('shell', 'readlink', '-f', '/userdata/inspireface-rv1126b')) -ne '/userdata/inspireface-rv1126b') {
    throw 'Board parent path resolved outside /userdata/inspireface-rv1126b'
}
# The path is fixed and resolved before deletion; cleanup cannot target another board directory.
Invoke-Adb -Arguments @('shell', 'sh', '-c', "rm -rf $RemoteDirectory && mkdir -p $RemoteDirectory")
if ((Get-AdbText -Arguments @('shell', 'readlink', '-f', $RemoteDirectory)) -ne $RemoteDirectory) {
    throw 'Board validation path resolved outside the fixed directory'
}

try {
    Invoke-Adb -Arguments @('push', $PackPath, "$RemoteDirectory/pack")
    Invoke-Adb -Arguments @('push', (Join-Path $ValidatorDirectory 'validate_pack_main'), "$RemoteDirectory/validate_pack_main")
    Invoke-Adb -Arguments @('push', (Join-Path $ValidatorDirectory 'libInspireFace.so'), "$RemoteDirectory/libInspireFace.so")
    Invoke-Adb -Arguments @('push', (Join-Path $ValidatorDirectory 'librknnrt.so'), "$RemoteDirectory/librknnrt.so")
    Invoke-Adb -Arguments @('shell', 'chmod', '755', "$RemoteDirectory/validate_pack_main")
    $remoteHash = Get-AdbText -Arguments @('shell', 'sh', '-c', "sha256sum $RemoteDirectory/pack | awk '{print `$1}'")
    if ($remoteHash -ne $hostHash) { throw 'Board pack SHA-256 does not match the host build report' }
    $runtime = Get-AdbText -Arguments @('shell', 'sh', '-c', "strings $RemoteDirectory/librknnrt.so | grep -m1 'librknnrt version:' || true")
    $driver = Get-AdbText -Arguments @('shell', 'sh', '-c', "cat /sys/kernel/debug/rknpu/version 2>/dev/null || cat /proc/rknn/version 2>/dev/null || true")
    & adb -s $Serial shell sh -c "LD_LIBRARY_PATH=$RemoteDirectory $RemoteDirectory/validate_pack_main $RemoteDirectory/pack > $RemoteDirectory/result.json"
    $boardExit = $LASTEXITCODE
    Invoke-Adb -Arguments @('pull', "$RemoteDirectory/result.json", $partial)
    $boardResult = Get-Content -Raw -LiteralPath $partial | ConvertFrom-Json
    $boardResult | Add-Member -NotePropertyName serial -NotePropertyValue $Serial
    $boardResult | Add-Member -NotePropertyName pack_sha256 -NotePropertyValue $remoteHash
    $boardResult | Add-Member -NotePropertyName runtime -NotePropertyValue $runtime
    $boardResult | Add-Member -NotePropertyName driver -NotePropertyValue $driver
    $boardResult | Add-Member -NotePropertyName process_exit_status -NotePropertyValue $boardExit
    $boardResult | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $partial -Encoding utf8
    Move-Item -LiteralPath $partial -Destination $resultPath -Force
    if ($boardExit -ne 0 -or $boardResult.status -ne 'success') { throw "Board loader validation failed; see $resultPath" }
} finally {
    Invoke-Adb -Arguments @('shell', 'sh', '-c', "rm -rf $RemoteDirectory")
}
