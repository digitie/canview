# T-007 최종 소프트웨어 수용 감사 — Reviewer B / Huygens

## 1. 실행 식별·격리

- Execution UUID: `d8856588-8ebd-4b0e-bf86-403566ed842f`
- 시작 UTC: `2026-09-19T06:19:03.7447900Z`
- 종료 UTC: `2026-09-19T06:28:02.1725099Z`
- 전문범위: schema·packager·native 검증, 시험 oracle·변이, SDK/CI 연결, 산출물 재현성과 근거 귀속.
- 저장소: `F:/dev/canview-wt/t007-ota-container`
- 실제 확인 PR base: `6cf1b8e57840a27b83c407d1325a92f869cf2f5d`
- 실제 확인 candidate: `77b84cf07cd868f0f0b858a00a0ae8c9b3eb9ef4`

시작·종료에 두 commit의 `git cat-file -e "<hash>^{commit}"`와 `git rev-parse "<hash>^{commit}"`가 성공했고, 반환 hash가 동일했다.

저장소 소스는 고정 commit의 `git show`와 commit 간 `git diff`로만 읽었다. Moving worktree의 내용·HEAD·clean 상태를 candidate 증거로 사용하지 않았다. 파일 수정, 시험·컴파일, commit/push, 다른 agent 호출, 상대 reviewer 원문·새 finding 조회는 하지 않았다.

적용 skill: `embedded-cstyle`, `embedded-architecture`, `embedded-documentation`. C 계약·책임 경계·문서 정합성 검토에 사용했으며 자동 수정은 하지 않았다.

## 2. 결론

**Verdict: CONDITIONAL**

T-007에 귀속되는 소프트웨어 계약·구현·등록 시험 구성에서는 추가 필수 구현 누락이나 새로운 P0–P3 결함을 확인하지 못했다. AC3도 이제 writer 부재만을 근거로 한 공허한 “0회”가 아니라, 실제 C stage와 정상 호출 양성 대조·실패 주입·변이 oracle로 뒷받침된다.

다만 **77b84cf의 최신 CI 및 실제 target 산출물 귀속 감사는 미검증**이다. 따라서 전체 수용 완료, Task DONE, PR36 merge를 승인하지 않는다. 아래 “충족”은 직접 읽은 소프트웨어 구현·시험 계약에 대한 정적 판정이며, 본인이 시험을 실행해 PASS했다는 뜻이 아니다.

## 3. 수용 기준별 대응

정본은 T-007 `:174–203`, `:219–225`, architecture/ota `:311–325`다. T-204/T-107의 실제 writer와 T-205의 영속 정책 완성품을 T-007 선행으로 되돌리지 않았다.

