# CANView ESP-NOW 양방향 프로토콜

## 1. 문서 목적

이 문서는 최대 3개 차량 CAN 버스를 수집하는 **Communicator**와 `ESP32-S3-Touch-LCD-3.5` 기반 **Controller** 사이의 양방향 프로토콜을 정의한다. 단순 텔레메트리 전송뿐 아니라 최초 등록, 상호 인증, 기능 협상, 시간 동기화, 명령 확인, 오류 복구, 버전 확장을 포함한다.

이 문서의 `MUST`, `MUST NOT`, `SHOULD`, `MAY`는 각각 필수, 금지, 권고, 선택을 뜻한다. 현재 wire protocol 버전은 `1.0`이다. C 레이아웃 기준은 [`protocol/canview_protocol.h`](../protocol/canview_protocol.h)다.

설계 원칙은 다음과 같다.

1. 차량 CAN 송신 권한과 최종 안전 판단은 항상 Communicator가 가진다.
2. Controller는 raw CAN frame을 생성하지 않고 검증된 “의도 명령”만 요청한다.
3. ESP-NOW의 MAC 계층 성공을 애플리케이션 처리 성공으로 간주하지 않는다.
4. 텔레메트리 손실과 제어 명령 손실을 다른 QoS로 처리한다.
5. 연결이 불확실하면 표시값을 stale로 바꾸고 제어를 중단한다.
6. v1의 모든 일반 frame은 ESP-NOW v1/v2 공통 범위 안인 240 byte 이하로 제한한다.
7. 미확인 차량 신호는 wire protocol에서도 `UNVERIFIED` 품질을 보존한다.

## 2. 전제와 ESP-NOW 제약

Espressif ESP-IDF 5.5.2 문서 기준으로 ESP-NOW v1의 최대 payload는 250 byte, v2는 1,470 byte다. v2 장치는 v1 frame을 받을 수 있지만, v1 장치는 250 byte를 넘는 v2 frame을 truncate하거나 버릴 수 있다. CANView v1은 구현·버전 혼합과 진단 도구의 단순성을 위해 header와 payload를 합쳐 **240 byte**로 제한한다.

ESP-NOW는 CCMP를 사용하며 PMK와 peer별 LMK 길이는 각각 16 byte다. PMK를 설정하지 않으면 기본 PMK가 사용되고 LMK가 없으면 unicast frame도 암호화되지 않는다. multicast/broadcast 암호화는 지원되지 않는다. CANView production 모드는 기본 PMK와 평문 unicast를 금지한다.

송신 callback의 `ESP_NOW_SEND_SUCCESS`는 MAC 계층 수신만 뜻한다. 애플리케이션 수신·검증·실행은 보장하지 않으므로 별도 ACK, timeout, 재전송, sequence 기반 중복 제거가 필요하다. 송수신 callback은 Wi-Fi 고우선순위 task에서 실행되므로 callback에서는 길이와 MAC을 최소 검증한 뒤 고정 크기 queue에 복사하고 즉시 반환한다.

peer channel은 로컬 Wi-Fi channel과 같아야 한다. `channel=0`은 현재 channel을 사용한다. CANView는 운행 중 channel hopping을 하지 않으며, pairing과 안전한 복구 상태에서만 제한적으로 scan한다.

공식 근거:

