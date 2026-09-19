# T-007 native CLI post-fix·소유권 대응 독립 리뷰 — Reviewer A

## 실행·격리

- Execution UUID: `8e947598-21a3-47af-9248-46100692ca16`
- 시작 UTC: `2026-09-19T05:20:39.2815120Z`
- 종료 UTC: `2026-09-19T05:24:17.1188931Z`
- 역할: 원 Reviewer A / Maxwell — native 암호·format/bounds·metadata/u64·신뢰 경계·false PASS
- 저장소: `F:/dev/canview-wt/t007-ota-container`
- Base: `7cb07be6a686866ee031f5816ca187949ce93bb1`
- Candidate: `6cf1106bd1f7a2facf96e620727581e300822814`

시작·종료 모두 두 commit의 `git cat-file -e "<hash>^{commit}"` 성공과 `git rev-parse "<hash>^{commit}"`의 정확한 hash를 확인했다. 저장소 내용은 고정 commit의 `git show`와 base→candidate `git diff`로만 읽었다. 이동 중 worktree/HEAD를 소스 근거로 사용하지 않았다.

파일 수정·시험 실행·compile·commit·다른 agent 호출은 하지 않았다. 상대 새 finding 및 보존된 reviewer raw/통합 report 본문도 열람하지 않았다.

## 판정

**이번 post-fix 코드·소유권 문서 delta의 정적 판정: PASS.**

- A-NCLI-01: **FIXED — 정적 재확인**
- A-AC-02: **FIXED — 소유권 충돌의 문서상 해소**
- A-AC-01: **FIXED — 일반 native-aware CLI 구현 부재에 한정**
- 신규 P0/P1/P2/P3: **없음**
- 전체 T-007 수용/완료 판정: **기존 BLOCK 유지**. AC3 연결 시험과 최종 target/자원 근거는 OPEN이다.

이 판정은 전체 Task 완료, CI 성공, target 실행, PR36 ready/merge 승인으로 사용할 수 없다.

## 원 finding 재검토

### A-NCLI-01 — FIXED

위치: `tools/ota/native.py:59–80`, `tests/ota/test_native_container.py:82–117`

기존 실패 시나리오는 ESP block의 scheme 또는 n/e/rr/m을 변경하고 CRC·outer hash·outer 서명까지 갱신했을 때, 외부 공개키를 이용한 서명 검증 성공을 해당 block의 내장 필드 정합성으로 오인하는 것이었다. 영향은 host native 검사 결과의 false PASS이며 장치 검증 우회나 Flash 실행과는 구분한다.

수정에서 다음을 직접 확인했다.

- 길이 최소값과 4KiB 배수 조건을 검사한 뒤 고정 3개 block만 접근한다.
- 각 block에 RSA scheme을 요구한다.
- **현재 block에서 추출한 서명**, caller가 제공한 외부 RSA3072 공개키, signature sector를 제외한 본문을 공식 helper에 함께 전달한다.
- helper 반환값과 **동일 block 전체**가 일치해야 `matched=True`가 된다. 다른 block의 key 일치와 서명 성공을 합산하는 상태는 없다.

