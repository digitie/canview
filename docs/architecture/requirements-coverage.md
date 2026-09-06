# CANView 요구사항 coverage와 두 차례 계획 감사

## 1. 범위와 판정

비교 기준: `4aeb2912da063c6fcb0d8715aa46f84c7d1d1b0f`, 2026-09-06. architecture 규범(OTA §14 리뷰 역사는 제외), 기존 상세 task 35개와 hardware/vehicle/UI 설계를 1차 통독해 실행 책임과 gate를 연결했다. 이 문서는 요구사항의 새 정본이 아니라 추적표다. 정본이 바뀌면 해당 행과 상세 task를 같이 대조한다.

**제품 전체 완료가 아니다. 차량 CAN TX는 NO-GO이며 이번 변경은 계획 보완과 제한된 UI/host 수정이다.** 코드가 존재하거나 정적 prototype/회로/ERC/host helper가 있다는 사실을 target·실차·OTA 완료로 표시하지 않는다. 1차 전체 계획 읽기와 main의 2차 요구/task/정본 대조는 §5–6에 구분하고 최종 전문 reviewer 2인의 독립 적대적 리뷰는 별도 실행으로 기록한다.

상태 해석: `설계`는 계약만 존재, `부분 host`는 제한된 코드/fixture 존재, `미구현/미검증`은 필요한 산출물·실행 근거가 없음이다. 아래의 gate는 **남은 조건**이며 체크 완료 목록이 아니다. task의 실제 상태·제목·선행은 [backlog](../tasks.md)와 상세 파일이 정본이다.

## 2. 요구 → 정본 → 실행 task → 검증 상태 → 남은 gate

