# Reviewer B 독립 원본

- 서브에이전트: `01a06f2e-735a-7221-9541-de848f1984b2`
- 전달 요청 원문·동일 기준선: [manifest](2026-09-05-r1-hardware-navigation-manifest.md)
- 아래는 reviewer final 원문이며, 최초 상호 공개 전에 보존했다.
- 2026-09-05 보존 형식 교정: 저장 과정에서 추가된 EOF 빈 줄 한 개만 제거했다. 아래 reviewer 원문 내용·판정은 바꾸지 않았다.

## 독립 판정: CONDITIONAL

Reviewer B 범위에서 **P0/P1은 확인하지 못했습니다. P2 3건, P3 1건**입니다. 설계 검토본으로만 조건부 수용하며, 제작·G1/차량 TX·실제 센서 firmware 승인은 포함하지 않습니다.

### 실행·격리 기록

- 실행 ID: `CANVIEW-R1-B-20260905T034629Z`
- 시작/종료 UTC: `2026-09-05 03:46:29` → `04:00:51`
- 실제 관찰 commit: `06bb51c72180f9c040db3ccf0b223a823c570409`
- 실제 관찰 parent: `1ed2576841075a1f98fb10e979736941f3deed72`
- Windows native `git.exe`로 commit type/hash/parent 확인.
- 소스는 commit 객체에서만 취득. **clean: N/A — working tree 미사용.**
- 파일 수정·Git mutation 없음. 다른 reviewer 결과를 열람하지 않았으며, 이 응답을 독립 원문으로 확정합니다.

아래 위치는 모두 위 commit 기준입니다.

## Findings

### B-R1-001 · P2 — 모순된 센서 capability를 정상 payload로 수용

위치: [schema:13](/mnt/f/dev/canview/protocol/schema/navigation-v1.json:13), [codec:25](/mnt/f/dev/canview/tools/protocol/navigation_codec.py:25), [계약:32](/mnt/f/dev/canview/docs/architecture/protocols/navigation.md:32).

재현: 다른 필드는 정상 기본값으로 두고 다음 두 입력을 각각 encode→decode하면 모두 성공합니다.

- `imu_rate_mask=64`: IMU rate 배열은 6개뿐이므로 bit6은 정의되지 않았습니다. 현재 상한 `127`이 이를 허용합니다.
- `hardware_profile=2, feature_mask=17, max_dr_age_ms=45000`: ESP-direct raw GNSS가 NAV+DR을 광고합니다. 계약의 “profile2는 DR bit를 올리지 않는다”와 충돌합니다.

영향: host 검증기를 통과했다는 이유로 지원하지 않는 rate 또는 raw GNSS의 DR 능력을 신뢰할 수 있습니다.

권고: rate mask를 배열 길이에서 계산하고, profile/DR bit/max DR age의 교차 제약을 추가하십시오. 두 입력을 회귀시험에 넣어야 합니다.

### B-R1-002 · P2 — UNAUTHORIZED 결과의 snapshot 비공개 계약 미강제

위치: [codec:57](/mnt/f/dev/canview/tools/protocol/navigation_codec.py:57), [계약:111](/mnt/f/dev/canview/docs/architecture/protocols/navigation.md:111).

재현: `SENSOR_RESULT`에 다음 값을 넣고 나머지는 0으로 설정하면 encode/decode 모두 성공합니다.

```text
request_id=1, source_boot_id=1, status=3
current_revision=7
nav_rate_hz=10, nav_count_limit=42
snapshot_sha256_128=b'XXXXXXXXXXXXXXXX'
```

영향: 권한 거부 응답인데도 구독 상태·revision·digest가 노출되는 payload를 정상으로 인정합니다. 실제 인증 우회가 입증된 것은 아니지만, 현재 host 계약 검증의 명백한 누락입니다.

권고: status3에서 ID/boot/status 외 필드와 digest가 모두 0인지 강제하고, 비영 값별 거부시험을 추가하십시오.

### B-R1-003 · P2 — 저장된 artifact hash가 commit 객체 바이트에 재현되지 않음

위치: [validator:82](/mnt/f/dev/canview/tools/hardware/validate_exports.py:82), [validation:14](/mnt/f/dev/canview/hardware/validation.json:14).

재현: 네 보드 각각의 `*.net`, `connectivity.json`, 총 **8개**에서 `SHA256(git show bytes)`가 기록값과 다릅니다. 반면 객체의 LF를 CRLF로 바꾸면 **8개 모두 기록값과 정확히 일치**합니다.

예: `communicator.net`

```text
Git 객체: 6040868dc3cd57be1439fef89a83f76d17ef6ea71af5791c3c8206542289aaf0
기록값:   b9a19817bcabfc7e38e7b12e97090750b1e5dc8c0bcda72e455a558460479837
```

영향: immutable commit을 대상으로 하는 바이트 무결성 검증은 현재 기록 그대로 재현되지 않습니다. **의미상 회로 drift나 변조 증거는 아닙니다.**

