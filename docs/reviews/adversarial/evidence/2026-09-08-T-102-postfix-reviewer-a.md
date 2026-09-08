**독립 적대적 리뷰 원문 — 판정: BLOCK**

- 실행 ID: `CV-HOSTILE-20260908-T102-POSTFIX-A-002`
- 전문 영역: STM32 embedded runtime·안전
- 시작: `2026-09-08T18:18:31.9417754+09:00`
- 검토 종료: `2026-09-08T18:22:31.8430548+09:00`
- Candidate: `eb0155e26c46fa2dc52fb64cfc11c502951a1814`
- Base: `1ebd5f575a9756fb08710021e3c9b882a8ce980f`
- 결과: **P0 0건 / P1 1건 / P2 1건 / P3 0건**
- Physical/HIL: **NOT_RUN**
- 차량 CAN TX: **NO-GO**

**격리·증거 수준**

소스와 상세 task·설계·시험은 지정된 Git 객체를 `git show`로 읽었다. branch 전환, 파일 수정, commit, push, worktree 생성, 다른 reviewer report 열람은 하지 않았다. 리뷰 아카이브 색인의 규칙과 목록은 읽었으며 연결된 report 본문은 열지 않았다.

절차상 예외를 명시한다. 첫 호출에서 `AGENTS.md`, `docs/README.md`, `docs/resume.md`는 공유 checkout의 `Get-Content`로 읽었다. 이후 후보 객체의 해당 문서 도입부도 확인했다. 따라서 모든 문서 읽기가 처음부터 object-only였다고 주장하지 않는다. 아래 finding의 코드·행 번호·시험 근거는 모두 immutable candidate 객체에서 얻었다. 공유 checkout의 clean 여부는 검사하지 않았으며 clean이라고 주장하지 않는다.

C compiler, CTest, target firmware, 기존 ELF/MAP/BIN은 실행하거나 검증하지 않았다. 아래 반례 실행은 **C 소스의 분기·루프를 옮긴 PowerShell 메모리 모델**이며 실제 C 회귀시험이나 MCU 시험이 아니다. task에 기록된 기존 116/116 및 target build 성공은 작성자 기록으로만 읽었다.

**A-002-P1-01 — 중간의 pattern byte를 free prefix 경계로 오인하여 stack health가 실패를 통과시킨다**

- 상태: `OPEN`
- 신규 변경에서 발견.
- 위치: [stack_watermark.c:55](F:/dev/canview/firmware/communicator/stm32/module/stack_watermark.c:55), 특히 61–72행.
- 영향 경로: [core_hw.c:348](F:/dev/canview/firmware/communicator/stm32/platform/stm32g474/core_hw.c:348), [core_hw.c:380](F:/dev/canview/firmware/communicator/stm32/platform/stm32g474/core_hw.c:380).
- 계약: [canview_stm_stack.h:46](F:/dev/canview/firmware/communicator/stm32/interface/canview_stm_stack.h:46).

`sample()`은 이전 minimum 경계에서 아래로 읽다가 **처음 만난 단일 `0xA5`**에서 멈추고 그 아래 전체가 미사용 prefix라고 보고한다. 그러나 사용된 stack 데이터 자체가 `0xA5`를 포함하거나, stack frame 내부에 쓰지 않은 byte가 남을 수 있다. 한 byte의 일치는 그 아래 prefix가 보존됐다는 증거가 아니다.

구체적 반례:

1. 512-byte 영역을 arm하여 모두 `0xA5`로 채운다.
2. `[64, 511]`을 사용된 영역으로 만들어 0으로 덮는다.
3. 사용된 영역의 `region[500]`만 `0xA5`로 둔다.
4. 실제 미변경 prefix는 64 bytes다.
5. 현 구현은 12 bytes만 검사하고 `current_free_bytes=501`, `minimum_free_bytes=501`, `valid=true`, `CANVIEW_OK`를 반환한다.
6. 소비자는 `501 >= 128`이므로 stack health를 통과시킨다.

실행한 소스 알고리즘 모델 결과:

```text
SOURCE_ALGORITHM_MODEL: reported_free=501 prefix_free=64 inspected=12 health_accepts=True
```

