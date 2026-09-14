# T-007 CBOR checkpoint Reviewer A 원본

- Agent ID: `01a097fa-7a44-7fb2-9fc9-64f6546f4c6d`
- 이 보고서는 전체 T-007 완료 리뷰가 아닌 source/document checkpoint다.

## 전달 요청 원문

```text
CANView T-007 진행 checkpoint의 독립 static 적대적 리뷰. 최종 T-007 완료 리뷰가 아니다. User requests: 전체적으로 가장 간단한 구현을 우선하고 문서에 기록; safety/required verification은 유지. Repo F:/dev/canview-wt/t007-ota-container. Candidate 6d83962b67f13a042f13dea00c103bf0f8c949af; base 490d2f8f8440bd3ccc119b36002b4c8659272e89. OBJECT-ONLY: 모든 review source는 git show candidate:path 및 git diff base candidate로 읽을 것. branch/plain working files를 immutable 기준선으로 읽지 말것. 처음/끝 cat-file -e, rev-parse candidate, git status 확인. 범위는 git diff --name-only base candidate의 16파일 전체: AGENTS.md, root/shared OTA CMake, shared/ota README 및 cbor_document.c/h,cbor_head.c, tools/ota/cbor.py, tests/ota/cbor_probe.c,test_cbor_document.c, test_cbor_document.py, synthetic T103 fixture와 expected hash, docs/tasks/T-007-ota-container.md,resume,journal. 문서 router docs/README 및 필요한 runbook/embedded skills를 먼저 읽되 docs/reviews 과거 보고서·상대방 finding은 읽지 말것. 원문 한국어로 반환; 파일쓰기/commit/push 금지. 이 실행에서는 static source/document review만 요청; compiler/runtime/physical HIL은 실행하지 않은 그대로 NOT_RUN 표기. 작성자 검증(직접 재실행 아님): Windows Debug/Release123/123; C/Python11989; focusedASanUBSan; CortexM4 object. 서명/manifest필드/전체container/target연결 미구현을 문서에서 공개했고 Task/PR는 계속IN_PROGRESS/DRAFT. 이 한정checkpoint를 전체 OTA/merge승인으로 주장하면 finding. 현재 없는 구현은 후속taskscope미완료이지 숨겨진완료주장이 아닌 한 가짜finding으로만들지말것. 실질결함의 정확file:line, failure scenario,impact,recommendation,P0/P1/P2/P3 및 BLOCK/CONDITIONAL/PASS를 명시. finding없어도 실제읽은파일/명령, 공격시나리오, 미검토범위를 기록. 시작/종료시각UTC, executionID, 확인hash/격리 metadata 포함. 약1500~3000한국어자 내외로 핵심 보고. 상대와 연락/내용공유 금지.
Reviewer A; executionID T007-CBOR-20260913-A01; 전문 영역 C memory bounds, integer overflow, lifetime, deterministic resource limits; 단순화 원칙이 안전경계를 약화하는지.
```

## 반환 보고서 원문

````text
판정: **PASS — T-007 진행 checkpoint의 독립 static 리뷰에 한정.** 확인된 P0/P1/P2/P3 finding은 각각 0건이다. 전체 T-007 완료·OTA 안전성·merge 승인을 뜻하지 않는다.

Reviewer A / `T007-CBOR-20260913-A01`
전문 영역: C memory bounds, integer overflow, lifetime, deterministic resource limits.

- 시작: `2026-09-12 23:36:26 UTC`
- 종료: `2026-09-12 23:38:01 UTC`
- candidate: `6d83962b67f13a042f13dea00c103bf0f8c949af`
- base: `490d2f8f8440bd3ccc119b36002b4c8659272e89`
- 저장소: `F:/dev/canview-wt/t007-ota-container`
- 격리: OBJECT-ONLY. 양 끝에서 두 commit의 `cat-file -e` 성공, candidate `rev-parse` 일치, `git status --short --branch` 변경 없음.
- Git directory: `F:/dev/canview/.git/worktrees/t007-ota-container`; common directory: `F:/dev/canview/.git`.

실제 검토한 변경 파일은 다음 16개다. 중괄호는 같은 디렉터리의 파일 목록을 묶은 표기다.

