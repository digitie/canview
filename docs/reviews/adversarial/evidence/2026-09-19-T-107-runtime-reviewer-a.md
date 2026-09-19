# T-107 runtime reviewer A 실행 evidence

- Subagent ID: `01a0b990-fc33-7463-8aa5-b705b4e80786`
- 요청 전달 UTC: `2026-09-19 12:08:01 UTC`
- 최초 완료 알림 원문을 아래 그대로 보존한다. 추가 공통 시험 결과 전달은 독립 판정을 대체하지 않는다.

## 전달 요청 원문

```text
T-107 bounded boot runtime source delta review. Repository F:/dev/canview-wt/t007-ota-container. Immutable candidate c0f1b81449ee0507437927a8cc29ff98d2438d0e; base ce3fe1f5d4f0148fd632beb8f7016fcc0bae5fc9. Scope: all 16 files from git diff --name-only base candidate. Main new source firmware/communicator/stm32/platform/stm32g474/boot_runtime.c, bootloader/include/canview_boot_runtime.h, tests/ota/test_stm32_boot_runtime.c; actual Arm link-test wires existing board safe output then runtime; CMake, register_model, test_link object count6->9; API docs73; firmware synthetic fixture digest update; README/resume/journal. Out-of-scope: final boot_go/policy/handoff, production provisioning, all physical/HIL; this is explicitly a NONDEPLOYABLE link-test, not final T107/merge approval. Do not invent findings solely because declared later implementation/physical gates remain open. Do look for unsafe present contracts/bugs/evidence claims. Canonical AGENTS.md then docs/README.md/docs/resume.md/T-107 task; related architecture/ota.md §5/7, agent-workflow. Read applicable embedded skills before review. Keep changes minimal; no changes to repository, SDK, user files; no commit or push. No external hardware access. Review every changed hunk independently without seeing other review. Verify candidate hash and clean start/end. Actual toolchain available Arm15.3 C:/Users/digit/AppData/Local/CANView/toolchains/arm-gnu-toolchain-15.3.rel1, SDK C:/cv/STM32CubeG4-1.6.3 (pinned clean d11b194a9f05d1b143d154771f3dbc282c8052a5), WSL Ubuntu-26.04 clang21/gcc15.2, Windows cmake F:/dev/canview-wt/t104-stm32-uart-control/.tools/cmake-4.4.3/bin/cmake-4.4.3-windows-x86_64/bin. You may run focused host tests in detached tree build/ or /tmp with unique A/B names. Main observed Windows runtime unit, actual primary D/R link-test4956/3660B and old app51768/39776B warning0, GCC/ASan/UBSan PASS, runtime line95.65 branch98.75 function100 (defensive second deadline branch unhit), docs strict API73, full Windows host D/R currently running (do not assume PASS). Actual IWDG/clock/stall/debug freeze/reset timing NOT_RUN. Report Korean Markdown raw text with executionID/start/end UTC/specialty/candidate/base/isolation+clean checks, actual files/commands, P0-P3 findings exact file:line/scenario/impact/recommendation, attacked cases if no finding, unreviewed scope and physical NOT_RUN, BLOCK/CONDITIONAL/PASS for this delta only. Return complete raw report as final message; do not write report file. Bounded focused review (~5-10 minutes), state limitations honestly.
Reviewer A: embedded watchdog/reset runtime, ownership, MCU register assumptions, clock/deadline/wrap/backward/fault latch, watchdog sequencing, safety output/preconditions; confirm real SDK values. Execution ID T107-RUNTIME-A-c0f1b81-20260919. Detached tree F:/dev/canview-wt/review-t107-runtime-a is ready at candidate. Do not inspect B outputs.
```

## 원본 결과

# T-107 bounded boot runtime 독립 리뷰 A

판정: **PASS — 지정된 NONDEPLOYABLE source delta에 한정한다.** 확인된 P0~P3 finding은 없다. T-107 전체 완료, 최종 bootloader, 배포 또는 merge 승인이 아니다.

## 실행·격리 확인