이는 전체 call-chain/WCET 증명을 하지 않는다는 문서의 제한과 별개다. API가 직접 약속한 pattern prefix를 잘못 계산하고, 그 결과가 watchdog refresh 전 health 판정에 연결된다. 급격한 overwrite를 scan budget 초과로 닫는 장치도 중간 pattern byte 하나로 우회된다.

기존 `test_platform.c` 83–103행과 `test_registers.c` 295–300행은 연속된 0 덮어쓰기만 사용한다. pattern이 섞인 사용 영역이나 구멍이 있는 stack frame은 검증하지 않는다.

권고:

- 단일 pattern byte를 근거로 미검사 prefix 전체를 `valid`로 승격하지 않는다.
- 낮은 주소부터 연속 prefix를 검증하거나, bounded 검사 진행 상태와 별도의 즉시 stack guard/MSP 경계를 결합한다.
- 검사 미완료 상태를 충분한 free 공간으로 취급하지 않는다.
- 위 512-byte 반례, sparse write, 사용 데이터의 `0xA5`, threshold 직전·직후, scan budget 초과를 실제 C 회귀시험에 추가한다.

현재 차량 송신 경로가 없다는 사실은 유지된다. 이 finding은 CAN TX 활성화 발견이 아니라 **새로 추가된 stack 안전 판정 자체의 실패**다.

**A-002-P2-01 — TIM2가 느리게 진행하는 고장을 정상 health로 인정한다**

- 상태: `OPEN`
- Base에도 존재하는 기존 결함이며 이번 변경의 신규 회귀로 분류하지 않는다.
- 위치: [core_hw.c:358](F:/dev/canview/firmware/communicator/stm32/platform/stm32g474/core_hw.c:358), 특히 362–363행.
- 소비 경로: [scheduler.c:108](F:/dev/canview/firmware/communicator/stm32/module/scheduler.c:108), [scheduler.c:118](F:/dev/canview/firmware/communicator/stm32/module/scheduler.c:118).

health 비교는 TIM2 경과가 0인지 또는 상한보다 큰지만 확인한다. 충분히 진행했는지에 대한 하한이 없다. TIM2 분주 설정이 잘못되거나 타이머가 간헐적으로 멎어도 SysTick 1 ms마다 TIM2가 1 tick만 증가하면 매번 정상으로 처리된다. `PSC`, `ARR`, APB 분주 등의 런타임 계약도 이 검사에 포함되지 않는다.

실행한 분기 모델 결과:

```text
elapsed_ms=1 elapsed_us=0    fault=True
elapsed_ms=1 elapsed_us=1    fault=False
elapsed_ms=1 elapsed_us=100  fault=False
elapsed_ms=1 elapsed_us=1000 fault=False
elapsed_ms=1 elapsed_us=2001 fault=True
```

scheduler는 동일 TIM2 값으로 callback budget과 progress deadline을 계산하므로 이러한 고장은 시간을 과소계산하게 한다. 현 image에는 health worker 하나만 있고 CAN/UART/control worker가 없어 현재의 직접적인 차량 안전 영향은 제한적이다. 이에 P2로 분류한다.

권고:

- tick 양자화·sample 위상을 고려한 누적 관찰 구간과 허용 하한을 둔다.
- TIM2 및 관련 clock 설정의 필수 불변값을 확인한다.
- 느린 진행, 간헐 정지, 분주 변경, 정상 wrap을 구분하는 named-register 회귀시험을 추가한다.
- 실제 시간 정확도는 별도 G1/G2 계측으로 검증한다.

**지정된 이전 수정의 재검토**

| 항목 | 코드·시험 검토 결과 |
|---|---|
| reset-reason serialization | `diagnostic.c` 81–82행에서 version 2와 reason byte를 기록한다. enum 범위 검사 후 기록하며, `test_platform.c` 291–305행은 8개 reason을 검사한다. 해당 누락은 소스상 수정됐다. |
| pre-IWDG HardFault reset | `core_hw.c` 412–419행에서 IWDG 준비 여부와 무관하게 `NVIC_SystemReset()`을 요청한다. `test_registers.c` 388–395행에 초기화 전 요청 검사가 있다. 해당 대기 고착 경로는 소스상 수정됐다. |
| health-fault terminal latch | `core_hw.c` 340–345행의 clock/readiness 실패도 `hardware.fault=true`로 고정한다. feed는 health를 재검사하고 fault 후 거부한다. 기존 timer 경과 실패의 latch도 유지된다. 다만 A-002-P1-01처럼 **실패를 검출하지 못하는 경로**까지 해결한 것은 아니다. |

