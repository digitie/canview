# T-107 runtime reviewer B 실행 evidence

- Subagent ID: `01a0b991-27a0-7611-9004-66250a3a9df3`
- 공통 요청 생성 UTC: `2026-09-19 12:08:01 UTC`
- 아래 원문은 요약·등급 변경 없이 보존한다.

## 전달 요청 원문

```text
T-107 bounded boot runtime source delta review. Repository F:/dev/canview-wt/t007-ota-container. Immutable candidate c0f1b81449ee0507437927a8cc29ff98d2438d0e; base ce3fe1f5d4f0148fd632beb8f7016fcc0bae5fc9. Scope: all 16 files from git diff --name-only base candidate. Main new source firmware/communicator/stm32/platform/stm32g474/boot_runtime.c, bootloader/include/canview_boot_runtime.h, tests/ota/test_stm32_boot_runtime.c; actual Arm link-test wires existing board safe output then runtime; CMake, register_model, test_link object count6->9; API docs73; firmware synthetic fixture digest update; README/resume/journal. Out-of-scope: final boot_go/policy/handoff, production provisioning, all physical/HIL; this is explicitly a NONDEPLOYABLE link-test, not final T107/merge approval. Do not invent findings solely because declared later implementation/physical gates remain open. Do look for unsafe present contracts/bugs/evidence claims. Canonical AGENTS.md then docs/README.md/docs/resume.md/T-107 task; related architecture/ota.md §5/7, agent-workflow. Read applicable embedded skills before review. Keep changes minimal; no changes to repository, SDK, user files; no commit or push. No external hardware access. Review every changed hunk independently without seeing other review. Verify candidate hash and clean start/end. Actual toolchain available Arm15.3 C:/Users/digit/AppData/Local/CANView/toolchains/arm-gnu-toolchain-15.3.rel1, SDK C:/cv/STM32CubeG4-1.6.3 (pinned clean d11b194a9f05d1b143d154771f3dbc282c8052a5), WSL Ubuntu-26.04 clang21/gcc15.2, Windows cmake F:/dev/canview-wt/t104-stm32-uart-control/.tools/cmake-4.4.3/bin/cmake-4.4.3-windows-x86_64/bin. You may run focused host tests in detached tree build/ or /tmp with unique A/B names. Main observed Windows runtime unit, actual primary D/R link-test4956/3660B and old app51768/39776B warning0, GCC/ASan/UBSan PASS, runtime line95.65 branch98.75 function100 (defensive second deadline branch unhit), docs strict API73, full Windows host D/R currently running (do not assume PASS). Actual IWDG/clock/stall/debug freeze/reset timing NOT_RUN. Report Korean Markdown raw text with executionID/start/end UTC/specialty/candidate/base/isolation+clean checks, actual files/commands, P0-P3 findings exact file:line/scenario/impact/recommendation, attacked cases if no finding, unreviewed scope and physical NOT_RUN, BLOCK/CONDITIONAL/PASS for this delta only. Return complete raw report as final message; do not write report file. Bounded focused review (~5-10 minutes), state limitations honestly.
Reviewer B: build/test/security/integration/evidence integrity, API ownership and test holes, generated fixture/source digest, strict compilation/target wiring/dependency changes. Execution ID T107-RUNTIME-B-c0f1b81-20260919. Detached tree F:/dev/canview-wt/review-t107-runtime-b ready at candidate. Do not inspect A outputs.
```

## 추가 공통 검증 전달 원문

```text
추가 공통 검증 관측(상대 reviewer finding 아님): candidate c0f1b81의 Windows Host Debug/Release 전체159/159,387.51s/101.79s로 완료. 실제 MCUboot108시나리오/11240cut 포함. main build/t107-runtime-host-{debug,release}-{build,test}.log, Arm final-primary D/R와 docs 로그 warning/error0. 이 결과는 coordinator 관측이며 직접 실행한 것과 구분하세요. 리뷰 범위를 확대하지 말고 현재 16파일 delta의 raw report를 완료해 주세요. 읽지 못한/시험하지 못한 부분은 명시하면 됩니다.
```

