# CANView 적대적 설계 리뷰와 조치 기록

## 1. 범위와 방법

2026-09-04 기준 두 독립 리뷰어가 `origin/main`의 merge commit `50b5e733ea469e37bbb762876386625ef8b3b11f`만 읽기 전용으로 검토했다. 작업 중 작성된 [구현 준비 기준](implementation-readiness.md)과 [task 명세](tasks.md)는 보지 못하게 분리해 기존 설계가 스스로 닫혀 있는지 확인했다.

- 리뷰어 A: 전원, brownout/reset, 핀맵, CAN PHY, TX safety, ESP-NOW/UART, 권한·재전송·Diagnostic Bridge를 공격했다.
- 리뷰어 B: build 가능성, 모듈/API 경계, LVGL 수명주기, DBC 생성, storage/web, CI·HIL·resource budget을 공격했다.

심각도는 다음 의미다.

| 등급 | 의미 |
|---|---|
| `P0` | 차량 연결·송신 또는 구현 착수를 즉시 차단하는 결함 |
| `P1` | 통합 전에 설계와 자동시험을 반드시 닫아야 하는 결함 |
| `P2` | 기능 구현 또는 검증 단계 전에 해소할 중요 결함 |
| `P3` | 모순·유지보수·API 수명 문제지만 상위 gate 뒤 처리 가능 |

## 2. 최종 판정

전체 firmware와 차량 송신은 **NO-GO**다. 지금 시작 가능한 것은 재현 가능한 host 환경, machine-readable schema/generator, 회로도와 독립 TX gate 설계, synthetic parser/decoder hardening뿐이다.

| 범위 | 판정 | 조건 |
|---|---|---|
| T-001 host/CI, T-100 schematic | `GO` | 실제 차량·secret 없이 진행 |
| protocol/profile schema와 host codec | `GO` | G0 완료 전 target integration 금지 |
| Controller/Bridge offline UI·synthetic capture | `조건부 GO` | real transport·차량 값으로 오인하지 않음 |
| bench `CAPTURE_ONLY` end-to-end | `NO-GO` | G0·G1·G2와 analyzer TX 0 증거 필요 |
| 차량 listen-only capture | `NO-GO` | bench read-only와 bus mapping 검증 필요 |
| Candidate 신호 기반 운전자 정상 표시 | `NO-GO` | `VALID + VERIFIED` 전까지 Signal Lab 전용 |
| audio/SPORT/CAN TX | `절대 NO-GO` | hard gate, verified profile, HIL, G4/G5 필요 |
| 양산/OEM-grade 표기 | `범위 밖` | AEC/PPAP·법규·EMC는 별도 program 필요 |

## 3. 리뷰어 A 결과와 반영

아래 표의 상태 `반영`은 코드가 완성됐다는 뜻이 아니라 정본 설계와 실행 task에 결함·결정·수용시험이 들어갔다는 뜻이다.

