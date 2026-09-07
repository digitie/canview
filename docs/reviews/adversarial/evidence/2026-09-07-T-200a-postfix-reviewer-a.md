# T-200a post-fix Reviewer A 원문

## 전달 입력

````text
T-200a post-fix 독립 재검토. 두 reviewer에게 동일 manifest 전달. candidate 8e958c9e5d769b6cbbe7ca396ff5fee260dd7c10, base 55c780106d75d9e557150e5da28b5b1e07e6e5af (최초 review), initial task base4a6cb9f9c5feb840d6d693cc3aa92d97ceba42d5. 본인 기존 F:/dev/canview-wt/t200a-review-a 또는 -b가 clean인지 확인하고 git checkout --detach candidate로 전환. 다른 작업/파일수정/커밋 금지, ignored build만허용. 시작/종료hash+clean+UTC/실행ID를 반환. 최초원문은 모두보존완료되어 docs/reviews/adversarial/2026-09-07-T-200a.md의 4finding(A01P2 panicHALT; B01P1 uintptr docs; B02P1 factoryerase; B03P2 selfreferentialtest)을 독립 확인 가능. 자신의 finding과 전체delta 회귀를 검토: CMake 실제SDKCONFIG강제검사, 표준 uintptr nitpick예외, app100%coverage, PRINT_REBOOT/DELAY0 및21금지항목, 독립21삭제변이, generateddefaults, nested raw fence parser+tests 및 문서. 원래 severity 유지, 원finding FIXED 여부와 신규finding 상세ID/심각도/재현/근거, verdict 제출. 두 사람이번원문저장전 서로새결과공유금지. C runtime은55c이후변경없음. 선택검증: config9/pythonunit/fence4/boardgenerator, core/app/SDKhost, strictAPI29, 실제SDK 정상+factory/HALT negative. 이전full89/89은 기록유지하되 실행안한이번gate PASS라하지말것. main은 clean candidate6종target/전체host/최종CI를 병렬검증하므로 무거운alltarget중복불필요. 실제HIL/보드/차량TX NOT_RUN/NO-GO, capability0/통신OTA미시작 유지. 재검토raw를 충분한 근거와함께 최종응답으로반환; peer작업기다리지말고 본인완료시반환. 문서수정없이 review only.
역할 A: embedded runtime/RTOS/watchdog 안전 전문, 기존 A worktree.
````

## 반환 원문

아래는 원 응답 재전송본이다. 원 종료 시각과 판정을 변경하지 않았다.

````text
# T-200a post-fix 독립 재검토 — Reviewer A

**코드·설정 수정 판정: PASS. Merge 판정: CONDITIONAL.**

기존 A-01은 **P2 심각도를 유지한 채 FIXED**로 확인했다. B의 기존 세 finding도 수정과 관련 회귀시험을 독립 확인했다. 신규 P0–P3 finding은 없다.

최종 candidate의 전체 host·6종 target·CI 통합 gate는 별도 확인 대상이다. 실제 보드/HIL은 NOT_RUN, 차량 TX는 NO-GO를 유지한다.

## 1. 실행 및 격리 manifest

| 항목 | 기록 |
|---|---|
| 실행 ID | `T200a-A-postfix-20260907T000053Z-8e958c9` |
| 역할 | Reviewer A — embedded runtime/RTOS/watchdog 안전 |
| 시작 UTC | `2026-09-07 00:00:53` |
| 종료 UTC | `2026-09-07 00:08:03` |
| 전환 전 HEAD | `55c780106d75d9e557150e5da28b5b1e07e6e5af` |
| Candidate / 실제 검사·종료 HEAD | `8e958c9e5d769b6cbbe7ca396ff5fee260dd7c10` |
| Review base | `55c780106d75d9e557150e5da28b5b1e07e6e5af` |
| Initial task base | `4a6cb9f9c5feb840d6d693cc3aa92d97ceba42d5` |
| Candidate 실제 parent | `1d85de5d5a10103798c09ffc1bb10946b0a98fbf` |
| Candidate tree | `0b44c5d862ed009bb8150bb3e53830c904754ef2` |
| Worktree | `F:/dev/canview-wt/t200a-review-a` |
| 상태 | 전환 전 clean, candidate 전환 후 및 종료 시 clean·detached |
| SDK | ESP-IDF6.0.3, `76f5dedd9950a3012fee8fb7d5586df21fc67802` |

