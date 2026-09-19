# T-107 BSP trust post-fix Reviewer A 원본

- Agent ID: `01a0b94f-d65f-7ab0-90ae-b43a0d4971a8` (Ramanujan)
- Coordinator dispatch: 2026-09-19 11:08:27 UTC

## 공통 전달 원문

```text
T-107 trust post-fix 원 reviewer 재검토. 동일 repository F:/dev/canview-wt/t007-ota-container. 새 candidate 5b153e00c12e54f7b5c9fc30c6a712722e08b3a5, base 6c14950d89bfe01f216dea33b58933dfc9e445e4. Object-only git cat-file/rev-parse/show/diff immutable 검증. 원본 A/B는 각각 evidence에 그대로 보존 완료. 자기 finding + 전체 delta 회귀를 독립 재검토한다(상대 post-fix 결과 공유금지).
B-BOOT-TRUST-01 P2: CMake cache FILEPATH→STRING으로 최초 untyped 상대 입력의 자동 절대경로 변환 방지. 회귀는 실제 유효public.der를 CWD에 두고 fresh untyped/explicit FILEPATH 두 configure + existing cache 상대경로 거절을 검사. Windows CMake4.4.3와 GNU Linux에서 새 regression을 수정 전에 실행해 기대거절 assertion 실패(return0)를 재현했고 수정후전체trust통과. 변경 C/P256알고리즘없음. sourcefixturehash만후속갱신. Windows trust1/1,GNU4개+통합,fixture13개,실제ArmD/R재빌드warning0. 이전CI2건산출물감사는journal로구분. 현재CI/전체task/실기승인아님.
Scope는 9-file delta의 code/build/tests 및 관련 기록. 실제개선: trust.cmake/tests/ota/test_stm32_boot_trust.py/fixture2. 원본report의trailingblank공백은원문보존으로허용했고원본body문자일치확인함. 원래scope배정유지하며짧고정확한최종rawreport: executionID/UTC시작종료/hash/격리/읽은파일명령/자기findingdisposition와신규P0..P3 file:line+scenario/미검토영역/PhysicalNOT_RUN/verdict. 시험재실행하지않으면정적확인으로구분. 타임아웃추측PASS금지.
```

## Raw report

판정: **PASS — 지정된 post-fix delta의 정적 source 재검토에 한정.** 전체 task·현재 CI·merge·실기 승인이 아니다.

- executionID: `T107-TRUST-POST-A-20260919T110844Z`
- UTC 시작/종료: `2026-09-19 11:08:44` / `2026-09-19 11:09:24`
- 전문범위: 원 Reviewer A의 embedded C/runtime·bounds·출력 불변·const/수명·BSP identity·MCUboot ABI·보안 주장.
- 실제 candidate: `5b153e00c12e54f7b5c9fc30c6a712722e08b3a5`
- 실제 base: `6c14950d89bfe01f216dea33b58933dfc9e445e4`
- 격리: immutable Git object-only. worktree clean은 N/A·미검사. 변경·시험 실행 없음. B의 **post-fix 결과는 열람하지 않았다.**

자기 finding disposition: 기존 A finding 0건이므로 해당 없음. B-BOOT-TRUST-01은 A 관점에서 수정 구조와 회귀 oracle이 적절함을 정적으로 확인했다. 원 B의 최종 disposition을 대신하지 않는다.

확인한 실패 시나리오와 결과:

- `firmware/communicator/stm32/bootloader/trust.cmake:3`, `:14`: STRING cache로 최초 untyped 상대 입력을 보존하여 절대경로 검사에 도달시킨다. 누락·파일 존재·디렉터리 검사 및 생성 의존성은 유지된다.
- `tests/ota/test_stm32_boot_trust.py:82`, `:95`, `:103`, `:118`: 유효 DER가 존재하는 CWD에서 서로 다른 fresh build 두 개로 untyped/explicit FILEPATH 상대 입력을 검사한다. 기존 cache에서도 같은 유효 상대 파일을 검사하므로 이전의 “파일 부재 때문에 거절”하는 허점을 제거했다. 예상 밖 return 0은 assertion 실패이며 timeout을 통과로 처리하지 않는다.
- C 공급자·DER 생성기·metadata hook·identity 계약·C 시험·연결 CMake는 별도 diff가 비어 있음을 확인했다. const 복사·오류 출력 불변·키 ABI·P-256 검증의 이번 변경에 따른 회귀는 발견하지 못했다.
- fixture 두 파일은 동일 digest로 교체됐고 synthetic·CAPTURE_ONLY·TX 금지 의미를 유지한다. 기록은 이전 CI 성공과 현재 candidate 검증을 구분한다. 원본 보고서의 보존 공백은 finding으로 취급하지 않았다.

신규 finding: **P0 0 / P1 0 / P2 0 / P3 0.** 구체적인 실패 증거가 없어 수정 권고는 없다.

실제 읽은 파일은 9개 delta 전부다.

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
```

추가로 candidate의 `AGENTS.md`, `docs/README.md`, `docs/resume.md`, `docs/tasks/T-107-stm32-mcuboot.md`, workflow §5를 읽었다. evidence의 A/B 보고서는 **최초 리뷰 원본**이다.

실제 명령: 양 hash에 대한 `git cat-file -e '<hash>^{commit}'`, `git rev-parse '<hash>^{commit}'`, `git diff --name-status B C`, 파일별 `git diff --find-renames [--unified=5] B C -- <paths>`, `git show C:<path>`. 주요 source에는 PowerShell로 line number를 부여했다. 위 불변 C·생성기·계약·연결 파일은 별도 `git diff B C -- <paths>`로 무변경을 확인했다.

시험은 재실행하지 않았다. Windows/GNU 수정 전 실패·수정 후 통과와 fixture 시험은 기록으로만 읽었다. 사용자 보고의 Arm D/R 재빌드, 원시 로그·산출물, fixture digest 재계산, 원본 body 문자 일치의 기계적 대조는 독립 검증하지 않았다.

제품 전체 키 선택·최종 링크/WRP·제조 provisioning·boot handoff·T-205 floor는 미검토·미완료 경계를 유지한다. **Physical/HIL NOT_RUN, 차량 TX NO-GO, Bridge read-only.**
