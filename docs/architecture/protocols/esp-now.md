# CANView ESP-NOW 양방향 프로토콜

이 문서는 [protocol index](README.md)에 속한 Controller·Communicator·Diagnostic Bridge 무선 계약의 상세 정본이다. 내부 MCU UART 계약과 독립적으로 versioning한다.

## 1. 문서 목적

이 문서는 최대 3개 차량 CAN 버스를 수집하는 **Communicator**와 `ESP32-S3-Touch-LCD-3.5` 기반 **Controller** 사이의 양방향 프로토콜을 정의한다. 단순 텔레메트리 전송뿐 아니라 최초 등록, 상호 인증, 기능 협상, 시간 동기화, 명령 확인, 오류 복구, 버전 확장을 포함한다. 미확정 CAN 신호를 휴대폰으로 검증하는 선택 장치 `Diagnostic Bridge`도 같은 보안·frame·QoS 규칙을 사용하되 차량 명령 권한은 갖지 않는다.

이 문서의 `MUST`, `MUST NOT`, `SHOULD`, `MAY`는 각각 필수, 금지, 권고, 선택을 뜻한다. 현재 check-in된 C draft는 `1.2`지만 완전한 payload ABI와 codec이 없어 runtime 구현 기준이 아니다. 첫 통합 구현은 Diagnostic Bridge의 observer/capture message까지 포함한 `1.3`으로 동결하며, 미완성 `1.2` compatibility path를 만들지 않는다. machine-readable schema, 생성 C header와 golden vector가 함께 들어가는 [T-002](../../tasks/T-002-espnow-schema-v1.3.md)가 끝나기 전에는 wire firmware를 구현하지 않는다. 전체 owner·시간 epoch·gate 기준은 [구현 준비 기준](../implementation-readiness.md)을 따른다.

설계 원칙은 다음과 같다.

1. 차량 CAN 송신 권한과 최종 안전 판단은 항상 Communicator가 가진다.
2. Primary Controller는 활성 차량 profile에서 검증된 기능별 “의도 명령”만 요청할 수 있다. 음량·fader/balance·mute 같은 필요한 제어는 허용하되 raw CAN frame은 생성하지 않는다.
3. ESP-NOW의 MAC 계층 성공을 애플리케이션 처리 성공으로 간주하지 않는다.
4. 텔레메트리 손실과 제어 명령 손실을 다른 QoS로 처리한다.
5. 연결이 불확실하면 표시값을 stale로 바꾸고 제어를 중단한다.
6. v1의 모든 일반 frame은 ESP-NOW v1/v2 공통 범위 안인 240 byte 이하로 제한한다.
7. runtime `signal_quality`와 정적 `evidence_grade`를 분리하고, 미확인 차량 신호는 `CANDIDATE` evidence를 보존한다.

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
| Communicator | CAN1–3 raw 수집, timestamp, raw bridge, upstream stream hint, control lease, 최종 안전 gate | Controller 명령, 무선 payload, 화면용 DBC 의미 |
| Primary Controller | 상태 표시, 사용자 입력, Controller-local CAN filter, DBC catalog/decode, 허용된 audio·SPORT 의도 명령, stale/error 표시 | Communicator가 보내는 값의 차량별 의미를 검증 없이 신뢰, 임의 raw CAN TX |
| Read-only Controller | 추가 화면·정비 화면. 상태 수신만 허용 | 모든 제어 요청 |
| Diagnostic Bridge | ID 통계·제한 raw capture, 후보 decoder/evidence, 휴대폰 웹 UI | control lease, 차량 명령, raw replay, 후보 자동 확정 |
| Provisioning host | USB를 통한 장치 로컬 PMK·pair별 link package 초기화 | 무선 discovery |

Communicator는 최대 20개 peer라는 ESP-NOW 한도보다 훨씬 작은 운영 한도를 둔다. 현재 권고값은 Primary Controller 1개, Read-only Controller 1개, Diagnostic Bridge 1개다. 여러 peer가 등록돼도 control lease는 Primary Controller 한 peer만 소유한다. Diagnostic Bridge의 diagnostic lease는 filter/capture에만 적용되며 control lease와 별도다.

### 3.1 데이터 방향

