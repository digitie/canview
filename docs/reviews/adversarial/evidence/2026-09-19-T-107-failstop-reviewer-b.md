# T-107 fail-stop Reviewer B 원본

## 공통 전달 manifest

````text
T107-Failstop 독립 적대적 리뷰. candidate 6b394ac7effbdc14e901b363a1594d4bf89b34fa, base e9f474b133912814aad255add3452aa0eb2fd44b. Task T107 IN_PROGRESS, PR37 Draft. 전체 Task merge 승인 아닌14file delta: git diff --find-renames base candidate 전체와 관련 원본을 line-level 확인. bootloader/fail_stop.c/h newlibGNU ABI wrappers→기존 FIH_PANIC, Releaseassert조건유지, hostlibc불변. 명시적trust일때기존 비배포 canview-boot-ram-link-test 에 boot_go/crypto/identity 강제참조전체link, two wrapper 분기 검사/heapstdio금지symbol, missingwrap 실제 link negative. finalentry/policy/recovery는 미완료이며 비배포 시험인 점을 scope문서가밝힘. physical/HILNOT_RUN, production/vehicleTXNO-GO. 새정책이나권한의완료를추정하지말 것.
AGENTS/docsREADME/resume/T107, 필요한workflow/skills읽기. candidate HEAD와clean 시작/종료확인. 자신의detachedtree에만 ignored build evidence, trackedsourceedit/commit 금지. 다른reviewerfinding보지말것. 검토 전후실제읽은파일/명령, 전문영역, executionID/UTC시작종료,candidate/base,격리, P0P1P2P3정확한file:line/failure/impact/recommendation, 미검토범위, physicalNOT_RUN, BLOCK/CONDITIONAL/PASS 한국어 raw report 반환. 불가능한기능을구현됐다고주장하는지와 실제scope구별, 범위를넓혀기존명시적미구현T107자체를새delta결함으로분류하지말 것.
환경: CMake F:/dev/canview-wt/t104-stm32-uart-control/.tools/cmake-4.4.3/bin/cmake-4.4.3-windows-x86_64/bin ; Clang .../.tools/llvm-23.1.0/clang+llvm-23.1.0-x86_64-pc-windows-msvc/bin; Ninja .../.tools/ninja-1.13.2/bin/ninja.exe; hostbuild및CTest전 반드시vswhere→Launch-VsDevShell -Arch amd64 -HostArch amd64 -SkipAutomaticLocation. SDK C:/cv/mcuboot-2.4.0 pin6d3b3d2 clean, STM C:/cv/STM32CubeG4-1.6.3, Arm C:/Users/digit/AppData/Local/CANView/toolchains/arm-gnu-toolchain-15.3.rel1. Root configure -DCANVIEW_MCUBOOT_ROOT=C:/cv/mcuboot-2.4.0. Arm full link explicit publicDER=F:/dev/canview-wt/t007-ota-container/build/t107-trust-test-public.der (testonlyPUBLIC), epoch0, manifestkeyID4294967295, STMABI2. powershell -D전체인자quote. 독립actualArm Debug/Release와hostfocused검증권장, 전체161는 coordinator진행중. 상세logs main ignored build/t107-failstop-* (source는자기기준선).
로컬관찰: ArmD/R fullclosure ELF/BIN37184/29116B,ram12264/12040B, warning0,4test+missingwrap2negativePASS; host새3/3,GCCstrictC99,ClangASANUBSAN; failstopCfunction2/2region4/4line10/10,branch0. wholehost161회귀진행중.
````

## 역할과 격리

Reviewer B — build/linker/toolchain isolation, validation mutation gaps, resource/CI evidence integrity/security.
Execution ID: `T107-FAILSTOP-B-6b394ac-20260919`.
Worktree: `F:/dev/canview-wt/review-t107-failstop-b`.

## 원본 결과

````markdown
# T107-Failstop Reviewer B 독립 raw report