| 기준 | 직접 확인한 구현·시험 근거 | 판정 |
|---|---|---|
| AC1: wrong identity/key/signature, unknown field·duplicate key·과도한 중첩·절단 거절 | `manifest.c:239–296,370–409`의 정확한 필드·identity 검사, `envelope.c:145–207`의 prefix·서명 검사, `cbor_document.c:98–203`의 key 순서·중복·깊이·item 상한. Python 대응 구현과 `test_manifest.py`, `test_cbor_document.py:23–128`, `test_envelope_signature.py:88–146`의 독립 기대값·C 교차 대조 확인. | **충족 — 정적 구현/시험 구성.** 독립 실행 NOT_RUN. |
| AC2: 정수·blob 배치·중복 target·서명/header·zero/초과 길이의 C/Python 일치 | `manifest.c:299–367`과 `manifest.py`가 역할별 target·길이·중복·계산 offset·총길이를 검사한다. 외부 offset 자체를 받지 않아 임의 겹침을 표현할 수 없으며, padding/길이 변조는 `body.c:119–184`, `container.py:45–60`에서 거절한다. `test_manifest.py`, `test_container.py:67–172`, `test_body.py:58–121,187–235`에서 경계·절단·배치 변이를 대조한다. | **충족 — 정적 구현/시험 구성.** |
| AC3: 사전검증 전 erase 금지, 허용 target만 수신, 전체 검증 전 설치 권한 금지 | `body.c:67–85`의 manifest preflight→floor→hash 순서 뒤에만 `stage.c:45–54`의 begin이 실행된다. `stage.c:68–73`은 feed 성공 뒤 write, `:85–91`은 전체 body 성공 뒤 native callback을 호출한다. `stage_probe.c:123–164,196–276`에 정상 begin/write/verify 양성 대조와 실패 시 호출·상태 검사가 있다. `test_stage.py:45–54`의 금지 enum/identity, `check_stage_oracle.py:24–49`의 다섯 실제 C 변이 검출 구성도 확인했다. | **T-007 수신 계약 범위 충족.** 실제 Flash allowlist/read-back·PREPARED/selector 영속 enforcement는 후속 owner gate이며 미검증. |
| AC4: signed sequence:u64 보존·native image 불일치 거절 | C는 `manifest.c:188–189`, `native_metadata.c:58–68`, `floor.c:71–92`에서 u64를 직접 사용한다. Python은 `native.py:43–49`에서 정수 그대로 native metadata를 대조한다. `test_manifest_json.py:75–132`, `test_native_metadata.c:84–91`, `test_native_container.py:119–188`의 2^53 경계·u64 최대·불일치 시험 확인. | **충족 — 정적 구현/시험 구성.** |
| AC5: 공식 도구 pin·golden digest·secret 없는 clean-host 합성 검증 | `native.py:57,94–104`의 esptool version/MCUboot clean commit 검사, dependency hash lock과 workflow `:25–34` 확인. `generate_signed_golden.py`는 메모리 시험키·공식 signing 도구를 사용하고 공개 자료만 저장한다. `test_signed_golden.py:129–158`은 provenance·digest·재조립을 검사한다. Golden blob 크기/SHA는 이번 감사에서 직접 확인했다. | **구현·보존 fixture 충족.** Clean-host 실행 결과와 최신 CI의 독립 귀속 확인은 **미검증**. |

### AC3 판정의 정확한 경계

`stage.h:8–19`는 begin의 비활성 slot/staging 대응, 보호 영역 거절, write/read-back, native 검증 및 callback의 설치·journal 변경 금지를 명시한다. `stage.h:31–43`은 `NATIVE_MATCHED`를 설치 권한과 분리한다.

이번 충족 판단은 다음 근거를 합친 것이다.

- 실제 parser/preflight/floor 실패가 begin 이전에 단락된다.
- 정상 begin/write가 실제로 실행되는 시험을 기준으로 실패 경로를 대조한다.
- Native 실패는 `NATIVE_MATCHED`로 승격되지 않는다.
- 조기 begin·거절 후 write·native 오류 무시 변이를 검출하도록 시험이 구성돼 있다.

**PREPARED/selector API가 없다는 사실만으로 실제 저장장치 변경 0회를 시험했다고 주장하지 않는다.** 실제 주소 보호·영속 승인 경계는 T-204 `:19–23,49–51`, T-107 `:19–22,41–46`, T-205 `:18–21,43–50`의 연결 시험으로 남는다.

## 4. 구현 범위·target gate 대응