```text
CAN1/2/3 -> Communicator -> CAN_BATCH / BUS_STATUS -> Controller
Controller ingress -> allow-list -> Controller DBC catalog/decoder -> UI model
Controller -> COMMAND_REQUEST / CONFIG_* / LEASE_REQUEST -> Communicator
Communicator -> ACK + COMMAND_RESULT / ERROR / SNAPSHOT -> Controller

Communicator -> CAN_ID_STATS / bounded CAN_BATCH -> Diagnostic Bridge
Diagnostic Bridge -> observer filter / capture request -> Communicator
Phone <-> Bridge SoftAP HTTP/WebSocket; CAN command path 없음
```

### 3.2 안전 경계

- 무선에서 임의 arbitration ID, DLC, data를 받아 차량에 그대로 송신하는 API를 만들지 않는다.
- drive mode·audio 제어는 Communicator firmware에 컴파일된 command ID와 차량 profile allow-list의 교집합만 허용한다.
- Controller는 자신이 알고 있는 상태 revision을 명령에 넣는다. Communicator 상태가 바뀌었으면 stale command를 거부한다.
- link가 `DEGRADED`가 되는 즉시 새 명령을 거부하고 control lease를 회수한다.
- 재부팅 뒤 이전 session의 command를 재실행하지 않는다.

### 3.3 Primary Controller 제어 권한

Primary Controller는 단순 read-only 장치가 아니다. CANView 기능 구현에 필요한 아래 제어를 **기능별 capability와 차량 profile allow-list가 모두 허용한 경우에만** 요청할 수 있다.

| control scope bit | 이름 | 허용 용도 |
|---:|---|---|
| 0 | `AUDIO_PROFILE` | `QUIET`, `REAR_BOOST`, `CENTER` 같은 검증된 profile transaction |
| 1 | `AUDIO_VOLUME_OFFSET` | 주행 소음 보정과 profile의 제한된 상대 음량 변경 |
| 2 | `AUDIO_FADER` | 취침·뒷좌석 강화 profile의 앞/뒤 패닝 |
| 3 | `AUDIO_BALANCE` | profile에 필요한 좌/우 패닝. 현재 운전자 UI에는 임의 조정 화면을 두지 않음 |
| 4 | `AUDIO_MUTE` | main mute 상태 설정·복원 |
| 5 | `AUDIO_REAR_MUTE` | 취침 mode의 뒷좌석 speaker mute 설정·복원 |
| 6 | `AUDIO_SDVC` | OEM SDVC 충돌 회피 또는 검증된 설정 복원 |
| 7 | `AUDIO_RESTORE` | 적용 전 OEM audio snapshot 원자적 복원 |
| 8 | `DRIVE_MODE_PULSE` | 검증된 SPORT button event와 이전 mode 복귀 |
| 9 | `ADAPTIVE_VOLUME_AUTOMATION` | 주행 소음 기반 음량 자동화 arm/disarm |
| 10 | `AUTO_SPORT_AUTOMATION` | 속도·가속도 기반 SPORT 자동화 arm/disarm |
| 11–15 | reserved | 송신 0, 수신 시 거부 |

권한 판정은 `인증된 Primary Controller peer ∩ control lease ∩ 협상된 control scope ∩ 활성 vehicle profile command allow-list ∩ 현재 safety precondition`의 교집합이다. 하나라도 없으면 명령을 실행하지 않는다. capability bit가 없거나 미확정이면 false가 기본값이다.

`AUDIO_PROFILE_SET`은 profile이 사용하는 모든 하위 scope가 있어야 한다. 예를 들어 `QUIET`이 rear mute, fader와 volume cap을 사용한다면 bit 0·1·2·5가 모두 필요하다. 일부만 허용된 상태에서 조용히 축소 적용하지 않고 `CAPABILITY_MISSING`으로 전체 transaction을 거부한다.

Read-only Controller와 Diagnostic Bridge에는 bit 0–10을 모두 0으로 고정한다. 문 잠금, 등화, 임의 arbitration ID/payload, 확인되지 않은 ECU 설정은 현재 허용 scope에 포함하지 않는다.

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
| `0x21` | decoded signal batch, optional legacy | Q0 | Communicator→Controller |
| `0x22` | bus status | Q0, 변화 시 Q1 | Communicator→Controller |
| `0x23` | CAN filter get/list | Q1 | Controller↔Communicator |
| `0x24` | CAN filter add/replace/delete/clear | Q1 | Controller↔Communicator |
| `0x25` | CAN filter result | Q1 | Controller↔Communicator |
| `0x26` | raw stream period/count/byte budget | Q1 | Controller↔Communicator |
| `0x27` | raw stream counters/status | Q0/Q1 | Controller↔Communicator |
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
- 최대 시도: 최초 1회 + 재시도 2회
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