공식 esptool v5.4.0 helper는 외부 공개키로 RSA-PSS/SHA256 서명을 검증한 뒤 그 키의 RSA 필드와 본문 digest로 block을 재구성하고 CRC·padding을 붙인다. 단순 인코더가 아님을 source에서 확인했다. [공식 helper 및 RSA block 구현](https://github.com/espressif/esptool/blob/v5.4.0/espsecure/__init__.py#L686-L836)

회귀시험은 CRC를 다시 계산한 6개 필드 변조, 정상 3개 slot, cross-block 혼합을 포함한다. `pack()`은 image hash와 outer 서명을 갱신하므로 이전 CRC/outer 검사에서 우연히 거절되는 것에만 의존하지 않는다. CLI 변조 시험도 `tests/ota/test_native_container.py:228–240`에서 새 출력 미생성과 기존 출력 보존을 검사한다.

권고: 현재 same-block 전체 비교와 해당 회귀시험을 유지한다. 추가 수정 요구 없음. 실제 시험 통과는 본인이 재실행한 결과가 아니다.

### A-AC-02 — FIXED

위치: `docs/tasks/T-007-ota-container.md:169–188`, `docs/resume.md:138–145`

기존 문제는 정상 writer·영속 policy 미구현 기록을 모두 T-007의 선행 완료조건으로 읽으면 상위 설계의 단계별 소유권과 충돌하고 순환 의존을 만들 수 있다는 것이었다.

새 대응은 다음 정본과 일치한다.

- `docs/architecture/ota.md:313–326`: OTA-01 parser/CLI와 후속 OTA-02/03/06 구분.
- `docs/tasks/T-204-esp-ota-recovery.md:6,19–23,49–51`: T-007에 의존하는 ESP normal/recovery·writer·Flash 경계.
- `docs/tasks/T-107-stm32-mcuboot.md:6,19–22,41–46`: T-007에 의존하는 STM bootloader·보호 map·Flash.
- `docs/tasks/T-205-ota-policy-migration.md:18–21,44–50`: ota_manager·영속 policy·단일 journal writer·activation/confirmation.

이는 후속 구현의 owner를 명시한 것이며 필수 검사를 삭제한 면제로 보이지 않는다. T-007의 5개 AC는 변경·체크되지 않았고, 특히 `:207`의 검증 전 erase 금지와 전체 image 검증 전 PREPARED/selector 금지가 유지된다. `:184–188`은 연결 시험·실제 SDK 산출물·target/예산을 남기며 writer 부재만으로 쓰기0회 PASS를 금지한다.

권고: 후속 owner의 실제 API 연결에서도 동일 금지 조건을 재검증한다. 소유권 finding은 닫되 AC3 자체는 OPEN으로 유지한다.

### A-AC-01 — FIXED, 구현 부재에 한정

위치: `tools/ota/container.py:119–136`, `tools/ota/native.py:33–49,52–160`

일반 CLI는 outer 검증 이후 ESP/STM native 검증을 수행하고, 요청한 검증이 끝난 뒤에만 출력을 생성한다. native 공개키는 package 밖에서 공급하며 version·identity·ABI·u64 metadata를 대조한다. STM 경로는 고정 clean checkout의 공식 imgtool을 호출하고 성공 exit를 요구한다.

따라서 합성 golden 전용 코드만 있고 일반 native 검사 연결은 없다는 구현 부재는 해소됐다. 이번 same-block 수정으로 ESP binding 결함도 함께 닫혔다.

다만 이는 제품 signing key 운용·장치 설치 승인·정상 owner 연결 또는 5개 AC 전체 충족을 뜻하지 않는다. 성공 메시지도 `local policy/install NOT_VERIFIED`를 유지한다.

## 신규 finding 및 공격 범위

- P0: 없음
- P1: 없음
- P2: 없음
- P3: 없음

정적으로 공격한 경로:

- CRC를 갱신한 scheme/n/e/rr/m/signature 변조와 outer-valid/native-invalid 입력.
- 한 block의 올바른 키와 다른 block의 올바른 서명 조합.
- slot 0에만 성공을 허용하는 과도한 제한 및 block slice/길이 경계.
- helper 실패 후 잘못된 성공 상태 유지.
- metadata/u64 비교 경로의 회귀와 native 성공의 설치 승인 오인.
- 도구 version·dirty checkout·timeout·실행 실패가 성공으로 처리되는 경로.
- 검증 실패 전 출력 생성 또는 기존 출력 훼손.
- 후속 task 소유권 대응을 이용한 AC3·실제 target·물리 gate의 면제.

확인한 delta에서 추가 결함은 발견하지 못했다. 이는 임의 입력 전수검사나 실행 검증 결과가 아니다.

## 실제 읽은 파일·명령

Candidate 기준 직접 읽은 범위:

- `AGENTS.md`, `docs/README.md`: 전체
- `docs/runbooks/agent-workflow.md`: 전체
- `tools/ota/native.py:1–160`: 전체
- `tests/ota/test_native_container.py:1–252`: 전체
- `tools/ota/container.py:1–141`: 전체
- `docs/resume.md:1–192`: 전체
- `docs/tasks/T-007-ota-container.md:1–217`: 전체
- `docs/journal.md`: 이번 추가분 `:3–25` 전체와 인접 native CLI 기록
- `docs/architecture/ota.md:179–285,311–363`: 관련 계약·소유권·gate
- T-204 `:1–59`, T-107 `:1–54`, T-205 `:1–58`, T-508 `:1–55`: 전체
- `docs/adr/009-ota-native-image-alignment.md:1–46`: 전체
- 공식 esptool v5.4.0 `espsecure/__init__.py`: helper `:686–751`, RSA block 생성 `:815–836`, block 검사 `:934–954` 및 관련 검증 경로

`embedded-architecture`, `embedded-documentation` SKILL.md를 직접 읽고 정본 우선순위·owner·검증 근거 구분에 적용했다. 문서나 코드를 생성·수정하지 않았다.

실행한 읽기 명령 계열:

```text
git cat-file -e "<base 또는 candidate>^{commit}"
git rev-parse "<base 또는 candidate>^{commit}"
git diff --find-renames --name-status <base> <candidate>
git diff --find-renames --stat <base> <candidate>
git diff --find-renames <base> <candidate> -- <해당 5개 변경 파일>
git show <candidate>:<위에 열거한 파일>
git diff --name-only <base> <candidate> -- firmware shared
  tools/ota/container.py tools/requirements-ota-native*.lock
  CMakeLists.txt .github/workflows/foundation.yml
  tests/fixtures/ota-signed-golden
```

마지막 범위의 diff 출력은 비어 있었다. 따라서 production C, 기존 container 연결, lock/build/workflow/golden에 이번 delta 변경이 없음을 확인했다. UUID와 UTC는 PowerShell로 발급·기록했다. 공식 helper는 고정 v5.4.0 URL을 읽었다.

## 실행 근거·미검토·남은 gate

본인의 시험·compile·mutant·SDK/device 실행은 모두 **NOT_RUN**이다. 작성자가 제공한 Debug 147/147, Release 147/147, 8개 공식 host 암호 시험 및 로그 시간은 독립 재실행·로그 감사 결과로 주장하지 않는다. 새 CI/artifact 성공도 확인하지 않았다.

보존 raw 2개, 통합 review 및 archive index의 내용·원문 동일성은 독립성 유지를 위해 재감사하지 않았다. 변경 없는 SDK/MCUboot 전체, dependency lock 공급망, C target 전체, golden binary digest도 이번에 재감사하지 않았다. 핵심 수정 source·시험·소유권 대응의 미열람 부분은 없다.

남은 필수 gate:

1. 검증 실패·금지 target·body/native 실패를 포함한 AC3 권한 경계 연결 시험. writer가 없다는 이유로 금지 호출0회를 PASS 처리하지 않는다.
2. T-007의 최종 actual target 산출물과 allocation/CPU/stack 예산 근거.
3. Candidate에 귀속되는 CI/artifact 및 필요한 독립 closure.
4. T-204/T-107/T-205의 실제 writer·boot·영속 정책 구현과 해당 단계의 재검증.

**Physical/HIL·device crypto·Flash·장치 timing/heap/stack 실측: NOT_RUN. Vehicle TX: NO-GO. PR36: Draft 유지.**

최종 결론: **이번 post-fix 정적 delta PASS, 전체 T-007 acceptance BLOCK 유지.**
