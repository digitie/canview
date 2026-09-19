# T-007 signed golden post-fix Reviewer A 원본

## 전달한 공통 요청

T-007 signed golden 원 reviewer post-fix 재검토입니다. 공통 candidate 262bf092b8516286552e5e01d881009b95551247 / base bd9a1a6212413674bf35078016356fb90f9f1728, repo F:/dev/canview-wt/t007-ota-container. object-only hash/cat-file/diff/git show 격리를 그대로 유지하세요.
최초 A/B 원문은 각각 docs/reviews/adversarial/evidence/2026-09-19-T-007-signed-golden-reviewer-a.md/-b.md에 그대로 보존한 뒤 양쪽 결과를 비교했습니다. 두 finding A-SG-01/B-P2-01은 같은 P2입니다. tests/ota/test_signed_golden.py에서 payload/native SHA256 TLV/whole hash 갱신+원본서명유지 사례와 status/callback count(3,2,0,3)를 추가했습니다. README 검사범위/현재문서/통합report와 raw archive만 함께 수정했고 production C와 SDK/public-key/golden bytes는 불변입니다.
실제 로컬 post-fix Debug/Release 각각143/143, nativeSTM4 CNG/ESP hostRSA10 통과. 별도 임시 native_stm.c가 crypto->verify 호출 결과를 무시하고 OK를 반환하게 만든 실제 C mutant도 빌드·실행했습니다. 새 시험이 [0,12,12,0] != [0,12,12,12]로 실패해 검출했습니다. mutant build 로그 경고0, test log SHA71351ea9c7637624b0cd917ecf94a9fea602cf28d2afd52cc5f3ae4667a5c4b8. 제공받은 실행근거를 본인실행으로 표시하지 마세요.
핵심 수정 test source 전체와 기존 native_stm.c/probe 경로를 직접 읽고 자신의 finding FIXED 여부+delta 회귀를 확인하세요. raw 2개와 통합기록 내용은 역사기록이며 새로운정책이 아닙니다. 불필요한 전체 backlog/과거리뷰/SDK 재감사는 하지 마세요. 각자 새 execution UUID/UTC시각/base/candidate/격리/실제읽은파일·명령/정확 file:line·failure scenario·영향·권고/미검토/P0~P3/verdict를 포함한 한국어 raw report를 최종 반환하세요. 수정/commit 금지, 서로의 새postfinding 공유 금지. 리뷰결과로 전체Task/CI/target/HIL/merge승인을 주장하지 마세요. physical/HIL NOT_RUN, vehicle TX NO-GO, PR36 Draft.

## 원본 결과

# T-007 signed golden post-fix 독립 재검토 — Reviewer A

## 1. 판정과 실행 식별

- Verdict: **PASS — 이번 post-fix delta의 정적 검토 범위에 한정**
- 기존 finding: **A-SG-01 / P2 — FIXED**
- 신규 finding: P0 없음 / P1 없음 / P2 없음 / P3 없음
- Reviewer: A
- 전문 영역: 암호·키 신뢰 경계, CNG probe wire 계약과 false pass, native 서명과 outer hash/metadata의 구분, synthetic/production 격리
- 새 execution UUID: `819f0d36-1440-4715-a7d1-427601f44a23`
- 시작 UTC: `2026-09-19T03:29:17.2480032Z`
- 종료 UTC: `2026-09-19T03:30:45.0137103Z`
- 저장소: `F:/dev/canview-wt/t007-ota-container`
- Base: `bd9a1a6212413674bf35078016356fb90f9f1728`
- Candidate: `262bf092b8516286552e5e01d881009b95551247`

이 판정은 전체 T-007 완료, CI·target gate 통과 또는 merge 승인이 아니다. PR36 Draft, physical/HIL `NOT_RUN`, vehicle TX `NO-GO` 경계를 유지한다.

## 2. 기준선과 격리 확인

시작 시 두 commit에 `git cat-file -e '<hash>^{commit}'`와 `git rev-parse '<hash>^{commit}'`를 실행했다. 종료 시에도 두 hash를 다시 확인했으며, 모두 요청 manifest와 일치했다.