| ID | 결함과 실패 형태 | 확정 보완 | 연결 task | 상태 |
|---|---|---|---|---|
| A-P0-1 | MCU 정지 뒤 GPIO가 Hi-Z가 된다는 잘못된 전제로 valid frame을 계속 송신할 수 있음 | tri-state TXD gate, output pull-up, 물리 TX_ARM, rail-good, 외부 100 ms guardian과 gate sense; CAPTURE_ONLY에는 TX symbol 없음 | [T-100](tasks/T-100-communicator-schematic.md), [T-101](tasks/T-101-hardware-bringup.md), [T-106](tasks/T-106-stm32-command-executor.md) | 반영 |
| A-P1-1 | MAX20040 EN·PGOOD/BIAS·parked state 미정으로 부팅 실패 또는 배터리 방전 | ACC/KL15 기반 OFF/STARTING/RUN_RX/RUN_TX_ARMED/SHUTDOWN, 1 mA parked 목표와 72시간 시험 | [T-100](tasks/T-100-communicator-schematic.md), [T-101](tasks/T-101-hardware-bringup.md) | 반영 |
| A-P1-2 | USB/SWD와 차량 rail 역순에서 phantom power·역급전 가능 | native USB data-only, SWD VTref output-only, 별도 bench 12 V와 전원 순서 matrix | [T-100](tasks/T-100-communicator-schematic.md), [T-101](tasks/T-101-hardware-bringup.md) | 반영 |
| A-P1-3 | brownout에서 3.3 V reset과 MAX3055 BATT/VCC 영역이 다르게 움직여 CAN3가 남을 수 있음 | PROTECTED_VBAT UV+rail-good으로 MAX EN/TXD permit hardware 차단, crank/RF worst-case 계산 | [T-100](tasks/T-100-communicator-schematic.md), [T-101](tasks/T-101-hardware-bringup.md) | 반영 |
| A-P1-4 | CAN3 ERR가 선택사항이면 fault를 모른 채 송신 가능 | CAN3 실장/TX variant에서 PB14 ERR와 fault inhibit를 mandatory | [T-100](tasks/T-100-communicator-schematic.md), [T-101](tasks/T-101-hardware-bringup.md) | 반영 |
| A-P1-5 | protocol 1.2는 message enum만 있고 role·payload·error ABI가 없음 | 첫 구현을 1.3으로 고정하고 전체 schema/C·Python codec/golden vector 생성 | [T-002](tasks/T-002-espnow-schema-v1.3.md), [T-003](tasks/T-003-espnow-codec-session.md), [T-004](tasks/T-004-uart-schema-codec.md) | 반영 |
| A-P1-6 | installation-wide secret 유출 시 Bridge가 Primary 자격을 파생할 수 있음 | device-local PMK, pair별 link root/LMK, role/scope transcript binding과 pair별 rotation | [T-002](tasks/T-002-espnow-schema-v1.3.md), [T-003](tasks/T-003-espnow-codec-session.md), [T-201](tasks/T-201-communicator-espnow.md) | 반영 |
| A-P1-7 | ESP에서 끝난 인증 신원이 STM32까지 전달되지 않아 UART 위조가 최종 TX를 열 수 있음 | Primary Controller↔STM 전용 control root/tag, opaque envelope, STM 검증과 tagged terminal result | [T-002](tasks/T-002-espnow-schema-v1.3.md), [T-104](tasks/T-104-stm32-uart-control.md), [T-106](tasks/T-106-stm32-command-executor.md) | 반영 |
| A-P1-8 | Controller·ESP·STM clock 사이 TTL/age 변환이 없음 | boot-bound end-to-end CONTROL_TIME_SYNC, 10초 갱신·30초 만료·50 ms uncertainty 상한 | [T-003](tasks/T-003-espnow-codec-session.md), [T-104](tasks/T-104-stm32-uart-control.md), [T-202](tasks/T-202-communicator-uart-router.md) | 반영 |
| A-P1-9 | ACK/retry/cache/anti-replay가 token과 live TTL을 일관되게 보존하지 않음 | ACK/RESULT token, origin/session/generation cache key, live entry 무축출과 pre-ACK BUSY | [T-002](tasks/T-002-espnow-schema-v1.3.md), [T-003](tasks/T-003-espnow-codec-session.md), [T-104](tasks/T-104-stm32-uart-control.md) | 반영 |
| A-P1-10 | sender precondition bitmask가 필수 safety 조건을 생략할 수 있음 | STM generated immutable required mask; admission/dequeue/각 TX 직전 local 재검사 | [T-105](tasks/T-105-stm32-safety-profile.md), [T-106](tasks/T-106-stm32-command-executor.md) | 반영 |
| A-P1-11 | filter batch 중 일부만 적용되면 revision과 허용범위가 갈라짐 | expected revision, staging 전체 검증, 단일 pointer/revision commit | [T-203](tasks/T-203-peer-subscriptions.md), [T-301](tasks/T-301-controller-can-pipeline.md) | 반영 |
| A-P1-12 | audio profile 도중 reset/bus-off가 나면 부분 적용을 성공으로 오인 | durable OEM snapshot, bounded steps, feedback마다 진행, rollback과 네 terminal 상태 | [T-106](tasks/T-106-stm32-command-executor.md), [T-503](tasks/T-503-audio-command-validation.md) | 반영 |
| A-P1-13 | 큰 `elapsed_ms` 한 번과 pending 중 stale 변화가 SPORT 전환을 확정 | distinct fresh sample, 250 ms gap reset, safety 우선, pending 직렬화 | [T-005](tasks/T-005-canonical-model.md), [T-505](tasks/T-505-auto-sport-validation.md) | 반영 |
| A-P1-14 | 인증된 Bridge flood와 HTTP bulk가 control radio/CPU를 고갈 | 설치 전체 scheduler, P0/P1·Primary reserve, moving/lease 중 HTTP pause | [T-203](tasks/T-203-peer-subscriptions.md), [T-400](tasks/T-400-diagnostic-bridge-bootstrap.md), [T-402](tasks/T-402-diagnostic-api-web.md) | 반영 |
| A-P2-1 | decoder가 지연 record도 `age_ms=0`으로 생성 | source time·boot/catalog revision을 API에 넣고 saturating age 계산 | [T-005](tasks/T-005-canonical-model.md), [T-301](tasks/T-301-controller-can-pipeline.md) | 반영 |
| A-P2-2 | 32/64-bit DBC 계약, NaN/Inf·overflow 처리가 모순 | operational 32-bit 상한, 64-bit diagnostic-only, finite/range/enum validator | [T-006](tasks/T-006-vehicle-profile-generator.md), [T-403](tasks/T-403-signal-lab-evidence.md) | 반영 |
| A-P2-3 | RTR/error/TX echo가 일반 DATA signal freshness를 갱신 | operational 기본 RX DATA-only, 특수 frame 명시 opt-in | [T-006](tasks/T-006-vehicle-profile-generator.md), [T-103](tasks/T-103-stm32-fdcan-capture.md), [T-301](tasks/T-301-controller-can-pipeline.md) | 반영 |
| A-P2-4 | filter fragment에 snapshot 식별자가 없어 hybrid snapshot 가능 | snapshot ID/revision/part index/count/total count와 all-parts commit | [T-002](tasks/T-002-espnow-schema-v1.3.md), [T-203](tasks/T-203-peer-subscriptions.md) | 반영 |
| A-P2-5 | 20/40 kB peer budget과 20/32 kB aggregate budget이 충돌 | 설치 전체 20 kB/s, 32 kB burst로 단일화; non-borrowable reserve | [T-203](tasks/T-203-peer-subscriptions.md), [T-500](tasks/T-500-bench-hil-harness.md) | 반영 |
| A-P2-6 | 8 MB Flash에서 문서상 10분 capture를 보장할 수 없음 | byte preflight, 256 KiB reserve, capture 1.5 MiB, no-SD 200 fps 180초 | [T-401](tasks/T-401-capture-cvtrace.md) | 반영 |
| A-P2-7 | ZIP import가 traversal, symlink, bomb, 변조에 취약 | entry allow-list/size/ratio, streaming validation, 별도 signed evidence approval | [T-401](tasks/T-401-capture-cvtrace.md), [T-403](tasks/T-403-signal-lab-evidence.md) | 반영 |
| A-P2-8 | read 권한으로 marker를 만들어 증거를 오염 가능 | marker 단일 message, D1 capture:write, lease/idempotency/rate limit | [T-401](tasks/T-401-capture-cvtrace.md), [T-402](tasks/T-402-diagnostic-api-web.md) | 반영 |
| A-P2-9 | SPORT setter가 불법값을 60/70/80으로 조용히 clamp | wire 600/700/800 exact enum, 오류·state 불변 | [T-305](tasks/T-305-controller-command-orchestration.md), [T-505](tasks/T-505-auto-sport-validation.md) | 반영 |
| A-P2-10 | 현재 BOM으로 양산 적합성을 주장할 수 없음 | 현 범위는 engineering prototype으로 명시; 양산은 별도 AEC/PPAP/법규 program | [T-506](tasks/T-506-release-qualification.md) | 범위 제한 |
| A-P2-11 | 코드에 MONITOR_ONLY 상태가 없어 enable이 action으로 이어질 수 있음 | enable/arm 분리, reboot MONITOR_ONLY, explicit G5 arm | [T-505](tasks/T-505-auto-sport-validation.md) | 반영 |
| A-P3-1 | SPORT feedback timeout이 1.0/1.5초로 충돌 | first physical TX-complete 기준 1,500 ms generated 상수 | [T-106](tasks/T-106-stm32-command-executor.md), [T-505](tasks/T-505-auto-sport-validation.md) | 반영 |
| A-P3-2 | RTC가 Controller-local이면서 vehicle command에도 존재 | v1.3에서 RTC command 제거, Controller local/confirmed direct config만 허용 | [T-002](tasks/T-002-espnow-schema-v1.3.md), [T-304](tasks/T-304-controller-local-config.md) | 반영 |
| A-P3-3 | 일부 wire struct만 size/offset assertion이 있음 | 모든 schema type의 size/offset/known-mask와 direct-cast 금지 검사 | [T-002](tasks/T-002-espnow-schema-v1.3.md), [T-003](tasks/T-003-espnow-codec-session.md) | 반영 |

