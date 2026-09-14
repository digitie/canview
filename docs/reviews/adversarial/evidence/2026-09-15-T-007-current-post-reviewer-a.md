# 2026-09-15 T-007 post-fix Reviewer A 요청

- execution ID: `01a0a229-5363-7f61-a607-ecae1f0a4ff8`
- submission ID: `01a0a237-e0ce-7d11-b452-0b6d84ca02d4`
- 요청 UTC: 2026-09-14 23:19:26 UTC
- 상태: PASS, A 정적 post-fix 범위만 해당. 전체 task/CI/merge 승인이 아님.

## 전달 원문

PR35 동일 reviewer post-fix 재검토. Repository F:/dev/canview-wt/t007-ota-container. Original task base d229772de77a48ae197e2ff1b4e55b6cef9a88ed, pre-fix21909e586d8fc9e7ebf8d0a4aacd7037e1d7f962 → post-fix63197e3e08ca63be0579432c0a7ee01ac9bccfa6. Object-only git cat-file/rev-parse 양 hash확인; source는gitshowpostfix, delta gitdiff pre post. Movingworktree 실행금지. 자신finding+전체delta회귀 검토, 모든최초범위재독보다 delta우선. 양원본initialCONDITIONAL report를그대로보존했고비교후finding전부수정, defer없음.
수정: body_reset이 이미FAILED일때 최초error보존, 반환값/cleanup_error는reset실패전달. probe scenario9로 AUTH/INCOMPLETE 뒤 두번추가cleanup실패+최종성공 (역할3종6vectors). generatorstagingdata_offset%65536검사, 4KiB이동15종negative. README옛미구현문구를기존검사기/adapter와미완료normalowner/target로구분. sourcehash변경에T103합성fixture동기화. docdelta는두rawreport/현재상태/요청scope대기/검증기록. 현재사용자범위질문은여전히답변없음. 전체T007미완료 유지, merge권한/완료판단아님.
coordinator실제검증: A신규회귀는수정전exit1, B15종수정전모두failure재현. 수정후 mock1702, WindowsCryptography48+CNG1708; generator11/11, generateddrift없음. WindowsDebug140/14030.81s Release140/14024.99s compiler/linker/CMakewarning0. WSLClangASanUBSanmock1702,body.c함수10/10행220/220분기94/94. 최종CI새실행중; 이전21909e5CI34906746695 6/6success를post-fix에적용하지않음. source639bfe1c01453de483e2dd3dbc9b6540c9064e3433526a053a8360f2d2cec1e3. physical/HILNOT_RUN vehicleTXNO-GO.
읽기전용, 자신의initial전문영역유지. 상대post결과접근금지. 한국어rawreport답변: executionID/이번시작종료UTC/정확hash와격리, 실제읽은파일명령, originalfindingclosure 각각FIXED검증orOPEN/새P0-P3file:line시나리오영향권고, 미검토/NOT_RUN, verdictBLOCK/CONDITIONAL/PASS. 이전원문수정금지 새postreport로보존예정.

## 원본 결과

# PR #35 Reviewer A — post-fix 재검토 원본

## 판정

`PASS` — **Reviewer A의 현재 구현분 정적 재검토 범위에 한정한다.**

A-01/P2와 A-02/P3를 모두 `FIXED`로 확인했다. 검토한 post-fix delta에서 새로운 P0/P1/P2/P3 finding은 없다. 전체 T-007 완료, CI·artifact gate 통과 또는 merge 권한을 뜻하지 않는다.

## 실행 및 격리

| 항목 | 값 |
|---|---|
| 동일 reviewer execution ID | `01a0a229-5363-7f61-a607-ecae1f0a4ff8` |
| Runtime session ID | `01a07aaf-b78b-7010-962e-8de2706b60ea` |
| 이번 시작 UTC | `2026-09-14T23:19:46.3073696Z` |
| 이번 종료 UTC | `2026-09-14T23:20:56.0315586Z` |
| 전문영역 | C memory/bounds/integer, callback·ownership·reset·fail-closed, floor/ABI, STM native parser, SDK/BSP 신뢰 경계 |
| Repository | `F:/dev/canview-wt/t007-ota-container` |
| Original task base | `d229772de77a48ae197e2ff1b4e55b6cef9a88ed` |
| Pre-fix | `21909e586d8fc9e7ebf8d0a4aacd7037e1d7f962` |
| Post-fix | `63197e3e08ca63be0579432c0a7ee01ac9bccfa6` |
| 방식 | Commit object-only |