기존 worktree의 clean 상태를 확인하고 요청대로 `git checkout --detach`로 전환했다. 종료 시 candidate 대비 diff와 porcelain status 출력이 없었다.

추적 파일 수정·commit·push·main 변경은 하지 않았다. 재현 입력과 산출물만 ignored `build/`에 생성했다. 이번 상대 reviewer 결과는 읽지 않았으며, 상대 작업을 기다리지 않았다. worktree는 유지한다.

## 2. 검사 범위

`55c7801..8e958c9`의 변경 19개 파일을 기준으로 다음을 검토했다.

- 실제 SDKCONFIG를 검사하는 Communicator CMake 연결.
- panic 정책, 비휘발성 변경 금지 항목, generated defaults.
- 독립 config fixture와 금지 항목 삭제 변이시험.
- app coverage 실행·집계 추가.
- `uintptr_t` API 문서 예외.
- Markdown fence parser와 회귀시험.
- task·상세 설계·기존 finding 통합 기록·검증 기록의 범위 구분.

`git diff --exit-code 55c7801 HEAD -- '*.c' '*.h'`로 **C와 header 변경이 없음을 확인**했다. 최초 runtime 분석을 새 실행코드 변경 검토처럼 재포장하지 않았다.

AGENTS, README/resume, task, 상세 설계와 관련 runbook을 참조했다. `embedded-architecture`, `embedded-cstyle`, `embedded-documentation`, `embedded-rtos-design` 스킬을 적용했다. CodeGraph는 사용하지 않았다.

## 3. 기존 finding disposition

원래 ID와 심각도를 유지한다. B 항목의 원 reviewer 최종 확인은 B의 독립 결과와 별개다.

| Finding | 기존 심각도 | A의 재검토 결과 | 근거 |
|---|---:|---|---|
| T200a-A-01 — panic HALT 허용 | P2 | **FIXED** | PRINT_REBOOT·delay0 필수, 다른 panic 정책 차단, 실제 SDK HALT configure 거부 |
| T200a-B-01 — `uintptr_t` strict API 실패 | P1 | **FIXED 확인** | 표준 타입 하나만 기존 예외에 추가, strict API 29개·warning0 |
| T200a-B-02 — factory reset/NVS erase 허용 | P1 | **FIXED 확인** | 금지 설정 추가, 실제 SDKCONFIG CMake 검사, 실제 factory/NVS 설정 configure 거부 |
| T200a-B-03 — 자기참조 금지 목록 시험 | P2 | **FIXED 확인** | 독립 21항목 oracle, 각 삭제 변이가 해당 assertion으로 실패함을 재확인 |

### A-01: 기존 실패 시나리오 차단 확인

[check_sdkconfig.py:27](F:/dev/canview-wt/t200a-review-a/tools/check_sdkconfig.py:27)에서 다음을 필수로 검사한다.

```text
CONFIG_ESP_SYSTEM_PANIC_PRINT_REBOOT=y
CONFIG_ESP_SYSTEM_PANIC_REBOOT_DELAY_SECONDS=0
```

HALT/GDBSTUB/SILENT_REBOOT/runtime GDBSTUB도 금지한다. [generate_boards.py:165](F:/dev/canview-wt/t200a-review-a/tools/generate_boards.py:165)의 생성 정책과 일치한다.

설치 SDK로 생성한 실제 HALT sdkconfig에는 HALT=y, PRINT_REBOOT unset이 반영됐다. CMake 실행 결과:

```text
FAIL: CONFIG_ESP_SYSTEM_PANIC_PRINT_REBOOT: expected y, found n;
CONFIG_ESP_SYSTEM_PANIC_REBOOT_DELAY_SECONDS: expected 0, found MISSING;
CONFIG_ESP_SYSTEM_PANIC_PRINT_HALT: ...
Communicator bench sdkconfig contract failed
CASE=halt EXIT=1
```