| 요구 | 정본 | task | 실행/검증 상태 | 남은 gate |
|---|---|---|---|---|
| R-01 세 장치 책임·Controller 의미 명령 제한 | [AGENTS](../../AGENTS.md), [ADR-001](../adr/001-canview-safety-boundary.md), [system §2–3](system.md) | T-002, T-105, T-106, T-201, T-305, T-400 | 설계·부분 host scaffold; end-to-end 미구현 | G0–G5; 어떤 ESP도 raw ID/data TX·Bridge control lease 허용 금지 |
| R-02 Communicator N16R8·STM·3 CAN PHY | [ADR-007](../adr/007-n16r8-independent-recoverable-ota.md), [OTA §2·6](ota.md), [R1](../hardware/r1/README.md) | T-100, T-100a, T-101, T-102, T-103, T-200 | N16R8 sdkconfig/회로 생성물 존재; PCB·target·실측 미완료 | land/DFM·각 bus type·RX/TX gate·power/thermal/SI; 3채널 지원이 차량 bus 연결 근거는 아님 |
| R-03 전원·독립 reset·외부 gate·USB service | [OTA §6·11](ota.md), [R1 검증](../hardware/r1/verification.md), [pinmap](../hardware/r1/firmware-pinmap.md) | T-100, T-100a, T-101, T-508 | 정적 회로/ERC 기록 존재; 아날로그 안전 미입증 | 외부 IGN/ACC·USB/SYS/PHY rail별 실측, J31·U56·stale ARM; OFF 1mA/CAN wake 주장 금지 |
| R-04 재현 가능한 Windows/target/host 도구 | [Windows](../development/windows.md), [toolchains](../development/toolchains.md) | T-001, T-102, T-200, T-300, T-400 | 잠금/환경 도구·일부 host tests 존재; 전체 CI 미완료 | clean configure/build/CTest·sanitizer·target map, 미생성 script/0 test를 pass로 집계 금지 |
| R-05 schema·wire·golden·악성 입력 | [ESP-NOW](protocols/esp-now.md), [UART](protocols/communicator-uart.md), [통합 §7](implementation-readiness.md) | T-002, T-003, T-004 | 기본 v1.3/v1.0 전체 ABI/codec 미완료 | generated drift·malformed·version/capability·원자 admission/ACK; draft SIGNAL_BATCH 비활성 |
| R-06 pairing/session·독립 key·lease | [통합 §7.5–7.6](implementation-readiness.md), [ESP-NOW](protocols/esp-now.md) | T-003, T-104, T-201, T-202, T-305, T-507 | 계약만, 실제 provisioning/rotation·보호 미검증 | pair별 root·device-local PMK·STM HMAC·boot generation; Bridge/Comm ESP에 control_root 없음 |
| R-07 재시도·중복·TTL·시간 동기 | [통합 §6–7](implementation-readiness.md), [CAN pipeline](controller-can-pipeline.md) | T-002, T-003, T-004, T-104, T-202, T-305 | 제한된 tracker/helper 존재; 전체 transport 미구현 | 250ms·remaining TTL/2·immutable tag·live cache 보존·reboot/wrap/reorder·ACK≠완료 |
| R-08 3 CAN capture·원본 시간·local safety 독립 | [UART](protocols/communicator-uart.md), [통합 §5·8–10](implementation-readiness.md) | T-102, T-103, T-104, T-105, T-500 | STM scaffold; DMA/FDCAN 실측 없음 | ISR bounded queue·timestamp·overflow·24h UART·observer 포화에도 safety decode 유지 |
| R-09 범용 raw 구독 CRUD·GET·revision | [CAN pipeline §3](controller-can-pipeline.md), [ESP-NOW filters](protocols/esp-now.md) | T-002, T-003, T-203, T-301, T-402 | local filter helper만 일부 존재; upstream 통합 없음 | ADD/REPLACE/DELETE/CLEAR/GET·존재/미존재·invalid batch·동시 revision·snapshot fragment 원자성 |
| R-10 횟수·주기·byte/burst·peer 격리 | [통합 §8](implementation-readiness.md), [ESP-NOW](protocols/esp-now.md) | T-203, T-301, T-401, T-100b | 계약만; 실제 radio/capture scheduler 미구현 | filter 20ms–60s·1–32 record/period, aggregate 20kB/s, P0/P1·Primary reserve와 sensor 합산 |
| R-11 quality/evidence/time·derived 값 | [통합 §6·9·12](implementation-readiness.md), [신호 목록](../vehicle/signal-catalog.md) | T-005, T-006, T-301, T-302, T-403, T-502 | helper·UI scaffold; 실차 evidence 없음 | candidate/observed/estimated 정상 숫자 금지·최저 dependency grade·stale/unknown 처리 |
| R-12 decoder 수치 안전·실사용 freshness | [CAN pipeline](controller-can-pipeline.md), [통합 §9](implementation-readiness.md) | T-006, T-301, T-502 | prototype descriptor NaN/Inf/변환 범위·age=0 경계 미검증 | NaN/±Inf·signed/u32 limits·overflow·range invert·timestamp/boot/evidence adapter negative fixture; helper 존재≠완료 |
| R-13 2017 Tucson 실차 bus·DBC 근거 | [대상 차량](../vehicle/target-2017-tucson.md), [DBC](../../dbc/README.md) | T-006, T-501, T-502, T-503, T-505a | upstream DBC와 후보 목록만; 실제 capture approval 없음 | G2 후 CAPTURE_ONLY G3, connector/bitrate/ignition 반복·license/digest·서명된 approval |
| R-14 4WD 앞뒤·네 바퀴·TPMS | [features §4](features.md), [UI](../ui/design.md), [신호 목록](../vehicle/signal-catalog.md) | T-005, T-006, T-302, T-502 | prototype 시각/후보 신호; 실차 VERIFIED 아님 | 휠/axle 의미·position/unit·stale·미지원, coupling을 torque 분배 정상값으로 추정 금지 |
| R-15 순간연비·평균연비 제외 | [features](features.md), [UI](../ui/design.md), [신호 목록](../vehicle/signal-catalog.md) | T-005, T-301, T-302, T-502 | 부분 UI/계산 helper; 입력 근거 미검증 | 연료량/유량/거리 시간·단위·zero speed/zero fuel·sentinel·scale; 평균연비 잔재 제거 |
| R-16 DPF lamp·부하·재생 상태 | [features §5](features.md), [신호 목록](../vehicle/signal-catalog.md) | T-006, T-302, T-502 | DBC 후보; lamp OFF 외 상태를 확정할 근거 없음 | lamp off≠전체 정상/재생 완료·부하율; 각 상태 별도 evidence·미지원 표시 |
| R-17 배터리 전압·변속기 lock/온도·엔진 온도 | [features](features.md), [UI](../ui/design.md), [신호 목록](../vehicle/signal-catalog.md) | T-005, T-006, T-302, T-502 | 계획 보완; 실제 신호 근거 없음 | 전압 기준점/단위, transmission temp≠engine temp, lock≠RPM 추정; unknown/stale 구분 |
| R-18 주 화면·모든 UI 디테일·능력별 상태 | [UI](../ui/design.md), [LVGL 검토](../ui/lvgl-demo-review.md), [통합 §12](implementation-readiness.md) | T-300, T-302, T-402 | 기준선 UI prototype; main 병렬 개선은 별도 검증 | 320×480 전체 상태·긴 한글·scroll/dropdown·pressed/disabled/pending/fault·safe area·60Hz/frame/8h soak |
| R-19 UI model/API lifetime·단일 LVGL owner | [통합 §5·12](implementation-readiness.md), [UI](../ui/design.md) | T-005, T-301, T-302, T-305 | 명시 context/adapter 완료 전; 기존 helper 전체 완료 주장 금지 | create/destroy·중복 create·callback by-value copy·publish/consume/release·느린 consumer·torn snapshot·250ms touch |
| R-20 CAN-only 밝기·미등 야간·30초 idle | [automation](automation.md), [UI](../ui/design.md), [통합 §12–13](implementation-readiness.md) | T-300, T-302, T-304 | 순수 C 일부 시험 가능; actual RTC/backlight 미통합 | 미등 debounce·CAN dimmer stale·idle/touch/화면복귀·Flash write debounce; GPS/조도 기본값으로 대체 금지 |
| R-21 과속 overlay·경고 우선순위 | [UI](../ui/design.md), [automation](automation.md) | T-302, T-304 | prototype·host 상태 일부; target 미측정 | main 큰 overlay/다른 화면 반투명 touch-through·headlamp보다 우선·종료 후 idle/night 밝기 복원 |
| R-22 RTC·일몰·전조등 미점등 경고 | [automation](automation.md), [통합 §13](implementation-readiness.md), [GPS 시간 조사](../vehicle/gps-time-investigation.md) | T-005, T-300, T-304, T-100b | 설계/host helper; 실물 RTC/solar source 미완료 | BCD/oscillator-stop·날짜/timezone/source·일몰/일출·백야/극야·stale light·config/RTC 비원자 복구; 차량 등화 TX 금지 |
| R-23 FFT·peak/level·차속/RPM 동시 표시 | [features](features.md), [통합 §12](implementation-readiness.md), [UI](../ui/design.md) | T-005, T-302, T-303, T-100b | 시각 prototype; target I2S/DSP/calibration 없음 | signed dBFS·valid/clipped/calibrated·source별 16/32kHz·차속/RPM 동시 상태·buffer 수명/drop |
| R-24 자연스러운 volume·OEM override·SDVC | [automation](automation.md), [features](features.md), [통합 §12.1](implementation-readiness.md) | T-303, T-305, T-503, T-503a, T-504 | 순수 C 동작 일부; 실제 OEM 제어/음향 근거 없음 | calibration 없는 speed-only 증폭 금지·step/feedback·silent/소스/call/reverse·수동 우선·청취 평가 |
| R-25 audio 한정 의미 프로필·snapshot restore | [통합 §10·12](implementation-readiness.md), [features](features.md) | T-503, T-105, T-106, T-305, T-503a | source/feedback 미확인, 실제 TX 금지 | QUIET/REAR_BOOST·허용 scope 전체·snapshot TTL/revision·부분 복원 결과; CENTER/고정 중앙 복원 금지 |
| R-26 SPORT 진입·이전 mode 복귀 | [automation](automation.md), [통합 §10–12](implementation-readiness.md) | T-505a, T-105, T-106, T-305, T-505 | host state machine 일부; 실제 mode/button/safety 미확인 | NORMAL 고정 복귀 아님·관측한 mode만·feedback/운전자 override·threshold/dwell/tick gap·G4/G5·최종 CONFIG_RESULT |
| R-27 GNSS/INS·기압·remote mic·sensor 구독 | [navigation](protocols/navigation.md), [sensor 회로](../hardware/r1/navigation-hardware.md), [mic 회로](../hardware/r1/bridge-controller-microphone.md) | T-002, T-004, T-100b, T-203, T-303, T-304 | 회로/companion schema·host codec fixture 존재; 실물 SI/INS calibration 없음 | 1.4/1.1 협상·sensor budget·P0/calibration owner/persistence·원음/위치 비공개 |
| R-28 휴대폰 Diagnostic Bridge·인증·운행 정책 | [Bridge §5·14](diagnostic-bridge.md), [통합 §14](implementation-readiness.md) | T-400, T-402 | prototype 설계·web shell만; firmware/backend 없음 | 고정 KR channel·memory bearer·1 client·physical service·PII redaction·이동/active lease bulk pause |
| R-29 REST/WS·raw CRUD·설정 확인 | [Bridge §15·18](diagnostic-bridge.md), [통합 §13–14](implementation-readiness.md) | T-304, T-305, T-402 | OpenAPI/actual routes 미구현 | CRUD/GET/operation/cancel 계약·expected_revision·64bit JSON·XSS·WS slow consumer·D2/D3 owner final APPLIED |
| R-30 capture/storage/export/import 수명 | [Bridge](diagnostic-bridge.md), [통합 §14–15](implementation-readiness.md), [OTA §4](ota.md) | T-204, T-401, T-402 | cvtrace/schema/storage backend 없음 | fixed entry allowlist·bounds/digest·torn journal·export pin/refcount·cleanup·capture quota·SD 없는 duration 제한 |
| R-31 Signal Lab candidate·서명 evidence 승인 | [Bridge](diagnostic-bridge.md), [통합 §9·14](implementation-readiness.md) | T-006, T-401, T-403, T-502, T-503, T-505a | 설계와 후보 목록; signed evidence 없음 | CRUD 참조 무결성·offline-only decode·승인 서명·checksum≠출처 인증·checkbox VERIFIED 승격 금지 |
| R-32 소유자 설정·persist·reboot/rollback | [통합 §13](implementation-readiness.md), [OTA §4·5·9](ota.md) | T-201, T-203, T-301, T-304, T-305, T-205 | versioned 저장 계약 보완; 구현/단전시험 없음 | config A/B 권위·NVS cache·RTC intent·단일 writer/endurance·root 별도보호·lease/arm 비영속 |
| R-33 OTA-01 서명 container | [ADR-007](../adr/007-n16r8-independent-recoverable-ota.md), [OTA §7·12](ota.md) | T-007 | 신규 BLOCKED; schema/parser/packager 없음 | 정규 CBOR·signature/role/layout 후 inactive erase·전체 hash 후 PREPARED·정수/길이 악성 fixture |
| R-34 OTA-02 세 ESP 독립 recovery | [OTA §4·12](ota.md) | T-204 | 기존 factory CSV는 OTA 미지원 | normal/recovery 실제 signed size·최초 유선 migration·PSRAM/SD/LCD/normal NVS 없는 AP·provisioning 보존 |
| R-35 OTA-03 G474 보호 bootloader | [OTA §5·12](ota.md) | T-107 | 전체 Flash scaffold; MCUboot port 없음 | DBANK/WRP·header/trailer/map·8B/ECC/NMI·swap/revert·SRAM flash critical code |
| R-36 OTA-04 ESP↔STM boot UART | [OTA §5·8·12](ota.md) | T-108 | recovery codec/transport 없음 | 115200 8N1/CRC32C 별도·readback ACK·중복/offset·mode handoff·J31/J32/reset·ACTIVATE_TEST |
| R-37 OTA-05 각 장치 AP·기본 브라우저 | [OTA §1·3·7–9·12](ota.md) | T-306 | 독립 AP/API/설치 UI 미구현 | Comm는 Controller/Bridge 없이 ESP+STM 갱신·세 역할 offline·고유 WPA2/CSRF·PREPARED≠승인·server 결과 |
| R-38 OTA-06 호환성·승인/floor·REPAIR | [OTA §7.1·8.1·9·12](ota.md) | T-205 | journal/manager·config migration 없음 | 4 MCU 조합·peer old/new·CONFIRM_INTENT·양 copy 손상 LOCKED·same digest repair·config 보존 |
| R-39 OTA-07 제조 보안·유선 복구 | [OTA §7·11–12](ota.md) | T-507 | 정책/샘플 provisioning 미검증 | 비가역 쓰기 사용자 승인·sample·키/보호 readback·label secret 복구 정책·복구불가/교체 한계 |
| R-40 OTA-08 실물 단전·Flash·CAN | [OTA §12](ota.md) | T-508 | HIL script/실물 cut-point evidence 없음 | 각 표 행·결정적 전수+variant별 무작위1000회·J31 GPIO고착·rail collapse·TX0·실제 digest |
| R-41 release·사용자 설치/복구·잔여 위험 | [통합 §16](implementation-readiness.md), [OTA](ota.md), [workflow](../runbooks/agent-workflow.md) | T-100b, T-502, T-503a, T-504, T-505, T-506, T-508 | G0–G6 미완료; 제품 전체 NO-GO | 네 runtime+세 recovery+STM bootloader manifest·실제 gate·72h/thermal/보안·독립 2인 최종 리뷰 |
| R-42 선택/미허용 확장 누락 방지 | [features](features.md), [Bridge](diagnostic-bridge.md), [OTA §3](ota.md), [AGENTS](../../AGENTS.md) | T-402, T-502, T-506 | Controller debug fallback은 선택 경로로만 추적; diagnostic TX/온라인 OTA 후속은 미착수 | OBD/UDS polling·DPF 강제 재생·raw replay·등화 제어·STA OTA를 현 v1 구현으로 포함하지 않음. 채택하려면 명시 요구·ADR/task·gate 추가 |

