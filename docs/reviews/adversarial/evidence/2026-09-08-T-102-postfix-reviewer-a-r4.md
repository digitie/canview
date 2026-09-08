**독립 적대적 재검토 원문 — PASS: immutable source 검토 범위**

- 실행 ID: `CV-HOSTILE-20260908-T102-POSTFIX-A-005`
- 시작: `2026-09-08T19:31:47.7414346+09:00`
- 종료: `2026-09-08T19:32:47.0639969+09:00`
- Candidate: `18941170ef475777c62db2f1471b74f937c807ea`
- Base: `1ebd5f575a9756fb08710021e3c9b882a8ce980f`
- 직전 검토 객체: `346257b7d9da27d5e9ac5092e5102483b1cb6e7f`
- 본 독립 검토 범위의 미해결 finding: **P0 0 / P1 0 / P2 0 / P3 0**
- 이전 finding: **A-004-P2-01 FIXED**, A-002 두 건 **FIXED 유지**
- Physical/HIL: **NOT_RUN**
- Vehicle CAN TX: **NO-GO**

**격리·검증 방법**

저장소 내용은 immutable Git 객체로만 확인했다. 공유 checkout 변경, 기존 artifact·시험 로그, 다른 reviewer report는 사용하지 않았다. 파일 생성·수정, branch 전환, commit, push는 수행하지 않았다.

STM32 디렉터리 전체와 관련 실행·보드 계약은 직전 검토 객체와 동일함을 `git diff --exit-code`의 빈 출력 및 exit 0으로 확인했다. 이전에 직접 읽은 동일 소스의 전체 분석을 재사용하고, watermark·TIM2 핵심 구현은 이번 객체에서 다시 읽었다.

변경된 Python 검사기는 candidate 객체에서 메모리로 읽어 **실제 함수를 실행**했다. 시험 입력의 파일 접근만 메모리 객체로 대체했다. repository 파일이나 임시 파일을 쓰지 않았으며, 이 결과를 C compiler·target 실행으로 표시하지 않는다.

**A-004-P2-01 — FIXED**

- 수정 위치: [check_stm32_core.py:61](F:/dev/canview/tools/check_stm32_core.py:61)
- 조합 회귀시험: [test_stm32_core_gate.py:56](F:/dev/canview/tests/test_stm32_core_gate.py:56), [test_stm32_core_gate.py:64](F:/dev/canview/tests/test_stm32_core_gate.py:64)

기존 실패는 주석 제거를 줄 이어붙이기보다 먼저 수행하여 다음 입력을 정상으로 인정하는 것이었다.

```c
#undef /\
* x */ CANVIEW_STM_TX_PERMIT
```

```c
void f(void) {
    FDCAN1-> /\
* x */ TXBAR = 1U;
}
```

수정본은 다음 순서를 사용한다.

```python
logical_text = _strip_c_comments(re.sub(r"\\\r?\n", "", text))
```

따라서 backslash-newline을 제거한 뒤 생성되는 주석 경계까지 인식하고, 주석 뒤의 mode directive·TX member 접근을 검사한다.

candidate 시험 파일의 AST에서 negative fixture tuple을 추출하여 candidate의 `check_source_safety()`에 직접 전달했다. 추가로 직전 두 반례의 LF/CRLF 버전과 정상 주석·이어붙인 line-comment를 검사했다.

실제 실행 결과:

```text
IMMUTABLE_SOURCE_GATE_NEGATIVE_FIXTURES 17 / 17 REJECTED
A004_ORIGINAL_COUNTEREXAMPLES_LF_CRLF 4/4 REJECTED
POSITIVE_COMMENT_AND_LINE_SPLICE_CASES 3/3 ACCEPTED
COMPILE_CONTRACT 2_VALID_UNITS_ACCEPTED 3_INVALID_CASES_REJECTED
```

