실행 ID: `CV-HOSTILE-20260908-T102-POSTFIX-B-003`  
최종 판정: **CONDITIONAL — P0 0건 / P1 0건 / P2 2건 / P3 0건**

이전 세 finding의 직접적인 수정은 확인했다. 다만 새 compile-contract 검사와 source 검사에 재현 가능한 P2 검증 공백이 남아 있으므로 **PASS를 부여하지 않는다.**

- 실제 시작: `2026-09-08T19:02:31.9090265+09:00`
- 실제 종료: `2026-09-08T19:08:47.2622794+09:00`
- Candidate: `6bf55ec7bedd277220ab3713ecb56351f2e7e29d`
- Base: `1ebd5f575a9756fb08710021e3c9b882a8ce980f`
- 수정 비교점: 이전 직접 검토 candidate `eb0155e26c46fa2dc52fb64cfc11c502951a1814`
- 격리: immutable Git object 기반 read-only. 공유 checkout·index·branch 변경 없음. 기존 build artifact 사용 없음. 다른 reviewer 원본 report 열람 없음.

검토 범위는 CAPTURE_ONLY, CMake/component 의존, compile contract, source/API/register/symbol gate, SDK·보드·generator, sdkconfig 부정·변이 시험, 자원 경계, warning/reproducibility 설정, CI provenance, 생성물 drift, Windows 문서 링크와 Diagnostic Bridge 분리다.

**이전 finding 재검증**

| 이전 finding | 이번 확인 | 판정 |
|---|---|---|
| B-001: clean SDK의 null `.Trim()` | 출력 배열 수집, exit code 보존, `-join` 후 Trim으로 수정. 실제 clean SDK 및 1행/2행 dirty·실패 입력 검증 | **FIXED** |
| B-002: 공용 C unit forced contract 누락 | `canview_wire`, `canview_app`에 token과 forced header 추가. 공용 source 경로도 scanner에 추가 | 원래 CMake 누락은 수정됨. 실제 compile database 검증은 **NOT_RUN**이며 새 B-004 존재 |
| B-003: 줄바꿈·alias·continued directive 누락 | 원래 반례들이 candidate regression에서 거부됨 | 원래 반례는 수정됨. 주석을 사용하는 동일 계열 반례 B-005 존재 |

**B-004 — P2 — compile-contract 검사가 실제 forced header를 확인하지 않음**

위치: [check_stm32_core.py:80](/F:/dev/canview/tools/check_stm32_core.py:80), [check_stm32_core.py:95](/F:/dev/canview/tools/check_stm32_core.py:95)

현재 검사는 전체 명령 문자열에 `-include`와 header **basename**이 각각 존재하는지만 확인한다. 두 문자열이 같은 옵션·인자 쌍인지, 지정한 canonical header 경로인지 확인하지 않는다.

실제 candidate 함수에서 다음 입력이 모두 `1`을 반환하여 승인됐다.

```text
cc -DCANVIEW_STM_CAPTURE_ONLY_CONTRACT=1
   -include F:/stale/canview_build_mode.h
```

```text
cc -DCANVIEW_STM_CAPTURE_ONLY_CONTRACT=1
   -include stdint.h
   -DHEADER_LABEL=canview_build_mode.h
```

두 번째 경우에는 실제 Arm GCC로 다음 source를 컴파일했다.

```c
#ifdef CANVIEW_STM_BUILD_MODE
#error Unexpected_contract_header
#endif
int probe = 1;
```

결과: **compiler exit 0**, compile gate도 승인했다. 즉 보호 header가 주입되지 않았다는 compiler 증거와 gate 승인이 동시에 성립했다.

추가로 `-DCANVIEW_BUILD_MODE_H=1`을 넣은 entry도 gate가 승인했다. 이는 사전 정의된 include guard를 검사하지 않는다는 관찰이며, 이 flag만으로 전체 현재 firmware가 성공적으로 빌드됨을 주장하지 않는다.

