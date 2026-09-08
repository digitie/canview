판정: **BLOCK — P1 1건, P2 2건.** 물리/HIL은 **NOT_RUN**, 차량 CAN TX는 **NO-GO**다.

실행 ID: `CV-HOSTILE-20260908-T102-POSTFIX-B-002`  
실제 시작: `2026-09-08T18:18:53.7499403+09:00`  
실제 종료: `2026-09-08T18:26:53.9751858+09:00`  
Candidate: `eb0155e26c46fa2dc52fb64cfc11c502951a1814`  
Base: `1ebd5f575a9756fb08710021e3c9b882a8ce980f`

독립 read-only object 리뷰다. 저장소 내용은 지정 commit의 `git show`, `git diff`, `git ls-tree`로 읽었다. 저장소 파일·index·branch 변경, commit, push, worktree 생성은 하지 않았다. 다른 리뷰어의 원본 보고서는 읽지 않았다. 리뷰 규칙과 archive index만 참조했다. 공유 checkout의 현재 HEAD나 기존 build 산출물을 candidate 검증 근거로 사용하지 않았으며 checkout clean 상태도 주장하지 않는다.

범위는 STM32 빌드·설정·통합·보안·evidence다. CAPTURE_ONLY 강제 계약, 공용 translation unit, source/API/register/symbol gate, CMake 의존, 보드 생성물, sdkconfig 부정·변이 시험, 실제 compiler 거부 동작, CubeG4 provenance, CI manifest, 자원 경계, Diagnostic Bridge 분리를 검토했다.

**Finding B-001 — P1 — 깨끗한 CubeG4 checkout에서 target CI가 중단됨**

위치: [foundation.yml:215](/F:/dev/canview/.github/workflows/foundation.yml:215)

```powershell
$cubeStatus = (& git -C $cubeRoot status --porcelain=v1).Trim()
```

정상적인 clean checkout에서 Git은 stdout을 출력하지 않는다. PowerShell에서는 결과가 `$null`이므로 `.Trim()`이 예외를 일으킨다. 해당 step은 `$ErrorActionPreference = 'Stop'`이므로 target configure/build에 도달하지 못한다.

실제 고정 SDK에서 재현했다.

```powershell
$sdk = 'C:/cv/STM32CubeG4-1.6.3'
@(git --no-optional-locks -C $sdk status --porcelain=v1).Count
# 0

(& git --no-optional-locks -C $sdk status --porcelain=v1).Trim()
# You cannot call a method on a null-valued expression.
```

영향: 정상 SDK 설치 상태에서 필수 target-firmware CI와 새로운 provenance 산출이 실패한다. 기존 target 성공 기록으로 이 후보의 CI gate를 닫을 수 없다.

권고: setup script의 기존 구현처럼 stdout을 `-join`으로 문자열화한 뒤 검사하고, Git exit code도 보존한다. stdout 0행·1행·여러 행과 Git 실패를 각각 시험한다.