## 3. 1차 발견과 처리

| 발견 | 처리 | 확인 경계 |
|---|---|---|
| 35개 backlog에 OTA §12 8단계가 없음 | T-007/T-204/T-107/T-108/T-306/T-205/T-507/T-508 추가 | 실제 구현은 전부 BLOCKED; 46개 metadata/DAG 검사와 제품 gate는 별개 |
| 회로→PCB 제작 입력의 명시 owner 누락 | T-100a 추가, T-101은 제작 후 실측 | PCB 부재·land/DFM 미승인·발주 외부 승인 유지 |
| T-106이 T-503 bench를 요구하지만 그 bench는 executor를 필요로 함 | T-503 수신 조사 / T-503a bench·G5 분리 | source evidence 검증이 주입 frame 합격을 대신하지 않음 |
| SPORT source/이전 mode/안전 입력 근거가 executor 앞에 없음 | T-505a 추가, T-106/T-505에 연결 | 차량 미관측 mode·pulse·safety default를 만들지 않음 |
| T-500 순서와 critical path 역전, T-100b 등 summary 선행 불일치 | 상세 선행/summary 동기화; 실제 소비 시험과 공용 rig 제공 분리 | DAG만으로 실물/근거의 의미상 순환이 모두 검출되지는 않음 |
| N4R2·170MHz·shared reset/PGOOD/RX 설명 잔재 | N16R8·R1 160/80MHz·독립 SYS/PHY/reset·RX_ALLOWED 조건으로 task/readiness 정리 | 실제 pin/netlist·clock/전원 실측은 T-100/T-101/T-102/T-508 |
| 미래 script가 존재/통과한 것처럼 읽히는 검증 절 | 각 소비 task의 구현 산출물로 지정; 0-test/NOT_RUN 분리 | T-001 CI/target gate는 이번 validator 생성만으로 DONE 아님 |
| API/queue/snapshot 수명·설정 내구성/복구·raw CRUD 경계 누락 | 각 task acceptance에 원자 admission·release·revision·power-cut·negative fixture 보강 | ABI·저장 규범을 task에서 임의 재설계하지 않음 |
| T-301 prototype의 NaN/Inf·변환 UB·age=0 stamp | 실제 adapter와 reference/sanitizer 회귀 gate 추가 | main의 C4018 cast 수정은 이 기능 구현 완료가 아님 |