## 4. 리뷰어 B 결과와 반영

| ID | 결함과 실패 형태 | 확정 보완 | 연결 task | 상태 |
|---|---|---|---|---|
| B-P0-01 | 세 target의 실행 가능한 end-to-end image가 없음 | Controller·Comm ESP·Bridge IDF bootstrap과 STM platform/listen-only path | [T-102](tasks/T-102-stm32-platform.md), [T-200](tasks/T-200-communicator-esp32-bootstrap.md), [T-300](tasks/T-300-controller-bootstrap.md), [T-400](tasks/T-400-diagnostic-bridge-bootstrap.md) | 반영 |
| B-P0-02 | ESP-NOW/UART ABI가 목록보다 불완전 | 전체 schema와 generated codec/golden vector를 G0 선행 | [T-002](tasks/T-002-espnow-schema-v1.3.md), [T-004](tasks/T-004-uart-schema-codec.md) | 반영 |
| B-P0-03 | 현 SPORT monitor가 실제 ENTER action을 만듦 | 기존 C는 target에서 격리하고 MONITOR_ONLY/TX_ALLOWED 분리 | [T-505](tasks/T-505-auto-sport-validation.md) | 반영 |
| B-P0-04 | 독립 TX 차단기가 없음 | A-P0-1과 같은 external gate/fault test | [T-100](tasks/T-100-communicator-schematic.md), [T-101](tasks/T-101-hardware-bringup.md) | 반영 |
| B-P0-05 | Candidate 신호인데 control provenance를 받지 않음 | profile/evidence generator가 VERIFIED만 STM command artifact로 생성 | [T-006](tasks/T-006-vehicle-profile-generator.md), [T-502](tasks/T-502-readonly-signal-validation.md) | 반영 |
| B-P1-01 | local filter와 upstream subscription owner/revision 충돌 | 두 store/type/revision 및 원자 transaction 분리 | [T-203](tasks/T-203-peer-subscriptions.md), [T-301](tasks/T-301-controller-can-pipeline.md) | 반영 |
| B-P1-02 | empty default-deny에서 pairing이 speed/lease에 의존하는 순환 | USB 우선, gate-off CAPTURE_ONLY read-only service bootstrap | [T-201](tasks/T-201-communicator-espnow.md) | 반영 |
| B-P1-03 | LVGL model에 freshness/evidence가 없음 | canonical metadata와 `VALID + VERIFIED` 표시 규칙 | [T-005](tasks/T-005-canonical-model.md), [T-302](tasks/T-302-controller-ui-model.md) | 반영 |
| B-P1-04 | UI가 capability/pending/result를 구분하지 않음 | visible/enabled/pending/applied/reason/token model과 matching-result commit | [T-302](tasks/T-302-controller-ui-model.md), [T-305](tasks/T-305-controller-command-orchestration.md) | 반영 |
| B-P1-05 | hop별 TTL과 boot epoch가 불명확 | end-to-end clock mapping과 immutable issued time/TTL | [T-003](tasks/T-003-espnow-codec-session.md), [T-104](tasks/T-104-stm32-uart-control.md), [T-202](tasks/T-202-communicator-uart-router.md) | 반영 |
| B-P1-06 | config key 513/514, km/h/0.1 km/h, RTC owner 충돌 | generated registry, SPORT key `0x0202`, 600/700/800 adapter, RTC local-only | [T-002](tasks/T-002-espnow-schema-v1.3.md), [T-304](tasks/T-304-controller-local-config.md), [T-305](tasks/T-305-controller-command-orchestration.md) | 반영 |
| B-P1-07 | stale·큰 elapsed·pending 거부가 automation state를 오염 | freshness ceilings, 250 ms gap reset, requested/confirmed 분리 | [T-005](tasks/T-005-canonical-model.md), [T-305](tasks/T-305-controller-command-orchestration.md), [T-504](tasks/T-504-adaptive-volume-release.md), [T-505](tasks/T-505-auto-sport-validation.md) | 반영 |
| B-P1-08 | 공유 pairing secret이 역할을 격리하지 못함 | A-P1-6의 pair별 root model | [T-002](tasks/T-002-espnow-schema-v1.3.md), [T-201](tasks/T-201-communicator-espnow.md) | 반영 |
| B-P1-09 | Diagnostic API read/write와 marker 경로가 모순 | OpenAPI 권한 guard, marker 단일 경로, challenge expiry/replay test | [T-402](tasks/T-402-diagnostic-api-web.md) | 반영 |
| B-P1-10 | 단일 host test가 통합·안전을 보장하지 않음 | root CI, sanitizer/fuzz/golden/HIL/analyzer와 mutation fixture | [T-001](tasks/T-001-host-toolchain-ci.md), [T-500](tasks/T-500-bench-hil-harness.md) | 반영 |
| B-P2-01 | `.cvtrace` format과 partial 복구가 미정 | CVJB journal, CVFRAME1 records, finalize-last ZIP, partial/GAP policy | [T-401](tasks/T-401-capture-cvtrace.md) | 반영 |
| B-P2-02 | resource budget이 없거나 문서끼리 충돌 | machine-readable budget manifest와 map/stack/runtime checker | [T-001](tasks/T-001-host-toolchain-ci.md), [T-500](tasks/T-500-bench-hil-harness.md) | 반영 |
| B-P2-03 | public include가 private protocol path에 의존하고 dependency가 움직임 | public IDF protocol component, `REQUIRES`, commit/hash/lock 고정 | [T-001](tasks/T-001-host-toolchain-ci.md), [T-300](tasks/T-300-controller-bootstrap.md) | 반영 |
| B-P2-04 | reason/revision/counter 폭이 불일치 | reason u16, revision u32, cumulative saturating u64, window count bounded u32 | [T-002](tasks/T-002-espnow-schema-v1.3.md), [T-005](tasks/T-005-canonical-model.md) | 반영 |
| B-P3-01 | UI callback이 stack-local command pointer를 전달 | borrowed lifetime 명시와 queue의 즉시 by-value copy | [T-302](tasks/T-302-controller-ui-model.md) | 반영 |
| B-P3-02 | global LVGL singleton에 destroy/duplicate 정책 없음 | `canview_ui_t` instance create/destroy와 반복 leak test | [T-302](tasks/T-302-controller-ui-model.md) | 반영 |

