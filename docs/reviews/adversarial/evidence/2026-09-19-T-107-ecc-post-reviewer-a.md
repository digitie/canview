# T-107 ECC post-fix Reviewer A 원본

- 실제 execution ID: `01a0b91a-acfd-7bb2-986d-241f09c0e0e6`
- Coordinator 시작: `2026-09-19 10:23:38 UTC`
- 격리: commit object-only. 아래 원문을 수정하지 않고 보존한다.

## 공통 전달 manifest

```text
CANView T107 ECC post-fix+IO 독립 source review. Repo F:/dev/canview-wt/t007-ota-container. Candidate 122924cd348f2d7a43b314fbc806817b86e30bd5, base481a80575c2f2a285f2e09353f572c71ee4218b5. 같은 object-only 절차(cat-file/rev-parse/diff/show)로 immutable source를 읽고 각자 자신의 이전 finding closure와 전체 구현delta를 검토. 20파일delta 중 기존 review evidence3파일은 과거 원본 보존이며 peer 결과를 읽지 말 것; docs/reviews/README.md의 peer 요약도 새독립판정 근거로 삼지말 것. source전체/테스트delta/docs는 읽어라. 주새파일 flash_io.c90줄 test_stm32_flash_io.c172줄: 기존read/write/eraseIO를bounded read/단일command로연결,부분실패/FFskip/readback/progress. 새로운 framework없음. 기존boot_go host모형및최종bootELF에는아직미연결, 범위밖미완료. 이번판정은 전체T107/PR37merge승인아님. 같은 AGENTS/필요embeddedskills/OTA설계 준수. 수정/commit/push금지.
제공근거:HostD/R157/157,최후test-only소수수정후focused3/3각각; ArmprimaryD/Rclean ELF/MAP/BIN+IOarchive warning0(readRAM564/456),실제Armnegativeobject정상bx lr/거절bxne,blxne,blx lr; ASAN/UBSANreadIO2/2,GNU3/3,readhostcoverage100%/IO branch98.28% 나머지100%; no-source및checker-only증분rerun성공. CI481a8056/6success는pre-fix근거뿐이며새CI대기. 제공결과를직접실행으로표시금지.
새 raw최종응답:execution/reviewID,UTC시작끝,candidate/base실제확인,격리,실제읽은파일/명령,own기존finding별FIXED/OPEN근거와정확한file:line,새P0/P1/P2/P3실패시나리오/영향/권고,미검토범위,physical/HIL NOT_RUN,source단위BLOCK/CONDITIONAL/PASS. 중단이면incomplete/BLOCK. 새IO전체도검토하되최종loader/physical미완료를구현결함으로중복보고하지마라. 서로의새finding공유없음.
```

## 역할 추가 지시

Review ID T107-ECC-POST-A-20260919-01. 원A 전문runtime/NMI/ownership,bounds/partialfailure; A-01 SR오류확인과새IO유효성 검토.

## 원본 결과

**Source 단위 판정: PASS.** 이전 A-01은 **FIXED**이며, 이번 구현 delta에서 새 P0/P1/P2/P3 finding은 확인하지 못했다. 전체 T-107·PR37 merge 또는 실제 bootloader 안전 승인은 아니다.

### 실행·기준선·격리

| 항목 | 확인 내용 |
|---|---|
| Review/Execution ID | `T107-ECC-POST-A-20260919-01` — 별도 execution ID는 알 수 없어 제공 ID 사용 |
| 전문영역 | Cortex-M/NMI runtime, VTOR·context 수명, ownership, bounds, 부분 실패 |
| UTC 시작 | 2026-09-19 10:23:51 |
| UTC 종료 | 2026-09-19 10:26:19 |
| Repo | `F:/dev/canview-wt/t007-ota-container` |
| 실제 candidate | `122924cd348f2d7a43b314fbc806817b86e30bd5` |
| 실제 base | `481a80575c2f2a285f2e09353f572c71ee4218b5` |
| Manifest 대조 | 두 commit 객체 존재와 hash 일치 확인. 전체 diff 통계 20파일, +873/-22 |

