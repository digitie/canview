# T-107 ECC read Reviewer B 원본

- 실제 subagent execution ID: `01a0b91a-ae86-7be3-afcd-ada989ce98cc`
- Coordinator 시작: `2026-09-19 09:58:48 UTC`
- 격리: commit object-only
- 원문을 요약하거나 finding 심각도를 바꾸지 않고 아래에 보존한다.

## 전달 공통 manifest

```text
CANView T-107 guarded read 중간 구현 단위 독립 적대적 리뷰. Repo F:/dev/canview-wt/t007-ota-container. Candidate 481a80575c2f2a285f2e09353f572c71ee4218b5; base f3283fbfb1efce4dc46d017e51156fc3f289cb6a. Scope: git diff base candidate의15파일(guarded Flash read C/header, register model/test, SRAM object checker/CMake, 근거 문서). 전체 T107/PR37 merge review가 아니며 Flash backend/최종 SRAM linker/copy/boot executable/BSP identity/T205 및 physical/HIL은 범위 밖·미완료로 명시된다. 그 미완료만을 새 source 결함으로 중복 보고하지 말고, 구현 자체와 나중 연결 시 숨은 가정/실패를 공격하라. 이전 reviewer 면제는 적용하지 않음.
읽기 전용 COMMIT OBJECT-ONLY 격리: git -C <repo> cat-file -e '<candidate>^{commit}', rev-parse candidate, diff --find-renames base candidate, show candidate:path로만 source/document 읽어라. 이동 branch/worktree 파일 내용을 기준선으로 읽지 마라. 컴파일을 꼭 해야 하면 별도 detached worktree를 만들어 HEAD/clean 시작끝 확인. 변경/commit/push/PR/다른 reviewer 결과 읽기 금지. AGENTS.md, docs/README.md, docs/resume.md, docs/tasks/T-107-stm32-mcuboot.md, docs/runbooks/agent-workflow.md 및 필요한 embedded skills를 따라라. 관련상세설계 docs/architecture/ota.md §5/7.1, bootloader/README.md. 주 변경 flash_read.c는187줄이다. 과거journal/review전체 탐색 대신 diff/new source를 우선 검토.
로컬 근거(직접 실행 증거로 바꾸지 말 것): Host D/R156/156, focused2/2, Arm4개 ELF/MAP/BIN warning0, read RAM528/436B 외부reloc0, ASan/UBSan, 새 C host모형 coverage100%. 이것만으로 실제MMIO/NMItiming/HIL이나 전체boot안전을 주장하지 않는다.
최종 응답을 그대로 raw evidence에 보존한다. 한국어 보고서: execution ID(모르면 제공한 review ID), UTC 시작/종료, 전문영역, 전달manifest요약+실제candidate/base확인, 격리, 실제 읽은 모든파일/명령, P0/P1/P2/P3 finding(정확한 file:line, 실패시나리오, 영향, 권고), finding없어도 공격한시나리오, 미검토범위, physical/HIL NOT_RUN, source단위 BLOCK/CONDITIONAL/PASS verdict. incomplete면읽지않은부분 명시하고BLOCK. 무한탐색하지말고 제한된187줄runtime와delta를 끝까지 읽고 보고하라.
```

## 역할 추가 지시

Review ID T107-ECC-B-20260919-01. Reviewer B: CMSIS/SDK 상수, CMake/build, SRAM checker의 false PASS, negative/mutation test의 독립성, coverage/evidence정합성 전문. 우선 CMakeLists.txt, STM32 CMakeLists, register_model.h, tools/ota/check_stm32_flash_ram.py 및 tests/ota/test_stm32_flash_ram.py/test_stm32_flash_read.c/guard.c. runtime입력과검증누락도검토.

## 원본 결과

Source 단위 판정: **CONDITIONAL**. P2 finding 2건을 확인했다. P0/P1/P3 finding은 없다. 지정된 15파일의 delta와 `flash_read.c` 187줄 전체 검토를 완료했다. 전체 T-107 또는 PR37의 merge·부트 안전 판정은 아니다.

**실행 식별·기준선**

