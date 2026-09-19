# T-007 native CLI post-fix·소유권 대응 독립 리뷰 — 원 Reviewer B

- Execution UUID: `b118c759-7526-42c7-a608-84d26f9910c1`
- 시작 UTC: `2026-09-19T05:20:41.0817178Z`
- 종료 UTC: `2026-09-19T05:22:49.5116510Z`
- 역할: Reviewer B / oracle·CLI 출력 보존·SDK/CI·문서 정합성
- 저장소: `F:/dev/canview-wt/t007-ota-container`
- 실제 확인 Base: `7cb07be6a686866ee031f5816ca187949ce93bb1`
- 실제 확인 Candidate: `6cf1106bd1f7a2facf96e620727581e300822814`
- Verdict: **PASS — 이번 post-fix와 소유권 문서 delta의 정적 검토에 한정**
- 전체 T-007 수용 판정: **BLOCK 유지**

## 1. 격리·실행 구분

시작과 종료에 두 commit의 `cat-file -e`와 `rev-parse`를 확인했다. 모두 exit 0이며 hash가 manifest와 일치했다.

저장소 소스는 고정 commit의 `git show`와 base→candidate `git diff`로만 읽었다. moving worktree source를 사용하지 않았다. 파일 수정·시험 실행·compile·설치·commit·다른 agent 호출은 하지 않았다. 상대 새 finding과 보존된 reviewer raw·통합 report도 열람하지 않았다.

`embedded-documentation`, `embedded-architecture` skill을 직접 읽고 소유권·검증 gate 대조에 적용했다. 문서 생성이나 수정은 수행하지 않았다.

## 2. 원 finding 재판정

### B-NCLI-01 — FIXED, 정적 재확인

위치: `tools/ota/native.py:59–80`, `tests/ota/test_native_container.py:82–117, 228–240`.

기존 실패 시나리오는 ESP block의 scheme 또는 내장 RSA key 자료를 바꾸고 CRC·outer hash/서명을 복구하면, 외부 공개키 서명 검사만 통과하여 잘못된 native artifact가 성공으로 표시되는 것이었다.

수정본은 다음을 같은 block에 결합한다.

1. CRC-valid block을 얻고 RSA scheme을 요구한다.
2. 해당 block의 signature를 추출한다.
3. 외부 trusted 공개키와 image 본문을 공식 helper에 전달한다.
4. helper가 재구성한 block과 입력 block 전체가 같을 때만 성공한다.

