**독립 적대적 재검토 원문 — CONDITIONAL**

- 실행 ID: `CV-HOSTILE-20260908-T102-POSTFIX-A-004`
- 시작: `2026-09-08T19:25:35.0607648+09:00`
- 종료: `2026-09-08T19:27:19.2180498+09:00`
- Candidate: `346257b7d9da27d5e9ac5092e5102483b1cb6e7f`
- Base: `1ebd5f575a9756fb08710021e3c9b882a8ce980f`
- 직전 검토 객체: `6bf55ec7bedd277220ab3713ecb56351f2e7e29d`
- 미해결 finding: **P0 0 / P1 0 / P2 1 / P3 0**
- 이전 A-002 finding: **두 건 모두 FIXED 유지**
- Physical/HIL: **NOT_RUN**
- Vehicle CAN TX: **NO-GO**

**격리와 검토 방식**

지정한 Git 객체만 읽었다. 공유 checkout 소스·기존 build artifact·시험 로그·다른 reviewer report는 근거로 사용하지 않았다. 파일 생성·수정, branch 전환, commit, push는 하지 않았다.

직전 객체와 이번 candidate 사이에서 **STM32 디렉터리 전체 및 관련 실행·보드 계약이 동일함**을 `git diff --exit-code`의 빈 출력과 exit 0으로 확인했다. 따라서 앞서 직접 읽었던 전체 런타임 소스의 분석을 동일한 객체 내용에 적용하고, 두 수정의 핵심 코드와 회귀시험을 이번 candidate에서 다시 읽었다. 이번에 변경된 source 검사기는 별도로 전체를 읽고 메모리 입력으로 직접 실행했다.

**A-004-P2-01 — C 전처리 순서가 뒤바뀌어 금지된 TX 접근·mode 재정의를 놓친다**

- 상태: **OPEN**
- 위치: [check_stm32_core.py:58](F:/dev/canview/tools/check_stm32_core.py:58)
- 관련 시험: [test_stm32_core_gate.py:48](F:/dev/canview/tests/test_stm32_core_gate.py:48)
- 심각도: **P2**

현재 검사기는 다음 순서로 입력을 정규화한다.

```python
logical_text = re.sub(r"\\\r?\n", "", _strip_c_comments(text))
```