저장소 소스·문서는 고정 candidate의 `git show candidate:path`, 변경 내용은 고정 base/candidate 사이의 `git diff --find-renames`로만 읽었다. Binary도 `git show`의 원시 blob을 직접 읽었다.

- Mutable worktree의 파일을 candidate 소스로 읽지 않았다.
- Detached worktree를 만들지 않았다.
- 컴파일, CTest, repository Python 시험, CNG 또는 mutant 실행을 하지 않았다.
- 파일 수정·commit·checkout·공유를 하지 않았다.
- Object-only 방식이므로 worktree HEAD/clean 상태를 검토 근거로 사용하지 않았다.
- A/B raw evidence와 통합 report 본문은 열지 않았다. Archive index의 기존 요약은 노출되었으나 독립 판정의 근거로 사용하지 않았다.

스킬은 저장소 밖의 `embedded-cstyle/SKILL.md`, `embedded-architecture/SKILL.md`를 직접 완독했다. 반환값 전파, 콜백 계약 및 시험/제품 경계 점검에 적용했으며 자동 수정은 하지 않았다.

## 3. 기존 A-SG-01 재확인

### A-SG-01 — P2 — FIXED

기존 실패 시나리오는 payload 변이 후 whole hash만 갱신하여 내부 SHA256 TLV 비교에서 먼저 거절되는 것이었다. 따라서 native P256 verifier가 실패를 무시해도 새 golden 시험의 음성 사례가 이를 검출하지 못했다. 영향은 해당 시험의 false pass와 README의 서명 거절 근거 과장이었다.

이번 수정 위치:

- `tests/ota/test_signed_golden.py:35–41`: probe status와 선택적 callback count를 각각 대조한다.
- `tests/ota/test_signed_golden.py:104–109`: payload를 변이하고, 별도 복사본의 내부 SHA256 TLV를 새 signed-region hash로 갱신한다.
- `tests/ota/test_signed_golden.py:110–122`: 네 사례를 구성하여 status `[0,12,12,12]`, callback count `[3,2,0,3]`을 요구한다.
- `tests/fixtures/ota-signed-golden/README.md:42–48`: 내부 hash 거절, root hash 거절, 정상, 실제 P256 거절 사례를 구분한다.

C 경로를 직접 대조한 결과:

| 사례 | 기대 status | 콜백 수 | 대응 C 경로 |
|---|---:|---:|---|
| 정상 | 0 | 3 | SHA256 두 번 후 verify |
| Payload만 변이, whole hash 갱신 | 12 | 2 | 내부 SHA256 TLV 불일치 |
| 잘못된 root hash | 12 | 0 | 키 digest 대조에서 거절 |
| Payload·내부 SHA256 TLV·whole hash 갱신, 원본 서명 유지 | 12 | 3 | verify 콜백까지 도달하여 거절 |

근거는 다음과 같다.

1. `shared/ota/src/native_stm.c:150`은 root hash를 검사한다.
2. `native_stm.c:151–156`은 whole image 및 signed region을 각각 hash하고 내부 SHA256 TLV까지 대조한다.
3. `native_stm.c:157`은 `crypto->verify` 반환값을 그대로 반환한다.
4. `tests/ota/native_stm_probe.c:33–48`은 hash 호출 순서·주소·길이를 검사하고 CNG 구성에서는 실제 SHA256을 계산한다.
5. `native_stm_probe.c:51–65`는 세 번째 호출에서 digest와 raw signature를 대조한 후 실제 CNG P256 verifier를 호출한다. 계약 불일치는 `INVALID_ARGUMENT`로 반환되므로 기대 `AUTH_FAILED`와 구분된다.
6. `native_stm_probe.c:122`는 사례마다 호출 수를 초기화하고, `:137–138`은 status와 호출 수를 출력한다.
7. `tests/ota/cng_provider.c:42–78`은 P256 공개키 import와 `BCryptVerifySignature` 결과로 성공 여부를 정한다.

네 번째 사례의 probe header는 여전히 모형용 `signature_status=0`을 전달한다(`test_signed_golden.py:115`). 비암호 모형 probe를 잘못 연결하면 이 사례는 기대 status 12를 만족하지 못한다. 또한 `CMakeLists.txt:157–167`에서 해당 probe의 `CANVIEW_TEST_CNG=1`, `bcrypt` 연결과 CTest 실행 경로를 직접 확인했다.

