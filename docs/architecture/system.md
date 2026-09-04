# CANView 장치 명칭과 운영 데이터 흐름

상위 구조와 상세 문서 선택은 [architecture overview](README.md)가 정본이다. 이 문서는 장치 명칭, peer routing, 권한과 장애 시 동작만 자세히 설명한다. wire layout은 [protocol 문서](protocols/README.md), 회로는 [hardware 문서](../README.md#하드웨어)를 따른다.

## 1. 정식 장치명

CANView의 기본 운행 경로는 Controller와 Communicator 두 장치로 구성한다. 미확정 CAN 신호를 실차에서 검증할 때는 차량 CAN에 직접 연결되지 않는 선택 장치 Diagnostic Bridge를 추가한다. 문서, 펌웨어, 로그, UI에서 아래 이름을 정본으로 사용한다.

| 정식 명칭 | 하드웨어 | 핵심 책임 |
|---|---|---|
| **Controller** | Waveshare `ESP32-S3-Touch-LCD-3.5` | 운전자 UI, 터치 입력, 상태 표시, ESP-NOW 명령 요청 |
| **Communicator** | `ESP32-S3-MINI-1-N4R2` + `STM32G474CEU6` + `TCAN1046AV-Q1` + `MAX3055` | 차량 CAN 3채널 raw 수집·송신, 안전 정책, ESP-NOW 연결 |
| **Diagnostic Bridge** | 별도 ESP32-S3, prototype 권장 `ESP32-S3-WROOM-1-N8R2` | ESP-NOW read-only observer, capture·후보 저장, 휴대폰 SoftAP·웹 UI |

`display`, `screen`, `gateway`는 일반 설명이나 외부 문서의 고유 용어가 아닌 이상 장치명으로 사용하지 않는다. 프로토콜 role 값은 `PRIMARY_CONTROLLER`, `READ_ONLY_CONTROLLER`, `COMMUNICATOR`, `DIAGNOSTIC_BRIDGE`로 정의한다. Diagnostic Bridge를 Controller나 Communicator라고 부르지 않는다.

## 2. 전체 데이터 경로

```text
                                  Controller
                         Waveshare ESP32-S3 LCD 3.5
                         ┌────────────────────────┐
                         │ LVGL UI / touch        │
                         │ user intent validation │
                         │ ESP-NOW secure peer    │
                         └───────────┬────────────┘
                                     │ ESP-NOW 양방향
                                     │ CANView wire protocol
                                     ▼
                                  Communicator
┌─────────────────────────────────────────────────────────────────────┐
│ ESP32-S3-MINI-1-N4R2                                                │
│ ESP-NOW · pairing · session · raw telemetry queue · configuration   │
└──────────────────────────────┬──────────────────────────────────────┘
                               │ UART 4,000,000 baud, 8-N-1
                               │ RTS/CTS, framed + CRC, full duplex
                               ▼
┌─────────────────────────────────────────────────────────────────────┐
│ STM32G474CEU6                                                       │
│ monotonic timestamp · FDCAN1/2/3 · bus error · TX safety           │
└───────────────┬───────────────────┬───────────────────┬─────────────┘
                │ FDCAN1            │ FDCAN2            │ FDCAN3
                ▼                   ▼                   ▼
       TCAN1046AV ch.1      TCAN1046AV ch.2          MAX3055
       high-speed CAN       high-speed CAN       fault-tolerant CAN
       classic/CAN FD       classic/CAN FD        최대 125 kbps
                │                   │                   │
              CAN 1               CAN 2               CAN 3
```

실차 신호 검증에서는 Communicator가 같은 raw/stat 원천을 peer별 encrypted unicast로 나눈다.

```text
Communicator ──> Controller          정상 운전자 telemetry·의도 명령
       │
       └───────> Diagnostic Bridge   ID 통계·제한 raw capture, 차량 명령 없음
                           │
                           └─ SoftAP + HTTP/WebSocket ─> 휴대폰
```

ESP-NOW 1:N은 한 Communicator가 Controller와 Bridge에 각각 필요한 stream을 보내는 데 사용한다. broadcast raw telemetry나 ESP-NOW multi-hop mesh를 만들지 않는다. peer별 filter, quota, sequence, ACK 상태를 독립적으로 유지한다.

`CAN 1/2/3`은 PCB와 프로토콜의 논리 채널명이다. 실제 차량의 C-CAN, M-CAN, B-CAN 같은 이름·bitrate·커넥터 핀은 실차 캡처 전에는 고정하지 않는다. 특히 `MAX3055` 채널은 125 kbps fault-tolerant bus에만 연결할 수 있으며 고속 CAN 채널의 대체품이 아니다.

## 3. 책임과 안전 권한

| 계층 | 맡는 일 | 맡지 않는 일 |
|---|---|---|
| Controller | 사람이 읽을 상태 표현, 입력 debounce, Controller-local CAN allow-list, DBC catalog/decode, stale/error 표시, 검증된 audio·SPORT 명령 의도 생성 | raw CAN frame 생성, 차량 안전 조건 최종 판정, profile 밖 기능 제어 |
| Communicator ESP32-S3 | ESP-NOW 인증·세션·재전송, Controller별 권한, raw telemetry bridge, 무선/내부 UART queue | DBC 의미 해석, CAN peripheral 직접 제어, 최종 TX 허용 |
| Communicator STM32 | 3개 CAN의 hardware timestamp·bus/error 처리, raw stream, command allow-list, 차량 상태 재검증 | UI 상태를 차량 사실로 신뢰, 무선 payload 직접 신뢰, 화면용 DBC decode |
| Diagnostic Bridge | ID inventory, 행동 전후 capture, generic 후보 decode, evidence/export, schema 기반 설정 요청 | control lease, raw replay, 차량 CAN TX, DBC 후보 자동 확정 |
| CAN transceiver | 논리 신호와 차량 bus 물리계층 변환, standby, 물리 보호 일부 | bitrate 결정, 메시지 의미 판정 |

차량 CAN 송신의 최종 권한은 STM32에 둔다. ESP32나 Controller가 raw arbitration ID와 data를 전달해 즉시 송신하게 만들지 않는다. STM32는 컴파일된 command ID, 차량 profile, 최신 차량 상태, control lease, 물리 TX enable을 모두 확인한 뒤 제한된 frame만 만든다.

Primary Controller에는 CANView 기능에 필요한 control lease와 의미 명령 요청 권한을 준다. 허용 대상은 검증된 음량 offset, 취침·뒷좌석 강화 profile 내부의 fader/balance·main/rear mute, OEM audio snapshot 복원, SPORT button pulse와 관련 자동화다. 임의 sound-position UI가 없더라도 profile 내부 패닝 권한은 유지한다. Read-only Controller와 Diagnostic Bridge에는 이 권한을 주지 않는다.

## 4. 장애 시 기본 상태

- Controller가 재부팅되거나 ESP-NOW가 끊겨도 Communicator는 차량 bus를 방해하지 않는다.
- Communicator ESP32가 멈추면 STM32는 UART heartbeat 만료 후 새 제어 명령을 거부한다.
- STM32가 재부팅되면 외부 pull resistor가 TCAN 두 채널을 standby, MAX3055를 Power-On Standby로 만든다. HSE와 세 FDCAN이 준비되고 listen-only profile을 검증한 뒤에만 채널별로 normal mode를 허용한다.
- UART framing 오류, sequence 불일치, queue overflow는 raw 명령 재시도로 해결하지 않는다. session 재동기화와 snapshot을 먼저 수행한다.
- MAX3055 채널의 bitrate 또는 bus 유형이 확인되지 않으면 해당 채널은 전기적으로 standby 상태를 유지한다.
- 차량 전원은 LM74800과 back-to-back N-FET, MAX20040 5 V, PGOOD-gated TPS629210 3.3 V 순서로 기동한다. TLV803E가 3.3 V brownout 동안 STM32 NRST와 ESP32 CHIP_PU를 동시에 low로 유지한다.
- UART RTS/CTS 외부 pull-up은 두 MCU 중 하나가 reset된 동안 양방향 송신을 막는다. heartbeat와 control lease는 reset 후 자동 승계하지 않는다.
- Diagnostic Bridge가 멈추거나 Wi-Fi client가 과도한 요청을 보내면 Bridge용 P4 raw capture부터 drop한다. Controller telemetry와 P0/P1 queue는 유지한다.
- Bridge reboot, phone disconnect, diagnostic lease timeout은 capture/filter 변경 권한만 회수한다. 차량 control lease와 Communicator PHY state를 바꾸지 않는다.
- Bridge SoftAP는 설치 ESP-NOW channel에 고정하며 외부 AP나 휴대폰 hotspot에 station으로 연결하지 않는다.

## 5. 관련 문서

- [구현 준비 기준·정본·gate](implementation-readiness.md)
- [프로토콜 인덱스](protocols/README.md)
- [Diagnostic Bridge](diagnostic-bridge.md)
- [Controller CAN pipeline](controller-can-pipeline.md)
- [기능 설계](features.md)
- [문서 지도](../README.md) — hardware·development·vehicle·UI·task·review 진입점
