# T-107 fail-stop Reviewer A 원본

## 공통 전달 manifest

````text
T107-Failstop 독립 적대적 리뷰. candidate 6b394ac7effbdc14e901b363a1594d4bf89b34fa, base e9f474b133912814aad255add3452aa0eb2fd44b. Task T107 IN_PROGRESS, PR37 Draft. 전체 Task merge 승인 아닌14file delta: git diff --find-renames base candidate 전체와 관련 원본을 line-level 확인. bootloader/fail_stop.c/h newlibGNU ABI wrappers→기존 FIH_PANIC, Releaseassert조건유지, hostlibc불변. 명시적trust일때기존 비배포 canview-boot-ram-link-test 에 boot_go/crypto/identity 강제참조전체link, two wrapper 분기 검사/heapstdio금지symbol, missingwrap 실제 link negative. finalentry/policy/recovery는 미완료이며 비배포 시험인 점을 scope문서가밝힘. physical/HILNOT_RUN, production/vehicleTXNO-GO. 새정책이나권한의완료를추정하지말 것.
AGENTS/docsREADME/resume/T107, 필요한workflow/skills읽기. candidate HEAD와clean 시작/종료확인. 자신의detachedtree에만 ignored build evidence, trackedsourceedit/commit 금지. 다른reviewerfinding보지말것. 검토 전후실제읽은파일/명령, 전문영역, executionID/UTC시작종료,candidate/base,격리, P0P1P2P3정확한file:line/failure/impact/recommendation, 미검토범위, physicalNOT_RUN, BLOCK/CONDITIONAL/PASS 한국어 raw report 반환. 불가능한기능을구현됐다고주장하는지와 실제scope구별, 범위를넓혀기존명시적미구현T107자체를새delta결함으로분류하지말 것.
환경: CMake F:/dev/canview-wt/t104-stm32-uart-control/.tools/cmake-4.4.3/bin/cmake-4.4.3-windows-x86_64/bin ; Clang .../.tools/llvm-23.1.0/clang+llvm-23.1.0-x86_64-pc-windows-msvc/bin; Ninja .../.tools/ninja-1.13.2/bin/ninja.exe; hostbuild및CTest전 반드시vswhere→Launch-VsDevShell -Arch amd64 -HostArch amd64 -SkipAutomaticLocation. SDK C:/cv/mcuboot-2.4.0 pin6d3b3d2 clean, STM C:/cv/STM32CubeG4-1.6.3, Arm C:/Users/digit/AppData/Local/CANView/toolchains/arm-gnu-toolchain-15.3.rel1. Root configure -DCANVIEW_MCUBOOT_ROOT=C:/cv/mcuboot-2.4.0. Arm full link explicit publicDER=F:/dev/canview-wt/t007-ota-container/build/t107-trust-test-public.der (testonlyPUBLIC), epoch0, manifestkeyID4294967295, STMABI2. powershell -D전체인자quote. 독립actualArm Debug/Release와hostfocused검증권장, 전체161는 coordinator진행중. 상세logs main ignored build/t107-failstop-* (source는자기기준선).
로컬관찰: ArmD/R fullclosure ELF/BIN37184/29116B,ram12264/12040B, warning0,4test+missingwrap2negativePASS; host새3/3,GCCstrictC99,ClangASANUBSAN; failstopCfunction2/2region4/4line10/10,branch0. wholehost161회귀진행중.
````

## 역할과 격리

Reviewer A — embedded runtime, ABI/noreturn, FIH preservation, watchdog/error path, memory safety, side effects.
Execution ID: `T107-FAILSTOP-A-6b394ac-20260919`.
Worktree: `F:/dev/canview-wt/review-t107-failstop-a`.

## 원본 결과

````markdown
판정: **CONDITIONAL — P2 1건 OPEN**. 현재 wrapper 구현의 P0/P1 결함은 발견하지 않았으나, 새 Arm 검사기가 noreturn 위반을 통과시키는 검증 결함을 재현했습니다. 이번 14파일 delta에 대한 판정이며 T107 전체 완료·PR37 merge 승인은 아닙니다.

