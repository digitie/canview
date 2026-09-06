# CANView 프로토콜 아키텍처

이 디렉터리는 장치 간 wire 계약의 상세 설계 정본이다. 전체 신뢰 경계는 [architecture overview](../README.md), 구현 순서와 gate는 [implementation readiness](../implementation-readiness.md)를 따른다.

| 링크 | 범위 | 함께 볼 task |
|---|---|---|
| [ESP-NOW](esp-now.md) | pairing, session, frame, QoS, peer filter, ACK/result | T-002, T-003, T-201, T-203 |
| [Communicator UART](communicator-uart.md) | 4 Mbps framing, semantic message, backpressure, idempotency | T-004, T-104, T-202 |
| [위치·관성·기압 확장](navigation.md) | ESP-NOW1.4/UART1.1 추가 센서, 구독·bandwidth·최종 결과 | T-002, T-004, T-100b |

프로토콜을 변경할 때만 두 문서 중 관련 문서를 읽는다. 전체 protocol 문서를 일반 UI·하드웨어 작업의 선행 문맥으로 불러오지 않는다.

추가 센서의 wire layout은 `protocol/schema/navigation-v1.json`과 host codec으로 검증한다. 기존 전체 transport schema의 동결은 T-002/T-004에 남아 있다. Markdown, generated header와 codec을 서로 다른 값으로 수동 관리하지 않는다.