## 4. main 2차 교차확인 대상

아래는 1차 읽기 중 발견한 정본 충돌과 당시 인계 내용이다. 최종 대조 결과는 §6에서 확인하며 이 표의 당시 `수정 중/확인 필요`를 현재 구현 완료로 해석하지 않는다.

| 정본 위치 | 1차 충돌/보완 | 현재 disposition |
|---|---|---|
| ADR-006·Windows·toolchains | accepted MINI/공유 reset ↔ 사용자 N16R8/독립 OTA | main ADR-007 추가와 구 부분 superseded·개발환경 수정 보고. ADR-007 본문을 직접 확인해 R-02/33에 연결 |
| system §4, features §3·7·8·11, UI design | 구 power tree·Communicator DBC owner·uncalibrated speed-only volume·추천 멘트·평균연비 | main 직접 수정 중/보고; R-03/15/18/24와 최신 본문을 재대조 |
| features의 4WD/DPF·신호 catalog | coupling 추정 표시와 lamp OFF=정상 등 evidence 과대 해석 | R-11/14/16/17 및 T-502에 금지/승격 조건 추가; 정본 표현 최종 확인 |
| Bridge §5·13·14·19 | ring 1MiB/512KiB·PAIR2/8초 vs3/5초·구 partition·reason 폭·capture allowlist·bearer/cookie | OTA/R1/통합 정본 우선으로 task scope를 맞춤. 원문 동기화·API/버튼/저장 예산 최종 확인 필요 |
| Bridge API/Signal Lab | DELETE/CLEAR/cancel·WS lifetime·candidate 참조·체크박스 VERIFIED 오인 | T-402/T-403 acceptance 추가; route/schema 동결 때 미정 field/HTTP status/owner 확정 |
| automation 전조등/SPORT | OR/AND guard와 ACK 뒤 mirror 갱신 | T-304/T-305 최종 결과·guard negative fixture; main 정본 논리식과 비교 |
| ESP-NOW/CAN pipeline | session 폭·retry250/300ms·live cache 퇴출·draft SIGNAL_BATCH | main 250ms/TTL/2·draft 비활성 정리 보고. T-002/T-003/readiness 필드·원자 ACK 반영, session/폭·tag byte열 최종 확인 |
| readiness §10.2·13 | gate 미존재/active-low 일괄 가정·NVS 권위와 OTA config A/B 충돌 | 본 감사가 생성 회로 존재/실측 미완료·gate별 polarity·권위 snapshot·RTC intent로 수정 |
| solar source·회복 자격증명·선택 fallback | source 우선순위/날짜 유효성·provisioning 양 copy 손상 UX·Controller fallback 범위 | T-304/T-507/T-402의 미결정으로 유지. main이 구현 전 규범 결정 필요 여부 판정; 임의 성공/미지원 은폐 금지 |