- executionID: `T107-FAILSTOP-A-6b394ac-20260919`
- 전문영역: embedded runtime, GNU/newlib ABI·noreturn, FIH 보존, watchdog/error path, 메모리 안전·부작용
- UTC 시작: `2026-09-19T13:17:43.5476158Z`
- UTC 종료: `2026-09-19T13:24:05.1072448Z`
- candidate: `6b394ac7effbdc14e901b363a1594d4bf89b34fa`
- base: `e9f474b133912814aad255add3452aa0eb2fd44b`
- 격리: `F:/dev/canview-wt/review-t107-failstop-a`, detached HEAD
- 시작·종료 모두 candidate 일치, `git status --porcelain=v1` 출력 없음. 종료 시 tracked/staged diff도 없음.
- 생성물·재현 자료는 자신의 ignored `build/`에만 저장했습니다. tracked source 수정·commit·flash는 하지 않았습니다. 이번 리뷰의 다른 reviewer 보고서는 열지 않았습니다.

**A-FAILSTOP-01 / P2 / OPEN — 복귀 가능한 wrapper를 실제 Arm 검사기가 허용**

위치: [tools/ota/check_stm32_boot_ram.py:118](/F:/dev/canview-wt/review-t107-failstop-a/tools/ota/check_stm32_boot_ram.py:118), 관련 시험 [tests/ota/test_stm32_boot_ram.py:34](/F:/dev/canview-wt/review-t107-failstop-a/tests/ota/test_stm32_boot_ram.py:34).

검사기는 조건부 `beq`까지 분기 정규식에 포함하지만, 추출한 mnemonic의 무조건 실행 여부를 검사하지 않습니다. 또한 `mov pc, lr` 같은 복귀 명령은 분기 개수에 포함하지 않습니다. 다음 실제 Arm assembler 입력을 허용했습니다.

```asm
__wrap___assert_func:
    cmp r0, #0
    beq.w fih_panic_loop
    mov pc, lr
```

`r0 != 0`이면 panic 없이 호출자로 복귀하여 noreturn 계약을 위반합니다. 작은 재현 ELF뿐 아니라 candidate Release의 기존 startup·boot_go·crypto·identity·Flash object/archive를 그대로 사용하고 wrapper만 이 음성 입력으로 교체한 전체 ELF에서도 `inspect(..., full_boot=True)`가 PASS를 반환했습니다.

영향은 새 검증기의 false PASS입니다. 현재 `fail_stop.c`가 위와 같이 동작한다는 뜻은 아닙니다. 다만 README가 설명하는 두 wrapper의 panic 경로 검사로는 이 noreturn 회귀를 검출하지 못합니다.

권고: 고정 toolchain에서 허용하는 wrapper 명령 형태를 제한하여 무조건적인 panic 진입과 복귀·우회 경로 부재를 검사하고, 조건부 panic 및 `mov pc,lr`/`pop {...,pc}` 등을 실제 Arm negative에 추가하십시오.

재현 명령: `python -X utf8 -B build/review-oracle.py`

근거: [재현 스크립트](/F:/dev/canview-wt/review-t107-failstop-a/build/review-oracle.py), [실제 전체 ELF false PASS 로그](/F:/dev/canview-wt/review-t107-failstop-a/build/review-oracle-full.log).

심각도 집계: P0 0건 / P1 0건 / P2 1건 / P3 0건.

검토한 현재 구현에서는 다음을 확인했습니다.

- `__assert_func(const char *, int, const char *, const char *)`와 `abort(void)`가 설치된 newlib 선언과 일치합니다. `int line`은 ABI 요구입니다.
- 두 wrapper 선언의 noreturn 속성, 인자 미역참조, 기존 `FIH_PANIC` 호출을 확인했습니다. Debug/Release ELF 모두 실제 `bl fih_panic_loop`로 연결됩니다.
- FIH MEDIUM과 기존 panic 구현을 유지합니다. Release bootutil 명령에서 `-DNDEBUG` 뒤 `-UNDEBUG`가 적용됩니다.
- wrapper에 heap·출력·watchdog feed·Flash 변경을 추가하지 않았습니다.
- host MCUboot 모형 링크에는 fail-stop library와 wrap 옵션이 없으며, host FIH object의 `__imp_abort` 참조가 유지됩니다.
- 명시적 trust가 있을 때만 전체 closure를 링크합니다. 실제 ELF의 `boot_go`, `uECC_verify`, `bootutil_keys`, `canview_boot_identity_read`를 확인했습니다.
- 비배포 시험의 main은 boot_go를 실행하지 않습니다. 문서는 최종 entry·정책 승인·handoff 연결·physical 수용이 미완료임을 명시합니다. 이를 새 기능이나 권한의 완료로 해석하지 않았습니다.

독립 실행 결과는 다음과 같습니다.

