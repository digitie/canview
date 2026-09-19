# T-107 ECC post-fix Reviewer B 원본

- 실제 execution ID: `01a0b91a-ae86-7be3-afcd-ada989ce98cc`
- Coordinator 시작: `2026-09-19 10:23:38 UTC`
- 격리: commit object-only. 아래 원문을 수정하지 않고 보존한다.

## 공통 전달 manifest

```text
CANView T107 ECC post-fix+IO 독립 source review. Repo F:/dev/canview-wt/t007-ota-container. Candidate 122924cd348f2d7a43b314fbc806817b86e30bd5, base481a80575c2f2a285f2e09353f572c71ee4218b5. 같은 object-only 절차(cat-file/rev-parse/diff/show)로 immutable source를 읽고 각자 자신의 이전 finding closure와 전체 구현delta를 검토. 20파일delta 중 기존 review evidence3파일은 과거 원본 보존이며 peer 결과를 읽지 말 것; docs/reviews/README.md의 peer 요약도 새독립판정 근거로 삼지말 것. source전체/테스트delta/docs는 읽어라. 주새파일 flash_io.c90줄 test_stm32_flash_io.c172줄: 기존read/write/eraseIO를bounded read/단일command로연결,부분실패/FFskip/readback/progress. 새로운 framework없음. 기존boot_go host모형및최종bootELF에는아직미연결, 범위밖미완료. 이번판정은 전체T107/PR37merge승인아님. 같은 AGENTS/필요embeddedskills/OTA설계 준수. 수정/commit/push금지.
제공근거:HostD/R157/157,최후test-only소수수정후focused3/3각각; ArmprimaryD/Rclean ELF/MAP/BIN+IOarchive warning0(readRAM564/456),실제Armnegativeobject정상bx lr/거절bxne,blxne,blx lr; ASAN/UBSANreadIO2/2,GNU3/3,readhostcoverage100%/IO branch98.28% 나머지100%; no-source및checker-only증분rerun성공. CI481a8056/6success는pre-fix근거뿐이며새CI대기. 제공결과를직접실행으로표시금지.
새 raw최종응답:execution/reviewID,UTC시작끝,candidate/base실제확인,격리,실제읽은파일/명령,own기존finding별FIXED/OPEN근거와정확한file:line,새P0/P1/P2/P3실패시나리오/영향/권고,미검토범위,physical/HIL NOT_RUN,source단위BLOCK/CONDITIONAL/PASS. 중단이면incomplete/BLOCK. 새IO전체도검토하되최종loader/physical미완료를구현결함으로중복보고하지마라. 서로의새finding공유없음.
```

## 역할 추가 지시

Review ID T107-ECC-POST-B-20260919-01. 원B 전문SDK/build/checker/negative/evidence; B-01/B-02 및 IO test누락/falsePASS검토.

## 원본 결과

Source 단위 판정: **PASS**. 본인의 기존 finding **B-01/B-02는 FIXED**로 재확인했다. 새 IO 구현을 포함한 검토 범위에서 새로운 P0/P1/P2/P3 finding은 확인하지 못했다. 전체 T-107/PR37 merge 승인이나 실제 boot 안전 판정은 아니다.

**실행·기준선·격리**

- Review/execution ID: `T107-ECC-POST-B-20260919-01` — 별도 execution ID가 없어 지정 review ID 사용.
- 전문영역: SDK/build, SRAM checker false PASS, negative 시험 독립성, IO 시험 누락, evidence 정합성.
- UTC 시작: `2026-09-19 10:23:50 UTC`
- UTC 종료: `2026-09-19 10:27:11 UTC`
- 저장소: `F:/dev/canview-wt/t007-ota-container`
- 실제 확인 candidate: `122924cd348f2d7a43b314fbc806817b86e30bd5`
- 실제 확인 base: `481a80575c2f2a285f2e09353f572c71ee4218b5`
- 전달 manifest와 대조: 20파일, 873줄 추가·22줄 삭제. 시작·종료에 candidate 존재와 candidate/base commit hash를 확인했다.