- execution ID: `T107-FAILSTOP-B-6b394ac-20260919`
- 전문 영역: build/linker/toolchain 격리, mutation 검증 누락, 자원·CI evidence 무결성/보안
- UTC 시작: `2026-09-19T13:18:11.2268098Z` — 지정 detached worktree 검토 시작
- UTC 종료: `2026-09-19T13:27:10.2034891Z` — 보고서 생성 뒤 candidate/clean·로그 최종 확인
- candidate: `6b394ac7effbdc14e901b363a1594d4bf89b34fa`
- base: `e9f474b133912814aad255add3452aa0eb2fd44b`
- verdict: **CONDITIONAL**, 이번 14-file delta에 한정
- finding: P0 0 / P1 0 / P2 2 / P3 0. 두 P2는 OPEN이며 수정 또는 명시적 disposition이 필요하다.
- T107 `IN_PROGRESS`, PR37 Draft 유지. 전체 task·merge·production·vehicle TX 승인 아님.
- physical/HIL: **NOT_RUN**. production 및 vehicle TX: **NO-GO**.

## 격리와 독립성

`F:/dev/canview-wt/review-t107-failstop-b`의 기존 detached worktree를 사용했다.
시작/종료 `git rev-parse HEAD`는 모두 candidate이고 `git status --porcelain=v1`은
비어 있다. 종료 tracked/staged diff도 비어 있고 `git symbolic-ref -q HEAD`는1이다.
tracked source 편집, stage, commit, push, PR 변경은 하지 않았다.

처음 환경 확인은 요청에 제공된 cwd `F:/dev/canview`에서 수행했다. 그 checkout은
`10716b1` 및 기존 untracked 항목이 있어 리뷰 기준선으로 사용하지 않았고 변경하지 않았다.
이후 읽기·빌드·변이는 모두 지정 candidate worktree 기준이다.

새 파일과 build/evidence는 이 worktree의 ignored `build/t107-review-b/`에만 만들었다.
시험의 TEMP/TMP도 그 아래로 지정했다. 변이는 생성 ELF 복사본에만 적용했다.
다른 reviewer의 이번 failstop 원본 report/finding과 coordinator의 상세 로그는 읽지 않았다.
필수 task/resume 및 review archive index에 포함된 과거 리뷰 요약은 읽었으며,
과거 reviewer 원본 링크는 열지 않았다.

MCUboot SDK는 시작/종료 clean이고 실제 HEAD는
`6d3b3d2c38ab20c242e5b9abb04d050086383eb2`이다. 외부 SDK를 수정하지 않았다.
CodeGraph는 사용하지 않았으며 diff, 원본, compile database, nm/objdump와 실행 시험으로 추적했다.

## B-FAILSTOP-01 — P2 / OPEN

위치: `tools/ota/check_stm32_boot_ram.py:118` 및 `:119`.
관련 시험: `tests/ota/test_stm32_boot_ram.py:34`.

실패: wrapper의 `b*` 명령 하나가 panic을 가리키는지만 세므로 `pop {..., pc}` 같은
PC 변경/반환을 보지 못한다. 실제 Debug ELF 복사본의 `__wrap_abort`에서 두 번째
halfword `0xaf00`을 `0xbd80`으로 바꿨다. ELF와 BIN은 objcopy로 서로 일치시켰고
MAP/section/address는 그대로 보존했다. 변경 뒤 명령은 다음과 같다.

```text
080059d6 <__wrap_abort>:
 80059d6: b580       push {r7, lr}
 80059d8: bd80       pop  {r7, pc}
 80059da: f7ff f84b  bl   8004a74 <fih_panic_loop>
```

`push`로 저장한 반환 주소를 `pop`으로 PC에 복원하므로 BL은 도달하지 않는다.
그런데 `boot.inspect(mutant, compiler, True)`는 실제 ELF/BIN 전체 검사에서 PASS했다.
이는 MCU 실측이 아니라 실제 Arm 명령열과 검사기의 false PASS 재현이다.

영향: 원본 C와 현재 생성 ELF의 panic 호출은 정상이다. 그러나 이번 delta가 추가한
fail-stop 검증은 panic 우회·반환 회귀를 잡는다고 신뢰할 수 없다. 현재 비배포 경계가
무너지거나 candidate가 실제로 반환한다는 finding은 아니므로 P2로 분류한다.

권고: 고정 toolchain의 정상 wrapper 명령열을 엄격히 검증하거나, 명령 파싱으로 모든
PC 변경·반환·조건부 우회와 도달 가능성을 검사한다. `pop {pc}`, `ldr/mov pc`, 조건부
panic, 간접 분기와 panic 앞 조기 반환을 실제 Arm object/ELF negative에 추가한다.
수정 후 이 ELF 변이가 거절되고 정상 Debug/Release가 통과하는지 확인한다.

