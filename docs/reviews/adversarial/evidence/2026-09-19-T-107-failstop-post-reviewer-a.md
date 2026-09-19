# T-107 fail-stop Reviewer A post-fix 원본

## 전달 입력

````text
원 failstop reviewer post-fix 재검토 요청. 두 원본 raw report를 서로 비교하기 전에 각각 main workspace docs/reviews/adversarial/evidence/2026-09-19-T-107-failstop-reviewer-{a,b}.md에 그대로 보존했습니다. 초기 candidate6b394ac7effbdc14e901b363a1594d4bf89b34fa/basee9f474b 그대로역사보존. post-fix candidate5a3790773645b2cc30747ccb44f99732978171bf. 자신의 기존detachedreviewtree에서 git status --porcelain=v1 비었음확인 후 git switch --detach 5a3790773645b2cc30747ccb44f99732978171bf, HEAD/clean확인해재검토. 이전 ignored evidence보존, trackededit금지.
A-FAILSTOP-01/B-FAILSTOP-01 P2: 조건부panic 및 mov/pop PC조기복귀검사빈틈. coordinator는조건부bne도재현해f4c938b에서1차보강했으나 now5a는고정 GCC Debug/Release wrapper의전체 instruction prefix+최종무조건BL/실제panic주소를 allowlist로검사. 실제Armassembler/ELF 2positive +8negative(각wrapper조건부beq/movPC/popPC/bxLR),48mnemonic변이,missingwrap2actualLinknegative. 원각자전체mutant ELF가신규checker에거절되는지도확인.제품C불변.
B-FAILSTOP-02 P2: CItrust누락수정. tests/ota/make_stm32_test_public_key.py가매실행private는메모리에만두고일회PUBLIC DER91B만출력(xb덮어쓰기거절). CIprimary두configure에4명시시험상수와cryptoPython을전달,별도 --full-boot 검사2회를명시호출하여trust누락이skip될수없음. artifact manifest추가 scope/publichash/constants/physicalNOT_RUN. no-default-production key원칙유지. 실제CI결과는아직대기이므로정적/로컬수정확인과원격CI수용조건구분.
delta git diff 6b394ac..5a37907(중간handoffCI원문closure외이번관련source전체). 각finding FIXED/OPEN직접판정,한국어rawpostreport ID/UTC/HEADbase/clean/실제파일명령/미검토영역/verdict 반환. 서로raw원본보존돼독립재검토후종합예정.
coordinator 실행: Host D/R 전체161/161 378.65s/135.37s;중간host회귀는새pythonchecker수정전source6b394ac,제품C이후불변. post-fix 실제ArmD/R 5/5 및 CI동일testpublic/4trust/cryptoPython configure+build+explicitfull검사 D/R각5/5 warning0. no-trust새build도SRAM-onlyPASS하지만FULL NOT_RUN 구분. YAMLutf8parsePASS(최초cp949read실패는수정후재실행),targetCI PowerShellASTparsePASS.원격CI성공으로대체하지말것.재검토후T107전체/PRmerge승인금지,physical/HIL NOT_RUN.
역할 A 재검토 executionID T107-FAILSTOP-A-POST-5a37907-20260919.
````

## 원본 결과

````markdown
판정: **PASS — 이번 post-fix source·로컬 검증 범위**. 원 A finding은 FIXED이며 신규 finding은 없습니다. 원격 CI 성공·artifact 수용, T107 전체 완료 및 PR37 merge는 승인하지 않습니다.

- executionID: `T107-FAILSTOP-A-POST-5a37907-20260919`
- 전문영역: embedded runtime, ABI·noreturn, FIH 보존, watchdog/error path, 메모리 안전·부작용
- UTC 시작: `2026-09-19T13:32:09.6904612Z`
- UTC 종료: `2026-09-19T13:35:59.5151280Z`
- post-fix candidate: `5a3790773645b2cc30747ccb44f99732978171bf`
- 재검토 base: `6b394ac7effbdc14e901b363a1594d4bf89b34fa`
- 최초 역사 base: `e9f474b133912814aad255add3452aa0eb2fd44b`
- 격리: `F:/dev/canview-wt/review-t107-failstop-a`

