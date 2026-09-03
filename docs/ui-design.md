# CANView 운전자 UI 설계

## 1. 설계 목표

CANView UI는 2017 Tucson TL의 순정 계기판을 대체하지 않는다. 일반 운전자가 차량 상태를 짧게 확인하고, 정차 상태에서 추가 기능을 설정하는 보조 화면이다. 화면은 `ESP32-S3-Touch-LCD-3.5`의 320×480 세로 해상도를 기준으로 한다.

핵심 목표는 다음과 같다.

1. **주행 중 한눈에 읽기**: 속도, 4WD 후륜 결합 추정, DPF 경고처럼 현재 판단에 필요한 값만 크게 보인다.
2. **상태와 제어 분리**: 주행 화면은 상태 확인, 오디오 화면은 편의 기능, 자동화 화면은 정책과 잠금 상태를 담당한다.
3. **프리미엄 가전의 차분함**: 검은 계기판 위에 정보를 계속 번쩍이기보다, 매트한 흑연색 표면과 한 가지 서리빛 청색을 사용한다.
4. **검증 수준 공개**: DBC 후보값을 확정값처럼 꾸미지 않는다. `후보`, `추정`, `확인`, `끊김`을 값과 함께 표시한다.
5. **실수 방지**: 주행 중 세부 조정, 미검증 제어, 연결이 불안정한 상태의 명령을 잠근다.

## 2. Hallmark 적용 판단

Hallmark는 이 프로젝트의 **시각적 품질 프레임**으로 적절하다. 빈번한 카드 반복, 과한 둥근 모서리, 무의미한 그라데이션, 작은 터치 표적, hover 의존과 같은 일반적인 UI 결함을 줄이고 토큰·상태·반응형 계약을 일관되게 만드는 데 사용했다.

다만 Hallmark는 차량 안전 규격이 아니다. 다음 영역에는 적용 권한을 주지 않는다.

- 주행 중 허용할 조작과 잠금 조건
- DBC 신호의 진위와 차량별 의미
- CAN 송신 allow-list, lease, timeout, snapshot 복원
- 경고 문구와 정비 판단

이 영역은 Hyundai 사용설명서, Google Design for Driving의 glanceability·touch 지침, 실차 검증 결과, [`feature-design.md`](feature-design.md)와 [`esp-now-protocol.md`](esp-now-protocol.md)를 우선한다.

Hallmark 기준으로 선택한 구성은 `Workbench` macrostructure, `N9` 상단 상태, `C4` 상시 노출 하단 탭이다. 테마는 프로젝트 전용 `Nocturne Alloy`이며 분위기 축은 “automotive clarity, appliance calm, technical precision”이다.

## 3. 조사·벤치마크