공식 helper는 실제 RSA-PSS/SHA256 검증을 수행한 뒤 외부 공개키에서 RSA 자료를 계산하고, digest·signature·CRC·padding을 포함한 block을 만든다. private key를 요구하거나 새 signature를 생성하는 경로가 아니다. [espsecure v5.4.0 helper 및 RSA block 생성](https://github.com/espressif/esptool/blob/v5.4.0/espsecure/__init__.py#L686-L836)

`matched`는 한 block의 전체 비교 성공으로만 설정된다. 따라서 서로 다른 block의 key 일치와 signature 성공을 조합할 수 없다.

회귀시험도 다음을 직접 구성한다.

- scheme/n/e/rr/m/signature 6필드 변이 후 CRC 재계산
- image hash·outer signature 갱신 및 outer 검사 성공 확인
- 정상 block을 각 3개 slot에 배치한 양성 사례
- key만 맞는 block과 signature만 맞는 block의 교차 혼합 거절
- 변조 입력의 CLI 출력 미생성·기존 출력 보존

기존 권고가 구현·시험 source에 반영됐다. 독립 실행 재현은 NOT_RUN이며, 작성자 실행 결과와 구분한다.

### B-T007-AUD-02 — FIXED, 일반 host native 검사 미구현 범위에 한정

위치: `tools/ota/native.py:43–49, 52–160`, `tools/ota/container.py:119–136`.

일반 입력에 대해 outer 검증 이후 공식 ESP/MCUboot 암호 검증과 metadata/version/u64 대조가 연결되어 있다. B-NCLI-01 수정으로 이전 closure 보류 사유도 해소됐다.

이는 일반 host 검사 연결에 대한 판정이다. 정상 target 설치 경로, local policy, writer 권한, 전체 T-007 acceptance 완료를 뜻하지 않는다. 대응되는 A-AC-01의 최종 disposition은 원 A의 독립 판정을 대신하지 않는다.

### B-T007-AUD-01 — FIXED, 책임 충돌 문서 범위에 한정

위치: `docs/tasks/T-007-ota-container.md:169–188`, `docs/resume.md:138–150`.

새 대응은 상위 설계와 기존 후속 task 책임에 부합한다.

- T-204: ESP normal/recovery와 writer allowlist — `T-204-esp-ota-recovery.md:19–23, 49–51`
- T-107: STM bootloader·보호 map·Flash — `T-107-stm32-mcuboot.md:19–22, 41–46`
- T-205: ota_manager·영속 policy·단일 journal writer — `T-205-ota-policy-migration.md:18–21, 43–50`
- 물리 qualification: 해당 target task 및 T-508 — `T-508-ota-power-can-hil.md:15, 41–51`

T-204/T-107은 이미 T-007을 선행으로 요구한다. 완성된 후속 writer를 다시 T-007 선행으로 요구하지 않는 정리는 타당하다.

동시에 `T-007:184–188`은 AC3 연결시험, 실제 SDK compile/link·산출물 근거를 유지하며 writer 부재만으로 쓰기 0회 성공을 인정하지 않는다. `T-007:205–209`의 5개 AC는 base와 문구가 동일하고 모두 미체크다. scope 면제나 임의 수용 기준 축소로 판단하지 않는다.

대응되는 A-AC-02의 원 A 판정을 대신하지 않는다. AC3 구현·시험 완료 자체는 **OPEN**이다.

## 3. 신규 finding

- P0: 없음
- P1: 없음
- P2: 없음
- P3: 없음

신규 결함이 없어 신규 failure scenario·영향·수정 권고는 없다. 이는 검토 범위 밖까지 안전성을 증명했다는 의미가 아니다.

## 4. 공격한 회귀·근거 경계

- block CRC만 맞춘 변이, 잘못된 scheme/key/signature, 교차 block 결합, 정상 slot 위치 변경을 추적했다.
- 최소 image 길이·sector 정렬 검사가 block 접근보다 앞서며, 최대 3개 block만 순회함을 확인했다.
- helper 검증 실패 후 다른 block을 검사하더라도 성공 조건은 동일 block 전체 일치임을 확인했다.
- 도구 version 불일치, dirty checkout, ImportError·timeout·process failure 모형을 읽었다. 모형 시험을 실제 SDK/장치 실행으로 해석하지 않았다.
- `test_native_container.py:217–227`은 실제 STM subprocess 호출 경계에 timeout/OSError/nonzero 결과를 주입하고 출력 미생성을 요구한다.
- `container.py:123–129`의 검증 후 `open("xb")` 순서는 유지된다.
- CTest 등록과 Windows CI 호출 경로는 유지되며, 이번 delta에서 CMake/workflow/lock/C target/golden 변경은 없다.
- journal과 resume는 c7f5780 CI·artifact 근거를 이후 native candidate 성공으로 재사용하지 않도록 구분한다.
- review 경로를 제외한 `git diff --check`는 exit 0이었다.

## 5. 실제 열람 범위·명령

직접 읽은 주요 candidate 파일:

- `tools/ota/native.py:1–160`
- `tests/ota/test_native_container.py:1–252`
- `tools/ota/container.py:86–141`
- `docs/tasks/T-007-ota-container.md:1–217`
- `docs/resume.md:1–192`
- `docs/journal.md` 이번 추가분
- T-204/T-107/T-205/T-508 상세 task 전체
- `AGENTS.md`, `docs/README.md`, `docs/architecture/README.md`
- `docs/architecture/ota.md:179–285, 311–347`
- ADR-009 전체
- `docs/runbooks/agent-workflow.md:104–216`
- `CMakeLists.txt:183–191`
- `.github/workflows/foundation.yml:21–46`

공식 외부 source: esptool v5.4.0 `espsecure/__init__.py`의 pre-calculated-signature helper와 RSA block 생성 함수.

실제 명령 계열:

```text
[guid]::NewGuid()
[DateTime]::UtcNow.ToString('o')
git cat-file -e "<base/candidate>^{commit}"
git rev-parse "<base/candidate>^{commit}"
git diff --find-renames --name-status <base> <candidate>
git diff --find-renames <base> <candidate> -- <검토 경로>
git show <candidate>:<path>
git diff --check <base> <candidate> -- . ':!docs/reviews/**'
git diff --stat <base> <candidate> -- <변경 없음 확인 경로>
Compare-Object <base의 5개 AC> <candidate의 5개 AC>
```

행 번호·관련 절 표시는 PowerShell로 처리했다. 공식 helper는 웹으로 직접 읽었다. AC 비교 결과는 양쪽 5개, 차이 없음이었다.

## 6. 미검토·NOT_RUN 및 최종 판정

작성자가 제공한 최종 Debug 147/147·43.79초, Release 147/147·37.30초, 8개 unittest method 결과는 **작성자 실행 근거**다. 이번 reviewer는 해당 시험이나 `build/t007-native-binding-final-*-test.log`를 실행·열람하지 않았다.

미검토 범위:

- 보존 raw의 exact-byte 감사 및 상대 report
- 신규 CI 결과·artifact 다운로드 감사
- 변경 없는 전체 C verifier·SDK·dependency 재감사
- 실제 target 통합·writer·영속 정책 구현
- 장치 stack/heap/timing 및 물리 시험

핵심 수정 source와 요청된 소유권 문서 delta는 직접 읽었다. 이번 검토는 incomplete가 아닌 **정적 범위 PASS**다.

**B-NCLI-01 FIXED, B-T007-AUD-02의 일반 host 검사 미구현 FIXED, B-T007-AUD-01의 책임 충돌 문서 FIXED. 신규 finding 없음.**

**전체 T-007은 BLOCK/IN_PROGRESS 유지. AC3 연결시험·최종 target/자원 근거는 OPEN. Device/Flash/physical/HIL/실측 budget NOT_RUN, vehicle TX NO-GO, PR36 Draft 유지. 완료·merge 승인이 아니다.**
