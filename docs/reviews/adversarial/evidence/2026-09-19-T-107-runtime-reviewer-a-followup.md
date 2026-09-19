# T-107 runtime reviewer A 추가 공통 검증 전달 응답

- Subagent ID: `01a0b990-fc33-7463-8aa5-b705b4e80786`
- 최초 원본은 [reviewer A](2026-09-19-T-107-runtime-reviewer-a.md)에 그대로 보존했다.
- 아래는 coordinator의 후속 시험 완료 통지에 대한 같은 reviewer의 응답 원문이다. 추가 source review 실행으로 계산하지 않는다.

## 추가 전달 원문

```text
추가 공통 검증 관측(상대 reviewer finding 아님): candidate c0f1b81의 Windows Host Debug/Release 전체159/159,387.51s/101.79s로 완료. 실제 MCUboot108시나리오/11240cut 포함. main build/t107-runtime-host-{debug,release}-{build,test}.log, Arm final-primary D/R와 docs 로그 warning/error0. 이 결과는 coordinator 관측이며 직접 실행한 것과 구분하세요. 리뷰 범위를 확대하지 말고 현재 16파일 delta의 raw report를 완료해 주세요. 읽지 못한/시험하지 못한 부분은 명시하면 됩니다.
```

## 원본 결과

# T-107 bounded boot runtime source delta — Reviewer A

**판정: PASS — 지정된 16파일 NONDEPLOYABLE delta에 한정한다.** 확인된 P0~P3 finding은 없다. T-107 전체 완료, 배포 또는 merge 승인이 아니다.

## 실행 정보와 격리

| 항목 | 결과 |
|---|---|
| Execution ID | `T107-RUNTIME-A-c0f1b81-20260919` |
| 검토 시작 UTC | `2026-09-19T12:08:16.6212975Z` |
| 검토·종료 검사 완료 UTC | `2026-09-19T12:13:29.7138967Z` |
| 전문 영역 | watchdog/reset runtime, 소유권, MCU register 전제, clock/deadline/wrap/역행/fault latch, watchdog 순서, 안전 출력 |
| Candidate | `c0f1b81449ee0507437927a8cc29ff98d2438d0e` |
| Base | `ce3fe1f5d4f0148fd632beb8f7016fcc0bae5fc9` |
| 격리 트리 | `F:/dev/canview-wt/review-t107-runtime-a` |
| 격리 방식 | 제공된 detached worktree |
| 시작·종료 HEAD | 모두 candidate와 일치 |
| 시작·종료 clean | `git status --porcelain=v1` 출력 없음. 종료에는 `--untracked-files=all`도 확인 |
| SDK | `C:/cv/STM32CubeG4-1.6.3`, commit `d11b194a9f05d1b143d154771f3dbc282c8052a5`, 시작·종료 clean |
| 독립성 | 이번 Reviewer B 출력·finding 미열람 |

위 시각은 실제 검토와 도구 실행 구간이다. 이후 전달된 coordinator 공통 검증 결과는 아래 별도 항목에 반영했다. 추가 범위 검토나 도구 실행은 하지 않았다.

Source·문서·SDK·사용자 파일을 수정하지 않았다. 검증 생성물은 detached tree의 `build/T107-RUNTIME-A-c0f1b81-arm-{debug,release}`와 WSL `/tmp/T107-RUNTIME-A-c0f1b81-*`에 생성했다. commit, push, 외부 하드웨어 접근, 보고서 파일 작성은 하지 않았다.

## 읽은 기준과 파일 범위

정본 `AGENTS.md`, `docs/README.md`, `docs/resume.md`, `docs/tasks/T-107-stm32-mcuboot.md`, `docs/architecture/ota.md` §5·7, `docs/runbooks/agent-workflow.md`, Windows 개발환경 문서를 참조했다.

`embedded-cstyle`, `embedded-driver-design`, `embedded-isr-design` 스킬을 읽고 적용했다.

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

호출 계약을 확인하기 위해 기존 `board.c`, `safe_gpio.c`, `flash_io.c`, `flash_read.c`, `flash_command.c`, startup 코드와 boot CMake의 관련 부분도 읽었다. 기존 코드 전체를 재감사한 것은 아니다.

## Findings

| 심각도 | 확인된 finding |
|---|---:|
| P0 | 0 |
| P1 | 0 |
| P2 | 0 |
| P3 | 0 |

