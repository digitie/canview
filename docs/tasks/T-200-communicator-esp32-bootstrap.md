# T-200 Communicator ESP32-S3-MINI-1-N4R2 bootstrap

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G1/G2`
- 선행: `T-001`, `T-004`
- 병렬 가능: `T-102`, `T-300`, `T-400`

## 목표

README만 있던 directory를 build 가능한 독립 ESP-IDF 6.0.3 application으로 만든다. UART와 ESP-NOW 기능은 mock/stub 상태로 시작하며 GPIO·memory·partition 기준을 먼저 검증한다.

## 고정 target

- module `ESP32-S3-MINI-1-N4R2`
- 4 MB Quad Flash, 2 MB Quad PSRAM
- GPIO26 사용 금지, GPIO19/20 USB Serial/JTAG 유지
- UART1 TX17/RX18/RTS15/CTS16
- production baseline ESP-IDF `v6.0.3`

## 구현 범위

- top-level `CMakeLists.txt`, `main`, component tree, `sdkconfig.defaults`
- 4 MB partition table와 encrypted NVS/key partition 공간
- build metadata와 safe boot state
- fixed pool/queue utility, diagnostic counters
- UART pins를 flow-stop 상태로 초기화한 뒤 driver handoff
- watchdog과 reset reason reporting
- host-testable component 분리

## 현재 준비된 bootstrap

- `firmware/communicator/esp32/CMakeLists.txt`와 `main/`이 독립 ESP-IDF application으로 구성되어 있다.
- generated public protocol header는 `canview_protocol` component로 노출하고 main component가 `REQUIRES`로 소비한다.
- `sdkconfig.defaults`와 `partitions.csv`가 4 MB Flash / 2 MB Quad PSRAM target을 고정한다.
- 현재 partition은 단일 factory image로 OTA를 의도적으로 비활성화하며, image budget·provisioning 확인 후 A/B 전략을 별도 task로 확정한다.
- ESP-NOW, UART, watchdog, memory budget과 실제 `idf.py build`는 아직 구현·검증하지 않았다.

## 수용 기준

- [ ] clean IDF 6.0.3에서 `idf.py build`가 된다.
- [ ] partition가 4 MB를 넘지 않고 OTA 전략이 명시된다.
- [ ] PSRAM mode와 GPIO26 reservation이 sdkconfig test로 고정된다.
- [ ] boot 중 UART가 상대 STM에 byte를 보내지 않는다.
- [ ] callback용 fixed pool exhaustion이 counter를 남기고 leak하지 않는다.
- [ ] internal free heap 80 KiB, largest block 32 KiB 초기 budget을 만족한다.
- [ ] build ID, protocol/UART version, hardware revision을 USB log로 확인한다.

## 검증

```powershell
. .\tools\environment\setup-windows.ps1
idf.py -C firmware/communicator/esp32 set-target esp32s3
idf.py -C firmware/communicator/esp32 build
idf.py -C firmware/communicator/esp32 size-components
```

현재 저장소에는 `check_sdkconfig.py`가 아직 없으므로 module-specific config 검사는 T-001의 validator task에서 추가한다.

## 범위 밖

실제 peer pairing/session, vehicle command, DBC decode. Controller sdkconfig를 복사해 16 MB/8 MB로 빌드하지 않는다.
