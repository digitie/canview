# T-007 receiver B-RX-01 post-fix — Reviewer A 독립 raw report

## 실행·격리

- 역할: Reviewer A / Maxwell
- 전문: C runtime, bounds, cleanup·context 수명, false PASS
- Execution UUID: `88ac26fc-3a43-4868-93f5-3f03e5d6a374`
- 시작 UTC: `2026-09-19T04:47:48.3049052Z`
- 종료 UTC: `2026-09-19T04:49:03.9761866Z`
- 저장소: `F:/dev/canview-wt/t007-ota-container`
- Base: `0acdc453cd8503631ee179c3bd8b49a1e30fce1f`
- Candidate: `c7f5780b48810b3e2f1ba1bf4fc0792c013b0d62`

시작·종료에 두 commit의 `cat-file -e` 성공과 `rev-parse` hash 일치를 확인했다. 소스는 해당 Git object의 `show/diff`로만 읽었다. moving worktree, 기존 raw 및 상대 새 finding은 열람하지 않았다. 파일 수정·compile·시험 실행·commit·다른 agent 호출은 하지 않았다.

## B-RX-01 P2 재판정

**FIXED — A의 독립 정적 확인. 직접 실행 검증이나 원 B의 판정을 대신하지 않는다.**

기존 실패 시나리오:

본문 변이 case에서 마지막 바이트를 공급하기 전에 `body_open` 또는 일반 `body_feed`가 AUTH_FAILED를 반환하면, 이전 `status == expected` 조건은 이를 변이 검출 성공으로 인정했다. 마지막 바이트 변이 검사를 수행하지 않고도 시험이 통과할 수 있는 P2 검증 결함이다. 이전 A 리뷰에서는 이를 구체적인 결함으로 식별하지 못했다.

수정 근거:

- `tests/fixtures/idf-ota-image/main/receiver.c:57`: case마다 `body_tamper_rejected=false`로 초기화한다.
- 같은 파일 `79–88`: 마지막 1B 변이 feed에 도달한 경우에만 그 호출의 AUTH_FAILED를 플래그에 기록한다.
- 같은 파일 `96–97`: 본문 변이 case는 최종 status뿐 아니라 플래그와 `offset == size`를 모두 요구한다.

따라서 조기 open/feed 오류는 플래그를 설정하지 못해 실패한다. 마지막 변이 feed가 OK이거나 다른 오류를 반환하는 경우도 통과하지 않는다. 플래그가 앞선 case에서 남아 성공을 만드는 경로도 없다.

권고: 이 조건과 아래 mutant 대조를 유지한다. 추가 수정 요구는 없다.

## 회귀·oracle 검토

### 실제 C mutant 구성

`tests/ota/check_receiver_oracle.py:11–27`은 치환 anchor가 정확히 한 번 존재하는지 검사한다. 조기 오류를 본문 변이 case에만 주입하며, 이전 oracle 대조는 **동일한 early-feed mutant**에서 판정식만 status-only로 되돌린다.

`30–44`는 세 실행파일을 각각 컴파일하고 정확한 종료값을 요구한다.

| 변형 | 요구 종료값 |
|---|---:|
| early-open + 수정 oracle | 1 |
| early-feed + 수정 oracle | 1 |
| 동일 early-feed + 이전 oracle | 0 |

`40`의 compile은 `check=True`, timeout 40초다. 실행 timeout은 `42`에서 10초이며, 예외를 성공으로 바꾸는 handler가 없다. 예상값과 다른 종료값도 실패한다. 따라서 compile 실패·timeout·일반적인 비정상 종료를 mutant 검출 PASS로 집계하지 않는다.

단, 주입은 실제 provider 내부 고장이 아니라 **호출 직후 caller status를 덮는 C 변형**이다. 이번 caller oracle 결함의 대조에는 적합하지만 SDK 내부 오류 상태 전체를 시험했다는 의미는 아니다.

### Bounds·입력 수명·cleanup

