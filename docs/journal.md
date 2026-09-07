# CANView 작업 일지

## 2026-09-07 (codex, T-200a merge와 T-400a 시작)

PR #22의 최종 `84080fe`에서 clean STM32 Debug/Release·ESP4종 binary warning0, local26개/원격18개 artifact 크기·SHA-256, CI5개 SUCCESS를 확인했다. 두 reviewer가 최초 P1 2건/P2 2건 모두 FIXED와 최종 문서 delta PASS를 독립 확인했다. `--match-head-commit` merge 결과는 `2222290`이며 [최종 evidence](reviews/adversarial/evidence/2026-09-07-T-200a-merge.md)에 원문·검증·NOT_RUN을 연결했다.

사용자 MCU/core 순서에 따라 새 `codex/t400a-bridge-core-bench`에서 T-400a를 분리했다. Bridge N8R2는 현재 실제 SDKCONFIG에서도 Quad/80MHz였으며 기존 GPIO4/5와 R14 외부 pull-up mapping을 유지한다. 공용 core와 보드별 메모리·GPIO 계약을 분리하고 Communicator 회귀를 보존한다. SoftAP/HTTP/무선/OTA·실물 acceptance는 T-400에 남겼고 단순 task 분리를 완료로 표시하지 않는다. embedded 구조/C/RTOS/driver/ISR/문서 지침을 다시 대조했으며 새 ISR·통신 task는 이 범위에 만들지 않는다.

## 2026-09-07 (codex, T-200a 구현·독립 리뷰 수정)

ESP32 C99 health/fixed pool·safe BSP·IDF TWDT/PSRAM/heap/USB adapter와 단일 owner app을 구현했다. Windows89/89, SDK fixture·app 실패 단계·동시 pool·coverage와 clean6종 warning0을 확인했다. [검증 evidence](reviews/adversarial/evidence/2026-09-07-T-200a-validation.md)는 commit별 결과와 미실행 HIL을 분리한다.

`55c7801`의 독립 리뷰 원본을 모두 저장한 후 비교했다. panic HALT 허용(P2), uintptr_t strict API 실패(P1), bootloader factory-reset/NVS 삭제 config 허용(P1), 자기참조 금지 목록 시험(P2)을 수정했다. PRINT_REBOOT/지연0·비휘발성 변경 차단과 독립21개 삭제 변이를 추가하고 실제 SDK 정상 build 및 factory/HALT configure 거부를 확인했다. 원문 내부 fence를 보존하기 위해 문서 link validator의 fence 처리를 보강하고 회귀4개를 추가했다. 모든 delta는 post-fix 독립 재검토 대상이며 아직 merge 승인이 아니다.

## 2026-09-07 (codex, T-102a merge와 T-200a 시작)

최종 head `3fd2b86`의 Windows/GCC/Clang/sanitizer/target CI5개 SUCCESS와 clean6종 binary warning0을 확인하고 PR #21을 `db5ed19`로 merge했다. 최초 리뷰5건 및 GCC portability1건은 두 reviewer가 독립 재확인해 모두 FIXED다. [최종 merge evidence](reviews/adversarial/evidence/2026-09-07-T-102a-merge.md)에 정확한 hash·원격 artifact·전체 host74/74를 보존했다. 보드 flash/HIL·차량 gate는 닫지 않았다.

사용자 요청의 MCU/core 순서에 따라 `codex/t200a-esp32-core-bench`에서 T-200의 boot/health/watchdog·고정 pool·config 검증 소프트웨어를 T-200a로 분리한다. 기존 실제 PSRAM/heap/USB/GPIO/UART 계측 acceptance는 T-200에 유지한다. ESP-IDF SDK API와 생성 board 설정을 먼저 대조했고 RTOS·C·구조·문서·driver 지침을 적용한다. 전체 phase를 건너뛰어 radio/CAN/OTA/provisioning을 활성화하지 않는다.

## 2026-09-07 (codex, T-102a 구현과 적대적 리뷰 수정)

`ca1a299`의 최초 2인 리뷰에서 deadline miss 소거(P1), NMI/feed 경쟁(P2), callback API 문서 실패(P1), 필수 vote 회귀시험 공백(P2), 개별 .su 누락 허용(P2)을 확인했다. [원본·교차 확인·수정 기록](reviews/adversarial/2026-09-07-T-102a.md)에 severity를 보존했다. 실제 완료 us 기반 deadline 판정, terminal NMI reset, 함수 type typedef, 독립 worker/vote 변이시험과 compile database별 stack evidence 대조를 적용했다. board flash/provisioning은 하지 않았다.

