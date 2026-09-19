# T-107 BSP trust post-fix Reviewer B 원본

- Agent ID: `01a0b94f-d774-7352-9e98-6d4bec9c9ada` (Goodall)
- Coordinator dispatch: 2026-09-19 11:08:27 UTC

## 공통 전달 원문

```text
T-107 trust post-fix 원 reviewer 재검토. 동일 repository F:/dev/canview-wt/t007-ota-container. 새 candidate 5b153e00c12e54f7b5c9fc30c6a712722e08b3a5, base 6c14950d89bfe01f216dea33b58933dfc9e445e4. Object-only git cat-file/rev-parse/show/diff immutable 검증. 원본 A/B는 각각 evidence에 그대로 보존 완료. 자기 finding + 전체 delta 회귀를 독립 재검토한다(상대 post-fix 결과 공유금지).
B-BOOT-TRUST-01 P2: CMake cache FILEPATH→STRING으로 최초 untyped 상대 입력의 자동 절대경로 변환 방지. 회귀는 실제 유효public.der를 CWD에 두고 fresh untyped/explicit FILEPATH 두 configure + existing cache 상대경로 거절을 검사. Windows CMake4.4.3와 GNU Linux에서 새 regression을 수정 전에 실행해 기대거절 assertion 실패(return0)를 재현했고 수정후전체trust통과. 변경 C/P256알고리즘없음. sourcefixturehash만후속갱신. Windows trust1/1,GNU4개+통합,fixture13개,실제ArmD/R재빌드warning0. 이전CI2건산출물감사는journal로구분. 현재CI/전체task/실기승인아님.
Scope는 9-file delta의 code/build/tests 및 관련 기록. 실제개선: trust.cmake/tests/ota/test_stm32_boot_trust.py/fixture2. 원본report의trailingblank공백은원문보존으로허용했고원본body문자일치확인함. 원래scope배정유지하며짧고정확한최종rawreport: executionID/UTC시작종료/hash/격리/읽은파일명령/자기findingdisposition와신규P0..P3 file:line+scenario/미검토영역/PhysicalNOT_RUN/verdict. 시험재실행하지않으면정적확인으로구분. 타임아웃추측PASS금지.
```

## Raw report

판정: PASS — 지정된 post-fix 9-file delta의 정적 source 재검토에 한정. `B-BOOT-TRUST-01`은 FIXED. 신규 P0/P1/P2/P3 각각 0건.

- executionID: `B-T107-TRUST-POST-20260919T110845Z`
- UTC 시작/종료: `2026-09-19 11:08:45` / `2026-09-19 11:09:36`
- 전문범위: security/build/generator, 입력 계약·CMake·negative oracle·evidence 정합성.
- 저장소: `F:/dev/canview-wt/t007-ota-container`
- 실제 candidate: `5b153e00c12e54f7b5c9fc30c6a712722e08b3a5`
- 실제 base: `6c14950d89bfe01f216dea33b58933dfc9e445e4`
- 격리: immutable commit object-only. 시작·종료 hash 일치. Worktree clean은 N/A·미검사.
- 독립성: 보존된 최초 A/B 원본은 읽었으며 상대 post-fix 결과는 읽거나 공유하지 않았다.
- 변경·시험 재실행: 없음.

`B-BOOT-TRUST-01` P2 → FIXED, 정적 확인.

`firmware/communicator/stm32/bootloader/trust.cmake:3`의 `CACHE STRING`으로 최초 untyped 상대 입력의 자동 절대경로 변환 원인이 제거됐다. `:14`의 절대경로·존재·디렉터리 검사가 유지된다.

`tests/ota/test_stm32_boot_trust.py:95`는 유효 DER를 준비하고, `:82`는 subprocess CWD를 그 디렉터리로 고정한다. `:103`부터 별도 fresh 디렉터리에서 untyped/explicit FILEPATH 상대 입력을 각각 거절하도록 검사한다. `:118`은 기존 cache에서도 실제 존재하는 상대 파일을 사용한다. 따라서 기존의 “없는 파일이라 실패”하는 oracle 공백이 해소됐다. `:120` 이후 absolute 입력 성공·실제 C 출력·key/상수 증분 교체 검사는 유지된다.

신규 finding 및 회귀 관점:

| 등급 | 결과 |
|---|---|
| P0 | 없음. 전체 delta에서 제품 C·P256 알고리즘·서명 hook 변경이나 기본 시험키 도입 없음. |
| P1 | 없음. `trust.cmake:9`, `:24`, `:27`, `:35`의 입력 누락 거절·명시 상수·재생성 의존성·미설정 경계 유지. |
| P2 | 없음. 위 fresh/existing 경로 시나리오와 `test_stm32_boot_trust.py:82`의 CWD 변경이 absolute source/build/probe 경로를 손상하는지 검토했다. 구체적인 실패 경로 없음. |
| P3 | 없음. `docs/journal.md:12`부터 이전 CI 감사와 현재 candidate를 구분한다. fixture 5행과 helper `:26`의 digest 일치. 허용된 원본 trailing blank는 finding으로 취급하지 않았다. |

실제 읽은 파일:

```text
docs/journal.md
docs/reviews/README.md
docs/reviews/adversarial/2026-09-19-T-107-trust.md
docs/reviews/adversarial/evidence/2026-09-19-T-107-trust-reviewer-a.md
docs/reviews/adversarial/evidence/2026-09-19-T-107-trust-reviewer-b.md
firmware/communicator/stm32/bootloader/trust.cmake
tests/ota/test_stm32_boot_trust.py
tests/hil/fixtures/t103-capture-only.jsonl
tests/test_t103_capture_helpers.py
tests/ota/boot_trust/CMakeLists.txt
```

앞의 9개 전체 delta를 읽고 trust CMake·Python·시험 CMake는 candidate 전체를 줄 번호와 함께 읽었다. 이전에 읽은 AGENTS·docs README·resume·T107·workflow 및 관련 제품 C·생성기·상위 CMake·SDK pin·CI는 경로별 diff로 무변경을 확인했다.

실제 명령은 `git cat-file -e '<hash>^{commit}'`, `git rev-parse --verify '<hash>^{commit}'`, `git diff --no-ext-diff --no-textconv`의 `--stat`·`--name-only`·경로별 전체 diff, `git show <candidate>:<path>`다. 줄 번호에는 PowerShell `ForEach-Object`, 시각에는 UTC 조회를 사용했다.

Windows/GNU 수정 전 실패·수정 후 통과와 fixture 결과는 작성자 기록으로 확인했다. 요청에 기재된 Arm D/R 재빌드도 제 실행 결과가 아니다. 원시 로그·현재 CI artifact·SDK ABI 재검증·fixture digest 재계산·원본 body의 기계적 문자 비교는 수행하지 않았다.

Physical/HIL: NOT_RUN. Vehicle TX: NO-GO. Bridge: read-only. 제품 전체 키 선택·최종 링크/WRP·제조 provisioning·boot handoff·T-205 floor 및 전체 task/merge/실기 승인은 이 PASS에 포함하지 않는다.
