# T-102a portability Reviewer A 원본

- 도구 execution ID: `01a078d5-b50b-77e0-addc-aad5c4d7008f`
- 요청 시각: 2026-09-06 22:53:51 UTC

## 전달 요청 원문

```text
T-102a 최종 portability delta 독립 적대적 재검토. 이전 두 post-fix 원문 모두 docs/reviews/adversarial/evidence/2026-09-07-T-102a-postfix-reviewer-{a,b}.md에 확정 보존했습니다. 새 immutable candidate 5ee60bc4690b92fb90fa03e771984161ac3ad9fd, parent 6d4757c663878b30de17f076f60b98942dcdd724, base caafc242568e43a34dcf4cdf977cdb914c75290e. 전체 delta는 해당 두 원문 추가와 host tests/test_registers.c의 NMI setjmp 블록을 stage loop 밖으로 분리한 것뿐; firmware 실행코드/API/빌드/규범 불변. 원격6d run34064871690 linux GCC에서 -Werror=clobbered(stage) 컴파일실패, local GCC15.2 O3에서도 재현해 수정. 경고 억제/volatile 추가 없이 테스트 assertion/시나리오 보존 여부를 공격하고 기존 finding closure 유지 여부 확인. 새 코드 Windows Debug/Release stm32 6/6, ASanUBSan6/6, GCC O3 register actual test PASS; coverage 동일 function/line100%,scheduler99.09branch/backend96.67. 6d clean all6 finalSTMDebug4300/Release3800, ESPcomm160048/bridge155648/controller159712/fixture144816 모두warning0, build/t102a-6d4757c/artifacts.json. Firmwarecode unchanged cannot claim5ee imageembeddedversion; CI5ee all6 targetpending. Full6d hostDebug74/74+ASan74/74PASS, Release/docsongoing. Coordinator handlesfinalCI; do not waitforCI. ScopeG0/G1prep onlyT102physicalBLOCKED/nohardwareclaim. Use immutable object-only diff, detached ifrun;hash/start/end/role/clean/executedvsNOT_RUN and fullKoreanfinaloriginal required. No source edits, no reading other reviewer's THIS-round result before your final.
```

## 반환 결과 원문

판정: **CONDITIONAL** — 이번 portability delta의 코드 리뷰는 **PASS**입니다. 새 finding은 없으며, A-01(P1)·A-02(P2)의 **FIXED** 상태를 유지합니다. 전체 merge 승인은 동일 candidate의 최종 CI·검증 완료를 조건으로 합니다.

**실행 식별·격리**

| 항목 | 값 |
|---|---|
| execution ID | `T102a-A-PORTABILITY-20260907T075359-KST` |
| 역할 | A — STM32 watchdog·ISR·fail-closed timing 및 시험 의미 보존 |
| 시작 / 종료 | 2026-09-07 07:53:59 / 07:54:56 KST |
| candidate / 실제 검사 hash | `5ee60bc4690b92fb90fa03e771984161ac3ad9fd` |
| parent | `6d4757c663878b30de17f076f60b98942dcdd724` |
| base | `caafc242568e43a34dcf4cdf977cdb914c75290e` |
| candidate tree | `388e7856dcfd1f568d2af94f27c13727653fd2b2` |
| 실행 위치 | `F:/dev/canview-wt/t102a-review-a`, detached HEAD |
| 시작·종료 상태 | 지정 HEAD 일치, porcelain 출력 없음, 종료 diff 없음 |

commit-object `git show/diff`로 분석했습니다. 독립 worktree만 지정 candidate로 전환하여 시험했으며, 추적 소스·문서와 main workspace는 수정하지 않았습니다. 이번 라운드의 상대 리뷰 결과는 읽지 않았습니다.

**변경 범위 확인**

전체 delta는 기존 post-fix 리뷰 원문 두 파일 추가와 [test_registers.c:272](F:/dev/canview/firmware/communicator/stm32/tests/test_registers.c:272)의 시험 재배치뿐입니다.

firmware 실행 코드, API, linker, 빌드·CI 설정, 규범 문서, scheduler fixture, stack gate에 변경이 없음을 diff로 확인했습니다. archive 추가를 새로운 실행 검증 증거로 합산하지 않았습니다.

**공격 관점과 결과**

