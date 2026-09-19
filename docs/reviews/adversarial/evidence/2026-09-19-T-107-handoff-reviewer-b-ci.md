# T-107 handoff Reviewer B CI 조건 감사 원본

원 reviewer subagent: `01a0b9b3-dc6b-78f3-bcb7-2e4d560d8112`.
아래 요청/결과는 원문 그대로 보존하며 로컬 evidence 링크도 fence 안에서 유지한다.

## 전달 요청

````text
원 T107-HANDOFF-B-1e66a04 리뷰의 열린 조건 증거를 독립 감사하고 후속 raw report를 반환하세요. 작업 기준선은 기존 clean detached F:/dev/canview-wt/review-t107-handoff-b 의 1e66a047c4a47a3535b0346fb0e01d17ccc703b6 그대로. CI35443737495가 현재 success/completed입니다. gh로 job/test 결과, target artifact manifest의33개 bytes/SHA256, 17 source canonicalLF/CRLF digest,34 target log warning/error0를 실제 다운로드/대조하세요(실제 수량이 다르면 관찰값 보고). 해당 CI로 full Debug/Release 회귀 조건이 충족되는지 확인하세요. 보조 로컬 evidence: coordinator build/t107-handoff-candidate-host-debug-test.log는159/160, trust nested configure가VS rc 미초기화로 실패한 원본; 환경을 정상화한 trust retry1/1은 build/t107-handoff-candidate-trust-retry.log, finalsourceRelease160/160은 build/t107-handoff-host-release-test.log. 원본 실패는 보존하고160/160단일Debug라고 바꾸지 않습니다. main workspace 현재 새 failstop 구현 변경 중이므로 거기 source는 검토/변경하지 마세요. 자신의 clean immutable tree와 CIsource만 비교. artifact 다운로드는 자기 ignored build 아래. 코드/문서 tracked edits 금지. 조건 충족 여부와 정확한실행ID/시각/명령/파일/미검토영역/physicalNOT_RUN/P0~P3/verdict 포함 한국어 raw report 반환. 원 report는이미그대로e9f474b에보존했으므로새실행결과만추가기록 예정. 전체T107또는PRmerge승인으로확대하지마세요.
````

## 원본 결과

````markdown
# T-107 handoff — Reviewer B 열린 조건 후속 감사

**판정: PASS — 기존 `1e66a04` handoff delta 리뷰의 전체 회귀·CI 증거 조건에 한정한다.** 두 조건 모두 독립 감사로 충족됐다. T-107 전체 완료, PR merge, 최종 bootloader 또는 차량 사용 승인이 아니다.

## 1. 실행·격리

| 항목 | 결과 |
|---|---|
| executionID | `T107-HANDOFF-B-CI-35443737495-20260919` |
| 원 executionID | `T107-HANDOFF-B-1e66a04-20260919` |
| 역할 | Reviewer B — build/security/test 증거 감사 |
| UTC 시작 | `2026-09-19 13:11:24 UTC` |
| UTC 종료 | `2026-09-19 13:15:10 UTC` |
| candidate | `1e66a047c4a47a3535b0346fb0e01d17ccc703b6` |
| 원 delta base | `965ada0faa6a40925fc460585481615005fcddf1` |
| worktree | `F:/dev/canview-wt/review-t107-handoff-b` |
| 시작·종료 | HEAD 일치, detached, porcelain 출력 없음 |
| tracked/index | 변경 없음 |
| 출력 위치 | 자기 worktree의 ignored `build/ci-audit-35443737495/` |

Main workspace의 진행 중 source는 읽거나 변경하지 않았다. 외부 로컬 접근은 명시된 coordinator 로그 3개에 한정했다. 코드·문서 tracked 수정, commit, push, 하드웨어·키 조작은 수행하지 않았다.

## 2. CI 실행과 source 결합

