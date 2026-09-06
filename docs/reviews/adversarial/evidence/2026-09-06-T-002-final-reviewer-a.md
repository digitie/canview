# T-002 최종 적대적 리뷰 evidence — reviewer A

## 리뷰 식별

- 전문 영역: embedded safety·protocol·security·target boundary
- 기준 commit: `fb30b29e4e0ac27170bce8bb989e0d93e9d861a`
- parent: `fe564fec1214b7d4daf9799a8390c921e273f08e`
- 최종 실행 ID: `07c18b40-8605-4c9f-8939-1d3c9be0678d`
- 실행 시각: `2026-09-06T23:43:20.1680589+09:00` – `2026-09-06T23:52:50.9071373+09:00`
- worktree: detached immutable archive, 시작·종료 clean
- 최종 판정: `PASS`

## 실행 결과

다음 검사를 exact target에서 수행했다.

- `python -B tools/generate_protocol.py --check`: PASS, 15 golden·6 malformed·4 compatibility·pairing
- `python -B tests/protocol/test_schema.py -v`: PASS, 24/24
- `tools/check_generated.py`, `tools/check_negative_fixtures.py`, `tools/check_budgets.py`: PASS
- 전체 Python unittest: PASS, 35/35
- generated inventory·duplicate vector·LF/CRLF canonical digest: PASS
- `role.macro_prefix=CANVIEW_CONTROL_SCOPE`: `CANVIEW_CONTROL_SCOPE_KNOWN_MASK` 충돌로 거부
- clear `DISCOVERY`·`PAIR_REQUEST`·`PAIR_CHALLENGE` decode에서 sender·receiver·link-state·expected session context 누락을 각각 `context_required`로 거부
- `BULK_BEGIN.timeout_ms`: 1000·30000 허용, 999·30001·120000 거부
- DIAGNOSTIC_LEASE 양방향 variant 및 역방향 거부: PASS

## 이전 finding disposition

- F-01 variant sender/receiver 방향: `CLOSED`
- F-02 stateful response correlation/token/sequence matching: `T-003`로 명시적 이관. T-002에는 static nonzero correlation 정책만 남김
- F-03 bulk range·bitmap/window: `CLOSED`
- F-04 public enum compatibility: `CLOSED`
- F-05 generated macro namespace collision: `CLOSED`

P0/P1/P2 finding은 없다. 최종 라운드 P3도 없다.

## 미실행·범위 밖

- reviewer 환경의 독립 CMake/CTest·C99/C11 compiler, STM32/ESP-IDF target build, Windows sanitizer
- RF·HIL·차량·보드 flash·ESP-NOW runtime replay 및 T-003 stateful session 시험

target binary와 host coverage는 coordinator가 직접 실행한 별도 evidence에 기록했다. 이 리뷰는 차량 송신·production OTA 승인으로 해석하지 않는다.
