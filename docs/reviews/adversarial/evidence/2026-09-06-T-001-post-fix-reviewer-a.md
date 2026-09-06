# T-001 post-fix 재리뷰 A 원문 evidence

- 실행 ID: `01a0757e-c13e-7300-8d9b-e733eea477ec` (Ptolemy)
- 전문 영역: embedded safety·target runtime·toolchain 재현성
- 기준선: `208bec5b4e621cbc06c8bbd012f891b0b7c8a7cc`
- 검토 commit: `f447d86e9084ab74c141327259e6b672dfdb2251`
- 격리 방식: commit/object 기준 읽기 전용 검토
- 저장소 상태: `git status --porcelain=v1` 빈 출력
- 시작·종료 시각: 2026-09-06, orchestrator dispatch와 완료 notification 사이. subagent API가 독립 wall-clock을 제공하지 않음.
- 전달 요청: 변경 파일 12개와 이전 P1/P2 finding의 해결 여부, CAN TX/OTA/raw replay 경계, target/CMake/PowerShell 회귀를 적대적으로 검토하고 파일을 변경하지 말 것.
- 최종 verdict: `CONDITIONAL`

## 원문 판정

- 차량 CAN TX·OTA·raw replay 안전 경계: `GO`
- host/정적 gate: `GO`
- target/reproducibility gate: `CONDITIONAL`
- 차량 release·실차 승인: `NO-GO`
- P0: 0건
- P1: 0건
- P2: 3건
- P3: 1건

## 이전 finding 확인

| Finding | 판정 |
|---|---|
| Windows native exit/warning fail-closed | 대부분 FIXED. wrapper가 exit code와 warning/error를 검사함 |
| Windows target artifact | 부분 FIXED. 필수 `.bin` 존재는 검사하지만 내용/크기 검증은 없음 |
| Arm root | 부분 FIXED. workflow가 명시적 root를 전달하지만 setup script는 fallback 가능 |
| budget parser 중복/malformed | 부분 FIXED. 중복 JSON/map 오류는 거부하지만 stack parser가 malformed prefix를 허용 |
| public `canview_can` strict C99 | FIXED. C99·strict warning 옵션과 public fixture compile 확인 |
| Communicator public protocol 직접 의존 | 빌드 구성은 FIXED. `canview_protocol`이 실제 dependency metadata에 존재 |
| firmware README link 범위 | FIXED. 151문서·1011 local target, 오류 0 |

## 원문 findings

### A-P2-01 — stack evidence parser가 malformed record를 수용

위치: `tools/check_budgets.py:14,54`

`parse_stack()`이 `STACK_RE.search(line)`을 사용하여 전체 행을 검증하지 않는다.

재현:

```text
not-a-record\t128\tstatic        -> {'stack_max_bytes': 128}
garbage\t999999\tstatic          -> {'stack_max_bytes': 999999}
```

잘못된 `.su` 행이 정상 evidence로 처리될 수 있고, 기존 negative fixture는 malformed map만 검사한다. 권고는 stack 행 전체를 `fullmatch()`로 검사하고 malformed prefix·추가 필드 fixture를 추가하는 것이다.

### A-P2-02 — 명시적 Arm root가 실패 시 ambient compiler로 fallback 가능

위치: `tools/environment/setup-windows.ps1:165-177,181,190-191`, `.github/workflows/foundation.yml:115-116`

workflow는 명시적 root를 전달하지만 setup script가 root에 compiler가 없으면 다른 후보 또는 PATH로 fallback하고 GCC `15.3.x` prefix만 확인한다. 잘못된 explicit root와 ambient GCC가 동시에 있으면 pinned archive가 아닌 compiler로 통과할 수 있다. 권고는 explicit root만 허용하고 resolved path가 root 내부인지 확인하는 것이다.

### A-P2-03 — target artifact 검증이 존재 여부에만 의존

위치: `.github/workflows/foundation.yml:134-136,157-177`

`Assert-TargetArtifact`가 `Test-Path -PathType Leaf`만 확인하여 0-byte 또는 손상된 `.bin`도 통과할 수 있고, upload에 `if-no-files-found: error`가 없다. 당시 확인한 non-zero 크기는 STM32 debug 1348, release 1244, Communicator 160048, Controller 159712, Bridge 155648, fixture 144816 byte였다.

권고는 크기·format 또는 digest/manifest 검사와 strict artifact upload다.

### A-P3-01 — Communicator protocol dependency 문서가 현재 CMake와 불일치

위치: `firmware/communicator/esp32/main/CMakeLists.txt:5`, `firmware/communicator/esp32/README.md:27`, `docs/development/foundation.md:78`

현재 CMake는 `REQUIRES canview_protocol`을 직접 선언하지만 README는 v1.2 header가 application dependency로 연결되지 않았다고 설명한다. runtime TX 권한은 만들지 않으나 후속 agent가 protocol 연결 상태를 오해할 수 있다.

## 안전 경계와 검증

새 diff에서 임의 CAN frame TX API, raw CAN replay, OTA bypass, ESP32/Controller의 raw CAN ID/data 전달, hardware TX gate 우회, Diagnostic Bridge control lease/TX 권한 추가는 확인되지 않았다. 실제 GPIO 파형·brownout·reset·PHY gate·HIL·차량 bus 시험은 실행하지 않았다.

실행 명령과 결과:

| 명령 | 결과 |
|---|---|
| `git diff --check 208bec5 f447d86` | PASS |
| `python -B tools/check_budgets.py` | PASS |
| `python -B tools/check_negative_fixtures.py` | PASS |
| `python -B tools/validate_document_links.py` | 151 documents, 1011 targets, errors=0 |
| `python -B tools/check_generated.py` | 14 generated outputs PASS |
| `python -B tools/validate_plan.py` | task 46, errors 0 |
| `python -B -m unittest discover -s tests -p "test_*.py"` | 35 tests PASS |
| workflow YAML PyYAML parse | PASS |
| PowerShell native exit smoke test | 실패 exit code 검출 PASS |
| target compile 재실행 | 리뷰어는 실행하지 않음; 기존 artifact만 읽음 |

## 잔여 위험

실제 target clean build/warning scan의 독립 재실행, toolchain fallback, malformed `.su`, hardware TX gate, reset/brownout, OTA fault recovery, HIL, 차량 CAN 시험은 미검증으로 남겼다. 따라서 `f447d86`은 안전 경계를 새로 깨뜨리지는 않지만 재현성/evidence gate 최종 `PASS`가 아니다.
