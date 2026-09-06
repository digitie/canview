# Reviewer A 요청·원본 evidence

- subagent ID: `01a07435-a059-7ae1-bbc4-6c89bb421599`
- 요청 전달 UTC: `2026-09-06 00:54:26 UTC`
- 원본 결과는 아래에 LF 정규화 외 내용 변경 없이 보존했다. 상대 원본을 전달하지 않았다.

## 전달한 요청 원문

```text
# 공통 독립 적대적 리뷰 요청 (2026-09-06-plan-ui)
사용자 요청: 전체 계획을 두 번 읽어 누락/불합리 보완, 모든 UI 디테일 개선, PR merge까지. 작성자가 아닌 두 새 전문 리뷰어의 독립 검토다.
Candidate: d078437722635a2ef4c067ca3c08bc6d801d270c
Parent/base: 4aeb2912da063c6fcb0d8715aa46f84c7d1d1b0f
Scope: 이 두 commit 사이 114개 파일 전체 diff; architecture/46 task/42요구 추적표, Windows/N16R8/OTA 기존 정본과의 일관성, driver/diagnostic web 각5뷰, LVGL8.4 UI, portable brightness/volume/SPORT helper, 새 host 검증. base까지의 기존 OTA 회로는 해당 변경의 전제 확인 대상이고 이번 delta에서 제작 승인을 주장하지 않는다.
범위 밖: 실제 OTA/transport/backend 구현, 새 차량 명령, 실물 PCB/HIL/실차/TX 승인, final Korean font/LCD FPS. 기존 사용자 변경 보존. 미래 task를 이번에 구현 완료하지 않는다.
Acceptance: 요청→정본→task→negative acceptance가 연결되고 불가능한 의존/모순이 없을 것. ACK!=적용, stale/후보 표시 금지, 단일 pending/복구/lifetime/시간 gap 안전. 큰4WD/중앙순간연비/작은 원형속도RPM/옆배터리LOCK온도/FFT+속도RPM/선택형시간설정/터치투과경고/정차잠금. 브라우저 DEMO는 real backend가 아닌 명시된 fixture이며 false APPLIED 금지. 계획완료/host통과 != 차량release.
읽기: worktree의 AGENTS.md, docs/README.md, docs/resume.md 먼저. docs/runbooks/agent-workflow.md §5 및 docs/reviews/README.md; 관련 embedded skills 직접 완독. Hallmark 사용 금지. docs/architecture/requirements-coverage.md와 implementation-readiness, protocols/ota 및 선택 task가 정본. 역사 review는 새 finding 복사하지 말 것.
격리: 지정 detached worktree에서 git rev-parse HEAD와 git status --porcelain=v1을 시작/끝에 실행. 둘 다 hash 일치/clean 필수. source를 수정하지 말 것. 검증 임시파일 필요하면 worktree .tools(ignored)에 apply_patch로만 생성. __pycache__ 방지 Python -B.
실행 가능한 검증:
- C:/Python314/python.exe -B -X utf8 tools/validate_plan.py
- C:/Python314/python.exe -B -X utf8 -m unittest discover -s tests -p test_plan_validation.py -v (35)
- C:/Python314/python.exe -B -X utf8 tools/validate_document_links.py
- Node Playwright: $env:NODE_PATH='C:/Users/digit/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules'; node tools/ui/check-browser.cjs (72driver +10diagnostic groups; --screenshots 쓰지 말 것: tracked PNG가 바뀜)
- ./tools/ui/validate-lvgl.ps1 -LvglSource F:/dev/canview/.tools/lvgl-8.4.0 (clean official8.4 commit4495f428630cc1741bd8bfd977f080e8460e8e8d; 실제 C 링크)
- automation: & 'C:/Program Files (x86)/Microsoft Visual Studio/18/BuildTools/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 -SkipAutomaticLocation; cmake -S tests/automation -B .tools/automation-review -G Ninja -DCMAKE_BUILD_TYPE=Debug; cmake --build .tools/automation-review --parallel 6; ctest --test-dir .tools/automation-review --output-on-failure (11)
다른 reviewer 결과는 제공하지 않는다. 각자 독립 원본을 확정 제출하고 main이 두 원문을 저장한 뒤 교차 검토한다. 결과는 한글 markdown 최종 메시지에: 실행ID, 시작/종료 UTC, 실제 hash/clean, 읽은 범위/검증 명령 결과/미실행, 공격한 시나리오, P0..P3 ID+정확한 파일라인+재현/영향/수정안, verdict BLOCK/CONDITIONAL/PASS. finding 없으면 공격 범위와 불확실성 기록. 보고서 파일은 main이 원문 그대로 저장하므로 직접 tracked 파일 편집/commit/push 금지.
Reviewer A: 임베디드 runtime·운전자 UI 안전 전문가. portable automation/time/pending/ownership/LVGL lifetime·상태 오해·actual LVGL 검증을 우선 공격하되 전체 delta의 교차 영향도 검토.
지정 worktree: F:/dev/canview-wt/review-plan-ui-a
상대 결과를 기다리지 말고 리뷰에 즉시 착수.
```

