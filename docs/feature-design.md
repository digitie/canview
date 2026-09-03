# CANView 기능 조사 및 구현 설계

## 1. 범위와 결론

이 문서는 2017년식 Hyundai Tucson TL 2.0 디젤 4WD BlueLink 차량을 1차 대상으로 다음 기능을 조사하고 구현 경계를 정한다.

1. 4WD 상태와 토크 배분 표시
2. DPF 및 관련 상태 표시
3. 취침 모드와 뒷좌석 증폭 모드
4. 속도·주변 소음 기반 자동 음량
5. 속도·가속도 기반 SPORT mode 전환·복귀
6. 차량 조명 CAN 기반 자동 밝기

가장 중요한 결론은 다음과 같다.

- 저장된 `opendbc`에는 4WD clutch torque·duty·pressure 후보가 있지만, **검증된 전륜:후륜 축 토크 비율은 없다**. 1차 UI는 `후륜 결합 추정`으로 표시해야 한다.
- 공개 DBC에서 확인되는 DPF 직접 신호는 `DPF_LAMP_STAT`뿐이며 raw code의 의미표도 없다. soot mass, regeneration active, ash load는 진단기와 실차 비교 전에는 제공할 수 없다.
- M-CAN DBC에 volume/fader/balance/rear speaker mute/SDVC 후보가 있으나 주기·alive·checksum·값 범위가 비어 있다. 물리 head unit 조작 capture 전에는 송신하지 않는다.
- 보드에는 ES8311과 SMD microphone이 있어 주변 소음 prototype은 추가 hardware 없이 시작할 수 있다. 설치 위치와 차량 오디오의 self-noise가 문제가 되면 별도 microphone node로 분리한다.
- SPORT mode 자동화는 powertrain 체감에 관여한다. 첫 release는 감지·추천만 제공하고, 실제 버튼 pulse는 bench와 폐쇄 시험 검증 뒤 opt-in으로 연다.
- 자동 밝기는 외부 조도 센서 없이 실제 등화 점등과 cluster rheostat CAN 값만 사용한다. 조명 switch의 `AUTO` 위치만으로 야간을 판정하지 않는다.

## 2. 근거 등급

UI와 구현 issue에는 모든 차량 신호에 다음 등급을 붙인다.

| 등급 | 의미 | UI 표현 | 제어 사용 |
|---|---|---|---|
| A | 대상 차량에서 반복 capture·물리 동작과 대조 완료 | 일반 표시 | 별도 안전 검증 후 가능 |
| B | 저장 DBC에 정의됐으나 대상 연식·트림 미검증 | `후보` 또는 `추정` | 금지 |
| C | 여러 신호로 계산한 heuristic | `추정`+confidence | 금지 |
| D | 존재 미확인·community 주장뿐 | 진단 화면에만 | 금지 |

현재 이 문서의 대상 차량 신호는 모두 B 이하에서 시작한다. 이름이 그럴듯하거나 다른 Hyundai 차량에서 동작했다는 사실은 A 승격 근거가 아니다.

## 3. 공통 architecture

```text
대상 차량 CAN 1/2/3
        |
        v
Communicator
  - listen-only capture
  - target profile / DBC decode
  - signal quality + timestamp
  - command allow-list / safety gate
        |
        | encrypted ESP-NOW
        v
320×480 Controller
  - 운전 화면: 2초 이내 상태 파악
  - 정차 화면: 설정·진단·calibration
  - 명령은 semantic request만 생성
```

차량 연결 Communicator와 Controller를 분리하는 이유는 차량 배선·transceiver·서지 보호를 화면에서 격리하고, UI 장애가 CAN timing을 방해하지 않게 하기 위해서다. 통신 단절 시 Communicator는 표시값 전송만 잃고 차량의 기존 통신에는 영향을 주지 않아야 한다.

## 4. 4WD 상태와 토크 배분

### 4.1 DBC 후보

[`dbc/opendbc/hyundai_can.dbc`](../dbc/opendbc/hyundai_can.dbc)에 다음 신호가 있다.

