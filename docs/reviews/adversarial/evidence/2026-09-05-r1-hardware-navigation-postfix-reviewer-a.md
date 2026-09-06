# Reviewer A 수정 후 원문

- 대상: `57ac03a7394c98a1464bf0d9f2777df666f711ad`
- 원 서브에이전트: `01a06f2e-7129-7261-99bc-1bf2a1d6d490`
- [공통 요청](2026-09-05-r1-hardware-navigation-postfix-manifest.md)
- 아래는 다른 reviewer의 수정 후 판정을 전달하기 전에 보존한 최종 응답 원문이다.

---

## PASS — 설계 검토 통합에 한정

**R1-A-001~004 모두 FIXED로 판정합니다.** 최종 `57ac03a`까지 확인했으며, 신규 OPEN P0/P1/P2/P3는 발견하지 못했습니다. **PCB 제작·G1·HIL·차량 TX 승인은 아닙니다.**

### 실행·격리 기록

| 항목 | 결과 |
|---|---|
| 실행 ID | `CANVIEW-R1-A-POSTFIX-20260905T041737Z` |
| 시작 / 종료 UTC | `2026-09-05 04:17:37` / `04:25:40` |
| 최초 candidate | `06bb51c72180f9c040db3ccf0b223a823c570409` |
| 수정본 | `59b2412627eb9773410545690f0609175b80e756` |
| 최종 대상 | `57ac03a7394c98a1464bf0d9f2777df666f711ad` |
| 관찰 parent | 위 세 commit의 연결 관계가 요청과 일치 |
| 격리 | Windows native Git object-only. 소스·PDF·시험 입력은 Git blob bytes |
| clean / 변경 | **N/A—working tree 미사용.** 파일 수정·Git mutation 없음 |

보존된 최초 A/B 원문과 coordinator 감사는 교차 확인했습니다. 다른 reviewer의 post-fix 판정은 읽지 않았습니다.

## 원 finding closure

### R1-A-001 · P1 → FIXED

위치: [power_circuits.py:86](/mnt/f/dev/canview/tools/hardware/power_circuits.py:86).

Q1/Q2가 `BUK7Y12-100E`로 교체됐고, S1–3=CS/G4/드레인 베이스 연결과 LFPAK56 footprint가 유지됩니다.

- 제조사 **DC VGS ±20V**
- LM74800 HGATE 최대14.5V / DGATE 최대13V
- DC 정격 여유 **5.5V / 7V**