생산 provisioning은 설치 전체가 공유하는 비밀값을 만들지 않는다. 설치 식별자는 공개 그룹 정보일 뿐이고, 신뢰 재료는 장치 로컬 또는 직접 연결할 두 장치에만 한정한다.

| 값 | 크기 | 저장 |
|---|---:|---|
| `installation_id` | 64 bit random | 설치 내 장치 NVS, 비밀 아님 |
| `device_id` | 64 bit random | 해당 장치 NVS |
| `local_pmk` | 128 bit CSPRNG | 해당 장치 encrypted NVS, 외부 반출 금지 |
| `link_root` | 256 bit CSPRNG | 직접 연결할 두 endpoint의 encrypted NVS에만 저장 |
| `link_key_generation` | 32 bit | 해당 peer record |
| `authorized_role/scope/classes` | 고정 policy | 각 endpoint의 peer record |
| `control_root` | 256 bit CSPRNG | Primary Controller encrypted NVS와 STM32 protected Flash에만 저장 |

각 링크는 `Communicator↔Primary Controller`, `Communicator↔Diagnostic Bridge`, 선택적인 `Controller↔Diagnostic Bridge`마다 서로 다른 `link_root`를 갖는다. Bridge가 침해되어도 `Communicator↔Primary Controller`의 LMK를 계산하거나 Primary role을 증명할 수 없어야 한다. 새 peer를 추가할 때 provisioning host가 새 pair package를 만들고 USB로 정확히 두 endpoint에만 넣는다. 봉인된 일회용 QR을 지원하더라도 같은 pair package를 전달한 뒤 폐기하며, 무선 pairing만으로 새 root를 전달하지 않는다.

`control_root`는 ESP-NOW link root와도 별개다. provisioning host가 Controller USB와 STM32의 물리 service interface에 각각 넣고 Communicator ESP32나 Bridge에는 전달하지 않는다. vehicle-TX image에서는 STM32 debug/readout protection과 boot authenticity가 확인되지 않으면 control capability를 0으로 유지한다. debug unlock이나 root 초기화는 기존 root를 지우고 hard gate off 상태에서 재provisioning하게 한다.

ESP32의 PMK는 각 장치가 peer LMK를 로컬에서 보호하기 위한 장치 로컬 random 값으로 취급한다. endpoint끼리 PMK를 공유하거나 PMK에서 여러 peer의 LMK를 파생하지 않는다. `VEHICLE_TX` production image는 NVS encryption, secure boot와 flash encryption을 필수로 하고 provisioning/recovery fixture를 통과해야 한다. 개발용 `CAPTURE_ONLY` image에서 이를 끌 수는 있지만 control scope는 항상 0이다.

### 7.3 물리 pairing window

양쪽 장치에서 120초 이내에 물리 버튼 또는 USB 명령으로 pairing window를 열어야 한다. 차량 속도가 0이 아니거나 ignition 상태를 신뢰할 수 없으면 Communicator는 control-capable pairing을 시작하지 않는다. 빈 NVS의 bootstrap은 차량 연결 전 USB provisioning이 원칙이다. 예외적인 service bootstrap은 hard TX gate off, `CAPTURE_ONLY`, control scope 0, signed bitrate-only profile에서 read-only peer와 ID inventory만 허용한다.

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
  |       양쪽에서 peer LMK 파생                 |
  |<== PAIR_CONFIRM(transcript hash) encrypted=|
  |== PAIR_RESULT(key generation) encrypted ==>|
