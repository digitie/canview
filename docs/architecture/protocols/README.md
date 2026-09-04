# CANView 프로토콜 아키텍처

이 디렉터리는 장치 간 wire 계약의 상세 설계 정본이다. 전체 신뢰 경계는 [architecture overview](../README.md), 구현 순서와 gate는 [implementation readiness](../implementation-readiness.md)를 따른다.

| 링크 | 범위 | 함께 볼 task |
|---|---|---|
| [ESP-NOW](esp-now.md) | pairing, session, frame, QoS, peer filter, ACK/result | T-002, T-003, T-201, T-203 |
| [Communicator UART](communicator-uart.md) | 4 Mbps framing, semantic message, backpressure, idempotency | T-004, T-104, T-202 |

프로토콜을 변경할 때만 두 문서 중 관련 문서를 읽는다. 전체 protocol 문서를 일반 UI·하드웨어 작업의 선행 문맥으로 불러오지 않는다.

wire layout의 최종 정본은 향후 `protocol/schema/`에 들어갈 machine-readable schema다. Markdown, generated header와 codec을 서로 다른 값으로 수동 관리하지 않는다.