초기 Windows Debug/Release·WSL ASan+UBSan은74/74 PASS였고 새 fixture·변이 검출·target gate를 재실행했다. `e928cf6` target 일괄 빌드는 리뷰 결함 수정으로 대체하므로 중단하고 부분 산출물을 최종 증거에서 제외했다. run34064143948의 Windows host는 Doxygen 다운로드 실패였으며 최종 CI로 다시 확인한다. 아직 T-102a DONE/merge가 아니다.

## 2026-09-07 (codex, T-004 merge와 T-102a 시작)

PR #20의 최종 head `182f975`에서 Windows/Linux host·sanitizer와 Windows target CI10건 SUCCESS를 확인하고 merge `caafc24`를 검증했다. 추가로 같은 clean head에서 STM32 Debug/Release·ESP32 네 프로젝트를 새 디렉터리에 모두 clean build해 warning/error0과 동일 ESP version을 확인했다. [merge evidence](reviews/adversarial/evidence/2026-09-07-T-004-merge.md)에 binary hash와 CI 링크를 보존하고 T-004를 DONE archive로 이동했다.

사용자의 재개·core/base 우선 요청에 따라 `codex/t102a-stm32-core-bench`를 만들었다. T-102의 최소 boot/fault image를 T-102a로 분리하되 G1/G2 실측 의무와 기존 후속 task의 선행은 유지한다. Windows PnP 조회에서 ST-LINK/STM32 시리얼 장비를 식별하지 못했으므로 장비 준비 정보를 사용자에게 질문했으며 자동 flash·option-byte 변경은 하지 않는다. embedded architecture/cstyle/documentation/driver/ISR 지침에 따라 SDK 독립 실패 정책·정적 queue·scheduler와 CMSIS backend를 분리한다.

## 2026-09-07 (codex, T-004 구현·검증 closure)

UART 1.0 schema/generator·semantic payload 검증·COBS/CRC stream·plan transaction·command cache·link/session/replay를 구현했다. 적대적 리뷰에서 발견한 stale heartbeat/cache, 방향·auth 혼동, enqueue 전 replay commit, borrowed payload 수명, quota와 staging timeout을 수정하고 재시험했다. 동기 queue copy와 단일 worker 소유권을 공개 계약에 명시했다. 최초·중간 finding과 2인 post-fix 원문은 [T-004 통합 기록](reviews/adversarial/2026-09-07-T-004.md)에 모두 보존했다.

- `3c6967a` 기준 Windows Debug/Release와 WSL Clang ASan+UBSan은 각각 68/68 PASS. UART function100%·line91.24%·branch79.12%, core coverage·schema·strict API docs도 PASS다.
- C `soak-24h`는 양방향 합계 69,120,000,130 byte를 실제 decoder에 입력했고 315,103회 손상 주입과 guard/resync/counter 검사를 801.87초에 통과했다. 이는 4Mbps·8-N-1의 방향당 24시간 byte budget이며 실제 벽시계 24시간·실물 UART 시험이 아니다. Python virtual-time 시험만으로 full-rate를 주장했던 근거는 대체했다.
- STM32 Debug/Release clean build와 ESP32 네 프로젝트 incremental build의 최종 바이너리·SHA-256·warning/error0을 [evidence](reviews/adversarial/evidence/2026-09-07-T-004-validation-final.md)에 기록했다. ESP 버전 문자열은 병행 문서 커밋 때문에 서로 다르며 단일 재현 bundle로 표시하지 않았다.
- B 최종 T-004 PASS, A 최종 조건부 PASS의 physical 문구 구분을 반영했다. 코드 P0/P1은 남지 않았으며 원격 CI 완료 전 merge하지 않는다. PR #20 merge 후 DONE 기록을 갱신한다.
- 최신 사용자 재개 요청에 따라 PR merge 후 후속 task를 계속한다. 실제 UART DMA/RTS/CTS·보드 flash·CAN/RF/HIL·production provisioning·차량은 NOT_RUN이다.

## 2026-09-07 (codex, T-004 UART schema/codec 시작)

T-003 PR #19가 `4ee017b`로 main에 merge된 것을 확인하고 `agent/codex-t004-uart-schema-codec`에서 T-004를 시작했다. T-004는 ESP-NOW tunnel이 아닌 Communicator ESP32↔STM32 내부 UART v1.0 semantic ABI, generated C header, fixed-buffer COBS/CRC codec/parser와 host fault simulation만 다룬다. UART DMA/실물 RTS/CTS, 보드 flash, CAN/HIL, recovery UART는 후속 task와 별도 gate다.

