# T-107 SRAM Reviewer A 원본

- Agent ID: `01a0b970-a265-7893-8977-1be0ae61b528` (Poincare)
- Coordinator dispatch: 2026-09-19 11:32:41 UTC

## 공통 전달 원문

```text
T107 SRAM link/startup 독립 적대적 source review. 저장소 F:/dev/canview-wt/t007-ota-container. candidate 43b52b85d97a73e914de958cfd8b2e4384afac37 base d20e9e3e374500fb866aa6912ce9916040a98309. 반드시 git cat-file/rev-parse 확인, object-only git show candidate:path/git diff base candidate로 읽고 immutable line 번호 보고. 파일 수정/commit 금지. AGENTS/docs README/resume/T107 상세 및 관련 workflow/skill 직접 읽기. SDK 원본 읽기 가능 C:/cv/STM32CubeG4-1.6.3. Scope 15-file diff: boot64KiB linker, startup_flash_ram C preinit barrier, target-local VTOR, 실제 SDK startup+Flash C 비배포 link test, 실제 ELF/copy/branch validator 및 negative tests, CI artifact additions/doc. 기존 primary linker는 변경하지 않음. main test는 boot_go/handoff/watchdog/policy 없음, final loader가 아니고 physical/HIL NOT_RUN. 실제 clean Arm D/R compile warning0 + link/copy test PASS, hostRelease158 전체 실행중. 범위밖 전체T107 수용/실기/production/merge 승인. 두 reviewer findings 공유하지 않음. 현재 새 SDK loop592bits/preinit32bits, 주소mutation,actualld5negative 시험. 검토 종료raw 한국어 report에는 executionID, UTC 시작/끝, 전문범위, candidate/base/격리/hash 검증, 실제읽은파일/실행명령, P0/P1/P2/P3 finding 정확file:line 실패시나리오/영향/권고, 못읽은범위, physical/HIL NOT_RUN, verdict BLOCK/CONDITIONAL/PASS 포함. 없는 finding 만들지 않지만 검토 못하면 incomplete 명시. 현재 scope에 한정한 완전한 line-level report 반환.
```

## 전문 범위 전달 원문

Reviewer A: embedded runtime/SDK startup/SRAM parity/reset/vector/copy/memory ownership/CPU barrier/stack isolation 전문. boot linker, startup_flash_ram, startup_ram, SDK Reset_Handler, flash_command/read 실제 연결 및 call ownership 우선 검토.

## 진행 확인 메시지

진행 상태를 짧게 알려주세요. 독립 범위의 정확한 line-level 최종 raw report가 다음 critical path입니다. 새 범위 확장 없이 현재 candidate delta에 집중하고, 미검토는 솔직히 명시해 반환해주세요. 상대 reviewer 결과는 공유하지 않습니다.

## Raw report

Verdict: **PASS — 지정된 candidate delta의 Reviewer A source review에 한정.** 신규 finding은 P0/P1/P2/P3 모두 0건이다. 전체 T-107 수용, 실제 부팅, production 또는 merge 승인을 의미하지 않는다.

- executionID: `T107-SRAM-A-43b52b8-20260919T113321Z`
- 기록된 UTC 시작: `2026-09-19T11:33:21Z`
- UTC 검토 종료: `2026-09-19T11:38:22Z`
- 전문 범위: embedded runtime, SDK startup, SRAM parity/reset, vector/copy, 메모리 소유권, CPU barrier, stack 격리.
- 저장소: `F:/dev/canview-wt/t007-ota-container`
- candidate: `43b52b85d97a73e914de958cfd8b2e4384afac37`
- base: `d20e9e3e374500fb866aa6912ce9916040a98309`
- 격리: **commit object-only**. 저장소 내용은 해당 hash의 `git show` 및 `git diff base candidate`로 읽었다. 다른 reviewer의 이번 결과를 조회하거나 공유하지 않았다. 파일 수정·commit·빌드 산출물 생성 없음.

**기준선·hash 검증**

두 hash 모두 `git cat-file -t`에서 `commit`, `git cat-file -e '<hash>^{commit}'`에서 exit 0이며 `rev-parse --verify` 결과가 요청 hash와 일치했다. commit 원문에서 Git object header를 포함한 SHA-1을 재계산해 두 hash를 확인했다. 변경된 15개 파일도 `git show candidate:path`의 bytes에서 blob SHA-1을 재계산하고 `git rev-parse candidate:path`와 **15/15 일치**했다.

base의 candidate ancestry 검사는 exit 0. 시작·종료 저장소 status 출력은 비어 있었고, 종료 HEAD는 candidate와 일치했다. `git diff --check base candidate`는 exit 0이었다. 기존 `STM32G474CEUx_PRIMARY.ld`와 `application-sections.ld`는 두 commit 사이 diff가 없음을 별도로 확인했다.

이하 모든 저장소 `file:line`은 **candidate object의 1-based line**이다. 경로 약어 `S/`는 `firmware/communicator/stm32/`를 의미한다.

**변경 15파일의 line-level 검토**