| Message | ID | Signal | DBC 단위/범위 | 후보 의미 |
|---|---:|---|---|---|
| `_4WD11` | `0x428` | `_4WD_TYPE` | raw 0–3 | 4WD 종류 |
| `_4WD11` | `0x428` | `_4WD_SUPPORT` | raw 0–3 | 지원 상태 |
| `_4WD11` | `0x428` | `_4WD_ERR` | raw 0–255 | fault code/status |
| `_4WD11` | `0x428` | `CLU_DUTY` | 0–64 % | coupling clutch 구동 duty |
| `_4WD11` | `0x428` | `_2H_ACT`, `_4H_ACT`, `AUTO_ACT`, `LOCK_ACT` | boolean | mode active 후보 |
| `_4WD11` | `0x428` | `_4WD_TQC_CUR` | 0–65,535 Nm | 현재 4WD torque command/estimate 후보 |
| `_4WD12` | `0x429` | `FRSS`, `FLSS`, `RRSS`, `RLSS` | km/h | 4WD ECU가 보는 wheel speed |
| `_4WD12` | `0x429` | `CLU_PRES` | −50–50 bar | clutch pressure 후보 |
| `_4WD13` | `0x42A` | `_4WD_CURRENT` | DBC 0.390625 A/bit | actuator current 후보 |
| `_4WD13` | `0x42A` | `_4WD_POSITION` | 0.015625 deg/bit | actuator position 후보 |
| `_4WD13` | `0x42A` | `_4WD_CLU_THERM_STR` | 0–100 % | clutch thermal stress 후보 |
| `_4WD13` | `0x42A` | `_4WD_STATUS` | raw | 상태 code |
| `WHL_SPD11` | `0x386` | `WHL_SPD_FL/FR/RL/RR` | 0.03125 km/h/bit | ABS wheel speed |

DBC의 `_4WD_CURRENT`와 `_4WD_POSITION`은 declared physical range와 unsigned encoding이 어색하므로 raw 값·offset·signedness를 특히 검증한다. `_4WD_TQC_CUR`도 16-bit 전체 범위가 실제 65,535 Nm일 수 없으므로 scale 또는 invalid value 영역이 미완성일 가능성이 크다.

### 4.2 “토크 배분”으로 바로 표시할 수 없는 이유

4WD clutch 명령 토크는 후륜축에서 실제 발생한 토크와 같지 않다. clutch slip, oil temperature, driveline ratio, 전륜 traction, ESC intervention, actuator calibration이 개입한다. 공개 DBC에는 front axle torque와 rear axle torque를 같은 기준점에서 측정한 쌍이 없다.

따라서 다음 표현을 구분한다.

| 표현 | 필요 근거 | 현재 가능 여부 |
|---|---|---|
| `4WD AUTO/LOCK` | mode bit 실차 대조 | capture 후 가능 |
| `후륜 결합 명령 38%` | clutch duty range 검증 | 가능 후보 |
| `후륜 결합 추정 38%` | duty/torque/pressure calibration | 단계적 가능 |
| `전륜 62 : 후륜 38 토크 배분` | 양 axle 실제 torque 검증 | 현재 불가 |

UI prototype의 `38%`는 layout 검증용 demo 값이며 차량 측정값이 아니다.

### 4.3 계산 model

1차 model은 `rear_coupling_index`만 계산한다.

```text
inputs = CLU_DUTY, _4WD_TQC_CUR, CLU_PRES, _4WD_CURRENT,
         front/rear wheel slip, speed, LOCK_ACT, quality

rear_coupling_index = calibrated_monotonic_map(inputs)  // 0..100
```

검증 전에는 `CLU_DUTY / verified_max_duty`를 별도 raw bar로만 표시한다. duty를 선형 토크 비율로 간주하지 않는다. calibration table은 차량 profile에 두며 다음 조건별 표본을 분리한다.

- 정지/저속/중속/고속
- 직진/조향
- AUTO/LOCK
- 건조 노면/저마찰 노면
- clutch cold/warm/thermal protection

물리 검증 없이 front ratio를 `100 - rear_coupling_index`로 만들지 않는다. dyno 또는 axle torque 계측이 가능해진 뒤에만 `torque distribution`이라는 label로 승격한다.

주행 UI는 네 바퀴 자체를 작은 구동 gauge로 사용한다. 공개 DBC만으로 바퀴별 torque를 얻을 수 없으므로 gauge 입력은 별도 품질값이 있는 `wheel_drive_percent[]` 구동 지수다. axle torque model이 검증되기 전에는 앞뒤 합계를 100으로 만들거나 rear clutch duty를 네 바퀴에 임의 배분하지 않는다.

같은 바퀴 안에는 `TPMS11`, ID `0x593`의 `PRESSURE_FL/FR/RL/RR` 후보를 표시한다. 단위, invalid raw, 바퀴 순서가 실차에서 확인되기 전에는 unavailable로 두며 압력 품질과 4WD 품질을 독립 관리한다.

### 4.4 상태기계와 오류

```text
UNAVAILABLE -> CANDIDATE -> CALIBRATING -> ESTIMATED -> VERIFIED
      ^             |             |             |
      +------ stale/fault/out-of-range ----------+
```

