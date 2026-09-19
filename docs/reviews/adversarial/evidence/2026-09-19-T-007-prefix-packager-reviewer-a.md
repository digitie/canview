# T-007 prefix·packager reviewer A 원본 evidence

- Subagent/thread ID: `01a0b784-5d6a-7540-97e8-ac159eb3a828`
- Coordinator: Codex
- Candidate: `63c87272fe9f2b00b76893055bfcc8a9ca71cf26`
- Base: `6cf1b8e57840a27b83c407d1325a92f869cf2f5d`

## 전달 요청 원문

```text
CANView T-007 이번 checkpoint 독립 적대적 리뷰. 저장소 F:/dev/canview-wt/t007-ota-container. Base 6cf1b8e57840a27b83c407d1325a92f869cf2f5d; candidate 63c87272fe9f2b00b76893055bfcc8a9ca71cf26. Object-only 격리: git cat-file -e, rev-parse candidate^{commit}, diff --find-renames base candidate 및 git show candidate:path로만 소스 읽기. 이동중 worktree 내용을 기준으로 삼지 말것. 파일 편집/커밋/peer찾기 또는 공유 금지. 필요한 AGENTS/docs README/resume/T-007/agent-workflow와 해당 embedded skill을 직접 읽기. 범위: tools/ota/container.py detached raw P256 외부서명 + hash/padding/identity host 조립검사; shared/ota/src/envelope.{c,h} bounded untrusted prefix collector; tests/ota/{body_probe.c,test_prefix_stream.c,test_container.py}; root CMake 및 README/task/resume, 합성 T103 fixture digest 변경. 기존 prefix/manifest/body는 필요 경로만 확인. native verifier와 정상 OTA Flash owner는 아직 미연결이라고 명시된 partial checkpoint이며 그 미구현 자체를 이번 delta의새결함으로 부풀리지 말고 실제 결함/잘못된주장을 공격. 전체T007완료/merge승인은 아님. scope밖: hardware/HIL, flash/power/vehicle TX, 실제nativepackager/fullgolden와정상target연결(후속미완료). 확인검증: C collector 4relatedtests PASS, 실제P256/SHA256 CNG90교차 PASS, Debug전체 최초141/142(합성firmware digest old로1failure) 후 fixture업데이트, Debug/Release와sanitizer 재실행중. compile 필요하면 별도detachedworktree를 새로 만들되 source수정금지; object-only정적리뷰도 유효하나 실행하지않은test를 PASS로쓰지말것. 최종 한국어 raw report에 execution ID(툴로새로부여), 시작/종료UTC, 담당전문범위, 실제candidate/base확인hash, 격리, 실제 읽은파일·명령, P0/P1/P2/P3정확file:line·failure scenario·영향·권고, finding없어도공격범위, 미검토범위, physical/HIL NOT_RUN, BLOCK/CONDITIONAL/PASS를 포함. raw원문그대로보존할것이므로 결과본문으로 완전보고 반환. 상대reviewfinding을 읽지말고독립수행.
Reviewer A 전문범위: embedded C runtime, bounds/overflow/alias, partial input/reset/error ownership/sticky state, caller lifetime, task/stack budget, prefix가 인증승인으로오인될경로. source line-level 직접 읽기 필수.
```

## 반환 원문

# CANView T-007 checkpoint 독립 적대적 리뷰 — Reviewer A

판정: **PASS — 지정 delta의 object-only 정적 리뷰 범위 한정.**  
확인된 신규 finding은 P0/P1/P2/P3 모두 0건이다. 테스트 실행 PASS, 전체 T-007 완료, merge 또는 배포 승인을 뜻하지 않는다.

## 실행 식별과 격리

