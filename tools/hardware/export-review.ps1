[CmdletBinding()]
param(
    [string]$RepositoryRoot,
    [string]$KiCadRoot = "C:\Program Files\KiCad\10.0",
    [switch]$SkipVersionCheck
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($RepositoryRoot)) {
    $RepositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
}

$manifestPath = Join-Path $RepositoryRoot "tools\toolchain-versions.json"
$manifest = Get-Content -Raw -Path $manifestPath | ConvertFrom-Json
$expectedKiCad = $manifest.eda.kicad.version
$kicadCli = Join-Path $KiCadRoot "bin\kicad-cli.exe"
$kicadPython = Join-Path $KiCadRoot "bin\python.exe"
$generator = Join-Path $RepositoryRoot "tools\hardware\build_schematics.py"
$boards = @("communicator", "bridge", "controller-adapter", "microphone")

foreach ($path in @($kicadCli, $kicadPython, $generator)) {
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Required hardware export input is missing: $path"
    }
}

$installedVersion = (Get-Item -LiteralPath $kicadCli).VersionInfo.ProductVersion
if (-not $SkipVersionCheck -and $installedVersion -ne $expectedKiCad) {
    throw "KiCad version mismatch: expected $expectedKiCad, found $installedVersion. Use -SkipVersionCheck only for a deliberate local diagnostic."
}

function Invoke-Checked {
    param(
        [Parameter(Mandatory = $true)][string]$Executable,
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )

    # Wait for the native process tree before consuming generated files. In the
    # desktop/WSL interop host, the call operator returned while child exporters
    # were still running, producing a mixed old/new netlist without an error.
    $quotedArguments = @($Arguments | ForEach-Object { '"' + $_.Replace('"', '\"') + '"' })
    $process = Start-Process -FilePath $Executable -ArgumentList $quotedArguments -NoNewWindow -Wait -PassThru
    $exitCode = [int]$process.ExitCode
    if ($exitCode -ne 0) {
        throw "Command failed with exit code ${exitCode}: $Executable $($Arguments -join ' ')"
    }
}

Push-Location -LiteralPath $RepositoryRoot
try {
    Write-Host "Generating four schematic projects..."
    Invoke-Checked -Executable $kicadPython -Arguments @($generator)
    foreach ($board in $boards) {
    $project = Join-Path $RepositoryRoot "hardware\$board\kicad\$board.kicad_sch"
    $outputRoot = Join-Path $RepositoryRoot "hardware\$board"
    Write-Host "Exporting XML netlist..."
    Invoke-Checked -Executable $kicadCli -Arguments @(
        "sch", "export", "netlist", "--format", "kicadxml",
        "-o", (Join-Path $outputRoot "netlist.xml"), $project
    )
    Invoke-Checked -Executable $kicadCli -Arguments @(
        "sch", "export", "netlist", "--format", "kicadsexpr",
        "-o", (Join-Path $outputRoot "$board.net"), $project
    )

    Write-Host "Running KiCad ERC..."
    Invoke-Checked -Executable $kicadCli -Arguments @(
        "sch", "erc", "--format", "json",
        "-o", (Join-Path $outputRoot "erc.json"), $project
    )

    Write-Host "Exporting schematic PDF..."
    Invoke-Checked -Executable $kicadCli -Arguments @(
        "sch", "export", "pdf", "--black-and-white",
        "-o", (Join-Path $outputRoot "schematic.pdf"), $project
    )
    }
    Write-Host "Checking exported connectivity, footprint pads, BOM and ERC..."
    Invoke-Checked -Executable $kicadPython -Arguments @(
        (Join-Path $RepositoryRoot "tools\hardware\validate_exports.py")
    )
    Write-Host "Checking static power and watchdog margins..."
    Invoke-Checked -Executable $kicadPython -Arguments @(
        (Join-Path $RepositoryRoot "tools\hardware\check_margins.py")
    )
}
finally {
    Pop-Location
}

Write-Host "Four review projects exported and consistency-checked with KiCad $installedVersion. This is not PCB fabrication or vehicle approval."
