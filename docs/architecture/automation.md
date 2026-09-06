# CANView 자동 제어 로직

이 문서는 [아키텍처 개요](README.md)에 속한 자동화 상세 정본이다. 기능 경계는 [기능 설계](features.md), 실제 구현 순서와 gate는 [구현 준비 기준](implementation-readiness.md)을 함께 따른다.

## 1. 범위와 실행 위치

이 문서는 CAN 자동 밝기, 무조작 감광·기본 화면 복귀, 제한속도 경고, 주행 소음 기반 음량 보정, 자동 SPORT 전환의 실행 가능한 초기 로직을 정의한다. 기능마다 입력과 실패 영향이 다르므로 다음처럼 나눈다.

| 기능 | 실행 위치 | 주 입력 | 출력 |
|---|---|---|---|
| CAN 자동 밝기 | Controller | 차량 조명 점등, 클러스터 rheostat | LCD backlight PWM 목표값 |
| 무조작 감광 | Controller | 마지막 touch 시각, 사용자 timeout | 주행 화면 복귀와 낮은 PWM 목표값 |
| 제한속도 경고 | Controller | 현재 속도, 유효한 내비 제한속도 | 전역 overlay blink와 임시 PWM boost |
| FFT 소음 보정 | Controller | microphone FFT 특징, 차속, 오디오 상태 | 제한된 음량 offset 의도 명령 |
| 자동 SPORT | Communicator STM32 | 차속, CAN 종가속도, 기어·브레이크·ESC, 현재 mode | 검증된 drive-mode button event |

Controller가 계산한 음량 명령과 설정값은 차량 CAN frame이 아니다. Communicator가 capability, lease, 최신 차량 상태와 vehicle profile을 다시 확인한 뒤 실행한다. 자동 SPORT 판단은 차량 상태와 최종 CAN 송신 권한이 있는 STM32에서 수행한다.

기준 구현은 다음 파일에 있다.

- [`canview_controller_automation.c`](../../firmware/controller/components/canview_automation/canview_controller_automation.c): 자동 밝기와 FFT 소음 보정
- [`canview_auto_sport.c`](../../firmware/communicator/stm32/src/canview_auto_sport.c): SPORT 진입·복원 상태기계
- [`test_automation.c`](../../tests/automation/test_automation.c): 지속시간, 히스테리시스, 수동 우선 동작 시험

모든 시간 계산은 wall clock이 아니라 wrap-safe monotonic tick 차이를 사용한다. 조건 evidence는 tick당 최대 100 ms만 누적한다. `elapsed_ms > 250`이면 밝기·과속·전조등·음량·SPORT의 dwell을 초기화하고 그 tick의 새 명령과 SPORT feedback 완료 승격을 차단한다. 유휴 시간·PWM ramp·명령 timeout은 실제 경과 시간을 사용한다. monotonic generation 변경은 호출 adapter가 discontinuity로 전달해야 하며, 큰 gap을 정상 tick 여러 개로 나눠서 실차 evidence인 것처럼 재생하지 않는다.

각 상태 객체는 호출 task가 소유하고 `update/reset/reconcile`을 직렬화한다. 이 순수 C 로직은 HAL, LVGL, RTOS, 송신 transport를 직접 호출하지 않는다. 출력 action은 의미 명령의 의도이고 UI preview나 ACK는 차량 적용의 evidence가 아니다.

## 2. CAN 기반 자동 밝기

### 2.1 입력 신호

외부 조도 센서는 사용하지 않는다. 저장된 Hyundai DBC의 후보는 다음과 같다.

| 후보 | 용도 | 주의점 |
|---|---|---|
| `C_TailLampActivity` | 실제 야간 조명 활성 판정의 1순위 | 대상 차량 enum을 실차 검증해야 함 |
| `CF_Gway_HeadLampLow` | low beam 실제 점등 보조 판정 | 주간 수동 점등도 야간 밝기로 처리됨 |
| `CF_Gway_LightSwState` | light switch 위치 보조 정보 | `AUTO` 위치만으로 야간이라고 판단하지 않음 |
| `CF_Clu_RheostatLevel` | 운전자 클러스터 밝기 의도 | raw `0–31`의 방향과 유효값 검증 필요 |
| M-CAN `Clu_RheostatLvl` | 오디오 bus에서의 밝기 연동 후보 | range와 주기 미확정 |

