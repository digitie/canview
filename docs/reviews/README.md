# CANView 리뷰 아카이브

이 디렉터리는 특정 commit 또는 diff를 대상으로 수행한 리뷰를 누적 보존한다. review report는 역사적 evidence이며 현재 설계의 정본은 아니다. finding을 반영한 결과는 architecture, ADR, task와 코드에 남긴다.

## 기록 규칙

1. 적대적 리뷰 요청마다 `adversarial/YYYY-MM-DD-<scope>.md` 파일을 새로 만든다. 같은 날 같은 범위가 반복되면 `-02`, `-03` suffix를 붙인다.
2. 과거 report에 새 review 결과를 덧붙이지 않는다. 오탈자나 깨진 링크를 고친 경우에만 correction note를 남긴다.
3. report는 review ID, 종류, 기준 commit과 base, 범위 밖, reviewer 전문 영역, 검증 방법, 시작·종료 시각을 기록한다.
4. 전문 리뷰어 서브에이전트 2명은 같은 manifest와 immutable 기준선을 서로 독립적으로 검토한다. 두 원본 결과가 확정되기 전에는 어느 reviewer에게도 상대 결과를 보여 주지 않는다.
5. reviewer별 실행 ID, 전달 입력, 실제 관찰 hash, 격리·clean 검증, 원본 결과는 `adversarial/evidence/YYYY-MM-DD-<scope>-reviewer-{a,b}.md`에 따로 보존하고 통합 report에서 연결한다.
6. finding ID는 report 안에서 고유하게 유지하고 심각도 `P0`~`P3`, 근거·위치, 재현 또는 실패 형태, 영향, 권고와 disposition을 포함한다.
7. 심각도와 disposition 상태의 정본은 [agent workflow §5](../runbooks/agent-workflow.md#5-전문-리뷰어-서브에이전트-2인-적대적-리뷰)다. `P0`/`P1`은 단순 risk acceptance나 release 차단으로 닫지 않는다.
8. review 반영 뒤 관련 검증을 다시 실행하고 post-fix commit을 두 reviewer가 재검토한다. report와 PR에 두 verdict를 기록한다.
9. review 원본·통합 disposition·재검증 결과와 archive index만 기록하는 closure commit은 같은 review를 재귀적으로 시작하지 않는다. 규범 문구를 함께 바꾸면 새 기준선 리뷰가 필요하다.

## 리뷰 목록

최신 항목을 위에 추가한다.

| 날짜 | 종류 | 기준선·범위 | 리뷰어 | 결과 |
|---|---|---|---|---|
| 2026-09-07 | T-003 ESP-NOW codec/session/QoS 최종 post-fix | `6a076a3` → `67ccee9`, session lifecycle/security binding·generated TLV·anti-replay·QoS/resource·STM32/ESP32 target evidence | embedded safety·target runtime / protocol·security·build/integration | [report·Reviewer A/B 원문·target evidence](adversarial/2026-09-07-T-003.md), 최초 B-P1-01 evidence freshness FIXED·양 reviewer post-fix PASS |
| 2026-09-06 | T-002 ESP-NOW v1.3 schema·generated header 최종 post-fix | `fe564fe` → `fb30b29`, schema ABI·generated namespace·clear decode context·bulk bounds·golden/negative·host/target evidence | embedded safety·protocol·security / protocol·ABI·generator·reproducibility | [report·reviewer A/B evidence·target/coverage evidence](adversarial/2026-09-06-T-002.md), 최종 finding 0건·양 reviewer PASS·F-02는 T-003 이관 |
| 2026-09-06 | T-001 host·target toolchain·CI 최종 post-fix | `68cac2b` → `9cb76e2`, Arm archive provenance·Windows SDK checkout·STM32/ESP32 target gate | embedded safety·target runtime / CI·재현성·evidence integrity | [report·최종 원문·target evidence](adversarial/2026-09-06-T-001.md), 최종 finding 0건·양 reviewer PASS·Draft PR/실차 gate 별도 |
| 2026-09-06 | C99 펌웨어 기반·protocol·시험/API | `59ac404` → `b084720`, 네 MCU 구조·MCU 독립 codec·pin 생성기·Windows/Linux CI | embedded runtime/board/reset / protocol/security/build quality | [report·원문·변이 재현](adversarial/2026-09-06-firmware-foundation.md), 원 P2 5건 전부 FIXED·양 reviewer PASS·target/HIL/차량 승인 아님 |
| 2026-09-06 | 전체 계획 두 차례 감사·UI | `d078437` → `1f93b8a`, 42요구/46task·웹5+5뷰·LVGL·자동화 | 임베디드/운전자 안전 / 프로토콜/계획/웹 통합 | [report·원문·재현](adversarial/2026-09-06-plan-ui.md), 원 finding 전부 FIXED·양 reviewer PASS·제품/차량 승인 아님 |
| 2026-09-06 | 독립 OTA·N16R8 회로 | `5abeae4` 및 post-fix, reset/CAN 인터록·Flash·OTA 계약 | 하드웨어 복구 / OTA 보안·상태기계 | 사용자 단일 MD 요청에 따른 예외: [설계 내 원문·수정·재검토](../architecture/ota.md#14-독립-적대적-리뷰-기록) |
| 2026-09-05 | R1 상세 회로·센서 protocol | `c1c15b5`, 네 보드 KiCad·전원/CAN gate·GNSS/INS·mic·host codec | 전원·CAN·reset / 센서·pin/land·protocol·생성물 | [report](adversarial/2026-09-05-r1-hardware-navigation.md), 원 finding 전부 FIXED·검토본 통합 PASS·제작 승인 아님 |
| 2026-09-05 | 최신 toolchain bootstrap post-fix | `cb7aae9`, recursive SDK submodule 검증과 최종 bootstrap | embedded safety·target build / reproducibility·integration | [report](adversarial/2026-09-05-latest-toolchain-bootstrap-post-fix.md) |
| 2026-09-05 | 최신 toolchain bootstrap | `bfdd2c2`, target 환경·SDK pin·ESP-IDF/STM32 CMake | embedded safety·target build / protocol·재현성·문서 | [report](adversarial/2026-09-05-latest-toolchain-bootstrap.md) |
| 2026-09-05 | 문서 정보구조·review gate | `b6f523f`, 문서 구조·운영 정책 | 문서 IA·agent 실행성 / 품질 gate·감사성 | [report](adversarial/2026-09-05-document-information-architecture.md) |
| 2026-09-04 | 독립 적대적 설계 리뷰 | `50b5e733`, 전체 구현 준비성 | 전원·안전·protocol / build·통합·시험 | [report](adversarial/2026-09-04-baseline-design.md) |

## 새 리뷰 시작

[적대적 리뷰 템플릿](adversarial/TEMPLATE.md)을 복사해 새 파일을 만들고, [agent workflow](../runbooks/agent-workflow.md)의 2인 리뷰 gate를 따른다. 일반 작업 시작 시 이 아카이브 전체를 읽지 않는다.