실패 시나리오: 잘못된 이전 checkout의 동명 header를 포함하거나, 관련 없는 forced include와 header-name 문자열이 명령에 함께 존재하면 canonical contract 누락을 놓친다.

영향: 새 compile database gate가 제공하려는 artifact 계약 증명이 실제 옵션과 일치하지 않는다. 현재 정상 CMake 명령 자체가 잘못됐다는 finding은 아니다.

권고:

- `arguments` 또는 적절히 token화한 `command`에서 `-include`와 인자를 쌍으로 검사한다.
- entry의 directory를 기준으로 경로를 해석하고 expected canonical header와 비교한다.
- token의 실제 정의·해제·재정의 및 contract 비활성화 옵션을 검사한다.
- 위 wrong-path/unrelated-include mutation을 회귀시험에 추가한다.

Disposition: **OPEN**.

**B-005 — P2 — C 주석으로 source TX/mode 검사를 통과할 수 있음**

위치: [check_stm32_core.py:12](/F:/dev/canview/tools/check_stm32_core.py:12), [check_stm32_core.py:55](/F:/dev/canview/tools/check_stm32_core.py:55)

logical text 변환은 backslash-newline을 제거하지만, C 전처리에서 공백으로 취급하는 주석은 처리하지 않는다.

다음 세 입력을 candidate의 `check_source_safety()`에 각각 넣었다. 모두 **True**를 반환했다.

```c
void send(void)
{
    FDCAN1-> /* command */ TXBAR = 1U;
}
```

```c
void send(void)
{
    FDCAN_GlobalTypeDef *bus = FDCAN1;
    bus-> /* command */ TXBAR = 1U;
}
```

```c
#undef /* contract */ CANVIEW_STM_TX_PERMIT
#define /* contract */ CANVIEW_STM_TX_PERMIT 1
int probe = CANVIEW_STM_TX_PERMIT;
```

candidate build-mode header와 실제 고정 CMSIS header를 연결하여 Arm GCC C99 syntax 검사도 수행했다. 세 입력 모두 **exit 0**, stderr 비어 있음이었다.

실패 시나리오: 설명 주석이 member operator와 register 이름 사이 또는 전처리 directive에 들어간 정상 C 문법을 scanner가 놓친다.

영향: 원래 줄바꿈·alias 반례는 차단됐지만 같은 TX/mode 검사 경계의 누락이 남는다. 현재 후보가 차량 TX를 실행한다는 증거는 아니다.

권고: C lexical 규칙에 따라 comment를 공백으로 정규화하거나 token/AST 기반 검사를 사용하고, 세 반례를 regression에 포함한다. 검사 보장 범위도 문서에 명시한다.

Disposition: **OPEN**.

**실제로 실행한 명령과 결과**

`git show`·읽기 필터는 candidate 객체에 대해서만 실행했다. Python 시험은 객체에서 module을 메모리로 로드하고 파일 입출력을 메모리 fixture/Git 객체 읽기로 연결했다. 이 방법은 source 수정과 시험용 파일 생성 없이 검증하기 위한 것이며, 원본 filesystem CLI 실행과 구분한다.

