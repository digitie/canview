# Reviewer A post-fix 요청·원본 evidence

- subagent ID: `01a07435-a059-7ae1-bbc4-6c89bb421599`
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
Reviewer A, worktree F:/dev/canview-wt/review-plan-ui-a.
```

## 독립 결과 원문

# Reviewer A — Post-fix 독립 재검토

## 최종 판정: PASS

Post-fix 17개 파일 전체 delta와 기존 변경의 회귀 영향을 검토했다. **B-01/B-02/B-03은 A의 교차검증 기준 FIXED**이며, 신규 P0–P3 finding은 없다. 이 판정은 차량 release·제작·OTA 완성·CAN TX 승인이 아니며, 원 reviewer B의 종결 재확인을 대신하지 않는다.

최초 A 리뷰는 ownership과 tick gap의 교차 반례를 놓쳤다. B-01은 독립 재현으로 확인된 유효한 finding이다. 최초 원본은 수정하지 않는다.

## 실행·격리 기록

| 항목 | 실제 기록 |
|---|---|
| 실행 ID | `2026-09-06-plan-ui-a-postfix-011728Z` |
| 시작 UTC | `2026-09-06 01:17:28 UTC` |
| 종료 UTC | `2026-09-06 01:22:23 UTC` |
| worktree | `F:/dev/canview-wt/review-plan-ui-a` |
| 검토 delta 시작 | `d078437722635a2ef4c067ca3c08bc6d801d270c` |
| 시작·종료 실제 HEAD | `1f93b8adb34a48537db69b950c8a99ce89859760` |
| 시작·종료 상태 | `git status --porcelain=v1` 출력 없음, clean |

Tracked 파일 수정·commit·push·merge는 하지 않았다. 독립 probe는 ignored `.tools`에만 `apply_patch`로 작성했다.

## Finding 교차 판정

### B-01 · P1 — FIXED

수정 위치: [SPORT 권한 철회](F:/dev/canview-wt/review-plan-ui-a/firmware/communicator/stm32/src/canview_auto_sport.c:217), [교차 회귀시험](F:/dev/canview-wt/review-plan-ui-a/tests/automation/test_automation_regressions.c:441).

Fresh 비-SPORT 관찰에 의한 권한 철회에서 `!tick_gap` 조건이 제거됐다. Gap의 새 명령·feedback 승격 금지는 유지된다.

동일 독립 probe를 이전 소스와 수정 소스에 각각 링크했다. 정상 ECO→SPORT 진입과 feedback으로 소유권을 얻은 뒤 다음 **80개 조합을 각 소스에서** 검사했다.

- 외부 mode 4개 × elapsed `100/250/251/5000/UINT32_MAX` × enabled 2개 × freshness 2개.
- 이전 소스: fresh 외부 mode와 gap이 겹치면 소유권·ECO snapshot이 남고, 다음 SPORT/disable에서 오래된 restore 의도가 발생했다.
- 수정 소스: fresh 외부 mode는 gap 여부와 무관하게 소유권·snapshot을 폐기하며 이후 오래된 restore가 발생하지 않았다.
- Stale 관찰만으로 소유권을 철회하는 부작용도 없었다.

두 비교시험 모두 기대한 이전 결함/수정 동작과 일치했다. 여기서 baseline 시험 PASS는 **결함 재현 성공**을 뜻한다. 실제 차량 송신 시험은 아니다.

### B-02 · P2 — FIXED

정본 위치: [implementation-readiness §9.2](F:/dev/canview-wt/review-plan-ui-a/docs/architecture/implementation-readiness.md:385).

`evidence_grade` 네 값과 별도 `review_status`가 분리됐고, 진단 descriptor·차량 문서·T-005·T-403이 같은 정본을 참조한다.

UNKNOWN·unknown enum·REJECTED revision·구 DEMO export의 암묵 변환 및 derived VERIFIED 승격을 거부하는 수용 조건을 확인했다. APPROVED만으로 검증 등급이나 서명 gate를 대신하지 않는다.

이는 **계획 정본 충돌의 수정**이다. 실제 schema·adapter·migration 구현은 T-005/T-403의 BLOCKED 범위로 남아 있다.

### B-03 · P3 / M-P2-01 — FIXED

수정 위치: [브라우저 촬영·상태 검사](F:/dev/canview-wt/review-plan-ui-a/tools/ui/check-browser.cjs:75).

Finite animation 종료 촬영, 독립 drive+warning fixture, 선택 탭과 본문 opacity 검사를 확인했다. Tracked PNG와 이번 재생성 PNG 모두 주행 본문이 보였으며 SHA-256도 동일했다.

`F66E502176084AE06E7EAF10A24CDBACA4A2FC4BA99BA07137C9F3A3447D2EB0`

### M-P2-02 — FIXED 확인, 신규 회귀 없음

수정 위치: [LVGL overlay](F:/dev/canview-wt/review-plan-ui-a/ui/lvgl/canview_ui.c:1279).

모든 탭의 128px 중앙 경고, 비주행 탭 60% opacity, 과속 우선순위, 정상 제한 표지의 36px header 배치와 연결 요약 숨김/복원을 확인했다. 중앙 hit-test가 overlay 표시 전후 같은 하위 객체를 반환하는 시험을 포함해 Debug·Release 모두 통과했다.

## 검토 범위

필수 진입 문서와 workflow §5를 다시 읽고, 최초 B 원본을 교차 확인했다. Post-fix **17개 파일 전체**와 관련 SPORT 상태기계, evidence 정본·두 task, UI 설계·시험을 대조했다.

기존 114개 파일 검토를 바탕으로 전체 automation·브라우저·LVGL 회귀를 재실행했다. Embedded architecture/cstyle/documentation/RTOS 스킬을 직접 재완독해 권한 철회·시간·수명·정본 추적에 적용했다. Hallmark는 사용하지 않았다.

## 실제 검증 결과

| 검증 | 결과 |
|---|---|
| `C:/Python314/python.exe -B -X utf8 tools/validate_plan.py` | 상세 46개, 오류 0 |
| `C:/Python314/python.exe -B -X utf8 -m unittest discover -s tests -p test_plan_validation.py -v` | 35/35 PASS |
| `C:/Python314/python.exe -B -X utf8 tools/validate_document_links.py` | 문서 133개·로컬 target 944개, 오류 0 |
| 지정 NODE_PATH + `node tools/ui/check-browser.cjs` | Driver 74개·diagnostic 10그룹 PASS, 페이지 오류·외부 요청 0 |
| 지정 `validate-lvgl.ps1 -LvglSource ...` | 실제 LVGL Debug CTest 1/1 PASS, 28.68초 |
| Automation Debug configure/build/CTest | 12/12 PASS |
| 실제 LVGL Release configure/build/CTest | assertion 활성 상태 1/1 PASS, 14.63초 |
| 독립 B-01 이전/수정 비교 probe | 2/2 PASS, 각 80개 조합 |
| A의 삭제→즉시 재생성 probe 재빌드·재실행 | 1/1 PASS, 36개 경우·heap 증가 없음 |
| Post-fix delta 및 원base→post-fix `git diff --check` | 모두 PASS |

Automation과 Release 검증은 지정 VS Developer PowerShell에서 다음 명령으로 수행했다.

```powershell
cmake -S tests/automation -B .tools/automation-review -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build .tools/automation-review --parallel 6
ctest --test-dir .tools/automation-review --output-on-failure