## 원본 결과

판정: **PASS — 지정된 16파일 bounded boot runtime source delta에 한정한다.** 확정된 P0–P3 finding은 없다. 최종 T-107 완료, merge 또는 배포 승인은 아니다. 대상 link-test는 **NONDEPLOYABLE**이다.

- executionID: `T107-RUNTIME-B-c0f1b81-20260919`
- 전문 영역: 빌드·시험·보안·통합·증거 무결성, API 소유권, 시험 누락, fixture/source digest, target wiring·의존성
- 직접 검토 시작 UTC: `2026-09-19T12:08:28.8978188Z`
- 직접 검토 종료 UTC: `2026-09-19T12:14:53.8127564Z`
- candidate: `c0f1b81449ee0507437927a8cc29ff98d2438d0e`
- base: `ce3fe1f5d4f0148fd632beb8f7016fcc0bae5fc9`
- 격리 경로: `F:/dev/canview-wt/review-t107-runtime-b`
- 격리 방식: candidate의 detached HEAD
- 독립성: 현재 Reviewer A의 출력·finding을 열람하지 않았다.
- 추가 공통 검증은 직접 검토 종료 후 전달받은 coordinator 관측으로 별도 기재한다. 검토 범위는 확대하지 않았다.

시작·종료 모두 HEAD가 candidate와 일치했고 `git status --porcelain=v1` 출력은 비어 있었다. 종료 시 unstaged/staged diff도 없었다. Base commit 존재와 candidate ancestry를 확인했다. STM32CubeG4는 시작·종료 모두 `d11b194a9f05d1b143d154771f3dbc282c8052a5`이며 clean이었다. MCUboot checkout도 종료 시 clean이었다.

저장소 source, SDK, 사용자 파일을 수정하지 않았다. 허용된 격리 트리의 `build/T107-RUNTIME-B-*`와 WSL `/tmp`에 검증 산출물만 생성했다. 보고서 파일 작성, commit, push, 외부 하드웨어 접근은 하지 않았다.

정본 `AGENTS.md`, `docs/README.md`, `docs/resume.md`, `docs/tasks/T-107-stm32-mcuboot.md`, architecture 지도와 OTA §5·7, agent-workflow 및 Windows 개발 절차를 참조했다. `embedded-architecture`, `embedded-cstyle`, `embedded-documentation` 스킬을 적용했다.

`git diff --name-only base candidate`의 다음 16개 파일 전체 변경 hunk를 검토했다. 읽지 못한 변경 hunk는 없다.

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

연결 관계 확인을 위해 기존 `bsp/board.c`, `safe_gpio.c`, `flash_io.c`, `canview_boot_flash.h`, `mcuboot_config.h`, toolchain/preset, warning 설정, fixture digest 계산기와 pinned SDK의 IWDG 구현·CMSIS 정의도 읽었다.

직접 실행한 검증 결과는 다음과 같다.

| 검증 | Reviewer B 직접 관측 |
|---|---|
| Windows Clang 23.1.0 focused Debug | 6/6 PASS |
| Windows Clang 23.1.0 focused Release | 6/6 PASS |
| WSL GCC 15.2 strict C99 runtime | PASS |
| WSL Clang 21.1.8 ASan/UBSan runtime | PASS |
| Arm GNU 15.3.1 Debug link-test | PASS, image 4,956B, SRAM copy 1,048B |
| Arm GNU 15.3.1 Release link-test | PASS, image 3,660B, SRAM copy 824B |
| 실제 Arm SRAM object 음성 시험·linker/copy 변이 시험 | D/R 각각 PASS |
| 기존 primary 앱 Debug/Release 빌드 | PASS, payload 51,768/39,776B |
| T-103 capture helper 시험 | 13/13 PASS |
| Doxygen 공개 API 계약 검사 | 73개 PASS |
| Sphinx 9.1.0 strict 문서 빌드 | PASS |
| `git diff --check base candidate` | PASS |

