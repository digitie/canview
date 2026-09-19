# T-007 signed golden Reviewer B 원본

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

# T-007 합성 native signed golden 독립 적대적 리뷰 — Reviewer B

## 1. 실행 정보와 판정

- Execution UUID: `ce8022d5-1f2d-4b3e-a89a-87c237145fa4`
- 시작 UTC: `2026-09-19T03:13:09.3076895Z`
- 종료 UTC: `2026-09-19T03:22:01.0168591Z`
- 역할: Reviewer B / SDK·버전·provenance, 재현성, 생성물 drift·줄바꿈, CI·의존성·수명주기, 검증 누락·근거 과장
- 저장소: `F:/dev/canview-wt/t007-ota-container`
- 실제 확인 base: `dfe3d8a292442a565cfb9029c2c40860a74b5dff`
- 실제 확인 candidate: `bd9a1a6212413674bf35078016356fb90f9f1728`
- 판정: **CONDITIONAL — 이번 합성 golden 추가분의 정적 검토에 한정**
- Findings: P0 없음 / P1 없음 / **P2 1건 OPEN** / P3 없음
- 전체 T-007 완료·제품 설치 승인·merge 승인이 아니다. PR36 Draft 및 PR33 waiver 불적용 전제를 유지한다.

핵심 생성기와 시험 소스는 직접 전부 읽었다. 핵심 소스 미열람에 따른 incomplete 상태는 아니다. 다만 compiler·CNG probe·공식 SDK 시험은 이번 실행에서 수행하지 않았다.

## 2. 격리와 독립성

시작과 종료에 두 commit의 존재와 해석 결과를 확인했다. 모든 저장소 소스는 고정 candidate의 `git show candidate:path` 및 두 hash 사이의 `git diff`로 읽었다.

- mutable working tree의 소스나 이동 가능한 HEAD를 candidate로 사용하지 않았다.
- binary와 공개키는 `git show`의 raw stdout stream에서 메모리로 읽고 길이·SHA256을 계산했다. 텍스트 변환을 거치지 않았다.
- detached worktree는 만들지 않았다. 따라서 detached HEAD/clean 검사는 해당 없음이며, 기존 checkout이 clean하다고 주장하지 않는다.
- 파일 수정·생성·commit·SDK 설치·보고서 저장·peer 조회·공유를 하지 않았다.
- 다른 reviewer의 원본 또는 통합 finding 보고서는 열지 않았다. 계약상 review archive README와 template은 확인했으며, archive의 과거 상태를 이번 finding의 근거로 사용하지 않았다.
- `embedded-cstyle`, `embedded-architecture`, `embedded-documentation`을 직접 읽고 C 검증 경계와 문서 근거 대조에 적용했다. 자동 수정 지침은 사용자 수정 금지 요청에 따라 수행하지 않았다.

## 3. Findings

### P0

없음.

### P1

없음.

### P2 — B-P2-01: STM golden 음성 사례가 P256 검증 전에 종료되어 실제 서명 거절 회귀를 검출하지 못함

상태: `OPEN`

주 위치:

- `tests/ota/test_signed_golden.py:101–115`
- 특히 payload 변이 `:102`, 기대 상태 `:103`, “실제 P256 서명으로 거부” 주장 `:104`
- probe 출력의 상태 코드만 확인하는 `tests/ota/test_signed_golden.py:35–38`
- 같은 검증 범위를 서술하는 `tests/fixtures/ota-signed-golden/README.md:42–44`

직접 대조한 기존 코드:

- `shared/ota/src/native_stm.c:150–157`
- `tests/ota/native_stm_probe.c:33–65`, `:137–138`

실패 경로:

1. 새 시험은 STM payload의 `blob[512]`를 반전한다.
2. 변경된 전체 이미지의 SHA256과 probe에 전달할 signed-range SHA256은 다시 계산한다.
3. 그러나 이미지 내부의 SHA256 TLV, 즉 `signed_size + 8`부터의 32바이트는 갱신하지 않는다.
4. C verifier는 전체 이미지 hash 확인 후 signed-range hash를 계산하고, `native_stm.c:156`에서 기존 SHA256 TLV와 비교한다.
5. 여기서 `CANVIEW_AUTH_FAILED`로 반환하므로 다음 줄의 `crypto->verify()` 및 CNG P256 검증에는 도달하지 않는다.
6. 다른 음성 사례인 잘못된 root hash도 `native_stm.c:150`에서 암호 callback 호출 전에 종료한다.

