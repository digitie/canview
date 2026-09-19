# T-007 SDK metadata Reviewer B 원본 evidence

- Subagent ID: `01a0b784-5e5f-7d42-8953-968159b85914`

## 전달 요청 원문

```text
T-007 다음 별도checkpoint 독립 리뷰. 직전prefix·packager post-fix 원문수신및보존완료. 이번은 새범위이므로 별도새execution ID/시각/완전raw보고. 동일저장소 object-only, base ec44647e4674cc382d655bb668e07254b2612fca -> candidate 624848a784f255c6be770ce1ae37fa824e56acba. gitshow/diff/hash로격리. scope8파일: tests/fixtures/idf-ota-image/main/metadata.c 합성 const C native descriptor, main/CMakeLists linker-u, fixture/CMakeLists PROJECT_VER, tests/ota/check_sdk_metadata.py 위치/값변이검사, .github/workflows/foundation.yml targetSDKbuild직후검사등록, fixture README/resume/T007상태. 실제 ESP-IDF6.0.3 build ELF/MAP/BIN경고0, BINsha e9dc9177c696a13bdba0631c1da2b3ec125b688862ab2c04921c05c0675d3735, offset288metadata168B+u64max일치, 전168바이트변이/4절단거절 로컬확인. 모든값synthetic-only role1target1epoch7abi2u64max board/layoutsynthetic, product/provisioning/native서명/boot/physicalHIL 아님. production/SDKadapter/portableparser코드불변. purpose: native signedgolden용실제SDK합성이미지준비. 전체T007미완료 정상owner/golden은다음. A C/SDKlayout/linkownership와권한경계, B checksumoracle/CI/drift/근거오인관점. 자신의범위직접라인레벨읽기. peerfinding조회공유금지. 실제읽은file:line/commands, basecandidatehash/격리, 실행ID/시작종료UTC, P0-P3findings(failure/impact/recommendation)또는0과공격범위/미검토/physicalHIL NOT_RUN/verdict포함. 파일수정commit금지. 관련AGENTS/skills 직접확인 필요. 최종한국어raw본문반환.
```

## 반환 원문

# T-007 SDK 합성 native descriptor checkpoint — Reviewer B 독립 리뷰

판정: **PASS — 지정 delta의 object-only 정적 리뷰에 한정**

P0/P1/P2/P3 finding은 **0건**이다. 검사기의 실제 범위와 문서의 주장, CI 실패 전파, 기존 metadata 계약과의 정합성을 확인했다. 전체 T-007 완료·merge 승인·native 서명 또는 부팅 검증 결과는 아니다.

## 실행 식별과 격리

| 항목 | 실제 확인값 |
|---|---|
| Execution ID | `fff9cf8a-4e72-45ed-b78c-67d7f7d3a1e3` |
| 시작 UTC | `2026-09-19T02:50:56.2304079Z` |
| 종료 UTC | `2026-09-19T02:52:09.8609774Z` |
| 저장소 | `F:/dev/canview-wt/t007-ota-container` |
| Base | `ec44647e4674cc382d655bb668e07254b2612fca` |
| Candidate | `624848a784f255c6be770ce1ae37fa824e56acba` |
| 전문범위 | Checksum/기대값 oracle, CI 실패 전파·artifact 귀속, metadata drift, 합성 시험 근거의 과장 여부 |
| 격리 방식 | 지정 commit object의 `git show`와 base→candidate `git diff`만 사용 |

두 commit의 존재와 전체 hash를 확인하고 종료 시 다시 대조했다. 이동 중인 checkout 파일, 로컬 BIN/ELF/MAP 및 build 로그는 읽지 않았다. Object-only 방식이므로 checkout clean 상태를 검사하거나 주장하지 않는다.

파일 수정·commit·worktree 생성·peer 탐색·finding 공유는 수행하지 않았다. 이번 checkpoint의 다른 reviewer 보고서는 조회하지 않았다. 필수 resume/task에 포함된 이전 리뷰 요약은 현재 finding의 근거로 사용하지 않았다.

