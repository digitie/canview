# ADR-005: 최신 Windows 임베디드 toolchain baseline

- 상태: accepted
- 날짜: 2026-09-05
- 결정자: CANView 유지보수자

## 문맥

기존 문서는 ESP-IDF `v5.5.2`와 CMake `3.22+`를 예시로만 언급했고, Controller·Communicator target project가 없어 같은 checkout을 재현하기 어려웠다. 현재 개발 정본은 Windows PowerShell이며 ESP32-S3와 STM32G474를 한 저장소에서 함께 빌드해야 한다. ESP-IDF v6는 CMake 3.22.1 이상을 요구한다.

## 결정

1. ESP32-S3 Controller, Communicator ESP32, 향후 Diagnostic Bridge는 ESP-IDF `v6.0.3`을 같은 baseline으로 사용한다.
2. Communicator STM32는 CMake `4.4.3`, Ninja `1.13.2`, Arm GNU Toolchain `15.3.Rel1`과 STM32CubeG4 `v1.6.3`을 사용한다.
3. ESP-IDF와 STM32CubeG4는 `tools/toolchain-versions.json`에 release tag와 확인된 Git commit을 함께 기록한다. 이동하는 branch/tag만으로는 dependency를 고정한 것으로 간주하지 않는다.
4. SDK 원본은 repository에 vendoring하지 않는다. `tools/environment/setup-windows.ps1`가 Windows 사용자 도구를 확인하고 SDK를 정해진 경로에 clone한 뒤 commit과 핵심 파일을 검증한다.
5. Controller와 Communicator ESP32는 각각 독립된 ESP-IDF project, `sdkconfig.defaults`, partition table, public `canview_protocol` component 경계를 유지한다. 한 장치의 Flash·PSRAM 설정을 다른 장치에 복사하지 않는다.
6. 최신 baseline 선택은 기능 완료를 의미하지 않는다. Waveshare BSP/LVGL, UART, ESP-NOW, FDCAN, RTC/audio bring-up은 각 task와 target gate를 통과해야 한다.
7. 하드웨어 회로도 산출물은 KiCad `10.0.6`의 `kicad-cli`와 bundled Python을 별도 EDA baseline으로 사용한다. 회로도 생성·netlist·ERC·PDF export는 `tools/hardware/export-review.ps1`로 재현하고, EDA 도구 부재가 ESP-IDF·STM32 firmware 준비를 막지는 않는다.

## 결과

- clean Windows checkout에서 같은 manifest와 setup script로 두 ESP-IDF project와 STM32 CMake project를 준비할 수 있다.
- IDF 5.5.x 예제와 IDF 6.x의 API·Kconfig 차이는 BSP bring-up에서 드러나며, 그때 migration diff를 review한다.
- 현재 setup script는 CMake·Ninja·Arm GCC를 자동으로 임의 upgrade하지 않는다. 설치된 버전이 manifest와 다르면 실패시키고, ESP-IDF export가 PATH를 바꿔도 host tool 경로를 다시 앞세워 drift를 숨기지 않는다.
- 현재 target bootstrap은 safe/log-only 상태다. 실제 차량 CAN 송신 권한은 이 결정으로 변경되지 않는다.
- 현재 KiCad 산출물은 공식 ESP32-S3 land pattern과 `PG10-NRST` 표기를 반영한다. ERC/정합성 통과 여부는 generated report로 확인하며, 제조사 land·PCB·SI·전원 transient·HIL gate를 통과하기 전 제작/차량 연결 정본이 아니다.

## 대안 검토

- **ESP-IDF 5.5.2 유지**: Waveshare 예제 호환성은 낮은 migration risk가 있지만 최신 ESP-IDF bugfix와 장기 baseline을 포기하므로 채택하지 않았다.
- **floating latest branch 사용**: 처음에는 편하지만 재현 가능한 build와 CI artifact 비교가 불가능하므로 채택하지 않았다.
- **STM32CubeG4 vendor source commit**: 즉시 offline build가 가능하지만 저장소 크기와 vendor 변경 추적 비용이 커지고, 현재는 repository 밖 pinned checkout 정책이 더 적절하다.

## 영향받는 문서와 task

- 정본: [`tools/toolchain-versions.json`](../../tools/toolchain-versions.json), [`docs/development/toolchains.md`](../development/toolchains.md)
- 실행: [`tools/environment/setup-windows.ps1`](../../tools/environment/setup-windows.ps1)
- target bootstrap: [T-200](../tasks/T-200-communicator-esp32-bootstrap.md), [T-300](../tasks/T-300-controller-bootstrap.md), [T-102](../tasks/T-102-stm32-platform.md)
- version upgrade 시 이 ADR을 삭제하지 말고 새 ADR을 추가한다.
