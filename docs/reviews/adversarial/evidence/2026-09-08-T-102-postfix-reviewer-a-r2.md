**독립 적대적 재검토 원문 — PASS: 지정된 immutable source 검토 범위**

- 실행 ID: `CV-HOSTILE-20260908-T102-POSTFIX-A-003`
- 전문 영역: STM32 embedded runtime·안전·메모리·실행 소유권
- 시작: `2026-09-08T19:02:27.3702236+09:00`
- 종료: `2026-09-08T19:05:55.4681982+09:00`
- Candidate: `6bf55ec7bedd277220ab3713ecb56351f2e7e29d`
- Base: `1ebd5f575a9756fb08710021e3c9b882a8ce980f`
- 이전 검토 candidate: `eb0155e26c46fa2dc52fb64cfc11c502951a1814`
- 미해결 finding: **P0 0 / P1 0 / P2 0 / P3 0**
- 이전 A-002 finding: **2건 모두 소스 수정 및 독립 알고리즘 검증으로 FIXED**
- Physical/HIL: **NOT_RUN**
- Vehicle CAN TX: **NO-GO**

이 PASS는 소스 리뷰 판정이다. 실제 C 회귀시험, compiler/target build, CI, 물리 gate의 실행 성공을 뜻하지 않는다.

**격리와 검증 방법**

이번 실행의 저장소 내용은 모두 지정한 commit에 대한 `git show`, `git diff`, `git grep`으로 읽었다. 공유 checkout 파일, 기존 ELF/MAP/BIN, 기존 시험 로그를 검증 근거로 사용하지 않았다. 파일 수정·생성, branch 전환, commit, push, worktree 생성은 하지 않았다.

다른 reviewer report는 읽지 않았다. diff stat에 report 파일명이 표시됐지만 본문은 열지 않았다. 본인의 이전 A-002 finding과 당시 직접 읽은 소스 문맥은 활용했다. 일부 변경 없는 참고 파일은 두 candidate 사이 `git diff --exit-code`가 0임을 확인하여 이전 직접 읽기를 재사용했다.

C 시험은 실행하지 않았다. 수정된 C의 조건과 루프를 직접 검토하고, 별도의 JavaScript 메모리 모델로 watermark 경계와 timer 누적·wrap을 확인했다. **JavaScript 모델 결과를 C 실행 결과로 표시하지 않는다.**

**이전 finding의 disposition**

| Finding | 판정 | 수정·검증 근거 |
|---|---|---|
| A-002-P1-01 stack watermark 오판 | **FIXED** | 낮은 주소부터 연속 prefix를 검사하고, 미검사 영역을 free로 승격하지 않는다. 이전 반례는 64 bytes로 계산되어 health에서 거부된다. |
| A-002-P2-01 느린 TIM2 정상 인정 | **FIXED** | PSC/ARR/DIER 불변값 검사와 4 ms 누적 진행 하한이 추가됐다. 1 µs/ms 진행은 4 ms 관찰 시 fault로 고정된다. |

**A-002-P1-01 재검증**

- 수정 위치: [stack_watermark.c:61](F:/dev/canview/firmware/communicator/stm32/module/stack_watermark.c:61)
- 소비 위치: [core_hw.c:354](F:/dev/canview/firmware/communicator/stm32/platform/stm32g474/core_hw.c:354)
- 계약: [canview_stm_stack.h:47](F:/dev/canview/firmware/communicator/stm32/interface/canview_stm_stack.h:47)
- 회귀시험 소스: [test_platform.c:95](F:/dev/canview/firmware/communicator/stm32/tests/test_platform.c:95)

기존 실패 시나리오는 512-byte 영역의 실제 미변경 prefix가 64 bytes인데, 사용 영역의 `region[500]=0xA5` 하나 때문에 501 bytes를 정상으로 보고하는 것이었다. 그 결과 128-byte stack health 기준을 잘못 통과했다.

수정본은 `scan_limit=min(region_size,256)`으로 제한하고 index 0부터 연속된 checkerboard를 검사한다. 첫 불일치에서 중단하므로 뒤쪽에 남은 pattern byte로 건너뛰지 않는다. 이전 반례의 index 64가 불일치하면 반환값은 64이며, health의 `<128` 조건에서 fault로 고정된다.