- `_4WD_ERR != verified_no_error`이면 숫자 대신 `4WD 점검`을 우선한다.
- 관련 frame이 250 ms 넘게 stale이면 bar를 고정하지 않고 `—`로 바꾼다.
- wheel speed 차이가 sensor plausibility 범위를 벗어나면 coupling 계산 confidence를 낮춘다.
- AUTO/LOCK bit가 충돌하면 raw 상태를 기록하고 `상태 확인 필요`로 표시한다.
- thermal stress가 검증된 warning threshold를 넘으면 torque graphic보다 warning을 우선한다.

### 4.5 실차 검증 절차

1. ignition off/on, engine off/on에서 `0x428–0x42A` 존재 확인
2. 4WD LOCK 물리 버튼 on/off와 bit 변화 대조
3. 리프트 또는 안전한 저속 환경에서 네 wheel speed 순서 확인
4. 일정 속도 건조 직진에서 duty/torque/pressure baseline 기록
5. steering, acceleration, deceleration에 따른 상관관계 기록
6. ESC/ABS 개입 capture는 폐쇄 시험장에서만 수행
7. 3회 이상 동일 동작에서 값과 timing 재현
8. vehicle profile에 enum, invalid raw, range, period를 확정

## 5. DPF 및 배출 관련 상태

### 5.1 공개 DBC에서 확인된 범위

`EMS19`, ID `0x492`에는 다음이 있다.

| Signal | DBC 내용 | 해석 상태 |
|---|---|---|
| `DPF_LAMP_STAT` | 2 bit raw code `0–3` | codebook 없음, B |
| `CR_Ems_EngOilTemp` | 0.75 scale, −40 offset | engine oil temperature 후보 |
| `CF_Ems_ModeledAmbTemp` | 0.5 scale, −41 offset | modeled ambient temperature 후보 |
| `BAT_LAMP_STAT` | boolean | 배터리 경고 후보 |
| `CF_Ems_OPSFail` | boolean | oil pressure switch fail 후보 |

현재 저장된 Hyundai DBC에는 대상 차량용 soot mass, soot percentage, ash load, differential pressure, EGT, regeneration active, distance since regeneration의 확정 신호가 없다.

### 5.2 기능 단계

#### 단계 1 — passive CAN monitor

- DPF lamp raw와 검증된 enum
- MIL과 engine fault 상태
- RPM, vehicle speed, coolant/oil temperature
- 마지막 정상 sample 시간
- lamp가 켜졌을 때 사용자 설명과 정비 권고

이 단계는 diagnostic request를 보내지 않으며 첫 release에 적합하다.

#### 단계 2 — standard OBD/UDS query

ECU supported PID bitmap을 먼저 확인하고, ECU가 지원한다고 응답한 표준 항목만 낮은 주기로 요청한다. SAE J1979 계열의 diesel aftertreatment PID 후보인 Mode 01 `0x7A/0x7B` 등은 사용 중인 표준판의 정식 bit layout으로 구현해야 한다. PID 번호만 보고 차종별 payload를 추측하지 않는다.

- query 주기 기본 0.5–1 Hz
- Communicator diagnostic scheduler가 기존 bus load를 감시
- multi-frame ISO-TP timeout과 flow control 분리
- negative response와 unsupported를 정상 상태로 취급
- engine/ECU sleep을 깨우는 polling 금지

#### 단계 3 — Hyundai proprietary DID

정식 진단기 또는 OEM scan tool에서 같은 차량을 조회하면서 request/response를 동시 capture해 DID, session, security requirement, unit을 확인한다. 인터넷 forum의 Mode 22 PID를 그대로 hard-code하지 않는다. brute-force DID scan은 ECU 부하·상태 변경 위험 때문에 금지한다.

검증되면 다음 필드를 독립적으로 제공할 수 있다.

- soot calculated / measured
- DPF differential pressure
- upstream/downstream EGT
- regeneration request / active / inhibited reason
- distance/time since last completed regeneration
- ash accumulation 또는 service status

각 값은 scan tool 표시와 최소 3개 운행 cycle에서 비교하고 unit·offset을 확정한다.

### 5.3 regeneration 추정

직접 `regen_active`가 없을 때 EGT, idle RPM, instantaneous fuel rate, fan, post-injection 관련 간접 변화로 `재생 가능성`을 계산할 수는 있다. 그러나 다음을 지킨다.

- label은 `재생 중`이 아니라 `재생 가능성 높음`이다.
- 최소 2개 독립 신호가 일정 시간 일치해야 한다.
- confidence와 근거 신호를 정차 상세 화면에 보여준다.
- lamp off를 regeneration 완료로 단정하지 않는다.
- 추정값으로 차량을 자동 제어하거나 운전자에게 무리한 주행을 요구하지 않는다.

