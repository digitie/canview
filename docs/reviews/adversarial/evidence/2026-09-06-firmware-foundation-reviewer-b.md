# 기반 코드 Reviewer B 원본 evidence

## Post-fix 재검토 전달 요청 원문

```text
Post-fix 독립 재검토. 원본 두 보고서를 docs/reviews/adversarial/evidence/2026-09-06-firmware-foundation-reviewer-{a,b}.md에 각각 보존한 뒤 코디네이터가 비교했습니다. 이번 검토에서는 상대 원문을 볼 필요 없습니다.
같은 immutable post-fix: b084720ff73da2ad95460a1c947478e62c63d35d, 최초 candidate: 59ac4046a02c3bb4d2612f4143d650ff4095f5b2. 이 둘의 전체 delta 회귀와 자기 finding closure를 확인하세요. 새 detached worktree 아래에서 시작/종료 HEAD 및 git status --porcelain=v1 clean, UTC 시각을 기록하세요. tracked 수정·commit 금지. old worktree와 main checkout은 도구/기존 본인 experiment 읽기 용도로만. initial manifest 범위/미실행 target/HIL 제한 유지. 다른 review/journal 과거본문 탐색 불필요.
변경: PLAY16/REC14+정본문서/fixture; SoC+모듈 GPIO allowlist(JSON/CSV, noninteger reject); main C99 startup/BSP와 SDK GNU canview_esp32_platform component 분리; startup4역할 host object gate. CAN roundtrip 전체 필드+Python/C208 batch byte golden 추가. prior CI locale -X utf8 및 GPIO와 무관한 기존 brightness assertion 9U->9 경고수정 포함. core runtime codec은 변경 없음.
작성자 post-fix Windows Debug/Release31/31, generator7그룹, envelope/COBS2195+CAN208, coverage546/546·295/296, strictAPI14 PASS. CAN bus상수0/offset오류/encoder+decoder delta endian동시반전 3mutant를 각각 compile해 새 golden이 모두 검출함. mutant파일 F:/dev/canview/build/foundation-mutation/ 읽기 가능. 이전 beae8a9 CI34009610099 Windows+LinuxGCC/ASanUBSan PASS, 최신postfix CI는 코디네이터가 별도로 확인.
도구는 기존 manifest maincheckout pinned paths를 사용하며 bootstrap 대용량 재다운로드 금지. 재검토 결과는 한글 최종 메시지에 full 원문으로 전달: execution ID/UTC/commitclean,각 finding FIXED 여부+증거,전체delta회귀/새finding,실행명령·실패포함 실제결과/미실행,최종 PASS/CONDITIONAL/BLOCK. 별도 파일에 쓴 경우 정확한 경로를 명시해 주세요.
Reviewer B: F:/dev/canview-wt/recheck-foundation-b. B-P2-01/B-P2-02 closure와 generator+golden의 반례 검출, CI 재현성 변화 중점.
```


- Subagent 실행 ID: `01a074c6-2a91-7c51-843a-9075f678f9bc`
- 원본 위치: reviewer가 작성한 ignored build/review-b/review-report.md를 그대로 보존한다.

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

