# T-007 최종 소프트웨어 수용 감사 — Reviewer A 원문

## 실행 식별·격리

- 역할: Reviewer A / Maxwell — C 계약, 메모리·수명, 상태 전이, target 연결, writer 권한 경계.
- Execution UUID: `4852ebbb-c56c-4ef5-b2f1-eb4e81bcc1e3`
- 시작 UTC: `2026-09-19T06:19:01.2667771Z`
- 종료 UTC: `2026-09-19T06:26:47.1203082Z`
- 저장소: `F:/dev/canview-wt/t007-ota-container`
- 실제 확인 PR base: `6cf1b8e57840a27b83c407d1325a92f869cf2f5d`
- 실제 확인 candidate: `77b84cf07cd868f0f0b858a00a0ae8c9b3eb9ef4`

시작·종료 모두 두 commit의 `git cat-file -e "<hash>^{commit}"` 성공과 `git rev-parse "<hash>^{commit}"` 일치를 확인했다. 저장소 내용은 고정 hash의 `git show`와 `git diff`로만 읽었다. 이동 중인 worktree나 HEAD를 candidate 소스로 사용하지 않았다.

파일 수정·시험·컴파일·commit·다른 agent 호출은 하지 않았다. 상대 reviewer의 새 finding이나 개별 raw report는 열람하지 않았다. 리뷰 아카이브 README의 절차·기존 목록은 읽었으며, 이를 현재 요구사항이나 독립 검증 결과로 사용하지 않았다.

## 요약 판정

**Verdict: CONDITIONAL**

현재 정본과 구현·시험 소스를 대응한 결과, T-007에 추가해야 할 필수 소프트웨어 구현 또는 시험 설계의 누락은 발견하지 못했다. 다섯 AC에 대응하는 거절 경로, 정상 양성 대조, 일반 native 검사, 수신 순서 조정, target 연결 및 정적 예산 기록이 존재하고 서로 연결된다.

다만 이것은 **정적 수용 감사 결과**다. candidate `77b84cf`의 최신 CI와 실제 target 산출물 귀속은 이번 실행에서 확인하지 않았으므로 최종 실행 gate는 미검증이다. 전체 T-007 완료·AC 체크·PR ready·merge 승인을 의미하지 않는다.

## 1. 수용 기준별 판정

아래의 “정적 충족”은 구현과 시험의 대응이 충분하다는 뜻이다. 해당 시험을 본 reviewer가 실행해 PASS를 얻었다는 뜻은 아니다.

