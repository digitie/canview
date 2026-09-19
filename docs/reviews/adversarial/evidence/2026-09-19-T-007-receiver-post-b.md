# T-007 receiver B-RX-01 post-fix — Reviewer B 원문

## 실행·판정

- Execution UUID: `622b300d-d9a2-4df8-b5dc-ca489228dca1`
- 시작 UTC: `2026-09-19T04:47:48.7336574Z`
- 종료 UTC: `2026-09-19T04:49:47.3540213Z`
- 역할: Reviewer B / Huygens — oracle, CTest/CI 연결, 검증 근거 귀속.
- 저장소: `F:/dev/canview-wt/t007-ota-container`
- 실제 확인 base: `0acdc453cd8503631ee179c3bd8b49a1e30fce1f`
- 실제 확인 candidate: `c7f5780b48810b3e2f1ba1bf4fc0792c013b0d62`

**B-RX-01: FIXED — 정적 재확인.**  
**Delta verdict: PASS — 이 제한된 수정의 정적 검토 범위.**

전체 T-007 acceptance 감사의 **BLOCK은 유지**한다. 전체 Task·SDK device 실행·CI·merge 승인이 아니다.

## 격리·실제 명령

시작·종료 모두 두 commit의 `cat-file -e` 성공과 `rev-parse` 일치를 확인했다. 소스는 Git object의 `show/diff`로만 읽었다. moving worktree 소스, 보존 raw, 다른 reviewer finding은 열지 않았다.

파일 수정·compile·시험 실행·commit·다른 agent 호출 없음.

실제 명령 계열:

```text
[guid]::NewGuid().ToString()
[DateTime]::UtcNow.ToString('o')
git cat-file -e "<hash>^{commit}"
git rev-parse "<hash>^{commit>"
git diff --find-renames --name-status <base> <candidate>
git diff --find-renames <base> <candidate> -- <대상 경로>
git show <candidate>:<path>
git diff --exit-code <base> <candidate> -- <불변 의존 경로>
git diff --check <base> <candidate> -- . ':!docs/reviews/**'
```

PowerShell로 행 번호를 붙였고 `rg -n`으로 build/coverage 관련 경로를 찾았다. 관련 embedded C·architecture·documentation skill을 직접 읽고 적용했으며 자동 수정은 하지 않았다.

직접 읽은 범위:

- `tests/fixtures/idf-ota-image/main/receiver.c:1–105` 전체
- `tests/ota/check_receiver_oracle.py:1–50` 전체
- `receiver.h:1–15`, `tests/ota/idf_receiver_host.c:1–20`
- root `CMakeLists.txt:1–25,144–162` 및 전체 변경 hunk
- `shared/ota/src/body.c:46–88,187–257`
- `CMakePresets.json`, `cmake/CanviewWarnings.cmake`
- `.github/workflows/foundation.yml`의 CTest·coverage·sanitizer 등록 행, `tools/check_coverage.py`의 실행 경로 검색
- `AGENTS.md`, `docs/README.md`
- `docs/resume.md:48–59`, T-007 상세 `:115–134` 및 변경 hunk
- SDK fixture `README.md:84–109`, `docs/journal.md:1–35`
- agent-workflow의 격리·심각도·산출물 규칙

## B-RX-01 수정 확인

기존 실패는 마지막 byte 변조 전에 발생한 `AUTH_FAILED`를 본문 변조 검출로 오인하는 것이었다.

수정 근거:

- `receiver.c:57`: 각 case마다 `body_tamper_rejected=false`로 초기화한다.
- `:84–87`: 실제 마지막 byte를 바꿔 전달한 feed의 반환값이 `AUTH_FAILED`일 때만 flag를 설정한다.
- `:96–97`: 본문 변조 case는 최종 오류뿐 아니라 flag와 `offset == size`를 모두 요구한다.

따라서 조기 `body_open` 실패나 변이 전 feed 실패는 flag를 설정하지 못하며 통과할 수 없다. 변이 feed가 OK이고 이후 finish에서만 실패하는 경우도 성공으로 오인하지 않는다.

정상·서명 변조·identity case의 기존 판정은 유지된다. flag는 지역 변수이며 입력·offset 처리 방식을 변경하지 않는다. 판정 실패 후에도 `:99`의 body reset을 거치고, SDK 경로는 `:102`에서 close한다. reset 실패 시 static 자원을 보존하고 즉시 실패하는 계약도 유지된다.

## Mutant 검사·CTest 확인

`check_receiver_oracle.py`는 다음을 요구한다.

- `:23–24`: 일반 feed 또는 open 호출 직후 조기 `AUTH_FAILED` 주입.
- `:26–27`: 같은 early-feed mutant에서 이전 status-only oracle을 복원한 대조군 생성.
- `:30–31`: early-open/early-feed는 정확히 exit 1, weak-oracle 대조군은 exit 0.
- `:11–14`: mutation anchor가 정확히 하나가 아니면 실패.
- `:40`: compile은 `check=True`; compiler 실패·예외·timeout을 검출 성공으로 처리하지 않는다.
- `:41–44`: 실행 timeout은 예외이며, 예상과 다른 종료 코드도 실패한다.

각 변형은 별도 executable로 receiver/CNG/host driver를 컴파일하고, CMake가 전달한 공통 `canview_ota` library를 재사용한다. 이는 세 개의 독립 실행 파일에 대한 **CNG host oracle 시험**이지 PSA SDK 오류 주입이나 장치 실행 시험은 아니다.

root `CMakeLists.txt:154–156`에 Windows 전용 CTest로 등록됐고, 기존 정상 receiver 시험도 유지된다. 새 시험을 Linux/SDK device 검증으로 집계하는 등록은 없다.

## 신규 findings

- P0: 없음
- P1: 없음
- P2: 없음
- P3: 없음

공격한 시나리오는 조기 open/feed 오류, 변이 지점 미도달, finish 단계 오류의 오인, case 간 flag 잔존, cleanup 실패, mutation anchor drift, compile 실패·timeout·비정상 종료의 오인, 공통 파일 입력 실패다.

불변 의존 경로 diff로 portable core, PSA provider, CNG provider, host driver, golden, SDK component/config 및 workflow가 변경되지 않았음을 확인했다. Review 기록을 제외한 delta의 `git diff --check`도 성공했다.

## 근거 귀속·한계

작성자의 Debug/Release `146/146`, mutant 기대 종료값, SDK ELF/MAP/BIN 경고 0·metadata 검사·receiver frame 624B는 **제공받은 실행 근거**다. 본인은 이를 재실행하거나 build 로그·산출물로 독립 확인하지 않았다.

핵심 변경 소스는 모두 읽었다. 미검토·미실행 범위는 실제 compiler/link 결과, coverage 등 계측 구성의 mutant 링크, CI 원격 상태·artifact, SDK 내부·device 실행, 전체 기존 시험 재감사다. 문서에 기록된 이전 CI 다운로드 실패를 candidate CI 성공으로 대체하지 않았다.

- SDK device / native Flash / physical / HIL: **NOT_RUN**
- Device 총 stack·heap·timing: **NOT_RUN**
- Vehicle TX: **NO-GO**
- 일반 native CLI·정상 owner·전체 acceptance: 기존 OPEN/BLOCK 유지

**최종: B-RX-01 FIXED, 신규 finding 없음, 제한된 post-fix 정적 verdict PASS.**