Reviewer B: protocol/security/build-quality specialist. Worktree F:/dev/canview-wt/review-foundation-b. Focus malformed/CRC/COBS/sequence wrap, byte ABI consistency with normative specs, API ownership/error state, generator drift, test/coverage false passes, CI/build reproducibility, docs usability. Review board generator safeguards too. Read relevant embedded skills fully before review.
```

## 최초 결과 원문

# Reviewer B 독립 적대적 리뷰 — canview 펌웨어 기반

판정: **CONDITIONAL**. P0 0건, P1 0건, P2 2건, P3 0건이다. 두 P2는 현재 OPEN이다. 수정 후 재검증하거나, agent-workflow §5.4에 따라 owner·상세 task·적용 gate·목표 기한을 모두 갖춘 DEFERRED로 disposition하는 것이 merge 조건이다. 이 결과는 코드 merge 리뷰이며 차량 release 승인이 아니다.

## 실행 식별과 격리

- 실행 ID: `review-foundation-b-20260906T033234Z` (이 보고서의 독립 리뷰 식별자).
- 전문 영역: protocol/security/build-quality. 다른 reviewer의 결과는 열람하지 않았으며 다른 agent를 생성하지 않았다.
- 작업 위치: `F:/dev/canview-wt/review-foundation-b`, detached worktree.
- 요청 후보 및 실제 시작 HEAD: `59ac4046a02c3bb4d2612f4143d650ff4095f5b2`.
- 요청 base 및 실제 HEAD parent: `b529a722fb813d5b60ab667675d73996895ea3fc`.
- 시작 UTC: `2026-09-06T03:32:34.6902310Z`.
- 시작 `git status --porcelain=v1`: 출력 없음, clean.
- 종료 감사: 아래 종료 확인 절에 기록한다.
- tracked 파일 수정·commit 없음. 실험 source/binary/문서와 build 출력은 이 worktree의 무시되는 `build/` 아래에만 작성했다. 기본 checkout에서는 사용자가 지정한 설치 도구·문서 interpreter만 읽기 전용으로 사용했다.
- flash, radio 시작, CAN 접근, erase, provisioning을 수행하지 않았다.

## 요청과 실제 검토 범위

사용자 요청은 전체 기반/구조, MCU 독립 protocol, 네 MCU의 보드 설정, C99 warning0, 넓은 unit test와 상세 생성 API 문서를 검토하는 것이다. 상세 제품 기능은 추후 구현하며 foundation이 미구현 기능을 안전하고 사실대로 격리하는지 평가했다.

93파일, +4435/-104 diff가 manifest와 일치했다. shared app/interface/protocol, transport schema/generator, 4MCU app/BSP/platform/IDF 및 STM CMake, board generator/partition, tests/CI/tools/API 문서와 ADR008을 검토했다. 전체 목록은 말미에 보존한다.

현재 요구 근거는 AGENTS.md, docs/README.md, docs/resume.md, agent-workflow §5, firmware-foundation.md와 accepted ADR008이다. T-001은 부분 IN_PROGRESS임을 확인했다. 실제 byte 계약은 ESP-NOW §4/§11 및 Communicator UART §3/§5, 기존 CAN flag 정의와 대조했다. 보드 검증은 source pinmap·R1 firmware pinmap·Controller 설정 및 고정 IDF GPIO/PSRAM source와 대조했다.

embedded-cstyle, embedded-architecture, embedded-documentation, embedded-driver-design의 SKILL.md를 완독했다. C99 사용자 요구와 ADR008이 우선하며, 스킬의 자동 수정 절차는 읽기 전용 리뷰에 적용하지 않았다. Hallmark는 사용하지 않았다.

독립성 보존을 위해 다른 review/evidence 보고서와 과거 journal의 finding은 읽지 않았다. docs/journal.md는 변경량 13행만 확인했고 본문 감사는 생략했다. 문서 link/plan validator는 CTest의 읽기 전용 자동 검사로 실행했다. requirements-docs.lock은 설치된 30개 패키지의 고정 version과 대조했으며 이번 실행에서 archive 재다운로드나 hash lock 재설치는 하지 않았다.

## B-P2-01 — board 생성기가 없는 GPIO와 메모리 점유 핀을 허용한다

- 등급/상태: P2 / OPEN.
- 위치: [tools/generate_boards.py:112](F:/dev/canview-wt/review-foundation-b/tools/generate_boards.py:112).
- 현재 조건은 0..48 범위 및 Octal 모드의 35/36/37만 검사한다. ESP32-S3에는 GPIO22..25가 없으며, GPIO26..32는 Flash/PSRAM 용도이고 R8 Octal 메모리에서는 33..37도 점유된다. 고정 IDF source의 유효 mask는 22..25를 제외한다. [IDF 고정 source](https://github.com/espressif/esp-idf/blob/76f5dedd9950a3012fee8fb7d5586df21fc67802/components/soc/esp32s3/include/soc/soc_caps.h#L170), [IDF v6.0.3 GPIO 문서](https://docs.espressif.com/projects/esp-idf/en/v6.0.3/esp32s3/api-reference/peripherals/gpio.html).
- 재현: Controller board manifest는 그대로 두고 메모리에 로드한 pin JSON의 LCD_BL만 22,23,24,25,26..34로 바꾸어 `board_outputs()`를 호출했다. 모두 예외 없이 해당 GPIO macro를 생성했다. 비교 입력 35와49는 거부했다.
- 명령: `C:/Python314/python.exe -B build/review-b/generator_attacks.py`.
- 실제 출력 예: `GPIO22: ACCEPTED #define CANVIEW_BOARD_LCD_BL_GPIO (22U)`, `GPIO33: ACCEPTED #define CANVIEW_BOARD_LCD_BL_GPIO (33U)`.
- 영향: 잘못된 pin source를 사전 검출하는 생성기 safeguard에 빈틈이 있다. nonexistent pin은 현재 IDF adapter에서 runtime 오류로 이어지며, memory pin은 번호 범위 검사만으로 충돌을 막을 수 없다. 현재 체크인된 LCD_BL=6 등 실제 pin 입력이 잘못됐다는 주장은 아니다. 현재 BSP mock의 하드코딩된 사용 pin은 일부 변경을 추가 검출하지만 전체 pin 계약 검사를 대신하지 않는다.
- 권고: SoC 유효 pin 집합과 module/package별 외부 사용 가능·메모리 예약 pin 집합을 분리해 allowlist로 검증한다. 22..25 및 Controller R8의26..37에 대한 negative fixture와 CSV 경로도 추가한다. 실제 board/pin 변경 전 gate에 포함한다.

## B-P2-02 — CAN batch 시험이 bus 식별 정보 손실을 놓친다

