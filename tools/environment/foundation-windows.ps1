[CmdletBinding()]
param([switch]$IncludeDocs)
Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$taskRoot = (Resolve-Path (Join-Path $PSScriptRoot "../..")).Path
$taskManifest = Get-Content -Raw -LiteralPath (Join-Path $taskRoot "tools/foundation-tools.json") | ConvertFrom-Json
$names = @("llvm", "cmake", "ninja")
if ($IncludeDocs) { $names += "doxygen" }
foreach ($name in $names) {
    $item = $taskManifest.windows.$name
    $directory = Join-Path $taskRoot (".tools/" + $item.directory)
    New-Item -ItemType Directory -Force $directory | Out-Null
    $archive = Join-Path $directory $item.archive
    if (-not (Test-Path -LiteralPath $archive)) {
        Invoke-WebRequest -Uri $item.url -OutFile $archive
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
