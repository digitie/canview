# 2026-09-15 T-007 post-fix Reviewer B 요청

- execution ID: `01a0a229-545f-7fc0-a90a-240470bb1206`
- submission ID: `01a0a237-e11c-7ee1-aa3f-4ac9010d882a`
- 요청 UTC: 2026-09-14 23:19:26 UTC
- 상태: CONDITIONAL, B finding 모두 FIXED. CI/artifact 및 최종 종합은 별도.

## 전달 원문

PR35 동일 reviewer post-fix 재검토. Repository F:/dev/canview-wt/t007-ota-container. Original task base d229772de77a48ae197e2ff1b4e55b6cef9a88ed, pre-fix21909e586d8fc9e7ebf8d0a4aacd7037e1d7f962 → post-fix63197e3e08ca63be0579432c0a7ee01ac9bccfa6. Object-only git cat-file/rev-parse 양 hash확인; source는gitshowpostfix, delta gitdiff pre post. Movingworktree 실행금지. 자신finding+전체delta회귀 검토, 모든최초범위재독보다 delta우선. 양원본initialCONDITIONAL report를그대로보존했고비교후finding전부수정, defer없음.
수정: body_reset이 이미FAILED일때 최초error보존, 반환값/cleanup_error는reset실패전달. probe scenario9로 AUTH/INCOMPLETE 뒤 두번추가cleanup실패+최종성공 (역할3종6vectors). generatorstagingdata_offset%65536검사, 4KiB이동15종negative. README옛미구현문구를기존검사기/adapter와미완료normalowner/target로구분. sourcehash변경에T103합성fixture동기화. docdelta는두rawreport/현재상태/요청scope대기/검증기록. 현재사용자범위질문은여전히답변없음. 전체T007미완료 유지, merge권한/완료판단아님.
coordinator실제검증: A신규회귀는수정전exit1, B15종수정전모두failure재현. 수정후 mock1702, WindowsCryptography48+CNG1708; generator11/11, generateddrift없음. WindowsDebug140/14030.81s Release140/14024.99s compiler/linker/CMakewarning0. WSLClangASanUBSanmock1702,body.c함수10/10행220/220분기94/94. 최종CI새실행중; 이전21909e5CI34906746695 6/6success를post-fix에적용하지않음. source639bfe1c01453de483e2dd3dbc9b6540c9064e3433526a053a8360f2d2cec1e3. physical/HILNOT_RUN vehicleTXNO-GO.
읽기전용, 자신의initial전문영역유지. 상대post결과접근금지. 한국어rawreport답변: executionID/이번시작종료UTC/정확hash와격리, 실제읽은파일명령, originalfindingclosure 각각FIXED검증orOPEN/새P0-P3file:line시나리오영향권고, 미검토/NOT_RUN, verdictBLOCK/CONDITIONAL/PASS. 이전원문수정금지 새postreport로보존예정.

## 원본 결과

# PR35 post-fix 독립 재검토 — Reviewer B

## 실행과 판정

| 항목 | 기록 |
|---|---|
| 실제 execution ID | `01a0a229-545f-7fc0-a90a-240470bb1206` — 최초 리뷰와 동일 |
| 이번 시작 UTC | `2026-09-14T23:19:44Z` |
| 이번 종료 UTC | `2026-09-14T23:21:26Z` |
| 전문영역 | Python/schema·암호 wire 계약·native metadata·generator·build/CI·evidence·요구사항 정합성 |
| original base | `d229772de77a48ae197e2ff1b4e55b6cef9a88ed` |
| pre-fix | `21909e586d8fc9e7ebf8d0a4aacd7037e1d7f962` |
| post-fix | `63197e3e08ca63be0579432c0a7ee01ac9bccfa6` |
| delta | 16파일, `+572/-22` |
| verdict | **CONDITIONAL** |

**B-01/P2와 B-02/P3는 모두 FIXED로 재확인한다. Reviewer B의 OPEN finding은 없으며, 검토한 delta에서 새 P0/P1/P2/P3를 발견하지 못했다.**

CONDITIONAL을 유지하는 이유는 post-fix CI·target artifact 감사와 최종 독립 리뷰 종합이 남아 있기 때문이다. 전체 T-007 완료나 merge 권한을 부여하는 판정은 아니다. 사용자 범위 질문도 답변 대기 상태로 유지한다.