확인한 추가 경계:

- region 크기 64, 127, 128, 129, 255, 256, 512, 24,576.
- 첫 불일치가 index 0, threshold 주변, scan 경계, 영역 끝에 있는 경우.
- 첫 불일치 뒤의 우연한 checkerboard byte.
- 256 bytes보다 긴 정상 prefix의 보수적 보고.
- minimum 값이 이전보다 증가하지 않는 갱신 조건.
- null, 미arm, busy, 범위 밖 region size, minimum 값이 region size를 초과하는 상태의 거부.
- 접근 index가 항상 `region_size` 및 256 미만이라는 조건.

독립 모델은 **1,490개 경계 사례에서 `reported=min(실제 연속 prefix,256)`**을 확인했다. 시험 소스에도 이전 혼합 pattern 반례가 직접 추가됐다.

잔여 한계는 문서와 일치한다. watermark는 실제 stack 사용량·전체 call-chain·IRQ nesting·MSP 순간 최저값을 증명하지 않는다. 사용 데이터가 전체 검사 pattern과 일치하는 경우까지 보장하는 메모리 보호 장치도 아니다. 이 일반적 한계는 이전의 잘못된 prefix 탐색과 구분한다.

**A-002-P2-01 재검증**

- 수정 위치: [core_hw.c:345](F:/dev/canview/firmware/communicator/stm32/platform/stm32g474/core_hw.c:345), [core_hw.c:385](F:/dev/canview/firmware/communicator/stm32/platform/stm32g474/core_hw.c:385)
- feed 경계: [core_hw.c:400](F:/dev/canview/firmware/communicator/stm32/platform/stm32g474/core_hw.c:400)
- 회귀시험 소스: [test_registers.c:334](F:/dev/canview/firmware/communicator/stm32/tests/test_registers.c:334)

기존 실패 시나리오는 SysTick 1 ms마다 TIM2가 1 tick만 진행해도 정상으로 인정하여 scheduler의 시간 budget과 deadline을 과소계산하는 것이었다.

수정본은 다음을 검사한다.

- `PSC=159`, `ARR=UINT32_MAX`, `DIER=0`.
- 기존 sample 간 정지·역행·과대한 경과·20 ms 초과 간격.
- 4 ms 이상 누적 관찰에서 `elapsed_us >= elapsed_ms*500`.

개별 sample timestamp와 누적 window timestamp가 분리되어 있어, 매 millisecond 호출이 누적 검사를 계속 초기화하지 않는다. 실패는 `hardware.fault`에 latch되고 feed 재검사에서도 거부된다.

독립 모델 결과:

| TIM2 진행량 | 정상 시작 | ms/us wrap 근처 시작 |
|---|---|---|
| 0 µs/ms | 첫 sample에서 fault | 동일 |
| 1 µs/ms | 4 ms에서 fault | 동일 |
| 100 µs/ms | 4 ms에서 fault | 동일 |
| 499 µs/ms | 4 ms에서 fault | 동일 |
| 500 µs/ms | 32 ms 관찰 동안 허용 | 동일 |
| 1,000 µs/ms | 32 ms 관찰 동안 허용 | 동일 |
| 2,001 µs/ms | 첫 sample에서 fault | 동일 |

50% 경계의 equality는 현재 명시된 허용 하한이다. 이 검사는 정밀 clock 정확도 인증이 아니다. 실제 oscillator 편차·IRQ latency·공통 clock 고장은 물리 검증 범위로 남는다.

**전체 범위 재공격 결과**

추가 actionable finding은 확정하지 않았다.

