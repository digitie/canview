# Communicator 내부 UART 프로토콜

이 문서는 [protocol index](README.md)에 속한 Communicator 내부 MCU 간 계약의 상세 정본이다. 외부 ESP-NOW frame을 그대로 터널링하지 않는다.

## 1. 목적과 경계

이 프로토콜은 한 Communicator PCB 안의 `ESP32-S3-MINI-1-N4R2`와 `STM32G474CEU6`를 연결한다. Controller와 Communicator 사이의 ESP-NOW wire protocol과는 별도다.

- ESP32는 무선 link, pairing, Controller session, configuration, raw telemetry bridge를 담당한다.
- STM32는 세 FDCAN, timestamp, bus 상태, 안전 신호, 차량 TX 최종 허용을 담당한다.
- Controller의 표시용 DBC catalog와 signal decoder는 이 UART 경계 밖의 Controller에 둔다. ESP32와 STM32는 signal name이나 scale을 해석하지 않는다.
- ESP-NOW frame을 UART로 그대로 tunnel하지 않는다. 경계마다 길이·권한·상태를 다시 검증한 semantic message만 전달한다.

## 2. 물리·UART 설정

| 항목 | 값 |
|---|---|
| topology | PCB 내부 point-to-point, full duplex |
| logic | 3.3 V CMOS, non-inverted |
| baud | `4,000,000` baud |
| frame | 8 data bits, no parity, 1 stop bit (`8-N-1`) |
| flow control | hardware RTS/CTS, 양방향 |
| ESP peripheral | UART1 |
| STM peripheral | USART2 |
| framing | COBS encoded packet + `0x00` delimiter |
| integrity | CRC-32/ISO-HDLC |
| raw packet limit | 1,024 byte, header 포함 |

배선은 다음처럼 교차한다.

```text
ESP GPIO17 UART1_TX  ─────────> STM PA3 USART2_RX
ESP GPIO18 UART1_RX  <───────── STM PA2 USART2_TX
ESP GPIO15 UART1_RTS ─────────> STM PA0 USART2_CTS
ESP GPIO16 UART1_CTS <───────── STM PA1 USART2_RTS
GND                  ────────── GND
```

RTS와 CTS 네트마다 각각 3.3 V 방향 10 kΩ pull-up을 둔다. 어느 MCU가 reset 또는 무전원이어도 상대방이 보는 CTS가 high가 되어 송신 금지 상태로 시작한다. TX/RX에는 22–47 kΩ weak pull-up footprint를 마련하되 기본 장착값은 4 Mbps eye 측정으로 정한다. 각 송신기 가까이 series resistor footprint도 둔다.

각 수신기는 자신의 buffer 여유를 RTS로 알리고, 반대쪽 송신기는 CTS가 허용할 때만 전송한다. RTS/CTS polarity는 두 MCU의 hardware peripheral 기본 active-low 동작을 사용하며 firmware에서 임의 invert하지 않는다. idle level만 보고 link가 연결됐다고 판단하지 않고 양쪽 `LINK_HELLO`와 새 `boot_id`를 확인해야 한다.

4 Mbps, 8-N-1의 line payload 상한은 start/stop bit를 포함해 방향당 약 400 kB/s다. protocol은 정상 운용률을 65% 이하인 260 kB/s로 잡아 flow-control 지연, control message, burst를 위한 여유를 둔다.

## 3. packet framing

전송 순서는 다음과 같다.

1. 32 byte header를 little-endian으로 작성한다.
2. `crc32` field를 0으로 두고 header와 payload의 CRC-32/ISO-HDLC를 계산한다.
3. 계산값을 header에 기록한다.
4. header+payload 전체를 COBS encode한다.
5. packet 경계 byte `0x00`을 하나 붙인다.

수신기는 `0x00`까지 최대 1,029 encoded bytes만 수집한다. 초과하면 다음 delimiter까지 버리고 `FRAME_OVERSIZE`를 증가시킨다. 빈 frame은 keepalive로 쓰지 않고 무시한다.

