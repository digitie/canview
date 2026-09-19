# T-007 PSA post-fix Reviewer B 원본

## 전달 요청 원문

```text
T-007 PSA 원 reviewer post-fix 독립 재검토. 새 executionUUID/start/end UTC 기록. A는 C수명·bounds·회귀, B는 PSA상수·SDK·검증근거 전문을 유지. Repo F:/dev/canview-wt/t007-ota-container. Base 70c7a384f3763b021546fd50f742ea51cf137dc4, candidate 6f078ace276eecce4f8ceabf464929de5eb6bfd7. git cat-file/rev-parse 시작종료 확인; object-only git show/diff로만읽기, movingworktree소스/파일수정/compile/다른agent소환 금지.
두 이전 원문은 candidate evidence에 정확히 보존됐고 통합 docs/reviews/adversarial/2026-09-19-T-007-psa.md에 disposition했다. 이번 post raw 확정전 서로결과 공유금지. B-PSA-01 P3: fixture usage0x0800 수정, host C99 typedef와 SDKfixture _Static_assert에 독립 기대값0x800 검사. 추가 작성자발견CI GCCfailure: 작은context→65/64B인자중첩시험이stringop-overread. 65Bunionbacking으로수정, 시험삭제/경고억제 없음. firmware/platform/esp32s3/ota_crypto.c/h는 불변.
실제검증(작성자): 최종Debug/Release144/144; GCC-O3 strict PASS; ClangASan/UBSan모형149/149lines114/118branches; SDK6.0.3 실제ELF/MAP/BIN 경고0, BIN262144B SHA fadfa94c32694baf02caa5d0e268665b796691aa4e72431e7395fc8768a5f18d,metadata168변이/4절단 PASS. 신규CI진행. 실제PSA standaloneWindows실행은 SDKport의존문제로buildfailed/NOT_RUN이며 journal에분리기록. 정상owner/root/Flash및wholeTask미완료.
자신의finding+전체delta회귀 직접검토. 최종raw한국어: 실행UUID/시작종료/전문/basecandidate/isolation/실제읽은파일명령/file:line finding P0~P3/실패시나리오영향권고/FIXED여부/미검토/physicalHIL NOT_RUN/vehicleTXNO-GO/BLOCK CONDITIONAL PASS. 추가결함없으면 범위정적판정으로명확히하고 전체T007승인말것. 읽지못하면incompleteBLOCK. raw최종응답 반환.
```

## 반환 원문

# T-007 PSA post-fix 독립 재검토 — Reviewer B

## 1. 실행 정보와 판정

- Execution UUID: `dbec2a2e-44e0-4ba8-9977-0783efb6985b`
- 시작 UTC: `2026-09-19T04:00:23.7096550Z`
- 종료 UTC: `2026-09-19T04:02:02.8007779Z`
- 역할: Reviewer B / Huygens
- 전문영역: PSA 상수·SDK 계약, 시험의 회귀 검출력, 검증 근거 귀속
- 저장소: `F:/dev/canview-wt/t007-ota-container`
- 실제 확인 base: `70c7a384f3763b021546fd50f742ea51cf137dc4`
- 실제 확인 candidate: `6f078ace276eecce4f8ceabf464929de5eb6bfd7`
- 기존 B-PSA-01(P3): **FIXED**
- 신규 finding: **P2 1건 OPEN**
- Verdict: **CONDITIONAL — 이번 post-fix delta의 정적 검토에 한정**

핵심 수정 소스와 기존 adapter 경로를 직접 읽었다. incomplete 상태는 아니다. compiler·시험은 요청에 따라 실행하지 않았다.

## 2. 격리와 독립성

시작·종료에 두 commit의 `cat-file -e`와 `rev-parse ...^{commit}` 결과를 확인했으며 요청 hash와 일치했다.

저장소 소스는 고정 candidate의 `git show candidate:path`와 명시적인 base→candidate `git diff`로만 읽었다.

- moving worktree 소스 사용 없음.
- 파일 수정·생성·commit·compile·다른 agent 소환 없음.
- 다른 reviewer의 새 post-fix 결과 조회·공유 없음.
- detached worktree는 만들지 않았다. 기존 checkout의 clean 상태를 주장하지 않는다.
- 이전 raw A/B 파일은 추가 경로·통계만 확인했다. 원문 byte 보존을 별도로 인증하지 않았다.
- 통합 report는 기존 finding의 disposition과 실행 이력을 확인하는 역사 기록으로 읽었다.
- `embedded-cstyle`·`embedded-documentation`을 직접 읽고 경계·시험·문서 대조에 적용했다.

이전 리뷰에서 직접 읽은 AGENTS·문서 라우터·ADR009·OTA 설계·workflow는 이번 delta에서 불변임을 확인했다.

