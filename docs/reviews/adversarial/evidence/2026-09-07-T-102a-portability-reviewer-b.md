# T-102a portability Reviewer B 원본

- 도구 execution ID: `01a078d5-b604-78c1-9ed2-1adc123414b4`
- 요청 시각: 2026-09-06 22:53:51 UTC

## 전달 요청 원문

```text
T-102a 최종 portability delta 독립 적대적 재검토. 이전 두 post-fix 원문 모두 docs/reviews/adversarial/evidence/2026-09-07-T-102a-postfix-reviewer-{a,b}.md에 확정 보존했습니다. 새 immutable candidate 5ee60bc4690b92fb90fa03e771984161ac3ad9fd, parent 6d4757c663878b30de17f076f60b98942dcdd724, base caafc242568e43a34dcf4cdf977cdb914c75290e. 전체 delta는 해당 두 원문 추가와 host tests/test_registers.c의 NMI setjmp 블록을 stage loop 밖으로 분리한 것뿐; firmware 실행코드/API/빌드/규범 불변. 원격6d run34064871690 linux GCC에서 -Werror=clobbered(stage) 컴파일실패, local GCC15.2 O3에서도 재현해 수정. 경고 억제/volatile 추가 없이 테스트 assertion/시나리오 보존 여부를 공격하고 기존 finding closure 유지 여부 확인. 새 코드 Windows Debug/Release stm32 6/6, ASanUBSan6/6, GCC O3 register actual test PASS; coverage 동일 function/line100%,scheduler99.09branch/backend96.67. 6d clean all6 finalSTMDebug4300/Release3800, ESPcomm160048/bridge155648/controller159712/fixture144816 모두warning0, build/t102a-6d4757c/artifacts.json. Firmwarecode unchanged cannot claim5ee imageembeddedversion; CI5ee all6 targetpending. Full6d hostDebug74/74+ASan74/74PASS, Release/docsongoing. Coordinator handlesfinalCI; do not waitforCI. ScopeG0/G1prep onlyT102physicalBLOCKED/nohardwareclaim. Use immutable object-only diff, detached ifrun;hash/start/end/role/clean/executedvsNOT_RUN and fullKoreanfinaloriginal required. No source edits, no reading other reviewer's THIS-round result before your final.
```

## 반환 결과 원문

# T-102a 최종 portability delta — Reviewer B 원문

판정: **portability delta PASS / 통합 완료 CONDITIONAL**.

`6d4757c..5ee60bc`에서 새로운 P0–P3 finding은 발견하지 않았다. GCC 경고를 독립 재현했고, 수정본의 strict/O3 컴파일·실행과 분리된 NMI assertion의 실패 검출을 확인했다. 기존 B-01/P1, B-02/P2, B-03/P2의 **FIXED 상태와 원 심각도를 유지**한다.

T-102 물리 G1/G2는 계속 **BLOCKED**이며, 이 판정은 T-102a의 G0/G1 소프트웨어 준비 범위에 한정한다.

## 1. 실행·격리 manifest