전환 전 clean을 확인하고 `git switch --detach 5a3790773645b2cc30747ccb44f99732978171bf`를 실행했습니다. 전환 후·종료 시 HEAD 일치, porcelain 출력 없음, tracked/staged diff 없음입니다. 기존 ignored evidence는 보존했고 새 자료도 자신의 ignored `build/`에만 생성했습니다. tracked 수정·commit은 없습니다. 다른 reviewer의 raw 보고서는 열지 않았습니다.

| finding | 판정 | 직접 확인한 근거 |
|---|---|---|
| A-FAILSTOP-01 P2 | **FIXED** | 원 A 전체 mutant ELF를 새 검사기가 거절 |
| B-FAILSTOP-01 P2 | **FIXED — 동일 결함의 A 확인** | 조건부 panic·mov/pop PC·bx LR를 양 wrapper에서 거절하는 실제 Arm 시험 통과 |
| B-FAILSTOP-02 P2 | **FIXED — source·로컬 범위** | CI 명시 trust 전달·강제 full 검사 확인, 동일 입력의 로컬 Arm 검증 통과. 원격 CI 수용은 미확인 |

A-FAILSTOP-01의 수정 위치는 [check_stm32_boot_ram.py:116](/F:/dev/canview-wt/review-t107-failstop-a/tools/ota/check_stm32_boot_ram.py:116)입니다. Debug/Release instruction prefix 전체와 마지막 무조건 BL을 제한하고, 표시된 분기 목적지를 실제 `fih_panic_loop` symbol 주소와 대조합니다. 이전의 단순 분기 개수 검사에서 빠졌던 조건부 진입·조기 복귀가 허용되지 않습니다.

보존된 `build/conditional-full-boot.elf`에 새 `inspect(..., full_boot=True)`를 적용한 결과, 정확히 `newlib wrapper의 FIH panic 경로 오류`로 거절됐습니다. 원 ELF의 SHA256은 전후 동일합니다.

```text
25cdd5163ee51d4bf11caeb437c699ea78d287ba28ae4a769c3e346552e84caa
```

원 mutant BIN과 최초 false-PASS 로그의 hash도 유지됐습니다. [재확인 로그](/F:/dev/canview-wt/review-t107-failstop-a/build/post-verify.log)

B-FAILSTOP-02의 수정은 [foundation.yml:305](/F:/dev/canview-wt/review-t107-failstop-a/.github/workflows/foundation.yml:305)에서 확인했습니다. 두 primary configure에 공개 DER·epoch `0`·manifest key ID `4294967295`·STM ABI `2`와 암호 라이브러리가 설치된 Python을 전달합니다. 별도의 두 `--full-boot` 호출은 SRAM-only 결과가 skip으로 성공 처리되는 것을 막습니다. 보존된 no-trust ELF에 강제 full 검사를 적용하여 `MCUboot/fail-stop link closure 누락` 거절도 직접 확인했습니다.

[공개키 생성기:13](/F:/dev/canview-wt/review-t107-failstop-a/tests/ota/make_stm32_test_public_key.py:13)는 개인키 직렬화 없이 공개키만 출력합니다. 독립 실행 두 번에서 서로 다른 P-256 SPKI DER 91B를 확인했고, 같은 경로 재실행은 `FileExistsError`로 실패하며 기존 bytes를 보존했습니다. 제조 기본값은 추가되지 않았고, manifest는 비배포 scope·공개키 hash·시험 상수·physical `NOT_RUN`을 기록합니다.

독립 실행 결과:

| 검증 | 결과 |
|---|---|
| 새 시험 공개키·명시 trust로 실제 Arm Debug | build 및 별도 full 검사 각 5/5 PASS |
| 동일 조건 Arm Release | build 및 별도 full 검사 각 5/5 PASS |
| 실제 Arm assembler/ELF | 2 positive·8 negative PASS |
| 조건부·간접 분기 mnemonic 변이 | 48개 거절 PASS |
| missing-wrap 실제 link negative | 두 구성에서 각각 2개 PASS |
| 기존 Flash SRAM 검사 | 두 구성 각각 4/4 PASS |
| 원 A 전체 mutant ELF | 거절 PASS |
| SRAM-only ELF의 강제 full 검사 | 거절 PASS |
| UTF-8 YAML / target job PowerShell AST | PASS / 3개 script PASS |
| 직접 캡처한 build/test warning·error | 0 |

독립 산출물은 Debug BIN 37,040B / RAM 12,264B, Release BIN 28,976B / RAM 12,040B입니다. CPU에서 실행한 결과가 아니라 실제 Arm compile·link·ELF 검사 결과입니다.

실제 읽은 범위는 `6b394ac..5a37907`의 관련 source delta 전체와 해당 원본입니다.

```text
.github/workflows/foundation.yml
tests/ota/make_stm32_test_public_key.py
tests/ota/test_stm32_boot_ram.py
tools/ota/check_stm32_boot_ram.py
```

추가로 `AGENTS.md`, `docs/README.md`, `docs/resume.md`, T107 상세 task, 기존 `trust.cmake`, `fail_stop.c/h`, 생성된 CMakeCache와 Arm disassembly를 읽었습니다. 이전에 읽은 workflow·Windows 환경·embedded-cstyle/architecture skill의 검토 기준을 유지했습니다. 제품 C·firmware/shared·기존 CMake 경계의 post-fix diff가 없음을 확인했으며, ABI·FIH·host libc 동작 변경은 없습니다. 중간 handoff CI closure 문서 3개는 이번 재검토에서 제외했습니다.

주요 실행 명령은 다음과 같습니다.

```text
git status --porcelain=v1
git switch --detach 5a3790773645b2cc30747ccb44f99732978171bf
git diff --find-renames 6b394ac... 5a37907... -- <관련 source 4파일>
git diff --check 6b394ac 5a37907
python -X utf8 -B build/post-verify.py
./build/post-arm.ps1
cmake --build build/post-Arm-{Debug,Release} --target canview_boot_ram_link_check --clean-first -j 6
python -X utf8 -B tests/ota/test_stm32_boot_ram.py --full-boot --elf <각 ELF> --compiler <고정 Arm GCC> -v
python -X utf8 -B build/post-ci.py
arm-none-eabi-objdump -d --disassemble=<wrapper> <ELF>
```

정확한 configure 인자는 [실행 스크립트](/F:/dev/canview-wt/review-t107-failstop-a/build/post-arm.ps1)에 있습니다. native 출력의 transcript 누락을 발견하여 직접 캡처로 재실행했고, 최종 근거는 [Debug build](/F:/dev/canview-wt/review-t107-failstop-a/build/post-Debug-build.log), [Release build](/F:/dev/canview-wt/review-t107-failstop-a/build/post-Release-build.log), [Debug full 검사](/F:/dev/canview-wt/review-t107-failstop-a/build/post-Debug-test.log), [Release full 검사](/F:/dev/canview-wt/review-t107-failstop-a/build/post-Release-test.log)입니다.

신규 P0/P1/P2/P3: 각각 0건. 원 A finding OPEN: 0건.

미실행 범위는 원격 CI 및 artifact 감사, 전체 target job·primary 앱 빌드, host 161개 회귀 재실행, sanitizer/coverage, 실제 boot_go·정책·recovery 통합과 물리 시험입니다. coordinator의 실행 수치를 제 독립 실행 결과로 전환하지 않았습니다. MCUboot SDK는 고정 commit과 clean 상태를 유지했습니다.

**Physical/HIL: NOT_RUN. Production·vehicle TX: NO-GO. T107 전체 및 PR merge: 미승인.**
````
