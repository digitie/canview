# ADR-003: Windows 개발환경과 일회성 worktree

- 상태: accepted
- 날짜: 2026-09-05
- 결정자: CANView 유지보수자

## 문맥

기존 운영 문서는 Linux/WSL과 에이전트별 고정 worktree를 표준으로 가정했다. 현재 CANView의 주 개발·하드웨어 검증 환경은 Windows이며, 고정 worktree는 장기간 남은 branch·빌드 산출물·CodeGraph 상태를 만들고 작업 종료 후 정리 누락을 유발한다.

## 결정

1. 정본 개발 환경은 Windows PowerShell과 Windows native 도구로 한다. Git for Windows, Node.js/npm, Python, CMake/Ninja, ESP-IDF, GNU Arm Embedded, KiCad CLI/GUI와 CodeGraph를 표준으로 기록한다.
2. WSL/Linux와 `/mnt/...` 경로는 필요한 경우의 보조 환경으로만 취급하며 정본 checkout, branch, commit, push 경로로 사용하지 않는다.
3. worktree는 에이전트별 고정 자산으로 유지하지 않는다. 기본은 메인 checkout의 작업 branch를 사용하고, 병렬 작업·격리·독립 리뷰가 필요한 경우에만 `F:/dev/canview-wt/<agent>-<task>` 같은 임시 worktree를 생성한다.
4. 작업이 merge 또는 abandon으로 끝나면 활성 프로세스와 미커밋 변경을 확인한 뒤 임시 worktree를 제거하고 `git worktree prune`으로 metadata를 정리한다. 사용자의 활성 작업이나 미커밋 변경은 강제로 제거하지 않는다.
5. 임시 worktree의 CodeGraph 상태는 그 worktree의 수명 동안만 유지한다. 최초 사용 시 `codegraph init -i`, branch 전환·pull·merge 뒤 `codegraph sync` → `codegraph status`를 실행한다.

## 결과

- 개발·검증 명령과 경로가 Windows에서 바로 재현된다.
- WSL과 Windows 간 개행·경로·도구 버전 차이로 생기는 재현성 문제를 줄인다.
- 작업 종료 시 worktree와 로컬 인덱스를 정리해야 하므로 cleanup 확인이 PR 절차의 일부가 된다.
- Windows 도구가 없는 CI 또는 다른 호스트에서는 해당 gate를 성공으로 표시하지 않고 미실행 사유를 기록한다.

## 대안 검토

- **Linux/WSL 고정 worktree 유지**: 기존 문서와 호환되지만 현재 주 개발환경과 어긋나고, 장기 worktree 정리 누락을 구조적으로 남기므로 채택하지 않았다.
- **작업마다 항상 worktree 생성**: 격리는 강해지지만 단순 문서·소규모 수정에도 관리 비용이 생긴다. 필요한 경우에만 생성하는 정책으로 비용과 격리를 균형 잡았다.