즉, 주석 제거를 먼저 하고 backslash-newline을 나중에 제거한다. C 전처리는 줄 이어붙이기 후 주석을 처리하므로, 줄 이어붙이기로 생성되는 주석 경계를 현 검사기는 놓친다. 이 순서는 GCC 공식 전처리 설명에서도 확인된다. [GCC Initial processing](https://gcc.gnu.org/onlinedocs/gcc-7.4.0/cpp/Initial-processing.html)

재현 입력 1:

```c
#undef /\
* x */ CANVIEW_STM_TX_PERMIT
```

재현 입력 2:

```c
void f(void) {
    FDCAN1-> /\
* x */ TXBAR = 1U;
}
```

줄 이어붙이기 이후 두 입력은 각각 정상적인 주석을 포함한 `#undef`와 TX register 접근이다. 그러나 현 검사기는 주석 제거 시점에 `/`와 `*`가 붙어 있지 않아 주석을 인식하지 못한다. 나중에 줄을 붙인 결과에는 `/* x */`가 그대로 남아 금지 패턴 일치를 방해한다.

**실행 증거**

candidate의 Python 소스를 `git show`로 메모리에 읽어 실행했다. 실제 `check_source_safety()`에 파일시스템 대신 메모리 file/root 객체를 제공한 결과:

```text
mode check_source_safety_return=True
tx check_source_safety_return=True
```

동일 source의 정규화 helper와 정규식을 비교한 결과:

```text
mode_spliced_comment candidate_detected=False splice_first_detected=True
tx_spliced_comment candidate_detected=False splice_first_detected=True
ordinary_comment candidate_detected=True splice_first_detected=True
```

이는 검사기 함수의 실제 실행 결과다. C compiler나 STM32를 실행한 결과는 아니다.

**영향**

이번 candidate의 런타임에서 실제 차량 송신 경로를 발견한 것은 아니다. 다만 source gate가 금지한다고 명시한 register 접근과 mode 변경을 정상으로 인정할 수 있어, 향후 변경에 대한 검증 신뢰성이 깨진다. 현재 CAPTURE_ONLY 런타임의 직접적인 fail-open이 확인되지 않아 P2로 분류한다.

**권고**

- backslash-newline 제거를 먼저 수행하고, 그 결과에 주석 처리를 적용한다.
- 위 두 반례와 이어붙인 line-comment를 회귀시험에 추가한다.
- 정상 주석·문자열·문자 literal의 처리도 함께 확인한다.
- 수정 후 해당 검사기의 negative fixture를 다시 실행한다.

기존 시험은 일반 주석과 줄 이어붙이기를 각각 검사하지만, **줄 이어붙이기로 주석 경계가 생성되는 조합**은 검사하지 않는다.

**이전 finding 재확인**

| Finding | 이번 판정 | 근거 |
|---|---|---|
| A-002-P1-01 watermark 중간 pattern 오판 | **FIXED 유지** | [stack_watermark.c:61](F:/dev/canview/firmware/communicator/stm32/module/stack_watermark.c:61)는 낮은 주소부터 연속 checkerboard prefix를 검사한다. 이전의 index 500 pattern 때문에 501 bytes로 뛰는 경로가 없다. |
| A-002-P2-01 느린 TIM2 정상 인정 | **FIXED 유지** | [core_hw.c:345](F:/dev/canview/firmware/communicator/stm32/platform/stm32g474/core_hw.c:345)의 PSC/ARR/DIER 검사와 [core_hw.c:385](F:/dev/canview/firmware/communicator/stm32/platform/stm32g474/core_hw.c:385)의 누적 하한이 그대로 유지된다. |

watermark는 `min(region_size,256)` 범위만 읽고 첫 불일치에서 멈춘다. 이전 반례는 64 bytes로 계산되며 health의 `<128` 조건에서 거부된다. minimum은 증가하지 않는다. [test_platform.c:95](F:/dev/canview/firmware/communicator/stm32/tests/test_platform.c:95)에 혼합 pattern 회귀시험이 유지된다.

TIM2는 개별 sample과 누적 window timestamp가 분리돼 있어 매 tick 호출이 누적 검사를 무효화하지 않는다. 4 ms에 4 µs만 진행하는 반례와 PSC/ARR/DIER 변조 시험도 [test_registers.c:334](F:/dev/canview/firmware/communicator/stm32/tests/test_registers.c:334)에 유지된다. 실패는 latch되고 feed에서 재검사한다.

이번 실행에서 C 회귀시험을 다시 실행하지 않았다. 직전 실행의 독립 알고리즘 분석과 **이번 객체의 동일성 확인·핵심 코드 재독**을 근거로 수정 유지 여부를 판정했다.

**전체 STM32 범위의 재공격 결론**

다음 경계에서 추가 finding은 확정하지 않았다. 전체 런타임이 직전 검토 객체와 동일함을 확인한 결과를 포함한다.

- **Boot·부분 초기화:** safe → IWDG → clock → time 순서, 첫 실패 중단, 반복 시작 거부가 유지된다.
- **Watchdog·timeout:** required 신규 vote, feed cadence, BUSY의 progress 부정, deadline·callback budget·전체 budget 및 terminal fault가 유지된다.
- **Reset/brownout:** raw flags와 보수적인 unknown/ambiguous 분류가 유지된다. 실제 flag 조합은 미검증이다.
- **HardFault/NMI:** IWDG 시작 전에도 system reset을 요청한다. NMI가 중단된 feed 명령으로 복귀하는 경로는 없다.
- **실행 ownership·재진입:** 실제 RTOS는 없으며 single-main cooperative scheduler다. descriptor 복사, callback 수명 계약, scheduler 재진입 거부가 유지된다.
- **Queue·메모리:** null/zero/max, storage 크기·overlap, bounded 복사, full/empty, counter 포화, PRIMASK 복원을 재검토했다. NMI/HardFault와 DMA의 queue 직접 접근은 금지돼 있다.
- **ISR:** SysTick은 counter 증가만 수행한다. parsing·logging·heap·Flash·통신 처리가 없다.
- **GPIO·보드:** latch를 output mode보다 먼저 설정하며 STB HIGH, FT_EN/ARM/WDI LOW, TX request HIGH를 유지한다. reset/brownout 중 안전을 GPIO 코드만으로 보장하지 않는다.
- **CAPTURE_ONLY:** 현재 runtime의 capability/TX 결과는 0/false다. raw CAN executor나 ARM/WDI 활성화 경로는 발견하지 못했다. source gate의 새 누락은 위 P2로 별도 기록했다.
- **Communicator/Bridge 격리:** 직전 객체 대비 관련 장치와 shared 내용은 동일하다. STM32에 외부 요청을 차량 송신으로 연결하는 transport/executor는 현재 없다.
- **Service·diagnostic:** Flash erase 미실행, malformed root/enum 거부, 40-byte version 2 record, reset-reason byte와 pointer 비직렬화가 유지된다.

**이번에 실제 읽은 파일**

candidate 객체에서 다시 읽은 본문:

```text
F:/dev/canview/firmware/communicator/stm32/module/stack_watermark.c
  전체 1–79행
F:/dev/canview/firmware/communicator/stm32/platform/stm32g474/core_hw.c
  294–458행
F:/dev/canview/firmware/communicator/stm32/tests/test_platform.c
  51–124행
F:/dev/canview/firmware/communicator/stm32/tests/test_registers.c
  322–359행
F:/dev/canview/tools/check_stm32_core.py
  전체 1–284행, immutable 소스를 메모리에서 실행
F:/dev/canview/tests/test_stm32_core_gate.py
  변경 diff 및 1–100행
```

전체 내용의 동일성을 확인하여 이전 직접 소스 검토를 재사용한 범위:

```text
F:/dev/canview/firmware/communicator/stm32/
F:/dev/canview/AGENTS.md
F:/dev/canview/docs/README.md
F:/dev/canview/docs/resume.md
F:/dev/canview/docs/tasks/T-102-stm32-platform.md
F:/dev/canview/docs/runbooks/agent-workflow.md
F:/dev/canview/docs/hardware/r1/firmware-pinmap.md
F:/dev/canview/CMakeLists.txt
F:/dev/canview/shared/
F:/dev/canview/firmware/communicator/esp32/
F:/dev/canview/firmware/diagnostic-bridge/
F:/dev/canview/firmware/controller/
```

동일성 확인은 해당 디렉터리의 모든 파일을 이번 실행에서 다시 읽었다는 뜻이 아니다.

**실행 명령·결과**

cwd는 `F:/dev/canview`였다. 아래 `C`, `B`, `P`는 각각 이번 candidate, base, 직전 검토 객체의 위 전체 hash를 뜻한다.

1. `Get-Date -Format o` — 시작 시각 기록.
2. `git cat-file -e '<C>^{commit}'`, `git cat-file -e '<B>^{commit}'` — 각각 exit 0.
3. `git rev-parse '<C>^{commit}' '<B>^{commit}'` — 지정 hash 확인.
4. `git diff --find-renames --stat B C` — 성공. 46 files changed, 3280 insertions, 74 deletions.
5. `git diff --find-renames P C -- firmware/communicator/stm32 tools/check_stm32_core.py tests/test_stm32_core_gate.py CMakeLists.txt docs/tasks/T-102-stm32-platform.md` — 검사기·검사기 시험 변경만 출력.
6. `git diff --find-renames --exit-code P C --`에 위 동일성 확인 범위를 전달 — 출력 없음, exit 0.
7. `git show C:<path>`와 PowerShell 행 번호 출력·범위 필터 — 위 본문 읽기 목록 모두 성공.
8. `@' ... '@ | python -B -` 두 번 — 각각 immutable 검사기 helper/정규식 비교와 실제 `check_source_safety()` 메모리 입력 재현. 두 금지 입력이 허용되는 결과 확인. 파일 쓰기 없음.
9. `git diff --find-renames --check B C -- firmware/communicator/stm32 tools/check_stm32_core.py tests/test_stm32_core_gate.py` — 출력 없음, exit 0.
10. GCC 공식 최신 경로 직접 열기 — HTTP 502 실패. 공식 GCC 문서 검색으로 초기 처리 순서 확인.
11. `git rev-parse '<C>^{commit}'` — 종료 전 candidate 재확인.
12. `Get-Date -Format o` — 종료 시각 기록.

**미검토·미실행 및 최종 판정**

C compiler/CTest/sanitizer/coverage, target build, ELF/MAP/BIN·`.su`·CI provenance 검증은 **NOT_RUN**이다. Python 검사기 재현을 이러한 gate의 성공으로 간주하지 않는다.

실제 board flash, clock·IWDG 시간, IRQ nesting·MSP guard, HardFault 물리 reset, rail/brownout, GPIO transient, 외부 pull/TX gate, PCB·package·KiCad 대조는 **Physical/HIL NOT_RUN**이다. ESP32/Bridge 전체 기존 runtime의 새 전면 감사, UART/FDCAN 실통신, Flash root·OTA·차량 통합도 수행하지 않았다.

**최종 verdict는 CONDITIONAL이다.** 이전 A-002 두 finding은 닫힌 상태를 유지하지만, 이번 candidate의 **A-004-P2-01이 OPEN**이므로 사용자 지정 PASS 조건을 충족하지 않는다. 차량 CAN TX는 계속 **NO-GO**다.