```powershell
$cubeStatus = ((& git -C $cubeRoot status --porcelain=v1) -join "`n").Trim()
```

Disposition: `OPEN`.

**Finding B-002 — P2 — 모든 STM32 C translation unit에 대한 강제 계약이 성립하지 않음**

위치: [STM32 CMakeLists.txt:111](/F:/dev/canview/firmware/communicator/stm32/CMakeLists.txt:111)  
관련: [check_stm32_core.py:130](/F:/dev/canview/tools/check_stm32_core.py:130), [check_stm32_build_mode.py:9](/F:/dev/canview/tools/check_stm32_build_mode.py:9)

실행 target, `canview_stm_core`, BSP provider에는 composition token과 강제 header가 있다. 그러나 `canview_wire`, `canview_app`에는 MCU/section/stack 옵션만 추가된다. 실행 target의 `PRIVATE` 옵션은 이 라이브러리들의 컴파일로 전파되지 않는다.

따라서 아래 5개 C unit은 task에 명시된 target-wide contract에서 빠진다.

- `canview_wire.c`
- `canview_uart.c`
- `canview_espnow.c`
- `canview_espnow_contract.c`
- `canview_app.c`

source gate도 STM32 디렉터리만 순회한다. 공용 소스는 검사하지 않는다. negative fixture는 스스로 `-include`와 token을 추가하므로 실제 CMake에서 이 옵션이 빠져도 성공할 수 있다.

실제 GCC/Clang에서 강제 header가 없는 다음 입력은 exit 0이었다.

```c
#define CANVIEW_STM_TX_PERMIT 1
int capture_only_probe = CANVIEW_STM_TX_PERMIT;
```

영향: 공용 라이브러리의 계약 누락이나 override를 현재 검증으로 탐지하지 못한다. 현재 후보에 이 우회를 사용하는 TX 구현이 있다는 뜻은 아니다.

권고: STM32 composition에서 모든 연결 C target에 공통 contract target을 적용하고, 실제 compile database에서 모든 C unit의 token·강제 include를 검증한다. 해당 옵션을 제거하는 mutation도 실패해야 한다.

Disposition: `OPEN`.

**Finding B-003 — P2 — source gate가 정상 C 문법의 TX register 접근·mode override를 놓침**

위치: [check_stm32_core.py:10](/F:/dev/canview/tools/check_stm32_core.py:10), [check_stm32_core.py:54](/F:/dev/canview/tools/check_stm32_core.py:54)  
관련: [test_stm32_core_gate.py:46](/F:/dev/canview/tests/test_stm32_core_gate.py:46)

다음 두 입력은 candidate의 source gate를 통과했고, 고정 CMSIS header를 사용하는 실제 Arm GCC에서도 exit 0이었다.

```c
void send(void)
{
    FDCAN1
        ->TXBAR = 1U;
}
```

```c
void send(void)
{
    FDCAN_GlobalTypeDef *bus = FDCAN1;
    bus->TXBAR = 1U;
}
```

검사는 물리적 한 줄 단위이며 register 패턴은 `FDCAN1..3` 직접 접근만 찾는다. 포인터 별칭과 줄바꿈을 놓친다. 일반 이름 `send`는 symbol 검사에서도 거부되지 않았다.

전처리 line continuation을 사용한 다음 override도 패턴을 통과했고, 실제 Arm GCC에서 candidate header 뒤에 연결했을 때 exit 0이었다.

```c
#undef \
CANVIEW_STM_TX_PERMIT
#define \
CANVIEW_STM_TX_PERMIT 1
int probe = CANVIEW_STM_TX_PERMIT;
```

영향: source/API/register gate를 포괄적인 TX 차단 증거로 사용할 수 없다. 현재 후보에 위 코드가 존재하거나 차량 송신이 가능함을 입증한 것은 아니다.

권고: 전처리 logical line을 처리하고, register member 접근은 별칭을 고려하는 AST 검사 또는 현재 capture-only 범위에 맞춘 명시적 금지 검사를 추가한다. 재현 입력을 regression fixture로 보존한다. 검사의 보장 범위도 문서에 한정한다.

Disposition: `OPEN`.

P0와 P3 finding은 없다.

**실행한 검사와 결과**

아래 명령은 PowerShell에서 실행했다. 파일 읽기 반복문은 뒤의 파일 목록에 대해 `git show "${r}:$f"`를 실행했으며, 일부는 행 번호를 붙이거나 필요한 절만 출력했다.

| 명령·실행 | 결과 |
|---|---|
| 시작·종료 `Get-Date -Format o` | 위 실제 시각 기록 |
| 양 hash의 `git cat-file -e`, 종료 시 `^{commit}` 검사 | 모두 성공 |
| 양 hash의 `git rev-parse`, 종료 시 `^{commit}` 해석 | 지정 hash와 일치 |
| `git diff --find-renames --stat <base> <candidate>` | 41 files, 1,895 additions, 68 deletions |
| `git diff --find-renames <base> <candidate> -- CMakeLists.txt …` | root CMake, main, linker, platform delta 확인 |
| `git diff --find-renames <base> <candidate> -- . ':(exclude)docs/reviews/**'` | 비-review delta 검사, 추가 1,503행 |
| `git ls-tree -r --name-only <candidate> firmware/communicator/stm32` | STM32 파일 목록 확인, sdkconfig.defaults 없음 |
| `git show` 반복 및 `Select-Object`/`Select-String` | 아래 파일·절 확인. 큰 출력 일부는 도구에서 잘려 해당 부분의 정독을 주장하지 않음 |
| 두 embedded skill의 `Get-Content` | architecture/C style 규칙 확인 |
| `Get-Command python,clang,arm-none-eabi-gcc,cmake,pwsh` | Python·pwsh 발견, 일반 PATH에 compiler/CMake 없음 |
| 도구 경로 확인용 `Get-ChildItem` | 아래 탐색 경로에서 LLVM, Arm GNU, CubeG4 위치 확인. `C:/tools` 조회는 부재로 exit 1 |
| `git for-each-ref`의 정상 0행 출력에 `.Trim()` | null-method 예외 재현 |
| 실제 clean CubeG4 `git status` 출력에 `.Trim()` | 같은 예외 재현 |
| GCC·Clang `--version` | Arm GCC 15.3.1 / Clang 23.1.0 |
| candidate header를 stdin에 연결한 `-x c -std=c99 -Werror -Wundef -fsyntax-only -` | 정상 token: 양 compiler exit 0 |
| mode/capture-mode/capability/TX permit/bench TX/vehicle TX override | 양 compiler 모두 exit 1 |
| token 누락 | 양 compiler 모두 exit 1 |
| token=0 | 단일 정의 재검사에서도 `contract_must_be_one`, exit 1 |
| include guard만 사전 정의하고 probe 컴파일 | mode 식별자 미정의로 양 compiler exit 1 |
| header 없는 override probe | 양 compiler exit 0, B-002 근거 |
| `-include -`를 통한 무파일 forced-header 시도 | 양 compiler가 `-`를 파일명으로 해석하여 실패. 유효한 negative PASS로 계산하지 않음 |
| 첫 stdin 재시도의 Python 기본 decoding | cp949 UnicodeDecodeError. `python -X utf8 -B -`로 재실행 |
| candidate Python generator/sdkconfig 시험을 Git object 읽기로 연결 | 파일 생성 CLI 시험을 제외한 21개 top-level test 통과 |
| board generator의 13개 출력 비교 | drift 0 |
| forbidden-setting 제거 mutation 및 malformed/duplicate 설정 | 위 21개 시험에 포함, 통과 |
| candidate source gate, 모든 STM32 `.c/.h` 객체 | baseline 통과 |
| HAL TX / LL TX / 직접 한 줄 TXBAR / 한 줄 `#undef` | 모두 거부 |
| 줄바꿈 TXBAR / pointer alias / continued directive | source gate 통과, B-003 근거 |
| 두 register 우회 입력 + 고정 CMSIS, 실제 Arm syntax compile | 모두 exit 0 |
| continued directive 첫 compiler 입력 | 끝 newline 누락 fixture 오류로 exit 1. 수정한 fixture는 exit 0 |
| required symbol을 갖춘 fixture에 heap/TX symbol 추가 | malloc/calloc/realloc/free/_sbrk/HAL TX/LL TX 거부 |
| 같은 fixture에 일반 이름 `send` 추가 | 허용 |
| memory gate: Flash 초과, RAM 초과, 음수, 빈 입력 | 모두 거부 |
| stack gate: 빈 목록, 2,049-byte frame, unbounded dynamic, malformed | 모두 거부 |
| 비-review 추가 행의 private-key/token/VIN 고신뢰 패턴 검사 | 일치 파일 0. 포괄적인 secret 부재 증명은 아님 |