| 공격 범위 | 확인 결과와 제한 |
|---|---|
| Boot·부분 초기화 | null callback, 반복 시작, 잘못된 state, 각 단계 오류를 추적했다. safe → IWDG → clock → time 순서와 첫 실패 중단이 유지된다. |
| 초기화 timeout | readiness/settle polling은 유한 횟수다. IWDG 준비 실패 시 main의 reset 대기 함수가 system reset을 요청한다. 실제 timeout 시간은 미계측이다. |
| Reset/brownout | raw reset flags 보존, 단일 원인 분류, 복합 원인 `AMBIGUOUS`, unknown 상태를 확인했다. 실제 reset의 flag 조합은 물리 시험으로 확인해야 한다. |
| HardFault/NMI | [core_hw.c:418](F:/dev/canview/firmware/communicator/stm32/platform/stm32g474/core_hw.c:418)과 434행은 system reset을 요청한다. pre-IWDG HardFault의 단순 영구 대기 회귀는 없다. NMI는 중단된 feed 명령으로 복귀하지 않는다. |
| Fault latch | clock/readiness, stack, timer sample 및 누적 검사 실패가 모두 latch된다. 정상 register 복원만으로 feed 권한이 되살아나지 않는다. |
| Watchdog·scheduler | required 신규 vote, 10 ms feed cadence, BUSY의 progress 부정, late success 전 deadline 검사, callback budget, 전체 budget, 재진입과 fault 후 재시도를 추적했다. |
| RTOS·실행 소유권 | 실제 RTOS는 연결되지 않았다. scheduler는 single-main cooperative owner다. descriptor를 복사하며 callback context 수명은 caller 책임이다. ISR 호출·동일 context 설정 변경은 금지돼 있다. |
| Queue | null/zero/max, 부족한 storage, overlap, full/empty, drop 포화와 index wrap을 검토했다. 동기 복사와 필수 critical port가 유지된다. NMI/HardFault 및 DMA 직접 storage 접근은 금지다. |
| Critical section | 이전 PRIMASK 저장·복원과 barrier가 유지된다. 이미 mask된 호출이 leave에서 임의 enable되지 않는다. |
| ISR 경계 | SysTick은 counter 증가만 수행한다. 현재 ISR에 parsing, logging, 동적 할당, Flash 또는 통신 작업이 없다. |
| 메모리 | static context, bounded array/loop, linker stack window 및 heap 0 설정을 확인했다. 임의 non-null 주소·caller의 거짓 크기·메모리 파손 전반을 API가 검증한다고 보지 않았다. |
| GPIO | BSRR latch 후 output mode 설정이다. STB HIGH, FT_EN/ARM/WDI LOW, CAN TX request HIGH를 유지한다. reset 이전·brownout 중 전기적 안전은 외부 회로에 의존한다. |
| Board contract | generated STM32 pin/clock 값과 firmware pinmap을 대조했다. HSE 16 MHz, SYSCLK 160 MHz, PCLK/FDCAN 80 MHz, BRR20 계산이 일치한다. package·KiCad·실물 연결을 재인증한 것은 아니다. |
| CAPTURE_ONLY | mode override 거부, capability 0/TX false, BSP anchor를 확인했다. 이번 변경은 shared C library에도 forced include를 적용하고 compile database 검사를 추가한다. |
| 직접 송신 우회 | 검토한 실행 경로에 FDCAN 송신 API, raw frame executor, ARM 상승 pulse, WDI 건강 pulse 생성은 발견하지 못했다. 정규식 source gate만으로 임의로 위장한 MMIO까지 증명한다고 보지 않는다. |
| Communicator/Bridge 격리 | Base 대비 해당 ESP32·Bridge·Controller 디렉터리의 변경은 board hardware digest macro 추가뿐이다. shared 소스 변경은 없다. STM32에는 현재 외부 요청을 받아 차량 송신하는 transport/executor가 연결되지 않았다. |
| Service root | malformed magic/version/size/enum은 거부된다. 모든 trust 값이 true여도 현재 build는 control/TX를 허용하지 않는다. 실제 Flash read/write/erase는 없다. |
| Diagnostic record | 40-byte little-endian, version 2, byte 5 reset reason이 유지된다. pointer를 wire로 복사하지 않으며 enum·capacity·크기 overflow·TX/capability 입력 검사를 확인했다. |

**이번 실행에서 실제로 읽은 파일**

다음 파일은 candidate 객체에서 본문을 읽었다.