기존 지속 gate 과전압 원인은 제거됐습니다. 동적 ringing·SOA·열 검증은 별개입니다. 근거: [BUK7Y12 PDF p2](https://assets.nexperia.com/documents/data-sheet/BUK7Y12-100E.pdf), [LM7480 PDF p6](https://www.ti.com/lit/ds/symlink/lm7480-q1.pdf).

### R1-A-002 · P1 → FIXED

위치: [core_circuits.py:111](/mnt/f/dev/canview/tools/hardware/core_circuits.py:111).

| 실제 연결 | 수정 결과 |
|---|---|
| U28/U29 | `CAHCT1G126DBVRQ1`, AUTO5V 공급 |
| U28 OE(1) | `TX_PERMIT`, R66 **1kΩ→GND** |
| U29 OE(1) | `RX_ALLOWED`, R67 **1kΩ→GND** |
| PHY3V3=0, AUTO5V 정상 | OE LOW → 출력 Z → 외부 pull로 **TXD HIGH / EN LOW** |

제조사 active-high 진리표와 power-up/down pull-down 권고에 부합합니다. 입력 누설은 VCC=0에서도 규정돼 있으며, 일반적인 **출력 Ioff를 가정하지 않았습니다**. [AHCT PDF p3–5, p8](https://www.ti.com/lit/ds/symlink/sn74ahct1g126-q1.pdf)

추가 확인:

- 약3.3mA/OE 부하는 U33/U36의 구동 범위 안입니다. VCC=3V에서 VOH 최소2.4V 조건이 AHCT VIH 2V보다 높습니다. [LVC1G11 p5](https://www.ti.com/lit/ds/symlink/sn74lvc1g11.pdf)
- PHY 무전원 시 관련 누설을 보수적으로 합산해도 1kΩ pull-down의 정상상태 LOW 여유가 있습니다.
- SYS reset·PHY POR·physical disarm·watchdog clear 경로와 새 ARM edge 요구가 유지됩니다.
- FD 쪽 active-low OE 연결과 FT 쪽 active-high OE가 혼용되지 않았습니다.

**기존 settled-state fail-open은 해결됐습니다.** 급격한 rail collapse의 지연·glitch는 여전히 실측 대상입니다.

### R1-A-003 · P1 → FIXED

위치: [power_circuits.py:35](/mnt/f/dev/canview/tools/hardware/power_circuits.py:35).

Communicator/Bridge 모두:

- U16 `TPS7A2033PDBVR`: **IN1·EN3=USB_VBUS, GND2, OUT5=USB_CC3V3**
- U1 VDD12, OUT1 pull-up, U2 VCC5가 USB_CC3V3로 이동
- limiter/mux 뒤 SYS rail에 의존하지 않음
- 3kΩ 최소부하와 입력1µF·출력2.2µF 추가

DBV ±1.5% 조건의 출력 **3.2505–3.3495V**는 TUSB320LAI 권장2.7–5.0V 안입니다. 3kΩ·1% 부하는 최저 출력에서도 약1.073mA로 LDO 정확도 조건을 충족합니다. [TPS7A20 p4–6](https://www.ti.com/lit/ds/symlink/tps7a20.pdf)

U3 전력 입력은 raw VBUS에 유지되며 3.3V enable은 VIH=1.1V 조건을 만족합니다. [TPS2553-Q1 p4](https://www.ti.com/lit/ds/symlink/tps2553-q1.pdf)

기동 순환 의존성과 기존 VDD 정격 초과 원인은 제거됐습니다. Suspend 전류·detach 과도·역급전 실측 승인을 뜻하지 않습니다.

### R1-A-004 · P2 → FIXED

위치: [normalize_exports.py:8](/mnt/f/dev/canview/tools/hardware/normalize_exports.py:8), [validate_exports.py:92](/mnt/f/dev/canview/tools/hardware/validate_exports.py:92), `.gitattributes`.

- 지정된 생성 text를 hash 계산 전에 LF로 정규화합니다.
- 기존 불일치 8건을 포함하여 **네 보드 전체 artifact 해시가 Git bytes와 일치**합니다.
- commit의 validator 코드를 메모리로 읽어 Windows KiCad Python에서 `--git-revision 59b241…` 분기를 실행했습니다: **mismatch 0, exit0**.
- 최종 `57ac03a`의 artifact bytes도 별도로 대조해 **mismatch 0**입니다.

## 전체 delta 회귀·추가 correction

초기→수정본 91개 파일의 변경을 검토했습니다. 생성물은 ref 재번호와 실제 연결 변경을 구분해 비교했으며, 의미 변경은 요청된 전원 수정과 Controller NC 이름 교정에 한정됐습니다.

| 독립 재검증 | 결과 |
|---|---|
| BOM/연결 모델/XML/사용 footprint named pad | **343개 / 1,235개 일치** |
| XML ↔ `.net` 전체 pin/net | 네 보드 일치 |
| Schematic S-expression | 45개 파싱 |
| Hardware 회귀시험 | **5/5 PASS**, PHY-off 포함32 Boolean 조합 |
| Protocol host 시험 | **16/16 PASS**, encode 및 hostile raw-wire decode 포함 |
| Margin 계산 | 저장 JSON과 일치 |
| 제조사 PDF | **56개 / 1,745쪽 hash·크기·parse PASS** |
| 최종 객체 기반 문서 링크 검사 | **113문서 / 690 local target, 오류0** |
| ERC | 저장된 네 보드 0건 확인; native ERC 재실행하지 않음 |

B 수정의 rate-mask/DR 교차 제약, UNAUTHORIZED 비공개 필드 zero, strict version 형식과 NC 헤더 수정에서도 새 회귀를 발견하지 못했습니다. 실제 session/cache/allocator 구현 검증으로 확대하지는 않았습니다.

`59b2412 → 57ac03a`는 [참조색인:68](/mnt/f/dev/canview/hardware/references/README.md:68)의 **한 줄 변경뿐**입니다. 저장 PDF를 직접 확인해 **17–19쪽=DBV, 20–22쪽=DCK**임을 확인했습니다. 회로·footprint·시험 코드는 변경되지 않았습니다.

## 남아 있는 승인 경계

전체 exporter/ERC 재생성, target build, 아날로그 시뮬레이션, 실물 fault injection은 수행하지 않았습니다. MAX land90-0409 원본 overlay, 최신/미확보 PDF, 구매 passive 확정, PCB·열·SOA·regulator loop, USB 과도, MAX3055 전압 여유, mic SI는 기존 physical release gate로 남습니다.

**최종 `57ac03a`의 설계 검토 통합: PASS. 원 P1 세 건과 P2 한 건의 closure를 승인하며, 제작·G1·HIL·차량 TX는 승인하지 않습니다.**