재현: `python -B -X utf8 build/t107-review-b/mutation_probe.py`.
증거: `mutation-probe.log`, `mutant-return.elf/.bin/.map`.
mutant ELF SHA256: `7d29429768e5334ac7c076a51983f4c0aede5c48d5bb8cb88e81d5983d9f6ef2`.

## B-FAILSTOP-02 — P2 / OPEN

위치: `firmware/communicator/stm32/CMakeLists.txt:266` 및 `:274`.
관련 위치: `.github/workflows/foundation.yml:311`, `:313`, `:433`;
`tests/ota/test_stm32_boot_ram.py:19`, `:119`.

실패: 새 full-link/`--full-boot` 검증은 `canview_boot_identity` target이 있어야 켜진다.
그 target은 공개 DER/epoch/manifest ID/STM ABI 네 입력을 모두 명시해야 생성된다.
현재 target CI primary Debug/Release configure는 MCUboot root만 전달하고 네 trust
입력을 공급하지 않는다. 이번 workflow delta는 source provenance 목록만 추가했다.

동일하게 trust를 생략한 깨끗한 Release configure/build를 독립 실행했다.
`Boot BSP trust NOT_CONFIGURED`, `OK (skipped=1)`로 통과했으며 image는3924B이고,
`boot_go`, `fih_panic_loop`, 두 fail-stop wrapper symbol이 모두 없다.
`test_linker_rejections`의 새 missing-wrap 두 negative도 `if self.full_boot` 때문에 실행되지 않는다.

영향: CI가 성공해도 이번 새 Arm 전체 링크, panic branch, forbidden symbol,
missing-wrap 회귀를 실행한 근거가 될 수 없다. source hash 기록은 실행 증거가 아니다.
trust 미설정의 기존 SRAM-only 동작 자체는 문서대로 정상이고, 로컬 explicit-trust
검증은 통과했다. 문제는 새 기능을 자동 회귀에서 지속적으로 검사하지 않는다는 점이다.

권고: CI 전용 공개 시험 DER와 네 명시적 시험 상수를 생성/공급하는 별도 비배포
Debug/Release 검증을 추가한다. production 기본키를 넣지 말고 full-boot 활성·실행 여부를
로그/manifest에서 구분한다. 중간 단계에서 연기한다면 담당자·task·CI 연결 gate·목표 시점을
명시하고 현재 CI artifact는 SRAM-only라고 유지한다.

증거: `arm-no-trust-configure.log`, `arm-no-trust-build.log`, 해당 ELF/nm 확인.
새로운 최종 entry/policy/recovery 미구현을 결함으로 분류한 것이 아니다.

## 독립 실행 결과

| 검증 | 결과 |
|---|---|
| actual Arm Debug + explicit test trust | full link, boot SRAM4시험, missing-wrap2 negative PASS |
| actual Arm Release + explicit test trust | full link, boot SRAM4시험, missing-wrap2 negative PASS |
| Arm command/read object 및 실제 assembler negative | 각 구성4시험 PASS |
| host Clang Debug focused | fail-stop/runtime/handoff3/3 PASS |
| host Clang Release focused | fail-stop/runtime/handoff3/3 PASS |
| host MCUboot 모형 | Debug/Release 컴파일·링크 PASS; 전체 모형 실행은 NOT_RUN |
| 합성 T103 fixture/helper | 13/13 PASS, source digest 일치 |
| trust 없는 Arm Release | SRAM-only PASS, full-boot skip 재현 |
| 실제 ELF 조기 반환 변이 | false PASS 재현 → B-FAILSTOP-01 |
| 정상 build/configure | compiler/linker/CMake warning0; 미해결 link symbol0 |
| `git diff --check base candidate` | PASS |

도구: CMake4.4.3, Ninja1.13.2, Clang23.1.0, Arm GNU15.3.1,
Python3.14.3, CubeG4 `C:/cv/STM32CubeG4-1.6.3`, 고정 MCUboot SDK.
모든 host configure/build/CTest는 `vswhere` 조회 후
`Launch-VsDevShell.ps1 -Arch amd64 -HostArch amd64 -SkipAutomaticLocation`을 실행한
동일 PowerShell process에서 수행했다.

명시적 Arm trust 입력은 사용자 지정 test-only PUBLIC DER
`F:/dev/canview-wt/t007-ota-container/build/t107-trust-test-public.der`, epoch0,
manifest key ID4294967295, STM ABI2다. 제조 승인이나 production trust로 분류하지 않았다.

