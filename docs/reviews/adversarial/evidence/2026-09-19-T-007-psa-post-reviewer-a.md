# T-007 PSA post-fix Reviewer A 원본

## 전달 요청 원문

```text
T-007 PSA 원 reviewer post-fix 독립 재검토. 새 executionUUID/start/end UTC 기록. A는 C수명·bounds·회귀, B는 PSA상수·SDK·검증근거 전문을 유지. Repo F:/dev/canview-wt/t007-ota-container. Base 70c7a384f3763b021546fd50f742ea51cf137dc4, candidate 6f078ace276eecce4f8ceabf464929de5eb6bfd7. git cat-file/rev-parse 시작종료 확인; object-only git show/diff로만읽기, movingworktree소스/파일수정/compile/다른agent소환 금지.
두 이전 원문은 candidate evidence에 정확히 보존됐고 통합 docs/reviews/adversarial/2026-09-19-T-007-psa.md에 disposition했다. 이번 post raw 확정전 서로결과 공유금지. B-PSA-01 P3: fixture usage0x0800 수정, host C99 typedef와 SDKfixture _Static_assert에 독립 기대값0x800 검사. 추가 작성자발견CI GCCfailure: 작은context→65/64B인자중첩시험이stringop-overread. 65Bunionbacking으로수정, 시험삭제/경고억제 없음. firmware/platform/esp32s3/ota_crypto.c/h는 불변.
실제검증(작성자): 최종Debug/Release144/144; GCC-O3 strict PASS; ClangASan/UBSan모형149/149lines114/118branches; SDK6.0.3 실제ELF/MAP/BIN 경고0, BIN262144B SHA fadfa94c32694baf02caa5d0e268665b796691aa4e72431e7395fc8768a5f18d,metadata168변이/4절단 PASS. 신규CI진행. 실제PSA standaloneWindows실행은 SDKport의존문제로buildfailed/NOT_RUN이며 journal에분리기록. 정상owner/root/Flash및wholeTask미완료.
자신의finding+전체delta회귀 직접검토. 최종raw한국어: 실행UUID/시작종료/전문/basecandidate/isolation/실제읽은파일명령/file:line finding P0~P3/실패시나리오영향권고/FIXED여부/미검토/physicalHIL NOT_RUN/vehicleTXNO-GO/BLOCK CONDITIONAL PASS. 추가결함없으면 범위정적판정으로명확히하고 전체T007승인말것. 읽지못하면incompleteBLOCK. raw최종응답 반환.
```

## 반환 원문

# T-007 PSA post-fix 독립 재검토 — Reviewer A

## 1. 실행 식별과 판정

- Reviewer: A / Maxwell
- 전문 영역: embedded C 수명·cleanup·부분 초기화·재진입·ownership·bounds 및 delta 회귀
- Execution UUID: `c610ea79-570d-43e6-b4f0-c0da0119deb1`
- 시작 UTC: `2026-09-19T04:00:22.2328900Z`
- 종료 UTC: `2026-09-19T04:01:29.5364810Z`
- 저장소: `F:/dev/canview-wt/t007-ota-container`
- 실제 확인 Base: `70c7a384f3763b021546fd50f742ea51cf137dc4`
- 실제 확인 Candidate: `6f078ace276eecce4f8ceabf464929de5eb6bfd7`
- Verdict: **CONDITIONAL**
- 신규 finding: **P2 1건**
- P0 없음 / P1 없음 / P3 없음

기존 Reviewer A 보고에는 finding이 없었다. 이번에는 중첩 시험 수정에서 새 검증 회귀를 발견했다. Production provider 자체의 신규 결함으로 판정한 것은 아니다.

## 2. 격리 확인

시작·종료 각각 두 commit에 `git cat-file -e`와 `git rev-parse`를 실행했다. 모두 요청 hash와 일치했다.

저장소 내용은 고정 candidate의 `git show candidate:path`와 고정 base/candidate의 `git diff`로만 읽었다.

- Moving worktree 소스 열람 없음
- 컴파일·시험·mutant 실행 없음
- 파일 수정·commit·push 없음
- 다른 agent 소환·결과 공유 없음
- 이전 A/B raw evidence 및 통합 report 본문 열람 없음
- Object-only 방식이므로 worktree HEAD/clean 상태를 검토 근거로 사용하지 않음

`embedded-cstyle`, `embedded-architecture` 스킬을 완독하고 storage·수명·오류 경로 검토에 적용했다. 자동 수정은 하지 않았다.

## 3. 신규 finding

### A-PSA-POST-01 — P2 — OPEN

**Signature/context 중첩 시험이 unready context를 사용하여 중첩 검사 누락을 검출하지 못한다.**

위치:

- `tests/ota/test_esp_ota_crypto.c:89` — 별도 `overlap` union을 `{0}`으로 초기화
- `tests/ota/test_esp_ota_crypto.c:95` — 이 context의 init은 의도적으로 실패
- `tests/ota/test_esp_ota_crypto.c:113` — 정상 초기화 대상은 별도의 `context`
- `tests/ota/test_esp_ota_crypto.c:120` — signature 중첩 시험은 초기화되지 않은 `overlap.context` 사용
- 관련 반환 경로: `firmware/platform/esp32s3/ota_crypto.c:31,94–100`

실패 시나리오:

1. `overlap.context.ready`는 false로 시작한다.
2. 95행의 중첩 init은 상태를 변경하지 않고 거절된다.
3. 이후 `overlap.context`를 정상 초기화하는 호출이 없다.
4. 120행에서 signature/context 중첩 검사가 정상 동작하면 `INVALID_ARGUMENT`가 반환된다.
5. 그러나 provider의 95행에 있는 **signature 중첩 조건만 제거하는 회귀**가 생겨도, `crypto_enter()`가 unready context를 보고 동일한 `INVALID_ARGUMENT`를 반환한다.
6. 따라서 해당 시험과 뒤의 `MOCK_VERIFY == 2` 검사 모두 이 회귀를 검출하지 못한다. SDK verify까지 진행하지 않으므로 mock의 인자 검사도 도움이 되지 않는다.

Base에서는 이 시험이 정상 초기화된 `context`를 사용했다. 이번 수정은 충분한 backing storage를 제공하지만 동시에 시험의 초기화 전제도 바꾸었다.

영향:

- Signature 중첩 거절 시험의 회귀 검출력이 약화된다.
- 시험문 유지 및 동일한 line/branch coverage만으로 원래 검사 목적이 보존됐다고 볼 수 없다.
- 현재 provider에는 중첩 검사가 그대로 있으므로, 현 candidate의 인증 우회나 메모리 손상을 입증한 finding은 아니다.

권고:

- 65B backing storage는 유지하되 signature 중첩 시험에 사용할 context를 정상 init하여 `ready=true`로 만든다.
- 시험 전후 SDK verify 호출 수가 증가하지 않았는지 확인하고 context가 정상 사용 가능한 상태인지 검사한다.
- 별도 context를 초기화한다면 mock 재진입 대상과 cleanup도 그 context에 맞춘다.
- Signature 중첩 조건만 제거한 mutant를 새 시험이 검출하는지 재검증한다.

검증 방식: **소스 제어 흐름에 의한 정적 확인**. Mutant 생성·컴파일·실행은 하지 않았다.

## 4. 수정 사항 재확인

### B-PSA-01 관련

다음 변경을 직접 확인했다.

- `tests/ota/psa_crypto_fixture.h:24`: usage 값을 `0x0800U`로 수정
- `tests/ota/test_esp_ota_crypto.c:10`: 독립 literal `0x00000800`과 비교하는 C99 typedef 검사
- `tests/fixtures/idf-ota-image/main/main.c:10`: 공식 SDK macro와 같은 literal을 비교하는 `_Static_assert`

두 compile-time 검사는 mock과 기대값이 같은 잘못된 macro를 공유하는 구조를 개선한다. 다만 이번 A 실행에서는 공식 SDK header를 다시 읽거나 컴파일하지 않았다. **수정 반영은 확인했으며, B finding의 원 reviewer closure를 대신 주장하지 않는다.**

### GCC backing storage 수정

`test_esp_ota_crypto.c:89`의 union은 context의 정렬과 최소 65B storage를 제공한다. Init의 65B 입력과 verify의 64B signature가 실제 backing 범위를 갖도록 변경되었다.

- 95행과 120행의 중첩 입력 시험문은 유지됐다.
- 이번 delta에 경고 억제 옵션이나 시험 삭제는 없다.
- 작은 context를 큰 고정 배열 인자로 넘기던 직접적인 크기 문제는 소스상 개선됐다.
- 다만 signature 사례의 시험 목적 보존에는 A-PSA-POST-01이 남는다.

## 5. Provider 및 전체 delta 회귀

`ota_crypto.c:1–186`, `ota_crypto.h:1–60`을 candidate에서 다시 직접 읽었다. Base와의 diff도 비어 있음을 확인했다.

다음 기존 경계는 이번 delta에서 변하지 않았다.

- SDK 호출 전 busy 설정과 단일 task 계약
- Partial hash setup에 대한 dirty 표시
- SDK update/finish 실패 뒤 reset 요구
- Abort/destroy 실패 시 context 보존
- Close 진행 후 새 검증/hash 차단
- Cleanup 성공 때만 context 초기화
- Context와 입력·출력 중첩 및 주소 overflow 검사
- Manifest/chunk 상한과 signature/digest 고정 길이
- 신뢰된 BSP root 입력 및 설치 권한과 암호 검증의 분리

