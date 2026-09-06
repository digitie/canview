# Reviewer A 독립 원본

- 서브에이전트: `01a06f2e-7129-7261-99bc-1bf2a1d6d490`
- 전달 요청 원문·동일 기준선: [manifest](2026-09-05-r1-hardware-navigation-manifest.md)
- 아래는 reviewer final 원문이며, 최초 상호 공개 전에 보존했다.
- 2026-09-05 보존 형식 교정: 저장 과정에서 추가된 EOF 빈 줄 한 개만 제거했다. 아래 reviewer 원문 내용·판정은 바꾸지 않았다.

## 판정: BLOCK — P1 3건, P2 1건

독립 원문을 아래와 같이 확정합니다. 다른 reviewer 결과는 읽지 않았습니다.

| 감사 항목 | 관찰 결과 |
|---|---|
| 실행 ID | `CANVIEW-R1-A-20260905T034626Z` |
| 시작 / 종료 UTC | `2026-09-05 03:46:26` / `03:59:16` |
| 실제 관찰 candidate | `06bb51c72180f9c040db3ccf0b223a823c570409` |
| 실제 parent | `1ed2576841075a1f98fb10e979736941f3deed72` |
| 격리 | Windows native Git의 `rev-parse`, `cat-file`, `show`, 고정 commit 간 `diff` |
| clean | **N/A — working tree를 검토 입력으로 사용하지 않음** |
| 변경 | 파일 수정·Git mutation·새 branch 없음 |

아래 위치·페이지는 모두 해당 candidate 기준입니다. 고장 시나리오는 회로 연결과 제조사 정격에 근거한 분석이며, 실물 고장 주입을 수행했다는 뜻은 아닙니다.

## 차단 findings

### R1-A-001 · P1 — LM74800의 gate 구동전압이 FET의 DC VGS 정격을 초과

위치: [power_circuits.py:83](/mnt/f/dev/canview/tools/hardware/power_circuits.py:83), Communicator 회로 PDF p3, Q1/Q2/U7.

| 실제 연결 | 제조사 한계 |
|---|---|
| U7 HGATE(8) → Q1 G(4), Q1 S(1–3)=CS |
| U7 DGATE(1) → Q2 G(4), Q2 S(1–3)=CS |
| Q1/Q2=`BUK9Y12-100E`; gate-source clamp 없음 | **DC VGS ±10V**, pulsed ±15V |
| LM74800, VS>5V | HGATE−OUT **11.1V typ / 14.5V max**, DGATE−A **11.5V typ / 13V max** |

정상 12V 전원에서 Q1을 계속 켜는 상태부터 DC 정격 초과 가능성이 있습니다. FET의 **100V VDS**나 **pulsed VGS ±15V**로 정당화할 수 없습니다. Gate oxide 손상과 역극성·과전압 보호 상실 가능성이 있으며, 미완료 surge 시험과 별개의 설계 결함입니다.

권고: **DC VGS ±20V급으로 적합성이 확인된 FET**를 선정하거나 clamp/구동을 재설계하십시오. 이후 SOA·열·reverse transient를 별도로 검증해야 합니다.

