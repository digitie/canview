# T-007 전체 수용 기준 완료 감사 — Reviewer B 원문

## 1. 실행·격리

- Execution UUID: `57e75399-5b5c-4e7b-9dd9-2b68f7d073f2`
- 시작 UTC: `2026-09-19T04:18:55.3488662Z`
- 종료 UTC: `2026-09-19T04:27:46.2640316Z`
- 역할: Reviewer B / Huygens — signed packager, SDK, 시험·산출물, 재현성·근거 귀속, 문서 정합성.
- 저장소: `F:/dev/canview-wt/t007-ota-container`
- 실제 확인 base: `6cf1b8e57840a27b83c407d1325a92f869cf2f5d`
- 실제 확인 candidate: `d87516a93439a5362a558b37ddb703633941c140`
- 시작·종료 모두 두 commit의 `cat-file -e` 성공 및 `rev-parse` 일치.
- 소스는 commit object의 `git show`와 `git diff`로만 읽었다. mutable worktree 소스를 사용하지 않았다. worktree clean 상태를 candidate 격리 근거로 주장하지 않는다.
- 파일 수정·compile·test 실행·commit·push·다른 agent 호출 없음. 다른 reviewer 원문은 열지 않았다. review archive의 절차·목록은 열었지만 과거 disposition을 독립 판정 근거로 사용하지 않았다.

판정: **BLOCK — T-007 전체 완료를 입증하기에는 구현·검증 근거가 부족하다.**  
부분 checkpoint의 기존 closure를 번복하거나 merge를 판정하는 보고서가 아니다.

## 2. 실제 수행 및 읽은 범위

실제 명령 계열:

```text
[guid]::NewGuid()
[DateTime]::UtcNow.ToString('o')
git cat-file -e "<base/candidate>^{commit}"
git rev-parse "<base/candidate>^{commit}"
git diff --find-renames --stat <base> <candidate> -- . ':!docs/reviews/*'
git diff --find-renames --name-status <base> <candidate> -- . ':!docs/reviews/**'
git diff --find-renames --numstat <base> <candidate> -- shared/ota/src tools/ota tests/ota
git diff --find-renames <base> <candidate> -- <선택한 소스·CMake 경로>
git show <candidate>:<path>
```

`git show` 출력에 PowerShell 행 번호를 붙이고 필요한 절을 선택하거나 `rg -n`으로 식별자를 검색했다. binary는 `git show`의 raw stdout을 메모리로 받아 .NET SHA256으로 계산했다. 존재하지 않는 component 경로와 `test_schema.py` 조회는 실패했고, 실제 component 및 `test_manifest_json.py` 경로로 정정했다.

직접 읽은 주요 파일·행:

- 정본: `AGENTS.md`, `docs/README.md`, `docs/resume.md:1–175`, `docs/tasks/T-007-ota-container.md:1–169`, `docs/architecture/ota.md:179–285,311–364`, `docs/adr/009-ota-native-image-alignment.md`.
- 책임 구분: `docs/tasks/T-204-esp-ota-recovery.md:1–59`, `T-205-ota-policy-migration.md:1–58`, `T-107-stm32-mcuboot.md:1–54`. `docs/tasks.md`는 관련 task 행만 검색했다.
- 절차: `docs/runbooks/agent-workflow.md:1–190`, review template 및 archive 안내·목록. embedded-architecture/cstyle/documentation skill을 읽어 책임·C 경계·근거 분류에 적용했다.
- Schema: `schema/cvota-v2.schema.json:1–62`, `protocol/schema/ota-container-v2.yaml:1–30`.
- Host 소스 전체: `tools/ota/container.py:1–126`, `manifest.py:1–131`, `manifest_json.py:1–106`, `cbor.py:1–152`, `envelope.py:1–87`.
- 시험·golden: `generate_signed_golden.py:1–126`, `test_signed_golden.py:1–165`, `test_container.py:1–174`, `test_manifest.py:1–206`, `test_manifest_json.py:1–155`, `test_native_stm.py:1–171`, `test_native_metadata.c:1–135`, `check_sdk_metadata.py:1–58`.
- Portable 경로: `shared/ota/src/envelope.{c,h}`, `body.{c,h}`, `manifest.c`, `cbor_head.c`, `cbor_document.h` 및 `cbor_document.c:98–204`, `native_metadata.c:1–81`, `native_stm.c:1–158`; `shared/ota/README.md` 관련 계약·잔여 범위 절.
- SDK/BSP: `firmware/platform/esp32s3/ota_image.c:1–83`, `ota_crypto.c:1–186`, `ota_crypto.h:1–60`, `firmware/communicator/esp32/bsp/ota.c:1–48`.
- Build: root CMake OTA 시험 등록부, `shared/ota/CMakeLists.txt`, `firmware/components/canview_esp32_ota/CMakeLists.txt`, foundation/ESP core/platform/runtime component CMake.
- 네 정상 target의 root/main CMake 및 진입 경로: Controller `firmware/app/startup.c`, Communicator ESP `firmware/app/esp_core.c`, STM `firmware/communicator/stm32/app/main.c`, Bridge `firmware/diagnostic-bridge/main/app_main.c:96–248`.
- SDK fixture의 root/main CMake, `sdkconfig.defaults`, README, `main/main.c:1–32`, `main/metadata.c:1–50`.
- Signed golden README·provenance·네 보존 blob, `.gitattributes`, toolchain pin 관련 행.
- `.github/workflows/foundation.yml`의 host 시험, target build, metadata/golden 검사, artifact source-revision 결합 구간.