## 독립 결과 원문

# Reviewer A 독립 적대적 리뷰

## 판정: PASS

이 candidate의 계획 보완·UI·portable helper 변경에서 확정할 신규 P0–P3 finding은 발견하지 못했다. **이번 delta의 merge 검토 판정이며, 차량 release·OTA 완성·제작·TX 승인이 아니다.** 다른 reviewer 결과는 열람하지 않았다.

## 실행 식별과 격리

| 항목 | 실제 기록 |
|---|---|
| 실행 ID | `2026-09-06-plan-ui-a-d078437` |
| 전문 영역 | 임베디드 runtime·운전자 UI 안전 |
| 시작 UTC 기록 | `2026-09-06 00:54:53 UTC` |
| 종료 UTC | `2026-09-06 01:07:02 UTC` |
| worktree | `F:/dev/canview-wt/review-plan-ui-a` |
| base | `4aeb2912da063c6fcb0d8715aa46f84c7d1d1b0f` |
| 시작·종료 실제 HEAD | `d078437722635a2ef4c067ca3c08bc6d801d270c` |
| 시작·종료 상태 | `git status --porcelain=v1` 출력 없음, clean |

Tracked source·문서·PNG는 수정하지 않았다. Commit·push·merge도 수행하지 않았다. 추가 시험 파일은 `apply_patch`로 ignored `.tools`에만 작성했다.

## 읽은 범위

- 두 commit 사이 **114개 파일 전체 delta**: 텍스트 102개와 PNG 12개. 변경된 화면 이미지 및 이번 실행에서 생성한 실제 LVGL 5뷰 출력도 직접 확인했다.
- AGENTS, 문서 지도, resume, agent workflow §5, review archive 절차.
- 요구 추적표 42행, 상세 task 46개 변경, architecture·protocol·implementation-readiness와 관련 기존 문맥. OTA §1–13은 기존 설계 전제와 후속 task의 일관성 확인에 사용했다.
- Windows/N16R8/독립 reset·OTA, Controller decode와 STM local safety 분리, 설정 권위·영속화·복구 경계를 대조했다.
- Driver/diagnostic 웹 각 5뷰, LVGL adapter·헤더·시험, automation/SPORT helper 및 새 검증 도구를 검토했다.
- `embedded-architecture`, `embedded-cstyle`, `embedded-documentation`, `embedded-rtos-design` 원문을 직접 완독했다. Ownership·수명·시간·미실행 gate 검토에 적용했으며 Hallmark는 사용하지 않았다.
- 과거 reviewer finding을 신규 finding의 근거로 사용하지 않았다.

## 실행 검증

명령은 지정 worktree에서 실행했다.

| 명령·검증 | 실제 결과 |
|---|---|
| `C:/Python314/python.exe -B -X utf8 tools/validate_plan.py` | 상세 task 46개, 오류 0 |
| `C:/Python314/python.exe -B -X utf8 -m unittest discover -s tests -p test_plan_validation.py -v` | 35개 PASS |
| `C:/Python314/python.exe -B -X utf8 tools/validate_document_links.py` | 문서 133개·로컬 target 939개, 오류 0 |
| 지정 `NODE_PATH` 설정 후 `node tools/ui/check-browser.cjs` | Driver 72개·diagnostic 10개 그룹 PASS, 페이지 오류·외부 요청 0 |
| `./tools/ui/validate-lvgl.ps1 -LvglSource F:/dev/canview/.tools/lvgl-8.4.0` | 실제 LVGL 컴파일·링크, CTest 1/1 PASS |
| 아래 automation 명령 | CTest 11/11 PASS |
| Base→candidate `git diff --check` | PASS |

```powershell
$env:NODE_PATH='C:/Users/digit/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules'

& 'C:/Program Files (x86)/Microsoft Visual Studio/18/BuildTools/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
cmake -S tests/automation -B .tools/automation-review -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build .tools/automation-review --parallel 6
ctest --test-dir .tools/automation-review --output-on-failure
```

