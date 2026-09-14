# 2026-09-15 T-007 현재 구현분 post-fix 재검토

- 종류: 동일 전문 reviewer 2인 독립 object-only 재검토
- 원 task base: `d229772de77a48ae197e2ff1b4e55b6cef9a88ed`
- pre-fix: `21909e586d8fc9e7ebf8d0a4aacd7037e1d7f962`
- post-fix: `63197e3e08ca63be0579432c0a7ee01ac9bccfa6`
- 대상: [최초 리뷰](2026-09-15-T-007-current.md)의 finding과16파일 delta
- 범위 밖: 전체 T-007 완료·장치 설치·차량 TX 승인
- 종합: 소스 finding closure 완료, 현재 구현분 CI/target artifact 확인 대기

## 독립 원본과 결과

공통 요청 원문과 submission ID는 각 파일에 보존했다. 두 post 결과를 보존한 뒤
종합했고 원문을 변경하지 않았다. reviewer 실행은 최종 결과 보존 후 종료했다.

| Reviewer | execution ID | UTC 시작·종료(2026-09-14) | 원본 verdict |
|---|---|---|---|
| A, C runtime·native/SDK | `01a0a229-5363-7f61-a607-ecae1f0a4ff8` | 23:19:46.3073696–23:20:56.0315586 | [PASS](evidence/2026-09-15-T-007-current-post-reviewer-a.md), 정적 재검토 범위만 |
| B, schema·generator·build/evidence | `01a0a229-545f-7fc0-a90a-240470bb1206` | 23:19:44–23:21:26 | [CONDITIONAL](evidence/2026-09-15-T-007-current-post-reviewer-b.md), CI/artifact·종합 조건 |

## Finding closure

| Finding | 원 등급 | 원 reviewer 확인 | 최종 disposition |
|---|---|---|---|
| A-01, cleanup 최초 오류 유실 | P2 | A가 error/cleanup_error·자원 소유·재진입·최종 reset과 새 회귀를 정적 확인 | FIXED |
| B-01, staging64KiB generator 누락 | P2 | B가 음성15종 거절·양성2종 수용·현재 생성물13개 일치 직접 실행 | FIXED |
| A-02, 이전 README 구현 상태 | P3 | A가 구현된 검사기와 미완성 통합 구분 확인 | FIXED |
| B-02, 같은 README 문구 | P3 | B가 독립적으로 해당 문구 확인 | FIXED |

새 P0/P1/P2/P3 finding은 양 reviewer 모두 없다. Deferred finding도 없다.
B는222개 Git 객체로 source SHA256 `639bfe1c01453de483e2dd3dbc9b6540c9064e3433526a053a8360f2d2cec1e3`을
직접 계산했고 합성 fixture와 대조했다. 이것은222개 파일 전체 줄 단위 리뷰를 뜻하지 않는다.

## Coordinator 검증

수정 전 실패·수정 후 로컬 Host/ASan/coverage는 [최초 통합 기록](2026-09-15-T-007-current.md)과
[journal](../../journal.md)에 있다. 새 CI34908276012는 현재5/6 job 성공, target 실행 중이다.

- Windows artifact를 내려받아 Debug/Release 각각140 Passed/0 Failed, body 모형1702·CNG1708을 확인했다.
- Debug LastTest.log SHA256: `749308c541ffab6d6a2f0d647fd0c451d6ced007eb844cf977f60eb53a12e4f3`.
- Release LastTest.log SHA256: `f0aac491b0729a0730a2fd61666afd5543d66ab55d5d266fac27b6251134d186`.
- host-sim report의 commit/source가 post-fix와 일치하며 validator12 scenario PASS다. physical/HIL은 NOT_RUN이다.
- Windows job log에는 Node/Git 경고와 예상된 argparse 음성시험 출력이 있다. compiler/linker/CMake
  경고와 구분하며 전체 CI warning0을 주장하지 않는다. 로그는 build/ci-34908276012-windows*에 있다.

## 남은 조건

post-fix target ELF/MAP/BIN·source/hash·warning 감사와 review 기록 반영 뒤 CI 확인이 남았다.
사용자의 "지금 작업까지만 머지" 요청에 따라 현재 구현분만 검증 후 merge한다.
전체 signed packager/golden, prefix 조립,
정상 owner/writer/provider·영속 policy·정상 target 통합의 미완성 수용 기준은 그대로 유지한다.
이 결과로 T-007을 DONE으로 바꾸거나 PR35를 merge하지 않았다.

physical/HIL NOT_RUN, 차량 CAN TX NO-GO. 다음 task는 시작하지 않는다.