지연99초와 GDBSTUB 부정 입력도 config 시험에서 거부됐다. debugger 연결 중 watchdog/reset 동작을 별도 조건으로 명시한 문서는 적절하다. 이는 실제 reset 시간 계측을 대신하지 않는다.

### B-02: 위험한 실제 설정의 빌드 진입 차단 확인

[Communicator CMakeLists.txt:10](F:/dev/canview-wt/t200a-review-a/firmware/communicator/esp32/CMakeLists.txt:10)은 IDF의 SDKCONFIG/PYTHON 속성을 얻어 검사기를 실행하고 실패 시 configure를 중단한다. 검사기 변경도 재구성 의존성으로 등록한다. SDK 자체의 sdkconfig 변경 추적도 확인했다.

실제 SDK에서 다음 설정을 생성했다.

```text
CONFIG_BOOTLOADER_FACTORY_RESET=y
CONFIG_BOOTLOADER_NUM_PIN_FACTORY_RESET=18
CONFIG_BOOTLOADER_DATA_FACTORY_RESET="nvs"
```

결과는 해당 factory-reset 항목을 이유로 **configure exit 1**이었다. 단순 구문 오류나 toolchain 오류로 실패한 것이 아니다.

SDK bootloader 원문에서도 이 설정이 GPIO 조건에 따른 data partition erase 경로를 활성화함을 확인했다. 따라서 관련 P1의 원래 영향은 유지하면서, 수정된 gate가 그 경로의 설정을 차단함을 확인했다.

### B-03: 변이가 시험 오류가 아닌 assertion으로 검출됨

[test_sdkconfig.py:26](F:/dev/canview-wt/t200a-review-a/tests/foundation/test_sdkconfig.py:26)의 기대 목록은 구현 `FORBIDDEN`에서 생성하지 않는다.

추가 재확인에서는 21개 항목을 하나씩 제거하고 각각 다음을 요구했다.

- failure가 정확히 1개.
- test error는 0개.
- 실패한 subtest가 제거한 항목과 일치.

결과:

```text
forbidden removal mutants killed 21 / 21
each exactly one intended assertion failure, zero test errors
```

## 4. 이번 직접 실행 결과

| 검사 | 결과 |
|---|---|
| Config unit | **9/9 PASS**, 21개 삭제 변이 포함 |
| Python unit discovery | **43/43 PASS** |
| Fence 전용 회귀 | **4/4 PASS** |
| Board generator `--check` | PASS |
| 문서 link 검사 | 205문서·1122 local target, errors=0 |
| Strict API | **29개 계약 PASS, warning0** |
| Windows Release 관련 CTest | **23/23 PASS** |
| 새 Windows Debug coverage 빌드 | 성공, 컴파일 경고 관측 0 |
| 실제 SDK 정상 configure | 최종 재실행 exit 0, CMake gate PASS |
| 실제 SDK HALT configure | 예상한 gate 거부, exit 1 |
| 실제 SDK factory/NVS configure | 예상한 gate 거부, exit 1 |

Release는 candidate에서 재구성한 기존 build를 사용했고 Ninja는 `no work to do`였다. C 입력이 동일하다는 확인과 별도로, coverage target들은 새 `build/postfix-coverage`에서 직접 컴파일했다.

주요 명령:

```text
python -B tests/foundation/test_sdkconfig.py -v
python -B -m unittest discover -s tests -p "test_*.py"
python -B tests/test_document_links.py -v
python -B tools/generate_boards.py --check
python -B tools/validate_document_links.py
python -B tools/build_docs.py --doxygen <Doxygen1.18.0>/doxygen.exe
```

```text
ctest --test-dir build/review --output-on-failure \
  -R "esp32-|foundation-bsp|foundation-python|generated-boards|python-unit|document-links"

python -B tools/check_esp32_core_coverage.py --build build/postfix-coverage
```

Coverage는 새 profile로 집계했다.