| 자원 | Debug | Release |
|---|---:|---:|
| 실제 BIN/Flash load span | 37100B | 29032B |
| linker 보고 RAM(8KiB reserve 포함) | 12264B | 12040B |
| SRAM code/data copy | 1076B | 852B |
| fail-stop assert/abort 개별 stack | 24B/8B | 8B/8B |

64KiB image 제한을 통과한다. 실제 전체 boot call-chain stack/고수위는 측정하지 않았다.
사용자 제공37184/29116B와 이 isolated build의 수치는 다르므로 동일 artifact라고
표시하지 않는다. 바이너리에 build별 절대 `__FILE__` 경로가 포함됨을 확인했다.
차이의 전 바이트 원인이나 coordinator artifact hash는 독립 대조하지 않았다.

Debug BIN SHA256: `b6c886d57b79a539087d2bef2cf3f4ac2788643856889b0da3c4bde9b65410c1`.
Release BIN SHA256: `715a334faa0733aaf4c7c82de09783af89ece839155abce8e005c28cb0c31785`.

## 코드·scope 판정

`fail_stop.c/h`의 인자는 newlib `assert.h`/`stdlib.h` ABI와 일치한다. 두 함수는
인자를 역참조하지 않고 기존 FIH_PANIC으로 들어간다. `int line`은 GNU/newlib ABI
보존에 필요한 것으로 embedded-cstyle의 일반 정수 규칙 위반 finding을 만들지 않았다.
embedded-architecture 스킬로 target/host 링크 경계를 점검했으며 구조 변경은 하지 않았다.

Release compile database의 bootutil에는 `-DNDEBUG` 뒤 `-UNDEBUG`가 있다.
FIH MEDIUM/GLOBAL_FAIL은 유지한다. 정상 ELF에서 두 wrapper의 panic BL과
boot_go/identity/key/crypto closure를 확인했으며 현재 출력·heap 유입은 발견하지 않았다.
host 모델 link에는 fail-stop library나 --wrap이 없고, 원래 host panic abort 경로를 유지한다.

검사기 입력의 puts/fputs/putchar 및 일부 IO symbol 주입은 허용됨도 확인했다.
이는 검사기가 모든 libc 출력/IO 이름을 망라하지 않는다는 제한이며, 실제 해당 libc
closure를 넣은 ELF까지 재현하지 않아 별도 확정 finding으로 중복 집계하지 않았다.

README234–257, resume74–81, journal의 이번 추가분은 비배포 image와 실제 boot_go
실행/서명 성공 뒤 handoff/T205 정책을 구분한다. 구현 불가능한 기능이 완료됐다고
주장하거나 새 차량 TX·복구 권한이 생겼다고 주장하는 내용은 이번 delta에서 찾지 못했다.
기존 명시적 미완료 final entry/policy/recovery를 이번 delta 결함으로 확대하지 않았다.

## 실제 읽은 파일

14개 파일 전체 `git diff --find-renames base candidate`를 읽었다. 변경 hunk와 관련
원본 문맥을 line-level로 확인했으며 journal은 이번 추가22행과 인접 문맥만 읽었다.

1. `.github/workflows/foundation.yml` — diff, host/target configure, provenance/artifact 문맥
2. `CMakeLists.txt` — diff, 공용 옵션, MCUboot/host 시험 target 문맥
3. `docs/journal.md` — 이번 delta
4. `docs/resume.md` — 전체
5. `firmware/communicator/stm32/CMakeLists.txt` — diff 및 boot target200–310 중심
6. `firmware/communicator/stm32/bootloader/CMakeLists.txt` — 전체
7. `firmware/communicator/stm32/bootloader/README.md` — 전체
8. `firmware/communicator/stm32/bootloader/fail_stop.c` — 전체
9. `firmware/communicator/stm32/bootloader/fail_stop.h` — 전체
10. `tests/hil/fixtures/t103-capture-only.jsonl` — 전체 old/new5행
11. `tests/ota/test_stm32_boot_fail_stop.c` — 전체
12. `tests/ota/test_stm32_boot_ram.py` — 전체
13. `tests/test_t103_capture_helpers.py` — 전체
14. `tools/ota/check_stm32_boot_ram.py` — 전체

추가 읽기: `AGENTS.md`, `docs/README.md`, T107 상세 task, `agent-workflow.md`,
`development/windows.md`, `reviews/README.md`, `architecture/README.md`,
`architecture/ota.md` §5; embedded-cstyle/embedded-architecture SKILL.md 전체.

