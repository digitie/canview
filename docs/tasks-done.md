# CANView 완료 task archive

완료·종료 task를 newest-first로 이동해 기록한다. 설계 감사와 문서 구조 정리는 구현 task 완료 이력과 분리해 PR·journal에 기록한다.

## 2026-09-19 T-007

| ID | 상태 | 우선순위 | 작업 | 선행 |
|---|---|---:|---|---|
| [T-007](tasks/T-007-ota-container.md) | DONE | P0 | OTA-01 서명 컨테이너와 packager | T-001 |

PR [#36](https://github.com/digitie/canview/pull/36)은
`c60641f218ca5a9966781cc1e8d417df26bb2712`로 origin/main에 merge됐다.
최종 HEAD e1df406의 CI35427174458은6/6, target ELF/MAP/BIN21개 bytes/SHA256·
source7개 일치, target logs28개 warning/error0이다. Manifest SHA256:
`dc2d83a51dfb76dddb0b389ebdd9d5fffa1714092e6a91163d95d85c2a69966b`.
Host Debug/Release150/150, 최신 소프트웨어 candidate ASan/UBSan139/139와 C 변이5개,
strict docs 및 [두 독립 전체 감사](reviews/adversarial/2026-09-19-T-007-final-acceptance.md)를 확인했다.
원 CONDITIONAL의 남은 CI/산출물 조건을 위 근거로 닫았으며 원문을 PASS로 바꾸지 않았다.
실제 Flash writer·영속 정책·physical/HIL·총 자원 실측은 후속 owner gate로 남고,
차량 TX는 NO-GO다. 제품 전체 OTA 설치 완료를 뜻하지 않는다.

## 2026-09-08 T-500

| ID | 상태 | 우선순위 | 작업 | 선행 |
|---|---|---:|---|---|
| [T-500](tasks/T-500-bench-hil-harness.md) | DONE | P0 | protocol/CAN fault bench와 HIL harness | T-001, T-003, T-004 |

PR [#30](https://github.com/digitie/canview/pull/30)은 merge commit
`8f5d97ff924fe7fdb757a3a86a30cde5a80c2a09`으로 `origin/main`에 통합됐다.
최종 CI `34235313714`의 6개 job과 target firmware build, focused unit
46/46, 전체 Python 97/97, host inventory 12/12, evidence validator,
sanitizer 및 문서·계획 검사를 통과했다. A-10/B-10 독립 적대적 reviewer는
모두 `PASS`이고 unresolved P0/P1/P2/P3는 0건이다. [통합 review report](reviews/adversarial/2026-09-08-T-500.md)와
[reviewer raw evidence A](reviews/adversarial/evidence/2026-09-08-T-500-reviewer-a.md),
[reviewer raw evidence B](reviews/adversarial/evidence/2026-09-08-T-500-reviewer-b.md)를 보존한다.
실제 rig는 `SKIPPED`, physical/HIL·flash·전원/reset·CAN analyzer·차량 bus·
provisioning은 `NOT_RUN`이며 차량 CAN TX는 `NO-GO`다.

## 2026-09-07 T-400a

| ID | 상태 | 우선순위 | 작업 | 선행 |
|---|---|---:|---|---|
| [T-400a](tasks/T-400a-bridge-core-bench.md) | DONE | P0 | Bridge 최소 core와 ESP 공용화 | T-001, T-200a |

PR [#23](https://github.com/digitie/canview/pull/23)은 `25eba080907257c6d90abaeec6d578d9dff6585a`로 main에 merge됐다. final CI `34114919104`의 다섯 job, target artifact manifest 18/18 SHA-256 대조, warning 0, host·target·coverage gate와 A-05/B-07 독립 review의 unresolved P0/P1 없음이 완료 조건을 충족했다. P2 세 건은 owner=T-400, gate=G1, 목표=2026-09-14로 defer했다. flash/HIL·전원/reset·장시간 watchdog/PSRAM·차량 CAN·provisioning·vehicle TX release는 `NOT_RUN`이며, T-400a 완료는 차량 송신 승인이 아니다.

## 2026-09-07 T-200a

| ID | 상태 | 우선순위 | 작업 | 선행 |
|---|---|---:|---|---|
| [T-200a](tasks/T-200a-esp32-core-bench.md) | DONE | P0 | Communicator ESP32 최소 core와 host 검증 | T-001, T-004 |

PR [#22](https://github.com/digitie/canview/pull/22)은 `2222290`으로 merge됐다. host Debug/Release·ASan89/89, core/SDK/app coverage·strict API, 2인 독립 리뷰 네 finding FIXED, 최종 head6종 binary warning0·CI5개와 원격 artifact18개 digest 대조를 완료했다. [merge evidence](reviews/adversarial/evidence/2026-09-07-T-200a-merge.md)에 실물/HIL NOT_RUN을 구분한다. 부모 T-200은 BLOCKED다.

## 2026-09-07 T-102a

| ID | 상태 | 우선순위 | 작업 | 선행 |
|---|---|---:|---|---|
| [T-102a](tasks/T-102a-stm32-core-bench.md) | DONE | P0 | STM32 최소 boot/fault 기반과 host 검증 | T-001 |

PR [#21](https://github.com/digitie/canview/pull/21)은 `db5ed19`로 main에 merge됐다. 전체 host Debug/Release/ASan 각각74/74, core coverage·변이·2인 리뷰·최종 head의 STM32/ESP32 clean6종 binary warning0·CI5개를 확인했다. [최종 evidence](reviews/adversarial/evidence/2026-09-07-T-102a-merge.md)에 물리 G1/G2·HIL NOT_RUN을 구분한다. T-102 전체는 BLOCKED다.

## 2026-09-07 T-004

| ID | 상태 | 우선순위 | 작업 | 선행 |
|---|---|---:|---|---|
| [T-004](tasks/T-004-uart-schema-codec.md) | DONE | P0 | Communicator UART v1.0 schema와 codec | T-001, T-002 |

PR [#20](https://github.com/digitie/canview/pull/20)은 `caafc24`로 main에 merge됐다. CTest Debug/Release/ASan 각각68/68, 실제 C parser69.12GB duplex soak, STM32/ESP32 전체 clean binary warning0, 2인 적대적 리뷰와 최신 원격 CI10건을 통과했다. [최종 merge evidence](reviews/adversarial/evidence/2026-09-07-T-004-merge.md)와 physical/HIL NOT_RUN 경계를 유지한다.

## 2026-09-06

| ID | 상태 | 우선순위 | 작업 | 선행 |
|---|---|---:|---|---|
| [T-001](tasks/T-001-host-toolchain-ci.md) | DONE | P0 | 재현 가능한 host toolchain과 CI | 없음 |

T-001은 acceptance·evidence·2인 적대적 리뷰·원격 target CI 통과 후 PR #17이 `74d43ff`로 main에 merge됐다. 실제 보드 flash·HIL·차량 CAN TX·production OTA는 별도 gate로 남겼다.

## 2026-09-07

| ID | 상태 | 우선순위 | 작업 | 선행 |
|---|---|---:|---|---|
| [T-002](tasks/T-002-espnow-schema-v1.3.md) | DONE | P0 | ESP-NOW v1.3 schema와 생성 header 동결 | T-001 |

T-002는 acceptance·host/target CI·2인 적대적 리뷰를 통과한 뒤 PR #18이 merge commit `c18a8a5`로 `main`에 통합됐다. 생성 header/golden/malformed/compatibility 계약은 유지하며, 보드 flash·RF/HIL·실차·production OTA signing은 별도 NOT_RUN gate다.

| [T-003](tasks/T-003-espnow-codec-session.md) | DONE | P0 | ESP-NOW codec, parser, session과 QoS | T-002 |

T-003은 generated TLV policy·byte-safe codec·session lifecycle/anti-replay·pairing/control adapter·bounded QoS와 C/Python fault 시험, STM32/ESP32 clean target build를 통과했다. PR #19 merge 뒤에도 board/RF/CCMP/CAN-HIL/차량/production security gate는 별도로 유지한다.