## 5. 읽기·검증 범위

CodeGraph project index를 사용하지 못한 환경에서는 `rg`/정본 인덱스/직접 파일 읽기로 추적했다. 신규 하드웨어/SDK 원문 사실을 만들어 확정하지 않았고 기존 문서에 기록된 revision·netlist·gate 계약을 비교했다. raw DBC/capture·과거 review 전체는 감사 규범 입력으로 열지 않았다.

### 직접 통독한 문서

다음 목록은 1차 기준선의 전체 읽기 범위다. 병렬 수정 후 최신판의 2차 통독은 main 담당이다. OTA는 §1–13 규범만 감사 대상으로 삼는다.

- `AGENTS.md`, `docs/README.md`, `docs/resume.md`
- `docs/runbooks/agent-workflow.md`, `docs/runbooks/documentation-maintenance.md`, `docs/tasks-rule.md`, `docs/tasks.md`, `docs/tasks-done.md`, `docs/tasks/README.md`
- `docs/architecture/README.md`, `system.md`, `implementation-readiness.md`, `controller-can-pipeline.md`, `features.md`, `automation.md`, `diagnostic-bridge.md`, `ota.md` §1–13
- `docs/architecture/protocols/README.md`, `esp-now.md`, `communicator-uart.md`, `navigation.md`
- `docs/hardware/controller.md`, `communicator.md`, `docs/hardware/r1/README.md`, `communicator-circuit.md`, `firmware-pinmap.md`, `navigation-hardware.md`, `bridge-controller-microphone.md`, `verification.md`, `pdf-acquisition.md`
- `docs/vehicle/target-2017-tucson.md`, `signal-catalog.md`, `gps-time-investigation.md`, `dbc/README.md`
- `docs/ui/design.md`, `docs/ui/lvgl-demo-review.md`, `docs/development/windows.md`, `docs/development/toolchains.md`
- `docs/adr/README.md`, `001-canview-safety-boundary.md`, `006-compact-hardware-power-and-sensors.md`, main 추가 후 `007-n16r8-independent-recoverable-ota.md`
- `docs/reviews/README.md` (리뷰 절차만), `tools/validate_document_links.py`, `firmware/communicator/esp32/sdkconfig.defaults`, `firmware/communicator/esp32/partitions.csv`