cmake -S tests/lvgl -B .tools/lvgl-review-a-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DLVGL_SOURCE_DIR=F:/dev/canview/.tools/lvgl-8.4.0
cmake --build .tools/lvgl-review-a-release --parallel 6
ctest --test-dir .tools/lvgl-review-a-release --output-on-failure

cmake -S .tools/review-a-postfix -B .tools/review-a-postfix/build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build .tools/review-a-postfix/build --parallel 6
ctest --test-dir .tools/review-a-postfix/build --output-on-failure -V
```

공식 LVGL은 종료 확인에서도 `4495f428630cc1741bd8bfd977f080e8460e8e8d`, clean이었다. Release의 `/DNDEBUG`→`/UNDEBUG` 재정의 경고는 시험 assertion 유지 설정이며 검증 실패는 없었다. `--screenshots`는 사용하지 않았다.

## 한계·기록 정합성

- 신규 P0/P1/P2/P3: **모두 0건**. A 자체 최초 finding은 없어 별도 closure 대상은 없다.
- 실제 transport/backend·schema migration·target build·sanitizer·RF·PCB·단전/HIL·실차/TX는 미검증이다.
- 최종 한글 font·LCD FPS·실기기 touch/PWM·장시간 soak·휴대폰 OTA도 미검증이다.
- Windows host PASS를 고정 Arm/전체 target toolchain PASS로 확대하지 않는다.
- 최초 B 원본의 “PNG 13개”와 달리 원base→candidate Git delta의 PNG는 **12개**임을 재확인했다. 통합 report에 차이를 기록하되 원본은 보존해야 한다.

**최종 verdict: PASS — 지정 post-fix 변경에 한정. 차량 CAN TX의 NO-GO는 유지한다.**
