# CANView 운전자 UI 설계

## 1. 설계 목표

CANView UI는 2017 Tucson TL의 순정 계기판을 대체하지 않는다. 일반 운전자가 차량 상태를 짧게 확인하고, 정차 상태에서 추가 기능을 설정하는 보조 화면이다. 화면은 `ESP32-S3-Touch-LCD-3.5`의 320×480 세로 해상도를 기준으로 한다.

핵심 목표는 다음과 같다.

1. **주행 중 한눈에 읽기**: 원형 속도·RPM 계기와 4WD·DPF처럼 현재 판단에 필요한 값만 크게 보인다.
2. **상태와 제어 분리**: 주행, 소리, FFT, 자동화, 설정을 독립 화면으로 나눈다.
3. **프리미엄 가전의 차분함**: 현대 표준형 5W 내비게이션처럼 흑색·딥네이비 표면에 선명한 청색 활성선을 제한적으로 사용한다.
4. **운전자 화면의 절제**: DBC 후보명, raw code, 판단 경로, 무선 진단값은 일반 화면에서 감추고 개발 로그와 서비스 진단에 보존한다.
5. **실수 방지**: 주행 중 세부 조정, 미검증 제어, 연결이 불안정한 상태의 명령을 잠근다.

## 2. Hallmark 적용 판단

Hallmark는 이 프로젝트의 **시각적 품질 프레임**으로 적절하다. 빈번한 카드 반복, 과한 둥근 모서리, 무의미한 그라데이션, 작은 터치 표적, hover 의존과 같은 일반적인 UI 결함을 줄이고 토큰·상태·반응형 계약을 일관되게 만드는 데 사용했다.

다만 Hallmark는 차량 안전 규격이 아니다. 다음 영역에는 적용 권한을 주지 않는다.

- 주행 중 허용할 조작과 잠금 조건
- DBC 신호의 진위와 차량별 의미
- CAN 송신 allow-list, lease, timeout, snapshot 복원
- 경고 문구와 정비 판단

이 영역은 Hyundai 사용설명서, Google Design for Driving의 glanceability·touch 지침, 실차 검증 결과, [`feature-design.md`](feature-design.md)와 [`esp-now-protocol.md`](esp-now-protocol.md)를 우선한다.

Hallmark 기준으로 선택한 구성은 수치를 화면의 주인공으로 두는 `Stat-Led` macrostructure, `N9` 상단 상태, `C4` 상시 노출 하단 탭이다. 프로젝트 전용 테마는 현대 표준형 5W 내비게이션의 색 관계를 참고하며 분위기 축은 “automotive clarity, appliance calm, technical precision”이다.

## 3. 조사·벤치마크

