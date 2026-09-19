# T-107 ECC read Reviewer A 원본

- 실제 subagent execution ID: `01a0b91a-acfd-7bb2-986d-241f09c0e0e6`
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

Review ID T107-ECC-A-20260919-01. Reviewer A: Cortex-M/NMI runtime, VTOR/stack/context lifetime, cache/ECC ordering, bounds/ownership, fail-safe 및 IRQ/reentry 전문. 우선 firmware/communicator/stm32/platform/stm32g474/flash_read.c 및 interface/canview_stm_flash_read.h, tests/ota/test_stm32_flash_read.c. 보고 전각실제라인확인.

## 원본 결과

판정: **BLOCK — guarded-read source 단위 P1 finding 1건.** 전체 T-107 또는 PR37의 merge 판정은 아니다.

### 실행 식별과 기준선

| 항목 | 확인 내용 |
|---|---|
| Execution ID | 별도 실행 ID를 알 수 없어 제공된 `T107-ECC-A-20260919-01` 사용 |
| Reviewer | A — Cortex-M/NMI runtime, VTOR·stack·context 수명, cache/ECC 순서, bounds·ownership, fail-safe·IRQ·재진입 |
| UTC 시작 | 2026-09-19 09:59:05 |
| UTC 종료 | 2026-09-19 10:03:32 |
| 저장소 | `F:/dev/canview-wt/t007-ota-container` |
| 실제 candidate | `481a80575c2f2a285f2e09353f572c71ee4218b5` |
| 실제 base | `f3283fbfb1efce4dc46d017e51156fc3f289cb6a` |
| Manifest 대조 | 전달된 두 hash와 일치. diff는 15파일, 679 insertions, 24 deletions |
| 검토 완료 범위 | 15파일의 변경 전체, `flash_read.c` 187줄 전체, 공개 header 25줄 전체, read 시험 295줄 전체 |

저장소 source/document는 명시한 커밋의 `git show`와 두 커밋 사이 `git diff`로만 읽었다. 이동 branch·worktree 파일은 기준선으로 읽지 않았다. 컴파일하지 않았으므로 detached worktree를 생성하지 않았으며 HEAD/clean 확인은 해당하지 않는다. 수정·commit·push·PR 작업과 다른 reviewer 결과 열람은 하지 않았다.

스킬 파일은 저장소 밖 지정 경로에서 읽었다. 또한 RDERR의 하드웨어 의미 확인을 위해 외부 ST 공식 문서 검색을 수행했다. 이를 candidate 내용이나 직접 실행 증거로 취급하지 않았다.

### Finding

**A-01 · P1 · OPEN — load 이후 발생한 Flash 읽기 오류를 성공 판정에서 누락한다.**

위치는 모두 candidate 기준이다.

- `firmware/communicator/stm32/platform/stm32g474/flash_read.c:99` — load 후 ECCR만 검사한다.
- 같은 파일 `:120` — `context->failed`만으로 `CANVIEW_OK`를 결정한다.
- 같은 파일 `:170` — SR 검사는 load 이전에만 수행한다.
- 같은 파일 `:185` — 위 성공 결과로 destination을 갱신한다.
- 관련 guard: `firmware/communicator/stm32/platform/stm32g474/flash_guard.c:34` — DBANK/BFB2/NRST와 WRP를 검사하지만 PCROP 범위를 검사하지 않는다.

실패 시나리오:

