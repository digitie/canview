# Controller CAN 수신·DBC 파이프라인

이 문서는 [아키텍처 개요](README.md)에 속한 Controller 수신·filter·decode 상세 정본이다. wire 자체는 [protocol index](protocols/README.md), 차량별 승격 근거는 [signal catalog](../vehicle/signal-catalog.md)에서 관리한다.

## 1. 책임 배치

CAN의 물리 수집과 Controller의 해석을 분리한다.

```text
차량 CAN 1/2/3
      │
      ▼
STM32G474 ── raw CAN batch ── ESP32 Communicator
                                      │
                                      └─ ESP-NOW
                                             │
                                             ▼
                                Controller 수신 필터
                                             │
                                             ▼
                              Controller DBC catalog/decoder
                                             │
                                             ▼
                                      LVGL model/UI
```

| 계층 | 책임 |
|---|---|
| STM32 Communicator | FDCAN 3채널 수집, timestamp, bus/error 상태, 차량 송신 안전 gate |
| ESP32 Communicator | UART·ESP-NOW bridge, session/auth, raw batch queue, 재전송·우선순위 |
| Controller | 허용 목록, raw batch admission, DBC signal decode, freshness/quality, 화면 모델 |

Communicator에는 화면용 DBC 의미를 넣지 않는다. 기존 ID 안에서 새 signal을 추가하거나, 다른 차량용 catalog를 선택하거나, candidate의 factor/bit layout을 바꿀 때는 Controller의 catalog/decoder만 배포하면 된다. 새 CAN ID를 사용하려면 Controller의 필터를 먼저 추가한다. 이때도 Communicator firmware는 변경하지 않는다.

실차에서 미확정 ID를 찾고 bit layout을 바꿔 보는 과정은 별도 `Diagnostic Bridge`와 휴대폰 웹 `Signal Lab`이 담당한다. Bridge도 Communicator의 raw record 계약을 사용하며 DBC별 코드를 Communicator에 추가하지 않는다. 전체 구조, 행동 전후 capture, candidate/evidence 형식은 [Diagnostic Bridge·모바일 CAN 검증 UI](diagnostic-bridge.md)를 따른다.

raw CAN 자체는 차량 제어 명령이 아니다. Controller 수신 필터는 **수신량을 제한하는 보안·대역폭 경계**이며, raw record를 차량에 다시 송신하는 권한을 만들지 않는다.

## 2. 이중 허용 경계

1. ESP-NOW peer/session/auth 검증: 올바른 설치 peer와 online session만 처리한다.
2. Controller local CAN record allow-list: session이 맞아도 필터에 일치하지 않는 record는 Controller model queue에 넣지 않는다. Communicator의 peer별 upstream subscription은 대역폭 절감용 별도 store이며 이 local 신뢰 경계를 대신하지 않는다.

기본 상태는 default-deny다. 저장된 필터가 없거나 모든 필터가 disabled이면 raw CAN은 하나도 통과하지 않는다. 필터를 통과한 값도 DBC descriptor의 `quality`, sample age, catalog revision을 함께 가지고 있어야 UI에 표시할 수 있다.

## 3. 필터 규칙

필터 한 개의 매칭은 다음 식을 따른다.

```text
(record.can_id & can_id_mask) == (filter.can_id & can_id_mask)
AND bus_id == filter.bus_id 또는 filter.bus_id == 0xFF
AND (record.flags & flags_mask) == (flags_value & flags_mask)
AND min_dlc <= record.dlc <= max_dlc
```

| 항목 | 정책 |
|---|---:|
| Controller 필터 슬롯 | 최대 32개 |
| 한 번에 설정할 필터 | 최대 8개 |
| 필터 주기 제한 | 20 ms–60 s |
| 필터별 record quota | 1–32 record/필터 주기 |
| 전체 raw record quota | 기본 32 record/100 ms |
| 전체 raw byte quota | 기본 20,000 byte/s, 최대 20,000 byte/s |
| period burst quota | 기본 512 byte/stream period, byte/s quota와 함께 적용 |
| 단일 record wire 크기 | 16 byte, classic CAN data 8 byte 보존 |
| ESP-NOW 전체 frame | 240 byte 이하 |

record quota, period burst quota, byte/s quota를 모두 만족해야 admission한다. 한쪽이 가득 차면 해당 record를 버리고 `dropped_by_budget`와 `rejected_records`를 증가시킨다. 필터 불일치도 default-deny drop으로 계산하지만 budget drop과 구분한다. `burst_bytes`는 configured stream period 안에서 허용하는 raw record byte 상한이고, `max_bytes_per_second`는 별도 1초 창이다.

기본 raw batch는 `canview_can_batch_header_t` 12 byte와 record 12개(192 byte)로 payload 204 byte를 사용한다. ESP-NOW 고정 header를 포함해 236 byte이며 240 byte 상한 아래다. 실제 transport는 batch가 작더라도 현재 session의 협상된 frame limit을 다시 적용한다.