| 실제 읽은 파일·위치 | 검토 결과 |
|---|---|
| `S/ld/STM32G474CEUx_BOOT.ld:5`, `:16`, `:54`, `:58`, `:75`, `:94` | Flash 64KiB, SRAM 96KiB, stack 8KiB, dummy 4B와 copy/BSS 경계를 대조했다. command→read→data의 VMA/LMA 연속성·word 정렬, CCM 금지, static/stack 분리 assertion을 확인했다. |
| `S/platform/stm32g474/startup_flash_ram.c:8`, `:11`, `:17` | SDK copy/BSS 이후 실행하는 preinit 함수와 DSB→ISB 순서를 확인했다. `used`와 linker의 `.preinit_array` `KEEP`가 함께 보존한다. |
| `S/CMakeLists.txt:167`, `:253`, `:262`, `:274`, `:283`, `:289` | primary VTOR 정의의 target-local 전환, boot VTOR offset 0, 실제 SDK startup/SystemInit 및 Flash archive 연결을 확인했다. link-check와 기존 앱 검사 사이 의존성도 확인했다. |
| `tests/ota/stm32_boot_ram_link.c:10`, `:14`, `:18` | read/command 연결 유지용 main이다. PROGRAM 인자는 all-FF로 실제 primitive에서 거절된다. 이 파일은 실제 Flash 성공 실행이나 final loader의 근거가 아니다. |
| `tools/ota/check_stm32_boot_ram.py:17`, `:46`, `:57`, `:69`, `:81`, `:95` | copy/BSS/stack symbol, 고정 SDK literal 위치·값, copy/zero 명령열, BL 목적지, preinit 포인터, SystemInit VTOR 명령, ELF/BIN 및 barrier 검사를 추적했다. |
| `tests/ota/test_stm32_boot_ram.py:18`, `:33`, `:39`, `:48` | 주소·크기 변이, section 누락, `(46+28)×8=592` startup/VTOR bit, preinit 포인터 32bit와 실제 linker 양성 1개·음성 5개 시험의 입력·실패 판정을 읽었다. |
| `tools/ota/check_stm32_flash_ram.py:18`, `:25`, `:32`, `:37` | linked VMA 기준으로 함수 extent와 직접 분기를 검사하는 변경을 확인했다. 기존 object relocation 검사와 간접 branch 거절은 유지된다. |
| `tests/ota/test_stm32_flash_ram.py:20` | linked section의 SRAM 범위, 함수 시작·끝, section 밖 branch/compare-branch 거절 시험을 확인했다. |
| `tools/ota/validate_stm32_map.py:51`, `:64`, `:76`, `:85`, `:95` | 기본 primary 계약을 유지하면서 boot 전용 origin/크기/CCM 금지를 주입한다. vector/MSP/reset 및 ELF/BIN load byte·LMA 중첩 검사를 확인했다. |
| `.github/workflows/foundation.yml:366`, `:401`, `:426`, `:478` | 비배포 D/R ELF/MAP/BIN 6개 추가, 존재·크기·digest 처리, source revision 결합과 기존 업로드 wildcard의 포함 여부를 확인했다. |
| `S/bootloader/README.md:220`, `:224`, `:234`, `:240` | link 시험 범위와 final loader·전체 stack·physical/HIL 미완료를 구분한다. 현재 변경을 실제 부팅 완료로 승격하지 않는다. |
| `docs/resume.md:72` | SRAM link 시험 추가와 다음 watchdog/handoff·정책 작업, physical NOT_RUN 경계를 확인했다. |
| `docs/journal.md:3` | 이번 변경의 기록을 읽었다. 빌드 크기·warning0·변이 시험 결과는 작성자 기록이며 이번 reviewer의 재실행 결과로 취급하지 않았다. |
| `tests/hil/fixtures/t103-capture-only.jsonl:1` | 5개 record의 firmware identity 변경을 확인했다. no-TX와 합성 fixture 의미는 유지된다. |
| `tests/test_t103_capture_helpers.py:26` | 기대 firmware identity가 fixture와 일치한다. source-tree digest 자체는 이번 리뷰에서 독립 재계산하지 않았다. |

**Runtime·SDK 연결의 추가 검토**