도구 경로 탐색에는 다음 디렉터리의 이름 목록만 사용했다.

```text
F:/dev
F:/dev/canview-external
F:/dev/canview/.tools
F:/
C:/
C:/tools
C:/cv
C:/Users/digit/.cache
C:/Users/digit/.espressif
C:/Users/digit/AppData/Local/CANView
C:/Users/digit/AppData/Local/CANView/toolchains
```

실제 compiler 경로:

```text
C:/Users/digit/AppData/Local/CANView/toolchains/arm-gnu-toolchain-15.3.rel1/bin/arm-none-eabi-gcc.exe
F:/dev/canview/.tools/llvm-23.1.0/clang+llvm-23.1.0-x86_64-pc-windows-msvc/bin/clang.exe
```

파일 변경 금지를 지키기 위해 Python module은 candidate 객체에서 메모리로 로드했다. generator/sdkconfig 시험의 파일 읽기는 같은 객체로 연결했다. source gate 공격은 메모리 fixture로 실행했다. compiler 입력은 stdin을 사용했고 ELF/object를 생성하지 않았다. 따라서 **원본 tempfile 기반 negative CLI나 실제 CMake forced-include 전체 실행을 완료했다고 주장하지 않는다.**

**STM32CubeG4 provenance**

다음 명령을 실제 설치본에 실행했다.