17개 negative에는 직접·별칭 TX member, 일반·분리된 주석, mode define/undef와 줄 이어붙이기 조합이 포함된다. compile contract도 정상 canonical 경로와 `arguments` 형식을 인정하고, stale header 경로·이름만 포함한 가짜 조건·누락된 token을 거부했다.

**영향·권고의 종결:** 이전 source gate 누락은 위 반례와 회귀 입력에서 차단됐다. A-004의 수정 권고는 반영됐으며 추가 수정 요구는 없다. 이 검증은 모든 가능한 C 전처리 표현에 대한 완전한 parser 증명을 뜻하지 않는다.

**이전 A-002 finding — FIXED 유지**

| Finding | 재확인 |
|---|---|
| A-002-P1-01 stack watermark 오판 | [stack_watermark.c:61](F:/dev/canview/firmware/communicator/stm32/module/stack_watermark.c:61)의 낮은 주소부터 연속 prefix 검사와 256-byte 상한이 유지된다. 중간 pattern 때문에 불일치를 건너뛰는 경로가 없다. minimum은 증가하지 않는다. |
| A-002-P2-01 느린 TIM2 정상 인정 | [core_hw.c:345](F:/dev/canview/firmware/communicator/stm32/platform/stm32g474/core_hw.c:345)의 설정 검사와 [core_hw.c:385](F:/dev/canview/firmware/communicator/stm32/platform/stm32g474/core_hw.c:385)의 누적 하한이 유지된다. 개별 sample 갱신이 누적 window를 무효화하지 않는다. |

이전 watermark 반례는 64 bytes로 계산되어 `<128` health 기준에서 거부된다. TIM2의 PSC/ARR/DIER 불변값, 4 ms/50% 누적 하한, 실패 latch와 feed 재검사가 유지된다. 이번 실행에서 이 C 시험을 다시 실행하지는 않았다.

**전체 런타임 재검토 결과**

직전 검토에서 직접 읽은 소스와 이번 객체의 동일성을 확인한 뒤 다음 실패 경로를 다시 평가했다. 추가 actionable finding은 확정하지 않았다.

- **Boot·부분 초기화:** safe → IWDG → clock → time, 첫 실패 중단, 반복 시작 거부, IWDG 준비 실패 시 system reset 요청이 유지된다.
- **Watchdog·scheduler:** 신규 required vote, feed cadence, BUSY의 progress 부정, late success 전 deadline 검사, callback·전체 budget 및 terminal fault가 유지된다.
- **Reset/brownout·예외:** raw flags와 unknown/ambiguous 분류가 유지된다. pre-IWDG HardFault도 system reset을 요청하며 NMI는 중단된 feed 명령으로 복귀하지 않는다.
- **실행 ownership·재진입:** 실제 RTOS는 없고 single-main cooperative scheduler다. descriptor 복사, callback context 수명 계약, 동일 context 재진입 거부가 유지된다.
- **Queue·memory bounds:** null/zero/max, storage 크기·overlap, bounded 복사, full/empty, counter 포화와 PRIMASK 복원을 확인했다. NMI/HardFault·DMA의 queue 직접 접근은 금지돼 있다.
- **ISR/task 경계:** SysTick은 counter만 갱신한다. ISR에 parsing·logging·heap·Flash·통신 처리는 없다.
- **GPIO·전기적 가정:** latch를 mode보다 먼저 설정하고 STB HIGH, FT_EN/ARM/WDI LOW, TX request HIGH를 유지한다. reset·brownout 이전/도중 안전을 GPIO 코드만으로 확정하지 않는다.
- **Board contract:** generated pin/clock 계약과 compile-time 계산이 유지된다. 실제 package·symbol·PCB 연결을 인증한 것은 아니다.
- **Fail-safe·CAPTURE_ONLY:** capability/TX는 0/false이며 현재 raw CAN executor, ARM 상승 pulse 또는 외부 WDI 건강 pulse 생성 경로는 없다.
- **Communicator/Bridge 격리:** 관련 장치·shared 내용은 직전 객체와 동일하다. STM32에 외부 입력을 차량 송신으로 연결하는 transport/executor는 현재 없다.
- **Service·diagnostic:** Flash erase 미실행, malformed root/enum 거부, pointer 없는 40-byte version 2 기록과 reset-reason byte가 유지된다.