- 등급/상태: P2 / OPEN.
- 위치: [tests/foundation/test_foundation.c:280](F:/dev/canview-wt/review-foundation-b/tests/foundation/test_foundation.c:280), [tests/foundation/test_vectors.py:31](F:/dev/canview-wt/review-foundation-b/tests/foundation/test_vectors.py:31).
- 기존 CAN 시험은 bus0/1/2를 입력하지만 round-trip 결과에서 bus_id·flags·dlc를 대조하지 않는다. 2195개의 독립 golden은 opaque payload를 사용한 envelope/COBS를 검사하며 CAN batch의 의미 필드와 byte layout을 직접 검사하지 않는다.
- 재현: 원본 codec을 `build/review-b/wire-mutant.c`로 복사한 뒤 원본528행의 `destination[2] = record->bus_id;` 한 줄만 `destination[2] = 0U;`로 치환했다. `git diff --no-index`로 단일 statement 변경임을 확인했다.
- 결과: mutant로 기존 test_foundation.c를 다시 strict C99 compile해 실행한 core9개 scenario 전부 PASS. mutant로 wire_vectors.c를 compile한 결과도 Python/C golden2195개 PASS.
- 실패 의미: 모든 CAN2/CAN3 record를 CAN1로 바꾸는 변이가 통과한다. 이는 후보 codec 자체에 발견된 bus 직렬화 오류가 아니라 현재 regression gate의 실질적인 검출 누락이다. mutant로 root 전체31CTest 또는 coverage gate까지 실행했다고 주장하지 않는다.
- 영향: 후속 codec 변경에서 bus별 source·filter·capture evidence가 잘못 연결되는 회귀를 놓칠 수 있다. line/function100% 및 branch99%는 이 의미 보존을 입증하지 못한다.
- 권고: 모든 record 필드의 encode/decode round-trip을 확인하고, 비대칭 data byte·bus0/1/2·각 flag·DLC·11/29-bit ID를 포함한 독립 CAN byte golden을 추가한다. bus_id 상수화 및 필드 offset/endian 변이가 실패하는지 검증한다.
- 별도 원본 검증: Reviewer B의 독립 CAN golden4000개에서는 현재 원본 codec이 모든 record 필드를 올바르게 encode/decode했다.

## 실제 실행과 결과

모든 명령의 기준 cwd는 `F:/dev/canview-wt/review-foundation-b`다. 외부 도구는 아래처럼 각 PowerShell 실행에 활성화했다.

```powershell
& 'C:/Program Files (x86)/Microsoft Visual Studio/18/BuildTools/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 -SkipAutomaticLocation | Out-Null
$env:PATH = 'F:/dev/canview/.tools/llvm-23.1.0/clang+llvm-23.1.0-x86_64-pc-windows-msvc/bin;F:/dev/canview/.tools/cmake-4.4.3/bin/cmake-4.4.3-windows-x86_64/bin;F:/dev/canview/.tools/ninja-1.13.2/bin;F:/dev/canview/.tools/doxygen-1.18.0/bin;' + $env:PATH
$env:PYTHONDONTWRITEBYTECODE='1'
```

실제 확인한 도구는 Clang23.1.0 GNU frontend, CMake4.4.3, Ninja1.13.2, Python3.14.3, Doxygen1.18.0이다. 문서 interpreter의 Sphinx9.1.0/Breathe4.36.0/Furo2025.12.19 및 lock의30개 version이 모두 일치했다.

```powershell
clang --version
cmake --version
ninja --version
cmake --preset host-debug -DPython3_EXECUTABLE=C:/Python314/python.exe
cmake --build --preset host-debug
ctest --preset host-debug
cmake --preset host-release -DPython3_EXECUTABLE=C:/Python314/python.exe
cmake --build --preset host-release
ctest --preset host-release
cmake --preset host-coverage -DPython3_EXECUTABLE=C:/Python314/python.exe
cmake --build --preset host-coverage
C:/Python314/python.exe -B tools/check_coverage.py
F:/dev/canview/.tools/docs-venv/Scripts/python.exe -B tools/build_docs.py
tools/environment/setup-windows.ps1 -VerifyOnly
git diff --check b529a722fb813d5b60ab667675d73996895ea3fc HEAD
```

configure/build/CTest 연속 호출에서는 각 단계 뒤 `if ($LASTEXITCODE) { exit $LASTEXITCODE }`로 실패를 전파했다.

| 검사 | 실제 결과 |
|---|---|
| host-debug | configure/build exit0, 경고0, CTest31/31 PASS |
| host-release | configure/build exit0, 경고0, CTest31/31 PASS |
| source compile flags | 공용 core와 BSP host compile_commands에 -std=c99, 요청 strict warning 및 -Werror 확인 |
| generator checks | CTest의 generated-transport, generated-boards PASS |
| Python generator unit | foundation-python 5 unittest 그룹 PASS |
| 기존 자동화 | 별도 legacy12CTest PASS; 새 제품 기능으로 집계하지 않음 |
| independent author golden | 2195 Python/C envelope/COBS vectors PASS |
| BSP mocks | 4보드의 호출 순서·pin·level·open-drain 및 각 step 실패 종료 PASS |
| coverage | 새 profile로 core9/9; line546/546, function20/20, branch295/296 PASS |
| API | Doxygen XML14함수 계약 PASS, Sphinx strict warning0, exit0 |
| target VerifyOnly | exit1: Required command 'arm-none-eabi-gcc' was not found on PATH |
| diff 공백 검사 | exit0 |

coverage 출력 위치: `build/host-coverage/coverage-6teafut5/summary.json`. protocol은 line519/519·function18/18·branch269/270, app은 line27/27·function2/2·branch26/26이다. BSP/SDK/생성기/legacy는 이 coverage 분모 밖이다.

### Reviewer B 독립 codec 검사