```powershell
git -C C:/cv/STM32CubeG4-1.6.3 cat-file -e 'd11b194a9f05d1b143d154771f3dbc282c8052a5^{commit}'
git -C C:/cv/STM32CubeG4-1.6.3 rev-parse HEAD 'HEAD^{tree}' 'v1.6.3^{commit}'
git -C C:/cv/STM32CubeG4-1.6.3 remote get-url origin
git --no-optional-locks -C C:/cv/STM32CubeG4-1.6.3 status --porcelain=v1
git -C C:/cv/STM32CubeG4-1.6.3 submodule status --recursive
```

결과:

- HEAD와 `v1.6.3^{commit}`: `d11b194a9f05d1b143d154771f3dbc282c8052a5`
- Tree: `f91beb5bd319d2fae53a9bc38d4d1149237fa616`
- Origin: `https://github.com/STMicroelectronics/STM32CubeG4.git`
- 최상위 status: 0행
- recursive submodule status: 18개, 불일치 접두사 `-`, `+`, `U` 없음
- CMSIS device: `25664ddc3a7624ae9627ae8c4c672073dc5b2539`
- HAL: `f5929f431f9effe45fbe18f5337e4753ced9ac92`

candidate register model에서 추출한 **54개 상수**를 실제 SDK header와 C99 compile-time 비교했다. Arm GCC exit 0이었다. 개별 nested worktree 전체 dirty 검사와 Arm archive 전체 digest 재검증은 수행하지 않았다.

**실제로 읽은 파일**

아래 목록은 사람에게 출력해 검토한 파일과 object 기반 자동 검사에서 읽은 파일을 포함한다. 자동 scanner가 읽었다는 이유로 모든 C runtime을 정독했다고 주장하지 않는다.

