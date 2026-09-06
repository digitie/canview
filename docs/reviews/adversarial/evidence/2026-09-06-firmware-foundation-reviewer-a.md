# 기반 코드 Reviewer A 원본 evidence

## Post-fix 재검토 전달 요청 원문

```text
Post-fix 독립 재검토. 원본 두 보고서를 docs/reviews/adversarial/evidence/2026-09-06-firmware-foundation-reviewer-{a,b}.md에 각각 보존한 뒤 코디네이터가 비교했습니다. 이번 검토에서는 상대 원문을 볼 필요 없습니다.
같은 immutable post-fix: b084720ff73da2ad95460a1c947478e62c63d35d, 최초 candidate: 59ac4046a02c3bb4d2612f4143d650ff4095f5b2. 이 둘의 전체 delta 회귀와 자기 finding closure를 확인하세요. 새 detached worktree 아래에서 시작/종료 HEAD 및 git status --porcelain=v1 clean, UTC 시각을 기록하세요. tracked 수정·commit 금지. old worktree와 main checkout은 도구/기존 본인 experiment 읽기 용도로만. initial manifest 범위/미실행 target/HIL 제한 유지. 다른 review/journal 과거본문 탐색 불필요.
변경: PLAY16/REC14+정본문서/fixture; SoC+모듈 GPIO allowlist(JSON/CSV, noninteger reject); main C99 startup/BSP와 SDK GNU canview_esp32_platform component 분리; startup4역할 host object gate. CAN roundtrip 전체 필드+Python/C208 batch byte golden 추가. prior CI locale -X utf8 및 GPIO와 무관한 기존 brightness assertion 9U->9 경고수정 포함. core runtime codec은 변경 없음.
작성자 post-fix Windows Debug/Release31/31, generator7그룹, envelope/COBS2195+CAN208, coverage546/546·295/296, strictAPI14 PASS. CAN bus상수0/offset오류/encoder+decoder delta endian동시반전 3mutant를 각각 compile해 새 golden이 모두 검출함. mutant파일 F:/dev/canview/build/foundation-mutation/ 읽기 가능. 이전 beae8a9 CI34009610099 Windows+LinuxGCC/ASanUBSan PASS, 최신postfix CI는 코디네이터가 별도로 확인.
도구는 기존 manifest maincheckout pinned paths를 사용하며 bootstrap 대용량 재다운로드 금지. 재검토 결과는 한글 최종 메시지에 full 원문으로 전달: execution ID/UTC/commitclean,각 finding FIXED 여부+증거,전체delta회귀/새finding,실행명령·실패포함 실제결과/미실행,최종 PASS/CONDITIONAL/BLOCK. 별도 파일에 쓴 경우 정확한 경로를 명시해 주세요.
Reviewer A: F:/dev/canview-wt/recheck-foundation-a. A-01/A-02/A-03 closure와 target CMake/부팅안전 변화 중점. 실제SDK빌드없이는 target PASS 금지.
```


- Subagent 실행 ID: `01a074c6-29ad-7be3-876e-4d30954faaca`
- 원본 재전달: 후속 상태 메시지로 원문이 wait 결과에서 가려져 동일 원문 재전달을 요청했다. 아래 원문은 요약하지 않았다.

## 전달 요청 원문