현재 밝기 상태기계의 야간 입력은 `lighting_valid + tail_lamps_on + lighting_age_ms`다. `lighting_valid`는 검증된 실제 미등 source에만 허용하며 AUTO switch 위치를 대신 넣지 않는다. low-beam은 후보 보조 정보이고, 현재 API의 미등 값으로 임의 대체하지 않는다. 터널 진입처럼 미등이 켜질 때는 500 ms 지속 후 야간으로 전환하고, 소등은 1.5초 지속 후 주간으로 돌아간다. 자동 밝기를 꺼도 night mode 판정은 계속하지만 PWM base는 수동 설정값을 따른다.

밝기의 미등 기반 night mode와 §2.4의 전조등 미점등 경고는 독립이다. 미등이 켜져 야간 화면이어도 실제 low-beam이 꺼져 있으면 전조등 경고가 가능하다. 두 입력을 같은 boolean으로 합치지 않는다.

### 2.2 밝기 곡선

기본값은 다음과 같다.

```text
주간 목표 = 사용자가 정한 화면 밝기
야간 목표 = lerp(14%, 48%, verified_rheostat_percent)
rheostat 미수신 야간 목표 = 30%
전환 속도 = 전체 PWM 범위를 약 1.2초에 이동
조명 신호 stale = 마지막 유효한 감광·boost 전 base 유지
```

밝기 출력의 target 통합 지점은 `GPIO6` LEDC PWM이며 실제 PWM 적용은 Controller BSP task에서 연결한다. `last_safe_base_percent`는 ramp 출력인 `current_tenth_percent`와 별도로 저장한다. 초기값은 clamp한 초기 수동 밝기이고, 유효한 조명으로 계산한 day/night 목표 또는 자동 밝기를 끈 뒤의 명시적 수동 목표로만 갱신한다. 조명 age가 500 ms를 초과하거나 invalid이면 pending debounce를 지우고 마지막 base를 사용한다. stale 중에는 새로운 rheostat·자동 mode의 수동 설정값으로 base를 덮어쓰지 않는다.

idle 감광이나 과속 boost 결과는 base에 저장하지 않는다. 예를 들어 base 80%의 idle 목표는 매 update 28%이며, 출력에 0.35를 반복 곱하지 않는다. 과속 중 출력이 90%였어도 stale 이후 base는 80%다. ramp 중 stale이 되어도 마지막 유효 목표까지 제한된 속도로 이동한다. 재수신하면 새 base로 갱신하고 ramp한다.

| source | 현재 core 검사 | adapter 책임 |
|---|---|---|
| 미등 | `lighting_valid`, `lighting_age_ms <= 500` 기본값 | 검증된 실제 점등과 source age 전달 |
| rheostat | `dimmer_valid`; invalid이면 야간 기본 30% | 별도 age/quality 확인 후 valid 결정; 미등 age를 재사용하지 않음 |
| 경고용 차속 | `speed_valid`, `speed_age_ms <= speed_stale_timeout_ms`; 기본 150 ms | [ESP-NOW](protocols/esp-now.md) §10의 표시 기준과 일치시키고 age를 포화 처리 |
| 제한속도 | `speed_limit_valid`, `speed_limit_active`, 양의 limit | valid에 source freshness·quality를 포함; 현재 core에는 limit age 필드 없음 |

state의 마지막 night/base 보존은 source를 VALID로 승격하는 동작이 아니다. `IDLE_DIM`/`SPEED_WARNING` status가 brightness stale status를 덮어쓸 수 있으므로 UI adapter는 source quality를 별도로 유지한다.

