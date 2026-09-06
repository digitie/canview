# 2026-09-06 펌웨어 기반 코드 적대적 리뷰

- Review ID: firmware-foundation-20260906
- 종류: 전문 리뷰어 서브에이전트 2인 독립 적대적 리뷰
- Review candidate: `59ac4046a02c3bb4d2612f4143d650ff4095f5b2`
- Base/parent: `b529a722fb813d5b60ab667675d73996895ea3fc`
- Post-fix: `b084720ff73da2ad95460a1c947478e62c63d35d`
- 범위: MCU 독립 C99 codec/app, 네 MCU startup/BSP/platform, board/schema 생성기, host 시험·CI·API 문서
- 범위 밖: 전체 ABI/인증/ACK/QoS/filter, 실제 driver·OTA·차량 기능, target/전기/HIL/차량 승인
- 요청: 기반 구조 코드만 구현, 상세 구현은 후속 agent; 넓은 unit test·warning0·상세 생성 문서
- 관련 task: [T-001](../../tasks/T-001-host-toolchain-ci.md), [기반 아키텍처](../../architecture/firmware-foundation.md)
- Coordinator: Codex
- 시작: 2026-09-06T03:32:34Z / coordinator closure: 2026-09-06T04:11:00Z
- 상태: `PASS` — foundation merge-code 범위 한정

## 1. 동일 manifest와 격리 증거

공통 manifest와 reviewer별 전달 요청 원문은 아래 evidence에 보존했다. 두 reviewer 모두 같은 93파일 candidate를 각각 detached worktree에서 검토했다.

| 항목 | Reviewer A | Reviewer B |
|---|---|---|
| 전문 영역 | embedded runtime·board·reset safety | protocol·security·build quality |
| subagent ID | 01a074c6-29ad-7be3-876e-4d30954faaca | 01a074c6-2a91-7c51-843a-9075f678f9bc |
| 원본 execution ID | foundation-a-59ac4046-20260906T033235Z | review-foundation-b-20260906T033234Z |
| 최초 UTC 시작 | 2026-09-06T03:32:35.683Z | 2026-09-06T03:32:34.6902310Z |
| 최초 UTC 종료 | 2026-09-06T03:43:23.992Z | 2026-09-06T03:46:52.1398909Z |
| 격리 | review-foundation-a detached | review-foundation-b detached |
| 시작·종료 HEAD | 59ac4046, 전체 SHA 위와 동일 | 59ac4046, 전체 SHA 위와 동일 |
| 시작·종료 clean | 빈 porcelain, tracked 변경 없음 | 빈 porcelain, tracked 변경 없음 |
| 원본 | [A 원문](evidence/2026-09-06-firmware-foundation-reviewer-a.md) | [B 원문](evidence/2026-09-06-firmware-foundation-reviewer-b.md) |
| 최초 verdict | CONDITIONAL, P2 세 건 | CONDITIONAL, P2 두 건 |

A는 OTA 정본 검색 중 같은 파일에 포함된 과거 리뷰 일부가 우발 노출됐다고 원본에 명시했다. 따라서 모든 과거 리뷰 미노출 조건이 완벽했다고 주장하지 않는다. 이번 candidate의 다른 reviewer 결과는 두 사람 모두 열지 않았고, 두 원문을 각각 저장한 후에만 coordinator가 비교했다. finding 삭제·등급 하향은 없다. A 원문 재전달은 후속 상태 메시지가 상세 결과를 가렸기 때문에 요청했으며 원문을 그대로 보존했다.

## 2. Reviewer A findings

| ID | 등급 | 위치·근거 | 재현·영향 | 권고 |
|---|---|---|---|---|
| A-01 | P2 | waveshare35-pins.json:15, Controller header:21, hardware/controller.md:108 | PLAY14/REC16은 공식 실제 GPIO 설정과 반대. 후속 I²S 무음/수음 실패·방향 충돌 가능 | MCU DOUT16/ DIN14로 교정, 생성물·정본·독립 fixture |
| A-02 | P2 | generate_boards.py:112 | LCD_BL을22..34로 바꿔도 생성. 없는 GPIO와 메모리 점유 GPIO를 사전 검출하지 못함 | SoC/module allowlist 및 JSON/CSV negative fixture |
| A-03 | P2 | ESP 세 main/CMakeLists.txt:3, ADR008 C99 계약 | startup/BSP와 SDK adapter가 한 GNU23 component이며 host startup 지속 compile gate 없음 | SDK adapter 별도 component, main C99, startup 네 역할 compile |

