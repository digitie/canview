# T-200a post-fix Reviewer B 원문

## 전달 입력

````text
T-200a post-fix 독립 재검토. 두 reviewer에게 동일 manifest 전달. candidate 8e958c9e5d769b6cbbe7ca396ff5fee260dd7c10, base 55c780106d75d9e557150e5da28b5b1e07e6e5af (최초 review), initial task base4a6cb9f9c5feb840d6d693cc3aa92d97ceba42d5. 본인 기존 F:/dev/canview-wt/t200a-review-a 또는 -b가 clean인지 확인하고 git checkout --detach candidate로 전환. 다른 작업/파일수정/커밋 금지, ignored build만허용. 시작/종료hash+clean+UTC/실행ID를 반환. 최초원문은 모두보존완료되어 docs/reviews/adversarial/2026-09-07-T-200a.md의 4finding(A01P2 panicHALT; B01P1 uintptr docs; B02P1 factoryerase; B03P2 selfreferentialtest)을 독립 확인 가능. 자신의 finding과 전체delta 회귀를 검토: CMake 실제SDKCONFIG강제검사, 표준 uintptr nitpick예외, app100%coverage, PRINT_REBOOT/DELAY0 및21금지항목, 독립21삭제변이, generateddefaults, nested raw fence parser+tests 및 문서. 원래 severity 유지, 원finding FIXED 여부와 신규finding 상세ID/심각도/재현/근거, verdict 제출. 두 사람이번원문저장전 서로새결과공유금지. C runtime은55c이후변경없음. 선택검증: config9/pythonunit/fence4/boardgenerator, core/app/SDKhost, strictAPI29, 실제SDK 정상+factory/HALT negative. 이전full89/89은 기록유지하되 실행안한이번gate PASS라하지말것. main은 clean candidate6종target/전체host/최종CI를 병렬검증하므로 무거운alltarget중복불필요. 실제HIL/보드/차량TX NOT_RUN/NO-GO, capability0/통신OTA미시작 유지. 재검토raw를 충분한 근거와함께 최종응답으로반환; peer작업기다리지말고 본인완료시반환. 문서수정없이 review only.
역할 B: SDK/config/build/API/test independence 전문, 기존 B worktree.
````

## 반환 원문

아래는 원 응답 재전송본이다. 원 종료 시각과 판정을 변경하지 않았다.

````text
# T-200a post-fix 독립 재검토 — Reviewer B 원문

**판정: CONDITIONAL — B의 finding closure 및 delta 검토는 PASS.**

B-01/P1, B-02/P1, B-03/P2를 모두 **원 심각도를 유지한 FIXED**로 재확인했다. `55c7801..8e958c9` 검토에서 새로운 P0–P3 finding은 발견하지 않았다.

동일 candidate의 전체 host·6종 target·최종 CI와 A의 독립 판정은 별도 통합 조건이다. 실물/HIL·차량 TX gate는 해제하지 않는다.

## 1. 실행·격리 manifest

| 항목 | 기록 |
|---|---|
| executionID | `T200a-B-POSTFIX-20260907T000054Z-8e958c9` |
| 역할 | Reviewer B / Anscombe — SDK/config/build/API/test independence |
| 시작 UTC | 2026-09-07 00:00:54 |
| 종료 UTC | 2026-09-07 00:06:05 |
| candidate / 실제 검토 hash | `8e958c9e5d769b6cbbe7ca396ff5fee260dd7c10` |
| candidate parent | `1d85de5d5a10103798c09ffc1bb10946b0a98fbf` |
| candidate tree | `0b44c5d862ed009bb8150bb3e53830c904754ef2` |
| 최초 review / delta base | `55c780106d75d9e557150e5da28b5b1e07e6e5af` |
| initial task base | `4a6cb9f9c5feb840d6d693cc3aa92d97ceba42d5` |
| worktree | `F:/dev/canview-wt/t200a-review-b` |
| 전환 | 기존 `55c7801` clean 확인 후 `checkout --detach` |
| 시작·종료 상태 | 지정 HEAD 일치, porcelain 빈 출력, detached |
| 종료 실행 프로세스 | 관련 도구 프로세스 0개 |
| 보존 | worktree와 ignored build evidence 유지 |