## Findings

| 등급 | 신규 finding |
|---|---:|
| P0 | 0 |
| P1 | 0 |
| P2 | 0 |
| P3 | 0 |

확인되지 않은 실행 결과를 PASS로 간주하지 않았으며, 명시된 후속 native signing/golden·정상 owner 연결의 미구현을 이번 delta의 결함으로 집계하지 않았다.

## 공격 범위와 판단 근거

### 1. 기대값 oracle과 metadata drift

확인 위치:

- `tests/ota/check_sdk_metadata.py:15–27`
- `tests/fixtures/idf-ota-image/main/metadata.c:14–50`
- `shared/ota/src/native_metadata.c:6–10`, `51–69`
- `shared/ota/src/native_metadata.h:7`
- `shared/ota/src/manifest.h:9`, `17`, `25`

Python 기대값은 입력 이미지에서 추출해 재사용하지 않고 고정 literal로 구성한다. 따라서 잘못 생성된 descriptor를 그대로 정답으로 채택하는 구조가 아니다.

`<8sHH5IQ`의 앞부분 40B와 board/layout 각 64B가 합쳐져 168B가 된다. 필드 순서와 offset은 기존 portable metadata 계약과 일치한다. 다음 값을 독립적으로 대조했다.

- Magic `CVIMG001`, version 1, size 168
- Role 1, target 1, epoch 7, ABI 2, reserved 0
- Sequence `UINT64_MAX`
- `synthetic-board`, `synthetic-layout` 및 나머지 zero padding

C의 구조 크기·board/layout offset·little-endian assertion과 Python의 전체 byte 비교가 서로 보완한다. C enum이나 구조 배치가 바뀌어 실제 BIN 값이 달라지면 고정 기대값 비교가 실패하는 구조다.

### 2. 위치·version 검사와 검사 범위의 한계

확인 위치:

- `tests/ota/check_sdk_metadata.py:7–27`
- `tests/fixtures/idf-ota-image/CMakeLists.txt:4`
- `tests/fixtures/idf-ota-image/README.md:38–43`

검사기는 app descriptor magic을 offset 32에서, version을 offset 48에서, custom metadata를 offset 288에서 확인한다. `PROJECT_VER`의 `1.2.3+4`와 검사기의 32B zero-padded 기대값도 일치한다.

이 검사는 전체 ESP 이미지 parser가 아니다. ESP image header·segment length·전체 checksum·서명을 검증하지 않으므로, 검사 대상 필드만 맞춘 불완전한 이미지도 통과할 수 있다. 그러나 README는 검사 범위를 metadata 위치/값 대조로 명시하고 있다. 현재 CI도 임의 외부 입력이 아니라 직전 SDK build의 BIN을 전달한다. 이 한계를 이번 범위의 결함으로 판정하지 않는다.

### 3. 변이·절단 검사의 의미

확인 위치:

- `tests/ota/check_sdk_metadata.py:36–52`

먼저 원본 입력이 정상 기대값과 일치하는지 검사한 후, metadata의 각 byte에 대해 `^= 1` 변이를 한 번씩 적용한다. 총 168개 변이며, 각 변이에서 `ValueError`가 발생하지 않으면 `AssertionError`로 실패한다.

절단 길이는 `0`, `31`, `288`, `455`다. 최소 요구 길이 456B보다 작으므로 모두 거절된다.

이 결과의 의미는 checker 자체의 고정 byte 대조와 최소 길이 검사가 작동한다는 것이다. 모든 bit 조합·모든 절단 위치·portable C parser·SDK native verifier를 시험한 결과는 아니다. 소스와 변경 문서는 그 범위를 넘어선 검증을 주장하지 않는다.

### 4. SHA256 출력과 근거 오인

확인 위치:

- `tests/ota/check_sdk_metadata.py:53–54`
- `tests/fixtures/idf-ota-image/README.md:30–43`
- `docs/tasks/T-007-ota-container.md:82–85`

