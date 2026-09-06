# Coordinator 독립 추가 감사

전문 reviewer2인의 원문과 별개인 주 작업자의 추가 검증이다. reviewer에게 이 결과를 전달해 최초 판정을 유도하지 않았으며 두 원본 확정 뒤 post-fix delta로 제공한다.

- 최초 기준: `06bb51c72180f9c040db3ccf0b223a823c570409`
- 확인:2026-09-05 03:47 UTC 이후
- 명령: Windows Python에서 native Git `show <hash>:hardware/validation.json`을 읽고 각 artifact의 Git blob bytes SHA-256과 저장 digest 비교
- 결과: 네 보드 각각 `.net`과 `connectivity.json` **8개 mismatch**. Windows CRLF가 commit LF로 변환되었다. XML/PDF mismatch는 없었다.
- 영향: 회로 net/pin 자체는 같지만 committed evidence의 무결성 검증을 깨뜨린다.
- 수정: `normalize_exports.py`가 알려진 생성 text만 LF로 변환하고 `.gitattributes`로 Git checkout LF를 고정. hash는 정규화 뒤 계산한다. 제조사 PDF·이미지·history는 변환하지 않는다.
- 회귀: `validate_exports.py --git-revision <post-fix hash>`로 저장 commit의 실제 bytes를 다시 검증한다. 단순 working-copy PASS로 닫지 않는다.

이 finding은 통합 report의 `C-01`로 추적하며 reviewer2인의 전체 post-fix delta 재검토에 포함한다.

## C-02 — FT enable의 독립 PHY rail 소실

04:00 UTC 전후 main이 별도로 확인한 경로다. AUTO5V/MCU3V3 정상·PHY3V3=0이면 U28/U29의 PHY-rail `/OE` pull-up이 LOW가 되어 gate가 열린다. 최초 reviewer 결과 공개 전 식별했으며 reviewer A의 `R1-A-002`와 같은 원인이다. 수정은 active-high AHCT126 OE에 각각1k GND pull-down을 두는 방식이다. normal/rail-off Boolean 회귀와 실제 exported pad 검사를 추가했다. 이는 빠른 collapse analog 검증을 대신하지 않는다.

독립 원문2개를 모두 저장한 뒤 A-001의 VGS 정격, A-003의 USB CC VDD와 B-001/002/004의 capability·비공개 result·헤더 표기를 함께 수정했다. P1 세 건의 closure는 reviewer A의 post-fix 재확인이 있어야 확정한다.