| 항목 | 확인 결과 |
|---|---|
| Execution ID | `T107-RUNTIME-A-c0f1b81-20260919` |
| 시작 UTC | `2026-09-19T12:08:16.6212975Z` |
| 종료 UTC | `2026-09-19T12:13:29.7138967Z` |
| 전문 영역 | embedded watchdog/reset runtime, 소유권, MCU register 전제, clock/deadline/wrap/역행/fault latch, watchdog 순서, 안전 출력 |
| Candidate | `c0f1b81449ee0507437927a8cc29ff98d2438d0e` |
| Base | `ce3fe1f5d4f0148fd632beb8f7016fcc0bae5fc9` |
| 검토 트리 | `F:/dev/canview-wt/review-t107-runtime-a` |
| 격리 | 제공된 detached worktree. `git symbolic-ref -q HEAD` 출력 없음 |
| 시작·종료 HEAD | 모두 candidate와 일치 |
| 시작·종료 clean | `git status --porcelain=v1` 출력 없음. 종료에는 `--untracked-files=all`도 확인 |
| SDK | `C:/cv/STM32CubeG4-1.6.3`, commit `d11b194a9f05d1b143d154771f3dbc282c8052a5`, 시작·종료 clean |
| 독립성 | Reviewer B의 이번 실행 출력·finding을 열람하지 않음 |

저장소 source·문서·SDK·사용자 파일을 수정하지 않았다. 검증 생성물은 해당 detached tree의 `build/T107-RUNTIME-A-c0f1b81-arm-{debug,release}`와 WSL `/tmp/T107-RUNTIME-A-c0f1b81-*`에만 생성했다. commit, push, 외부 하드웨어 접근, 보고서 파일 작성은 하지 않았다.

## 검토 범위

정본 `AGENTS.md`, `docs/README.md`, `docs/resume.md`, `docs/tasks/T-107-stm32-mcuboot.md`, `docs/architecture/ota.md` §5·7, `docs/runbooks/agent-workflow.md`, Windows 개발환경 문서를 참조했다. `embedded-cstyle`, `embedded-driver-design`, `embedded-isr-design` 스킬을 읽고 적용했다.

다음 16개 파일의 변경 hunk를 모두 검토했다.

```text
CMakeLists.txt
docs/api/Doxyfile
docs/api/stm32_core.rst
docs/journal.md
docs/resume.md
firmware/communicator/stm32/CMakeLists.txt
firmware/communicator/stm32/bootloader/README.md
firmware/communicator/stm32/bootloader/include/canview_boot_runtime.h
firmware/communicator/stm32/platform/stm32g474/boot_runtime.c
firmware/communicator/stm32/tests/register_model.h
tests/hil/fixtures/t103-capture-only.jsonl
tests/ota/stm32_boot_ram_link.c
tests/ota/test_stm32_boot_ram.py
tests/ota/test_stm32_boot_runtime.c
tests/test_t103_capture_helpers.py
tools/build_docs.py
```

호출 전후 계약 확인을 위해 기존 `board.c`, `safe_gpio.c`, `flash_io.c`, `flash_read.c`, `flash_command.c`, startup 코드, boot 설정과 CMake도 관련 부분을 읽었다. 변경되지 않은 코드 전체를 재감사한 것은 아니다.

## Findings

| 심각도 | 확인된 finding |
|---|---:|
| P0 | 0 |
| P1 | 0 |
| P2 | 0 |
| P3 | 0 |

선언된 후속 구현·물리 gate의 미완료 자체를 finding으로 만들지 않았다. 현재 delta의 계약·동작·evidence 표현에서 차단할 결함은 확인하지 못했다.

## 공격한 시나리오와 판단 근거