| 수용 기준 | 직접 확인한 근거 | 판정 |
|---|---|---|
| AC1 — 잘못된 identity/key/epoch/서명, unknown field, 중복 key, 깊이 초과, 절단 거절 | `shared/ota/src/cbor_document.c:98`, `:122`의 key 순서·자원 제한, `envelope.c:126`, `:198`, `:203`의 header/CBOR/서명 검사, `manifest.c:370`의 identity 연결. `test_cbor_document.py:24`, `test_manifest.py:1`, `test_body.py:187`, `:239`의 음성 입력·실제 CNG 경로 | **정적 충족**. 최신 실행 결과 독립 미검증 |
| AC2 — 정수·blob 배치·중복 target·서명/header 길이·zero/초과 길이의 C/Python 일치 | `cbor_head.c:59`의 제한된 폭 읽기, `manifest.c:299`의 역할별 target·길이·정렬·총길이 검사, `tools/ota/manifest.py:1`, `envelope.py:62`의 대응 검사. `test_manifest.py`, `test_container.py`, `test_body.py:85`의 정렬 경계·padding·절단·최대 크기 시험 및 결과 개수/상태 비교 | **정적 충족**. 임의 offset/address를 허용해 겹침을 사후 처리하는 구조가 아니라 정해진 배치를 검증하는 구조임 |
| AC3 — 사전 검증 전 erase 금지, 허용 target만 수신, 전체 image 검증 전 설치 상태 변경 금지 | `stage.c:45`에서 body 사전 검증 후 `:51`의 begin, `:68` 성공 후 `:71`의 write, `:85` 성공 후 `:88`의 native callback. `stage.h:8`에 BSP map·금지 영역·설치 금지 계약. `stage_probe.c:198`, `:239`, `:254`의 실패 차단과 정상 대조, `check_stage_oracle.py:24`의 5개 C 변이 | **T-007의 호출 순서·권한 분리 계약은 정적 충족**. 실제 Flash allowlist 및 PREPARED/selector enforcement는 미구현 후속 owner gate이며 본 감사의 실제 실행 PASS가 아님 |
| AC4 — signed sequence의 정확한 u64 처리와 native/manifest 일치 | `manifest.c:189`, `native_metadata.c:58`, `:66`의 정수 읽기·대조, `native_stm.c:130`, `tools/ota/native.py:81`, `:107`의 native metadata 연결. `test_native_container.py`와 signed golden의 u64 경계·불일치 시험 | **정적 충족**. 문자열 또는 JavaScript 부동소수 비교로 대체되는 경로를 발견하지 못함 |
| AC5 — 공식 도구 pin, golden digest, secret 없는 clean-host 합성 binary 검사 | `tools/toolchain-versions.json`, `native.py:57`, `:94`의 version/commit/clean checkout 검사, `generate_signed_golden.py:74` 이후의 일시적 키·공식 signing 연결, `test_signed_golden.py:134` 이후 provenance·재조립 검사. 보존 package Git blob의 크기·SHA256은 직접 확인 | **구현·보존물은 정적 충족. clean-host 실행 및 최종 CI 귀속은 미검증** |

직접 확인한 golden Git blob:

- 경로: `tests/fixtures/ota-signed-golden/communicator.cvota`
- 길이: `394310 B`
- SHA256: `68eb18e10bf35d351c1604500bf85f6e95aa41c6b49477ffbbdacd9477902655`

이는 blob 길이·digest 검사이며 native 서명 검증 실행은 아니다.

## 2. 구현 범위·actual target gate 대응

| 구현 범위 | 근거와 경계 | 판정 |
|---|---|---|
| 컨테이너 schema | `schema/cvota-v2.schema.json:1`, `protocol/schema/ota-container-v2.yaml:1`, `manifest_json.py:1`의 필드·정수형·unknown/duplicate 제한 | 정적 충족 |
| C/Python bounded parser | CBOR 16 KiB/2048 item/depth 8, 고정 크기 C context, 길이 확인 후 접근. `cbor_head.c`, `cbor_document.c`, `envelope.c`, `manifest.c`와 Python 대응 구현 | 정적 충족 |
| enum→target map | `manifest.c:299`의 역할/target 조합 검사와 `stage.h:9`의 고정 BSP map 계약. 입력으로 임의 Flash 주소·경로를 받지 않음 | T-007 계약 충족. 실제 inactive slot/보호 영역 선택은 T-204/T-107 |
| image 수·길이·총길이·overflow | manifest 의미 검사와 body의 다음 offset·image 경계·padding 검사. `test_body.py:85`, `:191`, `:231`의 정렬 경계·chunk·최대 크기 대응 | 정적 충족 |
| CLI packager·검사기 | `container.py:38`, `:64`, `:86`의 outer 검증·조립·CLI, `:119` 이후 native 선택 검증과 출력 전 검사 | 정적 충족. outer-only 성공은 native 또는 설치 승인으로 취급하지 않음 |
| ESP/STM signing 연결 | `native.py:65`의 동일 ESP block scheme/key/signature/canonical byte 결합, `:107` 이후 STM profile·metadata·공식 imgtool 검증. 일반 identity 및 3역할 시험 존재 | 정적 충족. 합성 golden 전용 검사에 머물지 않음 |
| 동일 manifest byte열·signed golden | deterministic CBOR, detached raw P256 계약, 공식 ESP/STM signing으로 만든 보존 fixture와 재조립 검사 | 정적 충족. private key가 없는 보존 fixture로 검사가 가능함 |
| streaming 소유권·부분 입력/reset | `envelope.c:63`, `body.c:46`, `:119`, `:232`, `stage.h:40`. callback context는 reset 성공까지 유지하며 입력 pointer의 비동기 보존을 금지 | 정적 충족 |
| 재진입·partial init·cleanup | `stage.c:29`, `:50`, `:96`의 재진입 방어·partial begin close·재시도. PSA 모형은 partial setup, abort/destroy 실패와 context 보존을 검사 | 정적 충족. busy는 mutex가 아니라 single-task 계약임 |
| allocation·CPU 정적 예산 | `shared/ota/README.md:325` 이후 구조 크기·산술 합계·자체 frame·연산 상한·실측 한계 구분 | 정적 기록 충족. SDK 전체 heap/stack/WCET 인증은 아님 |
| 전체 parser + PSA read-only target 조합 | IDF parser component가 실제 core를 컴파일하고, fixture `main.c:46`이 `receiver.c`의 prefix→body→PSA 경로를 호출하도록 연결 | **연결 구현 충족. 현 candidate build/artifact gate 미검증** |
| stage actual SDK 연결 | fixture `main.c:23`의 네 API NULL 호출, component source 목록에 stage 포함 | compile/link 목적 연결만 충족. SDK에서 저장 callback 정상 실행을 했다는 증거가 아님 |
| 정상 4 firmware target 회귀 | 정상 ESP 3종 및 STM CMake 경로와 workflow의 target build 등록 확인 | 구성 확인. 현 candidate 실행 결과 미검증이며 정상 OTA owner 완료 증거도 아님 |

