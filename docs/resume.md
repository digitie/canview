# resume.md

## 현재 작업

2026-09-19, [T-107 OTA-03](tasks/T-107-stm32-mcuboot.md)을 source-only로 시작했다.
사용자의 G1 이전 firmware 구현 승인에 따라 C source·host·실제 Arm target을
진행하며 외부 하드웨어 수용 기준은 열어 둔다.

- branch: `codex/t107-stm32-mcuboot`
- Draft PR: [37](https://github.com/digitie/canview/pull/37)
- worktree: `F:/dev/canview-wt/t007-ota-container` (기존 SDK·evidence 보존 목적 재사용)
- 시작 commit: `c60641f218ca5a9966781cc1e8d417df26bb2712`
- T-107은 IN_PROGRESS, 아직 bootloader/Flash 구현·검증 완료가 아니다.

T-007 [PR36](https://github.com/digitie/canview/pull/36)은 위 commit으로 merge돼
origin/main과 검토 HEAD ancestry를 확인했다. 최종 CI35427174458 6/6,
target21개 bytes/hash·source7개·target logs28개 warning/error0이다.
[완료 archive](tasks-done.md), [최종 감사](reviews/adversarial/2026-09-19-T-007-final-acceptance.md),
[merge 증거](https://github.com/digitie/canview/pull/36#issuecomment-5740075821)를 따른다.
T-007을 다시 구현하지 않는다.

## 다음 한 작업

고정 map의 C99 BSP 조회·범위 검사와 전체 byte offset 거절 시험을 추가했다.
Host Debug/Release151/151, 새 C 파일 ASan/UBSan·분기 coverage100%, 기존 STM32
Debug/Release target 빌드가 통과했다. 자세한 범위·제한은 [STM32 README](../firmware/communicator/stm32/README.md#t-107-flash-배치-구현-중)를 따른다.
다음은 이 BSP를 MCUboot2.4.0 Flash API와 별도 boot/app linker에 연결하는 작업이다.
Profile/중복 write/ECC·swap/revert는 아직 구현·검증하지 않았다. 기존 전체 Flash
scaffold와 새 library의 Arm 컴파일을 OTA loader 완료로 표시하지 않는다.

[T-102](tasks/T-102-stm32-platform.md) source/review PR29의 merge50410ba는 확인했다.
T-102 전체 수용은 물리 측정과 T-107 map 연결 등이 남아 IN_PROGRESS다.
이 상태를 DONE으로 바꿔 선행을 충족한 척하지 않는다.
T-107의 상세 수용 기준도 유지하며 장비 없이 확인할 수 없는 항목은 NOT_RUN이다.

## 유지할 경계

- 작은 구현과 기존 SDK/MCUboot 기능을 먼저 사용한다. 새 범용 framework를 만들지 않는다.
- 실제 option-byte/eFuse·production key·Flash/provisioning을 변경하지 않는다.
- Physical/HIL·전원/reset/rail·실제 ECC/erase stall·총 자원 실측·차량 evidence는 NOT_RUN.
- 차량 TX는 NO-GO, Bridge는 read-only. Bootloader는 CAN/ARM을 활성화하지 않는다.
- T-205의 영속 정책/journal이 MCUboot trailer를 대체하지 않는다.
- PR33 Reviewer A 면제는 그 PR 한 건뿐이며 [이슈34](https://github.com/digitie/canview/issues/34)는 OPEN이다.

## 환경·기록

Windows PowerShell과 [고정 도구](development/windows.md)를 사용한다.
MCUboot pin은 tools/toolchain-versions.json의 v2.4.0/6d3b3d2이며 기존 설치
`C:/cv/mcuboot-2.4.0`을 검증해 재사용한다. 사용자 checkout·다른 worktree·SDK·
이전 build/evidence는 보존한다. WSL은 보조 host sanitizer/coverage다.

절차는 [workflow](runbooks/agent-workflow.md), 실제 명령·실패·merge 이력은
[journal](journal.md)과 [review archive](reviews/README.md)에 둔다.
