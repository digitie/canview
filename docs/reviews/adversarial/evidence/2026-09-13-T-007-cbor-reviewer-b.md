# T-007 CBOR checkpoint Reviewer B 원본

- Agent ID: `01a097fa-7b2f-74e2-ad78-2c80bfa4f42d`
- 이 보고서는 전체 T-007 완료 리뷰가 아닌 source/document checkpoint다.

## 전달 요청 원문

```text
CANView T-007 진행 checkpoint의 독립 static 적대적 리뷰. 최종 T-007 완료 리뷰가 아니다. User requests: 전체적으로 가장 간단한 구현을 우선하고 문서에 기록; safety/required verification은 유지. Repo F:/dev/canview-wt/t007-ota-container. Candidate 6d83962b67f13a042f13dea00c103bf0f8c949af; base 490d2f8f8440bd3ccc119b36002b4c8659272e89. OBJECT-ONLY: 모든 review source는 git show candidate:path 및 git diff base candidate로 읽을 것. branch/plain working files를 immutable 기준선으로 읽지 말것. 처음/끝 cat-file -e, rev-parse candidate, git status 확인. 범위는 git diff --name-only base candidate의 16파일 전체: AGENTS.md, root/shared OTA CMake, shared/ota README 및 cbor_document.c/h,cbor_head.c, tools/ota/cbor.py, tests/ota/cbor_probe.c,test_cbor_document.c, test_cbor_document.py, synthetic T103 fixture와 expected hash, docs/tasks/T-007-ota-container.md,resume,journal. 문서 router docs/README 및 필요한 runbook/embedded skills를 먼저 읽되 docs/reviews 과거 보고서·상대방 finding은 읽지 말것. 원문 한국어로 반환; 파일쓰기/commit/push 금지. 이 실행에서는 static source/document review만 요청; compiler/runtime/physical HIL은 실행하지 않은 그대로 NOT_RUN 표기. 작성자 검증(직접 재실행 아님): Windows Debug/Release123/123; C/Python11989; focusedASanUBSan; CortexM4 object. 서명/manifest필드/전체container/target연결 미구현을 문서에서 공개했고 Task/PR는 계속IN_PROGRESS/DRAFT. 이 한정checkpoint를 전체 OTA/merge승인으로 주장하면 finding. 현재 없는 구현은 후속taskscope미완료이지 숨겨진완료주장이 아닌 한 가짜finding으로만들지말것. 실질결함의 정확file:line, failure scenario,impact,recommendation,P0/P1/P2/P3 및 BLOCK/CONDITIONAL/PASS를 명시. finding없어도 실제읽은파일/명령, 공격시나리오, 미검토범위를 기록. 시작/종료시각UTC, executionID, 확인hash/격리 metadata 포함. 약1500~3000한국어자 내외로 핵심 보고. 상대와 연락/내용공유 금지.
Reviewer B; executionID T007-CBOR-20260913-B01; 전문 영역 Python/C differential oracle, CMake/test wiring, documentation/gate claims, synthetic hash integrity and unnecessary complexity.
```

## 반환 보고서 원문

````text
판정: **PASS — T-007 진행 checkpoint의 독립 정적 리뷰에 한정.** P0/P1/P2/P3 실질 finding은 0건이다. 전체 OTA 완료·merge·배포 승인을 뜻하지 않는다.

Reviewer B / executionID `T007-CBOR-20260913-B01`  
전문 영역: Python/C differential oracle, CMake·시험 연결, 문서 gate 주장, 합성 해시 무결성·복잡성.  
UTC 시작 `2026-09-12 23:36:29`, 종료 `2026-09-12 23:39:37`.

기준선·격리:

- candidate: `6d83962b67f13a042f13dea00c103bf0f8c949af`
- base: `490d2f8f8440bd3ccc119b36002b4c8659272e89`
- 저장소: `F:/dev/canview-wt/t007-ota-container`
- Git dir: `F:/dev/canview/.git/worktrees/t007-ota-container`; common dir: `F:/dev/canview/.git`.
- OBJECT-ONLY. 시작·종료 `cat-file -e`, candidate `rev-parse`, `git status` 확인이 성공했고 작업 트리는 모두 clean이었다. branch는 상태 metadata로만 확인했다. 파일쓰기·commit·push·상대 연락·보고서 열람은 하지 않았다.

