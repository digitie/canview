# T-007 전체 수용 기준 완료 감사 — Reviewer A / Maxwell

## 실행·격리

- Execution UUID: `29cd5517-a7fe-49c3-8565-c6c61bd155a4`
- 시작 UTC: `2026-09-19T04:18:55.3352523Z`
- 종료 UTC: `2026-09-19T04:26:05.1455638Z`
- 전문영역: embedded C 수명·bounds·reset/cleanup, target 연결, native 검증과 writer 권한 경계.
- 저장소: `F:/dev/canview-wt/t007-ota-container`
- 실제 확인 base: `6cf1b8e57840a27b83c407d1325a92f869cf2f5d`
- 실제 확인 candidate: `d87516a93439a5362a558b37ddb703633941c140`
- 시작·종료 모두 두 commit의 `cat-file -e` 성공과 `rev-parse ...^{commit}` 일치를 확인했다.
- 소스는 고정 commit의 `git show`와 두 commit 간 `git diff`로만 읽었다. Moving worktree를 candidate로 사용하지 않았다. Worktree clean 상태는 이 격리 방식의 검증 대상으로 사용하지 않았다.
- 파일 수정·commit·compile·시험 실행·다른 agent 호출·상대 finding 열람/공유 없음.
- 적용 스킬: embedded-cstyle, embedded-architecture, embedded-documentation. 정적 검토에만 사용했다.

## 결론

**Verdict: BLOCK — T-007 전체 완료를 입증하는 근거가 아직 부족하다.**

핵심 정적 감사 범위는 읽었다. 핵심 소스 미열람으로 인한 incomplete 판정은 아니다. 다만 이는 모든 실행 결과나 SDK 내부 구현을 독립 검증했다는 뜻이 아니다.

현재 `IN_PROGRESS` 표시는 타당하다. 아래 finding은 완료 조건·검증의 공백이며, 명시적으로 partial인 구현을 새 runtime 취약점으로 재분류한 것이 아니다. 전체 Task·CI·target 실행·배포·merge 승인은 하지 않는다.

## 1. 수용 기준별 판정

| T-007 기준 | 현재 직접 확인한 근거 | 판정 |
|---|---|---|
| AC1, `T-007:157`: identity/key/signature, unknown field, duplicate, depth, truncation 거절 | `envelope.c:145–207`, `manifest.c:239–409`, `cbor_document.c:122–203`. `test_manifest.py:59–141,172–200`, `test_cbor_document.py:23–128`에 대응 negative/differential 경로가 있다. | **Parser 계층 구현 근거 있음.** Native 검증까지 포함한 일반 package 검사 완료는 부족하다. 이번 독립 실행은 NOT_RUN. |
| AC2, `T-007:158`: C/Python 동일 경계·중복·길이 거절 | `manifest.c:299–367`, `tools/ota/manifest.py:99–130`. 외부 blob offset은 받지 않고 signed length와 고정 정렬로 계산한다. Header count/total과 대조하며 role별 길이·중복 target을 제한한다. | **정적 범위에서 충족 근거 있음.** C/Python 교차 시험 소스도 확인했다. 현재 candidate의 실행 성공을 독립 확인한 것은 아니다. |
| AC3, `T-007:159`: 검증 전 erase 금지, 검증 후 비활성/staging만 허용, 전체 검증 전 PREPARED/selector 금지 | `body.c:68–77`은 manifest/preflight/floor 후 hash를 시작한다. `body.h:67–86`, `shared/ota/README.md:244–247`는 hash 성공과 writer 권한을 구분한다. 현재 API에는 writer/selector callback이 없다. | **부분 충족, 전체 문장 미입증.** 현재 parser가 Flash를 호출하지 않는다는 근거는 있으나, 실제 수신용 writer의 허용 대상·호출 순서·승인 금지를 검증한 근거는 아니다. 후속 writer 소유권과 T-007 경계 검증을 분리해야 한다. |
| AC4, `T-007:160`: signed u64 및 image/manifest 불일치 거절 | `manifest.c:188–189`, `native_metadata.c:58–68`, `native_stm.c:130–157`. JSON은 float/string 대신 정수로 처리한다. `test_native_metadata.c:84–91`, `test_manifest_json.py:75–125`에 u64 및 불일치 시험이 있다. | **Primitive 수준 충족 근거 있음.** 일반 CLI는 native metadata 대조를 호출하지 않으므로 end-to-end 완료는 부족하다. |
| AC5, `T-007:161`: 공식 signing 도구·golden digest 고정, secret 없는 synthetic 검증 | Generator/test와 provenance의 공식 도구 pin, 별도 outer P256, public-key-only 보존, 재조립 검사를 확인했다. Git blob의 길이·SHA256도 직접 대조했다. | **보존 산출물·시험 경로 근거 있음.** 고정 synthetic golden을 일반 signing CLI나 clean-host 독립 실행 성공으로 확대할 수 없다. 이번 암호 실행은 NOT_RUN. |