### 3.1 고정 header

| offset | 크기 | field | 설명 |
|---:|---:|---|---|
| 0 | 2 | `magic` | wire bytes `43 55`, ASCII `CU` |
| 2 | 1 | `major` | 호환 불가능한 변경 |
| 3 | 1 | `minor` | additive 변경 |
| 4 | 1 | `message_type` | message 종류 |
| 5 | 1 | `flags` | ACK, RESPONSE, ERROR 등 |
| 6 | 2 | `header_len` | v1은 32 |
| 8 | 2 | `payload_len` | v1 최대 992 |
| 10 | 2 | `reserved` | 송신 0, 수신 nonzero 거부 |
| 12 | 4 | `sequence` | 방향별 단조 증가 |
| 16 | 4 | `correlation_id` | 요청 sequence 또는 0 |
| 20 | 8 | `sender_time_us` | 송신 MCU boot 기준 monotonic time |
| 28 | 4 | `crc32` | decoded header+payload CRC |

COBS decode 뒤 실제 길이는 `header_len + payload_len`과 정확히 같아야 한다. packed struct cast 대신 명시적 little-endian reader를 사용한다.

### 3.2 flags

| bit | 이름 | 의미 |
|---:|---|---|
| 0 | `ACK_REQUIRED` | semantic ACK 필요 |
| 1 | `RESPONSE` | correlation_id의 응답 |
| 2 | `ERROR` | 정상 payload 대신 error |
| 3 | `HIGH_PRIORITY` | control queue 사용 |
| 4 | `SNAPSHOT` | delta가 아닌 전체 상태 |
| 5–7 | reserved | v1에서 0 |

## 4. message catalog

| 값 | 이름 | 방향 | QoS/용도 |
|---:|---|---|---|
| `0x01` | `LINK_HELLO` | 양방향 | protocol 범위, boot ID, build ID, capability |
| `0x02` | `LINK_HELLO_ACK` | 양방향 | 협상 결과와 reject reason |
| `0x03` | `HEARTBEAT` | 양방향 | 100 ms, queue/counter/watchdog 상태 |
| `0x04` | `ACK` | 양방향 | 문법·queue 수락 확인 |
| `0x05` | `ERROR` | 양방향 | rate-limited link/protocol 오류 |
| `0x10` | `CAN_RX_BATCH` | STM→ESP | raw record 최신값 우선, 재전송 없음 |
| `0x11` | `CAN_BUS_STATUS` | STM→ESP | bus-off/error counter/bitrate/listen-only |
| `0x12` | `SAFETY_SNAPSHOT` | STM→ESP | 명령 판단에 사용한 신호와 revision |
| `0x13` | `CAN_TX_AUDIT` | STM→ESP | 실행한 제한 명령의 결과와 feedback |
| `0x20` | `COMMAND_REQUEST` | ESP→STM | end-to-end request token, ACK required |
| `0x21` | `COMMAND_RESULT` | STM→ESP | accepted/executing/completed/rejected |
| `0x22` | `CONTROL_LEASE` | ESP→STM | Controller lease와 만료 시간 |
| `0x30` | `CONFIG_GET` | ESP→STM | 정차/service mode 한정 |
| `0x31` | `CONFIG_SET` | ESP→STM | allow-listed key만, ACK required |
| `0x32` | `CONFIG_RESULT` | STM→ESP | 적용 결과와 revision |
| `0x40` | `DIAGNOSTIC_COUNTERS` | 양방향 | UART/CAN/queue/watchdog counter |
| `0x50` | `FIRMWARE_PREPARE` | ESP→STM | 향후 확장 예약, v1에서는 미지원 |

알 수 없는 type은 length와 CRC가 정상인 경우에만 `UNSUPPORTED_MESSAGE`로 응답한다. 오류 응답에 다시 ACK를 요구하지 않는다.