각 상세 source URL·실제 명령·UTC·미실행은 A 원문에 있다. 현재 audio/무선/CAN을 시작하지 않는 기반이라는 범위를 유지한다.

## 3. Reviewer B findings

| ID | 등급 | 위치·근거 | 재현·영향 | 권고 |
|---|---|---|---|---|
| B-P2-01 | P2 | generate_boards.py:112 | GPIO22..34 반례 허용. A-02와 독립적으로 발견한 동일 누락 | SoC/module allowlist, CSV 포함 negative fixture |
| B-P2-02 | P2 | test_foundation.c:280, test_vectors.py:31 | CAN bus_id를 항상0으로 바꾼 mutant가 core9·golden2195를 통과. 현재 정상 codec의 오류가 아니라 회귀 검출 공백 | bus/flags/DLC를 포함한 전체 필드 비교·독립 CAN byte golden·변이 검증 |

B는 별도 참조로 원본 envelope4000·CRC 재계산 malformed20000·CAN4000·COBS 정상/손상 각10000·sequence100000을 확인했다. 이 수치는 reviewer의 별도 host 실험이며 상시 CTest·sanitizer·target 검증으로 집계하지 않는다.

## 4. 교차 확인과 최초 판정

A-02와 B-P2-01은 같은 결함이며 두 원 ID와 P2 등급을 모두 보존한다. 나머지 세 finding은 독립적이다. P0/P1 없음은 미검증 전기/차량 안전의 증명이 아니다. 최초 판정은 두 reviewer 모두 CONDITIONAL이었다.

## 5. 반영과 재현

| Finding | 작성자 disposition | 수정·시험 | 원 reviewer 재확인 |
|---|---|---|---|
| A-01 | FIXED | PLAY16/REC14, Controller 생성 header/digest, 하드웨어 문서, audio 방향 fixture | A FIXED/PASS |
| A-02 | FIXED | chip 유효 집합과 R8/N16R8/N8R2 외부 GPIO 집합, JSON/CSV 금지 범위·bool/float 거부 시험 | A FIXED/PASS |
| B-P2-01 | FIXED | A-02와 동일 수정. 두 원 finding 유지 | B FIXED/PASS |
| A-03 | FIXED | canview_esp32_platform GNU adapter 분리; main과 common core strict C99; startup4역할 object build | A FIXED/PASS |
| B-P2-02 | FIXED | roundtrip 전체 field, Python struct 기반 CAN208개 byte golden과 전체 필드 decode 대조 | B FIXED/PASS |

모든 수정은 위 post-fix commit에 있으며 연기한 finding은 없다. 두 원 reviewer가 자신의 finding 및 전체 delta 회귀를 재확인했다. 원 등급은 변경하지 않았고 열린 finding은 없다.

### 작성자 post-fix 검증

고정 Windows 도구 활성화 뒤 다음을 실제 실행했다. 명령별 exit code를 확인했고 공용 runtime codec은 candidate와 동일하다.

- host-debug, host-release configure/build/CTest 각각31/31 PASS, strict C99 경고0. 네 startup object도 실제 compile.
- generator7그룹 PASS, generated-transport/boards drift PASS.
- 독립 envelope/COBS2195 + CAN208 =2403개 golden PASS.
- 새 coverage profile: 두 공용 source 실행line546/546, function20/20, branch295/296. BSP/SDK/Python/legacy 제외.
- Doxygen XML14함수 brief/param/return 및 Sphinx -n -W PASS.
- 문서·plan validator PASS. 최초 원문 archive의 임시 worktree 링크를 탐색 링크로 검사한 실패는 literal 원문 보존 방식으로 해결했다. 원문의 링크 문자열은 바꾸지 않았다.
- 깨끗한 API venv의 hash-lock 설치 및 pip check PASS. 기존 plan validator 부정 fixture35개 별도 PASS.

### CAN 변이 검출

원본을 바꾸지 않고 ignored build/foundation-mutation에 각각 복제·변경한 시험 source를 compile했다.