### 2.3 무조작 감광과 제한속도 경고

마지막 touch부터 설정 시간이 지나면 `idle_dimmed`를 latch하고 `return_to_default_screen`을 한 번 발생시킨다. 기본 timeout은 30초이며 UI에서 15/30/60/120초만 고를 수 있다. 유휴 밝기는 기본 35%다. touch 입력은 현재 screen에 관계없이 latch를 즉시 해제하고 정상 day/night 목표로 ramp한다.

제한속도는 `speed_limit_active`, source valid, 현재 속도 freshness가 모두 참일 때만 평가한다. 진입과 해제 문턱을 분리한다.

```text
경고 진입 = speed >= limit × 1.10 상태가 500 ms 지속
경고 해제 = speed <  limit × 1.05 상태가 1,000 ms 지속
표지 blink = 400 ms 간격
경고 중 목표 밝기 = 기존 목표와 90% 중 큰 값
```

속도 invalid/151 ms 이상 stale 또는 제한속도 invalid/inactive/0이면 경고와 진입·해제·blink timer를 즉시 지운다. 이때 정상 속도 해제용 1초 debounce를 기다리지 않는다. 정상 입력으로 회복해도 500 ms 진입 evidence를 다시 채워야 한다.

경고는 touch를 가로채지 않고 idle latch도 지우지 않는다. 따라서 이미 감광된 상태에서 과속하면 경고 기간에만 밝아지고, 해제 뒤에는 base의 35% 유휴 목표로 부드럽게 돌아간다. 밝기 우선순위는 `주간/야간 base → idle dim → speed warning boost`다. warning blink는 인지성을 위해 단계 전환하고 backlight는 급격한 점프를 막기 위해 ramp한다.

### 2.4 일몰 후 전조등 미점등 경고

저장 DBC에는 1차 차량의 GPS 좌표·현재 시각 CAN signal이 없으므로 `PCF85063 RTC + 설정된 일출·일몰`을 사용한다. GPS·시간 후보의 조사 결과는 [GPS·시간 조사](../vehicle/gps-time-investigation.md)에 있다.

경고 입력은 `rtc_valid`, `solar_valid`, `vehicle_awake`, 실차에서 확인된 실제 low-beam 상태 `headlamp_valid/headlamps_on`과 각 입력의 age다. `C_TailLampActivity`는 밝기 night mode용이며 전조등 ON의 대체 근거로 쓰지 않는다. 현재 경고 core는 RTC·solar·headlamp age에 기본 2,000 ms 상한을 적용한다. 다음 조건이 모두 충족될 때만 밤으로 판정한다.

```text
enabled AND RTC·일출·일몰·실제 전조등 signal이 valid/fresh
AND vehicle_awake
AND (local_time >= sunset OR local_time < sunrise)
AND 가장 최근 sunset으로부터 경과 >= 60 s
```

밤에 실제 전조등 OFF가 2초 유지되면 `warning_active`를 켠다. 실제 전조등 ON이 1초 유지되면 끄며, 순간적인 CAN bit 흔들림은 무시한다. 입력이 stale이거나 차량이 sleep이면 경고와 evidence를 즉시 지운다. OR의 일출 전 분기도 전체 valid/fresh/awake guard 아래 있으며, 자정 이후에도 직전 일몰에서의 grace를 계산한다. 제한속도 경고가 동시에 있으면 UI는 제한속도 경고를 우선하고, 정상 상태에서는 경고 overlay를 숨긴다. 이 core는 경고 상태만 반환하며 PWM boost는 만들지 않는다.

## 3. FFT 기반 주행 소음 음량 보정

### 3.1 FFT 특징값

16 kHz, 1024-point FFT의 표시용 peak 값을 그대로 음량에 연결하지 않는다. DSP task가 다음 두 특징을 별도로 계산한다.

```text
focus_band_level = 선택 대역의 power 합 -> log10 -> 1초 Leq
band_excess = focus_band_level - learned_baseline(speed, source_volume)
dominant_frequency = attack/release smoothing을 거친 focus peak
```