### 4.1 UART protocol 1.0 Diagnostic Bridge 메시지

미확정 CAN ID를 signal별 firmware 수정 없이 찾기 위해 아래 generic message를 UART protocol의 첫 완전 구현인 `1.0`에 포함한다. ESP-NOW protocol `1.3`과 UART version은 서로 독립이다.

| 값 | 이름 | 방향 | 역할 |
|---:|---|---|---|
| `0x14` | `CAN_ID_STATS` | STM→ESP | bus/ID/DLC별 rate·period·change mask·last data |
| `0x15` | `CAN_OBSERVER_PLAN` | ESP→STM | 여러 ESP-NOW peer 요청을 합친 software filter·budget plan |
| `0x16` | `CAN_CAPTURE_CONTROL` | ESP→STM | arm/start/stop/cancel |
| `0x17` | `CAN_CAPTURE_STATUS` | STM→ESP | 실제 적용 revision, record/drop, 완료·오류 |
| `0x18` | `CAN_EVENT_MARKER` | ESP→STM | marker ID, 종류, sender time과 uncertainty |

현재는 UART header와 codec이 없으므로 어떤 version도 전송 가능한 구현 상태가 아니다. 정확한 payload, plan 원자 적용, diagnostic lease, peer별 재필터링은 [T-004](../../tasks/T-004-uart-schema-codec.md)와 [Diagnostic Bridge·모바일 CAN 검증 UI](../diagnostic-bridge.md)를 따른다. 이 확장은 DBC signal 이름이나 factor를 UART에 넣지 않는다.

## 5. CAN batch 형식

`CAN_RX_BATCH` payload는 공통 protocol의 `canview_can_batch_header_t`와 `canview_can_record_t` 배열을 사용한다. 내부 UART는 이 raw payload를 semantic message로 전달하며 ESP32가 DBC decode를 수행하지 않는다.

### 5.1 batch header

| field | 크기 | 설명 |
|---|---:|---|
| `base_time_us_le` | 8 | 첫 record의 STM monotonic time |
| `count` | 1 | record 수, ESP-NOW profile은 최대 12 |
| `dropped_since_last` | 1 | STM queue overflow로 제거된 raw record 수 |
| `reserved_le` | 2 | 0이어야 함 |

### 5.2 CAN record

| field | 크기 | 설명 |
|---|---:|---|
| `delta_us_le` | 2 | base time으로부터 0–65,535 us |
| `bus_id` | 1 | `0`, `1`, `2`가 논리 CAN1, CAN2, CAN3 |
| `flags_dlc` | 1 | 상위 nibble flags, 하위 nibble DLC |
| `can_id_le` | 4 | 11/29-bit ID |
| `data` | 8 | DLC 뒤 byte는 0으로 정규화 |

classic CAN record는 고정 16 byte다. v1은 0–8 byte data만 보존하며, CAN FD 12/16-byte DLC와 FD/BRS 의미는 별도 record type과 capability를 추가하기 전까지 거부한다. MAX3055 채널에서 FD/BRS flag는 항상 거부한다. ESP-NOW로 넘길 때도 [`canview_protocol.h`](../../../protocol/canview_protocol.h)의 16 byte layout을 그대로 사용한다.

## 6. queue와 backpressure

각 MCU는 최소 세 queue를 둔다.

| queue | 예 | overflow 정책 |
|---|---|---|
| safety/control | HELLO, command, result, bus fault | telemetry 때문에 제거하지 않음. 가득 차면 새 명령 거부 후 degraded |
| state | safety snapshot, bus status | 동일 key의 오래된 상태를 coalesce |
| raw telemetry | CAN batch, verbose diagnostic | 가장 오래된 raw batch부터 drop |

