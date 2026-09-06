# 2026-09-05 R1 상세 회로·센서 프로토콜 적대적 리뷰

- Review ID: `CANVIEW-R1-HARDWARE-NAV-20260905`
- 종류: 전문 reviewer 서브에이전트2인 독립 적대적 리뷰
- 최초 candidate: `06bb51c72180f9c040db3ccf0b223a823c570409`
- parent/base: `1ed2576841075a1f98fb10e979736941f3deed72`
- 기능 수정: `59b2412627eb9773410545690f0609175b80e756`
- 참조 쪽수 교정 포함: `57ac03a7394c98a1464bf0d9f2777df666f711ad`
- 상세 sheet 수 표기 교정: `c1c15b573476e176e0f445ea74d408bc923fa025`
- 범위: 네 보드 상세 KiCad/netlist/BOM/footprint/핀맵, 전원·CAN 차단, GNSS/INS·기압·원격 mic, navigation wire/schema/host codec와 관련 R1 문서
- 범위 밖: PCB 제작·실장, loop/SOA/EMC/SI·HIL·차량 승인, target sensor firmware 구현, 기존 UI/DBC 승격
- 요청: 가격보다 소형화 우선의 상세 회로·원문 PDF 보존·FW pinmap·센서/프로토콜 확장
- 관련 task: [T-100](../../tasks/T-100-communicator-schematic.md), [T-100b](../../tasks/T-100b-navigation-audio-bringup.md)
- Coordinator: Codex
- 상태: `PASS` — 설계 검토본 통합에 한정, 제작·G1·HIL·차량 TX 승인 아님

## 1. 공통 manifest와 독립성

[최초 요청 원문](evidence/2026-09-05-r1-hardware-navigation-manifest.md), [수정본 요청 원문](evidence/2026-09-05-r1-hardware-navigation-postfix-manifest.md)을 보존했다. 두 최초 원문을 확정·저장한 뒤에만 교차 공개했다. source는 Windows native Git의 고정 객체로만 읽었고 working tree는 리뷰 입력이 아니다. 따라서 clean 판정은 두 reviewer 모두 `N/A — object-only`다.

| 항목 | Reviewer A | Reviewer B |
|---|---|---|
| 전문 영역 | 전원·CAN·reset·고장 차단 | 센서·핀/land·protocol·생성물 정합성 |
| 서브에이전트 | `01a06f2e-7129-7261-99bc-1bf2a1d6d490` | `01a06f2e-735a-7221-9541-de848f1984b2` |
| 실행 ID | `CANVIEW-R1-A-20260905T034626Z` | `CANVIEW-R1-B-20260905T034629Z` |
| 최초 UTC |03:46:26~03:59:16|03:46:29~04:00:51|
| 실제 candidate | `06bb51c72180f9c040db3ccf0b223a823c570409` | 동일 |
| 원본 | [A 전체](evidence/2026-09-05-r1-hardware-navigation-reviewer-a.md) | [B 전체](evidence/2026-09-05-r1-hardware-navigation-reviewer-b.md) |
| 최초 verdict | `BLOCK` | `CONDITIONAL` |

## 2. Reviewer A findings

| ID | 등급 | 위치·근거 | 실패·영향 | 권고 |
|---|---|---|---|---|
| R1-A-001 | P1 | Q1/Q2 BUK9Y12 p2, LM7480 p6 | DC VGS±10V 부품에11~14.5V 지속 게이트 구동, oxide 손상 가능 | DC±20V 적합 FET 또는 clamp 재설계 |
| R1-A-002 | P1 | U28/U29 AUTO5V, `/OE`는 PHY3V3 pull-up | PHY3V3만0V면 FT gate enable, physical arm/WD 우회 | PHY 소실 시 기본 disable인 교차 도메인 회로 |
| R1-A-003 | P1 | U1 TUSB320LAI VDD=VBUS, DS p4 | USB5.25V에서 권장 VDD5.0V 초과, 전류 허가 동작 보장 불가 | limiter 앞 독립 USB3.3V |
| R1-A-004 | P2 | artifact SHA·실제 Git blob | 네 보드 `.net`/connectivity8개 hash 불일치 | LF 정규화·Git blob 재검사 |

