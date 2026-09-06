[CmdletBinding()]
param([switch]$IncludeDocs)
Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
# Korean diagnostics must also work in redirected output on English Windows.
$env:PYTHONUTF8 = "1"
$taskRoot = (Resolve-Path (Join-Path $PSScriptRoot "../..")).Path
$taskManifest = Get-Content -Raw -LiteralPath (Join-Path $taskRoot "tools/foundation-tools.json") | ConvertFrom-Json

function Invoke-VerifiedDownload {
    param(
        [Parameter(Mandatory = $true)][string]$Url,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    $partial = "$Destination.partial"
    $curl = Get-Command curl.exe -ErrorAction SilentlyContinue
    for ($attempt = 1; $attempt -le 5; $attempt++) {
        if (Test-Path -LiteralPath $partial) {
            Remove-Item -LiteralPath $partial -Force
        }
        $downloadExitCode = 0
        if ($null -ne $curl) {
            & $curl.Source --fail --location --retry 3 --retry-delay 2 --retry-all-errors --silent --show-error --output $partial $Url
            $downloadExitCode = $LASTEXITCODE
        } else {
            try {
                Invoke-WebRequest -Uri $Url -OutFile $partial
            } catch {
                $downloadExitCode = 1
            }
        }
        $valid = ($downloadExitCode -eq 0) -and
            (Test-Path -LiteralPath $partial -PathType Leaf) -and
            ((Get-Item -LiteralPath $partial).Length -gt 0)
        if ($valid) {
            Move-Item -LiteralPath $partial -Destination $Destination
            return
        }
        if ($attempt -lt 5) {
            Start-Sleep -Seconds $attempt
        }
    }
    throw "Download failed or returned an empty archive after retries: $Url"
}

$names = @("llvm", "cmake", "ninja")
if ($IncludeDocs) { $names += "doxygen" }
foreach ($name in $names) {
    $item = $taskManifest.windows.$name
    $directory = Join-Path $taskRoot (".tools/" + $item.directory)
    New-Item -ItemType Directory -Force $directory | Out-Null
    $archive = Join-Path $directory $item.archive
    if (-not (Test-Path -LiteralPath $archive)) {
        Invoke-VerifiedDownload -Url $item.url -Destination $archive
    } elseif ((Get-Item -LiteralPath $archive).Length -le 0) {
        throw "Cached archive is empty: $archive. Remove it and rerun the setup script."
    }
    if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant() -ne $item.sha256) {
        throw "Archive SHA256 mismatch: $archive. No automatic overwrite/removal."
    }
    $binary = Join-Path $directory $item.binary
    if (-not (Test-Path -LiteralPath $binary)) {
        if ($item.archive.EndsWith(".zip")) {
            Expand-Archive -LiteralPath $archive -DestinationPath (Join-Path $directory "bin")
        } else {
            & tar -xf $archive -C $directory
            if ($LASTEXITCODE -ne 0) { throw "Extraction failed: $name" }
        }
    }
    $versionLines = & $binary --version
    $version = $versionLines | Select-Object -First 1
    if ($LASTEXITCODE -ne 0 -or $version -notmatch [regex]::Escape($item.version)) {
        throw "Tool version mismatch: $name $version"
    }
    $env:PATH = (Split-Path $binary) + ";" + $env:PATH
    Write-Output "$name : $version"
}
# Clang's GNU frontend uses Microsoft's SDK/linker on Windows.
$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio/Installer/vswhere.exe"
$vsInstall = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsInstall) { throw "Visual Studio C++ Build Tools + Windows SDK are required." }
& (Join-Path $vsInstall "Common7/Tools/Launch-VsDevShell.ps1") -Arch amd64 -HostArch amd64 -SkipAutomaticLocation | Out-Null
# DevShell may prepend its bundled CMake/Ninja. Restore pinned tools last.
foreach ($name in $names) {
    $item = $taskManifest.windows.$name
    $env:PATH = (Split-Path (Join-Path $taskRoot (".tools/" + $item.directory + "/" + $item.binary))) + ";" + $env:PATH
}
