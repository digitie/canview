# CANView 구현 준비 기준과 통합 설계

이 문서는 [아키텍처 개요](README.md)의 subsystem 설계를 구현 gate와 코드 경계로 통합한다. 개별 구현 범위와 acceptance는 해당 [상세 task](../tasks/)가 정본이다.

## 1. 문서 목적과 현재 판정

이 문서는 CANView의 여러 설계 문서를 실제 구현 순서와 코드 경계로 통합한 정본이다. 구현자는 개별 문서 사이의 빈칸을 임의로 해석하지 않고 이 문서와 [`tasks/`](../tasks/)의 원자 작업을 따른다.

1차 계획 감사의 비교 기준은 `4aeb2912da063c6fcb0d8715aa46f84c7d1d1b0f`다. 아래는 그 기준선의 구현/검증 상태이며, 병렬 UI 수정이나 이번 계획 보완을 target·실차 통과로 계산하지 않는다. 요구별 추적은 [요구사항 coverage](requirements-coverage.md), 실행 순서는 [task backlog](../tasks.md)가 담당한다.

| 범위 | 판정 | 근거 |
|---|---|---|
| 순수 C 자동화 알고리즘·수신 필터·DBC bit decoder | host 시험 가능 | 제한된 구현과 단위시험이 존재함 |
| Controller LVGL 화면 구조 | 정적 통합 준비 | 화면 계층은 있으나 BSP·thread-safe model adapter가 없음 |
| Communicator STM32 | T-102a boot/clock/watchdog bench 구현·검증 중 | 실물 G1, DMA UART, FDCAN, 안전 profile과 command executor가 없음 |
| Controller·Communicator ESP32 | bootstrap 준비 | ESP-IDF top-level project·기본 component·partition은 있으나 v1.3 protocol 전까지 application dependency·BSP·transport task가 없음 |
| Diagnostic Bridge | prototype 설계만 존재 | firmware, REST/WS backend, capture storage가 없음 |
| ESP-NOW·UART wire | 기본 통합 미완료 | v1.3/v1.0 전체 schema·codec·golden gate가 없음. navigation companion v1.4/v1.1의 별도 schema/host fixture만으로 기본 경로를 통과시키지 않음 |
| 2017 Tucson TL 신호 | 후보 단계 | 저장 DBC의 대상 차량 적합성을 입증한 capture가 없음 |
| 하드웨어 | 생성 회로와 정적 검증 존재, 제작·실측 미완료 | R1 및 N16R8 OTA 회로/ERC 기록은 있으나 land·PCB·조립·reset/rail/CAN 실측 gate가 열려 있음 |
| 독립 OTA | 설계 계약만 존재 | 정상/복구 앱·bootloader·서명 packager·journal·API·provisioning·전원 차단 HIL 미구현 |
| 차량 CAN 송신 | **금지** | hard TX gate의 실물 증명·검증된 vehicle profile·HIL/실차 증거가 없음 |

따라서 현재 기준선은 `설계/host prototype`이지 차량용 release가 아니다. G0 계약 이전에는 기능 firmware 통합을 시작하지 않는다. T-100/T-102/T-200/T-300 등의 회로·boot scaffold와 합성 host fixture 준비는 통합/G1 통과가 아니다. 실제 차량 TX는 G4 bench 통과 뒤에도 기능별 G5 시험 승인·profile·lease·build mode·local check·외부 gate를 모두 요구한다.

## 2. 구현자가 다시 결정하지 않을 사항

다음 사항은 이 감사에서 확정한다.

1. 장치명은 `Controller`, `Communicator`, `Diagnostic Bridge`다.
2. Communicator의 두 MCU는 `ESP32-S3-WROOM-1-N16R8`와 `STM32G474CEU6`다. [OTA 설계](ota.md)의 독립 reset·서비스 인터록·복구 계약을 따른다.
3. Communicator 정상 앱 내부 링크는 4 Mbps, 8-N-1, RTS/CTS, COBS, CRC-32/ISO-HDLC를 쓰는 독립 UART protocol `1.0`이다. navigation companion `1.1`과 OTA recovery UART는 각 정본의 별도 협상/baud/codec을 사용하며 서로 자동 대체하지 않는다.
4. 첫 통합 ESP-NOW 구현은 현재 `1.2` header에 Diagnostic Bridge 메시지를 합친 protocol `1.3`이다. `1.2`와 `1.3`을 동시에 구현하지 않는다. navigation `1.4` companion은 capability 협상 뒤에만 추가하며 `1.3` 기본 ABI를 재정의하지 않는다.
5. 무선 일반 frame 상한은 240 byte다. CAN FD payload는 v1.3에 억지로 넣지 않고 별도 미래 record type으로 남긴다.
6. 화면용 DBC decode는 Controller가 수행한다. Communicator ESP32는 DBC signal 이름·scale을 알지 않는다.
7. CAN 송신의 최종 판단과 frame 생성은 STM32가 수행한다. 어느 ESP32도 arbitration ID와 raw payload를 STM32에 전달해 송신시킬 수 없다.
8. Controller에는 검증된 음량·fader·balance·mute·rear mute·SDVC·audio restore와 SPORT button pulse에 필요한 의미 권한만 줄 수 있다. 도어 잠금·등화·임의 ECU 설정은 허용하지 않는다.
9. RTC, 화면 밝기, FFT 분석 설정, 유휴 timeout은 Controller-local 설정이다. RTC 변경을 차량 `COMMAND_REQUEST`로 보내지 않는다.
10. 자동 SPORT 상태기계는 STM32에서 실행하고 `SPORT ↔ 이전 mode`로 동작한다. Controller는 arm/disarm과 제한된 설정만 요청한다.
11. Diagnostic Bridge는 read-only observer다. diagnostic lease는 filter/capture 변경만 허용하며 control lease와 별도다.
12. STM32 firmware는 첫 버전에서 RTOS 없이 interrupt/DMA와 cooperative super-loop를 쓴다. 초기화 이후 heap allocation을 금지한다.
13. UI는 LVGL task 하나만 호출한다. 수신 callback, protocol task, FFT task가 LVGL API를 직접 호출하지 않는다.
14. 실차 검증 전 후보 신호는 운전자 화면의 정상 숫자로 표시하지 않는다. 후보 관찰은 Diagnostic Bridge에서만 한다.

## 3. 정본과 생성물

사람이 같은 enum과 구조를 여러 파일에 손으로 복사하지 않는다. 구현 후의 정본 관계는 아래와 같다.

| 영역 | 사람이 편집하는 정본 | 생성물 | 금지사항 |
|---|---|---|---|
| ESP-NOW ABI | `protocol/schema/espnow-v1.3.yaml` | `protocol/canview_protocol.h`, reference codec metadata, golden vectors | 생성 header 직접 편집 |
| UART ABI | `protocol/schema/uart-v1.0.yaml` | `protocol/canview_uart_protocol.h`, golden vectors | ESP-NOW header를 UART에 재사용 |
| 공통 enum | protocol schema의 named enum | C/Python/JSON enum table | UI별 숫자 enum 재정의·cast |
| 차량 profile | `vehicle-profiles/<profile>/profile.yaml`와 evidence manifest | Controller catalog, STM32 safety profile, filter defaults, report | DBC 이름만으로 command 생성 |
| DBC 원본 | `dbc/opendbc/*`와 고정 SHA-256 | parse cache와 report | upstream DBC 수정 |
| 설정 UI | `config/schema/*.yaml` | Controller descriptor, Bridge JSON schema | web/LVGL 옵션을 따로 작성 |
| 시각 token | `ui/tokens/theme.yaml` | LVGL C token, CSS variables | `tokens.css`와 C header 수동 이중 관리 |
| hardware pin | 승인된 KiCad schematic/netlist | pinmap CSV와 MCU pin header | 제안 CSV만 보고 PCB 제작 |
| Diagnostic API | `api/diagnostic-v1.openapi.yaml` | C route table 검증자료, web client type | 문서 prose만으로 endpoint 구현 |
| capture bundle | `schema/cvtrace-v1.schema.json` | validator와 fixture | schema version 없는 export |
| navigation 확장 | `protocol/schema/navigation-v1.json`와 [navigation 계약](protocols/navigation.md) | ESP-NOW 1.4/UART 1.1 companion codec·fixture | 기본 wire version에 미협상 센서 메시지 삽입 |
| 독립 OTA | [OTA §4–9·12](ota.md)의 layout/서명/정책 계약 → T-007/T-204/T-107/T-108/T-205 schema | 역할별 normal/recovery·STM bootloader·packager·golden vector | 현재 factory scaffold를 OTA 지원으로 표시 |