따라서 현재 STM 3사례는 정상 서명 수용, native hash 불일치 거절, root hash 불일치 거절을 확인하지만, **잘못된 P256 서명의 실제 거절은 확인하지 않는다.**

`native_stm_probe.c:138`은 상태와 callback 호출 수를 모두 출력하지만, 새 시험의 `run_probe()`는 첫 번째 숫자만 읽는다. 정적 경로상 현재 세 사례의 callback 호출 수는 각각 3·2·0이며, 음성 사례에서 verify까지 도달하지 않은 사실을 시험이 확인하지 않는다.

Failure scenario:

STM native verify callback이 잘못된 서명에도 성공을 반환하도록 회귀하더라도, 이번 golden의 STM 3사례만으로는 이를 검출하지 못한다. 정상 사례는 계속 성공하고 두 음성 사례는 verify 이전 검사에서 계속 거절되기 때문이다. 이는 해당 3사례에 대한 분석이며, 저장소 전체의 다른 시험까지 통과한다는 주장은 아니다.

영향:

- 새로운 공식 imgtool golden이 CNG native P256의 서명 거절까지 교차 검증한다는 근거가 부족하다.
- `test_signed_golden.py:104`와 fixture README `:44`의 설명이 실제 검사 경로보다 강하다.
- 현재 제품 verifier의 서명 우회 취약점을 발견한 것은 아니다. 이번 delta의 시험·검증 근거 결함이다.

권고:

- 기존 hash 변이 사례는 hash 거절 시험으로 유지한다.
- 별도로 payload 변이 후 native SHA256 TLV까지 갱신하고 원래 서명을 유지하거나, 구조적으로 유효한 잘못된 서명을 넣어 **앞선 hash/root 검사를 통과한 뒤 CNG P256에서 거절되는 사례**를 추가한다.
- whole-image digest와 probe 입력도 해당 이미지에 맞게 갱신한다.
- STM probe에서는 상태뿐 아니라 callback 호출 수도 검사한다. 서명 거절 사례는 verify까지 도달한 호출 수 3을 요구한다.
- README와 시험 설명을 실제 검사별로 구분하고, 수정된 candidate에서 Debug/Release의 해당 시험을 재실행한다.

검증 수준: 소스 경로를 직접 추적한 정적 finding이다. compiler 실행이나 verifier 변이 실험으로 재현한 것으로 표시하지 않는다.

### P3

없음.

## 4. 직접 확인한 artifact와 검증 결과

### Git blob 무결성

다음 네 파일은 Git blob에서 직접 계산한 길이와 SHA256이 provenance와 모두 일치했다.

| 파일 | 길이 | SHA256 |
|---|---:|---|
| `communicator.cvota` | 394310B | `68eb18e10bf35d351c1604500bf85f6e95aa41c6b49477ffbbdacd9477902655` |
| `manifest-public.pem` | 178B | `01a7ff058c7809196aab45447570c04fb943e7948696672addd4a597df84dfcb` |
| `esp-public.pem` | 625B | `bc07b4a88abd41a93f8ac2c9171749e2160a64ccbb7ca697df9be8801fd2c89c` |
| `stm-public.pem` | 178B | `c28064dbffdb0a027af839066fbb630f0c9635ef6c9acf0a250added91bcea0e` |

세 PEM은 `PUBLIC KEY` 형식이며 raw blob의 CR 바이트 수는 모두 0이었다.

추가 blob 계산:

- package offset `65536`의 `262144B` 구간 SHA256:
  `e9dc9177c696a13bdba0631c1da2b3ec125b688862ab2c04921c05c0675d3735`
- 요청된 unsigned ESP BIN digest와 일치했다.
- package offset `393216`의 STM 구간은 `1094B`, 시작 magic은 `3DB8F396`이었다.
- STM 최초 `944B`의 계산 SHA256과 저장된 native SHA256 TLV는 모두 다음 값이었다:
  `b9603106153c6cfea578481f959fdb30cb6170a3130cce443a6a26d1a6fc67d5`

