# T-007 SDK metadata Reviewer A 원본 evidence

- Subagent ID: `01a0b784-5d6a-7540-97e8-ac159eb3a828`

## 전달 요청 원문

```text
T-007 다음 별도checkpoint 독립 리뷰. 직전prefix·packager post-fix 원문수신및보존완료. 이번은 새범위이므로 별도새execution ID/시각/완전raw보고. 동일저장소 object-only, base ec44647e4674cc382d655bb668e07254b2612fca -> candidate 624848a784f255c6be770ce1ae37fa824e56acba. gitshow/diff/hash로격리. scope8파일: tests/fixtures/idf-ota-image/main/metadata.c 합성 const C native descriptor, main/CMakeLists linker-u, fixture/CMakeLists PROJECT_VER, tests/ota/check_sdk_metadata.py 위치/값변이검사, .github/workflows/foundation.yml targetSDKbuild직후검사등록, fixture README/resume/T007상태. 실제 ESP-IDF6.0.3 build ELF/MAP/BIN경고0, BINsha e9dc9177c696a13bdba0631c1da2b3ec125b688862ab2c04921c05c0675d3735, offset288metadata168B+u64max일치, 전168바이트변이/4절단거절 로컬확인. 모든값synthetic-only role1target1epoch7abi2u64max board/layoutsynthetic, product/provisioning/native서명/boot/physicalHIL 아님. production/SDKadapter/portableparser코드불변. purpose: native signedgolden용실제SDK합성이미지준비. 전체T007미완료 정상owner/golden은다음. A C/SDKlayout/linkownership와권한경계, B checksumoracle/CI/drift/근거오인관점. 자신의범위직접라인레벨읽기. peerfinding조회공유금지. 실제읽은file:line/commands, basecandidatehash/격리, 실행ID/시작종료UTC, P0-P3findings(failure/impact/recommendation)또는0과공격범위/미검토/physicalHIL NOT_RUN/verdict포함. 파일수정commit금지. 관련AGENTS/skills 직접확인 필요. 최종한국어raw본문반환.
```

## 반환 원문

# T-007 SDK 합성 native descriptor checkpoint 독립 리뷰 — Reviewer A

판정: **PASS — 지정 8파일 delta의 object-only 정적 리뷰 한정.**  
신규 P0/P1/P2/P3 finding은 없다. 실제 SDK 빌드·native 서명·부팅·설치 검증의 PASS나 전체 T-007 완료·merge 승인은 아니다.

## 실행 식별과 격리

- Execution ID: `d0f53615-1599-4611-8928-e1da298aee7f`
- 발급: PowerShell `[guid]::NewGuid().ToString()`
- 시작 UTC: `2026-09-19T02:50:56.596Z`
- 종료 UTC: `2026-09-19T02:52:54.315Z`
- 저장소: `F:/dev/canview-wt/t007-ota-container`
- 실제 확인 base: `ec44647e4674cc382d655bb668e07254b2612fca`
- 실제 확인 candidate: `624848a784f255c6be770ce1ae37fa824e56acba`
- 담당: Reviewer A — C 객체 표현·SDK layout·linker 보존과 소유권·인증/설치 권한 경계.

시작에 두 commit 객체의 존재와 해시를 확인했고 종료에 해시를 재확인했다. 저장소 내용은 고정 commit의 `git show`와 `git diff --find-renames`로만 읽었다. 이동 중인 worktree 파일·HEAD·로컬 빌드 산출물을 검토 기준으로 사용하지 않았다.

파일 편집·커밋·worktree 생성·peer 탐색·결과 공유를 하지 않았다. 이번 peer report/finding을 조회하지 않았다. 필수 문서에 남아 있는 과거 리뷰 요약은 이번 판단의 근거로 사용하지 않았다. Worktree clean 상태는 검사하지 않았으며 clean이라고 주장하지 않는다.

## 범위 확인

전체 diff는 요청과 동일한 8파일, 142행 추가·3행 삭제다.

