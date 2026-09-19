# T-107 SRAM Reviewer B 원본

- Agent ID: `01a0b970-a370-73b0-8f38-123862512686` (Kepler)
- Coordinator dispatch: 2026-09-19 11:32:41 UTC

## 공통 전달 원문

```text
T107 SRAM link/startup 독립 적대적 source review. 저장소 F:/dev/canview-wt/t007-ota-container. candidate 43b52b85d97a73e914de958cfd8b2e4384afac37 base d20e9e3e374500fb866aa6912ce9916040a98309. 반드시 git cat-file/rev-parse 확인, object-only git show candidate:path/git diff base candidate로 읽고 immutable line 번호 보고. 파일 수정/commit 금지. AGENTS/docs README/resume/T107 상세 및 관련 workflow/skill 직접 읽기. SDK 원본 읽기 가능 C:/cv/STM32CubeG4-1.6.3. Scope 15-file diff: boot64KiB linker, startup_flash_ram C preinit barrier, target-local VTOR, 실제 SDK startup+Flash C 비배포 link test, 실제 ELF/copy/branch validator 및 negative tests, CI artifact additions/doc. 기존 primary linker는 변경하지 않음. main test는 boot_go/handoff/watchdog/policy 없음, final loader가 아니고 physical/HIL NOT_RUN. 실제 clean Arm D/R compile warning0 + link/copy test PASS, hostRelease158 전체 실행중. 범위밖 전체T107 수용/실기/production/merge 승인. 두 reviewer findings 공유하지 않음. 현재 새 SDK loop592bits/preinit32bits, 주소mutation,actualld5negative 시험. 검토 종료raw 한국어 report에는 executionID, UTC 시작/끝, 전문범위, candidate/base/격리/hash 검증, 실제읽은파일/실행명령, P0/P1/P2/P3 finding 정확file:line 실패시나리오/영향/권고, 못읽은범위, physical/HIL NOT_RUN, verdict BLOCK/CONDITIONAL/PASS 포함. 없는 finding 만들지 않지만 검토 못하면 incomplete 명시. 현재 scope에 한정한 완전한 line-level report 반환.
```

## 전문 범위 전달 원문

Reviewer B: build/linker validation/CI/evidence/security/negative mutation 전문. CMake dependency/VTOR scope, ELF validator 및 mutable evidence drift/허용오류, CI와 tests 우선 검토.

## 진행 확인 메시지

진행 상태를 짧게 알려주세요. 독립 범위의 정확한 line-level 최종 raw report가 다음 critical path입니다. 새 범위 확장 없이 현재 candidate delta에 집중하고, 미검토는 솔직히 명시해 반환해주세요. 상대 reviewer 결과는 공유하지 않습니다.

## Raw report

Reviewer B 최종 raw report — **PASS**
판정 범위는 지정된 15-file delta의 독립 source review다. 전체 T-107 수용, 실기, production, merge 승인이 아니다.

- executionID: `T107-SRAM-B-43b52b85-20260919T113259Z` — 이 실행의 보고서 식별자
- UTC 시작: `2026-09-19T11:32:59.9122773Z`
- UTC 종료: `2026-09-19T11:38:38.0305343Z`
- 전문범위: build/linker validation, CMake dependency·VTOR scope, ELF/BIN 검증, negative mutation, CI·evidence 정합성·보안 경계
- 저장소: `F:/dev/canview-wt/t007-ota-container`
- candidate: `43b52b85d97a73e914de958cfd8b2e4384afac37`
- base: `d20e9e3e374500fb866aa6912ce9916040a98309`

**기준선·독립성**

시작과 종료에 `git cat-file` 및 `git rev-parse`로 두 commit 객체를 확인했다. candidate의 직접 parent가 base이며, 둘 사이 commit은 1개다.

candidate tree는 `ccafd315af079c507a048e6642e72ad1ddeac934`, base tree는 `7e6a33f42d766974167f213f65ae55df8f3ada72`다. 시작·종료 HEAD는 candidate와 일치했고 `git status --porcelain=v1`은 비어 있었다.

저장소 내용은 `git show candidate:path`, `git diff base candidate` 및 객체 기반 검색으로 읽었다. 아래 줄 번호는 모두 **candidate blob 기준**이다. 파일 수정·생성·commit·build 실행은 하지 않았다. 이번 상대 reviewer 보고서나 findings를 열람하거나 공유하지 않았다.

**Line-level 검토 결과**