- [ESP-IDF 5.5.2 ESP-NOW](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32s3/api-reference/network/esp_now.html)
- [ESP-IDF ESP-NOW 예제](https://github.com/espressif/esp-idf/tree/v5.5.2/examples/wifi/espnow)

## 3. 시스템 역할과 신뢰 경계

| 역할 | 책임 | 신뢰하지 않는 입력 |
|---|---|---|
| Communicator | CAN1–3 수집, timestamp, DBC decode, 송신 allow-list, control lease, 최종 안전 gate | Controller 명령, 무선 payload, 미검증 DBC |
| Primary Controller | 상태 표시, 사용자 입력, profile 요청, stale/error 표시 | Communicator가 보내는 값의 차량별 의미 |
| Read-only Controller | 추가 화면·정비 화면. 상태 수신만 허용 | 모든 제어 요청 |
| Provisioning host | USB를 통한 설치 secret·peer 초기화 | 무선 discovery |

Communicator는 최대 20개 peer라는 ESP-NOW 한도보다 훨씬 작은 운영 한도를 둔다. v1 권고값은 Primary Controller 1개, Read-only Controller 2개다. 여러 Controller가 등록돼도 control lease는 동시에 한 peer만 소유한다.

### 3.1 데이터 방향

```text
CAN1/2/3 -> Communicator -> CAN_BATCH / SIGNAL_BATCH / BUS_STATUS -> Controller
Controller -> COMMAND_REQUEST / CONFIG_* / LEASE_REQUEST -> Communicator
Communicator -> ACK + COMMAND_RESULT / ERROR / SNAPSHOT -> Controller
```

### 3.2 안전 경계

- 무선에서 임의 arbitration ID, DLC, data를 받아 차량에 그대로 송신하는 API를 만들지 않는다.
- drive mode·audio 제어는 Communicator firmware에 컴파일된 command ID와 차량 profile allow-list의 교집합만 허용한다.
- Controller는 자신이 알고 있는 상태 revision을 명령에 넣는다. Communicator 상태가 바뀌었으면 stale command를 거부한다.
- link가 `DEGRADED`가 되는 즉시 새 명령을 거부하고 control lease를 회수한다.
- 재부팅 뒤 이전 session의 command를 재실행하지 않는다.

## 4. 무선 frame

### 4.1 크기와 byte order

| 항목 | 값 |
|---|---:|
| 최대 frame | 240 byte |
| 고정 header | 32 byte |
| 최대 payload | 208 byte |
| byte order | little-endian |
| magic wire bytes | `43 56`, ASCII `CV` |
| CRC | CRC-32/ISO-HDLC |

수신 구현은 packed C struct로 buffer를 직접 cast하면 안 된다. 먼저 실제 길이가 32 byte 이상인지 확인하고, byte 단위로 header를 읽은 뒤 `magic`, `header_len`, `payload_len`, 전체 길이, reserved, version, CRC 순서로 검증한다. 정렬되지 않은 접근과 host endian 의존을 피하기 위해 `read_le16/32/64` helper를 사용한다.

CRC는 header의 `crc32` 4 byte를 0으로 둔 값과 payload 전체에 계산한다. CRC는 accidental corruption과 codec 구현 오류를 찾기 위한 것이며 인증 수단이 아니다. 보안은 CCMP와 pairing transcript HMAC이 담당한다.

### 4.2 고정 header

| Offset | 크기 | 필드 | 설명 |
|---:|---:|---|---|
| 0 | 2 | `magic` | `0x43 0x56` |
| 2 | 1 | `major` | 호환 불가능한 변경 시 증가 |
| 3 | 1 | `minor` | additive 변경 시 증가 |
| 4 | 1 | `header_len` | v1은 32 |
| 5 | 1 | `message_type` | 메시지 종류 |
| 6 | 1 | `flags` | ACK, response, fragment 등 |
| 7 | 1 | `priority` | 0이 가장 높음 |
| 8 | 4 | `session_id` | secure session 식별자 |
| 12 | 4 | `sequence` | 방향별 단조 증가 번호 |
| 16 | 4 | `sender_time_ms` | boot 기준 monotonic ms, wrap 허용 |
| 20 | 4 | `correlation_id` | 요청 sequence 또는 0 |
| 24 | 2 | `payload_len` | 0–208 |
| 26 | 2 | `reserved` | v1 송신은 0, 수신은 0이 아니면 거부 |
| 28 | 4 | `crc32` | header+payload CRC |

`session_id=0`은 discovery와 pairing frame에만 허용한다. online 상태의 `session_id=0` frame은 폐기한다. sequence는 방향별로 관리하고 unsigned 32-bit modular 비교를 사용한다. wrap 자체는 오류가 아니며 session이 바뀌면 sequence window를 초기화한다.

### 4.3 flags

| Bit | 이름 | 의미 |
|---:|---|---|
| 0 | `ACK_REQUIRED` | 애플리케이션 ACK 필요 |
| 1 | `RESPONSE` | `correlation_id`가 가리키는 응답 |
| 2 | `ERROR` | 정상 응답 대신 오류 포함 |
| 3 | `FRAGMENT` | bulk fragment |
| 4 | `LAST_FRAGMENT` | 마지막 fragment |
| 5 | `BROADCAST` | discovery 계열 broadcast |
| 6 | `READ_ONLY` | 송신자가 read-only 역할로 동작 |
| 7 | reserved | v1에서 0 |

암호화 여부를 payload의 self-asserted flag로 판단하지 않는다. 수신 peer가 등록돼 있고 해당 peer 설정의 `encrypt=true`인지 transport context에서 확인한다.

### 4.4 priority와 queue

| Priority | 용도 | queue overflow 시 정책 |
|---:|---|---|
| P0 | heartbeat, auth, safety state, lease revoke | 절대 telemetry 때문에 제거하지 않음 |
| P1 | command, ACK, command result | P3/P4를 먼저 제거 |
| P2 | DPF 경고, bus-off, fault event | P3/P4를 먼저 제거 |
| P3 | 일반 telemetry | 오래된 동일 signal batch부터 제거 |
| P4 | raw dump, bulk, counters | 가장 먼저 제거 |

단일 FIFO에 모든 종류를 섞지 않는다. 최소 `control`과 `telemetry` 두 queue를 두거나 priority queue를 사용한다. queue가 가득 차면 P4, P3 순으로 drop하며 누적 drop 수를 heartbeat와 diagnostic counters에 보고한다.

## 5. 메시지 종류

| 범위 | 메시지 | QoS | 방향 |
|---|---|---|---|
| `0x01–05` | discovery·pairing | Q1 | 양방향 |
| `0x10` | `HELLO` | Q1 | 양방향 |
| `0x11` | `CAPABILITIES` | Q1 | 양방향 |
| `0x12–13` | time sync request/response | Q1 | 양방향 |
| `0x14` | heartbeat | Q0 | 양방향 |
| `0x15` | ACK | 자체 ACK 없음 | 양방향 |
| `0x16` | ERROR | 필요 시 Q1 | 양방향 |
| `0x17–18` | state snapshot request/response | Q1 | Controller→Communicator→Controller |
| `0x20` | raw CAN batch | Q0 | Communicator→Controller |
| `0x21` | decoded signal batch | Q0 | Communicator→Controller |
| `0x22` | bus status | Q0, 변화 시 Q1 | Communicator→Controller |
| `0x30–31` | command request/result | Q1 | 양방향 |
| `0x32–33` | control lease | Q1 | 양방향 |
| `0x40–42` | configuration | Q1 | 양방향 |
| `0x50` | diagnostic counters | Q0/Q1 | 양방향 |
| `0x60–63` | bounded bulk transfer | Q1 window | 양방향 |

알 수 없는 message type을 받으면 길이와 CRC 검증 후 `UNSUPPORTED_MESSAGE`를 rate-limit해 반환한다. broadcast에 오류 응답을 보내지 않는다. broadcast error response는 reply storm을 만들 수 있다.

## 6. QoS, ACK, 재전송

### 6.1 QoS 0: 최신값 우선

`CAN_BATCH`, `SIGNAL_BATCH`, 일반 heartbeat는 재전송하지 않는다. telemetry는 과거 packet을 복구하는 것보다 최신 packet을 받는 편이 낫다. 수신자는 sequence gap을 loss counter에 반영하고 각 signal의 sample time과 age로 freshness를 계산한다.

### 6.2 QoS 1: 적어도 한 번 전달 + 정확히 한 번 실행 효과

명령·설정·협상은 `ACK_REQUIRED`를 세우고 ACK가 없으면 재전송한다. 이 방식은 같은 packet이 여러 번 도착할 수 있으므로 수신자는 중복 제거 cache를 가져야 한다.

- 초기 ACK timeout: 최근 RTT가 없으면 80 ms
- 적응 timeout: `clamp(2 × smoothed_RTT + 20 ms, 40 ms, 300 ms)`
- 최대 시도: 최초 1회 + 재시도 3회
- retry jitter: ±10%
- TTL이 먼저 끝나면 재전송 중단
- ACK와 ERROR frame에는 ACK를 요구하지 않음

송신 callback이 끝나기 전에 다음 ESP-NOW 송신을 밀어 넣지 않는다. P0/P1 대기 frame이 있으면 P3/P4보다 먼저 보낸다.

### 6.3 ACK와 처리 결과의 분리

ACK는 “frame을 받고 문법·인증·중복 여부를 판단했다”는 뜻이다. 실제 제어 결과는 `COMMAND_RESULT`다.

```text
Controller                   Communicator
   |---- COMMAND_REQUEST ------->|
   |<--- ACK(ACCEPTED) -----------|  수신/검증 완료
   |<--- RESULT(ACCEPTED) --------|  정책 queue 수락
   |<--- RESULT(EXECUTING) -------|  선택적 중간 상태
   |<--- RESULT(COMPLETED) -------|  feedback 상태까지 확인
```

`ACK(DUPLICATE)`를 받은 송신자는 실패로 보지 않는다. 수신자는 같은 `request_token`에 대해 저장된 최신 `COMMAND_RESULT`를 다시 보낸다.

## 7. 최초 등록과 보안

### 7.1 금지되는 간편 구현

- 코드에 모든 장치가 공유하는 PMK/LMK를 hard-code하지 않는다.
- 기본 PMK를 사용하지 않는다.
- LMK나 provisioning secret을 broadcast/unicast 평문으로 전송하지 않는다.
- MAC 주소만 확인하고 인증됐다고 간주하지 않는다. MAC spoofing은 가능하다.
- 한쪽 장치만 버튼을 눌렀다는 이유로 임의 peer를 등록하지 않는다.
- 기존 secure peer 연결 실패를 자동으로 평문 mode로 downgrade하지 않는다.

### 7.2 provisioning 정보

생산 설치 단위마다 최소 다음 값을 만든다.

| 값 | 크기 | 저장 |
|---|---:|---|
| `installation_id` | 64 bit random | 양쪽 NVS |
| `pairing_secret` | 256 bit CSPRNG | 양쪽 encrypted NVS |
| `device_id` | 64 bit random | 각 장치 NVS |
| `key_generation` | 32 bit | 양쪽 NVS |

권장 방법은 USB serial provisioning이다. QR을 사용할 경우 외부에서 계속 보이는 고정 QR이 아니라 봉인된 일회용 설치 secret을 사용하고 pairing 후 폐기한다. ESP32 production 설정에서는 NVS encryption, secure boot, flash encryption을 함께 검토한다.

### 7.3 물리 pairing window

양쪽 장치에서 120초 이내에 물리 버튼 또는 USB 명령으로 pairing window를 열어야 한다. 차량 속도가 0이 아니거나 ignition 상태를 신뢰할 수 없으면 Communicator는 pairing을 시작하지 않는다.

pairing 중 broadcast는 1 Hz 이하로 제한하고 다음 정보만 담는다.

- protocol major/minor 범위
- Communicator `device_id`
- 128-bit `communicator_nonce`
- Wi-Fi channel
- `installation_id`의 비가역 짧은 식별값
- transcript HMAC tag

VIN, CAN data, key, 차량 위치, 사용자 이름은 discovery에 넣지 않는다.

### 7.4 인증 handshake

```text
Communicator                             Controller
  |-- DISCOVERY(comm_nonce, HMAC) broadcast ->|
  |<-- PAIR_REQUEST(controller_nonce, HMAC) --|
  |-- PAIR_CHALLENGE(selection, HMAC) -------->|
  |       양쪽에서 HKDF로 PMK/LMK 파생          |
  |<== PAIR_CONFIRM(transcript hash) encrypted=|
  |== PAIR_RESULT(key generation) encrypted ==>|
```

HMAC은 `HMAC-SHA-256(pairing_secret, domain || canonical_fields)`를 사용하고 wire에는 앞 16 byte를 싣는다. `domain`은 message마다 `CV-DISCOVERY-1`, `CV-PAIR-REQUEST-1`처럼 분리한다. field 연결은 길이 prefix가 있는 canonical binary encoding을 사용한다.

ESP-NOW의 PMK는 장치당 하나이고 LMK는 peer별이다. 따라서 PMK를 pairing session별 nonce로 만들면 Communicator가 여러 Controller를 동시에 유지할 수 없다. PMK는 설치 단위 전역으로, LMK만 peer별로 파생한다.

```text
install_salt = SHA-256("canview/install/v1" || installation_id || key_generation)
PMK = HKDF-SHA-256(pairing_secret, install_salt,
                   "canview/esp-now/pmk/v1")[0:16]

peer_salt = communicator_nonce || controller_nonce
LMK = HKDF-SHA-256(pairing_secret, peer_salt,
                   "canview/esp-now/lmk/v1" || installation_id ||
                   communicator_device_id || controller_device_id || key_generation)[0:16]
```

PMK와 LMK 자체는 공중으로 보내지 않는다. 같은 installation의 모든 node는 같은 PMK generation을 사용하지만 각 peer pair의 LMK는 nonce와 두 device ID 때문에 다르다. `PAIR_CONFIRM`부터 encrypted unicast여야 하며 transcript hash가 일치해야 한다. 성공 후 peer MAC, channel, PMK/LMK, generation을 encrypted NVS에 원자적으로 저장한다. 새 key 확인이 끝나기 전에는 encrypted NVS의 이전 generation 복구 사본을 지우지 않는다.

이 PSK 방식은 unique secret이 유출되면 forward secrecy를 제공하지 않는다. 더 높은 위협 모델이 필요하면 차기 major에서 ECDH+서명 handshake를 별도 도입하되 v1 wire에 임시 확장을 섞지 않는다.

### 7.5 평문 개발 mode

개발 mode는 compile-time flag와 물리 jumper를 동시에 요구하고, 화면에 항상 `UNSECURED / READ ONLY`를 표시한다. 이 mode에서는 CAN TX, configuration write, control lease를 모두 금지한다. production binary에서는 해당 flag를 빌드하지 않는다.

### 7.6 key 회전·삭제

- key 회전은 차량 정지, 양쪽 물리 확인, 기존 encrypted session 인증 후에만 가능하다.
- PMK generation 회전은 설치 전체 작업이다. ESP-NOW 장치에 PMK가 하나뿐이므로 Communicator가 서로 다른 PMK generation의 peer를 동시에 운용하지 않는다. 필요한 peer에 새 generation을 staging하고 모두 확인한 뒤 같은 activation 시점에 전환하며, 참여하지 못한 peer는 재등록 전까지 offline으로 둔다.
- 5회 연속 HMAC/CCMP 인증 실패 시 60초 `AUTH_LOCKED`로 들어가 rate-limit한다.
- peer 삭제는 USB 또는 양쪽 물리 확인으로만 수행한다.
- Communicator 초기화 시 Controller의 key가 남아 있어도 평문 fallback하지 않고 재-pairing을 요구한다.

## 8. 연결 상태기계

```text
DISABLED
   |
RADIO_INIT
   | paired                      | unpaired + physical window
PEER_RESTORE                     DISCOVERING
   |                                  |
SECURE_HELLO <-------------------- PAIRING
   |
NEGOTIATING -- incompatible --> READ_ONLY_INCOMPATIBLE
   |
TIME_SYNC
   |
STATE_SYNC
   |
ONLINE -- 3 heartbeat misses --> DEGRADED -- 3 s --> RECOVERING
   ^                                  |                    |
   |--------- valid heartbeat --------|---- secure hello ---|

Any state -- repeated auth failure --> AUTH_LOCKED
```

### 8.1 secure reconnect

paired boot의 절차는 다음과 같다.

1. Wi-Fi station 시작
2. 고정 regulatory country와 저장 channel 적용
3. `esp_now_init`
4. 저장 PMK 설정
5. encrypted peer 추가
6. 양쪽이 random 64-bit `boot_id`와 nonce를 담은 `HELLO` 교환
7. transcript에서 새 non-zero `session_id` 합의
8. capability 협상
9. time sync 3회
10. state snapshot 수신
11. `ONLINE`

secure HELLO가 실패해도 discovery로 downgrade하지 않는다. recovery scan은 저장된 peer MAC으로 인증 가능한 frame만 찾으며 운행 중에는 display control을 중단한 상태에서 짧은 dwell로 제한한다.

### 8.2 heartbeat와 timeout

| 조건 | 기본값 | 동작 |
|---|---:|---|
| heartbeat 주기 | 500 ms | 양방향 |
| 3회 누락 | 1.5 s | `DEGRADED`, 값 stale 표시, 새 command 금지 |
| 마지막 valid frame 3 s | 3.0 s | `RECOVERING`, lease 폐기 |
| recover scan 시작 | 차량 제어 중단 후 | 저장 channel 우선 |
| 화면 복귀 | HELLO+CAPS+SYNC+SNAPSHOT 완료 후 | 이전 command 재개 안 함 |

heartbeat에는 `boot_id`, uptime, state revision, RSSI, bus mask, bus error mask, queue depth, telemetry drop 수를 담는다. IDF의 수신 metadata에서 RSSI를 읽되 RSSI만으로 online/offline을 판단하지 않는다.

### 8.3 channel 정책

- production 기본은 설치 시 정한 고정 2.4 GHz channel이다.
- Communicator와 Controller의 Wi-Fi country 설정이 일치해야 한다.
- Controller가 AP에 연결되면 ESP-NOW도 AP channel 제약을 받는다. 운행 mode에서는 일반 AP 연결을 금지하거나, 정차 상태에서 인증된 channel 재설정 절차를 거친다.
- active control lease 중 channel을 바꾸지 않는다.
- scan 결과만으로 peer channel을 영구 저장하지 않고 secure HELLO 성공 뒤 commit한다.

## 9. version·capability 협상

### 9.1 version 규칙

- major가 다르면 command를 금지한다.
- 상대 major를 지원하지 않으면 `INCOMPATIBLE_MAJOR`와 지원 범위를 보낸다.
- 같은 major에서 낮은 minor의 수신자는 모르는 TLV를 건너뛴다.
- 고정 struct의 의미나 기존 enum 값을 바꾸지 않는다. 새 의미는 새 TLV 또는 새 message type으로 추가한다.
- reserved bit/field를 임의 기능에 재사용하지 않는다.

### 9.2 capability 내용

`CAPABILITIES`는 고정 prefix 뒤 TLV로 확장한다.

- role: Communicator, Primary Controller, Read-only Controller
- 최대 frame 크기와 ESP-NOW transport version
- CAN bus 수: 현재 `0–3`
- raw CAN, decoded signal, bulk 지원 여부
- control lease 지원 여부
- audio/drive-mode command bitset
- firmware semantic version와 build hash
- vehicle profile ID
- DBC upstream commit digest
- signal catalog revision·SHA-256 digest
- config schema version

UI는 capability가 없거나 차량 profile에서 검증되지 않은 기능을 숨기거나 `검증 필요`로 disabled 표시한다. 단순히 DBC에 signal 이름이 존재한다는 이유로 control capability를 켜지 않는다.

### 9.3 TLV

TLV header는 `type:u16`, `length:u16`이고 value가 뒤따른다. 다음 규칙을 적용한다.

- frame 끝을 넘는 length는 전체 frame을 malformed 처리한다.
- unknown non-critical TLV는 건너뛴다.
- critical TLV는 type의 최상위 bit를 1로 표시한다. 모르면 요청을 거부한다.
- 같은 singleton TLV가 중복되면 malformed다.
- text는 UTF-8이지만 운행 telemetry에는 사용하지 않는다.
- 최대 nesting depth는 0이다. TLV 안에 재귀 TLV를 넣지 않는다.

## 10. 시간 동기화와 freshness

Communicator STM32 monotonic clock을 CAN sample 기준으로 사용한다. Controller wall clock이나 RTC는 로그 표시용일 뿐 ordering 기준이 아니다.

time sync는 NTP와 같은 4 timestamp를 사용한다.

```text
t1 Controller send
t2 Communicator receive
t3 Communicator send
t4 Controller receive
offset = ((t2 - t1) + (t3 - t4)) / 2
rtt    = (t4 - t1) - (t3 - t2)
```

3회 측정해 RTT가 가장 작은 sample을 채택한다. 10초마다 또는 drift가 5 ms를 넘을 때 다시 맞춘다. time sync 실패는 telemetry 표시를 막지 않지만 서로 다른 node의 event 정렬 신뢰도를 낮춘다.

신호마다 `sample_time`, `age_ms`, `quality`를 보존한다. UI stale 기본값은 signal class별로 둔다.

| Signal class | 예상 주기 | stale | unavailable |
|---|---:|---:|---:|
| 속도·RPM·가속도 | 20–50 Hz | 150 ms | 1 s |
| 4WD clutch·wheel speed | 10–50 Hz | 250 ms | 1 s |
| drive/audio state | 5–20 Hz | 500 ms | 2 s |
| 온도·DPF lamp | 1–5 Hz | 2 s | 10 s |
| 진단 DID | 0.2–2 Hz | 5 s | 30 s |

stale 값을 0으로 바꾸지 않는다. 마지막 값과 `stale` 상태를 함께 표시하거나 `—`로 전환한다.

## 11. telemetry payload

### 11.1 raw CAN batch

payload prefix:

| 필드 | 크기 | 설명 |
|---|---:|---|
| `base_time_us` | 8 | 첫 frame의 Communicator STM32 monotonic time |
| `count` | 1 | record 수, 최대 12 |
| `dropped_since_last` | 1 | 포화 시 drop 수, 255에서 포화 |
| `reserved` | 2 | 0 |

각 record는 16 byte다.

| 필드 | 크기 | 설명 |
|---|---:|---|
| `delta_us` | 2 | base time으로부터 0–65,535 us |
| `bus_id` | 1 | `1`, `2`, `3`; 그 외 v1 invalid |
| `flags_dlc` | 1 | 상위 4 bit flags, 하위 4 bit DLC |
| `can_id` | 4 | 11/29-bit ID, 나머지 bit 0 |
| `data` | 8 | DLC 뒤 byte도 0으로 정규화 |

12 byte batch header + 12×16 byte = 204 byte다. header까지 236 byte이므로 상한 안에 든다. `delta_us`가 범위를 넘거나 12개가 차면 새 batch를 시작한다.

raw stream은 상시 전체 bus mirror가 아니다. 3×500 kbit/s CAN line traffic은 ESP-NOW 1 Mbit/s와 protocol overhead를 고려하면 그대로 보낼 수 없다. raw mode는 다음으로 제한한다.

- 한 번에 1개 bus
- CAN ID allow-list 또는 변화 frame만
- 기본 200 frame/s 상한
- 30초 자동 종료, 사용자 재승인 필요
- control command보다 낮은 P4

### 11.2 decoded signal batch

signal catalog가 `(signal_id, type, unit, scale, display name, source message)`를 정의한다. wire record는 12 byte다.

| 필드 | 크기 | 설명 |
|---|---:|---|
| `signal_id` | 2 | catalog key |
| `value_type` | 1 | bool/u32/i32/f32/enum/bitset |
| `quality` | 1 | valid/stale/unavailable/unverified/out-of-range/fault |
| `age_ms` | 2 | sample 생성 후 경과, 65,535에서 포화 |
| `reserved` | 2 | 0 |
| `value_bits` | 4 | type에 따른 LE bit pattern |

12 byte batch header와 16 records를 합치면 204 byte다. 빠른 signal group은 20–50 Hz, 중간 group은 5–10 Hz, 온도·진단은 1–2 Hz로 보낸다. 동일 signal이 한 frame에 두 번 나오면 뒤 record를 사용하되 protocol counter를 올린다.

### 11.3 signal ID namespace

| 범위 | 용도 |
|---|---|
| `0x0000–0x0FFF` | protocol/core 상태 |
| `0x1000–0x3FFF` | 공통 차량 신호 |
| `0x4000–0x7FFF` | Hyundai/Kia/Genesis profile |
| `0x8000–0xBFFF` | 설치·vendor 확장 |
| `0xC000–0xEFFF` | 실험/캡처 |
| `0xF000–0xFFFF` | reserved |

실험 신호를 검증 없이 공통 범위로 승격하지 않는다. 승격 시 새 catalog revision과 mapping 문서를 함께 배포한다.

## 12. command와 control lease

### 12.1 lease

Primary Controller는 명령 전에 2,000 ms control lease를 요청하고 500 ms마다 갱신한다. Communicator는 다음 경우 lease를 주지 않는다.

- peer가 read-only
- secure session이 아님
- vehicle profile 미검증
- TX global disable 또는 service jumper off
- 다른 peer가 lease 보유
- link/bus/signal 상태가 degraded

heartbeat 3회 누락, session 변경, Communicator reboot, physical TX disable에서 lease는 즉시 폐기한다. lease가 사라져도 이미 순간적으로 전송된 CAN frame을 되돌릴 수 있다고 가정하지 않는다. 따라서 차량 명령은 가능한 한 짧은 pulse와 feedback 확인으로 설계한다.

### 12.2 command request

| 필드 | 설명 |
|---|---|
| `request_token:u64` | CSPRNG 기반 idempotency key |
| `command_id:u16` | 사전 정의된 의도 명령 |
| `ttl_ms:u16` | 생성 후 실행 가능한 최대 시간 |
| `expected_state_revision:u32` | optimistic concurrency |
| `precondition_flags:u32` | Controller가 기대한 조건. Communicator가 독립 재검증 |
| `argument_tlv_length:u16` | 뒤 TLV 길이 |
| `reserved:u16` | 0 |

Communicator는 `request_token` 결과를 최소 60초 또는 256건 LRU로 보관한다. 같은 token과 다른 payload가 오면 auth/protocol 오류로 취급하고 실행하지 않는다.

### 12.3 명령 수명주기

1. frame·session·CRC·peer 인증
2. TTL·중복 검사
3. lease와 capability 검사
4. state revision 검사
5. Communicator가 현재 vehicle signal로 precondition 재검사
6. 차량 profile이 만든 CAN event 실행
7. 별도 feedback message에서 기대 상태 관찰
8. `COMPLETED` 또는 timeout `FAILED`

화면에 optimistic UI를 보여줄 수 있지만 `요청 중` 상태와 실제 적용 상태를 구분한다. 실패 시 원래 상태로 돌아가고 구체적인 거부 사유를 표시한다.

### 12.4 command ID 정책

v1의 공개 명령은 raw frame이 아니라 다음과 같은 의미 단위다.

- `AUDIO_PROFILE_SET`: 중앙/취침/뒷좌석 강화 profile
- `AUDIO_VOLUME_OFFSET_SET`: 제한된 상대 offset
- `AUDIO_RESTORE_SNAPSHOT`: Communicator가 저장한 OEM 상태 복원
- `DRIVE_MODE_BUTTON_PULSE`: 검증된 물리 버튼 1회 동작만 요청
- `AUTOMATION_ARM`, `AUTOMATION_DISARM`

`DRIVE_MODE_BUTTON_PULSE`도 Communicator가 속도·기어·브레이크·ESC·신호 freshness를 검사한다. 특정 drive mode state를 ECU에 직접 쓰는 명령은 정의하지 않는다.

## 13. 상태 snapshot과 revision

Communicator의 `state_revision`은 다음이 바뀔 때 증가한다.

- active vehicle profile
- audio OEM 상태 또는 active CANView profile
- drive mode 또는 sport automation 상태
- control lease owner
- bus online/error 상태
- safety inhibit bitset

reconnect 뒤 display는 telemetry를 받기 전에 `STATE_SNAPSHOT`을 적용한다. snapshot은 여러 frame이 필요하면 bounded bulk를 사용하지만, core 상태는 208 byte 안의 첫 frame에 넣어야 한다. 화면은 이전 session의 snapshot과 새 telemetry를 섞지 않는다.

## 14. error 모델

error code 상위 byte는 category다.

| 범위 | 범주 | 예 |
|---|---|---|
| `0x01xx` | transport | send fail, channel mismatch |
| `0x02xx` | protocol | bad length, CRC, reserved |
| `0x03xx` | auth/session | HMAC 실패, session mismatch |
| `0x04xx` | compatibility | unsupported type/major |
| `0x05xx` | resource | queue full, no memory |
| `0x06xx` | application | config key, unavailable feature |
| `0x07xx` | safety | lease, precondition, stale, TX disabled |

error payload는 code, severity, origin, offending message type/sequence, 짧은 numeric detail, `retry_after_ms`만 담는다. key, nonce 전체, secret, VIN, raw memory, stack trace를 넣지 않는다.

오류 응답도 rate-limit한다. 같은 peer·code에 초당 2회, 전체 초당 10회를 기본 상한으로 둔다. malformed broadcast는 조용히 버리고 counter만 올린다.

## 15. 장애별 동작

| 장애 | 탐지 | 처리 | 사용자 표시 |
|---|---|---|---|
| telemetry packet loss | sequence gap | 재전송 없음, 최신값 대기 | 심하지 않으면 표시 없음 |
| command ACK loss | timeout | 같은 token으로 재전송 | `요청 확인 중` |
| duplicate command | token cache hit | 재실행 금지, 저장 결과 반환 | 기존 결과 유지 |
| out-of-order telemetry | time/revision older | 오래된 값 폐기 | 없음 |
| Communicator reboot | `boot_id` 변경 | lease 폐기, HELLO부터 재동기화 | `Communicator 재연결` |
| Controller reboot | session 불일치 | Communicator가 이전 lease 폐기 | snapshot 전 제어 잠금 |
| bad length/CRC | parser | 폐기, counter, rate-limited error | 진단 화면에만 |
| channel mismatch | send error/timeout | control 중단, 저장 channel 우선 복구 | `무선 채널 확인` |
| queue overflow | watermark | P4→P3 drop | `데이터 지연` if sustained |
| version mismatch | HELLO/CAPS | read-only fallback 또는 incompatible | `버전 업데이트 필요` |
| key mismatch | encrypted HELLO 실패 | retry 제한 후 AUTH_LOCKED | `다시 등록 필요` |
| CAN bus-off | controller status | 해당 bus TX 금지, recovery policy | bus 번호와 오류 |
| stale safety signal | age threshold | 관련 command 거부/automation 해제 | 구체적 inhibit 이유 |
| external OEM override | feedback≠requested | automation 중단, snapshot 갱신 | `차량에서 직접 변경됨` |

## 16. backpressure와 처리 task

ESP-NOW callback이 하는 일은 다음으로 제한한다.

1. source MAC과 길이 상한 확인
2. 고정 pool buffer 획득
3. payload와 RSSI/channel metadata 복사
4. queue send without blocking
5. 실패 시 atomic drop counter 증가

CRC, HMAC, TLV, DBC decode, flash write, LVGL update, CAN TX는 callback에서 하지 않는다. 권장 task 분리는 다음과 같다.

```text
Wi-Fi callback -> rx ingress queue -> protocol task -> control queue -> safety/CAN task
                                             \-----> telemetry queue -> UI/model task
```

flash/NVS write는 운행 중 빈번히 하지 않고 config commit과 pairing에만 사용한다. diagnostic counter는 RAM에 누적하고 정상 shutdown 또는 명시적 capture에서 저장한다.

## 17. 대역폭 예산

ESP-NOW 기본 bit rate는 1 Mbit/s지만 MAC overhead, airtime 경쟁, retry를 제외한 값이 아니다. CANView v1은 한 display당 application payload sustained 20 kB/s, 1초 burst 40 kB/s를 초기 engineering budget으로 둔다. 이 값은 실차 RF 시험 후 낮출 수 있으며 보장 throughput이 아니다.

권장 traffic 예시는 다음과 같다.

| 데이터 | 주기 | 대략 payload |
|---|---:|---:|
| fast signal 16개 | 20 Hz | 4.1 kB/s |
| medium signal 16개 | 5 Hz | 1.0 kB/s |
| slow/diagnostic 16개 | 1 Hz | 0.2 kB/s |
| heartbeat 양방향 | 2 Hz | 0.3 kB/s 이하 |
| event/command | 비정기 | 0.1 kB/s 이하 |
| 제한 raw capture | 최대 200 frame/s | 약 3.5–6 kB/s |

Communicator는 전송 전 동일 signal의 오래된 queue item을 coalesce한다. 화면 refresh가 20 Hz라면 같은 signal을 100 Hz로 무선 전송하지 않는다.

## 18. bulk와 확장성

bulk는 log snippet, signal catalog, 작은 config blob을 위한 선택 기능이다. firmware OTA나 전체 DBC 배포는 v1 범위에서 제외한다.

- 최대 object: 64 KiB
- fragment payload: 최대 192 byte
- window: 4 fragments
- selective ACK bitmap
- object SHA-256 필수
- active control lease 중 bulk 시작 금지
- P4, telemetry 혼잡 시 일시 정지
- 30초 inactivity timeout

DBC 원본은 Controller로 매번 보내지 않는다. Communicator와 Controller가 catalog digest를 비교하고 불일치하면 UI에 `신호 정의 불일치`를 표시한다. compatible catalog를 갖춘 경우에만 decoded signal ID를 사용한다.

향후 확장은 다음 공간을 보존한다.

- CAN `bus_id`는 v1에서 1–3만 사용하지만 wire는 8 bit다.
- signal ID는 16 bit namespace다.
- capability bitset과 TLV로 optional 기능을 협상한다.
- 새 transport가 필요하면 같은 application frame을 BLE/USB에서 재사용할 수 있다.
- CAN FD는 별도 capability와 record type으로 추가하며 기존 classic record 의미를 바꾸지 않는다.

## 19. parser 구현 순서

수신 parser는 다음 순서를 고정한다.

1. source MAC allow-list와 transport encryption context
2. `data != NULL`, `32 <= len <= 240`
3. magic
4. header length 범위
5. `payload_len == len - header_len`
6. reserved field/flag
7. supported major와 message type별 허용 state
8. CRC
9. session ID와 anti-replay window
10. message별 최소/최대 payload 길이
11. TLV bounds·duplicate·critical type
12. semantic range
13. ACK enqueue
14. 실제 처리 queue enqueue

오류 frame을 처리하다 다시 오류 frame을 생성하지 않는다.

## 20. 검증 계획

### 20.1 codec 단위시험

- header golden byte vector와 CRC golden value
- 모든 integer endian round-trip
- 0, 1, 208 byte payload 경계
- 잘린 header와 payload
- reserved bit/field
- sequence wrap `0xFFFFFFFF -> 0`
- float NaN/Inf 정책
- TLV duplicate, overflow, unknown critical/non-critical
- C와 Python/reference codec 교차 검증

### 20.2 fuzz·보안시험

- random length 0–1,500 byte
- bit flip, forged length, forged CRC
- replayed pairing transcript
- nonce reuse 탐지
- 동일 token+다른 command payload
- auth failure rate limit
- NVS power-loss 중 key rotation
- peer MAC spoof와 unauthenticated broadcast flood
- parser가 buffer 밖을 읽지 않는지 ASan/UBSan host test

### 20.3 network fault injection

- 1%, 5%, 20%, 50% packet loss
- 20–300 ms delay와 jitter
- duplicate·reorder
- Communicator/Controller 개별 reboot
- channel mismatch
- queue saturation과 memory allocation failure
- RSSI 단계 하강
- v1/v2 ESP-NOW 조합
- incompatible protocol minor/major

### 20.4 차량 안전시험

1. host simulation에서 command state machine
2. 두 ESP32 보드 RF bench
3. CAN simulator, Communicator TX는 dummy bus
4. 차량 harness bench, ECU 대신 capture replay
5. 실차 ignition off listen-only
6. 실차 stationary read-only
7. 실제 버튼과 feedback frame 동시 capture
8. 폐쇄 시험장에서 한 command pulse
9. fault injection 상태에서 fail-safe 확인

sport 자동 전환과 audio spoof는 마지막 단계 전에는 monitor-only다. 강제 DPF regeneration 명령은 이 프로젝트 기능으로 구현하지 않는다.

## 21. 관측 지표

양쪽 node가 최소 다음 counter를 유지한다.

- ESP-NOW send success/fail
- application ACK timeout/retry
- rx bad length/magic/CRC/version/session
- replay/duplicate/out-of-order
- control/telemetry queue high-water mark와 drop
- heartbeat missed와 reconnect 횟수
- RTT smoothed/max, RSSI smoothed/min
- bus별 CAN rx, overflow, error passive, bus-off
- command accepted/completed/rejected/expired/duplicate
- safety inhibit reason별 count

counter overflow는 saturating 또는 64-bit로 처리한다. UI는 운전 화면에 상세 counter를 노출하지 않고 연결 상태 한 줄로 요약하며, 정차 진단 화면에서만 상세값을 제공한다.

## 22. 구현 완료 조건

- [ ] `protocol/canview_protocol.h`와 reference encoder의 golden vector 일치
- [ ] 모든 일반 frame 240 byte 이하
- [ ] production에서 PMK/LMK와 encrypted peer 강제
- [ ] unencrypted mode에서 control lease 발급 불가
- [ ] duplicate command가 한 번만 차량 효과를 만듦
- [ ] 1.5초 link degraded에서 새 차량 command 0건
- [ ] 재부팅·session 변경 뒤 이전 command 재실행 0건
- [ ] 3개 bus telemetry에서 bus ID와 timestamp 보존
- [ ] queue 포화에서 P0/P1이 P3/P4 때문에 drop되지 않음
- [ ] unknown minor TLV 건너뛰기와 incompatible major 거부 검증
- [ ] UI stale/error/inhibit 상태가 protocol 상태와 일치

## 23. 관련 문서

- [기능 조사 및 구현 설계](feature-design.md)
- [UI/UX 설계와 LVGL 매핑](ui-design.md)
- [하드웨어 및 개발환경](hardware-and-development.md)
- [DBC 출처·검증 절차](../dbc/README.md)
