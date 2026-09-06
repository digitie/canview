# 2026-09-05 최신 toolchain bootstrap post-fix 적대적 리뷰

- Review ID: `CV-20260905-ENV-01-POST`
- 종류: post-fix target build bootstrap·환경 재현성·문서 정합성
- 최종 기준 commit: `cb7aae92e741f4bdeb5dc9a8a81011fc3b17c391`
- parent: `e7ad3c9e3a13ddcb2fe5b53364f12f582e9eaf2f`
- 최초 candidate: `bfdd2c2d9242840261c85abfa73f051acfc54c97`
- 범위: `tools/`, Controller·Communicator ESP-IDF scaffold, STM32 CMake/toolchain, partition·sdkconfig, 관련 development/task 문서
- 범위 밖: 실제 ESP-IDF/Arm/CMake target build, 실제 board bring-up, 차량 CAN 송신
- 상태: **PASS** (P0–P3 finding 0개)

## reviewer 실행

| reviewer | 실행 ID | 전문 영역 | 기준선 격리 | 결과 원본 |
|---|---|---|---|---|
| A | `01a06f3e-a770-7d61-a07a-32bbb40e928a` | embedded safety·target build·PowerShell | `cb7aae9`와 parent hash 확인, read-only | [evidence A](evidence/2026-09-05-latest-toolchain-bootstrap-post-fix-reviewer-a.md) |
| B | `01a06f3e-a9c9-75a1-a3d3-6969e1b0dbd8` | protocol·security·reproducibility·integration·문서 | `cb7aae9`와 parent hash 확인, read-only | [evidence B](evidence/2026-09-05-latest-toolchain-bootstrap-post-fix-reviewer-b.md) |

두 reviewer는 서로의 결과와 최초 report를 보지 않고 최종 commit을 독립 확인했다. 실행일은 2026-09-05이며 세부 wall-clock은 orchestrator가 제공하지 않았다.

## 최초 finding disposition

최초 report의 `ENV-001` annotated tag object/peeled commit 불일치, `ENV-002` verify-only 약한 SDK 확인, `ENV-003` ESP-IDF export 뒤 host tool drift, `ENV-004` incomplete v1.2 protocol application 연결, `ENV-005` Windows 문서 명령 불일치는 `e7ad3c9`에서 수정했다. `cb7aae9`는 recursive submodule 내부의 dirty·untracked worktree까지 검사하도록 보강했다.

post-fix reviewer는 tag/HEAD/remote·clean·submodule 검증, export/PATH와 CMake executable pin, ESP-IDF component 경계, partition/config 및 문서·secret 검사를 재확인했고 P0–P3 finding을 보고하지 않았다.

## 재검증

- PowerShell AST parse: 통과
- JSON parse (`toolchain-versions.json`, STM32 `CMakePresets.json`): 통과
- `git diff --check`: 통과
- host C11 automation test: `automation tests passed`
- Controller 16 MiB 및 Communicator ESP32 4 MiB partition 경계·겹침 검사: 통과
- ESP-IDF v6.0.3 및 STM32CubeG4 v1.6.3 tag의 peeled commit: manifest와 일치
- Windows CMake/Ninja/Arm GCC/`idf.py` 및 SDK checkout: 현재 review 환경에 없어 target configure/build 미실행

도구 부재는 코드 finding이 아니라 미실행 target gate로 남긴다. 따라서 T-200, T-300, T-102의 실제 target acceptance를 완료로 표시하지 않으며, 차량 CAN 송신 NO-GO도 변경하지 않는다.