저장소 내용은 고정 hash의 `git show`와 `git diff`로만 읽었다. 이동 branch·worktree 파일은 읽지 않았다. 수정·commit·push·PR 작업, 컴파일·시험 실행, detached worktree 생성은 하지 않았다. 따라서 worktree HEAD/clean 확인은 해당하지 않는다.

`docs/reviews/**`는 내용 조회에서 제외했다. 기존 review 3파일과 `docs/reviews/README.md`는 열지 않았다. 필수 문서인 resume/task 및 journal delta에 포함된 과거 B finding 요약은 노출되었으나 독립 판정이나 peer finding closure의 근거로 사용하지 않았다. 서로의 새 finding 공유는 없었다.

### 이전 finding closure

**A-01 · 기존 P1 · FIXED — post-load SR 오류의 성공 전달 차단**

아래 위치는 모두 이번 candidate 기준이다.

| 근거 | 정확한 위치 |
|---|---|
| 각 load와 ECCD 대기 뒤 SR을 읽음 | `firmware/communicator/stm32/platform/stm32g474/flash_read.c:99` |
| 예상 밖 BSY는 SRAM fatal 경로로 분기 | 같은 파일 `:101` |
| EOP 이외 SR 상태가 있으면 `failed=1` | 같은 파일 `:102` |
| 실패하면 해당 word를 staging에 채택하기 전에 중단 | 같은 파일 `:112` |
| 실패 결과는 `CANVIEW_INCOMPLETE` | 같은 파일 `:124` |
| destination 복사는 여전히 `CANVIEW_OK`에만 수행 | 같은 파일 `:189` |

이전 반례인 **초기 SR=0 → load 중 RDERR 설정 → ECCR=0**을 정적으로 다시 추적했다. 이제 `failed=1`로 중단하고, cache·VTOR·PRIMASK 복원 후 비성공을 반환한다. 새 코드가 SR을 지우지 않아 오류도 보존된다.

회귀시험 소스도 반례를 직접 겨냥한다.

- `tests/ota/test_stm32_flash_read.c:81`: load hook에서 SR 오류 설정.
- `:255`: unaligned 256B 읽기의 65개 load 위치 각각에 RDERR 주입.
- `:262`: `INCOMPLETE`, 중단 위치, SR 보존, 출력 전체 불변과 복원 검사.
- `:271`: 슬롯 마지막 주소의 최소 1B 읽기 검사.
- `:292`: ECC 없이 BSY만 발생하는 fatal 경로 검사.

**Closure는 source와 회귀시험 설계의 독립 재검토 결과다.** 이번 reviewer가 해당 시험을 실행했다는 의미는 아니다. 제공된 실행 결과는 아래에 별도로 기록한다.

### 새 delta 검토 결과

새 finding: **P0 0건 / P1 0건 / P2 0건 / P3 0건.**

다음 실패 시나리오를 구현·계약·시험과 대조했다.