| 대상 | Function / Line | Branch |
|---|---:|---:|
| Health | 100% / 100% | 111/112, 99.11% |
| Pool | 100% / 100% | 91/92, 98.91% |
| SDK adapter | 100% / 100% | 84/85, 98.82% |
| BSP runtime | 100% / 100% | 분기 없음 |
| 실제 app composition | **100% / 100%** | **12/12, 100%** |

산출물은 `build/postfix-coverage/esp32-coverage-66lf_sl9`에 있다. app 8개 시나리오를 별도 실행하고 fresh directory의 profile을 병합하므로 과거 profile을 성공 근거로 재사용하지 않는다.

실제 SDK 검사는 `build/postfix-idf`와 별도 SDKCONFIG 파일을 사용했다. HALT/factory 입력은 `build/postfix-config-inputs/`에만 생성했다. 최종적으로 정상 SDKCONFIG로 configure를 다시 성공시켰다.

### 실행 중 환경 문제

숨기지 않고 구분한다.

- 공용 IDF export는 미설치 `riscv32-esp-elf-gdb` 때문에 실패했다. 설치를 추가하지 않고 기존 ESP32-S3 도구 경로를 사용했다.
- 첫 정상 configure는 `ESP_ROM_ELF_DIR` 누락으로 GDB 초기화 파일 생성 경고가 있었다. 설치된 ROM ELF 경로를 지정한 최종 재실행은 traceback/CMake warning 없이 성공했다.
- `NVS_ENCRYPTION`이 현재 선택 component에서 unknown이라는 Kconfig NOTE는 남았다. 이를 감추거나 전체 target warning0 증거로 해석하지 않았다.
- HALT에서는 PRINT_REBOOT 해제에 따른 delay 설정 비가시성 NOTE도 발생했다. 최종 거부 이유는 의도한 config gate였다.

## 5. 전체 delta 회귀 공격 결과

- **Defaults만 고정하고 실제 설정이 우회하는 경우:** 실제 SDKCONFIG를 CMake에서 검사함을 실행 확인했다.
- **금지 목록과 시험이 함께 약화되는 경우:** 독립 oracle 및 21개 삭제 변이로 차단했다.
- **API warning을 광범위하게 숨기는 경우:** 기존 표준 타입 목록에 `uintptr_t`만 추가했고 strict 모드를 유지한다.
- **App coverage가 일부 정상 경로만 집계하는 경우:** 8개 실패·정상 시나리오와 새 profile로 전체 app 분기를 확인했다.
- **원문 내부 짧은 fence가 외부 fence를 닫아 live-link 검사를 혼동시키는 경우:** 문자·길이·닫기 조건, CRLF, 미종결 fence와 fence 뒤 live link 회귀가 통과했다.
- **설정 수정이 runtime 권한을 확장하는 경우:** C/header delta가 없고 capability0, RUN_OK LOW, 통신/OTA 미시작 계약이 유지된다.

신규 P0/P1/P2/P3 finding은 확인하지 못했다.

## 6. 남은 gate와 최종 판정

이번에 직접 실행하지 않은 항목:

- Candidate 전체 Windows Debug/Release·GCC·ASan/UBSan 회귀.
- Candidate 6종 target binary 빌드·warning scan·artifact digest 검증.
- 최종 CI.
- 실물 Flash/PSRAM·heap/stack 장시간 측정, GPIO/reset/brownout, watchdog reset 시간·critical WCET, HIL 및 차량 시험.

최초 `55c7801`에서 제가 실행한 **Release 89/89 PASS는 그대로 유효한 과거 기록**이다. 이를 이번 `8e958c9` 전체 회귀 PASS로 표시하지 않는다.

**A-01: P2 FIXED. 기존 네 수정에 대한 A의 독립 확인: PASS. 신규 finding: 없음.**

**Merge: CONDITIONAL** — 동일 최종 candidate의 통합 검증 evidence와 양 reviewer의 원문·disposition 확정이 필요하다. peer 결과는 기다리거나 참조하지 않았다.

T-200a는 G0/G1 준비만 충족하는 범위다. 실제 T-200/T-500 gate는 NOT_RUN/BLOCKED, 차량 TX는 NO-GO를 유지한다.
````