### 구현 범위와 사용자 target gate

- `T-007:133–134`: schema, C/Python bounded parser, enum·role 제한, outer 조립/검사 CLI는 존재한다. **Native-aware 일반 packager/검사 연결은 미완료**다.
- `T-007:135`: 공식 ESP/STM signing 연결은 **고정 합성 identity의 시험 generator**에 존재한다. Signed synthetic golden 자체가 없다고 판정하지 않는다.
- `T-007:136`: 입력 수명·partial/reset·cleanup·고정 상한은 구현되어 있다. **Target 전체 allocation/stack·CPU budget 입증은 부족**하다.
- 사용자 actual-target gate:
  - `.github/workflows/foundation.yml:285–310`에는 정상 4개 target 빌드와 별도 OTA SDK fixture 빌드가 등록되어 있다.
  - 현재 정상 target의 CMake 연결에는 OTA parser/provider가 포함되지 않는다.
  - SDK fixture는 adapter·공통 metadata·Communicator BSP를 compile/link하지만, 전체 portable parser/body/native STM 연결이나 정상 owner 실행을 증명하지 않는다.
  - 최신 CI `35420771316`은 사용자 제공 정보상 진행 중이다. 이번 감사에서 결과·artifact를 조회하지 않았다. **현재 candidate target gate PASS로 인정하지 않는다.**

## 2. Findings

### P0: 없음

읽은 범위에서 즉각적인 차량·비밀·Flash 안전 경계 붕괴를 확인하지 않았다.

### P1: 없음

현재 비활성/partial 구현에서 실제 writer 권한 우회가 발생한다고 입증한 finding은 없다.

### P2 — A-AC-01: 일반 CLI의 native 검증·metadata 결합이 빠져 있다

- 위치: `tools/ota/container.py:38–61,64–75,121`, `tests/ota/generate_signed_golden.py:69–105`, `docs/tasks/T-007-ota-container.md:133–135,160`.
- 실패 시나리오: Native signature가 잘못됐거나 native sequence가 manifest와 다른 image라도, 그 bytes의 SHA256과 유효한 outer manifest 서명을 준비하면 현재 `check_container()`의 검사 범위는 통과할 수 있다.
- 영향: 정상적으로 표시되는 결과는 `MANIFEST_AND_HASHES_MATCHED`뿐이다. 이를 전체 signed packager/검사 완료 근거로 사용하면 native 검증과 image/manifest 일치 조건이 빠진다. 현재 CLI가 설치 승인이라고 거짓 출력하는 결함은 아니다.
- 권고: 기존 공식 signing 도구를 재사용해 일반 CLI의 native 서명·protected metadata·manifest 대조를 연결한다. 고정 synthetic identity 밖의 지원 role/target 조합도 검사하고, outer hash가 맞지만 native 서명 또는 sequence가 틀린 사례를 거절해야 한다.

### P2 — A-AC-02: T-007 완료 경계와 후속 writer/policy 소유권이 혼재한다

