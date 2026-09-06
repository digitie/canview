# Reviewer B post-fix 요청·원본 evidence

- subagent ID: `01a07435-a143-7653-baec-15a8b86f32a8`
- 요청 전달 UTC: `2026-09-06 01:17:20 UTC`
- 최초 A/B 원문이 저장된 뒤 전달한 재검토다. LF 정규화 외 결과 원문을 변경하지 않았다.

## 전달한 요청 원문

```text
# Post-fix 독립 재검토 (2026-09-06-plan-ui)
최초 원본 A/B 모두 F:/dev/canview/docs/reviews/adversarial/evidence/2026-09-06-plan-ui-reviewer-{a,b}.md에 확정 저장했으므로 이제 교차 확인한다. 최초A PASS(확정finding0), B BLOCK B-01(P1:tick_gap && 외부mode에서SPORT소유권유지), B-02(P2:evidence enum UNKNOWN/REJECTED정본충돌), B-03(P3:가상clock전환직후빈경고스크린샷).
Post-fix immutable commit: 1f93b8adb34a48537db69b950c8a99ce89859760
검토 delta: d078437722635a2ef4c067ca3c08bc6d801d270c..1f93b8adb34a48537db69b950c8a99ce89859760 (17개 파일), 원base부터의 전체변경에 대한 회귀 영향도 확인.
지정 detached worktree는 main이 이미 post-fix로 전환했다. 시작/끝 exactHEAD+clean을 다시 기록. trackedsource수정금지, 필요probes는 ignored .tools apply_patch.
수정:
- B01 gap에도 fresh 비SPORT 관찰로 권한철회. 정상 ECO→SPORT enter/feedback부터 시작해 4mode×100/250/251/5000/UINT32_MAX 교차20경우, 이후 사용자가 고른SPORT/disable에도 오래된restore없음. tests/automation12/12 PASS. red시assert가WindowsCRT대기로 main이해당PID만종료해실패로그확보; 테스트 stderr/abort설정+30secCTesttimeout추가.
- B02 readiness§9.2 evidence_grade UNKNOWN/CANDIDATE/OBSERVED/VERIFIED정본한곳; 별도candidate review_status PENDING/APPROVED/REJECTED, 거절/unknown/구demo묵시변환/derived승격거부. T005/T403/진단/차량문서동기화. 실제schema필드는후속task BLOCKED.
- B03(main M-P2-01중복관찰):사진촬영 finite animation종료,독립drive+warningfixture,selected/opacity회귀;최종PNG본문존재확인. driver74/diag10 PASS.
- main M-P2-02 추가개선:LVGL 모든탭128px중앙경고+비주행60%투명/touch; 정상제한36pxheader·연결요약겹침방지. 실제LVGL Debug/ReleasePASS(중앙hit-test포함). 기존상위freshness/결과확인경계유지.
기존manifest의범위/안전gate/환경명령그대로. 자신의 finding(B는B01/B02/B03)과 전체 post-fix delta에대해 FIXED/REJECTED_WITH_EVIDENCE/OPEN, 신규회귀/P0P1유무,한계,최종verdict를원문markdown으로확정하라. A도B01반례를교차검사하되동의강요아님. 새코드수정이필요하면구체재현후BLOCK.
원본B의PNG13개 언급과 gitdiff PNG12개 차이는통합report에기록하되원본을고쳐쓰지않는다.
결과:실행ID,시작종료UTC,실제hash/clean,정확한검증결과,최종verdict.
Reviewer B, worktree F:/dev/canview-wt/review-plan-ui-b.
```

## 독립 결과 원문

# Reviewer B — Post-fix 독립 재검토

**최종 판정: PASS. B-01·B-02·B-03 모두 FIXED. 신규 확정 P0–P3 finding과 미해결 P0/P1은 없다.**

지정 변경의 merge 검토 판정이며, 차량 release·제작·OTA 완성·CAN TX 승인은 아니다.

## 실행·격리 기록

