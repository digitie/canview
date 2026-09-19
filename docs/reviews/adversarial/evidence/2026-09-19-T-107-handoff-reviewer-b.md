# T-107 handoff Reviewer B 원본 evidence

- subagent execution ID: `01a0b9b3-dc6b-78f3-bcb7-2e4d560d8112`
- candidate: `1e66a047c4a47a3535b0346fb0e01d17ccc703b6`
- base: `965ada0faa6a40925fc460585481615005fcddf1`
- raw 결과는 아래에 그대로 보존한다. 판정 범위와 NOT_RUN은 원문을 따른다.
- 최초 final은 thread의 `final_answer` 항목에서 직접 회수했다. 후속 메시지 처리 뒤의 보충 final과 구분한다.

## 전달한 요청 원문

```text
T-107 handoff primitive immutable review
candidate 1e66a047c4a47a3535b0346fb0e01d17ccc703b6
base 965ada0faa6a40925fc460585481615005fcddf1
Task T-107 IN_PROGRESS, PR37 Draft. Scope ALL19files in git diff base..candidate; focus boot_handoff.c, public contract, BSP constants, register model, C test, real Arm naked trampoline checker+128 bit mutations, CMake, CI provenance, fixture identity, API/docs.
Read AGENTS.md/docs/README.md/docs/resume.md/current T107 task; applicable embedded skills fully then relevant architecture OTA5/7 and workflow/review instructions. Don't read other reviewer results or prior-review conclusions as endorsement.
Review only immutable candidate detached worktree, verify HEAD and clean at start/end. Do not edit tracked files, commit, push, touch hardware or keys. Builds ignored build/ allowed; source outside assigned worktree read only.
Acceptance: bounded primary-only vector/authenticated-length, MSP exact, Thumb and initial32bit fetch in payload; reject before cleanup on bad runtime/read/MPU/FPU; no new signature/policy authorization; keep IWDG/safe GPIO; cleanup SysTick/NVIC before minimum MSP tail branch without C stack access afterMSP. Not final bootloader: boot_go/policy-v1/floor/activation/confirmation linkage remains, no caller may jump before those gates. No hardware physical handoff/NMI/reset/torn flash qualification: NOT_RUN, vehicleTX NO-GO.
Locally Windows whole Debug160passed before final min-fetch2->4 tightening; rebuilt focused8passed after; Release whole160 in progress. actual Arm D/R ELF/MAP/BIN link-test5284/3924B plus primary51768/39776B warning0. LinuxGCC strict+ASanUBSan handoff and hostCcoverage100. APIs74strict. These are coordinator claims not independent evidence.
SDK C:/cv/STM32CubeG4-1.6.3, MCUboot C:/cv/mcuboot-2.4.0; ArmGCC C:/Users/digit/AppData/Local/CANView/toolchains/arm-gnu-toolchain-15.3.rel1; CMake F:/dev/canview-wt/t104-stm32-uart-control/.tools/cmake-4.4.3/bin/cmake-4.4.3-windows-x86_64/bin; Ninja F:/dev/canview-wt/t104-stm32-uart-control/.tools/ninja-1.13.2/bin/ninja.exe. WSL Ubuntu-26.04 has GCC15/clang21.1.8 for focused test direct compilation with CANVIEW_STM_REGISTER_TEST=1 includes bootloader/include,tests,interface,bsp,shared/interface. Real Arm config use existing CMake presets with STM32CUBE_G4_ROOT and CANVIEW_MCUBOOT_ROOT explicit -D and same installed toolchain; don't copy old artifacts as your evidence.
Return complete Korean raw Markdown report in final response including executionID, UTC start/end, role, candidate/base, isolation/start/endclean, all actually read files/commands, P0/P1/P2/P3 with exactfile:line/scenario/impact/recommendation (or explicitzero), unreviewed scope, NOT_RUN, BLOCK/CONDITIONAL/PASS limited delta. Do not claim PASS if incomplete. No broad entireT107 approval. Don't wait for instructions at end.
Execution T107-HANDOFF-B-1e66a04-20260919. Reviewer B build/security/test role. Assigned detached worktree F:/dev/canview-wt/review-t107-handoff-b. Focus negative tests/assembly mutation robustness/CMake actual target link/compiler warnings/CMSIS fixture constants/provenance/docs consistency/security approval boundaries. Return raw report without writing tracked files.
```

## 공통 추가 전달