```text
F:/dev/canview/AGENTS.md
F:/dev/canview/docs/README.md
F:/dev/canview/docs/resume.md
F:/dev/canview/docs/tasks/T-102-stm32-platform.md
F:/dev/canview/docs/runbooks/agent-workflow.md
F:/dev/canview/docs/reviews/README.md
F:/dev/canview/docs/development/windows.md
F:/dev/canview/docs/development/toolchains.md
F:/dev/canview/docs/architecture/README.md
F:/dev/canview/docs/architecture/firmware-foundation.md
F:/dev/canview/docs/journal.md
F:/dev/canview/docs/tasks.md
F:/dev/canview/.github/workflows/foundation.yml
F:/dev/canview/CMakeLists.txt
F:/dev/canview/CMakePresets.json
F:/dev/canview/cmake/CanviewWarnings.cmake
F:/dev/canview/shared/app/CMakeLists.txt
F:/dev/canview/shared/protocol/CMakeLists.txt
F:/dev/canview/shared/protocol/include/canview_wire_layout.h
F:/dev/canview/protocol/schema/transport-foundation-v1.json
F:/dev/canview/firmware/boards/boards.json
F:/dev/canview/firmware/boards/waveshare35-pins.json
F:/dev/canview/hardware/communicator/pinmap.csv
F:/dev/canview/hardware/bridge/pinmap.csv
F:/dev/canview/firmware/communicator/stm32/CMakeLists.txt
F:/dev/canview/firmware/communicator/stm32/CMakePresets.json
F:/dev/canview/firmware/communicator/stm32/README.md
F:/dev/canview/firmware/communicator/stm32/cmake/arm-none-eabi-gcc.cmake
F:/dev/canview/firmware/communicator/stm32/docs/core-bench.md
F:/dev/canview/firmware/communicator/stm32/ld/STM32G474CEUx_FLASH.ld
F:/dev/canview/firmware/communicator/stm32/app/boot.c
F:/dev/canview/firmware/communicator/stm32/app/main.c
F:/dev/canview/firmware/communicator/stm32/bsp/board.c
F:/dev/canview/firmware/communicator/stm32/bsp/board_pins.h
F:/dev/canview/firmware/communicator/stm32/bsp/build_metadata.c
F:/dev/canview/firmware/communicator/stm32/bsp/core.c
F:/dev/canview/firmware/communicator/stm32/include/canview_auto_sport.h
F:/dev/canview/firmware/communicator/stm32/interface/canview_build_mode.h
F:/dev/canview/firmware/communicator/stm32/interface/canview_stm_board_core.h
F:/dev/canview/firmware/communicator/stm32/interface/canview_stm_build.h
F:/dev/canview/firmware/communicator/stm32/interface/canview_stm_core.h
F:/dev/canview/firmware/communicator/stm32/interface/canview_stm_diagnostic.h
F:/dev/canview/firmware/communicator/stm32/interface/canview_stm_queue.h
F:/dev/canview/firmware/communicator/stm32/interface/canview_stm_reset.h
F:/dev/canview/firmware/communicator/stm32/interface/canview_stm_service.h
F:/dev/canview/firmware/communicator/stm32/interface/canview_stm_stack.h
F:/dev/canview/firmware/communicator/stm32/module/CMakeLists.txt
F:/dev/canview/firmware/communicator/stm32/module/diagnostic.c
F:/dev/canview/firmware/communicator/stm32/module/queue.c
F:/dev/canview/firmware/communicator/stm32/module/scheduler.c
F:/dev/canview/firmware/communicator/stm32/module/service_policy.c
F:/dev/canview/firmware/communicator/stm32/module/stack_watermark.c
F:/dev/canview/firmware/communicator/stm32/platform/stm32g474/core_hw.c
F:/dev/canview/firmware/communicator/stm32/platform/stm32g474/core_hw.h
F:/dev/canview/firmware/communicator/stm32/platform/stm32g474/safe_gpio.c
F:/dev/canview/firmware/communicator/stm32/platform/stm32g474/safe_gpio.h
F:/dev/canview/firmware/communicator/stm32/platform/stm32g474/syscalls.c
F:/dev/canview/firmware/communicator/stm32/src/canview_auto_sport.c
F:/dev/canview/firmware/communicator/stm32/tests/register_model.h
F:/dev/canview/firmware/communicator/stm32/tests/test_core.c
F:/dev/canview/firmware/communicator/stm32/tests/test_platform.c
F:/dev/canview/firmware/communicator/stm32/tests/test_registers.c
F:/dev/canview/firmware/communicator/esp32/bsp/board_pins.h
F:/dev/canview/firmware/communicator/esp32/sdkconfig.defaults
F:/dev/canview/firmware/communicator/esp32/partitions.csv
F:/dev/canview/firmware/communicator/esp32/partitions.ota-template.csv
F:/dev/canview/firmware/controller/bsp/board_pins.h
F:/dev/canview/firmware/controller/sdkconfig.defaults
F:/dev/canview/firmware/controller/partitions.csv
F:/dev/canview/firmware/controller/partitions.ota-template.csv
F:/dev/canview/firmware/diagnostic-bridge/CMakeLists.txt
F:/dev/canview/firmware/diagnostic-bridge/main/CMakeLists.txt
F:/dev/canview/firmware/diagnostic-bridge/bsp/board_pins.h
F:/dev/canview/firmware/diagnostic-bridge/sdkconfig.defaults
F:/dev/canview/firmware/diagnostic-bridge/partitions.csv
F:/dev/canview/firmware/diagnostic-bridge/partitions.ota-template.csv
F:/dev/canview/tests/foundation/CMakeLists.txt
F:/dev/canview/tests/foundation/test_generators.py
F:/dev/canview/tests/foundation/test_sdkconfig.py
F:/dev/canview/tests/test_stm32_core_gate.py
F:/dev/canview/tools/check_stm32_build_mode.py
F:/dev/canview/tools/check_stm32_core.py
F:/dev/canview/tools/check_stm32_coverage.py
F:/dev/canview/tools/check_generated.py
F:/dev/canview/tools/check_sdkconfig.py
F:/dev/canview/tools/generate_boards.py
F:/dev/canview/tools/generate_transport.py
F:/dev/canview/tools/sdkconfig-allowlist/esp32s3-idf-6.0.3.keys
F:/dev/canview/tools/toolchain-versions.json
F:/dev/canview/tools/foundation-tools.json
F:/dev/canview/tools/environment/setup-windows.ps1
F:/dev/canview/tools/environment/foundation-windows.ps1
C:/Users/digit/.codex/skills/embedded-architecture/SKILL.md
C:/Users/digit/.codex/skills/embedded-cstyle/SKILL.md
```