- Execution ID: `7d17f5f8-9f65-4f26-96e8-ea6a410ccc6f`
- 발급: PowerShell `[guid]::NewGuid().ToString()`
- 시작 UTC: `2026-09-19T02:35:17.157Z`
- 종료 UTC: `2026-09-19T02:37:58.420Z`
- 저장소: `F:/dev/canview-wt/t007-ota-container`
- 실제 확인 base: `6cf1b8e57840a27b83c407d1325a92f869cf2f5d`
- 실제 확인 candidate: `63c87272fe9f2b00b76893055bfcc8a9ca71cf26`
- 담당 전문범위: embedded C runtime, bounds/overflow/alias, partial input/reset/sticky error, caller lifetime, task/stack budget, prefix 성공과 인증·설치 권한의 분리.

시작과 종료에 Git 객체 존재 및 commit 해시를 확인했다. 저장소 내용은 고정 해시의 `git show`와 `git diff --find-renames`로만 읽었다. 이동 중인 checkout·branch·빌드 산출물은 근거로 사용하지 않았다. Worktree clean 상태는 검사하지 않았으며, object-only 격리이므로 clean이라고 주장하지 않는다.

파일 편집·커밋·worktree 생성·peer 탐색·결과 공유는 하지 않았다. 이번 상대 reviewer의 report/finding을 열람하지 않았다. 필수 문서와 archive index에 포함된 과거 리뷰 요약은 이번 판정의 근거로 사용하지 않았다.

## 확인한 실패 시나리오와 근거

| 공격 관점 | 정확한 위치 | 정적 검토 결과 |
|---|---|---|
| 공격자가 manifest 길이를 0, 16,385, `UINT32_MAX`로 지정 | `shared/ota/src/envelope.c:107` | 24바이트 header를 모은 뒤 길이를 검사한다. 유효 길이만 `24 + manifest_size + 64`에 들어가므로 최대 16,472바이트다. 공격자 길이를 그대로 복사 크기에 사용하지 않는다. |
| header 경계를 가르는 chunk, 최대 chunk, 마지막 prefix와 body가 같은 chunk | `shared/ota/src/envelope.c:92` | `limit`, `remaining`, `available`의 작은 값만 복사한다. Prefix 완료 즉시 반복을 끝내고 실제 복사량을 `consumed`로 돌려준다. Body 부분까지 prefix buffer에 복사하는 경로는 확인되지 않았다. |
| 주소 구간 덧셈 overflow 및 context/input/output alias | `shared/ota/src/envelope.c:27`, `shared/ota/src/envelope.c:66` | 주소 끝 계산의 overflow를 먼저 거절한다. 세 인자 구간의 중첩을 검사한 뒤에만 `consumed`와 context를 변경한다. 대상 ESP32/STM32/Windows의 주소 모델과 유효한 caller 객체 전제에서 우회 경로를 확인하지 못했다. |
| 중복·누락 offset, 0/초과 chunk, 완료 후 재입력 | `shared/ota/src/envelope.c:78` | 기존 오류를 먼저 반환하며 새 길이·순서 오류를 context에 보존한다. 완료 후 추가 feed도 실패한다. 정상 재전송으로 실패 상태가 해제되지 않는다. |
| partial input, 반복 finish, reset 후 재사용 | `shared/ota/src/envelope.c:38`, `shared/ota/src/envelope.c:49` | `finish()`는 상태 조회이며 부분 입력을 `INCOMPLETE`로 유지한다. `init()`은 buffer와 카운터·오류를 함께 초기화한다. 연결 종료 시 폐기는 owner 책임으로 문서화되어 있다. |
| 입력 chunk 소멸 및 collector 조기 재사용 | `tests/ota/body_probe.c:287`, `tests/ota/body_probe.c:299`, `tests/ota/body_probe.c:326` | 원 입력을 덮어쓴 뒤 collector로 body를 열고, body open 직후 collector도 초기화하는 시험 경로가 있다. `shared/ota/src/manifest.h:65`의 결과는 포인터 없는 구조체이고, `shared/ota/src/body.c:68`부터 body 소유 descriptor에 해석 결과를 저장한다. 새 dangling pointer 경로를 확인하지 못했다. |
| collector의 `OK`를 서명·정책 승인으로 오인 | `shared/ota/src/envelope.h:49`, `shared/ota/src/envelope.h:78`, `tests/ota/body_probe.c:302` | collector는 미인증 조립기로 명시되어 있다. 연결된 probe는 기존 `body_open()`을 호출한다. `shared/ota/src/body.c:68`의 preflight와 `:71`의 floor 검사, `shared/ota/src/manifest.c:388`의 envelope/서명 검사를 건너뛰지 않는다. |
| task stack과 실행량 고갈 | `shared/ota/src/envelope.h:50`, `shared/ota/src/envelope.c:92`, `tests/ota/body_probe.c:199` | 큰 buffer는 caller storage이며 task stack 밖에 두도록 명시했다. 새 probe 객체도 static이다. Feed는 호출당 최대 16KiB 복사이고 heap·재귀·암호 callback이 없다. 16,472바이트는 data 배열 크기이며 context 전체 크기나 전체 call-chain stack 측정값은 아니다. |
| host 조립 성공을 native 검증·설치 성공으로 과장 | `tools/ota/container.py:48`, `tools/ota/container.py:54`, `tools/ota/container.py:72`, `tools/ota/container.py:120` | manifest 서명·identity·전체 길이·padding·image hash를 검사한 뒤 출력한다. 성공 메시지는 native signatures/local policy/install을 `NOT_VERIFIED`로 구분한다. Native verifier와 정상 Flash owner의 미연결을 이번 신규 결함으로 계산하지 않았다. |

