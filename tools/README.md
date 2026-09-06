# CANView 개발 도구

이 디렉터리는 Windows 정본 개발환경의 버전 manifest와 SDK 준비 스크립트를 보관한다. 실제 제품 기능이나 차량 설정은 이 디렉터리에 두지 않는다.

## 고정 버전

`toolchain-versions.json`이 현재 선택한 Windows x64 환경의 machine-readable 정본이다.

- ESP-IDF `v6.0.3`, target `esp32s3`
- STM32CubeG4 `v1.6.3`, MCU `STM32G474CEU6`
- CMake `4.4.3`
- Ninja `1.13.2`
- Arm GNU Toolchain `15.3.Rel1` (GCC `15.3.x`)
- KiCad `10.0.6` (`kicad-cli`, 회로도 export/ERC)

ESP-IDF와 STM32CubeG4는 manifest에 기록한 commit까지 검증한다. 버전 tag만 확인하지 않으므로 이동 tag나 잘못된 checkout을 조용히 사용하지 않는다.

## Windows 준비

먼저 CMake, Ninja, Git을 Windows `PATH`에 설치한다. Arm GNU Toolchain은 manifest의 `archiveUrl`에서 직접 내려받아 압축을 풀거나 이미 설치된 `arm-none-eabi-*`를 PATH에 둔다. archive SHA-256은 `archiveSha256`과 일치해야 한다. ESP-IDF build path에는 공백을 사용할 수 없으므로 Windows 사용자 profile 경로에 공백이 있으면 공백 없는 `-ToolRoot`를 지정한다. 그 다음 PowerShell에서 다음을 실행한다.

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
. .\tools\environment\foundation-windows.ps1
. .\tools\environment\setup-windows.ps1
```

기본 직접 설치 위치는 `%LOCALAPPDATA%\CANView\toolchains\arm-gnu-toolchain-15.3.rel1`이며 setup script가 자동 탐색한다. 다른 위치에 설치했으면 다음처럼 명시한다.

```powershell
. .\tools\environment\setup-windows.ps1 -ArmGnuRoot C:\ArmGNU\arm-gnu-toolchain-15.3.rel1
```

예를 들어 사용자 profile에 공백이 있는 경우:

```powershell
. .\tools\environment\setup-windows.ps1 -ToolRoot C:\CANViewToolchains
```

스크립트는 최신으로 임의 갱신하지 않고, manifest에 고정된 ESP-IDF와 STM32CubeG4를 `%LOCALAPPDATA%\CANView\toolchains` 아래에 clone한다. ESP-IDF Python 도구를 설치한 뒤 현재 세션에 `IDF_PATH`와 `STM32CUBE_G4_ROOT`를 설정한다.

이미 SDK를 준비한 뒤 검증만 할 때는 다음처럼 실행한다.

```powershell
. .\tools\environment\setup-windows.ps1 -VerifyOnly
```

`-VerifyOnly`에서도 CMake·Ninja·Arm GCC 버전과 SDK commit, `idf.py`, STM32G4 device header를 모두 확인한다.

## 대상별 빌드

```powershell
# Controller: ESP32-S3R8, 16 MB Flash / 8 MB PSRAM
idf.py -C firmware/controller set-target esp32s3
idf.py -C firmware/controller build

# Communicator ESP32: ESP32-S3-WROOM-1-N16R8, 16 MB Flash / 8 MB Octal PSRAM (ECC)
idf.py -C firmware/communicator/esp32 set-target esp32s3
idf.py -C firmware/communicator/esp32 build

# 외부 component의 public protocol 의존성 compile fixture
idf.py -C tests/fixtures/idf-public-component set-target esp32s3
idf.py -C tests/fixtures/idf-public-component build

# Communicator STM32: Debug 또는 Release
Push-Location firmware/communicator/stm32
cmake --preset debug
cmake --build --preset debug
Pop-Location
```

## KiCad 회로도 산출물 재생성

KiCad `10.0.6`의 bundled Python으로 회로도 원본을 재생성하고, `kicad-cli`로 XML netlist·ERC JSON·PDF를 같은 revision으로 갱신한다.

```powershell
. .\tools\hardware\export-review.ps1
```

이 스크립트는 `hardware/`의 Communicator·Bridge·Controller adapter·microphone 네 보드를 갱신하고, ERC·BOM·netlist·named-pad 정합성 및 정적 전원/WD 계산까지 검사한다. 검사 실패 시 nonzero로 종료한다. [상세 사용법과 제작 전 제한](../hardware/README.md)을 따른다. 실제 전원·PCB·HIL 승인을 뜻하지 않는다.

PowerShell에서 `idf.py`를 찾지 못하면 새 세션에서 다시 dot-source하고, STM32 configure가 실패하면 Arm archive 경로와 `STM32CUBE_G4_ROOT`를 확인한다. target build가 실제로 실행되지 않은 상태를 성공으로 기록하지 않는다.
