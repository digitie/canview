[CmdletBinding()]
param(
    [string]$ToolRoot = (Join-Path $env:LOCALAPPDATA "CANView\toolchains"),
    [string]$ArmGnuRoot = "",
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

function Assert-ArmGccVersion {
    param([Parameter(Mandatory = $true)][string]$Text)

    $version = Get-FirstVersion -Text $Text
    $prefix = [regex]::Escape($manifest.tools.armGnuToolchain.compilerVersionPrefix)
    if ($version -notmatch "^$prefix(?:\.|$)") {
        throw "Arm GNU Toolchain compiler mismatch: expected GCC $($manifest.tools.armGnuToolchain.compilerVersionPrefix).x from release $($manifest.tools.armGnuToolchain.release), found $Text"
    }
    return $version
}

function Assert-ArmGnuProvenance {
    param([Parameter(Mandatory = $true)][string]$Root)

    $provenancePath = Join-Path $Root "canview-arm-gnu-provenance.json"
    if (-not (Test-Path -LiteralPath $provenancePath -PathType Leaf)) {
        throw "Verified Arm GNU provenance is missing: $provenancePath. Run tools/environment/install-arm-gnu.ps1."
    }
    $provenance = Get-Content -Raw -LiteralPath $provenancePath | ConvertFrom-Json
    $provenanceMatches = (($provenance.schemaVersion -eq 1) -and `
        ($provenance.release -eq $manifest.tools.armGnuToolchain.release) -and `
        ($provenance.archiveUrl -eq $manifest.tools.armGnuToolchain.archiveUrl) -and `
        ($provenance.archiveSha256.ToLowerInvariant() -eq $manifest.tools.armGnuToolchain.archiveSha256.ToLowerInvariant()))
    if (-not $provenanceMatches) {
        throw "Arm GNU provenance does not match manifest: $provenancePath"
    }

    $requiredFiles = @(
        "arm-none-eabi-gcc.exe",
        "arm-none-eabi-objcopy.exe",
        "arm-none-eabi-size.exe"
    )
    foreach ($file in $requiredFiles) {
        $path = Join-Path (Join-Path $Root "bin") $file
        $record = $provenance.files.PSObject.Properties[$file]
        if ($null -eq $record -or -not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Arm GNU provenance is incomplete: $path"
        }
        $actualHash = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($actualHash -ne $record.Value.ToLowerInvariant()) {
            throw "Arm GNU executable hash mismatch: $path"
        }
    }
}

function Normalize-GitUrl {
    param([Parameter(Mandatory = $true)][string]$Value)

    return (($Value.Trim() -replace "[\\/]+$", "") -replace "\.git$", "").ToLowerInvariant()
}

function Invoke-PinnedClone {
    param(
        [Parameter(Mandatory = $true)][string]$Repository,
        [Parameter(Mandatory = $true)][string]$Tag,
        [Parameter(Mandatory = $true)][string]$Commit,
        [Parameter(Mandatory = $true)][string]$Destination,
        [switch]$AllowClone
    )

    $temporaryDestination = $null
    try {
        if (-not (Test-Path -LiteralPath $Destination)) {
            if (-not $AllowClone) {
                throw "Pinned checkout is missing in verify-only mode: $Destination"
            }

            New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Destination) | Out-Null
            $temporaryDestination = "$Destination.partial-$([guid]::NewGuid().ToString('N'))"
            & git clone --recursive --depth 1 --branch $Tag $Repository $temporaryDestination
            if ($LASTEXITCODE -ne 0) {
                throw "Failed to clone $Repository at $Tag."
            }
        }

        $checkoutPath = if ($null -ne $temporaryDestination) {
            $temporaryDestination
        } else {
            $Destination
        }

        $remoteUrl = (& git -C $checkoutPath remote get-url origin).Trim()
        if ($LASTEXITCODE -ne 0 -or (Normalize-GitUrl -Value $remoteUrl) -ne (Normalize-GitUrl -Value $Repository)) {
            throw "Pinned checkout remote mismatch at $checkoutPath. Expected $Repository, found $remoteUrl."
        }

        $tagRef = "refs/tags/$Tag"
        & git -C $checkoutPath show-ref --verify --quiet $tagRef
        if ($LASTEXITCODE -ne 0) {
            throw "Pinned tag $Tag is not present at $checkoutPath."
        }

        $tagCommit = (& git -C $checkoutPath rev-parse "$Tag^{commit}").Trim()
        if ($LASTEXITCODE -ne 0 -or $tagCommit -ne $Commit) {
            throw "Pinned tag mismatch at $checkoutPath. Expected $Commit, found $tagCommit."
        }

        $currentCommit = (& git -C $checkoutPath rev-parse HEAD).Trim()
        if ($LASTEXITCODE -ne 0 -or $currentCommit -ne $Commit) {
            throw "Pinned checkout mismatch at $checkoutPath. Expected $Commit, found $currentCommit."
        }

        $dirtyFiles = ((& git -C $checkoutPath status --porcelain=v1) -join "`n").Trim()
        if ($LASTEXITCODE -ne 0 -or $dirtyFiles.Length -ne 0) {
            throw "Pinned checkout is not clean at $checkoutPath."
        }

        if ($AllowClone) {
            & git -C $checkoutPath submodule update --init --recursive
            if ($LASTEXITCODE -ne 0) {
                throw "Failed to update submodules at $checkoutPath."
            }
        }

        $submoduleStatus = @(& git -C $checkoutPath submodule status --recursive)
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to inspect submodules at $checkoutPath."
        }
        foreach ($line in $submoduleStatus) {
            if ($line.Length -gt 0 -and @('-', '+', 'U') -contains $line.Substring(0, 1)) {
                throw "Submodule checkout is incomplete or differs from the pinned commit at ${checkoutPath}: $line"
            }
        }

        $nestedDirtyStatus = @(& git -C $checkoutPath submodule foreach --quiet --recursive "git status --porcelain=v1")
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to inspect nested submodule worktrees at $checkoutPath."
        }
        foreach ($line in $nestedDirtyStatus) {
            if ($line.Trim().Length -gt 0) {
                throw "Nested submodule worktree is not clean at ${checkoutPath}: $line"
            }
        }

        if ($null -ne $temporaryDestination) {
            Move-Item -LiteralPath $temporaryDestination -Destination $Destination
            $temporaryDestination = $null
        }
    } catch {
        if ($null -ne $temporaryDestination -and (Test-Path -LiteralPath $temporaryDestination)) {
            Remove-Item -LiteralPath $temporaryDestination -Recurse -Force
        }
        throw
    }
}