- `receiver.c:61–68`: prefix의 실제 `consumed`만 offset에 더하는 기존 동작이 유지된다.
- `79–88`: 마지막 바이트는 지역 복사본으로 변조한다. 원본 입력은 변경하지 않으며 새 플래그는 입력 pointer를 보존하지 않는다.
- `94–99`: 실패 status를 finish로 덮지 않고, 판정 실패 뒤에도 body reset을 수행한다.
- `99–102`: body reset 실패 시 즉시 false로 종료하고 PSA close를 먼저 실행하지 않는다. 기존 정적 context 보존과 cleanup 순서가 유지된다.
- 새로운 동적 할당·큰 배열·재귀·task·ISR·writer 호출은 없다. 추가 bool의 실제 frame 영향은 본 리뷰에서 측정하지 않았다.

### CTest·문서

`CMakeLists.txt:144–156`에서 기존 정상 receiver 시험을 유지하면서 Windows 전용 oracle CTest를 추가한다. compiler와 현재 `canview_ota` target library를 전달한다.

fixture `README.md:92–95`는 마지막 변이 feed의 오류와 이전 oracle 대조를 설명한다. `97–109`는 synthetic-only, native/install 권한 없음, 장치 자원 측정 NOT_RUN을 유지한다.

`docs/resume.md:48–57`, T-007 `115–129`, journal `3–19`는 수정 후 작성자 검증과 원 reviewer·새 CI 대기를 구분하며 전체 acceptance BLOCK을 유지한다.

## 신규 findings

- P0: 없음
- P1: 없음
- P2: 없음
- P3: 없음

검토한 delta에서 추가 결함은 확인하지 못했다. 신규 finding의 실패 시나리오·영향·권고는 해당 없음이다.

## 실제 읽은 파일·명령

Candidate에서 직접 읽은 범위:

- `receiver.c:1–105`, `receiver.h:1–15`
- `check_receiver_oracle.py:1–50`
- `idf_receiver_host.c:1–20`
- `shared/ota/src/body.c:1–257`, `body.h:1–105`
- `firmware/platform/esp32s3/ota_crypto.c:1–186`, `ota_crypto.h:1–60`
- `tests/ota/cng_provider.c:1–133`, `cng_provider.h:1–28`
- fixture `main/main.c:1–46`, `README.md:1–109`
- root `CMakeLists.txt:1–55,140–160` 및 전체 변경분
- `AGENTS.md`, `docs/README.md` 전체
- `docs/resume.md:1–187`, T-007 상세 `1–186`
- journal 이번 추가분 `3–19`
- `docs/runbooks/agent-workflow.md:107–188`

embedded-cstyle·embedded-architecture·embedded-documentation 스킬을 직접 읽고 수명·의존성·검증 주장 점검에 적용했다.

실제 명령 형태:

```text
git cat-file -e "<hash>^{commit}"
git rev-parse "<hash>^{commit}"
git diff --find-renames --name-status <base> <candidate>
git diff --find-renames <base> <candidate> -- <핵심 source·CMake·현재 문서>
git show <candidate>:<path>
git diff --find-renames --name-only <base> <candidate> -- <기존 core·provider·caller·golden 경로>
```

마지막 경로 한정 diff에서 기존 core/provider/caller/golden 변경이 없음을 확인했다. PowerShell은 출력 행 번호와 범위 선택에 사용했다.

## 한계·미실행

핵심 수정 source와 새 oracle 전체는 읽었다. review archive/evidence 본문은 의도적으로 제외했고, 변경 없는 parser 전체·SDK 내부 구현을 재감사하지 않았다.

작성자의 Debug/Release 146/146, CNG mutant 3건, SDK 빌드·metadata 검사·frame 624B는 **제공된 실행 근거**이며 본인의 실행 결과가 아니다. 로그·CI artifact도 직접 검증하지 않았다.

- Compile·CNG mutant·SDK build 독립 실행: **NOT_RUN**
- SDK device PSA·native Flash·총 stack/heap/timing: **NOT_RUN**
- Physical/HIL: **NOT_RUN**
- Vehicle TX: **NO-GO**

## Verdict

**PASS — B-RX-01 수정 및 해당 delta의 object-only 정적 재검토 범위에 한정.**

B-RX-01은 정적으로 FIXED이며 신규 P0–P3는 없다. 전체 acceptance **BLOCK은 유지**한다. 전체 T-007 완료, CI 통과, 장치 검증 또는 merge 승인이 아니다.
