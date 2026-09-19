# T-107 fail-stop post-fix 판정과 서비스 중단

- Review ID: `T107-FAILSTOP-POST-5a37907-20260919`
- candidate: `5a3790773645b2cc30747ccb44f99732978171bf`
- base: `6b394ac7effbdc14e901b363a1594d4bf89b34fa`
- 범위: 기존 wrapper 검사기·실제 Arm negative·CI 명시 trust 연결의 수정.
- 범위 밖: 최종 boot entry·영속 정책·실기·T107 전체 및 PR37 merge.
- 통합 상태: **BLOCK — 원 B post-fix report 미반환**. A의 source/local PASS와 구분한다.

## 실행 근거

| 항목 | A | B |
|---|---|---|
| execution ID | T107-FAILSTOP-A-POST-5a37907-20260919 | T107-FAILSTOP-B-POST-5a37907-20260919 |
| UTC 시작/종료 | 13:32:09.6904612 / 13:35:59.5151280 | 서비스 turn 13:31:57 / 13:35:08 |
| 격리 | review-t107-failstop-a | review-t107-failstop-b |
| 원본 | [A post-fix](evidence/2026-09-19-T-107-failstop-post-reviewer-a.md) | [B 중단·명령 metadata](evidence/2026-09-19-T-107-failstop-post-reviewer-b.md) |
| 결과 | PASS — source/local만 | 최종 report 없음, 서비스 cybersecurity flag |

동일 manifest/candidate를 원 reviewer에게 전달했다. A 원문을 그대로 보존한다.
B는 일부 파일/명령 실행 후 중단됐지만 최종 finding·판정이 없으므로 읽은 범위의
완전성이나 검증 성공을 추정하지 않는다. A 결과로 B 판정을 대체하지 않는다.
[최초 원문](2026-09-19-T-107-failstop.md)은 역사 상태 그대로 유지한다.

## Disposition

| finding | 상태 | 근거와 남은 조건 |
|---|---|---|
| A-FAILSTOP-01 P2 | FIXED | 원 A가 기존 전체 mutant ELF의 거절과 hash 불변을 직접 확인 |
| B-FAILSTOP-01 P2 | OPEN | 5a37907 수정 및 A의 동일 범주 확인 존재; 원 B 판정 미반환 |
| B-FAILSTOP-02 P2 | OPEN | 5a37907 CI 입력/강제 full 검사 수정 및 A 로컬 확인 존재; 원 B와 원격 CI 증거 수용 미완료 |

A 신규 P0/P1/P2/P3 각각0, B 신규 finding 수는 미확정이다. OPEN 두 건은
수정 구현이 없다는 뜻이 아니라 원 reviewer의 종결 조건이 충족되지 않았다는 뜻이다.
DEFERRED 또는 사용자 면제를 적용하지 않는다.

## 확인한 검증과 미실행

A는 실제 Arm Debug/Release build와 별도 full 검사 각5/5, assembler positive2/
negative8, mnemonic48 변이, missing-wrap 실제 link negative, no-trust ELF 거절을
확인했다. 독립 BIN37040/28976B, RAM12264/12040B, 직접 캡처 warning/error0이다.
이는 CPU 실행이나 final bootloader 수용이 아니다. Host161/161·sanitizer·coverage는
[최초 coordinator 기록](2026-09-19-T-107-failstop.md)과 구분한다.

기록 시점 CI35446012777(candidate)와 CI35446138640(e261d15 closure)는 진행 중이다.
최신 CI artifact 감사를 완료하지 않았다. 이전 SDK 다운로드 HTTP500 실패도
성공으로 변경하지 않는다.

## 차단과 다음 조치

서비스 flag를 요청 재표현·다른 reviewer·모델/계정 변경으로 우회하지 않는다.
사용자가 오류의 Trusted Access 안내 또는 오탐을 서비스 측에 확인한 뒤 승인된
방법으로 원 B 재검토를 마쳐야 한다. PR37은 Draft를 유지한다. PR33 한정 면제를
T107에 확대하지 않는다. 추가 구현도 완료로 표시하지 않는다.

실제 boot entry·T205 영속 정책·전체 call-chain stack과 최종 task 검증은 남아 있다.
Physical/HIL NOT_RUN, production·vehicle TX NO-GO, Bridge read-only다.