Hyundai 설명서는 DPF가 주행 조건에서 soot를 자동 산화하고, 짧은 거리·저속 반복 시 충분히 제거되지 않을 수 있다고 설명한다. 일부 Hyundai diesel 설명서는 DPF lamp가 켜졌을 때 60 km/h 초과 또는 2단 이상 1,250–2,500 rpm을 약 25분 유지하는 안내를 제공한다. 이 수치는 **해당 설명서의 조건**이며 한국형 2017 Tucson의 정확한 절차를 대체하지 않는다. 대상 차량 사용설명서와 계기판 문구를 우선한다.

### 5.4 UI 상태

| 상태 | 큰 label | 보조 정보 | 색상 |
|---|---|---|---|
| normal verified | `DPF 정상` | lamp off, last update | 기본 ink |
| possible regen | `재생 추정` | confidence와 EGT 근거 | accent |
| lamp on | `DPF 확인` | 차량 설명서 확인 | warning amber |
| lamp blink/fault | `배출가스 점검` | 안전한 곳에서 정비 권고 | error red + icon |
| unsupported | `세부 정보 없음` | lamp만 감시 중 | muted |
| stale | `DPF 신호 끊김` | 마지막 수신 시각 | muted + 끊김 icon |

강제 regeneration, DTC clear, actuator test는 일반 운전자 화면에서 제공하지 않는다.

## 6. 취침 모드와 뒷좌석 증폭

### 6.1 제품 동작 기준

Hyundai의 현재 Quiet Mode 문서는 rear-seat speaker를 끄고 front speaker volume을 적절히 조정한다고 설명한다. Hyundai sound 설정은 차량 그림 또는 화살표로 sound position을 조절하고 speed-dependent volume을 제공한다. 2018 Tucson TL 캐나다 설명서도 Up/Down/Left/Right로 Fader/Balance를 조절하고 SDVC를 On/Off하는 UI를 확인해 준다. 다만 대상 한국형 2017 BlueLink head unit/amp가 같은 CAN 제어를 제공하는지는 별도 검증 대상이다.

### 6.2 M-CAN 후보

[`dbc/opendbc/hyundai_2015_mcan.dbc`](../dbc/opendbc/hyundai_2015_mcan.dbc)의 후보는 다음과 같다.

| Message | ID | 주요 신호 | 방향 후보 |
|---|---:|---|---|
| `HU_AMP_E_08` | `0x00F` | `AMP_MainVolumeSet`, `AMP_BalanceSet`, `AMP_FadeSet` | HU→AMP 설정 |
| `AMP_HU_PE_03` | `0x183` | volume/balance/fade state, `AMP_VehicleSpeedamp` | AMP→HU feedback |
| `HU_AMP_E_02` | `0x009` | `AMP_Mute`, `AMP_RearSpMute` | HU→AMP 설정 |
| `AMP_HU_PE_02` | `0x181` | mute/rear speaker mute state | AMP→HU feedback |
| `HU_AMP_E_01` | `0x008` | `AMP_SDVCStep`, `AMP_AutoVolume` | HU→AMP 설정 |
| `AMP_HU_PE_01` | `0x180` | SDVC/auto volume state | AMP→HU feedback |
| `AMP_HU_P_01` | `0x580` | speed adjust/rear mute/auto volume support | capability |
| `AMP_HU_E_06` | `0x085` | max volume/balance/fade step | range feedback |
| `HU_CLU_PE_05` | `0x197` | HU mute/volume status | cluster 표시 |

이 DBC의 관련 signal 대부분은 range가 `[0|0]`이고 value table, message period, alive counter, checksum이 확인되지 않는다. 대상 차량에 해당 M-CAN message가 존재하는지부터 확인한다.

### 6.3 semantic profile

raw CAN 값 대신 다음 semantic profile을 정의한다.

| Profile | rear speaker mute | fader | balance | volume | SDVC |
|---|---|---|---|---|---|
| `OEM` | 변경 없음 | 변경 없음 | 변경 없음 | 변경 없음 | 변경 없음 |
| `QUIET` | 지원 시 on | 미지원 시 front bias | 보존 | calibrated quiet cap 이하 | 보존 |
| `REAR_BOOST` | off | calibrated rear bias | 보존 | 선택적 +1 step, cap 적용 | 보존 |
| `CENTER` | off | center | center | 보존 | 보존 |

`front`, `rear`, `left`, `right`의 raw 증가 방향은 capture로 정한다. DBC field 이름만으로 `+` 방향을 가정하지 않는다. `QUIET`의 quiet cap도 고정 숫자가 아니라 head unit의 max step과 사용자가 정차 상태에서 지정한 상대 cap으로 저장한다.

### 6.4 snapshot과 복원

