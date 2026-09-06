# T-002 ESP-NOW v1.3 schema와 생성 header 동결

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G0`
- 선행: `T-001`
- 후속: `T-003`, `T-005`, `T-200`, `T-300`, `T-400`

## 목표

현재 메시지 번호와 일부 packed struct만 있는 protocol을 완전한 machine-readable ABI로 바꾼다. 구현자가 discovery, HELLO, capability, lease, snapshot, diagnostic, bulk payload를 추측하지 않게 한다.

## 고정 결정

- 첫 구현 version은 `1.3`; 미완성 `1.2` runtime compatibility는 제공하지 않는다.
- 일반 frame은 32-byte header 포함 최대 240 byte, little-endian이다.
- `protocol/schema/espnow-v1.3.yaml`이 정본이고 `protocol/canview_protocol.h`는 생성물이다.
- role은 `COMMUNICATOR`, `PRIMARY_CONTROLLER`, `READ_ONLY_CONTROLLER`, `DIAGNOSTIC_BRIDGE` 네 개다.
- RTC·밝기·FFT·idle 설정은 Controller owner이며 vehicle command enum에서 제거한다.
- runtime quality와 evidence grade를 별도 enum으로 생성한다.
- diagnostic `0x28–0x2D`, remote config `0x43–0x45`를 v1.3에 포함한다.
- 모든 reserved bit/byte는 송신 0, 수신 nonzero 거부다.
- command request에 immutable `issued_at_controller_ms`와 `control_sync_generation`을 넣어 retry가 TTL을 늘리지 못하게 한다. 정본 필드명과 canonical control_tag 입력을 모든 hop에서 유지한다.
- pairing transcript는 최신 ESP-NOW 정본의 단계별 prefix를 서명한다. DISCOVERY는 pair binding·발신 nonce·제안 range, REQUEST는 discovery digest·상대 nonce, CHALLENGE는 full transcript·locally authorized 값, CONFIRM은 transcript hash를 bind한다. 첫 DISCOVERY에 아직 없는 상대 nonce/선택 version을 요구하지 않으며 requested role은 권한 부여 근거가 아니다.
- 설치 전체 공유 secret은 금지한다. 장치 로컬 PMK와 직접 두 endpoint만 가진 pair별 `link_root`/LMK를 사용한다.
- `CAN_EVENT_MARKER`만 marker를 표현하며 `CAN_CAPTURE_CONTROL`에는 MARK action을 두지 않는다.
- stage/status는 `u8`, reason/error는 `u16`, revision은 `u32`, boot 누적 counter는 saturating `u64`로 고정한다.
- vehicle-control envelope는 Controller↔STM pair-specific control generation/tag, origin device/boot/session, immutable issued time/TTL과 control-sync generation을 가진다.
- ACK와 모든 command/config RESULT는 original sequence와 request token을 함께 가진다.

## schema 필수 항목

각 message에 아래 필드를 적는다.

```text
numeric ID, symbolic name, since version
allowed sender role / receiver role / link state
QoS, priority, ACK 여부, response message
exact size 또는 prefix + record size + count 상한
field offset/type/endian/range/reserved
duplicate/idempotency key
sensitive-log policy
```

payload 범위는 [통합 설계 §7](../architecture/implementation-readiness.md#7-protocol-구현-기준)의 모든 계열을 포함한다. control scope가 CAPABILITIES wire에 실제로 존재해야 하고, snapshot에는 STM boot ID·TX build mode·hard gate·profile digest·safety inhibit가 있어야 한다.

## 예상 변경 파일

```text
protocol/schema/espnow-v1.3.yaml
protocol/canview_protocol.h
protocol/golden/espnow-v1.3/*.json
protocol/golden/espnow-v1.3/*.bin
tools/generate_protocol.py
tests/protocol/test_schema.py
docs/architecture/protocols/esp-now.md
```

## 구현 순서

1. 기존 header enum/struct를 schema로 옮기고 숫자를 보존한다.
2. 누락 payload와 role/link-state matrix를 추가한다.
3. `CAN_BATCH` 12-byte prefix는 유지하고 STM reboot 시 session 교체 규칙을 schema companion rule로 넣는다.
4. Controller-local command/config를 제거하고 remote config owner를 명시한다.
5. C11 static assertion과 enum known-mask를 생성한다.
6. 각 message의 최소·최대 길이 table을 생성한다.
7. 최소 한 개의 정상 vector와 경계/invalid vector를 생성한다.
8. 기존 prose 문서를 schema field 이름과 자동 대조한다.
9. pair별 provisioning record와 Bridge 침해 격리 vector를 schema companion policy로 생성한다.

## 수용 기준

- [ ] 선언된 모든 message ID에 payload schema 또는 명시적 `payload: none`이 있다.
- [ ] role/state/direction/QoS가 없는 message가 0개다.
- [ ] 모든 count 기반 payload가 208 byte 이하임을 generator가 증명한다.
- [ ] C header를 다시 생성해 diff가 없다.
- [ ] protocol minor assertion과 문서가 모두 `1.3`이다.
- [ ] RTC vehicle command와 owner 없는 config key가 남지 않는다.
- [ ] role/control scope가 local provisioning policy보다 넓게 협상될 수 없다.
- [ ] retry해도 command issued time과 TTL이 바뀌지 않는 golden vector가 있다.
- [ ] Bridge용 `link_root`/LMK로 Primary Controller session이나 scope를 인증할 수 없다.
- [ ] 모든 mutating marker 경로가 단일 message와 16-bit reason namespace를 사용한다.
- [ ] `u8` catalog revision, plain wrapping cumulative `u32` counter, `u8` common reason이 schema에 0개다.
- [ ] Bridge/read-only schema state에서는 control root/tag 생성 API와 nonzero control scope가 존재하지 않는다.
- [ ] C, Python에서 모든 golden binary size/field 값이 일치한다.

## 계획 보완 수용 기준

- [ ] navigation-v1.json의 ESP-NOW 1.4 확장을 companion schema로 연결하고 1.3 peer에는 새 sensor message/capability가 노출되지 않는다. 1.3 기본 ABI를 1.4로 조용히 재정의하지 않는다.
- [ ] bulk의 object/fragment/window/timeout/digest·config schema 16 KiB 제한, config owner/revision/status를 전 메시지 golden vector로 검사한다. 최신 정본의 wireless_session_id:u32와 공통 header/boot binding, capture STATUS의 reason:u16·구 reserved 0B·총44B를 exact offset/size vector로 대조한다.
- [ ] pairing 각 phase의 canonical prefix/domain·필드 순서·nonce/digest·authorized role/range 변경 negative vector를 독립적으로 고정한다. 권위 key record는 보호된 provisioning A/B, normal NVS는 cache로 구분한다. 이 문서 변경은 schema/codec/golden 구현 완료가 아니다.

## 검증 명령

```bash
python tools/generate_protocol.py --check
python -m pytest -q tests/protocol/test_schema.py
cmake --build --preset host-debug
ctest --preset host-debug -R protocol-schema --output-on-failure
```

## 안전·rollback

schema migration 전후 숫자표를 PR에 첨부한다. 이미 배포된 firmware가 없으므로 임시 alias를 추가하지 않는다. compatibility를 위해 불완전한 1.2 parser를 유지하면 task 실패다.


## 산출물·범위 경계

- 상기 schema 필수 항목과 예상 변경 파일이 구현 범위다. runtime codec/board 통합(T-003/T-201)은 범위 밖이다.
- ABI 동결 증거는 message별 golden·malformed·version/capability 결과와 generator digest로 남긴다.