현재 delta에서 구체적인 실패 시나리오로 뒷받침되는 결함은 확인하지 못했다. 선언된 후속 구현·물리 gate 미완료 자체를 finding으로 분류하지 않았다.

## 공격한 시나리오와 판단 근거

아래 경로의 `boot_runtime.c`는 `firmware/communicator/stm32/platform/stm32g474/boot_runtime.c`다.

- **잘못된 context와 초기화 재시도 — `boot_runtime.c:59`, `:79`:** ISR, CONTROL, interrupt mask, boot VTOR를 검사한다. 최초 시도부터 `attempted`를 설정해 부분 초기화 실패 후 재시도를 거절한다. 정상 시작 뒤 중복 호출도 추가 feed 없이 거절한다.
- **조기 feed·동기화 실패 — `boot_runtime.c:89`, `:98`, `:107`, `:113`:** DWT 증가와 LSI 준비 이후 IWDG enable/write/PR/RLR 순서로 진행한다. SR 완료와 전체 readiness 검사 전에는 최초 reload가 없다. WINR 변이, LSI 미준비, SR stuck, 초기화 후반 context/config 손상 시험이 통과했다.
- **무조건 feed·readiness polling — `boot_runtime.c:119`, `:137`:** readiness 조회는 feed하지 않는다. progress에는 nominal100ms 간격을 적용하고 nominal30초 만료 이후 feed를 중단한다. 반복 progress, 간격 경계, readiness1,000회 호출 시험이 통과했다.
- **wrap·역행·정지·latch 해제 — `boot_runtime.c:127`:** 시작점과 직전 표본에 대한 unsigned 차이를 각각 검사한다. 단일 wrap, 시작점 이전 역행, 시작점 이후이지만 직전 표본보다 역행, counter 정지, deadline 이후 시간 복구를 시험했다. register 복구나 재시작으로 오류 latch가 풀리지 않았다.
- **안전 출력·소유권 — `tests/ota/stm32_boot_ram_link.c:17`:** 기존 BSP safe output 성공 뒤 runtime을 시작한다. GPIO 구현은 출력 latch 설정 뒤 output mode로 전환한다. boot link에 앱 `core_hw.c`가 함께 들어가지 않으며 runtime은 CAN/UART/DMA를 시작하거나 ARM/WD pulse를 발생시키지 않는다.
- **Flash 임시 context와의 충돌 — `firmware/communicator/stm32/platform/stm32g474/flash_io.c:67`, `:87`:** 기존 primitive가 임시 VTOR·PRIMASK를 복원한 뒤 검증 완료 progress를 호출함을 확인했다. runtime 호출을 SRAM critical section이나 ISR에 새로 삽입하지 않았다.
- **검증 계약·evidence 과장:** API 추가2개와 기대 개수71→73, link object6→9가 실제 변경과 일치한다. README는 비배포 link-test와 최종 boot_go·정책·handoff·물리 검증을 구분한다. fixture 수정은 합성 firmware digest 갱신이며 실제 차량 evidence로 승격하지 않는다.

### 실제 SDK 대조

고정 SDK 원본에서 다음을 직접 확인했다.

| SDK 항목 | 대조 결과 |
|---|---|
| `stm32g474xx.h` | HSION=`0x100`, HSIRDY=`0x400`, SW_HSI=`1`, SWS_HSI=`4`, LSION/LSIRDY=`1/2` |
| IWDG 정의 | PVU/RVU/WVU=`1/2/4`, WIN mask=`0xFFF`, prescaler256=`6` |
| IWDG key | enable/write/reload=`0xCCCC/0x5555/0xAAAA` |
| `stm32g4xx_hal_iwdg.c` | enable → write access → PR/RLR → update 완료 대기 → reload 순서 일치 |
| WINR | 쓰기의 암묵 reload 설명 확인 |
| `core_cm4.h` | CYCCNTENA·TRCENA 비트와 register model 일치 |
| SDK `SystemInit`·target 설정 | boot VTOR `0x08000000` 계약과 일치 |

이 결과는 정의값·source·링크 확인이다. 실제 oscillator나 watchdog reset 측정으로 해석하지 않는다.

## Reviewer A가 직접 실행한 검증