생성물 첫 줄에는 `DO NOT EDIT`, generator version, source digest를 넣는다. CI는 생성기를 다시 실행한 뒤 diff가 생기면 실패한다. 같은 상수가 schema·C·Python·JavaScript에서 다르면 build를 통과시키지 않는다.

memory, stack, queue, WCET, latency와 radio budget도 `config/budgets/*.yaml`을 정본으로 두고 linker map·`.su`·runtime evidence 검사기가 읽는다. Markdown 표만 고치고 machine budget을 갱신하지 않는 변경은 CI에서 실패한다.

## 4. repository 목표 구조

```text
api/
└─ diagnostic-v1.openapi.yaml
config/schema/
├─ controller.yaml
└─ communicator.yaml
config/budgets/
├─ controller.yaml
├─ communicator-esp32.yaml
├─ communicator-stm32.yaml
└─ diagnostic-bridge.yaml
protocol/
├─ schema/
│  ├─ espnow-v1.3.yaml
│  └─ uart-v1.0.yaml
├─ golden/
├─ canview_protocol.h                 # generated
└─ canview_uart_protocol.h            # generated
schema/
└─ cvtrace-v1.schema.json
tools/
├─ generate_protocol.py
├─ generate_vehicle_profile.py
├─ generate_ui_tokens.py
├─ toolchain-versions.json
├─ validate_cvtrace.py
└─ environment/
   └─ setup-windows.ps1
vehicle-profiles/tucson-tl-2017/
├─ profile.yaml
├─ evidence/
└─ generated/
   ├─ controller_signal_catalog.c
   ├─ controller_filter_defaults.c
   ├─ stm32_safety_profile.c
   └─ profile-report.md
firmware/
├─ controller/                        # ESP-IDF app scaffold → BSP/UI
├─ communicator/
│  ├─ esp32/                          # ESP-IDF bridge scaffold → transport
│  └─ stm32/                          # CMake firmware
└─ diagnostic-bridge/                 # future ESP-IDF app
tests/
├─ host/
├─ protocol/
├─ fixtures/
├─ hil/
└─ ui/
```

현재 파일은 원자 task에서 이 구조로 단계적으로 옮긴다. 한 PR에서 전체 tree를 빈 directory로 먼저 만들지 않는다.

## 5. 장치별 책임과 런타임 구조

### 5.1 Communicator STM32

STM32는 차량 bus에 가장 가까운 최종 안전 경계다.

```text
FDCAN RX IRQ
  -> hardware timestamp + bounded RX ring
  -> bus counter / inventory
  -> safety signal decoder
  -> observer software filter
  -> UART telemetry queue

UART DMA RX
  -> delimiter scanner
  -> COBS/CRC/parser
  -> control/config queue
  -> lease + idempotency + safety checks
  -> compiled vehicle command builder
  -> FDCAN TX
  -> feedback matcher
  -> COMMAND_RESULT / audit
```

초기 task는 RTOS를 넣지 않는다. interrupt는 register와 timestamp를 읽어 고정 ring에 복사하고 즉시 반환한다. main loop는 아래 순서를 반복한다.

1. UART RX packet budget 처리
2. FDCAN RX ring budget 처리
3. 1 ms clock service와 10 ms safety tick
4. command state machine 한 단계
5. priority UART TX queue 처리
6. bus error와 PHY state 처리
7. 모든 필수 progress marker가 갱신됐을 때만 IWDG refresh

초기화 후 `malloc`, `calloc`, `realloc`, `free`를 호출하지 않는다. ISR에서 COBS, CRC, DBC decode, command build, UART blocking TX를 하지 않는다. safety/control queue가 가득 차면 새 명령을 `BUSY`로 거부하고 TX lease를 폐기한다.

### 5.2 Communicator ESP32

권장 task 경계는 다음과 같다.

| task | 입력 | 출력 | 금지 |
|---|---|---|---|
| `espnow_ingress` | Wi-Fi callback 고정 pool | protocol RX queue | parse, NVS, UART blocking |
| `espnow_protocol` | RX queue | peer/session state, ACK/control/telemetry queue | DBC decode |
| `uart_link` | UART driver event/DMA ring | semantic UART messages | ESP-NOW frame tunnel |
| `router` | UART semantic data·peer subscriptions | peer별 bounded telemetry queue | 한 peer filter를 다른 peer에 적용 |
| `control` | 인증된 Primary 명령 | UART command transaction | raw CAN TX 생성 |
| `storage` | pairing/config commit | encrypted NVS | callback에서 write |

Wi-Fi callback은 `source MAC + receive metadata + 240 byte 이하 frame`을 고정 pool로 복사할 뿐이다. control queue와 telemetry queue를 분리한다. 모든 queue와 pool 크기는 compile-time 상수이며 high-water mark를 heartbeat에 넣는다.

### 5.3 Controller

```text
ESP-NOW callback
 -> fixed RX pool
 -> protocol/session task
 -> local RX allow-list
 -> catalog decoder + freshness store
 -> automation/model task
 -> double-buffer UI snapshot
 -> LVGL task(20 Hz consume, 60 Hz animation)

I2S microphone DMA
 -> audio ring
 -> window/FFT/noise feature task
 -> automation/model task
```

LVGL object는 UI task가 생성·갱신·삭제한다. producer는 inactive model buffer를 완성한 뒤 monotonically increasing `model_revision`과 함께 pointer/index를 교체한다. UI task는 한 revision의 snapshot만 읽고 오래된 revision을 적용하지 않는다. 화면 전환 중에도 callback이 LVGL object pointer를 보관하지 않는다.

### 5.4 Diagnostic Bridge

Bridge는 Controller와 Communicator 각각에 encrypted peer로 등록할 수 있다.

- Communicator 직접 link: observer filter, inventory, capture, marker, `.cvtrace`
- Controller 직접 link: Controller-local 설정 schema 조회와 사용자 확인이 필요한 remote setting
- 휴대폰 link: 고정 channel SoftAP, local HTTP/WebSocket

Bridge는 두 ESP-NOW link 사이에서 raw packet을 중계하지 않는다. 요청은 target owner의 semantic message로 새로 encode한다. Bridge reboot나 휴대폰 disconnect는 control lease에 영향을 주지 않는다.

Bridge–Controller direct peer는 자동 생성하지 않는다. 양쪽 physical service window와 설치자 선택으로 별도 등록하고, transcript에 양쪽 device ID, MAC, role, 허용 message class, channel, protocol version과 nonce를 모두 bind한다. 이 peer의 control scope는 항상 0이며 owner-targeted remote config 외 메시지는 거부한다. SoftAP와 두 ESP-NOW link의 airtime도 설치 단위 20 kB/s budget 안에서 합산한다.

## 6. 시간, boot epoch와 revision