## 3. 기존 finding 재확인

### B-PSA-01 — FIXED

수정 위치:

- `tests/ota/psa_crypto_fixture.h:23–24`
- `tests/ota/test_esp_ota_crypto.c:9–10`
- `tests/fixtures/idf-ota-image/main/main.c:10`

확인 결과:

- 모형의 `PSA_KEY_USAGE_VERIFY_MESSAGE`가 공식 값 `0x0800U`로 수정됐다.
- host 시험의 C99 typedef는 매크로를 독립 literal `UINT32_C(0x00000800)`과 비교한다. 다시 잘못된 값이 되면 음수 배열 크기로 compile이 실패하는 구조다.
- SDK fixture도 공식 헤더에서 가져온 매크로를 같은 독립 literal과 `_Static_assert`로 비교한다.
- host와 SDK 경로가 각각 다른 헤더를 사용한다는 기존 include/CMake 분리가 유지됐다.
- 제품 adapter는 불변이며 계속 공식 매크로를 사용한다.

원 권고인 상수 수정과 drift 검사가 반영됐다. **B-PSA-01을 FIXED로 판정한다.** 이는 소스상의 수정 확인이며, 해당 compile 검사를 reviewer가 실행했다는 뜻은 아니다.

## 4. 신규 findings

### P0

없음.

### P1

없음.

### P2 — B-PSA-02: union으로 옮긴 서명 중첩 시험이 미초기화 상태 검사로도 통과함

상태: `OPEN`

주 위치:

- `tests/ota/test_esp_ota_crypto.c:89`
- **`tests/ota/test_esp_ota_crypto.c:120`**
- 관련 초기화 위치 `:112–113`

대조 경로:

- `firmware/platform/esp32s3/ota_crypto.c:27–33`
- `firmware/platform/esp32s3/ota_crypto.c:94–102`

실패 시나리오:

1. 새 `overlap` union은 `:89`에서 `{0}`으로 초기화된다.
2. `:95`의 중첩 init 호출은 거절되므로 `overlap.context`는 ready 상태가 되지 않는다.
3. 이후 정상 초기화하는 대상은 `:113`의 별도 변수 `context`뿐이다.
4. 그런데 서명 중첩 시험 `:120`은 `&overlap.context`를 전달한다.
5. adapter의 **서명 중첩 검사만 제거하는 회귀**가 생겨도, `crypto_enter()`가 `ready == false`를 확인하여 `CANVIEW_INVALID_ARGUMENT`를 반환한다.
6. 시험은 같은 기대 오류를 받으므로 통과한다. PSA verify가 호출되지 않아 `calls[MOCK_VERIFY] == 2` 검사도 이를 구분하지 못한다.

기존 base의 해당 시험은 이미 초기화된 `context`를 사용했다. 이번 backing storage 수정으로 배열 크기는 충분해졌지만, 그와 함께 검증에 필요한 ready 전제조건이 사라졌다.

영향:

- 서명 입력과 context의 중첩 검사를 제거하거나 우회하는 회귀를 해당 시험이 검출하지 못한다.
- 정상 시험 PASS나 동일한 행·분기 coverage만으로 이 검출력 저하를 확인할 수 없다.
- **현재 production adapter의 중첩 검사가 제거됐다는 뜻은 아니다.** adapter는 불변이며, 결함은 새 시험 구성의 회귀다.

권고:

- 서명 중첩 사례는 **정상 초기화된 union-backed context**에서 실행한다.
- 별도 context를 초기화한다면 reentry 대상·호출 수·close 정리도 그 context에 맞게 관리한다.
- 또는 기존 정상 시험 context 자체를 union backing 안에 두어 ready 상태와 충분한 입력 storage를 동시에 유지한다.
- 서명 중첩 검사만 제거한 제한적 mutant가 이 시험에서 실패하는지 확인하면 거절 원인을 독립적으로 입증할 수 있다.

검증 수준: 직접 읽은 소스로부터의 정적 경로 분석이다. mutant를 빌드하거나 시험을 실행하지 않았다.

### P3

신규 없음. 기존 B-PSA-01은 FIXED다.

## 5. Delta 회귀와 근거 검토

### 확인한 사항

- union의 `bytes`는 65바이트이므로 해당 public-key 65바이트와 signature 64바이트 인자를 위한 backing 크기를 제공한다.
- 두 음성 호출 자체를 삭제하거나 경고 억제 옵션을 추가한 delta는 없다.
- 다만 서명 중첩 사례의 검출력은 B-PSA-02와 같이 별도 문제가 남는다.
- 기존 SDK 실패 주입·재진입·hash lifecycle 시험 나머지는 유지됐다.
- root CMake의 host 모형 정의와 SDK component의 공식 `mbedtls` 연결은 유지됐다.
- `ota_crypto.c/h`, shared OTA, CMake·workflow·SDK 설정·pin·dependency lock·signed golden은 명시적 경로 diff에서 불변이었다.