1. `tests/fixtures/idf-ota-image/main/metadata.c`
2. `tests/fixtures/idf-ota-image/main/CMakeLists.txt`
3. `tests/fixtures/idf-ota-image/CMakeLists.txt`
4. `tests/ota/check_sdk_metadata.py`
5. `.github/workflows/foundation.yml`
6. `tests/fixtures/idf-ota-image/README.md`
7. `docs/resume.md`
8. `docs/tasks/T-007-ota-container.md`

별도 경로 diff에서 `firmware`, `shared`, `tools`, `protocol`, `schema`의 변경이 없음을 확인했다. Production 코드·기존 SDK adapter·portable parser의 변경은 없다.

## 공격 시나리오와 정적 확인 결과

| 관점·정확한 위치 | 실패 시나리오 | 확인 결과 |
|---|---|---|
| C 객체 layout — `tests/fixtures/idf-ota-image/main/metadata.c:14` | 컴파일러 padding·정렬·endianness 차이로 descriptor가 기존 parser와 다른 byte열을 생성 | 고정 폭 정수 필드와 배열을 사용한다. `:29`부터 little-endian, 전체 168바이트, board offset 40, layout offset 104를 정적 검사한다. 기존 `shared/ota/src/native_metadata.c:8`의 필드 배치와 일치한다. |
| u64 표현 — `metadata.c:24`, `metadata.c:47` | 최대 sequence가 signed 정수·32비트·부동소수 변환으로 잘림 | 저장 필드는 `uint64_t`, 초기값은 `UINT64_MAX`다. Python 기대값도 `<…Q`와 `(1 << 64) - 1`을 사용한다. 기존 parser는 두 u32를 u64로 확장하여 조합한다. 이번 delta에 폭 축소 경로는 없다. |
| 문자열과 reserved — `metadata.c:46` | 문자열 종단·미초기화 tail·reserved garbage로 native 대조가 불안정 | `reserved`는 명시적으로 0이다. 64바이트 char 배열의 짧은 문자열 초기화는 나머지를 0으로 채운다. Checker의 64바이트 zero padding 및 기존 parser의 엄격한 문자열 대조와 일치한다. |
| 객체 수명·가변 상태 — `metadata.c:35` | descriptor가 stack 객체이거나 초기화 이후 변경되는 공유 상태가 됨 | 객체는 static storage duration의 `const`다. 포인터 필드·동적 초기화·heap·callback·task 호출이 없다. 외부 linkage는 linker가 찾는 보존 symbol의 목적이며 가변 전역 상태를 추가하지 않는다. |
| compiler/archive/linker 제거 — `metadata.c:37`, `main/CMakeLists.txt:7` | 사용처 없는 별도 object가 제거되어 실제 BIN에 metadata가 빠짐 | source 등록, `used`, `.rodata_custom_desc`, 외부 symbol에 대한 `-u`가 함께 있다. Compiler 보존과 최종 링크의 symbol 요구를 모두 표현한다. 실제 toolchain의 최종 배치·보존은 이번 실행에서 ELF/MAP으로 확인하지 않았다. |
| fixture 설정의 production 전파 — `main/CMakeLists.txt:1`, fixture `CMakeLists.txt:4` | 합성 descriptor·버전·link option이 정상 firmware에 들어감 | 새 source와 link option은 fixture의 main component에만 추가됐다. `PROJECT_VER`도 fixture project에서 설정한다. 전체 delta에 production CMake·component 수정은 없다. |
| app version 불일치 — fixture `CMakeLists.txt:4`, `check_sdk_metadata.py:20` | SDK가 다른 project version을 넣어 후속 native metadata 대조가 실패 | `PROJECT_VER`는 SDK project include 전에 `1.2.3+4`로 설정한다. Checker는 32바이트 version 배열 전체를 동일 문자열과 zero padding으로 비교한다. 값이 다르면 실패하도록 연결되어 있다. |
| custom descriptor 위치 — `check_sdk_metadata.py:7`, `:25`, `firmware/platform/esp32s3/ota_image.c:62` | metadata를 다른 offset에서 찾거나 app descriptor와 겹쳐 잘못된 값을 읽음 | Checker는 `24 + 8 + 256 = 288`에서 정확히 168바이트를 비교한다. 기존 adapter는 SDK header·segment header 뒤의 app descriptor 다음에서 custom 데이터를 읽는다. 저장소 내부 계약은 서로 일치한다. 고정 SDK의 실제 linker script·외부 문서 원문은 이번에 재검증하지 않았다. |
| 조립된 metadata를 인증으로 오인 — `check_sdk_metadata.py:53`, fixture `README.md:42` | 고정 metadata 일치와 출력 SHA-256을 native 서명·부팅·설치 성공으로 사용 | 출력은 descriptor 검사 결과와 digest를 제공하며 native signature/boot/physical/HIL을 `NOT_RUN`으로 명시한다. README도 위치·값 검사로 한정한다. 신규 권한 부여 경로는 없다. |
| 정상 실행 경로로 확대 — fixture `main/main.c:13` | 합성 이미지가 정상 OTA owner나 Flash writer를 실행 | 기존 main은 NULL negative 호출만 유지한다. 새 metadata는 이를 호출하거나 변경하지 않는다. 정상 owner/golden 미완료 상태도 task와 resume에 유지된다. |