기존 상세 task 35개는 아래 파일을 전부 읽었다.

```text
docs/tasks/T-001-host-toolchain-ci.md
docs/tasks/T-002-espnow-schema-v1.3.md
docs/tasks/T-003-espnow-codec-session.md
docs/tasks/T-004-uart-schema-codec.md
docs/tasks/T-005-canonical-model.md
docs/tasks/T-006-vehicle-profile-generator.md
docs/tasks/T-100-communicator-schematic.md
docs/tasks/T-100b-navigation-audio-bringup.md
docs/tasks/T-101-hardware-bringup.md
docs/tasks/T-102-stm32-platform.md
docs/tasks/T-103-stm32-fdcan-capture.md
docs/tasks/T-104-stm32-uart-control.md
docs/tasks/T-105-stm32-safety-profile.md
docs/tasks/T-106-stm32-command-executor.md
docs/tasks/T-200-communicator-esp32-bootstrap.md
docs/tasks/T-201-communicator-espnow.md
docs/tasks/T-202-communicator-uart-router.md
docs/tasks/T-203-peer-subscriptions.md
docs/tasks/T-300-controller-bootstrap.md
docs/tasks/T-301-controller-can-pipeline.md
docs/tasks/T-302-controller-ui-model.md
docs/tasks/T-303-controller-fft.md
docs/tasks/T-304-controller-local-config.md
docs/tasks/T-305-controller-command-orchestration.md
docs/tasks/T-400-diagnostic-bridge-bootstrap.md
docs/tasks/T-401-capture-cvtrace.md
docs/tasks/T-402-diagnostic-api-web.md
docs/tasks/T-403-signal-lab-evidence.md
docs/tasks/T-500-bench-hil-harness.md
docs/tasks/T-501-tucson-bus-discovery.md
docs/tasks/T-502-readonly-signal-validation.md
docs/tasks/T-503-audio-command-validation.md
docs/tasks/T-504-adaptive-volume-release.md
docs/tasks/T-505-auto-sport-validation.md
docs/tasks/T-506-release-qualification.md
```