- **잘못된 실행 context 및 초기화 재시도:** `boot_runtime.c:59`, `:79`에서 ISR, CONTROL, interrupt mask, boot VTOR를 제한한다. 최초 시도부터 `attempted`를 설정하므로 부분 초기화 실패 후 같은 boot에서 재시도하지 않는다. 정상 시작 뒤 중복 호출도 추가 feed 없이 거절한다.
- **watchdog 조기 feed와 설정 동기화 실패:** `boot_runtime.c:89`, `:98`, `:107`, `:113`을 확인했다. DWT 증가·LSI 준비 이후 IWDG enable/write/PR/RLR 순서로 진행하며, SR 완료와 전체 readiness 검사 전에는 최초 `0xAAAA`를 쓰지 않는다. WINR 변이, LSI 미준비, SR stuck, 초기화 후반 context/config 손상 시험이 통과했다.
- **무조건 feed 및 readiness polling:** `boot_runtime.c:119`, `:137`에서 readiness 조회는 feed하지 않는다. progress에도 nominal 100ms 간격을 적용하고 nominal 30초 이후 feed를 중단한다. 반복 progress, 경계 직전·정확한 간격, readiness 1,000회 호출을 시험했다.
- **wrap·역행·정지·오류 복구에 의한 latch 해제:** `boot_runtime.c:127`에서 시작점과 직전 표본을 각각 unsigned 차이로 검사한다. 단일 wrap, 시작점 이전 역행, 시작점 이후이지만 직전 표본보다 역행, counter 정지, deadline 이후 시간 복구가 기존 시험에서 거절됐다. 오류 뒤 register 복구와 재시작으로 latch가 풀리지 않았다.
- **안전 출력 선행과 소유권 충돌:** `tests/ota/stm32_boot_ram_link.c:17`은 기존 BSP safe output 성공 뒤 runtime을 시작한다. 기존 GPIO 구현은 출력 latch를 설정한 뒤 output mode로 전환한다. boot link 대상에 앱 `core_hw.c`가 함께 들어가지 않으며, runtime이 CAN/UART/DMA를 시작하거나 ARM/WD pulse를 발생시키지 않는다.
- **Flash 임시 context와 runtime의 충돌:** 기존 Flash primitive가 임시 VTOR·PRIMASK를 복원한 뒤 `flash_io.c:67`, `:87`의 검증 완료 progress에 도달함을 확인했다. runtime 호출을 SRAM critical section 또는 ISR에 새로 삽입하지 않았다.
- **검증·문서의 과장:** README는 link-test를 비배포 image로 명시하고 boot_go·정책·handoff·물리 검증과 구분한다. journal은 전체 host D/R과 새 독립 리뷰를 진행 중으로 기록한다. API 추가 2개와 기대 개수 71→73, link object 6→9가 변경 내용과 일치한다. fixture 변경은 합성 firmware digest 교체이며 실제 차량 evidence 승격이 아니다.

### 실제 SDK 대조

고정 SDK 원본에서 다음을 직접 확인했다.

- `stm32g474xx.h`: HSION=`0x100`, HSIRDY=`0x400`, SW_HSI=`1`, SWS_HSI=`4`, LSION/LSIRDY=`1/2`, IWDG PVU/RVU/WVU=`1/2/4`, WIN mask=`0xFFF`.
- `stm32g4xx_hal_iwdg.h`: prescaler256=`6`, enable/write/reload key=`0xCCCC/0x5555/0xAAAA`.
- `stm32g4xx_hal_iwdg.c`의 `HAL_IWDG_Init`: enable → write access → PR/RLR → update 완료 대기 → reload 순서. WINR 쓰기의 암묵 reload 설명도 확인했다.
- `core_cm4.h`: DWT CYCCNTENA와 DEMCR TRCENA 비트가 register model과 일치한다.
- SDK `SystemInit` 및 target-local VTOR 설정: boot link의 vector base `0x08000000` 계약과 일치한다.

이는 source·정의값·링크 확인이며 실제 oscillator나 watchdog reset 측정은 아니다.

## 직접 실행한 검증

| 검증 | 결과 |
|---|---|
| Windows Clang 23.1, strict C99 runtime unit | PASS |
| WSL GCC 15.2, strict C99 runtime unit | PASS |
| WSL Clang 21.1.8, ASan/UBSan runtime unit | PASS, sanitizer 진단 없음 |
| Windows capture helper 회귀 | 13 tests PASS |
| Arm 15.3 Debug link 검사 | PASS, image 4956B, SRAM copy 1048B |
| Arm 15.3 Release link 검사 | PASS, image 3660B, SRAM copy 824B |
| Arm SRAM closure/negative object 검사 | D/R 각각 4 tests PASS |
| Arm startup/copy·실제 linker 거절 검사 | D/R 각각 2 tests PASS |
| `git diff --check base candidate` | PASS |

성공한 Arm Debug/Release configure·compile·link 출력에서 warning/error는 관찰되지 않았다. fixture 시험의 `BLOCKED`와 argparse 오류 출력은 의도된 음성 시험이며 최종 unittest 결과는 13/13 PASS다.

주요 실행 명령은 다음과 같다. 실행 위치는 모두 검토 트리이며, `cmake`는 사용자 지정 Windows CMake 4.4.3 실행 파일을 사용했다.