- Review/execution 식별자: `T107-ECC-B-20260919-01` — 별도 execution ID는 제공되지 않아 지정 review ID 사용.
- UTC 시작: `2026-09-19 09:59:06 UTC`
- UTC 종료: `2026-09-19 10:03:01 UTC`
- 역할: Reviewer B. CMSIS/SDK 상수 검증 연결, CMake/build, SRAM checker false PASS, negative/mutation 시험 독립성, coverage/evidence 정합성.
- 저장소: `F:/dev/canview-wt/t007-ota-container`
- 실제 확인 candidate: `481a80575c2f2a285f2e09353f572c71ee4218b5`
- 실제 확인 base: `f3283fbfb1efce4dc46d017e51156fc3f289cb6a`
- manifest 대조: 15파일, 679줄 추가·24줄 삭제. 시작과 종료에 candidate 존재 및 hash를 확인했고, 종료에는 candidate/base 모두 `^{commit}`으로 재확인했다.

격리는 **COMMIT OBJECT-ONLY**였다. 저장소 내용은 지정 hash의 `git show`와 base→candidate `git diff`로만 읽었다. 이동 branch나 checkout 파일을 기준선으로 사용하지 않았다. detached worktree·빌드·시험 실행·파일 변경·commit·push·PR 조작은 없었다. checkout HEAD/clean 상태는 이 격리 방식에서 검토 기준으로 사용하지 않았으며 확인했다고 주장하지 않는다. 다른 reviewer 결과를 읽거나 이전 면제를 적용하지 않았다.

적용 skill은 `embedded-cstyle`, `embedded-architecture`, `embedded-isr-design`이다. 각각 C 계약, platform 경계, NMI의 책임과 공유 상태 검토에 사용했다.

**Finding B-01 — P2 / OPEN: SRAM 검사기가 조건부 간접 분기를 놓친다**

위치:

- `tools/ota/check_stm32_flash_ram.py:37`
- `tools/ota/check_stm32_flash_ram.py:43`
- `tests/ota/test_stm32_flash_ram.py:27`

검사기는 간접 분기를 정확히 `blx` 또는 `bx`인 mnemonic에 대해서만 거절한다. 뒤의 직접 분기 정규식도 `bxne` 같은 조건부 간접 분기를 포함하지 않는다. 따라서 다음 형태가 assembly 입력에 있으면 해당 분기를 검사하지 않고 통과시킨다.

```text
40: 2b00 cmp r3, #0
42: bf18 it ne
44: 4718 bxne r3
46: 4770 bx lr
```

필수 section·symbol·함수 label과 빈 relocation 입력을 유지하면, `bxne r3`의 목적지가 section 밖인지 판정하는 코드가 없다. `blx lr`도 현재 `operand == "lr"` 예외 때문에 허용된다. 이 예외는 반환용 `bx lr`과 구분되어야 한다.

실패 시나리오는 후속 수정에서 SRAM 함수에 조건부 callback 또는 tail branch가 들어오는 경우다. register를 통한 목적지에는 직접 외부 symbol relocation이 없을 수 있으므로 relocation 검사만으로 보완되지 않는다. 현재 negative 시험은 `blx r3`, `bx r3`, 직접 분기와 `cbz/cbnz`를 다루지만 조건부 간접 분기를 다루지 않는다.

영향은 checker의 `local functions only` PASS가 실제 검사 범위를 초과한다는 것이다. **현재 candidate의 실제 Arm object에 이 명령이 있다는 주장은 아니다.** 기존 parser의 한계가 이번 read profile에도 적용된 검증 결함으로 분류한다.

권고:

- 조건 suffix를 포함해 간접 분기를 판별하고, 허용 반환 명령을 별도로 제한한다.
- `bxne r3`, 조건부 간접 call, `blx lr` 거절 회귀를 추가한다.
- 문자열 fixture 외에 고정 Arm assembler/compiler가 생성한 작은 negative object를 검사해 실제 objdump 출력과 parser의 대응도 확인한다.

근거 수준: source의 정규식·분기 경로를 추적한 정적 반례다. 위 입력으로 validator를 직접 실행하지 않았다.

**Finding B-02 — P2 / OPEN: 검사 스크립트 변경이 Arm object 재검사를 유발하지 않는다**

위치:

- `firmware/communicator/stm32/CMakeLists.txt:230`
- `firmware/communicator/stm32/CMakeLists.txt:233`