| 실행 | 결과 |
|---|---|
| 시작·종료 `Get-Date -Format o` | 위 실제 시각 |
| 양 commit의 `git cat-file -e '<hash>^{commit}'` | 시작·종료 모두 성공 |
| 양 commit의 `git rev-parse '<hash>^{commit}'` | 지정 hash와 일치 |
| 이전 candidate→새 candidate `git diff --find-renames --stat`, review 제외 | 13 files, +194/-60 |
| base→candidate `git diff --find-renames --numstat`, review 제외 | 전체 변경 범위 확인 |
| base→candidate 비-review diff 자동 검사 | 추가 1,643행 |
| `git show <candidate>:<path>` 및 `Select-Object`/`Select-String` | 아래 코드·문서 확인 |
| 이전/새 candidate의 관련 24개 파일 blob hash 비교 | 모두 동일. 미변경 부분은 이전 직접 검토와 객체 동일성으로 연결 |
| STM32 core gate unittest, 메모리 filesystem 연결 | **6/6 통과** |
| generator/sdkconfig unittest, 객체 읽기 연결 | 파일 생성 CLI 시험 제외 **21/21 통과** |
| `check_generated.main([])`, 객체 filesystem 연결 | **exit 0**, 생성 출력 15개 검사 통과 |
| `check_generated.main(['--negative-fixture'])` | **exit 0**, drift mutation 거부 |
| protocol generator `write=False` | header + golden 15 + malformed 6 + compatibility 4 + pairing 검증 |
| STM32 source scanner | `.c/.h` 31개, baseline 통과 |
| 공용 app source scanner | C 1개, baseline 통과 |
| 공용 protocol source scanner | C 4개, baseline 통과 |
| B-004 compile database 반례 | 잘못된 entry 승인 |
| B-005 source 반례 | 세 입력 승인 |
| B-004/B-005 실제 Arm syntax 검사 | 보호 header 미포함 반례 및 주석 반례 모두 exit 0 |
| 수정된 CI clean-status 식, 실제 SDK | 0행/exit 0/문자열 길이 0, 정상 허용 |
| CI dirty 1행·2행 및 exit 1 fixture | 모두 거부 |
| document-link 기존 regression, 메모리 filesystem 연결 | **5/5 통과** |
| `/F:/…`, `F:/…`, `/mnt/f/…`와 `:line` fixture | 존재하는 세 경로 허용, 누락 경로 1개 거부, fenced 예제 제외 |
| 고신뢰 private-key/token/VIN 패턴, 비-review 추가 행 | 일치 0 |
| tracked 파일 목록의 `compile_commands.json` 검색 | 없음 |

생성물 검사 첫 시도에서는 리뷰 harness의 `Path.glob` 대체 함수가 `case_sensitive` 인자를 지원하지 않아 `TypeError`가 발생했다. harness의 `glob/rglob/is_file` 연결을 보완한 후 재실행이 성공했다. 이를 repository 결함이나 최초 PASS로 계산하지 않았다.

실제 compiler 명령의 공통 옵션:

```text
C:/Users/digit/AppData/Local/CANView/toolchains/arm-gnu-toolchain-15.3.rel1/bin/arm-none-eabi-gcc.exe
-x c -std=c99 -Wall -Wextra -Werror -Wundef -fsyntax-only
-mcpu=cortex-m4 -mthumb -DSTM32G474xx
-DCANVIEW_STM_CAPTURE_ONLY_CONTRACT=1
-IC:/cv/STM32CubeG4-1.6.3/Drivers/CMSIS/Device/ST/STM32G4xx/Include
-IC:/cv/STM32CubeG4-1.6.3/Drivers/CMSIS/Core/Include
-
```

compiler 입력은 stdin을 사용했다. ELF/object는 생성하지 않았다.

**실제 compile database 확인의 한계**

CMake source상 실행 target·STM32 core·BSP provider·공용 wire/app에 token과 forced header가 적용된 것은 확인했다. post-build에서 compile database를 검사하도록 연결된 것도 확인했다.

그러나 **이 후보를 새로 configure하여 생성한 실제 compile database는 검사하지 않았다.** immutable 객체에는 해당 파일이 없고, read-only 수행 및 기존 artifact 사용 금지 조건에 따라 공유 checkout의 기존 database도 읽지 않았다. 따라서 “실제 모든 C unit의 컴파일 명령을 확인했다”거나 target build를 재현했다고 표시하지 않는다.

**SDK·CI·생성물 근거**

실제 설치 SDK에서 다음을 실행했다.

```powershell
git --no-optional-locks -C C:/cv/STM32CubeG4-1.6.3 status --porcelain=v1
git -C C:/cv/STM32CubeG4-1.6.3 rev-parse HEAD 'HEAD^{tree}' 'v1.6.3^{commit}'
```