```text
공통 manifest 증거 갱신만 전달합니다(상대 finding 없음): Release전체160/160완료103.07s. finalcandidate Debug전체재실행159/160(372.43s), 실패trust는 coordinator가 VS DevShell없이실행해 CMAKE_RC_COMPILER없음으로configure실패; 올바른환경으로해당시험재실행중. handoff/MCUboot108시나리오11240cut 포함 나머지159통과. 기준commit변경없음. 새발견/완료전에는최종보고하지말되 현재분량 scope의보고를마무리해주세요.
```

## Reviewer 원본 결과

로컬 worktree 링크를 저장소 navigation으로 바꾸지 않도록 원문을 fence 안에 보존한다.

````markdown
# T-107 handoff primitive — Reviewer B 독립 리뷰

판정: **CONDITIONAL — 지정된 19파일 delta에 한정**. 재현된 P0/P1/P2/P3 finding은 0건이다. 다만 candidate 전체 160개 host 시험과 해당 candidate의 원격 CI artifact 감사는 이번 실행에서 수행하지 않았으므로 무조건 PASS로 판정하지 않는다. T-107 전체 완료·PR37 merge·배포 승인이 아니다.

## 실행 및 격리

| 항목 | 확인 결과 |
|---|---|
| executionID | `T107-HANDOFF-B-1e66a04-20260919` |
| 역할 | Reviewer B — build/security/test |
| UTC 시작 | `2026-09-19 12:46:21 UTC` |
| UTC 종료 | `2026-09-19 12:54:21 UTC` |
| candidate | `1e66a047c4a47a3535b0346fb0e01d17ccc703b6` |
| base | `965ada0faa6a40925fc460585481615005fcddf1` |
| 격리 | `F:/dev/canview-wt/review-t107-handoff-b`, detached worktree |
| 시작 | HEAD 일치, `git status --short` 출력 없음, symbolic ref 없음 |
| 종료 | HEAD 일치, `git symbolic-ref -q HEAD` exit 1, porcelain 출력 없음 |
| tracked/index 변경 | `git diff --exit-code`, `git diff --cached --exit-code` 모두 차이 없음 |
| 산출물 | 지정 worktree의 ignored `build/`에 새로 생성 |
| 금지 작업 | tracked 파일 수정·commit·push·하드웨어 접근·키 조작 없음 |

다른 reviewer 원문은 열지 않았다. 필수 resume/task/review index에 포함된 과거 판정 요약은 승인 근거로 사용하지 않았다. Coordinator의 수치는 아래에서 직접 재실행한 항목만 독립 증거로 인정했다.

## Finding

| 심각도 | 건수 | 판정 |
|---|---:|---|
| P0 | 0 | 이번 delta에서 확인된 finding 없음 |
| P1 | 0 | 이번 delta에서 확인된 finding 없음 |
| P2 | 0 | 이번 delta에서 확인된 finding 없음 |
| P3 | 0 | 이번 delta에서 확인된 finding 없음 |

Finding이 없으므로 결함별 위치·재현·영향·권고 항목은 해당 없음이다. 아래는 실제 공격 범위와 확인 근거이며, 물리 안전 보증으로 확대하지 않는다.

## 수용 조건 검토