### 문서와 실행 귀속

- journal은 최초 GCC 실패와 수정 후 검증을 구분한다.
- Windows standalone PSA 시도는 build failed / 실행 NOT_RUN으로 기록돼 있다.
- 실제 ESP-IDF fixture build와 standalone host 실패를 혼합해 PASS로 표시하지 않는다.
- resume·task·통합 report는 원 reviewer 재확인과 수정본 CI·정상 owner 연결이 남았다고 명시한다.

raw A/B 두 파일을 제외한 변경분의 `git diff --check`는 직접 실행하여 PASS였다. raw 파일의 공백·원문 보존 검사는 이번 실행에서 수행하지 않았다.

## 6. 실제 읽은 파일·명령

### 직접 읽은 candidate 소스

- `tests/ota/psa_crypto_fixture.h:1–44` 전체
- `tests/ota/test_esp_ota_crypto.c:1–212` 전체
- `tests/fixtures/idf-ota-image/main/main.c:1–32` 전체
- `firmware/platform/esp32s3/ota_crypto.c:1–186` 전체
- `firmware/platform/esp32s3/ota_crypto.h:1–60` 전체
- `CMakeLists.txt:55–63`
- `firmware/components/canview_esp32_ota/CMakeLists.txt` 전체
- `tests/fixtures/idf-ota-image/main/CMakeLists.txt` 전체

문서:

- `docs/journal.md` 이번 추가분
- `docs/resume.md` 변경 diff 및 `:28–44`
- `docs/tasks/T-007-ota-container.md` 변경 diff 및 `:99–114`
- `docs/reviews/README.md` 추가 행과 diff 문맥
- `docs/reviews/adversarial/2026-09-19-T-007-psa.md:1–60` 전체 추가 diff

### 주요 실행 명령

```text
git cat-file -e '<base>^{commit}'
git cat-file -e '<candidate>^{commit}'
git rev-parse '<base>^{commit}'
git rev-parse '<candidate>^{commit}'
git diff --find-renames --stat <base> <candidate>
git diff --find-renames --name-status <base> <candidate>
git diff --find-renames <base> <candidate> -- <변경 소스·문서 경로>
git show <candidate>:<위 열거 파일>
git diff --exit-code <base> <candidate> -- <불변 확인 경로>
git diff --check <base> <candidate> -- . <raw A/B 경로 제외>
```

UUID·시각은 PowerShell `[guid]::NewGuid()`와 `[DateTime]::UtcNow`로 기록했다. `git show` 출력에 `ForEach-Object`로 행 번호를 붙였다.

이번 실행에서는 로컬 SDK 헤더를 다시 열지 않았다. 원 finding의 공식 상수 근거는 이전 리뷰에서 직접 확인한 SDK 계약을 사용했다.

## 7. 제공받은 결과와 미검토 범위

다음은 **작성자 제공 결과**이며 reviewer 실행 결과로 집계하지 않는다.

- 최종 Debug/Release 144/144
- GCC-O3 strict PASS
- Clang ASan/UBSan 모형 149/149 lines, 114/118 branches
- SDK 6.0.3 ELF/MAP/BIN warning 0
- BIN 262144B, SHA256:
  `fadfa94c32694baf02caa5d0e268665b796691aa4e72431e7395fc8768a5f18d`
- metadata 168변이·4절단 PASS
- standalone Windows PSA build failed / 실행 NOT_RUN

해당 로그·BIN을 읽거나 digest·coverage를 재계산하지 않았다.

미실행·미검토:

- 모든 compile·CTest·sanitizer·mutant 실행
- 새 CI·artifact 감사
- 실제 SDK PSA 양성·음성 암호 실행
- raw evidence의 원문 byte 보존 인증
- 정상 OTA owner·root 공급·provisioning·영속 policy·Flash 통합
- MCU 실행·boot·rollback·power-fault·physical/HIL

**physical/HIL: NOT_RUN. Vehicle TX: NO-GO.**

## 8. 최종 verdict

**CONDITIONAL**

B-PSA-01은 FIXED다. 그러나 GCC 대응으로 바뀐 서명 중첩 시험에 신규 B-PSA-02(P2)가 있어, 이번 delta를 무조건 PASS로 닫지 않는다.

정상 초기화된 충분한 backing storage로 중첩 거절 시험을 복구하고 관련 검증을 재실행해야 한다. 이 결과는 전체 T-007, 정상 OTA·root·Flash, CI·target 또는 merge 승인이 아니다.