- 위치: `docs/tasks/T-007-ota-container.md:48–50,70–71,119,159`; `docs/architecture/ota.md:313,319–321`; `docs/tasks/T-204-esp-ota-recovery.md:6,19–23,49–51`; `docs/tasks/T-205-ota-policy-migration.md:18–20`.
- 실패 시나리오: T-007 진행 기록을 전체 완료 조건으로 해석하면 정상 writer·영속 policy까지 T-007에 요구하게 된다. 반면 T-204는 T-007을 선행 조건으로 둔다. 반대로 이를 모두 후속이라고만 처리하면 AC3의 erase 금지·대상 제한 검증까지 빠질 수 있다.
- 영향: 완료 조건이 순환하거나 필수 권한 경계가 인수인계에서 누락된다. Parser에 writer가 없다는 사실만으로 AC3 전체를 체크할 수 없다.
- 권고: 정본 우선순위에 따라 T-007의 검증 결과·입력 수명·enum 대상·실패 시 무권한 계약과 T-204/T-205의 실제 writer/manager 책임을 명시적으로 대응시킨다. 검증 전 erase 0회, 금지 target 쓰기 0회, 전체 native 검증 전 PREPARED/selector 0회의 연결 시험을 필수 gate로 유지한다. 현재 AC3를 근거 없이 완료 처리하지 않는다.

### P2 — A-AC-03: SDK fixture 빌드와 전체 OTA target 연결 근거가 다르다

- 위치: `tests/fixtures/idf-ota-image/main/CMakeLists.txt:1–5`, `main/main.c:15–28`; `firmware/components/canview_esp32_ota/CMakeLists.txt:3–5`; 정상 ESP 3개 `main/CMakeLists.txt:2–6`; `firmware/communicator/stm32/CMakeLists.txt:102–129`.
- 실패 시나리오: Adapter symbol과 정상 firmware 빌드가 각각 성공해도, 전체 parser/body와 실제 crypto provider의 target 결합 오류는 그 빌드에서 발견되지 않을 수 있다.
- 영향: SDK fixture warning0·symbol 존재·Arm object compile만으로 `T-007:125,144–145`의 실제 target 연결 완료를 입증할 수 없다.
- 권고: 정상 설치 owner 전체를 앞당겨 구현하지 말고, 우선 기존 fixture/target build 경로에서 T-007 portable 검증 코드와 해당 SDK provider의 read-only compile/link 조합을 검증한다. 현재 candidate의 정상 4개 target 회귀 빌드 및 ELF/MAP/BIN 근거도 별도로 확보한다.

### P2 — A-AC-04: Bounded 연산량과 target allocation/CPU budget 검증이 아직 분리되어 있다

- 위치: `docs/tasks/T-007-ota-container.md:136`; `shared/ota/README.md:70–72,149–154,225–232`; `shared/ota/src/native_stm.h:12–16`; `config/budgets/foundation.yaml:3–14`.
- 실패 시나리오: 16KiB chunk와 무힙 portable core만 보고 전체 target budget을 통과시킬 경우, SDK 암호 연산의 실행 시간·내부 할당·호출 체인 stack을 누락한다. STM native 검사는 최대 180KiB image에 대한 두 hash 호출도 포함한다.
- 영향: Bounds 위반을 확인한 것은 아니지만, 실제 task stack·watchdog·응답 지연 예산이 충분하다는 완료 근거는 없다. 기존 foundation budget은 OTA 전체 경로 측정치가 아니다.
- 권고: 기존 경로의 context/buffer 배치, target stack 근거, SDK 자원 사용, 최대 입력당 연산량과 시간 한계를 기록한다. 정적으로 확인 가능한 상한과 장치에서 측정해야 하는 시간 gate를 구분한다. Host timeout·coverage를 MCU CPU budget으로 대체하지 않는다.

### P3: 없음

역사 기록이나 이미 종료된 finding을 새 결함으로 재등록하지 않았다.

## 3. 직접 공격한 경계와 확인 결과

