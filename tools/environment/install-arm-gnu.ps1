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

function Get-FileInventory {
    param([Parameter(Mandatory = $true)][string]$Root)

    $resolvedRoot = (Resolve-Path -LiteralPath $Root -ErrorAction Stop).Path.TrimEnd('\', '/')
    $files = [ordered]@{}
    $items = Get-ChildItem -LiteralPath $resolvedRoot -File -Recurse -Force |
        Where-Object { $_.Name -ne "canview-arm-gnu-provenance.json" } |
        Sort-Object FullName
    foreach ($item in $items) {
        $relative = $item.FullName.Substring($resolvedRoot.Length).TrimStart('\', '/') -replace '\\', '/'
        $files[$relative] = (Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    }
    return $files
}

function Assert-FileInventoriesEqual {
    param(
        [Parameter(Mandatory = $true)][System.Collections.IDictionary]$Expected,
        [Parameter(Mandatory = $true)][System.Collections.IDictionary]$Actual,
        [Parameter(Mandatory = $true)][string]$Description
    )

    if ($Expected.Count -ne $Actual.Count) {
        throw "$Description file count mismatch: expected $($Expected.Count), found $($Actual.Count)"
    }
    foreach ($path in $Expected.Keys) {
        if (-not $Actual.Contains($path)) {
            throw "$Description is missing file: $path"
        }
        if ($Expected[$path] -ne $Actual[$path]) {
            throw "$Description file hash mismatch: $path"
        }
    }
}

function Find-ExtractedRoot {
    param([Parameter(Mandatory = $true)][string]$Parent)

    $candidates = @()
    $gccFiles = Get-ChildItem -LiteralPath $Parent -Filter "arm-none-eabi-gcc.exe" -File -Recurse -Force
    foreach ($gccFile in $gccFiles) {
        $binRoot = $gccFile.Directory.FullName
        $candidate = $gccFile.Directory.Parent.FullName
        $complete = $true
        foreach ($name in $requiredNames) {
            if (-not (Test-Path -LiteralPath (Join-Path $binRoot $name) -PathType Leaf)) {
                $complete = $false
                break
            }
        }
        if ($complete) {
            $candidates += $candidate
        }
    }
    $uniqueCandidates = @($candidates | Sort-Object -Unique)
    if ($uniqueCandidates.Count -ne 1) {
        throw "Expected one complete Arm GNU root under $Parent, found $($uniqueCandidates.Count)"
    }
    return $uniqueCandidates[0]
}

function Assert-ArchiveLayout {
    param([Parameter(Mandatory = $true)][string]$Archive)

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip = [System.IO.Compression.ZipFile]::OpenRead($Archive)
    try {
        $entryNames = @($zip.Entries | ForEach-Object { $_.FullName.TrimStart('/') -replace '\\', '/' })
        $gccEntries = @($entryNames | Where-Object { $_ -match '(?:^|/)bin/arm-none-eabi-gcc\.exe$' })
        $rootPrefixes = @()
        foreach ($entry in $gccEntries) {
            $binIndex = $entry.LastIndexOf('/bin/')
            $prefix = if ($binIndex -lt 0) { "" } else { $entry.Substring(0, $binIndex) }
            $hasAllTools = $true
            foreach ($name in $requiredNames) {
                $expected = if ($prefix.Length -eq 0) { "bin/$name" } else { "$prefix/bin/$name" }
                if ($entryNames -notcontains $expected) {
                    $hasAllTools = $false
                    break
                }
            }
            if ($hasAllTools) {
                $rootPrefixes += $prefix
            }
        }
        $uniquePrefixes = @($rootPrefixes | Sort-Object -Unique)
        if ($uniquePrefixes.Count -ne 1) {
            throw "Unexpected Arm GNU archive layout in ${Archive}: expected one complete bin root, found $($uniquePrefixes.Count)"
        }
        Write-Host "Verified Arm GNU archive layout root prefix: '$($uniquePrefixes[0])'"
    } finally {
        $zip.Dispose()
    }
}

function Write-Provenance {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][System.Collections.IDictionary]$Files
    )

    [ordered]@{
        schemaVersion = 1
        release = $tool.release
        archiveUrl = $tool.archiveUrl
        archivePath = $archive
        archiveSha256 = $archiveHash
        fileCount = $Files.Count
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
Assert-ArchiveLayout -Archive $archive

$existingRootHandled = $false
if (Test-Path -LiteralPath $installRoot -PathType Container) {
    if (-not $AdoptExisting) {
        throw "Existing Arm GNU root has no verified provenance. Re-run with -AdoptExisting to compare required executables against the pinned archive: $installRoot"
    }
    $adoptStaging = Join-Path $Destination ".arm-gnu-adopt-$([guid]::NewGuid().ToString('N'))"
    try {
        Expand-Archive -LiteralPath $archive -DestinationPath $adoptStaging
        $extractedRoot = Find-ExtractedRoot -Parent $adoptStaging
        $expectedFiles = Get-FileInventory -Root $extractedRoot
        $existingFiles = Get-FileInventory -Root $installRoot
        Assert-FileInventoriesEqual -Expected $expectedFiles -Actual $existingFiles -Description "Existing Arm GNU installation"
        Write-Provenance -Root $installRoot -Files $existingFiles
        Write-Host "Adopted existing Arm GNU installation after complete pinned archive comparison: $installRoot"
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
        $files = Get-FileInventory -Root $installRoot
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