| 구현 범위 | 근거 | 판정 |
|---|---|---|
| 컨테이너 schema·C/Python parser | `schema/cvota-v2.schema.json`, `protocol/schema/ota-container-v2.yaml`, C/Python CBOR·envelope·manifest. `test_manifest_json.py:43–73`의 schema/wire/C enum drift 검사. | **충족** |
| Enum→target map | `manifest.h:14–35`, `manifest.c:196–204,317–329`, `stage_probe.c:132–138`; 임의 주소·경로 target 없음. | **충족**, 물리 주소 선택은 후속 BSP writer 책임 |
| Manifest 16KiB·image 수/길이·overflow·역할 조합 | CBOR 상한, `envelope.c:166–195`, `manifest.c:269–275,299–367`, Python 대응 검사. | **충족** |
| CLI 조립·검사·출력 보존 | `container.py:78–136`: bounded read, 외부 root, 검증 후 `xb` 생성. `test_native_container.py:190–248`의 실패 시 미생성/기존 파일 보존. | **충족** |
| ESP/STM signing 연결점·일반 native 검증 | `native.py:52–160`, 공식 도구를 사용하는 golden generator. ESP는 같은 block의 scheme·외부 key·signature·canonical bytes를 결합하고 STM은 고정 profile 뒤 공식 verifier를 호출한다. | **충족**. 제품 키 생성/배포 서비스는 요구하지 않음 |
| 동일 manifest bytes·signed golden | 정규 CBOR 인코딩, detached signature, `test_signed_golden.py:155`의 정확한 재조립. 신규 키/RSA-PSS로 재생성할 때 동일 서명 bytes까지 재현한다고 주장하지 않는다. | **충족** |
| Streaming 소유권·수명·부분 입력/reset | `envelope.h:49–87`, `body.h:36–103`, `stage.h:40–95`; `test_prefix_stream.c`, `body_probe.c:284–330,349–446`, stage 수명 시험. | **충족** |
| Allocation/CPU 정적 예산 | `shared/ota/README.md:325–353`의 객체 크기·자체 frame·연산 상한·후속 실측 구분. C core는 고정 메모리이며 SDK 내부 할당은 별도다. | **정적 기록 충족**, 전체 SDK heap/stack/WCET 미검증 |
| 전체 parser+PSA read-only target 조합 | IDF parser component `:3–11`, fixture main CMake `:1–8`, receiver `:39–102`, main `:44–51`. 기존 C parser/body와 실제 PSA provider가 동일 receiver 경로에 연결된다. | **구현 충족**. 실제 장치 실행 NOT_RUN |
| 실제 SDK 및 정상 4 target build gate | Workflow `:287–335`의 STM Debug/Release·ESP 세 project·SDK fixture build, `:336–417`의 산출물 hash/source binding. | **등록 구성 충족 / 77b 최종 실행·artifact 감사 미검증** |

정적 자원 산술도 일치한다.

- Receiver: `16488 + 856 + 108 = 17452B`
- 향후 stage 조합: `16488 + 896 + 108 = 17492B`
- 선택적 16KiB chunk 포함: `33876B`

Stage 안에 포함된 body를 중복 합산하지 않았다. 이 수치나 fixture stack 예약 16384B를 전체 RAM·호출 chain stack·WCET 보증으로 해석하지 않았다.

### 기존 지적의 현재 판정

- **B-STAGE-01: FIXED — 정적 재확인.** `stage_probe.c:323–326` 양성 대조, `:338,360,366,371`의 reset 전 body EMPTY 검사, `check_stage_oracle.py:30–31`의 hash-context/identity guard 변이가 기존 동일 오류 masking을 구별한다. 이번 실행에서 mutant를 재실행한 것은 아니다.
- **일반 native 미구현 B-T007-AUD-02: FIXED — 구현 범위.** 합성 generator만이 아니라 일반 `container.py --native`에서 공식 verifier를 호출하며, 다른 identity·실패·출력 보존 시험이 등록돼 있다.
- **소유권 충돌 B-T007-AUD-01: FIXED.** T-007 `:185–203`은 상위 architecture 및 기존 후속 task에 책임을 대응하면서 AC3 연결 시험을 유지한다.
- **정적 예산 기록 부족 B-T007-AUD-03: FIXED — 기록 범위.** 실제 장치 실측 완료를 뜻하지 않는다.
- **Old A-AC03:** B 관점에서도 **전체 parser+PSA read-only 조합의 소프트웨어 미연결 문제는 해소**됐다. `main.c:23–26`의 stage NULL-link 시험과 `receiver.c`의 실제 전체 수신 경로를 혼동하지 않았다. 최신 target artifact 확정과 device 실행까지 FIXED/PASS로 확대하지 않는다.