- Prefix 완료를 인증으로 오인하는 경로: collector는 unsigned bytes 조립이며 서명 callback을 호출하지 않는다. 인증은 후속 manifest/body 경로에 남아 있다.
- Partial input, 중복·누락 offset, reset 후 재사용, borrowed buffer 덮어쓰기: 구현과 회귀시험 경로를 확인했다.
- Hash partial init·실패·cleanup 재실패: `hash_live`를 start 전에 설정하며 reset 실패 시 provider 소유 상태를 유지한다.
- PSA 재진입·close 실패·입력 alias: 현재 source는 busy/closing과 범위 검사를 유지한다. `test_esp_ota_crypto.c:148–157`의 ready union context 시험도 확인했다.
- Native STM false-pass: `native_stm.c:151–157`은 whole hash, 내부 hash, 실제 verifier 반환을 구분한다. Golden 시험의 원본 서명 유지 변조와 callback count 검사를 확인했다.
- Trusted root·identity 경계: package의 key/identity를 BSP root로 채택하는 정상 target 연결은 존재하지 않는다. 이를 구현 완료나 provisioning 검증으로 계산하지 않았다.

새로운 C bounds/cleanup 회귀는 읽은 범위에서 확인하지 못했다. 이는 실행 기반 무결함 보증이 아니다.

## 4. 실제 읽은 파일·명령

행 번호는 candidate blob 기준이다. 아래 `전체`는 해당 파일을 EOF까지 읽었다는 뜻이다.

- 정본: `AGENTS.md:1–132`, `docs/README.md:1–110`, `docs/resume.md:1–175`, `T-007-ota-container.md:1–169`, `architecture/ota.md:179–285,311–347`, `adr/009-ota-native-image-alignment.md:1–46`.
- 관련 task: `T-204-esp-ota-recovery.md`, `T-205-ota-policy-migration.md`, `T-107-stm32-mcuboot.md` 전체.
- 절차: `docs/runbooks/agent-workflow.md:1–234`, review `TEMPLATE.md:1–70`. Journal은 T-007/PSA 식별자 관련 검색 결과만 읽었다. Peer raw/report 본문은 읽지 않았다.
- Host 도구: `tools/ota/container.py:1–126`, `manifest.py:1–131`, `manifest_json.py:1–106`, `cbor.py:1–152`, `envelope.py:1–87`.
- Schema: `protocol/schema/ota-container-v2.yaml:1–30`, `schema/cvota-v2.schema.json:1–62`.
- Portable C: `shared/ota/README.md:1–388`, `CMakeLists.txt:1–6`; `src/envelope.c:1–211`, `.h:1–106`; `body.c:1–257`, `.h:1–105`; `manifest.c:1–505`, `.h:1–164`; `native_stm.c:1–158`, `.h:1–44`; `native_metadata.c:1–81`, `.h:1–38`; `floor.c:1–126`, `.h:1–79`; `cbor_head.c:1–78`, `cbor_document.c:1–204`, `.h:1–32`.
- Platform/BSP: `ota_crypto.c:1–186`, `.h:1–60`, `ota_image.c:1–83`; Communicator BSP `ota.c:1–48`, `.h:1–22`.
- 시험: `generate_signed_golden.py:1–126`, `test_signed_golden.py:1–165`, `test_container.py:1–174`, `test_manifest.py:1–206`, `test_manifest_json.py:1–155`, `test_body.py:1–267`, `test_cbor_document.py:1–134`, `test_native_metadata.c:1–135`, `test_prefix_stream.c:1–114`, `test_esp_ota_crypto.c:1–222`, `psa_crypto_fixture.h:1–44`, `body_probe.c:1–448`, `native_stm_probe.c:1–140`, `cng_provider.c:1–133`, `check_sdk_metadata.py:1–58`.
- Build: root CMake 전체; 정상 4개 firmware project CMake와 ESP main CMake 전체; 관련 foundation/esp_core/platform/runtime/OTA component CMake 및 STM module CMake 전체.
- Fixture: IDF fixture CMake·main CMake·`main.c:1–32`·`metadata.c:1–50`·sdkconfig.defaults·README 전체; signed golden README·provenance 전체.
- CI/budget: `.github/workflows/foundation.yml:1–452`, `tools/check_budgets.py:1–178`, `config/budgets/foundation.yaml:1–16`.
- `.gitattributes`, 합성 T103 fixture와 helper의 digest 변경은 diff로 읽었다.

