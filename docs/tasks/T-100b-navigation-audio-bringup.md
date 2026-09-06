# T-100b R1 센서·원격 수음·전원 상태 firmware와 bring-up

- 상태: `BLOCKED`
- 우선순위: `P1`
- Gate: `G1/G2` 보조, 차량 TX 승인 아님
- 선행: `T-002`, `T-004`, `T-101`, `T-200`, `T-201`, `T-202`, `T-203`, `T-300`, `T-303`, `T-304`
- 외부 선행: 실물 보드·GNSS·마이크, 승인된 센서 장착·배선

## 목표와 고정 결정

[R1 pinmap](../hardware/r1/firmware-pinmap.md), [센서 회로](../hardware/r1/navigation-hardware.md), [mic 회로](../hardware/r1/bridge-controller-microphone.md), [navigation wire](../architecture/protocols/navigation.md)를 firmware와 bench evidence로 연결한다. Communicator N16R8/Bridge N8R2 module 차이를 보존하고 BMP384는 MTi AUX SPI 소유다. 기존 generic CAN 경로와 Controller 제한 의미 제어 권한을 바꾸지 않는다.

## 구현 범위

1. ESP-IDF BSP UART/RTS/CTS, MTSSP SPI/DRDY/open-drain reset, GPS switched power/read-only tap/PPS를 pinmap revision으로 구현한다. STM CMake BSP에 HSE/PG10-NRST/AF/gate health를 반영한다.
2. driver와 business model을 분리한다. ISR은 event/timestamp만 넘기고 host parser/fusion quality/telemetry는 task에서 수행한다. bounded buffers/queue bytes/priority/WCET/watchdog ownership을 문서화한다.
3. capability/version 협상과 peer별 atomic subscribe allocator, 전체8192B/s/peer4096B/s,latest-only/drop/count,16entry/30s result cache/high-water mark를 구현한다. ACK와 APPLIED를 구분한다.
4. GNSS/MTi firmware identity·baud/config readback, PPS epoch/clock anchor,45초 DR expiry,stale/reacquire,baro P0/mount revision을 구현한다. sensor fault로 CAN capture를 재부팅하지 않는다.
5. Controller I²S1 32kHz/64fs, DMA/FFT/level/peak와 mic health, 기존 volume automation holdoff/ramp를 통합한다. camera conflict를 build configuration에서 금지한다.
6. RTC0x51 oscillator-stop/백업/write-readback/source quality와 일몰 경고의 invalid 처리를 실제 보드로 검증한다.

## 예상 변경 파일

`firmware/communicator/{esp32,stm32}/`, `firmware/controller/`, `protocol/schema/`, 생성 codec, `tests/`, R1 pinmap 검증, `docs/hardware/r1/`의 실측 evidence. 구체 module 경계는 해당 embedded skills와 기존 architecture를 먼저 따른다.

## 수용 기준

- [ ] Windows target build와 host CI가 같은 pin/schema revision을 사용한다.
- [ ] 실제4Mbps CTS stall/overflow/reboot/frame corruption에서 bounded recovery한다.
- [ ] 수신/센서-only 모드에서 physical ARM 유무와 관계없이 vehicle TX를 실행하지 않는다.
- [ ] duplicate/lost RESULT/expired cache/conflict/reboot/revision wrap/동시 peer quota 시험이 통과한다.
- [ ] F9P UART1+UART2 PPS와 MTi firmware 조합이 readback·outage·재획득 시험을 통과한다.
- [ ] mic0.5/1/3m·고온/단락·탈착·clock loss에서 data integrity와 host 보호가 확인된다.
- [ ] 원음/실제 위치/키가 공개 Git에 들어가지 않는다. 합성 golden fixture만 공유한다.
- [ ] 모든 미지원 센서는 정상 숫자 대신 invalid/미지원으로 표시한다.

## 계획 보완 수용 기준

- [ ] ESP-NOW 1.4/UART 1.1 미협상 peer에는 센서 메시지가 0건이며 1.3/1.0 CAN 기능은 보존된다.
- [ ] calibration/P0/mount 설정의 owner, schema/revision, 영속 write/readback와 실패 복구를 T-304/T-205 저장 계약에 맞춰 확인한다.
- [ ] 16 kHz 온보드 분석과 32 kHz 원격 수음은 source별 설정·calibration을 분리한다. T-303 DSP를 재사용하고 T-100b는 원격 SI/센서 통합 evidence를 소유한다.

## 검증·evidence·rollback

현 단계 host 명령은 `python -m unittest discover -s tools/protocol -p test_navigation_codec.py -v`다. 이것만으로 위 acceptance를 체크하지 않는다. target map/build log, logic analyzer/Oscope waveform, sample counter/latency/power trace를 revision·환경·장비 정보와 함께 기록한다. 민감 위치/음성 원본은 로컬에 둔다.

범위 밖: 차량 제어 신호 확정, RTK 보정망 운영, 무제한 DR, 새 raw CAN 송신 API, 상시 BAT/CAN wake 회로. 실패 시 sensors/mic automation capability를 끄고 capture-only를 유지한다. 보호 gate를 firmware로 우회해 시험을 진행하지 않는다.


## 산출물·범위 경계

- CAN evidence 승격·차량 제어 권한 확대·OTA 공통 manager 구현은 범위 밖이다. 지원 센서가 없으면 정상 수치로 대체하지 않는다.
