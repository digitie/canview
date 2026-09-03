# CANView 시스템 아키텍처와 명명 규칙

## 1. 정식 장치명

CANView는 다음 두 장치로 구성한다. 문서, 펌웨어, 로그, UI에서 아래 이름을 정본으로 사용한다.

| 정식 명칭 | 하드웨어 | 핵심 책임 |
|---|---|---|
| **Controller** | Waveshare `ESP32-S3-Touch-LCD-3.5` | 운전자 UI, 터치 입력, 상태 표시, ESP-NOW 명령 요청 |
| **Communicator** | `ESP32-S3-MINI-1-N4R2` + `STM32G474CEU6` + `TCAN1046AV-Q1` + `MAX3055` | 차량 CAN 3채널 수집·송신, DBC decode, 안전 정책, ESP-NOW 연결 |

`display`, `screen`, `gateway`는 일반 설명이나 외부 문서의 고유 용어가 아닌 이상 장치명으로 사용하지 않는다. 프로토콜 role 값은 `CONTROLLER`, `READ_ONLY_CONTROLLER`, `COMMUNICATOR`로 정의한다.

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
│ ESP-NOW · pairing · session · telemetry queue · configuration       │
└──────────────────────────────┬──────────────────────────────────────┘
                               │ UART 4,000,000 baud, 8-N-1
                               │ RTS/CTS, framed + CRC, full duplex
                               ▼
┌─────────────────────────────────────────────────────────────────────┐
│ STM32G474CEU6                                                       │
│ monotonic timestamp · FDCAN1/2/3 · filter · bus error · TX safety  │
└───────────────┬───────────────────┬───────────────────┬─────────────┘
                │ FDCAN1            │ FDCAN2            │ FDCAN3
                ▼                   ▼                   ▼
       TCAN1046AV ch.1      TCAN1046AV ch.2          MAX3055
       high-speed CAN       high-speed CAN       fault-tolerant CAN
       classic/CAN FD       classic/CAN FD        최대 125 kbps
                │                   │                   │
              CAN 1               CAN 2               CAN 3
```

`CAN 1/2/3`은 PCB와 프로토콜의 논리 채널명이다. 실제 차량의 C-CAN, M-CAN, B-CAN 같은 이름·bitrate·커넥터 핀은 실차 캡처 전에는 고정하지 않는다. 특히 `MAX3055` 채널은 125 kbps fault-tolerant bus에만 연결할 수 있으며 고속 CAN 채널의 대체품이 아니다.

## 3. 책임과 안전 권한

| 계층 | 맡는 일 | 맡지 않는 일 |
|---|---|---|
| Controller | 사람이 읽을 상태 표현, 입력 debounce, 명령 의도 생성, stale/error 표시 | raw CAN frame 생성, 차량 안전 조건 최종 판정 |
| Communicator ESP32-S3 | ESP-NOW 인증·세션·재전송, Controller별 권한, 무선/내부 UART queue | CAN peripheral 직접 제어, 최종 TX 허용 |
| Communicator STM32 | 3개 CAN의 hardware timestamp·filter·error 처리, DBC 입력용 raw stream, command allow-list, 차량 상태 재검증 | UI 상태를 차량 사실로 신뢰, 무선 payload 직접 신뢰 |
| CAN transceiver | 논리 신호와 차량 bus 물리계층 변환, standby, 물리 보호 일부 | bitrate 결정, 메시지 의미 판정 |

차량 CAN 송신의 최종 권한은 STM32에 둔다. ESP32나 Controller가 raw arbitration ID와 data를 전달해 즉시 송신하게 만들지 않는다. STM32는 컴파일된 command ID, 차량 profile, 최신 차량 상태, control lease, 물리 TX enable을 모두 확인한 뒤 제한된 frame만 만든다.

## 4. 장애 시 기본 상태

- Controller가 재부팅되거나 ESP-NOW가 끊겨도 Communicator는 차량 bus를 방해하지 않는다.
- Communicator ESP32가 멈추면 STM32는 UART heartbeat 만료 후 새 제어 명령을 거부한다.
- STM32가 재부팅되면 모든 transceiver를 standby로 시작하고, 세 CAN 채널을 listen-only로 검증한 뒤에만 정책에 따라 상태를 전환한다.
- UART framing 오류, sequence 불일치, queue overflow는 raw 명령 재시도로 해결하지 않는다. session 재동기화와 snapshot을 먼저 수행한다.
- MAX3055 채널의 bitrate 또는 bus 유형이 확인되지 않으면 해당 채널은 전기적으로 standby 상태를 유지한다.

## 5. 관련 문서

- [Controller 하드웨어와 Waveshare 핀맵](hardware-and-development.md)
- [Communicator 회로·IC·핀맵](communicator-hardware.md)
- [개발환경과 빌드](development-environments.md)
- [Communicator MCU 간 UART 프로토콜](communicator-uart-protocol.md)
- [Controller–Communicator ESP-NOW 프로토콜](esp-now-protocol.md)
- [기능 안전 설계](feature-design.md)
