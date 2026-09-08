실행 ID: `CV-HOSTILE-20260908-T102-POSTFIX-B-004`  
최종 판정: **CONDITIONAL — P0 0건 / P1 0건 / P2 1건 / P3 0건**

**B-004는 수정 확인됐다. B-005는 C 전처리 단계 순서 결함으로 OPEN이다. 따라서 PASS를 부여하지 않는다.**

- 실제 시작: `2026-09-08T19:25:33.6175879+09:00`
- 실제 종료: `2026-09-08T19:28:16.0474258+09:00`
- Candidate: `346257b7d9da27d5e9ac5092e5102483b1cb6e7f`
- Base: `1ebd5f575a9756fb08710021e3c9b882a8ce980f`
- 이전 직접 검토 candidate: `6bf55ec7bedd277220ab3713ecb56351f2e7e29d`
- 격리: immutable Git object 기반 read-only. 공유 checkout·index·branch 변경 없음. 기존 build artifact 사용 없음. 다른 reviewer report 열람 없음.

검토 범위는 이전 B finding, compile contract, source/API/register/symbol gate, SDK 설정, board/profile/generator, CMake/component 의존, 보안·자원 경계, warning/reproducibility 설정, CI evidence와 generated drift다.

**이전 finding 재검증**

| Finding | 결과 |
|---|---|
| B-001: clean SDK null `.Trim()` | 수정 유지. 실제 SDK 0행/exit 0 출력의 배열 수집·join·Trim 정상 처리 확인 |
| B-002: shared C unit forced contract 누락 | 이전 CMake 수정과 shared source 검사 범위 유지. 이번 delta에 회귀 없음 |
| B-003: 줄바꿈·alias·continued directive | 해당 regression 통과 |
| B-004: canonical path·옵션 pairing | **FIXED**. 잘못된 경로와 unrelated forced-header 반례 거부, 정상 canonical 경로 허용 |
| B-005: tokens 사이 C 주석 | 일반 주석 반례는 수정됐지만 아래 조합 반례가 남아 **OPEN** |

**B-005 — P2 — 주석 제거와 line splicing 순서가 반대여서 TX 접근 검사를 통과함**

위치: [check_stm32_core.py:58](/F:/dev/canview/tools/check_stm32_core.py:58)  
관련 구현: [check_stm32_core.py:81](/F:/dev/canview/tools/check_stm32_core.py:81)

현재 순서:

```python
logical_text = re.sub(r"\\\r?\n", "", _strip_c_comments(text))
```

주석을 제거한 다음 backslash-newline을 연결한다. 실제 C에서는 줄 연결이 주석 처리보다 먼저 일어난다. 따라서 줄 연결로 만들어지는 주석을 현재 scanner가 제거하지 못한다.

실제 반례:

```c
void send(void)
{
    FDCAN1-> /\
* command */ TXBAR = 1U;
}
```

검증 결과:

- candidate의 `check_source_safety()`: **True**
- candidate build-mode header와 고정 CMSIS를 사용한 실제 Arm GCC C99 syntax 검사: **exit 0**
- compiler stderr: 비어 있음

compiler는 `/`와 다음 줄의 `*`를 연결하여 정상 주석으로 처리하고 `FDCAN1->TXBAR` 접근을 해석한다. scanner는 주석 제거 시점에 이 주석을 인식하지 못한다. 이후 줄 연결로 만들어진 `/* command */`가 정규식의 member 접근 검사를 가로막는다.

실패 시나리오: token 사이 주석과 line continuation이 함께 사용된 정상 C source가 CAPTURE_ONLY source gate를 통과한다.

영향: B-005의 일반 주석 반례는 차단됐지만 같은 lexical 검사 경계의 누락이 남는다. 현재 candidate firmware에 이 반례가 존재하거나 실제 차량 송신이 가능하다는 의미는 아니다.

권고: line splicing을 먼저 적용하고 그 결과에 주석 제거를 수행한다. 위 조합 반례와 continued line-comment의 정상 무시 동작을 regression으로 추가한다.

```python
logical_text = _strip_c_comments(re.sub(r"\\\r?\n", "", text))
```

Disposition: **OPEN — P2 유지**.

**B-004 closure 근거**