위 판정은 실제 reset 발생, rail 동작 또는 타이머 정확도의 물리 검증을 의미하지 않는다.

**공격했으나 추가 finding을 확정하지 않은 시나리오**

- Boot: null port/callback, zero-init, 반복 시작, 각 단계 실패를 추적했다. safe → IWDG → clock → time 순서를 유지하고 첫 실패 후 다음 단계로 진행하지 않는다. 잘못된 boot enum도 재시작을 거부한다.
- 부분 초기화: LSI/IWDG 준비 실패, clock 단계 timeout, SysTick 구성 실패, MSP 범위 실패를 추적했다. main은 reset 대기로 들어가며 capability를 열지 않는다.
- Reset flags: 없음, 단일 원인, 복합 원인, watchdog 두 종류를 검토했다. 복합 원인은 `AMBIGUOUS`로 남고 raw flags도 기록한다. 실제 보드에서 발생하는 flag 조합의 의미는 미확정이다.
- NMI/CSS: feed 직전 NMI가 발생해도 중단된 feed 명령으로 돌아가지 않는 구조다. CSS clear 후 system reset을 요청한다.
- ISR: SysTick은 millisecond counter만 증가시킨다. 현재 ISR에 parsing, logging, heap, Flash 작업은 없다.
- Scheduler: worker 수 0/최대 초과, null callback, period/deadline/budget 경계, required worker 부재, BUSY, 오류 enum, late success, 재진입, unsigned wrap/backward, vote 소모와 counter 포화를 추적했다.
- Queue: 용량·record 크기 0/최대 초과, 부족한 storage, null, queue/storage overlap, full/empty, 포화 drop counter를 검토했다. push/pop은 critical port 안에서 record 전체를 복사한다. NMI/HardFault/DMA 직접 접근 금지와 single-core ownership은 문서화돼 있다.
- Pointer: 공개 API의 null 검사를 확인했다. 임의 non-null 주소의 접근 가능성이나 caller가 거짓으로 제시한 메모리 크기까지 검증한다고 주장하지 않는다.
- GPIO: 유효 A/B 포트와 pin 범위를 확인한 뒤 BSRR latch를 mode보다 먼저 쓴다. 실제 호출은 STB HIGH, FT_EN/ARM/WDI LOW, TX request HIGH로 고정된다.
- CAPTURE_ONLY: mode override 거부 header, forced include 대상, BSP anchor, capability 0/TX false, source/symbol 검사기를 읽었다. 현재 검토한 STM32 실행 경로에 raw CAN 송신·ARM 재활성화·WDI pulse 생성 호출은 발견하지 못했다. source 검사기의 정규식이 임의로 위장한 MMIO까지 증명하는 것은 아니다.
- Service policy: malformed magic/version/size와 invalid enum은 거부하고 decision을 0으로 초기화한다. trust가 모두 true여도 현재 build에서 control/TX는 열리지 않는다. Flash erase는 실행하지 않는다.
- Diagnostic ABI: 고정 40-byte little-endian 기록이며 pointer를 복사하지 않는다. 짧은 buffer, enum 범위, 32-bit 크기 초과, nonzero capability/TX 입력을 거부한다.

**실제로 읽은 파일**

다음은 후보 객체에서 읽은 파일이다. 별도 표시가 없으면 전체 내용을 읽었다.

```text
F:/dev/canview/docs/tasks/T-102-stm32-platform.md
F:/dev/canview/docs/runbooks/agent-workflow.md
F:/dev/canview/docs/reviews/README.md
F:/dev/canview/docs/hardware/r1/firmware-pinmap.md
F:/dev/canview/CMakeLists.txt
F:/dev/canview/shared/app/CMakeLists.txt
F:/dev/canview/shared/protocol/CMakeLists.txt
F:/dev/canview/tools/check_stm32_build_mode.py
F:/dev/canview/tools/check_stm32_core.py
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
F:/dev/canview/firmware/communicator/stm32/platform/stm32g474/safe_gpio.h
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
F:/dev/canview/firmware/communicator/stm32/tests/test_core.c
F:/dev/canview/firmware/communicator/stm32/tests/register_model.h
F:/dev/canview/firmware/communicator/stm32/ld/STM32G474CEUx_FLASH.ld
F:/dev/canview/firmware/communicator/stm32/docs/core-bench.md
```