원본 shared/protocol source를 Windows DLL로 compile한 뒤 ctypes로 호출했다. seed는0x59AC4046이다. 아래 추가 검사는 host 상호운용 비교이며 sanitizer나 target 검증이 아니다.

```powershell
clang -std=c99 -Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef -shared -Ishared/protocol/include -Ishared/interface shared/protocol/src/canview_wire.c -o build/review-b/wire.dll '-Wl,/export:canview_wire_envelope_encode,/export:canview_wire_envelope_decode,/export:canview_wire_cobs_encode,/export:canview_wire_cobs_decode,/export:canview_wire_can_batch_encode,/export:canview_wire_can_batch_decode,/export:canview_sequence_window_accept'
C:/Python314/python.exe -B build/review-b/independent_wire_checks.py
```

- struct/zlib 기반 envelope4000개, version·reserved·flags·header length를 바꾸고 CRC를 다시 계산한20000개 거부 확인 PASS.
- 비대칭 data와 전체 record 필드를 비교한 독립 CAN batch4000개 PASS.
- 별도 COBS decoder와 비교한 정상10000개 및 변조10000개 PASS.
- set 기반 참조와 sequence전이100000개 비교 PASS. 63/64 경계·양방향 wrap·half-range·duplicate/stale 상태 불변 포함.

최초 DLL compile 호출은 PowerShell의 따옴표 없는 `-Wl,` 파싱 때문에 실행 전 실패했다. 인자를 따옴표로 감싸 해결했다. 최초 Python harness는 temporary ctypes 입력을 즉시 해제해 borrowed view 비교가 실패했다. 문서 계약대로 입력 수명을 유지하도록 harness만 수정한 후 전부 통과했다. 두 실패를 후보 코드 결함으로 집계하지 않았다.

### 변이 시험 명령

```powershell
clang -std=c99 -Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef -Ishared/protocol/include -Ishared/interface -Ishared/app/include build/review-b/wire-mutant.c shared/app/src/canview_app.c tests/foundation/test_foundation.c -o build/review-b/mutant-tests.exe
foreach ($scenario in @('crc','envelope','cobs','stream','can','sequence','nulls','noise','app')) {
  & build/review-b/mutant-tests.exe $scenario
  if ($LASTEXITCODE) { exit $LASTEXITCODE }
  Write-Output "MUTANT $scenario PASS"
}
clang -std=c99 -Wall -Wextra -Wpedantic -Werror -Ishared/protocol/include -Ishared/interface build/review-b/wire-mutant.c tests/foundation/wire_vectors.c -o build/review-b/mutant-vectors.exe
C:/Python314/python.exe -B tests/foundation/test_vectors.py build/review-b/mutant-vectors.exe
git diff --no-index -- shared/protocol/src/canview_wire.c build/review-b/wire-mutant.c
```

변이 파일은 후보와 별개이며 후보 파일을 교체하지 않았다. diff의 예상 차이 한 줄을 확인했고, Git의 LF/CRLF 안내는 compiler warning과 구분했다.

## 공격 관점과 남은 불확실성

실제 header32바이트·little-endian·CRC영역zeroing·maximum payload는 정본과 일치했다. unaligned/bit corruption, null, overflow, malformed COBS, oversize 후 delimiter 재동기화, CAN ID/DLC/padding/time overflow, sequence wrap·half-range와 오류 상태 보존을 확인했다. encoder의 실패 시 written0, decode 결과0 초기화, borrowed view 및 단일 owner 계약은 현 구현과 맞는다.

unknown message_type과 session0을 structural decoder가 받아들이는 것은 명시된 범위다. 현재 dispatch/auth/ACK/control admission이 없고 startup에서 codec을 차량 송신에 연결하지 않는다. sequence commit은 이미 인증·queue admission된 입력에 대해서만 호출하라는 계약이 있다. 미구현 auth/session/ACK/QoS/filter를 구현된 보안 경계로 오인하는 연결은 발견하지 않았다.

보드 현재 생성물과 source drift 검사는 통과했다. GPIO latch-before-mode 및 fail-stop은 source/BSP mock 수준에서 확인했다. 외부 TX gate, reset/brownout 및 rail 전기 안전은 host로 검증하지 않았다. STM32 HSE/PLL/UART/FDCAN을 시작하지 않는 것과 새 IDF target에서 legacy component가 제외되는 CMake 의도를 확인했으며 실제 target link/map으로 검증하지 않았다.

OTA factory partition과 비선택 template 및 전체Flash bench linker의 구분은 명시돼 있다. 사용자가 범위 밖으로 정한 full ABI catalog freeze, crypto/session/ACK/QoS/filter, FDCAN/UART driver/ISR/RTOS, OTA/LVGL/audio/차량 기능을 구현 요구 누락 finding으로 만들지 않았다.

미실행: Arm/ESP-IDF target configure/build, 실제 보드/HIL, UART4Mbps, GPIO 파형, PSRAM 용량·ECC 실측, Flash/OTA 단전, CAN/radio, 실차 evidence. VerifyOnly는 Arm GCC 부재로 더 진행하지 못했다. Linux GCC/ASan/UBSan job은 workflow를 읽었지만 이 Windows 리뷰에서 실행하지 않았다. remote CI 실행·native archive 재다운로드/digest 재검증·SDK 설치도 하지 않았다. 지속 fuzzing, MC/DC, MCU stack/heap/cache/DMA budget 검증을 이 결과로 대체할 수 없다.