서로 다른 clock을 같은 숫자축으로 암묵적으로 비교하지 않는다.

| clock | 단위 | 수명 | 용도 |
|---|---|---|---|
| STM monotonic | µs | STM `boot_id` 동안 | CAN capture ordering, sample age |
| Communicator ESP monotonic | ms | ESP `boot_id` 동안 | ESP-NOW header, retry, lease |
| Controller monotonic | ms | Controller `boot_id` 동안 | UI freshness, interaction timeout |
| RTC wall clock | local date/time | RTC validity 동안 | 표시, 일출·일몰 경고, 파일 이름 |

STM32가 새 `LINK_HELLO.boot_id`로 연결되면 Communicator ESP32는 아래를 원자적으로 수행한다.

1. UART RX/TX telemetry와 pending command를 폐기한다.
2. control lease와 diagnostic lease를 폐기한다.
3. 현재 ESP-NOW application session을 종료하고 새 random `session_id`로 HELLO를 다시 시작한다.
4. 새 `STATE_SNAPSHOT` 전에는 CAN batch를 peer에 보내지 않는다.

따라서 `canview_can_batch_header_t.base_time_us`는 **현재 ESP-NOW session 안에서만** 해석한다. 이 규칙으로 기존 12 byte batch header를 바꾸지 않으면서 STM boot 전후 sample을 섞지 않는다. STM reboot 뒤 무선 session이 유지되는 구현은 금지한다.

`state_revision`은 Communicator가 소유하고 profile, bus 상태, lease owner, safety inhibit, audio snapshot, drive-mode automation 상태가 바뀔 때 증가한다. Controller-local config에는 별도 `controller_config_revision`, peer subscription에는 `(peer_device_id, subscription_revision)`, diagnostic observer에는 `(bridge_device_id, observer_revision)`을 쓴다. 서로 다른 revision을 한 정수로 재사용하지 않는다.

### 6.1 end-to-end command 시간 mapping

인접 ESP끼리의 일반 TIME_SYNC만으로 STM32가 Controller에서 만든 TTL을 판정하지 않는다. control capability를 열기 전에 Controller와 STM32가 Communicator 두 MCU를 단순 전달 hop으로 사용하는 4-timestamp `CONTROL_TIME_SYNC`를 세 번 수행한다. mapping key는 `(controller_boot_id, stm_boot_id, control_sync_generation)`이고 가장 작은 RTT sample의 offset과 uncertainty를 사용한다.

- control sync는 10초마다 갱신하고 30초가 지나면 invalid다.
- command TTL은 500–30,000 ms이며 `issued_at_controller_ms`, generation과 원래 TTL을 모든 hop에서 바꾸지 않는다.
- STM32는 mapped deadline에서 uncertainty를 빼 보수적으로 만료를 계산한다. uncertainty가 50 ms를 넘거나 generation/boot ID가 다르면 명령을 거부한다.
- Controller, Communicator ESP 또는 STM32 중 하나가 reboot하면 mapping, lease와 pending을 모두 폐기한다.
- telemetry mapping은 STM boot ID와 source timestamp를 보존하고, 각 hop의 offset/uncertainty를 합성한다. 합성 age가 음수가 되면 clock fault로 처리하며 0으로 clamp해 fresh로 만들지 않는다.

## 7. protocol 구현 기준

### 7.1 version과 상태

ESP-NOW `1.3`과 UART `1.0`은 독립 version이다. 양쪽 모두 major가 다르면 online이 되지 않는다. 같은 major의 minor 차이는 schema에 `since_minor`가 표시된 optional message/TLV만 허용한다.

ESP-NOW 상태는 아래 한 방향으로 진행한다.

```text
UNPROVISIONED -> PAIRING -> KEYED -> HELLO -> TIME_SYNC -> SNAPSHOT -> ONLINE
      ^             |         |        |          |           |         |
      +-------------+---------+--------+----------+-----------+----> LOCKED/DEGRADED
```

`ONLINE`이 되려면 encrypted peer context, 양방향 HELLO, capability 교집합, time mapping, current snapshot이 모두 있어야 한다. telemetry만 필요하면 read-only online이 가능하지만 control lease는 별도 조건을 모두 만족해야 한다.

### 7.2 payload ABI 범위

schema는 아래 메시지 각각에 대해 exact length 또는 bounded count, 방향, 허용 role/state, QoS, duplicate key, response type을 반드시 정의한다.

| 계열 | 필수 payload 정보 |
|---|---|
| discovery/pairing | device·boot ID와 MAC binding, protocol range, channel, 두 128-bit nonce, installation hint, key generation, 요청/로컬 승인/할당 role, 허용 message class, transcript digest/HMAC, 결과 |
| HELLO | device·boot ID, role, selected version, max frame, build ID digest, random challenge |
| CAPABILITIES | feature bits, control scope, max filter/peer/batch, profile/config schema digest |
| TIME_SYNC | four-timestamp exchange 값, time domain, 계산된 offset·RTT·uncertainty |
| HEARTBEAT | sender boot ID, state revision, queue/drop/error, bus masks, link state |
| ACK/ERROR | original sequence, semantic status/code, retry hint; ACK 자체 ACK 금지 |
| STATE_SNAPSHOT | STM boot ID, state/profile revisions·digest, build/TX mode, bus states, lease state, safety inhibit, audio/SPORT summary |
| CAN_BATCH/BUS_STATUS | STM monotonic sample, bus/ID/flags/DLC/data, bitrate/mode/error counter |
| peer filter/stream | peer namespace revision, add/replace/delete/clear, exact effective quota와 result |
| observer/capture | diagnostic lease, observer revision, bounded filter plan, capture lifecycle, marker uncertainty |
| command | request token, command ID, TTL, expected state revision, preconditions, typed argument TLV, result lifecycle |
| control lease | owner device, random lease ID, acquire/renew/release, granted duration·expiry·scope |
| config | target owner, schema version, expected owner revision, typed records, staged/confirmed/final status |
| bulk | object ID/type/size/digest, fragment index/count, four-fragment ACK bitmap, timeout/abort reason |

고정 struct가 없는 메시지는 protocol 구현 전에 존재하지 않는 것으로 취급한다. enum 번호만 추가해 switch branch부터 만드는 것을 금지한다.

wire 폭도 schema에서 공통으로 고정한다. `COMMAND_RESULT`와 상태 응답의 `stage/status`는 `u8`, 공통 `reason/error code`는 `u16`, catalog/filter/config/state revision은 `u32`다. window 안에서만 쓰는 record count는 명시적으로 reset되는 bounded `u32`이고, boot 동안 누적되는 health/drop/auth counter는 saturating `u64`다. 누적 counter를 전송하는 모든 payload는 sender boot ID를 직접 포함하거나 같은 boot-bound snapshot에 속해야 한다. `u8` revision이나 설명 없이 wrap하는 `u32` 누적 counter를 새로 만들지 않는다.

### 7.3 parser 순서

모든 ESP-NOW 수신자는 다음 순서를 지킨다.

1. callback metadata에서 source MAC, channel, encrypted peer 여부 확인
2. null과 `32 <= length <= 240` 확인
3. byte reader로 magic/header length/payload length/reserved/known flags 확인
4. 전체 길이와 CRC 확인
5. message가 현재 role·link state에서 허용되는지 확인
6. pre-session message이면 pairing HMAC과 nonce/transcript 확인
7. online message이면 session ID와 boot binding 확인
8. sequence anti-replay window 확인
9. message별 exact/bounded payload, reserved, enum, count, TLV bounds 확인
10. semantic quota·revision·TTL 검사
11. idempotency cache 조회
12. worker slot·idempotency entry·ACK용 control queue 자원을 원자 예약하고 admission을 commit한 뒤 accepted ACK를 전송. 하나라도 실패하면 예약을 해제하고 pre-ACK BUSY 또는 정본 drop 정책을 적용하며, 실행 queue 등록 실패 뒤 수락 ACK를 남기지 않음