추가 읽기:

- `F:/dev/canview/AGENTS.md`: checkout 전체, candidate 도입부 12행.
- `F:/dev/canview/docs/README.md`: checkout 전체, candidate 도입부 20행.
- `F:/dev/canview/docs/resume.md`: checkout 전체, candidate 도입부 8행.
- `C:/Users/digit/.codex/skills/embedded-cstyle/SKILL.md`
- `C:/Users/digit/.codex/skills/embedded-isr-design/SKILL.md`: 최초 출력과 마지막 100행 보완.
- Base의 `core_hw.c`: candidate와의 diff 및 206–260행에 해당하는 두 부분 출력.
- 요청된 `module/boot.c`는 후보에 존재하지 않았다. tree 목록으로 실제 위치 `app/boot.c`를 찾아 읽었다.

**실행 명령·결과 기록**

모든 shell 호출의 작업 디렉터리는 `F:/dev/canview`였다. 다음 기호는 실제 실행한 긴 hash와 경로를 줄여 기록하기 위한 것이다.

```powershell
$C='eb0155e26c46fa2dc52fb64cfc11c502951a1814'
$B='1ebd5f575a9756fb08710021e3c9b882a8ce980f'
$S='firmware/communicator/stm32'
```

파일별 번호 출력에는 다음 형태를 사용했다. 위 파일 목록의 candidate 전체 읽기는 모두 이 `git show` 방식으로 수행했다.

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

실행 순서와 결과:

1. `Get-Date -Format o`; `Get-Content AGENTS.md`; `Get-Content docs/README.md`; `Get-Content docs/resume.md` — 성공. 시작 시각 기록.
2. 따옴표 없는 두 `git cat-file -e <hash>^{commit}` — 각각 `unknown switch 'n'` 오류. 뒤 명령 때문에 shell 전체 exit 0으로 보였으므로 성공으로 계산하지 않았다.
3. `git rev-parse $C $B` — 두 지정 hash 출력.
4. `git diff --find-renames --stat $B $C` — 성공. 41 files changed, 1895 insertions, 68 deletions.
5. 따옴표를 적용한 `git cat-file -e '<hash>^{commit}'` 두 번 및 각각 `$LASTEXITCODE` 출력 — **둘 다 0**.
6. 두 embedded skill의 `Get-Content` — 실행 성공. 큰 도구 출력 일부가 잘려 ISR 마지막 100행을 나중에 보완했다.
7. candidate task, workflow, main, core_hw.c/h 번호 출력 — 실행 성공. 출력 절단을 보완하여 task/workflow/main을 다시 읽고 core_hw.c의 처음 90행도 별도로 출력했다.
8. safe_gpio.c, 요청된 module/boot.c, scheduler.c, queue.c 번호 출력 — boot 경로만 `does not exist` 실패. 나머지 성공.
9. `git ls-tree -r --name-only $C firmware/communicator/stm32` — 성공. 실제 `app/boot.c` 위치 확인.
10. diagnostic.c, service_policy.c, stack_watermark.c, build_metadata.c 번호 출력 — 성공.
11. 위 목록의 interface header 9개 번호 출력 — 성공.
12. app/boot.c, bsp/core.c, bsp/board.c, board_pins.h, target/module CMake, linker, core-bench 문서 번호 출력 — 성공.
13. test_platform.c 번호 출력 — 성공.
14. `git diff --find-renames $B $C -- <core_hw.c 경로> <core_hw.h 경로>` — 성공. reset serialization 관련 공급 데이터, watermark 연결, HardFault reset, health latch와 clock 계약 변경 확인.
15. test_registers.c 번호 출력 — 성공.
16. register_model.h, firmware-pinmap.md, root CMake, check_stm32_build_mode.py 번호 출력 — 성공.
17. check_stm32_core.py, test_core.c, safe_gpio.h, shared/app 및 shared/protocol CMake 번호 출력 — 성공.
18. 아래 PowerShell 반례 모델 — 성공. `501 / 64 / 12 / True` 결과와 slow-timer 분기 결과는 finding에 기록했다.
19. `git rev-parse '<C>^{commit}' '<B>^{commit}'` — 지정 hash 재확인. `Get-Date -Format o` — `18:21:32.3760901+09:00`.
20. ISR skill `Select-Object -Last 100`; candidate review index `Select-Object -First 110` — 성공. report 본문은 열지 않았다.
21. `git diff --find-renames --check $B $C -- firmware/communicator/stm32` — 출력 없음, exit 0.
22. 다음 object 검색 — 성공. GPIO safe 설정, clock 선택, capability/diagnostic 관련 위치를 반환했다.