| 공격 시나리오 | 검토 결과 |
|---|---|
| zero·`UINT32_MAX` length, 슬롯 밖 주소, primary→secondary 경계 횡단 | `flash_io.c:11`의 `io_range()`가 기존 BSP 범위 검사를 재사용한다. 한 슬롯 안의 요청만 허용하고 program/erase 정렬을 검사한다. |
| 큰 read의 후속 chunk 실패 | `flash_io.c:45`에서 즉시 실패한다. 앞선 chunk는 남지만 전체 출력을 버려야 한다는 계약이 `canview_boot_flash.h:11`에 명시되어 있다. `test_stm32_flash_io.c:108`에서 이를 검사한다. |
| 기존 non-FF word에 재쓰기, all-FF 입력 | `flash_io.c:60`에서 먼저 기존 내용을 guarded read하고 non-FF를 거절한다. 입력 all-FF는 `:62`에서 program 없이 건너뛴다. 기존 데이터가 non-FF이면 all-FF 입력도 성공으로 처리하지 않는다. |
| program 실패 또는 성공 반환 뒤 잘못된 데이터 | `flash_io.c:63`에서 명령 실패를 전파하고, `:65`에서 guarded read-back 및 byte 대조를 수행한다. 실패 뒤 자동 retry/erase 또는 후속 단위 진행이 없다. |
| erase 성공 반환 뒤 일부 page 미소거 | `flash_io.c:82`에서 2KiB 전체를 256B씩 검사한다. 모든 chunk가 FF여야 해당 page의 progress를 호출한다. |
| 부분 write/erase 뒤 실패 | 이전 단위의 변경을 rollback했다고 주장하지 않는다. API 단위 원자성을 약속하지 않는 문서와 일치한다. 실패한 현재 단위의 내용까지 정상으로 간주할 근거도 없다. |
| 실패·FF skip에도 progress 호출 | program은 read-back 대조 뒤 `flash_io.c:67`, erase는 전체 page 검증 뒤 `:87`에서만 호출한다. FF skip·오류 경로에는 호출이 없다. |
| progress를 통한 재진입·source 변경 | 단일 owner, ISR/task/reentry 금지, 호출 중 source 불변 계약을 확인했다. hook이 이 계약을 어기는 사용은 허용되지 않는다. |
| SR 수정이 기존 NMI/ECC cleanup에 미치는 영향 | 기존 ECC 처리와 armed/context 수명, VTOR·PRIMASK 복원 경로를 다시 읽었다. 새 SR 검사가 실패한 데이터를 채택하거나 context 수명을 늘리는 경로는 찾지 못했다. |
| 검사기·CMake 변경의 회귀 | 조건부 `bx`/`blx` 처리, `blx lr` 거절, 실제 Arm probe 시험, archive가 의존하는 검사 target을 소스로 확인했다. 실행 결과나 peer finding closure를 대신 판정하지 않았다. |

FF 판독만으로 과거 program 이력이나 fresh erase를 증명할 수 있다는 가정은 승인하지 않았다. README가 torn-word 재개와 상위 복구 책임을 별도로 남겨 두고 있어, 이를 이번 IO의 새 결함으로 중복 보고하지 않았다.

### 실제 읽은 파일

20파일 delta 중 review 관련 4파일을 제외한 **16파일의 변경 전체**를 읽었다.

| 파일 | 조회 범위 |
|---|---|
| `CMakeLists.txt` | delta 및 78–127행 |
| `docs/journal.md` | 이번 delta와 diff 문맥 |
| `docs/resume.md` | 전체 |
| `docs/tasks/T-107-stm32-mcuboot.md` | 전체 |
| `firmware/communicator/stm32/CMakeLists.txt` | delta 및 214–253행 |
| `firmware/communicator/stm32/bootloader/README.md` | 전체 214줄 |
| `firmware/communicator/stm32/bootloader/include/canview_boot_flash.h` | 전체 |
| `firmware/communicator/stm32/interface/canview_stm_flash_read.h` | 전체 |
| `firmware/communicator/stm32/platform/stm32g474/flash_io.c` | 전체 90줄 |
| `firmware/communicator/stm32/platform/stm32g474/flash_read.c` | 전체 191줄 |
| `tests/hil/fixtures/t103-capture-only.jsonl` | delta 전체 |
| `tests/ota/test_stm32_flash_io.c` | 전체 172줄 |
| `tests/ota/test_stm32_flash_ram.py` | 전체 |
| `tests/ota/test_stm32_flash_read.c` | delta 및 전체 323줄 |
| `tests/test_t103_capture_helpers.py` | delta와 diff 문맥 |
| `tools/ota/check_stm32_flash_ram.py` | 전체 |

추가 candidate 파일:

