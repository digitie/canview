# T-102a post-fix Reviewer B 원본

- 도구 execution ID: `01a078d5-b604-78c1-9ed2-1adc123414b4`
- 요청 시각: 2026-09-06 22:44:30 UTC

## 전달 요청 원문

```text
Post-fix 독립 재리뷰: immutable commit 6d4757c663878b30de17f076f60b98942dcdd724, initial ca1a2994258586404c4dc57302c1f2408c8fd806, base caafc242568e43a34dcf4cdf977cdb914c75290e. 두 최초 원문을 docs/reviews/adversarial/evidence/2026-09-07-T-102a-reviewer-{a,b}.md에 모두 보존했습니다. 이제 자신의 finding과 ca1a299..6d4757c 전체 delta 회귀를 검토해 주세요. 원본 finding 심각도 보존. A-01 실제 연속 us 완료 시각으로 이전 deadline 검사 후 vote반영, A-02 NMI는 system reset으로 비복귀+feed직전 host NMI주입, B-01 함수type typedef로 strict docs warning0, B-02 독립 required worker와 stale-vote 시험/두 변이 assertion실패, B-03 compile DB 15C object별 .su 존재검사(빈 const-table unit은 nm code symbol 부재를 확인). 수정된 core host function/line100%,scheduler branch99.09,backend96.67; Arm Debug4300B/Release3800B warning0 and 47CMSISconstants/15stackfiles PASS. 같은 commit 최종 host74/74·ESP4·CI는 coordinator가 지금 재실행하며 아직pending, 기다리지 말고 코드finding closure와 남은외부gate 구분하여 verdict해주세요. clean detached worktree 또는 object-only hash검증, 시작/종료·실행검증·미실행을 기록한 전체 원문 final을 반환해 주세요. 재리뷰 완료 전 서로의 재리뷰 결과는 읽지 마세요. 추적 source 수정 금지.
```

## 반환 결과 원문

판정: **CONDITIONAL** — B-01/P1, B-02/P2, B-03/P2는 모두 **FIXED**로 재확인했다. `ca1a299..6d4757c` 전체 delta에서 새로운 P0–P3 finding은 발견하지 않았다. B의 코드 재리뷰는 PASS이며, 동일 commit의 최종 통합 검증과 다른 원 reviewer의 확인은 별도 완료 조건으로 남는다.

### 실행·격리 manifest