[CI35443737495](https://github.com/digitie/canview/actions/runs/35443737495)를 `gh`로 조회하고 로그와 artifact를 실제 다운로드했다.

- 상태: `completed / success`
- event: `pull_request`
- head SHA: candidate와 정확히 일치
- 생성: `2026-09-19T12:44:46Z`
- 최종 갱신: `2026-09-19T13:03:59Z`
- 6개 job 모두 `completed / success`

| Job | Job ID | 확인 결과 |
|---|---:|---|
| windows-c99 | 105898994180 | Debug·Release 전체 회귀 성공 |
| browser-contract | 105898994223 | job 성공 |
| linux-portability (clang) | 105898994230 | 147/147, 14.11초 |
| linux-sanitizer | 105898994231 | 147/147, 47.54초 |
| target-firmware-windows | 105898994263 | target 빌드·artifact 업로드 성공 |
| linux-portability (gcc) | 105898994364 | 147/147, 14.66초 |

Workflow의 checkout ref, 실제 checkout 로그, run metadata, target manifest의 `sourceRevision` 및 `expectedSourceRevision`이 candidate를 가리킨다.

Manifest의 PR base는 `c60641f218ca5a9966781cc1e8d417df26bb2712`, synthetic merge revision은 `68a00649636636f93070ab04c8ab5247405eb6ae`다. 원 리뷰의 delta base와 역할이 다르며, 빌드 source가 synthetic merge revision으로 바뀐 증거는 없었다.

## 3. 전체 Debug/Release 회귀 조건

CI job 로그와 다운로드한 두 `LastTest.log`를 교차 확인했다. 각 파일에서 시험 이름 160개, 고유 이름 160개, `Test Passed.` 160개, `Test Failed.` 0개를 확인했다.

| CI 구성 | 결과 | 전체 시간 | trust 시험 |
|---|---:|---:|---:|
| Windows Debug | **160/160** | 329.96초 | 성공, 8.69초 |
| Windows Release | **160/160** | 76.51초 | 성공, 10.39초 |

양쪽 로그에 다음이 포함된다.

- `stm32-boot-handoff` 성공.
- MCUboot image/identity/IO **108시나리오** 성공.
- program/erase 전후 cut **5,610 + 5,630 = 11,240개** 성공.
- trust generator 4개 시험 및 BSP/CMake integration 성공.

전체 job 로그의 근거 위치는 Debug 결과 `run.log:877`, 시간 `run.log:879`, Release 결과 `run.log:1483`, 시간 `run.log:1485`다.

Linux 세 job은 각각 **147개**이며 MCUboot root 미설정에 따른 `MCUboot C model NOT_RUN`이 기록돼 있다. 이를 Windows 160개 회귀나 MCUboot 검증으로 합산하지 않았다.

### 보조 로컬 로그의 원본 실패 보존

세 원본을 읽고 자기 ignored 디렉터리에 복사했다. 복사 전후 원본 hash와 사본 hash가 일치했다.

| 로컬 로그 | 관찰 결과 |
|---|---|
| `t107-handoff-candidate-host-debug-test.log` | **159/160**, 372.43초. trust nested configure에서 `No CMAKE_RC_COMPILER could be found.` |
| `t107-handoff-candidate-trust-retry.log` | **별도 재시도 1/1**, 15.52초 |
| `t107-handoff-host-release-test.log` | **160/160**, 103.07초 |

원본 Debug 실패는 성공으로 변경하지 않았다. **로컬 Debug를 단일 실행 160/160으로 표현하지 않는다.** 환경 문제 설명과 일치하는 RC compiler 누락은 확인했지만, 원 실행의 DevShell 환경 전체를 사후 입증한 것은 아니다.

이 조건의 closure 근거는 별도로 candidate SHA에 결합된 **CI Debug 160/160 및 Release 160/160**이다.

## 4. Target artifact 전수 감사

다운로드한 artifact:

| Artifact | ID |
|---|---:|
| target-firmware-images | 10584333029 |
| target-firmware-logs | 10584727281 |
| windows-foundation-evidence | 10584372499 |

관찰된 수량은 요청의 예상치와 일치했다.

| 검사 | 관찰 결과 |
|---|---:|
| manifest artifact 항목 | 33 |
| 실제 다운로드된 image 파일 | 33 |
| bytes/SHA-256 일치 | **33/33** |
| 누락·중복·manifest 밖 image 파일 | **0** |
| source provenance 항목 | 17 |
| candidate source digest 일치 | **17/17** |
| target `.log` | 34 |
| compiler/linker/CMake warning·error 진단 | **0** |

33개 파일의 크기와 SHA-256을 각각 계산해 manifest와 비교했다. 전체 대조값은 [audit-results.json](/F:/dev/canview-wt/review-t107-handoff-b/build/ci-audit-35443737495/audit-results.json)에 보존했다.

| 산출물 그룹 | BIN bytes | ELF bytes | MAP bytes |
|---|---:|---:|---:|
| STM32 primary-debug boot link 시험 | 5,284 | 62,888 | 101,678 |
| STM32 primary-release boot link 시험 | 3,924 | 18,536 | 86,886 |
| STM32 primary-debug 앱 | 51,768 | 1,070,308 | 300,327 |
| STM32 primary-release 앱 | 39,776 | 58,756 | 210,151 |
| STM32 debug 앱 | 51,760 | 1,074,280 | 300,076 |
| STM32 release 앱 | 39,768 | 62,852 | 209,900 |
| Communicator ESP32 | 164,544 | 3,407,396 | 3,068,586 |
| Diagnostic Bridge | 862,608 | 9,311,108 | 8,769,205 |
| Controller | 159,728 | 3,352,392 | 3,039,405 |
| idf-public-component fixture | 144,816 | 3,153,040 | 2,919,391 |
| idf-ota-image fixture | 655,360 | 4,703,852 | 3,088,232 |

CI target 로그에서 Debug/Release 각각 boot link 검사 3개 성공, trampoline 16B, SRAM copy 1,048/824B도 확인했다. 이는 CI 실행 증거 감사이며 이번 실행에서 target을 다시 빌드하거나 보드에서 실행한 것은 아니다.

## 5. Source digest와 provenance

Clean immutable worktree의 각 파일을 읽어 canonical LF와 CRLF byte열의 SHA-256을 각각 계산했다. **17개 모두 CI manifest의 digest가 CRLF 형태와 일치**했다. LF digest도 개별적으로 결과 JSON에 기록했다. 줄바꿈 외 내용 변경이나 불일치는 없었다.

```text
firmware/communicator/stm32/ld/application-sections.ld
firmware/communicator/stm32/ld/STM32G474CEUx_PRIMARY.ld
firmware/communicator/stm32/ld/STM32G474CEUx_BOOT.ld
firmware/communicator/stm32/platform/stm32g474/startup_flash_ram.c
firmware/communicator/stm32/platform/stm32g474/boot_runtime.c
firmware/communicator/stm32/platform/stm32g474/boot_handoff.c
tools/ota/check_stm32_boot_ram.py
tests/ota/test_stm32_boot_ram.py
tools/ota/validate_stm32_map.py
tests/ota/check_stm32_primary_image.py
firmware/communicator/esp32/sdkconfig.defaults
firmware/diagnostic-bridge/sdkconfig.defaults
firmware/controller/sdkconfig.defaults
tests/fixtures/idf-ota-image/sdkconfig.defaults
tools/generate_boards.py
tools/generate_bridge_web_assets.py
.gitattributes
```

SDK provenance의 STM32CubeG4 commit은 candidate의 pin과 일치했다.

```text
STM32CubeG4: d11b194a9f05d1b143d154771f3dbc282c8052a5
Arm release: 15.3.Rel1
Arm archive SHA-256:
b85669d3408e2ae713b17b0cc59bc4ea26369a7f2bd19108fd11df7095f159e6
```

Arm archive 자체를 이번 감사에서 다시 다운로드해 hash한 것은 아니다. 위 archive 값은 다운로드한 CI provenance의 기록이다.

## 6. 실행 명령과 읽은 증거

모든 명령의 작업 디렉터리는 지정 reviewer worktree였다.

```powershell
git rev-parse HEAD
git symbolic-ref -q HEAD
git status --porcelain=v1 --untracked-files=all
git diff --exit-code
git diff --cached --exit-code
git remote -v
Get-Command gh

gh run view 35443737495 --repo digitie/canview `
  --json databaseId,headSha,headBranch,event,status,conclusion,createdAt,updatedAt,url,jobs,workflowName

gh api repos/digitie/canview/actions/runs/35443737495/artifacts --paginate

gh run download 35443737495 --repo digitie/canview `
  --dir build/ci-audit-35443737495/download `
  -n target-firmware-images -n target-firmware-logs -n windows-foundation-evidence

gh run view 35443737495 --repo digitie/canview --log

& ./build/ci-audit-35443737495/audit.ps1 `
  > build/ci-audit-35443737495/audit-results.json
```

[감사 스크립트](/F:/dev/canview-wt/review-t107-handoff-b/build/ci-audit-35443737495/audit.ps1)는 ignored 디렉터리에만 작성했다. 경로 범위 검사, artifact 크기/hash, 중복·누락, LF/CRLF source hash, target 진단 검색, host 시험 개수를 검사한다.

추가 실행은 `Get-Content`, `rg -n`, `Select-String`, `Get-ChildItem`, `Get-FileHash -Algorithm SHA256`, 명시된 세 로그의 `Copy-Item`, `git check-ignore`였다. Target 진단 검색식은 다음과 같다.

```text
warning:|CMake Warning|ld\.exe: warning|error:|fatal error|FAILED:
```

이번 실행에서 읽은 파일·증거:

- immutable tree의 `AGENTS.md`, `docs/README.md`, `docs/resume.md`.
- `.github/workflows/foundation.yml`의 host 실행, checkout, artifact 및 provenance 관련 부분.
- `tools/toolchain-versions.json`.
- 위 source provenance 17개 파일의 byte 내용.
- `run.json`, `artifacts.json`, `run.log`.
- 다운로드된 `target-artifacts.json`, `stm32cubeg4-provenance.json`.
- manifest에 열거된 target 산출물 33개 전체의 bytes/hash.
- target 로그 34개 전체의 진단 검색 및 STM32 primary 빌드 검사 결과.
- Windows Debug/Release `LastTest.log` 전체의 시험 이름·성공/실패 집계와 관련 시험 출력.
- coordinator 로그 원본 3개와 hash가 동일한 사본.

34개 로그 이름, 33개 artifact 경로·크기·hash, 17개 source의 LF/CRLF hash는 결과 JSON에 모두 열거돼 있다.

감사 결과 파일 SHA-256:

```text
a530bfe5d107e71df89b9680ee3f311844b973c5b174ea0b7d8e2f6cb39ce8bd
```

## 7. Findings·미검토 영역·최종 판정

| 심각도 | 신규 finding |
|---|---:|
| P0 | 0 |
| P1 | 0 |
| P2 | 0 |
| P3 | 0 |

결함별 위치·시나리오·영향·권고는 신규 finding이 없어 해당 없음이다. 로컬 Debug의 RC 환경 실패는 원본 그대로 보존했으며, candidate CI의 별도 성공으로 전체 회귀 조건을 충족했다.

이번 실행은 원 리뷰의 증거 조건 감사다. 다음은 검토·실행하지 않았다.

- candidate 이후 failstop 구현과 다른 source 변경.
- 기존 19파일 source review의 재수행.
- browser screenshot의 시각적 검토.
- CI 외 환경에서 전체 회귀·target 빌드 재실행.
- 최종 `boot_go`·policy-v1·floor·activation·confirmation·handoff 통합 승인.
- 실제 physical handoff, NMI/reset timing, torn flash, ECC·전원 차단·watchdog/GPIO qualification.

**기존 CONDITIONAL의 전체 회귀·candidate CI artifact 감사 조건은 충족됐다. 해당 handoff delta의 Reviewer B 판정을 PASS로 종결한다.** 원 report는 변경하지 않으며 이 결과는 별도 후속 기록이다.

**Physical/HIL NOT_RUN, 차량 TX NO-GO. T-107 전체 완료 및 PR merge 승인은 부여하지 않는다.**
````