- [boot_handoff.c:49](/F:/dev/canview-wt/review-t107-handoff-b/firmware/communicator/stm32/platform/stm32g474/boot_handoff.c:49): payload 길이를 `476..183296B`로 제한한 뒤 주소 산술을 수행한다. 고정 primary `0x08010200`에서 첫 8B만 읽는다. 임의 vector/secondary 주소 입력은 없다.
- [boot_handoff.c:63](/F:/dev/canview-wt/review-t107-handoff-b/firmware/communicator/stm32/platform/stm32g474/boot_handoff.c:63): MSP는 정확히 `0x20018000`, entry는 Thumb이며 vector 472B 뒤부터 시작해야 한다. 마지막 4B fetch까지 authenticated payload에 들어가야 한다. 끝에서 2B만 남은 entry는 거절된다.
- [test_stm32_boot_handoff.c:112](/F:/dev/canview-wt/review-t107-handoff-b/tests/ota/test_stm32_boot_handoff.c:112): 잘못된 길이 8종, MSP 32개 단일 비트 변이, entry 경계 9종, runtime/MPU/FPU 전후 오류 6종, read 오류 4종, 성공 경계 3종을 실행했다. 거절 시 IRQ disable·handoff register 변경·branch가 발생하지 않는 검사도 통과했다.
- [boot_handoff.c:70](/F:/dev/canview-wt/review-t107-handoff-b/firmware/communicator/stm32/platform/stm32g474/boot_handoff.c:70): 최종 readiness 확인 뒤 IRQ를 막고 SysTick, NVIC enable/pending, PendST/PendSV를 정리한다. 이어 barrier와 VTOR 변경을 수행한다. Debug/Release 실제 disassembly에서도 이 순서를 확인했다.
- [boot_handoff.c:18](/F:/dev/canview-wt/review-t107-handoff-b/firmware/communicator/stm32/platform/stm32g474/boot_handoff.c:18): 양쪽 ELF의 trampoline은 `LR=-1 → MSP=r0 → ISB → CPSIE i → BX r1`의 16B다. MSP 변경 뒤 C stack 접근이나 복귀 명령이 없다.
- [check_stm32_boot_ram.py:94](/F:/dev/canview-wt/review-t107-handoff-b/tools/ota/check_stm32_boot_ram.py:94): 실제 BIN의 해당 명령열을 검사한다. 상위 검사는 ELF/BIN load byte 일치도 확인하므로 BIN만 별도로 맞추는 검사가 아니다. Debug/Release 각각 128개 단일 비트 변이와 truncation을 거절했다.
- [canview_boot_handoff.h:8](/F:/dev/canview-wt/review-t107-handoff-b/firmware/communicator/stm32/bootloader/include/canview_boot_handoff.h:8): `boot_go` FIH 성공, T-205 정책 승인, 동일 이미지의 authenticated length, Flash/DMA/ISR/callback 변경 금지가 caller 전제로 명시돼 있다. 새 signature 검증 대체 수단이나 approval flag를 추가하지 않았다.
- [stm32_boot_ram_link.c:25](/F:/dev/canview-wt/review-t107-handoff-b/tests/ota/stm32_boot_ram_link.c:25): 현재 비배포 caller는 길이 0의 거절만 시험한다. 검색된 제품 경로에 유효 길이로 승인 전 jump하는 caller는 없었다.
- handoff 자체는 GPIO와 IWDG를 변경하거나 feed하지 않는다. 기존 runtime의 context/clock/IWDG readiness 계약을 사용한다. safe output 유지의 전기적 성질은 이 소프트웨어 검증으로 입증하지 않았다.
- 새 mock mask 3개는 실제 CMSIS 비교 gate에 포함된다. SDK의 `ICER[8]`, `ICPR[8]`도 모델과 일치한다. BSP vector 크기·MSP·payload 상수는 primary linker 및 실제 ELF 검사와 일치한다.
- fixture digest는 현재 source에서 재계산되는 값과 일치했고, 다른 candidate/harness identity와 재생·잘린 evidence를 거절했다. 합성 fixture는 실차 capture 증거로 취급하지 않았다.
- CI 변경은 runtime/handoff source hash를 기존 provenance 목록에 추가한다. immutable PR head 비교와 실제 artifact hash 수집 경로를 확인했다. 해당 candidate의 CI 실행 결과 자체는 감사하지 않았다.

## 독립 실행 결과

| 검증 | 결과 |
|---|---|
| Windows Clang 23.1.0 host Debug 빌드 | 성공 |
| Windows host Debug 관련 CTest | **12/12 통과** |
| Windows Clang 23.1.0 host Release 빌드 | 성공 |
| Windows host Release 관련 CTest | **12/12 통과** |
| T103 capture helper unittest | **13/13 통과** |
| document-links | **1/1 통과** |
| Linux GCC 15.2.0 strict C99 handoff | 통과 |
| Linux Clang 21.1.8 ASan+UBSan handoff | 통과, sanitizer 진단 없음 |
| handoff C coverage | region **88/88**, function **3/3**, line **44/44**, branch **26/26** |
| Doxygen 1.18.0 / Sphinx 9.1.0 strict | **공개 API 74개 통과** |
| Arm GCC 15.3.1 primary Debug/Release | 각각 새 configure/build 성공 |
| Arm SRAM command/read negative tests | 각 구성 **4개 통과** |
| Arm boot link/startup/handoff mutation tests | 각 구성 **3개 통과**, handoff 128bit 변이 포함 |
| `git diff --check base..candidate` | 출력 없음 |