| 항목 | 기록 |
|---|---|
| executionID | `T102a-B-POSTFIX-20260907T074439-6d4757c` |
| 시작 | 2026-09-07 07:44:39 KST |
| 종료 | 2026-09-07 07:48:38 KST |
| 역할 | B — build/test/integration, API·security 계약 |
| post-fix / 실제 검토 commit | `6d4757c663878b30de17f076f60b98942dcdd724` |
| post-fix tree | `0c113998f250fc61988da6aa810456cdf55e74d1` |
| post-fix parent | `e928cf6ecc2ed324be8ae2159e762bdaeb411b6b` |
| 최초 candidate | `ca1a2994258586404c4dc57302c1f2408c8fd806` |
| base | `caafc242568e43a34dcf4cdf977cdb914c75290e` |
| PR | [#21](https://github.com/digitie/canview/pull/21) |
| 실행 격리 | `F:/dev/canview-wt/t102a-review-b`, detached |
| 시작·종료 상태 | HEAD 일치, `status --porcelain=v1` 빈 출력 |

분석은 지정 commit 객체의 `git show/diff`, 실행 검증은 독립 worktree를 사용했다. 추적 source와 기본 workspace는 수정하지 않았다. 변이·독립 재현 프로그램은 compiler stdin으로 전달했다. 부분 누락시험에서 이동한 생성 `.su`는 즉시 복원했다.

보존된 최초 A/B 원문과 통합 기록은 이번 delta의 역사 기록으로 확인했다. 다른 reviewer의 **재리뷰 결과는 읽지 않았다**.

종료 시 관련 build/test 프로세스 0개와 clean 상태를 확인하고 리뷰 worktree 및 재생성 가능한 산출물을 제거했다. 원본 commit은 보존했다.

### 원본 finding 재확인

**B-01 · 원 심각도 P1 유지 · FIXED**

수정 위치: [canview_stm_core.h:33](https://github.com/digitie/canview/blob/6d4757c663878b30de17f076f60b98942dcdd724/firmware/communicator/stm32/interface/canview_stm_core.h#L33), `canview_stm_queue.h:15`, `app/boot.c`.

함수 pointer typedef를 함수 type typedef와 pointer 필드로 분리했다. boot의 callback 배열도 해당 표현으로 변경됐다. API 문서 입력·페이지·warning 정책은 제외하거나 완화하지 않았다.

동일한 재현 명령을 실행했다.

```powershell
python -B tools/build_docs.py --doxygen F:/dev/canview/.tools/doxygen-1.18.0/bin/doxygen.exe
```

결과: **종료 코드 0**, Doxygen XML public API 20개 계약 검사 PASS, Sphinx strict build **warning 0**. 최초의 typedef 선언 오류와 미해결 identifier 경고가 해소됐다.

**B-02 · 원 심각도 P2 유지 · FIXED**

수정 위치: [test_core.c:338](https://github.com/digitie/canview/blob/6d4757c663878b30de17f076f60b98942dcdd724/firmware/communicator/stm32/tests/test_core.c#L338).

필수 worker 두 개가 독립 fixture를 사용한다. 처음부터 하나만 BUSY인 경우와 정상 feed 이후 하나의 신규 vote가 사라지는 경우를 모두 검사한다. deadline 이전 feed 횟수와 만료 후 latched fault를 확인한다.

최초 보고서의 두 변이를 post-fix source에 각각 메모리 내 적용하여 다시 컴파일했다.

| 변이 | 재실행 결과 |
|---|---|
| 전체 required vote 비교를 `!= 0U`로 변경 | 종료 코드 1 |
| 성공한 feed 뒤 `votes = 0U` 제거 | 종료 코드 1 |

두 경우 모두 `test_core.c:356`의 다음 assertion이 실패했다.

```text
clock_worker.feeds == (after_feed != 0U && tick >= 10U ? 1U : 0U)
```

컴파일 실패가 아니라 의도한 동작 assertion으로 변이를 검출했다. 원본 수정본은 같은 fixture를 통과했다.

변이 harness 최초 실행은 source decoding의 cp949 오류로 시험 전에 중단됐으며, UTF-8을 명시한 재실행에서 위 결과를 확인했다. 제품 source 결함으로 분류하지 않는다.

**B-03 · 원 심각도 P2 유지 · FIXED**

수정 위치: [check_stm32_core.py:43](https://github.com/digitie/canview/blob/6d4757c663878b30de17f076f60b98942dcdd724/tools/check_stm32_core.py#L43), `tests/test_stm32_core_gate.py`.

검사 대상이 단순 `.su` 검색에서 compile database의 C object별 대조로 바뀌었다. 각 object의 존재·build 내부 위치·`-fstack-usage`·개별 `.su` 존재와 중복을 검사한다. 빈 `.su`는 `nm`으로 code symbol 부재를 확인하는 경우만 허용한다.

최초 재현과 동일하게 실제 Debug build의 `core_hw.c.su`만 일시적으로 제외했다.

```text
RuntimeError: 개별 stack evidence 누락: .../core_hw.c.su
Missing core_hw.c.su gate exit=1
```

복원 후 같은 ELF gate는 **15 C object stack files, 47 CMSIS/model constants PASS**였다.

Python gate 시험 4/4도 통과했다. 부분 누락, 빈 code evidence, const-table 예외, compile flag 누락, object 누락, 중복과 C 목록 부재를 확인했다. assembly·prebuilt library 제외 및 전체 stack watermark 미증명 경계는 문서에 명시되어 있다.

### 전체 delta 회귀 검토

변경된 19개 파일의 코드·시험·문서·최초 리뷰 보존물을 확인했다. 주요 구현 범위는 boot callback 표현, core/queue header, scheduler, hardware backend와 fixture, stack 검사기다. CHANGELOG·journal·resume·task·review 기록의 gate 표현도 대조했다.

**A-01 관련 변경**

[scheduler.c:118](https://github.com/digitie/canview/blob/6d4757c663878b30de17f076f60b98942dcdd724/firmware/communicator/stm32/module/scheduler.c#L118)은 이전 실제 완료 timestamp를 기준으로 시작·완료 deadline을 검사한 뒤 새 timestamp와 vote를 기록한다. init부터 연속인 us clock 사용, init callback 재진입 방어, 최종 freshness 재검사도 확인했다.

최초 실패 형태를 별도 stdin 프로그램으로 재현했다.

```text
단일 worker: us=10100, TIMEOUT, DEADLINE, feeds=0, previous_progress=0
선행 worker 지연: us=11000, TIMEOUT, DEADLINE, feeds=0, previous_progress=0
```

두 경우 모두 늦은 성공으로 이전 progress가 덮어써지지 않았다. 선행 worker가 deadline을 소진한 경우 필수 worker 실행 전에 차단됐다. 정확한 deadline 완료·u32 wrap·역행·callback 재진입 fixture도 통과했다.

**A-02 관련 변경**

`NMI_Handler()`는 fault latch·CSS clear 후 system reset을 요청한다. 실행된 register fixture는 health 성공과 KR write 사이에 NMI를 주입하고 **reset 요청 1회, KR 미갱신**을 검사한다. reset 함수가 반환하는 host fallback도 시험한다.

실제 Release ELF에서도 확인했다.

```text
NMI_Handler → __NVIC_SystemReset
__NVIC_SystemReset → AIRCR write → DSB → 무한 대기
```

중단된 feed 위치로 복귀하는 경로가 없다. target symbol 목록에 host 전용 `canview_stm_test*` hook도 없었다. 실제 MCU reset·예외 주입은 실행하지 않았다.

A finding의 원 심각도인 A-01/P1, A-02/P2를 변경하지 않는다. 위 결과는 B의 전체 delta 회귀 확인이며, A의 원 reviewer 최종 판정을 대신하지 않는다.

queue 구현·BSP·build-mode 제한·CI warning 정책은 이번 delta에서 변경되지 않았다. T-102 원래 수용 기준도 변경되지 않았고, T-102a의 G0/G1 준비 범위와 물리 G1/G2 차단이 유지된다.

### 직접 실행한 검증

기존 설치된 Windows Clang 23.1.0, CMake 4.4.3, Ninja 1.13.2, Arm GCC 15.3.1과 CubeG4 경로를 사용했다. SDK 다운로드나 coordinator 결과 대기는 하지 않았다.

| 검증 | 결과 |
|---|---|
| host Debug configure/build | PASS |
| host Debug 선별 CTest | **10/10 PASS** |
| host Release configure/build | PASS |
| host Release 선별 CTest | **10/10 PASS** |
| STM32 coverage build·새 profile 검사 | PASS |
| strict API 문서·XML 계약 | PASS, warning 0, API 20개 |
| B-02 두 변이 | 모두 의도한 assertion으로 검출 |
| 실제 `.su` 부분 누락·복원 재검사 | 누락 거부, 복원 후 PASS |
| Python STM32 gate 시험 | 4/4 PASS |
| 독립 deadline 재현 2개 | 모두 deadline fault·feed 0 |
| Arm Debug/Release build | PASS, compiler/linker warning 0 |
| ELF memory·symbol·stack·CMSIS gate | 두 구성 모두 PASS |
| Release NMI/reset disassembly | 비복귀 경로 확인 |
| 전체 delta whitespace 검사 | PASS |

선별 CTest는 `stm32-core-*` 6개, `foundation-bsp-3`, `python-unit`, `plan-validation`, `document-links`다. **전체 74/74를 이번 재리뷰에서 실행했다고 주장하지 않는다.**

Coverage:

| 파일 | line/function | branch |
|---|---:|---:|
| boot.c | 100% / 100% | 100% |
| queue.c | 100% / 100% | 100% |
| scheduler.c | 100% / 100% | 109/110, 99.09% |
| core_hw.c host model | 100% / 100% | 87/90, 96.67% |

실제 Arm 산출물:

| 항목 | Debug | Release |
|---|---:|---:|
| BIN bytes | 4,300 | 3,800 |
| text / data | 4,296 / 4 | 3,796 / 4 |
| bss와 예약 stack | 8,580 | 8,580 |
| 최대 개별 stack frame | 928 | 992 |
| 검사된 C object `.su` | 15 | 15 |
| CMSIS/model 상수 | 47 | 47 |

BIN SHA-256:

```text
Debug   18592ca77f5d374e8c3a29dac7d35d7c7352ab707569d63cc4f7cf787fecdc95
Release 17a6673aa65629ca51e873f012de080a5adb36c73a063c09540fbdd0053f87a6
```

### 남은 조건과 최종 판정

새로운 P0/P1/P2/P3 finding은 없다. **B-01/P1, B-02/P2, B-03/P2는 원 심각도를 보존한 FIXED**이며, 이 commit에 대한 B의 코드 finding closure와 delta 재리뷰는 PASS다.

다음은 이번 reviewer 실행에서 **NOT_RUN/미확인**이다.

- 동일 commit의 전체 host 74/74, ASan+UBSan·Linux portability
- ESP32 네 프로젝트 최종 binary와 원격 CI
- SDK 전체 provenance 재검증
- 실제 clock/reset/IWDG 시간·PHY·IRQ latency·WCET·stack watermark
- UART/CAN/HIL·Flash/OTA/provisioning·차량 TX

coordinator 담당 검증을 기다리거나 과거 commit의 성공으로 대체하지 않았다. 동일 commit의 필수 통합 gate와 A의 재확인이 남아 있으므로 통합 완료·merge 판정은 **CONDITIONAL**이다. T-102의 물리 G1/G2는 계속 BLOCKED이며, 이 소프트웨어 재리뷰로 닫히지 않는다.