```powershell
git rev-parse HEAD
git symbolic-ref -q HEAD
git status --porcelain=v1 --untracked-files=all
git diff --name-only ce3fe1f5d4f0148fd632beb8f7016fcc0bae5fc9 c0f1b81449ee0507437927a8cc29ff98d2438d0e
git diff ce3fe1f5 c0f1b814 -- <검토 파일>
git diff --check ce3fe1f5 c0f1b814

python -B -m unittest discover -s tests -p test_t103_capture_helpers.py
```

Runtime unit은 아래 공통 입력·옵션으로 Windows Clang, WSL GCC, WSL Clang에서 각각 컴파일하고 생성 실행 파일을 실행했다.

```text
-std=c99 -O1 -g -Wall -Wextra -Werror -Wpedantic
-Wconversion -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wundef
-DCANVIEW_STM_REGISTER_TEST=1
-Ifirmware/communicator/stm32/bootloader/include
-Ifirmware/communicator/stm32/tests
-Ifirmware/communicator/stm32/bsp
-Ishared/interface
tests/ota/test_stm32_boot_runtime.c
firmware/communicator/stm32/platform/stm32g474/boot_runtime.c
```

WSL Clang에는 `-fsanitize=address,undefined -fno-omit-frame-pointer`를 추가했다. WSL 실행은 `wsl -d Ubuntu-26.04 --cd /mnt/f/dev/canview-wt/review-t107-runtime-a --exec ...`를 사용했다.

Arm 검증은 Debug/Release 각각 다음 구성과 target으로 실행했다.

```text
cmake -S firmware/communicator/stm32
  -B build/T107-RUNTIME-A-c0f1b81-arm-{debug,release}
  -G Ninja
  -DCMAKE_BUILD_TYPE={Debug,Release}
  -DCMAKE_TOOLCHAIN_FILE=F:/dev/canview-wt/review-t107-runtime-a/firmware/communicator/stm32/cmake/arm-none-eabi-gcc.cmake
  -DCMAKE_MAKE_PROGRAM=F:/dev/canview-wt/t104-stm32-uart-control/.tools/ninja-1.13.2/bin/ninja.exe
  -DSTM32CUBE_G4_ROOT=C:/cv/STM32CubeG4-1.6.3
  -DCANVIEW_MCUBOOT_ROOT=C:/cv/mcuboot-2.4.0
  -DCANVIEW_STM_IMAGE_LAYOUT=MCUBOOT_PRIMARY

cmake --build build/T107-RUNTIME-A-c0f1b81-arm-{debug,release}
  --target canview_boot_ram_link_check -j 4
```

초기 검증 호출에는 PowerShell/WSL 인자 quoting 실패와 toolchain 인자 분리로 인한 configure 실패가 있었다. 인자를 수정해 위 성공 실행으로 재검증했으며 source 결함으로 분류하지 않았다.

## 한계·미실행 범위

- 변경된 16개 파일 중 미검토 hunk는 없다. 약 5분의 집중 리뷰로 전체 T-107이나 모든 기존 dependency를 검토하지는 않았다.
- 전체 Windows host Debug/Release, 기존 앱 image 51768/39776B, strict docs/API73, 이전 CI artifact 감사는 이번 실행에서 재검증하지 않았다. 사용자가 전달한 관찰·candidate 문서 기록과 독립 실행 결과를 구분한다.
- coverage를 다시 측정하지 않았다. 보고된 line95.65/branch98.75/function100 수치를 독립 확인으로 주장하지 않는다. `boot_runtime.c:141`의 readiness 직후 deadline을 넘는 두 번째 방어 분기도 별도 재현하지 않았다.
- 실제 IWDG reset, HSI/LSI 편차, debug freeze/halt, reset·brownout timing, Flash stall/ECC/전원 차단, 전체 swap 실행 시간, 외부 CAN gate 전기 동작은 **NOT_RUN**이다.
- DWT full-wrap 전에 IWDG가 reset한다는 물리 전제는 이번 시험으로 입증되지 않는다. runtime의 nominal 시간값을 독립 wall-clock 보장으로 해석하지 않는다.
- 최종 boot_go·영속 정책·handoff, production provisioning, 모든 physical/HIL은 요청대로 범위 밖이다.

**최종 판정: PASS — candidate의 bounded boot runtime source delta만 승인 가능한 검토 결과다. NONDEPLOYABLE 경계와 기존 차량 TX NO-GO는 유지한다.**