RTS/CTS는 byte buffer overflow를 막을 뿐 application queue의 의미적 우선순위를 해결하지 않는다. sender도 queue class를 분리하고 high-priority frame 사이에 큰 telemetry frame을 무제한으로 넣지 않는다.

CTS가 100 ms 이상 전송을 막으면 `FLOW_STALLED`로 표시하고 새 control request를 중단한다. 1초 이상 지속되면 link offline 처리한다. raw telemetry drop 수는 heartbeat에 포함한다.

## 7. 초기 연결 절차

```text
ESP32                                     STM32
  |                                         |
  |  reset: RTS/CTS pull-up = flow stop      | transceivers hardware-safe
  |  UART init, RTS not-ready               | HSE/FDCAN init, PHY standby
  |                                         | FDCAN init/listen-only
  |<--------- delimiter resync ------------>|
  |--------- LINK_HELLO -------------------->|
  |<-------- LINK_HELLO ---------------------|
  |<------ LINK_HELLO_ACK -------------------|
  |-------- LINK_HELLO_ACK ----------------->|
  |<------ SAFETY_SNAPSHOT ------------------|
  |-------- state digest -------------------->
  |<=========== ONLINE / heartbeat =========>|
```

`LINK_HELLO`에는 random `boot_id`, protocol min/max, hardware revision, firmware version, capability bitset, max packet, build ID를 넣는다. 양쪽 boot ID가 모두 새로 확인되고 major version이 맞을 때만 online이 된다.

STM32는 UART online 여부와 무관하게 reset 직후 transceiver standby를 유지한다. CAN bitrate/profile가 검증되면 listen-only 수신을 시작할 수 있지만 vehicle TX는 다음 조건을 모두 만족해야 한다.

1. UART online과 heartbeat 정상
2. STM32가 `control_root` tag로 직접 검증한 Primary Controller control lease와 origin/session/generation
3. STM32의 현재 vehicle state와 safety revision 일치
4. command ID와 parameter가 local allow-list에 존재
5. generated 필수 precondition을 admission/dequeue/각 TX 직전에 재검사
6. physical TX_ARM, rail-good와 외부 guardian permit 충족

Controller와 STM32는 UART/ESP 두 hop을 통과하는 `CONTROL_TIME_SYNC`로 별도 clock mapping을 만든다. mapping은 양쪽 boot ID와 generation에 묶이고 10초마다 갱신하며 30초 또는 uncertainty 50 ms 초과에서 invalid다. command의 origin, issued time, TTL, sync generation과 16-byte end-to-end control tag는 ESP32가 재작성하지 않고 opaque bytes로 전달한다.

## 8. 명령 신뢰성과 중복 제거

UART가 point-to-point라고 해서 command를 exactly-once transport로 가정하지 않는다.

- Controller가 만든 64-bit `request_token`을 ESP-NOW와 UART 경계에서 유지한다.
- STM32는 256개 entry에 live TTL과 terminal result를 최소 60초 보관한다. live entry는 LRU로 축출하지 않고 모두 차면 새 요청을 ACK 전에 `BUSY`로 거부한다.
- 같은 token·같은 payload는 실행하지 않고 기존 result를 다시 보낸다.
- 같은 token·다른 payload는 protocol/auth fault로 거부한다.
- ACK는 packet을 queue에 수락했다는 뜻이고 차량 feedback 완료는 `COMMAND_RESULT(COMPLETED)`만 뜻한다.
- ESP32 reboot, STM32 reboot, safety revision 변경 시 이전 pending command를 자동 재실행하지 않는다.
- ACK가 `ACCEPTED` 또는 `DUPLICATE`여도 완료로 확정하지 않는다. `COMMAND_RESULT(COMPLETED)`만 feedback 확인 후 최종 성공이다.
- result가 ACK보다 먼저 도착해도 terminal result를 유지하며, 늦은 ACK가 `WAITING_RESULT`로 되돌리지 않는다.
- cache key는 origin device/boot, wireless session, control generation, token, command ID와 canonical payload digest다. ACK와 RESULT는 token을 모두 포함한다.
- Primary Controller와 STM32만 가진 pair-specific `control_root` tag가 틀리면 UART frame CRC가 정상이어도 실행하지 않는다. terminal result도 STM32 tag로 Controller가 확인한다.

