실행 ID: `CV-HOSTILE-20260908-T102-POSTFIX-B-005`  
최종 판정: **PASS — 검토한 source 범위의 unresolved P0/P1/P2/P3 없음**

이전 B-005의 전처리 순서 결함이 수정됐으며, 기존 반례 차단과 순서 역전 mutation 검출을 확인했다. 이 판정은 target build·CI·물리 검증 완료를 뜻하지 않는다.

- 실제 시작: `2026-09-08T19:31:45.2103750+09:00`
- 실제 종료: `2026-09-08T19:34:07.5001226+09:00`
- Candidate: `18941170ef475777c62db2f1471b74f937c807ea`
- Base: `1ebd5f575a9756fb08710021e3c9b882a8ce980f`
- 직전 직접 검토 candidate: `346257b7d9da27d5e9ac5092e5102483b1cb6e7f`
- 격리: immutable Git object 기반 read-only. 공유 checkout·index·branch 변경, 기존 build artifact 사용, 다른 reviewer report 열람 없음.

검토 범위는 이전 B finding, compile contract, source/API/register/symbol gate, SDK 설정, board/profile/generator, CMake/component 의존, 보안·자원 경계, warning/reproducibility 설정, CI evidence와 generated drift다.

**Finding 및 closure**

| 항목 | 결과 |
|---|---|
| P0 | 0건 |
| P1 | 0건 |
| P2 | 0건 |
| P3 | 0건 |
| B-001: clean SDK null `.Trim()` | FIXED 유지. 실제 clean SDK 출력 정상 처리 재확인 |
| B-002: shared C unit forced contract 누락 | FIXED 유지. 이전 CMake 수정과 scanner 범위 변경의 회귀 없음 |
| B-003: 줄바꿈·alias·continued directive | FIXED 유지. regression 통과 |
| B-004: canonical path·옵션 pairing | FIXED 유지. 정상 entry 허용, stale path·unpaired header 거부 |
| B-005: C preprocessing 순서 | **이번 실행에서 FIXED 확인** |

새 finding이 없어 별도의 failure scenario/impact/recommendation 항목은 없다.

**B-005 수정 근거**

수정 위치: [check_stm32_core.py:61](/F:/dev/canview/tools/check_stm32_core.py:61)

```python
logical_text = _strip_c_comments(re.sub(r"\\\r?\n", "", text))
```

backslash-newline 연결을 먼저 수행하고 주석을 제거한다. 직전 candidate의 반대 순서가 수정됐다.

직전의 실제 반례:

```c
void send(void)
{
    FDCAN1-> /\
* command */ TXBAR = 1U;
}
```

검증 결과:

- 위 원래 반례: LF·CRLF 모두 **REJECT**
- spliced comment와 mode directive 조합: LF·CRLF 모두 **REJECT**
- comment 시작·끝이 모두 분리된 TX member·pointer alias·`#undef`·`#define`: candidate regression에서 **REJECT**
- 이어진 `//` 주석 안의 TX 문구: LF·CRLF 모두 정상 **ACCEPT**

추가로 candidate 코드를 메모리에서만 변이하여 주석 제거→줄 연결 순서로 되돌렸다. 새 source-boundary regression은 **assertion failure 4건, 실행 오류 0건**으로 변이를 검출했다. 정상 candidate의 core gate regression은 **6/6 통과**했다.

따라서 단순 정상 예제 통과뿐 아니라 이전 결함의 재발을 시험이 잡는 것도 확인했다. 저장소 파일은 수정하지 않았다.

**실행 명령과 결과**

| 실행 | 결과 |
|---|---|
| 시작·종료 `Get-Date -Format o` | 위 실제 시각 |
| candidate/base `git cat-file -e '<hash>^{commit}'` | 성공 |
| candidate/base `git rev-parse '<hash>^{commit}'` | 지정 hash 일치, 종료 시 재확인 |
| 직전 candidate→현재 `git diff --find-renames --stat`, review 제외 | 2 files, +9/-2 |
| 같은 비교의 `git diff --find-renames --name-only` | gate와 regression 두 파일만 변경 |
| 해당 두 파일 `git diff --find-renames` | 전체 수정 내용 확인 |
| base→candidate 비-review `git diff --find-renames` | 전체 변경 범위 자동 검사 |
| `git show <candidate>:<path>` | source·test·문서·generator 입력 읽기 |
| `git ls-tree -r --name-only <candidate>` | 객체 filesystem 목록 구성 |
| STM32 core gate unittest | **6/6 통과** |
| 전처리 순서 역전 mutation | assertion failure 4건으로 검출, 오류 0건 |
| 원래 반례·mode 조합의 LF/CRLF 검사 | 모두 거부 |
| continued line-comment 정상 fixture | LF/CRLF 모두 허용 |
| canonical compile-contract 직접 fixture | 정상 허용, stale path·unpaired header 거부 |
| generator/sdkconfig unittest | 파일 생성 CLI 시험 제외 **21/21 통과** |
| `check_generated.main([])` | **exit 0**, 생성 출력 15개 검사 통과 |
| `check_generated.main(['--negative-fixture'])` | **exit 0**, drift mutation 거부 |
| protocol generator `write=False` | header + golden 15 + malformed 6 + compatibility 4 + pairing 검증 |
| STM32/shared app/shared protocol source gate | 세 경로 baseline 통과 |
| 실제 CubeG4 clean-status 정규화 | 0행, exit 0, 문자열 길이 0 |
| CubeG4 `rev-parse HEAD 'HEAD^{tree}'` | 아래 고정 값 확인 |
| 비-review 추가 1,748행 고신뢰 secret/VIN 패턴 검사 | 일치 0 |

