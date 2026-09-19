# T-007 signed golden Reviewer A 원본

## 전달한 공통 요청

T-007 합성 native signed golden 추가분의 독립 적대적 리뷰를 요청합니다. 한국어 raw report를 최종 응답으로 반환하세요. 수정/commit은 하지 마세요. 다른 reviewer finding을 읽지 마세요.
공통 manifest:
repo F:/dev/canview-wt/t007-ota-container
candidate bd9a1a6212413674bf35078016356fb90f9f1728
base dfe3d8a292442a565cfb9029c2c40860a74b5dff
isolation commit object-only: cat-file -e/hash 확인 후 git diff base candidate / git show candidate:path로만 source를 읽으세요. binary는 Git blob에서 hash/length를 확인할 수 있습니다. mutable working tree를 candidate로 읽지 마세요. compiler/test 실행이 필요하면 별도 detached worktree를 만들고 처음/끝 HEAD 및 clean 상태를 확인하세요.
scope: tests/ota/generate_signed_golden.py, test_signed_golden.py, tests/fixtures/ota-signed-golden/*, .gitattributes, CMakeLists.txt, .github/workflows/foundation.yml, docs/resume.md, docs/tasks/T-007-ota-container.md, docs/journal.md 추가분.
contract: AGENTS.md→docs/README.md→resume→T007, ADR009/OTA§7–9 필요 부분, agent-workflow, review archive/template만 필요한 만큼. 테스트 전용 synthetic fixture이고 제품 signing CLI나 설치 owner 구현이 아닙니다. 실제 ESP-IDF SDK BIN(624848a fixture, unsigned SHA e9dc9177c696a13bdba0631c1da2b3ec125b688862ab2c04921c05c0675d3735)에 espsecure5.4.0 RSA3072/PSS 서명, 공식 imgtool v2.4.0 STM synthetic/P256, 별도 outer P256. private key는 memory-only. 보존 package SHA68eb18e10bf35d351c1604500bf85f6e95aa41c6b49477ffbbdacd9477902655 394310B.
로컬 결과(본인이 실행한 것으로 표시 금지): Debug/Release 각각143/143; CNG C prefix/body12, CNG nativeSTM3, official imgtool 정상 verify, official ESP host RSA10 cases; staged secret/VIN/private-key pattern0; generator/link/plan pass. 새 CI 진행 예정. MCU native execution, Flash, HIL NOT_RUN / vehicle TX NO-GO. 전체 T007 미완료, PR36 Draft 유지; PR33 waiver 불적용.
필수 raw: 새 reviewer execution UUID, 시작/종료UTC시각, 전문영역, base/candidate, 격리확인, 실제 읽은 파일/명령, P0/P1/P2/P3별 finding(없으면 없음), 정확 file:line/failure scenario/영향/권고, 미검토범위, physical/HIL NOT_RUN, BLOCK/CONDITIONAL/PASS verdict와 정적검토 scope한계. 핵심 source 미열람이면 incomplete BLOCK이지 PASS가 아닙니다.
A 전문: 암호/키 trust boundary, 실제 CNG probe wire계약/false pass, native signature vs outer hash/metadata, synthetic/production isolation.
B 전문: SDK/version/provenance, reproducibility, generated artifact drift/line endings, CI/dependency/lifecycle, 실패·증거 과장.
두 리뷰어에게 전달하는 공통 요청은 동일하며 자신의 기존 역할(A/Maxwell 또는 B/Huygens)에 맞춰 독립 검토하세요.

## 원본 결과

# T-007 합성 native signed golden 독립 적대적 리뷰 — Reviewer A

판정: **CONDITIONAL**. 신규 P2 finding 1건을 확인했다. 보존 golden의 STM 음성 시험이 실제 P256 서명 거절 경로를 검증한다는 설명과 달리, 두 음성 사례 모두 서명 검증 전에 거절된다.

Production verifier의 신규 취약점을 발견했다는 뜻은 아니다. 전체 T-007 완료·merge·배포 승인은 부여하지 않는다.

## 실행 식별·기준선·격리

- Execution UUID: `0178e029-d7a6-4d76-a353-33cf34961c4f`
- 발급 명령: `[guid]::NewGuid().ToString()`
- 시작 UTC: `2026-09-19T03:13:05.472Z`
- 종료 UTC: `2026-09-19T03:16:50.375Z`
- 역할: Reviewer A
- 전문영역: 암호·키 신뢰 경계, CNG probe wire 계약과 false PASS, native 서명과 outer hash/metadata의 구분, synthetic/production 격리.
- 저장소: `F:/dev/canview-wt/t007-ota-container`
- 실제 확인 base: `dfe3d8a292442a565cfb9029c2c40860a74b5dff`
- 실제 확인 candidate: `bd9a1a6212413674bf35078016356fb90f9f1728`

시작에 두 commit 객체의 존재와 해시를 확인하고 종료에 해시를 재확인했다. 소스는 고정 commit의 `git show`·`git diff --find-renames`로만 읽었다. Binary도 `git show candidate:path`의 원시 stdout으로 읽었다.

Mutable worktree·HEAD·로컬 빌드 결과를 candidate의 내용으로 사용하지 않았다. Detached worktree는 만들지 않았다. 파일 수정·commit·peer 탐색·공유를 하지 않았고, 다른 reviewer report/finding을 열람하지 않았다. Worktree clean 상태는 검사하지 않았으며 clean이라고 주장하지 않는다.

## Finding

### A-SG-01 — P2 — STM golden 음성 시험이 P256 거절 경로에 도달하지 않아 서명 검증 누락을 놓친다

- 상태: `OPEN`
- 주 위치: `tests/ota/test_signed_golden.py:101`
- 관련 위치:
  - `tests/ota/test_signed_golden.py:104`
  - `tests/ota/test_signed_golden.py:35`
  - `tests/fixtures/ota-signed-golden/README.md:44`
- 경로 근거:
  - `shared/ota/src/native_stm.c:150`
  - `shared/ota/src/native_stm.c:156`
  - `shared/ota/src/native_stm.c:157`
  - `tests/ota/native_stm_probe.c:51`

**실패 시나리오**

새 STM 시험은 정상 이미지, payload byte 512 변이, 잘못된 root hash의 세 사례를 만든다.

Payload 변이에서는 외부 whole-image 기대 digest와 probe에 전달하는 signed digest를 다시 계산하지만, 이미지 내부의 SHA-256 TLV는 갱신하지 않는다. 따라서 C verifier는 `native_stm.c:156`의 내부 digest 비교에서 `AUTH_FAILED`를 반환하며 `:157`의 P256 verify 호출에 도달하지 않는다.

잘못된 root hash 사례도 `native_stm.c:150`에서 거절된다. 결과적으로 새 golden 시험에는 **P256 verifier까지 도달한 뒤 서명 때문에 실패해야 하는 음성 사례가 없다.**

`run_probe()`는 출력의 첫 번째 status 열만 비교하며 native probe가 출력하는 호출 횟수를 버린다. 가령 native 서명 검증이 항상 성공하도록 잘못 바뀌어도, 정상 사례는 성공하고 두 음성 사례는 앞선 검사에서 거절되므로 이 새 golden 시험의 기대 status는 그대로 일치한다.

**Git blob으로 직접 확인한 근거**

보존 package에서 STM 시작 offset은 393,216, STM 길이는 1,094바이트였다. Header 512 + payload 256 + protected TLV 176으로 signed 영역은 944바이트다.

- 내부 SHA-256 TLV와 원본 signed 영역 digest:

  `b9603106153c6cfea578481f959fdb30cb6170a3130cce443a6a26d1a6fc67d5`

- 시험과 동일하게 payload byte 512를 XOR 1한 signed 영역 digest:

  `32121d98b60f81538fc80deff79045eb344354c07ef50cf2c72e872c5bcd7c42`

- 변이 후 이미지 내부 SHA-256 TLV는 원본 값 그대로임을 확인했다.

이는 Git blob의 메모리상 변이·digest 대조와 C 경로 정적 추적으로 확인한 결과다. CNG 실행 결과를 관찰한 것으로 표시하지 않는다.

**영향**

`test_signed_golden.py:104`의 “실제 P256 서명으로 거부”라는 설명과 README의 해당 보장이 현재 시험으로 입증되지 않는다. 보존 golden의 native 서명 회귀 검출력이 설명보다 약하다.

다만 기존 `tests/ota/test_native_stm.py:95`에는 잘못된 실제 검증 공개키 사례가 있다. 따라서 전체 기존 native 시험에 서명 음성 검증이 전혀 없다고 주장하지 않는다. 이번 finding은 새 golden 시험과 그 설명에 한정한다.

**권고**

보존 STM 이미지로 다음 사례를 추가한다.

1. Payload를 변이한다.
2. 변이된 signed 영역 digest로 이미지 내부 SHA-256 TLV를 갱신한다.
3. 갱신된 전체 이미지의 기대 digest도 다시 계산한다.
4. 원래 P256 서명은 유지한다.
5. P256 verify 호출까지 도달한 뒤 `AUTH_FAILED`가 나오는지 확인한다.

현재 probe의 호출 횟수도 함께 검사하면 정상·이 사례의 verify 도달을 확인할 수 있다. 또는 실제 검증 공개키만 바꾸고 앞선 hash 검사는 통과시키는 사례를 추가한다. 시험에 맞춰 README·주석의 검증 범위도 정정한다.

## 나머지 공격 범위와 확인 결과

- **키 분리·저장 경계:** `generate_signed_golden.py:74`에서 ESP RSA3072, STM P256, outer manifest P256 키를 각각 생성한다. 출력 파일 집합은 package·공개키·provenance이며 코드에 private-key serialization 경로는 없다. SDK 내부 동작·OS 수준 메모리 잔존까지 검증한 것은 아니다.
- **Outer/native 분리:** `generate_signed_golden.py:104`는 native image를 outer container에 넣고 별도 manifest 키를 사용한다. `test_signed_golden.py:49`의 ESP 음성 시험은 outer hash 검사에 기대지 않고 `espsecure`를 직접 호출한다.
- **CNG body wire:** `test_signed_golden.py:81`의 `<7I>`·`<8IQ>`·64바이트 공개키·prefix/body 순서는 `body_probe.c:241` 이후의 읽기 계약과 일치한다. Scenario 10은 기존 collector 경로다.
- **CNG STM wire:** `<8IQ2I>` header와 공개키·root hash·digest·raw signature·고정 문자열의 순서는 `native_stm_probe.c:111` 이후와 일치한다. 보존 blob의 `signed_size=944`와 DER 시작 `944+80`도 새 시험의 값과 일치한다.
- **실제 CNG 연결:** `CMakeLists.txt:148`, `:158`은 두 probe에 `CANVIEW_TEST_CNG=1`을 지정하고 `bcrypt`를 링크한다. Provider의 실제 SHA-256/P256 호출도 직접 읽었다. 신규 CTest가 비암호 body probe를 가리키지는 않는다.
- **합성·제품 경계:** fixture identity는 synthetic board/layout, epoch 7, key ID 11, u64 최대값이다. Production source·portable verifier·CNG provider 자체는 이번 delta에서 변경되지 않았다. 정상 owner·설치·차량 TX 권한을 추가하지 않는다.
- **재현 주장:** fixture README는 랜덤 키·salt·서명 재생성을 byte 재현으로 주장하지 않고, 보존 서명/image를 이용한 정확한 재조립으로 범위를 한정한다.
- **CI 실패 전파:** 새 ESP 검사는 기존 `Invoke-TargetCommand` 안에서 실행되며 nonzero exit는 실패로 처리한다. Target job 안의 host 암호 실행을 MCU 실행과 구분한다.

## Git blob 직접 검사 결과

다음 네 파일의 길이와 SHA-256을 Git 객체에서 직접 계산하여 `provenance.json`과 일치함을 확인했다.

| 파일 | 실제 길이 | 결과 |
|---|---:|---|
| `communicator.cvota` | 394,310B | provenance 일치 |
| `manifest-public.pem` | 178B | provenance 일치 |
| `esp-public.pem` | 625B | provenance 일치 |
| `stm-public.pem` | 178B | provenance 일치 |

Package SHA-256:

`68eb18e10bf35d351c1604500bf85f6e95aa41c6b49477ffbbdacd9477902655`

Package header는 `CVOTA002`, version 2, header 24B, manifest 262B, signature 64B, image count 2, total 394,310B였다. 세 PEM의 armor는 모두 `BEGIN PUBLIC KEY`였다.

이 검사는 서명 검증 실행을 대체하지 않는다. Unsigned ESP 입력 SHA의 원본 SDK 빌드 귀속도 독립 재현하지 않았다.

## 실제 읽은 파일·명령

이번에 전체를 직접 읽은 핵심 파일:

- `tests/ota/generate_signed_golden.py:1` — 126행 전체
- `tests/ota/test_signed_golden.py:1` — 158행 전체
- `tests/fixtures/ota-signed-golden/README.md:1` — 50행 전체
- `tests/fixtures/ota-signed-golden/provenance.json`
- `tests/ota/native_stm_probe.c:1` — 140행 전체
- `tests/ota/cng_provider.c:1` — 133행 전체
- `shared/ota/src/native_stm.c:1` — 158행 전체
- `tests/ota/check_sdk_metadata.py:1` — 58행 전체
- `.gitattributes`, `tools/toolchain-versions.json`
- `AGENTS.md`, `docs/README.md`, `docs/resume.md`
- `docs/tasks/T-007-ota-container.md`
- `docs/runbooks/agent-workflow.md`
- `docs/adr/009-ota-native-image-alignment.md`

관련 구간·delta를 읽은 파일:

- `tests/ota/body_probe.c`: 129–142, 196–338, 408–446행
- `tests/ota/test_native_stm.py`: 1–170행
- `tests/ota/test_body.py`, `test_manifest.py`: 각각 1–90행
- `CMakeLists.txt`: 131–175행 및 delta
- `.github/workflows/foundation.yml`: 242–255, 300–324행 및 delta
- `docs/journal.md`: 이번 추가분만
- Golden package·PEM 세 파일: Git blob 전체 bytes, hash/length 및 필요한 binary 필드

앞선 실행에서 직접 읽은 OTA §7–9와 공통 container/envelope/body 경로는 이전 검토 candidate 대비 diff가 없음을 확인하여 재사용했다. 기존에 읽은 embedded C·architecture skill의 소유권·권한 경계 점검 기준을 적용했다. Peer report는 열지 않았다.

실제 명령 계열은 다음과 같다. `<base>`·`<candidate>`는 위에 기록한 전체 해시다.

```powershell
[guid]::NewGuid().ToString()
[DateTime]::UtcNow.ToString('yyyy-MM-ddTHH:mm:ss.fffZ')

git cat-file -e '<base>^{commit}'
git cat-file -e '<candidate>^{commit}'
git rev-parse '<base>^{commit}'
git rev-parse '<candidate>^{commit}'

git diff --find-renames --stat <base> <candidate>
git diff --find-renames --name-status <base> <candidate>
git diff --find-renames <base> <candidate> -- <검토 경로>
git show <candidate>:<검토 경로>
git diff --find-renames --check <base> <candidate>
```

행 번호는 PowerShell로 부여했다. Binary 검사는 `python -B -`의 표준 라이브러리 스크립트에서 `subprocess.check_output(['git','show',…])`, `hashlib.sha256`, `struct.unpack_from`을 사용했다. Repository 모듈은 import/실행하지 않았고 파일도 쓰지 않았다.

`git diff --check`는 진단 출력이 없었다.

## 실행 검증·미검토 범위

이번 reviewer는 generator·`test_signed_golden.py`·CNG probe·imgtool·espsecure·compiler·CTest를 실행하지 않았다. 실행한 것은 Git 객체 검사, binary hash/필드 검사 및 메모리상 digest 대조다.

사용자가 제공한 Debug/Release 143/143, CNG body 12건, native STM 3건, imgtool 정상 verify, ESP RSA 10건, secret/VIN 검사 및 generator/link/plan 결과는 coordinator evidence이며 이번 reviewer의 실행 PASS로 집계하지 않는다.

미검토·미실행:

- SDK 원문·설치본·의존성의 독립 provenance 및 재현 빌드
- 실제 ESP RSA·STM P256·outer P256 실행 검증
- 서명 무력화 mutant를 빌드한 실행 재현
- 전체 CI·ELF/MAP/BIN·로그 재감사
- 제품 signing CLI·정상 OTA owner·영속 policy·target 통합
- MCU native execution, Flash·전원 차단·rollback·실차 동작

**Physical/HIL: NOT_RUN. Vehicle TX: NO-GO.**  
PR33 waiver를 이번 검토에 적용하지 않았다.

## 최종 집계·판정

- P0: 없음
- P1: 없음
- P2: **A-SG-01 1건, OPEN**
- P3: 없음

**Reviewer A verdict: CONDITIONAL.** 핵심 source는 직접 열람했다. 새 golden의 STM 서명 음성 검증 공백과 설명을 수정하거나 정식 disposition한 뒤 재확인이 필요하다. 이 판정은 정적 검토 범위에 한정하며 전체 T-007 완료·PR36 merge·배포 승인이 아니다.