새 구현은 command/arguments를 분리하고, `-include`와 다음 인자를 연결하여 canonical path를 비교한다.

직접 실행 결과:

```text
-include F:/stale/canview_build_mode.h
→ REJECT

-include stdint.h -DHEADER_LABEL=canview_build_mode.h
→ REJECT

-include <expected canonical header>
→ ACCEPT, checked=1
```

candidate regression의 `firmware/../interface` 정규화 경로도 통과했다. 따라서 이전 B-004의 canonical path·옵션 pairing 반례는 닫는다. 이 결과를 실제 target compile database 전체 검사로 확대하지 않는다.

**실행 명령과 결과**

| 실행 | 결과 |
|---|---|
| 시작·종료 `Get-Date -Format o` | 위 시각 기록 |
| candidate/base `git cat-file -e '<hash>^{commit}'` | 성공 |
| candidate/base `git rev-parse '<hash>^{commit}'` | 지정 hash 일치, 종료 시 재확인 |
| 이전 candidate→현재 `git diff --find-renames --stat`, review 제외 | 2 files, +111/-13 |
| 같은 비교의 `git diff --find-renames --name-only` | gate와 regression 두 파일만 변경 |
| base→candidate `git diff --find-renames --numstat`, review 제외 | 전체 변경 범위 확인 |
| `git show <candidate>:<path>` | 아래 source·test·문서와 generator 입력 읽음 |
| `git ls-tree -r --name-only <candidate>` | 객체 filesystem 목록 구성 |
| STM32 core gate unittest, 메모리 filesystem 연결 | **6/6 통과** |
| B-004 이전 반례 직접 호출 | 잘못된 두 entry 거부, 정상 entry 허용 |
| B-005 일반 주석 반례 직접 호출 | 거부 |
| B-005 spliced-comment 반례 | source gate 허용, 실제 Arm compiler 통과 |
| generator/sdkconfig unittest, 객체 읽기 연결 | 파일 생성 CLI 시험 제외 **21/21 통과** |
| `check_generated.main([])` | **exit 0**, 생성 출력 15개 검사 통과 |
| `check_generated.main(['--negative-fixture'])` | **exit 0**, drift mutation 거부 |
| protocol generator `write=False` | header + golden 15 + malformed 6 + compatibility 4 + pairing 검증 |
| STM32/shared app/shared protocol source gate | 세 경로 baseline 통과 |
| 실제 CubeG4 clean-status 처리 | 0행, exit 0, 정규화 문자열 길이 0 |
| CubeG4 `rev-parse HEAD 'HEAD^{tree}'` | 고정 commit/tree 일치 |
| 비-review 추가 1,741행 고신뢰 secret/VIN 패턴 검사 | 일치 0 |

Python은 `python -X utf8 -B -`로 실행했다. module은 candidate 객체에서 메모리로 로드했고, 시험 파일 입출력은 메모리 fixture 또는 같은 commit 객체 읽기로 연결했다. 원본 filesystem CLI 실행과 구분한다.

실제 compiler:

```text
C:/Users/digit/AppData/Local/CANView/toolchains/arm-gnu-toolchain-15.3.rel1/bin/arm-none-eabi-gcc.exe
```

사용 옵션:

```text
-x c -std=c99 -Wall -Wextra -Werror -Wundef -fsyntax-only
-mcpu=cortex-m4 -mthumb -DSTM32G474xx
-DCANVIEW_STM_CAPTURE_ONLY_CONTRACT=1
-IC:/cv/STM32CubeG4-1.6.3/Drivers/CMSIS/Device/ST/STM32G4xx/Include
-IC:/cv/STM32CubeG4-1.6.3/Drivers/CMSIS/Core/Include
-
```

입력은 stdin으로 전달했고 object/ELF를 생성하지 않았다.

SDK 확인 명령:

```powershell
git --no-optional-locks -C C:/cv/STM32CubeG4-1.6.3 status --porcelain=v1
git -C C:/cv/STM32CubeG4-1.6.3 rev-parse HEAD 'HEAD^{tree}'
```

결과:

- Commit: `d11b194a9f05d1b143d154771f3dbc282c8052a5`
- Tree: `f91beb5bd319d2fae53a9bc38d4d1149237fa616`

**실제 읽은 파일·객체 범위**