| 검사한 실패 가능성 | 확인 결과 |
|---|---|
| loop 축소로 CSS NMI 시나리오 삭제 | 기존 stage 5가 loop 직후 독립 블록으로 이동했으며 실행 순서 유지 |
| 이전 fault 상태가 NMI 시험에 유입 | 이동한 블록도 `healthy_boot()`로 시작하여 초기화 유지 |
| reset·CSS clear assertion 약화 | reset 1회와 CSS clear를 모두 검사하는 `&&` assertion으로 보존 |
| NMI 이후 health/feed 거부 누락 | 두 TIMEOUT assertion 모두 보존 |
| capability/TX 검사 누락 | `check_no_control()` 유지 |
| NMI가 반환해도 시험 성공 | `NMI_Handler()` 직후 `CHECK(false)` 유지 |
| feed 직전 NMI 경쟁 시험 손실 | 해당 주입·KR 미갱신 assertion은 변경 없음 |
| 경고 억제 또는 volatile로 우회 | compiler 옵션·pragma·volatile 추가 없음 |

이동된 블록은 [test_registers.c:300](F:/dev/canview/firmware/communicator/stm32/tests/test_registers.c:300)에 있습니다. 일반 고장 5개와 CSS NMI 1개의 기존 시나리오가 모두 남습니다. CSS 없는 NMI, reset mock 반환, HardFault 및 reset 대기 시험도 보존됐습니다.

loop 변수의 수명이 끝난 뒤 `setjmp` 블록을 실행하므로 해당 변수와 jump 경계의 결합이 제거됐습니다. GCC 진단을 재현했지만, 그 진단만으로 부모 코드에 실제 런타임 undefined behavior가 있었다고 확대 해석하지는 않습니다.

**직접 실행한 검증**

| 검증 | 결과 |
|---|---|
| 부모 register fixture, GCC 15.2.0 `-O3`·strict warnings·`-Werror -Wclobbered` | `stage might be clobbered` 재현, 예상대로 compile exit 1 |
| candidate fixture + 실제 backend, 동일 GCC 옵션 | warning 없이 compile 및 실제 시험 PASS |
| Windows Clang Debug STM32 CTest | 6/6 PASS |
| Windows Clang Release STM32 CTest | 6/6 PASS |
| Windows Clang coverage STM32 CTest | 6/6 PASS |
| `python -B tools/check_stm32_coverage.py` | PASS |

GCC 검증은 WSL의 Ubuntu GCC `15.2.0-16ubuntu1`을 사용한 보조 portability 검사입니다. 부모 fixture는 commit 객체에서 stdin으로 전달했고, candidate backend는 변경되지 않았음을 먼저 확인했습니다. 산출물은 독립 worktree의 ignored `build/`에만 생성했습니다.

새 profile의 coverage 결과는 다음과 같습니다.

| 대상 | line / function | branch |
|---|---:|---:|
| boot·queue | 100% / 100% | 100% |
| scheduler | 100% / 100% | 99.0909% |
| backend 모델 | 100% / 100% | 96.6667% |

**기존 finding 종결 유지**

- **A-01 — P1 / FIXED 유지:** 실제 완료 us 기준 deadline 판정과 관련 회귀시험은 변경되지 않았고 이번 STM32 시험에서도 통과했습니다.
- **A-02 — P2 / FIXED 유지:** terminal NMI reset 구현은 변경되지 않았습니다. feed 직전 NMI 주입과 비복귀·KR 미갱신 검사가 유지되어 통과했습니다.

심각도를 낮추거나 finding을 삭제하지 않습니다. 이번 delta에서 추가 P0/P1/P2/P3 finding은 발견하지 못했습니다.

**미실행·남은 gate**

이번 리뷰에서는 전체 host 74개 suite, ASan+UBSan, strict docs, mutation 실험, Arm/ESP32 target 재빌드 및 원격 CI를 실행하지 않았습니다. 사용자가 전달한 coordinator 결과는 별도 evidence이며 이번 직접 실행 결과에 합산하지 않았습니다. CI를 기다리거나 현재 성공으로 추정하지 않았습니다.

`6d4757c`의 기존 STM32/ESP 산출물과 크기를 `5ee60bc`의 embedded version 또는 최종 binary 증거로 재표기하지 않습니다. firmware 소스 불변과 candidate별 artifact 검증은 구분해야 합니다.

실제 clock/reset·brownout·PHY gate·WCET·stack watermark·UART/CAN/HIL·차량 TX는 **NOT_RUN**입니다. T-102a는 G0/G1 준비 범위이며, T-102의 물리 G1/G2 **BLOCKED**와 기존 수용 기준은 그대로 유지됩니다.