| 변이 | 변경 | 새 gate 결과 |
|---|---|---|
| bus | encode의 destination[2] = record->bus_id를0으로 상수화 | C 전체 필드 비교 실패, Python subprocess nonzero |
| offset | bus 기록 위치를 destination[2]에서[1]로 이동 | C 전체 필드 비교 실패, Python subprocess nonzero |
| endian | delta encode를 high-byte-first로 변경하고 decode도 source[0]*256+source[1]로 동시에 반전 | C 왕복은 성립하지만 Python 독립 byte golden mismatch |

```powershell
clang -std=c99 -Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef -Ishared/protocol/include -Ishared/interface build/foundation-mutation/bus.c tests/foundation/wire_vectors.c -o build/foundation-mutation/bus.exe
python -X utf8 -B tests/foundation/test_vectors.py build/foundation-mutation/bus.exe
```

offset/endian도 같은 명령에 해당 파일명을 사용했다. 세 compile은 성공했고 각 시험의 nonzero가 기대 결과다. 실제 codec이나 정상 시험을 실패 상태로 남긴 것이 아니다. 이는 세 가지 구체적 변이 검출이지 전체 mutation score 또는 MC/DC 주장이 아니다.

### CI 이력

- [34009244387](https://github.com/digitie/canview/actions/runs/34009244387): 최초 candidate, Windows cp1252 출력 오류와 GCC C99 시험 정수 변환 경고로 실패.
- [34009476690](https://github.com/digitie/canview/actions/runs/34009476690): Windows 전체 PASS, Linux의 기존 C11 brightness 시험 signedness 경고로 실패.
- [34009610099](https://github.com/digitie/canview/actions/runs/34009610099): beae8a9, Windows 전체 및 Linux GCC Release/Clang ASan+UBSan PASS.
- [34010254548](https://github.com/digitie/canview/actions/runs/34010254548): b084720 post-fix, Windows 전체 및 Linux GCC Release/Clang ASan+UBSan PASS. 새 source/설정·시험 변경을 재실행한 결과다.

## 6. Post-fix 재검토

두 reviewer에게 같은 b084720 전체 SHA와 최초 candidate 대비 전체 delta를 제공했다. 자신의 finding과 전체 delta 회귀를 새 detached recheck-foundation-a/b에서 각각 검토했다. 전달 요청·실제 재검토 원문은 reviewer evidence에 보존했다.

| Reviewer | 확인 commit | 범위 | verdict | 남은 위험 |
|---|---|---|---|---|
| A | b084720 | A-01..03 + 전체 delta | PASS, 새 finding 없음 | target/HIL 미실행 |
| B | b084720 | B-P2-01..02 + 전체 delta | PASS, 새 finding 없음 | target/HIL 미실행 |

A 재검토는 foundation-a-postfix-b084720-20260906T035744Z, UTC 03:57:44.008–04:04:57.527이다. B는 recheck-foundation-b-20260906T035746Z, UTC 03:57:46.3200725–04:07:12.7483595다. 양쪽 모두 시작·종료 전체 HEAD 일치·detached·clean을 기록했다.
두 사람 모두 Debug/Release31/31, generator7그룹, golden2403, coverage546/546·295/296, strictAPI14를 재실행했다. 독립 GPIO 공격도 A67건, B부정64/정상13건을 검증했고, 세 CAN mutant를 각각 재컴파일해 검출했다. B는 PYTHONUTF8=0에서도 CTest 6개 Python 진입점의 UTF-8 override를 확인했다. 원문에는 실패한 reviewer fixture와 미실행도 보존했다.

## 7. 최종 판정

- 최종: PASS. A/B 전 finding FIXED 재확인, 열린 finding0, 새 P0–P3 없음. 최신 post-fix Windows 및 Linux CI PASS.
- 미실행: 정식 ESP-IDF/Arm GCC target build·link, GPIO/rail/reset 파형, PSRAM/clock/DMA/UART4Mbps, OTA 단전/HIL/차량 시험.
- Target VerifyOnly: Arm GCC 부재로 실패. CMake/host 도구 활성화와 target SDK 준비 완료는 다르다.
- 제품/차량 CAN TX gate는 닫지 않는다. T-001은 부분 IN_PROGRESS이고 후속 상세 task를 유지한다.
- PR 반영 위치: 본문의 검증·독립 리뷰·미실행 gate 절에서 이 report와 원문 evidence를 연결한다. 이번 요청에서는 PR 생성까지만 수행하고 merge는 별도 사용자 지시를 따른다.
