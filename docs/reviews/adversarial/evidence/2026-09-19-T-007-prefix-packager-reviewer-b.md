# T-007 prefix·packager reviewer B 원본 evidence

- Subagent/thread ID: `01a0b784-5e5f-7d42-8953-968159b85914`
- Coordinator: Codex
- Candidate: `63c87272fe9f2b00b76893055bfcc8a9ca71cf26`
- Base: `6cf1b8e57840a27b83c407d1325a92f869cf2f5d`

## 전달 요청 원문

```text
CANView T-007 이번 checkpoint 독립 적대적 리뷰. 저장소 F:/dev/canview-wt/t007-ota-container. Base 6cf1b8e57840a27b83c407d1325a92f869cf2f5d; candidate 63c87272fe9f2b00b76893055bfcc8a9ca71cf26. Object-only 격리: git cat-file -e, rev-parse candidate^{commit}, diff --find-renames base candidate 및 git show candidate:path로만 소스 읽기. 이동중 worktree 내용을 기준으로 삼지 말것. 파일 편집/커밋/peer찾기 또는 공유 금지. 필요한 AGENTS/docs README/resume/T-007/agent-workflow와 해당 embedded skill을 직접 읽기. 범위: tools/ota/container.py detached raw P256 외부서명 + hash/padding/identity host 조립검사; shared/ota/src/envelope.{c,h} bounded untrusted prefix collector; tests/ota/{body_probe.c,test_prefix_stream.c,test_container.py}; root CMake 및 README/task/resume, 합성 T103 fixture digest 변경. 기존 prefix/manifest/body는 필요 경로만 확인. native verifier와 정상 OTA Flash owner는 아직 미연결이라고 명시된 partial checkpoint이며 그 미구현 자체를 이번 delta의새결함으로 부풀리지 말고 실제 결함/잘못된주장을 공격. 전체T007완료/merge승인은 아님. scope밖: hardware/HIL, flash/power/vehicle TX, 실제nativepackager/fullgolden와정상target연결(후속미완료). 확인검증: C collector 4relatedtests PASS, 실제P256/SHA256 CNG90교차 PASS, Debug전체 최초141/142(합성firmware digest old로1failure) 후 fixture업데이트, Debug/Release와sanitizer 재실행중. compile 필요하면 별도detachedworktree를 새로 만들되 source수정금지; object-only정적리뷰도 유효하나 실행하지않은test를 PASS로쓰지말것. 최종 한국어 raw report에 execution ID(툴로새로부여), 시작/종료UTC, 담당전문범위, 실제candidate/base확인hash, 격리, 실제 읽은파일·명령, P0/P1/P2/P3정확file:line·failure scenario·영향·권고, finding없어도공격범위, 미검토범위, physical/HIL NOT_RUN, BLOCK/CONDITIONAL/PASS를 포함. raw원문그대로보존할것이므로 결과본문으로 완전보고 반환. 상대reviewfinding을 읽지말고독립수행.
Reviewer B 전문범위: host packager security, actual P256/SHA256/identity trust, malformed input/resource exhaustion, outputpreservation, C/Pythoncross-test/CI inclusion, reproducibility/evidence integrity. source line-level 직접 읽기 필수.
```

## 반환 원문

# T-007 checkpoint 독립 적대적 리뷰 — Reviewer B

판정: **CONDITIONAL**

이번 delta에서 확인된 P0/P1/P2 finding은 없다. P3 문서·근거 표시 finding 2건을 보고한다. Object-only 정적 리뷰 결과이며, 전체 T-007 완료 또는 merge 승인이 아니다.

## 실행 식별과 격리

| 항목 | 실제 확인값 |
|---|---|
| Execution ID | `13439260-779e-4310-b42e-df5ac25969c8` |
| 시작 UTC | `2026-09-19T02:35:30.9295493Z` |
| 종료 UTC | `2026-09-19T02:38:37.4372250Z` |
| 저장소 | `F:/dev/canview-wt/t007-ota-container` |
| Base | `6cf1b8e57840a27b83c407d1325a92f869cf2f5d` |
| Candidate | `63c87272fe9f2b00b76893055bfcc8a9ca71cf26` |
| 담당 전문범위 | Host packager 보안, P256/SHA256와 identity 신뢰, malformed/resource exhaustion, 출력 보존, C/Python 교차시험·CI 등록, 재현성과 evidence 정합성 |
| 격리 | 지정 commit object만 사용. 이동 중인 checkout의 파일 내용을 소스 근거로 사용하지 않음 |
| 변경·공유 | 파일 편집·commit·worktree 생성·peer 탐색·외부 공유 없음 |
| Clean 상태 | Object-only 방식이므로 checkout clean 여부는 검사하지 않았고 요구하지 않음 |