```text
F:/dev/canview/AGENTS.md
F:/dev/canview/docs/README.md
F:/dev/canview/docs/resume.md
F:/dev/canview/docs/tasks/T-102-stm32-platform.md
F:/dev/canview/docs/hardware/r1/firmware-pinmap.md
F:/dev/canview/firmware/communicator/stm32/CMakeLists.txt
F:/dev/canview/firmware/communicator/stm32/app/main.c
F:/dev/canview/firmware/communicator/stm32/app/boot.c
F:/dev/canview/firmware/communicator/stm32/bsp/board.c
F:/dev/canview/firmware/communicator/stm32/bsp/core.c
F:/dev/canview/firmware/communicator/stm32/bsp/board_pins.h
F:/dev/canview/firmware/communicator/stm32/bsp/build_metadata.c
F:/dev/canview/firmware/communicator/stm32/platform/stm32g474/core_hw.c
F:/dev/canview/firmware/communicator/stm32/platform/stm32g474/core_hw.h
F:/dev/canview/firmware/communicator/stm32/platform/stm32g474/safe_gpio.c
F:/dev/canview/firmware/communicator/stm32/platform/stm32g474/syscalls.c
F:/dev/canview/firmware/communicator/stm32/module/CMakeLists.txt
F:/dev/canview/firmware/communicator/stm32/module/scheduler.c
F:/dev/canview/firmware/communicator/stm32/module/queue.c
F:/dev/canview/firmware/communicator/stm32/module/diagnostic.c
F:/dev/canview/firmware/communicator/stm32/module/service_policy.c
F:/dev/canview/firmware/communicator/stm32/module/stack_watermark.c
F:/dev/canview/firmware/communicator/stm32/interface/canview_build_mode.h
F:/dev/canview/firmware/communicator/stm32/interface/canview_stm_build.h
F:/dev/canview/firmware/communicator/stm32/interface/canview_stm_diagnostic.h
F:/dev/canview/firmware/communicator/stm32/interface/canview_stm_reset.h
F:/dev/canview/firmware/communicator/stm32/interface/canview_stm_stack.h
F:/dev/canview/firmware/communicator/stm32/interface/canview_stm_service.h
F:/dev/canview/firmware/communicator/stm32/interface/canview_stm_core.h
F:/dev/canview/firmware/communicator/stm32/interface/canview_stm_queue.h
F:/dev/canview/firmware/communicator/stm32/interface/canview_stm_board_core.h
F:/dev/canview/firmware/communicator/stm32/tests/test_platform.c
F:/dev/canview/firmware/communicator/stm32/tests/test_registers.c
F:/dev/canview/firmware/communicator/stm32/tests/register_model.h
F:/dev/canview/firmware/communicator/stm32/ld/STM32G474CEUx_FLASH.ld
F:/dev/canview/firmware/communicator/stm32/docs/core-bench.md
F:/dev/canview/tools/check_stm32_core.py
```

문서 묶음의 큰 출력에서 `resume.md` 역사 부분 일부가 잘렸다. 현재 상태와 task 범위는 확인했으며, 잘린 역사 부분을 이번 검증 근거로 사용하지 않았다.

다음은 변경 부분만 읽었다.

```text
F:/dev/canview/firmware/communicator/stm32/README.md
F:/dev/canview/tests/test_stm32_core_gate.py
F:/dev/canview/.github/workflows/foundation.yml
F:/dev/canview/tools/validate_document_links.py
F:/dev/canview/firmware/communicator/esp32/bsp/board_pins.h
F:/dev/canview/firmware/diagnostic-bridge/bsp/board_pins.h
F:/dev/canview/firmware/controller/bsp/board_pins.h
```

다음은 이전 실행에서 직접 읽었으며, 이번에 이전 candidate와 동일함을 object diff로 확인했다.

```text
F:/dev/canview/firmware/communicator/stm32/tests/test_core.c
F:/dev/canview/docs/runbooks/agent-workflow.md
F:/dev/canview/CMakeLists.txt
F:/dev/canview/shared/app/CMakeLists.txt
F:/dev/canview/shared/protocol/CMakeLists.txt
F:/dev/canview/tools/check_stm32_build_mode.py
```

**명령·결과 기록**

모든 shell 명령의 cwd는 `F:/dev/canview`였다. 아래 기호는 실제 hash를 줄여 기록한 것이다.

```powershell
$C='6bf55ec7bedd277220ab3713ecb56351f2e7e29d'
$B='1ebd5f575a9756fb08710021e3c9b882a8ce980f'
$P='eb0155e26c46fa2dc52fb64cfc11c502951a1814'
```

