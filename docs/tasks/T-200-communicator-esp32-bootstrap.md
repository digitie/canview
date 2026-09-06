# T-200 Communicator ESP32-S3-WROOM-1-N16R8 bootstrap

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G1/G2`
- 선행: `T-001`, `T-004`
- 병렬 가능: `T-102`, `T-300`, `T-400`

## 목표

README만 있던 directory를 build 가능한 독립 ESP-IDF 6.0.3 application으로 만든다. UART와 ESP-NOW 기능은 mock/stub 상태로 시작하며 GPIO·memory·partition 기준을 먼저 검증한다.

## 고정 target

- module `ESP32-S3-WROOM-1-N16R8`, board ID `comm-r2-n16r8`
- 16 MiB Quad Flash, 8 MiB Octal PSRAM / 80 MHz / ECC (가용 7.5 MiB)
- GPIO35/36/37 외부 사용 금지, GPIO19/20 USB Serial/JTAG 유지
- UART1 TX17/RX18/RTS15/CTS16
- production baseline ESP-IDF `v6.0.3`

## 구현 범위

- top-level `CMakeLists.txt`, `main`, component tree, `sdkconfig.defaults`
- N16R8 memory 검증과 encrypted NVS/key 영역; OTA partition/recovery 구현은 T-204
- build metadata와 safe boot state
- fixed pool/queue utility, diagnostic counters
- UART pins를 flow-stop 상태로 초기화한 뒤 driver handoff
- watchdog과 reset reason reporting
- host-testable component 분리

## 현재 준비된 bootstrap

- `firmware/communicator/esp32/CMakeLists.txt`와 `main/`이 독립 ESP-IDF application으로 구성되어 있다.
- 향후 generated public protocol header를 받을 `canview_protocol` component directory를 준비하되, 현재 incomplete v1.2 header는 main component가 소비하지 않는다. T-002 완료 뒤 `REQUIRES`를 연결한다.
- `sdkconfig.defaults`는 16 MiB Flash / 8 MiB Octal PSRAM·80 MHz·ECC로 변경됐으나 target build evidence는 없다.
- `partitions.csv`는 아직 4 MiB 범위의 단일 factory scaffold다. [OTA §4](../architecture/ota.md#4-esp-부팅flash-배치)의 N16R8 A/B·recovery·staging 배치는 T-204에서 최초 유선 설치용으로 구현한다. 현재 CSV를 OTA 완료 상태로 읽지 않는다.
- ESP-NOW, UART, watchdog, memory budget과 실제 `idf.py build`는 아직 구현·검증하지 않았다.

## 수용 기준

- [ ] clean IDF 6.0.3에서 `idf.py build`가 된다.
- [ ] scaffold partition가 실장 16 MiB 안에 있고 T-204의 OTA layout migration과 구분된다.
- [ ] Octal/80 MHz/ECC와 GPIO35/36/37 reservation이 sdkconfig 검사로 고정되며 7.5 MiB 가용 PSRAM을 실물에서 확인한다.
- [ ] boot 중 UART가 상대 STM에 byte를 보내지 않는다.
- [ ] callback용 fixed pool exhaustion이 counter를 남기고 leak하지 않는다.
- [ ] internal free heap 80 KiB, largest block 32 KiB 초기 budget을 만족한다.
- [ ] build ID, protocol/UART version, hardware revision을 USB log로 확인한다.

## 계획 보완 수용 기준

- [ ] GPIO7 LOW, GPIO1 reset 요청 해제, GPIO9 open-drain 해제, GPIO48 input·GPIO38 mux sense를 현행 pinmap과 대조한다. 정상 앱 health/서비스 검사 후에만 RX 허용하며 J31 우회 pull-up은 없다.
- [ ] `check_sdkconfig.py`를 이 task에서 추가하고 T-001 CI에 연결한다. 구형 MINI, R8 reserved pin 사용, ECC 해제·120 MHz fixture가 실패한다.

## 검증

```powershell
. .\tools\environment\setup-windows.ps1
idf.py -C firmware/communicator/esp32 set-target esp32s3
idf.py -C firmware/communicator/esp32 build
idf.py -C firmware/communicator/esp32 size-components
```

현재 저장소에는 `check_sdkconfig.py`가 아직 없다. 이 task에서 module-specific 검사와 negative fixture를 구현하고 T-001의 CI에 연결한다. 파일이 존재하기 전 이 검증을 실행·통과했다고 기록하지 않는다.

## 범위 밖

실제 peer pairing/session, vehicle command, DBC decode. Controller와 메모리 크기가 같아도 GPIO·ECC·복구 버튼·partition·역할 설정을 복사해 공용하지 않는다.


## 산출물·범위 경계

- 예상 산출물은 위 ESP-IDF project tree와 `tools/check_sdkconfig.py`·module-specific negative fixture다. 실제 target log/map/memory watermark를 남기고 초기화 실패 시 radio/control/UART 송신을 시작하지 않는다.