1. RDP=`0xaa`, 정상 DBANK/WRP/NRST, 잠긴 CR, 초기 SR/ECCR=0인 환경으로 진입한다. 요청한 슬롯 영역에 PCROP가 설정되어 있어도 현재 guard는 이를 구분하지 않는다.
2. 해당 주소의 data load가 `FLASH_SR.RDERR`를 발생시킨다. RM0440 Rev9는 PCROP 영역의 D-bus 읽기가 RDERR를 설정한다고 명시한다. RDP0 검사만으로 PCROP 부재를 보장하지 않는다. [ST RM0440 Rev9, §3.5.2 및 FLASH_SR.RDERR](https://www.st.com/resource/en/reference_manual/rm0440-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
3. ECCR 오류 없이 load가 반환하면 `failed`는 0으로 남는다. 이후 SR을 읽는 코드가 없으므로 staging 내용을 destination에 복사하고 `CANVIEW_OK`를 반환한다.

**정적으로 확인한 사실은 `SR=RDERR, ECCR=0`인 post-load 상태를 성공으로 처리한다는 점이다.** 실제 G474의 해당 접근에서 반환되는 데이터 값과 예외 동작은 이번 실행에서 재현하지 않았다. HardFault가 발생하는 경우에는 기존 fatal 경로로 들어가지만, 이를 RDERR 검사 생략의 근거로 삼을 수 있는 증거는 확인하지 못했다.

영향: 읽기 오류가 발생한 결과를 정상 읽기로 상위 backend에 전달할 수 있다. 이후 호출은 남아 있는 RDERR 때문에 거절될 수 있어 최초 호출과 후속 호출의 결과도 달라진다. 서명 우회나 실제 손상 이미지 부팅까지 입증했다는 뜻은 아니다.

이는 생산 보호 profile 승인 미완료를 재보고한 것이 아니다. **현재 함수가 허용하는 입력 환경에서, 실행 중 발생한 오류를 현재 성공 판정이 놓치는 문제**다.

권고:

- 각 load의 동기화 뒤, staging 채택 전에 SR의 읽기 오류를 확인하고 비성공으로 종료한다.
- 오류 시 destination 불변과 cache·VTOR·PRIMASK 복원을 유지한다. SR 오류를 자동으로 지우는 정책은 별도로 정한다.
- PCROP 비활성을 필수 환경으로 삼는다면 이를 검사와 공개 계약에 명시한다.
- host load hook에서 **load 이후** RDERR를 설정하는 회귀시험을 추가한다. 최소 길이와 마지막 load 주입으로 비성공·출력 불변을 확인한다.

현재 시험의 `tests/ota/test_stm32_flash_read.c:78` 오류 주입은 ECCR·clock·parity를 대상으로 하며, post-load RDERR를 주입하지 않는다. `:45`, `:139`의 SR=0 assertion은 이 실패 입력을 시험한 증거가 아니다. 위 회귀시험은 권고이며 이번 리뷰에서 실행하지 않았다.

P0: 없음. 추가 P1/P2/P3: 없음.

### 공격한 시나리오와 판단

| 공격 관점 | 확인 결과 |
|---|---|
| zero·과대 length, 주소 overflow, 슬롯 끝 unaligned 읽기 | length를 먼저 제한하고 상한을 뺄셈으로 검사한다. 고정 map의 정렬을 대조했으며 새 bounds 결함은 찾지 못했다. |
| 늦은 load에서 ECCC/ECCD 발생 | 정상 결과만 최종 복사하므로 앞서 채운 staging이 부분 출력으로 노출되는 경로는 찾지 못했다. |
| 임시 VTOR 설치 직후·read 종료 직후 NMI | `armed=0`이면 fatal로 분기한다. 관련 경계 주입 시험을 읽었으나 실제 NMI timing 검증으로 해석하지 않았다. |
| VTOR가 가리키는 stack context 수명 | 초기화 후 VTOR 설치, 함수 반환 전 원 VTOR 복원 순서를 확인했다. 정상 반환 경로의 dangling context는 찾지 못했다. |
| ISR·NMI 재진입, 기존 PRIMASK=1 | IPSR/CONTROL 거절과 저장된 PRIMASK 복원을 확인했다. 단일 owner·DMA 정지는 호출자 계약이며 자동 검증되지 않는다. |
| ECCD 지연·clear 실패·동시 clock/parity 오류 | 유한 poll 또는 reset/fail-stop으로 분기한다. watchdog feed·erase·복구 정책은 NMI에 없다. |
| cache 상태와 오류 cleanup | cache disable/reset 및 원 ACR 복원 순서를 읽었다. 실제 MMIO 적용 순서·지연은 입증하지 않았다. |
| SRAM 함수·literal 외부 의존 | checker의 함수/section/relocation/branch 검사와 read용 변이시험을 검토했다. 실제 object와 최종 배치는 직접 검사하지 않았다. |
| RAM 주소 검사의 과대 해석 | 함수 시작 주소와 context/output 범위 검사는 전체 함수 extent·stack 여유·복사 완료 증명이 아니다. 이미 명시된 최종 linker/stack gate로 유지했다. |

### 실제 읽은 파일

아래 15파일은 변경 전체를 읽었다. “전체” 표기는 candidate 파일 전체도 읽었다는 뜻이다.

| 파일 | 읽은 범위 |
|---|---|
| `CMakeLists.txt` | delta 및 61–120행 |
| `docs/journal.md` | 이번 변경 delta와 diff 문맥만 |
| `docs/resume.md` | 전체 |
| `docs/tasks/T-107-stm32-mcuboot.md` | 전체 |
| `firmware/communicator/stm32/CMakeLists.txt` | delta 및 196–255행 |
| `firmware/communicator/stm32/bootloader/README.md` | 전체 |
| `firmware/communicator/stm32/interface/canview_stm_flash_read.h` | 전체 |
| `firmware/communicator/stm32/platform/stm32g474/flash_read.c` | 전체 |
| `firmware/communicator/stm32/tests/register_model.h` | 전체 |
| `tests/hil/fixtures/t103-capture-only.jsonl` | delta 전체 |
| `tests/ota/test_stm32_flash_guard.c` | 전체 |
| `tests/ota/test_stm32_flash_ram.py` | 전체 |
| `tests/ota/test_stm32_flash_read.c` | 전체 |
| `tests/test_t103_capture_helpers.py` | delta와 diff 문맥 |
| `tools/ota/check_stm32_flash_ram.py` | 전체 |

추가로 읽은 candidate 파일:

- `AGENTS.md` — 전체
- `docs/README.md` — 전체
- `docs/runbooks/agent-workflow.md` — 전체
- `docs/architecture/ota.md` — heading 목록, §5의 93–132행, §7.1의 216–230행
- `firmware/communicator/stm32/platform/stm32g474/flash_guard.c` — 전체
- `firmware/communicator/stm32/bsp/flash_layout.h` — 전체
- `firmware/communicator/stm32/bootloader/include/canview_boot_flash.h` — 전체

읽고 적용한 스킬:

- `C:/Users/digit/.codex/skills/embedded-cstyle/SKILL.md`
- `C:/Users/digit/.codex/skills/embedded-isr-design/SKILL.md`
- `C:/Users/digit/.codex/skills/embedded-driver-design/SKILL.md`

스킬은 C의 공유 상태·오류 반환, NMI 책임, register/interface 경계를 점검하는 데 사용했다. 자동 수정 지침은 읽기 전용 요청에 따라 실행하지 않았다.

### 실제 명령과 검증 구분

다음에서 `R`은 위 저장소 경로, `C`와 `B`는 위에 기록한 candidate/base 전체 hash를 뜻한다. 반복 호출은 합쳐 기록했다.

```text
git -C R cat-file -e 'C^{commit}'
git -C R cat-file -e 'B^{commit}'
git -C R rev-parse C B
git -C R rev-parse 'C^{commit}' 'B^{commit}'
git -C R diff --find-renames --stat B C
git -C R diff --find-renames B C
git -C R diff --find-renames B C -- \
  firmware/communicator/stm32/tests/register_model.h \
  tests/ota/test_stm32_flash_guard.c \
  tests/hil/fixtures/t103-capture-only.jsonl
git -C R diff --find-renames B C -- tests/test_t103_capture_helpers.py
git -C R show C:<위 목록의 show로 읽은 각 경로>
Get-Content -Raw <위 세 SKILL.md 경로>
```

`show` 출력에는 필요한 경우 다음 PowerShell 필터를 적용했다.

```powershell
ForEach-Object -Begin {$n=0} -Process {$n++; '{0,4}: {1}' -f $n,$_}
Select-Object -Skip 60 -First 60
Select-Object -Skip 195 -First 60
Select-Object -Skip 92 -First 40
Select-Object -Skip 215 -First 15
rg -n '^#'
```

보고 직전 `flash_read.c`의 84–120행과 143–187행, 공개 header 전체를 줄번호와 함께 다시 확인했다. 최초 대량 출력의 잘린 부분은 별도 `show`/경로별 diff로 보완했다.

외부 도구는 UTC 시각 조회와 ST 공식 RM0440·AN4758의 PCROP/RDERR/RDP0/예외 동작 검색·open·find에 사용했다. PDF 직접 열기는 크기 제한·timeout·404로 실패한 호출이 있었으며, 전체 PDF를 읽었다고 주장하지 않는다. finding의 규격 근거는 검색으로 반환된 **RM0440 Rev9 category 3 본문**이다. 다른 MCU의 검색 결과는 G474 동작 근거로 사용하지 않았다.

컴파일, CTest, sanitizer, coverage, objdump 및 물리 시험은 **직접 실행하지 않았다**. 전달된 Host D/R 156/156, focused 2/2, Arm 4구성, warning 0, SRAM 528/436B·외부 relocation 0, ASan/UBSan·coverage 100%는 제공된 근거로만 기록한다.

### 미검토 범위와 최종 판정

요청 범위의 새 runtime 및 15파일 delta에서 읽지 않은 부분은 없다. 기존 대형 파일의 변경 밖 본문은 위 표에 명시한 범위 외에는 읽지 않았다.

Flash backend 연결, 최종 SRAM linker/copy·전체 stack budget, boot executable/handoff, BSP identity, T-205, production provisioning, 전체 T-107/PR37 merge 검증은 범위 밖이며 미완료 상태로 유지한다. 이 미완료만으로 별도 finding을 만들지 않았다.

**Physical/HIL: NOT_RUN.** 실제 MMIO, ECC/NMI latency·예외 복귀, reset·power-cut·torn word/page와 전체 boot 안전을 승인하지 않는다.

**Source 단위 최종 verdict: BLOCK. A-01의 수정 또는 이를 배제하는 구체적 근거와 재검토가 필요하다.**