| 항목 | 실제 기록 |
|---|---|
| 실행 ID | `2026-09-06-plan-ui-b-postfix-011732Z` |
| 시작 UTC | `2026-09-06T01:17:32.2060742Z` |
| 종료 UTC | `2026-09-06T01:21:32.0965491Z` |
| worktree | `F:/dev/canview-wt/review-plan-ui-b` |
| 시작·종료 실제 HEAD | `1f93b8adb34a48537db69b950c8a99ce89859760` |
| 시작·종료 상태 | `git status --porcelain=v1` 출력 없음, clean |
| 검토 delta | `d078437722635a2ef4c067ca3c08bc6d801d270c..1f93b8adb34a48537db69b950c8a99ce89859760` |

17개 파일 전체 delta와 기존 변경에 대한 회귀 영향을 검토했다. 확정 저장된 A/B 최초 원문도 교차 확인했다.

필수 문서와 workflow/review 규칙, 관련 정본 및 T-005/T-403을 읽었다. `embedded-architecture`, `embedded-cstyle`, `embedded-documentation`을 직접 완독하여 소유권·상태 전이·정본 일관성 검토에 적용했다. Hallmark는 사용하지 않았다.

Tracked 파일·원본 보고서 수정, commit·push 없음. 추가 probe만 ignored `.tools`에 `apply_patch`로 생성했다.

## Finding disposition

### B-01 · P1 — FIXED

수정 위치: [canview_auto_sport.c:217](F:/dev/canview-wt/review-plan-ui-b/firmware/communicator/stm32/src/canview_auto_sport.c:217).  
회귀: [test_automation_regressions.c:441](F:/dev/canview-wt/review-plan-ui-b/tests/automation/test_automation_regressions.c:441).

fresh 비-SPORT 관찰에 의한 권한 철회에서 `!tick_gap` 제한이 제거됐다. gap의 새 명령·feedback 승격 금지는 유지하면서 소유권과 이전 snapshot을 폐기한다.

- 정상 ECO→SPORT 진입·feedback부터 시작하는 **4개 mode×5개 elapsed 경계, 20개 경우**가 통과했다.
- Reviewer B의 별도 [post-fix probe](F:/dev/canview-wt/review-plan-ui-b/.tools/review-b-post-sport.c)는 **40개 경우**를 통과했다.
- 추가 probe는 stale 관찰 후 freshness 회복도 포함한다. 이후 사용자가 선택한 SPORT, disable/re-enable 및 30초 후속 호출에서 오래된 복원 의도가 발생하지 않았다.

기존 B-01 반례는 해소됐다. stale feedback·pending timeout·일반 gap 회귀도 통과했다. Windows stderr/abort 설정과 CTest 30초 timeout이 추가된 것도 확인했다.

### B-02 · P2 — FIXED

정본: [implementation-readiness.md:383](F:/dev/canview-wt/review-plan-ui-b/docs/architecture/implementation-readiness.md:383).

공통 `evidence_grade`와 candidate `review_status`가 별도 축으로 명확히 분리됐다.

- `UNKNOWN`과 `REJECTED`를 같은 등급이나 별칭으로 처리하지 않는다.
- `APPROVED`만으로 VERIFIED 또는 서명 profile gate를 대체하지 않는다.
- unknown enum, 거절된 revision, derived 승격, 과거 DEMO export의 암묵 변환을 거절한다.
- 진단·차량 문서와 T-005/T-403의 부정 수용 기준이 같은 정본을 참조한다.

이는 **계획·규범 충돌의 수정 완료**다. 실제 schema/adapter 구현과 해당 실행 시험은 여전히 BLOCKED 후속 task이며, 이번에 구현 완료로 계산하지 않았다.

### B-03 · P3 — FIXED

수정 위치: [check-browser.cjs:74](F:/dev/canview-wt/review-plan-ui-b/tools/ui/check-browser.cjs:74).

독립 drive+warning fixture, animation 완료 처리, 선택 탭·본문 opacity 검사가 추가됐다. tracked 경고 PNG와 이번 실행의 `.tools/ui-check/ui-drive-warning.png` 모두 주행 본문이 표시되는 것을 직접 확인했다.

두 파일의 SHA-256도 동일하다.

