# T-107 BSP trust Reviewer B 원본

- Agent execution ID: `01a0b94f-d774-7352-9e98-6d4bec9c9ada` (Goodall)
- Coordinator dispatch: 2026-09-19 10:56:52 UTC
- 격리: immutable commit object-only. 원본 verdict는 아래 그대로 보존한다.

## 전달한 공통 manifest

```text
T-107 BSP boot trust 중간 독립 적대적 source review. 저장소 F:/dev/canview-wt/t007-ota-container. Candidate 6c14950d89bfe01f216dea33b58933dfc9e445e4, base 06383d6cf9aa8b0c68646f96b3507e2144ca074e. 두 reviewer는 동일 manifest/서로 결과 미공유. Object-only: git cat-file -e, rev-parse, git diff base candidate, git show candidate:path로만 repo 내용을 읽고 필요한 source에 line number를 매긴다. 움직이는 worktree 내용에 의존하지 말 것. 변경/commit/PR/merge 금지. AGENTS/docsREADME/resume/T107/관련 workflow 및 skills를 필요한 순서로 따르되 관련 없는 기록 전체는 읽지 않는다.
Scope: delta 전체 14 files, 특히 tools/ota/generate_stm32_boot_trust.py, firmware/communicator/stm32/bsp/boot_identity.c, bootloader/trust.cmake와 두 CMake 연결, tests/ota/test_stm32_boot_identity.c,test_stm32_boot_trust.py,boot_trust/CMakeLists.txt,README/fixture. 관련 canview_boot_identity.h, shared/ota/src/manifest.h, image_hooks.c, OTA§6 참조.
요구: 표준 P256 SPKI public DER만,const BSP role/board/layout, 제조 epoch/manifest rootID/ABI 명시 u32; missing default 없음; 실제 SDK bootutil key ABI; error출력불변/constcopy; key/상수 변경 재생성; synthetic key/identity 제품기본값 금지. 제품 전체키선택/최종링크/WRP/제조프로비저닝/boot handoff/T205 floor는 아직 미구현이며 여기서완료주장금지. 물리/HIL NOT_RUN,vehicleTX NO-GO,Bridge read-only.
이미 수행한 검증은 journal에 정확히기록: host관련157D/R,새BSPsanitizer/coverage100,실제ArmprimaryD/Ridentityarchive+기존app,입력negative/변이/증분,C99GNU. 이를 reviewer자신이실행했다고 쓰지말것. 이 review는 좁은 중간단위이며 전체task/merge승인이 아니다.
최종 한국어 raw report: executionID,UTC시작/종료,전문범위,candidate/base실제hash,격리와clean해당여부,실제 읽은 파일/명령, P0/P1/P2/P3 각각file:line/failure scenario/영향/권고(없으면공격한시나리오),미검토범위,physicalNOT_RUN, BLOCK/CONDITIONAL/PASS. 특정실패 증거없는취향finding금지. 직접검토불가면불완전명시하고 PASS금지.
```

## 개별 전문 범위

B 전문: security/build/generator, DER malformed/잘못된 curve/privatekey, integer coercion, CMake incremental drift/누락 입력, negative test oracle/CI evidence와 문서 일치.

## Raw report

판정: CONDITIONAL — P2 1건 OPEN. 좁은 중간 source review이며 전체 T-107 완료·merge 승인이 아니다.

실행 정보

- executionID: `B-T107-BOOT-TRUST-20260919T105713Z`
- UTC 시작: `2026-09-19 10:57:13`
- UTC 종료: `2026-09-19 11:00:40`
- 전문범위: security/build/generator, DER·u32 입력, CMake 의존성, negative oracle, 문서·검증 근거 정합성.
- 저장소: `F:/dev/canview-wt/t007-ota-container`
- 실제 확인 candidate: `6c14950d89bfe01f216dea33b58933dfc9e445e4`
- 실제 확인 base: `06383d6cf9aa8b0c68646f96b3507e2144ca074e`
- 격리: commit object-only. 움직이는 checkout·HEAD·생성물에 의존하지 않았다.
- clean: object-only 방식이므로 적용 대상 아님. worktree clean 여부는 확인하지 않았으며 clean이라고 주장하지 않는다.
- 독립성: 제공된 manifest의 B 역할로 검토했다. 상대 reviewer 결과를 열람하거나 공유하지 않았다. A에게 전달된 manifest 자체는 확인하지 않았다.
- 변경·commit·PR·merge·테스트 실행: 없음.

