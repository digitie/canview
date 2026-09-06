# T-102a post-fix Reviewer A 원본

- 도구 execution ID: `01a078d5-b50b-77e0-addc-aad5c4d7008f`
- 요청 시각: 2026-09-06 22:44:30 UTC

## 전달 요청 원문

```text
Post-fix 독립 재리뷰: immutable commit 6d4757c663878b30de17f076f60b98942dcdd724, initial ca1a2994258586404c4dc57302c1f2408c8fd806, base caafc242568e43a34dcf4cdf977cdb914c75290e. 두 최초 원문을 docs/reviews/adversarial/evidence/2026-09-07-T-102a-reviewer-{a,b}.md에 모두 보존했습니다. 이제 자신의 finding과 ca1a299..6d4757c 전체 delta 회귀를 검토해 주세요. 원본 finding 심각도 보존. A-01 실제 연속 us 완료 시각으로 이전 deadline 검사 후 vote반영, A-02 NMI는 system reset으로 비복귀+feed직전 host NMI주입, B-01 함수type typedef로 strict docs warning0, B-02 독립 required worker와 stale-vote 시험/두 변이 assertion실패, B-03 compile DB 15C object별 .su 존재검사(빈 const-table unit은 nm code symbol 부재를 확인). 수정된 core host function/line100%,scheduler branch99.09,backend96.67; Arm Debug4300B/Release3800B warning0 and 47CMSISconstants/15stackfiles PASS. 같은 commit 최종 host74/74·ESP4·CI는 coordinator가 지금 재실행하며 아직pending, 기다리지 말고 코드finding closure와 남은외부gate 구분하여 verdict해주세요. clean detached worktree 또는 object-only hash검증, 시작/종료·실행검증·미실행을 기록한 전체 원문 final을 반환해 주세요. 재리뷰 완료 전 서로의 재리뷰 결과는 읽지 마세요. 추적 source 수정 금지.
```

## 반환 결과 원문

판정: **CONDITIONAL** — A-01(P1), A-02(P2)는 모두 **FIXED**로 재확인했습니다. 코드 finding 종결 판정은 PASS이며, 전체 merge 판정은 동일 commit의 최종 회귀·ESP4·CI 및 리뷰어 B의 종결 확인을 조건으로 합니다.

**실행 식별·기준선**

| 항목 | 값 |
|---|---|
| execution ID | `T102a-A-POSTFIX-20260907T074437-KST` |
| 역할 | A — STM32 클록·부팅·watchdog·ISR·fail-closed timing |
| 시작 / 종료 | 2026-09-07 07:44:37 / 07:45:52 KST |
| 실제 검사 commit | `6d4757c663878b30de17f076f60b98942dcdd724` |
| 최초 candidate | `ca1a2994258586404c4dc57302c1f2408c8fd806` |
| base | `caafc242568e43a34dcf4cdf977cdb914c75290e` |
| post-fix parent | `e928cf6ecc2ed324be8ae2159e762bdaeb411b6b` |
| post-fix tree | `0c113998f250fc61988da6aa810456cdf55e74d1` |
| 격리 | `F:/dev/canview-wt/t102a-review-a`, detached HEAD |
| 시작·종료 확인 | 지정 HEAD 일치, `status --porcelain=v1` 빈 출력 |

분석은 commit-object `git show/diff`로 수행했습니다. 기존 독립 worktree를 clean 상태에서 지정 commit으로 전환하고 변경된 target을 재빌드했습니다. 추적 소스·문서와 main workspace는 수정하지 않았습니다. 독립 재현 코드는 compiler stdin으로 입력하고 산출물만 ignored `build/`에 생성했습니다. 상대 재리뷰 결과는 읽지 않았습니다.

**A-01 — P1 유지 / FIXED**

위치: [scheduler.c](F:/dev/canview/firmware/communicator/stm32/module/scheduler.c), [core 계약](F:/dev/canview/firmware/communicator/stm32/interface/canview_stm_core.h).

최초 결함은 늦은 성공이 이전 progress timestamp를 덮어써 deadline miss를 소거하고 watchdog 갱신을 허용하는 것이었습니다.

수정에서는 다음 순서를 확인했습니다.

- init 시 연속 `now_us` 기준점을 저장합니다.
- worker 시작 시 이전 완료부터의 deadline을 검사합니다.
- callback 완료 후 budget과 **이전 완료 기준 deadline을 먼저 검사**합니다.
- 검사에 성공한 경우에만 완료 timestamp와 vote를 갱신합니다.
- feed 직전 모든 필수 worker의 freshness를 다시 검사합니다.

따라서 앞선 worker의 실행시간과 현재 callback의 실행시간 모두 deadline 판정에 반영됩니다. `last_progress_ms`는 진단값으로만 남고 freshness 판정에는 사용되지 않습니다.

candidate fixture 외에 독립 harness로 다음 조합 32개를 Debug/Release library 각각에 실행했습니다.

- 필수 worker 단독 / 선택 worker 뒤 필수 worker
- 이전 완료 이후 9ms / 10ms에 dispatch
- callback 실행시간 0 / 100 / 500 / 1000µs
- 일반 timestamp / u32 wrap

두 구성 모두 32/32 PASS입니다. deadline 초과는 `DEADLINE` fault·feed 0·vote 0으로 종료되고 이전 progress가 보존됐습니다. 정확히 deadline에 완료하는 경우는 허용됐습니다. 최초의 단일 worker 및 앞선 worker 지연 결함은 재현되지 않았습니다.