`band_excess`는 절대 dBA가 아니라 같은 차속·오디오 조건에서 학습한 기준보다 얼마나 커졌는지를 뜻한다. FFT invalid, clipping, confidence 65% 미만에서는 증감 evidence를 즉시 지우고 자동 증감 명령은 만들지 않는다. clipping/underrun은 입력 adapter가 `fft_valid=false`로 전달한다.

설정의 주파수 대역은 다음 preset으로 변환한다.

| 설정 | focus band | 목적 |
|---|---:|---|
| 노면 | 125–500 Hz | 타이어·노면 저주파가 지배적인 장착 위치 |
| 표준 | 160–1,250 Hz | 기본값. 노면과 중역 소음을 함께 감시 |
| 풍절 | 500–2,000 Hz | 고속 풍절음이 지배적인 장착 위치 |

### 3.2 민감도와 반응

| 민감도 | 올림 문턱 | 내림 문턱 | 히스테리시스 |
|---|---:|---:|---:|
| 낮음 | 기준 대비 +7.0 dB | +4.5 dB 이하 | 2.5 dB |
| 보통 | 기준 대비 +5.0 dB | +2.5 dB 이하 | 2.5 dB |
| 높음 | 기준 대비 +3.5 dB | +1.5 dB 이하 | 2.0 dB |

| 반응 | 올림 지속시간 | 내림 지속시간 |
|---|---:|---:|
| 느리게 | 8초 | 18초 |
| 자연스럽게 | 5초 | 12초 |
| 빠르게 | 3초 | 8초 |

기본 설정은 `표준 / 보통 / 자연스럽게 / 최대 +4 step`이다. 일반적인 상승보다 하강을 더 느리게 해 경계에서 음량이 오르내리는 pumping을 줄인다.

### 3.3 evidence와 step 제어

매 FFT update에서 다음 조건을 평가한다.

```text
raise = speed >= 30 km/h
        AND peak inside focus band
        AND band_excess >= raise_threshold
        AND valid/confident/not reverse

lower = speed < 30 km/h
        OR peak outside focus band
        OR band_excess <= lower_threshold
        OR reverse
```

`raise`가 참이면 attack evidence를 누적하고 release evidence를 2배 속도로 줄인다. `lower`가 참이면 반대로 처리한다. 두 문턱 사이에서는 양 evidence를 천천히 줄인다. 따라서 1–2회의 peak 이탈이나 짧은 경계 진동은 이전 누적을 즉시 뒤집지 않는다.

누적시간을 채우면 목표 offset을 정확히 한 step만 바꾼다. 이후 최소 3초 command cooldown과 2초 microphone feedback freeze를 적용한다. 같은 소음이 계속돼도 다음 step은 새 attack 시간을 다시 만족해야 한다. 음량이 한 번에 크게 뛰지 않고 `0 → +1 → +2`처럼 완만하게 움직인다.

`SET_OFFSET` 반환 시 `command_pending`을 세우고 다음 offset 요청과 evidence 누적을 막는다. `target_offset_steps`는 미확인 의도이고 `applied_offset_steps`와 같아졌다는 사실이나 `ACK(ACCEPTED/DUPLICATE)`만으로 pending을 해제하지 않는다. 초기 입력의 applied 값은 reset 시 초기값으로만 사용한다.

호출 orchestration은 일치하는 terminal `COMMAND_RESULT(COMPLETED)`와 최신 AMP feedback을 모두 확인한 후 `canview_adaptive_volume_reconcile()`로 실제 offset을 주입한다. 거부·취소·timeout 뒤에도 현재 snapshot을 확인하기 전에는 reconcile하지 않는다. reconcile은 pending과 attack/release evidence를 지우되 수동 hold를 유지한다. pending 중 수동 조작은 즉시 hold를 시작하고 0 offset 의도를 보존하며, 진행 중 요청의 취소/종결과 실제 상태 확인 전에는 두 번째 요청을 내지 않는다. 미확인 target을 NVS에 성공값처럼 저장하지 않는다.