```text
Review manifest — canview firmware foundation
Candidate: 59ac4046a02c3bb4d2612f4143d650ff4095f5b2
Base/parent: b529a722fb813d5b60ab667675d73996895ea3fc
Request: 전체 기반/구조 코드, MCU 독립 protocol, 보드별 설정, C99 warning0, 넓은 unit test와 상세 생성 API 문서. 상세 기능은 추후 agent가 구현.
Scope: git diff base candidate 93파일. 새 shared app/interface/protocol, transport schema/generator, 4MCU app/BSP/platform/IDF+STM CMake, board config generator/partition templates, host tests/CI/tools/docs/ADR008.
Outside current implementation: full ABI catalog freeze/auth/session/ACK/QoS/filter, FDCAN/UART driver/ISR/RTOS, real OTA/LVGL/audio/vehicle features, actual hardware/target/HIL. These are explicitly unimplemented; assess whether foundation safely and truthfully isolates them.
Authority/acceptance: AGENTS.md, docs/README.md, docs/resume.md, docs/runbooks/agent-workflow.md §5, docs/architecture/firmware-foundation.md, ADR008, related exact protocol/hardware source. User C99 request overrides generic C11 skill. No Hallmark.
Author tests: pinned Windows Clang23.1.0 GNU C99/CMake4.4.3/Ninja1.13.2 Debug+Release each31CTest PASS. core9groups line546/546 functions20/20 branches295/296. 2195 Python/C vectors, BSP4 mocks with fault each step, generator5 unittest groups and legacy12 tests. Doxygen1.18.0/Sphinx9.1.0/Breathe4.36.0/Furo site strict build and14public-function XML contracts pass. Host success is not MCU/HIL success.
Target VerifyOnly after host activation fails missing arm-none-eabi-gcc; SDK/realboards not available. Do not flash, start radio, use CAN, erase, provision.
Isolation: assigned detached worktree only, same immutable candidate. Confirm HEAD and empty git status --porcelain=v1 at START and END, record UTC times. No tracked edits/commits. Build artifacts only under worktree build/ or .tools/ ignored. Do not read other review report/findings or create other agents.
Tools already installed read-only in main checkout (do not redownload 900MB LLVM in review):
F:/dev/canview/.tools/llvm-23.1.0/clang+llvm-23.1.0-x86_64-pc-windows-msvc/bin
F:/dev/canview/.tools/cmake-4.4.3/bin/cmake-4.4.3-windows-x86_64/bin
F:/dev/canview/.tools/ninja-1.13.2/bin
F:/dev/canview/.tools/doxygen-1.18.0/bin
C:/Python314/python.exe
F:/dev/canview/.tools/docs-venv/Scripts/python.exe
Load VS DevShell (C:/Program Files (x86)/Microsoft Visual Studio/18/BuildTools/Common7/Tools/Launch-VsDevShell.ps1 -Arch amd64 -HostArch amd64 -SkipAutomaticLocation), prepend the tool paths. In your worktree: cmake --preset host-debug -DPython3_EXECUTABLE=C:/Python314/python.exe; cmake --build --preset host-debug; ctest --preset host-debug. Other gates per docs/development/foundation.md (docs interpreter absolute path).
Output Korean independent adversarial report: execution ID, UTC start/end, actual hash/clean start/end, exact scope/commands/results/omissions; P0-P3 findings with IDs, file:line, evidence/failure reproduction/impact/recommendation; if none list attacks and residual uncertainty. Verdict BLOCK/CONDITIONAL/PASS. This is merge-code review, not vehicle release approval.

Reviewer A: embedded runtime/board/reset safety specialist. Worktree F:/dev/canview-wt/review-foundation-a. Focus board pinmap/rails/reset/PHY safe defaults, fault startup, correct IDF/STM CMake and memory/config/OTA bench-template isolation, effects of missing target compile. Review core integration ownership boundary too. Read relevant embedded skills fully before review.
```

## 최초 결과 원문

원문의 임시 worktree 주소는 현재 문서 탐색 링크가 아니라 리뷰 당시 evidence다. 본문은 그대로 literal block에 보존한다.

````text
판정: **CONDITIONAL**. P2 세 건을 확인했습니다. 현재 구현에서 P0/P1은 발견하지 못했습니다. 아래 finding의 수정 또는 워크플로 §5.4에 따른 명시적 연기가 필요합니다. 차량 release 승인은 아닙니다.

| 실행 증거 | 확인값 |
|---|---|
| Reviewer / 실행 ID | A · embedded runtime/board/reset safety / `foundation-a-59ac4046-20260906T033235Z` |
| UTC 시작 | `2026-09-06T03:32:35.683Z` |
| UTC 종료 | `2026-09-06T03:43:23.992Z` |
| Worktree | `F:/dev/canview-wt/review-foundation-a` |
| 시작·종료 HEAD | `59ac4046a02c3bb4d2612f4143d650ff4095f5b2` |
| 실제 parent | `b529a722fb813d5b60ab667675d73996895ea3fc` |
| 시작·종료 상태 | `git status --porcelain=v1` 모두 빈 출력. 종료 detached 확인 |
| 변경 | tracked 수정·stage·commit 없음. 산출물은 ignored `build/` 내부 |

절차상 한계가 있습니다. OTA 정본 검색 중 같은 문서 §14에 포함된 **과거 리뷰 일부가 검색 출력에 노출**됐습니다. 따라서 다른 리뷰 미열람 조건을 완전히 충족했다고 주장하지 않습니다. 이번 candidate의 다른 reviewer 결과는 열지 않았으며, 아래 finding은 직접 읽은 코드·재현·공식 원문을 근거로 작성했습니다. 다른 agent는 생성하지 않았습니다.

검토 기준은 지정된 93파일 diff입니다. 집중 검토 범위는 네 MCU의 startup/BSP/platform, IDF·STM CMake와 linker, board schema/generator/config/partition, 공용 app·codec의 소유권 경계, 관련 시험과 foundation/ADR008 계약입니다. embedded-architecture·embedded-driver-design·embedded-cstyle을 완독해 적용했고, 사용자 C99 요청을 우선했습니다.

