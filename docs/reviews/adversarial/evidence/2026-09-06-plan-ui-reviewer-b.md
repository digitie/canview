# Reviewer B 요청·원본 evidence

- subagent ID: `01a07435-a143-7653-baec-15a8b86f32a8`
- 요청 전달 UTC: `2026-09-06 00:54:27 UTC`
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
Reviewer B: 프로토콜 보안·계획 의존/정본·웹 통합 전문가. 42요구/46task DAG·OTA/설정/페어링 규범 일관성, diagnostic malformed/state, driver 상호작용·검증 거짓양성을 우선 공격하되 전체 delta도 검토.
지정 worktree: F:/dev/canview-wt/review-plan-ui-b
상대 결과를 기다리지 말고 리뷰에 즉시 착수.
```

## 독립 결과 원문

# Reviewer B 독립 적대적 리뷰

**판정: BLOCK — P1 1건, P2 1건, P3 1건.** 제공된 검증은 모두 통과했지만, 추가 재현에서 SPORT 소유권 안전 계약 위반을 확인했다.

## 실행·격리 기록

- 실행 ID: `2026-09-06-plan-ui-b-005438Z`
- 시작 UTC: `2026-09-06T00:54:38.0310369Z`
- 종료 UTC: `2026-09-06T01:07:38.4566270Z`
- worktree: `F:/dev/canview-wt/review-plan-ui-b`
- 시작·종료 실제 HEAD: `d078437722635a2ef4c067ca3c08bc6d801d270c`
- 시작·종료 `git status --porcelain=v1`: 모두 출력 없음, clean.
- tracked 파일 수정·commit·push 없음. 추가 재현 소스는 ignored `.tools`에만 `apply_patch`로 생성했다.
- 다른 reviewer 결과는 열람하지 않았다. 관련 embedded 6개 skill을 직접 읽고 적용했으며 Hallmark는 사용하지 않았다.

## 검토 범위

base `4aeb2912da063c6fcb0d8715aa46f84c7d1d1b0f`부터 candidate까지 114개 파일 delta를 검토했다. 변경 PNG 13개도 확인했다.

필수 진입 문서, workflow/review 규칙, 42개 요구 추적표, 46개 상세 task 변경과 DAG, implementation-readiness, OTA·ESP-NOW·UART 계약, 설정·pairing·evidence 규범을 대조했다. 운전자·진단 웹 각 5뷰, LVGL adapter와 host 시험, portable automation 변경 및 검증 도구도 검토했다.

OTA 기존 회로는 변경 전제로만 확인했다. 제조·차량 release 승인을 판단하지 않았다.

## Finding

### B-01 · P1 — 시간 gap에서 외부 mode 관찰을 버려 오래된 SPORT 복원 권한이 살아남음

위치: [canview_auto_sport.c:217](F:/dev/canview-wt/review-plan-ui-b/firmware/communicator/stm32/src/canview_auto_sport.c:217), 217–224행.  
위반 계약: [automation.md:226](F:/dev/canview-wt/review-plan-ui-b/docs/architecture/automation.md:226).

`!tick_gap` 때문에 다른 유효 mode를 관찰했더라도 소유권 해제가 생략된다. 뒤의 gap 처리도 `owns_sport_mode`와 `previous_mode`를 보존한다.

재현은 정상 진입 후 도달 가능한 `ACTIVE / owns=true / previous=ECO` 상태에서 수행했다. 입력은 fresh, local safety 정상, 물리 버튼 이벤트 누락 조건이다.

| 입력 순서 | 실제 결과 |
|---|---|
| NORMAL 관찰, elapsed=100 ms | `MANUAL_HOLD`, 소유권·snapshot 삭제 |
| NORMAL 관찰, elapsed=251 ms | `INHIBITED`, 소유권·ECO snapshot 유지 |
| 두 번째 경우 다음 tick: SPORT 관찰, enabled=false, elapsed=100 ms | `RESTORE_PREVIOUS(ECO)` 발생 |

독립 MSVC 재현: [.tools/review-b-sport-gap.c](F:/dev/canview-wt/review-plan-ui-b/.tools/review-b-sport-gap.c).

영향: 자동화가 이미 관찰한 외부 mode 변경을 잊고, 이후 운전자가 선택한 SPORT에 오래된 ECO 복원 의도를 낼 수 있다. 현재는 순수 helper 출력이며 실제 CAN 송신을 재현한 것은 아니다.

수정안: fresh한 비-SPORT 관찰에 의한 **권한 철회**는 gap의 명령·feedback 승격 금지와 분리한다. gap에서도 소유권·이전 snapshot을 폐기하고 `MANUAL_HOLD`로 가야 한다. 기존 ownership 시험과 gap 시험을 교차한 회귀를 추가해야 한다.

### B-02 · P2 — runtime evidence 정본이 서로 다른 enum을 요구함

위치: [diagnostic-bridge.md:327](F:/dev/canview-wt/review-plan-ui-b/docs/architecture/diagnostic-bridge.md:327), [target-2017-tucson.md:97](F:/dev/canview-wt/review-plan-ui-b/docs/vehicle/target-2017-tucson.md:97).  
충돌 정본: [implementation-readiness.md:388](F:/dev/canview-wt/review-plan-ui-b/docs/architecture/implementation-readiness.md:388).

- 새 진단·차량 문구: `CANDIDATE/OBSERVED/VERIFIED/REJECTED`가 T-005의 runtime evidence enum이라고 지정한다.
- readiness §9.2: `UNKNOWN/CANDIDATE/OBSERVED/VERIFIED`로 고정한다.
- T-005 자체에는 이 차이를 해결하는 전체 enum 또는 변환 규칙이 없다.

영향: 공통 schema 작성자가 UNKNOWN과 REJECTED의 포함 여부, candidate 심사 상태와 evidence 등급의 관계를 다시 결정해야 한다. 이는 이번 계획의 “정본→task가 모순 없이 연결됨” 수용 조건을 충족하지 못한다. metadata/DAG 검사는 이 의미 충돌을 검출하지 않는다.

수정안: 공통 `evidence_grade`를 하나로 확정하고, candidate의 `REJECTED` 심사 상태가 별도 축인지 명시한다. 세 문서와 T-005를 동기화하고 UNKNOWN·REJECTED의 변환 및 derived evidence 부정 시험을 연결해야 한다.

### B-03 · P3 — 경고 스크린샷이 투명한 전환 프레임인데 검사 통과로 저장됨

위치: [check-browser.cjs:74](F:/dev/canview-wt/review-plan-ui-b/tools/ui/check-browser.cjs:74), 74–75행.  
산출물: [ui-drive-warning.png](F:/dev/canview-wt/review-plan-ui-b/docs/images/ui-drive-warning.png).

`isVisible()` 확인 직후 캡처하므로 `screen-enter` 시작 프레임을 저장한다. tracked PNG와 이번 검사 산출물 모두 주행 본문이 보이지 않았다.

동일 Playwright 조건으로 독립 재현했다.

```text
탭 전환 직후: active=true, hidden=false, opacity=0, animation time=0
clock 200 ms 진행 후: opacity=1, animation finished
```

재현: [.tools/review-b-browser.cjs](F:/dev/canview-wt/review-plan-ui-b/.tools/review-b-browser.cjs).

영향: 경고 중 주행 화면의 실제 배치를 보여 주지 않는 이미지가 검증 근거로 남는다. 영구적인 화면 소실 결함은 아니며 캡처 타이밍 문제다.

수정안: 전환 완료와 computed opacity를 확인한 후 캡처하고 이미지를 재생성한다. `isVisible()`만으로 렌더링 완료를 판단하지 않아야 한다.

## 검증 결과

| 실행 명령 | 결과 |
|---|---|
| `C:/Python314/python.exe -B -X utf8 tools/validate_plan.py` | 상세 task 46, 오류 0 |
| `C:/Python314/python.exe -B -X utf8 -m unittest discover -s tests -p test_plan_validation.py -v` | 35/35 PASS |
| `C:/Python314/python.exe -B -X utf8 tools/validate_document_links.py` | 문서 133, 로컬 대상 939, 오류 0 |
| 지정 `NODE_PATH` 설정 후 `node tools/ui/check-browser.cjs` | driver 72, diagnostic 10그룹 PASS, JS 오류·외부 요청 0 |
| `./tools/ui/validate-lvgl.ps1 -LvglSource F:/dev/canview/.tools/lvgl-8.4.0` | 공식 clean `4495f428630cc1741bd8bfd977f080e8460e8e8d` 실제 C 링크, CTest 1/1 PASS |
| 지정 VS dev shell → `cmake -S tests/automation -B .tools/automation-review -G Ninja -DCMAKE_BUILD_TYPE=Debug` → build `--parallel 6` → CTest | 11/11 PASS |
| 두 commit 사이 `git diff --check` | PASS |
| 독립 SPORT·브라우저 probe | B-01·B-03 재현 |

SPORT probe 최초 빌드는 `/utf-8` 누락으로 C4819 `/WX` 실패했다. 옵션을 추가한 재실행에서 정상 빌드·재현했다. LVGL upstream의 C4090 경고는 있었으며 프로젝트 시험 실패는 없었다.

## 공격 범위와 남은 불확실성

다음 조합을 코드·계약 및 실행 시험으로 확인했다.

- ACK와 적용 완료 분리, duplicate/result/timeout, 단일 pending, manual override, stale feedback와 scheduler gap.
- task의 형식적 DAG와 실제 산출물 의존, OTA fixture 단계와 실물 qualification 분리.
- 진단 ID/DLC identity, Intel·Motorola·signed 64-bit decode, malformed decimal·범위·enum, stale/gap/disconnect, pending 취소, 후보 export whitelist.
- 운전자 값별 품질, DPF lamp와 전체 정상 판정 분리, 정차 잠금, RTC 날짜·분 선택, 경고 터치 투과.
- LVGL callback 삭제, 부모 삭제, 반복 create/destroy, animation 정리와 pending 표시.

실제 OTA/transport/backend, target firmware, RF·전원 차단·PCB/HIL·실차/TX, Android/iOS 실기기, 최종 한글 font·LCD FPS·장시간 soak는 미실행이다.

B-01과 B-02를 수정하고 관련 회귀·정본 대조를 다시 통과하기 전에는 이 candidate의 merge 승인을 내릴 수 없다. **계획 완료와 host PASS는 차량 release 승인이 아니다.**