`--screenshots`는 사용하지 않았다. 공식 LVGL checkout은 시작 검증과 종료 확인에서 `4495f428630cc1741bd8bfd977f080e8460e8e8d`, clean이었다.

### Reviewer 추가 시험

Navigation activity·dropdown popup activity·명령 callback 내부에서 **삭제→즉시 재생성**하는 36개 경우를 추가했다. 실제 LVGL library와 링크하여 이전 event 무효화, 새 instance pending 초기화, animation 정리, heap 무결성·증가 없음을 검사했다.

```powershell
cmake -S .tools/review-a-extra -B .tools/review-a-extra/build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build .tools/review-a-extra/build --parallel 6
ctest --test-dir .tools/review-a-extra/build --output-on-failure
```

최종 **1/1 PASS, 36개 경우 통과**. 최초 임시 harness는 공식 헤더를 SYSTEM include로 지정하지 않아 MSVC C4200 경고가 오류 처리되었고 시험은 NOT RUN이었다. Harness만 기존 시험과 동일한 SYSTEM include 방식으로 수정한 후 재실행했다. Candidate 결함으로 분류하지 않았다.

## 공격한 시나리오와 결과

- **ACK·pending:** 중복 ACK, terminal 이후 ACK, timeout·wrap, 중복 클릭, 음량 pending 중 manual override를 확인했다. ACK만으로 완료·적용을 만들지 않는 경계가 유지된다.
- **시간 단절:** 251ms·5초·`UINT32_MAX` gap, 제한된 tick evidence 누적, 만료와 dwell 처리를 확인했다. 긴 공백을 정상 관측 시간으로 채워 자동화를 실행하는 경로는 확인되지 않았다.
- **SPORT ownership:** stale feedback, timeout 경계, 물리 override, 자동 소유 후 외부 mode 변경, disable·복구를 공격했다. 현재 helper의 제한과 physical TX-complete·인증 adapter 후속 gate가 구분되어 있다.
- **밝기:** stale 반복 호출의 중복 감광, 경고 boost의 base 오염, 경고 종료 후 복원, stale speed와 idle 전환을 확인했다.
- **LVGL 수명:** 중복 create, 부모 외부 삭제, 열린 dropdown 삭제, callback 재진입, 반복 화면 전환·재생성을 확인했다. 지정 시험과 추가 시험에서 수명 오류나 heap 증가가 발생하지 않았다.
- **운전자 상태 오해:** 개별 candidate/stale/invalid 값, 수치 범위·signed dBFS, DPF lamp와 전체 상태 구분, 정차 잠금, RTC 윤년·분 선택·초안, 경고 우선순위·터치 투과를 대조했다.
- **Diagnostic DEMO:** 미연결·malformed·stale·gap, pending 중 취소/연결 상태 변경, 후보 중복·export 실패·재시도를 확인했다. 합성 샘플과 로컬 초안이 실제 backend의 APPLIED 또는 차량 evidence로 승격되지 않는다.
- **계획 의존:** source 조사와 bench 실행 분리, synthetic framework와 실차 evidence gate 분리, OTA 단계별 fixture 계약을 대조했다. 검토한 delta에서 불가능한 의존 순환이나 요구→정본→task→negative acceptance의 중대한 단절은 확인하지 못했다.

## Finding과 남은 불확실성

| 등급 | 신규 확정 finding |
|---|---:|
| P0 | 0 |
| P1 | 0 |
| P2 | 0 |
| P3 | 0 |

확정 finding이 없어 ID·수정 위치는 발급하지 않는다.

이번에 실행하지 않은 검증은 다음과 같다.

- ESP-IDF/Arm target build·map, 실제 transport/backend·인증·영속 저장 통합.
- Sanitizer, 장시간 target soak, 실제 LCD FPS·touch/PWM·최종 한글 font. LVGL 출력의 한글은 placeholder였으며 최종 가독성은 미검증이다.
- PCB·아날로그 reset/brownout·Flash 단전·HIL·실차·차량 TX.
- 실제 휴대폰/AP OTA·Android/iOS 브라우저 복구 절차.

Host 검증은 MSVC 환경에서 수행했으며 고정 Arm/전체 target toolchain 검증을 대신하지 않는다. 따라서 **PASS는 지정 candidate의 제한된 delta에만 적용**하며, 제품 전체와 차량 CAN TX의 NO-GO 경계는 유지한다.
