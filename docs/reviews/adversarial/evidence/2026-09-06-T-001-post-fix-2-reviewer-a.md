# T-001 post-fix-2 재리뷰 A 원문 evidence

- 실행 ID: `01a0759c-c40d-7480-afe9-ac9591b8b007` (Ohm)
- 전문 영역: embedded safety·target runtime·toolchain semantics
- 기준선: `f447d86`
- 검토 commit: `7e0a8e1756dab624a620fb442e166cd25b43b25a`
- 격리 방식: immutable Git object 읽기 전용; untracked evidence는 읽지 않음
- 파일 변경: 없음
- 최종 verdict: `BLOCK`

## 원문 P1

### A-POST-001 — Arm archive 경로·provenance 불일치

위치: `.github/workflows/foundation.yml:111-124`, `:130-131`, `tools/environment/install-arm-gnu.ps1:33-45`

CI는 archive를 `$root`에 풀고 `$root\bin\...`을 검사하며 provenance도 `$root`에 기록하지만, 다음 step은 `$root\arm-gnu-toolchain-15.3.rel1`을 verified root로 전달한다. pinned archive root listing은 `.version`, `arm-none-eabi/`, `arm-none-eabi/bin/`이며 top-level `bin/` layout도 확인됐다. setup은 존재하지 않는 root/marker를 찾다가 실패하므로 target CI가 build와 artifact manifest까지 도달하지 못한다.

## 원문 P2

### A-POST-002 — provenance marker가 자기 서명 상태

위치: `tools/environment/setup-windows.ps1:57-88`, `tools/environment/install-arm-gnu.ps1:19-30,48-60,90-98`

setup은 marker archive metadata와 gcc/objcopy/size hash만 비교하고 archive 자체, `ld.exe`, `cc1.exe`, runtime library를 재검증하지 않는다. marker와 세 hash를 함께 바꾸면 비-pinned toolchain이 인정될 수 있다.

## 이전 finding 상태

- stack `fullmatch`·malformed fixture: 정적 폐쇄
- Arm provenance/ambient fallback: ambient fallback은 폐쇄됐으나 설치·CI 경로는 P1 미폐쇄
- `.bin/.elf/.map` non-empty/hash manifest/upload: workflow 정적 구현, 실제 target build는 미실행
- T-002 전 v1.2 protocol main dependency 제거: 폐쇄
- budget closed-world/boolean limit: 정적 폐쇄

## 안전 경계·검증

새 CAN TX, raw replay, Diagnostic Bridge TX, OTA bypass는 발견하지 않았다. foundation image는 `COMPONENTS main`과 protocol dependency 제거로 제한된다. 실제 target image, hardware TX gate, OTA signing/provisioning, HIL, 차량 CAN은 미검증이다.

확인한 명령:

- `git show 7e0a8e1 --stat`, parent/diff 확인: PASS
- pinned Arm archive HTTP range read: nested/top-level archive layout 확인
- 실제 CMake/IDF/target build, board flash, HIL: 미실행

P1 target bootstrap 회귀 때문에 `7e0a8e1`은 BLOCK이다.