추적 source·문서 수정, commit, push, PR 변경은 하지 않았다. HALT 실험 defaults와 실행 산출물은 ignored `build/`에만 생성했다.

보존된 최초 finding 통합 기록은 확인했지만, 상대 reviewer의 **이번 재검토 결과는 읽지 않았다**. 기존 embedded 스킬의 계층·소유권·실패 처리·문서 계약 기준을 적용했다. CodeGraph 독립 index는 사용하지 않았다.

## 2. 검토 범위와 불변 확인

19개 파일 delta의 변경 범위, 실행 코드·설정·시험·문서 변경을 검토했다.

주요 대상:

- 실제 `SDKCONFIG`를 검사하는 Communicator CMake
- `check_sdkconfig.py`, generated defaults와 board generator
- 독립 금지 목록·삭제 변이·factory/panic fixture
- `uintptr_t` 문서 설정
- app coverage 추가
- Markdown fence parser와 회귀시험
- 상세설계·task·resume·journal·review 상태 표현

전체 `.c`/`.h` 경로의 `55c7801..8e958c9` diff는 없었다. 따라서 portable core/pool, 실제 SDK adapter, BSP/app, public C API의 구현 변경은 없다. 기존 memory·동시성·capability/TX 경계를 변경하는 delta도 발견하지 않았다.

## 3. 기존 finding 재확인

### B-01 · 원 심각도 P1 · FIXED

위치: [docs/api/conf.py:16](F:/dev/canview-wt/t200a-review-b/docs/api/conf.py:16)

`uintptr_t` 하나를 기존 표준 C 타입 참조 예외에 추가했다. `nitpicky=True`, strict warning 정책, API 페이지와 추출 범위는 유지됐다.

최초 실패 명령을 재실행했다.

```powershell
python -X utf8 -B tools/build_docs.py `
  --doxygen F:/dev/canview/.tools/doxygen-1.18.0/bin/doxygen.exe
```

결과:

```text
build succeeded.
PASS: 29 public API briefs/parameters/returns
```

종료 코드 **0**, warning **0**이었다. 최초의 미해결 `uintptr_t` 참조가 해소됐으며, API 제외나 전역 warning 완화로 통과시킨 것이 아니다.

증거: [postfix-docs.log](F:/dev/canview-wt/t200a-review-b/build/postfix-docs.log)

### B-02 · 원 심각도 P1 · FIXED

위치:

- [check_sdkconfig.py:44](F:/dev/canview-wt/t200a-review-b/tools/check_sdkconfig.py:44)
- [Communicator CMakeLists.txt:10](F:/dev/canview-wt/t200a-review-b/firmware/communicator/esp32/CMakeLists.txt:10)

factory reset, OTA data erase, anti-rollback, virtual eFuse, Flash coredump 등의 금지 항목이 추가됐다.

CMake는 IDF의 `SDKCONFIG`·`PYTHON` build property를 읽어 실제 생성 설정을 검사한다. 검사 실패는 `FATAL_ERROR`로 configure를 중단한다. 검사기 파일 변경도 configure dependency에 등록했다.

**최초 재현 재실행**

최초 리뷰에서 SDK가 생성했고 당시 PASS했던 `build/idf-factory-probe/sdkconfig`를 그대로 새 검사기에 입력했다.

```text
FAIL: CONFIG_BOOTLOADER_FACTORY_RESET: bench에서 금지
exit=1
```

**실제 IDF/CMake 경로**

| 설정 | IDF reconfigure 결과 |
|---|---|
| 정상 N16R8 bench | 종료 **0**, actual sdkconfig PASS |
| 최초 factory-reset/NVS erase 설정 | 종료 **2**, configure 차단 |
| 별도 PANIC_PRINT_HALT 설정 | 종료 **2**, configure 차단 |

factory-reset 경로의 실제 출력:

```text
FAIL: CONFIG_BOOTLOADER_FACTORY_RESET: bench에서 금지
Communicator bench sdkconfig contract failed
Configuring incomplete, errors occurred!
```

단순 mock 반환이나 문법 오류가 아니라, 실제 SDK가 생성한 설정을 새 CMake gate가 거부하는 것을 확인했다. flash·NVS erase는 실행하지 않았다.

증거:

- [정상 configure](F:/dev/canview-wt/t200a-review-b/build/postfix-idf-review.log)
- [factory-reset configure](F:/dev/canview-wt/t200a-review-b/build/postfix-idf-factory-probe.log)

### B-03 · 원 심각도 P2 · FIXED

위치: [test_sdkconfig.py:26](F:/dev/canview-wt/t200a-review-b/tests/foundation/test_sdkconfig.py:26), [삭제 변이시험:94](F:/dev/canview-wt/t200a-review-b/tests/foundation/test_sdkconfig.py:94)

시험에 독립적인 `EXPECTED_FORBIDDEN` 21개가 명시됐다. 부정 시험은 구현의 `GATE.FORBIDDEN`을 시험 대상 목록으로 사용하지 않는다.

정식 config 시험 **9/9 PASS** 안에서 21개 금지 항목을 각각 제거하는 변이가 모두 검출됐다. 시험은 단순 실패뿐 아니라 assertion이 아닌 실행 오류가 없는지도 검사한다.

추가로 최초 실패 형태를 독립 재실행했다. imported gate에서 다음 항목을 각각 제거하고 부정 시험을 호출했다.

```text
CONFIG_SECURE_BOOT:
  failures=1, errors=0, AssertionError: ValueError not raised