이는 보존 바이트의 대조 결과다. ESP-IDF rebuild나 native 서명 검증을 이번 reviewer가 수행했다는 뜻은 아니다.

### 정적 확인 사항

- provenance의 ESP-IDF v6.0.3 commit은 `76f5dedd9950a3012fee8fb7d5586df21fc67802`, MCUboot v2.4.0 commit은 `6d3b3d2c38ab20c242e5b9abb04d050086383eb2`이며 저장소 SDK pin과 일치했다.
- 생성기는 esptool `5.4.0`, SDK HEAD와 clean 상태를 검사한다.
- 무작위 키·서명의 재생성 결과가 동일하다고 주장하지 않는다. 보존 image·서명을 이용한 정확한 컨테이너 재조립을 재현 gate로 정의했다.
- `.cvota`는 binary, PEM·JSON은 LF 속성을 지정했다.
- Windows CTest 등록은 CNG 정의와 `bcrypt` 연결이 있는 probe target을 전달한다.
- target CI의 `--esp-sdk` 실행은 SDK fixture build 다음에 배치되어 있다. 실패 exit code는 기존 wrapper에서 전파된다.
- host 경로의 MCUboot import와 target의 ESP 검증 경로를 구분했다. target job에 추가된 검사는 장치 실행이 아닌 host 암호 실행이라고 명시한다.
- 생성기에서 개인키 직렬화·파일 저장 경로는 발견하지 않았다. 기존 output directory를 거절하고 새 파일 생성 모드를 사용한다. OS swap·crash dump까지 포함한 비밀 제거를 검증한 것은 아니다.
- `git diff --check`는 직접 실행하여 PASS였다.
- production OTA 코드, 기존 probes, SDK fixture, toolchain pin·OTA lock·SDK setup 경로는 이번 base→candidate에서 변경되지 않았음을 직접 확인했다.

## 5. 실제 읽은 파일과 명령

모든 저장소 위치·행 번호는 candidate object 기준이다.

### 핵심 변경 소스·artifact

- `tests/ota/generate_signed_golden.py:1–126` 전체
- `tests/ota/test_signed_golden.py:1–158` 전체
- `tests/fixtures/ota-signed-golden/README.md:1–50` 전체
- `tests/fixtures/ota-signed-golden/provenance.json:1–36` 전체
- `tests/fixtures/ota-signed-golden/esp-public.pem` 전체
- `tests/fixtures/ota-signed-golden/manifest-public.pem` 전체
- `tests/fixtures/ota-signed-golden/stm-public.pem` 전체
- `tests/fixtures/ota-signed-golden/communicator.cvota` raw blob 전체
- `.gitattributes` 전체, 특히 `:23–26`
- `CMakeLists.txt:137–169`
- `.github/workflows/foundation.yml`의 host 설정 `:1–76`, target 환경 `:149–215`, 명령 wrapper `:242–254`, SDK/golden 실행 `:304–311`

### 계약·문서

- `AGENTS.md` 전체
- `docs/README.md` 전체
- `docs/resume.md:1–162` 전체
- `docs/tasks/T-007-ota-container.md` 전체
- `docs/journal.md` 이번 추가분 `:3–21`
- `docs/adr/009-ota-native-image-alignment.md` 전체
- `docs/architecture/ota.md:179–284`, §7–9
- `docs/runbooks/agent-workflow.md` 전체
- `docs/reviews/README.md`, `docs/reviews/adversarial/TEMPLATE.md`

### 필요한 기존 의존 경로

- `tests/ota/native_stm_probe.c:1–140`
- `tests/ota/cng_provider.c:1–133`
- `shared/ota/src/native_stm.c:1–158`
- `tests/ota/body_probe.c:1–336`
- `tests/ota/check_sdk_metadata.py:1–58`
- `tests/ota/test_manifest.py:1–27`
- `tests/ota/test_body.py:1–20`
- `tools/ota/container.py:1–126`
- `tools/ota/manifest.py:1–131`
- `tools/ota/envelope.py:1–87`
- `tools/requirements-ota.lock` 전체
- `tools/toolchain-versions.json` SDK pin 절
- `tools/environment/setup-windows.ps1` SDK root·설치·export 관련 구간