## 5. 리뷰 뒤 확정한 핵심 불변식

1. `CAPTURE_ONLY`는 compile/link 단계에서 CAN TX API가 없어야 한다.
2. hard gate off에서는 MCU가 어떤 TXD waveform을 만들어도 CANH/L에 ACK, error, data, dominant bit가 나오지 않아야 한다.
3. Candidate/Observed 신호는 운전자 정상값이나 command precondition이 될 수 없다.
4. Controller와 STM32 사이 control tag가 없는 lease·명령·terminal result는 차량 상태를 바꾸지 못한다.
5. retry는 sequence만 바꾸며 token, issued time, TTL, argument와 end-to-end tag를 바꾸지 않는다.
6. ACK는 queue 수락이고 성공은 tagged terminal result와 matching vehicle feedback 뒤에만 성립한다.
7. filter/config/audio transaction은 old 또는 new 전체 상태만 보이며 silent partial success가 없다.
8. stale·clock uncertainty·boot/session 변화는 값을 fresh로 보정하지 않고 command를 fail-closed한다.
9. Bridge는 raw replay와 vehicle control API를 build에 포함하지 않는다.
10. 차량 TX release는 기능별 G4/G5 증거 없이는 만들 수 없다.

## 6. 잔여 위험과 다음 검토점

- exact TX guardian logic IC, UV comparator, FET/TVS와 harness pin은 schematic/ERC/SI 계산 뒤에만 확정된다.
- external watchdog은 CPU stall을 자르지만 정상 cadence로 watchdog을 갱신하는 논리 오동작의 valid-frame flood를 의미 분석하지 못한다. generated executor와 analyzer 시험이 두 번째 경계다.
- 2017 Tucson TL의 bus mapping, bitrate, 4WD/DPF/audio/drive-mode signal과 counter/checksum은 실차 evidence 전부가 아직 Candidate다.
- CAN3가 실제 125 kbps fault-tolerant bus인지 확인 전 MAX3055는 enable하지 않는다.
- AEC/PPAP·양산 EMC/법규는 사용자가 제외한 범위이므로 이 설계의 G6도 engineering prototype qualification이다.

구현 순서, 파일, 수용 기준과 검증 명령은 [작업 인덱스](tasks.md)를 정본으로 사용한다.