```powershell
git grep -n -E 'FDCAN|USART|canview_stm_output|STM_ARM|WD_PULSE|tx_permit|control_capabilities' $C -- `
  firmware/communicator/stm32/app `
  firmware/communicator/stm32/bsp `
  firmware/communicator/stm32/module `
  firmware/communicator/stm32/platform
```

23. candidate AGENTS/문서 지도/resume 도입부를 각각 `git show ... | Select-Object -First 12/20/8`로 확인 — 성공.
24. base core_hw.c를 `Select-Object -Skip 225 -First 35`로 출력 — 성공. 원하는 health 조건보다 뒤에서 시작했으므로 다시 읽었다. 당시 시각 `18:22:18.8060931+09:00`.
25. base core_hw.c를 `Select-Object -Skip 205 -First 30`으로 출력 — 성공. P2 조건식이 base에도 있음을 확인.
26. 마지막 `Get-Date -Format o` — 검토 종료 시각 `18:22:31.8430548+09:00`.

반례 모델의 실행 코드:

```powershell
$region = [byte[]]::new(512)
for($i=0; $i -lt 64; $i++){ $region[$i]=0xa5 }
$region[500]=0xa5
$cursor=512
$inspected=0
$reported=0
while($cursor -gt 0 -and $inspected -lt 256){
  $cursor--
  $inspected++
  if($region[$cursor] -eq 0xa5){ $reported=$cursor+1; break }
}
$prefix=0
while($prefix -lt $region.Length -and $region[$prefix] -eq 0xa5){
  $prefix++
}
Write-Output "SOURCE_ALGORITHM_MODEL: reported_free=$reported prefix_free=$prefix inspected=$inspected health_accepts=$($reported -ge 128)"
foreach($elapsedUs in @(0,1,100,1000,2001)){
  $elapsedMs=1
  $fault=($elapsedMs -gt 20 -or $elapsedUs -eq 0 -or
          $elapsedUs -gt (($elapsedMs+1)*1000))
  Write-Output "HEALTH_PREDICATE_MODEL: elapsed_ms=$elapsedMs elapsed_us=$elapsedUs fault=$fault"
}
```

**미검토·미실행 범위와 판정 조건**

- 실제 C 반례 실행, CTest, sanitizer, coverage, target compile, ELF/MAP/stack-usage 및 CI provenance: **NOT_RUN**.
- 외부 CMSIS startup/SystemInit 구현, 최종 예외 handler disassembly, pre-IWDG 실제 HardFault, stack 파손 상태의 reset, IRQ 중첩 및 MSP guard 실제 여유: **미검증**.
- HSE/PLL/LSI 정확도, IWDG timeout, brownout/reset flags, GPIO 순간 파형, rail/pull/external gate, ST-LINK flash, PCB pin/package/symbol 대조: **Physical/HIL NOT_RUN**.
- Communicator ESP32·Diagnostic Bridge의 전체 runtime/security 구현은 이번 상세 소스 검토 밖이다. STM32 쪽에 현재 수신 parser·raw TX executor가 없음을 확인한 범위로 격리 판단을 제한한다.
- UART/FDCAN 실제 송수신, service root Flash 배치·인증·erase, OTA, 실제 차량 연결: **NOT_RUN**.
- 기존 report나 host/target 성공 기록을 물리 검증으로 승격하지 않았다.

**최종 판정은 BLOCK이다.** A-002-P1-01을 수정하거나 반례가 계약상 성립하지 않는다는 증거로 소명하고 실제 C 회귀시험 및 immutable post-fix 검토를 수행해야 한다. P2는 별도 disposition이 필요하다. 차량 CAN TX는 계속 **NO-GO**다.