`metadata.c`의 구조체는 fixture에서 byte열을 생성하기 위한 객체다. Portable parser가 불신 입력을 해당 구조체로 cast하는 변경은 없다. 이번 변경으로 unaligned 입력 dereference나 strict-aliasing 위험이 추가된 경로도 확인하지 못했다.

## 검사기·CI 주장 범위 확인

`tests/ota/check_sdk_metadata.py:16`은 최소 456바이트와 최대 4MiB를 검사한 뒤 고정 offset을 읽는다. `:35`의 파일 읽기도 상한보다 한 바이트만 더 읽어 초과 입력을 거절한다.

`check()`의 검사는 app descriptor magic·version 및 custom metadata의 정확한 byte 비교다. 전체 ESP image header, segment 구조, image checksum, appended hash 또는 native signature를 검증하지 않는다. 현재 문서는 이 범위를 위치·값 검사로 한정하므로, 전체 image verifier가 아니라는 사실을 신규 결함으로 계산하지 않았다.

변이 시험의 정확한 범위는 다음과 같다.

- Metadata 168개 위치마다 한 번씩 `^= 1`을 적용한다.
- 절단 위치는 0, 31, 288, 455바이트다.
- 모든 bit 조합·전체 image 변이·SDK verifier 거절 시험은 아니다.

`.github/workflows/foundation.yml:306`은 fixture SDK build 직후 검사기를 호출한다. 기존 `Invoke-TargetCommand`의 `:245` 실행과 `:246` exit-code 검사를 통해 Python 실패가 target step 실패로 이어지는 구조다. `:326`에는 해당 fixture의 project·target·binary 확인도 유지된다. 이번 변경은 검사 실패를 무시하는 분기를 추가하지 않는다.

## 실제 읽은 파일·명령

Candidate에서 직접 읽은 파일과 구간:

- `AGENTS.md`, `docs/README.md`: 전체.
- `docs/resume.md`: 전체, 변경 위치 105–108행.
- `docs/tasks/T-007-ota-container.md`: 전체 1–142행, 변경 위치 82–85행.
- Fixture `main/metadata.c`: 1–50행.
- Fixture `main/CMakeLists.txt`: 1–7행.
- Fixture `CMakeLists.txt`: 1–6행.
- Fixture `main/main.c`: 1–25행.
- Fixture `README.md`: 1–80행.
- Fixture `sdkconfig.defaults`: 1–5행.
- `tests/ota/check_sdk_metadata.py`: 1–58행.
- `.github/workflows/foundation.yml`: 175–340행 및 전체 delta.
- `shared/ota/src/native_metadata.h`: 1–38행.
- `shared/ota/src/native_metadata.c`: 1–81행.
- `shared/ota/src/manifest.h`: 1–36행.
- `firmware/platform/esp32s3/ota_image.c`: 1–83행.
- `firmware/platform/esp32s3/ota_image.h`: 1–38행.