전체 backlog·과거 raw review·journal 전체는 통독하지 않았다.

## 3. 다섯 수용 기준 판정

아래 “정적 근거 있음”은 이번 실행에서 시험 PASS를 확인했다는 뜻이 아니다.

| T-007 기준 | 확인한 근거 | 완료 감사 판정 |
|---|---|---|
| ① 잘못된 identity/key/서명, critical field, duplicate/depth/truncation 거절 — `:157` | C/Python bounded CBOR, 고정 manifest schema, 외부 trusted identity와 P256 검증, 관련 negative 시험 소스 | **부분 충족.** outer 검사 근거는 있으나 native까지 연결된 일반 검사 경로는 미완료 |
| ② 정수·겹침·중복 target·서명/header/길이 오류의 C/Python 동일 거절 — `:158` | 상한·정렬 offset 계산, 중복 target/총길이 검사, differential 시험의 상태·출력 비교 | **구현·시험 설계 근거 있음.** candidate의 실제 전체 실행 결과는 독립 확인하지 않았으며 완료 gate로 확정하지 않음 |
| ③ 사전 erase 금지, 비활성 enum target만 쓰기, 전체 검증 전 PREPARED/boot 금지 — `:159` | parser/body에 writer·boot callback이 없음. body 결과는 `HASHES_MATCHED`에 제한 | **부분 충족.** 무쓰기 분리는 확인되지만 실제 writer allowlist·호출 순서의 충족 증거는 아님. T-204/T-205와 책임 분리가 필요하며 gate 자체는 유지해야 함 |
| ④ 정확한 u64 및 image/manifest 불일치 거절 — `:160` | 정수 기반 decode와 native metadata 비교, `UINT64_MAX`·2⁵³ 초과·불일치 시험 | **부분 충족.** 개별 checker에는 있으나 `container.py`가 native metadata 검사를 호출하지 않음 |
| ⑤ 공식 tool pin·golden digest·secret 없는 clean host 합성 시험 — `:161` | SDK/MCUboot commit 및 tool version 검사, memory-only 임시 key, 고정 fixture와 provenance, 재조립 시험 | **고정 산출물·pin 근거 확인.** clean host 암호 시험 실행은 작성자 제공 근거이며 이번 독립 실행은 NOT_RUN |

직접 확인한 보존 package:

```text
tests/fixtures/ota-signed-golden/communicator.cvota
length: 394310 bytes
SHA256: 68eb18e10bf35d351c1604500bf85f6e95aa41c6b49477ffbbdacd9477902655
```

세 public-key blob의 길이·SHA256도 provenance와 일치했다. 이는 blob 동일성 확인이지 암호 검증 실행이 아니다. 원 unsigned SDK BIN의 재빌드·digest 재계산은 수행하지 않았다.

Golden 재생성은 새 임시 key와 서명 때문에 byte-identical 재생성이 아니다. 고정된 보존 package 검증 및 동일 입력 재조립과 구분한 README 설명은 타당하다.