profile 적용 전 gateway가 다음 OEM 상태 snapshot을 만든다.

- audio source와 source별 volume
- mute / rear speaker mute
- fader / balance
- SDVC / auto volume
- snapshot timestamp와 state revision

profile 해제는 “중앙값”을 쓰는 것이 아니라 snapshot을 복원한다. 적용 중 사용자가 head unit knob/button으로 값을 바꾸면 feedback이 요청 상태와 달라진다. 이때 CANView가 값을 계속 덮어쓰지 않고 다음처럼 처리한다.

1. 외부 변경 감지
2. 자동 profile write 중지
3. 새 OEM 상태를 현재값으로 채택
4. UI에 `차량에서 직접 변경됨` 표시
5. 사용자가 다시 profile을 눌렀을 때만 새 snapshot으로 재적용

### 6.5 송신 검증

1. head unit off/on과 amp 존재 확인
2. volume knob 1 step up/down capture
3. fader와 balance를 각 방향 1 step씩 capture
4. rear speaker mute가 실제로 제공되는지 확인
5. message source MAC이 아니라 CAN sender와 주기 확인
6. alive/checksum/rolling counter 존재 확인
7. setting frame과 feedback frame timing 대조
8. 기존 HU frame과 arbitration 충돌 없이 button event를 재현할 방법 결정
9. bench speaker에서 짧은 1회 명령과 복원 검증

주기 frame을 계속 spoof해야만 동작한다면 owner node와 arbitration을 완전히 이해하기 전까지 기능을 출시하지 않는다.

## 7. 속도·주변 소음 기반 자동 음량

### 7.1 hardware 선택

Waveshare 공식 FAQ는 이 보드에 ES8311 audio codec, speaker, SMD microphone이 있다고 명시한다. 공식 Arduino ES8311 예제의 capture path는 다음과 같다.

| I²S 기능 | GPIO | 방향 |
|---|---:|---|
| MCLK | 12 | ESP32→ES8311 |
| BCLK | 13 | ESP32→ES8311 |
| playback DIN | 14 | ESP32→ES8311 |
| LRCLK | 15 | ESP32→ES8311 |
| recording DOUT | 16 | ES8311→ESP32 |

따라서 1차 prototype은 온보드 SMD microphone을 사용한다. 이 핀은 이미 보드 내부 audio에 연결돼 있으므로 외부 microphone을 같은 GPIO에 병렬 연결하지 않는다.

다음 중 하나면 별도 microphone hardware를 검토한다.

- Controller 설치 위치에서 HVAC 송풍음이 실제 road noise보다 6 dB 이상 지배
- speaker 재생음에 따라 추정 noise가 volume과 함께 계속 상승
- 손가락·case·mount가 microphone port를 가림
- stationary repeatability가 ±2 dB보다 나쁨
- 목표 대역 100 Hz–4 kHz의 SNR이 부족

별도 node는 automotive 온도 범위를 확인한 I²S/PDM MEMS microphone, clean 3.3 V, 짧고 차폐된 배선, headliner/상부 console 후보 위치를 사용한다. raw cabin audio는 저장하거나 ESP-NOW로 전송하지 않고 `noise_level`, `confidence`, `clipping`, `sample_age`만 전송한다.

### 7.2 signal processing

권장 pipeline은 다음과 같다.

```text
16 kHz / 16-bit mono capture
 -> DC blocker
 -> 100 Hz high-pass + 4 kHz low-pass
 -> A-weighting 근사 biquad
 -> 125 ms RMS
 -> 1 s Leq
 -> 5 s median / outlier rejection
 -> relative cabin noise + confidence
```

FFT 화면용 분석은 같은 16 kHz capture에서 별도 저우선순위 task로 계산한다.

```text
1024 sample frame (64 ms)
 -> DC blocker
 -> Hann window
 -> 1024-point real FFT (15.625 Hz/bin)
 -> magnitude squared
 -> 50 Hz–8 kHz의 23개 log-frequency display bin으로 energy 합산
 -> peak bin의 인접 3점 포물선 보간
 -> attack 120 ms / release 600 ms smoothing
 -> UI 8 Hz 갱신
```

`canview_ui_model_t`에는 `fft_bins[23]`, `fft_peak_hz`, `fft_peak_tenth_db`만 전달한다. raw PCM과 complex FFT buffer는 audio task 내부에만 두고 LVGL task나 ESP-NOW payload로 복사하지 않는다. FFT bin은 화면 높이에 맞춘 0–100 상대값이며, peak level도 calibration 전에는 상대 dB다. 8 kHz는 16 kHz sampling의 Nyquist 경계이므로 마지막 display bin은 8 kHz 미만 유효 bin만 합산한다. clipping, sample underrun, microphone mute 시에는 이전 spectrum을 유지하지 말고 invalid 상태로 전환한다.