- 직접 설치한 SDK 기준은 유지한다: ESP-IDF `6.0.3` (`C:\cv\esp-idf-6.0.3`), STM32CubeG4 `1.6.3` (`C:\cv\STM32CubeG4-1.6.3`), Arm GNU `15.3.Rel1`, CMake `4.4.3`, Ninja `1.13.2`.
- 먼저 schema→generator→semantic codec→fault/negative test→host/target build 순으로 진행하고, draft PR에 작은 커밋을 원격 반영한다. 생성 ABI drift와 실제 target warning을 별도로 기록한다.

## 2026-09-07 (codex, T-003 ESP-NOW codec/session/QoS closure)

T-002 merge `c18a8a5` 이후 branch `agent/codex-t003-espnow-codec-session`에서 T-003을 구현했다. 사용자 변경은 보존했으며, target SDK를 직접 설치·검증했다: ESP-IDF `6.0.3` (`C:\cv\esp-idf-6.0.3`), STM32CubeG4 `1.6.3` (`C:\cv\STM32CubeG4-1.6.3`), Arm GNU `15.3.Rel1`, CMake `4.4.3`, Ninja `1.13.2`.

- generated ESP-NOW contract/TLV table, byte-safe C codec, session lifecycle cookie·secure binding·anti-replay·pairing/control adapter, fixed pool/rate limiter/QoS1 scheduler와 C/Python reference/fault tests를 추가했다. generated output check와 source/header byte stability gate를 유지했다.
- 최신 local 재검증: host Debug/Release 각각 CTest `49/49`, Coverage core `9/9`와 line/function `100%`, branch `99.63%`, Python unit `35`, generated/negative/budget/plan/link/API docs gate PASS. hash-locked docs dependency는 `tools/requirements-docs.lock` 경로로 strict install PASS했다.
- STM32 Debug/Release와 communicator/diagnostic bridge/controller/public IDF fixture ESP32-S3를 current source commit `6a076a3`에서 clean build했다. 모두 exit code `0`, strict `warning:|error:|CMake Error|ninja: error` scan `0`이다. 결과와 SHA-256은 [target evidence](reviews/adversarial/evidence/2026-09-07-T-003-target-final.md)에 둔다.
- 최초 B 리뷰의 P1은 stale target evidence(`1780e0a`)였다. 전 target image를 재빌드하고 evidence를 `6a076a3`에 bind한 commit `67ccee9`를 push한 뒤 A/B post-fix 재검토가 모두 PASS했다. 원본·disposition은 [T-003 review](reviews/adversarial/2026-09-07-T-003.md)에 보존했다.
- PR #19는 draft 상태에서 시작해 최종 gate 이후 merge한다. board flash/boot, reset/brownout, RF/CCMP·mbedTLS runtime, CAN/HIL·차량, production OTA signing/provisioning은 장비·승인 범위 밖이라 `NOT_RUN`으로 유지한다.

## 2026-09-06 (codex, C99 기반 코드·넓은 시험·생성 API)

기준선 b529a722fb813d5b60ab667675d73996895ea3fc에서 agent/codex-firmware-foundation을 만들었다. 사용자 dirty 변경 없이 시작했으며 기존 v1.2 prototype은 수정하지 않고 별도 host 회귀로 보존했다. embedded-architecture/cstyle/documentation 원칙으로 SDK-free codec/app, BSP/platform, caller ownership, API 오류·수명 계약을 분리했다.