정적 예산의 산술은 일치한다.

- receiver: `16488 + 856 + 108 = 17452 B`
- 향후 prefix + stage + PSA: `16488 + 896 + 108 = 17492 B`
- 선택적 16 KiB chunk 포함: `33876 B`

해당 수치의 원 ELF/DWARF 및 `.su`를 이번에 재측정하지는 않았다. 문서가 이를 전체 RAM, 전체 call-chain stack 또는 MCU WCET로 확대하지 않는지 확인했다.

## 3. AC3 및 기존 finding 재판정

### AC3

`docs/tasks/T-007-ota-container.md:194`–`:203`의 책임 구분은 architecture §12의 OTA-01/02/03/06 분리와 부합한다. T-204/T-107의 완성된 Flash writer나 T-205의 영속 정책을 T-007 선행 조건으로 다시 요구하지 않는다.

이번 판단은 “writer가 없으므로 쓰기 0회”에 근거하지 않는다.

- `stage_probe.c:123`의 정상 begin과 write 모형이 실제 호출 가능하다.
- 사전 인증·identity·runtime·floor 실패는 `:198` 이후 begin/write/native callback 0회를 요구한다.
- 본문 실패·절단 뒤 native 검증을 진행하지 않으며, native 실패는 `NATIVE_MATCHED`로 승격되지 않는다.
- `check_stage_oracle.py:25`–`:31`은 early begin, 거절 후 write, native 실패 무시, hash context 및 identity guard 삭제를 각각 변이한다.
- compile 실패·timeout을 정상 변이 검출로 삼지 않고, 정상 baseline과 CHECK 실패 exit를 구별한다.
- `stage.h:16`, `:31`은 모든 storage callback의 PREPARED/selector 변경을 금지하고 `NATIVE_MATCHED`를 설치 권한과 분리한다.

따라서 **T-007 AC3의 소프트웨어 연결 계약과 시험 구성은 충족**으로 판단한다. PREPARED API가 없다는 사실 자체를 실제 selector 변경 0회의 실행 증거로 계산하지 않았다. 실제 저장 map·read-back·전원·freshness enforcement는 후속 owner에서 다시 검증해야 한다.