## 4. ESP-NOW 필터 관리 메시지

공통 header의 `message_type`에 다음 값을 추가했다.

| 값 | 메시지 | 방향 | 의미 |
|---:|---|---|---|
| `0x23` | `CAN_FILTER_GET` | peer ↔ Communicator | 해당 peer의 upstream subscription snapshot 조회/응답 |
| `0x24` | `CAN_FILTER_SET` | peer ↔ Communicator | peer namespace의 ADD/REPLACE/DELETE/CLEAR 요청 |
| `0x25` | `CAN_FILTER_RESULT` | Communicator → peer | subscription revision, filter id, action, 결과와 effective quota |
| `0x26` | `CAN_STREAM_CONFIG` | peer ↔ Communicator | peer별 upstream period/count/byte budget 요청·수락값 |
| `0x27` | `CAN_STREAM_STATUS` | Communicator → peer | 해당 peer stream의 허용·거부·budget drop 통계 |

아래 크기는 현재 `1.2` draft의 migration 입력일 뿐 runtime 정본이 아니다. 실제 wire layout은 [T-002](../tasks/T-002-espnow-schema-v1.3.md)의 schema와 생성 [`canview_protocol.h`](../../protocol/canview_protocol.h)가 함께 동결된 뒤 확정한다.

- `canview_can_filter_t`: 22 byte
- `canview_can_filter_batch_header_t`: 8 byte
- `canview_can_filter_get_payload_t`: 8 byte
- `canview_can_filter_result_payload_t`: 12 byte
- `canview_can_stream_config_t`: 16 byte
- `canview_can_stream_status_t`: 16 byte

`CAN_FILTER_SET`은 batch header 뒤에 filter entry를 붙인다. `action`은 한 batch에서 하나만 사용한다. `ADD`, `REPLACE`, `DELETE`는 1–8개이고 `CLEAR`는 count 0만 허용한다. `config_revision`이 해당 peer namespace의 현재 subscription revision과 다르면 아무 entry도 적용하지 않고 conflict를 반환한다. 초기 provisioning에서만 revision 0을 “현재값 무관”으로 허용할 수 있으며 운행 중에는 exact match를 사용한다. Communicator는 이를 raw stream 조건으로만 취급하고 DBC 의미를 해석하지 않는다.

Controller local allow-list는 별도 `controller_config_revision`과 [OTA config A/B snapshot](ota.md)을 권위 저장으로 사용한다. NVS는 비권위 cache다. 축소·삭제는 local deny를 먼저 적용한 뒤 upstream을 갱신하고, 확대·추가는 local staging 후 upstream 수락과 matching catalog digest가 있을 때만 effective로 승격한다. 두 store가 어긋나도 local allow-list보다 넓은 frame을 decode하지 않는다.

`CAN_FILTER_GET`의 전체 조회는 최대 8개 entry 단위로 나누며 각 result가 `snapshot_id`, `subscription_revision`, `part_index`, `part_count`, `total_count`를 가진다. 모든 part가 같은 snapshot/revision으로 모이기 전에는 표시·적용하지 않고, 손실·중복·조회 중 변경에서는 전체를 폐기한다. 조회 결과는 설정을 적용하지 않는다. filter entry의 `reserved0`, batch header의 `reserved_le`, stream config의 `reserved_le`은 반드시 0이어야 한다.

## 5. Controller catalog와 범용 decoder

Controller는 DBC에서 다음 descriptor를 생성하거나 런타임 catalog로 선택한다.

```text
signal_id, bus_id, can_id, can_id_mask
start_bit, bit_length, byte_order(Intel/Motorola), signedness
factor, offset, optional min/max, value_type, quality
```

decoder는 classic CAN record의 0–8 byte data에서 Intel little-endian과 Motorola big-endian bit field를 추출하고 signed extension 후 `physical = raw × factor + offset`을 계산한다. descriptor와 record의 bus/ID가 맞지 않으면 decode하지 않는다. 범위를 벗어나면 값을 버리지 않고 `OUT_OF_RANGE` quality로 바꾸며, stale·unavailable·unverified는 transport에서 보존한다.

새 signal을 추가하는 절차는 다음과 같다.

1. `dbc/opendbc` 원본과 대상 차량 적용 등급을 catalog 항목에 기록한다.
2. 기존 허용 ID이면 Controller catalog descriptor만 추가·교체한다.
3. 새 ID이면 `CAN_FILTER_SET(ADD)`로 Controller filter를 추가한다.
4. catalog revision과 digest를 바꾸고 이전 revision의 decoded value를 섞지 않는다.
5. 실차 검증 전에는 evidence를 `CANDIDATE`로 유지해 Signal Lab에만 보내고, 운전자 화면은 `VALID + VERIFIED`가 아니면 `—`를 표시한다.