Host 12개는 handoff/runtime, flash guard/command/read/io/ram-validator/layout, platform-contracts, uart-platform, fdcan-platform, core-register-model이다. **전체 160개 실행으로 표시하지 않는다.**

| 새 Arm 산출물 | Debug | Release |
|---|---:|---:|
| boot link 시험 BIN | 5,284B | 3,924B |
| boot SRAM copy | 1,048B | 824B |
| primary 앱 BIN | 51,768B | 39,776B |
| boot link 시험 ELF | 64,936B | 18,536B |
| boot link 시험 MAP | 120,322B | 105,667B |

Release Arm와 Windows Debug/Release host 빌드, API 저장 로그의 warning/error 패턴 검색 결과는 0건이었다. Debug Arm은 빌드 성공·strict compilation·검사 결과를 직접 확인했으나 전체 빌드 로그를 별도 파일로 보존하지 않았다.

Boot link 시험 BIN SHA-256:

```text
Debug   11dc0e1e7f900887767e2328a95e3d260706c49014ea1c23d102293032a87d1d
Release a5fa2b0541dd12a7fed795e96ffe2b20de6851a3f5ce2de4ce909a7170172200
```

SDK checkout도 직접 확인했다. 두 checkout의 porcelain 출력은 비어 있었다.

```text
STM32CubeG4 d11b194a9f05d1b143d154771f3dbc282c8052a5
MCUboot     6d3b3d2c38ab20c242e5b9abb04d050086383eb2
```

## 실제 읽은 파일

아래 19개 파일의 `base..candidate` 변경 전체를 검토했다. 긴 기존 파일은 변경부와 관련 문맥만 읽었다.

```text
.github/workflows/foundation.yml
CMakeLists.txt
docs/api/Doxyfile
docs/api/stm32_core.rst
docs/journal.md                         — 이번 delta만
docs/resume.md
firmware/communicator/stm32/CMakeLists.txt
firmware/communicator/stm32/bootloader/README.md
firmware/communicator/stm32/bootloader/include/canview_boot_handoff.h
firmware/communicator/stm32/bsp/flash_layout.h
firmware/communicator/stm32/platform/stm32g474/boot_handoff.c
firmware/communicator/stm32/tests/register_model.h
tests/hil/fixtures/t103-capture-only.jsonl
tests/ota/stm32_boot_ram_link.c
tests/ota/test_stm32_boot_handoff.c
tests/ota/test_stm32_boot_ram.py
tests/test_t103_capture_helpers.py
tools/build_docs.py
tools/ota/check_stm32_boot_ram.py
```

추가로 읽은 정본·구현 문맥·도구 설정:

```text
AGENTS.md
docs/README.md
docs/tasks/T-107-stm32-mcuboot.md
docs/architecture/README.md
docs/architecture/ota.md                — 제목 검색, §5·§7
docs/runbooks/agent-workflow.md
docs/reviews/README.md                  — 규칙 및 index
docs/reviews/adversarial/TEMPLATE.md
docs/development/windows.md
docs/development/toolchains.md          — 공통/도구 부분
docs/development/foundation.md
CMakePresets.json
cmake/CanviewWarnings.cmake
firmware/communicator/stm32/CMakePresets.json
firmware/communicator/stm32/cmake/arm-none-eabi-gcc.cmake
firmware/communicator/stm32/platform/stm32g474/boot_runtime.c
firmware/communicator/stm32/ld/STM32G474CEUx_PRIMARY.ld
firmware/communicator/stm32/ld/application-sections.ld — 관련 부분
tools/environment/foundation-windows.ps1
tools/foundation-tools.json
tools/toolchain-versions.json            — SDK pin 부분
tools/check_stm32_core.py                — startup/model/constants/검사 부분
tools/ota/validate_stm32_map.py
tests/hil/run.py                         — source digest/identity 부분
C:/cv/STM32CubeG4-1.6.3/Drivers/CMSIS/Core/Include/core_cm4.h
                                         — 관련 mask 및 NVIC 배열
```

적용한 skill은 다음 네 파일을 모두 읽었다. 자동 수정 대신 리뷰 기준으로 사용했다.

```text
C:/Users/digit/.codex/skills/embedded-architecture/SKILL.md
C:/Users/digit/.codex/skills/embedded-cstyle/SKILL.md
C:/Users/digit/.codex/skills/embedded-documentation/SKILL.md
C:/Users/digit/.codex/skills/embedded-isr-design/SKILL.md
```