| 근거 | 차용한 점 | 차용하지 않은 점 |
|---|---|---|
| [Google Design for Driving interaction principles](https://developers.google.com/cars/design/design-foundations/interaction-principles) | 약 2초 안의 glanceability, 즉각적인 피드백, 단순하고 중단 가능한 흐름 | Android Automotive의 화면 구조를 그대로 복제하지 않음 |
| [Google Automotive OS sizing](https://developers.google.com/cars/design/automotive-os/design-system/sizing) | 최소 76×76 dp touch target을 주요 내비게이션과 profile에 반영 | 320×480의 76 px가 실제 76 dp와 같다고 단정하지 않음 |
| [Hyundai Quiet Mode](https://ownersmanual.hyundai.com/ivi/ccNC/AVNT/KOR/English/Quietmode.html) | 뒤 스피커를 끄고 앞 음량을 조정하는 사용자 개념 | 최신 ccNC의 그래픽을 복제하지 않음 |
| [Hyundai sound settings](https://ownersmanual.hyundai.com/ivi/DA_GEN2_V/AV/AUS/English/010_Settings_sound.html) | 차량 도식 기반 sound position과 speed-dependent volume 개념 | 대상 차량에서 CAN 제어가 검증됐다고 간주하지 않음 |
| [2018 Tucson TL 설명서](https://www.hyundaicanada.com/-/media/hyundai/feature/ownerssection/manuals/english/2018/tuscon/tl-can-eng-4.pdf) | fader/balance의 상·하·좌·우 조작과 SDVC를 같은 세대 참고로 사용 | 한국형 2017 BlueLink와 동일 사양이라고 단정하지 않음 |
| [RealDash 공식 앱 설명](https://play.google.com/store/apps/details?id=com.napko.RealDash) | 상세 진단 화면의 사용자 구성·알람 개념만 참고 | 주행 홈에 작은 gauge를 다수 배치하는 방식은 배제 |
| [LVGL 8.4 meter](https://lvgl.io/docs/open/8.4/widgets/extra/meter), [button matrix](https://lvgl.io/docs/open/8.4/widgets/core/btnmatrix), [tabview](https://lvgl.io/docs/open/8.4/widgets/extra/tabview) | arc/bar/button과 상태 style을 임베디드 primitive로 구현 | 기본 widget 외형을 그대로 사용하지 않음 |

벤치마크 결론은 “최신 클러스터의 정보 우선순위 + 순정 오디오의 익숙한 기능 모델 + 프리미엄 가전의 낮은 시각 소음”이다. 계기판처럼 보이게 만드는 장식보다 읽는 순서와 상태의 신뢰도를 우선한다.

## 4. 정보 구조

```text
상단 36 px: CANView / 활성 CAN bus / ESP-NOW 상태
본문 368 px
├─ 주행: 속도·RPM / 후륜 결합 추정 / DPF / 주행 mode
├─ 오디오: 취침·뒷좌석+ / sound position / 소음 보정
└─ 자동: SPORT 감시 / 판단 입력 / 제어 잠금·무선 진단
하단 76 px: 주행 / 오디오 / 자동
```

주행 중 기본 화면은 항상 `주행`이다. 알림이 발생해도 앱이 임의로 탭을 바꾸지 않고, 상단 상태 또는 해당 카드의 색·문구만 바꾼다. 치명적인 링크 단절은 현재 값을 `stale`로 바꾸되 마지막 값을 지워 운전자가 갑작스러운 숫자 변화를 실제 상태 변화로 오해하지 않게 한다.

### 4.1 주행 화면

읽기 순서는 속도 → 후륜 결합 추정 → DPF → 보조 상태다.

- 속도는 가장 큰 숫자이며 단위는 작고 고정된 위치에 둔다.
- RPM은 정밀 눈금 대신 짧은 bar와 숫자를 함께 둔다.
- 중앙 drivetrain 도식은 실제 전·후 축 토크 비율이 아니라 `_4WD11.CLU_DUTY` 기반 `후륜 결합 추정`이다.
- `_4WD_TQC_CUR`는 차량 검증 전 `DBC 후보값`을 항상 붙인다.
- DPF는 lamp와 상세 sensor 검증 여부를 분리한다. 정상처럼 보이는 데모값도 `세부 센서 미검증`을 남긴다.
- 자동 SPORT는 초기 release에서 `감시 전용`이다.

### 4.2 오디오 화면

상단의 `취침`, `뒷좌석 +`는 상호 배타적인 76 px 이상 profile 버튼이다. 중앙 sound position은 차량 도식과 상·하·좌·우 48 px 보조 버튼으로 구성한다. 이 버튼은 주행 중 disabled 상태가 되어야 하며, 정차 상태에서만 반복 입력을 허용한다.

- `취침`: 가능하면 rear speaker mute, 불가능하면 front bias와 quiet volume cap
- `뒷좌석 +`: rear mute 해제, 검증된 방향으로 fader 2 step, 사용자 cap 안에서 선택적 volume +1
- profile 해제: 중앙값이 아니라 적용 전 OEM snapshot 복원
- 차량 head unit에서 직접 조작 감지: 자동 쓰기 중단 후 새 상태를 채택
- `주행 소음 보정`: 상대 noise와 적용 중인 volume offset을 함께 표시

### 4.3 자동화 화면

SPORT 자동화는 기술 설정 화면이 아니라 일반 사용자용 상태 화면으로 표현한다.

- 큰 문장: `필요할 때만 SPORT`
- 현재 단계: `감시 → 조건 유지 → 제안`
- 현재 mode: `NORMAL`, `SPORT`, `UNKNOWN`
- 정책 입력 예: longitudinal acceleration과 진입 threshold
- 하단 진단: encryption, TX lock, RTT

실제 전환 기능이 검증되기 전에는 버튼 label을 `감시 중`으로 제한한다. 향후 실제 제어를 열더라도 `자동 전환`이라는 별도 opt-in 문구, 즉시 취소, cooldown, 사용자 수동 변경 우선 규칙이 필요하다.

## 5. 시각 시스템

정본 token은 루트의 [`tokens.css`](../tokens.css)다. LVGL의 sRGB 근사값은 [`canview_theme.h`](../ui/lvgl/canview_theme.h)에 있다.

| 역할 | 토큰 | 의도 |
|---|---|---|
| 바탕 | `--color-paper` | 청색기가 약한 짙은 흑연색 |
| 카드 | `--color-paper-2` | 바탕과 작은 명도 차이만 둔 매트 표면 |
| 구분 | `--color-rule`, `--color-rule-2` | 그림자 대신 1 px 경계 사용 |
| 강조 | `--color-accent` | 연결·활성·현재값에만 쓰는 서리빛 청색 |
| 주의 | `--color-warning` | DPF 확인 등 행동이 필요한 상태 |
| 오류 | `--color-error` | bus-off, 제어 실패처럼 실제 오류에만 사용 |

색에만 의미를 맡기지 않고 icon·label·quality tag를 함께 사용한다. 광택, 유리 효과, 배경 bloom, 장식용 gradient는 사용하지 않는다. radius는 8/12/18 px 세 단계이며 모든 작은 요소를 pill로 만들지 않는다.

폰트는 숫자·영문 display에 `Space Grotesk`, 한글 본문에 `IBM Plex Sans KR`을 사용한다. LVGL 빌드에서는 실제 사용 글리프만 변환한 한글 폰트와 별도 숫자 폰트를 주입한다.

## 6. 320×480 레이아웃 명세

| 영역 | 위치·크기 | 비고 |
|---|---|---|
| app | `0,0,320,480` | scroll 없음 |
| top status | 높이 36 | 12 px 좌우 padding |
| content | `y=36`, 높이 368 | 12 px 좌우, 10/8 px 상하 padding |
| bottom nav | `y=404`, 높이 76 | 탭 3개, 각 약 106×76 |
| 기본 gap | 8 | 4 px spacing 계열 |
| primary touch | 최소 76×76 | profile·navigation |
| secondary touch | 최소 48×48 | 정차 시 sound position |

320×480은 실제 hardware의 정본 viewport다. 브라우저 prototype은 375, 414, 768 px에서도 320 px app을 중앙 정렬해 화면 비율과 터치 좌표가 바뀌지 않게 한다. 펌웨어에서는 회전·해상도를 고정하고 display flush buffer만 조정한다.

Google의 76 dp 지침을 여기서는 76 logical px로 먼저 옮겼다. 실제 패널 크기·운전자 거리·장갑 사용 환경에서 물리 표적 크기를 측정하고, 부족하면 보조 버튼도 76 px로 확대하거나 주행 중 잠금 범위를 늘린다.

## 7. 구성요소와 LVGL 매핑

| UI 구성요소 | Web prototype | LVGL 8.4 | 상태 입력 |
|---|---|---|---|
| 상단 연결 상태 | flex status row | `lv_obj` + `lv_label` | bus count, RSSI, stale |
| 속도/RPM | metric text + bar | `lv_label`, `lv_bar` | vehicle speed, RPM |
| 후륜 결합 | hand-built SVG | `lv_arc` + labels | duty/quality/age |
| DPF 상태 | semantic status row | card + labels | lamp raw/decoded quality |
| audio profile | 76 px buttons | checkable `lv_btn` | desired + feedback revision |
| cabin focus | SVG cabin map | styled `lv_obj` geometry | fader/balance steps |
| 방향 조작 | four 48 px buttons | four `lv_btn` | semantic step delta |
| SPORT monitor | mode card/dial | `lv_arc`, button, labels | mode, accel, policy state |
| bottom nav | fixed tabs | three `lv_btn` | current screen |

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
| sound position step | 허용 | 금지 | 반복 조작·시선 분산 |
| 소음 보정 on/off | 허용 | 상태 조회만 권장 | calibration 변경은 정차 |
| SPORT 감시 on/off | 허용 | off는 허용, on은 정차 | 운전자 의도 우선 |
| pairing/configuration | 허용 | 금지 | 통신·보안 설정 |

## 9. 화면 prototype

아래 이미지는 실제 CAN 값을 읽은 화면이 아니라 레이아웃과 상태 언어를 검토하기 위한 **데모 데이터 prototype**이다.

### 주행 상태

![CANView 주행 화면 prototype](images/ui-drive.png)

### 오디오 기능

![CANView 오디오 화면 prototype](images/ui-audio.png)

### SPORT 자동화

![CANView 자동화 화면 prototype](images/ui-automation.png)

브라우저에서 [`ui/prototype/index.html`](../ui/prototype/index.html)을 열고 query parameter를 바꾸면 각 화면을 확인할 수 있다.

```text
index.html?screen=drive
index.html?screen=audio
index.html?screen=automation
```

## 10. 검증 체크리스트

### 시각·조작성

- 320×480에서 잘림과 수평 scroll이 없어야 한다.
- 375/414/768 px browser viewport에서도 app 내부 좌표가 변하지 않아야 한다.
- 주요 터치 표적은 76 px, 정차 전용 보조 표적은 48 px 이상이다.
- 한글·숫자 baseline과 단위 위치를 실제 LVGL font에서 재검증한다.
- 100%, 50%, 0% coupling과 긴 error 문구에서도 넘침이 없어야 한다.
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

## 11. 구현 산출물

- 디자인 token: [`tokens.css`](../tokens.css)
- 상호작용 prototype: [`ui/prototype/`](../ui/prototype/)
- LVGL 8.4 UI: [`ui/lvgl/`](../ui/lvgl/)
- 기능·CAN 신호 설계: [`feature-design.md`](feature-design.md)
- ESP-NOW protocol: [`esp-now-protocol.md`](esp-now-protocol.md)

다음 디자인 단계는 새 화면을 늘리는 작업이 아니라 실제 3.5인치 패널에서 font rendering, touch hit area, 야간 밝기, 설치 각도를 검증하는 것이다. 그 결과로 먼저 spacing과 typography token을 보정하고, 정보 구조는 실차 capture에서 새로운 확정 신호가 생길 때만 확장한다.