read object 검사는 `canview_boot_flash_guard` archive의 `POST_BUILD`에 연결되어 있다. 하지만 `tools/ota/check_stm32_flash_ram.py` 자체를 재검사 의존성으로 등록하지 않았다.

실패 시나리오는 한 번 빌드한 뒤 검사 스크립트만 수정하고 같은 build directory에서 다시 빌드하는 경우다. object/archive 입력이 그대로면 archive가 다시 생성될 이유가 없고, 강화된 checker도 기존 object에 실행되지 않을 수 있다. Python 단위시험을 별도로 통과시켜도 그것은 synthetic 입력 검사이므로 실제 Arm object 재검사를 대신하지 않는다.

영향은 증분 빌드에서 최신 checker로 검증했다고 오인할 수 있다는 것이다. 전달받은 **clean Arm 빌드 결과를 무효라고 판단한 것은 아니다.** 기존 command 검사 방식에서 상속되어 새 read 검사에도 존재하는 의존성 누락이다.

권고: object와 checker script를 명시적 `DEPENDS`로 갖는 검증 stamp/target을 두거나, 필요한 build target에서 검사를 항상 실행하도록 연결한다. script만 변경했을 때 실제 object 재검사가 수행되는지도 회귀 확인한다.

근거 수준: CMake 선언의 정적 검토다. 증분 빌드를 직접 실행하지 않았다.

**공격한 시나리오와 추가 판정**

- 입력: NULL, 길이 0/257/`UINT32_MAX`, 슬롯 전후 경계, 끝을 넘는 길이, 비정렬 byte 범위와 마지막 word 접근을 추적했다. 길이 제한 후 주소 상한을 계산하므로 해당 계산의 unsigned underflow 경로는 확인하지 못했다.
- 오류 출력: 앞부분 staging 이후 각 load에서 ECCC/ECCD가 발생해도 성공한 경우에만 `memcpy`하는 구조를 확인했다. per-load 오류 시험 65×3과 전체 출력 불변 검사가 대응한다.
- NMI: 미무장 상태, 기존 ECC flag, 지연·미전달, sticky clear, clock/parity 동시 오류, HardFault, read 전후 경계를 검토했다. NMI의 정상 처리에는 erase·parsing·logging·heap·watchdog feed가 없다.
- 복원: 반환 경로의 VTOR·PRIMASK·ACR 복원과 fatal 경로의 fail-stop 계약을 구분했다. 실제 예외 복귀 timing은 검증하지 않았다.
- 시험 독립성: host 모형은 실제 MMIO load, W1C write, AIRCR reset, SRAM 주소 검사를 대체한다. 특히 `read_ram_ready()`의 target 계산은 host에서 Boolean hook으로 대체된다. 따라서 host coverage 100%로 target 주소 검증을 입증할 수 없다. journal이 이 제외 범위를 명시하고 있어 이를 별도 허위 evidence finding으로 보고하지 않는다.
- CMSIS 대조: 새 register 상수들은 기존 `check_stm32_core.py`의 상수 추출 형식에 들어가며 vendor header와 비교하는 compile-time assertion 생성 경로를 확인했다. 모형 상수끼리만 비교하는 구조는 아니다. 실제 SDK header와 compiler 실행 결과는 이번 리뷰에서 재확인하지 않았다.
- evidence 정합성: 98,816 word, alignment 4×length 256, per-load ECC 195, 지연 31, fatal 12+경계 2, RDP 256이라는 문서 수치는 시험 loop 구조와 일치한다. fixture와 Python 기대 digest 변경도 서로 일치한다. digest 자체는 재계산하지 않았다.

**실제 읽은 파일**

다음 15파일은 **변경 delta 전부**를 읽었다. 새 runtime/header와 read 시험, register model, SRAM 검사기·시험, guard 시험, bootloader README는 전체 source도 읽었다.