| 실제 읽은 변경 파일·위치 | 검토 내용과 결론 |
|---|---|
| `firmware/communicator/stm32/CMakeLists.txt:167` | primary VTOR 정의가 executable의 `PRIVATE` 정의로 이동했다. boot의 `:262` 정의와 분리되며 공유 SDK source 속성으로 누출되는 기존 구성을 제거한다. |
| 같은 파일 `:253`, `:272`, `:280`, `:283`, `:289` | 실제 SDK startup/SystemInit·제품 Flash C를 사용하는 별도 link 시험이다. linker 변경의 relink 의존성, BIN 생성, 매 빌드 실행되는 검사 target, 앱보다 선행하는 object/archive 의존성을 확인했다. unittest 실패가 build 실패로 전달되는 구조다. |
| `firmware/communicator/stm32/ld/STM32G474CEUx_BOOT.ld:5`, `:9`, `:54`, `:58`, `:69`, `:76`, `:94` | Flash64KiB·RAM96KiB, RX/RW segment 구분, dummy4B, SRAM code/data 연속 VMA·LMA, bss·stack 경계, 정렬·크기·CCM 거절 ASSERT를 확인했다. MPU 보호나 전체 호출 경로 stack 검증으로 해석하지 않았다. |
| `firmware/communicator/stm32/platform/stm32g474/startup_flash_ram.c:11`, `:17` | 정적 preinit 함수 포인터와 순차적인 DSB→ISB다. SDK 복사 루프를 중복 구현하지 않으며 새로운 가변 상태·할당·ISR을 추가하지 않는다. |
| `tests/ota/stm32_boot_ram_link.c:10` | 실제 read/command 호출부 보존용 main이다. boot_go·watchdog·정책·handoff가 없고 비배포 시험이라는 선언과 일치한다. 실행 성공을 증명하는 시험으로 분류하지 않았다. |
| `tools/ota/check_stm32_boot_ram.py:17`, `:46`, `:57`, `:69`, `:81`, `:88`, `:95` | section 연속성·symbol 대조, 고정 literal pool, BL 목적지, SDK copy/bss 명령, preinit 포인터, boot VTOR 명령, ELF/BIN 대조와 연결된 SRAM 함수 검사를 추적했다. |
| `tools/ota/check_stm32_boot_ram.py:108` | barrier 확인은 disassembly의 DSB/ISB 존재·순서 검사다. 임의 함수의 모든 실행 경로를 증명하지는 않는다. 이번 candidate의 실제 C 함수는 조건 없는 두 명령이므로 이를 현재 결함으로 판정하지 않았다. |
| `tests/ota/test_stm32_boot_ram.py:18`, `:33`, `:40`, `:48` | 주소·크기 36개 변이, section 누락, startup/VTOR 592bit, preinit32bit 및 실제 linker 정상 대조군+5개 negative case의 oracle을 확인했다. linker negative는 실패 코드와 지정 diagnostic을 함께 요구한다. |
| `tools/ota/check_stm32_flash_ram.py:12`, `:23`, `:25`, `:32`, `:37` | object 주소0 계약을 유지하며 linked mode에서 SRAM 절대주소와 함수 끝·분기 목적지를 검사한다. 외부 relocation 검사는 CMake의 object 검사와 함께 적용된다. |
| `tests/ota/test_stm32_flash_ram.py:20` | linked 정상 입력, Flash/상단 경계 VMA, 함수 범위 초과, section 밖 branch/CBZ 거절을 확인했다. 기존 object 검사와의 구분도 시험한다. |
| `tools/ota/validate_stm32_map.py:51`, `:59`, `:73`, `:90`, `:95` | primary 기본값을 유지하면서 boot만 명시적64KiB·CCM 금지를 전달한다. vector, load 영역, ELF/BIN byte, load 중첩·끝·zero-fill gap 검사가 유지된다. |
| `.github/workflows/foundation.yml:311`, `:366`, `:401`, `:410`, `:426`, `:472` | primary D/R clean build에서 검사 실행 경로가 연결된다. 추가6개 산출물은 존재·크기·SHA256 manifest에 들어가고 기존 upload glob에 포함된다. PR-head revision 대조와 실패 시 성공 artifact upload 차단을 확인했다. |
| `tests/hil/fixtures/t103-capture-only.jsonl:1`, `tests/test_t103_capture_helpers.py:26` | 변경은 합성 fixture의 firmware digest 갱신이다. 객체에서 재계산한 digest와 5개 record 및 기대 상수가 일치했다. 실차 evidence로 승격하는 변경은 없다. |
| `firmware/communicator/stm32/bootloader/README.md:220`, `:224` | 비배포 link 시험, SDK 재사용, target-local VTOR, 검사 범위 및 전체 stack/runtime 미검증 경계가 구현과 일치한다. |
| `docs/journal.md:3`, `docs/resume.md:72` | 해당 단위의 구현·작성자 검증 주장과 후속 loader 작업을 구분한다. physical/HIL 및 전체 task 미완료를 유지한다. |