생성 산출물에서는 host CTest 목록·빌드 로그·API 로그, Arm ELF disassembly·MAP/BIN 크기와 BIN hash를 확인했다. 빌드가 자동 실행한 하위 checker 전체를 별도의 수동 코드 리뷰 완료로 계산하지 않았다.

## 실행 명령 기록

아래는 중복 조회를 합친 실행 명령이다. `Get-Content` 및 `Select-Object -Skip/-First`의 대상은 위 파일 목록과 같다.

기준선·탐색:

```powershell
Get-Location
git status --short
git rev-parse HEAD
git symbolic-ref -q HEAD
git diff --stat 965ada0faa6a40925fc460585481615005fcddf1..HEAD
git diff --name-only 965ada0faa6a40925fc460585481615005fcddf1..HEAD
git diff --no-ext-diff 965ada0faa6a40925fc460585481615005fcddf1..HEAD
git diff --check 965ada0faa6a40925fc460585481615005fcddf1..HEAD
rg --files -g AGENTS.md -g '!build/**'
rg -n 'canview_boot_handoff|boot_branch' firmware tests tools docs/api
```

추가 `rg` 조회는 OTA 제목, compiler/docs 도구 위치, source digest, CMSIS mask/배열, startup/model 상수, 생성 CTest 등록을 대상으로 했다. 존재하지 않는 `setup-host.ps1`, `tests/hil/t103_capture_helpers.py`, Windows glob `tools/host*` 조회는 결과를 얻지 못했고 실제 경로를 다시 확인했다.

Windows 도구 경로:

```text
TOOLS = F:/dev/canview-wt/t104-stm32-uart-control/.tools
CMake/CTest = TOOLS/cmake-4.4.3/bin/cmake-4.4.3-windows-x86_64/bin
Ninja = TOOLS/ninja-1.13.2/bin/ninja.exe
Clang = TOOLS/llvm-23.1.0/clang+llvm-23.1.0-x86_64-pc-windows-msvc/bin
Arm root = C:/Users/digit/AppData/Local/CANView/toolchains/arm-gnu-toolchain-15.3.rel1
```

`vswhere`로 Visual Studio C++ 설치를 찾고 `Launch-VsDevShell.ps1 -Arch amd64 -HostArch amd64 -SkipAutomaticLocation`을 실행한 뒤 위 도구를 PATH에 지정했다.

```powershell
cmake --preset host-debug -DCANVIEW_MCUBOOT_ROOT=C:/cv/mcuboot-2.4.0
cmake --build --preset host-debug --parallel 6
ctest --preset host-debug -N

cmake --preset host-release -DCANVIEW_MCUBOOT_ROOT=C:/cv/mcuboot-2.4.0
cmake --build --preset host-release --parallel 6
```

두 host preset에서 실행한 필터:

```text
^stm32-(boot-(handoff|runtime)|flash-(guard|command|read|io|ram-validator|layout)|core-register-model|fdcan-platform|uart-platform|platform-contracts)$
```

```powershell
ctest --preset host-debug -R <위 필터> --output-on-failure
ctest --preset host-release -R <위 필터> --output-on-failure
ctest --preset host-debug -R '^document-links$' --output-on-failure
python -X utf8 -B -m unittest discover -s tests -p test_t103_capture_helpers.py -v
```

Arm은 STM32 프로젝트 디렉터리에서 환경변수 `CANVIEW_ARM_GNU_ROOT`, `CANVIEW_NINJA_EXECUTABLE`을 위 설치본으로 지정하고 실행했다.

```powershell
cmake --preset primary-debug `
  -B F:/dev/canview-wt/review-t107-handoff-b/build/arm-debug `
  -DSTM32CUBE_G4_ROOT=C:/cv/STM32CubeG4-1.6.3 `
  -DCANVIEW_MCUBOOT_ROOT=C:/cv/mcuboot-2.4.0

cmake --preset primary-release `
  -B F:/dev/canview-wt/review-t107-handoff-b/build/arm-release `
  -DSTM32CUBE_G4_ROOT=C:/cv/STM32CubeG4-1.6.3 `
  -DCANVIEW_MCUBOOT_ROOT=C:/cv/mcuboot-2.4.0

cmake --build F:/dev/canview-wt/review-t107-handoff-b/build/arm-debug --parallel 6
cmake --build F:/dev/canview-wt/review-t107-handoff-b/build/arm-release --parallel 6
```