실제 사용 명령 형식:

```text
git cat-file -e "<base/candidate>^{commit}"
git rev-parse "<base/candidate>^{commit}"
git diff --find-renames --stat <base> <candidate>
git diff --find-renames --name-status <base> <candidate>
git diff --find-renames [--numstat] <base> <candidate> -- <관련 경로>
git show <candidate>:<위 파일 또는 directory>
git show <candidate>:<파일> | 행 번호 부여 / rg / 필요한 행 선택
git cat-file -s <candidate>:tests/fixtures/ota-signed-golden/communicator.cvota
git rev-parse <candidate>:tests/fixtures/ota-signed-golden/communicator.cvota
```

Golden은 `git show`의 binary stdout을 메모리로 받아 .NET SHA256으로 직접 계산했다. 파일 추출·변경은 하지 않았다.

- Blob ID: `7d0930ea559b6ff984186daa99b293015c886e9d`
- 길이: `394310B`
- SHA256: `68eb18e10bf35d351c1604500bf85f6e95aa41c6b49477ffbbdacd9477902655`
- Candidate provenance와 일치. **이 검사는 서명 검증 실행이 아니다.**

## 5. 남은 최소 작업 순서

1. T-007의 5개 수용 기준을 현재 근거와 대응시키고, T-204/T-205/T-107 소유 기능 및 연결 gate를 명확히 한다. Erase/native/target 필수 조건은 삭제하거나 면제하지 않는다.
2. 기존 공식 도구를 사용한 일반 native-aware packager/검사 연결을 완성하고, native 서명·metadata·manifest 불일치 negative를 추가한다.
3. 기존 검증 API에서 writer로 넘어가는 경계의 금지 동작·대상 제한·호출 순서를 인수시험으로 고정한다. 실제 writer/manager 구현은 정본상 후속 task에서 수행한다.
4. T-007 코드·SDK provider의 target compile/link와 정상 4개 target 회귀 빌드, allocation/stack·CPU budget 근거를 확보한다.
5. 최종 candidate에 귀속된 host/sanitizer/native/target/CI artifact 근거를 모은 뒤 전체 수용 기준을 다시 판정한다.

새 범용 framework나 별도 암호 구현은 필요하지 않다.

## 6. 근거 귀속·미검토·물리 gate

작성자 제공 Debug/Release144/144, PSA coverage·mutant 검출, 실제 SDK warning0/symbol 결과는 **작성자 실행 근거**로만 취급했다. 이번 reviewer가 재실행한 PASS가 아니다. 과거 CI artifact는 해당 과거 commit 근거이며 최신 candidate로 승격하지 않았다.

이번 미검토/미실행: 실제 SDK 내부 암호 구현 재감사, 전체 dependency/secret 재감사, 모든 시험의 독립 실행, 최신 CI/artifact 실물 검증, target timing·heap 측정, 장치 positive native/PSA 실행.

후속 소유 기능:

- T-204: normal/recovery image·partition 및 실제 Flash writer 경계.
- T-205: `ota_manager`, 영속 policy/journal·activation/migration.
- T-107: STM MCUboot port·보호 설정·실제 swap/revert.
- 위 기능의 미완료 자체를 이번 T-007 delta의 새 runtime 결함으로 계산하지 않았다.

**Physical/HIL: NOT_RUN. 실제 Flash/보드 실행: NOT_RUN. Vehicle TX: NO-GO. PR36 Draft 유지.**

최종 판정은 **T-007 전체 완료 근거에 대한 BLOCK**이며, 현재 partial checkpoint의 일괄 폐기나 merge 승인/거부를 대신하는 판정이 아니다.