```text
F66E502176084AE06E7EAF10A24CDBACA4A2FC4BA99BA07137C9F3A3447D2EB0
```

main의 `M-P2-01` 중복 관찰과 함께 종결 가능하다. 최초 B-03의 ID·심각도는 그대로 보존한다.

## 추가 변경·회귀 확인

main `M-P2-02`의 [LVGL 경고 배치 변경](F:/dev/canview-wt/review-plan-ui-b/ui/lvgl/canview_ui.c:1279)을 검토했다.

- 모든 탭에서 128px 중앙 경고, 비주행 탭 60% 불투명도.
- 중앙 hit-test가 overlay 유무에 따라 바뀌지 않음.
- 정상 제한 표지는 36px header 영역에 배치하고 연결 요약을 숨겨 겹침 방지.
- 제한 해제 시 연결 요약 복원.
- 기존 품질·freshness·pending·결과 확인 경계 유지.

실제 LVGL Debug/Release 회귀에서 관련 검사와 기존 수명·상태 검사가 모두 통과했다.

## 실행 검증

모두 지정 worktree에서 직접 실행했다. 브라우저 명령에 `--screenshots`는 사용하지 않았다.

| 검증 | 실제 결과 |
|---|---|
| `C:/Python314/python.exe -B -X utf8 tools/validate_plan.py` | 상세 46, 오류 0 |
| `C:/Python314/python.exe -B -X utf8 -m unittest discover -s tests -p test_plan_validation.py -v` | 35/35 PASS |
| `C:/Python314/python.exe -B -X utf8 tools/validate_document_links.py` | 문서 133, 로컬 대상 944, 오류 0 |
| 지정 `NODE_PATH` 후 `node tools/ui/check-browser.cjs` | driver 74, diagnostic 10그룹 PASS; JS 오류·외부 요청 0 |
| `./tools/ui/validate-lvgl.ps1 -LvglSource F:/dev/canview/.tools/lvgl-8.4.0` | Debug CTest 1/1 PASS, 30.59초 |
| automation Debug configure/build/CTest | 12/12 PASS, 0.40초 |
| LVGL Release configure/build/CTest | 1/1 PASS, 18.38초 |
| 독립 SPORT probe, MSVC `/std:c11 /utf-8 /W4 /WX` | 40개 경우 PASS |
| post-fix delta 및 원base→post-fix `git diff --check` | 모두 PASS |

빌드는 지정 VS Developer PowerShell에서 다음 구성을 사용했다.

```powershell
cmake -S tests/automation -B .tools/automation-review -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build .tools/automation-review --parallel 6
ctest --test-dir .tools/automation-review --output-on-failure

cmake -S tests/lvgl -B .tools/lvgl-review-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DLVGL_SOURCE_DIR=F:/dev/canview/.tools/lvgl-8.4.0
cmake --build .tools/lvgl-review-release --parallel 6
ctest --test-dir .tools/lvgl-review-release --output-on-failure
```

공식 LVGL checkout은 종료 시에도 `4495f428630cc1741bd8bfd977f080e8460e8e8d`, clean이었다. Release 빌드에는 upstream C4090 경고와 assertion 유지용 `/DNDEBUG`→`/UNDEBUG` D9025 경고가 있었으며, 빌드·시험 실패는 없었다.

## 기록 정정·한계

최초 B 원문의 “변경 PNG 13개”는 개수 오기다. Git diff 기준 실제 변경은 **PNG 12개와 텍스트 102개, 총 114개**다. 최초 원문은 수정하지 않았으며 이 정정을 통합 report에 기록해야 한다.

이번 재검토에서 target firmware, 실제 transport/backend·인증·영속 저장, OTA·RF·전원 차단·PCB/HIL·실차/TX, Android/iOS 실기기, sanitizer, 최종 한글 font·LCD FPS·장시간 soak는 실행하지 않았다.

**B-01~03은 모두 FIXED로 종결하며 지정 post-fix candidate에 PASS를 부여한다. 차량 CAN TX의 기존 NO-GO 경계는 유지한다.**