정식 dBA 계측기 calibration 전에는 `dBA`가 아니라 `상대 소음 dB`로 취급한다. 94 dB calibrator 또는 비교 계측기로 offset을 맞춘 경우에만 dBA label을 쓴다.

음성·음악 오염을 완전히 분리할 수 없으므로 다음 guard를 둔다.

- volume 변경 후 2초간 microphone correction freeze
- clipping이면 sample 무효
- 높은 crest factor/voice modulation 구간은 confidence 감소
- door/window/HVAC 상태를 얻을 수 있으면 model feature로만 사용
- OEM audio가 mute일 때 road-noise baseline을 우선 학습
- microphone correction이 speed feed-forward를 2 step 넘게 바꾸지 못하게 제한

### 7.3 control law

차속은 자동 보정을 허용하는 최소 조건이고, 실제 step 판단은 FFT focus band가 차속별 학습 baseline보다 지속적으로 큰지로 결정한다. 기본 `표준` 대역은 160–1,250 Hz다.

```text
raise = speed >= 30 km/h
        AND smoothed_peak inside selected_band
        AND band_excess >= raise_threshold
        for attack_seconds

lower = speed < 30 km/h
        OR smoothed_peak outside selected_band
        OR band_excess <= lower_threshold
        for release_seconds
```

- 기본 민감도: 올림 +5.0 dB, 내림 +2.5 dB의 히스테리시스
- 기본 반응: 올림 5초, 내림 12초
- 변경 속도: 3초에 최대 1 volume step
- 자동 step 뒤 2초간 microphone feedback freeze
- 기본 `user_max_offset`: +4 step
- 수동 volume 조작: offset 0 복원 후 60초 pause
- navigation/parking/safety warning: OEM priority를 방해하지 않음
- reverse: 새 volume 상승 금지

짧은 peak 이탈은 누적 evidence를 즉시 0으로 만들지 않고 반대 방향으로 서서히 감쇠한다. head unit의 기존 SDVC가 활성화돼 있으면 이중 보상이 생기므로 `OEM SDVC`와 `CANView adaptive`를 동시에 켜지 않는다. preset과 정수 상태기계는 [자동 제어 로직](automation-control.md)을 따른다.

### 7.4 calibration

정차 설정 화면에서 다음 순서를 안내한다.

1. engine off, cabin quiet 10초 noise floor
2. engine idle, HVAC off 10초
3. engine idle, 평소 HVAC 10초
4. 안전한 도로에서 40/80/110 km/h 각각 30초
5. media mute sample과 평소 volume sample 비교
6. 사용자가 각 속도에서 원하는 volume offset 확인

profile은 microphone raw sample이 아니라 curve와 통계량만 저장한다. calibration이 없으면 speed-only로 동작하고 UI에 `소음 보정 학습 전`을 표시한다.

## 8. SPORT mode 자동 전환·복귀

### 8.1 후보 신호

| Message | ID | Signal | 용도 |
|---|---:|---|---|
| `CLU13` | `0x50C` | `CF_Clu_DrivingModeSwi` | drive mode switch/state 후보 |
| `CLU13` | `0x50C` | `CF_Clu_ActiveEcoSW` | ECO 상태 후보 |
| `CLU_HU_PE_01` | M-CAN `0x1DF` | `C_DrivingModeState`, `C_DrivingModeOn` | HU/cluster feedback 후보 |
| `ESP12` | 차량 profile 확인 | `LONG_ACCEL`, status/diag | 종가속도·품질 |
| `WHL_SPD11` | `0x386` | wheel speeds | 차량 속도·slip plausibility |
| `EMS12` | `0x329` | `PV_AV_CAN` | accelerator position 후보 |
| `LVR11/12` | `0x368/0x367` | gear/lever state | D/R/N/P inhibit |
| `TCS15` 등 | profile 확인 | ABS/TCS/ESC lamp/state | safety inhibit |

2017 Tucson manual 계열 자료는 DRIVE MODE button으로 `NORMAL ↔ SPORT`를 선택하고, engine restart 시 SPORT가 NORMAL로 돌아가며, cluster fault가 있으면 NORMAL에 머물고 SPORT로 전환되지 않을 수 있다고 설명한다. 실제 한국형 2.0 diesel 4WD의 mode 순서와 feedback raw code는 물리 버튼 capture로 확정한다.

`CF_Clu_DrivingModeSwi`가 switch request인지 current state인지 이름만으로 결정하지 않는다.

### 8.2 release 단계

