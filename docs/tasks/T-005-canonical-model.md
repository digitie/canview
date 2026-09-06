# T-005 공통 quality, evidence, time과 owner model

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G0`
- 선행: `T-002`
- 병렬 가능: `T-003`, `T-004`

## 목표

protocol, Controller decoder, automation, LVGL과 Diagnostic Bridge가 서로 다른 enum과 시간 의미를 사용하지 않도록 공통 model을 고정한다.

## 고정 결정

- runtime `signal_quality`와 정적 `evidence_grade`를 분리한다.
- UI enum 숫자 cast를 금지하고 exhaustive adapter를 사용한다.
- 모든 차량 값은 sample time, age, source revision, evidence를 가진다.
- 자동화 입력도 같은 metadata를 가지며 raw 숫자만 받는 API를 두지 않는다.
- RTC wall clock을 CAN ordering이나 TTL에 사용하지 않는다.
- RTC·화면·FFT·idle은 Controller owner다.
- SPORT config/state와 차량 audio snapshot은 STM32 owner다.
- peer subscription은 Communicator ESP32 owner다.

## 구현 범위

- generated/common `canview_value_meta_t`, quality/evidence enum
- wrap-safe monotonic elapsed helper와 explicit time-domain ID
- owner enum 및 config routing table
- derived signal quality/evidence propagation helper
- protocol→domain→UI adapter; unknown enum fail-closed
- FFT signed dBFS model과 valid/clipped/calibrated 상태
- 현재 UI model의 speed/RPM/drive mode/speed limit/audio/FFT metadata 보완 계획
- wire width 계약: status/stage `u8`, reason `u16`, revision `u32`, boot 누적 counter saturating `u64`
- automation tick discontinuity, freshness ceiling과 pending command 상태 model

## 표시 규칙

운전자 화면은 `VALID + VERIFIED`만 숫자를 표시한다. `STALE/UNAVAILABLE`은 `—`, `OUT_OF_RANGE/FAULT`는 상태 경고로 바꾼다. `CANDIDATE/OBSERVED`는 Signal Lab에서만 raw 값과 근거를 표시한다.

## 수용 기준

- [ ] protocol quality와 UI quality 사이 C cast가 0개다.
- [ ] 모든 운전자 화면 입력에 metadata가 있다.
- [ ] derived value가 dependency보다 높은 evidence를 만들 수 없다.
- [ ] `uint32_t` monotonic wrap test가 통과한다.
- [ ] RTC invalid/oscillator-stop 상태가 CAN freshness에 영향을 주지 않는다.
- [ ] negative dBFS peak/level이 정확히 표시 model에 전달된다.
- [ ] owner가 다른 config를 잘못된 endpoint로 보내면 compile/test에서 실패한다.
- [ ] speed/FFT/audio/drive-mode metadata가 stale이면 dwell이 증가하지 않는다.
- [ ] 250 ms 초과 scheduler gap 한 번으로 automation 조건이 충족되지 않는다.
- [ ] window-local counter와 boot 누적 counter의 reset/wrap 의미가 type과 test로 구분된다.

## 검증 명령

```bash
cmake --build --preset host-sanitize
ctest --preset host-sanitize -R 'model|quality|time|owner' --output-on-failure
rg -n '\(canview_.*quality_t\)' firmware ui protocol
```

## migration

`CANVIEW_UI_QUALITY_*`, unsigned `fft_peak_tenth_db`, `sport_monitor_enabled`, 사용하지 않는 평균연비 field를 한 PR에서 정리한다. 임시 숫자 호환 enum은 만들지 않는다.


## 산출물·범위 경계

- 예상 산출물은 schema의 공통 enum, `shared/model/` 시간·품질 helper와 Controller/UI exhaustive adapter·model tests다. 차량 signal을 VERIFIED로 승격하거나 새 제어 scope를 만드는 작업은 범위 밖이다.
- migration 전후 enum/unit/time fixture와 consumer compile 결과를 남기며 unknown을 정상값으로 호환하지 않는다. 오류 시 해당 output을 unavailable로 닫는다.