```

HMAC은 해당 pair package의 `HMAC-SHA-256(link_root, domain || canonical_fields)`를 사용하고 wire에는 앞 16 byte를 싣는다. `domain`은 message마다 `CV-DISCOVERY-1`, `CV-PAIR-REQUEST-1`처럼 분리한다. canonical fields에는 정렬된 양쪽 device ID와 MAC, 양쪽 nonce, channel, 선택 protocol version, 요청 role, 로컬 승인 role/scope/message class와 key generation을 모두 길이 prefix와 함께 넣는다. 요청 role은 로컬 peer record의 권한을 넓힐 수 없다.

ESP-NOW의 PMK는 장치당 하나이고 LMK는 peer별이다. 각 장치는 자신의 `local_pmk`를 설정하고, 두 endpoint는 pair 전용 `link_root`에서 같은 LMK만 파생한다.

```text
endpoint_order = lexicographic_sort(device_id || MAC)
peer_salt = nonce_of_endpoint_0 || nonce_of_endpoint_1
LMK = HKDF-SHA-256(link_root, peer_salt,
                   "canview/esp-now/lmk/v1" || installation_id ||
                   endpoint_order || selected_version || channel ||
                   authorized_role || authorized_scope ||
                   authorized_message_classes || link_key_generation)[0:16]
