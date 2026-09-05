# CANView 작업 일지

## 2026-09-05 (codex, R1 독립 리뷰 수정)

동일 immutable `06bb51c72180f9c040db3ccf0b223a823c570409`를 전문 reviewer2명이 object-only 방식으로 검토했다. 두 원문을 상호 공개 전에 보존했다. P1 세 건(게이트 DC VGS 정격, PHY rail 소실 시 FT fail-open, USB CC 제어기 VDD 범위)을 BUK7Y12·active-high AHCT126·USB전용3.3V로 수정했다. B의 capability/UNAUTHORIZED payload/헤더 이름과 양쪽의 CRLF 해시 finding도 반영했다.

- Windows KiCad10.0.6 전체 export: ERC0/waiver0,343개 BOM item/1,235 named pad 정합성 PASS.
- Windows Python: navigation16시험, hardware net/Boolean5시험 PASS. 후자는 HIL/아날로그 과도 시뮬레이션이 아니다.
- PDF56개/1,745쪽/95,230,736byte의 크기·SHA·parse 오류0. 기존 원문/land 미확보 gate는 유지한다.
- 생성 text canonical LF와 immutable Git blob hash 검사 경로를 추가했다. 신규 USB회로/FET/FT enable은 기존 footprint 또는 소형 LDO만 사용하며 보드 소형화 우선을 유지한다.
- 수정 기준선의 원 reviewer 재검토와 최종 disposition은 별도 review report에 기록한다. 이 로그만으로 P1 closure나 제작 허용을 선언하지 않는다.

## 2026-09-05 (codex, R1 상세 회로·센서 확장)

**범위**: 사용자가 명확히 선택한 가격보다 소형화 우선 기준으로 네 보드 회로·local footprint·BOM·FW 핀맵과 센서 protocol을 작성했다. 이전 작업자의 KiCad version/Windows 문서·S3 footprint 수정을 유지하고 합쳤다. `embedded-architecture`와 `embedded-documentation` 원칙에 따라 센서 owner, wire 정본, 실측 gate와 후속 task를 분리했다.

**설계**: 자동차/USB-C 전원 mux, automotive-only PHY/GPS, MAX20040 adjustable5.0875V/외부 bootstrap diode, TCAN1046 DYY pin 수정, reset/rail/WD latch 차단, MAX3055 자체 rail을 따르는 TX/EN gate,24개 테스트 패드. MTi7 DR+BMP384 AUX SPI, cased GPS UART/PPS, LVDS 원격 T5848 mic, 기존 Waveshare RTC 재사용. 전원·센서 경계는 ADR-006이다.

**검증**:

- Windows KiCad10.0.6 `tools/hardware/export-review.ps1`: 네 보드 ERC0개, waiver0개, 총333개 BOM item(테스트 패드·DNP 포함)/1,209 named pad의 net·pad·BOM 정합성 PASS. source에서 XML/sexpr/PDF를 재생성했다.
- `tools/hardware/check_margins.py`:5V/supervisor/OV/WD 정적 계산 통과. FB 누설 가정과 빠른 collapse 지연·ripple·SOA 미포함을 명시했다.
- Windows Python `-m unittest discover -s tools/protocol -p test_navigation_codec.py -v`:12개 host 시험 통과. 실제 session allocator/cache·role 검사·RF/STM 통합 구현은 T-100b 후속이다.
- 제조사 PDF54개,1,709쪽,93,272,603byte의 전체 페이지 parse·SHA-256·크기 검사 오류0. 미확보2건과 최신판/land 미확보는 별도 기록했다.
- 회로 PDF의 diode 극성과 global label 방향을 눈으로 확인하고 preview를 실제 export에서 다시 만들었다. native exporter 순서 race는 `Start-Process -Wait`와 독립 netlist 검사로 수정했다.
- CodeGraph의 현재 연결은 다른 프로젝트이므로 사용하지 않았다. `rg`, source/schema 참조, local link 검사와 독립 export 검사로 영향 범위를 확인했다. WSL은 검색·patch·다운로드 보조, 생성/검증/Git은 Windows executable이다.

**남은 조건**: MAX20040 land90-0409 원본 overlay, 구판/미확보 원문, 구매 R/C·harness·PCB/열/loop/SOA·HIL은 미완료다. T-100은 IN_PROGRESS, T-100b는 BLOCKED를 유지한다. 실제 보드·오실로스코프·차량이 없는 상태를 시험 완료로 표시하지 않는다. 전문2인 immutable 적대적 리뷰 결과는 별도 archive에 기록한다.