- 세 장치·네 MCU startup/BSP/config와 생성 pin header, strict C99 envelope/CRC/COBS/classic CAN batch/sequence를 추가했다. radio/CAN/OTA 활성화 경로는 없다.
- 사용자가 Doxygen 허용을 명확히 한 뒤 Sphinx9.1.0/Breathe4.36.0/Doxygen1.18.0/Furo2025.12.19와 hash lock을 선택했다. 공개 함수14개 계약 검사 및 warning0 사이트 build를 실행했다.
- 독립 C 표준 자문 Boole(01a074a0-9ee0-7883-83da-4e4a79fbe3ea)은 C17을 권고했으나 사용자 C99 지시를 유지했다. SDK GNU23과 공용 C99를 분리하고 C99 정적 검사를 추가했다. 자문은 2인 코드 리뷰 PASS를 대체하지 않는다.
- 공식 archive SHA256 검증 후 .tools의 Clang23.1.0/CMake4.4.3/Ninja1.13.2/Doxygen1.18.0으로 Windows Debug/Release 각각31/31 CTest를 통과했다. core9그룹·독립 Python/C2195 vector·BSP4종 실패 주입·generator·legacy12회귀·문서/plan gate 포함.
- 새 profile coverage: core 실행line546/546(100%), function20/20(100%), branch295/296(99.66%). BSP/SDK/Python/legacy를 분모에 포함하지 않았다.
- tools/build_docs.py 실제 통과. 작성 중 Doxygen output parent 부재와 Sphinx 함수 pointer 표기 경고를 발견하고 builder 및 명시적인 함수 type typedef로 수정한 뒤 재검증했다.
- tools/environment/setup-windows.ps1 -VerifyOnly: 일반 shell CMake 부재, 고정 host shell에서는 arm-none-eabi-gcc 부재로 실패. target SDK build/실보드/단전/차량 gate는 NOT_RUN이다.
- T-001은 IN_PROGRESS이고 전체 ABI·SDK target CI·HIL acceptance는 열어 둔다. 새 2인 독립 리뷰와 remote CI 결과는 후속 review closure에 기록한다.
- 최초 remote CI에서 Windows cp1252 한글 출력 오류와 GCC 정수 승격 경고를 확인했다. CTest Python -X utf8/host PYTHONUTF8, byte version 손상의 XOR 복원과 기존 brightness 시험 비교 상수 한 곳을 수정했다. v1.2 runtime 동작은 변경하지 않았다. beae8a9의 [CI34009610099](https://github.com/digitie/canview/actions/runs/34009610099)는 Windows 전체 gate와 Linux GCC Release/Clang ASan+UBSan 모두 PASS다.
- 새 .tools/api-venv의 hash-locked 설치가 끝나기 전에 문서 생성을 실행한 한 차례는 Sphinx 미설치로 실패했다. 설치 완료 후 동일 명령의 strict build 및 pip check를 통과했으며 이 초기 순서 오류를 성공으로 집계하지 않았다. 기존 plan validator 부정 fixture35개도 별도로 통과했다.
- 59ac404의 독립 리뷰 A(P2 세 건)·B(P2 두 건) 원문을 각각 보존한 뒤 비교했다. GPIO 검증 finding은 중복이며, 오디오 PLAY16/REC14 교정·SoC/module allowlist·IDF GNU adapter 분리·startup4역할 C99 compile·CAN 전체 필드/독립208 golden을 반영했다. core runtime byte 알고리즘은 변경하지 않았다.
- 수정 후 Windows Debug/Release 각31/31, generator7그룹, envelope/COBS2195+CAN208 vector, coverage546/546·295/296, API strict14계약 PASS. bus 상수0·bus offset 오기·encode/decode 동시 delta endian 반전의 시험용 변이3종은 새 golden gate가 모두 검출했다. 원문에 포함된 임시 worktree 링크는 literal evidence로 보존했다. post-fix 재검토와 최종 CI는 closure report에서 추적한다.

## 2026-09-06 (codex, 전체 계획 두 차례 점검·UI 개선)

기준선 `4aeb2912da063c6fcb0d8715aa46f84c7d1d1b0f`의 사용자 변경을 보존하고 기존 PR16 branch에서 작업했다. 계획/자동화/LVGL/진단 웹을 분리한 작업자와 main의 2차 대조를 수행했다. [요구 추적표](architecture/requirements-coverage.md)에 42개 요구와 46개 상세 task, 남은 실제 gate를 연결했다. embedded-architecture/documentation/cstyle 원칙으로 의미 명령·상태 소유권·ISR/queue 경계·실행되지 않은 gate를 분리했다. Hallmark는 사용하지 않았다.

- 계획: 누락된 OTA8단계·PCB 제작·audio bench/SPORT source task11개, 순환 의존성 제거, schema phase/session/reason·설정 A/B 정본 충돌을 수정했다. 상세 task DONE 승격 없음.
- UI: 운전자/진단 웹 각5뷰, 4WD·순간연비·RPM 옆 보조값·FFT 차속/RPM·signed dBFS·날짜/60개 분·정차잠금·경고 touch-through·미수신/로컬 초안을 개선했다. DPF lamp OFF를 전체 정상으로 표시하지 않는다.
- 자동화: stale/idle 반복 감광·boost base 오염, volume pending·FFT invalid, SPORT stale/수동 mode·tick gap을 수정했다. 수정 전7개 failure의 재현과 수정 후 회귀를 확인했다.

작성자 실제 검증:

| 명령/환경 | 결과 |
|---|---|
| `python -B -X utf8 tools/validate_plan.py` | 상세46·metadata/선행 DAG 오류0 |
| `python -B -X utf8 -m unittest discover -s tests -p test_plan_validation.py -v` | 부정 fixture 포함35시험 PASS; 작성 중 최초0-test 결과는 통과로 집계하지 않음 |
| `node tools/ui/check-browser.cjs --screenshots` (Playwright+Edge, 외부망 차단) | 최초 운전자72검사·진단10그룹, 촬영 전환 상태 회귀 추가 뒤 운전자74검사; JS 오류/외부 요청0; `docs/images/` 재생성 |
| VS Developer PowerShell → `cmake -S tests/automation -B .tools/automation-main-build -G Ninja -DCMAKE_BUILD_TYPE=Debug`, build, CTest | MSVC19.50 `/W4 /WX /utf-8`, 최초11/11·SPORT gap 철회 교차 회귀 추가 뒤12/12 PASS |
| `./tools/ui/validate-lvgl.ps1` | 공식 LVGL8.4.0 `4495f42` 실제 C 링크·수명/상태 Debug 회귀 PASS. 같은 CMake를 `.tools/lvgl-release-build`/Release로 빌드한 assertion-enabled 회귀도 PASS. 최종 변경은 독립 review에서 재확인 |
| Python navigation/hardware unittest | 기존16+9시험 PASS; 실제 RF/전기적 HIL 아님 |
| KiCad10.0.6 Python `tools/hardware/validate_exports.py`, `check_margins.py` | 4보드 export/pad/BOM 정합성 PASS, 저장 ERC0 확인·정적 margin 재검산; PCB/아날로그 승인 아님 |
| `tools/validate_document_links.py`, `git diff --check` | 상대 링크·공백 검사 PASS, 최종 closure 후 재실행 |

일반 PowerShell의 `setup-windows.ps1 -VerifyOnly`는 CMake PATH 부재로 실패했다. 설치된 VS dev shell의 CMake4.2.3-msvc3/Ninja1.12.1은 host 시험에 사용했지만 잠금 target toolchain CMake4.4.3/Ninja1.13.2/Arm15.3.Rel1을 충족했다고 표시하지 않는다. ESP/STM target, OTA runtime·PCB·전원 차단/HIL·Android/iOS 실기기·최종 한글 font/LCD FPS/8시간 soak·실차 evidence는 미실행이다. CodeGraph 미초기화로 `rg`·정본 직접 읽기·compiler/test로 추적했다.

작업자와 별도의 독립 reviewer2명이 동일 immutable `d078437`을 검토했다. 최초 A는 PASS, B는 gap에서 SPORT 소유권 철회 누락(P1)·evidence enum 정본 충돌(P2)·경고 촬영 시점(P3)을 발견했다. 두 원문을 먼저 저장한 뒤 교차 검토했다. gap에서도 fresh 외부 mode 관찰은 권한을 철회하도록 수정하고, UNKNOWN 포함 evidence 등급과 REJECTED 심사 상태를 분리했다. 촬영 문제와 모든 LVGL 탭의 경고 면적도 보완했다.

새 `sport-gap-revocation`은 정상 ECO→SPORT 진입 후 4개 외부 mode×5개 elapsed 경계를 교차한다. 수정 전 assertion 실패가 Windows CRT dialog에서 대기해 해당 시험 process만 종료했고, headless stderr/abort 설정과 CTest 30초 timeout을 추가했다. 수정 뒤20경우와 전체12시험을 통과했다. 최종 post-fix 재검토·원문·finding disposition과 PR 결과는 [새 리뷰 기록](reviews/README.md)에 보존한다. 이 일지나 prototype으로 차량 CAN TX를 허용하지 않는다.

## 2026-09-06 (codex, 독립 OTA·N16R8 회로)

사용자의 N16R8 선택에 따라 WROOM-1-N16R8, 내부 bundle staging, 외장 SPI NOR 미실장을 채택했다. reset/BOOT0·복구 버튼·물리 CAN 차단을 실제 생성 입력과 KiCad 산출물에 반영했다. embedded-architecture/documentation 스킬을 적용해 플랫폼 경계와 전원 차단 수용 조건을 분리했다. 사용자 단일 MD 요청을 우선해 설계와 두 전문 리뷰어 원문·disposition·재검토를 [OTA 문서](architecture/ota.md)에 누적한다.

- 최초 리뷰 P1 두 건(인터록 GPIO 역구동, 승인 전 PREPARED 자동 설치), P2 한 건(영속 downgrade 정책 부재)을 회로/테스트/설계에서 수정했다. 최종 closure는 OTA 문서의 동일 immutable post-fix 재검토가 정본이다.
- KiCad10.0.6 전체 재생성:4보드 ERC0, 정합성·전원 margin·hardware9시험 PASS. CSV 접근 일시 실패 후 전체 재실행 성공.
- ESP target VerifyOnly는 CMake 부재로 실패. OTA target 구현·PCB·전원 차단 HIL·제작 및 차량 송신 승인은 미완료다.

## 2026-09-05 (codex, R1 독립 리뷰 수정)

동일 immutable `06bb51c72180f9c040db3ccf0b223a823c570409`를 전문 reviewer2명이 object-only 방식으로 검토했다. 두 원문을 상호 공개 전에 보존했다. P1 세 건(게이트 DC VGS 정격, PHY rail 소실 시 FT fail-open, USB CC 제어기 VDD 범위)을 BUK7Y12·active-high AHCT126·USB전용3.3V로 수정했다. B의 capability/UNAUTHORIZED payload/헤더 이름과 양쪽의 CRLF 해시 finding도 반영했다.

- Windows KiCad10.0.6 전체 export: ERC0/waiver0,343개 BOM item/1,235 named pad 정합성 PASS.
- Windows Python: navigation16시험, hardware net/Boolean5시험 PASS. 후자는 HIL/아날로그 과도 시뮬레이션이 아니다.
- PDF56개/1,745쪽/95,230,736byte의 크기·SHA·parse 오류0. 기존 원문/land 미확보 gate는 유지한다.
- 생성 text canonical LF와 immutable Git blob hash 검사 경로를 추가했다. 신규 USB회로/FET/FT enable은 기존 footprint 또는 소형 LDO만 사용하며 보드 소형화 우선을 유지한다.
- 수정 기준선의 원 reviewer 재검토와 최종 disposition은 별도 review report에 기록한다. 이 로그만으로 P1 closure나 제작 허용을 선언하지 않는다.

## 2026-09-05 (codex, R1 상세 회로·센서 확장)

**범위**: 사용자가 명확히 선택한 가격보다 소형화 우선 기준으로 네 보드 회로·local footprint·BOM·FW 핀맵과 센서 protocol을 작성했다. 이전 작업자의 KiCad version/Windows 문서·S3 footprint 수정을 유지하고 합쳤다. `embedded-architecture`와 `embedded-documentation` 원칙에 따라 센서 owner, wire 정본, 실측 gate와 후속 task를 분리했다.

**설계**: 자동차/USB-C 전원 mux, automotive-only PHY/GPS, MAX20040 adjustable5.0875V/외부 bootstrap diode, TCAN1046 DYY pin 수정, reset/rail/WD latch 차단, MAX3055 자체 rail을 따르는 TX/EN gate,24개 테스트 패드. MTi7 DR+BMP384 AUX SPI, cased GPS UART/PPS, LVDS 원격 T5848 mic, 기존 Waveshare RTC 재사용. 전원·센서 경계는 ADR-006이다.

**검증**:

- Windows KiCad10.0.6 `tools/hardware/export-review.ps1`: 네 보드 ERC0개, waiver0개, 총333개 BOM item(테스트 패드·DNP 포함)/1,209 named pad의 net·pad·BOM 정합성 PASS. source에서 XML/sexpr/PDF를 재생성했다.
- `tools/hardware/check_margins.py`:5V/supervisor/OV/WD 정적 계산 통과. FB 누설 가정과 빠른 collapse 지연·ripple·SOA 미포함을 명시했다.
- Windows Python `-m unittest discover -s tools/protocol -p test_navigation_codec.py -v`:12개 host 시험 통과. 실제 session allocator/cache·role 검사·RF/STM 통합 구현은 T-100b 후속이다.
- 제조사 PDF54개,1,709쪽,93,272,603byte의 전체 페이지 parse·SHA-256·크기 검사 오류0. 미확보2건과 최신판/land 미확보는 별도 기록했다.
- 회로 PDF의 diode 극성과 global label 방향을 눈으로 확인하고 preview를 실제 export에서 다시 만들었다. native exporter 순서 race는 `Start-Process -Wait`와 독립 netlist 검사로 수정했다.
- CodeGraph의 현재 연결은 다른 프로젝트이므로 사용하지 않았다. `rg`, source/schema 참조, local link 검사와 독립 export 검사로 영향 범위를 확인했다. WSL은 검색·patch·다운로드 보조, 생성/검증/Git은 Windows executable이다.

**남은 조건**: MAX20040 land90-0409 원본 overlay, 구판/미확보 원문, 구매 R/C·harness·PCB/열/loop/SOA·HIL은 미완료다. T-100은 IN_PROGRESS, T-100b는 BLOCKED를 유지한다. 실제 보드·오실로스코프·차량이 없는 상태를 시험 완료로 표시하지 않는다. 전문2인 immutable 적대적 리뷰 결과는 별도 archive에 기록한다.

## 2026-09-05 (codex)

**작업**: 최신 Windows EDA export와 Communicator 회로 산출물 정합성 보정

**변경**:

- KiCad `10.0.6`을 현재 안정 EDA baseline으로 manifest와 문서에 고정하고, `export-review.ps1`로 생성기·XML netlist·ERC JSON·PDF export를 한 번에 재현하게 했다.
- STM32 UFQFPN48 7번 패드 표기를 공식 `PG10-NRST` 이중 기능으로 맞췄다.
- ESP32-S3-MINI-1-N4R2에 S2 footprint를 사용하던 참조를 제거하고, Espressif 공식 S3 land pattern 기반 전용 footprint로 교체했다.
- `09_can_ft`와 `14_can_connectors`를 생성기 호출 순서와 동일하게 분리해 11개 hierarchical sheet, BOM, pinmap, connectivity, schematic, netlist가 같은 입력에서 나오도록 갱신했다.

**검증**:

- Windows KiCad bundled Python과 KiCad CLI `10.0.6`으로 생성·netlist·ERC·PDF export를 실제 실행했다.
- ERC는 23개 violation을 보고했다. 기존 power/isolated-label 및 PCB·SI·transient 미검증 gate가 남아 있어 제작·차량 연결 승인은 아니다.
- ESP-IDF/STM32 target compile은 현재 셸에 해당 host tool이 없어 미실행으로 유지했다.

## 2026-09-05 (codex)

**작업**: 최신 Windows 임베디드 개발환경과 target build bootstrap 구성

**결정**:

- ESP-IDF `v6.0.3`, STM32CubeG4 `v1.6.3`, CMake `4.4.3`, Ninja `1.13.2`, Arm GNU Toolchain `15.3.Rel1`을 manifest에 고정했다.
- ESP-IDF `v6.0.3` peeled commit `76f5dedd9950a3012fee8fb7d5586df21fc67802`, STM32CubeG4 `v1.6.3` peeled commit `d11b194a9f05d1b143d154771f3dbc282c8052a5`을 기록했다.
- 버전 선택과 upgrade 규칙을 [ADR-005](adr/005-latest-windows-embedded-toolchain.md)에 기록했다.

**변경**:

- `tools/environment/setup-windows.ps1`가 Windows host tool version, SDK checkout commit, ESP-IDF export와 핵심 SDK 파일을 검증한다.
- `firmware/controller/`와 `firmware/communicator/esp32/`에 독립 ESP-IDF project, `main`, 향후 public `canview_protocol` component, `sdkconfig.defaults`, partition table를 추가했다. T-002 전에는 incomplete v1.2 header를 application dependency로 연결하지 않는다.
- STM32 CMake minimum/preset/toolchain에서 CMake 4.4, Ninja, Arm GCC 15.3.x를 검증하고 memory usage report를 출력하도록 했다.
- `canview_can`의 private protocol include path를 public `REQUIRES canview_protocol` 경계로 바꿨다.

**검증**:

- `git diff --check` 통과.
- 현재 실행 셸에는 CMake, Ninja, Arm GNU compiler, ESP-IDF와 STM32CubeG4 checkout이 없어 실제 target configure/build는 미실행이다. 따라서 T-200/T-300/T-102 acceptance는 완료로 표시하지 않는다.
- Windows에서 실행할 전체 준비 명령은 [tools README](../tools/README.md)와 [장치별 toolchain](development/toolchains.md)에 기록했다.

## 2026-09-05 (codex)

**작업**: 문서 정보구조 재정립과 실행별 독립 적대적 리뷰 gate 도입

**변경**:

- `AGENTS.md`를 공통 정책·안전 경계·단계별 읽기 규칙의 짧은 정본으로 정리하고, 사용자가 보강한 Ruthless Review 원칙을 유지했다.
- `docs/README.md`를 중앙 router로 추가하고 상세 설계를 architecture·hardware·development·vehicle·UI 하위 디렉터리로 이동했다.
- `SKILL.md`는 정책 사본이 아닌 작업별 문서 router로 축소했다.
- 적대적 리뷰는 매 실행마다 새 report를 만들고, 서로 다른 전문 영역의 reviewer subagent 2명이 같은 immutable 기준선을 독립 검토하도록 workflow와 archive를 정의했다.
- 기존 기준선 리뷰는 `docs/reviews/adversarial/2026-09-04-baseline-design.md`로 이관하고 ADR-004에서 새 정본 관계를 기록했다.

**환경**: branch·status·commit은 Windows Git을 정본으로 사용했다. 대량 상대 링크 경로 수정에는 Windows Python을 찾지 못해 WSL `python3`를 일회성 보조 도구로 사용했으며, 이후 Windows Git diff와 별도 link 검증으로 결과를 확인했다.

**적대적 리뷰**: 서로 다른 전문 영역의 reviewer subagent 2명이 immutable commit `b6f523f`를 독립 검토해 6개 P1, 3개 P2, 2개 P3와 추가 관찰 1개를 보고했다. 수정 commit `ab613c8`에서 두 reviewer가 모든 항목의 해소와 신규 P0/P1 회귀 없음에 동의했다. 두 `CONDITIONAL` verdict의 유일한 조건인 post-fix 결과·disposition 기록은 [통합 report](reviews/adversarial/2026-09-05-document-information-architecture.md)와 별도 evidence로 종결했다.

**검증**: 1차에는 Markdown local link 459개와 fragment 8개를 확인했다. closure 포함 Markdown 89개, local link 476개, fragment 11개에서 오류 0개, 상세 task 파일·요약 link 34/34, 이동 전 경로 잔존 0개, Windows Node `prototype.js --check`, Windows Git `diff --check`를 통과했다. staged 보안 감사 결과는 PR에 남긴다.

## 2026-09-05 (codex)

**작업**: Windows 개발환경과 일회성 worktree 정책 반영 및 embedded-skills 설치

**변경 파일**:

- AGENTS.md, SKILL.md
- docs/development/windows.md
- docs/runbooks/agent-workflow.md, docs/runbooks/agent-failure-patterns.md
- docs/adr/003-windows-development-and-ephemeral-worktrees.md
- docs/adr/README.md, docs/decisions.md, CHANGELOG.md, docs/resume.md

**외부 설치**: `rovinax/embedded-skills` `master` (`022ce31b469b1a1d0c1261c2c8d0f3e07b2c0bbc`)에서 `embedded-architecture`, `embedded-cstyle`, `embedded-documentation`, `embedded-driver-design`, `embedded-isr-design`, `embedded-rtos-design`을 Codex skills 디렉터리에 설치했다.

**결정**: Windows PowerShell과 Windows native 도구를 정본 개발환경으로 삼고, worktree는 병렬·격리·독립 리뷰가 필요할 때만 생성하며 merge 또는 abandon 후 제거한다. WSL/Linux는 보조 환경으로만 취급한다.

**검증**: GitHub 설치 스크립트가 6개 스킬 설치를 완료했다. 저장소 문서의 경로·worktree 표현과 ADR 색인을 갱신했다. Markdown local link 78개, task 상세/요약 34개, host C automation, UI JavaScript syntax 검증을 통과했다. 현재 셸에는 CMake/Ninja가 없어 해당 build gate는 미실행이다.

## 2026-09-05 (codex)

**작업**: kor-travel-geo 문서 운영 구조를 canview에 적용 (문서 구조 task)

**변경 파일**:

- AGENTS.md, SKILL.md, CHANGELOG.md와 .gitignore
- README.md
- docs/architecture/README.md
- docs/architecture/system.md, docs/architecture/implementation-readiness.md, docs/reviews/adversarial/2026-09-04-baseline-design.md
- docs/development/windows.md, docs/runbooks/documentation-maintenance.md, docs/resume.md, docs/journal.md
- docs/adr/, docs/runbooks/
- docs/tasks.md, docs/tasks-rule.md, docs/tasks-done.md, docs/tasks/README.md

**결정**: 열린 task 요약은 docs/tasks.md에 두고, 상세 task는 docs/tasks/ 아래에 하나씩 유지한다. 완료 task는 docs/tasks-done.md로 이동한다.

**발견**: 기존 canview에는 docs/tasks/README.md에만 task 요약이 있었고 AGENTS.md·SKILL.md·ADR·runbook·resume·journal 정본이 없었다.

**다음**: T-001 host toolchain/CI와 T-100 KiCad 회로도·BOM을 병렬 착수한다.