기존 `STM32G474CEUx_PRIMARY.ld`와 `application-sections.ld`는 base→candidate diff가 비어 있음을 별도로 확인했다.

**직접 읽은 추가 근거**

- 필수 문서: `AGENTS.md`, `SKILL.md`, `docs/README.md`, `docs/resume.md`, `docs/tasks/T-107-stm32-mcuboot.md`.
- 절차: `docs/runbooks/agent-workflow.md`, `docs/reviews/README.md` 기록 규칙, `docs/reviews/adversarial/TEMPLATE.md`.
- 관련 정본: `docs/development/windows.md`, `docs/architecture/README.md`, `docs/architecture/ota.md` §5, `implementation-readiness.md` §15.
- 관련 source: `startup_ram.c`, `syscalls.c`, `flash_command.c`, `flash_read.c`, STM32 `CMakePresets.json`, `bsp/flash_layout.cmake`, bootloader CMake의 의존성 관련 부분, `check_stm32_core.py`의 startup·compile contract·stack 부분, `tests/test_stm32_primary_map.py`, `tests/hil/run.py`의 digest 부분.
- 스킬: `embedded-architecture/SKILL.md`, `embedded-cstyle/SKILL.md`를 직접 읽고 의존성·초기화 C 검토에 적용했다. 자동 수정 지침은 사용자 read-only 요청에 따라 수행하지 않았다.

허용된 SDK 원본에서 `startup_stm32g474xx.s:1–125`, `system_stm32g4xx.c:1–220`을 읽었다. `SystemInit → copy → bss → __libc_init_array → main` 순서와 VTOR macro 동작을 확인했다. SDK HEAD는 저장소 pin과 같은 `d11b194a9f05d1b143d154771f3dbc282c8052a5`이며 status는 clean이었다.

**실제 실행 및 결과**

주요 명령은 다음과 같다. `C`와 `B`는 위의 전체 candidate/base hash다.

```text
git cat-file -t C
git cat-file -t B
git cat-file -e C^{commit}
git cat-file -e B^{commit}
git rev-parse C^{commit} B^{commit} HEAD
git rev-parse C^{tree} B^{tree}
git show -s --format=%H%n%P%n%T C
git status --porcelain=v1
git diff --find-renames B C
git diff --check B C
git diff --numstat B C
git show C:<검토 경로>
git ls-tree -r --name-only C
git grep -n <식별자> C -- <관련 경로>
git -C C:/cv/STM32CubeG4-1.6.3 rev-parse HEAD
git -C C:/cv/STM32CubeG4-1.6.3 status --porcelain=v1
```

추가로 PowerShell here-string을 `python -B -`에 전달했다. 메모리의 Git loader가 candidate blob을 직접 로드하여 다음을 실행했다. 저장소 Python source를 일반 filesystem import로 읽거나 시험 파일을 생성하지 않았다.

- primary map 기존 시험 6개: **6/6 PASS**
- SRAM 검사기의 `test_linked_bounds`, `test_read_profile`, `test_mutations`: **3/3 PASS**
- `git cat-file --batch`로 firmware/shared/protocol의 259개 blob을 읽어 canonical digest 재계산: **PASS**
- 계산값: `303dd13916fdc387c569b4c4613abb18e31c24549295b495175f0e9f62c16fcf`
- fixture 5개 record·시험 기대값 대조: **PASS**
- `git diff --check`: **PASS**

읽기 도중 hash 오타 1회와 존재하지 않는 `tests/ota/test_stm32_map.py` 경로 조회 1회가 실패했다. 올바른 hash 및 실제 `tests/test_stm32_primary_map.py`로 정정해 읽었으며 검토 누락으로 남지 않았다.

**Findings 및 한계**

- P0: 0
- P1: 0
- P2: 0
- P3: 0

이번 delta에서 실패 시나리오와 영향까지 뒷받침해 확정할 finding은 발견하지 못했다. 지정된 15-file delta의 source 검토 누락은 없다.

다만 실제 Arm D/R 재컴파일, 실제 ELF/BIN 재검사, 592bit·32bit의 실제 산출물 변이 실행, actual linker5negative 재실행, hostRelease158 완료 확인, CI artifact 다운로드·hash 감사는 **이번 reviewer 실행에서 NOT_RUN**이다. 관련 시험 source는 읽었지만 작성자의 실행 결과를 독립 실행 결과로 재분류하지 않았다. 변경되지 않은 firmware 전체 및 SDK 전체도 감사하지 않았다.

Physical/HIL, 실제 SRAM 실행·Flash stall·ECC/reset·전원 fault, 전체 MCUboot call-chain stack은 **NOT_RUN**이다.

최종 verdict: **PASS — 지정 candidate delta의 독립 source review 한정.**