CRC 실패 frame은 auth error response를 만들지 않는다. broadcast에는 오류 응답을 보내지 않는다. parsing 실패가 peer·내용에 따라 timing oracle이 되지 않도록 동일 category 오류를 rate-limit한다. 인증 실패 잠금은 알려진 peer·pairing candidate별 exponential backoff로만 적용한다. 알 수 없는 MAC의 flood가 기존 정상 peer나 전체 설치를 `AUTH_LOCKED`로 만들 수 없다.

UART parser는 transport encryption 대신 COBS decode와 CRC를 먼저 검사하지만 이후 length/state/sequence/idempotency 원칙은 같다.

### 7.4 명령 정확성

명령 성공은 아래 네 상태를 구분한다.

```text
MAC 전달 -> ACK(문법·queue 수락) -> RESULT(실행 중) -> RESULT(COMPLETED, feedback 확인)
```

Controller의 8-slot pending tracker는 송신 측 UI 상태용이다. 수신 측 STM32에는 256-entry idempotency cache가 필요하다. cache key는 `(origin_device_id, origin_boot_id, wireless_session_id, control_generation, request_token, command_id, canonical argument digest)`다. command payload는 `issued_at_controller_ms`와 `control_sync_generation`을 포함한다. 모든 retry는 새 packet sequence를 쓰되 token, issued time, TTL과 canonical argument bytes를 바꾸지 않는다. ACK와 모든 RESULT도 original packet sequence뿐 아니라 `request_token`을 포함한다.

- 같은 token과 같은 digest: 재실행하지 않고 마지막 result 재전송
- 같은 token과 다른 digest: auth/protocol fault, 실행 금지
- unexpired TTL 또는 terminal result 보존 60초 안의 entry는 LRU로 축출하지 않음. cache가 모두 live면 새 요청을 ACK하기 전에 `BUSY`로 거부
- STM 또는 ESP boot 변경: pending 자동 재실행 금지
- result가 ACK보다 먼저 와도 terminal result 유지
- TTL은 Controller 생성 monotonic을 직접 비교하지 않고 matching time-sync generation의 offset·uncertainty로 남은 수명을 계산한다. mapping이 없거나 uncertainty가 command별 한도를 넘으면 거부한다.
- 같은 packet sequence가 window 안에서 다시 오면 handler를 재실행하지 않고 cached ACK만 반환할 수 있다. window 밖의 오래된 sequence는 조용히 폐기한다.

### 7.5 provisioning과 peer 신뢰 경계

`installation_id`는 장치를 묶어 보여 주는 비밀 아닌 식별자다. 설치 전체가 공유하는 `pairing_secret` 또는 공용 PMK를 두지 않는다. 각 ESP32는 외부로 내보내지 않는 random `local_pmk`를 가지고, 직접 통신하는 두 endpoint만 공유하는 256-bit `link_root`에서 peer LMK를 파생한다.

```text
Communicator ↔ Primary Controller : link_root_cp
Communicator ↔ Diagnostic Bridge  : link_root_cb
Controller   ↔ Diagnostic Bridge  : link_root_pb (명시적 service pairing 때만)
```

각 peer record는 상대 device ID/MAC, locally authorized role, control scope, message classes, fixed channel, protocol range와 link generation을 가진다. pairing transcript는 이 값과 양쪽 nonce를 모두 인증한다. peer가 wire에서 요구한 role/scope는 로컬 record보다 넓게 협상될 수 없다. pair root 추가·교체는 정확히 두 endpoint에 USB 또는 봉인된 일회성 pair package로 provision하며 radio로 root를 전달하지 않는다. Bridge 한 대의 NVS 유출로 Primary Controller 링크를 사칭할 수 있으면 G0 실패다.

인증 실패 backoff와 key rotation은 pair별이다. unknown-source flood와 한 peer의 손상은 다른 established peer의 heartbeat·telemetry·복구를 막을 수 없다. NVS transaction은 새 record staging, encrypted HELLO 확인, activation, 이전 generation 폐기 순서를 따르며 각 power-loss 지점의 복구 fixture가 있어야 한다.

빈 NVS provisioning이 vehicle speed 신호와 control lease에 의존하는 순환은 허용하지 않는다. 새 장치는 차량 연결 전에 USB pair package를 넣는 것이 기본이다. 예외적으로 물리 service window에서 bootstrap할 때는 `CAPTURE_ONLY`, hard gate off, control scope 0, built-in signed bitrate/bus-type-only profile로만 동작한다. 이 상태에서는 listen-only ID inventory와 pairing만 가능하고 DBC decode, control lease, command와 persistent broad filter는 금지한다.

### 7.6 STM32까지 이어지는 control 인증

ESP-NOW 암호화가 끝나는 Communicator ESP32만 믿고 STM32가 차량 명령을 실행하지 않는다. Primary Controller와 STM32에만 별도 pair-specific `control_root`를 provision하고, Communicator ESP32와 Diagnostic Bridge는 이를 저장하지 않는다. `CONTROL_LEASE`, `COMMAND_REQUEST`, SPORT 설정과 STM32의 terminal `COMMAND_RESULT`는 schema가 정의한 canonical envelope에 HMAC-SHA-256의 앞 16 byte `control_tag`를 붙인다.

요청 envelope에는 origin device/boot ID, wireless session ID, control key generation, locally assigned role/scope, request token, command/config ID, issued time, TTL, control-sync generation, expected state revision와 canonical argument digest를 포함한다. Communicator ESP32는 bytes를 변경하지 않고 UART로 전달한다. STM32는 tag, provisioned Primary identity, generation, lease, TTL과 generated command table을 모두 확인한다. 결과 envelope에는 token, terminal stage/reason, 실제 TX audit digest, feedback revision/time을 넣고 STM32가 tag를 만든다. Bridge와 read-only peer에는 `control_root`를 provision하지 않는다.

이 구조에서 Communicator ESP32는 가용성 TCB이지만 차량 명령 권한 TCB는 아니다. ESP firmware나 UART frame이 변조되어도 유효한 새 control envelope를 만들 수 없어야 한다. root rotation은 차량 정지·양 endpoint 물리 service·old/new generation 교차 확인을 요구하고, STM32 보호 Flash와 Controller encrypted NVS의 power-loss transaction을 시험한다.

## 8. 수신 filter와 대역폭 경계

`CAN_FILTER_*`의 의미를 두 경계로 분리한다.

1. **Controller local RX allow-list**: Controller가 소유하는 최종 default-deny 경계다. transport가 무엇을 보냈든 여기를 통과하지 못하면 decoder/model에 들어가지 않는다.
2. **Communicator peer subscription**: 각 peer가 요청하고 Communicator가 소유하는 upstream 대역폭 절감 경계다. `(peer_device_id, revision)` namespace를 갖는다.

같은 `canview_can_filter_t` entry layout을 쓸 수 있지만 store와 revision은 공유하지 않는다. Communicator의 prefilter를 보안상 신뢰하지 않는다.

변경 순서는 다음과 같다.

| 변경 | Controller local 처리 | upstream 처리 |
|---|---|---|
| 범위 축소·삭제 | 즉시 local deny 적용 | 이후 subscription 갱신, 실패해도 local deny 유지 |
| 범위 확대·추가 | local desired에 staging하되 decode 비활성 | upstream 수락 후 catalog digest가 맞을 때 local effective 승격 |
| replace | 새 범위의 확대·축소 부분을 위 규칙으로 각각 처리 | 단일 revision transaction |