권고: LF canonical hash와 그 정책을 명시하거나, 실제 Git blob의 hash를 별도로 기록하십시오. Windows export 바이트 checksum과 commit checksum을 구분해야 합니다.

### B-R1-004 · P3 — 사용하지 않는 Waveshare UART 접점 이름 반전

위치: [sensor_circuits.py:76](/mnt/f/dev/canview/tools/hardware/sensor_circuits.py:76), [공식 회로 사본](/mnt/f/dev/canview/hardware/references/pdf/waveshare-controller.pdf) p.1 J8.

공식 회로는 **25=ESP_TXD, 27=ESP_RXD**인데, 생성 입력은 25=`RXD44_NC`, 27=`TXD43_NC`입니다.

현재 둘 다 NC이므로 마이크 동작에는 영향이 없습니다. 추후 헤더 활용 시 잘못된 배선 근거가 됩니다. 이름을 바로잡고 NC는 유지하십시오.

## 독립 재현 결과

| 검증 | 결과 |
|---|---|
| Windows Python에서 객체를 메모리로 로드한 기존 host 시험 | 12/12 PASS |
| 생성 소스 실행 → committed 연결 모델 비교 | 4보드, flag 포함 344개 일치 |
| BOM·XML/sexpr netlist·연결 모델·named pad 비교 | 333개 / 1,209개 일치 |
| custom footprint 메모리 재생성 | 6개 일치 |
| reference PDF hash·바이트 수·전체 페이지 파싱 | 54개 / 1,709쪽 PASS |
| margin 계산 재실행 | committed JSON과 일치 |
| 저장된 ERC와 waiver 기록 | 네 보드 0 / 0 확인 |

출력을 쓰는 native KiCad 전체 export/ERC 명령은 재실행하지 않았습니다. **저장된 ERC0을 전기적 안전 증거로 사용하지 않았습니다.**

## 센서·마이크 확인 및 잔여 조건

- MTi host SPI와 GNSS AUX UART는 분리되어 있습니다. MTi→GNSS TX 링크는 실장, ESP TX 링크는 DNP이며 RX/PPS tap 배선이 생성 netlist와 일치합니다.
- MTi SPI strap/mode3/2MHz 제한, open-drain reset, VDDA filter 및 BMP384 AUX 연결은 저장된 제조사 문서와 대조했습니다. BMP384는 MTi firmware **1.18.0 이상** 조건이 유지되어야 합니다.
- T5848 핀, 1.8V 전원, AXC 방향, LVDS 극성·receiver termination, 32kHz/64fs=2.048MHz가 일치합니다. BMP384/T5848의 top/bottom 방향과 주요 land 치수도 도면으로 확인했습니다.
- Waveshare **2×16, 2.54mm**, GPIO38/39/40의 접점 **7/9/11**은 공식 schematic과 일치합니다. 결합 높이·기계 간섭은 미검증입니다.
- Holybro 저장 PDF는 구판으로 UART1 기본 **115200/5Hz**를 뒷받침하지만, **GH6 PPS는 그 PDF에 나타나지 않습니다.** committed 문서의 공식 웹 pinout 링크가 별도 근거입니다. 이번 immutable 검토에서 해당 웹 자료의 현재 상태나 구매품 도통은 재검증하지 않았습니다.
- MTi ZED-F9 profile 지원과 실제 Holybro firmware/config 조합의 qualification은 다릅니다. PPS 누락을 지원되는 GNSS/INS 운용으로 승인하지 않습니다.

검토한 공격 경로는 malformed 길이·범위·reserved/validity, DR 45초 경계, UTC leap 상태, 버전 왜곡, 과도한 rate, 오류 응답, reset/rail-domain 전환입니다. `[1,4,999]`, `[1,4.0]`도 버전 인자로 수용되므로 transport 경계의 형식 검증은 추가 방어가 필요합니다.

Replay/idempotency cache, 전체 peer allocator, 실제 brownout·PPS outage·3m 케이블 타이밍은 실행 가능한 target 구현/HIL이 없어 입증하지 못했습니다. 현재 T100b 차단은 이러한 미구현을 완료로 오인하지 않게 하는 적절한 경계입니다.

검토 자료는 AGENTS/runbook, ADR006, T100/T100b, navigation 계약·schema·codec·시험, R1 주요 문서, hardware 생성/검증 소스, 네 보드 연결/BOM/netlist/footprint와 선택 제조사 페이지입니다. 전체 문서 링크 109개/669 target 검사는 재현하지 않았으며, 모든 PDF 표·도면의 정독이나 모든 전력부품의 최악조건 qualification도 하지 않았습니다. `embedded-documentation` 스킬은 핀·소유권·계약 추적 기준으로만 적용했고 파일은 생성하지 않았습니다.

**조건:** P2 세 건을 수정하거나, 각각 담당자·작업 항목·마감·재검증 gate를 명시한 수용 기록이 필요합니다. 기존 제작 금지만으로 이 세 결함이 해결되지는 않습니다.