```text
F:/dev/canview-wt/t007-ota-container/{AGENTS.md,CMakeLists.txt}
F:/dev/canview-wt/t007-ota-container/docs/{resume.md,journal.md,tasks/T-007-ota-container.md}
F:/dev/canview-wt/t007-ota-container/shared/ota/{CMakeLists.txt,README.md,src/cbor_document.c,src/cbor_document.h,src/cbor_head.c}
F:/dev/canview-wt/t007-ota-container/tools/ota/cbor.py
F:/dev/canview-wt/t007-ota-container/tests/ota/{cbor_probe.c,test_cbor_document.c,test_cbor_document.py}
F:/dev/canview-wt/t007-ota-container/tests/hil/fixtures/t103-capture-only.jsonl
F:/dev/canview-wt/t007-ota-container/tests/test_t103_capture_helpers.py
```

`git diff --name-only base candidate`, `git diff --stat base candidate`, 경로별 `git diff base candidate -- …`, `git show candidate:path`를 사용했다. 추가로 candidate의 문서 라우터·agent workflow·OTA §7–8·head header·공용 status enum을 읽었다. 외부 `embedded-cstyle/SKILL.md`는 읽기만 하여 안전성·결정성 점검에 적용했다. 저장소 source를 plain working file로 읽거나 파일쓰기·commit·push·시험 실행을 하지 않았다.

공격 시나리오와 판단 근거:

- **길이·정수 경계:** `NULL`, 길이 0/`SIZE_MAX`, 잘린 9-byte head, `UINT64_MAX` payload/container 선언을 추적했다. [문서 순회기](/F:/dev/canview-wt/t007-ota-container/shared/ota/src/cbor_document.c:152)는 head 성공 뒤 offset을 증가시키며, payload는 남은 길이를 확인한 뒤 이동한다. 191행의 상한 검사가 196행의 `size_t` 변환·map 개수 두 배 계산보다 앞선다. 범위 밖 읽기나 정수 wrap 경로를 확인하지 못했다.
- **stack·수명·실행 상한:** root/빈 container/중첩 형제/깊이 8→9/2048→2049 item을 추적했다. 124행의 고정 stack과 190행의 push 전 검사가 대응하고, depth 0의 종료 frame은 loop 조건으로 제외되어 pop underflow가 없다. 입력을 저장하거나 callback·heap·공유 가변 상태를 사용하지 않는다. 불변 입력과 실제 readable length라는 호출 계약 아래 순회량은 byte+item 상한으로 제한된다.
- **잘못된 구조:** UTF-8 overlong·surrogate·최댓값 초과·끊긴 continuation, 비최소 정수, 비정수 key, 중복·역순 key, trailing byte를 추적했다. UTF-8 continuation 읽기 전에 남은 길이를 확인한다. 중첩 map의 key 상태도 frame별로 초기화된다.
- **검증 연결·회귀:** CMake의 두 신규 시험과 probe 연결, Python 기대 상태·prefix·변이·순환 입력 검사를 대조했다. probe는 길이 검사 후 `fread`하며 short read를 실패 처리한다. T103 fixture 다섯 행과 expected hash는 동일하게 갱신되고 no-TX 필드는 유지된다. source digest 자체는 재계산하지 않았다.

단순화 원칙은 AGENTS에 명시됐으며 필수 기능·안전 경계·검증 gate 생략을 금지한다. task의 작은 manifest·순차 image 접근은 OTA 정본과 일치한다. 문서와 header는 구조 검사 성공을 erase/write/PREPARED 권한으로 승격하지 않고, 서명·manifest 필드·전체 container·target 연결 미구현을 공개한다. 이 미완료 범위를 결함으로 집계하지 않았다.

작성자 보고인 Debug/Release 각각 123/123, C/Python 11,989건, focused ASan/UBSan, Cortex-M4 object 결과는 **직접 재실행한 evidence가 아니다**. 본 실행의 compiler/runtime/sanitizer/target/physical HIL은 모두 **NOT_RUN**이다. 실제 MCU timing·전체 call-chain stack·서명/설치 통합·CI/PR 현재 상태는 미검토다.

상대 reviewer와 연락·내용 공유는 없었다. `docs/reviews`는 열지 않았다. OTA 정본 검색에서 과거 §14 일부가 함께 노출됐으나 이번 판정 근거에서 제외했다.
````
