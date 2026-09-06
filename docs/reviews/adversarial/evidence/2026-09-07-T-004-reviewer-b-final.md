# T-004 최종 evidence 재검토 B

- 리뷰어: Turing (`01a07797-94d3-7000-8f0e-f561638b5734`), protocol/security/build 전문
- 요청: immutable `d5d323f8e7251031bfa5a222e08796b2160226eb`의 완료된 C byte-accounted soak·최종 68개 host test·target binary evidence를 검토하고 기존 finding closure와 T-004 범위 최종 판정을 제공한다. remote CI와 physical/HIL은 구분한다. 다른 reviewer 원문은 읽지 않는다.
- 보존 메모: 아래 제공자 원문은 줄 끝 Markdown hard-break 공백만 정규화했다. 내용·심각도·판정은 변경하지 않았다.

## 제공자 원문

# T-004 Reviewer B 최종 RAW 보고서

- 실행 ID: `10f0676b-9969-42bc-866c-f5f1ab29e670`
- 대상: `d5d323f8e7251031bfa5a222e08796b2160226eb`
- 부모: `4a0b5fa0c40a3849e9e98e5aa4486aca28c5c144`
- 검토 시간: `2026-09-06T21:46:26.6708293Z` ~ `21:46:26.8105555Z`
- 검토 방식: immutable Git object 기준, 다른 리뷰어 원문 미열람
- `git diff --check`: PASS

확인 문서:

- `docs/reviews/adversarial/evidence/2026-09-07-T-004-soak-final.md`
- `docs/reviews/adversarial/evidence/2026-09-07-T-004-validation-final.md`
- `docs/reviews/adversarial/2026-09-07-T-004.md`

## 검증 결과

- 최신 커밋은 docs/evidence-only 변경.
- production C/schema/generated ABI는 `3c6967a`와 동일.
- C full-rate duplex soak: EXIT 0, `801.8717662 s`.
- Direction 0: `34,560,000,091` bytes, required `34,560,000,000`.
- Direction 1: `34,560,000,039` bytes, required `34,560,000,000`.
- frame guard/resync, malformed faults, counters, final buffer state assertions: PASS.
- Windows Debug/Release/ASan CTest: 각각 `68/68 PASS`.
- UART coverage: functions `100%`, lines `91.24%`, branches `79.12%`.
- STM32 Debug/Release: clean.
- ESP32 incremental builds: exit 0, warning/error gate 0.
- Core coverage/docs: PASS.
- Artifact metadata 차이는 문서 변경 provenance로 명시되었고, 단일 reproducible bundle hash를 과장하지 않음.
- 이전 soak 산식의 잘못된 추가 `×10 bits` 표현은 현재 문서에서 올바른 byte 산식으로 정정됨.

## Finding disposition

- 기존 B-P1-08 — Python-only/불충분한 soak: **CLOSED/FIXED**
  위치: `docs/reviews/adversarial/2026-09-07-T-004.md`
  근거: 양방향 각각 34.56 GB 이상 실제 byte-accounted C soak PASS.

- direction/auth/replay/TTL/session lifecycle: **PASS**, 미해결 P0/P1 없음.
- queue-full retry 및 payload lifetime: **PASS**, 회귀 테스트 포함.
- generated ABI/schema/generator consistency: **PASS**.
- malformed input/build reproducibility evidence: **PASS**.
- target physical UART/DMA/RTS/CTS, real auth/TX executor, HIL/vehicle: **NOT_RUN / T-004 범위 외**. 이를 source compile 통과로 주장하지 않음.

## 최종 판정

**T-004 범위: PASS**

**PR merge 상태: BLOCKED — remote CI가 아직 완료되지 않음.**
CI 성공 확인 후 merge 가능.