Python은 `python -X utf8 -B -`로 실행했다. module은 candidate 객체에서 메모리로 로드했고, 시험 파일 입출력을 메모리 fixture 또는 같은 commit 객체 읽기로 연결했다. **원본 filesystem CLI 실행과 구분한다.** 이번 실행에서 새 target compile/link는 하지 않았다.

SDK 명령:

```powershell
git --no-optional-locks -C C:/cv/STM32CubeG4-1.6.3 status --porcelain=v1
git -C C:/cv/STM32CubeG4-1.6.3 rev-parse HEAD 'HEAD^{tree}'
```

결과:

- Commit: `d11b194a9f05d1b143d154771f3dbc282c8052a5`
- Tree: `f91beb5bd319d2fae53a9bc38d4d1149237fa616`

이는 로컬 SDK 관찰이며 candidate CI 성공 증거가 아니다.

**실제로 읽은 파일·객체 범위**

직접 수정 검토와 반례 실행:

```text
F:/dev/canview/tools/check_stm32_core.py
F:/dev/canview/tests/test_stm32_core_gate.py
F:/dev/canview/docs/tasks/T-102-stm32-platform.md
F:/dev/canview/docs/resume.md
```

generator/sdkconfig 실행에서 읽은 도구·시험:

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

자동 객체 검사에서 총 **122개 파일**을 읽었다. 도구·시험 외 범위:

- `F:/dev/canview/firmware/communicator/stm32/`의 `.c/.h` 전체: app, BSP, interface, module, platform, prototype, tests.
- `F:/dev/canview/shared/app/src/canview_app.c`
- `F:/dev/canview/shared/protocol/src/canview_wire.c`
- `F:/dev/canview/shared/protocol/src/canview_uart.c`
- `F:/dev/canview/shared/protocol/src/canview_espnow.c`
- `F:/dev/canview/shared/protocol/src/canview_espnow_contract.c`
- 네 보드의 generated header와 세 ESP32의 sdkconfig/partition 산출물.
- board manifest·Waveshare pin 입력·Communicator/Bridge pinmap.
- ESP-NOW/UART/transport/navigation schema, 생성 protocol/contract header와 source.
- ESP-NOW golden 15종, malformed 6종, compatibility 4종의 `.bin/.json` 및 pairing-negative fixture.

CMake/component·CI·warning 설정·문서 링크 관련 source는 직전 직접 검토 기준선과 **변경 없음**을 immutable diff로 확인했다. 자동 scanner의 파일 읽기를 전체 runtime 정독으로 간주하지 않았다.

**추가 finding 없이 확인한 공격 범위**

- HAL/LL TX, 직접 register, 줄바꿈, pointer alias, 일반 주석, spliced 주석, continued mode directive: regression에서 거부.
- 정상 canonical 경로와 경로 정규화는 허용하고, stale header 및 옵션 pairing 반례는 거부.
- memory 초과·malformed, stack evidence 누락·빈 파일·중복, 금지 heap/TX symbol: regression 통과.
- board/profile uniqueness, pin·memory·path 변조, sdkconfig 필수값 누락·금지값 활성화·중복·잘못된 문법·금지항목 제거 mutation: 실행 범위에서 통과.
- board 및 protocol generated drift와 drift negative fixture: 통과.
- Bridge routing/forwarding·blocking HTTP queue 경계: sdkconfig 시험에서 유지.
- 이번 delta는 Python gate와 시험에 한정된다. target resource 사용·component 의존·Bridge 권한 변경 없음.
- 고신뢰 secret/VIN 패턴 일치 없음. 포괄적인 secret 부재 증명은 아니다.

**미검토·NOT_RUN**

이번 실행에서는 다음을 수행하지 않았다.

- 실제 candidate configure/build와 전체 실제 compile database
- ELF/MAP/BIN/HEX·`.su`·target warning-free 재현
- 전체 host CTest·coverage·sanitizer
- 원본 filesystem 기반 CLI 전체
- 현재 GitHub CI run·artifact byte/SHA-256 대조
- Arm archive 전체 digest 및 nested SDK 상태 전체
- 전체 runtime/ISR·stack call-chain·WCET
- Flash root/bootloader 배치·provisioning·erase
- 전체 문서 링크·Git history·private capture/secret 감사

실제 보드 flash, UART/FDCAN, clock/reset/brownout/rail, 외부 TX gate, RF, 차량 bus와 **physical/HIL은 NOT_RUN**이다. 기존 target/CI 성공 기록을 이번 실행의 증거로 대체하지 않았다.

**최종 source review 판정은 PASS다. 이전 B finding은 모두 닫혔고 검토 범위의 unresolved P0/P1/P2/P3는 없다. 차량 CAN TX는 계속 NO-GO다.**
