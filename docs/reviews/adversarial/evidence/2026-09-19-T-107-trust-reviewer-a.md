# T-107 BSP trust Reviewer A 원본

- Agent execution ID: `01a0b94f-d65f-7ab0-90ae-b43a0d4971a8` (Ramanujan)
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

A 전문: embedded C/runtime 신뢰 경계, 메모리 bounds, 출력불변, const/수명, BSP board/role/ABI 구별, MCUboot key ABI와 false security claim.

## Raw report

판정: PASS — T-107 BSP boot trust의 지정된 중간 source review 범위에 한정한다. P0/P1/P2/P3 finding은 각각 0건이다. 전체 T-107 완료·제품 부팅·merge 승인이 아니다.

executionID: `T107-BOOT-TRUST-A-20260919T105712Z`
UTC 시작: `2026-09-19 10:57:12`
UTC 종료: `2026-09-19 11:00:03`
전문 범위: embedded C/runtime 신뢰 경계, 메모리 bounds, 오류 출력 불변, const·수명, BSP identity 구별, MCUboot 공개키 ABI, 과장된 보안·완료 주장.

실제 확인한 기준선:

```text
repository: F:/dev/canview-wt/t007-ota-container
candidate: 6c14950d89bfe01f216dea33b58933dfc9e445e4
base:      06383d6cf9aa8b0c68646f96b3507e2144ca074e
MCUboot:   6d3b3d2c38ab20c242e5b9abb04d050086383eb2
SDK 저장소: C:/cv/mcuboot-2.4.0
```

격리는 commit object-only다. 저장소 내용은 고정 hash의 `git show`와 base→candidate `git diff`로 읽었다. SDK도 고정 commit 객체로 읽었다. 움직이는 worktree 파일·빌드 산출물에 의존하지 않았다. worktree/SDK checkout clean 여부는 **N/A·미검사**이며 clean이라고 주장하지 않는다. 변경·commit·PR·merge·시험 실행은 하지 않았다. 전달받은 사용자 manifest를 기준으로 A만 검토했고 B의 결과를 받거나 참조하지 않았다.

실제 읽은 delta는 다음 14개 전부다. 기존 파일은 변경 hunk와 필요한 주변 문맥, 새 파일은 전체를 읽었다.

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

추가로 읽은 계약·참조 파일:

```text
AGENTS.md
docs/README.md
docs/tasks/T-107-stm32-mcuboot.md
docs/runbooks/agent-workflow.md
docs/reviews/README.md
docs/architecture/README.md
docs/architecture/ota.md — §6 및 인접 보안 계약
firmware/communicator/stm32/bootloader/include/canview_boot_identity.h
firmware/communicator/stm32/bootloader/include/mcuboot_config/mcuboot_config.h
firmware/communicator/stm32/bootloader/image_hooks.c
firmware/communicator/stm32/bootloader/mcuboot_compat.cmake
shared/ota/src/manifest.h
shared/ota/src/native_metadata.h
shared/ota/src/native_metadata.c
tools/toolchain-versions.json

고정 MCUboot 객체:
boot/bootutil/include/bootutil/sign_key.h
boot/bootutil/src/image_ecdsa.c
boot/bootutil/include/bootutil/crypto/ecdsa.h — 적용되는 TinyCrypt 경로 중심
boot/bootutil/src/image_validate.c — key 선택·index bounds 관련 절
```

`embedded-cstyle`와 `embedded-driver-design`의 SKILL.md를 읽고 const·포인터·고정 메모리·BSP 책임 경계에 적용했다. 자동 수정은 수행하지 않았다.

실제 명령은 아래 형식이며 `C`, `B`, `S`는 위의 실제 전체 hash다.

```text
git cat-file -e 'C^{commit}'
git cat-file -e 'B^{commit}'
git rev-parse C
git rev-parse B
git rev-parse 'C^{commit}'
git rev-parse 'B^{commit}'
git diff --name-status B C
git diff --find-renames --unified=5 B C -- <위 delta 파일 묶음>
git diff --find-renames B C -- <새 시험 파일 3개>
git show C:<위 저장소 파일>

SDK 저장소:
git cat-file -e 'S^{commit}'
git rev-parse 'S^{commit}'
git show S:<위 MCUboot 파일>
```

출력 절 선택에는 `Select-Object`·`Select-String`, line number에는 `ForEach-Object`를 사용했다. 최초 `cat-file` 두 호출은 인용하지 않은 `^{commit}`의 PowerShell 전달 오류로 실패했고, 작은따옴표로 수정한 재호출에서 두 객체 존재를 확인했다. 종료 시에도 candidate/base를 다시 확인했다.