## 2026-09-05 (codex)

**작업**: 최신 Windows EDA export와 Communicator 회로 산출물 정합성 보정

**변경**:

- KiCad `10.0.6`을 현재 안정 EDA baseline으로 manifest와 문서에 고정하고, `export-review.ps1`로 생성기·XML netlist·ERC JSON·PDF export를 한 번에 재현하게 했다.
- STM32 UFQFPN48 7번 패드 표기를 공식 `PG10-NRST` 이중 기능으로 맞췄다.
- ESP32-S3-MINI-1-N4R2에 S2 footprint를 사용하던 참조를 제거하고, Espressif 공식 S3 land pattern 기반 전용 footprint로 교체했다.
- `09_can_ft`와 `14_can_connectors`를 생성기 호출 순서와 동일하게 분리해 11개 hierarchical sheet, BOM, pinmap, connectivity, schematic, netlist가 같은 입력에서 나오도록 갱신했다.

**검증**:

- Windows KiCad bundled Python과 KiCad CLI `10.0.6`으로 생성·netlist·ERC·PDF export를 실제 실행했다.
- ERC는 23개 violation을 보고했다. 기존 power/isolated-label 및 PCB·SI·transient 미검증 gate가 남아 있어 제작·차량 연결 승인은 아니다.
- ESP-IDF/STM32 target compile은 현재 셸에 해당 host tool이 없어 미실행으로 유지했다.

## 2026-09-05 (codex)

**작업**: 최신 Windows 임베디드 개발환경과 target build bootstrap 구성

**결정**:

- ESP-IDF `v6.0.3`, STM32CubeG4 `v1.6.3`, CMake `4.4.3`, Ninja `1.13.2`, Arm GNU Toolchain `15.3.Rel1`을 manifest에 고정했다.
- ESP-IDF `v6.0.3` peeled commit `76f5dedd9950a3012fee8fb7d5586df21fc67802`, STM32CubeG4 `v1.6.3` peeled commit `d11b194a9f05d1b143d154771f3dbc282c8052a5`을 기록했다.
- 버전 선택과 upgrade 규칙을 [ADR-005](adr/005-latest-windows-embedded-toolchain.md)에 기록했다.

**변경**:

- `tools/environment/setup-windows.ps1`가 Windows host tool version, SDK checkout commit, ESP-IDF export와 핵심 SDK 파일을 검증한다.
- `firmware/controller/`와 `firmware/communicator/esp32/`에 독립 ESP-IDF project, `main`, 향후 public `canview_protocol` component, `sdkconfig.defaults`, partition table를 추가했다. T-002 전에는 incomplete v1.2 header를 application dependency로 연결하지 않는다.
- STM32 CMake minimum/preset/toolchain에서 CMake 4.4, Ninja, Arm GCC 15.3.x를 검증하고 memory usage report를 출력하도록 했다.
- `canview_can`의 private protocol include path를 public `REQUIRES canview_protocol` 경계로 바꿨다.

**검증**:

- `git diff --check` 통과.
- 현재 실행 셸에는 CMake, Ninja, Arm GNU compiler, ESP-IDF와 STM32CubeG4 checkout이 없어 실제 target configure/build는 미실행이다. 따라서 T-200/T-300/T-102 acceptance는 완료로 표시하지 않는다.
- Windows에서 실행할 전체 준비 명령은 [tools README](../tools/README.md)와 [장치별 toolchain](development/toolchains.md)에 기록했다.

## 2026-09-05 (codex)

**작업**: 문서 정보구조 재정립과 실행별 독립 적대적 리뷰 gate 도입

**변경**:

- `AGENTS.md`를 공통 정책·안전 경계·단계별 읽기 규칙의 짧은 정본으로 정리하고, 사용자가 보강한 Ruthless Review 원칙을 유지했다.
- `docs/README.md`를 중앙 router로 추가하고 상세 설계를 architecture·hardware·development·vehicle·UI 하위 디렉터리로 이동했다.
- `SKILL.md`는 정책 사본이 아닌 작업별 문서 router로 축소했다.
- 적대적 리뷰는 매 실행마다 새 report를 만들고, 서로 다른 전문 영역의 reviewer subagent 2명이 같은 immutable 기준선을 독립 검토하도록 workflow와 archive를 정의했다.
- 기존 기준선 리뷰는 `docs/reviews/adversarial/2026-09-04-baseline-design.md`로 이관하고 ADR-004에서 새 정본 관계를 기록했다.