**COMMIT OBJECT-ONLY**로 수행했다. 저장소 내용은 고정 hash의 `git show`와 `git diff`로만 읽었다. 이동 branch나 checkout 내용은 기준선으로 사용하지 않았다. 파일 변경, detached worktree 생성, compile/test 실행, commit/push/PR 조작은 없었다. checkout HEAD/clean 확인은 이 방식의 검증 근거로 사용하지 않았다.

과거 review 산출물 3파일과 `docs/reviews/README.md`는 열지 않았다. 필수 상태 문서에 포함된 과거 finding 요약은 독립 판정 근거로 사용하지 않았다. peer의 새 결과를 읽거나 공유하지 않았다.

이전 턴에서 전체를 읽은 `embedded-cstyle`, `embedded-architecture`, `embedded-isr-design`을 계속 적용했다. workflow는 base 대비 변경 없음도 확인했다.

**기존 finding closure**

| Finding | 판정 | Candidate의 직접 source 근거 |
|---|---|---|
| B-01 — 조건부 간접 분기 누락 | **FIXED** | `tools/ota/check_stm32_flash_ram.py:37`에서 조건 suffix와 폭 suffix를 포함한 `bx/blx`를 판별한다. `:38`에서 `blx lr`도 거절하고 반환용 `bx … lr`만 예외로 둔다. |
| B-02 — checker 변경 시 증분 재검사 누락 | **FIXED** | `firmware/communicator/stm32/CMakeLists.txt:230`의 항상 실행 custom target, `:237`의 object/script 의존성, `:241`의 archive→검사 target 의존성으로 변경됐다. archive 재생성 여부에만 묶였던 조건이 제거됐다. |

B-01의 회귀시험도 정적 문자열 추가에 그치지 않는다. `tests/ota/test_stm32_flash_ram.py:20`에서 실제 assembler로 probe object를 만들고 objdump 결과를 검사한다. `:23`의 정상 `bx lr`와 거절 대상 `bxne r3`, `blxne r3`, `blx lr`를 구분하며, `:44`는 **간접 분기 오류**로 실패했는지 확인한다. 따라서 symbol/section 오류 때문에 우연히 거절된 것을 해당 회귀의 성공으로 세지 않는다. compiler 실패와 stderr도 검사한다.

Arm CMake의 `:235`에서 compiler/objdump를 함께 전달하므로 해당 경로에서는 negative-object 시험이 skip되지 않는다. host 호출에서 Arm 도구가 없을 때는 명시적으로 skip한다.

두 closure는 **source 재검토 판정**이다. 전달된 negative-object·증분 재실행 성공 결과를 본인이 직접 실행한 결과로 표시하지 않는다. 또한 B-01 closure가 모든 가능한 명령열에 대한 SRAM 독립성 증명을 의미하지 않는다.

**새 구현과 시험에서 공격한 시나리오**

새 finding은 없으며, 다음 경로를 추적했다.