심각도별 결과와 공격한 시나리오:

- **P0: 없음.** `firmware/communicator/stm32/bsp/boot_identity.c:9`, `:12`, `:20`에서 이미지가 기대 identity·키를 덮어쓰거나 null 오류가 부분 출력을 만드는 경로를 추적했다. 원본은 static const이고 두 출력 검사 후에만 복사한다. `shared/ota/src/manifest.h:98`의 문자열은 64B 내장 배열이므로 복사 결과 수정이 BSP 원본에 전파되지 않는다. 현재 호출자는 별도 identity와 ABI 저장소를 사용한다. heap·보존된 caller pointer·가변 전역 상태도 추가되지 않았다.

- **P1: 없음.** `tools/ota/generate_stm32_boot_trust.py:16`, `:25`, `:60`에서 음수·overflow·비정수·누락, 잘린/추가된 DER, 다른 curve·private key 입력을 추적했다. 명시적 u32 검사, 최대 92B 읽기, 91B 길이·P-256·SPKI 재직렬화 일치 검사 뒤에만 출력한다. `firmware/communicator/stm32/bootloader/trust.cmake:6`은 부분 설정을 거절하며, `:34`의 전체 미설정은 identity target을 만들지 않는다. 제품 기본 시험키로 대체하는 경로는 없다.
  
  `boot_identity.c:10`의 Communicator role·전체 OTA board·layout은 `docs/architecture/ota.md:137`과 일치한다. MCU pin profile을 OTA board로 사용하지 않는다. `boot_identity.c:14`의 `unsigned int` 길이와 const export는 pinned SDK `sign_key.h:35`, `:65`의 실제 ABI와 일치한다.
  
  `image_hooks.c:37`, `:93`, `:112`는 로컬 identity/ABI를 대조하고 성공 시 `FIH_BOOT_HOOK_REGULAR`로 원래 서명 검증을 이어간다. manifest root ID를 MCUboot key index로 사용하는 경로도 확인되지 않았다.

- **P2: 없음.** `trust.cmake:20`, `:26`, `:28`에서 키/생성기 의존성, 생성 header와 BSP compile 연결을 검토했다. 제조 상수는 생성 명령 인자에 포함된다. `tests/ota/test_stm32_boot_trust.py:116`, `:119`, `:122`는 Ninja build에서 독립 입력 DER·상수와 C 출력을 대조하고 기존 build의 키·상수 교체를 검사한다. `:91`은 잘못된 입력 후 기존 생성 출력 보존을 검사한다. `tests/ota/test_stm32_boot_identity.c:19`, `:31`은 null 출력 불변과 반복 복사 격리를 검사한다. 이 시험들은 **source를 검토했으며 직접 실행하지 않았다.**

- **P3: 없음.** `firmware/communicator/stm32/bootloader/README.md:114`, `:118`, `:126`, `docs/journal.md:18`, `:23`, `:31`에서 미설정·제조 승인·최종 링크·시험 범위가 완료로 오인될 수 있는지 검토했다. archive compile과 부트로더 final binary를 구분하며, 장시간 모형 시험의 이전 실행과 이번 실행도 구분한다. capture fixture 변경은 digest 교체이고 synthetic 표기를 유지한다.

구체적인 실패 시나리오가 입증된 finding이 없으므로 수정 권고는 없다. 위 미구현 gate는 이번 source PASS로 닫지 않는다.

기존 검증에 관해서는 `docs/journal.md:13` 이후의 **작성자 기록**만 확인했다. 관련 host Debug/Release 각각 157/157, 새 BSP ASan/UBSan 및 coverage 100%, 실제 Arm primary Debug/Release identity archive와 기존 앱, 입력 negative·변이·증분·GNU 시험이 기록돼 있다. 시험 CMake의 C99 설정도 확인했다. 원시 로그·coverage 파일·ELF/archive를 독립 감사하거나 reviewer가 재실행한 결과가 아니다.

미검토·미실행 범위는 제품 전체 키 선택, 최종 bootloader 링크와 map/WRP 배치, 제조 입력의 진위·승인과 provisioning, 실제 boot handoff, T-205 floor, 실제 Flash/ECC·전원 차단·watchdog 시간, 전체 source digest 재계산, CI/산출물 감사다.

**Physical/HIL: NOT_RUN. 차량 TX: NO-GO. Diagnostic Bridge: read-only 유지. 최종 판정: 지정된 중간 source 범위 PASS.**
