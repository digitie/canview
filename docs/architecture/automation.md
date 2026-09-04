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

모든 시간 계산은 wall clock이 아니라 wrap-safe monotonic tick 차이를 사용한다. task가 오래 정지했다가 재개되면 한 번의 큰 `elapsed_ms`를 그대로 넣지 않고 운행 입력을 stale 처리한 뒤 상태기계를 재동기화한다.

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

점등 여부는 switch 위치가 아니라 실제 tail/low-beam 활성 신호를 우선한다. 터널 진입처럼 조명이 켜질 때는 500 ms 지속 후 야간으로 전환하고, 짧은 가로수 그림자나 조명 신호 흔들림으로 밝아지는 것을 막기 위해 소등은 1.5초 지속 후 주간으로 돌아간다.

### 2.2 밝기 곡선

기본값은 다음과 같다.

```text
주간 목표 = 사용자가 정한 화면 밝기
야간 목표 = lerp(14%, 48%, verified_rheostat_percent)
rheostat 미수신 야간 목표 = 30%
전환 속도 = 전체 PWM 범위를 약 1.2초에 이동
조명 신호 stale = 마지막 안전 밝기 유지
```

밝기 출력은 `GPIO6` LEDC PWM에 적용하되 목표값이 바뀌어도 즉시 점프시키지 않는다. 조명 신호가 500 ms 넘게 stale이면 주간 100%로 되돌아가 운전자를 눈부시게 하지 않고 마지막 유효 밝기를 유지한다. 재수신 시 새 목표까지 다시 ramp한다.

### 2.3 무조작 감광과 제한속도 경고

마지막 touch부터 설정 시간이 지나면 `idle_dimmed`를 latch하고 `return_to_default_screen`을 한 번 발생시킨다. 기본 timeout은 30초이며 UI에서 15/30/60/120초만 고를 수 있다. 유휴 밝기는 기본 35%다. touch 입력은 현재 screen에 관계없이 latch를 즉시 해제하고 정상 day/night 목표로 ramp한다.

제한속도는 `speed_limit_active`, source valid, 현재 속도 freshness가 모두 참일 때만 평가한다. 진입과 해제 문턱을 분리한다.

```text
경고 진입 = speed >= limit × 1.10 상태가 500 ms 지속
경고 해제 = speed <  limit × 1.05 상태가 1,000 ms 지속
표지 blink = 400 ms 간격
경고 중 목표 밝기 = 기존 목표와 90% 중 큰 값
```

경고는 touch를 가로채지 않고 idle latch도 지우지 않는다. 따라서 이미 감광된 상태에서 과속하면 경고 기간에만 밝아지고, 해제 뒤에는 다시 35% 유휴 밝기로 부드럽게 돌아간다. 밝기 우선순위는 `주간/야간 base → idle dim → speed warning boost`다. warning blink는 인지성을 위해 단계 전환하고 backlight는 급격한 점프를 막기 위해 ramp한다.

### 2.4 일몰 후 전조등 미점등 경고

저장 DBC에는 1차 차량의 GPS 좌표·현재 시각 CAN signal이 없으므로 `PCF85063 RTC + 설정된 일출·일몰`을 사용한다. GPS·시간 후보의 조사 결과는 [`can-gps-time-investigation.md`](../vehicle/gps-time-investigation.md)에 있다.

경고 입력은 `rtc_valid`, `solar_window_valid`, `vehicle_awake`, `CF_Gway_HeadLampLow`/`C_TailLampActivity` 중 실차에서 확인된 조명 상태와 각 입력의 age다. 다음 조건이 모두 충족될 때만 밤으로 판정한다.

```text
RTC·일출·일몰·조명 signal이 stale 아님
AND vehicle_awake
AND local_time >= sunset + 60 s
   OR local_time < sunrise
```

밤에 전조등/미등 OFF가 2초 유지되면 `headlamp_warning_active`를 켠다. 조명 ON이 1초 유지되면 끄며, 순간적인 CAN bit 흔들림은 무시한다. 입력이 stale이거나 차량이 sleep이면 false positive를 피하기 위해 경고를 즉시 끈다. 제한속도 경고가 동시에 있으면 제한속도 경고를 우선하고, 정상 상태에서는 경고 overlay를 숨긴다. 터치 입력을 가로채지 않으며, 경고 해제 후 backlight는 원래 야간/유휴 목표까지 ramp한다.

## 3. FFT 기반 주행 소음 음량 보정

### 3.1 FFT 특징값

16 kHz, 1024-point FFT의 표시용 peak 값을 그대로 음량에 연결하지 않는다. DSP task가 다음 두 특징을 별도로 계산한다.

```text
focus_band_level = 선택 대역의 power 합 -> log10 -> 1초 Leq
band_excess = focus_band_level - learned_baseline(speed, source_volume)
dominant_frequency = attack/release smoothing을 거친 focus peak
```

`band_excess`는 절대 dBA가 아니라 같은 차속·오디오 조건에서 학습한 기준보다 얼마나 커졌는지를 뜻한다. FFT invalid, clipping, confidence 65% 미만에서는 증감 evidence를 서서히 지우고 음량 명령은 만들지 않는다.

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

`SET_OFFSET` 요청 뒤 상태기계의 target은 Communicator `COMMAND_RESULT`와 AMP feedback으로 확인한다. 거부·timeout·외부 knob 변경에서는 `canview_adaptive_volume_reconcile()`로 실제 적용 offset을 다시 주입하고 attack/release evidence를 버린다. 확인되지 않은 target을 반복 송신하거나 NVS에 성공값처럼 저장하지 않는다.

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
- mode feedback은 command 후 1.5초 안에 확인해야 한다. 진입 실패는 다시 `ARMED`, 복귀 실패는 반복 pulse 대신 `MANUAL_HOLD`로 간다.
- link loss나 bus fault 중에는 복귀를 시도하지 않는다. 상태가 다시 신뢰 가능해진 뒤 현재 mode와 snapshot을 대조한다.

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

Controller-local 값인 밝기와 FFT preset은 Controller NVS에 저장한다. SPORT 설정은 `CONFIG_SET`으로 Communicator에 보내고 ACK 뒤에만 Controller NVS mirror를 갱신한다. 저장 중 전원이 끊겨도 이전 valid generation을 복원할 수 있도록 version, length, CRC가 있는 두 slot을 번갈아 commit한다.

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