```

PMK, LMK와 `link_root`는 공중으로 보내지 않는다. `PAIR_CONFIRM`부터 encrypted unicast여야 하며 transcript hash가 일치해야 한다. 성공 후 peer MAC, channel, LMK, link generation과 로컬 승인 권한을 encrypted NVS에 원자적으로 저장한다. 새 key 확인이 끝나기 전에는 encrypted NVS의 이전 generation 복구 사본을 지우지 않는다.

이 pair별 PSK 방식은 해당 `link_root`가 유출되면 그 링크에 forward secrecy를 제공하지 않는다. 다만 다른 pair의 root나 LMK까지 파생되지는 않아야 한다. 더 높은 위협 모델이 필요하면 차기 major에서 ECDH+서명 handshake를 별도 도입하되 v1 wire에 임시 확장을 섞지 않는다.

### 7.5 평문 개발 mode

개발 mode는 compile-time flag와 물리 jumper를 동시에 요구하고, 화면에 항상 `UNSECURED / READ ONLY`를 표시한다. 이 mode에서는 CAN TX, configuration write, control lease를 모두 금지한다. production binary에서는 해당 flag를 빌드하지 않는다.

### 7.6 key 회전·삭제

- link key 회전은 차량 정지, 해당 두 endpoint의 물리 확인, 기존 encrypted session 인증 후에만 가능하다. 다른 peer의 link generation에는 영향을 주지 않는다.
- 새 `link_root`와 generation은 두 endpoint에 staging하고 양쪽 확인 뒤 전환한다. 실패하면 그 pair만 이전 generation으로 원자 복구하며 설치 전체를 잠그지 않는다.
- `local_pmk` 회전은 장치 로컬 maintenance transaction으로 처리하고 power loss 전후 모든 peer LMK record의 복구를 시험한다.
- 5회 연속 HMAC/CCMP 인증 실패는 알려진 peer 또는 pairing candidate별 exponential `AUTH_BACKOFF`를 적용한다. 알 수 없는 MAC이나 한 peer의 실패가 다른 정상 peer를 잠그지 못한다.
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

That peer/candidate -- repeated auth failure --> AUTH_BACKOFF
Other established peers remain online
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
- Diagnostic Bridge는 `WIFI_MODE_APSTA`에서 STA interface를 ESP-NOW에, SoftAP를 휴대폰에 사용하되 외부 AP에 연결하지 않는다. SoftAP channel은 installation channel과 같아야 한다.
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

- role: Communicator, Primary Controller, Read-only Controller, Diagnostic Bridge
- 최대 frame 크기와 ESP-NOW transport version
- CAN bus 수: 최대 3개, wire `bus_id`는 `0–2`
- raw CAN, Controller-local filter, decoded signal, bulk 지원 여부
- control lease 지원 여부
- 위 3.3절의 16-bit control scope bitset. Primary Controller만 non-zero이며 차량 profile 검증 결과를 반영
- TPMS, lighting/rheostat, longitudinal-acceleration signal capability
- firmware semantic version와 build hash
- vehicle profile ID
- Controller DBC upstream commit digest
- Controller signal catalog revision·SHA-256 digest
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

Communicator STM32 monotonic clock을 CAN sample 기준으로 사용한다. Controller의 PCF85063 RTC와 wall clock은 화면·로그·일몰 계산용일 뿐 ordering 기준이 아니다. 저장된 1차 Hyundai DBC에는 GPS 좌표나 현재 날짜·시각 CAN signal이 없으므로 시간 동기화가 GPS/RTC 값을 대신 만들어내지 않는다. 후보 조사 결과는 [GPS·시간 조사](../../vehicle/gps-time-investigation.md)에 있다.

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

차량 명령은 이 인접 ESP time sync만 사용하지 않는다. Controller와 STM32 사이의 `CONTROL_TIME_SYNC`를 두 ESP/UART hop이 그대로 전달해 `(controller_boot_id, stm_boot_id, control_sync_generation)` mapping을 만든다. 10초마다 갱신하고 30초 또는 uncertainty 50 ms 초과에서 invalid로 만들며, 어느 MCU든 reboot하면 mapping·lease·pending을 모두 폐기한다. command의 issued time/TTL/generation은 hop마다 변경하지 않는다.

신호마다 `sample_time`, `age_ms`, `quality`를 보존한다. UI stale 기본값은 signal class별로 둔다.

| Signal class | 예상 주기 | stale | unavailable |
|---|---:|---:|---:|
| 속도·RPM·가속도 | 20–50 Hz | 150 ms | 1 s |
| 4WD clutch·wheel speed | 10–50 Hz | 250 ms | 1 s |
| 종가속도 | 20–50 Hz | 150 ms | 1 s |
| 등화·rheostat | 5–20 Hz | 500 ms | 2 s |
| TPMS pressure | event–1 Hz | 2 s | 10 s |
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
| `bus_id` | 1 | wire 값 `0`, `1`, `2`가 논리 CAN1, CAN2, CAN3; `0xFF`는 filter에서만 wildcard |
| `flags_dlc` | 1 | 상위 4 bit flags, 하위 4 bit DLC |
| `can_id` | 4 | 11/29-bit ID, 나머지 bit 0 |
| `data` | 8 | DLC 뒤 byte도 0으로 정규화 |

12 byte batch header + 12×16 byte = 204 byte다. header까지 236 byte이므로 상한 안에 든다. `delta_us`가 범위를 넘거나 12개가 차면 새 batch를 시작한다.

raw stream은 상시 전체 bus mirror가 아니다. 3×500 kbit/s CAN line traffic은 ESP-NOW 1 Mbit/s와 protocol overhead를 고려하면 그대로 보낼 수 없다. Communicator는 raw batch를 bridge하되 Controller가 수신 직후 동일 allow-list를 적용한다. raw mode는 다음으로 제한한다.

- Controller filter 최대 32개, 한 설정 batch 최대 8개
- filter period 20 ms–60 s, filter별 1–32 record/period
- 전체 admission 기본 32 record/100 ms, period burst 기본 512 byte, raw record byte budget 최대 20,000 byte/s
- frame당 record는 최대 12개이며 payload+header는 240 byte 이하
- raw telemetry는 P4로 두고 command/ACK queue를 침범하지 않음
- filter에 일치하지 않거나 quota를 넘긴 record는 Controller model queue에 넣지 않음

### 11.1.1 Controller 수신 필터 관리

`CAN_FILTER_SET`의 payload는 `canview_can_filter_batch_header_t`와 `canview_can_filter_t` 배열이다. `ADD`, `REPLACE`, `DELETE`는 1–8개, `CLEAR`는 count 0만 허용한다. Communicator는 peer namespace에 적용하기 전에 secure session, peer role, expected `subscription_revision`, reserved 값과 모든 entry 범위를 staging copy에서 확인한다. 하나라도 실패하거나 revision이 맞지 않으면 전체 batch와 active revision을 바꾸지 않는다. Controller의 local RX allow-list는 별도 owner/store/revision이며 이 message로 직접 수정하지 않는다.

`CAN_FILTER_GET`은 현재 filter snapshot을 조회한다. 32개 전체는 8개 단위 fragment로 반환하며 각 fragment는 `snapshot_id`, revision, part index/count와 total count를 갖는다. 하나라도 빠지거나 조회 중 revision이 바뀌면 전체 snapshot을 폐기한다. `CAN_FILTER_RESULT`는 action 결과와 새 revision을 담는다. `CAN_STREAM_CONFIG`는 stream period, max record count, byte/s budget, burst 상한을 바꾸고 `CAN_STREAM_STATUS`는 accepted/rejected/budget drop을 보고한다. 이 메시지는 raw CAN을 차량에 재송신하는 기능이 아니다.

### 11.2 decoded signal batch

기본 경로는 Controller가 raw record를 자체 DBC catalog로 decode하는 것이다. `SIGNAL_BATCH`는 이전 Communicator decode 구현과의 선택적 호환 경로로만 남긴다. signal catalog가 `(signal_id, type, unit, scale, display name, source message)`를 정의한다. wire record는 12 byte다.

| 필드 | 크기 | 설명 |
|---|---:|---|
| `signal_id` | 2 | catalog key |
| `value_type` | 1 | bool/u32/i32/f32/enum/bitset |
| `quality` | 1 | valid/stale/unavailable/out-of-range/fault |
| `age_ms` | 2 | sample 생성 후 경과, 65,535에서 포화 |
| `evidence_grade` | 1 | unknown/candidate/observed/verified |
| `reserved` | 1 | 0 |
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
| `origin_device_id/origin_boot_id:u64` | provisioned Primary와 현재 boot binding |
| `wireless_session_id:u64` | 현재 encrypted session binding |
| `control_generation:u32` | Controller↔STM32 control root generation |
| `issued_at_controller_ms:u32` | retry에서도 바뀌지 않는 생성 시각 |
| `control_sync_generation:u32` | end-to-end clock mapping |
| `expected_state_revision:u32` | optimistic concurrency |
| `precondition_flags:u32` | Controller가 기대한 조건 hint. STM32 필수조건을 대체하지 않음 |
| `argument_tlv_length:u16` | 뒤 TLV 길이 |
| `reserved:u16` | 0 |
| `control_tag:16 bytes` | canonical envelope의 end-to-end HMAC |

Primary Controller와 STM32에만 별도 pair-specific `control_root`를 provision한다. Communicator ESP32는 root를 저장하지 않고 canonical command/lease bytes를 UART로 전달한다. STM32는 origin identity, role/scope, tag, sync generation, TTL, lease와 generated command table을 검증하고 terminal result와 TX/feedback digest에도 tag를 붙인다. Bridge와 read-only peer에는 control root가 없다.

STM32는 `(origin, boot, session, control generation, request token, command ID, payload digest)` 결과를 terminal 뒤 최소 60초 보관한다. TTL이 살아 있거나 result 보존 기간 안인 entry는 LRU로 축출하지 않으며 256개가 모두 live면 새 요청을 ACK 전에 `BUSY`로 거부한다. 같은 token과 같은 digest는 기존 result를 재전송하고, 같은 token과 다른 payload가 오면 auth/protocol 오류로 취급하고 실행하지 않는다. ACK와 RESULT는 모두 request token을 포함한다.

### 12.3 명령 수명주기

1. frame·session·CRC·peer 인증
2. TTL·중복 검사
3. lease와 capability 검사
4. state revision 검사
5. STM32가 generated command별 immutable known-mask와 현재 local vehicle signal로 precondition 재검사
6. dequeue와 각 frame/pulse 직전 같은 조건을 다시 검사한 뒤 차량 profile이 만든 bounded CAN event 실행
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

Controller에 audio 제어 권한을 주는 것은 위 의미 명령을 허용한다는 뜻이다. `AUDIO_PROFILE_SET` argument는 profile enum과 기대 snapshot revision만 전달하고, Communicator가 해당 profile의 volume·fader·balance·main/rear mute·SDVC 값을 vehicle profile의 검증된 범위에서 만든다. Controller가 CAN ID, byte offset 또는 임의 값 조합을 전달하지 않는다.

현재 UI에서 volume ± 버튼과 임의 sound-position 조정 화면을 숨기는 결정은 이 내부 제어 권한을 제거하지 않는다. 주행 소음 자동 음량, 취침 mode, 뒷좌석 강화 mode와 정확한 OEM 상태 복원을 위해 필요한 scope는 계속 사용할 수 있다.

`DRIVE_MODE_BUTTON_PULSE`도 STM32가 속도·기어·브레이크·ESC·신호 freshness를 검사한다. 송신자가 보낸 zero/partial/unknown precondition mask로 generated 필수조건을 생략할 수 없다. 특정 drive mode state를 ECU에 직접 쓰는 명령은 정의하지 않는다.

`AUTOMATION_ARM/DISARM`의 argument에는 `canview_automation_id_t`를 넣는다. 자동 SPORT가 arm되면 속도·종가속도 상태기계는 Communicator STM32에서 실행된다. Controller가 매 sample마다 SPORT 전환 명령을 보내지 않는다. STM32는 진입 직전 mode를 snapshot하고 vehicle profile의 제한된 button event와 feedback을 이용해 `SPORT -> previous mode`를 수행한다.

### 12.5 자동화 설정

`CONFIG_GET/SET/RESULT` payload는 `canview_config_batch_header_t` 뒤에 고정 8 byte `canview_config_record_t`를 `count`개 배치한다. `value_type`은 signal record와 같은 `CANVIEW_VALUE_*`를 사용하고 `reserved`는 0이어야 한다.

첫 통합 protocol 1.3에서 Communicator가 소유하는 SPORT key는 다음과 같다.

| Key | 형식 | 범위 |
|---|---|---|
| `SPORT_AUTOMATION_ENABLED` | bool | 0/1 |
| `SPORT_ENTRY_SPEED_TENTH_KPH` | u32 | 600/700/800 |
| `SPORT_ACCELERATION_ENABLED` | bool | 0/1 |

진입 속도를 바꾸면 복귀 속도는 Communicator가 `진입 속도 - 15 km/h`로 계산한다. Controller가 서로 맞지 않는 진입·복귀 쌍을 따로 쓰지 못하게 하기 위해 복귀 threshold key는 공개하지 않는다.

설정 변경은 secure session, Primary Controller, 차량 정지, control lease, compatible config schema를 모두 요구한다. `CONFIG_RESULT`의 성공과 새 `state_revision`을 받은 뒤에만 Controller mirror를 commit한다. 범위를 벗어난 값은 clamp하지 않고 application error로 거부해 UI와 Communicator의 실제 설정이 조용히 달라지지 않게 한다.

화면 밝기, FFT 주파수 대역·민감도·반응·최대 offset, RTC와 유휴 설정은 Controller-local NVS 값이다. RTC 시간 변경은 Controller가 PCF85063에 직접 적용하며 차량 command 또는 Communicator config로 전송하지 않는다. 휴대폰 설정을 구현할 때는 Diagnostic Bridge와 Controller 사이의 owner-targeted remote config를 사용한다. 실제 음량 offset만 `AUDIO_VOLUME_OFFSET_SET` 의미 명령으로 전달한다. CAN filter는 Controller local default-deny 경계와 Communicator의 peer별 upstream subscription을 별도 revision으로 관리한다.

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
| key mismatch | encrypted HELLO 실패 | 해당 peer만 retry 제한 후 AUTH_BACKOFF | `다시 등록 필요` |
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

ESP-NOW 기본 bit rate는 1 Mbit/s지만 MAC overhead, airtime 경쟁, retry를 제외한 값이 아니다. CANView v1은 모든 peer를 합친 application payload sustained 20 kB/s, 1초 burst 32 kB를 installation hard cap으로 둔다. P0/P1에 2 kB/s, Primary Controller runtime에 8 kB/s를 빌려주지 않는 reserve로 두고 나머지를 read-only, stats, raw capture 순서로 배분한다. 이 값은 실차 RF 시험 후 낮출 수 있으며 보장 throughput이 아니다.

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

DBC 원본은 무선으로 매번 보내지 않는다. Controller가 선택한 catalog의 source commit·SHA-256·revision을 보관하고, digest가 맞지 않으면 해당 decoded signal을 사용하지 않는다. Communicator는 signal 의미를 해석하지 않고 raw record를 전달한다. compatible catalog를 갖춘 경우에만 Controller가 decoded signal ID를 만든다.

향후 확장은 다음 공간을 보존한다.

- CAN `bus_id`는 v1에서 wire 값 `0`, `1`, `2`를 논리 CAN1, CAN2, CAN3으로 사용하며 wire는 8 bit다. `0xFF`는 filter wildcard로만 예약한다.
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
11. filter batch count·revision·reserved·quota range
12. TLV bounds·duplicate·critical type
13. semantic range
14. ACK enqueue
15. 실제 처리 queue enqueue

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

- [기능 조사 및 구현 설계](../features.md)
- [UI/UX 설계와 LVGL 매핑](../../ui/design.md)
- [하드웨어 및 개발환경](../../hardware/controller.md)
- [DBC 출처·검증 절차](../../../dbc/README.md)
- [Controller CAN 수신·DBC 파이프라인](../controller-can-pipeline.md)
- [Diagnostic Bridge·모바일 CAN 검증 UI](../diagnostic-bridge.md)
- [CAN 신호의 GPS·시간 조사](../../vehicle/gps-time-investigation.md)