현재 순수 C API에는 token/session 식별자와 차속·FFT·audio feedback별 age가 없다. source별 freshness ceiling, authenticated terminal 매칭, 중복/late result 차단, 취소 및 reconnect snapshot adapter는 T-305 범위다. `reconcile()` 자체를 token 검증기로 간주하지 않으며, ACK callback이나 UI preview에서 호출하지 않는다. 이 adapter가 연결되기 전 volume action은 host 의도 시험에만 사용한다.

다음 guard를 함께 적용한다.

- 최대 offset은 사용자 설정 `+2/+3/+4`를 넘지 않는다.
- 사용자가 head unit volume을 직접 바꾸면 offset을 0으로 복원하고 60초간 자동화를 멈춘다.
- reverse에서는 새 상승을 금지하고 release 조건으로 처리한다.
- navigation·parking·안전 안내처럼 OEM priority audio가 활성화된 동안 evidence와 명령을 정지한다.
- 자동 step 뒤 2초간 microphone 판정을 멈춰 스피커 음량 증가가 다시 소음 증가로 측정되는 양의 feedback을 줄인다.
- head unit의 OEM SDVC와 동시에 켜지지 않도록 한다.

음악의 저음이 선택 대역에 오래 남으면 road noise로 오인할 수 있다. 따라서 실차 검증 전에는 최대 offset을 작게 두고, source volume별 baseline과 mute sample을 함께 수집해야 한다. 완전한 source separation을 보장하지 않는 한 이 기능은 청취 편의 기능이지 소음계가 아니다.

## 4. 자동 SPORT와 이전 mode 복원

### 4.1 진입 조건

자동 SPORT는 두 경로 중 하나가 지속될 때만 진입한다.

```text
고속 경로:
  speed >= 설정값(60/70/80 km/h) for 2.5 s

급가속 경로:
  speed >= 35 km/h
  AND filtered_longitudinal_acceleration >= 1.40 m/s² for 0.8 s
```

CAN 종가속도는 status/diagnostic bit가 정상인 sample만 사용한다. 20–50 Hz sample에 5-sample median과 약 250 ms 저역통과를 적용한 뒤 상태기계에 넣어 단일 spike, pothole, quantization noise를 제거한다. 급가속 감지를 꺼도 고속 경로는 유지된다.

진입 직전 mode가 `NORMAL`, `ECO`, `COMFORT`, `SMART` 중 하나로 확인되면 그 값을 `previous_mode` snapshot으로 고정한다. SPORT가 이미 운전자에 의해 선택돼 있거나 현재 mode가 unknown이면 자동 진입하지 않는다.

### 4.2 복귀 히스테리시스

진입 속도와 복귀 속도를 같게 두지 않는다.

| 진입 설정 | SPORT 진입 | 복귀 후보 |
|---:|---:|---:|
| 60 km/h | 60 km/h 이상 2.5초 | 45 km/h 이하 |
| 70 km/h | 70 km/h 이상 2.5초 | 55 km/h 이하 |
| 80 km/h | 80 km/h 이상 2.5초 | 65 km/h 이하 |

복귀 후보 속도 이하이면서 종가속도가 `0.35 m/s²` 이하인 상태가 8초 유지돼야 복귀한다. SPORT 최소 유지시간은 15초다. 급가속 경로로 40–50 km/h에서 진입해도 최소 유지시간과 8초 release가 모두 끝난 뒤 직전 mode로 돌아간다.

```text
ARMED
  -> ENTER_PENDING: 고속 또는 중속 급가속 지속
  -> SPORT_ACTIVE: SPORT feedback 확인
  -> EXIT_PENDING: 저속·낮은 가속 release 지속
  -> ARMED: previous_mode feedback 확인
```