Controller 32개, 한 batch 8개, 최소 period 20 ms, 최대 60 s, filter당 period 최대 32 record를 유지한다. 모든 peer의 ESP-NOW application 합계는 sustained 20 kB/s, 1초 burst 32 kB다. P0/P1에 2 kB/s를 예약하고 Controller runtime telemetry에 최소 8 kB/s를 예약한다. Diagnostic stats와 raw capture는 남은 token만 쓰며 가장 먼저 drop한다.

STM32 union observer plan은 최대 64 entry다. Communicator ESP32는 peer filter union을 STM32에 보내고 STM32에서 받은 raw record를 다시 peer별로 필터링한다. union이 가득 차면 기존 active plan을 보존하고 새 transaction 전체를 거부한다.

filter snapshot fragment는 `(peer_device_id, snapshot_id, revision, part_index, part_count, total_count)`를 갖는다. part 하나라도 빠지거나 revision이 바뀌면 전체 snapshot을 폐기하며 서로 다른 조회의 fragment를 합치지 않는다. ADD/REPLACE/DELETE/CLEAR도 expected revision을 가진 staging transaction이고 모든 entry 검증 후 pointer/revision을 한 번만 바꾼다.

20 kB/s는 display당 값이 아니라 설치 전체 ESP-NOW application payload hard cap이다. P0/P1 control·lease·ACK·heartbeat 2 kB/s와 Primary Controller runtime 8 kB/s는 빌려주지 않는 최소 reserve다. 남은 10 kB/s 안에서 read-only telemetry, Bridge stats, raw capture 순으로 사용하고 P4부터 drop/coalesce한다. Diagnostic Bridge SoftAP의 HTTP bulk는 이 byte cap과 별개지만 같은 radio를 쓰므로 차량 이동, active control lease 또는 P0/P1 deadline miss가 있으면 upload/download를 정지한다. 정차 service에서만 HTTP bulk를 64 kB/s로 제한하며 authenticated peer도 ingress token bucket과 fixed queue를 통과한다.

## 9. 차량 profile과 DBC pipeline

### 9.1 profile 입력

`vehicle-profiles/tucson-tl-2017/profile.yaml`은 최소 다음을 가진다.

```yaml
identity: 차량 연식·엔진·구동·BlueLink와 profile UUID
source: opendbc commit, 각 DBC SHA-256, generator version
buses: 논리 bus, 실차 이름, transceiver type, bitrate, connector evidence
signals: id, bus, frame, bit layout, scale, unit, range, freshness, evidence grade
derived_signals: 식별된 dependency와 품질 전파 규칙
commands: semantic ID, control scope, preconditions, fixed builder, feedback, timeout
captures: 승인 evidence ID 목록
```

### 9.2 evidence와 품질

runtime 품질과 검증 증거를 하나의 enum으로 섞지 않는다.

- `signal_quality`: `VALID`, `STALE`, `UNAVAILABLE`, `OUT_OF_RANGE`, `FAULT`
- `evidence_grade`: `UNKNOWN`, `CANDIDATE`, `OBSERVED`, `VERIFIED`
- candidate 저장소의 별도 `review_status`: `PENDING`, `APPROVED`, `REJECTED`

운전자 UI에 숫자를 표시하려면 `quality == VALID && evidence_grade == VERIFIED`여야 한다. `OBSERVED` 이하 값은 Signal Lab에서만 볼 수 있다. derived signal의 quality는 dependency 중 가장 나쁜 runtime 품질을 따르고 evidence는 가장 낮은 dependency grade보다 높아질 수 없다.

위 `evidence_grade` 네 값이 공통 정본이며 `REJECTED`를 다섯 번째 등급이나 `UNKNOWN`의 별칭으로 추가하지 않는다. `UNKNOWN`은 아직 근거가 없는 상태다. `review_status`는 특정 candidate revision의 심사 결과로, `PENDING`이 기본이고 `APPROVED`도 검증 등급·서명 profile gate를 대신하지 않는다. `REJECTED`는 반대 evidence/제외 사유를 남긴 심사 결과이며 audit에는 기존 grade를 보존할 수 있지만 operational export/adapter 입력으로 사용할 수 없다. 재심사는 새 revision과 새 승인 근거가 필요하다.

schema의 unknown enum 또는 승인되지 않은 descriptor는 exhaustive adapter에서 `UNAVAILABLE`로 닫는다. 심사 상태를 wire quality/evidence로 숫자 cast하지 않는다. derived 입력 하나라도 `UNKNOWN`이거나 사용할 수 없는 revision이면 VERIFIED 값이 생성되지 않는다. `REJECTED` 후보에서 derived 값을 만들거나 기존 서명을 새 revision에 재사용하는 것도 거절한다. 이 규칙은 승인된 profile에서 데이터를 소비하는 경계이며 Diagnostic Bridge가 기존 차량 profile·lease를 직접 수정할 권한을 뜻하지 않는다.

`VERIFIED` 승격에는 서로 다른 ignition cycle의 반복 capture, marker/evidence, DBC/decoder golden vector, 단위·방향·range·stale 기준, 반례 검토가 필요하다. drop/gap이 수용 한도를 넘는 capture는 승격 증거가 될 수 없다.

### 9.3 분리 생성

generator는 같은 profile에서 서로 다른 최소 산출물을 만든다.

- Controller: 표시용 signal descriptor, default local filter, label/unit metadata
- STM32: 안전 판단에 필요한 signal decoder, semantic command builder, feedback matcher, 허용 scope
- Communicator ESP32: profile ID/digest와 wire capability만. signal 이름과 bit layout은 넣지 않음
- Diagnostic Bridge: candidate schema와 evidence validator. command builder는 생성하지 않음

`evidence_grade != VERIFIED`인 command 또는 feedback signal이 하나라도 있으면 STM32 command artifact를 생성하지 않고 build를 실패시킨다. DBC에 message가 존재한다는 사실만으로 송신 frame을 만들지 않는다.

`SIGNAL_BATCH`는 [ESP-NOW 정본](protocols/esp-now.md)의 draft 비교 자료이며 v1.3 송수신 경로에서 비활성이다. 화면 DBC decode는 Controller가 raw CAN을 받아 수행한다. Controller operational catalog와 STM32 safety profile의 `bit_length <= 32` 제한은 별도로 유지한다. 33–64 bit DBC field는 Diagnostic Bridge가 raw frame에서 별도 64-bit candidate로 분석할 수 있지만 운전자/safety signal로 자동 승격하지 않는다. operational 지원이 필요하면 새 value/record type을 protocol major 호환성 검토와 함께 추가한다.

profile validator는 endian/value type/enum, DLC, signedness, factor/offset/min/max가 finite인지와 scaling overflow를 검사한다. 일반 operational descriptor의 frame kind는 `RX DATA`만 허용한다. RTR, error frame과 TX echo는 Diagnostic Bridge descriptor가 명시적으로 opt-in할 때만 관찰하며 일반 signal 값이나 freshness를 갱신하지 않는다. DLC 뒤 byte는 수신 normalization 단계에서 0이어야 한다.

## 10. 차량 송신 안전 계층

### 10.1 build mode

| mode | CAN 수신 | CAN ACK/TX | command executor | 사용처 |
|---|---|---|---|---|
| `CAPTURE_ONLY` | 허용 | FDCAN silent/listen-only, 송신 0 | compile-time 비활성 | 최초 board·실차 조사 |
| `BENCH_TX` | 허용 | bench bus에서만 | physical hard gate와 bench fixture가 있어야 활성 | simulator/HIL |
| `VEHICLE_TX` | 허용 | 승인 channel만 | 승인 profile·release manifest·hard gate 필요 | 최종 폐쇄시험 이후 |