관련 원본: root/STM32 `CMakePresets.json`, `cmake/CanviewWarnings.cmake`,
STM32 `cmake/arm-none-eabi-gcc.cmake`, `ld/STM32G474CEUx_BOOT.ld`,
`platform/stm32g474/syscalls.c`, bootloader `trust.cmake`, `mcuboot_compat.cmake`,
`include/mcuboot_config/mcuboot_config.h`, `tests/ota/stm32_boot_ram_link.c`,
`tests/ota/mcuboot_host_fih.c`; `tools/ota/validate_stm32_map.py` ELF/BIN 검증 관련 검색.
SDK `fault_injection_hardening.c:59–84`, 해당 header의 profile/panic 선언,
설치 newlib `assert.h` 전체와 `stdlib.h` abort 선언을 확인했다.

생성물: own build의 compile_commands.json, build.ninja, ELF/MAP/BIN,
fail_stop.c.su, configure/build/CTest 로그, 변이 ELF disassembly 및 SDK git 상태.

## 실제 실행 명령과 재현 위치

초기/최종: `git rev-parse HEAD`, `git status --porcelain=v1`,
`git symbolic-ref -q HEAD`, `git diff --name-only`, `git diff --cached --name-only`,
`git check-ignore build/t107-review-b/...`, SDK `rev-parse HEAD`/`status`, UTC 조회.
초기 환경 위치 확인에 `git worktree list`, 파일 탐색에 `rg --files`/`rg -n`,
문서/원본 읽기에 `Get-Content`와 line-number 출력도 사용했다.

범위 검토 명령:

```powershell
git diff --stat e9f474b133912814aad255add3452aa0eb2fd44b 6b394ac7effbdc14e901b363a1594d4bf89b34fa
git diff --find-renames e9f474b133912814aad255add3452aa0eb2fd44b 6b394ac7effbdc14e901b363a1594d4bf89b34fa
git diff --check e9f474b133912814aad255add3452aa0eb2fd44b 6b394ac7effbdc14e901b363a1594d4bf89b34fa
& ./build/t107-review-b/run.ps1 -Kind arm
& ./build/t107-review-b/run.ps1 -Kind host
python -B -X utf8 build/t107-review-b/mutation_probe.py
python -B -m unittest discover -s tests -p test_t103_capture_helpers.py -v
```

`run.ps1`에 실제 전체 configure/build/CTest 인자와 VS 환경 준비를 보존했다.
no-trust 재현은 같은 Arm configure에서 네 CANVIEW_BOOT_* 인자를 생략하고
별도 `build/t107-review-b/arm-no-trust`에서 `canview_boot_ram_link_check`를 빌드했다.
추가로 `arm-none-eabi-objdump -d --disassemble=...`, `nm --defined-only`,
`nm --undefined-only`, `size -A`, `strings`, `Get-FileHash -Algorithm SHA256`,
build/configure warning/error 검색을 실행했다. 실제 SDK/MCU 도구 실행이며 mock 성공으로
대체하지 않았다. 문서 보고를 위한 ignored 파일 작성은 apply_patch만 사용했다.

## 미검토·미실행과 조건

coordinator의 전체 host161 회귀, GCC host strict C99 실행, ASan/UBSan,
coverage 재측정, GitHub candidate CI 실제 run/artifact 감사는 이번 reviewer가 재실행하지 않았다.
사용자 제공 성공 관찰을 독립 PASS로 승격하지 않는다. 정상 앱 전체 target build도
이번에는 수행하지 않고 지정 비배포 boot-link target과 host 집중 범위만 빌드했다.

실제 MCU 부팅, IWDG reset, ECC/NMI/stall, torn Flash, 전원·HIL,
전체 MCUboot stack/시간, production provisioning, 최종 entry/영속 정책/recovery,
차량 기능은 검증/승인하지 않았다. physical/HIL NOT_RUN, production/vehicle TX NO-GO다.

이번 delta의 원본 fail-stop 구현에 대한 P0/P1 결함은 발견하지 못했다.
검사기 false PASS와 CI 미실행 경로 두 P2를 수정하거나 workflow에 맞게 명시적으로
disposition해야 하므로 verdict는 CONDITIONAL이다. 전체 T107 완료·PR37 merge 승인은 아니다.
````