`SPORT ↔ NORMAL`로 고정하지 않고 `SPORT ↔ previous_mode`로 동작한다. 복귀 명령은 snapshot의 raw mode 값을 직접 ECU에 쓰지 않는다. 대상 vehicle profile이 검증한 DRIVE MODE button event를 한 번씩 실행하고 매번 mode feedback을 확인해 snapshot mode까지 제한적으로 순환한다.

### 4.3 운전자 우선과 실패 처리

- 운전자가 물리 DRIVE MODE 버튼을 조작하면 즉시 `MANUAL_HOLD`로 들어가고 자동화가 더 이상 mode를 덮어쓰지 않는다.
- `MANUAL_HOLD`는 자동 기능을 껐다 다시 켜는 명시적 재arm 또는 ignition cycle까지 유지한다.
- 기어·브레이크·ABS/TCS/ESC·mode·link·lease 중 하나라도 유효하지 않으면 `INHIBITED`이며 새 command를 만들지 않는다.
- pending 완료에는 freshness와 local safety가 필요하다. stale mode가 SPORT/previous_mode와 같아도 완료나 소유권 확보로 승격하지 않는다. deadline 이상 경과한 tick은 feedback보다 timeout을 먼저 판정하고 `MANUAL_HOLD`로 가며, 명시적 재arm 없이 재시도하지 않는다.
- 자동화가 소유하던 SPORT와 다른 유효 mode가 관찰되면 물리 버튼 이벤트가 누락됐어도 소유권·previous snapshot을 버리고 `MANUAL_HOLD`로 간다. 이는 권한 철회이므로 scheduler gap에도 수행하며 새 명령/feedback 승격 금지와 혼동하지 않는다. 그 tick과 이후 사용자가 선택한 SPORT에서 오래된 snapshot으로 복귀 pulse를 만들지 않는다.
- link loss나 bus fault 중에는 복귀를 시도하지 않는다. 상태가 다시 신뢰 가능해진 뒤 현재 mode와 snapshot을 대조한다.

현재 core의 1,500 ms pending timer는 action 생성 다음 tick부터 계산하는 host 상태기계 시간이다. target executor의 첫 성공한 physical TX-complete 시각과 transaction feedback 연결은 T-106/T-305에서 구현해야 한다. core의 `ACTIVE`를 인증된 wire `COMPLETED`나 차량 TX gate 통과로 표시하지 않는다.

## 5. 4WD 바퀴 게이지와 TPMS

주행 화면은 중앙 차량 주변의 네 바퀴마다 8단 수평 분절 gauge를 사용한다. 청색 segment 수는 `wheel_drive_percent[FL/FR/RL/RR]`, 각 gauge 아래 숫자는 `tire_pressure_tenth_psi[]`다. 순간연비는 차량 중앙 정보 창에 표시한다. pressure, 4WD, 연비 source의 품질은 서로 독립적으로 관리한다.

저장 DBC의 `TPMS11`, ID `0x593`에는 `PRESSURE_FL/FR/RL/RR`, `STATUS_TPMS`, `TPMS_W_LAMP` 후보가 있다. pressure field의 단위, invalid raw, 좌우 순서는 대상 차량에서 공기압을 한 바퀴씩 변화시키며 검증해야 한다. 경고 bit가 확인된 바퀴는 gauge 테두리와 숫자를 amber로 바꾸고, 신호가 unavailable이면 마지막 압력을 고정하지 않고 `—`로 표시한다.

공개 DBC만으로 바퀴별 실제 구동 토크는 얻을 수 없다. 따라서 `wheel_drive_percent`는 검증된 axle/wheel model이 생기기 전까지 `구동 지수`이며 실제 torque percentage로 기록하거나 합계 100을 강제하지 않는다. 현재의 rear clutch duty를 앞뒤 바퀴 토크로 임의 분배하지 않는다.

## 6. 설정과 저장

운전자 설정 화면에는 다음 항목만 둔다.

