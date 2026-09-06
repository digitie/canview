[CmdletBinding()]
param(
    [string]$Destination = (Join-Path $env:LOCALAPPDATA "CANView\toolchains"),
    [string]$ArchivePath = "",
    [switch]$AdoptExisting
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$manifest = Get-Content -Raw -LiteralPath (Join-Path $repositoryRoot "tools\toolchain-versions.json") | ConvertFrom-Json
$tool = $manifest.tools.armGnuToolchain
$release = $tool.release.ToLowerInvariant()
$installRoot = Join-Path $Destination "arm-gnu-toolchain-$release"
$provenancePath = Join-Path $installRoot "canview-arm-gnu-provenance.json"
$requiredNames = @("arm-none-eabi-gcc.exe", "arm-none-eabi-objcopy.exe", "arm-none-eabi-size.exe")

function Get-RequiredToolHashes {
    param([Parameter(Mandatory = $true)][string]$Root)

    $files = [ordered]@{}
    foreach ($name in $requiredNames) {
        $path = Join-Path (Join-Path $Root "bin") $name
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Arm GNU installation is missing required executable: $path"
        }
        $files[$name] = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
    }
    return $files
}

function Find-ExtractedRoot {
    param([Parameter(Mandatory = $true)][string]$Parent)

    $candidates = @(
        $Parent,
        (Join-Path $Parent "arm-gnu-toolchain-$release")
    )
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath (Join-Path $candidate "bin\arm-none-eabi-gcc.exe") -PathType Leaf) {
            return $candidate
        }
    }
    throw "Unexpected Arm GNU archive layout under: $Parent"
}

function Write-Provenance {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][hashtable]$Files
    )

    [ordered]@{
        schemaVersion = 1
        release = $tool.release
        archiveUrl = $tool.archiveUrl
        archivePath = $archive
        archiveSha256 = $archiveHash
        files = $Files
    } | ConvertTo-Json -Depth 4 | Out-File -LiteralPath (Join-Path $Root "canview-arm-gnu-provenance.json") -Encoding utf8
}

if (Test-Path -LiteralPath $Destination -PathType Leaf) {
    throw "Arm GNU destination is a file: $Destination"
}
New-Item -ItemType Directory -Force -Path $Destination | Out-Null

$archive = if ($ArchivePath.Trim().Length -gt 0) {
    (Resolve-Path -LiteralPath $ArchivePath -ErrorAction Stop).Path
} else {
    Join-Path $env:TEMP "arm-gnu-toolchain-$release.zip"
}
if (-not (Test-Path -LiteralPath $archive -PathType Leaf)) {
    Write-Host "Downloading pinned Arm GNU archive..."
    Invoke-WebRequest -Uri $tool.archiveUrl -OutFile $archive
}
$archiveHash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
if ($archiveHash -ne $tool.archiveSha256.ToLowerInvariant()) {
    throw "Arm GNU archive SHA256 mismatch: $archiveHash"
}

$existingRootHandled = $false
if (Test-Path -LiteralPath $installRoot -PathType Container) {
    if (-not $AdoptExisting) {
        throw "Existing Arm GNU root has no verified provenance. Re-run with -AdoptExisting to compare required executables against the pinned archive: $installRoot"
    }
    $adoptStaging = Join-Path $Destination ".arm-gnu-adopt-$([guid]::NewGuid().ToString('N'))"
    try {
        Expand-Archive -LiteralPath $archive -DestinationPath $adoptStaging
        $extractedRoot = Find-ExtractedRoot -Parent $adoptStaging
        $expectedFiles = Get-RequiredToolHashes -Root $extractedRoot
        $existingFiles = Get-RequiredToolHashes -Root $installRoot
        foreach ($name in $requiredNames) {
            if ($expectedFiles[$name] -ne $existingFiles[$name]) {
                throw "Existing Arm GNU executable differs from the pinned archive: $name"
            }
        }
        Write-Provenance -Root $installRoot -Files $existingFiles
        Write-Host "Adopted existing Arm GNU installation after pinned executable comparison: $installRoot"
        $existingRootHandled = $true
    } finally {
        if (Test-Path -LiteralPath $adoptStaging -PathType Container) {
            Remove-Item -LiteralPath $adoptStaging -Recurse -Force
        }
    }
}

if (-not $existingRootHandled) {
    $staging = Join-Path $Destination ".arm-gnu-staging-$([guid]::NewGuid().ToString('N'))"
    try {
        Expand-Archive -LiteralPath $archive -DestinationPath $staging
        $extractedRoot = Find-ExtractedRoot -Parent $staging
        Move-Item -LiteralPath $extractedRoot -Destination $installRoot
        $files = Get-RequiredToolHashes -Root $installRoot
        Write-Provenance -Root $installRoot -Files $files
        Write-Host "Installed verified Arm GNU Toolchain $($tool.release) at $installRoot"
    } catch {
        if (Test-Path -LiteralPath $installRoot -PathType Container) {
            Remove-Item -LiteralPath $installRoot -Recurse -Force
        }
        throw
    } finally {
        if (Test-Path -LiteralPath $staging -PathType Container) {
            Remove-Item -LiteralPath $staging -Recurse -Force
        }
    }
}