직접 변경 검토와 반례 실행:

```text
F:/dev/canview/tools/check_stm32_core.py
F:/dev/canview/tests/test_stm32_core_gate.py
F:/dev/canview/firmware/communicator/stm32/interface/canview_build_mode.h
F:/dev/canview/docs/tasks/T-102-stm32-platform.md
F:/dev/canview/docs/resume.md
```

generator/sdkconfig 실행에 읽은 도구·시험:

```text
F:/dev/canview/tests/foundation/test_generators.py
F:/dev/canview/tests/foundation/test_sdkconfig.py
F:/dev/canview/tools/check_generated.py
F:/dev/canview/tools/check_sdkconfig.py
F:/dev/canview/tools/generate_boards.py
F:/dev/canview/tools/generate_transport.py
F:/dev/canview/tools/generate_protocol.py
F:/dev/canview/tools/generate_uart_protocol.py
F:/dev/canview/tools/toolchain-versions.json
F:/dev/canview/tools/sdkconfig-allowlist/esp32s3-idf-6.0.3.keys
```

자동 객체 검사는 총 **122개 파일**을 읽었다. 위 도구·시험 외 범위는 다음과 같다.

- `F:/dev/canview/firmware/communicator/stm32/`의 모든 `.c/.h`: app, BSP, interface, module, platform, prototype, tests.
- `F:/dev/canview/shared/app/src/canview_app.c`
- `F:/dev/canview/shared/protocol/src/canview_wire.c`
- `F:/dev/canview/shared/protocol/src/canview_uart.c`
- `F:/dev/canview/shared/protocol/src/canview_espnow.c`
- `F:/dev/canview/shared/protocol/src/canview_espnow_contract.c`
- 네 보드의 generated `board_pins.h`, 세 ESP32의 `sdkconfig.defaults`, `partitions.csv`, `partitions.ota-template.csv`.
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
- `F:/dev/canview/protocol/golden/espnow-v1.3/`의 golden 15종, malformed 6종, compatibility 4종의 `.bin/.json` 및 `pairing-negative.json`.

자동 scanner의 파일 읽기를 모든 runtime의 정독으로 간주하지 않는다. CMake/component·CI·warning·문서 링크 부분은 이전 직접 검토 candidate와 **변경 없음**을 immutable diff로 확인했다. 다른 reviewer report나 공유 checkout의 artifact는 사용하지 않았다.

**추가 finding 없이 확인한 범위**

- 기존 HAL/LL TX·직접 register·줄바꿈·alias·mode override 회귀시험은 통과했다.
- canonical header 정상 경로와 `..` 정규화는 허용하고 기존 wrong-path/옵션 pairing 반례는 거부했다.
- memory 초과·malformed, stack evidence 누락·빈 파일·중복, 금지 heap/TX symbol 회귀시험이 통과했다.
- board/profile·pin·memory·path·sdkconfig 부정/변이 시험과 생성물 drift 검사가 통과했다.
- Bridge routing/forwarding·blocking HTTP queue 설정 경계가 sdkconfig 시험에서 유지됐다.
- 이번 변경은 Python gate와 시험에 한정되며 target resource 사용, component 의존, Bridge 권한을 변경하지 않는다.
- 고신뢰 secret/VIN 패턴 일치 없음. 전체 secret 부재 증명은 아니다.

**미검토·NOT_RUN과 최종 조건**

이번 실행에서 실제 candidate configure/build, 실제 전체 compile database, ELF/MAP/BIN/HEX, `.su`, target warning-free 재현, 전체 host CTest·coverage·sanitizer, GitHub candidate CI run·artifact digest를 검증하지 않았다. 기존 성공 기록으로 대체하지 않았다.

전체 runtime/ISR·stack call-chain·WCET, Flash root/bootloader 배치, provisioning/erase, 전체 secret/private history 감사도 미검증이다.

실제 보드 flash, UART/FDCAN, clock/reset/brownout/rail, 외부 TX gate, RF, 차량 bus 및 **physical/HIL은 NOT_RUN**이다. **차량 CAN TX는 NO-GO**다.

**최종 CONDITIONAL. B-004는 FIXED이나 B-005 P2가 OPEN이므로, 모든 finding이 닫혔다는 판정이나 PASS를 제공할 수 없다.**