## 9. heartbeat와 오류 복구

Heartbeat 주기는 100 ms다.

| 상태 | 조건 | 동작 |
|---|---|---|
| `ONLINE` | 최근 heartbeat 300 ms 이내 | 정상 telemetry, 정책에 따른 command |
| `SUSPECT` | 300–1,000 ms | 새 command 거부, TX lease 폐기, 수신/진단 유지 |
| `OFFLINE` | 1,000 ms 초과 또는 boot ID 변경 | command queue 폐기, transceiver safe policy, HELLO 재협상 |

오류 처리는 다음과 같다.

- UART overrun, framing, noise, parity: hardware flag를 읽고 DMA/ring 상태를 복구한 뒤 delimiter까지 resync한다.
- COBS/length/CRC 오류: packet 한 개만 버리고 counter 증가. 오류 응답은 초당 10회 이하로 제한한다.
- sequence gap: telemetry는 loss로 기록하고 최신값을 계속 받는다. control은 snapshot/retry로 복구한다.
- 반복 CRC 오류: baud/clock/signal integrity fault로 `DEGRADED`; 자동 baud downgrade는 하지 않는다.
- CTS stuck: control 중단, service diagnostic에 pin level과 duration 노출.
- boot ID 변경: 모든 pending request와 lease를 폐기하고 HELLO부터 시작한다.
- queue overflow: raw telemetry를 먼저 버리며 safety/control overflow는 fail-closed 상태다.

한쪽 MCU가 상대 MCU를 무조건 reset하는 회로는 기본안에 넣지 않는다. watchdog reset loop가 차량 bus에 반복적으로 영향을 줄 수 있기 때문이다. 독립 watchdog과 transceiver safe pull resistor를 우선 사용하고, cross-reset은 prototype fault-injection 결과가 있을 때만 추가한다.

## 10. 확장성

- major가 다르면 online 진입을 거부한다.
- 같은 major에서 minor 차이는 capability bit와 payload length로 협상한다.
- payload의 확장 field는 TLV로 추가하고 unknown optional TLV는 건너뛴다.
- 필수 capability가 없으면 관련 기능만 비활성화하며 raw command로 우회하지 않는다.
- max packet은 양쪽 값의 최솟값을 사용하고 내부 UART v1 hard limit 1,024 byte를 넘지 않는다.
- firmware update message는 예약만 하고 v1에서 구현하지 않는다. STM32 update는 SWD/service mode부터 검증한다.

## 11. 시험 항목

1. 두 방향 4 Mbps 연속 PRBS/packet CRC 24시간
2. RTS/CTS를 임의 정지했을 때 byte loss와 recovery
3. random byte 삽입·삭제·bit flip 뒤 delimiter resync
4. 최대/초과 length, unknown type/TLV, sequence wrap
5. 세 CAN channel 최대 예상 load에서 control latency p99
6. ESP32/STM32 개별 reset과 brownout
7. duplicate command, stale revision, delayed result
8. telemetry queue saturation 시 safety/control 무손실 확인
9. CAN bus-off와 MAX3055 ERR fault가 ESP-NOW/UI까지 전달되는지 확인

## 12. 공식 근거

- [Espressif ESP32-S3 UART API](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/uart.html)
- [Espressif ESP32-S3 GPIO matrix](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/gpio.html)
- [ST STM32G474CE 데이터시트](https://www.st.com/resource/en/datasheet/stm32g474ce.pdf)

관련 observer/capture 구현 명세는 [Diagnostic Bridge·모바일 CAN 검증 UI](../diagnostic-bridge.md)에 있다.