현재 checkpoint의 다른 reviewer report는 조회하지 않았다. 다만 필수 resume/task의 과거 리뷰 요약과 `ota.md` 검색 결과에 과거 리뷰 발췌가 노출되었다. 해당 발췌는 이번 finding의 근거로 사용하지 않았다.

## Findings

### B-01 — P3 — 이전 전체시험 결과가 현재 candidate의 결과처럼 남아 있음

위치:

- `docs/resume.md:56`
- `docs/tasks/T-007-ota-container.md:30`
- `docs/tasks/T-007-ota-container.md:31`

근거:

두 문서는 “현재 source”의 Windows Debug/Release가 각각 `140/140` 통과했다고 서술한다. 이번 delta에는 `CMakeLists.txt:120`의 `ota-prefix-stream`과 `CMakeLists.txt:154`의 Windows `ota-container` 시험이 추가되어 있다. 사용자 제공 실행 상태도 최초 Debug `141/142`, fixture 수정 뒤 Debug/Release·sanitizer 재실행 중이다.

실패 시나리오:

다음 작업자가 현재 상태 문서만 읽으면 이전 `140/140` 결과를 새 collector·container와 fixture 변경까지 포함하는 전체시험 완료 근거로 사용할 수 있다.

영향:

검증 결과의 source 귀속과 인수인계 정확성이 떨어진다. 문서의 별도 merge gate와 미완료 표시는 유지되므로 이를 즉각적인 보안 경계 붕괴로 판정하지 않는다.

권고:

`140/140`을 해당 이전 commit의 결과로 명시하고, candidate 전체시험은 확정된 실행 결과·commit·로그에 연결한다. 진행 중인 재실행을 완료로 선기록하지 않는다.

상태: **OPEN**. 이전 문구가 이번 source 변경 이후에도 남아 생긴 귀속 문제이며, 실행 코드 결함은 아니다.

### B-02 — P3 — 입력 길이 상한을 host RAM 상한으로 표현함

위치:

- `tools/ota/container.py:42`
- 관련 검사: `tools/ota/container.py:44`
- 관련 추가 할당: `tools/ota/container.py:49`, `tools/ota/envelope.py:84`, `tools/ota/cbor.py:70`

근거:

`check_container()`는 “host RAM 상한은 MAX_BUNDLE”이라고 설명하지만 실제 검사는 `len(data) <= MAX_BUNDLE`이다. 입력 bytes를 유지한 상태에서 prefix slice, manifest slice, decoded 객체 등을 추가로 만든다. Python·암호 provider의 메모리도 이 길이 검사에 포함되지 않는다.

실패 시나리오:

호출자가 이 설명을 전체 메모리 예산 보장으로 해석하여 `MAX_BUNDLE`만 확보하면, 입력 길이 검사를 통과하더라도 추가 메모리 할당이 필요하다.

영향:

리소스 계약의 표현이 실제 구현보다 강하다. 입력 크기·CBOR 깊이·item 수는 제한되어 있으며, 이번 검토에서 무제한 할당 또는 MCU 메모리 경계 우회를 확인한 것은 아니다.

권고:

“단일 입력 bytes 길이 상한”으로 수정한다. 실제 peak RAM 보장이 필요하면 입력 보존·임시 복사·decoded 객체·provider 비용을 포함한 별도 근거를 둔다.

상태: **OPEN**. 메모리 측정은 수행하지 않았으며, 추가 할당의 존재에 근거한 문서 finding이다.

## 공격한 범위와 정적 판단