CONFIG_BOOTLOADER_FACTORY_RESET:
  failures=1, errors=0, AssertionError: ValueError not raised

CONFIG_ESP_SYSTEM_PANIC_PRINT_HALT:
  failures=1, errors=0, AssertionError: ValueError not raised
```

최초에 살아남았던 secure-boot 금지 항목 삭제 변이는 이제 실제 assertion으로 검출된다. source 수정 없이 메모리 내 변이로 확인했다.

## 4. A-01 및 전체 delta 회귀

### A-01/P2 관련 설정 변경

B의 전체 delta 검토에서도 다음을 확인했다.

- `PRINT_REBOOT=y`, 추가 reboot delay `0` 필수
- HALT/GDBSTUB 등 거부
- generated defaults와 실제 정상 SDK 설정 일치
- debugger 연결 조건과 물리 reset 검증의 제한을 상세설계에 명시

별도 ignored defaults로 실제 HALT 구성을 생성한 결과:

```text
CONFIG_ESP_SYSTEM_PANIC_PRINT_HALT=y
# CONFIG_ESP_SYSTEM_PANIC_PRINT_REBOOT is not set
```

CMake gate는 PRINT_REBOOT 불일치, delay 설정 부재, HALT 금지를 보고하고 IDF 종료 코드 **2**로 차단했다. 지연 99초 부정 fixture도 통과했다.

이는 B의 설정 회귀 확인이며 **A-01/P2의 원 reviewer 최종 판정을 대신하지 않는다**.

증거: [HALT configure](F:/dev/canview-wt/t200a-review-b/build/postfix-postfix-halt-probe.log)

### App coverage

새 임시 profile 디렉터리에서 app의 8개 시나리오를 각각 실행하고 병합하는 것을 확인했다. 기존 portable/SDK coverage 범위는 유지됐다.

| 구현 | line / function | branch |
|---|---:|---:|
| `health.c` | 100% / 100% | 111/112, 99.11% |
| `pool.c` | 100% / 100% | 91/92, 98.91% |
| SDK `runtime.c` | 100% / 100% | 84/85, 98.82% |
| BSP `runtime.c` | 100% / 100% | branch 없음 |
| app `main.c` | **100% / 100%** | **12/12, 100%** |

### Fence parser

다음 경계를 검토하고 Windows·WSL에서 회귀시험을 실행했다.

- 4-backtick 외부 fence 안의 3-backtick 원문
- backtick/tilde 구분, 닫는 fence 길이
- 들여쓰기·LF/CRLF
- 닫히지 않은 fence와 짧은 closer
- 잘못된 backtick info string
- fence 밖 실제 broken link가 계속 검출되는지

양 환경에서 **4/4 PASS**였다. 실제 문서 검사도 **205문서 / 1,122 local targets / 오류 0**이었다.

전체 `git diff --check`는 보존된 A 최초 원문 24행의 Markdown hard-break용 trailing spaces 한 곳으로 종료 1이었다. 해당 원문을 제외한 delta는 종료 0이다. 원문 보존 요청에 따라 수정하지 않았으며 신규 코드 finding으로 분류하지 않았다.

## 5. 이번 라운드 직접 실행

| 검증 | 결과 |
|---|---|
| Windows config fixture | **9/9 PASS**, 금지 항목 삭제 변이 21개 검출 |
| Windows fence fixture | **4/4 PASS** |
| Windows Python unit discovery | **43/43 PASS** |
| board generator `--check` | PASS |
| Windows Release 선별 CTest | **24/24 PASS** |
| portable/SDK/app coverage | PASS |
| strict API | **29개 계약, warning 0, 종료 0** |
| WSL config fixture | **9/9 PASS** |
| WSL fence fixture | **4/4 PASS** |
| 정상 IDF configure | PASS |
| 실제 factory-reset / HALT configure | 의도한 gate 실패로 각각 차단 |
| document links | 오류 0 |
| task metadata/DAG | 상세 task 48개, 오류 0 |

각 실행에는 중복 포함된 시험이 있으므로 위 숫자를 합산한 고유 시험 수를 주장하지 않는다.

주요 명령:

```text
python -X utf8 -B tests/foundation/test_sdkconfig.py -v
python -X utf8 -B tests/test_document_links.py -v
python -X utf8 -B -m unittest discover -s tests -p 'test_*.py'
python -X utf8 -B tools/generate_boards.py --check