| 검증 | 결과 |
|---|---|
| 실제 Arm Debug 전체 closure | PASS, BIN 37,056B / RAM 12,264B |
| 실제 Arm Release 전체 closure | PASS, BIN 28,984B / RAM 12,040B |
| Arm boot 검사 | 각 4/4 PASS, 각 missing-wrap 2개 실제 link negative 포함 |
| Arm Flash SRAM 검사 | 각 4/4 PASS |
| Windows Clang host 집중 시험 | Debug 3/3, Release 3/3 PASS |
| trust 없는 Arm Release | 기존 SRAM 검사 통과, full-boot 시험 1개 명시적 skip |
| T103 합성 fixture | 13/13 PASS, source digest 일치 |
| compiler/linker/CMake warning·error | 0 |
| 추가 적대적 Arm 입력 | noreturn 위반 false PASS 재현 |

Host configure/build/CTest 전에 `vswhere`로 C++ 설치를 찾고 `Launch-VsDevShell -Arch amd64 -HostArch amd64 -SkipAutomaticLocation`을 실행했습니다. 실제 명령·인자는 [실행 스크립트](/F:/dev/canview-wt/review-t107-failstop-a/build/review-run.ps1)에 있습니다. [Arm 로그](/F:/dev/canview-wt/review-t107-failstop-a/build/review-Arm.log), [host 로그](/F:/dev/canview-wt/review-t107-failstop-a/build/review-Host.log), [trust 미설정 로그](/F:/dev/canview-wt/review-t107-failstop-a/build/review-no-trust.log)를 보존했습니다.

실제 읽은 변경 범위는 아래 14파일의 `git diff --find-renames base candidate` 전체이며, 관련 candidate 원본도 줄 단위로 대조했습니다.

```text
.github/workflows/foundation.yml
CMakeLists.txt
docs/journal.md                         이번 추가 절
docs/resume.md
firmware/communicator/stm32/CMakeLists.txt
firmware/communicator/stm32/bootloader/CMakeLists.txt
firmware/communicator/stm32/bootloader/README.md
firmware/communicator/stm32/bootloader/fail_stop.c
firmware/communicator/stm32/bootloader/fail_stop.h
tests/hil/fixtures/t103-capture-only.jsonl
tests/ota/test_stm32_boot_fail_stop.c
tests/ota/test_stm32_boot_ram.py
tests/test_t103_capture_helpers.py
tools/ota/check_stm32_boot_ram.py
```

추가로 읽은 파일·부분은 다음과 같습니다.

- `AGENTS.md`, `docs/README.md`, `docs/tasks/T-107-stm32-mcuboot.md`, workflow·Windows 개발환경·review archive 안내, architecture README와 OTA §5.
- `embedded-cstyle/SKILL.md`, `embedded-architecture/SKILL.md` 전체.
- root/STM32 CMake presets, Arm toolchain, `CanviewWarnings.cmake`.
- bootloader의 `mcuboot_compat.cmake`, `trust.cmake`, `mcuboot_config.h`, `asn1_memory.c`.
- `stm32_boot_ram_link.c`, `mcuboot_host_fih.c`, `boot_runtime.c`, `syscalls.c`, boot linker, HIL source digest 구현.
- 고정 MCUboot FIH source와 관련 header 정의, 설치된 newlib `assert.h` 및 `stdlib.h`의 ABI 선언.
- 생성된 compile commands, host 링크 명령, 실제 Arm symbol·wrapper/FIH disassembly.

주요 읽기·검증 명령은 `Get-Content`, `rg`, 전체 `git diff --find-renames`, `git diff --check`, `cmake`, `ctest`, `ninja -t commands`, `arm-none-eabi-nm/objdump`, `llvm-nm`, 위 Python 시험입니다. MCUboot SDK는 전후 모두 `6d3b3d2c38ab20c242e5b9abb04d050086383eb2`, clean이었습니다.

미검토·미실행 범위: 전체 host 161개 회귀, 별도 host GCC·ASan/UBSan·coverage 재실행, 새 CI 결과, 전체 MCUboot call-chain stack 수용, 최종 entry·T205 정책·recovery 통합, 실제 fault injection·IWDG reset·Flash/ECC·전원 시험입니다. 기존 명시적 미완료 T107 항목은 이번 delta의 신규 결함으로 분류하지 않았습니다.

**Physical/HIL: NOT_RUN. Production 및 vehicle TX: NO-GO.** P2 수정 또는 정식 disposition 전까지 이번 delta 판정은 CONDITIONAL입니다.
````