`docs/journal.md`, `docs/tasks.md`는 비-review delta만 자동 검사했다. compiler는 위 SDK의 `stm32g474xx.h`와 전이 CMSIS/표준 header를 읽었다. compiler가 전이적으로 연 모든 header의 개별 파일 목록은 수집하지 않았다.

**공격했으나 추가 finding이 없었던 범위**

- 보드 profile uniqueness, hardware digest, 입력 변조·잘못된 메모리·pin·path와 생성물 비교: 실행한 시험 통과.
- Bridge routing/forwarding 및 blocking HTTP queue 설정의 활성화·필수 비활성 행 제거: sdkconfig 시험에서 거부.
- STM32 service policy는 trust flag가 모두 정상이어도 현재 capability 0/TX false를 반환한다. diagnostic encoder는 비영 capability/TX, invalid reset enum, 부족한 capacity를 거부하는 코드와 시험을 갖춘다.
- 추가 diagnostic record는 포인터를 wire buffer로 복사하지 않는다. 새로운 root 구조에도 key 원문이 없다.
- watermark arm의 최대 크기와 sample의 256-byte 한도, malformed context 거부가 존재한다. 물리 stack 안전이나 WCET를 이 사실로 승인하지 않는다.
- Bridge target CMake에 STM32 provider/command executor 의존을 추가한 변경은 없다. Bridge 변경은 생성 header digest 추가이며, 13개 생성물 비교가 통과했다.
- CI는 PR head revision, artifact byte 수·SHA-256과 SDK commit/tree를 기록하도록 설계됐다. B-001 때문에 해당 후보의 실제 CI 산출 성공은 아직 입증되지 않았다.
- BSP provider로 metadata 조립을 옮긴 변경에서 module의 직접 generated-header include는 발견하지 않았다.

**미검토·미실행 범위와 판정 조건**

- 전체 host Debug/Release CTest, coverage, ASan/UBSan, 원본 tempfile 기반 negative CLI: `NOT_RUN`.
- 실제 candidate CMake configure/link, ELF/MAP/BIN/HEX, compile database, `.su`와 warning-free target 재현: `NOT_RUN`.
- 전체 protocol generator drift·전체 저장소 secret scan·과거 Git history/private capture 감사: `NOT_RUN`.
- 현재 후보의 GitHub CI run·다운로드 artifact digest 대조: `NOT_RUN`. 과거 성공 기록은 대체 evidence로 인정하지 않는다.
- 완전한 runtime/ISR/stack call-chain 적대적 검증, bootloader/root Flash 배치, 실제 erase/provisioning: 본 실행에서 미검증.
- 실제 보드 flash, ST-LINK, UART, FDCAN, reset/brownout/rail, 외부 TX gate, clock/WCET, RF, 차량 bus와 HIL: **NOT_RUN**.

B-001 수정과 해당 target CI 재검증이 필요하다. B-002/B-003은 수정하거나 owner·task·gate·기한을 갖춘 P2 disposition이 필요하다. 현재 merge verdict는 **BLOCK**, 차량 CAN TX는 **NO-GO**다.