Root/component CMake, shared OTA 코드·README, workflow, golden fixture도 이번 base/candidate 사이에서 불변임을 확인했다.

변경 문서는 다음을 구분하고 있다.

- 이전 PSA candidate의 GCC CI 실패
- 수정 후 작성자 host·sanitizer 결과
- ESP-IDF fixture compile/link
- Standalone Windows PSA build 실패와 실행 `NOT_RUN`
- 새 CI 및 정상 OTA owner 미완료

별도 Windows PSA 실행 시도가 실패했다는 기록을 숨기거나 실제 암호 실행 PASS로 바꾼 delta는 발견하지 못했다.

## 6. 실제 열람 파일·명령

전체 직접 열람:

- `tests/ota/test_esp_ota_crypto.c:1–212`
- `tests/ota/psa_crypto_fixture.h:1–44`
- `firmware/platform/esp32s3/ota_crypto.c:1–186`
- `firmware/platform/esp32s3/ota_crypto.h:1–60`
- `tests/fixtures/idf-ota-image/main/main.c:1–32`
- `firmware/components/canview_esp32_ota/CMakeLists.txt`
- Candidate의 `AGENTS.md`, `docs/README.md`, `docs/resume.md`, T-007 task

부분·delta 열람:

- `CMakeLists.txt:54–67`
- `docs/journal.md:3–30` 추가분
- Resume/task 및 시험 파일 전체 delta
- `docs/runbooks/agent-workflow.md`의 격리·심각도·재검토 절

이전 실행에서 직접 읽은 OTA §7–9, ADR-009, shared OTA 계약은 이번 diff에서 불변임을 확인했다. 이번에 전체 재독했다고 표시하지 않는다.

주요 명령:

```powershell
[guid]::NewGuid().ToString()
[DateTime]::UtcNow.ToString('o')

git cat-file -e '<base>^{commit}'
git cat-file -e '<candidate>^{commit}'
git rev-parse '<base>^{commit}' '<candidate>^{commit}'

git diff --find-renames --stat <base> <candidate>
git diff --find-renames <base> <candidate> -- <명시한 검토 경로>
git show <candidate>:<위 열람 파일>

git diff --check <base> <candidate> -- . ':(exclude)docs/reviews'
```

`<base>`와 `<candidate>`는 §1의 full hash다. 행 번호는 `git show` 출력에 PowerShell `ForEach-Object`를 적용했다.

`git diff --check`는 `docs/reviews`를 제외한 범위에서 출력 없이 종료했다. Review archive의 내용·whitespace·원문 보존 정확성은 감사하지 않았다. 전체 변경 목록 10파일 중 review 기록 4파일은 목록만 확인하고 본문을 열지 않았다.

## 7. 실행 근거 귀속과 한계

다음은 **작성자 제공 결과이며 본 reviewer 실행이 아니다.**

- Debug/Release 각각 144/144
- GCC `-O3` strict PASS
- Clang ASan/UBSan 모형 149/149 lines, 114/118 branches
- SDK6.0.3 ELF/MAP/BIN 경고 0
- BIN 262144B, SHA256  
  `fadfa94c32694baf02caa5d0e268665b796691aa4e72431e7395fc8768a5f18d`
- Metadata 168변이·4절단 PASS
- Standalone Windows PSA build 실패, 실행 `NOT_RUN`

실제 로그·BIN digest·coverage artifact·새 CI 상태를 독립 검증하지 않았다. 제공된 시험 PASS는 A-PSA-POST-01의 false-pass 가능성을 반증하지 않는다.

미실행·미검토:

- 컴파일, CTest, sanitizer, mutant 실행: `NOT_RUN`
- 실제 SDK 양성 PSA 실행: `NOT_RUN`
- SDK 내부 구현·전체 config 재감사
- Raw evidence 및 통합 report 보존 감사
- 정상 OTA owner/root·영속 policy·Flash 통합
- 전체 T-007 수용 기준 검증
- Physical/HIL: `NOT_RUN`
- Vehicle TX: `NO-GO`

## 8. 최종 결론

**CONDITIONAL — 신규 P2 A-PSA-POST-01 OPEN.**

Provider는 불변이며 이번 정적 검토에서 새 production C 결함은 발견하지 못했다. 그러나 signature 중첩 시험은 backing storage 수정 과정에서 unready context를 사용하게 되어 검출력이 약화됐다. 해당 시험의 초기화 전제를 복원하고 재검증할 것을 권고한다.

이 결과는 전체 T-007, CI·target·HIL 또는 merge 승인이 아니다. **PR36 Draft / physical·HIL NOT_RUN / vehicle TX NO-GO**를 유지한다.

