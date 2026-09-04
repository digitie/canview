# LVGL 공식 데모 검토 기록

이 문서는 [UI 설계](design.md)의 선택 근거를 보존하는 참고 기록이다. 현재 제품 화면 규칙은 UI 설계 문서가 정본이다.

## 범위

CANView의 펌웨어 기준 API는 LVGL 8.4다. 다만 최신 임베디드 UI 패턴을 놓치지 않기 위해 2026-09-03 기준 별도 [`lv_demos/src`](https://github.com/lvgl/lv_demos/tree/master/src)의 모든 7개 데모와 [`lvgl/v8.4.0/demos`](https://github.com/lvgl/lvgl/tree/v8.4.0/demos)의 모든 5개 데모를 각각 검토했다.

이 문서의 `채용`은 데모의 정보 구조나 API 사용법을 참고했다는 뜻이다. 데모 자산·화면·소스를 제품에 복사하거나 런타임 의존성으로 추가하지 않았다.

## 최신 `lv_demos` 검토

### eBike

홈·통계·설정 파일이 분리되고, 큰 주행 지표를 중심에 두며, 보조값과 설정을 낮은 계층으로 보낸다. CANView는 이 데모를 주 기준으로 삼아 다음을 적용했다.

- 기본 주행 화면을 첫 화면으로 유지
- 큰 실시간 상태와 짧은 하단 navigation
- 설정의 slider, toggle, dropdown 문법
- 수치 변경의 짧은 animation과 화면 전환 fade

원형 계기 자체는 복제하지 않았고, 4WD가 가장 중요한 프로젝트 요구이므로 화면 중심을 중앙 차량과 네 바퀴에 양보했다.

### flex_layout

가변 폭 행, 균등 분배, grow를 같은 행의 연비 수치와 wheel card에 반영했다. 320 px 고정 화면에서도 폰트 폭과 `—` 상태가 달라질 수 있으므로 절대 좌표만으로 열을 맞추지 않는다.

### high_res

해상도별 token과 자산 분리 개념만 채용했다. CANView는 물리 화면이 320×480으로 고정되어 있어 큰 이미지, 다단 breakpoint와 고해상도 장식은 메모리·가독성 이점이 없다.

### multilang

한글 UI 폰트와 숫자 metric 폰트를 분리하고 필요한 글리프만 변환하는 기반으로 사용한다. 1차 제품은 한글 고정이므로 language picker와 런타임 전체 relayout은 만들지 않는다.

### scroll

설정 본문에만 세로 scroll과 momentum을 적용한다. 숫자 선택은 관성 scroll로 직접 바뀌지 않고 dropdown을 명시적으로 연 뒤 고르게 한다. 주행·FFT 화면과 하단 navigation은 scroll되지 않는다.

### smartwatch

작은 화면에서 한 화면에 한 가지 목적을 두고 상태를 짧게 표시하는 원칙을 사용한다. 원형 viewport, edge swipe 중심 탐색, 작은 아이콘에만 의존하는 구성은 차량 장착 touch에 적합하지 않아 제외한다.

### transform

transform은 데모와 성능 시험에만 사용한다. 실시간 숫자·바퀴·FFT 막대를 확대/회전하면 raster 흔들림과 render 부하가 생기므로 제품 animation은 값 보간과 opacity에 제한한다.

## LVGL 8.4 기본 데모 검토

### widgets

제품이 사용하는 arc, bar, chart, slider, button, switch, dropdown의 이벤트와 state 기준이다. 숫자 설정은 slider 또는 유한 dropdown으로 구현하고 자유 text input은 사용하지 않는다.

### music

공식 설명상 272×480에서도 동작하는 세로형 현대 UI다. CANView 소리 화면의 큰 현재 음량, 하단 spectrum, 제한된 profile button과 FFT 화면의 높은 그래프 비중에 이 위계를 적용했다.

### keypad_encoder

현재 입력은 capacitive touch지만 향후 rotary encoder 또는 steering switch를 붙일 때 `lv_group` focus와 press 동작의 기준으로 보존한다. touch 좌표와 focus selected 상태가 서로 다른 명령을 만들지 않도록 동일 callback 경로를 사용한다.

### benchmark

화면 장식이 아니라 성능 gate로 사용한다. ESP32-S3 실물에서 평균/최저 FPS, render time, display flush time, peak heap을 기록하고 chart point 수와 opacity 사용을 결정한다.

### stress

탭 변경, settings scroll, FFT series 갱신, animation cancellation, screen 재생성의 heap 손상·누수·use-after-free를 찾는 soak-test 모델로 사용한다.

## 채택 결과

| 제품 결정 | 주 근거 |
|---|---|
| 4WD 중심 기본 화면과 큰 수치 | eBike, smartwatch |
| flex 기반 행·열 정렬 | flex_layout |
| 설정 본문만 scroll | scroll, widgets |
| 현재 음량 + 대형 FFT | music |
| slider/toggle/dropdown, text 숫자입력 없음 | widgets, keypad_encoder |
| 160–300 ms의 짧은 fade·값 보간 | eBike, music, transform 성능 검토 |
| 실물 FPS/heap gate | benchmark, stress |
| 한글/숫자 font 분리 | multilang, high_res |

구체 화면 명세와 스크린샷은 [운전자 UI 설계](design.md)에 있다.
