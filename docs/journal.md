# CANView 작업 일지

## 2026-09-05 (codex)

**작업**: 최신 Windows 임베디드 개발환경과 target build bootstrap 구성

**결정**:

- ESP-IDF `v6.0.3`, STM32CubeG4 `v1.6.3`, CMake `4.4.3`, Ninja `1.13.2`, Arm GNU Toolchain `15.3.Rel1`을 manifest에 고정했다.
- ESP-IDF `v6.0.3` commit `06e31f0c9ac86f713a1b10d252e7396ac8a1552a`, STM32CubeG4 `v1.6.3` commit `64d78dd7042d277a31878178284e17882af51690`을 기록했다.
- 버전 선택과 upgrade 규칙을 [ADR-005](adr/005-latest-windows-embedded-toolchain.md)에 기록했다.

**변경**:

- `tools/environment/setup-windows.ps1`가 Windows host tool version, SDK checkout commit, ESP-IDF export와 핵심 SDK 파일을 검증한다.
- `firmware/controller/`와 `firmware/communicator/esp32/`에 독립 ESP-IDF project, `main`, public `canview_protocol` component, `sdkconfig.defaults`, partition table을 추가했다.
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