결과:

- status 0행, exit 0
- HEAD/tag commit: `d11b194a9f05d1b143d154771f3dbc282c8052a5`
- Tree: `f91beb5bd319d2fae53a9bc38d4d1149237fa616`

이는 로컬 SDK 관찰이며 candidate CI 성공 증거가 아니다. Arm archive 전체 digest, nested SDK worktree 전체 상태, 현재 GitHub run과 내려받은 artifact digest는 이번 실행에서 재검증하지 않았다.

보드 생성물·profile·sdkconfig·partition과 protocol 생성물은 candidate 객체로 직접 재생성 결과를 비교했다. 기존 checkout의 generated 파일을 읽지 않았다.

**읽은 파일과 객체 범위**

직접 내용·delta를 확인한 주요 파일:

```text
F:/dev/canview/docs/resume.md
F:/dev/canview/docs/tasks/T-102-stm32-platform.md
F:/dev/canview/.github/workflows/foundation.yml
F:/dev/canview/firmware/communicator/stm32/CMakeLists.txt
F:/dev/canview/firmware/communicator/stm32/README.md
F:/dev/canview/firmware/communicator/stm32/docs/core-bench.md
F:/dev/canview/firmware/communicator/stm32/interface/canview_stm_stack.h
F:/dev/canview/firmware/communicator/stm32/module/stack_watermark.c
F:/dev/canview/firmware/communicator/stm32/platform/stm32g474/core_hw.c
F:/dev/canview/firmware/communicator/stm32/tests/test_platform.c
F:/dev/canview/firmware/communicator/stm32/tests/test_registers.c
F:/dev/canview/tests/test_stm32_core_gate.py
F:/dev/canview/tests/test_document_links.py
F:/dev/canview/tests/foundation/test_generators.py
F:/dev/canview/tests/foundation/test_sdkconfig.py
F:/dev/canview/tools/check_stm32_core.py
F:/dev/canview/tools/validate_document_links.py
F:/dev/canview/tools/check_generated.py
F:/dev/canview/tools/check_sdkconfig.py
F:/dev/canview/tools/generate_boards.py
F:/dev/canview/tools/generate_transport.py
F:/dev/canview/tools/generate_protocol.py
F:/dev/canview/tools/generate_uart_protocol.py
F:/dev/canview/tools/toolchain-versions.json
F:/dev/canview/tools/sdkconfig-allowlist/esp32s3-idf-6.0.3.keys
```

source scanner가 실제 읽은 C/header 객체:

```text
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
F:/dev/canview/shared/app/src/canview_app.c
F:/dev/canview/shared/protocol/src/canview_espnow.c
F:/dev/canview/shared/protocol/src/canview_espnow_contract.c
F:/dev/canview/shared/protocol/src/canview_uart.c
F:/dev/canview/shared/protocol/src/canview_wire.c
```

generator가 실제 읽은 입력·출력에는 다음이 포함된다.

- 네 보드의 generated header, 세 ESP32의 `sdkconfig.defaults`, `partitions.csv`, `partitions.ota-template.csv`
- `F:/dev/canview/firmware/boards/boards.json`
- `F:/dev/canview/firmware/boards/waveshare35-pins.json`
- `F:/dev/canview/hardware/bridge/pinmap.csv`
- `F:/dev/canview/hardware/communicator/pinmap.csv`
- `F:/dev/canview/protocol/canview_protocol.h`
- `F:/dev/canview/protocol/canview_uart_protocol.h`
- `F:/dev/canview/protocol/schema/espnow-v1.3.yaml`
- `F:/dev/canview/protocol/schema/navigation-v1.json`
- `F:/dev/canview/protocol/schema/transport-foundation-v1.json`
- `F:/dev/canview/protocol/schema/uart-v1.0.yaml`
- `F:/dev/canview/shared/protocol/include/canview_espnow_contract.h`
- `F:/dev/canview/shared/protocol/include/canview_wire_layout.h`
- `F:/dev/canview/shared/protocol/src/canview_espnow_contract.c`