| 항목 | 기록 |
|---|---|
| executionID | `T102a-B-PORTABILITY-20260907T075407-5ee60bc` |
| 시작 | 2026-09-07 07:54:07 KST |
| 종료 | 2026-09-07 07:57:42 KST |
| 역할 | B — build/test/integration, API·security 계약 |
| candidate / 실제 검토 hash | `5ee60bc4690b92fb90fa03e771984161ac3ad9fd` |
| parent | `6d4757c663878b30de17f076f60b98942dcdd724` |
| candidate tree | `388e7856dcfd1f568d2af94f27c13727653fd2b2` |
| 최초 candidate | `ca1a2994258586404c4dc57302c1f2408c8fd806` |
| base | `caafc242568e43a34dcf4cdf977cdb914c75290e` |
| PR | [#21](https://github.com/digitie/canview/pull/21) |
| 실행 격리 | `F:/dev/canview-wt/t102a-review-b`, detached |
| 시작·종료 clean | HEAD 일치, `status --porcelain=v1` 빈 출력 |

분석은 지정 commit 객체의 `git show/diff`로 수행했다. 실행은 독립 detached worktree에서 수행했으며, 추적 source는 수정하지 않았다. 변이 source는 메모리에서 구성하여 compiler stdin으로 전달했다.

기본 workspace의 사용자 변경은 읽거나 수정하지 않았다. 이번 라운드 다른 reviewer의 결과는 읽지 않았다. 이전 B post-fix 보존 원문은 finding 추적에 사용했다.

종료 시 관련 실행 프로세스 0개와 clean 상태를 확인했다. 이후 리뷰 worktree와 재생성 가능한 빌드·변이 산출물을 제거했다. 원본 commit은 보존되어 있다.

## 2. 검토 파일과 변경 범위

전체 delta는 다음 세 파일뿐임을 확인했다.

- `docs/reviews/adversarial/evidence/2026-09-07-T-102a-postfix-reviewer-a.md` 추가
- `docs/reviews/adversarial/evidence/2026-09-07-T-102a-postfix-reviewer-b.md` 추가
- `firmware/communicator/stm32/tests/test_registers.c` 수정

변경 파일 외 경로를 대상으로 한 `git diff --quiet`는 종료 코드 0이었다. 따라서 firmware 실행코드, API, scheduler/queue, build·warning 정책, stack/CMSIS gate, 규범 문서와 T-102 acceptance는 parent와 동일하다.

시험 source blob:

```text
parent    fe5582e253b6ee8c5830b40b70dfbd97052083fb
candidate b5fe86bfe25df1d0cabbca352ea2b2fdc25911cd
```

`test_registers.c` 전체와 관련 `core_hw.c`, root/module/STM32 CMake, warning 정책 및 preset을 대조했다. AGENTS·문서 라우터·resume·T-102a와 기존에 확인한 T-102/설계/workflow 경계를 적용했다.

embedded 스킬은 수명·ISR·계층·문서 계약 검토 기준으로 사용했다. 사용자 요청에 따라 자동 수정이나 문서 생성은 하지 않았다.

## 3. GCC 실패 재현과 수정 확인

동일한 GCC 15.2.0에서 parent와 candidate 시험 source를 각각 commit 객체로 읽어 컴파일했다. backend는 두 commit에서 동일한 `core_hw.c`를 사용했다.

주요 옵션:

```text
-std=c99 -O3
-Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion
-Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef
-DCANVIEW_STM_REGISTER_TEST=1
```

| 대상 | 결과 |
|---|---|
| parent `6d4757c` | 컴파일 종료 1, `-Werror=clobbered` 재현 |
| candidate `5ee60bc` | 컴파일 종료 0, warning 0 |
| candidate register 실행 | 종료 0, PASS |

parent의 실제 오류:

```text
<stdin>:272:19: error: variable ‘stage’ might be clobbered
by ‘longjmp’ or ‘vfork’ [-Werror=clobbered]
```

후보 첫 시도는 병렬 configure가 출력 디렉터리를 생성하기 전에 링크하여 `build/register-5ee60bc` 출력 경로 부재로 실패했다. 디렉터리 생성 후 같은 source·옵션으로 재실행하여 통과했다. 이 실행 준비 오류를 source 결함이나 최초 성공으로 기록하지 않는다.

경고 억제, `volatile` 추가, strict 옵션 제거는 없다. 이번 증거는 GCC 진단의 재현·해소이며, parent에 실제 런타임 undefined behavior가 있었다고 추가 단정하지 않는다.

## 4. 공격한 시나리오와 assertion 보존

검토 위치: [candidate test_registers.c:272](https://github.com/digitie/canview/blob/5ee60bc4690b92fb90fa03e771984161ac3ad9fd/firmware/communicator/stm32/tests/test_registers.c#L272).

| 공격 관점 | 확인 결과 |
|---|---|
| loop 상한 6→5로 fault가 삭제되는가 | stage 0–4는 유지되고, 기존 stage 5 CSS NMI는 loop 직후 별도 실행 |
| 이전 fault 상태로 인한 거짓 성공인가 | 각 loop iteration과 분리된 CSS NMI 모두 `healthy_boot()`로 초기화 |
| NMI가 반환해도 통과하는가 | 호출 직후 `CHECK(false)` 유지 |
| reset 요청 또는 CSS clear 검사 손실인가 | 두 조건을 `&&`로 결합했으며 둘 다 참이어야 통과 |
| NMI 이후 health/feed 검사가 사라졌는가 | 두 `CANVIEW_TIMEOUT` assertion 유지 |
| capability/TX 검사 누락인가 | 분리된 블록의 `check_no_control()` 유지 |
| 다른 NMI 시나리오가 대체됐는가 | feed 직전 NMI, reset 반환 fallback, CSS 없는 NMI 시험 모두 별도 유지 |
| longjmp 이후 loop 제어 변수에 의존하는가 | 분리된 NMI 지점은 `stage`의 lexical scope 밖이며 이후 참조 없음 |

관찰 상태인 `reset_requests`, register model, jump buffer는 정적 저장기간을 가진다. 새로 분리된 블록에 longjmp 전후 값 보존을 요구하는 변경된 자동 지역변수를 추가하지 않았다.

결합된 assertion의 조건들은 관찰값 비교다. 첫 조건 실패 시 두 번째 비교를 생략해도 시험은 즉시 실패하므로 성공 판정이 약해지지 않는다.

### 독립 실패 주입

이동된 CSS NMI 블록에만 메모리 내 변이를 적용했다. 세 변이 모두 GCC strict/O3 **컴파일 성공 후 assertion 실패**로 검출됐다.

| 변이 | 실행 결과 |
|---|---|
| 해당 NMI 호출을 no-op으로 바꾸어 정상 복귀 모사 | 종료 1, `<stdin>:305: false` |
| 검사 직전 `reset_requests = 0U` 주입 | 종료 1, reset/CSS 결합 assertion |
| 검사 직전 `RCC->CICR = 0U` 주입 | 종료 1, reset/CSS 결합 assertion |

뒤의 두 변이는 한 줄 삽입으로 실패 위치가 `<stdin>:308`이며, 원본 assertion 위치는 candidate 307행이다.

이는 이동된 시험의 검출력 확인이다. 실제 MCU fault/reset 동작 검증으로 해석하지 않는다.

## 5. 기존 finding closure

| Finding | 원 심각도 | 이번 판정 | 근거 |
|---|---:|---|---|
| B-01 strict docs callback typedef 오류 | P1 | FIXED 유지 | 수정된 typedef·boot·API 문서·warning 정책 불변 |
| B-02 required worker/stale vote 시험 누락 | P2 | FIXED 유지 | 독립 fixture와 두 결함 검출 시험 불변; scheduler 시험 재통과 |
| B-03 개별 `.su` 누락 검출 실패 | P2 | FIXED 유지 | compile database 기반 gate와 gate 시험 불변 |

이 표는 이전 post-fix 실행 증거와 이번 객체 동일성·회귀 결과에 근거한다. B-01 strict docs, B-02 두 scheduler 변이, B-03 실제 `.su` 누락 시험을 **이번 라운드에 다시 실행했다고 주장하지 않는다**.

A-01/P1의 deadline 수정과 A-02/P2의 비복귀 NMI 구현도 불변이다. 특히 A-02 관련 feed 직전 NMI 시험은 변경되지 않았고 이번 register 실행에 포함됐다. A의 원 심각도나 원 reviewer 판정을 변경하지 않는다.

새로운 P0/P1/P2/P3 finding은 없다.

## 6. 이번 라운드 직접 실행

| 검증 | 결과 |
|---|---|
| immutable hash·parent·tree·변경 경로 검사 | PASS |
| Windows Clang 23.1.0 Debug configure/선별 target build | PASS, warning 0 |
| Windows Debug `stm32-core-*` | **6/6 PASS** |
| Windows Clang 23.1.0 Release configure/선별 target build | PASS, warning 0 |
| Windows Release `stm32-core-*` | **6/6 PASS** |
| WSL Ubuntu GCC 15.2.0 parent strict/O3 | 예상 clobbered 오류 재현 |
| 동일 GCC candidate strict/O3 register | PASS, warning 0 |
| 이동된 NMI 블록의 세 실패 주입 | 모두 assertion으로 검출 |
| document links | 195문서, 1,096 local targets, 오류 0 |
| plan metadata/DAG | 상세 task 47개, 오류 0 |
| 전체 delta `git diff --check` | PASS |

Windows 실행 범위:

```text
cmake --preset host-debug / host-release
cmake --build --preset <preset> --target
  canview-stm32-core-tests canview-stm32-register-tests
ctest --preset <preset> -R '^stm32-core-'
```

여섯 시험은 register model, boot, scheduler-validation, scheduler-healthy, scheduler-faults, queue다. 전체 host 74개를 실행한 것은 아니다. 문서 링크·task metadata 통과도 제품 gate 통과를 뜻하지 않는다.

## 7. NOT_RUN과 최종 판정

이번 reviewer가 실행하지 않은 항목:

- candidate 전체 host 74/74 및 ASan/UBSan
- coverage 재계측, strict Doxygen/Sphinx 재빌드
- candidate Arm/ESP target image 생성·embedded version 검사
- ELF memory/symbol/`.su`/CMSIS 상수 gate 재실행
- 원격 CI 및 SDK provenance 재검증
- 실제 clock/IWDG/reset, PHY, IRQ latency, WCET, stack watermark, HIL
- UART/CAN/Flash/OTA/provisioning/차량 TX

사용자가 전달한 coverage·sanitizer·6d target 이미지 수치는 coordinator 증거로 구분한다. 이번 독립 실행 결과로 합산하지 않았다. 특히 firmware source 불변을 이유로 **6d 이미지가 5ee embedded version을 가진다고 주장하지 않는다**.

최종 결론은 다음과 같다.

- **PASS:** `6d4757c..5ee60bc` portability delta, assertion·시나리오 보존, 기존 B finding closure 유지.
- **CONDITIONAL:** 동일 candidate의 최종 통합·CI gate 완료 판정. coordinator 결과를 기다리거나 추정하지 않았다.
- **BLOCKED 유지:** T-102 물리 G1/G2. 원래 acceptance는 변경하지 않는다.

`CAPTURE_ONLY`, capability/TX 0 경계는 유지되며, 이번 재검토는 하드웨어 안전 또는 차량 송신 허용을 승인하지 않는다.