추가로 새 init clock callback의 재진입 방어, backward time, callback·전체 step budget, 마지막 freshness 검사 경계를 확인했습니다. 새 회귀 finding은 없습니다.

**A-02 — P2 유지 / FIXED**

위치: [core_hw.c](F:/dev/canview/firmware/communicator/stm32/platform/stm32g474/core_hw.c), [register fixture](F:/dev/canview/firmware/communicator/stm32/tests/test_registers.c).

NMI는 fault latch와 CSS flag clear 후 `NVIC_SystemReset()`을 호출하며, 중단된 watchdog write로 복귀하지 않습니다.

실행한 host fixture는 health 검사 직후·IWDG write 직전에 NMI를 주입하고 다음을 확인합니다.

- reset 요청 1회
- IWDG KR 미갱신
- CSS flag clear
- feed 호출로 복귀하지 않음
- reset mock이 반환하더라도 fallback 대기로 진입

실제 Release ELF에서도 `NMI_Handler`가 `__NVIC_SystemReset`을 호출하고, 해당 함수가 AIRCR에 reset 요청을 쓴 뒤 비복귀 loop에 머무르는 것을 확인했습니다. 기존 `bx lr` 복귀 경로는 없습니다.

이는 소프트웨어 제어 흐름의 종결 근거입니다. 실제 보드의 NMI·system reset·외부 gate 파형은 **NOT_RUN**입니다.

**전체 delta 회귀 검토**

`ca1a299..6d4757c`의 실행 코드·API·시험·검증 gate·정본문서 변경을 검사했습니다. archive 파일은 추가 목록과 연결을 확인했으며 상대 재리뷰 결과는 열지 않았습니다.

- 함수 type typedef와 각 사용처의 명시적 포인터 변환이 일치하며, host/Arm 컴파일에서 회귀가 없었습니다.
- 독립 필수 worker의 진척 누락과 feed 이후 stale vote 재사용을 검사하는 fixture를 확인하고 실행했습니다. 두 변이 실험 자체는 이번 리뷰에서 재실행하지 않았습니다.
- stack gate는 compile database의 모든 C object에 대응하는 `.su` 존재를 검사합니다. 빈 `.su`는 object의 code symbol 부재를 확인해야 허용합니다. 관련 부정 fixture와 실제 15개 C object 검사가 통과했습니다.
- clock 상수·safe GPIO·queue 구현·CAPTURE_ONLY 경계에 권한 확대 변경은 없습니다.
- 문서는 연속 us deadline, terminal NMI, stack 검사 한계를 반영합니다. T-102의 기존 수용 기준과 물리 G1/G2 BLOCKED 상태는 유지됩니다.

B finding의 최종 종결은 원 리뷰어 B의 재확인으로 남깁니다. 이번 A 재리뷰에서 추가 P0/P1/P2/P3 finding은 없습니다.

**직접 실행한 검증**

| 검증 | 결과 |
|---|---|
| host Debug STM32 core/register CTest | 6/6 PASS |
| host Release STM32 core/register CTest | 6/6 PASS |
| host coverage STM32 core/register CTest | 6/6 PASS |
| 독립 deadline harness | Debug 32/32, Release 32/32 PASS |
| `python -B tools/check_stm32_coverage.py` | PASS |
| `python -B tests/test_stm32_core_gate.py -v` | 4/4 PASS |
| Arm Debug/Release 재빌드 | 모두 PASS, warning 0 |
| memory·symbol·개별 stack·CMSIS 상수 gate | 모두 PASS |
| Release NMI/reset 명령어 검사 | 비복귀 확인 |

Coverage는 boot·queue line/function/branch 100%, scheduler line/function 100%·branch 99.0909%, backend 모델 line/function 100%·branch 96.6667%였습니다.

| Arm 결과 | Debug | Release |
|---|---:|---:|
| FLASH 사용량 | 4300 B | 3800 B |
| data | 4 B | 4 B |
| bss + reserved stack | 8580 B | 8580 B |
| 최대 개별 stack frame | 928 B | 992 B |
| 확인한 C object `.su` | 15개 | 15개 |
| CMSIS/model 상수 | 47개 | 47개 |

BIN SHA-256:

```text
Debug   18592ca77f5d374e8c3a29dac7d35d7c7352ab707569d63cc4f7cf787fecdc95
Release 17a6673aa65629ca51e873f012de080a5adb36c73a063c09540fbdd0053f87a6
```

빌드는 이전 독립 산출물에서 변경 파일을 재컴파일한 결과이며, 완전히 새로운 build directory에서 수행한 clean build로 표기하지 않습니다.

**미실행·남은 gate**

동일 commit의 전체 host 74/74, ESP32 네 프로젝트 최종 binary, 원격 CI는 coordinator의 진행 중인 검증이며 이번 결과에 합산하지 않았습니다. 이번 리뷰에서는 전체 ASan+UBSan, strict API 문서 생성, 두 mutation 실험도 재실행하지 않았습니다.

보드 flash·클록 파형·watchdog/system reset 실측·brownout·PHY gate·IRQ latency·WCET·stack watermark·UART/CAN/HIL·OTA·provisioning·vehicle TX는 **NOT_RUN**입니다.

**A-01 P1과 A-02 P2는 심각도를 낮추지 않고 FIXED로 종결합니다.** 남은 외부 검증과 B 재확인 전 전체 merge 판정은 CONDITIONAL이며, T-102a 소프트웨어 종결이 T-102의 물리 G1/G2 완료를 의미하지 않습니다.