- `AGENTS.md` — 전체
- `docs/README.md` — 전체
- `docs/runbooks/agent-workflow.md` — 전체
- `docs/architecture/ota.md` — §5, §7.1
- `firmware/communicator/stm32/platform/stm32g474/flash_command.c` — 전체 192줄
- `firmware/communicator/stm32/platform/stm32g474/flash_guard.c` — 전체
- `firmware/communicator/stm32/bsp/flash_layout.c` — 전체
- `firmware/communicator/stm32/bsp/flash_layout.h` — 전체
- `firmware/communicator/stm32/interface/canview_stm_flash_command.h` — 전체
- `firmware/communicator/stm32/bootloader/flash_map.c` — 전체

동일 대화의 이전 리뷰에서 완독한 다음 스킬을 재사용했다. 이번 실행에서 다시 파일을 읽지는 않았다.

- `C:/Users/digit/.codex/skills/embedded-cstyle/SKILL.md`
- `C:/Users/digit/.codex/skills/embedded-isr-design/SKILL.md`
- `C:/Users/digit/.codex/skills/embedded-driver-design/SKILL.md`

### 실제 명령

아래 `R`, `C`, `B`는 각각 위 저장소 경로와 candidate/base 전체 hash의 표기상 축약이다. 반복 호출은 합쳐 기록했다.

```text
git -C R cat-file -e 'C^{commit}'
git -C R cat-file -e 'B^{commit}'
git -C R rev-parse 'C^{commit}' 'B^{commit}'
git -C R diff --find-renames --stat B C
git -C R diff --find-renames B C -- . \
  ':(exclude)docs/reviews/**' \
  ':(exclude)tests/ota/test_stm32_flash_io.c' \
  ':(exclude)firmware/communicator/stm32/platform/stm32g474/flash_io.c' \
  ':(exclude)docs/journal.md'
git -C R diff --find-renames B C -- docs/journal.md
git -C R show C:<위 목록에서 전체 또는 부분 조회한 각 경로>
```

신규 IO 두 파일은 위 diff 출력에서만 제외하고, 각각 `show`로 전체를 읽었다. 필요한 출력에는 PowerShell 줄번호 필터를 적용했다.

```powershell
ForEach-Object -Begin {$n=0} -Process {$n++; '{0,4}: {1}' -f $n,$_}
Select-Object -Skip 92 -First 40
Select-Object -Skip 215 -First 15
```

CMake 두 파일은 줄번호를 붙인 뒤 각각 78–127행, 214–253행만 출력했다. 종료 직전 두 commit hash를 다시 확인했다. UTC 시각 조회 외 외부 검색·서비스 호출은 하지 않았다.

### 실행 증거와 미검토 범위

이번 실행은 **정적 source review**다. 다음은 사용자 제공 근거이며 직접 실행·로그 감사 결과가 아니다.

- Host Debug/Release 157/157, 최후 test-only 수정 뒤 focused 각 3/3.
- Arm primary Debug/Release clean ELF/MAP/BIN·IO archive, warning 0, read SRAM 564/456B.
- 실제 Arm negative object 및 no-source/checker-only 증분 재검사 성공.
- ASan/UBSan read·IO 2/2, GNU 3/3.
- read host coverage 100%, IO branch 98.28%·나머지 100%.
- `481a805` CI 6/6은 **수정 전 근거**이며 새 candidate CI 통과로 취급하지 않았다.

요청된 구현 source 전체와 비리뷰 delta에서 미독한 부분은 없다. review 원본 3파일과 review index는 독립성 유지를 위해 의도적으로 제외했다. 기존 대형 문서·CMake·시험 파일의 변경 밖 부분은 위에 표시한 범위 외에는 감사하지 않았다.

기존 `boot_go` host 모형과 새 IO의 통합, 최종 boot ELF, SRAM linker/copy·전체 stack, BSP identity, T-205, torn-word 재개·watchdog 시간 정책·production 보호 수용은 범위 밖이며 미완료로 유지한다.

**Physical/HIL: NOT_RUN.** 실제 MMIO·ECC/NMI timing·예외 복귀·reset·전원 차단 검증을 수행하지 않았다.

**최종: A-01 FIXED, 새 finding 없음, 지정된 post-fix+IO source 단위 PASS.**