## 5. Findings·공격 범위

- P0: 없음.
- P1: 없음.
- P2: 없음.
- P3: 없음.

신규 결함으로 확정할 정확한 실패 위치·시나리오는 발견하지 못했다. 최신 CI 대기는 구현 결함으로 발명하지 않고 미완료 검증 gate로 분류한다.

실제로 공격한 관점은 다음과 같다.

- Unknown/duplicate 필드, 비정규 CBOR·깊이/item/byte 상한, 정수 overflow·u64 반올림.
- Header 주장과 서명된 길이 불일치, zero padding·잘림·중복 target·잘못된 역할 조합.
- Outer-valid/native-invalid, ESP CRC 재계산 변조·다른 block 간 key/signature 혼합, STM hash 갱신 후 원 서명 유지.
- 잘못된 root·도구 version·dirty checkout·timeout·도구 오류가 성공으로 바뀌는 경로.
- 검증 전 출력 생성·기존 출력 덮어쓰기.
- Stage 사전 begin·거절 후 write·native 실패 무시, 중첩 시험의 다른 오류에 의한 masking.
- Receiver 조기 AUTH_FAILED를 마지막 byte 변조 검출로 오인하는 경로.
- 모형/CNG/PSA target build/device 실행의 혼동, 이전 commit CI·coverage의 현재 귀속 오인.
- Binary line-ending 변환, golden drift, 정적 자원 중복 합산과 전체 실측으로의 과장.

## 6. 실제 열람·명령 기록

주요 직접 열람 범위는 다음과 같다. `전문`은 해당 파일 전체를 뜻한다.

- 정본: `AGENTS.md`, `docs/README.md` 전문; `docs/resume.md:1–168`; T-007 전문; `docs/architecture/ota.md` §7, §8·§9 관련 승인/호환성 경계, §12; ADR009 전문; `docs/runbooks/agent-workflow.md:1–60,63–190`; T-204/T-107/T-205 전문.
- Schema/host: 두 schema 전문; `tools/ota/{cbor.py,envelope.py,manifest.py,manifest_json.py,container.py,native.py}` 전문; `tools/toolchain-versions.json`, OTA/native/build lock 전문.
- C: `shared/ota/src/{cbor_head.c,cbor_document.c,envelope.c,manifest.c,body.c,floor.c,native_metadata.c,native_stm.c,stage.c}` 전문; `envelope.h,manifest.h,body.h,native_metadata.h,native_stm.h,stage.h` 전문.
- SDK/BSP: `firmware/platform/esp32s3/ota_crypto.{c,h}`, `ota_image.c`, `firmware/communicator/esp32/bsp/ota.c` 전문.
- 핵심 시험: `tests/ota/{test_manifest.py,test_manifest_json.py,test_cbor_document.py,test_envelope_signature.py,test_container.py,test_native_container.py,test_body.py,test_stage.py,test_signed_golden.py,generate_signed_golden.py,check_stage_oracle.py,check_receiver_oracle.py,check_sdk_metadata.py}` 전문.
- C 시험/probe: `stage_probe.c:1–406`, `body_probe.c:1–448`, `native_stm_probe.c:1–140`, `manifest_probe.c:1–170`, `envelope_crypto_probe.c:1–89`, `cng_provider.c:1–133`, `test_prefix_stream.c:1–114`, `test_esp_ota_crypto.c:1–222`, `psa_crypto_fixture.h:1–44`, `test_comm_ota.c:1–109`, `test_esp_image_sdk.c:1–161`, `test_native_metadata.c:1–135`, `idf_receiver_host.c:1–20`.
- Fixture/build: IDF fixture `main/{main.c,metadata.c,receiver.c,CMakeLists.txt}`, root CMake/sdkconfig/README 전문; golden README/provenance 전문 및 binary blob; root CMake의 OTA 등록부 `:55–207`; shared/component CMake 전문; 정상 네 firmware project CMake 전문; workflow `:1–454`; `.gitattributes` 전문.
- 문서/근거: `shared/ota/README.md` schema·CLI·stage·예산·native 관련 절, `docs/journal.md:1–101`; development foundation 및 T103 합성 fixture/helper 변경 diff.