| 그룹 | 항목 | 값 |
|---|---|---|
| DISPLAY | 화면 밝기, CAN 자동 밝기 | 10–100%, 사용/끔 |
| DISPLAY | 무조작 복귀 | `15/30/60/120초`, 기본 `30초` |
| ROAD NOISE | 소음 보정, 주파수 대역, 민감도, 반응, 최대 보정 | preset과 `+2/+3/+4` |
| SPORT AUTO | 자동 전환, 진입 속도, 급가속 감지 | 사용/끔, `60/70/80 km/h` |

Controller-local 값인 밝기와 FFT preset은 [OTA](ota.md)의 Controller config A/B에 저장한다. SPORT 설정은 `CONFIG_SET`으로 Communicator에 보내고 일치하는 `CONFIG_RESULT` 성공과 새 `state_revision`, owner readback을 확인한 뒤에만 Controller mirror를 갱신한다. NVS는 비권위 cache이며 transport ACK는 적용 완료가 아니다. 적용 상태 `APPLIED`는 owner가 확정한 결과를 뜻하며, 차량 명령에는 별도로 `COMMAND_RESULT(COMPLETED)`와 matching feedback이 필요하다. 저장 중 전원이 끊겨도 이전 valid generation을 복원할 수 있도록 version, length, CRC가 있는 두 slot을 번갈아 commit한다.

설정 변경은 차량 정차에서만 허용한다. 운행 중에는 현재 자동화 상태와 offset만 표시하고 세부 threshold는 편집하지 않는다.

## 7. 검증 기준

- 조명 신호 300 ms pulse로 밝기가 야간 mode로 바뀌지 않는다.
- 조명 signal stale에서 밝기가 갑자기 최대값으로 이동하지 않는다.
- 무조작 30초에 한 번만 주행 화면 복귀 pulse가 발생하고 touch가 감광을 해제한다.
- 제한속도 110% 미만의 순간 spike로 경고가 켜지지 않고 105–110% 구간에서 상태가 왕복하지 않는다.
- 감광 중 제한속도 경고가 끝나면 감광 밝기로 복귀하고 touch는 항상 계속 동작한다.
- 선택 대역의 높은 소음이 attack 시간보다 짧으면 음량이 오르지 않는다.
- peak가 짧게 대역을 벗어나도 음량이 즉시 내려가지 않는다.
- 지속 release에서 1 step씩, step interval보다 빠르지 않게 내려간다.
- 수동 volume 조작 뒤 60초간 자동 명령이 없다.
- 진입 속도와 복귀 속도 사이에서 SPORT가 반복 전환되지 않는다.
- 중속 급가속 한 sample spike로 SPORT가 켜지지 않는다.
- SPORT 진입 전 mode와 복귀 feedback이 일치한다.
- 물리 mode 조작 뒤 자동 command가 재개되지 않는다.
- TPMS stale과 실제 `0 psi`를 같은 상태로 표현하지 않는다.

현재 실행 가능한 host 회귀는 다음 명령이다. Windows에서는 먼저 Visual Studio Developer PowerShell 환경을 연다. 이 명령은 target/HIL·차량 검증이나 UI preview 실행 명령이 아니다.

```powershell
cmake -S tests/automation -B .tools/automation-worker-build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build .tools/automation-worker-build
ctest --test-dir .tools/automation-worker-build --output-on-failure
```

`test_automation_regressions.c`는 반복 stale+idle, boost 후 stale 복귀/ramp, 차속 150/151 ms 경계, invalid FFT evidence, 단일 pending, stale SPORT feedback·외부 mode·deadline, 251 ms/5초/`UINT32_MAX` gap, 중복 ACK/result와 tick wrap을 검사한다. 정상 지속시간 fixture는 100 ms sample 반복이고 gap 시험은 원 API를 직접 호출한다. MSVC는 `/W4 /WX /utf-8`로 build하며 assert를 Release에서도 유지한다. v1.2 draft protocol의 GNU packed 대신 사용하는 `msvc_wire_layout.h`는 시험 전용 경계이고 runtime ABI 이식 완료를 의미하지 않는다.