- **범위·overflow·정렬:** `flash_io.c:11`의 슬롯 선택과 실제 `canview_stm_flash_range()`를 대조했다. 전체 요청을 먼저 검사하므로 primary→secondary 횡단, 정책 영역 진입, zero/과대 길이, 잘못된 program/erase 정렬이 primitive 호출 전에 거절된다. 범위 검사 성공 후의 offset 증가가 허용 영역을 넘는 경로는 확인하지 못했다.
- **read chunk와 부분 출력:** `flash_io.c:36`은 256B 이하로 나누고 첫 오류에서 반환한다. 이전 chunk가 남을 수 있다는 header/README 계약과 `test_stm32_flash_io.c:108`의 두 번째 chunk 실패 시험이 대응한다. 기존 bounded-read의 출력 불변 보장을 큰 IO 전체의 원자성으로 잘못 확장하지 않았다.
- **write·FF skip·중복:** `flash_io.c:60`에서 목적지 guarded read 및 erased 확인을 먼저 하고, `:62`에서 all-FF input을 skip한다. skip 경로에는 program이나 progress 호출이 없다. non-FF 목적지에 FF input을 전달해도 성공으로 덮어 처리하지 않는다.
- **잘못된 command 성공:** `flash_io.c:65`의 guarded read-back과 전체 8B 비교가 command 반환값만 믿는 것을 막는다. 시험은 각 program 위치에서 command 오류와 성공 후 데이터 변조를 주입한다.
- **erase 검증:** `flash_io.c:82`에서 page 전체를 256B씩 확인한다. 두 page의 16개 read 위치 오류와 command 실패·변조, secondary 전체 97page erase 및 영역 밖 불변 시험을 읽었다.
- **progress 조기 보고:** program은 read-back 비교 뒤 `:67`, erase는 전체 page 검증 뒤 `:87`에서만 progress를 호출한다. 오류 위치별 시험의 progress 수가 이에 대응한다. 이 hook을 실제 watchdog feed 검증으로 해석하지 않았다.
- **실패 후 계속 진행·자동 retry:** 각 primitive 실패에서 즉시 반환한다. 실패 이후 다음 단위 실행이나 자동 erase/retry 경로는 없다. 앞선 단위의 변경이 남는다는 계약도 명시되어 있다.
- **post-load SR 변경:** `flash_read.c:99`에서 load 이후 SR을 읽고, BSY는 fail-stop, EOP 외 상태는 실패로 처리한다. staging에서 출력으로 복사하기 전에 이 판정을 거친다. 새 RDERR 시험은 ECC NMI를 발생시키지 않도록 설정하고 반환값·SR 보존·출력 불변을 검사한다. 다른 reviewer의 finding closure를 대신 판정한 것은 아니다.
- **하위 primitive의 전제:** 실제 `flash_command.c`와 공개 계약도 읽었다. IO 사전 guarded read 이후 command 내부에서 다시 수행하는 raw 사전 read의 fault 경계, fresh-erase 소유권, 재개 정책은 문서에서 별도로 남겨 두고 있다. 이 연결만으로 torn-word 복구가 완성됐다고 주장하지 않는다.

IO 시험은 fake primitive를 사용하지만 실제 `flash_io.c`와 BSP 범위 구현을 실행하도록 구성되어 있다. 반면 실제 read/command와의 동작 통합이나 `boot_go` 실행 증거는 아니다. 이 구분은 CMake와 문서에서 유지된다.

**Evidence 정합성**

전달된 다음 결과는 모두 **제공 근거**이며 이번 직접 실행 결과가 아니다.

- Host Debug/Release 157/157, 최후 test-only 수정 후 focused 3/3 각각.
- Arm primary Debug/Release clean ELF/MAP/BIN 및 IO archive, warning 0, read SRAM 564/456B.
- 실제 Arm negative object의 정상/거절 결과.
- ASan/UBSan read·IO 2/2, GNU 3/3.
- read host coverage 100%, IO branch 57/58·98.28%, 나머지 100%.
- no-source 및 checker-only 증분 재검사 성공.

IO의 미실행 branch가 `flash_io.c:18`의 “BSP 성공인데 반환 주소 불일치”라는 설명은 실제 BSP 구현과 일치한다. 해당 계산에서는 성공 시 `begin + (address - begin)`을 반환하므로 정상 계약 아래 이 불일치를 만들 수 없다. coverage artifact 자체는 재확인하지 않았다.

fixture와 Python 기대값의 새 digest는 서로 일치하지만, digest 재계산은 하지 않았다. `481a805`의 CI 6/6은 **수정 전 근거**로만 취급했다. 새 candidate의 CI 성공을 확인하거나 주장하지 않는다.

**실제 읽은 파일·명령**