시작·종료에 pre/post commit의 존재와 전체 hash를 확인했다. 원 task base도 일치했다. Delta는 `16 files, +572/-22`다.

소스는 post-fix 객체의 `git show`, 변경은 고정 pre/post의 `git diff --find-renames`로 읽었다. Moving worktree source·실행파일을 사용하지 않았다. 파일 수정, build/test 실행, merge는 수행하지 않았다. 상대 post-fix 결과에 접근하지 않았다.

## Original finding closure

### A-01 / P2 — `FIXED`

**대상:** 반복 cleanup 실패가 최초 오류를 덮어쓰던 문제.

**수정 위치:** [shared/ota/src/body.c:250](https://github.com/digitie/canview/blob/63197e3e08ca63be0579432c0a7ee01ac9bccfa6/shared/ota/src/body.c#L250), `shared/ota/src/body.h:100`.

소스 경로를 추적해 다음을 확인했다.

- 이미 FAILED이면 `body_mark_failed()`를 다시 호출하지 않아 최초 `body.error`를 보존한다.
- `body_close_hash()`는 이번 cleanup 오류를 `cleanup_error`에 기록하고, `body_reset()`은 그 오류를 반환한다.
- 실패 중 provider/context와 `hash_live`를 보존하고, 반환 전에 `busy`를 해제한다.
- RECEIVING 상태에서 처음 reset이 실패하면 기존처럼 FAILED로 전환하고 해석 결과를 무효화한다.
- Reset 성공 시에만 객체를 초기화하여 오류와 소유 상태를 해제한다.
- Callback 재진입 방어와 실패한 stream의 재사용 금지는 유지된다.

**회귀시험 검토:** `tests/ota/body_probe.c:368`, `tests/ota/test_body.py:217`.

Scenario 9는 최초 AUTH_FAILED/INCOMPLETE 후 fault 주입을 재설정하여 cleanup을 추가로 두 번 실패시킨다. 각 실패마다 반환값, 최초 오류, cleanup 오류, `hash_live`, `busy`를 검사한다. 이후 최초 오류의 지속 반환, 결과 무효화, 최종 reset 성공 및 실제 provider 자원 해제를 확인한다. 역할 3종 × 최초 오류 2종의 6개 vector다.

이 검사는 수정 전 코드에서 최초 재시도 직후 `body.error != status`로 실패하는 구조이며, 보고된 수정 전 exit 1과 일치한다.

**검증 구분:** 수정·시험의 적합성은 reviewer가 정적으로 확인했다. 수정 전 실패와 수정 후 mock1702/CNG1708 실행 결과는 coordinator 제공 근거이며, 본 execution에서 재실행하지 않았다.

### A-02 / P3 — `FIXED`

**대상:** 구현된 검사기를 미구현이라고 설명하던 README 문구.

**수정 위치:** [shared/ota/README.md:63](https://github.com/digitie/canview/blob/63197e3e08ca63be0579432c0a7ee01ac9bccfa6/shared/ota/README.md#L63), 같은 파일 `:101`, `:167`, `:241`.

Manifest·preflight·body streaming·STM 검사·ESP SDK adapter/BSP 연결의 기존 구현과, prefix 조립·signed packager/golden·정상 owner·target 통합의 미완료 상태를 구분하도록 수정됐다. 최초 finding의 모순은 해소됐다. 설치·물리 검증 완료로 의미를 확대하지 않았다.

## 전체 delta 회귀 검토

- **Generator:** `tools/generate_boards.py:86`의 새 조건은 staging이 있는 보드에만 64KiB 시작 정렬을 요구한다. 기존 overlap·Flash extent·staging size 검사와 함께 적용된다. 런타임 BSP의 정렬 요구와 충돌하지 않는다.
- **Generator 시험:** `tests/foundation/test_generators.py:27`은 Flash 끝을 유지하며 시작 주소를 4KiB씩 15가지 이동시킨다. 새 정렬 오류를 특정하여 검사하므로 다른 extent 오류로 잘못 통과하는 시험이 아니다. B finding의 최종 closure 판정은 원 reviewer의 책임이다.
- **합성 fixture:** T103 JSONL과 대응 시험의 변경은 동일한 `firmware_identity` 문자열 교체다. CAPTURE_ONLY, `vehicle_tx=false`, TX/ACK 0 및 시나리오 내용은 유지된다. 새 digest 자체는 독립 재계산하지 않았다.
- **문서·기록:** 변경된 현재 상태·task·journal·통합 보고서는 최초 CONDITIONAL과 재검토 대기, 이전 CI의 적용 commit, 사용자 범위 답변 대기를 구분한다. T-007 acceptance를 완료로 바꾸지 않았다.
- **변경 범위:** SDK/BSP·manifest·floor·STM native parser 구현에는 이번 delta가 없다. 최초 범위를 전부 재독하지 않고 변경과 영향 경로를 우선 검토했다.

**새 finding: 없음.**

## 실제 읽은 파일과 명령

### 소스·시험 및 계약

- `shared/ota/src/body.c`, `shared/ota/src/body.h`
- `tests/ota/body_probe.c`, `tests/ota/test_body.py`
- `tools/generate_boards.py`
- `tests/foundation/test_generators.py`
- `shared/ota/README.md`
- `tests/hil/fixtures/t103-capture-only.jsonl`
- `tests/test_t103_capture_helpers.py`

위 파일은 delta 전체와 수정 주변의 post-fix 행을 읽었다.

### 문서 delta

- `docs/journal.md`
- `docs/resume.md`
- `docs/tasks/T-007-ota-container.md`
- `docs/reviews/README.md`
- `docs/reviews/adversarial/2026-09-15-T-007-current.md`

추가된 initial 원본 A/B 파일은 보관 산출물로 분류했으며 원문 보존의 byte 단위 감사는 수행하지 않았다. 상대 post-fix 보고서는 읽지 않았다. 최초에 직접 읽은 embedded-cstyle·architecture·documentation 스킬을 같은 전문영역에 적용했다.

### 실행 명령 계열

```powershell
git cat-file -e '<pre/post>^{commit}'
git rev-parse '<pre/post/base>^{commit}'
git diff --find-renames --stat <pre> <post>
git diff --find-renames <pre> <post> -- <검토 경로>
git show <post>:<path>
Get-Date -AsUTC -Format o
```

`git show` 출력에는 PowerShell로 행 번호를 붙였다. 실제 환경의 `CODEX_THREAD_ID`와 `CODEX_SESSION_ID`도 확인했다.

## 미검토·미완료 gate

- 본 reviewer의 build/CTest/sanitizer/CNG 실행: `NOT_RUN`.
- Coordinator가 보고한 Debug/Release140/140, mock1702/CNG1708, generator11/11, sanitizer·coverage 수치는 독립 실행이나 로그 감사 결과로 표시하지 않는다.
- Post-fix CI와 target artifact/source hash·warning 감사는 본 재검토 밖이다. 이전 `21909e5`의 CI34906746695 성공을 post-fix에 적용하지 않았다.
- Signed packager/golden, prefix 수신 조립, 정상 OTA owner/writer/provider, 영속 policy 및 실제 target 통합은 미완료다.
- SDK fixture는 실제 장치 RSA 성공 또는 설치 증거가 아니다.
- Physical/HIL: `NOT_RUN`. 차량 CAN TX: `NO-GO`.
- 사용자 범위 질문은 답변 대기다. 구현·merge·일시중지는 coordinator만 처리한다.

**최종 verdict: `PASS` — A-01/A-02 closure 및 현재 구현분의 Reviewer A 정적 post-fix 재검토에 한정.**
