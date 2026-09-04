# T-201 Communicator ESP-NOW provisioning, session과 QoS

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G2`
- 선행: `T-003`, `T-200`
- 병렬 가능: `T-202`

## 목표

공통 codec/state machine을 ESP-IDF ESP-NOW transport에 연결하고 Primary·read-only·Bridge peer의 신뢰 경계를 강제한다.

## 구현 범위

- USB provisioning command와 pair package 생성/import
- encrypted NVS peer record, 장치 로컬 PMK, pair별 `link_root`/LMK와 link generation
- 양쪽 physical pairing window와 pairing state machine
- peer allow-list, role, control scope, secure HELLO/TIME_SYNC/SNAPSHOT
- callback fixed pool, P0–P4 scheduler, one-send-at-a-time 규칙
- ACK/retry/RTT, anti-replay, reconnect/channel policy
- Primary 한 개, read-only 한 개, Bridge 한 개 운영 quota
- key rotate/delete와 auth lockout
- requested role과 local provisioning authorization 분리, transcript의 role/scope/device/MAC binding
- unknown source와 known peer별 독립 rate limit/backoff
- 빈 NVS의 USB 우선 provisioning과 gate-off `CAPTURE_ONLY` read-only service bootstrap
- vehicle command canonical envelope/control tag를 변경 없이 UART로 전달하는 opaque route

## 안전 규칙

- 기본 PMK, hard-coded 공용 key, plaintext fallback을 금지한다.
- installation-wide shared secret과 한 pair에서 다른 pair key를 파생하는 구조를 금지한다.
- MAC 주소만으로 role을 부여하지 않는다.
- read-only와 Bridge control scope는 항상 0이다.
- encrypted peer context와 current snapshot 전에는 control lease를 주지 않는다.
- broadcast에는 CAN/VIN/location/secret을 넣지 않는다.

## 수용 기준

- [ ] 두 physical window가 없으면 새 peer 등록 0건이다.
- [ ] transcript replay/nonce reuse/default PMK/plaintext control이 거부된다.
- [ ] peer가 요청한 role/scope가 local 승인값을 넓히지 못한다.
- [ ] Bridge NVS/pair package 유출 fixture로 Communicator↔Primary Controller를 인증할 수 없다.
- [ ] 한 link key 회전/손상이 다른 established peer를 offline 또는 rekey 상태로 만들지 않는다.
- [ ] unknown MAC flood가 정상 peer를 global lockout시키지 않는다.
- [ ] 세 role이 capability와 권한 matrix대로 동작한다.
- [ ] send callback success만으로 command UI가 완료되지 않는다.
- [ ] 1/5/20/50% loss·duplicate·reorder·reboot에서 state가 복구된다.
- [ ] channel mismatch에서 운행 중 무제한 scan/hopping하지 않는다.
- [ ] queue 포화에서 P0/P1이 P3/P4 때문에 drop되지 않는다.
- [ ] 빈 NVS와 차량 speed 미확정에서도 hard gate off read-only provisioning은 가능하지만 scope/lease/TX는 0이다.
- [ ] ESP가 command origin/scope/TTL/argument를 바꾸면 STM end-to-end tag 검증이 실패한다.
- [ ] secret·nonce 전체·VIN이 log에 나오지 않는다.

## 검증

```bash
idf.py build
ctest --preset host-sanitize -R espnow --output-on-failure
python tests/hil/run_espnow_faults.py --roles primary,readonly,bridge --seed 1
```

## evidence

각 role의 capability snapshot, packet loss 결과, queue high-water, NVS power-loss key rotation 결과를 G2 bundle에 넣는다. 실제 secret은 artifact에서 redaction한다.