## 산출물 digest

- generator_attacks.py: `1A06E4DBFD89912A6E57F3344203309C5A9FF402E13E0201561ED852114CCDA6`
- independent_wire_checks.py: `72BEAD50A4D5F4155369E285A9734FE9E93487C8CC4FE0B6A514CF284C73F07A`
- wire-mutant.c: `5F4917BE14F3750C1FB006DA0F80CF7C6E86D4BD8F2C3934F3E2D1D95A095FE3`

모두 SHA256, `build/review-b/` 내부 파일이다. 재현 binary와 API HTML/CTest 로그도 ignored build 안에 남겼다. tracked review archive에는 작성하지 않았다.

## 종료 확인

- 종료 UTC: `2026-09-06T03:46:52.1398909Z`.
- 실제 종료 HEAD: `59ac4046a02c3bb4d2612f4143d650ff4095f5b2` (manifest와 일치).
- `git status --porcelain=v1`: 출력 0행, clean.
- `git diff --exit-code` 및 `git diff --cached --exit-code`: exit0, tracked/staged 변경 없음.
- `git symbolic-ref -q HEAD`: exit1, detached 유지.
- 이후 이 ignored 보고서의 종료 metadata만 채웠다. 후보 code/규범/commit은 변경하지 않았다.

## manifest diff 파일 목록

```text
.clang-format
.clangd
.editorconfig
.github/workflows/foundation.yml
CHANGELOG.md
CMakeLists.txt
CMakePresets.json
cmake/CanviewWarnings.cmake
docs/README.md
docs/adr/008-portable-foundation-and-api-docs.md
docs/adr/README.md
docs/api/Doxyfile
docs/api/app.rst
docs/api/conf.py
docs/api/index.rst
docs/api/protocol.rst
docs/architecture/README.md
docs/architecture/firmware-foundation.md
docs/decisions.md
docs/development/foundation.md
docs/development/windows.md
docs/hardware/controller.md
docs/journal.md
docs/resume.md
docs/tasks.md
docs/tasks/T-001-host-toolchain-ci.md
firmware/README.md
firmware/app/startup.c
firmware/boards/boards.json
firmware/boards/waveshare35-pins.json
firmware/communicator/esp32/CMakeLists.txt
firmware/communicator/esp32/README.md
firmware/communicator/esp32/bsp/board.c
firmware/communicator/esp32/bsp/board_pins.h
firmware/communicator/esp32/main/CMakeLists.txt
firmware/communicator/esp32/main/main.c
firmware/communicator/esp32/partitions.csv
firmware/communicator/esp32/partitions.ota-template.csv
firmware/communicator/esp32/sdkconfig.defaults
firmware/communicator/stm32/CMakeLists.txt
firmware/communicator/stm32/README.md
firmware/communicator/stm32/bsp/board.c
firmware/communicator/stm32/bsp/board_pins.h
firmware/communicator/stm32/ld/STM32G474CEUx_FLASH.ld
firmware/communicator/stm32/platform/stm32g474/safe_gpio.c
firmware/communicator/stm32/platform/stm32g474/safe_gpio.h
firmware/communicator/stm32/src/main.c
firmware/components/canview_foundation/CMakeLists.txt
firmware/controller/CMakeLists.txt
firmware/controller/README.md
firmware/controller/bsp/board.c
firmware/controller/bsp/board_pins.h
firmware/controller/main/CMakeLists.txt
firmware/controller/main/main.c
firmware/controller/partitions.csv
firmware/controller/partitions.ota-template.csv
firmware/controller/sdkconfig.defaults
firmware/diagnostic-bridge/CMakeLists.txt
firmware/diagnostic-bridge/README.md
firmware/diagnostic-bridge/bsp/board.c
firmware/diagnostic-bridge/bsp/board_pins.h
firmware/diagnostic-bridge/main/CMakeLists.txt
firmware/diagnostic-bridge/partitions.csv
firmware/diagnostic-bridge/partitions.ota-template.csv
firmware/diagnostic-bridge/sdkconfig.defaults
firmware/interface/canview_board.h
firmware/interface/canview_gpio.h
firmware/platform/esp32s3/canview_gpio.c
protocol/schema/transport-foundation-v1.json
shared/app/CMakeLists.txt
shared/app/include/canview_app.h
shared/app/src/canview_app.c
shared/interface/canview_platform_port.h
shared/interface/canview_status.h
shared/protocol/CMakeLists.txt
shared/protocol/include/canview_wire.h
shared/protocol/include/canview_wire_layout.h
shared/protocol/src/canview_wire.c
tests/foundation/CMakeLists.txt
tests/foundation/test_bsp.c
tests/foundation/test_foundation.c
tests/foundation/test_generators.py
tests/foundation/test_vectors.py
tests/foundation/wire_vectors.c
tools/build_docs.py
tools/check_coverage.py
tools/environment/foundation-windows.ps1
tools/foundation-tools.json
tools/generate_boards.py
tools/generate_transport.py
tools/requirements-docs.in
tools/requirements-docs.lock
tools/toolchain-versions.json
```