```text
CMakeLists.txt
docs/journal.md                         — 이번 추가 항목과 diff 문맥만
docs/resume.md
docs/tasks/T-107-stm32-mcuboot.md
firmware/communicator/stm32/CMakeLists.txt
firmware/communicator/stm32/bootloader/README.md
firmware/communicator/stm32/interface/canview_stm_flash_read.h
firmware/communicator/stm32/platform/stm32g474/flash_read.c
firmware/communicator/stm32/tests/register_model.h
tests/hil/fixtures/t103-capture-only.jsonl
tests/ota/test_stm32_flash_guard.c
tests/ota/test_stm32_flash_ram.py
tests/ota/test_stm32_flash_read.c
tests/test_t103_capture_helpers.py       — 변경 hunk와 문맥
tools/ota/check_stm32_flash_ram.py
```

추가로 읽은 commit object:

```text
AGENTS.md
docs/README.md
docs/runbooks/agent-workflow.md
docs/architecture/ota.md                — 제목 목록, §5 전체와 §7.1
firmware/communicator/stm32/platform/stm32g474/flash_guard.c
firmware/communicator/stm32/bsp/flash_layout.h
firmware/communicator/stm32/bootloader/CMakeLists.txt
cmake/CanviewWarnings.cmake
tools/check_stm32_core.py               — 상수 추출·CMSIS 대조와 관련 문맥
```

저장소 밖에서 전체를 읽은 skill:

```text
C:/Users/digit/.codex/skills/embedded-cstyle/SKILL.md
C:/Users/digit/.codex/skills/embedded-architecture/SKILL.md
C:/Users/digit/.codex/skills/embedded-isr-design/SKILL.md
```

일부 큰 묶음 출력은 잘려 필요한 문서를 분할 재조회했다. 지정 delta와 187줄 runtime에 읽지 않은 부분은 없다. root CMake의 무관한 기존 target 전체를 감사한 것은 아니다.

**실제 실행 명령 기록**

아래는 반복 호출과 출력용 필터를 묶어 표현한 실행 목록이다. `R/C/B`는 위 저장소/candidate/base의 약기이며, 실제 호출에는 전체 경로와 hash를 사용했다.

```text
git -C R cat-file -e 'C^{commit}'
git -C R rev-parse C B
git -C R rev-parse 'C^{commit}' 'B^{commit}'
git -C R diff --find-renames --stat B C
git -C R diff --find-renames B C -- <아래 경로 그룹>
git -C R show C:<위 commit-object 파일 목록의 각 경로>
Get-Content -Raw <위 skill 경로 각각>
```

`diff` 경로 그룹은 다음 네 묶음이었다.

1. root/STM32 CMake, register model, SRAM checker·시험, guard 시험.
2. journal/resume/task/bootloader README, capture fixture·Python 기대값.
3. `flash_read.c`와 공개 header.
4. `test_stm32_flash_read.c`.

`show` 출력에는 행 번호용 `ForEach-Object`, 분할용 `Select-Object`, 관련 절·상수 추출용 `Select-String`을 사용했다. UTC 기록은 시계 도구로 읽었다. 위에 없는 compiler·CTest·sanitizer·coverage·validator 실행은 하지 않았다.

**미검토·미실행 범위와 조건**

전달받은 Host Debug/Release 156/156, focused 2/2, Arm 네 구성 ELF/MAP/BIN 및 warning 0, read SRAM 528/436B·외부 relocation 0, ASan/UBSan, host coverage 100%는 **제공된 로컬 근거**다. 이번 리뷰의 직접 실행 증거로 승격하지 않았다. 원본 build log, 실제 object/ELF/MAP/BIN, coverage artifact도 열지 않았다.

Flash backend, 최종 SRAM linker/copy·전체 stack budget, boot executable/handoff, trusted BSP identity, T-205, 전체 T-107/PR37 merge 수용은 범위 밖·미완료다. 이 미완료 자체를 새 source 결함으로 중복 보고하지 않았다.

**Physical/HIL: NOT_RUN.** 실제 MMIO·ECC 주입·NMI latency·reset/power-cut·torn word/page·차량 안전은 입증하지 않는다.

CONDITIONAL의 조건은 B-01/B-02를 수정·재검증하거나, 각각 owner·상세 task·적용 gate·목표 시점을 갖춘 P2 disposition으로 명시하는 것이다. 현재 guarded-read runtime에서 별도의 P0/P1 결함은 확인하지 못했지만, checker PASS를 완전한 SRAM 실행 독립성이나 전체 boot 안전의 증거로 사용할 수는 없다.