빌드가 `test_stm32_flash_ram.py`, `test_stm32_boot_ram.py`, core/CMSIS 검사, FDCAN layout 검사, primary ELF/MAP/BIN 검사를 실행했다. 양쪽 boot ELF에 다음 명령도 직접 실행했다.

```text
arm-none-eabi-objdump -d --disassemble=canview_boot_handoff <boot ELF>
arm-none-eabi-objdump -d --disassemble=boot_branch <boot ELF>
```

Linux 보조 검증은 `wsl -d Ubuntu-26.04 -- bash -lc ...`로 동일 worktree에서 실행했다. GCC/Clang 공통 인자:

```text
-std=c99 -Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion
-Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef
-DCANVIEW_STM_REGISTER_TEST=1
-Ifirmware/communicator/stm32/bootloader/include
-Ifirmware/communicator/stm32/tests
-Ifirmware/communicator/stm32/interface
-Ifirmware/communicator/stm32/bsp
-Ishared/interface
tests/ota/test_stm32_boot_handoff.c
firmware/communicator/stm32/platform/stm32g474/boot_handoff.c
```

GCC 출력 `build/handoff-gcc`를 실행했다. Clang에는 다음 인자를 추가하고 coverage를 수집했다.

```bash
-fsanitize=address,undefined -fno-omit-frame-pointer \
-fprofile-instr-generate -fcoverage-mapping -o build/handoff-sanitize

LLVM_PROFILE_FILE=build/reviewer-b-handoff.profraw ./build/handoff-sanitize
llvm-profdata merge -sparse build/reviewer-b-handoff.profraw \
  -o build/reviewer-b-handoff.profdata
llvm-cov report build/handoff-sanitize \
  -instr-profile=build/reviewer-b-handoff.profdata \
  firmware/communicator/stm32/platform/stm32g474/boot_handoff.c
```

API 및 종료 확인:

```powershell
& "$TOOLS/api-venv/Scripts/python.exe" -X utf8 -B tools/build_docs.py `
  --doxygen "$TOOLS/doxygen-1.18.0/bin/doxygen.exe"

git -C C:/cv/STM32CubeG4-1.6.3 rev-parse HEAD
git -C C:/cv/mcuboot-2.4.0 rev-parse HEAD
git -C C:/cv/STM32CubeG4-1.6.3 status --porcelain=v1
git -C C:/cv/mcuboot-2.4.0 status --porcelain=v1

git rev-parse HEAD
git rev-parse 965ada0faa6a40925fc460585481615005fcddf1
git symbolic-ref -q HEAD
git status --porcelain=v1 --untracked-files=all
git diff --exit-code
git diff --cached --exit-code
git check-ignore build/arm-debug/canview-boot-ram-link-test.elf `
  build/host-debug/canview-stm-boot-handoff-tests.exe
```

`Get-ChildItem`으로 산출물 크기, `Get-FileHash -Algorithm SHA256`으로 두 boot BIN hash, `Select-String`으로 저장 로그의 warning/error를 확인했다.

## 미검토·NOT_RUN 및 판정 조건

19파일 **변경부의 미검토 항목은 없다**. 기존 T-107 전체 구현, upstream 전체, 다른 MCU/UI/hardware 영역은 이번 독립 리뷰 범위가 아니다.

다음은 **NOT_RUN**이다.

- candidate 전체 Debug/Release CTest 160개 실행 및 원격 CI/artifact 감사.
- 최종 bootloader에서 `boot_go → policy-v1/floor/activation/confirmation → handoff` 통합 실행.
- 실제 보드의 physical handoff, NMI/reset 전환, watchdog 시간, GPIO/rail 상태.
- torn flash word/page, 실제 ECC·전원 차단·swap/revert qualification.
- signing/provisioning key 작업 및 새 서명 이미지 qualification.

따라서 이번 source delta에는 확인된 수정 요구 finding이 없지만, 통합 승인에는 candidate에 결합된 전체 회귀와 CI 증거가 추가로 필요하다. 최종 caller는 서명과 영속 정책 gate가 연결되기 전 유효 길이로 jump해서는 안 된다.

**T-107 IN_PROGRESS / PR37 Draft 유지. Physical/HIL NOT_RUN. 차량 TX NO-GO.**
````