$espIdfRoot = Join-Path $ToolRoot "esp-idf-$($manifest.sdk.espIdf.version.TrimStart('v'))"
$cubeG4Root = Join-Path $ToolRoot "STM32CubeG4-$($manifest.sdk.stm32CubeG4.version.TrimStart('v'))"

if ($ToolRoot -match "\s") {
    throw "ToolRoot cannot contain spaces because ESP-IDF does not support spaces in SDK paths: $ToolRoot"
}

$armRelease = $manifest.tools.armGnuToolchain.release.ToLowerInvariant()
$armCandidates = if ($ArmGnuRoot.Trim().Length -gt 0) {
    @((Resolve-Path -LiteralPath $ArmGnuRoot -ErrorAction Stop).Path)
} else {
    @(
        (Join-Path $ToolRoot "arm-gnu-toolchain-$armRelease"),
        (Join-Path $env:LOCALAPPDATA "CANView\toolchains\arm-gnu-toolchain-$armRelease")
    )
}
$armRoot = $armCandidates |
    Where-Object { Test-Path -LiteralPath $_ -PathType Container } |
    Select-Object -First 1
if ($null -eq $armRoot) {
    throw "Verified Arm GNU installation was not found. Run tools/environment/install-arm-gnu.ps1 or pass -ArmGnuRoot to its verified root."
}
Assert-ArmGnuProvenance -Root $armRoot
$armBin = Join-Path $armRoot "bin"
$env:Path = "$armBin;$env:Path"

$cmakeCommand = Get-RequiredCommand -Name "cmake"
$ninjaCommand = Get-RequiredCommand -Name "ninja"
$gccCommand = Get-RequiredCommand -Name "arm-none-eabi-gcc"
$objcopyCommand = Get-RequiredCommand -Name "arm-none-eabi-objcopy"
$sizeCommand = Get-RequiredCommand -Name "arm-none-eabi-size"
$null = Get-RequiredCommand -Name "git"