### 기존 A finding

- **A-AC-01 일반 native 연결 부족: FIXED, 정적 확인 유지.** 일반 CLI와 공식 도구 검증이 연결되어 있고 동일 ESP block의 key/scheme/signature 결합 검사도 존재한다.
- **A-AC-02 책임 충돌: FIXED, 정적 확인 유지.** task 소유권 표가 architecture 책임을 반영하며 다섯 AC 자체를 삭제·축소하지 않았다.
- **A-AC-03 전체 parser/PSA read-only target 조합 부족: 구현 결손 FIXED.** `receiver.c:39`–`:47`, `:64`, `:73`, `:91`과 fixture `main.c:46`에서 실제 조합을 확인했다. 다만 **77b의 target 실행 근거 closure는 별도 미검증**이다.
- **A-AC-04 정적 예산 기록 부족: FIXED, 정적 확인 유지.** 전체 장치 실측은 계속 NOT_RUN이다.

stage 인자 oracle의 최신 수정도 확인했다. `stage_probe.c:323`의 유효 입력 양성 대조, `:338`, `:360`, `:366`의 reset 전 body EMPTY 검사, `check_stage_oracle.py:31`의 identity 변이가 있어 같은 오류 코드로 가려지는 문제를 구별한다. 본 실행에서 mutant를 재실행한 것은 아니다.

## 4. 신규 finding

- P0: 없음.
- P1: 없음.
- P2: 없음.
- P3: 없음.

새 결함이 없으므로 신규 finding의 실패 위치·영향·수정 권고는 해당 없음이다. 최신 CI가 아직 확인되지 않았다는 사실은 아래의 열린 검증 gate로 기록하며 새 runtime 결함으로 분류하지 않는다.

공격한 주요 시나리오는 malformed/중복/과도한 CBOR, 길이와 offset 경계, padding·마지막 byte 변조, u64 상한, role/target 오용, outer-valid/native-invalid, ESP cross-block 결합, 사전 검증 이전 begin, 실패 뒤 write/native 호출, partial begin, 재진입, cleanup 실패와 최초 오류 보존, 중첩 인자가 하위 초기화 오류에 가려지는 경우다.

## 5. 실제 읽은 파일·명령

핵심 source는 줄 번호를 붙여 직접 읽었다. 아래 범위는 현재 감사에서 사용한 주요 파일이며, `tests/ota/*` 전체를 전수 검토했다는 주장은 아니다.

- 정본: `AGENTS.md`, `docs/README.md`, `docs/resume.md:136–183`, `docs/tasks/T-007-ota-container.md:141–233`, `docs/architecture/ota.md:179–285,311–346`, `docs/adr/009-ota-native-image-alignment.md:1–46`, T-204/T-107/T-205 상세 task, agent-workflow, review archive README.
- C core 전체: `cbor_head.c:1–78`, `cbor_document.c:1–204`, `envelope.c:1–211`, `manifest.c:1–505`, `body.c:1–257`, `floor.c:1–126`, `native_metadata.c:1–81`, `native_stm.c:1–158`, `stage.c:1–116`, `stage.h:1–97`.
- 플랫폼: `ota_crypto.c:1–186`, `ota_crypto.h:1–60`, `ota_image.c:1–83`, Communicator BSP `ota.c:1–48`.
- target: 두 OTA component CMake, SDK fixture CMake/main CMake/`main.c:1–52`/`receiver.c:1–105`/header/`metadata.c:1–50`/sdkconfig/README, 정상 4 target CMake.
- host: 두 schema, `cbor.py:1–152`, `envelope.py:1–87`, `manifest.py:1–131`, `manifest_json.py:1–106`, `container.py:1–141`, `native.py:1–160`, toolchain pin 파일.
- 시험: `stage_probe.c:1–406`, `test_stage.py:1–59`, `check_stage_oracle.py:1–55`, `body_probe.c:1–448`, `test_body.py:1–267`, `test_prefix_stream.c:1–114`, `test_cbor_document.py:1–134`, `test_manifest.py:1–206`, `test_manifest_json.py:1–155`, `test_container.py:1–174`, `test_native_container.py:1–252`, `test_signed_golden.py:1–165`, `generate_signed_golden.py:1–126`, native STM/CNG probe, `test_esp_ota_crypto.c:1–222`, receiver host/oracle, SDK metadata 검사기.
- 등록·근거: root CMake OTA 등록부, shared CMake, workflow host/target/artifact 관련 절, golden README/provenance 및 package blob, shared OTA README의 stage·예산 절.

