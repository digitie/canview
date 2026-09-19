# resume.md

## 현재 상태

2026-09-19, [T-007 OTA-01](tasks/T-007-ota-container.md)의 소프트웨어 수용 근거를
충족했다. 아직 PR merge 전이므로 상태는 IN_PROGRESS다. 사용자 요청에 따라
merge를 확인한 뒤 다음 미완료 firmware task를 계속한다.

- worktree: `F:/dev/canview-wt/t007-ota-container`
- branch: `codex/t007-ota-packager`
- Draft PR: [#36](https://github.com/digitie/canview/pull/36)
- 검토·검증 candidate: `77b84cf07cd868f0f0b858a00a0ae8c9b3eb9ef4`
- PR base: `6cf1b8e57840a27b83c407d1325a92f869cf2f5d` ([PR35](https://github.com/digitie/canview/pull/35))

[최종 전체 감사와 실행 근거](reviews/adversarial/2026-09-19-T-007-final-acceptance.md)에
두 reviewer 원문, AC별 대응, CI/artifact 감사와 후속 owner를 모았다.
양쪽 verdict는 CONDITIONAL이며 추가 소프트웨어 결함 없이 최신 CI/산출물 확인만
남겼다. 해당77b CI35426085834는6/6 성공이고, target21개 bytes/SHA256·source7개·
target logs28개 warning/error0을 직접 확인했다. 원 verdict를 PASS로 덮어쓰지 않는다.

Windows Debug/Release 각150/150, 최신 Linux ASan/UBSan139/139,
stage 실제 C mutant5개, strict docs71API와 일반 native CLI가 통과했다.
상세 명령·이전 실패·coverage 귀속은 위 report와 [journal](journal.md)에 보존한다.

## 다음 한 작업

Review closure 기록을 commit/push하고 그 HEAD의 CI·target artifact 귀속을 다시
확인한다. 성공하면 PR36 ready/merge, origin/main과 merge commit 확인, T-007 DONE
archive 갱신 순서로 진행한다. 다음 task 선정은 merge 확인 후 [backlog](tasks.md)와
상세 task의 의존성으로 결정한다. 앞선 구현을 다시 작성하지 않는다.

현재 C 계약은 작은 서명 manifest와 순차 image만 사용한다. 새로운 범용 package
framework는 만들지 않는다. [단순한 구현 우선](../AGENTS.md#2-작업-원칙)을 유지한다.
[OTA 모듈](../shared/ota/README.md), [T-007 소유권 표](tasks/T-007-ota-container.md#계약과-설치-구현의-소유권)를
기준으로 실제 ESP writer는 T-204, STM boot/Flash는 T-107, 영속 정책은 T-205에서 구현한다.

## 안전 경계와 미실행 gate

- 실제 board flash, HIL, 전원/rail/reset/brownout, 장치 암호 실행, 총 stack/heap/WCET,
  차량 evidence와 provisioning은 NOT_RUN이다.
- 실제 Flash 보호 map/read-back 및 PREPARED/selector enforcement는 후속 owner gate다.
  T-007 stage의 정상 모형·실패 순서 시험을 실제 Flash 실행으로 표시하지 않는다.
- 차량 CAN TX는 NO-GO, Diagnostic Bridge는 read-only다.
- T-007 소프트웨어 수용은 전체 제품 OTA 설치나 차량 release 승인이 아니다.
- PR33 Reviewer A 면제는 그 PR 한 건뿐이다. [이슈34](https://github.com/digitie/canview/issues/34)는
  OPEN이며 다른 PR의 리뷰 면제로 사용하지 않는다.

## 환경·보존

Windows PowerShell과 [고정 도구](development/windows.md)를 사용한다.
기존 t104 worktree의 foundation 환경을 재사용하되 현재 source를 빌드한다.
WSL은 보조 sanitizer/coverage다. 기본 `F:/dev/canview` 및 다른 worktree의 사용자
변경·SDK·evidence를 보존하고 강제 정리하지 않는다.

Branch/검증/merge 절차는 [workflow](runbooks/agent-workflow.md),
과거 구현·실패·merge 이력은 [journal](journal.md)과 [review archive](reviews/README.md)에 있다.
