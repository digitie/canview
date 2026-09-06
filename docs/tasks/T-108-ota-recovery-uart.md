# T-108 OTA-04 ESP와 STM recovery UART

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G0/G2 / OTA-04`
- 선행: `T-007`, `T-107`, `T-204`

## 목표

Communicator가 Controller·Bridge 없이 STM을 reset하고 안전하게 서명 image를 전달·복구하도록 boot UART를 구현한다.

## 고정 결정

[OTA §5·6·8](../architecture/ota.md)을 따른다. runtime UART 4 Mbps/RTS/CTS/CRC-32와 recovery 115200 8N1/no-flow-control/CRC32C, ROM AN3155 8E1/autobaud를 서로 다른 mode로 분리한다. 일반 웹에는 ROM raw 명령을 노출하지 않는다.

## 구현 범위

- 별도 recovery schema/generated C/Python codec·enum·CRC 파라미터·golden vector
- HELLO/BEGIN/WRITE/STATUS/PREPARE/ACTIVATE_TEST/CANCEL stop-and-wait
- reset ownership·PB9 recovery request·BOOT0/J32·runtime queue 폐기와 mode handoff
- transaction/request/offset/hash echo, readback ACK, static buffer와 ISR→worker 경계

## 범위 밖

runtime UART v1.0/1.1 재정의, ROM mass erase/option-byte 웹 기능, 실제 차량 연결.

## 예상 변경 파일

아래는 이 task가 생성·확정할 미래 산출물이다. 경로가 아직 없다는 사실을 검증 통과로 해석하지 않는다.

```text
protocol/schema/ota-uart-v1.yaml
protocol/generated/ota_uart/
firmware/communicator/esp32/components/canview_ota_uart/
firmware/communicator/stm32/bootloader/uart/
tests/ota/test_recovery_uart.py
```

## 수용 기준

- [ ] payload≤512B·encoded frame≤576B 및 모든 header/CRC/golden vector를 C/Python에서 검사한다.
- [ ] duplicate chunk는 다시 쓰지 않고 동일 ACK를 반환하며 reorder/hole/길이 초과/같은 offset 다른 data를 거절한다.
- [ ] PREPARED만으로 boot 선택이 바뀌지 않고 영속 activation intent 뒤에만 TEST pending을 기록한다. cancel/activate race 결과는 저장된 commit 순서로 결정한다.
- [ ] ACK 유실·한쪽 reset·새 boot_id·CTS 고착·중간 mode bytes에서 임의 재실행하지 않고 STATUS/digest로 실제 상태를 판정한다.
- [ ] 기본 2초·최대3회 retry, erase/prepare IN_PROGRESS 최대30초를 target worst case로 검증하고 timeout만으로 erase/swap을 중복 시작하지 않는다.
- [ ] 정상 MCU reset은 상대 reset을 끌어내리지 않고 service 중 CAN TX0·stale ARM 폐기를 계측한다. pin 고착 회로 판정은 T-508과 연결한다.

## 검증 계획

이 task의 recovery codec/parser tests와 fault-inject link fixture를 만든 뒤 malformed/fuzz/golden·HIL reset/ACK 유실을 실행한다. runtime UART 회귀시험도 함께 실행한다.

## evidence와 rollback

schema digest·통신 trace·request별 Flash write count·reset scope를 남긴다. mode handoff 실패 시 RECOVERY_WAIT와 gate off로 남고 runtime command를 자동 재전송하지 않는다.