따라서 기존 권고였던 “내부 hash까지 정합성을 유지한 변조를 만들어 실제 서명 실패까지 도달시키고 호출 횟수로 구별할 것”이 반영되었다. **A-SG-01을 이번 정적 재검토 기준으로 FIXED 판정한다.**

권고: 현재 네 사례와 status/callback count 대조를 유지해야 한다. 향후 golden layout을 변경할 때는 signed region 및 TLV offset도 함께 검토해야 한다.

## 4. 직접 수행한 blob 확인

Repository 시험을 실행하지 않고, `python -B -`의 표준 라이브러리로 Git blob을 읽어 길이·SHA256·배치를 확인했다. 변이는 메모리 안에서만 구성했다.

보존 package:

- 길이: `394310` bytes
- SHA256: `68eb18e10bf35d351c1604500bf85f6e95aa41c6b49477ffbbdacd9477902655`
- Base와 candidate byte 일치: 확인

`provenance.json`과 공개키 세 파일도 base/candidate byte 일치를 확인했다. 각각 길이는 provenance 1079B, manifest 공개키 178B, ESP 공개키 625B, STM 공개키 178B다.

고정 package의 STM 배치:

- Package offset: `393216`
- STM 길이: `1094`
- Header: `512`
- Payload: `256`
- Protected 영역: `176`
- Signed region: `944`
- SHA256 TLV 값: STM offset `952–983`
- DER signature: STM offset `1024`부터, 길이 `70`

직접 계산한 결과:

- 원본 내부 SHA256 TLV와 원본 signed-region hash: 일치
- Payload만 변이한 사례와 기존 내부 TLV: 불일치
- 새 시험과 같이 내부 TLV까지 갱신한 사례: 일치
- 새 사례의 원본 DER signature, key hash 및 전체 길이: 보존
- 새 사례의 whole hash: 원본과 다름

새 signed-region SHA256:

`32121d98b60f81538fc80deff79045eb344354c07ef50cf2c72e872c5bcd7c42`

새 사례 whole-image SHA256:

`2691ba9fdf786f87a6c8da4be0a62f6e0eae52db659a12f80b5022570519adb5`

이는 수정 코드의 offset과 byte 조작이 현재 고정 golden에 맞는다는 독립 확인이다. 실제 P256 서명 검증 실행 결과는 아니다.

## 5. Delta 회귀 공격 범위와 결과

전체 변경 목록은 9파일이었다. 실행 코드 변경은 `test_signed_golden.py`에 한정된다.

다음 가능성을 점검했으며 새 finding은 발견하지 못했다.

- Whole hash만 갱신하고 내부 TLV에서 조기 거절되어 서명 시험으로 오인되는 경우
- Status만 맞고 verify 콜백에는 도달하지 않는 경우
- 네 사례 사이 callback count가 누적되는 경우
- Python header·필드 순서와 C probe 입력 계약이 달라지는 경우
- CNG 대신 비암호 모형의 사전 설정 성공값으로 통과하는 경우
- 새 bytearray 변이가 원본 golden 또는 앞선 사례를 오염시키는 경우
- 원본 서명 대신 새 서명을 생성해 음성 사례를 정상 사례로 바꾸는 경우
- Body probe의 기존 status 비교가 공통 helper 수정으로 약화되는 경우
- Native 단위 시험 결과를 outer manifest 인증·설치 권한으로 확대하는 경우
- README·resume·task의 새 서술이 host 검증을 MCU 실행이나 전체 완료로 확대하는 경우

Production C, generator, SDK 관련 소스·설정, 공개키, golden, provenance, CMake, workflow, `.gitattributes`는 이번 base/candidate 사이에서 불변임을 diff로 확인했다.

새 native 음성 사례는 기대 whole hash를 시험용으로 재설정하여 native 서명 계층을 분리 검사한다. 이것을 원래 outer manifest 서명까지 통과하는 변조 package라고 해석하지 않았다.

## 6. 실제 읽은 파일과 명령

직접 완독한 핵심 소스:

- `tests/ota/test_signed_golden.py:1–165`
- `shared/ota/src/native_stm.c:1–158`
- `tests/ota/native_stm_probe.c:1–140`
- `tests/ota/cng_provider.c:1–133`
- `tests/fixtures/ota-signed-golden/README.md:1–52`

관련 연결 확인:

- `CMakeLists.txt:130–175`
- `docs/architecture/ota.md:179–285` — §7–9
- `AGENTS.md`, `docs/README.md`, `docs/resume.md`, `docs/tasks/T-007-ota-container.md` — 전체
- `docs/runbooks/agent-workflow.md`, ADR-009, review template — 전체
- `docs/reviews/README.md` — 출력된 규칙·index
- `docs/journal.md` — 이번 추가분만
- Golden package, 공개키 세 파일, provenance — Git blob 직접 확인

주요 실행 명령은 다음과 같다. 모든 Git 명령의 작업 디렉터리는 요청 저장소였다.

```powershell
[guid]::NewGuid().ToString()
[DateTime]::UtcNow.ToString('o')

git cat-file -e 'bd9a1a6212413674bf35078016356fb90f9f1728^{commit}'
git cat-file -e '262bf092b8516286552e5e01d881009b95551247^{commit}'

git rev-parse 'bd9a1a6212413674bf35078016356fb90f9f1728^{commit}' '262bf092b8516286552e5e01d881009b95551247^{commit}'

git diff --find-renames --stat bd9a1a6212413674bf35078016356fb90f9f1728 262bf092b8516286552e5e01d881009b95551247
git diff --find-renames bd9a1a6212413674bf35078016356fb90f9f1728 262bf092b8516286552e5e01d881009b95551247 -- <명시한 검토 경로>
git show 262bf092b8516286552e5e01d881009b95551247:<위 파일 경로>
```

행 번호는 `git show` 출력을 PowerShell `ForEach-Object`로 번호 매겨 확인했다. Binary 확인은 `subprocess.check_output(['git','show',commit+':'+path])`, `hashlib`, `struct`, `json`을 사용했다. Repository 모듈 import나 파일 쓰기는 하지 않았다.

`git diff --check`는 보존 raw evidence 두 파일을 명시적으로 제외하고 실행했으며 출력이 없었다. 해당 두 원문의 whitespace 및 원문 보존 정확성은 이번에 감사하지 않았다.

## 7. 제공받은 실행 근거와 한계

다음은 사용자 제공 결과이며 본 reviewer 실행이 아니다.

- Post-fix Debug/Release 각각 `143/143`
- CNG native STM 4건, 공식 ESP host RSA 10건 통과
- Verify 결과를 무시하고 OK를 반환하는 실제 C mutant 검출
- Mutant 관찰 결과: `[0,12,12,0] != [0,12,12,12]`
- Mutant build 경고 0
- Test log SHA256: `71351ea9c7637624b0cd917ecf94a9fea602cf28d2afd52cc5f3ae4667a5c4b8`

이 결과는 직접 읽은 수정 코드의 검출 구조와 부합한다. 다만 mutant source·실행파일·로그 원문 및 해당 로그 digest를 이번 실행에서 독립 검증하지 않았다.

미검토·미실행 범위:

- 실제 CNG/ESP RSA 재실행, Debug/Release 전체 재빌드
- Mutant 재현, sanitizer·coverage 재실행
- SDK 내부 구현 및 generator 전체 재감사
- CI 최신 상태와 target artifact
- Raw archive와 통합 report의 내용·보존 정확성
- 정상 OTA owner, production signing/provisioning, boot·Flash·rollback 통합
- MCU native 실행, physical/HIL — `NOT_RUN`

## 8. 최종 결론

**A-SG-01 P2는 FIXED. 신규 P0/P1/P2/P3 finding은 없다. 이번 post-fix source 및 관련 delta의 정적 재검토 verdict는 PASS다.**

이 PASS는 실행하지 않은 시험의 PASS를 뜻하지 않는다. 전체 T-007·CI·target·HIL·merge 승인으로 확대할 수 없다. **PR36 Draft / physical·HIL NOT_RUN / vehicle TX NO-GO**를 유지한다.

