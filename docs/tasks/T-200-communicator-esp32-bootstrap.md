# T-200 Communicator ESP32-S3-WROOM-1-N16R8 bootstrap

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G1/G2`
- 선행: `T-001`, `T-004`, `T-200a`
- 병렬 가능: `T-102`, `T-300`, `T-400`

## 목표

독립 ESP-IDF6.0.3 application의 GPIO·memory·partition와 실제 보드 bring-up을 검증한다. 기존 foundation image는 이미 target build됐으며 최소 boot/health/watchdog·고정 pool·config 검사 소프트웨어는 [T-200a](T-200a-esp32-core-bench.md)에서 선행 구현한다. UART와 ESP-NOW 실제 transport는 후속 task다.

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
- `canview_foundation`이 완료된 T-002/T-003/T-004의 generated public protocol과 C codec을 사용한다. incomplete v1.2 draft component는 main이 소비하지 않는다.
- `sdkconfig.defaults`의16MiB Flash /8MiB Octal PSRAM·80MHz·ECC로 foundation target binary를 생성했다. [최신 소프트웨어 evidence](../reviews/adversarial/evidence/2026-09-07-T-102a-merge.md)와 실물 메모리 계측은 구분한다.
- `partitions.csv`는 아직 4 MiB 범위의 단일 factory scaffold다. [OTA §4](../architecture/ota.md#4-esp-부팅flash-배치)의 N16R8 A/B·recovery·staging 배치는 T-204에서 최초 유선 설치용으로 구현한다. 현재 CSV를 OTA 완료 상태로 읽지 않는다.
- 실제 UART/ESP-NOW와 보드 memory/USB 계측은 미실행이다. T-200a의 소프트웨어 core 완료가 이 task의 G1/G2 완료를 뜻하지 않는다.

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
- [ ] T-200a가 제공한 `check_sdkconfig.py`와 CI 연결을 확인한다. 구형 MINI, R8 reserved pin 사용, ECC 해제·120MHz fixture 실패를 유지하고 실물 설정과 대조한다.

## 검증

```powershell
. .\tools\environment\setup-windows.ps1
idf.py -C firmware/communicator/esp32 set-target esp32s3
idf.py -C firmware/communicator/esp32 build
idf.py -C firmware/communicator/esp32 size-components
```

`check_sdkconfig.py`의 module-specific 검사·negative fixture·CI 연결은 T-200a에서 선행 제공한다. 파일이 존재하기 전 이 검증을 실행·통과했다고 기록하지 않는다. 이 task는 실제 보드에서 같은 설정과 메모리·pin 동작을 확인한다.

## 범위 밖

실제 peer pairing/session, vehicle command, DBC decode. Controller와 메모리 크기가 같아도 GPIO·ECC·복구 버튼·partition·역할 설정을 복사해 공용하지 않는다.


## 산출물·범위 경계

- 예상 산출물은 위 ESP-IDF project tree와 `tools/check_sdkconfig.py`·module-specific negative fixture다. 실제 target log/map/memory watermark를 남기고 초기화 실패 시 radio/control/UART 송신을 시작하지 않는다.
