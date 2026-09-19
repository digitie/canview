# T-007 SDK 합성 metadata checkpoint 독립 리뷰

- Review ID: `T007-SDK-METADATA-20260919`
- Base: `ec44647e4674cc382d655bb668e07254b2612fca`
- Candidate: `624848a784f255c6be770ce1ae37fa824e56acba`
- 범위: SDK fixture C descriptor·CMake·BIN 위치/값 검사·CI·관련 문서8파일
- 범위 밖: 실제 native signing/golden, production descriptor·정상 OTA/Flash owner·HIL
- Coordinator: Codex
- 결과: A/B object-only 정적 PASS, P0/P1/P2/P3 finding0

## 1. 같은 manifest와 독립 원문

위 base/candidate와 범위·실행 가능한 검증·미완료 gate를 같은 요청으로 전달했다.
두 원문 보존 전 finding을 공유하지 않았다. 실제 전달 요청과 읽은 파일·명령은 각
evidence에 원문 그대로 보존했다. reviewer들은 객체 hash와 diff를 확인했으며
checkout clean이나 직접 SDK 실행을 주장하지 않았다.

| Reviewer | 전문 범위 | Subagent ID | Execution ID | 시작·종료 UTC | 원본 |
|---|---|---|---|---|---|
| A | C layout·const 수명·linker·권한 분리 | `01a0b784-5d6a-7540-97e8-ac159eb3a828` | `d0f53615-1599-4611-8928-e1da298aee7f` | 02:50:56.596–02:52:54.315 | [A](evidence/2026-09-19-T-007-sdk-metadata-reviewer-a.md) |
| B | 고정 기대값·drift·CI 실패 전파·근거 범위 | `01a0b784-5e5f-7d42-8953-968159b85914` | `fff9cf8a-4e72-45ed-b78c-67d7f7d3a1e3` | 02:50:56.2304079–02:52:09.8609774 | [B](evidence/2026-09-19-T-007-sdk-metadata-reviewer-b.md) |

시각은 모두2026-09-19다. 두 reviewer 모두 지정delta 정적 PASS이며 확정 finding이
없으므로 FIXED/DEFERRED 등 disposition 대상도 없다.

## 2. 공격 범위와 교차 확인

C의 고정 폭·168B 크기·offset·endianness·u64 최대값·zero padding, const 객체 수명과
`used`/`-u` 보존, fixture 설정의 production 전파 여부를 검토했다. Python 기대값은
BIN에서 복사하지 않는 고정 literal이며 CI는 직전 SDK 생성물에 검사기를 실행하고
비정상 종료를 실패로 전파한다. 기존 production/portable/SDK adapter 코드는 불변이다.

양쪽 모두 검사기가 전체 ESP header/segment/checksum/native signature verifier가
아니라고 명확히 구분했다.168개 byte 각각의 최하위 bit 변이와4개 절단만 검사하며
전체 bit 공간 또는 SDK native verifier의 거절을 입증하지 않는다. SHA256 출력은
파일 식별값이지 아직 고정 signed golden digest 대조가 아니다.

## 3. Coordinator 검증과 미실행 gate

ESP-IDF6.0.3에서 실제 fixture ELF/MAP/BIN을 만들고 compiler/linker/CMake 경고0을
확인했다. 실제 BIN offset288의168B·u64최대값 대조와168개 변이/4개 절단을 실행했다.
BIN SHA256은 `e9dc9177c696a13bdba0631c1da2b3ec125b688862ab2c04921c05c0675d3735`다.
명령은 fixture README, 로그와 이전checkpoint CI/artifact 감사는 [journal](../../journal.md)에 있다.
이 실행은 reviewer들이 직접 재현한 검증이 아니다.

이전63c8727의 CI35416056122는6/6·artifact21개·source7개·target log26개 진단0을
확인했지만 metadata를 포함한 새21cb047의 CI35416950979는 진행 중이다. 이전 artifact를
새 source의 완료 증거로 대체하지 않는다. 실제 signing/golden·정상 OTA owner 연결이
남아 있으므로 전체 T-007 `IN_PROGRESS`, PR36 `DRAFT`, physical/HIL `NOT_RUN`,
차량 TX `NO-GO`를 유지한다. 이후 추가 코드는 별도 검토가 필요하다.