## Post-fix 결과 원문

Markdown hard-break용 행끝 공백 1곳만 정리했다. 본문·명령·수치·판정은 수정하지 않았다.

````text
# Reviewer B post-fix 독립 재검토

**판정: PASS — 지정된 기반 코드의 merge-code 재검토 범위.**
B-P2-01과 B-P2-02는 모두 **FIXED**입니다. 새로운 P0–P3 actionable finding은 확인하지 못했습니다. MCU 실행·HIL·차량 release 승인을 의미하지 않습니다.

## 1. 실행 및 격리 기록

| 항목 | 실제 기록 |
|---|---|
| Execution ID | `recheck-foundation-b-20260906T035746Z` |
| Worktree | `F:/dev/canview-wt/recheck-foundation-b` |
| 시작 UTC | `2026-09-06T03:57:46.3200725Z` |
| 종료 UTC | `2026-09-06T04:07:12.7483595Z` |
| 비교 기준 | `59ac4046a02c3bb4d2612f4143d650ff4095f5b2` |
| 시작·종료 실제 HEAD | `b084720ff73da2ad95460a1c947478e62c63d35d` |
| 시작·종료 porcelain status | 모두 빈 출력, **clean** |
| Checkout | detached HEAD. 종료 `git symbolic-ref -q HEAD` exit 1 |

Tracked 수정·commit·추가 agent 생성은 하지 않았습니다. 다른 reviewer 보고서를 열람하지 않았습니다. 생성한 시험·빌드 산출물은 해당 worktree의 ignored `build/` 아래에만 있습니다. 기존 main checkout의 도구와 허용된 mutant source는 읽기 전용으로 사용했습니다.

## 2. 기존 finding closure

### B-P2-01 — GPIO 허용 범위 검증 누락: FIXED

수정 위치:

- [generate_boards.py:16](F:/dev/canview-wt/recheck-foundation-b/tools/generate_boards.py:16): SoC 및 모듈별 allowlist.
- [generate_boards.py:117](F:/dev/canview-wt/recheck-foundation-b/tools/generate_boards.py:117): 정확한 정수 타입 검사와 공통 JSON/CSV 검증.

원래 문제는 존재하지 않는 GPIO22–25, 메모리 점유·모듈 비노출 핀을 생성기가 승인할 수 있다는 것이었습니다.