| 공격 관점 | 확인 내용 |
|---|---|
| 외부서명 대체·잘못된 curve | `_verifier()`가 EC 공개키와 SECP256R1을 요구하고 raw `r‖s`를 DER로 변환하여 정확한 message에 ECDSA/SHA256 검증을 수행한다. |
| Manifest와 다른 detached signature | 조립 후 `check_container()`가 다시 인증한다. 조립기의 signature callback이 message를 사용하지 않는다는 사실만으로 인증 우회가 되지 않는다. |
| Package 자체 identity/root 신뢰 | Host는 외부 인자의 identity·공개키를 사용한다. Python manifest와 C manifest 모두 role/board/layout/epoch/key ID를 대조한다. 역할별 root를 올바르게 제공하는 책임은 caller에 남는다. |
| 길이·padding·본문 변조 | Typed manifest가 계산한 total/offset을 사용하고, 최종 길이·선행/중간 zero padding·각 image SHA256을 검사한다. 마지막 image 뒤 추가 bytes도 거절한다. |
| Malformed 입력·리소스 소진 | CLI 입력별 bounded read, image 수·target별 크기 제한, CBOR bytes/depth/item 제한을 확인했다. Collector는 manifest 길이를 상한 검사한 뒤 고정 buffer에 복사한다. |
| Collector overflow·alias·offset | 포인터 범위 덧셈 overflow, context/input/consumed 겹침, chunk 상한, 중복·hole, sticky error, 완료 후 추가 feed를 추적했다. 초기화된 단일 owner context 계약 안에서 새 메모리 경계 결함을 찾지 못했다. |
| 조립 성공을 인증으로 오인 | Collector는 magic/서명/identity를 인증하지 않는다고 명시한다. 이후 body open이 기존 manifest/preflight/floor 경로를 호출한다. 이 partial checkpoint의 책임 구분과 일치한다. |
| Borrowed buffer 수명 | Probe scenario 10은 입력 prefix를 덮어쓴 뒤 collected buffer로 body를 열고, 이후 collector도 reset한다. 소유권 검증 의도가 실제 시험 코드에 반영되어 있다. |
| 출력 덮어쓰기·검증 실패 | 전체 검증 후 `open("xb")`를 호출한다. 기존 파일 보존과 입력 검증 실패 전 파일 미생성 경로를 확인했다. 쓰기/close의 `OSError`는 실패 반환으로 이어진다. 부분 파일의 자동 삭제나 crash durability는 보장하지 않는다. |
| 교차시험의 실제 암호 경로 | Windows probe는 CNG P256 및 SHA256을 사용하도록 compile definition·link가 설정되어 있다. 모형 provider를 실제 암호 시험으로 잘못 연결한 흔적을 찾지 못했다. |
| 90개 사례의 의미 | 3 role × 3 chunk 크기 × 5 입력 변형 × 2 수신 경로다. 새 시험 전체가 90개의 서로 다른 보안 속성을 입증한다는 뜻은 아니다. |
| CTest/CI 포함 | Collector 시험은 공통 host 등록, container 교차시험은 Windows 등록이다. Windows workflow가 locked dependency 설치 후 Debug/Release CTest를 실행한다. Linux sanitizer에는 collector가 포함되지만 Windows CNG container 시험은 포함되지 않는다. |
| 재현성 | 같은 manifest/images/detached signature의 재조립 일치를 검사한다. 실행마다 test key를 새로 생성하므로 고정 signed golden digest 시험으로 해석할 수 없다. |
| 합성 T103 digest | Fixture 5행과 helper 기대값이 모두 같은 새 digest로 변경되었음을 확인했다. 실제 source digest 재계산은 하지 않았다. 물리 capture evidence로 취급하지 않았다. |

Native verifier와 정상 OTA Flash owner의 미연결, full native packager/signed golden 미완성은 명시된 후속 범위다. 그 미구현 자체를 이번 delta의 새 finding으로 집계하지 않았다.

## 실제 읽은 파일

다음은 모두 candidate의 `git show` 또는 지정 base/candidate diff로 읽었다. “관련 부분”은 전체 파일 감사를 뜻하지 않는다.