실제 읽은 변경 범위는 `git diff --name-only base candidate`의 16파일 전부다. 아래 경로는 모두 candidate 객체 기준이다.

- `AGENTS.md`, `CMakeLists.txt`
- `shared/ota/CMakeLists.txt`, `shared/ota/README.md`
- `shared/ota/src/cbor_document.c`, `cbor_document.h`, `cbor_head.c`
- `tools/ota/cbor.py`
- `tests/ota/cbor_probe.c`, `test_cbor_document.c`, `test_cbor_document.py`
- `tests/hil/fixtures/t103-capture-only.jsonl`, `tests/test_t103_capture_helpers.py`
- `docs/tasks/T-007-ota-container.md`, `docs/resume.md`, `docs/journal.md`

문서 라우터·agent workflow·architecture 개요와 OTA 관련 계약, 공용 status·head header·기존 head 시험, HIL digest/identity 연결도 필요한 부분을 읽었다. journal은 이번 delta만 검토했다. embedded-cstyle·architecture·documentation 스킬은 경계·의존성·문서 계약 검토에 사용했다.

핵심 공격 시나리오와 판단:

1. **C/Python 분류 불일치:** 잘린 head/payload, 비최소 정수, 미지원 major, 잘못된 map key, 중복·역순, trailing byte, UTF-8 overlong·surrogate·최댓값 초과를 추적했다. 두 구현의 검사 순서에서 결과 불일치를 찾지 못했다. C는 고정 stack, Python은 재귀와 표준 UTF-8 decoder를 사용하며 명시적 기대 상태 fixture도 있어 단순 동일 구현 비교에만 의존하지 않는다. 다만 변이 corpus 전체의 기대값은 Python oracle이므로 공통 오해를 모두 배제하는 증거는 아니다.

2. **자원 한도·상태 오염:** 16KiB, depth8, item2048의 정확한 경계, 거대한 uint64 길이·개수, 빈 container, 형제 map 전환, Python 순환 입력을 추적했다. C의 길이 검사 후 변환·곱셈, frame 재초기화와 종료 조건에서 실질 결함을 찾지 못했다. callback·heap·SDK·writer 의존성 없이 현재 요구를 제한된 함수로 구현했다. 불필요한 framework 도입도 확인되지 않았다.

3. **시험 누락·가짜 성공:** root CMake는 document 시험과 differential 시험을 등록하고 실제 probe 경로를 전달한다. probe는 Windows stdin을 binary로 설정하며 길이·입력 실패를 비정상 종료한다. Python은 종료 코드, 결과 개수와 각 상태를 검사한다. Release에서도 C의 `CHECK`가 제거되지 않는다.

4. **합성 evidence 오표시:** `_source_digest`의 경로 ordinal 정렬·LF 정규화·길이 prefix 규칙으로 candidate의 firmware/shared/protocol 204파일을 메모리에서 해시 계산했다. 결과  
   `a3a72738db64375a554557cbc819fbd61cdf4725b97ca6ad22497c8bac7b6810`  
   는 fixture 5행·기대 상수·journal과 일치한다. 실제 차량 capture나 physical PASS로 승격한 변경은 없다.

5. **완료 주장·안전 gate 약화:** AGENTS의 단순화 원칙은 필수 안전·검증 유지 조건을 포함한다. task는 `IN_PROGRESS`이며 서명·manifest 의미·전체 container·target 연결 미완료를 공개한다. 구조 검사 OK를 erase/write/PREPARED 권한으로 설명하지 않는다. 이 후속 미구현 범위는 이번 finding으로 만들지 않았다.

사용 명령은 `git show candidate:path`, `git diff --find-renames base candidate`, `git diff --name-only/--stat`, 객체·상태 확인 명령이다. digest용 파일 목록만 `git ls-tree`로 얻고 내용은 모두 `git show`로 읽었다. 해시는 PowerShell/.NET에서 계산했으며 제품 코드는 실행하지 않았다.

직접 compiler·runtime·CTest·sanitizer·Cortex-M4·physical HIL 검증은 전부 **NOT_RUN**이다. Windows Debug/Release 123/123, C/Python 11,989건, focused ASan/UBSan, Cortex-M4 object는 작성자 제공 결과이며 재실행·로그 인증하지 않았다. 서명 안전성, 전체 container, target call-chain·시간, Flash/복구, 실차 동작과 live PR/CI 상태는 미검토다.
````