SHA256은 읽은 BIN의 식별값으로 계산해 출력한다. 기대 digest와 비교하는 checksum oracle이나 고정 golden 검증은 아니다.

출력은 descriptor/u64 검사 성공과 `Native signature/boot/physical/HIL: NOT_RUN`을 함께 표시한다. 합성 이미지를 production identity·서명된 배포물·부팅 성공 근거로 승격하는 새 문구는 발견하지 못했다.

### 5. CI 실패 전파와 artifact 귀속

확인 위치:

- `.github/workflows/foundation.yml:242–254`
- `.github/workflows/foundation.yml:304–308`
- `.github/workflows/foundation.yml:326`, `350–372`
- `.github/workflows/foundation.yml:413–449`

새 검사 명령은 SDK fixture build 바로 다음에 실행되며, 해당 build의 기본 BIN 경로를 사용한다.

파일 부재, 길이·값 불일치, 변이 검사 실패는 Python의 비정상 종료로 이어진다. `Invoke-TargetCommand`는 `$LASTEXITCODE`를 검사해 예외를 발생시키며, 바깥 catch도 다시 throw한다. 실패를 로그만 남기고 계속 진행하는 경로는 확인되지 않았다.

기존 artifact 처리에는 fixture BIN/ELF/MAP 존재 검사와 SHA256 기록이 포함되어 있다. Source revision도 기대 revision과 대조한다. 새 검사 로그는 `target-firmware-logs`의 wildcard 수집 대상이며, 이미지 artifact 업로드는 성공 시에만 수행된다.

이는 CI 코드 경로의 정적 확인이다. 실제 candidate CI 실행·업로드 결과는 조회하지 않았다.

### 6. 리소스 사용과 파일 보존

확인 위치:

- `tests/ota/check_sdk_metadata.py:16`, `34–35`, `39–43`

CLI는 최대 `4MiB + 1B`만 읽고 상한 초과를 거절한다. 변이 loop는 이미지 복사를 반복하므로 비용이 발생하지만 입력 크기와 반복 횟수가 제한되어 있다. 변이 사본들을 목록으로 누적 보존하지 않는다.

입력 파일은 `rb`로만 열며 변이는 메모리 사본에 적용한다. BIN 덮어쓰기·자동 수정·서명 생성은 없다. 실제 peak RAM과 실행시간은 측정하지 않았다.

### 7. Link 보존과 권한 경계

확인 위치:

- `tests/fixtures/idf-ota-image/main/metadata.c:29–37`
- `tests/fixtures/idf-ota-image/main/CMakeLists.txt:1–7`
- `tests/fixtures/idf-ota-image/main/main.c:7–24`
- `tests/fixtures/idf-ota-image/sdkconfig.defaults:1–5`

Descriptor는 fixture의 const symbol이며 `.rodata_custom_desc`, `used`, linker `-u`를 지정한다. `metadata.c`가 fixture component source에 등록되어 있다.

새 descriptor가 product firmware나 신뢰 identity provider에 연결되는 변경은 없다. 기존 `app_main()`은 NULL negative 호출만 유지하고, 자동 signed binary 생성 비활성 설정도 불변이다.

SDK linker script와 실제 ELF section 배치는 직접 검증하지 않았다. 해당 부분에 대한 판단은 선언·build 연결 및 BIN checker의 정적 구조에 한정한다.

## 실제 읽은 파일·범위

모든 저장소 파일은 candidate object에서 읽었다.