### 구현 범위·실제 target gate

- Schema/C/Python parser·enum map·외부 detached signature 조립기는 구현되어 있다.
- 공식 native signing 연결은 합성 fixture generator에서 확인된다. 이를 모든 지원 입력을 처리하는 일반 packager/checker 완료로 확대할 수 없다.
- prefix/body 소유권·부분 입력·reset·크기 상한은 존재한다. 그러나 `shared/ota/README.md:153–154`처럼 MCU timing/전체 call-chain stack은 미측정이다. SDK 내부 자원을 포함한 target별 allocation/CPU budget 완료 근거는 부족하다.
- 실제 SDK fixture compile/link는 유효한 target 근거다. 다만 `main/main.c:15–28`은 NULL negative 경로이며 정상 PSA/native 검증 실행 증거가 아니다.
- 네 정상 firmware target의 현재 CMake/진입 경로는 OTA validator/provider의 정상 사용을 입증하지 않는다. 기존 정상 firmware build 성공과 SDK fixture 성공을 합쳐 “정상 OTA target 연결 완료”로 볼 수 없다.
- workflow에는 immutable source-revision 결합과 검사 등록이 있다. **등록된 gate와 candidate에서 통과한 gate는 다르다.** CI `35420771316` 및 신규 artifact는 이번에 조회·검증하지 않았다.

## 4. Findings

### P0: 없음
### P1: 없음

현재 확인한 범위에서 P0/P1 결함을 발견하지 않았다. 전체 완료 근거 부족과 현재 실행 가능한 보안 우회를 동일시하지 않는다.

### B-T007-AUD-01 — P2 — 완료 책임이 후속 task와 순환한다

- 위치: `docs/tasks/T-007-ota-container.md:119`, `:125`; `docs/architecture/ota.md:319–324`; `docs/tasks/T-204-esp-ota-recovery.md:6,19–23`; `docs/tasks/T-205-ota-policy-migration.md:18–20`.
- 실패 시나리오: T-007 완료를 정상 OTA owner·실제 영속 policy까지 요구하는 것으로 해석하면, T-007을 선행으로 기다리는 T-204 및 그 downstream 작업의 책임을 선행 task에 다시 요구하게 된다.
- 영향: 완료 범위가 계속 확대되거나, 반대로 scope 정리를 이유로 명시적 erase/native gate까지 누락할 수 있다.
- 권고: 정본 우선순위에 따라 OTA-01의 schema/parser/CLI/golden 및 검증 계약은 T-007, 정상/recovery writer·buffer lifecycle은 T-204, STM bootloader는 T-107, 영속 policy/ota_manager는 T-205로 명확히 연결한다. `:159`의 parser 측 무권한 계약과 runtime enforcement를 구분하되 후자를 충족으로 체크하거나 삭제하지 않는다.

### B-T007-AUD-02 — P2 — native signing/checking 연결은 아직 합성 시험 경로에 한정된다

- 위치: `tools/ota/container.py:38–61,64–75,120–121`; `tests/ota/generate_signed_golden.py:74–105`; `docs/tasks/T-007-ota-container.md:133–135,160`.
- 실패 시나리오: native image의 signed sequence는 그대로 두고 manifest sequence만 다른 유효 u64로 작성하여 정당한 outer 서명을 붙이면, image hash가 같으므로 현재 `check_container()`는 native metadata 불일치를 발견하지 않는다. native 서명을 손상시킨 뒤 whole hash와 outer 서명을 갱신한 경우도 이 함수의 검사 대상 밖이다.
- 영향: 현재 명시된 `MANIFEST_AND_HASHES_MATCHED` 계약에는 부합하지만, 이를 T-007의 전체 signed image 검사·u64 일치 수용 기준 완료로 사용할 수 없다.
- 권고: 기존 공식 signing/verification 도구와 metadata checker를 일반 입력 경로에 연결한다. package 밖 trusted native root를 사용하고, outer가 유효한 native signature/identity/sequence 오류를 CLI 수준에서 거절하는 시험을 추가한다. 제품 key 생성·provisioning 구현을 요구하는 것은 아니다.

### B-T007-AUD-03 — P2 — target 연결·자원 예산의 완료 근거가 부족하다