## 3. Reviewer B findings

| ID | 등급 | 위치·근거 | 실패·영향 | 권고 |
|---|---|---|---|---|
| B-R1-001 | P2 | navigation schema/codec/계약 | 정의 없는 IMU bit6, profile2의 DR 광고 수용 | 배열 기반 mask 및 profile/DR/age 교차 검사 |
| B-R1-002 | P2 | SENSOR_RESULT status3 | UNAUTHORIZED인데 revision·slot·digest 비공개 규칙 미강제 | 공개 correlation 외0 강제·양방향 거부시험 |
| B-R1-003 | P2 | Git blob SHA | A-004와 같은8개 CRLF/LF mismatch | 실제 commit bytes 무결성 경로 |
| B-R1-004 | P3 | Waveshare header25/27 | NC이지만 TX43/RX44 이름 반전으로 미래 배선 오인 | 공식 접점 이름 교정, NC 유지 |

B의 추가 관찰인 version `[1,4,999]`/`[1,4.0]` 수용도 정확히2개 u8 검사와 거부시험으로 반영했다. [Coordinator 감사](evidence/2026-09-05-r1-hardware-navigation-coordinator-audit.md)의 C-01/C-02는 각각 hash/FT rail 문제와 중복이며 독립 추가 결함 수로 부풀리지 않는다.

## 4. 최초 통합 판정

P1 세 건으로 최초 `BLOCK`이다. B가 자기 영역에서 P0/P1을 찾지 못한 사실이 A의 finding을 상쇄하지 않는다. 양쪽 P2 hash finding은 회로 변조 증거가 아닌 바이트 재현성 결함이다. 제작 차단 문구나 미완료 HIL을 P1의 대체 closure로 쓰지 않았다.

## 5. 수정과 재검증

| Finding | 수정 내용 | 재검증 | 원 reviewer closure |
|---|---|---|---|
| R1-A-001 | BUK7Y12 DC±20V, 같은 LFPAK56·원문 추가 | net/MPN 회귀, LM max14.5/13V 대조 | FIXED — A 재확인 |
| R1-A-002 / C-02 | AHCT126 active-high OE마다1k GND, AUTO5V 공급 유지 | exported pin/pull oracle,32조합 정상상태 Boolean | FIXED — A 재확인 |
| R1-A-003 | U16 USB_CC3V3, U1/U2/pull-up 공급 분리·3k 최소부하 | 두 보드 rail 경로,3.2505~3.3495V 정적 계산 | FIXED — A 재확인 |
| R1-A-004 / B-R1-003 / C-01 | LF normalize·gitattributes·immutable 검사 | 59b2412와57ac03a Git blob16개 hash mismatch0 | FIXED — A/B 재확인 |
| B-R1-001 | mask/profile/DR/NAV/age 교차 조건 | encode와 검증 우회 raw-wire decode 거부 | FIXED — B 재확인 |
| B-R1-002 | UNAUTHORIZED 공개3필드 외0 | 비공개 필드 각각 비영 값 거부 | FIXED — B 재확인 |
| B-R1-004 |25=TX43,27=RX44,둘 다NC | generated header name/net 회귀 | FIXED — B 재확인 |

모든 기능 수정은59b2412에 들어 있다.57ac03a는 AHCT 원문 도면의 DBV17~19쪽/DCK20~22쪽 구분 한 줄만 교정하며 회로·land geometry 변경은 없다. [A 수정 후 원문](evidence/2026-09-05-r1-hardware-navigation-postfix-reviewer-a.md)과 [B 수정 후 원문](evidence/2026-09-05-r1-hardware-navigation-postfix-reviewer-b.md)에서 원 reviewer가 각 finding을 재확인했다. 추가 version boundary도 B가 FIXED로 확인했다. 심각도 하향·DEFERRED·근거 없는 risk acceptance는 없다.