Diagnostic Bridge에서 export한 `.cvtrace`를 근거로 작업하는 에이전트는 다음 순서를 지킨다.

1. manifest의 protocol, vehicle profile, bus, filter, drop/gap, DBC digest를 먼저 확인한다.
2. candidate가 참조한 capture와 marker 반복 횟수를 확인한다.
3. Bridge generic decoder와 Controller decoder에 같은 raw vector를 넣어 값이 일치하는지 확인한다.
4. `OBSERVED/B`까지는 candidate catalog에서 시험할 수 있지만 `VERIFIED/A` 조건이 없으면 운전자 화면의 정상값으로 승격하지 않는다.
5. Controller catalog 변경과 함께 source capture ID, bit numbering, endian, scale, range, stale 기준을 문서화한다.
6. 새 ID이면 Controller filter와 stream budget 영향도 함께 검토한다. Communicator에 signal 이름별 분기문을 추가하지 않는다.

Communicator는 signal name, factor, UI label을 알 필요가 없다. 새 차량이나 새 DBC가 들어와도 raw `bus_id`, arbitration ID, flags, DLC, data, timestamp를 그대로 전달하는 계약만 유지한다.

## 6. 명령 응답 확인

명령은 두 단계의 확인을 가진다.

```text
전송
  │
  ├─ transport callback 성공       : 무선 MAC 전달만
  ├─ ACK(ACCEPTED/DUPLICATE)        : 수신·검증·queue 등록
  ├─ COMMAND_RESULT(EXECUTING)     : 실제 처리 시작
  └─ COMMAND_RESULT(COMPLETED)     : feedback까지 확인한 최종 성공
```

Controller command tracker는 최대 8개의 pending token을 관리한다.

- 현재 tracker는 RTT 측정값이 없으므로 ACK 재전송 대기를 80 ms에서 시작해 160 ms로 늘리고, 250 ms와 `ttl_ms/2` 중 작은 값을 상한으로 둔다. 추후 측정 RTT를 넣더라도 이 상한은 유지한다.
- 재전송은 ACK를 받지 못한 경우에만 최대 2회다. 같은 64-bit `request_token`을 유지한다. 완료·실패·만료 슬롯은 새 pending 명령에 재사용할 수 있고, 중복 제거 이력은 별도 cache가 보존한다.
- `ACK(DUPLICATE)`는 실패가 아니며 기존 결과를 기다린다.
- `COMMAND_RESULT(COMPLETED)`가 ACK보다 먼저 오면 완료를 유지한다. 뒤늦은 ACK가 완료를 `WAITING_RESULT`로 되돌리면 안 된다.
- `FAILED`, `REJECTED`, `EXPIRED`는 terminal 상태다. 늦게 도착한 성공 결과로 되살리지 않는다.
- session 또는 Communicator `boot_id`가 바뀌면 pending command와 control lease를 폐기한다. 자동 재실행하지 않는다.
- 수신 endpoint는 `request_token + command_id + payload digest`를 최소 60초 또는 256건 보존한다. 같은 digest는 저장된 result를 재전송하고, 같은 token의 다른 payload는 실행하지 않는다.

ACK만으로 UI의 차량 상태를 바꾸지 않는다. 상태 snapshot 또는 검증된 feedback signal을 받은 후 model을 commit한다. raw CAN batch가 유실되어도 command를 raw frame 재생으로 보상하지 않는다.

## 7. 구현 위치와 시험

핵심 구현은 다음에 있다.

- [`canview_controller_can.h`](../../firmware/controller/components/canview_can/include/canview_controller_can.h): filter store, stream budget, generic descriptor API
- [`canview_controller_can.c`](../../firmware/controller/components/canview_can/canview_controller_can.c): default-deny admission과 quota
- [`canview_controller_signal.c`](../../firmware/controller/components/canview_can/canview_controller_signal.c): endian/signed/scale decoder
- [`canview_command_tracker.c`](../../firmware/controller/components/canview_can/canview_command_tracker.c): ACK/result 상태기계와 재시도

최소 시험 항목은 다음과 같다.

1. empty store에서 모든 record가 거부되는지 확인
2. exact/masked ID, bus wildcard, flags, DLC 조건 확인
3. 필터별 period/count와 전체 byte budget이 각각 독립적으로 제한되는지 확인
4. reserved/non-boolean wire 값과 stale config revision 거부
5. Intel/Motorola, signed, scale/offset, out-of-range decoder golden vector
6. duplicate ACK, result-before-ACK, late result, token collision과 reboot 처리
7. 세 CAN bus 고부하에서 P1 command queue가 raw telemetry drop보다 우선하는지 확인
8. Bridge candidate decoder와 Controller decoder가 같은 golden raw frame에서 같은 값을 내는지 확인
9. gap/drop이 있는 capture가 자동으로 `VERIFIED`로 승격되지 않는지 확인
