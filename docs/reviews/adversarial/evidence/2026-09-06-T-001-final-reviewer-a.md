# T-001 최종 post-fix 재리뷰 A 원문 evidence

- 실행 ID: `01a075b0-f14c-79d1-8bc5-1546bae2767a` (Herschel)
- 전문 영역: embedded safety·target runtime·toolchain semantics
- 최종 기준선: `68cac2b69991a7631e5676f62779e501e63f83e1` → `9cb76e2a5c31da38e74d6754978f869f99d635f0`
- 격리 방식: immutable commit/diff 정적 검토; untracked evidence와 다른 reviewer 결과는 읽지 않음
- 파일 변경: 없음
- 최종 verdict: `PASS` (정적/로컬 리뷰 기준)

## 최종 판정

- P0: 0
- P1: 0
- P2: 0
- P3: 0

## 확인 항목

- `git config --global core.longpaths true`가 target SDK checkout보다 먼저 실행되고 exit code를 확인한다.
- `CANVIEW_TARGET_TOOLROOT`와 `CANVIEW_TARGET_ARM_GNU_ROOT`가 분리되어 ESP-IDF·STM32CubeG4가 Arm GNU root에 섞이지 않는다.
- archive layout, duplicate ZIP entry, archive 밖 entry, 전체 provenance hash binding이 fail-closed다.
- 초기 CI 실패 로그와 provenance를 `always()` log artifact로 보존한다.
- ESP32-S3는 `fullclean → set-target → build`를 수행하고 `project_name`, `project_path`, `target`, `app_bin`을 확인한다.
- CAN TX, OTA bypass, raw replay, Diagnostic Bridge TX 권한 변경은 없다.

## 실행한 검토 명령

- `git diff --check 68cac2b 9cb76e2` — PASS
- 변경 PowerShell 3개 AST parse — PASS
- `.github/workflows/foundation.yml` YAML parse — PASS
- longpaths 순서, archive binding, project identity 정적 assertion — PASS

## 미실행 gate

- 실제 GitHub Actions Windows SDK checkout/build와 artifact upload
- CAN TX, OTA, raw replay, HIL·실차 시험
