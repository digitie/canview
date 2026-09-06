# T-002 최종 host test·coverage evidence

- 기준 commit: `e1b0e8e` (`fix: close final ESP-NOW schema policy findings`)
- 환경: Windows Clang `23.1.0`, CMake `4.4.3`, Ninja `1.13.2`, Python `3.14.3`

## 결과

| gate | 결과 |
|---|---|
| `python -B tools/generate_protocol.py --check` | PASS: header + 15 golden + 6 malformed + 4 compatibility + pairing |
| `python -B tests/protocol/test_schema.py -v` | PASS: 24/24 |
| host Debug CTest | PASS: 39/39, C99·C11 header gate 포함 |
| host Release CTest | PASS: 39/39, C99·C11 header gate 포함 |
| coverage CTest | PASS: 9/9 core 시험 |
| `shared/app/src/canview_app.c` | line/function/branch 100%/100%/100% |
| `shared/protocol/src/canview_wire.c` | line/function/branch 100%/100%/99.6296296% |

coverage는 기존 profile을 재사용하지 않고 새 profile directory에서 실행했다. Windows sanitizer는 프로젝트 계약상 Linux portability job을 사용하므로 Windows에서 `PASS`로 표시하지 않았고, 원격 Linux sanitizer는 별도 CI 결과로 확인한다.
