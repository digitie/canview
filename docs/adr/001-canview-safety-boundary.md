# ADR-001: Controller·Communicator·Diagnostic Bridge와 차량 송신 안전 경계

- 상태: accepted
- 날짜: 2026-09-04
- 근거 문서: `docs/architecture/implementation-readiness.md`, `docs/reviews/adversarial/2026-09-04-baseline-design.md`

## 컨텍스트

CANView는 운전자 화면, CAN 수집·안전 계층, 신호 검증 도구의 요구가 서로 다르다. 무선 peer나 후보 DBC 신호가 차량 CAN 송신 권한으로 확장되면 reset·통신 오류·잘못된 해석이 실제 차량 동작으로 이어질 수 있다.

## 결정

Controller, Communicator, Diagnostic Bridge를 분리한다. Communicator STM32가 generated command profile, local safety snapshot, lease, build mode, hardware TX gate를 최종 확인한다. Diagnostic Bridge는 read-only observer로 제한한다. 차량 송신은 CAPTURE_ONLY → BENCH_TX → VEHICLE_TX 순서의 gate를 통과해야 한다.

## 결과

- UI와 진단 개발은 차량 TX 없이 계속할 수 있다.
- 신호 후보와 제어 명령은 evidence가 확인될 때까지 승격되지 않는다.
- 하드웨어·firmware·protocol·차량 시험이 모두 필요하므로 초기 구현 속도는 느려진다.

## 후속

- T-001, T-002~T-006에서 ABI와 생성 profile을 고정한다.
- T-100~T-106에서 STM32와 hard TX gate를 구현·검증한다.
- T-500 이후에만 vehicle capture와 제한 송신을 검토한다.