1. `Get-Date -Format o` — 시작 시각 기록.
2. `git cat-file -e '<C>^{commit}'`, `git cat-file -e '<B>^{commit}'` 및 각각 `$LASTEXITCODE` 출력 — 둘 다 0.
3. `git rev-parse '<C>^{commit}' '<B>^{commit}'` — 지정 hash와 일치.
4. `git diff --find-renames --stat $B $C` — 성공. 44 files changed, 2633 insertions, 74 deletions.
5. `git diff --find-renames $P $C -- firmware/communicator/stm32` — 성공. 두 finding 수정, tests, 문서, shared compile 계약 변경 확인.
6. `git show "${rev}:$p"` 반복 — AGENTS, 문서 지도, resume, 상세 task 읽기.
7. `git diff --find-renames $P $C -- tools/check_stm32_core.py tests/test_stm32_core_gate.py .github/workflows/foundation.yml tools/validate_document_links.py` — 성공. 검사기·negative fixture와 주변 변경 확인.
8. 다음 번호 출력 명령을 위 본문 읽기 목록의 나머지 candidate 파일에 적용 — 전부 성공.

```powershell
foreach($p in $paths){
  Write-Output "FILE $p"
  $n=0
  git show "${rev}:$p" | ForEach-Object {
    $n++
    '{0}: {1}' -f $n,$_
  }
}
```

9. `git diff --find-renames --exit-code $P $C --` 뒤에 위 “이전 읽기 재사용” 목록의 여섯 repository-relative 경로를 전달 — 출력 없음, exit 0.
10. 독립 JavaScript 메모리 모델 실행 — watermark 1,490 cases 성공. timer rate 7개 × 정상/wrap 시작 2개 결과가 각각 일치. 실제 C 실행은 아님.
11. `git diff --find-renames --check $B $C -- firmware/communicator/stm32 tools/check_stm32_core.py tests/test_stm32_core_gate.py` — 출력 없음, exit 0.
12. `git diff --find-renames $B $C -- firmware/communicator/esp32 firmware/diagnostic-bridge firmware/controller shared` — 세 board header의 digest macro 추가만 출력. shared 변경 없음.
13. 다음 object 검색 — 성공. safe GPIO, clock 선택, capability/diagnostic 위치를 반환했다.

```powershell
git grep -n -E 'FDCAN|USART|canview_stm_output|STM_ARM|WD_PULSE|malloc|calloc|realloc|tx_permit|control_capabilities' $C -- `
  firmware/communicator/stm32/app `
  firmware/communicator/stm32/bsp `
  firmware/communicator/stm32/module `
  firmware/communicator/stm32/platform
```

14. `git rev-parse '<C>^{commit}' '<B>^{commit}'` — 종료 전 hash 재확인.
15. `Get-Date -Format o` — 종료 시각 기록.

**미검토·미실행 범위**

- 실제 C 회귀시험, CTest, ASan/UBSan, coverage, compiler negative fixture 실행: **NOT_RUN**.
- 새 target compile, ELF/MAP/BIN/compile database·`.su` 대조, CI artifact provenance: **NOT_RUN**.
- 외부 CMSIS startup/SystemInit 본문과 최종 machine code, stack 파손 상태의 exception entry/reset, IRQ nesting·64-byte arm guard의 실제 여유: **미검증**.
- HSE/PLL/LSI, IWDG reset 시간, timer 정확도, GPIO transient, rail/brownout, 외부 TX gate·pull, MCU option byte, ST-LINK flash: **Physical/HIL NOT_RUN**.
- PCB·package·KiCad symbol/netlist의 전기적 재검증: **NOT_RUN**.
- ESP32/Bridge 전체 기존 runtime·security 구현은 재감사하지 않았다. 해당 디렉터리의 이번 delta와 STM32의 통신·송신 미연결 경계를 확인한 범위다.
- UART/FDCAN 실통신, service root Flash 배치·인증·erase, OTA, 차량 통합: **NOT_RUN**.

**최종 verdict: PASS — immutable source 리뷰 범위.** 이전 두 finding의 실패 원인은 수정됐고, 독립 정적·알고리즘 검증에서 이를 재확인했다. 이번 검토 범위에서 미해결 P0/P1/P2/P3는 없다. 실행하지 않은 host/target/CI·물리 gate는 별도로 남으며, 차량 CAN TX는 계속 **NO-GO**다.