## 6. 실제 실행한 검증

- Windows KiCad10.0.6 `tools/hardware/export-review.ps1`: 네 보드 ERC0/waiver0,343 BOM item/1,235 named pad 정합성 PASS.
- Windows Python `-m unittest discover -s tools/protocol -v`:16/16 PASS.
- Windows KiCad Python `-m unittest discover -s tools/hardware -v`:5/5 PASS. 회로 조건의 Boolean/net 회귀이지 과도응답 simulation이 아니다.
- `validate_exports.py --git-revision 59b2412627eb9773410545690f0609175b80e756`:16 artifact의 Git blob SHA mismatch0.
- 제조사 PDF56개/1,745쪽/95,230,736byte의 hash·bytes·전체 페이지 parse 오류0.
- staged secret/private-key/token/credential URL 및 금지 local 파일 검사 오류0. synthetic host fixture와 공개 제조사 원문만 보존했다.

## 7. Post-fix 재검토와 최종 판정

| 항목 | Reviewer A | Reviewer B |
|---|---|---|
| 실행 ID | `CANVIEW-R1-A-POSTFIX-20260905T041737Z` | `CANVIEW-R1-B-CLOSURE-20260905T041729Z` |
| 시작·종료 UTC |04:17:37~04:25:40|04:17:29~04:25:05|
| 실제 최종 대상 | `57ac03a7394c98a1464bf0d9f2777df666f711ad` | 동일 |
| 격리 | Windows Git object-only, clean N/A | 동일 |
| 원 finding | R1-A-001~004 모두 FIXED | B-R1-001~004·version 모두 FIXED |
| 신규 finding | P0~P3 발견 없음 | P0~P3 발견 없음 |
| verdict | PASS — 설계 검토본 통합 | PASS — 설계 검토본 통합 |

두 원문은 다른 reviewer의 post-fix 판정을 공유하기 전에 보존했다. 두 reviewer 모두 전체 수정 delta를 다시 검토했고 16개 protocol·5개 hardware 시험, 실제 Git blob digest, 56개 원문 PDF를 독립 확인했다. native 전체 KiCad exporter/ERC는 Coordinator가 실행했으며 reviewer는 저장된 ERC를 검토한 것이다. 독립 리뷰 실행을 실물 시험이나 승인된 PCB로 과장하지 않는다.

마지막 `c1c15b5`는 Communicator README의 상세 sheet 수를27→28로 고치는 한 줄뿐이다. 기능·산출물은57ac03a와 동일하다. [A 추가 원문](evidence/2026-09-05-r1-hardware-navigation-count-reviewer-a.md), [B 추가 원문](evidence/2026-09-05-r1-hardware-navigation-count-reviewer-b.md)에서 root·연결 모델28개와 PDF29쪽을 각각 확인했고 PASS를 유지했다. A 실행은 `R1-A-COUNT-20260905T043052Z`(04:30:52~04:31:06 UTC), B 실행은 `R1-B-COUNT-20260905T043044Z`(04:30:44~04:31:26 UTC)이며 격리는 계속 object-only다. Coordinator의 최종c1c15b5 Git blob 검사도 mismatch0이다.

## 8. 물리 검증 경계

MAX20040 land90-0409 원본 overlay, 최신/미확보 PDF, 구매 MPN·harness·PCB/loop/SOA/EMC/SI·HIL은 [R1 제작 전 조건](../../hardware/r1/verification.md)에 남아 있다. P1 수정의 소스 리뷰 통과와 G1/실제 차량 TX 허가는 별개다. target sensor firmware와 session allocator/cache는 T-100b의 미구현 상태를 유지한다. PR은 기존 [#16](https://github.com/digitie/canview/pull/16)에 반영하며 이 작업에서 자동 merge하지 않는다.