foreach ($command in @($gccCommand, $objcopyCommand, $sizeCommand)) {
    $resolvedCommand = (Resolve-Path -LiteralPath $command).Path
    if (-not $resolvedCommand.StartsWith("$armBin\", [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Arm GNU command resolved outside the verified root: $resolvedCommand"
    }
}

$cmakeVersion = Get-FirstVersion -Text (& cmake --version | Select-Object -First 1)
$cmakeBin = Split-Path -Parent $cmakeCommand
$ninjaVersion = Get-FirstVersion -Text (& ninja --version | Select-Object -First 1)
$ninjaBin = Split-Path -Parent $ninjaCommand
$gccVersionText = (& arm-none-eabi-gcc --version | Select-Object -First 1)
$gccVersion = Assert-ArmGccVersion -Text $gccVersionText
$gccBin = Split-Path -Parent $gccCommand

Assert-ExactVersion -Name "CMake" -Actual $cmakeVersion -Expected $manifest.tools.cmake.version
Assert-ExactVersion -Name "Ninja" -Actual $ninjaVersion -Expected $manifest.tools.ninja.version

$allowClone = -not $VerifyOnly
Invoke-PinnedClone `
    -Repository $manifest.sdk.espIdf.repository `
    -Tag $manifest.sdk.espIdf.version `
    -Commit $manifest.sdk.espIdf.gitCommit `
    -Destination $espIdfRoot `
    -AllowClone:$allowClone

Invoke-PinnedClone `
    -Repository $manifest.sdk.stm32CubeG4.repository `
    -Tag $manifest.sdk.stm32CubeG4.version `
    -Commit $manifest.sdk.stm32CubeG4.gitCommit `
    -Destination $cubeG4Root `
    -AllowClone:$allowClone

if (-not $VerifyOnly) {
    Write-Host "Installing ESP-IDF tools for esp32s3..."
    & (Join-Path $espIdfRoot "install.ps1") esp32s3
    if ($LASTEXITCODE -ne 0) {
        throw "ESP-IDF install.ps1 failed."
    }
}

$idfPyCandidates = @(
    (Join-Path $espIdfRoot "tools\idf.py"),
    (Join-Path $espIdfRoot "idf.py")
)
if (-not ($idfPyCandidates | Where-Object { Test-Path -LiteralPath $_ })) {
    throw "ESP-IDF checkout is incomplete: $espIdfRoot (idf.py not found)"
}
if (-not (Test-Path -LiteralPath (Join-Path $cubeG4Root "Drivers\CMSIS\Device\ST\STM32G4xx\Include\stm32g474xx.h"))) {
    throw "STM32CubeG4 checkout is incomplete: $cubeG4Root"
}

$env:IDF_PATH = $espIdfRoot
$env:STM32CUBE_G4_ROOT = $cubeG4Root
$env:CANVIEW_TOOLCHAIN_ROOT = $ToolRoot
$env:CANVIEW_ARM_GNU_ROOT = $armRoot

if (Test-Path -LiteralPath (Join-Path $espIdfRoot "export.ps1")) {
    . (Join-Path $espIdfRoot "export.ps1")
}

# ESP-IDF export may prepend its own tools. Keep the manifest-pinned Windows
# tool directories first for the checks below and for STM32 CMake presets.
$hostToolBins = @(
    $cmakeBin,
    $ninjaBin,
    $gccBin,
    (Split-Path -Parent $objcopyCommand),
    (Split-Path -Parent $sizeCommand)
) | Where-Object { $_ } | Select-Object -Unique
$env:Path = (($hostToolBins -join ";") + ";" + $env:Path)

$cmakeAfterExport = Get-FirstVersion -Text (& cmake --version | Select-Object -First 1)
$ninjaAfterExport = Get-FirstVersion -Text (& ninja --version | Select-Object -First 1)
$gccAfterExportText = (& arm-none-eabi-gcc --version | Select-Object -First 1)
$gccAfterExport = Assert-ArmGccVersion -Text $gccAfterExportText
Assert-ExactVersion -Name "CMake after ESP-IDF export" -Actual $cmakeAfterExport -Expected $manifest.tools.cmake.version
Assert-ExactVersion -Name "Ninja after ESP-IDF export" -Actual $ninjaAfterExport -Expected $manifest.tools.ninja.version

$env:CANVIEW_CMAKE_EXECUTABLE = $cmakeCommand
$env:CANVIEW_NINJA_EXECUTABLE = $ninjaCommand
$env:CANVIEW_ARM_GCC_EXECUTABLE = $gccCommand
$env:CANVIEW_ARM_OBJCOPY_EXECUTABLE = $objcopyCommand
$env:CANVIEW_ARM_SIZE_EXECUTABLE = $sizeCommand

$idfCommand = Get-Command idf.py -ErrorAction SilentlyContinue
if ($null -eq $idfCommand) {
    throw "idf.py is not available after ESP-IDF export. Dot-source this script from PowerShell: . .\tools\environment\setup-windows.ps1"
}

Write-Host "CANView Windows toolchain is ready."
Write-Host "  IDF_PATH=$env:IDF_PATH"
Write-Host "  STM32CUBE_G4_ROOT=$env:STM32CUBE_G4_ROOT"
Write-Host "  CMake=$cmakeAfterExport, Ninja=$ninjaAfterExport, Arm GCC=$gccAfterExport"
Write-Host "Run this script with dot-sourcing so the environment remains in the current PowerShell session."