## 격리·독립성

시작과 종료에 pre/post 객체의 `git cat-file -e`, `git rev-parse`를 확인했다. 종료에 original base도 확인했다.

- source: `git show <post-fix>:<path>`
- delta: `git diff --find-renames <pre-fix> <post-fix>`
- Python 검사: post-fix Git 객체를 메모리에서 로드하고 저장소 입력도 `git show`로 읽음.
- 생성 결과·변이 입력은 메모리에만 유지.
- moving worktree 실행, 파일 작성, 코드 수정, commit/push/merge, CI artifact 다운로드 없음.
- 상대 post-fix 결과에 접근하지 않음.
- 최초 raw report를 수정하지 않음.

기존에 읽은 embedded-cstyle·architecture·documentation 지침을 계속 적용하되, 사용자의 읽기 전용 범위를 유지했다.

## Original finding closure

### B-01 — P2 → FIXED

**수정 위치**

- `tools/generate_boards.py:86`
- `tests/foundation/test_generators.py:27`

staging이 있는 board의 `data_offset % 65536 != 0`을 `ValueError("OTA staging image alignment")`로 거절한다.

**직접 실행한 검증**

post-fix generator를 Git 객체에서 실행했다.

| 입력·검사 | 결과 |
|---|---|
| `data_offset += 4096…61440`, `data_size -= 동일값` | 15/15 거절, 정확한 오류 메시지 확인 |
| 현재 정렬 위치 | 수용 |
| `data_offset += 65536`, `data_size -= 65536` | 수용 |
| 현재 board 생성물 | 13/13 candidate byte열과 일치 |

최초 발견한 sector-only staging 입력을 거절하면서, 유효한 정렬 입력과 기존 생성물을 보존한다. 새 회귀시험도 Flash 끝을 유지하여 다른 제약의 실패를 정렬 검사 성공으로 오인하지 않도록 구성됐다.

**잔여 권고:** 이 finding에 추가 수정 요구 없음. 실제 partition 설치·SDK 장치 실행의 완료 증거는 아니다.

### B-02 — P3 → FIXED

**수정 위치**

`shared/ota/README.md:63`, `:101`, `:167`, `:218`, `:241`

기존 manifest·서명 검사·body streaming·ESP SDK adapter/BSP 구현과 다음 미완료 범위를 구분하도록 수정됐다.

- prefix 부분 수신 조립
- 전체 signed packager/검사 CLI/golden
- 정상 OTA owner·writer·provider·target 통합
- 실제 장치 실행과 최종 검증

최초 지적한 구현 상태의 상충 문구가 해소됐다. `HASHES_MATCHED`와 설치 권한의 구분, physical/HIL NOT_RUN도 유지한다.

**검증:** post-fix 본문과 delta의 해당 절을 줄 단위로 대조했다. 추가 수정 요구 없음.

## 전체 delta 회귀 검토

### Cleanup 오류 보존

관련 위치:

- `shared/ota/src/body.c:243`
- `shared/ota/src/body.h:100`
- `tests/ota/body_probe.c:368`
- `tests/ota/test_body.py:217`

정적 추적으로 다음을 확인했다.

- 이미 FAILED이면 추가 reset 실패가 최초 `body.error`를 덮어쓰지 않는다.
- reset 실패 반환값과 `cleanup_error`는 provider의 cleanup 실패를 전달한다.
- 실패한 cleanup의 `hash_live`·context 소유를 유지하며 `busy`는 해제한다.
- 아직 FAILED가 아닌 객체의 reset 실패는 기존처럼 FAILED로 전이한다.
- reset 성공 시 전체 객체를 초기화한다.

scenario9는 최초 AUTH/INCOMPLETE 이후 fault를 다시 활성화하여 추가 cleanup 실패를 두 번 발생시킨다. 각 반복에서 최초 오류·cleanup 오류·소유 상태·busy를 검사하고, 이후 성공 reset과 native/mock resource 해제도 확인한다.

이 execution에서 C 실행을 재현하지는 않았다. 해당 변경의 원 finding closure는 Reviewer A의 독립 판정을 대신하지 않는다.

### 합성 evidence identity

post-fix의 `firmware/shared/protocol` **222개 Git 객체**를 경로 정렬·LF 정규화·길이 prefix 규칙으로 직접 해시했다.