요청된 스킬 원문도 직접 끝까지 읽었다: `C:/Users/digit/.codex/skills/embedded-architecture/SKILL.md`, `C:/Users/digit/.codex/skills/embedded-documentation/SKILL.md`. 이 스킬은 module/interface 경계·API 수명·task/ISR 예산·미실행 gate를 빠뜨리지 않는 데 사용했으며 새 별도 정책 정본이나 범위 밖 문서를 만들지 않았다.

### 실행 검증과 한계

문서 metadata/DAG는 `tools/validate_plan.py`, 부정 fixture는 `tests/test_plan_validation.py`, 상대 링크는 `tools/validate_document_links.py`로 검사한다. 1차 인계의 실제 결과는 상세 46개·오류0, 부정 fixture 35개 PASS다. 1차 sidecar는 firmware/OTA/차량 기능을 구현·target/HIL 검증하거나 stage/commit/push/merge하지 않았다.

## 6. 2차 읽기와 수정 종결 범위

main은 수정된 상세 task 46개 전체, architecture/protocol 규범과 OTA §1–13을 다시 읽고 §2의 요구 42행을 원 사용자 요구와 대조했다. hardware의 현재 pinmap/검증 gate, vehicle의 승격 규칙과 UI 설계/실제 화면을 추가 대조했다. raw DBC·제조사 PDF 전체와 과거 review/journal 전체를 두 번 읽었다는 뜻은 아니다. 변경 후 task delta·DAG를 다시 검사했다.

