# T-500 protocol/CAN fault bench와 HIL harness

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G2/G4`
- 선행: `T-001`, `T-003`, `T-004`
- 병렬 가능: firmware implementation

## 목표

실차 없이 protocol loss, MCU reset, CAN load와 command feedback을 재현하는 반복 가능한 bench를 만든다. 수동 성공 화면이 아니라 machine-readable pass/fail evidence를 생산한다.

## 구성

- 두 ESP32 peer 또는 deterministic radio fault proxy
- STM32 target 또는 host semantic simulator
- 세 독립 CAN simulator channel과 analyzer
- programmable power/reset/CTS/hard-gate control
- scenario YAML, seed, expected event/frame assertions
- timestamped packet/CAN/power log collector

## 필수 scenario

1. ESP-NOW 0/1/5/20/50% loss, delay, duplicate, reorder
2. Controller/Communicator ESP/STM 개별·동시 reset
3. UART byte insert/delete/flip, CTS 100 ms/1초 stall
4. 세 CAN channel expected peak load와 bus-off/error passive
5. queue/pool exhaustion과 telemetry flood
6. stale/revision/profile/lease/hard-gate deny
7. duplicate command와 result-before-ACK
8. feedback success/mismatch/timeout, manual override
9. brownout during config/capture/command
10. end-to-end control tag 변조, UART injection, 256 live-token cache 포화
11. ACC/UV/guardian timeout과 gate-off valid-frame flood
12. SoftAP bulk+Bridge flood+최악 RSSI의 공용 radio pressure

## 수용 기준

- [ ] scenario가 seed와 firmware digest로 재실행 가능하다.
- [ ] analyzer log에서 capture-only TX 0건을 자동 판정한다.
- [ ] command scenario는 expected allow-list 밖 frame 0건을 검사한다.
- [ ] power/reset event와 protocol timeline을 한 monotonic report로 합친다.
- [ ] 실패 시 첫 violated invariant와 관련 log offset을 출력한다.
- [ ] CI에서는 host subset, lab에서는 hardware subset을 같은 scenario 형식으로 실행한다.
- [ ] machine-readable budget manifest의 map/stack/heap/queue/WCET/latency 위반이 첫 invariant로 보고된다.

## 계획 보완 수용 기준

- [ ] 먼저 host runner·rig adapter 계약·합성 실패 fixture와 scenario inventory를 완성한다. target별 실제 실행 evidence는 T-101/T-103/T-104/T-201/T-501/T-503a/T-505/T-508에서 생성한다.
- [ ] 이 task의 완료는 harness 준비다. 자체 G2/G4 통과나 차량 연결 승인이 아니며 실제 장치 미실행 결과는 SKIPPED/BLOCKED로 출력한다.
- [ ] consumer task에 적힌 새 script/CTest 이름은 해당 consumer가 구현하고 suite에 등록한다. 존재하지 않는 테스트 필터가 0 tests로 성공하지 않도록 `--no-tests=error` 또는 동등 검사를 제공한다.

## 검증

```bash
python tests/hil/run.py --suite host --seed 1
python tests/hil/run.py --suite g2-readonly --rig-config private/rig.yaml
python tests/hil/validate_evidence.py evidence/latest
```

## 증거

rig schematic/version, calibration, scenario files, summary JSON과 digest를 남긴다. local serial/port/secret은 Git에 넣지 않는다.

## 산출물·범위 경계

- 위 rig 구성과 공용 scenario 계약이 구현 범위다. 산출물은 `tests/hil/` runner/adapter/로그 schema·실패 로그 fixture와 rig 설정 template다. 모든 소비 firmware의 시험 실행·실차 연결은 범위 밖이다.
- seed/rig revision/scenario coverage를 남긴다. 장비/script/test 수 부재는 SKIP/NOT_RUN이며 결과 parser 실패 시 pass를 생성하지 않는다.