- 위치: `docs/tasks/T-007-ota-container.md:125,136`; `shared/ota/README.md:153–154`; `tests/fixtures/idf-ota-image/main/main.c:15–28`; `firmware/controller/main/CMakeLists.txt:1–11`, `firmware/communicator/esp32/main/CMakeLists.txt:1–9`, `firmware/diagnostic-bridge/main/CMakeLists.txt:1–10`, `firmware/communicator/stm32/CMakeLists.txt:64–75,128–129`.
- 실패 시나리오: host 144개 시험과 SDK symbol link 성공만으로 모든 역할의 실제 target 검증 경로 및 메모리/CPU budget까지 닫는다.
- 영향: 제품 build의 설정·링크 경로, 실제 crypto 자원 사용, target별 검증 경로의 공백이 완료 기록에서 가려진다.
- 권고: 기존 build 구조에서 T-007의 read-only 검증 경로를 실제 target 구성과 연결하고, 해당 commit의 ELF/MAP/BIN·설정·symbol 및 storage/stack/crypto 자원 예산 근거를 남긴다. 정상 설치 owner 전체를 먼저 만들 필요는 없지만 SDK fixture 한 개를 네 정상 target 통합 근거로 대신하지 않는다. 측정하지 못한 runtime 수치는 미완료 gate로 유지한다.

### B-T007-AUD-04 — P3 — 현재 모듈 README에 이미 구현된 항목의 미완료 문구가 남아 있다

- 위치: `shared/ota/README.md:131,246–247`.
- 실패 시나리오: 현재 상태를 읽는 사람이 영속 signed golden과 prefix 부분 수신 조립 자체가 아직 없다고 판단한다.
- 영향: 남은 작업과 기존 구현의 구분이 흐려지고 완료 감사·후속 작업 선정이 부정확해진다.
- 권고: 현재 README에서 구현된 prefix/golden, 아직 부분적인 일반 CLI, 미연결 target owner를 분리한다. 과거 raw evidence는 수정하지 않는다.

## 5. 남은 최소 작업 순서

1. **완료 책임 정리:** T-007과 T-204/T-107/T-205의 경계 및 `:159`의 미충족 runtime gate를 명시적으로 연결한다.
2. **T-007 필수 연결:** 기존 native signing/verification·metadata checker를 일반 packager/checker에 연결한다. outer-valid/native-invalid 및 image/manifest u64 불일치 회귀시험을 둔다.
3. **실제 target·budget 근거 확보:** 기존 target/qualification 경로로 검증 코드를 compile/link하고 역할별 설정·산출물·자원 예산을 확인한다. host timeout을 MCU CPU budget으로 사용하지 않는다.
4. **최종 commit 검증 귀속:** clean host differential/golden/negative/drift 시험과 CI artifact를 동일 revision에 결합해 확인한다. 이전 commit의 성공을 자동 승계하지 않는다.
5. 이후 **T-204/T-107/T-205에서** 실제 writer allowlist·single owner, bootloader, 영속 policy·activation을 구현·검증한다. 이는 T-007의 부족한 native 검사나 actual-target gate를 면제하지 않는다.

## 6. 한계·최종 판정

공격한 관점은 outer-valid/native-invalid, signed metadata 불일치, u64 정밀도, CBOR/길이/중복 경계, output 보존, 고정 golden과 재생성의 혼동, SDK fixture와 정상 target의 혼동, source revision 및 task 책임 혼동이다.

이번 작업은 **수용 기준 추적 중심의 object-only 정적 감사**다. 전체 144개 시험과 CNG/SDK 내부 구현의 전수 재감사, 새 clean build, sanitizer·coverage·암호 검증 실행, CI artifact 다운로드는 하지 않았다. 실행 검증 부분은 미완료이며 PASS로 추정하지 않는다.

작성자의 Debug/Release `144/144`, PSA closure, SDK 경고 0 및 symbol 확인은 제공받은 근거로만 분류했다.

- Physical / 실제 PSA device / Flash / HIL: **NOT_RUN**
- Vehicle TX: **NO-GO**
- 정상 OTA owner 통합: 미완료
- T-007: **IN_PROGRESS 유지**
- PR36: Draft 유지, merge 승인 없음

**최종 verdict: BLOCK — T-007 전체 완료 선언의 근거가 아직 부족하다.**
