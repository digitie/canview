# T-500 protocol/CAN fault bench와 HIL harness

- 상태: `DONE`
- 우선순위: `P0`
- Gate: `G2/G4`
- 선행: `T-001`, `T-003`, `T-004`
- 병렬 가능: firmware implementation

## 2026-09-08 host harness 구현 시작

T-001/T-003/T-004가 main에 merge되어 공용 host runner를 시작한다. 현재 범위는
`tests/hil/`의 JSON-compatible YAML scenario inventory, deterministic host
adapter, fail-closed lab-rig adapter 계약, JSONL event와 machine-readable report,
negative fixture다. host PASS는 G2/HIL PASS가 아니며 실제 board·power rig·CAN
analyzer·차량 bus는 실행하지 않는다. 차량 CAN TX와 raw replay는 만들지 않는다.

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

- [x] scenario가 seed와 firmware digest로 재실행 가능하다. (12개 inventory와 canonical scenario digest)
- [x] analyzer log에서 capture-only TX 0건을 자동 판정한다. (직접 TX와 channel summary의 `tx_frames` 모두 검사)
- [x] command scenario는 expected allow-list 밖 frame 0건을 검사한다. (합성 forbidden-frame fixture와 negative unit test)
- [x] power/reset event와 protocol timeline을 한 monotonic report로 합친다.
- [x] 실패 시 첫 violated invariant와 관련 log offset을 출력한다.
- [x] CI에서는 host subset, lab에서는 hardware subset을 같은 scenario 형식으로 실행한다. (lab adapter는 연결 전 `SKIPPED/BLOCKED`)
- [x] machine-readable budget manifest의 map/stack/heap/queue/WCET/latency 위반이 첫 invariant로 보고된다.

## 2026-09-08 merge closure

T-500 source와 host harness는 PR [#30](https://github.com/digitie/canview/pull/30)의
merge commit `8f5d97ff924fe7fdb757a3a86a30cde5a80c2a09`으로 `origin/main`에
통합됐다. 최종 candidate `ff3121ce04328ff61a73f13492f8be9927f0dc98`에 대해
Reviewer A `CV-HOSTILE-20260908-T500-A-10`과 Reviewer B
`CV-HOSTILE-20260908-T500-B-10`이 모두 `PASS`를 반환했고 unresolved
P0/P1/P2/P3는 0건이다. 원본은 [통합 report](../reviews/adversarial/2026-09-08-T-500.md),
[A raw](../reviews/adversarial/evidence/2026-09-08-T-500-reviewer-a.md),
[B raw](../reviews/adversarial/evidence/2026-09-08-T-500-reviewer-b.md)에 보존했다.

최종 CI `34235313714`의 6개 job과 target firmware build가 success로 종료했고,
host unit 46/46, 전체 Python 97/97, host inventory 12/12, sanitizer,
문서 링크·plan 검사를 확인했다. g2 read-only rig는 장비 부재로 `SKIPPED`이며
물리 board/HIL·flash·전원/reset/brownout·CAN analyzer·차량 bus·provisioning은
`NOT_RUN`이다. 이 task는 harness 준비 완료이지 physical G2/G4 또는 차량 기능
승인이 아니며 차량 CAN TX는 `NO-GO`다.

## 계획 보완 수용 기준

- [x] 먼저 host runner·rig adapter 계약·합성 실패 fixture와 scenario inventory를 완성한다. target별 실제 실행 evidence는 T-101/T-103/T-104/T-201/T-501/T-503a/T-505/T-508에서 생성한다.
- [x] 이 task의 완료는 harness 준비다. 자체 G2/G4 통과나 차량 연결 승인이 아니며 실제 장치 미실행 결과는 SKIPPED/BLOCKED로 출력한다.
- [x] consumer task에 적힌 새 script/CTest 이름은 해당 consumer가 구현하고 suite에 등록한다. 현재 새 consumer script를 선행 가정하지 않으며, scenario가 없으면 runner가 nonzero로 종료한다.

## 검증

```bash
python -B tests/hil/run.py --suite host --seed 1 --output build/hil-host
python -B tests/hil/validate_evidence.py build/hil-host --expect-status PASS
python -B tests/hil/run.py --suite g2-readonly --rig-config tests/hil/rig.example.yaml --output build/hil-g2
python -B tests/hil/validate_evidence.py build/hil-g2 --expect-status SKIPPED
python -B -m unittest discover -s tests -p "test_*.py"
```

선택 host 검증은 `--expected-scenario`로 caller selection을 함께 전달하고,
selection을 생략한 PASS 검증은 trusted 전체 inventory를 요구한다. 2026-09-08
최종 수정 후 검증은 host scenario 12/12 PASS, T-500 단위 46/46 PASS, 전체
Python 회귀 97/97 PASS, evidence validator PASS, document link와 plan 검사
PASS였다.
실제 rig가 없어 G2 read-only 실행은 `SKIPPED`이며, board/HIL·power·CAN
analyzer·차량 bus 결과로 승격하지 않았다.

## 증거

rig schematic/version, calibration, scenario files, summary JSON과 digest를 남긴다. local serial/port/secret은 Git에 넣지 않는다.

## 산출물·범위 경계

- 위 rig 구성과 공용 scenario 계약이 구현 범위다. 산출물은 `tests/hil/` runner/adapter/로그 schema·실패 로그 fixture와 rig 설정 template다. 모든 소비 firmware의 시험 실행·실차 연결은 범위 밖이다.
- seed/rig revision/scenario coverage를 남긴다. 장비/script/test 수 부재는 SKIP/NOT_RUN이며 결과 parser 실패 시 pass를 생성하지 않는다.