| 근거 | 차용한 점 | 차용하지 않은 점 |
|---|---|---|
| [Google Design for Driving interaction principles](https://developers.google.com/cars/design/design-foundations/interaction-principles) | 약 2초 안의 glanceability, 즉각적인 피드백, 단순하고 중단 가능한 흐름 | Android Automotive의 화면 구조를 그대로 복제하지 않음 |
| [Google Automotive OS sizing](https://developers.google.com/cars/design/automotive-os/design-system/sizing) | 최소 76×76 dp touch target을 주요 내비게이션과 profile에 반영 | 320×480의 76 px가 실제 76 dp와 같다고 단정하지 않음 |
| [현대자동차 내비게이션 업데이트 FAQ의 표준형 5W 화면](https://update.hyundai.com/KR/KO/cs/faq/2351) | 거의 검은 바탕, 딥네이비 선택면, 밝은 청색 활성선, 흰색 본문과 회색 보조 정보의 관계 | 순정 메뉴 구조·아이콘·그래픽을 그대로 복제하지 않음 |
| [Hyundai Quiet Mode](https://ownersmanual.hyundai.com/ivi/ccNC/AVNT/KOR/English/Quietmode.html) | 뒤 스피커를 끄고 앞 음량을 조정하는 사용자 개념 | 최신 ccNC의 그래픽을 복제하지 않음 |
| [Hyundai sound settings](https://ownersmanual.hyundai.com/ivi/DA_GEN2_V/AV/AUS/English/010_Settings_sound.html) | speed-dependent volume 개념 | 대상 차량에서 CAN 제어가 검증됐다고 간주하지 않음 |
| [2018 Tucson TL 설명서](https://www.hyundaicanada.com/-/media/hyundai/feature/ownerssection/manuals/english/2018/tuscon/tl-can-eng-4.pdf) | fader/balance의 상·하·좌·우 조작과 SDVC를 같은 세대 참고로 사용 | 한국형 2017 BlueLink와 동일 사양이라고 단정하지 않음 |
| [RealDash 공식 앱 설명](https://play.google.com/store/apps/details?id=com.napko.RealDash) | 상세 진단 화면의 사용자 구성·알람 개념만 참고 | 주행 홈에 작은 gauge를 다수 배치하는 방식은 배제 |
| [LVGL 8.4 meter](https://lvgl.io/docs/open/8.4/widgets/extra/meter), [button matrix](https://lvgl.io/docs/open/8.4/widgets/core/btnmatrix), [tabview](https://lvgl.io/docs/open/8.4/widgets/extra/tabview) | arc/bar/button과 상태 style을 임베디드 primitive로 구현 | 기본 widget 외형을 그대로 사용하지 않음 |

벤치마크 결론은 “최신 클러스터의 정보 우선순위 + 순정 오디오의 익숙한 기능 모델 + 프리미엄 가전의 낮은 시각 소음”이다. 계기판처럼 보이게 만드는 장식보다 읽는 순서와 상태의 신뢰도를 우선한다.

## 4. 정보 구조

```text
상단 36 px: CANView / 활성 CAN bus / ESP-NOW 상태
본문 368 px
├─ 주행: 원형 속도·RPM / DPF·주행 mode / 4WD 바퀴 gauge·공기압
├─ 소리: 취침·뒷좌석+ / 음량 / 소음 보정
├─ FFT: 원형 속도·RPM / spectrum / peak 주파수·레벨
├─ 자동: SPORT 자동화 / 현재 mode / 사용 여부
└─ 설정: 화면 / 주행 소음 보정 / SPORT 자동 / 단위
하단 76 px: 주행 / 소리 / FFT / 자동 / 설정
```

주행 중 기본 화면은 항상 `주행`이다. 알림이 발생해도 앱이 임의로 탭을 바꾸지 않고, 상단 상태 또는 해당 카드의 색·문구만 바꾼다. 치명적인 링크 단절은 현재 값을 `stale`로 바꾸되 마지막 값을 지워 운전자가 갑작스러운 숫자 변화를 실제 상태 변화로 오해하지 않게 한다.

### 4.1 주행 화면

읽기 순서는 속도·RPM → DPF → 주행 mode·4WD다.

- 속도와 RPM은 동일한 원형 계기 문법을 쓰되 속도 숫자를 조금 더 크게 둔다.
- 원형 arc는 각각 표시 범위 대비 현재값을 나타내며 정밀 판독은 중앙 숫자로 한다.
- 4WD는 네 바퀴 자체를 세로 gauge로 사용하고, 각 바퀴 안에는 TPMS 공기압을 표시한다.
- gauge 입력은 품질값이 있는 바퀴별 구동 지수다. 공개 DBC만으로 실제 바퀴별 torque를 만들 수 없으므로 검증 전에는 rear clutch duty를 네 바퀴에 임의 분배하지 않는다.
- TPMS와 4WD 데이터는 freshness를 따로 관리하며, 둘 중 하나가 unavailable이어도 다른 값은 계속 표시할 수 있다.
- DBC 검증 수준과 raw DPF code는 서비스 진단에 기록하되 일반 운전자 화면에는 노출하지 않는다.
- 경고가 없을 때는 `DPF 정상`, 경고가 있을 때만 `DPF 확인`으로 바뀐다.

### 4.2 오디오 화면

상단의 `취침`, `뒷좌석 +`는 상호 배타적인 76 px 이상 profile 버튼이다. 임의 sound position 조작은 제공하지 않고, 중앙에는 현재 음량과 `−`/`+` 한 단계 조절만 둔다.

- `취침`: 가능하면 rear speaker mute, 불가능하면 front bias와 quiet volume cap
- `뒷좌석 +`: rear mute 해제, 검증된 방향으로 fader 2 step, 사용자 cap 안에서 선택적 volume +1
- profile 해제: 중앙값이 아니라 적용 전 OEM snapshot 복원
- 차량 head unit에서 직접 조작 감지: 자동 쓰기 중단 후 새 상태를 채택
- `주행 소음 보정`: 적용 중인 volume offset만 짧게 표시

### 4.3 자동화 화면

SPORT 자동화는 일반 사용자용 상태 화면으로 표현한다. 화면에는 `SPORT 자동`, 현재 mode, 복귀할 이전 mode, `사용`/`끔`만 남긴다. 상세 decision path, encryption, TX lock, RTT는 서비스 진단으로 이동한다. 진입 시 직전 mode를 snapshot하고 종료 시 그 mode를 복원하며, 사용자의 물리 버튼 조작이 항상 우선한다.

### 4.4 FFT 화면

실내 마이크 또는 별도 microphone board에서 만든 FFT bin을 표시한다. 상단에는 차속과 RPM을 작은 원형 계기로 함께 유지해 소음 peak가 주행 조건과 동시에 읽히게 한다.

- 23개 log-frequency bin을 50 Hz–8 kHz 범위의 막대로 표시한다.
- 가장 큰 성분은 `PEAK` 주파수와 `LEVEL` dB 값으로 분리해 표시한다.
- 화면은 분석 결과만 표시하며 운행 중 calibration이나 microphone gain을 조정하지 않는다.
- dB 값은 microphone·ADC·window·reference calibration 전에는 상대 레벨이며, 절대 SPL로 표기하려면 별도 보정 절차가 필요하다.

### 4.5 설정 화면

설정은 정차 상태에서 세로 scroll되는 네 그룹으로 구성한다.

- `DISPLAY`: 화면 밝기, CAN 자동 밝기
- `ROAD NOISE`: 소음 보정, 주파수 대역, 민감도, 반응, 최대 보정
- `SPORT AUTO`: 자동 전환, 진입 속도, 급가속 감지
- `UNITS`: 속도 단위

자동 밝기는 외부 조도 센서가 아니라 차량 등화와 rheostat CAN 값만 사용한다. pairing, CAN 송신 허용, DBC 선택, raw Hz/dB threshold 같은 엔지니어링 항목은 운전자 설정 화면에 두지 않는다. 각 preset의 실제 수치와 히스테리시스는 [자동 제어 로직](automation-control.md)에 고정한다.

## 5. 시각 시스템

정본 token은 루트의 [`tokens.css`](../tokens.css)다. LVGL의 sRGB 근사값은 [`canview_theme.h`](../ui/lvgl/canview_theme.h)에 있다.

| 역할 | 토큰 | 의도 |
|---|---|---|
| 바탕 | `--color-paper` | 청색기가 약한 짙은 흑연색 |
| 카드 | `--color-paper-2` | 바탕과 작은 명도 차이만 둔 매트 표면 |
| 구분 | `--color-rule`, `--color-rule-2` | 그림자 대신 1 px 경계 사용 |
| 강조 | `--color-accent` | 현대 5W 화면을 참고한 선명한 시안블루. 연결·활성·현재값에만 사용 |
| 주의 | `--color-warning` | DPF 확인 등 행동이 필요한 상태 |
| 오류 | `--color-error` | bus-off, 제어 실패처럼 실제 오류에만 사용 |

색에만 의미를 맡기지 않고 icon·label·quality tag를 함께 사용한다. 광택, 유리 효과, 배경 bloom, 장식용 gradient는 사용하지 않는다. radius는 8/12/18 px 세 단계이며 모든 작은 요소를 pill로 만들지 않는다.

폰트는 숫자·영문 display에 `Space Grotesk`, 한글 본문에 `IBM Plex Sans KR`을 사용한다. LVGL 빌드에서는 실제 사용 글리프만 변환한 한글 폰트와 별도 숫자 폰트를 주입한다.

## 6. 320×480 레이아웃 명세

| 영역 | 위치·크기 | 비고 |
|---|---|---|
| app | `0,0,320,480` | scroll 없음 |
| top status | 높이 36 | 12 px 좌우 padding |
| content | `y=36`, 높이 368 | 12 px 좌우, 8 px 상하 padding |
| bottom nav | `y=404`, 높이 76 | 탭 5개, 각 64×76 |
| 기본 gap | 8 | 4 px spacing 계열 |
| primary touch | 최소 76×76 | profile·주요 toggle |
| bottom navigation | 64×76 | 5개 고정 탭; 실제 장착 거리에서 별도 검증 |
| secondary touch | 최소 48×48 | 음량 step, toggle, 단위 선택 |

320×480은 실제 hardware의 정본 viewport다. 브라우저 prototype은 375, 414, 768 px에서도 320 px app을 중앙 정렬해 화면 비율과 터치 좌표가 바뀌지 않게 한다. 펌웨어에서는 회전·해상도를 고정하고 display flush buffer만 조정한다.

Google의 76 dp 지침을 주요 profile과 toggle에는 76 logical px로 먼저 옮겼다. 하단 5개 탭은 화면 폭 때문에 64×76 px이며 탭 전체 면적이 hit target이다. 실제 패널 크기·운전자 거리·장갑 사용 환경에서 측정해 부족하면 설정을 상단 진입점으로 옮겨 탭 수를 줄인다.

## 7. 구성요소와 LVGL 매핑

| UI 구성요소 | Web prototype | LVGL 8.4 | 상태 입력 |
|---|---|---|---|
| 상단 연결 상태 | flex status row | `lv_obj` + `lv_label` | bus count, RSSI, stale |
| 속도/RPM | SVG circular gauge | `lv_arc` + `lv_label` | vehicle speed, RPM |
| 4WD·TPMS | 네 바퀴 gauge + pressure | four `lv_bar` + pressure labels | wheel drive index, pressure, 독립 quality |
| DPF 상태 | semantic status row | card + labels | lamp raw/decoded quality |
| audio profile | 76 px buttons | checkable `lv_btn` | desired + feedback revision |
| 음량 | large number + step buttons | `lv_label` + two `lv_btn` | OEM volume level |
| FFT | SVG bar spectrum | `lv_chart` bar series | 23 bins, peak Hz/dB |
| SPORT 자동 | mode card/dial | `lv_arc`, button, labels | current/previous mode, enabled |
| 설정 | scroll group + range/toggle/value button | `lv_slider`, `lv_btn` | brightness, CAN auto, noise presets, SPORT threshold, units |
| bottom nav | fixed tabs | five `lv_btn` | current screen |

[`canview_ui.c`](../ui/lvgl/canview_ui.c)는 단일 320×480 인스턴스를 제공한다. 화면은 `canview_ui_model_t`만 받고 사용자 입력은 `CANVIEW_UI_CMD_*` 콜백으로만 내보낸다. callback에서 raw CAN frame을 만들지 않고 제어 정책 queue에 복사해야 한다.

## 8. 상호작용·상태 계약

모든 조작 요소는 다음 상태를 가져야 한다.

| 상태 | 시각·동작 |
|---|---|
| default | 중립 카드와 명시적 label |
| hover | 브라우저·마우스 테스트에서만 경계 강조; 기능 의존 금지 |
| focus-visible | 2 px accent outline |
| active/pressed | 1 px 눌림 이동과 accent wash |
| selected | accent line + wash + `aria-pressed`/LVGL checked state |
| disabled | opacity 감소, pointer/click 처리 차단, 잠금 이유 제공 |
| loading | 짧은 opacity pulse, 버튼 label을 `적용 중`으로 변경 |
| error | red wash와 구체적인 실패 문구 |
| success | accent 상태와 feedback으로 확인, toast 남발 금지 |

명령 UI 상태는 `요청됨`, `Communicator 수락`, `실행 중`, `feedback 확인`, `복원됨`, `실패`를 구분한다. 버튼을 누른 즉시 차량 상태가 바뀐 것처럼 표시하지 않는다. 낙관적 pressed feedback은 250 ms 안에 제공하되, selected 상태는 Communicator 결과와 feedback revision으로 확정한다.

### 8.1 주행 중 제한

| 기능 | 정차 | 주행 | 근거 |
|---|---|---|---|
| 화면 탭 전환 | 허용 | 허용, 한 번의 터치 | 짧은 상태 조회 |
| 취침/뒷좌석 profile | 허용 | 검증 후 한 번의 터치만 고려 | 즉시 취소 가능해야 함 |
| 음량 한 단계 조절 | 허용 | 검증 후 허용 | 한 번의 터치와 즉각 취소 |
| 소음 보정 on/off | 허용 | 상태 조회만 권장 | calibration 변경은 정차 |
| FFT 조회 | 허용 | 허용 | 조작 없는 상태 조회 |
| SPORT 감시 on/off | 허용 | off는 허용, on은 정차 | 운전자 의도 우선 |
| 밝기·자동화 세부 설정·단위 | 허용 | 금지 | 반복 조작·시선 분산 |

## 9. 화면 prototype

아래 이미지는 실제 CAN 값을 읽은 화면이 아니라 레이아웃과 상태 언어를 검토하기 위한 **데모 데이터 prototype**이다.

### 주행 상태

![CANView 주행 화면 prototype](images/ui-drive.png)

### 소리 제어

![CANView 오디오 화면 prototype](images/ui-audio.png)

### 소리 FFT

![CANView FFT 화면 prototype](images/ui-fft.png)

### SPORT 자동화

![CANView 자동화 화면 prototype](images/ui-automation.png)

### 설정

![CANView 설정 화면 prototype](images/ui-settings.png)

![CANView 소음·SPORT 설정 prototype](images/ui-settings-automation.png)

브라우저에서 [`ui/prototype/index.html`](../ui/prototype/index.html)을 열고 query parameter를 바꾸면 각 화면을 확인할 수 있다.

```text
index.html?screen=drive
index.html?screen=audio
index.html?screen=fft
index.html?screen=automation
index.html?screen=settings
index.html?screen=settings&section=sport
```

## 10. 검증 체크리스트

### 시각·조작성

- 320×480에서 잘림과 수평 scroll이 없어야 한다.
- 375/414/768 px browser viewport에서도 app 내부 좌표가 변하지 않아야 한다.
- 주요 touch target은 76 px, 하단 탭은 64×76 px, 보조 표적은 48×48 px 이상이다.
- 한글·숫자 baseline과 단위 위치를 실제 LVGL font에서 재검증한다.
- 100%, 50%, 0% coupling과 0/6500 rpm에서도 넘침이 없어야 한다.
- 23개 FFT bin이 320 px 화면에서 서로 붙지 않고 peak 값과 함께 보여야 한다.
- 네 바퀴 pressure 3자리와 `—`가 gauge 안에서 잘리지 않아야 한다.
- 설정 scroll 중 하단 navigation이 움직이지 않아야 한다.
- 색각 이상 simulation에서도 label만으로 상태를 구분할 수 있어야 한다.
- 야간 최소 밝기에서 accent가 번지지 않고 muted text가 읽혀야 한다.

### 상태·안전

- ESP-NOW heartbeat 만료 후 500 ms 안에 stale 표시와 제어 잠금이 보여야 한다.
- 세 CAN bus 중 하나만 끊겨도 해당 신호의 source bus와 age를 기준으로 stale 처리해야 한다.
- profile 적용 중 head unit 수동 변경이 생기면 반복 송신을 중단해야 한다.
- 거부·timeout·부분 적용마다 복원 결과를 별도로 표시해야 한다.
- `four_wd_quality != VERIFIED`이면 `토크 배분`이라는 확정 표현을 쓰지 않는다.
- DPF lamp raw code의 value table이 확정되기 전에는 raw와 사용자 label을 함께 기록한다.
- SPORT 실제 송신 기능은 monitor-only 검증과 별도 release gate를 통과해야 한다.
- SPORT 해제는 `NORMAL` 고정값이 아니라 진입 직전 mode feedback으로 끝나야 한다.

## 11. 구현 산출물

- 디자인 token: [`tokens.css`](../tokens.css)
- 상호작용 prototype: [`ui/prototype/`](../ui/prototype/)
- LVGL 8.4 UI: [`ui/lvgl/`](../ui/lvgl/)
- 기능·CAN 신호 설계: [`feature-design.md`](feature-design.md)
- ESP-NOW protocol: [`esp-now-protocol.md`](esp-now-protocol.md)

다음 디자인 단계는 새 화면을 늘리는 작업이 아니라 실제 3.5인치 패널에서 font rendering, touch hit area, 야간 밝기, 설치 각도를 검증하는 것이다. 그 결과로 먼저 spacing과 typography token을 보정하고, 정보 구조는 실차 capture에서 새로운 확정 신호가 생길 때만 확장한다.