위 결론은 초기화된 유효 context, 실제 길이만큼 읽을 수 있는 입력, 단일 owner, 모듈 상태를 외부에서 임의 변경하지 않는다는 API 계약 아래의 결론이다. 임의 메모리 손상이나 동시 호출에 대한 방어가 입증됐다는 뜻은 아니다.

## 테스트·문서 대조

- `tests/ota/test_prefix_stream.c:20`은 manifest 최소/최대 길이, header 주변 chunk 크기, 16KiB chunk, trailing body와 reset을 다룬다.
- `tests/ota/test_prefix_stream.c:56`은 NULL·alias·초과 길이·중복·누락·잘못된 manifest 길이·최소 prefix의 각 절단 위치를 다룬다.
- `tests/ota/test_container.py:147`의 CNG 교차 입력은 역할 3 × chunk 3 × 정상/변조/절단 5 × 경로 2 = 90개다. 이 중 collector 경로는 45개다. 90개 모두가 독립적인 collector 경계 시험이라는 의미는 아니다.
- `tests/ota/body_probe.c:297`은 scenario 10이 구조상 완전한 prefix만 받는다고 명시한다. Collector 자체 reject 시험과 CNG body 연결 시험을 구분한 설명과 일치한다.
- `CMakeLists.txt:117`은 portable collector CTest를 등록한다. `CMakeLists.txt:147`부터 CNG 매크로와 `bcrypt`를 사용하는 probe에 `ota-container`를 연결하며, 해당 교차 시험은 Windows 조건 안에 있다.
- `shared/ota/README.md:89`의 조립·소유권·미인증 완료 설명은 구현과 일치한다.
- 합성 T103 fixture 5개 행과 helper 기대값의 digest는 모두 `8e128a30cfe2b153bcd0e1f17267078fe22dc12c2c4cbe374c85018bab325fe1`로 변경됐다. 다른 JSON 필드 변경은 없다. Digest 재산출의 정확성은 이번 실행에서 검증하지 않았다.

## 실제 읽은 파일과 명령

Candidate 전체를 직접 읽은 파일:

- `AGENTS.md`
- `docs/README.md`
- `docs/resume.md`
- `docs/tasks/T-007-ota-container.md`
- `docs/runbooks/agent-workflow.md`
- `docs/architecture/README.md`
- `shared/ota/src/envelope.c`, `envelope.h`
- `shared/ota/src/body.c`, `body.h`, `manifest.h`, `cbor_document.h`
- `shared/interface/canview_status.h`
- `tests/ota/body_probe.c`, `test_prefix_stream.c`, `test_container.py`
- `tools/ota/container.py`, `envelope.py`, `manifest.py`