단순 `#define` 하나로 `VEHICLE_TX`를 켤 수 없게 한다. release build는 승인 profile digest, hardware revision, evidence manifest digest와 별도 CI environment를 요구한다.

### 10.2 hard TX gate

현재 R2/N16R8 생성 회로에는 firmware와 독립적인 PHY RX/TX gate·외부 watchdog·ARM latch·J31 서비스 인터록이 존재한다. 이는 회로/ERC 상태이며 reset·brownout·PHY dominant·지연/역급전의 실물 검증 완료가 아니다. 허용식과 pin은 [OTA §6](ota.md), [R1 회로](../hardware/r1/communicator-circuit.md)와 해당 생성 netlist가 정본이다. 정상 RUN에서 TX만 unarmed인 capture-only 수신과 J31 제거/rail fault로 RX/TX 모두 차단된 service 상태를 구분한다.

- 전원 off/reset/unprogrammed MCU: TXD forced recessive
- service jumper 또는 sealed population option이 없으면 arm 불가
- logic fault 한 개가 임의 dominant를 만들지 않는지 검토
- CAN FD 목표 bitrate에서 propagation delay와 duty distortion 검증
- gate 상태를 STM32가 읽어 heartbeat/audit에 보고하되, 읽은 값만으로 gate를 우회할 수 없음

구체 IC·GPIO·외부 pull은 현재 BOM/pinmap/netlist에 정의되어 있다. T-100/T-100a의 원문·PCB 대조와 T-101/T-508의 실측으로 닫으며 free GPIO를 임의 배정하지 않는다. gate가 없거나 실제 차단이 검증되지 않은 prototype을 차량 송신 release로 표시하지 않는다. `CAPTURE_ONLY` 이름도 하드웨어 안전 실측을 대체하지 않는다.

각 gate의 disable level·pull·PHY TXD recessive·propagation budget은 생성 회로 정본에 따른다. active-low `/OE`로 일괄 가정하지 않으며 FT 경로의 active-high OE와 외부 pull-down도 대조한다. permit은 물리 TX_ARM·J31 SERVICE_RUN·독립 reset/rail-good·외부 watchdog·새 STM ARM edge를 반영한다. `CAPTURE_ONLY` variant는 TX_ARM을 실장하지 않고 TX API도 link하지 않는다. 이 gate는 reset·brownout·CPU stall을 제한하지만 올바른 cadence로 watchdog을 갱신하는 논리 오동작의 정상 형식 frame까지 판별하는 bus guardian은 아니다. 따라서 valid-frame flood는 generated executor의 ID/DLC/data/rate/total-frame 제한과 HIL analyzer가 별도로 막는다.

### 10.3 실행 조건

STM32는 다음 조건의 교집합에서만 command를 시작한다.

```text
VEHICLE_TX 또는 BENCH_TX build
AND hardware TX gate armed
AND UART online + current STM boot acknowledged
AND encrypted Primary Controller session
AND unexpired control lease
AND negotiated scope
AND approved vehicle profile digest
AND expected state revision
AND every safety signal fresh/valid/verified
AND bus normal, no bus-off/error-passive inhibit
AND command-specific preconditions
```

하나라도 false가 되면 시작 전 command는 거부한다. 실행 중 link/lease가 사라지면 새 frame을 중단하고 command별 safe termination을 수행한다. 이미 보낸 pulse를 되돌릴 수 있다고 가정하지 않는다.

request의 `precondition_flags`는 송신자가 관찰한 기대값일 뿐 권한이 아니다. STM32 generated command table이 command별 필수 known-mask와 조건을 소유한다. unknown/zero/partial sender mask로 필수조건을 생략할 수 없다. STM32는 admission, queue dequeue, 각 CAN frame 또는 button pulse 직전에 현재 local safety snapshot을 다시 검사하고 하나라도 나빠지면 이후 TX를 중단한다.

audio profile은 첫 TX 전에 OEM snapshot과 transaction generation을 bounded recovery Flash record에 원자 저장한다. 각 step은 feedback 뒤에만 다음으로 진행하며 결과는 `COMPLETED`, `PARTIAL`, `ROLLED_BACK`, `ROLLBACK_FAILED`를 구분한다. power loss, bus-off, lease 상실 또는 외부 knob 조작 뒤 reboot reconciliation이 끝나기 전 새 profile을 시작하지 않는다.

SPORT automation은 `DISABLED`, `MONITOR_ONLY`, `ARMED_TX`, `ENTER_PENDING`, `ACTIVE`, `EXIT_PENDING`, `INHIBITED`를 구분한다. enable의 reboot 기본은 `MONITOR_ONLY`이고 explicit arm과 G5 capability가 있어야 `ARMED_TX`가 된다. button feedback timeout은 첫 성공한 physical TX-complete timestamp부터 1,500 ms이며 generated profile 상수 하나를 STM/UI가 공유한다.

## 11. hardware 보완 기준

R1 [상세 회로](../hardware/r1/README.md)에 [OTA §6](ota.md)의 N16R8·독립 reset·J31/J32/U56 서비스 인터록 변경을 적용한 생성 `hardware/communicator/pinmap.csv`와 netlist가 현재 검토 입력이다. 과거 `pinmap-proposed.csv`와 MINI/공유 reset 그림은 현행 제작 입력이 아니다. T-100은 회로·계산·land 승인, T-100a는 PCB/DRC/제조 입력, T-101은 조립품 실측, T-508은 OTA fault qualification을 소유한다. 회로 task는 다음을 모두 산출해야 한다.

1. KiCad schematic, PCB constraints, net classes, ERC 결과
2. 정확한 manufacturer part number와 대체품 정책을 포함한 BOM
3. LM74800 FET/TVS/fuse/inductor/capacitor worst-case 계산
4. 5 V·3.3 V startup, RF burst, PHY dominant worst-case 전류·thermal 예산
5. 현 R1의 외부 fused IGN/ACC 차단·USB service 정책과 OFF 전류/역급전 시험 조건. 상시 BAT+/CAN wake variant는 별도 설계 승인 전 포함하지 않음
6. 차량 harness connector pinout, keying, ground/chassis/shield 정책
7. CAN1/2 split termination과 CAN3 RTH/RTL의 독립 DNP variant
8. MAX3055 `WAKE`, `INH`, `ERR`, BATT 처리. CAN3가 실장·enable되는 variant에서는 `ERR` 감시를 mandatory로 둠
9. hard TX gate, service jumper, state sense, test point
10. 4 Mbps UART series resistor/eye 측정 계획
11. HSE 값·load capacitor·FDCAN timing 계산
12. programming/SWD/USB와 ESP/STM 독립 reset·J31 재삽입·stale ARM 상호작용, vehicle rail이 켜진 상태에서 USB VBUS 역급전 방지

R1 전원 정책은 [ADR-006](../adr/006-compact-hardware-power-and-sensors.md)에 따라 **외부에서 차단되는 fused IGN/ACC 입력**이며 별도 상시 BAT+·ACC sense/CAN wake 회로는 없다. 상시 BAT+ harness를 무조건 허용하지 않는다. USB-C는 MCU service 전원을 공급하지만 CAN PHY/GPS rail과 mux로 분리된다. 차량 전압/PHY reset이 유효하지 않으면 RX/TX gate가 닫히고 회복 뒤에도 새 ARM edge가 필요하다. OFF1mA 요구는 상시전원 variant의 별도 acceptance이며 현 회로가 달성했다고 주장하지 않는다. SWD VTref는 target 출력 전용이다. bench12V는 동일 보호 입력을 사용한다.