Windows focused 6개는 `stm32-boot-runtime`, `stm32-core-register-model`, `stm32-flash-command`, `stm32-flash-read`, `stm32-flash-guard`, `stm32-flash-io`다.

추가로 전달받은 coordinator 관측은 다음과 같다. 이 결과와 원본 로그를 Reviewer B가 직접 실행·열람·재검증한 것으로 표시하지 않는다.

| 항목 | Coordinator 전달 결과 |
|---|---|
| Candidate `c0f1b81` Windows Host Debug 전체 | 159/159 PASS, 387.51초 |
| Candidate `c0f1b81` Windows Host Release 전체 | 159/159 PASS, 101.79초 |
| 실제 MCUboot 모형 | 108시나리오·11,240 cut 포함 |
| 전체 host 증거 위치 | main `build/t107-runtime-host-{debug,release}-{build,test}.log` |
| Arm final-primary D/R 및 docs 로그 | warning/error 0 |

직접 실행한 주요 명령은 다음과 같다. 아래 `{debug,release}`·`{Debug,Release}` 표기는 각각 별도로 실행한 명령을 묶은 것이다. Windows CMake 4.4.3, Ninja 1.13.2, Clang 23.1.0은 기존 설치의 절대 경로를 명시했다.

```text
git rev-parse HEAD
git symbolic-ref -q HEAD
git status --porcelain=v1
git diff --name-only <base> <candidate>
git diff <base> <candidate> -- <검토 대상>
git diff --check <base> <candidate>
git merge-base --is-ancestor <base> <candidate>

cmake -S firmware/communicator/stm32
  -B build/T107-RUNTIME-B-arm-{debug,release} -G Ninja
  -DCMAKE_TOOLCHAIN_FILE=<격리 트리>/firmware/communicator/stm32/cmake/arm-none-eabi-gcc.cmake
  -DSTM32CUBE_G4_ROOT=C:/cv/STM32CubeG4-1.6.3
  -DCANVIEW_MCUBOOT_ROOT=C:/cv/mcuboot-2.4.0
  -DCMAKE_BUILD_TYPE={Debug,Release}
  -DCANVIEW_STM_IMAGE_LAYOUT=MCUBOOT_PRIMARY

cmake --build build/T107-RUNTIME-B-arm-{debug,release}
  --target canview_boot_ram_link_check -j 4
cmake --build build/T107-RUNTIME-B-arm-{debug,release}
  --target canview-communicator-stm32 -j 4

cmake -S . -B build/T107-RUNTIME-B-host-{debug,release} -G Ninja
  -DCMAKE_C_COMPILER=<기존 LLVM23>/bin/clang.exe
  -DCMAKE_BUILD_TYPE={Debug,Release} -DBUILD_TESTING=ON
cmake --build <각 host build> --target <위 6개 시험 executable> -j 4
ctest --test-dir <각 host build>
  -R '^(stm32-boot-runtime|stm32-core-register-model|stm32-flash-command|stm32-flash-read|stm32-flash-guard|stm32-flash-io)$'
  --output-on-failure

python -X utf8 -B -m unittest discover -s tests -p test_t103_capture_helpers.py -v
arm-none-eabi-nm.exe build/T107-RUNTIME-B-arm-release/canview-boot-ram-link-test.elf
```

WSL에서는 실제 `test_stm32_boot_runtime.c`와 `boot_runtime.c`를 함께 컴파일했다. GCC와 Clang 모두 `-std=c99 -Wall -Wextra -Werror -Wpedantic -Wconversion -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wundef`를 적용했고, Clang에는 `-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer`를 추가했다.

문서는 Doxyfile의 출력만 `build/T107-RUNTIME-B-api/doxygen`으로 지정해 생성했다. XML에 `tools.build_docs.check_api()`를 실행하고, Sphinx에는 해당 XML 경로와 `nitpicky=True`, `warningiserror=True`를 지정했다.

최초 실행에서 WSL 변수 전달, PowerShell toolchain 인자 분리, Windows 개발 셸 미초기화에 따른 RC 탐색, XML 생성 전 Sphinx 시작 오류가 있었다. 명령 인용·환경·실행 순서를 바로잡은 뒤 위 검증은 통과했다. Candidate 결함으로 분류하지 않았다.