근거: [Nexperia PDF p2](https://assets.nexperia.com/documents/data-sheet/BUK9Y12-100E.pdf), [TI LM7480 PDF p6, p30 §10.3.2.5](https://www.ti.com/lit/ds/symlink/lm7480-q1.pdf). TI도 이 구성에 20V VGS 정격을 권고합니다.

### R1-A-002 · P1 — PHY3V3 소실 시 FT CAN의 TX/EN gate가 열림

위치: [core_circuits.py:111](/mnt/f/dev/canview/tools/hardware/core_circuits.py:111), 같은 파일 114·120·138·149행, Communicator PDF p18.

| 부품/핀 | 실제 net |
|---|---|
| U28 VCC(5), U29 VCC(5) | `AUTO5V` |
| U28 `/OE`(1) | `TX_OE_N`, R65 10k로 **PHY3V3**에 pull-up |
| U29 `/OE`(1) | `FT_RX_OE_N`, R66 10k로 **PHY3V3**에 pull-up |
| `/OE` 구동 U34/U27 VCC | **PHY3V3** |
| MAX3055 VCC(10), STB(5) | `AUTO5V` |
| MAX3055 EN(6), TXD(2) | U29 Y(4), U28 Y(4) |

재현 조건:

1. AUTO5V와 MCU 3V3는 정상 유지.
2. `FT_EN_REQ=HIGH`, `CAN3_TX_REQ`에 펄스 입력.
3. **PHY3V3만 0V로 강제**—LDO 출력 단락 등의 단일 rail fault.
4. R65/R66이 `/OE`를 LOW로 내리고, 무전원 U27/U34는 이를 HIGH로 유지하지 못합니다.
5. AUTO5V로 살아 있는 U28/U29가 활성화되어 MAX3055 EN/TXD를 구동합니다.

따라서 **physical ARM이 없어도**, watchdog/latch 전원이 죽은 상태에서 FT 송신 경로가 열립니다. MAX3055는 STB=1, EN=1이면 normal mode입니다. SYS_RESET이나 AUTO_GOOD 논리를 추가해도 해당 논리의 전원이 사라지는 이 경로는 막지 못합니다.

권고: FT용 `/OE`의 기본 disable을 **AUTO5V 영역에서 보장**하고, PHY 전원 소실 시 제어가 해제되는 cross-domain 회로로 변경하십시오. 기존 공유 `TX_OE_N`에 단순히 5V pull-up만 옮기는 처방은 별도 입력 정격 검토 없이 적용하면 안 됩니다.

근거: [SN74LV1T125 p4–6](https://www.ti.com/lit/ds/symlink/sn74lv1t125.pdf), [SN74LVC1G04 p6 Ioff](https://www.ti.com/lit/ds/symlink/sn74lvc1g04.pdf), [MAX3055 p16 Table 3](https://www.analog.com/media/en/technical-documentation/data-sheets/max3054-max3056.pdf).

### R1-A-003 · P1 — USB current-permission 제어기가 정상 USB 전압 범위를 감당하지 못함

위치: [power_circuits.py:35](/mnt/f/dev/canview/tools/hardware/power_circuits.py:35), Communicator와 Bridge의 U1.

- `TUSB320LAI VDD(12)=USB_VBUS` 직결입니다.
- 제조사 **권장 VDD는 2.7–5.0V**, absolute maximum은 6V입니다.
- VBUS=5.25V를 인가하면 PD 협상 없이도 즉시 권장 동작 범위를 벗어납니다.

즉시 파손을 단정하는 finding은 아닙니다. 하지만 이 IC의 OUT1이 USB limiter 허가를 결정하므로, **허용 USB 입력 조건에서 default-current 부팅 금지 기능을 제조사 보증 범위 안에서 입증할 수 없습니다.**

권고: limiter 앞 USB 영역에 적합한 소전류 3.3V 공급을 두고 U1 및 관련 pull-up/논리의 전원 영역을 정리하십시오. Mux 뒤 SYS3V3로 공급하면 USB 단독 기동의 순환 의존성이 생기므로 피해야 합니다.

근거: [TUSB320LAI PDF p4](https://www.ti.com/lit/ds/symlink/tusb320lai.pdf). 동일 VBUS 직결 질문에 TI도 [외부 LDO 사용을 권고](https://e2e.ti.com/support/interface-group/interface/f/interface-forum/936742/tusb320-maximum-recommended-vdd-is-5v-where-the-usb-voltage-may-be-up-to-5-25v-can-the-vbus-supply-the-chip)합니다.

## 비차단 finding

### R1-A-004 · P2 — artifact manifest 해시가 immutable Git bytes와 불일치

위치: [validate_exports.py:82](/mnt/f/dev/canview/tools/hardware/validate_exports.py:82), [validation.json:14](/mnt/f/dev/canview/hardware/validation.json:14).

네 보드 각각의 `.net`과 `connectivity.json`, **총 8개**가 기록된 SHA-256과 다릅니다. 모두 Git 객체의 LF를 CRLF로 변환하면 기록값과 정확히 일치합니다.

예: `communicator.net`

```text
manifest: b9a19817bcabfc7e38e7b12e97090750b1e5dc8c0bcda72e455a558460479837
Git blob: 6040868dc3cd57be1439fef89a83f76d17ef6ea71af5791c3c8206542289aaf0
```

**회로 내용 변경 증거는 아닙니다.** XML과 `.net`의 전체 pin/net 의미는 별도 비교에서 일치했습니다. 문제는 manifest가 commit bytes를 직접 식별하지 못하는 재현성입니다.

권고: 생성·해시 계산의 줄바꿈을 canonical LF로 고정하거나, 정규화 규칙과 원본/정규화 해시를 명시하십시오.

## 검토·재검증 범위

| 항목 | 실제 수행 |
|---|---|
| 현재 소스 | power/core/sensor circuits, schematic model/build/custom footprints, margin/export 검증 도구 검토 |
| 현재 생성물 | 네 보드 connectivity/BOM/XML/`.net`/사용 local footprint 대조, 모든 schematic S-expression 파싱 |
| 부품·패드 | **333개 부품 / 1,209개 named pad 정합성 PASS** |
| XML ↔ `.net` | 네 보드 전체 pin/net 의미 일치 |
| 제조사 자료 | **54 PDF / 1,709쪽 hash·bytes·parse PASS**. 전원/CAN/logic 핵심 페이지는 내용 검토 |
| Footprint | DYY0014A·DRB0008A 주요 치수와 pin1 방향 원문 이미지 대조. 전체 footprint 제조 승인 아님 |
| 계산 | commit의 margin 계산을 메모리에서 실행, 기록 결과와 일치 |
| Protocol | commit의 codec/schema/test를 메모리에 로드해 **12시험 PASS** |
| ERC | 네 보드의 저장된 0건 보고서 확인. **KiCad ERC 자체 재실행은 하지 않음** |
| 정본 | AGENTS/runbook, ADR006, T100/T100b, R1 회로·검증·pinmap 문서, navigation 계약 및 관련 delta 검토 |

전원 조합·USB-first·reset·watchdog 복귀·physical disarm·독립 rail collapse·핀 정격·artifact drift를 공격했습니다. 프로토콜의 malformed/version/budget 검사는 확인했지만 실제 replay cache·동시 allocator·target transport 구현 검증으로 확대하지 않았습니다.

잔여 사항:

- MAX20040 land `90-0409` 원본 overlay, regulator loop/열/SOA, ripple·overshoot·급격한 collapse, USB reverse-current 실측은 **기존에 명시된 미완료 제작 gate**로 인정합니다.
- CWD120pF 계산과 정상 전원 상태의 latch 재무장 구조에서 별도 차단 결함은 발견하지 않았습니다. **A-002 때문에 전체 고장안전 경로가 성립한다는 뜻은 아닙니다.**
- 모든 PDF의 모든 전기 정격, 센서·마이크·GNSS 전체 호환성, PCB/HIL/실차는 검토 완료를 주장하지 않습니다.
- 109문서/669 local target 링크 검사는 재실행하지 않았습니다.

**P1 세 건은 “제작 미승인” 표시만으로 닫을 수 없습니다. 수정된 immutable commit에서 원 reviewer 재확인 전까지 BLOCK입니다.**
