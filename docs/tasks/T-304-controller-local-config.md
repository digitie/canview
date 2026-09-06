# T-304 Controller local 설정, RTC, 밝기와 전조등 경고

- 상태: `BLOCKED`
- 우선순위: `P1`
- Gate: `G3`
- 선행: `T-300`, `T-302`
- 병렬 가능: `T-303`

## 목표

Controller가 소유하는 설정을 OTA 계약의 versioned config A/B와 PCF85063에 연결하고 CAN 기반 자동 밝기, 유휴 복귀, 일몰 후 전조등 미점등 경고를 완성한다. 현재 NVS scaffold를 권위 저장으로 유지하지 않는다.

## owner와 설정

- RTC date/time/timezone, RTC quality
- sunrise/sunset minutes 또는 검증된 외부 위치 결과
- headlamp warning enable
- manual/auto brightness, idle timeout 기본 30초
- FFT band/sensitivity/response/max offset
- local RX allow-list는 T-301 API를 사용

## 구현 범위

- schema-generated LVGL widget과 versioned config serializer, 비권위 NVS cache
- staging→validate→commit→readback transaction
- PCF85063 BCD/range/oscillator-stop handling
- CAN tail-lamp/dimmer freshness와 debounce
- idle dim/default-screen/touch restore
- speed-limit warning brightness priority와 종료 복원
- sunset grace/headlamp on/off hysteresis
- Bridge direct remote config의 D2/D3 confirmation hook (D1은 진단 stream)

## 고정 동작

- tail lamp valid on 500 ms에 night, off 1.5초에 day
- default idle timeout 30초; 선택지는 schema로 제한
- 과속이면 일시 밝기 상승·깜빡임, 종료 후 이전 idle/night 목표로 복귀
- 속도 경고가 headlamp 경고보다 overlay 우선
- RTC invalid 또는 headlamp signal stale이면 허위 경고를 내지 않고 diagnostic 상태만 남김

## 수용 기준

- [ ] config Flash와 RTC는 한 번에 원자 commit할 수 없으므로 pending intent→RTC write/readback→설정 commit의 각 power cut에서 복구/불명 상태를 판정하고 부분 적용을 성공으로 표시하지 않는다.
- [ ] RTC invalid/date rollover/DST policy가 test로 고정된다.
- [ ] tail-lamp chattering에서 night mode가 반복 전환하지 않는다.
- [ ] 30초 유휴→감광/기본화면, touch→정상 밝기가 된다.
- [ ] 과속 중 touch가 동작하고 종료 후 이전 밝기로 돌아간다.
- [ ] sunset 이후 headlamp off만 경고하고 정상일 때 아무 표시가 없다.
- [ ] Controller-local 설정이 vehicle `COMMAND_REQUEST`에 나타나지 않는다.

## 계획 보완 수용 기준

- [ ] solar 값에 날짜/시간대/source/revision/validity를 보존하고 날짜 변경·수동 시간 점프·GNSS 부재·잘못된 날짜·극야/백야·일몰 전후 경계에서 허위 경고가 없다. 미정 source 우선순위는 owner 계약을 확정한 뒤 구현한다.
- [ ] 저장은 [OTA §9](../architecture/ota.md#9-상태기계와-두-mcu의-호환성)의 config A/B 권위 snapshot과 정합하게 설계한다. NVS는 비권위 cache이며 실패 시 전체 erase하지 않는다.
- [ ] 설정 저장은 변경 종료/debounce 후 단일 writer로 수행하고, touch/FFT/20 Hz publish마다 Flash에 쓰지 않는다. schema mismatch/readback 실패/양 slot 손상에서 safe default와 이유를 표시한다.
- [ ] Bridge D3는 Controller 확인→owner 최종 APPLIED→matching revision 전에는 성공 표시·mirror 저장이 0건이다. 움직임·stale speed·승인 TTL 만료·동시 local 변경에서 무변경 거부한다.

## 검증

```bash
ctest --preset host-sanitize -R 'rtc|brightness|headlamp|config' --output-on-failure
python tests/hil/controller_powerloss_config.py --iterations 1000
```

## rollback

config readback 실패 시 마지막 valid generation 또는 safe defaults를 사용한다. 잘못된 RTC 때문에 차량 명령을 만들지 않는다.

## 산출물·범위 경계

- 예상 산출물은 Controller config owner/RTC/backlight adapter·schema serializer와 CTest/HIL powerloss fixture다. 차량 headlamp 송신·GPS 밝기 대체·OTA transaction 구현은 범위 밖이다.
- cut point별 config generation·RTC quality·미적용 reason을 evidence로 남긴다.