- 필수 문서 전체: `AGENTS.md`, `docs/README.md`, `docs/resume.md`, `docs/tasks/T-007-ota-container.md`, `docs/runbooks/agent-workflow.md`.
- 변경 소스·시험 전체: `tools/ota/container.py`, `shared/ota/src/envelope.c`, `shared/ota/src/envelope.h`, `tests/ota/body_probe.c`, `tests/ota/test_prefix_stream.c`, `tests/ota/test_container.py`.
- 기존 Python 경로 전체: `tools/ota/envelope.py`, `tools/ota/manifest.py`, `tools/ota/manifest_json.py`, `tools/ota/cbor.py`.
- 기존 C 관련 부분: `shared/ota/src/body.c:1–215`, `shared/ota/src/manifest.c`의 identity/decode/check/preflight 관련 부분.
- 암호 시험 provider 전체: `tests/ota/cng_provider.c`.
- 기존 시험 관련 부분: `tests/ota/test_manifest.py:1–125`, `tests/ota/test_body.py:1–125`, `tests/test_t103_capture_helpers.py:1–100`, `tests/hil/run.py`의 source digest·identity 관련 부분.
- Build/CI: root `CMakeLists.txt:1–190` 및 delta, `CMakePresets.json` 전체, `shared/ota/CMakeLists.txt` 전체, `.github/workflows/foundation.yml:1–190`, `tools/requirements-ota.lock` 관련 pin/hash.
- 문서·fixture delta: `shared/ota/README.md`, `docs/journal.md`의 새 항목, `tests/hil/fixtures/t103-capture-only.jsonl`, `tests/test_t103_capture_helpers.py`.
- 설계·라우팅 관련 부분: `docs/architecture/README.md`, `docs/architecture/ota.md` §7 및 검색 출력, `docs/development/windows.md`, `docs/development/foundation.md` 검색 출력.

저장소 밖에서 직접 전부 읽은 skill:

- `C:/Users/digit/.codex/skills/embedded-cstyle/SKILL.md`
- `C:/Users/digit/.codex/skills/embedded-architecture/SKILL.md`

Skill은 C 경계 검사·결정적 메모리 사용·책임 분리의 검토 관점으로 적용했다. 파일 수정 절차는 수행하지 않았다.

## 실제 실행 명령과 검증 구분

실행 식별자는 PowerShell `[guid]::NewGuid()`로 새로 생성했고, 시각은 `[DateTime]::UtcNow.ToString('o')`로 기록했다.

저장소 조회 명령:

```text
git cat-file -e '<base>^{commit}'
git cat-file -e '<candidate>^{commit}'
git rev-parse '<base>^{commit}'
git rev-parse '<candidate>^{commit}'
git diff --find-renames --stat <base> <candidate>
git diff --find-renames --numstat <base> <candidate>
git diff --find-renames <base> <candidate> -- <검토 경로들>
git show <candidate>:<위 파일 경로>
```

`base`와 `candidate`는 실행 식별 표의 전체 hash다. `git show` 출력에 PowerShell `ForEach-Object`로 줄 번호를 붙였고, 필요한 절은 `Select-Object`와 `rg -n`으로 골랐다. 종료 시 두 commit hash를 다시 확인했다.

| 검증 | 이번 리뷰에서의 상태 |
|---|---|
| Base/candidate commit 존재·hash | 확인 |
| 지정 delta 소스 정적 검토 | 수행 |
| Compile / CTest / Python 시험 | **NOT_RUN** |
| 실제 CNG 90개 교차시험 재실행 | **NOT_RUN** |
| Debug/Release 전체 재실행 | **NOT_RUN** |
| ASan/UBSan·coverage | **NOT_RUN** |
| T103 source digest 재계산 | **NOT_RUN** |
| Remote CI·artifact 감사 | **NOT_RUN** |
| Physical/HIL | **NOT_RUN** |

사용자가 제공한 collector 관련 4개 시험 PASS, CNG 90개 PASS, 최초 Debug 141/142 및 후속 재실행 상태는 전달받은 정보다. 이 리뷰가 직접 실행해 확인한 PASS로 전환하지 않는다.

## 미검토 범위와 판정 조건

실제 native image 생성·서명·full golden, 정상 target 연결, Flash/power/boot 복구, hardware/HIL, 차량 TX, production root provisioning, OS I/O fault injection, peak RAM·MCU 실행시간 측정은 검토·실행하지 않았다. 기존 parser/body 전체의 재감사도 아니다.

**CONDITIONAL**의 조건은 B-01/B-02 수정 또는 명시적 disposition, 그리고 해당 candidate의 진행 중인 검증 결과를 정확한 source와 연결하는 것이다. 현재 정적 검토에서 P0/P1/P2는 발견하지 못했지만, 이를 전체 시험 통과·전체 T-007 완료·merge 승인으로 확대할 수 없다.

