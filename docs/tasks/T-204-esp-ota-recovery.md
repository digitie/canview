# T-204 OTA-02 ESP 파티션과 세 역할 복구 앱

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G1/G2 / OTA-02`
- 선행: `T-007`, `T-200`, `T-300`, `T-400`
- 외부 선행: 역할별 실물 보드와 유선 최초 설치 환경

## 목표

Controller·Communicator·Bridge의 단일 factory scaffold를 OTA 정본 §4 주소표와 독립 recovery 앱으로 전환한다. 기존 설치본의 partition table을 OTA로 바꾸지 않는다.

## 고정 결정

[OTA §4·7·11](../architecture/ota.md)의 내부 Flash A/B·불변 app/test recovery·비권위 NVS·별도 config/provisioning A/B를 그대로 구현한다. Communicator N16R8이며 Bridge는 N8R2다. recovery는 LCD·SD·PSRAM·정상 NVS에 의존하지 않는다.

## 구현 범위

- 세 ESP normal/recovery 별도 project·서명 artifact와 최초 유선 flash manifest
- 주소표 생성/중복·끝·alignment·image+padding+128 KiB 여유 검사 및 normal writer allowlist
- config A/B bounded storage I/O와 write/readback/commit primitive 제공. 역할별 config serializer는 T-203/T-301/T-304, OTA trial/migration policy는 T-205가 소비
- Communicator 4.5 MiB staging 검증, old app/config 보존, source별 resource budget
- recovery AP/상태·서명 검증 기본 경로와 물리 버튼 진입; 공통 ota_manager→flash/crypto adapter 경계

## 범위 밖

제조 security lock(T-507), ESP↔STM recovery UART(T-108), 완성 웹UX(T-306), cut-point 전수 HIL(T-508).

## 예상 변경 파일

아래는 이 task가 생성·확정할 미래 산출물이다. 경로가 아직 없다는 사실을 검증 통과로 해석하지 않는다.

```text
firmware/*/partitions.csv
firmware/communicator/esp32/partitions.csv
firmware/*/recovery/
firmware/communicator/esp32/recovery/
config/ota/
tools/ota/validate_layout.py
tests/ota/test_layout.py
```

## 수용 기준

- [ ] 현재 각 실제 project 경로를 조사해 manifest에 고정하고 factory CSV 변경을 최초 유선 migration으로 분류한다.
- [ ] 세 역할 signed image·recovery·partition/bootloader 크기를 실제 map으로 검사하며 SDK 없는 host 검사를 target 통과로 표시하지 않는다.
- [ ] 버튼 Controller41/Comm8/Bridge4, active-low reset hold5초로 복구한다. GPIO0 ROM strap과 정상 Bridge PAIR3초를 혼동하지 않는다.
- [ ] PSRAM/SD/LCD/normal NVS를 고장 처리한 상태에도 role별 recovery AP가 뜨고 서명 bundle을 검증한다. 자격증명 두 사본 손상 시 무암호 fallback은 금지한다.
- [ ] bootloader/table/recovery/provisioning write가 enum map과 writer 양쪽에서 거절된다. config A/B와 policy journal은 capture/cache cleanup 대상이 아니다.
- [ ] local health 없는 후보는 rollback하고 PHY 무전원·다른 장치 부재만으로 자기 OTA 확정을 막지 않는다. system trial/config policy 최종 통합은 T-205다.
- [ ] API/Flash buffer lifetime·single writer·erase/readback 실패·고갈·cancel의 release 경계를 host fixture와 target에서 확인한다.

## 검증 계획

`tools/ota/validate_layout.py`와 부정 fixture를 이 task에서 만든 뒤 세 normal/recovery project의 IDF build/size/map 검사를 실행한다. 초기 유선 설치와 버튼·고장 boot 시험은 실제 보드별로 수행한다.

## evidence와 rollback

역할별 sdkconfig/CSV/image digest·실제 슬롯 여유·boot trace를 남긴다. layout mismatch는 OTA를 거절하고 승인된 유선 서비스만 안내한다. 이전 정상본을 지워 문제를 감추지 않는다.
