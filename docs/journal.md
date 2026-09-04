# CANView 작업 일지

## 2026-09-05 (codex)

**작업**: Windows 개발환경과 일회성 worktree 정책 반영 및 embedded-skills 설치

**변경 파일**:

- AGENTS.md, SKILL.md
- docs/dev-environment.md
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
- docs/architecture/architecture.md
- docs/system-architecture.md, docs/implementation-readiness.md, docs/adversarial-design-review.md
- docs/dev-environment.md, docs/agent-guide.md, docs/resume.md, docs/journal.md
- docs/adr/, docs/runbooks/
- docs/tasks.md, docs/tasks-rule.md, docs/tasks-done.md, docs/tasks/README.md

**결정**: 열린 task 요약은 docs/tasks.md에 두고, 상세 task는 docs/tasks/ 아래에 하나씩 유지한다. 완료 task는 docs/tasks-done.md로 이동한다.

**발견**: 기존 canview에는 docs/tasks/README.md에만 task 요약이 있었고 AGENTS.md·SKILL.md·ADR·runbook·resume·journal 정본이 없었다.

**다음**: T-001 host toolchain/CI와 T-100 KiCad 회로도·BOM을 병렬 착수한다.