**실제로 읽은 파일·재사용 범위**

이번 candidate에서 읽고 실행한 파일:

```text
F:/dev/canview/tools/check_stm32_core.py
  전체 객체를 메모리에서 로드·실행, 변경 diff 및 48–65행 재독
F:/dev/canview/tests/test_stm32_core_gate.py
  전체 객체를 AST로 읽어 fixture 추출, 변경 diff 및 40–70행 재독
F:/dev/canview/firmware/communicator/stm32/module/stack_watermark.c
  43–79행 재독
F:/dev/canview/firmware/communicator/stm32/platform/stm32g474/core_hw.c
  342–410행 재독
```

전체 object diff가 동일하여 이전 직접 소스 검토를 재사용한 범위:

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

동일성 검증은 위 모든 파일을 이번 실행에서 다시 통독했다는 의미가 아니다.

**명령·결과 기록**

cwd는 `F:/dev/canview`였다. 아래 `C`, `B`, `P`는 각각 위 candidate, base, 직전 검토 객체의 전체 hash를 뜻한다.

1. `Get-Date -Format o` — 시작 시각 기록.
2. `git cat-file -e '<C>^{commit}'`, `git cat-file -e '<B>^{commit}'` — 각각 exit 0.
3. `git rev-parse '<C>^{commit}' '<B>^{commit}'` — 지정 hash 확인.
4. `git diff --find-renames --stat B C` — 성공. 46 files changed, 3287 insertions, 74 deletions.
5. `git diff --find-renames P C -- tools/check_stm32_core.py tests/test_stm32_core_gate.py firmware/communicator/stm32` — 전처리 순서 및 네 조합 fixture 추가 확인. STM32 변경 없음.
6. `git diff --find-renames --exit-code P C --`에 위 동일성 확인 범위를 전달 — 출력 없음, exit 0.
7. `@' … '@ | python -B -` — 내부 `git show C:<path>`로 검사기·시험 소스를 읽어 메모리에서 실행. negative 17/17, 원 반례 4/4 거부, positive 3/3 허용, compile contract 정상 2개·비정상 3개 확인. exit 0.
8. `git show C:<path>`와 PowerShell 행 번호·범위 필터 — 위 재독 범위 모두 성공.
9. `git diff --find-renames --check B C -- firmware/communicator/stm32 tools/check_stm32_core.py tests/test_stm32_core_gate.py` — 출력 없음, exit 0.
10. `Get-Date -Format o` — 종료 시각 기록.

**미검토·미실행 및 최종 판정**

C compiler/CTest/sanitizer/coverage, 새 target build, ELF/MAP/BIN·`.su`·CI provenance 검증은 **NOT_RUN**이다. 메모리 기반 Python 함수 시험을 전체 unittest 또는 target 검증 성공으로 간주하지 않는다.

실제 board flash, HSE/PLL/LSI·IWDG 시간, IRQ nesting·MSP guard, 물리 HardFault reset, rail/brownout, GPIO transient, 외부 pull/TX gate, PCB·package·KiCad 대조는 **Physical/HIL NOT_RUN**이다. UART/FDCAN 실통신, Flash root·OTA, 차량 통합 및 ESP32/Bridge 기존 구현 전체의 신규 전면 감사도 수행하지 않았다.

**최종 verdict: PASS — 본 immutable source 리뷰 범위.** A-004-P2-01은 실제 candidate 검사기 실행으로 닫혔으며, A-002 두 수정은 유지된다. 본 검토 범위의 미해결 P0/P1/P2/P3는 없다. 물리·target·CI gate의 별도 판정을 대체하지 않으며, 차량 CAN TX는 계속 **NO-GO**다.