### 실행 명령

저장소 작업 디렉터리는 모두 지정 repo였다. 주요 Git 명령은 다음과 같다.

```text
git cat-file -e 'dfe3d8a292442a565cfb9029c2c40860a74b5dff^{commit}'
git cat-file -e 'bd9a1a6212413674bf35078016356fb90f9f1728^{commit}'
git rev-parse 'dfe3d8a292442a565cfb9029c2c40860a74b5dff^{commit}'
git rev-parse 'bd9a1a6212413674bf35078016356fb90f9f1728^{commit}'
git diff --find-renames dfe3d8a292442a565cfb9029c2c40860a74b5dff bd9a1a6212413674bf35078016356fb90f9f1728
git diff --name-status --find-renames dfe3d8a292442a565cfb9029c2c40860a74b5dff bd9a1a6212413674bf35078016356fb90f9f1728
git diff --check dfe3d8a292442a565cfb9029c2c40860a74b5dff bd9a1a6212413674bf35078016356fb90f9f1728
git show bd9a1a6212413674bf35078016356fb90f9f1728:<위 열거 경로>
```

필요한 파일은 `ForEach-Object`로 행 번호를 붙이거나 필요한 구간을 선택했다. 종료 시 `git diff --exit-code`에 production·probe·SDK·pin·lock·setup 경로를 명시하여 의존 경로 delta가 비어 있음을 확인했다.

UUID·UTC는 PowerShell의 GUID/UTC API로 생성·기록했다. binary 감사는 `System.Diagnostics.Process`로 `git show`를 실행하고 `StandardOutput.BaseStream`→`MemoryStream`→`.NET SHA256.HashData` 순서로 수행했다. 파일로 추출하거나 저장소 코드를 실행하지 않았다.

## 6. 공격 범위와 미검토 범위

검토한 공격 관점:

- package·공개키·provenance 사이의 길이/hash 불일치
- Windows 줄바꿈 변환에 따른 보존 artifact drift
- SDK pin·도구 버전과 생성 근거의 불일치
- 무작위 재생성을 byte 재현으로 오인하는 주장
- output 덮어쓰기, 생성 실패를 성공으로 표시하는 경로
- CI의 Python 환경·의존성·시험 등록·실패 전파
- Python/C probe wire 계약과 mock/CNG 구분
- native hash 거절을 실제 서명 거절로 오인하는 시험
- host 암호 검증을 MCU 실행·설치 승인으로 확대하는 문서 주장

이번에 수행하지 않은 검증:

- Debug/Release configure·build·CTest
- CNG prefix/body 12사례 및 native STM 3사례 실행
- 공식 imgtool verify와 espsecure RSA 10사례 실행
- generator 재실행 및 ESP-IDF rebuild
- upstream SDK 구현·배포 package·전이 의존성 전체 감사
- 새 CI run·artifact·로그 조회
- generator/link/plan 및 secret/VIN pattern scanner 실행
- 전체 portable parser·정상 OTA owner·영속 policy·제품 signing CLI 감사
- MCU native execution, Flash, boot·rollback·power-fault 검증

사용자가 제시한 Debug/Release 143/143, CNG·공식 SDK 결과, generator/link/plan 및 secret scan 결과는 **제공받은 로컬 근거**이며 이번 reviewer의 실행 결과로 계산하지 않았다.

**physical/HIL: NOT_RUN. Vehicle TX: NO-GO.**

## 7. 최종 verdict

**CONDITIONAL**

보존 artifact의 길이·SHA256과 요청된 unsigned ESP digest는 직접 확인했고, 핵심 source·CI 연결·synthetic 경계의 정적 검토를 완료했다.

다만 B-P2-01은 OPEN이다. 이번 STM golden 시험에는 실제 P256 서명 거절 경로를 검증하는 음성 사례가 필요하다. 수정 및 해당 실행 근거 확인, 또는 workflow에 따른 명시적 disposition 전에는 이 checkpoint를 무조건 PASS로 닫지 않는다.

이 판정은 이번 합성 signed golden 추가분에만 해당한다. 전체 T-007 완료, 정상 target 연결, 제품 signing·설치·배포 또는 merge 승인을 의미하지 않는다.