```text
639bfe1c01453de483e2dd3dbc9b6540c9064e3433526a053a8360f2d2cec1e3
```

coordinator가 제시한 값, T-103 합성 fixture 5개 record, helper의 기대 identity와 모두 일치했다. 이 변경을 실물 evidence 갱신으로 해석하지 않는다.

### 문서·검증 범위

현재 상태·task·journal·통합 review의 delta는 다음 경계를 유지한다.

- 이전 candidate의 CI 성공을 post-fix에 적용하지 않음.
- 원 reviewer 재확인과 post-fix CI/artifact 감사 대기.
- 전체 T-007 acceptance 미완료.
- 사용자 범위 질문 대기, 다음 task 시작 금지.
- physical/HIL NOT_RUN, 차량 TX NO-GO.

새 schema·wire·CMake·CI workflow·의존성 변경은 이번 delta에 없다. 따라서 최초 범위 전체를 다시 읽지 않았다.

## 검증 evidence 구분

**직접 확인**

- pre/post/base 객체 identity.
- B-01 음성 입력 15건, 정렬 양성 입력 2건.
- 현재 생성물 13개 일치.
- source digest와 합성 fixture identity 일치.
- 보존 raw evidence 파일을 제외한 delta `git diff --check`: exit 0.
- B-02 문서 및 cleanup 변경의 정적 회귀 검토.

**coordinator 전달 evidence — 이 reviewer의 재실행 결과 아님**

- 수정 전 신규 cleanup 회귀 exit 1, generator 음성 입력 15건 모두 실패 재현.
- 수정 후 mock 1702건, Cryptography48+CNG 1708건.
- generator 11/11, 생성물 drift 없음.
- Windows Debug 140/140·30.81초, Release 140/140·24.99초.
- 해당 build의 compiler/linker/CMake warning0.
- WSL Clang ASan/UBSan mock 1702건, body 함수 10/10·행 220/220·분기 94/94.

post-fix CI는 새 실행 중이다. pre-fix run `34906746695`의 6/6 성공은 이번 commit의 통과 증거로 사용하지 않는다.

## 실제 읽은 파일·명령

**Delta와 관련 source를 읽은 파일**

- `tools/generate_boards.py`
- `tests/foundation/test_generators.py`
- `shared/ota/README.md`
- `shared/ota/src/body.c`, `body.h`
- `tests/ota/body_probe.c`, `test_body.py`
- `tests/hil/fixtures/t103-capture-only.jsonl`
- `tests/test_t103_capture_helpers.py`
- `tests/hil/run.py`의 canonical source digest 구현
- `docs/journal.md` 신규 기록
- `docs/resume.md`
- `docs/tasks/T-007-ota-container.md`
- `docs/reviews/README.md`
- `docs/reviews/adversarial/2026-09-15-T-007-current.md`

추가로 generator 입력·생성물과 source digest 대상 222개 파일을 **Git 객체 데이터로 읽었다**. 이 222개 전부를 줄 단위 코드 리뷰했다는 뜻은 아니다. 새로 보존된 최초 raw report 두 파일은 diff 목록에서 확인했으며 본문을 재독·수정하지 않았다.

**명령**

```powershell
git cat-file -e '<pre/post>^{commit}'
git rev-parse '<pre/post/base>^{commit}'
git diff --find-renames --stat <pre> <post>
git diff --find-renames <pre> <post> -- <관련 경로>
git show <post>:<path>
git ls-tree -r --name-only <post> -- firmware shared protocol
python -B -  # Git 객체 전용 메모리 검사
git diff --find-renames --check <pre> <post> -- . ':!docs/reviews/adversarial/evidence/**'
```

## 남은 한계와 최종 판정

C host suite·sanitizer·coverage·target build·실제 RSA를 이 execution에서 재실행하지 않았다. CI artifact 감사, 장치 Flash·전원 차단·복구 시험도 수행하지 않았다.

signed packager/golden, 정상 owner/writer/provider, 영속 policy와 target integration 등 최초 보고서의 미완료 acceptance는 그대로 남는다.

**최종: CONDITIONAL. B-01/B-02는 FIXED, 새 finding 없음. post-fix CI/artifact 및 독립 리뷰 종합 조건은 미완료다. Physical HIL NOT_RUN, vehicle CAN TX NO-GO.**