| 단계 | 기능 | 차량 TX |
|---|---|---|
| R0 | speed/acceleration과 현재 mode 기록 | 없음 |
| R1 | `SPORT 권장` 알림, 사용자가 물리 버튼 조작 | 없음 |
| R2 | 정차 bench에서 button pulse 재현 | bench만 |
| R3 | 폐쇄 시험장 opt-in automation | 제한 허용 |
| R4 | 장기 검증 후 일반 사용 opt-in | 조건부 |

기본 설정은 `DISABLED`다. firmware update로 자동 활성화하지 않는다.

### 8.3 상태기계

```text
DISABLED -> MONITOR_ONLY -> ARMED
                           |
                 sustained demand
                           v
                    ENTER_PENDING
                      | feedback SPORT
                      v
                    SPORT_ACTIVE
                      | speed below exit threshold
                      v
                    EXIT_PENDING
                      | feedback captured previous mode
                      v
                       ARMED

Any active state -- physical user change --> MANUAL_HOLD
Any active state -- stale/fault/link loss --> INHIBITED -> MONITOR_ONLY
```

`previous mode`는 SPORT 진입 직전 확인된 `NORMAL/ECO/COMFORT/SMART` 중 하나다. `MANUAL_HOLD`는 ignition cycle 또는 사용자의 명시적 `자동 재개`까지 유지한다. CANView가 사용자의 물리 버튼 선택과 싸우지 않게 하기 위해서다.

### 8.4 초기 threshold

entry는 한 sample spike가 아니라 지속 조건이다.

```text
armed
AND forward gear confirmed
AND no brake / ABS / TCS / ESC intervention
AND all safety signals fresh
AND (
      speed >= configured 60/70/80 km/h for 2.5 s
      OR (
          speed >= 35 km/h
          AND filtered_longitudinal_accel >= 1.4 m/s² for 0.8 s
      )
    )
=> ENTER_PENDING
```

exit 조건은 hysteresis를 둔다.

```text
SPORT_ACTIVE for at least 15 s
AND speed <= configured_entry_speed - 15 km/h
AND filtered_longitudinal_accel <= 0.35 m/s²
for 8 s
=> restore captured previous mode
```

threshold는 calibration 시작값이며 실차 log로 조정한다. 진입과 복귀 사이의 15 km/h 히스테리시스와 최소 SPORT 유지시간이 threshold 부근의 반복 전환을 막는다. 값 변경은 정차 설정에서만 가능하고 gateway가 60–80 km/h 범위를 강제한다.

### 8.5 command 방법

목표 ECU mode 값을 직접 쓰지 않는다. 실제 DRIVE MODE switch frame이 검증되면 물리 버튼 한 번에 해당하는 bounded event만 Communicator command로 제공한다.

1. 현재 mode와 mode cycle 확인
2. 한 번의 button pulse 송신
3. 최대 1초 feedback 대기
4. 원하는 mode가 아니면 실패
5. 자동으로 여러 번 빠르게 cycle하지 않음
6. 최소 2초 command 간격

mode cycle에 ECO가 포함돼 여러 pulse가 필요하다면 각 pulse 뒤 feedback을 확인하고 vehicle profile이 허용한 최대 pulse 수를 넘지 않는다. state가 unknown이면 송신하지 않는다.

### 8.6 inhibit

다음 하나라도 참이면 자동 전환을 금지한다.

- reverse, park, neutral, gear unknown
- brake active 또는 급감속
- ABS/TCS/ESC active·fault·signal stale
- cluster/drive mode fault
- wheel speed plausibility fault
- ESP-NOW degraded 또는 control lease 없음
- Communicator reboot/state revision mismatch
- CAN bus error passive/bus-off/overflow sustained
- 현재 mode unknown
- 사용자의 physical mode 조작 감지
- 차량 profile이 exact target으로 승인되지 않음

UI는 단순히 toggle을 회색으로 만들지 않고 `브레이크 입력`, `주행 모드 신호 끊김`, `검증 전`처럼 한 가지 최우선 inhibit 이유를 설명한다.

## 9. CAN 기반 자동 밝기

`C_TailLampActivity`, `CF_Gway_HeadLampLow`, `CF_Gway_LightSwState`, `CF_Clu_RheostatLevel`을 후보로 사용한다. 실제 tail/low-beam 활성 신호를 1순위로 하고 rheostat를 야간 밝기 범위에 매핑한다. 점등은 500 ms, 소등은 1.5초 확인하며 1.2초 ramp로 PWM을 바꾼다. 신호 stale에서는 갑자기 밝아지지 않고 마지막 유효 밝기를 유지한다. 상세값과 구현은 [자동 제어 로직](automation-control.md)에 있다.

## 10. 공통 UI 요구사항