cmake --build --preset host-release
ctest --preset host-release -R
  'esp32-|foundation-bsp|foundation-python|generated-boards|python-unit|document-links|plan-validation'

cmake --build --preset host-coverage --target
  canview-esp32-core-tests canview-esp32-runtime-tests canview-esp32-app-tests
python -X utf8 -B tools/check_esp32_core_coverage.py
```

Release build는 `ninja: no work to do`였다. C 소스 불변을 확인한 뒤 기존 binary로 회귀를 재실행한 것이며, fresh 전체 compile로 표시하지 않는다.

증거: [host log](F:/dev/canview-wt/t200a-review-b/build/postfix-host.log), [coverage log](F:/dev/canview-wt/t200a-review-b/build/postfix-coverage.log)

## 6. 미실행 범위와 최종 판정

최초 `55c7801`에서 확인한 Windows Release·GCC 전체 **89/89 PASS** 기록은 유지한다. 이를 이번 `8e958c9` 전체 회귀 성공으로 옮겨 적지 않는다.

이번 라운드에서 다음은 **NOT_RUN/미확인**이다.

- 동일 candidate의 전체 host 회귀·전체 ASan/UBSan
- 6종 target 최종 build, image version·digest·warning 0
- 원격 최종 CI
- 실제 보드/HIL·watchdog reset·GPIO 파형·메모리/stack 실측
- flash, eFuse/provisioning, NVS erase, OTA, 차량 TX

이번 target 실행은 **configure 검증**이다. worktree에 남아 있는 최초 `55c7801` binary를 `8e958c9` 이미지로 주장하지 않는다.

최종 결론:

- **B-01/P1, B-02/P1, B-03/P2: FIXED — 원 심각도 유지**
- **신규 P0–P3 finding: 없음**
- **B의 finding closure·delta 재검토: PASS**
- **Merge verdict: CONDITIONAL — 동일 candidate 최종 통합 gate와 A의 독립 판정 별도**

capability/TX 0, 통신·OTA 미시작 경계는 유지된다. T-200/T-500의 실물 gate와 차량 TX NO-GO는 이 소프트웨어 재검토로 해제되지 않는다.
````

