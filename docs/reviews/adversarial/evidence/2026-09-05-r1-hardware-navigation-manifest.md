# R1 하드웨어·센서 protocol 리뷰 manifest

공통 요청을 그대로 보존한다. dispatch 시작은2026-09-05 03:45 UTC다. 아래 공통 요청 뒤에 reviewer별 전문영역 문장을 추가해 각각 전달했다. 두 원본 결과는 교차 공유 전 별도 evidence에 저장한다.

```text
CANView R1 정식 독립 적대적 리뷰입니다.
기준 commit: 06bb51c72180f9c040db3ccf0b223a823c570409
base/parent: 1ed2576841075a1f98fb10e979736941f3deed72
repo: F:/dev/canview (/mnt/f/dev/canview), Windows native Git.
사용자 요청: 소형화 우선 상세회로+KiCad10 netlist, automotive/USB power, CAN fault tolerance, onboard DR/baro/external cased UART GNSS, remote I2S microphone, 정확한 pinmap/부품사양/local PDF, generic 센서 protocol/응답확인.
범위: hardware/ 전체 현재 생성 회로와 BOM/netlist/local footprints/references, tools/hardware/, tools/protocol/, protocol/schema/navigation-v1.json, docs/hardware/r1/, ADR006, navigation protocol 및 T100/T100b와 관련 정본 delta. base에 이미 부분 hardware가 들어있으므로 diff만 보지 말고 현재 대상 파일 전체를 확인할 것.
범위 밖: 기존 LVGL UI/DBC/target bootstrap 자체 구현, 실제 PCB routing/Gerber/실물 HIL/실차 시험. G1/차량TX와 제작은 미승인 상태다.
검증 근거: KiCad10.0.6 네 보드 ERC0/waiver0, 333 BOM item/1209 named pad 정합성 PASS, host protocol12시험 PASS,54PDF1709쪽 hash/parse PASS,109문서669local targets 오류0. 결과는 hardware/validation.json, hardware/margin-check.json; 재현 tools/hardware/export-review.ps1, Windows Python -m unittest discover -s tools/protocol -p test_navigation_codec.py -v.
격리: commit object-only. git cat-file/rev-parse로 hash를 확인하고 소스는 git show <hash>:<path>, diff는 git diff <base> <hash>로만 읽을 것. 이동 가능한 branch/plain worktree/다른 reviewer report를 근거로 읽지 말 것. PDF는 git show의 bytes를 PyMuPDF stream으로 열 수 있다(기존 .tools/hardware-python dependency는 tool만). 새로운 작업branch나파일 수정/Git mutation 없음.
수용/판정: AGENTS.md와 docs/runbooks/agent-workflow.md의 P0~P3/독립원문규칙. 정상경로뿐 아니라 reset/rail collapse/replay/malformed/버전왜곡/oversubscription/핀·land mismatch/검증누락을 공격. 알려진 제작 전 제한도 충분한 차단인지 실제 회로/계약결함인지 구분해 근거로 판정할 것. ERC0을 전기적 안전 증거로 대체하지 말 것.
출력: 실행ID, 시작/종료UTC, 실제관찰hash, object-only확인(clean은working tree사용안함으로명시), 읽은파일/검증/미검토범위, finding별 고유ID·심각도·정확위치·재현/영향/권고, BLOCK/CONDITIONAL/PASS verdict. 해당없어도공격한시나리오와잔여불확실성. 상대 결과를 보기 전에 독립 원문을 확정할 것. 새조사무한확장없이 약15분내 핵심결과를 제출해주세요.
```

| Reviewer | 실행 ID | 추가 전달한 전문영역 |
|---|---|---|
| A | `01a06f2e-7129-7261-99bc-1bf2a1d6d490` | Reviewer A: 전원·CAN·MCU 고장안전 전문. power/core circuits, MAX/LM/PHY/gates/reset/WD, USB hotplug/backfeed, regulator qualification와 제조사 전기정격/footprint의 치명적 오류를 우선 검토. |
| B | `01a06f2e-735a-7221-9541-de848f1984b2` | Reviewer B: sensor integration·protocol·재현성 전문. MTi/BMP/GNSS/PPS/mic pin·footprint·clock, schema/hostcodec의 malformed/version/validity/idempotency/bandwidth, source↔generated/document 정합성과 구현가능성을 우선 검토. |