- 주행 중 한 화면에서 핵심 상태를 2초 이내 읽을 수 있어야 한다.
- 320×480에서 주요 touch target은 76×76 px 이상을 목표로 한다.
- 움직이는 장식, auto-scrolling text, 실시간 그래프 남용을 피한다.
- 숫자는 tabular figure를 사용하고 단위는 숫자보다 한 단계 작게 둔다.
- `0`과 `미수신`을 시각적으로 구분한다.
- 추정값에는 `추정` label과 quality를 함께 보낸다.
- warning은 색상뿐 아니라 icon과 문구를 함께 쓴다.
- audio와 automation 설정의 multi-step 편집은 speed 0에서만 연다.
- touch 후 250 ms 안에 pressed/pending feedback을 표시한다.

상세 화면 구성과 LVGL mapping은 [UI/UX 설계](ui-design.md)를 따른다.

## 11. 우선순위와 구현 backlog

### P0 — read-only 기반

- ESP-NOW secure link와 stale 상태
- 3개 bus 상태
- speed/RPM/wheel speed
- TPMS pressure raw와 조명/rheostat raw capture
- DPF lamp raw capture
- 4WD `0x428–0x42A` raw capture·graph
- drive mode 물리 버튼 capture
- audio M-CAN 존재 여부 capture

### P1 — 검증된 표시

- 4WD mode enum과 coupling candidate
- DPF lamp enum
- SPORT current mode 표시
- onboard microphone relative noise meter
- signal quality/catalog revision
- CAN 자동 밝기 monitor와 PWM ramp

### P2 — 안전한 audio 기능

- amp capability/range 확인
- state snapshot/restore
- QUIET/REAR_BOOST bench 구현
- manual override detection
- speed-only adaptive volume

### P3 — calibration 기능

- 4WD rear coupling estimate
- microphone correction
- DPF standard diagnostic PID
- 정차 calibration wizard

### P4 — 차량 동작 automation

- SPORT monitor-only rule 평가
- closed-course button pulse
- opt-in 자동 진입/복귀
- proprietary DPF DID는 scan-tool 검증된 항목만

## 12. test matrix

| 기능 | 정상 | 경계 | 장애 |
|---|---|---|---|
| 4WD | AUTO/LOCK/가감속 | duty 0/max, 저속 | stale, conflicting bits, fault |
| TPMS | 네 바퀴 pressure | low warning, invalid raw | stale, wheel order mismatch |
| DPF | lamp off/on | raw 0–3 | missing message, diagnostic NACK |
| Quiet | apply/restore | volume cap, source change | external knob, amp offline |
| Rear boost | fader+volume | max/min step | rear mute unsupported |
| Auto volume | 0/40/80/110 km/h | hysteresis crossing | mic clip, music contamination |
| SPORT | entry/exit | threshold hover | brake, reverse, ESC, link loss |
| 자동 밝기 | lamp on/off, rheostat | 500 ms/1.5 s debounce | 조명 신호 stale |
| 공통 | ignition cycle | session/sequence wrap | Communicator/Controller reboot |

모든 제어 test는 “명령을 보냈다”가 아니라 feedback state와 물리 결과까지 확인해야 통과다.

## 13. 출처

- [commaai/opendbc 고정 commit](https://github.com/commaai/opendbc/tree/3e92d112129507debe45364891954db70238997a)
- [Hyundai 2017 Tucson owner manual mirror — drive mode와 4WD](https://www.manualslib.com/manual/1233073/Hyundai-Tucson-2017.html)
- [Hyundai Canada 2018 Tucson TL manual — fader/balance와 SDVC](https://www.hyundaicanada.com/-/media/hyundai/feature/ownerssection/manuals/english/2018/tuscon/tl-can-eng-4.pdf)
- [Hyundai Quiet Mode](https://ownersmanual.hyundai.com/ivi/ccNC/AVNT/KOR/English/Quietmode.html)
- [Hyundai sound position·speed-dependent volume](https://ownersmanual.hyundai.com/ivi/DA_GEN2_V/AV/AUS/English/010_Settings_sound.html)
- [Hyundai DPF 설명](https://ownersmanual.hyundai.com/full_webhelp/US4/2025/en_GN/id75497873a8b.html)
- [Waveshare ESP32-S3-Touch-LCD-3.5 FAQ — SMD microphone](https://docs.waveshare.com/ESP32-S3-Touch-LCD-3.5/FAQ)
- [Waveshare 고정 예제 commit](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.5/tree/283ec84c566c096f8c30493b93dcd4b0bb608de7)
- [Google Design for Driving — interaction principles](https://developers.google.com/cars/design/design-foundations/interaction-principles)
- [Google Design for Driving — sizing](https://developers.google.com/cars/design/automotive-os/design-system/sizing)