MAX20040 pin4 내부VCC와外부PGOOD pull-up rail(PHY3V3), EN, MAX3055 BATT/WAKE/INH는 R1 pinmap에서 확인한다. PROTECTED_VBAT UV comparator와 실제5V supervisor가 MAX3055 EN/TXD permit을 차단한다. threshold의 정적 계산과 빠른 collapse 지연·ripple/overshoot 실제 검증을 구분하며, firmware BOD만으로 이 hardware 보호를 대체하지 않는다.

차량 connector pin과 bus type을 모르면 회로도에는 `UNASSIGNED_VEHICLE_BUS_n`으로 남기고 harness adapter에서 매핑한다. 이름만 보고 C-CAN/M-CAN에 연결하지 않는다.

## 12. Controller model과 UI 계약

현재 protocol quality와 UI quality의 숫자값이 다르므로 cast를 금지한다. `canview_model_adapter`가 exhaustive switch로 변환하고 unknown 값은 `UNAVAILABLE/FAULT`로 처리한다.

모든 차량 표시값은 다음 metadata를 가져야 한다.

```c
typedef struct {
    uint32_t model_revision;
    uint32_t source_state_revision;
    uint32_t sample_time_ms;
    uint16_t age_ms;
    uint8_t quality;
    uint8_t evidence_grade;
} canview_value_meta_t;
```

속도, RPM, 제한속도, drive mode, 배터리 전압, clutch lock, 온도, 순간연비, 4WD, TPMS, DPF도 예외가 아니다. UI는 stale 값을 마지막 숫자로 고정하지 않는다.

FFT 값은 unsigned dB가 아니다. calibration 전에는 `int16_t peak_tenth_dbfs`, `int16_t level_tenth_dbfs`, `valid`, `clipped`, `calibrated`를 사용한다. SPL calibration을 완료한 경우에만 dB SPL로 이름과 단위를 바꾼다. FFT bin은 표시용 23개와 분석용 원본 bin을 분리한다.

기존 화면 요구는 유지한다.

- 기본 순서: 4WD → 순간연비 → DPF → 작은 속도/RPM
- 차량 중앙 순간연비, 네 바퀴 torque/drive gauge와 TPMS
- 모든 화면에 현재 속도, 제한속도 overlay
- 메인 화면 과속 경고는 중앙 크게, 다른 touch 화면은 pointer-transparent 반투명
- 제한속도보다 10% 이상 빠를 때 깜빡이며 touch를 가로채지 않음
- SPORT red, NORMAL blue, ECO green, 별도 상태 원형 장식 없음
- volume control button과 임의 sound-position UI 없음; 현재 volume만 표시
- Cabin FFT 안에 peak와 level, FFT 상세 화면은 세로 비중 확대
- 미등 신호로 night mode, 30초 기본 유휴 시 감광·기본 화면 복귀, touch로 복원

animation은 model sample을 직접 튀겨 그리지 않고 시간 기반 보간을 쓴다. UI update 20 Hz와 LVGL render/animation tick을 분리하고, 화면 frame miss가 누적되면 장식 animation부터 줄인다.

### 12.1 자동화 입력 freshness와 명령 직렬화

자동화 함수는 raw 숫자만 받지 않는다. 모든 입력에 quality, evidence, sample time/age와 source revision을 전달한다. 기본 release ceiling은 차량 speed/gear/brake/stability/drive-mode 300 ms, FFT feature 250 ms, audio feedback 500 ms다. vehicle profile은 더 짧게 만들 수 있지만 새 evidence 없이 이 상한을 늘릴 수 없다.

- 필수 입력이 `VALID + VERIFIED`가 아니거나 age ceiling을 넘으면 attack/release/entry/exit dwell을 누적하지 않고 해당 dwell을 0으로 초기화한다.
- scheduler tick의 정상 `elapsed_ms`는 최대 100 ms만 누적한다. 호출 간격이 250 ms를 넘거나 monotonic discontinuity가 있으면 모든 dwell을 초기화하고 그 tick에서 명령을 만들지 않는다. 긴 task stall 한 번으로 조건이 충족되어서는 안 된다.
- 속도 경고도 speed와 limit metadata를 모두 검사한다. 둘 중 하나가 stale이면 overlay를 지우고 진단 상태만 남긴다.
- adaptive volume은 automation당 한 개의 pending token만 허용한다. terminal result와 matching audio feedback snapshot이 모두 온 뒤에만 다음 step을 판단하며, applied offset은 feedback으로만 바꾼다.
- FFT/speed가 사라지면 새 volume-up은 즉시 금지한다. 2초 연속 invalid 뒤에도 link, lease와 valid audio feedback이 있으면 baseline 방향 한 step만 요청하고 다시 feedback을 기다린다. 그 조건도 없으면 차량 명령을 만들지 않는다.
- SPORT는 필수 safety 입력 하나라도 stale이면 새 button pulse를 만들지 않는다. 이미 SPORT여도 stale 신호만으로 이전 mode pulse를 추정 전송하지 않는다.
- pending timeout, peer/STM reboot, session 또는 time-sync generation 변경은 pending을 실패로 닫고 current snapshot 재동기화 전까지 같은 자동화를 inhibit한다.
- 운전자의 OEM 조작은 모든 automation pending보다 우선하고 manual-hold를 시작한다.

## 13. 설정 owner routing

| 설정/동작 | owner | 저장 | wire 경로 |
|---|---|---|---|
| RTC 시각·날짜 | Controller | config A/B intent + PCF85063 readback | local UI 또는 Bridge↔Controller remote config |
| 일출·일몰·전조등 경고 | Controller | Controller config A/B | local/confirmed remote config |
| 화면 밝기·자동 밝기·유휴 timeout | Controller | Controller config A/B | local/confirmed remote config |
| FFT band·민감도·반응·최대 offset | Controller | Controller config A/B | local/confirmed remote config |
| local RX allow-list | Controller | Controller config A/B | local config; upstream subscription 별도 동기화 |
| peer subscription·quota | Communicator ESP32 | config A/B default + session별 RAM effective | peer `CAN_FILTER_*` transaction |
| control lease·peer role/scope | Communicator ESP32 + STM mirror | RAM, pairing record | ESP-NOW + UART semantic state |
| SPORT automation threshold/arm | STM32 | threshold는 config A/B, arm은 RAM | Controller command/config → Communicator → UART |
| vehicle audio snapshot/profile | STM32 | RAM + bounded recovery record | semantic command only |
| diagnostic capture/filter | Bridge + Communicator peer namespace | Bridge storage/Communicator RAM | diagnostic lease messages |

저장의 권위 영역·크기·trial/rollback 보존은 [OTA §4·5·9](ota.md)를 따른다. normal NVS는 비권위 cache이며 T-304/T-205가 versioned snapshot과 readback·전원 차단 복구를 연결한다. config Flash와 PCF85063은 하나의 원자 transaction이 아니므로 pending intent와 실제 RTC 판독을 조정하고 부분 성공을 숨기지 않는다. link/control root는 protected provisioning 정책을 따르며 일반 config/default 복원이나 OTA 때문에 session·control lease·SPORT arm을 복원하지 않는다.

RTC command ID와 Communicator config key를 현재 header에서 제거하는 migration은 protocol `1.3` 생성 task에서 한 번에 수행한다. 중간 호환 alias를 만들지 않는다.

## 14. Diagnostic Bridge와 `.cvtrace`

`.cvtrace` v1은 zip container로 고정하고 최소 다음 entry를 갖는다.

```text
manifest.json              # schema/version/digests/device/vehicle/bus/time/drop/gap
frames.bin                 # fixed endian record stream
markers.jsonl              # marker kind, local/remote time, uncertainty
inventory.json             # ID/DLC/rate/change-mask summary
candidates.json            # candidate descriptors and evidence links
README.txt                 # 사람이 읽는 짧은 설명, 비정본
```