1. **A-01 · P2 · Controller의 재생·녹음 데이터 핀이 뒤바뀜**

   위치: [waveshare35-pins.json:15](/F:/dev/canview-wt/review-foundation-a/firmware/boards/waveshare35-pins.json:15), [생성 header:21](/F:/dev/canview-wt/review-foundation-a/firmware/controller/bsp/board_pins.h:21).

   schema는 `I2S_PLAY_DATA=14`, `I2S_REC_DATA=16`을 생성합니다. 그러나 명시된 Waveshare commit의 실제 IDF 설정은 `gpio_cfg.dout=16`, `gpio_cfg.din=14`입니다. Arduino 재생 예제도 GPIO16을 출력으로 사용합니다. 예제의 혼동되는 주석보다 실제 설정을 근거로 판정했습니다. [공식 IDF 구현](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.5/blob/283ec84c566c096f8c30493b93dcd4b0bb608de7/ESP-IDF/01_factory/components/esp_port/esp_es8311_port.cpp#L20), [공식 재생 예제](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.5/blob/283ec84c566c096f8c30493b93dcd4b0bb608de7/Arduino/examples/01_audio_out/01_audio_out.ino#L9).

   재현: 생성 결과가 PLAY14/REC16임을 확인했습니다. 이 매크로를 후속 I²S 출력·입력에 연결하면 데이터 방향이 뒤바뀝니다. 현재 audio는 미시작이므로 기반 부팅에는 영향이 없지만, 후속 구현에서 무음·수음 실패와 출력 충돌을 유발할 수 있습니다.

   권고: PLAY16/REC14로 교정하고 재생성하십시오. 같은 오기가 있는 [하드웨어 정본:108](/F:/dev/canview-wt/review-foundation-a/docs/hardware/controller.md:108)도 함께 수정하고, 독립적인 방향 검증 fixture를 추가해야 합니다.

2. **A-02 · P2 · 생성기가 존재하지 않거나 메모리에 점유된 ESP GPIO를 허용함**

   위치: [generate_boards.py:112](/F:/dev/canview-wt/review-foundation-a/tools/generate_boards.py:112).

   현재 검사는 `0..48`과 Octal 모드의 GPIO35–37만 검사합니다. Controller 입력의 `LCD_BL`을 변경하는 메모리상 공격에서 GPIO22–25, 26·27·32·33·34가 모두 정상 생성됐습니다. GPIO35–37만 거부됐습니다. 재현 스크립트는 [board_attacks.py](/F:/dev/canview-wt/review-foundation-a/build/review-a/board_attacks.py)입니다.

   ESP32-S3에는 GPIO22–25가 없고, GPIO26–32는 Flash/PSRAM용이며, S3R8의 Octal PSRAM은 GPIO33–37도 점유합니다. [고정 IDF의 공식 GPIO 설명](https://github.com/espressif/esp-idf/blob/76f5dedd9950a3012fee8fb7d5586df21fc67802/docs/en/api-reference/peripherals/gpio/esp32s3.inc#L243).

   영향: 현재 정상 pinmap에서 직접 발생하는 고장은 아닙니다. 그러나 잘못된 후속 board 입력이 생성·drift 검사를 통과합니다. 없는 핀은 부팅 초기화 실패로, 메모리 핀은 Flash/PSRAM 간섭으로 이어질 수 있습니다.

   권고: 칩·패키지·모듈별 사용 가능 핀과 메모리 점유 핀을 검사하고, 위 반례를 거부하는 시험을 추가하십시오.

3. **A-03 · P2 · IDF의 GNU 모드 예외가 BSP·공용 startup까지 확대됨**

   위치: [Controller main/CMakeLists.txt:3](/F:/dev/canview-wt/review-foundation-a/firmware/controller/main/CMakeLists.txt:3). Communicator ESP32와 Diagnostic Bridge에도 같은 구성이 있습니다.

   세 프로젝트 모두 `startup.c`, `bsp/board.c`, SDK adapter를 한 component로 등록하고 C99 override를 지정하지 않습니다. 고정 IDF의 기본은 GNU23입니다. 별도 `canview_foundation` component의 PRIVATE C99 옵션은 이 `main` component로 전파되지 않습니다. [고정 IDF C 언어 계약](https://github.com/espressif/esp-idf/blob/76f5dedd9950a3012fee8fb7d5586df21fc67802/docs/en/api-guides/c.rst#L18).

   이는 [ADR008:10](/F:/dev/canview-wt/review-foundation-a/docs/adr/008-portable-foundation-and-api-docs.md:10)의 공용 app/BSP strict C99 및 platform adapter만 GNU 예외라는 계약과 다릅니다. 실제 host compile database에는 BSP C99 항목 네 개가 있지만 `firmware/app/startup.c`는 없습니다.

   현재 startup은 별도로 수행한 네 역할의 strict C99 구문검사를 통과했습니다. 따라서 현재 소스의 문법 오류가 아니라 **target 설정과 지속 검증의 공백**입니다.

   권고: 순수 startup/BSP와 SDK adapter의 컴파일 설정을 분리하고, startup도 host compile gate에 포함하십시오.

실행 결과는 다음과 같습니다.

| 검증 | 실제 결과 |
|---|---|
| Windows Clang23.1.0 / CMake4.4.3 / Ninja1.13.2 Debug | 31/31 CTest PASS, 컴파일 경고 없음 |
| 같은 도구 Release | 31/31 CTest PASS, 컴파일 경고 없음 |
| 독립 vector·생성기·BSP | 2195 Python/C vector, 생성기 5 unittest, BSP4 각 호출 실패 시험 PASS |
| 새 coverage profile | line546/546, function20/20, branch295/296 |
| API 문서 | Doxygen1.18.0, Sphinx9.1.0/Breathe4.36.0/Furo2025.12.19 strict build PASS, 함수 계약14개 PASS |
| 추가 구문검사 | startup 네 역할 C99 PASS; STM GPIO ARM Clang+C99 PASS |
| Target VerifyOnly | **FAIL:** `arm-none-eabi-gcc` 없음 |
| diff 검사 | `git diff --check base candidate` PASS |

지정된 VS DevShell을 `-Arch amd64 -HostArch amd64 -SkipAutomaticLocation`으로 활성화하고 제공된 main checkout 도구 경로를 PATH 앞에 넣었습니다. 주요 실행 명령은 다음과 같습니다.

```powershell
cmake --preset host-debug -DPython3_EXECUTABLE=C:/Python314/python.exe
cmake --build --preset host-debug
ctest --preset host-debug

cmake --preset host-release -DPython3_EXECUTABLE=C:/Python314/python.exe
cmake --build --preset host-release
ctest --preset host-release

cmake --preset host-coverage -DPython3_EXECUTABLE=C:/Python314/python.exe
cmake --build --preset host-coverage
& C:/Python314/python.exe -B tools/check_coverage.py

& F:/dev/canview/.tools/docs-venv/Scripts/python.exe -B tools/build_docs.py `
  --doxygen F:/dev/canview/.tools/doxygen-1.18.0/bin/doxygen.exe

& ./tools/environment/setup-windows.ps1 -VerifyOnly
& C:/Python314/python.exe -B build/review-a/board_attacks.py

git diff --check b529a722fb813d5b60ab667675d73996895ea3fc 59ac4046a02c3bb4d2612f4143d650ff4095f5b2
```

추가 구문검사는 공통으로 `-std=c99 -Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef -fsyntax-only`를 사용했습니다. STM 검사에는 `--target=arm-none-eabi -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard`와 고정 CubeG4 및 해당 CMSIS device submodule의 공식 헤더를 사용했습니다. 이는 정식 GCC target build나 링크 성공이 아닙니다. 검사 준비 중 PowerShell 인자 전달 오류와 누락된 `mpu_armv7.h`로 각각 한 차례 실패했으며, 준비를 교정한 뒤 통과했습니다.

안전 경계에서 확인한 내용과 잔여 불확실성은 다음과 같습니다.

- RUN_OK·ARM·WD 비활성, STB/FT_EN 안전값, TX recessive, reset 요청 해제·open-drain, GPIO 실패 시 후속 호출 중단을 대조했습니다. reset/brownout/rail 파형과 실제 전기적 안전은 미검증입니다.
- 기본 factory partition과 OTA template은 선택 경로·offset·표기로 분리되어 있습니다. STM 전체 Flash linker도 bench 전용임을 명시합니다.
- 공용 codec은 caller 소유 버퍼와 단일 worker 계약을 사용합니다. 구조 검증 성공을 인증·dispatch·ACK·차량 송신 승인으로 연결하는 구현은 없습니다.
- 정식 IDF/Arm GCC 빌드·링크, PSRAM 실측, clock/DMA/stack budget, Linux sanitizer, HIL·단전 시험은 실행하지 않았습니다. flash·radio·CAN·erase·provisioning도 수행하지 않았습니다.

A-01~03을 연기하려면 owner·상세 task·적용 gate·목표 시점을 기록해야 합니다. host 시험 성공으로 MCU 또는 차량 gate를 닫아서는 안 됩니다.
````

## Post-fix 결과 원문

````text
# Reviewer A — post-fix 독립 재검토 원문

최종 판정: **PASS — 지정된 foundation merge-code 범위에 한정.**

A-01/A-02/A-03은 모두 **FIXED**로 판정합니다. 최초 candidate 이후 전체 delta에서 새로운 P0–P3 finding은 확인하지 못했습니다. 실제 SDK target build와 하드웨어/HIL은 미실행이며, 이 판정은 target 빌드 성공이나 차량 release 승인이 아닙니다.

## 1. 실행 식별·격리 증거

| 항목 | 실제 확인값 |
|---|---|
| Execution ID | `foundation-a-postfix-b084720-20260906T035744Z` |
| 역할 | Reviewer A — embedded runtime·board·reset safety |
| Worktree | `F:/dev/canview-wt/recheck-foundation-a` |
| 검토 기준 | `59ac4046a02c3bb4d2612f4143d650ff4095f5b2` → `b084720ff73da2ad95460a1c947478e62c63d35d` |
| START UTC | `2026-09-06T03:57:44.008Z` |
| START HEAD | `b084720ff73da2ad95460a1c947478e62c63d35d` |
| START 상태 | detached, `git status --porcelain=v1` 출력 없음 |
| END UTC | `2026-09-06T04:04:57.527Z` |
| END HEAD | `b084720ff73da2ad95460a1c947478e62c63d35d` |
| END 상태 | detached, `git status --porcelain=v1` 출력 없음 |

Tracked 수정·commit을 하지 않았습니다. 생성한 시험·빌드·문서 산출물은 해당 worktree의 ignored `build/` 아래에만 있습니다. 추가 agent를 만들지 않았습니다.

이번 실행에서는 상대 reviewer 원문이나 finding을 열지 않았습니다. `docs/journal.md`는 전체 delta에 포함된 신규 4행만 확인했으며, 과거 본문은 탐색하지 않았습니다. Main checkout은 지정된 기존 도구와 허용된 mutation 소스의 읽기에만 사용했습니다. 대용량 bootstrap/download는 실행하지 않았습니다.

## 2. 기존 finding closure

### A-01 — I²S 재생·녹음 핀 방향 오류: FIXED

수정 위치:

- [waveshare35-pins.json:15](F:/dev/canview-wt/recheck-foundation-a/firmware/boards/waveshare35-pins.json:15): `I2S_PLAY_DATA=16`, `I2S_REC_DATA=14`.
- [board_pins.h:21](F:/dev/canview-wt/recheck-foundation-a/firmware/controller/bsp/board_pins.h:21): 생성 매크로도 PLAY16/REC14.
- [controller.md:108](F:/dev/canview-wt/recheck-foundation-a/docs/hardware/controller.md:108): MCU DIN14=녹음, MCU DOUT16=재생으로 정본 수정.
- [test_generators.py:103](F:/dev/canview-wt/recheck-foundation-a/tests/foundation/test_generators.py:103): 두 방향의 명시적 회귀 assertion 추가.

기존 finding의 고정 Waveshare 원문 근거인 MCU DOUT16/DIN14와 source·생성물·정본문서·fixture가 일치합니다. Board 생성물 exact-match와 generator 시험이 Debug/Release 양쪽에서 통과했습니다.

이전 잘못된 상수를 후속 audio 구현이 소비할 위험은 수정됐습니다. 실제 I²S/audio 동작을 검증했다는 의미는 아닙니다.

### A-02 — SoC·모듈별 금지 GPIO 검증 누락: FIXED

수정 위치:

- [generate_boards.py:16](F:/dev/canview-wt/recheck-foundation-a/tools/generate_boards.py:16): SoC 유효 GPIO와 모듈별 허용 집합을 분리.
- [generate_boards.py:117](F:/dev/canview-wt/recheck-foundation-a/tools/generate_boards.py:117): 정수 타입을 먼저 검사하고, SoC와 모듈 양쪽 허용 여부를 확인.
- [test_generators.py:109](F:/dev/canview-wt/recheck-foundation-a/tests/foundation/test_generators.py:109): JSON/CSV 경로의 금지 GPIO 시험 추가.

Controller S3R8·Communicator N16R8은 GPIO22–37을 거부하고, Bridge N8R2는 GPIO22–34를 거부합니다. N8R2에서 사용할 수 있는 GPIO35–37까지 일괄 차단하는 과잉 수정도 없습니다.

저장소 시험 외에 별도 in-memory 입력 변이로 다음 67건을 실행했습니다.

| 독립 공격 | 결과 |
|---|---|
| Controller JSON 금지 GPIO 20건 | 모두 `ValueError` |
| Communicator N16R8 CSV 금지 GPIO 20건 | 모두 `ValueError` |
| Bridge N8R2 CSV 금지 GPIO 17건 | 모두 `ValueError` |
| JSON `True`, `False`, `6.0`, `"6"`, `None`, `[]`, `{}` | 7건 모두 `ValueError` |
| N8R2 GPIO35·36·37 양성 경계 | 3건 모두 생성 성공 및 해당 매크로 확인 |

음수 CSV pin 이름은 필수 신호를 만족하지 못해 거부됩니다. 양의 금지 GPIO는 실제 allowlist에서 거부됩니다.

기존 불가능·메모리 점유 핀의 생성 통과 반례는 닫혔습니다. 다만 이 검사는 모든 핀의 전기적 용도·strap·외부 회로 적합성을 자동 승인하는 검사가 아닙니다.

독립 시험 원본: [verify_board_closure.py](F:/dev/canview-wt/recheck-foundation-a/build/recheck-a/verify_board_closure.py).

### A-03 — IDF startup/BSP의 strict C99 경계 및 host startup gate 누락: FIXED

수정 위치:

- [Controller main/CMakeLists.txt:2](F:/dev/canview-wt/recheck-foundation-a/firmware/controller/main/CMakeLists.txt:2)
- [Communicator ESP32 main/CMakeLists.txt:2](F:/dev/canview-wt/recheck-foundation-a/firmware/communicator/esp32/main/CMakeLists.txt:2)
- [Bridge main/CMakeLists.txt:2](F:/dev/canview-wt/recheck-foundation-a/firmware/diagnostic-bridge/main/CMakeLists.txt:2)
- [canview_esp32_platform/CMakeLists.txt:2](F:/dev/canview-wt/recheck-foundation-a/firmware/components/canview_esp32_platform/CMakeLists.txt:2)
- [tests/foundation/CMakeLists.txt:25](F:/dev/canview-wt/recheck-foundation-a/tests/foundation/CMakeLists.txt:25)

세 IDF main은 실제 `startup.c`와 각 BSP만 컴파일하며, `C_STANDARD 99`, `C_STANDARD_REQUIRED ON`, `C_EXTENSIONS OFF`, 명시적 `-std=c99`와 strict warning/error 옵션을 가집니다.

IDF GPIO/FreeRTOS adapter는 별도 `canview_esp32_platform` component가 소유합니다. 소스·include 상대 경로와 component 의존을 확인했고, adapter의 중복 소유나 main에 SDK GNU 모드를 유지해야 하는 기존 결합은 남아 있지 않습니다.

Debug와 Release에서 실제 startup의 네 역할 object가 모두 빌드됐습니다. Debug `compile_commands.json`에서도 네 항목 각각의 `-std=c99`, strict warning 옵션, 역할 매크로를 확인했습니다. ESP 세 역할에는 `CANVIEW_ENTRY_ESP_IDF=1`, STM 역할에는 해당 정의가 없습니다.

따라서 **소스/CMake 경계와 host 누락 gate는 수정됐습니다.** 실제 IDF component configure·target compile·link 성공은 이번 증거에 포함되지 않습니다.

## 3. 전체 delta 회귀·부팅 안전 검토

전체 변경은 **21파일, +195/-41**입니다. GPIO 수정뿐 아니라 CMake, 생성물·문서, 시험, UTF-8 설정, legacy brightness assertion 변경까지 확인했습니다.

다음 영역은 두 commit 사이 `git diff --exit-code`가 빈 출력으로 성공했습니다.

- `shared/app`, `shared/protocol`
- 실제 `firmware/app/startup.c`
- ESP32 GPIO adapter 구현
- STM32 subtree
- 각 BSP `board.c`
- partition CSV와 OTA template

이에 더해 현재 부팅 경로를 다시 읽고 BSP4 mock fault 시험을 재실행했습니다.

- Communicator의 RUN_OK low 선행, BOOT0/GPS 요청 low, reset release, recovery open-drain release 순서가 유지됩니다.
- STM의 STB high, FT_EN/ARM/WD low, CAN TX 요청 high 기본 상태가 유지됩니다.
- GPIO 실패 시 다음 초기화 호출을 진행하지 않으며 app FAULT 고정 경로가 유지됩니다.
- 출력 latch를 mode보다 먼저 설정하는 adapter 동작이 바뀌지 않았습니다.
- Composition root는 역할 상수와 BSP port를 연결하고, 공용 app/codec에 SDK 타입이나 driver 소유권을 옮기지 않았습니다.
- 무선·FDCAN·UART 송신·watchdog pulse 활성화 경로가 추가되지 않았습니다.
- 기본 factory bench partition과 선택되지 않는 OTA 설계 template의 구분이 유지됩니다. Controller `sdkconfig.defaults` 변경은 입력 digest 변경입니다.
- reset 이전·brownout·rail 이상 안전을 외부 pull/gate/supervisor 및 실제 시험에 의존한다고 명시한 문구도 유지됩니다.

검증 delta도 확인했습니다.

- CAN roundtrip이 `bus_id`, `flags`, `dlc`를 포함한 필드 검증으로 보강됐습니다.
- 독립 Python `struct` 참조의 CAN batch 208개가 추가됐습니다.
- Version 손상 fixture는 XOR 후 복원하도록 변경됐습니다.
- CTest Python 진입점의 `-X utf8`과 Windows bootstrap의 `PYTHONUTF8=1`이 추가됐습니다.
- 기존 brightness assertion의 `9U→9`는 정수 승격 후 비교 타입 경고 수정이며, 시험 조건의 수치 의미를 바꾸지 않습니다.

새로운 P0/P1/P2/P3 finding은 없습니다.

## 4. 실제 실행 명령·결과

모든 저장소 명령의 작업 디렉터리는 `F:/dev/canview-wt/recheck-foundation-a`였습니다.

기존 도구를 사용하기 위한 준비:

```powershell
& 'C:/Program Files (x86)/Microsoft Visual Studio/18/BuildTools/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
$env:PATH='F:/dev/canview/.tools/llvm-23.1.0/clang+llvm-23.1.0-x86_64-pc-windows-msvc/bin;F:/dev/canview/.tools/cmake-4.4.3/bin/cmake-4.4.3-windows-x86_64/bin;F:/dev/canview/.tools/ninja-1.13.2/bin;' + $env:PATH
$env:PYTHONDONTWRITEBYTECODE='1'
```

Clang23.1.0 GNU frontend, CMake4.4.3, Ninja1.13.2를 사용했습니다.

### Host Debug·Release

```powershell
cmake --preset host-debug -DPython3_EXECUTABLE=C:/Python314/python.exe
cmake --build --preset host-debug
ctest --preset host-debug
cmake --preset host-release -DPython3_EXECUTABLE=C:/Python314/python.exe
cmake --build --preset host-release
ctest --preset host-release
```

모두 exit 0입니다.

| 항목 | 실제 결과 |
|---|---|
| Debug | 31/31 CTest PASS |
| Release | 31/31 CTest PASS |
| 새 startup object | 네 역할 모두 빌드 |
| 컴파일 warning | 관찰된 warning 없음 |
| Python generator | 7 unittest 그룹 PASS |
| Golden | envelope/COBS 2195 + CAN batch 208 PASS |
| BSP mock | 4종 모두 순서·safe level·open-drain·각 GPIO 실패 시험 PASS |
| Legacy automation | 기존 12개 시험 PASS |

31개 CTest에는 생성물 drift, plan validation, document links도 포함됩니다. Legacy 12개는 별도 C11 prototype 시험이며 새 C99 기능 검증으로 합산하지 않습니다.

로그:

- [Debug LastTest.log](F:/dev/canview-wt/recheck-foundation-a/build/host-debug/Testing/Temporary/LastTest.log)
- [Release LastTest.log](F:/dev/canview-wt/recheck-foundation-a/build/host-release/Testing/Temporary/LastTest.log)

### Coverage

```powershell
cmake --preset host-coverage -DPython3_EXECUTABLE=C:/Python314/python.exe
cmake --build --preset host-coverage
& C:/Python314/python.exe -X utf8 -B tools/check_coverage.py
```

Exit 0, core 9그룹 PASS입니다.

| 측정 대상 | Lines | Functions | Branches |
|---|---:|---:|---:|
| 공용 app | 27/27 | 2/2 | 26/26 |
| 공용 wire | 519/519 | 18/18 | 269/270 |
| 합계 | 546/546 | 20/20 | 295/296 |

새 profile을 사용했습니다. 이 분모는 두 공용 C 파일에 한정되며 BSP·SDK·Python·legacy coverage가 아닙니다.

[Coverage summary.json](F:/dev/canview-wt/recheck-foundation-a/build/host-coverage/coverage-lkhybgj4/summary.json)

### API 문서

```powershell
& F:/dev/canview/.tools/docs-venv/Scripts/python.exe -X utf8 -B tools/build_docs.py --doxygen F:/dev/canview/.tools/doxygen-1.18.0/bin/doxygen.exe
```

Exit 0. Doxygen/Sphinx strict build와 공개 함수 XML 계약 **14개 PASS**입니다.

[생성 API 문서](F:/dev/canview-wt/recheck-foundation-a/build/api/html/index.html)

### 독립 GPIO closure

```powershell
& C:/Python314/python.exe -X utf8 -B build/recheck-a/verify_board_closure.py
```

Exit 0, **67건 PASS**입니다.

### CAN mutation 재검증

허용된 `bus.c`, `offset.c`, `endian.c`를 현재 codec과 `git diff --no-index`로 먼저 비교했습니다. 의도된 변이 외에 의미 있는 구현 차이는 없었습니다. 이 비교의 exit 1은 diff 존재를 나타내며, LF/CRLF 안내 경고도 출력됐습니다.

각 변이에 다음 명령을 실행했습니다.

```powershell
foreach ($mutationName in @('bus','offset','endian')) {
  & clang -std=c99 -Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef -Ishared/protocol/include -Ishared/interface tests/foundation/wire_vectors.c "F:/dev/canview/build/foundation-mutation/$mutationName.c" -o "build/recheck-a/$mutationName.exe"
  & C:/Python314/python.exe -X utf8 -B tests/foundation/test_vectors.py "build/recheck-a/$mutationName.exe"
}
```

| 변이 | Compile | 검출 결과 |
|---|---|---|
| bus 필드 상수 0 | exit 0 | C roundtrip 검사 실패, Python 실행 exit 1 |
| bus 필드 offset 오류 | exit 0 | C roundtrip 검사 실패, Python 실행 exit 1 |
| encoder·decoder delta endian 동시 반전 | exit 0 | Python 독립 바이트 golden 불일치, exit 1 |

세 변이가 모두 검출됐습니다. 정확히는 앞의 두 변이는 Python 바이트 비교에 도달하기 전에 C 검사에서 실패했고, 동시 endian 반전은 roundtrip을 통과해도 독립 golden에서 실패했습니다.

### Target 환경 확인 — 실패

```powershell
& ./tools/environment/setup-windows.ps1 -VerifyOnly
```

**Exit 1**:

```text
Required command 'arm-none-eabi-gcc' was not found on PATH.
```

실패 위치는 `tools/environment/setup-windows.ps1:19`입니다. Host 도구 활성화 뒤에도 target toolchain 부재가 재현됐습니다. 이를 target gate PASS로 처리하지 않았습니다.

### 최종 무결성 확인

`git diff --check`는 exit 0이었습니다. 위 불변 runtime 경로에 대한 `git diff --exit-code`도 exit 0이었습니다.

종료 직전 다음 명령으로 immutable HEAD, detached 상태와 빈 status를 다시 확인했습니다.

```powershell
git rev-parse HEAD
git branch --show-current
git status --porcelain=v1
```

## 5. 미실행·잔여 불확실성

- 실제 ESP-IDF 3 target 및 STM32 target configure/compile/link는 실행하지 못했습니다. 새 component의 SDK 통합, 실제 target warning0, linker·image·memory 사용량은 미검증입니다.
- Startup4 object gate는 무한 main loop 실행 시험이나 MCU 부팅 시험이 아닙니다.
- GPIO 파형, reset/brownout, rail 순서, PHY 외부 gate, PSRAM/ECC, clock, DMA, UART4 Mbps, 실제 audio는 미검증입니다.
- Flash·radio 시작·CAN 사용·erase·provisioning·HIL은 실행하지 않았습니다.
- 이번 후보의 원격 CI와 Linux GCC/ASan/UBSan은 직접 실행·조회하지 않았습니다. 사용자가 전달한 이전 `beae8a9` CI 성공을 이번 hash의 독립 검증 결과로 합산하지 않습니다.
- 영문 Windows locale 자체를 별도 재현하지 않았습니다. UTF-8 진입점 변경과 현재 Windows 실행 결과를 확인했습니다.
- 문서 의존성의 신규 venv/hash-lock 재설치 및 별도 plan-validator 부정 fixture 35건은 이번 실행에서 재실행하지 않았습니다.
- Coverage의 미실행 branch 1개가 남습니다. 높은 core coverage가 target·전기적 안전이나 완전한 입력 공간 검증을 뜻하지 않습니다.
- 전체 ABI catalog/auth/session/ACK/QoS/filter, peripheral driver/ISR/RTOS, 실제 OTA/LVGL/audio/vehicle 기능은 여전히 미구현입니다. 이번 delta가 이를 구현 완료로 표시하거나 송신 권한으로 연결하지 않았습니다.

## 6. 판정 및 산출물

**A-01 FIXED / A-02 FIXED / A-03 FIXED. 새 finding 없음. Merge-code verdict: PASS.**

미실행 target/HIL gate는 계속 열려 있고 차량 송신은 **NO-GO**입니다. 실제 SDK 빌드 없이 target PASS를 선언하지 않습니다.

`embedded-architecture`, `embedded-driver-design`, `embedded-cstyle` 기준으로 소유권·SDK 경계·부팅 실패 경로를 검토했고, `embedded-documentation` 기준으로 수정된 정본문서와 검증 한계의 표현을 확인했습니다. 언어 기준에는 사용자 지정 C99를 우선했으며 Hallmark는 사용하지 않았습니다.

보고서 자체는 별도 파일에 쓰지 않았습니다. **이 최종 메시지가 보존할 전체 원문**입니다. 추가 시험 소스와 재컴파일한 mutation 실행 파일은 `F:/dev/canview-wt/recheck-foundation-a/build/recheck-a/`에 있습니다.
````