**Finding: P0 0건, P1 0건, P2 0건, P3 0건.**

Finding이 없더라도 검토한 실패 시나리오와 근거는 다음과 같다.

- `firmware/communicator/stm32/platform/stm32g474/boot_runtime.c:77`: 초기화 전 호출, 재호출, 부분 초기화 실패, 잘못된 context·clock·window를 검토했다. 실패 후 같은 boot에서 재시도와 feed 재개를 거절하는 시험이 통과했다.
- 같은 파일 `:98`: IWDG enable/write/PR/RLR/SR 대기/최초 feed 순서를 pinned HAL과 대조했다. WINR를 쓰지 않고 reset window 불일치를 거절한다. 새 HSI register 모형 상수는 CMSIS와 일치한다.
- 같은 파일 `:119`, `:128`, `:129`, `:137`: context/config 변이, deadline 경계, unsigned 단일 wrap, counter 정지·역행, 반복 progress와 readiness 조회를 공격했다. Readiness 자체는 feed하지 않고 오류 latch 이후 설정을 복원해도 feed를 재개하지 않는다.
- `tests/ota/stm32_boot_ram_link.c:17`: safe-state 함수 존재와 성공을 확인한 뒤 runtime을 시작하는 short-circuit 순서다. 실제 Arm ELF에서 safe-state·runtime·progress 심볼이 유지됨을 확인했다.
- `firmware/communicator/stm32/CMakeLists.txt:253`: runtime은 boot link-test에 추가되며 일반 앱 `core_hw`와 동일 executable에 합쳐지지 않는다. Compile command의 strict C99, CAPTURE_ONLY forced include, boot VTOR offset 0을 확인했다. 9개 객체로 실제 linker 변이 시험이 통과했고 기존 primary 앱도 빌드됐다.
- `tests/hil/fixtures/t103-capture-only.jsonl:1`, `tests/test_t103_capture_helpers.py:26`: 실제 계산한 source digest는 `355e0b97e4a16ef7449e01864ac2cfc26057e449ff9489d384880c46893393e6`으로 fixture 5개 레코드와 기대값에 일치했다. 잘못된 candidate·harness·channel·TX·replay 거절 시험이 통과했다. 합성 fixture를 실차 evidence로 승격한 변경은 없다.
- `docs/api/Doxyfile:7`, `tools/build_docs.py:26`: 공개 API 증가 71→73을 실제 추출로 확인했다. README/resume/journal은 비배포 image와 final loader 미연결, physical NOT_RUN을 구분한다. 이전 CI를 새 후보 CI 통과로 대체하지 않았다.

검증하지 않은 부분과 한계는 다음과 같다.

- 전체 Windows host 159/159와 MCUboot 108시나리오·11,240 cut은 coordinator 관측이다. Reviewer B는 전체 suite와 해당 원본 로그를 직접 재검증하지 않았다.
- 제시된 runtime coverage line 95.65%·branch 98.75%·function 100%는 재측정하지 않았다. `boot_runtime.c:141`의 readiness 직후 deadline 재검사 분기는 별도 경계 주입으로 입증하지 않았다.
- 새 후보 CI·artifact 감사는 직접 확인하지 않았다.
- 실제 IWDG/HSI/LSI 동작, debug freeze, Flash stall, reset timing, DWT full-wrap 이전 reset 전제, 전원·ECC·torn word/page와 모든 physical/HIL은 **NOT_RUN**이다.
- Final `boot_go`/정책/handoff, production provisioning, 전체 MCUboot 실행 시간·최종 stack 수용은 범위 밖이다. 선언된 후속 gate가 열려 있다는 이유만으로 finding을 만들지 않았다.

최종 판정은 **이 16파일 delta에 대한 PASS**다. **NONDEPLOYABLE, T-107 IN_PROGRESS, physical/HIL NOT_RUN, 차량 TX NO-GO** 경계는 유지한다.