실제 명령 계열:

```text
git cat-file -e "<base/candidate>^{commit}"
git rev-parse "<base/candidate>^{commit}"
git diff --find-renames --name-status <base> <candidate>
git diff --find-renames --stat <base> <candidate> -- <관련 경로>
git diff --find-renames <base> <candidate> -- <component/workflow 경로>
git show <candidate>:<path>
```

출력은 PowerShell 줄 번호 처리 및 `rg -n`으로 필요한 절을 선택했다. golden은 `git show <candidate>:<binary-path>`의 binary stdout을 메모리에서 읽어 길이와 SHA256을 계산했다. 파일로 추출하지 않았다.

`embedded-cstyle`, `embedded-architecture`, `embedded-documentation` SKILL.md를 읽고 C 수명·계층·문서/근거 구분에 적용했다. 이로 인한 파일 변경은 없다.

## 6. 실행 근거·미검토 범위

작성자가 제공한 Debug/Release 각 150/150, stage sanitizer, coverage 및 CLI 결과는 **작성자 제공 근거**다. 본 reviewer 실행 PASS로 집계하지 않았다.

이번 감사에서 하지 않은 일:

- host 시험·sanitizer·mutant·native CLI 재실행 및 compile.
- `77b84cf` CI `35426085834`의 실시간 상태·로그·산출물 다운로드와 hash 대조.
- 이전 candidate CI를 현 candidate 성공 근거로 전용.
- 공식 SDK/암호 라이브러리 내부 구현 전수 감사, 모든 dependency lock hash 재검증.
- 실제 ELF/DWARF/`.su` 재측정 및 모든 보조 시험 파일의 전수 검토.
- 정상 OTA owner, 실제 Flash map, 영속 journal/floor, provisioning 및 activation 구현 검증.

요청한 핵심 구현·AC 연결 범위의 미열람 때문에 발생한 incomplete는 없다. 다만 위 실행·산출물 검증은 명확히 미완료다.

**Physical/device/Flash/HIL/전체 stack·heap·WCET: NOT_RUN.  
Vehicle TX: NO-GO.**

## 7. 최소 다음 작업

1. **77b 후보의 최신 CI/actual target gate를 닫는다.** 정상 4 target과 OTA SDK fixture의 성공, ELF/MAP/BIN, source hash, artifact 길이·digest, warning/error 로그를 해당 commit에 귀속시킨다. 이전 569/82c 결과로 대체하지 않는다.
2. 현재 host·sanitizer·native·변이 시험 근거와 독립 reviewer 결과를 다섯 AC에 연결해 최종 소프트웨어 수용 결정을 기록한다. 이 정적 보고서만으로 체크하지 않는다.
3. 실제 Flash writer·보호 map은 T-204/T-107, 영속 policy/PREPARED/selector는 T-205, 실제 자원·전원/HIL qualification은 해당 target 및 T-508에서 수행한다. 이를 T-007의 새 선행 구현으로 순환 요구하지 않는다.

**최종: CONDITIONAL — 추가 필수 코드/시험 설계 누락은 발견하지 못했으나, 현 candidate의 최종 CI·actual target 산출물 증거 closure가 남아 있다. 전체 T-007 DONE 또는 merge 승인이 아니다.**