- N16R8·독립 reset/OTA는 ADR-007과 현재 회로 정본으로 통일했다. PCB 제작·공장 provisioning·실물 fault gate는 상세 task로 남겼다.
- 화면 DBC decode owner는 Controller, STM32 local safety는 별도다. v1.3 SIGNAL_BATCH는 비활성 draft로 명시했다. session u32/boot binding, 단계별 pairing transcript, reason u16과 원자 admission/ACK 계약을 정리했다. 실제 codec 동결은 T-002/T-003/T-004다.
- 설정 권위 config A/B와 NVS cache, Bridge 512KiB ring/OTA partition/3초 commissioning·reset 중 5초 recovery를 일치시켰다. privacy export와 후보 심사/VERIFIED 발급을 분리했다.
- 평균연비·추천 멘트·CENTER 복원·무보정 speed-only 증폭·lamp OFF→DPF 전체 정상 표현을 제거했다. 과거 문자 evidence 등급은 지역적 조사표로 한정하고 runtime enum/freshness와 직접 변환하지 않게 했다.
- 운전자/진단 웹 각 5뷰를 실제 Edge에서 확인하고 LVGL8.4를 공식 소스로 컴파일했다. 메인 4WD/순간연비/작은 원형계기와 RPM 옆 보조값, FFT 차속/RPM·signed dBFS, 날짜·분 선택·정차 잠금·터치 투과 경고·pending·수명 회귀를 검사했다.
- 밝기 반복 감광·경고 boost의 base 오염, 음량 pending/invalid FFT, SPORT stale/외부 mode/time gap을 host에서 수정했다. 이 helper의 source/token/physical TX-complete adapter 미연결은 T-301/T-305/T-106에서 계속 차단한다.

solar source 우선순위, provisioning 양 사본 손상 시 물리 복구 UX, 선택적 Controller 진단 fallback의 상세 구현은 각각 T-304/T-507/T-402의 명시된 설계·검증 산출물이다. 미결정을 임의 성공값으로 채우지 않았으며 요구 누락과 구분한다. 실행 명령과 미실행 gate는 [작업 일지](../journal.md), 최종 독립 리뷰는 [리뷰 색인](../reviews/README.md)으로 이어진다.