**환경**: branch·status·commit은 Windows Git을 정본으로 사용했다. 대량 상대 링크 경로 수정에는 Windows Python을 찾지 못해 WSL `python3`를 일회성 보조 도구로 사용했으며, 이후 Windows Git diff와 별도 link 검증으로 결과를 확인했다.

**적대적 리뷰**: 서로 다른 전문 영역의 reviewer subagent 2명이 immutable commit `b6f523f`를 독립 검토해 6개 P1, 3개 P2, 2개 P3와 추가 관찰 1개를 보고했다. 수정 commit `ab613c8`에서 두 reviewer가 모든 항목의 해소와 신규 P0/P1 회귀 없음에 동의했다. 두 `CONDITIONAL` verdict의 유일한 조건인 post-fix 결과·disposition 기록은 [통합 report](reviews/adversarial/2026-09-05-document-information-architecture.md)와 별도 evidence로 종결했다.

**검증**: 1차에는 Markdown local link 459개와 fragment 8개를 확인했다. closure 포함 Markdown 89개, local link 476개, fragment 11개에서 오류 0개, 상세 task 파일·요약 link 34/34, 이동 전 경로 잔존 0개, Windows Node `prototype.js --check`, Windows Git `diff --check`를 통과했다. staged 보안 감사 결과는 PR에 남긴다.

## 2026-09-05 (codex)

**작업**: Windows 개발환경과 일회성 worktree 정책 반영 및 embedded-skills 설치

**변경 파일**:

- AGENTS.md, SKILL.md
- docs/development/windows.md
- docs/runbooks/agent-workflow.md, docs/runbooks/agent-failure-patterns.md
- docs/adr/003-windows-development-and-ephemeral-worktrees.md
- docs/adr/README.md, docs/decisions.md, CHANGELOG.md, docs/resume.md

**외부 설치**: `rovinax/embedded-skills` `master` (`022ce31b469b1a1d0c1261c2c8d0f3e07b2c0bbc`)에서 `embedded-architecture`, `embedded-cstyle`, `embedded-documentation`, `embedded-driver-design`, `embedded-isr-design`, `embedded-rtos-design`을 Codex skills 디렉터리에 설치했다.

**결정**: Windows PowerShell과 Windows native 도구를 정본 개발환경으로 삼고, worktree는 병렬·격리·독립 리뷰가 필요할 때만 생성하며 merge 또는 abandon 후 제거한다. WSL/Linux는 보조 환경으로만 취급한다.

**검증**: GitHub 설치 스크립트가 6개 스킬 설치를 완료했다. 저장소 문서의 경로·worktree 표현과 ADR 색인을 갱신했다. Markdown local link 78개, task 상세/요약 34개, host C automation, UI JavaScript syntax 검증을 통과했다. 현재 셸에는 CMake/Ninja가 없어 해당 build gate는 미실행이다.

## 2026-09-05 (codex)

**작업**: kor-travel-geo 문서 운영 구조를 canview에 적용 (문서 구조 task)

**변경 파일**:

- AGENTS.md, SKILL.md, CHANGELOG.md와 .gitignore
- README.md
- docs/architecture/README.md
- docs/architecture/system.md, docs/architecture/implementation-readiness.md, docs/reviews/adversarial/2026-09-04-baseline-design.md
- docs/development/windows.md, docs/runbooks/documentation-maintenance.md, docs/resume.md, docs/journal.md
- docs/adr/, docs/runbooks/
- docs/tasks.md, docs/tasks-rule.md, docs/tasks-done.md, docs/tasks/README.md

**결정**: 열린 task 요약은 docs/tasks.md에 두고, 상세 task는 docs/tasks/ 아래에 하나씩 유지한다. 완료 task는 docs/tasks-done.md로 이동한다.

**발견**: 기존 canview에는 docs/tasks/README.md에만 task 요약이 있었고 AGENTS.md·SKILL.md·ADR·runbook·resume·journal 정본이 없었다.

**다음**: T-001 host toolchain/CI와 T-100 KiCad 회로도·BOM을 병렬 착수한다.