| 파일 | 직접 읽은 범위 |
|---|---|
| `AGENTS.md` | 전체 |
| `docs/README.md` | 전체 |
| `docs/resume.md` | 1–151, 전체 |
| `docs/tasks/T-007-ota-container.md` | 1–142, 전체 |
| `docs/runbooks/agent-workflow.md` | 99–198 및 base/candidate 불변 확인 |
| `tests/ota/check_sdk_metadata.py` | 1–58, 전체 |
| `tests/fixtures/idf-ota-image/main/metadata.c` | 1–50, 전체 |
| `tests/fixtures/idf-ota-image/main/CMakeLists.txt` | 1–7, 전체 |
| `tests/fixtures/idf-ota-image/CMakeLists.txt` | 1–6, 전체 |
| `tests/fixtures/idf-ota-image/README.md` | 1–80, 전체 |
| `.github/workflows/foundation.yml` | 149–165, 220–449 및 전체 delta |
| `shared/ota/src/native_metadata.h` | 1–38, 전체 |
| `shared/ota/src/native_metadata.c` | 1–81, 전체 |
| `shared/ota/src/manifest.h` | TEXT_BYTES·role/target enum·관련 descriptor 선언 검색 부분 |
| `tests/fixtures/idf-ota-image/main/main.c` | 1–25, 전체 |
| `tests/fixtures/idf-ota-image/sdkconfig.defaults` | 1–5, 전체 |
| `tools/toolchain-versions.json` | ESP-IDF version/commit 주변 검색 부분 |

다음 skill도 직접 전부 다시 읽었다.

- `C:/Users/digit/.codex/skills/embedded-cstyle/SKILL.md`
- `C:/Users/digit/.codex/skills/embedded-architecture/SKILL.md`

고정 자료형·const storage·배치 검사·fixture와 제품 책임 분리의 검토 관점으로 적용했다. 자동 수정은 수행하지 않았다.

## 실제 명령과 결과

아래 `B`, `C`는 실행 식별 표의 전체 base/candidate hash다.

```text
[guid]::NewGuid().ToString()
[DateTime]::UtcNow.ToString('o')

git cat-file -e '<B>^{commit}'
git cat-file -e '<C>^{commit}'
git rev-parse '<B>^{commit}'
git rev-parse '<C>^{commit}'

git diff --find-renames --stat <B> <C>
git diff --find-renames --name-status <B> <C>
git diff --find-renames <B> <C>
git show <C>:<위 파일 경로>

git diff --find-renames --exit-code <B> <C> -- docs/runbooks/agent-workflow.md

git diff --find-renames --exit-code <B> <C> -- firmware shared tools CMakeLists.txt CMakePresets.json tests/fixtures/idf-ota-image/main/main.c tests/fixtures/idf-ota-image/sdkconfig.defaults

git diff --find-renames --check <B> <C>
```

줄 번호는 PowerShell `ForEach-Object`로 부여했고, 관련 부분 선택에는 `Select-Object`와 `rg -n`을 사용했다. Skill은 명시된 절대경로의 `Get-Content -LiteralPath`로 읽었다.

실제 결과:

- 변경 목록: 요청한 8파일과 일치
- Workflow 규칙 불변 검사: exit `0`
- Production·SDK adapter·portable parser·기존 fixture main/config 등 불변 검사: exit `0`
- 전체 delta whitespace 검사: exit `0`
- 종료 시 base/candidate hash: 시작 값과 일치

## 실행하지 않은 검증과 최종 한계

이번 리뷰에서 SDK build, ELF/MAP 분석, 실제 BIN 검사, 168개 변이·4개 절단 실행, host/sanitizer 시험, CI 및 artifact 다운로드는 **NOT_RUN**이다.

사용자가 제공한 ESP-IDF 6.0.3 ELF/MAP/BIN 경고 0, offset 288의 168B metadata와 u64 일치, 변이·절단 거절은 전달받은 실행 근거다. 다음 BIN SHA256도 독립 재계산하지 않았다.

```text
e9dc9177c696a13bdba0631c1da2b3ec125b688862ab2c04921c05c0675d3735
```

고정 SDK 원문·linker script, 전체 이미지 checksum·native signature, 실제 부팅·Flash·provisioning, production identity 연결, 정상 OTA owner 및 signed golden은 이번 직접 검증 범위에 포함되지 않는다.

**Physical/HIL: NOT_RUN. 차량 TX: NO-GO.**

최종 verdict는 **지정 8파일 delta의 Reviewer B 정적 PASS, 신규 finding 0건**이다. 전체 T-007 완료나 merge 승인을 뜻하지 않는다.