P2 — `B-BOOT-TRUST-01`: 최초 configure에서 상대 공개키 경로가 절대경로 검사 전에 변환된다. OPEN.

위치:

- `firmware/communicator/stm32/bootloader/trust.cmake:2`
- `firmware/communicator/stm32/bootloader/trust.cmake:13`
- `tests/ota/test_stm32_boot_trust.py:105`
- `tests/ota/test_stm32_boot_trust.py:110`

실패 시나리오: cache가 없는 build에서, 현재 작업 디렉터리에 유효한 `public.der`가 있고 나머지 세 제조 상수를 명시하면서 `-DCANVIEW_BOOT_PUBLIC_DER=public.der`를 전달한다. 타입 없는 CLI cache 입력에 2행의 `CACHE FILEPATH` 선언이 적용되면 CMake가 상대경로를 절대경로로 변환한다. 따라서 13행의 `IS_ABSOLUTE`는 원래 상대 입력을 거절하지 못한다. 이 변환은 [CMake 4.4.3 공식 set 문서](https://cmake.org/cmake/help/latest/command/set.html#set-cache-entry)에 명시돼 있다.

현재 시험은 105행에서 먼저 빈 설정으로 typed cache를 만든 다음 상대경로를 검사한다. 또한 `relative.der`라는 유효 파일을 준비하지 않으므로, 절대경로 검사에 결함이 있어도 `NOT EXISTS`만으로 negative case가 통과할 수 있다.

영향: README 109행의 “기존 절대 파일 경로” 입력 계약이 최초 configure에서 보장되지 않는다. 동일한 상대 파일명이 작업 디렉터리마다 다른 공개키를 가리킬 수 있다. DER 검증이나 서명 검증 우회가 입증된 것은 아니므로 P2로 분류한다.

권고: `FILEPATH` 자동 변환 전에 원래 입력의 절대경로 여부를 검사하거나, 원문을 보존하는 `STRING` cache로 검사한다. 유효 DER 파일을 실제 준비하고 새 build 디렉터리에서 상대경로 거절을 시험한다. 기존 cache 경로 시험도 유지한다.

근거 수준: candidate source와 공식 CMake 의미론에 따른 정적 실패 경로 확인이다. 동적 재현을 실행했다고 주장하지 않는다.

나머지 심각도 및 공격 결과

| 등급 | 결과와 검토한 실패 시나리오 |
|---|---|
| P0 | 발견 없음. `generate_stm32_boot_trust.py:29`의 길이 제한, `:32`의 공개키 파싱, `:35`의 curve 제한, `:38`의 canonical 재직렬화 대조를 통해 private DER·다른 curve·잘림·추가 바이트 수용 경로를 검토했다. `image_hooks.c:112`는 metadata 일치 뒤에도 `FIH_BOOT_HOOK_REGULAR`를 반환한다. |
| P1 | 발견 없음. 생성기 `:16`, `:26`에서 음수·overflow·Unicode 숫자·bool/float coercion을 검토했다. `trust.cmake:8`의 일부 입력 누락 거절, `:34`의 전체 미설정 표시, `boot_identity.c:20`의 오류 시 출력 불변과 `:21`의 배열 포함 값 복사를 확인했다. Ninja 구성의 key 파일·생성기 의존성과 상수 변경 명령도 검토했다. |
| P3 | 발견 없음. README `:90`, `:126`, journal `:23`, `:31`은 archive compile·기존 앱 검증과 최종 boot 연결을 구분한다. fixture 변경은 digest 교체이며 physical evidence 승격이나 차량 TX 허용 변경은 없다. |

검증 근거의 구분

`docs/journal.md:10`부터 기록된 아래 결과는 작성자의 기존 검증 기록으로 읽었다. 제가 실행하거나 원본 로그로 재확인한 결과가 아니다.

- 관련 host Debug/Release 각각 157/157. 기존 108시나리오·11240cut은 이번 실행에서 제외했다고 명시한다.
- 새 BSP C의 ASan/UBSan 및 line/region/function/branch coverage 100%.
- 실제 Arm primary Debug/Release identity archive와 기존 앱 검증.
- Python 경계·728bit 변이, C null/출력 불변/복사, CMake 누락·오류·증분 교체, GNU 검증.

CI source에는 Windows host 시험 연결이 있다. 다만 `.github/workflows/foundation.yml:311`과 `:313`의 Arm configure에는 네 trust 입력이 없으므로, 그 경로를 새 identity archive의 Arm CI 검증으로 계산할 수 없다. journal은 새 CI를 후속 작업으로 표시하며 완료를 주장하지 않는다.

실제 읽은 파일

다음 14개 파일의 전체 delta를 읽었다. 주요 C/Python/CMake source는 candidate blob에 줄 번호를 붙여 추가 확인했다.

```text
CMakeLists.txt
docs/journal.md
docs/resume.md
firmware/communicator/stm32/CMakeLists.txt
firmware/communicator/stm32/bootloader/CMakeLists.txt
firmware/communicator/stm32/bootloader/README.md
firmware/communicator/stm32/bootloader/trust.cmake
firmware/communicator/stm32/bsp/boot_identity.c
tests/hil/fixtures/t103-capture-only.jsonl
tests/ota/boot_trust/CMakeLists.txt
tests/ota/test_stm32_boot_identity.c
tests/ota/test_stm32_boot_trust.py
tests/test_t103_capture_helpers.py
tools/ota/generate_stm32_boot_trust.py
```

추가로 읽은 정본·연결 문서와 source는 다음과 같다. 긴 문서는 관련 부분만 검토했다.

```text
AGENTS.md
docs/README.md
docs/tasks/T-107-stm32-mcuboot.md
docs/runbooks/agent-workflow.md
docs/reviews/README.md
docs/architecture/README.md
docs/architecture/ota.md (§6·§7)
docs/development/windows.md (도구·검증 관련 부분)
firmware/communicator/stm32/bootloader/include/canview_boot_identity.h
firmware/communicator/stm32/bootloader/image_hooks.c
firmware/communicator/stm32/bootloader/mcuboot_compat.cmake
firmware/communicator/stm32/bootloader/include/mcuboot_config/mcuboot_config.h
shared/ota/src/manifest.h
tests/ota/mcuboot_model.c (identity·key 공급 부분)
tools/toolchain-versions.json (MCUboot pin)
cmake/CanviewWarnings.cmake
CMakePresets.json
.github/workflows/foundation.yml (host·Arm·artifact 관련 부분)
```

저장소 밖에서는 `embedded-cstyle/SKILL.md`, `embedded-architecture/SKILL.md`를 읽어 const·오류 계약과 계층 연결 검토에 적용했다. 수정 지침은 적용하지 않았다.

실제 명령·도구

- `git cat-file -e '<hash>^{commit}'`
- `git rev-parse <candidate> <base>` 및 각 `--verify '<hash>^{commit}'`
- `git diff --stat <base> <candidate>`
- `git diff --no-ext-diff --no-textconv <base> <candidate> -- <검토 경로들>`
- 같은 기준의 `git diff --name-only`
- `git show <candidate>:<위 파일/디렉터리 경로>`와 PowerShell 줄 번호·범위 필터
- 저장소 밖 skill 읽기에 `Get-Content -Raw`
- UTC 시계 조회
- `cmake --help-command set`: PATH에 cmake가 없어 실패. 공식 문서를 읽어 의미론을 확인했다.

최초 인용하지 않은 `^{commit}` 인수는 PowerShell 해석 때문에 실패했다. 인용한 명령으로 재확인했으며 최종 hash는 위와 같다.

미검토·미완료 범위

실제 SDK의 `sign_key.h` 원문과 compiler 결과는 직접 확인하지 못했다. candidate에서 실제 SDK include 경로, pin/clean 검사, `bootutil_keys` 공급 연결은 확인했지만, SDK ABI의 직접 대조·재실행 검증은 불완전하다. 로컬 검증 로그·CI 실행 artifact·fixture digest 재계산도 수행하지 않았다.

제품 전체 키 선택, 최종 링크·map·WRP, 제조 provisioning, boot handoff, T-205 floor는 이번 완료 범위가 아니다. Physical/HIL: NOT_RUN. Vehicle TX: NO-GO. Diagnostic Bridge: read-only.

CONDITIONAL 조건은 `B-BOOT-TRUST-01`의 수정·회귀검증 또는 근거 있는 disposition이다. 현재 보고서는 PASS나 전체 task/merge 승인으로 사용할 수 없다.