실제 사용 명령 계열:

```text
git cat-file -e "<base 또는 candidate>^{commit}"
git rev-parse "<base 또는 candidate>^{commit}"
git diff --find-renames --stat <base> <candidate> ...
git diff --find-renames --name-status <base> <candidate> ...
git diff --find-renames [--unified=2] <base> <candidate> -- <관련 경로>
git show <candidate>:<위 열람 경로>
git cat-file -s <candidate>:tests/fixtures/ota-signed-golden/communicator.cvota
```

`git show` 출력에 PowerShell 행 번호·범위 필터를 적용했다. Golden hash는 `git show`의 바이너리 stdout을 `.NET SHA256.ComputeHash`로 직접 계산했다. 파일 추출·저장은 하지 않았다. Skill 원문만 외부 skill 경로에서 `Get-Content`로 읽었다.

직접 확인한 golden:

```text
length = 394310
sha256 = 68eb18e10bf35d351c1604500bf85f6e95aa41c6b49477ffbbdacd9477902655
```

존재하지 않는 `tests/ota/test_envelope.py` 조회는 성공한 열람으로 집계하지 않았다. 이후 실제 CMake 등록의 `test_envelope_signature.py`와 C envelope 구현을 확인했다.

## 7. 근거 한계·최소 다음 작업

이번 감사는 **핵심 소스 열람 기준 incomplete는 아니다.** 다만 실행 증거 감사는 다음 한계가 있다.

- 시험·컴파일·CLI·mutant 실행: **본인 NOT_RUN**.
- Debug/Release150/150, stage ASan/UBSan27그룹, 기존 coverage와 실제 SDK 경고0은 **작성자 제공/기록 근거**이며 본인 실행 결과가 아니다.
- CI `35426085834`의 현재 상태·로그·artifact를 직접 조회하지 않았다.
- 569/82c의 CI·Linux 결과를 77b 전체 PASS로 재사용하지 않았다.
- 공식 SDK/암호 라이브러리 내부 수학 전체, 설치된 dependency bytes, 현재 ELF DWARF·`.su` 원본, 모든 과거 시험·리뷰 원문은 재감사하지 않았다.
- 정상 firmware 전체 기능, 실제 writer·provisioning·영속 policy 구현은 이번 T-007 소프트웨어 경계 밖이다.

최소 다음 작업:

1. **77b84cf에 귀속된 최신 CI/target 증거를 확정한다.** Windows Debug/Release, GNU/Clang/sanitizer, SDK fixture 및 정상 네 target 결과와 경고·오류를 확인한다. Workflow `:336–417`에 따라 산출물 bytes/SHA256, sourceRevision/expectedSourceRevision, source provenance를 실제 내려받은 파일과 대조한다.
2. 독립 A/B 최종 결과가 모두 확정된 뒤 AC별 실행 근거와 미실행 경계를 대응해 수용 판정을 기록한다. 실패가 없다면 이번 B 감사가 요구하는 추가 프레임워크나 신규 소프트웨어 기능은 없다.
3. 실제 Flash map/read-back·보호 영역, 영속 승인·selector·floor, 장치 자원 qualification은 각각 T-204/T-107/T-205/T-508에서 연결·재검증한다. 이를 T-007의 순환 선행 조건으로 만들지 않는다.

**Physical/device/Flash/HIL 및 전체 runtime heap·stack·WCET: NOT_RUN. Vehicle TX: NO-GO. PR36 Draft 유지. 최종 판정 CONDITIONAL은 최신 증거 확정 전의 소프트웨어 수용 감사 결과이며, 전체 T-007 완료 또는 merge 승인이 아니다.**