- `S/platform/stm32g474/startup_ram.c:10`, `:21`: 첫 SRAM word의 이중 write/DSB, 나머지 세 cut read, naked wrapper의 SDK tail branch를 확인했다. 첫 접근 전에 C prologue로 stack을 사용하는 구조가 아니다.
- 실제 SDK `startup_stm32g474xx.s:61`, `:68`, `:85`, `:99`: MSP 설정→SystemInit→word copy→BSS zero→`__libc_init_array`→main 순서를 직접 대조했다. vector table도 `:133`부터 `:253`까지 확인했다.
- 실제 SDK `system_stm32g4xx.c:112`, `:181`: target별 macro가 Flash VTOR 값을 결정하며 SystemInit은 이번 SRAM code를 조기에 호출하지 않는다.
- 실제 SDK `Core/Include/cmsis_gcc.h:933`, `:944`: ISB/DSB가 volatile assembly와 `memory` clobber를 사용함을 확인했다.
- `S/platform/stm32g474/flash_command.c:49`, `:62`, `:145`: busy 구간의 함수·literal은 command SRAM section에 배치된다. 임시 vector는 호출 stack이 소유하고, 정상/오류 반환 시 VTOR와 PRIMASK를 복원한다. timeout은 Flash 호출자로 반환하지 않는 SRAM reset/fail-stop 경로다.
- `S/platform/stm32g474/flash_read.c:37`, `:58`, `:74`, `:147`: NMI context와 staging의 호출 수명, SRAM 함수 주소 조건, VTOR 전환과 복원, 성공 시에만 최종 destination으로 복사하는 순서를 확인했다.
- `S/platform/stm32g474/flash_guard.c:25`, `flash_io.c:36`, `:51`, `:72` 및 두 primitive interface header를 읽고 단일 privileged MSP owner, DMA/다른 Flash 사용자 정지, copy 완료 후 호출이라는 전제를 확인했다. 이번 main이 MCUboot IO 전체 경로를 실행한다고 해석하지 않았다.
- `S/platform/stm32g474/syscalls.c`도 읽었다. 새 boot 시험의 연결을 위해 재사용한 기존 syscall stub이며 heap/서비스 구현을 추가하지 않는다.

SDK root는 `d11b194a9f05d1b143d154771f3dbc282c8052a5`, CMSIS device submodule은 `25664ddc3a7624ae9627ae8c4c672073dc5b2539`였으며 관련 status는 clean이었다. root hash는 candidate의 `tools/toolchain-versions.json` pin과 일치했다.

읽은 SDK 파일의 SHA-256:

```text
startup_stm32g474xx.s
9145d66f4689c8c98f4bcf22b1804061b24d340dd6b4cb822f1f6469bf3c2aa9

system_stm32g4xx.c
7727a9f71724f56d6835f362259f648ea8d578891eba4489d41db54314b0064d
```

**지침·실행 명령과 검증 결과**

직접 읽은 지침은 candidate의 `AGENTS.md`, `docs/README.md`, `docs/resume.md`, `docs/tasks/T-107-stm32-mcuboot.md`, `docs/runbooks/agent-workflow.md`, `docs/reviews/README.md`, `docs/architecture/README.md`와 OTA §5다. 적용 skill은 `embedded-cstyle`, `embedded-architecture`, `embedded-driver-design`, `embedded-isr-design`이며 각 `SKILL.md`를 직접 읽었다. 자동 수정 지침은 사용자의 수정 금지에 따라 적용하지 않았다.

실행한 주요 명령은 다음과 같다. `C`와 `B`는 위에 기록한 전체 candidate/base hash다.

```text
git rev-parse --show-toplevel
git rev-parse --show-object-format
git rev-parse --verify C^{commit}
git rev-parse --verify B^{commit}
git cat-file -t C / B
git cat-file -e C^{commit} / B^{commit}
git diff --stat B C
git diff --find-renames --no-ext-diff B C
git show C:<검토 파일>
git rev-parse C:<변경 파일>
git merge-base --is-ancestor B C
git diff --check B C
git diff --exit-code B C -- <기존 primary linker 두 파일>
git status --porcelain=v1
git -C <SDK> rev-parse HEAD
git -C <SDK> submodule status Drivers/CMSIS/Device/ST/STM32G4xx
Get-Content <skill 및 허용된 SDK 원본>
Get-FileHash -Algorithm SHA256 <SDK startup/SystemInit>
python -B -c <object hash 재계산 및 메모리 전용 validator 시험>
```

마지막 Python 실행은 candidate의 checker source를 `git show`로 읽어 메모리 module로 실행하고, candidate 시험 클래스에서 파일 접근 없는 다음 세 시험을 선택했다.

```text
test_linked_bounds   PASS
test_read_profile   PASS
test_mutations      PASS
총 3/3, 실행 시간 0.002초
```

이는 **합성 objdump 입력에 대한 검사기 회귀시험**이다. 실제 Arm object/linker/ELF 실행 시험으로 계산하지 않는다.

**Finding 및 미검토 한계**

P0: 0건. P1: 0건. P2: 0건. P3: 0건. 따라서 실패 시나리오·영향·수정 권고를 붙일 확정 finding은 없다.

변경된 15파일의 delta 중 읽지 못한 hunk는 없다. 다만 실제 D/R ELF·MAP·BIN, clean compile 로그와 warning0, 592bit/32bit 변이 실행 결과, 실제 linker 음성 5건, CI artifact bytes/hash는 이번 object-only 실행에서 독립 확인하지 않았다. 사용자 제공 결과와 문서 기록으로만 구분했다. 진행 중이라고 전달된 host Release 158개 결과도 확인하지 않았다.

전체 MCUboot call-chain stack, newlib의 실제 링크된 preinit 실행 경로, 하드웨어 reset/parity/ECC/NMI timing, Flash stall 및 전원 차단 복구는 이번 source 판정으로 닫지 않는다. **Physical/HIL: NOT_RUN.** 최종 boot_go/watchdog/policy/handoff, 전체 T-107 수용·production·merge 승인은 범위 밖이다.