20파일 중 review 관련 4파일을 제외한 **16파일의 구현·시험·문서 delta를 모두 읽었다**. 새 `flash_io.c` 90줄, 새 IO 시험 172줄, 수정된 `flash_read.c` 191줄 전체를 읽었다.

```text
CMakeLists.txt                          — delta와 관련 설정·시험 등록부
docs/journal.md                         — 이번 추가 항목과 diff 문맥
docs/resume.md
docs/tasks/T-107-stm32-mcuboot.md
firmware/communicator/stm32/CMakeLists.txt
firmware/communicator/stm32/bootloader/README.md
firmware/communicator/stm32/bootloader/include/canview_boot_flash.h
firmware/communicator/stm32/interface/canview_stm_flash_read.h
firmware/communicator/stm32/platform/stm32g474/flash_io.c
firmware/communicator/stm32/platform/stm32g474/flash_read.c
tests/hil/fixtures/t103-capture-only.jsonl
tests/ota/test_stm32_flash_io.c
tests/ota/test_stm32_flash_ram.py
tests/ota/test_stm32_flash_read.c         — 전체 delta와 관련 문맥
tests/test_t103_capture_helpers.py       — 변경 hunk
tools/ota/check_stm32_flash_ram.py
```

추가로 candidate object에서 읽은 파일:

```text
AGENTS.md
docs/README.md
docs/architecture/ota.md                — §5, §7.1
firmware/communicator/stm32/platform/stm32g474/flash_command.c
firmware/communicator/stm32/interface/canview_stm_flash_command.h
firmware/communicator/stm32/bsp/flash_layout.c
firmware/communicator/stm32/bsp/flash_layout.h
firmware/communicator/stm32/bootloader/flash_map.c
```

이전 검토에서 읽은 뒤 이번 diff로 변경 없음을 확인한 파일:

```text
docs/runbooks/agent-workflow.md
docs/architecture/ota.md
firmware/communicator/stm32/tests/register_model.h
tools/check_stm32_core.py
firmware/communicator/stm32/bootloader/CMakeLists.txt
```

실행 명령은 다음과 같다. 반복 호출을 묶었으며 `R/C/B`는 위에 기록한 저장소/candidate/base의 약기다. 실제 호출에는 전체 경로와 hash를 사용했다.

```text
git -C R cat-file -e 'C^{commit}'
git -C R rev-parse 'C^{commit}' 'B^{commit}'
git -C R diff --find-renames --stat B C
git -C R diff --find-renames B C -- <위 변경 파일 경로 그룹>
git -C R diff --find-renames B C -- <위 변경 없음 확인 경로>
git -C R show C:<위에서 읽었다고 명시한 파일>
```

출력 분할에는 `Select-Object`, 행 번호에는 `ForEach-Object`, OTA 절 선택에는 `ForEach-Object` 필터를 사용했다. 큰 묶음 출력에서 잘린 README 부분은 전체 `show`로 보완했다. UTC는 시계 도구로 읽었다. compile, CTest, validator, sanitizer, coverage, 증분 build 명령은 실행하지 않았다.

**미검토·미실행 경계**

지정된 구현 검토에 읽지 않은 delta는 없다. 의도적으로 제외한 파일은 과거 통합 review 1개, reviewer evidence 2개, `docs/reviews/README.md`다. 무관한 기존 코드 전체를 감사한 것은 아니다.

실제 SDK header 재대조, build log·object·ELF/MAP/BIN·coverage artifact 감사, 새 CI 확인은 이번에 수행하지 않았다. 기존 `boot_go` host 모형과 새 IO의 통합, 최종 boot ELF, SRAM linker/copy·전체 stack, BSP identity, T-205, 복구·watchdog 시간 정책은 범위 밖·미완료다. 이를 새 구현 결함으로 중복 보고하지 않았다.

**Physical/HIL: NOT_RUN.** 실제 MMIO, ECC/NMI timing, Flash stall, reset/power-cut, torn word/page 및 실기 부팅 안전은 이번 PASS의 대상이 아니다.
