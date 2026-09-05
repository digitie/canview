[CmdletBinding()]
param(
    [string]$ToolRoot = (Join-Path $env:LOCALAPPDATA "CANView\toolchains"),
    [switch]$VerifyOnly
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$manifestPath = Join-Path $repositoryRoot "tools\toolchain-versions.json"
$manifest = Get-Content -Raw -Path $manifestPath | ConvertFrom-Json

function Get-RequiredCommand {
    param([Parameter(Mandatory = $true)][string]$Name)

    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($null -eq $command) {
        throw "Required command '$Name' was not found on PATH. See tools/README.md."
    }
    return $command.Source
}

function Get-FirstVersion {
    param([Parameter(Mandatory = $true)][string]$Text)

    if ($Text -match "\d+\.\d+(?:\.\d+)?") {
        return $Matches[0]
    }
    throw "Could not parse a version from: $Text"
}

function Assert-ExactVersion {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Actual,
        [Parameter(Mandatory = $true)][string]$Expected
    )

    if ($Actual -ne $Expected) {
        throw "$Name version mismatch: expected $Expected, found $Actual."
    }
}

function Invoke-PinnedClone {
    param(
        [Parameter(Mandatory = $true)][string]$Repository,
        [Parameter(Mandatory = $true)][string]$Tag,
        [Parameter(Mandatory = $true)][string]$Commit,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    if (-not (Test-Path -LiteralPath $Destination)) {
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Destination) | Out-Null
        & git clone --recursive --depth 1 --branch $Tag $Repository $Destination
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to clone $Repository at $Tag."
        }
    }

    $currentCommit = (& git -C $Destination rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0 -or $currentCommit -ne $Commit) {
        throw "Pinned checkout mismatch at $Destination. Expected $Commit, found $currentCommit. Remove the checkout or repair it manually."
    }

    & git -C $Destination submodule update --init --recursive
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to update submodules at $Destination."
    }
}

$cmakeCommand = Get-RequiredCommand -Name "cmake"
$ninjaCommand = Get-RequiredCommand -Name "ninja"
$gccCommand = Get-RequiredCommand -Name "arm-none-eabi-gcc"
$null = Get-RequiredCommand -Name "arm-none-eabi-objcopy"
$null = Get-RequiredCommand -Name "arm-none-eabi-size"
$null = Get-RequiredCommand -Name "git"

$cmakeVersion = Get-FirstVersion -Text (& cmake --version | Select-Object -First 1)
$cmakeBin = Split-Path -Parent $cmakeCommand
$ninjaVersion = Get-FirstVersion -Text (& ninja --version | Select-Object -First 1)
$gccVersionText = (& arm-none-eabi-gcc --version | Select-Object -First 1)
$gccVersion = Get-FirstVersion -Text $gccVersionText

Assert-ExactVersion -Name "CMake" -Actual $cmakeVersion -Expected $manifest.tools.cmake.version
Assert-ExactVersion -Name "Ninja" -Actual $ninjaVersion -Expected $manifest.tools.ninja.version
if ($gccVersion -notmatch "^$([regex]::Escape($manifest.tools.armGnuToolchain.compilerVersionPrefix))(?:\.|$)") {
    throw "Arm GNU Toolchain compiler mismatch: expected GCC $($manifest.tools.armGnuToolchain.compilerVersionPrefix).x from release $($manifest.tools.armGnuToolchain.release), found $gccVersionText"
}

$espIdfRoot = Join-Path $ToolRoot "esp-idf-$($manifest.sdk.espIdf.version.TrimStart('v'))"
$cubeG4Root = Join-Path $ToolRoot "STM32CubeG4-$($manifest.sdk.stm32CubeG4.version.TrimStart('v'))"

if ($ToolRoot -match "\s") {
    throw "ToolRoot cannot contain spaces because ESP-IDF does not support spaces in SDK paths: $ToolRoot"
}

if (-not $VerifyOnly) {
    Invoke-PinnedClone `
        -Repository $manifest.sdk.espIdf.repository `
        -Tag $manifest.sdk.espIdf.version `
        -Commit $manifest.sdk.espIdf.gitCommit `
        -Destination $espIdfRoot

    Invoke-PinnedClone `
        -Repository $manifest.sdk.stm32CubeG4.repository `
        -Tag $manifest.sdk.stm32CubeG4.version `
        -Commit $manifest.sdk.stm32CubeG4.gitCommit `
        -Destination $cubeG4Root

    Write-Host "Installing ESP-IDF tools for esp32s3..."
    & (Join-Path $espIdfRoot "install.ps1") esp32s3
    if ($LASTEXITCODE -ne 0) {
        throw "ESP-IDF install.ps1 failed."
    }
}

if (-not (Test-Path -LiteralPath (Join-Path $espIdfRoot "idf.py"))) {
    throw "ESP-IDF checkout is incomplete: $espIdfRoot"
}
if (-not (Test-Path -LiteralPath (Join-Path $cubeG4Root "Drivers\CMSIS\Device\ST\STM32G4xx\Include\stm32g474xx.h"))) {
    throw "STM32CubeG4 checkout is incomplete: $cubeG4Root"
}

$env:IDF_PATH = $espIdfRoot
$env:STM32CUBE_G4_ROOT = $cubeG4Root
$env:CANVIEW_TOOLCHAIN_ROOT = $ToolRoot

if (Test-Path -LiteralPath (Join-Path $espIdfRoot "export.ps1")) {
    . (Join-Path $espIdfRoot "export.ps1")
}

# ESP-IDF export may prepend its own bundled CMake. Keep the manifest-pinned
# Windows CMake first so STM32 presets and IDF both use the validated host tool.
$env:Path = "$cmakeBin;$env:Path"

$idfCommand = Get-Command idf.py -ErrorAction SilentlyContinue
if ($null -eq $idfCommand) {
    throw "idf.py is not available after ESP-IDF export. Dot-source this script from PowerShell: . .\tools\environment\setup-windows.ps1"
}

Write-Host "CANView Windows toolchain is ready."
Write-Host "  IDF_PATH=$env:IDF_PATH"
Write-Host "  STM32CUBE_G4_ROOT=$env:STM32CUBE_G4_ROOT"
Write-Host "  CMake=$cmakeVersion, Ninja=$ninjaVersion, Arm GCC=$gccVersion"
Write-Host "Run this script with dot-sourcing so the environment remains in the current PowerShell session."
