# T-202 Communicator ESP32 UART link, router와 boot epoch

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G2`
- 선행: `T-104`, `T-200`
- 병렬 가능: `T-201`

## 목표

UART semantic message를 ESP-NOW peer로 route하고 STM boot epoch를 무선 session과 안전하게 결합한다.

## 구현 범위

- ESP-IDF UART1 4 Mbps RTS/CTS driver와 fixed RX/TX rings
- UART v1.0 HELLO/heartbeat/link state adapter
- P0/P1/state/raw queue mapping
- CAN batch/bus state→peer router
- command/lease/config→STM transaction router
- Controller↔STM control envelope의 byte-identical opaque forwarding과 end-to-end clock exchange
- STM boot change transaction
- flow stall, parser error, sequence gap, queue pressure diagnostics

## STM boot change 원자 동작

새 STM `boot_id`를 보면 순서대로 UART queue flush, pending/lease 폐기, ESP-NOW session close, 새 random session HELLO, snapshot 요청을 수행한다. 새 snapshot 전 CAN batch 송신을 막는다. 이 과정의 어느 중간 상태에서도 old/new timestamp를 섞지 않는다.

## 수용 기준

- [ ] STM reboot마다 모든 online peer session ID가 바뀐다.
- [ ] reboot 전 CAN batch가 새 session에서 전달되지 않는다.
- [ ] pending command를 자동 resend하지 않고 terminal `session changed`로 정리한다.
- [ ] UART heartbeat loss에서 control lease가 STM과 ESP 양쪽에서 사라진다.
- [ ] route table에 raw CAN→vehicle TX 경로가 없다.
- [ ] flow stall 중 telemetry가 control queue를 막지 않는다.
- [ ] end-to-end STM timestamp ordering이 session 내부에서 보존된다.
- [ ] Controller↔STM time mapping의 offset/uncertainty/generation이 hop reboot·wrap에서 invalidated된다.
- [ ] router가 control envelope를 재직렬화하거나 origin/issued time/TTL/tag를 변경하지 않는다.

## 검증

```bash
ctest --preset host-sanitize -R 'router|boot-epoch' --output-on-failure
python tests/hil/run_dual_mcu_reset_matrix.py --seed 1
```

## evidence

STM-only reset, ESP-only reset, simultaneous reset의 state timeline과 packet capture를 저장한다. session이 유지된 시험 결과는 실패다.