또한 `F:/dev/canview/protocol/golden/espnow-v1.3/`에서 아래 각 이름의 `.bin`과 `.json`을 읽었다.

```text
bulk-ack, bulk-begin, bulk-end, bulk-fragment, capabilities,
capture-status, command-retry, config-get, config-result,
config-schema-request, config-set, hello, pair-discovery,
remote-config-request, remote-config-status
```

같은 디렉터리의 `pairing-negative.json`, `malformed/`의 `bad-crc`, `bad-magic`, `payload-length-overrun`, `reserved-header`, `truncated-header`, `unknown-flags`, `compatibility/`의 `future-minor-same-major`, `major-mismatch`, `unknown-critical-tlv`, `unknown-noncritical-tlv`에 해당하는 `.bin/.json`도 읽었다. 생성물 재검사 한 번의 객체 읽기 목록은 82개였다.

미변경 정책·root CMake·shared CMake·Bridge CMake·generator/config/coverage 도구 일부는 이전 직접 검토와 현재 blob hash 동일성을 확인했다. 다른 reviewer report의 내용을 재사용하지 않았다.

**추가 finding 없이 확인한 공격 범위**

- 기존 HAL/LL TX, 직접 register, 줄바꿈 register, pointer alias, continued mode override fixture: 수정 후 거부.
- token/header 누락, C unit 없는 compile database fixture: 거부.
- memory 초과·음수·malformed, 누락·빈·중복 stack evidence, heap/TX symbol fixture: 회귀시험 통과.
- board/profile uniqueness, memory/pin/path 변조, sdkconfig 필수값 누락·금지값 활성화·중복/잘못된 문법·금지항목 제거 mutation: 실행 범위에서 통과.
- Bridge routing/forwarding·blocking HTTP queue 경계: sdkconfig 시험 통과.
- checkerboard watermark와 low-address prefix scan 변경은 최대 256-byte lower bound라는 문서 계약과 일치한다. 전체 실제 stack 안전 증명으로 취급하지 않았다.
- TIM2 설정과 느린 timer 검사가 추가됐으며 source regression이 존재한다. 물리 timer/WCET 검증은 아니다.
- Bridge component 구성에 STM32 TX/provider 권한을 추가한 변경은 없다.
- key/VIN 고신뢰 패턴 일치 없음. 전체 secret 부재나 private capture 감사를 완료한 것은 아니다.

**미검토·NOT_RUN 및 최종 조건**

다음은 이번 실행에서 수행하지 않았다.

- 실제 candidate CMake configure/build와 모든 C unit의 실제 compile database
- target ELF/MAP/BIN/HEX, 실제 `.su`, 전체 warning-free 재현
- 전체 host Debug/Release CTest, coverage, sanitizer
- 원본 filesystem 기반 CLI 실행 전체
- GitHub candidate CI run 및 artifact byte/SHA-256 대조
- 전체 문서 링크 검사: 다른 reviewer report를 읽지 않도록 focused fixture만 실행
- 전체 runtime/ISR 실패 경로, stack call-chain·interrupt nesting·WCET
- Flash root/bootloader 배치, erase/provisioning
- 전체 Git history·secret/private capture 감사

실제 보드 flash, UART/FDCAN, clock/reset/brownout/rail, 외부 TX gate, RF, 차량 bus와 **physical/HIL은 NOT_RUN**이다. **차량 CAN TX는 NO-GO**다.

B-001은 닫혔다. 이전 B-002/B-003의 직접 반례도 수정됐다. 그러나 **B-004와 B-005가 OPEN이고 실제 compile database 검증이 남아 있어 최종 판정은 CONDITIONAL**이다. 두 P2의 수정·재검증 또는 허용된 명시적 disposition 없이 무조건 승인으로 해석하면 안 된다.