`manifest.json`은 protocol version, profile ID, DBC SHA-256, 각 device/boot ID, capture ID, logical bus mapping, start/end RTC와 monotonic time, time uncertainty, filter revisions, accepted/dropped/gap count, tool version을 포함한다. importer는 고정 entry allow-list, path traversal·symlink 금지, entry 수·개별 크기·전체 uncompressed 크기·compression ratio 상한을 먼저 확인한다. validator가 digest·record count·bounds를 모두 확인하기 전에는 generator 입력으로 쓰지 않는다.

bundle 내부 SHA-256은 무결성만 보여 주고 출처를 인증하지 않는다. `VERIFIED` 승격에는 validator를 통과한 bundle digest를 별도 `evidence-approval.json`에 기록하고 신뢰된 maintainer key의 Ed25519 signature로 승인한다. private signing key는 Bridge와 repository에 저장하지 않는다. 서명되지 않은 bundle은 candidate 분석에는 쓸 수 있지만 command/profile 승격 근거가 될 수 없다.

capture 중에는 ZIP에 직접 쓰지 않는다. 4 KiB append-only journal block은 magic `CVJB`, format version, block sequence, used length와 CRC-32를 가지며 마지막 valid block까지만 복구한다. finalize 때 `frames.bin`을 만들고 마지막에 manifest와 ZIP central directory를 commit한다. `frames.bin`은 8-byte magic `CVFRAME1`, little-endian version/record-size header 뒤에 고정 24-byte FRAME/GAP record를 둔다. complete되지 않은 복구물은 `partial=true`와 명시적 GAP/end reason을 가진 별도 export만 허용하고 VERIFIED evidence 입력은 될 수 없다.

internal flash admission은 `required = 64 KiB journal/meta reserve + ceil(rate_limit × 24 × duration × 1.15)`로 보수 계산한다. `available = min(capture_partition_free - 256 KiB emergency_reserve, 1.5 MiB per capture)`이며 required가 available보다 크면 시작 전에 거부한다. SD가 없는 200 record/s capture는 최대 180초로 제한하고 10분 `ARMED_DRIVE`는 검증된 SD/외부 저장공간이 있을 때만 허용한다. active/pinned/exporting capture는 cleanup 대상이 아니다.

Diagnostic REST API는 OpenAPI에서 생성·검증하며 operation을 비동기 resource로 표현한다. HTTP `202`는 요청 접수일 뿐 capture 완료가 아니다. WebSocket은 snapshot revision과 event sequence를 포함하고 reconnect 시 REST snapshot 이후의 event만 적용한다.

## 15. 초기 성능·자원 예산

측정 전 출발값이며 task 수용 기준에서 더 엄격하게 바꿀 수 있다. 조용히 완화할 수는 없다.

| 대상 | 기준 |
|---|---|
| STM32 SRAM | `.data + .bss <= 80 KiB`, worst stack 합계 `<= 24 KiB`, 최소 24 KiB margin |
| STM32 safety tick | 10 ms 주기, p99 실행 1 ms 이하, deadline miss 시 TX inhibit |
| STM32 FDCAN ISR | frame copy·timestamp만, p99 50 µs 이하 |
| UART | 방향당 정상 payload 260 kB/s 이하, control p99 enqueue-to-wire 20 ms 이하 |
| Communicator ESP internal heap | online idle free 80 KiB 이상, largest block 32 KiB 이상 |
| Controller internal heap | UI online free 96 KiB 이상, largest block 48 KiB 이상 |
| Controller PSRAM | 정상 UI+FFT에서 free 1 MiB 이상 |
| Bridge PSRAM | capture off free 512 KiB 이상, raw ring 기본 512 KiB 이하 |
| telemetry latency | STM timestamp→Controller model p95 100 ms, p99 200 ms 이하 |
| UI | animation 60 Hz 목표, frame p95 16.7 ms, p99 25 ms, 50 ms 초과 0.1% 미만 |
| ESP-NOW | 전체 sustained 20 kB/s, 1초 burst 32 kB, P0/P1 2 kB/s reserve |
| Bridge HTTP bulk | 정차·control lease 없음에서 64 kB/s, 이동/lease/deadline miss에서 0 |
| command ACK | 정상 RF에서 p95 150 ms 이하; 완료는 command별 feedback timeout 적용 |

24시간 soak에서 queue high-water, heap 최소값, stack watermark, loss/retry, bus error를 artifact로 남긴다.

## 16. 검증 gate

| gate | 허용되는 다음 단계 | 필수 증거 |
|---|---|---|
| G0 계약 | firmware feature coding | schema-generated header, C/Python golden vector, host sanitizer, CI green |
| G1 board boot | bench 통신 | 회로/ERC/BOM, reset/power rail scope, hard gate off 증거, 각 firmware boot |
| G2 bench read-only | 차량 capture-only | UART 24 h, 두 ESP RF fault test, 3-channel CAN simulator, TX 0 증거 |
| G3 차량 capture-only | signal 승격 | 별도 ignition cycle capture, bus mapping/bitrate, drop/gap report |
| G4 bench TX | 제한된 폐쇄시험 | VERIFIED command profile, hard gate, HIL feedback/fault matrix, audit log |
| G5 폐쇄시험 | 기능별 opt-in | 한 기능씩 실제 button 비교, rollback, 운전자 override, no unintended frame |
| G6 release | 일반 설치 후보 | security provisioning, brownout/soak/thermal, signed artifacts, release manifest |

G3까지 모든 차량 연결 build는 `CAPTURE_ONLY`다. DPF 강제 regeneration은 어느 gate에서도 범위에 포함하지 않는다.

## 17. critical path와 병렬화

의존 DAG의 정본은 [task backlog](../tasks.md#4-critical-path)와 상세 task의 `선행`이다. 이 문서에 다른 순서의 그래프를 복제하지 않는다. 읽기 전용 `tools/validate_plan.py`는 개수·ID·상태·요약·선행 참조·사이클을 검사하되 실물/evidence gate를 자동 통과시키지 않는다.

T-500은 공용 rig/시나리오 API를 먼저 제공하고 T-103/T-101 등 각 소비 task가 실제 firmware·하드웨어 시험을 완료한다. T-101용 최소 boot image 제공과 T-102/T-200의 전체 완료를 혼동하지 않는다. T-503/T-505a의 수신 조사 → T-106 executor → T-503a/T-505 bench/폐쇄시험 순서로 실제 source evidence와 주입 검증을 분리한다.

[OTA §12](ota.md)의 8단계는 T-007/T-204/T-107/T-108/T-306/T-205/T-507/T-508에 대응한다. 각 장치는 독립 AP/복구를 제공하며 개발 DAG의 선행은 런타임 Controller/Bridge 의존성을 뜻하지 않는다. OTA 변경 firmware의 차량 연결 전 작업대 gate와 기능별 G4/G5 승인은 별개다.

## 18. 구현 준비 완료 정의

어떤 task도 아래 질문에 답하지 못한 채 `READY`로 표시하지 않는다.

- 입력과 출력의 owner가 누구인가?
- byte layout, enum, unit, time domain, revision owner가 정해졌는가?
- callback/ISR와 worker 경계, queue overflow 동작이 정해졌는가?
- power loss, reboot, duplicate, stale, partial apply에서 상태가 어디로 수렴하는가?
- 차량 송신이 가능한 코드인가? 그렇다면 build/hardware/profile gate는 무엇인가?
- 자동화할 수 있는 수용 시험과 사람이 확인할 evidence artifact는 무엇인가?
- rollback은 무엇이며 이전 설정·OEM 상태를 어떻게 확인하는가?

이 항목이 빠졌으면 구현자가 합리적으로 추측하지 말고 해당 task를 `BLOCKED`로 바꾸고 설계 결정을 먼저 추가한다.
