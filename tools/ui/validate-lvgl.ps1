param(
    [string]$LvglSource = "$PSScriptRoot/../../.tools/lvgl-8.4.0"
)
$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path "$PSScriptRoot/../..").Path
$sourceRoot = (Resolve-Path $LvglSource).Path
$expectedCommit = '4495f428630cc1741bd8bfd977f080e8460e8e8d'
$sourceCommit = (& git -C $sourceRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $sourceCommit -ne $expectedCommit) {
    throw "공식 LVGL 8.4.0 commit 불일치: $sourceCommit"
}
$sourceDirty = & git -C $sourceRoot status --porcelain
if ($LASTEXITCODE -ne 0 -or $sourceDirty) {
    throw 'LVGL 원본 checkout이 변경되어 있습니다.'
}
if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    $vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
    if (Test-Path -LiteralPath $vswhere) {
        $vsRoot = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($vsRoot) {
            & "$vsRoot/Common7/Tools/Launch-VsDevShell.ps1" -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
        }
    }
}
foreach ($tool in @('cl.exe', 'cmake.exe', 'ninja.exe')) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
        throw "Windows native 도구를 찾을 수 없습니다: $tool"
    }
}
$buildRoot = Join-Path $repoRoot '.tools/lvgl-host-build'
& cmake -S "$repoRoot/tests/lvgl" -B $buildRoot -G Ninja "-DLVGL_SOURCE_DIR=$sourceRoot" -DCMAKE_BUILD_TYPE=Debug
if ($LASTEXITCODE -ne 0) { throw 'LVGL host configure 실패' }
& cmake --build $buildRoot --parallel 6
if ($LASTEXITCODE -ne 0) { throw 'LVGL host compile 실패' }
& ctest --test-dir $buildRoot --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'LVGL host regression 실패' }
Write-Output "PASS: 공식 LVGL 8.4.0 $sourceCommit / Windows native host. 보드·실차 gate는 미실행."
