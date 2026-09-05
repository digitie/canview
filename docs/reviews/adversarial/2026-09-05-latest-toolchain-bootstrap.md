# 2026-09-05 최신 toolchain bootstrap 적대적 리뷰

- Review ID: `CV-20260905-ENV-01`
- 종류: target build bootstrap·환경 재현성·문서 정합성
- 최초 기준 commit: `bfdd2c2d9242840261c85abfa73f051acfc54c97`
- base: `783a36f31c080687377bf5844ea3c3f0983df8bf`
- 범위: `tools/`, Controller·Communicator ESP-IDF scaffold, STM32 CMake/toolchain, partition·sdkconfig, 관련 architecture/development/task 문서
- 범위 밖: 실제 ESP-IDF/Arm/CMake target build, 실제 board bring-up, 차량 CAN 송신
- 상태: 최초 리뷰 finding 기록, 수정 후 post-fix 독립 재검토 대기

## reviewer 실행

| reviewer | 실행 ID | 전문 영역 | 기준선 격리 | 결과 원본 |
|---|---|---|---|---|
| A | `01a06f14-9a6e-7ad1-a2f8-f57eb4648058` | embedded safety·target build·PowerShell | commit object 검증, 파일 변경 없음 | [evidence A](evidence/2026-09-05-latest-toolchain-bootstrap-reviewer-a.md) |
| B | `01a06f14-9cc5-7391-8f10-951301a6d511` | protocol·재현성·문서·보안 | commit object 검증, 파일 변경 없음 | [evidence B](evidence/2026-09-05-latest-toolchain-bootstrap-reviewer-b.md) |

두 reviewer는 동일한 commit과 parent를 독립적으로 확인했다. 실행 도구가 세부 wall-clock을 반환하지 않아 실행일만 기록하며, 각 원본 결과는 수정하지 않는다.

## 최초 finding과 disposition

### ENV-001 — `P1` annotated tag object를 checkout commit으로 비교

- 위치: `tools/toolchain-versions.json`, `tools/environment/setup-windows.ps1`
- 관찰: manifest가 annotated tag object ID를 기록했지만 `git clone --branch` 후 `HEAD`는 peeled commit이었다. ESP-IDF는 `76f5dedd...`, STM32CubeG4는 `d11b194a...`로 resolve된다.
- 영향: 깨끗한 환경도 SDK clone 직후 mismatch로 중단되어 모든 target build가 차단된다.
- disposition: peeled commit ID로 manifest를 교체하고 tag ref·remote·HEAD를 모두 검증한다. clone은 temporary path에서 끝까지 검증한 뒤 destination으로 이동해 partial clone이 다음 실행을 오염시키지 않게 한다.
- 상태: 수정 commit에서 반영, post-fix review 필요.

### ENV-002 — `P2` `-VerifyOnly`가 임의 SDK checkout을 허용

- 위치: `tools/environment/setup-windows.ps1`, `tools/README.md`
- 관찰: 최초 구현은 verify-only에서 SDK path와 핵심 파일만 확인했다.
- 영향: 다른 remote, 다른 tag, dirty 또는 다른 commit의 SDK가 통과할 수 있다.
- disposition: verify-only도 clone을 생략할 뿐 remote URL, tag ref의 peeled commit, `HEAD`, clean status, recursive submodule 상태를 동일하게 검증하도록 변경한다.
- 상태: 수정 commit에서 반영, post-fix review 필요.

### ENV-003 — `P2` ESP-IDF export 뒤 host tool drift

- 위치: setup script, `firmware/communicator/stm32/CMakePresets.json`, STM32 toolchain file
- 관찰: 최초 구현은 export 전 CMake/Ninja/GCC만 확인했고, preset은 PATH 검색에 의존했다.
- 영향: ESP-IDF가 PATH를 앞세우면 다른 Ninja 또는 compiler가 선택되어 log와 실제 build가 달라질 수 있다.
- disposition: export 뒤 CMake/Ninja/GCC를 다시 확인하고, manifest에 확인한 host tool directory를 PATH와 CMake preset/toolchain executable에 전달한다. Arm GCC는 `15.3.x` 범위로 고정한다.
- 상태: 수정 commit에서 반영, post-fix review 필요.

### ENV-004 — `P2` incomplete v1.2 protocol header가 application에 연결됨

- 위치: `firmware/controller/components/canview_protocol/`, `firmware/communicator/esp32/components/canview_protocol/`, 각 `main/`
- 관찰: 현재 `protocol/canview_protocol.h`는 T-002 전의 incomplete v1.2 header다. 최초 scaffold main은 이를 실제 application dependency로 소비했다.
- 영향: v1.3 schema가 없는 상태에서 target build가 protocol 통합 완료처럼 보이고, 이후 wire ABI drift를 숨길 수 있다.
- disposition: public component directory와 `canview_can`의 future `REQUIRES` 경계는 보존하되, T-002 v1.3 전에는 두 bootstrap main이 component를 link/include하지 않도록 변경한다.
- 상태: 수정 commit에서 반영, post-fix review 필요.

### ENV-005 — `P3` Windows 정책과 문서 명령 불일치

- 위치: `docs/development/toolchains.md`
- 관찰: Communicator와 아직 존재하지 않는 Diagnostic Bridge에 Bash, `/dev/ttyACM*`, absent directory 명령이 남아 있었다.
- 영향: Windows 정본 환경에서 그대로 복사 실행할 수 없고, 없는 Bridge firmware가 이미 있는 것처럼 오해할 수 있다.
- disposition: PowerShell/COM 명령으로 바꾸고 Diagnostic Bridge는 T-400 이후의 future project로 명시한다.
- 상태: 수정 commit에서 반영, post-fix review 필요.

## 최초 검증

- `git diff --check`: 통과
- PowerShell AST, JSON parse, Markdown local link, partition overlap/size: 통과
- host C11 automation test: 통과
- target configure/build: 도구와 SDK 부재로 미실행

수정 후에는 manifest commit, verify-only, export 후 tool selection, v1.3 dependency 경계를 다시 독립 검토한다. P1/P2가 남아 있으면 merge/release gate를 닫지 않는다.