현재는 S3R8/N16R8에서 GPIO22–37, N8R2에서 GPIO22–34를 거부합니다. SoC의 유효 범위와 octal 메모리 점유 구분은 고정 [Espressif 원문](https://github.com/espressif/esp-idf/blob/76f5dedd9950a3012fee8fb7d5586df21fc67802/docs/en/api-reference/peripherals/gpio/esp32s3.inc)과 대조했습니다.

독립 재현 결과:

- 실제 JSON/CSV를 메모리에서 변형한 **부정 입력 64개 모두 거부**.
- 음수, 범위 초과, 모듈 금지 핀, JSON `True`, `False`, `6.0`, `"6"`, `null`, 배열·객체 포함.
- 정상 입력 **13개 수락** 및 생성 매크로 값 확인.
- 특히 **N8R2 GPIO35·36·37은 수락**하여 octal 제한을 quad 모듈에 과잉 적용하지 않음을 확인.
- 저장소 generator unittest **7그룹 PASS**.

원래 재현 조건은 차단됐습니다. 다만 이 allowlist는 핀 존재·메모리 점유 검증이지, 전기적 연결·strapping·주변장치 충돌·제작 승인 검증은 아닙니다.

### B-P2-02 — CAN 필드 손상을 놓치는 시험: FIXED

수정 위치:

- [test_foundation.c:281](F:/dev/canview-wt/recheck-foundation-b/tests/foundation/test_foundation.c:281): `bus_id`, `flags`, `dlc` 비교 추가.
- [wire_vectors.c:16](F:/dev/canview-wt/recheck-foundation-b/tests/foundation/wire_vectors.c:16): CAN batch 생성 및 decode 전체 필드 비교.
- [test_vectors.py:43](F:/dev/canview-wt/recheck-foundation-b/tests/foundation/test_vectors.py:43): Python의 독립 little-endian byte 기준 추가.

원래 문제는 codec 자체 결함이 아니라, bus 손상 mutant가 기존 시험을 통과하는 **검출 공백**이었습니다.

현재 golden은 envelope/COBS 2,195개에 CAN batch 208개를 더한 **2,403개**를 비교합니다. CAN에는 count 0–12, bus 0–2, flags 16종, DLC 0–8, 비대칭 data와 11/29-bit ID가 포함됩니다. 필드 순서·폭·endian은 정본 ESP-NOW §11.1 및 UART §5와 대조했습니다.

허용된 mutant source 세 개의 diff를 직접 확인한 뒤, 기존 exe를 사용하지 않고 새 worktree에서 strict C99로 각각 다시 컴파일했습니다.

| 변이 | C CAN 시험 | C vector 프로그램 | Python golden |
|---|---:|---:|---:|
| encoder bus를 항상 0으로 기록 | 실패 | 실패 | 실패 |
| bus 기록 offset을 2→1로 변경 | 실패 | 실패 | 실패 |
| encoder와 decoder의 delta endian을 함께 반전 | **통과** | **2,403개 출력·통과** | **byte 불일치로 실패** |

세 변이 모두 컴파일은 성공했습니다. 특히 마지막 결과는 roundtrip의 자기 일치만으로 놓치는 ABI 손상을 새 독립 golden이 검출한다는 직접 증거입니다.

권고는 해당 시험을 유지하고, 향후 layout 변경에서도 roundtrip과 독립 byte golden을 함께 갱신하는 것입니다.

## 3. 전체 delta 회귀 검토

비교 범위는 최초 후보 → post-fix의 **21파일, +195/−41줄**입니다. 코드·설계·빌드·시험 변경을 검토했습니다. Journal 4줄 추가는 변경 목록·규모만 확인했으며 역사 본문이나 타 reviewer finding을 근거로 사용하지 않았습니다.

- **PLAY16/REC14:** JSON, 생성 header, fixture, 하드웨어 문서가 일치합니다. 공식 고정 Waveshare source의 `gpio_cfg.dout=16`, `gpio_cfg.din=14`와 대조했습니다. 혼동 가능한 원문 주석 대신 실제 MCU 설정을 기준으로 판단했습니다. [Waveshare 원문](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.5/blob/283ec84c566c096f8c30493b93dcd4b0bb608de7/ESP-IDF/01_factory/components/esp_port/esp_es8311_port.cpp#L20)
- **C99/SDK 분리:** ESP main 세 개는 startup/BSP를 strict C99로 구성하고, SDK GPIO adapter는 별도 `canview_esp32_platform` component가 소유합니다. 네 startup 역할의 실제 compile command에서 역할 정의와 `-std=c99`, 필수 경고 옵션을 확인했습니다. 이는 **host object compile**이며 IDF link 성공 증거는 아닙니다.
- **생성물 일치:** transport·board `--check` 통과. 추가로 생성물 읽기 결과에 drift·누락을 메모리 주입했을 때 두 생성기 모두 exit 1로 검출했습니다. 추적 파일은 변경하지 않았습니다.
- **CI locale:** CTest Python 진입점 6개에 `-X utf8`이 실제 전달됐습니다. 부모 환경을 `PYTHONUTF8=0`으로 설정해도 6/6 통과하고 한글 진단이 출력됐습니다.
- **Brightness 경고 수정:** `9U→9`는 정수 승격에 맞춘 시험 피연산자 수정입니다. 앞선 단조 감소 assertion을 유지하며 제품 runtime은 변경하지 않습니다. Release의 legacy assertion 활성화도 확인했습니다.
- **Runtime 비변경:** shared protocol/app, startup, platform, STM32, protocol schema의 지정 경로 diff는 비어 있었습니다. malformed/CRC/COBS/sequence·오류 상태 시험은 새 빌드에서 다시 실행했습니다.

CRC를 인증으로 취급하지 않는 계약, caller 소유·단일 worker 계약, dispatch 미구현, bench partition과 미선택 OTA template의 구분도 유지됩니다. 이번 delta가 미구현 기능을 실행 가능하거나 차량 송신 승인 상태로 승격하는 경로는 확인하지 못했습니다.

## 4. 실제 검증 결과

| 검증 | 결과 |
|---|---|
| Windows Debug configure/build/CTest | **31/31 PASS**, 컴파일 경고 0 |
| Windows Release configure/build/CTest | **31/31 PASS**, 컴파일 경고 0 |
| Startup 네 역할 | Debug·Release strict C99 object compile PASS |
| Generator unittest | **7그룹 PASS** |
| 독립 golden | **2,195 envelope/COBS + 208 CAN PASS** |
| Coverage 전용 core 재실행 | **9/9 PASS** |
| Coverage | **lines 546/546, functions 20/20, branches 295/296** |
| Strict API 문서 | Doxygen/Sphinx 성공, **공개 함수 14개 XML 계약 PASS** |
| 기존 docs 환경 `pip check` | PASS |
| 독립 GPIO 시험 | 부정 64개·정상 13개 PASS |
| CAN 변이 검출 | **3/3 검출** |
| 생성물 drift·누락 주입 | 두 생성기 모두 검출 |
| `git diff --check` | PASS |
| Target VerifyOnly | **FAIL: `arm-none-eabi-gcc` 없음** |

실제 사용 버전은 Clang 23.1.0 GNU frontend, CMake 4.4.3, Ninja 1.13.2, Python 3.14.3, Doxygen 1.18.0, Sphinx 9.1.0, Breathe 4.36.0, Furo 2025.12.19입니다.

Coverage 분모는 공용 codec/app 두 파일입니다. BSP·startup·generator·SDK·legacy의 coverage로 확대 해석하지 않았습니다.

### 실행 명령

아래 명령은 지정 worktree에서 실행했습니다. Native 검증마다 지정 VS DevShell을 로드하고 manifest의 기존 LLVM/CMake/Ninja/Doxygen 경로를 PATH 앞에 배치했습니다. 대용량 bootstrap은 실행하지 않았습니다.

```powershell
& 'C:/Program Files (x86)/Microsoft Visual Studio/18/BuildTools/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 -SkipAutomaticLocation

clang --version
cmake --version
ninja --version
doxygen --version

cmake --preset host-debug -DPython3_EXECUTABLE=C:/Python314/python.exe
cmake --build --preset host-debug
ctest --preset host-debug

cmake --preset host-release -DPython3_EXECUTABLE=C:/Python314/python.exe
cmake --build --preset host-release
ctest --preset host-release

cmake --preset host-coverage -DPython3_EXECUTABLE=C:/Python314/python.exe
cmake --build --preset host-coverage
& C:/Python314/python.exe -X utf8 -B tools/check_coverage.py

& F:/dev/canview/.tools/docs-venv/Scripts/python.exe -X utf8 -B tools/build_docs.py
& F:/dev/canview/.tools/docs-venv/Scripts/python.exe -m pip check

$env:PYTHONUTF8='0'
ctest --preset host-debug -R '^(generated-transport|generated-boards|foundation-python|plan-validation|document-links|foundation-golden)$' -V
& C:/Python314/python.exe -X utf8 -B tests/foundation/test_generators.py -v

& C:/Python314/python.exe -X utf8 -B build/recheck-b/recheck_adversarial.py
& C:/Python314/python.exe -X utf8 -B build/recheck-b/recheck_drift.py

& ./tools/environment/setup-windows.ps1 -VerifyOnly
```

격리·비교에는 시작/종료 `git rev-parse HEAD`, `git status --porcelain=v1`, detached 확인과 다음 명령을 사용했습니다.

```powershell
git diff --stat 59ac4046a02c3bb4d2612f4143d650ff4095f5b2 HEAD
git diff --check 59ac4046a02c3bb4d2612f4143d650ff4095f5b2 HEAD
git diff --exit-code 59ac4046a02c3bb4d2612f4143d650ff4095f5b2 HEAD -- shared/protocol shared/app firmware/app/startup.c firmware/platform firmware/communicator/stm32 protocol/schema
```

재현 스크립트와 정확한 mutant 컴파일 명령은 다음 ignored 파일에 남겼습니다.

- [recheck_adversarial.py](F:/dev/canview-wt/recheck-foundation-b/build/recheck-b/recheck_adversarial.py)
- [recheck_drift.py](F:/dev/canview-wt/recheck-foundation-b/build/recheck-b/recheck_drift.py)

별도 보고서 파일은 작성하지 않았습니다. 이 최종 메시지가 보고서 원문입니다.

### 실패·재시도 기록

- 독립 GPIO 시험 첫 실행은 양성 fixture에 이미 `MIC_BCLK`로 사용 중인 GPIO38을 지정하여 `duplicate GPIO`로 실패했습니다. **리뷰 시험 fixture만** 빈 핀으로 수정한 뒤 전체 재실행이 통과했습니다.
- `rg`에 Windows에서 유효하지 않은 경로 glob을 전달한 검색 한 번이 오류 123을 냈습니다. 디렉터리 경로로 수정하여 재검색했습니다.
- Mutant·drift 검사의 exit 1은 의도한 검출 결과입니다.
- Target VerifyOnly는 실제 환경 gate 실패입니다. 첫 차단이 Arm GCC 부재였으며 후속 SDK 검사는 도달하지 못했습니다.

## 5. 미실행 및 잔여 불확실성

- ESP-IDF/STM32 실제 target configure·compile·link, 실제 보드·HIL, GPIO 파형, PSRAM, clock/DMA/UART/FDCAN, reset·brownout·TX gate 검증은 **미실행**입니다.
- Flash, radio 시작, CAN 사용, erase, provisioning은 하지 않았습니다.
- Linux GCC/ASan/UBSan과 최신 원격 CI는 직접 실행·조회하지 않았습니다. 코디네이터의 별도 확인 대상이며, 과거 CI 성공을 이번 로컬 결과에 합산하지 않았습니다.
- 기존 pinned 도구와 docs 환경을 재사용했습니다. 신규 bootstrap 및 깨끗한 환경의 hash-lock 재설치는 하지 않았습니다.
- 영문 Windows runner 자체를 재현한 것은 아닙니다. 로컬에서 환경 UTF-8 설정을 끄고 CTest의 명시적 override를 검증했습니다.
- 이전 독립 대량 무작위 실험 전체를 다시 돌리지는 않았습니다. 변경 없는 runtime에는 이번 Debug/Release core 시험과 golden·coverage 재실행을 적용했습니다.
- 전체 ABI catalog/auth/session/ACK/QoS/filter, 실제 driver/ISR/RTOS, OTA/LVGL/audio/vehicle 기능은 여전히 미구현입니다.

## 최종 결론

**PASS.** B-P2-01/B-P2-02의 원래 실패 조건이 닫혔고, 특히 roundtrip을 통과하는 endian 동시 반전까지 새 golden이 검출했습니다. 검토한 post-fix 기능 delta에서 새 merge-code 차단 사유는 발견하지 못했습니다.

`embedded-architecture`, `embedded-cstyle`, `embedded-documentation`, `embedded-driver-design` 기준을 적용해 계층·SDK 분리, 오류/소유권 계약, 문서의 미구현 경계를 검토했습니다. 사용자 지시에 따라 C99를 우선했으며 Hallmark는 사용하지 않았습니다.
````
