# T-003 ESP-NOW codec, parser, session과 QoS

- 상태: `DONE`
- 우선순위: `P0`
- Gate: `G0`
- 선행: `T-002`
- 병렬 가능: `T-004`, `T-005`
- 작업 branch: `agent/codex-t003-espnow-codec-session`
- PR: [#19](https://github.com/digitie/canview/pull/19) (draft)

## 목표

ESP-IDF에 종속되지 않는 공통 C codec과 state machine을 구현하고 세 ESP32 firmware가 같은 검증 코드를 사용하게 한다.

## 고정 결정

- untrusted byte를 packed struct로 cast하지 않는다.
- encoder/decoder는 caller-provided buffer만 사용하고 heap allocation하지 않는다.
- CRC-32/ISO-HDLC은 corruption 검사이며 인증으로 취급하지 않는다.
- production session은 encrypted peer context 없이는 ONLINE/control 상태가 될 수 없다.
- anti-replay는 session별 64-packet sliding window를 사용한다.
- QoS1은 initial 80 ms, adaptive timeout `clamp(2×SRTT+20,40,250)` ms, 총 3회 시도다. 실제 retry 대기는 매 시도 남은 TTL/2 이하로 제한하며 재전송 때문에 원래 TTL을 연장하지 않는다. 현재 tracker의 80→160 ms 동작과 정본 정책을 회귀시험으로 대조한다.
- command idempotency와 packet sequence duplicate를 별도 처리한다.
- QoS1 retry는 새 sequence와 동일한 token/issued time/TTL/argument bytes를 사용한다.
- auth backoff는 peer/candidate별이며 unknown-source flood가 정상 peer를 잠그지 못한다.

## 구현 범위

- byte reader/writer, CRC, header/payload length validator
- schema-generated message constraint table 적용
- pair별 `link_root` 기반 pairing transcript canonical encoding, HMAC/HKDF adapter interface와 장치 로컬 PMK lifecycle
- Controller↔STM control envelope/tag reference codec와 `CONTROL_TIME_SYNC` generation state
- HELLO/CAPABILITY/TIME_SYNC/SNAPSHOT link state machine
- sequence window, ACK/retry scheduler, error rate limiter
- fixed pool and queue-neutral API; ESP-IDF callback integration은 후속 task
- C reference와 독립 Python reference codec

## API 원칙

```c
canview_decode_result_t canview_frame_decode(
    const canview_transport_meta_t *meta,
    const uint8_t *bytes, size_t length,
    canview_peer_session_t *session,
    canview_decoded_frame_t *out);

canview_encode_result_t canview_frame_encode(
    const canview_encode_request_t *request,
    uint8_t *out, size_t capacity, size_t *written);
```

decode result는 `DROP_SILENT`, `ERROR_RATE_LIMITED`, `ACK_REQUIRED`, `DELIVER`를 구분한다. payload view의 lifetime은 input buffer보다 길지 않으며 worker queue로 넘길 때 명시적으로 복사한다.

## 수용 기준

- [x] 모든 v1.3 message 정상 vector가 C/Python round-trip한다.
- [x] length 0–1,500, bit flip, bad CRC/reserved/session이 sanitizer에서 안전하게 거부된다.
- [x] sequence wrap과 64-window reorder/duplicate가 정해진 결과를 낸다.
- [x] pairing nonce 재사용과 transcript replay가 거부된다.
- [x] self-requested Primary role/scope와 transcript field substitution이 거부된다.
- [x] 한 pair의 root/LMK fixture로 다른 pair의 transcript·session을 인증할 수 없다.
- [x] caller-owned two-slot key staging의 staged-not-ready/ready recovery 경계와 pair별 root 격리를 시험했다. 실제 NVS/Flash 전원 차단 주입은 NOT_RUN이다.
- [x] peer/session별 auth backoff와 encrypted session의 성공 복구 경계를 시험했다. 실제 unknown-MAC RF flood 중 peer 유지 시험은 NOT_RUN이다.
- [x] stale time-sync generation 또는 과도한 uncertainty의 command가 거부된다.
- [x] control tag의 origin/boot/session/scope/TTL/argument 한 bit 변경도 STM reference verifier에서 거부된다.
- [x] unencrypted transport metadata에서 control lease 발급 경로가 없다.
- [x] 1/5/20/50% loss simulation에서 QoS1 최대 시도와 TTL을 넘지 않는다.
- [x] ERROR/ACK/broadcast contract와 duplicate suppression이 response storm 경계를 넘지 않는다.
- [x] fixed pool·QoS pending 자원을 bounded하게 예약하고 고갈 시 BUSY/drop 경계를 지킨다. 실제 command executor queue·ACK admission 통합은 후속 task다.
- [x] 40/250 ms clamp·남은 TTL/2·TTL 만료·80→160 ms·마지막 시도 경계에서 총 3회와 immutable token/time/argument 계약을 보존한다.

## 검증 명령

```bash
cmake --build --preset host-sanitize
ctest --preset host-sanitize -R espnow --output-on-failure
python -m pytest -q tests/protocol/test_espnow_reference.py
python tests/protocol/fault_transport.py --seed 1 --loss 0,1,5,20,50
```

## 증거

golden vector count, fuzz seed corpus, branch coverage, sequence/session state graph를 PR artifact로 남긴다. cryptographic primitive 자체를 새로 구현하지 않고 ESP-IDF mbedTLS adapter와 host library를 사용한다.

현재 구현·검증 기록:

- generated contract는 CAPABILITIES와 COMMAND_REQUEST의 TLV policy를 함께 내보내며 C parser가 generated table만 사용한다. unknown optional/critical, fixed-size, singleton, truncated TLV regression을 `espnow-tlv-contracts`에서 검사한다.
- CTest에 `vectors`, `malformed`, `tlv-contracts`, `session`, `security`, `control`, `qos`, `pool-fuzz` 8개 C scenario와 독립 Python reference/fault transport를 등록했다. 최신 로컬 host Debug/Release 전체 suite는 각각 49/49, coverage core는 9/9, Python unit은 35개 PASS이며 Linux portability/sanitizer와 target CI는 PR check에서 추적한다.
- ESP-NOW 전용 계측은 70개 함수 100%, region 83.52%, line 70.51%, branch 61.26%를 기록했다. 기존 공용 foundation coverage gate는 9/9, core line/function 100%, branch 99.63%다.
- STM32 Debug/Release, 세 ESP32-S3 image와 public IDF fixture의 clean target 결과·artifact digest는 [T-003 target evidence](../reviews/adversarial/evidence/2026-09-07-T-003-target-final.md)에 둔다.
- 생성 check·negative fixture·budget/plan/link 검증과 hash-locked docs dependency 설치 및 API docs build도 PASS했다.
- 두 전문 reviewer의 원본과 disposition은 [T-003 review report](../reviews/adversarial/2026-09-07-T-003.md)에 보존했다. 최초 B-P1-01 evidence freshness는 target 재빌드·digest 갱신 후 양 reviewer가 `PASS`로 재확인했다.
- board flash, reset/brownout fault injection, RF, CAN/HIL, 차량, production OTA signing/provisioning, 실제 mbedTLS/CCMP runtime은 `NOT_RUN`이며 이 task의 host/compile PASS로 대체하지 않는다.


## 산출물·범위 경계

- 예상 산출물은 `shared/protocol/` 공통 codec·session API와 이 문서의 `tests/protocol/` reference/fault script다. IDF callback·radio/Flash adapter 구현은 범위 밖이다.
- API context는 caller 소유이며 session reset/destroy 전에 pending retry와 borrowed payload를 해제한다. 실패 시 ONLINE/control을 닫고 이전 session queue를 재사용하지 않는다.

## 구현 순서

1. 기존 foundation framing과 분리된 byte reader/writer, v1.3 message contract table, C/Python reference round-trip을 추가한다.
2. transport metadata·peer/session context·anti-replay·link state를 고정 pool과 함께 추가한다.
3. pairing transcript와 control tag는 crypto primitive를 재구현하지 않고 caller-owned HMAC/HKDF adapter에 연결한다. unencrypted metadata에서는 control path를 열지 않는다.
4. ACK/retry와 resource reservation을 bounded scheduler로 구현하고 loss·delay·duplicate·reorder fault 시험을 추가한다.
5. host sanitizer/coverage와 Arm/ESP target warning gate를 실행하고, 독립 전문 리뷰어 2명의 finding을 반영한 뒤 merge한다.