직접 다시 읽고 적용한 skill:

- `C:/Users/digit/.codex/skills/embedded-cstyle/SKILL.md`
- `C:/Users/digit/.codex/skills/embedded-architecture/SKILL.md`

Skill은 객체 표현·소유권·의존성·권한 경계 점검에 사용했다. 자동 수정은 수행하지 않았다. 이전에 직접 읽은 `docs/runbooks/agent-workflow.md`는 이번 diff가 없음을 확인하고 재사용했다.

실제 실행 명령 계열:

```powershell
[guid]::NewGuid().ToString()
[DateTime]::UtcNow.ToString('yyyy-MM-ddTHH:mm:ss.fffZ')

git cat-file -e '<base>^{commit}'
git cat-file -e '<candidate>^{commit}'
git rev-parse '<base>^{commit}'
git rev-parse '<candidate>^{commit}'

git diff --find-renames --stat <base> <candidate>
git diff --find-renames --name-status <base> <candidate>
git diff --find-renames <base> <candidate>

git diff --find-renames <base> <candidate> -- `
  firmware shared tools protocol schema `
  docs/runbooks/agent-workflow.md docs/architecture/ota.md

git show <candidate>:<위에 열거한 경로>
git diff --find-renames --check <base> <candidate>

Get-Content -Raw C:/Users/digit/.codex/skills/embedded-cstyle/SKILL.md
Get-Content -Raw C:/Users/digit/.codex/skills/embedded-architecture/SKILL.md
```

위 `<base>`·`<candidate>`는 실행 식별 절의 전체 해시다. `git show` 출력에 PowerShell로 행 번호를 부여하고 필요한 구간을 선택했다. 출력이 잘린 관련 소스와 resume은 별도 호출로 재확인했다.

Whitespace 검사는 출력 없음이었다. 소스·빌드·테스트 실행은 하지 않았다.

## 실행 검증과 미검토 범위

이번 reviewer의 SDK build, Python 검사기 실행, 변이·절단 실행, ELF/MAP/BIN 확인 및 digest 재산출은 **NOT_RUN**이다.

사용자가 제공한 다음 결과는 coordinator evidence로만 취급한다.

- ESP-IDF 6.0.3 ELF/MAP/BIN 생성·경고 0.
- BIN offset 288의 metadata 168바이트와 u64 최대값 일치.
- 168개 byte 변이·4개 절단 거절.
- BIN SHA-256: `e9dc9177c696a13bdba0631c1da2b3ec125b688862ab2c04921c05c0675d3735`.

이 digest를 실제 파일과 독립 대조하지 않았으며 reviewer 실행 PASS로 재집계하지 않는다.

미검토·미실행 범위:

- 고정 ESP-IDF 외부 원문의 linker script·custom descriptor 배치 구현.
- 실제 link command, archive extraction, ELF symbol/section 및 MAP 배치.
- 전체 native image checksum·서명·signed golden 생성.
- 정상 target/OTA owner·Flash 단일 owner·provisioning 연결.
- 실제 MCU 부팅·Flash·전원 차단·복구·차량 동작.
- 전체 CI·artifact provenance 재감사.

**Physical/HIL: NOT_RUN. 차량 CAN TX: NO-GO 유지.**

## Finding과 최종 판정

- P0: 0건
- P1: 0건
- P2: 0건
- P3: 0건

확정 finding이 없으므로 finding별 수정 권고·disposition은 해당 없다. 공격 시나리오와 판정의 한계는 위에 기록했다.

**Reviewer A: PASS — `ec44647e…` → `624848a7…`의 fixture descriptor 준비 checkpoint 정적 범위 한정.** 합성 descriptor의 C 표현·소유권·저장소 내부 layout 계약과 인증 권한 경계에서 신규 결함을 확인하지 못했다. 전체 T-007 완료·merge·배포 승인은 부여하지 않는다.