관련 절·변경분만 읽은 파일:

- `docs/architecture/ota.md`: §7–9 및 제목 목록. 내부 과거 reviewer 원문 미열람.
- `docs/reviews/README.md`: 기록 규칙·archive index.
- `shared/ota/README.md`: 1–185행과 전체 delta.
- `shared/ota/src/manifest.c`: 함수 위치 검색 및 264–505행.
- `CMakeLists.txt`: 1–175행과 전체 delta.
- `docs/journal.md`: 이번 추가분 diff만.
- `tests/hil/fixtures/t103-capture-only.jsonl`, `tests/test_t103_capture_helpers.py`: 이번 diff.

직접 읽고 적용한 skill:

- `C:/Users/digit/.codex/skills/embedded-cstyle/SKILL.md`
- `C:/Users/digit/.codex/skills/embedded-architecture/SKILL.md`

두 skill은 메모리·오류·소유권·계층 경계 점검에 사용했다. 수정은 수행하지 않았다.

실제 실행 명령 계열은 다음과 같다. Git 명령의 작업 디렉터리는 위 지정 저장소였다.

```powershell
[guid]::NewGuid().ToString()
[DateTime]::UtcNow.ToString('yyyy-MM-ddTHH:mm:ss.fffZ')

git cat-file -e 6cf1b8e57840a27b83c407d1325a92f869cf2f5d
git cat-file -e 63c87272fe9f2b00b76893055bfcc8a9ca71cf26
git cat-file -e '<hash>^{commit}'
git rev-parse '<hash>^{commit}'

git diff --find-renames --stat <base> <candidate>
git diff --find-renames <base> <candidate> -- <검토 경로들>
git show <candidate>:<위에 열거한 경로>
```

`git show` 출력에는 PowerShell `ForEach-Object`로 행 번호를 부여하거나 `Select-String`으로 관련 위치를 검색했다. Skill 두 파일은 `Get-Content -Raw`로 읽었다. 출력이 잘린 핵심 소스·필수 문서는 별도 호출로 재확인했다.

## 실행 검증과 미검토 범위

이번 reviewer가 실행한 compile·CTest·Python test·sanitizer는 **없다 — NOT_RUN**.

사용자가 전달한 collector 관련 4개 시험 PASS, CNG 90개 교차 PASS, 최초 Debug 141/142 및 fixture 수정 후 재실행 상태는 제공받은 정보다. 실행 로그를 독립 확인하거나 이번 reviewer의 PASS로 재집계하지 않았다.

다음은 미검토 또는 미실행이다.

- Debug/Release 전체 재실행, ASan/UBSan, coverage 및 target compile/link.
- MCU 실제 stack high-water, WCET, scheduler 영향, transport timeout 통합.
- T103 firmware digest 독립 재산출.
- 기존 CBOR·manifest·floor·암호 provider 전체의 재감사.
- 실제 native packager/full golden, 정상 target·OTA Flash owner 연결.
- Flash 쓰기·전원 차단·복구·차량 TX 동작.

**Physical/HIL: NOT_RUN. 차량 CAN TX: NO-GO 유지.**

## 최종 finding과 판정

- P0: 0건
- P1: 0건
- P2: 0건
- P3: 0건
- 확정 finding이 없으므로 finding별 수정 권고·disposition은 해당 없음.

**Reviewer A 판정은 PASS, object-only 정적 리뷰에 한정한다.** 이번 delta에서 경계 초과 복사, 최초 오류 소실, 입력 수명 위반 또는 collector 성공을 통한 인증 우회를 확인하지 못했다. 남아 있는 실행 검증과 명시된 후속 구현의 gate는 그대로 유지하며, 전체 T-007 완료나 merge 승인은 부여하지 않는다.