| 검증 | 결과 |
|---|---|
| Windows Clang23.1 strict C99 runtime unit | PASS |
| WSL GCC15.2 strict C99 runtime unit | PASS |
| WSL Clang21.1.8 ASan/UBSan runtime unit | PASS, sanitizer 진단 없음 |
| Windows capture helper 회귀 | 13 tests PASS |
| Arm15.3 Debug link 검사 | PASS, image4956B, SRAM copy1048B |
| Arm15.3 Release link 검사 | PASS, image3660B, SRAM copy824B |
| SRAM closure/negative object 검사 | D/R 각각4 tests PASS |
| startup/copy·실제 linker 거절 검사 | D/R 각각2 tests PASS |
| `git diff --check base candidate` | PASS |

성공한 Arm D/R configure·compile·link 출력에서 warning/error를 관찰하지 않았다. Capture helper의 `BLOCKED`·argparse 오류 출력은 의도된 음성 시험이며 최종 unittest 결과는13/13 PASS다.

주요 실행 명령:

```powershell
git rev-parse HEAD
git symbolic-ref -q HEAD
git status --porcelain=v1 --untracked-files=all
git diff --name-only ce3fe1f5d4f0148fd632beb8f7016fcc0bae5fc9 c0f1b81449ee0507437927a8cc29ff98d2438d0e
git diff ce3fe1f5 c0f1b814 -- <검토 파일>
git diff --check ce3fe1f5 c0f1b814
python -B -m unittest discover -s tests -p test_t103_capture_helpers.py
```

Runtime unit은 Windows Clang, WSL GCC, WSL Clang에서 다음 공통 입력으로 컴파일 후 실행했다.

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

WSL Clang에는 `-fsanitize=address,undefined -fno-omit-frame-pointer`를 추가했다. WSL은 `Ubuntu-26.04`의 해당 Windows worktree 경로에서 실행했다.

Arm은 사용자 지정 Windows CMake4.4.3, Ninja1.13.2, Arm GNU15.3.Rel1을 사용했다. 아래 구성은 Debug/Release 각각 실행했다.

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

초기 호출에는 PowerShell/WSL 인자 quoting 실패와 toolchain 인자 분리에 따른 configure 실패가 있었다. 인자를 수정해 성공 실행으로 재검증했으며 source finding으로 분류하지 않았다.

## Coordinator 관측 — 직접 실행·로그 열람과 구분

검토 종료 뒤 coordinator가 다음 완료 결과를 전달했다.

| 공통 검증 | 전달받은 결과 |
|---|---|
| candidate Windows Host Debug 전체 | 159/159 PASS,387.51초 |
| candidate Windows Host Release 전체 | 159/159 PASS,101.79초 |
| 포함 범위 | 실제 MCUboot108시나리오·11240cut |
| host 로그 | main `build/t107-runtime-host-{debug,release}-{build,test}.log` |
| Arm final-primary D/R 및 docs 로그 | warning/error0 |

이 결과는 **coordinator 관측**으로 반영했다. Reviewer A가 전체 host suite를 직접 실행하거나 위 main 로그를 열람한 결과가 아니다. 상대 reviewer finding을 받은 것도 아니다. Candidate journal의 “전체 host D/R 진행 중”은 작성 당시 기록이며, 후속 완료 관측과 구분한다.

## 한계·미실행 범위

- 지정16파일의 미검토 hunk는 없다. 약5분 집중 리뷰이며 전체 T-107과 모든 기존 dependency를 재감사하지 않았다.
- 전체 Windows host D/R, 기존 앱 image 크기, strict docs/API73, 이전 CI artifact 감사는 독립 재실행하지 않았다.
- Coverage를 다시 측정하지 않았다. 전달된 line95.65/branch98.75/function100을 독립 확인 수치로 주장하지 않는다. `boot_runtime.c:141`의 readiness 직후 deadline을 넘는 두 번째 방어 분기도 별도 재현하지 않았다.
- 실제 IWDG reset, HSI/LSI 편차, debug freeze/halt, reset·brownout timing, Flash stall/ECC/전원 차단, 전체 swap 실행 시간, 외부 CAN gate 전기 동작은 **NOT_RUN**이다.
- DWT full-wrap 전에 IWDG가 reset한다는 물리 전제는 입증하지 않았다. Nominal 시간값은 독립 wall-clock 보장이 아니다.
- 최종 boot_go·영속 정책·handoff, production provisioning, 모든 physical/HIL은 요청대로 범위 밖이다.

**최종 판정: PASS — 현재16파일 source delta에 한정한다. NONDEPLOYABLE 경계, T-107 미완료 및 차량 TX NO-GO를 유지한다.**
