# CANView LVGL UI

`ESP32-S3-Touch-LCD-3.5`의 320×480 세로 화면을 위한 LVGL 8.4 UI 계층이다. 주행·소리·FFT·자동화·설정 화면, 지속 속도, 제한속도 overlay와 짧은 animation을 포함한다. CAN frame 생성, ESP-NOW 재전송과 차량 제어 정책은 포함하지 않는다.

## 파일

- `canview_ui.h`: 화면에 주입할 표시 모델과 의미 기반 명령 callback
- `canview_ui.c`: 5개 화면, 전역 overlay, 값 보간과 화면 fade
- `canview_theme.h`: `tokens.css`의 LVGL sRGB 색상

## 통합 예

```c
#include "canview_ui.h"

static void on_ui_command(const canview_ui_command_t *command, void *user_data)
{
    /* command를 정책 queue에 복사한다. CAN payload는 여기서 만들지 않는다. */
}

void app_ui_start(void)
{
    canview_ui_config_t config = {
        .font = &font_ibm_plex_sans_kr_14,
        .metric_font = &font_space_grotesk_28,
        .command_cb = on_ui_command,
    };
    canview_ui_create(lv_scr_act(), &config);
}
```

CAN/ESP-NOW task에서 `canview_ui_update()`를 직접 호출하지 않는다. 수신값을 `canview_ui_model_t`로 정규화한 뒤 LVGL task에서 갱신한다. root까지 전달되는 press와 root 밖 dropdown popup의 press는 각각 한 번만 `CANVIEW_UI_CMD_USER_ACTIVITY`를 발생시킨다. 상위 자동화가 유휴 latch와 밝기를 갱신한다.

이 adapter는 단일 인스턴스다. `canview_ui_create()` 재호출은 기존 root를 반환하며 설정을 덮어쓰지 않는다. `canview_ui_destroy()`는 여러 번 호출해도 안전하고, 외부 parent 삭제도 감지한다. 삭제 후 `update()`는 무시하며 다시 생성할 수 있다. 폰트는 해당 UI 객체보다 오래 살아 있어야 한다. 모든 API와 callback은 하나의 LVGL 실행 스레드에서 호출한다. LVGL 자체 `lv_deinit()` 뒤의 재초기화는 이 수명 계약에 포함하지 않는다.

명령 callback은 전달받은 구조체를 queue에 복사한다. 같은 의미 명령은 완료 전 재발행하지 않는다. 상위 orchestration은 matching terminal 결과와 필요한 feedback/owner readback을 확인한 뒤 LVGL 스레드에서 `canview_ui_command_complete(id)`로 해당 요청을 해제한다. 거부·취소·timeout에서도 실제 상태와 진행 중 요청의 종결을 확인한다. transport ACK만으로 완료 처리하지 않는다. 수신 모델을 갱신하지 않은 완료 호출은 표시 상태를 바꾸지 않는다. `update()`·`command_complete()`·`destroy()`를 callback에서 동기 호출할 수도 있다. transaction 식별·중복/late 결과 폐기·timeout 소유권은 상위 orchestration에 있다.

설정의 권위 snapshot은 [config A/B](../../docs/architecture/ota.md)이며 NVS는 비권위 cache다. 차량 설정은 matching `CONFIG_RESULT` 성공/`APPLIED`, 새 `state_revision`과 owner readback 확인 뒤에만 Controller 표시 모델과 mirror를 갱신한다. 실제 차량 명령의 적용 확인에는 별도로 matching terminal `COMMAND_RESULT(COMPLETED)`와 최신 matching feedback이 필요하다. UI 선택이나 transport ACK는 적용 성공의 근거가 아니다.

## 화면 계약

- 주행: 352px 본문 중 250px의 중앙 SUV·앞뒤 differential·shaft와 네 wheel의 8단 구동 지수·TPMS, 차량 중앙 순간연비와 km/L, 하단 30px DPF와 56px 원형 속도/RPM. RPM 오른쪽 3줄에 배터리 전압·미션 clutch lock·온도를 표시하고 자동 화면에도 같은 모델을 복사한다. 현재 mode는 4WD 오른쪽 위에 둔다. 전장/전폭·144px 축간·86px 윤거의 기존 TL 윤곽 비율을 유지한다.
- 소리: 취침/뒷좌석+ profile, 현재 음량, Cabin FFT. 임의 sound position과 volume ± control 없음
- FFT: 상단 `PEAK`·`LEVEL`, signed digital `dBFS`, 50Hz–8kHz spectrum, 하부 차속·RPM strip. 두 telemetry는 FFT 품질과 독립적으로 표시한다.
- 자동화: 원 장식 없는 mode 상태. SPORT red, NORMAL blue, ECO green
- 설정: UTC+09:00 기준 RTC 연도 2000–2099·월·일·시·분 0–59 dropdown과 `적용 요청`, 밝기 slider, toggle, 제한된 preset dropdown. 존재하지 않는 날짜는 요청을 막는다. RTC 원천/품질이 invalid이면 현재값과 초기 선택은 미확정으로 표시한다. 선택 초안은 현재 RTC가 아니며 수신 확인 전 현재값으로 승격하지 않는다. 자유 숫자 text input과 속도 단위 설정은 없다.
- 전역: 현재 속도와 speed-limit overlay, 일몰 후 실제 전조등 미점등 경고. 과속/전조등 경고는 모든 화면 중앙의 128px 영역이며 주행 이외 탭은 반투명이다. 과속이 아닌 유효 제한 표지는 36px header 영역에 두고 연결 요약과 겹치지 않는다. overlay와 label은 touch를 가로채지 않는다. 초기 모델에는 경고나 실차 성공 데모값을 넣지 않는다.

실행 중 arc·DPF는 현재 보간값에서 180–300ms animation으로 이어지고 screen은 160ms fade한다. 같은 목표값은 animation을 재시작하지 않는다. invalid 전환은 animation을 즉시 취소하고 표시를 지운다. 숨긴 화면의 fade도 취소한다. idle 진입 에지에서 한 번만 주행으로 복귀하며 popup을 닫는다. warning overlay는 검증된 속도·제한값이 있을 때만 model의 blink 상태를 따른다. PWM·과속 지속시간·SPORT 상태기계와 OTA는 이 adapter에서 구현하지 않는다.

## 폰트와 메모리

한글은 `lv_font_conv`로 실제 글리프만 변환해 `font`에 지정한다. 대형 숫자·영문은 `metric_font`에 따로 주입한다. 생략하면 `LV_FONT_DEFAULT`를 사용하므로 기본 빌드에서는 한글이 보이지 않을 수 있다.

최신 공식 데모의 설계 패턴을 참고했지만 코드는 LVGL 8.4 API만 사용하고 `lv_demos`에 runtime 의존하지 않는다. 채용·제외 내역은 [공식 데모 검토](../../docs/ui/lvgl-demo-review.md)에 있다.

## 안전 경계

- UI callback은 `CANVIEW_UI_CMD_*` 의미 명령만 만든다.
- 상위 계층이 정차, 신호 freshness, lease, feedback과 OEM snapshot 복원을 확인한다.
- `four_wd_quality`가 검증되지 않으면 wheel 값을 실제 바퀴 torque로 해석하지 않는다.
- 4WD와 TPMS quality는 독립이며 unavailable은 `0` 대신 `—`다.
- DPF load는 검증된 source가 없으면 unavailable로 둔다. lamp 상태 문구는 검증된 off일 때 빈 문자열, on일 때 `확인`, invalid일 때 `—`다. lamp off를 DPF 전체 정상이나 재생 완료로 해석하지 않는다.
- FFT LEVEL은 digital full-scale 기준 dBFS이며 음압 dBA가 아니다. 자동 음량의 baseline 대비 dB 차이와도 구분한다.
- 자동 밝기는 차량 미등·rheostat CAN만 사용한다.
- `settings_valid && settings_edit_allowed && speed_quality == VERIFIED && speed == 0`일 때만 설정을 편집한다. 실제 control과 callback 양쪽에서 잠금을 검사하고 열린 popup도 닫는다. 오디오 profile 요청에는 별도의 `audio_control_allowed`와 검증된 profile 상태가 필요하다.

## 상태 모델 통합 변경

- `QUALITY_STALE`을 추가했다. 속도·RPM·현재 mode·previous mode·OEM volume·audio profile·offset·FFT·제한속도에 독립 quality를 전달한다. 기존 4WD/TPMS/연비/DPF quality와 마찬가지로 VERIFIED만 정상 수치로 표시한다. 후보·추정·stale·범위 밖 값은 `—`다. 4WD는 여전히 구동 지수이며 실제 torque로 승격하지 않는다.
- `wheel_drive_unavailable_mask`, `tire_pressure_unavailable_mask`로 FL/FR/RL/RR의 개별 유실을 표현한다. 전체 quality와 함께 검사한다. 검증된 0psi는 `0.0 psi`이며 unavailable과 다르다.
- `fft_peak_tenth_db`는 signed `int16_t`, 0.1 digital dBFS이며 UI 표시 범위는 -160.0–0.0 dBFS다. 음수 소수도 보존하며 dBA로 표시하지 않는다. FFT invalid 또는 0–100 밖 bin은 이전 spectrum을 지운다. raw PCM이나 FFT complex buffer는 받지 않는다. 실제 full-scale reference·FFT window·peak 산식·정규화와 adapter 승격은 [T-303](../../docs/tasks/T-303-controller-fft.md)의 미완료 gate다. 기준이 없는 상대 dB를 숫자 그대로 넣고 VERIFIED로 표시하면 안 된다.
- `rtc_year/month/day/source`와 `CANVIEW_UI_CMD_SET_RTC_DATETIME`의 `value.datetime`을 추가했다. 기존 `SET_RTC_TIME` 식별자와 의미는 보존하지만 새 UI는 날짜·시각을 하나의 callback으로 요청한다. wire protocol 변경이 아니다.
- 설정은 확인된 snapshot을 `settings_valid`와 함께 전달한다. dropdown·toggle·밝기가 모델을 따라가며 임의 선택은 차량 성공 상태가 되지 않는다. 초기 확인 전에는 잠금과 미확정 표시가 기본값이다.
- `average_fuel_economy_tenth_kmpl`은 과거 소스 호환용 미사용 필드다. 현재 UI는 차량 중앙 순간연비만 표시하며 wire ABI 호환을 뜻하지 않는다.
- 입력 모델은 정수 fixed-point다. upstream의 float `NaN`/`Inf`는 정수 변환 전에 거부하고 quality를 unavailable로 내려야 한다. 이 API는 float를 받아 추정 변환하지 않는다.
- 자동화 입력 `speed_valid`, `speed_age_ms`, `speed_stale_timeout_ms`는 상위 adapter가 원본 age와 함께 자동화에 전달해야 한다. 표시 모델도 같은 freshness 판정 결과로 VERIFIED/STALE을 정한다. UI가 수신 시각을 새 sample age로 덮어쓰거나 밝기를 재계산하지 않는다.

## 실제 LVGL 8.4 host 검증

공식 [LVGL v8.4.0](https://github.com/lvgl/lvgl/tree/v8.4.0), commit `4495f428630cc1741bd8bfd977f080e8460e8e8d`를 사용한다. `.tools`는 Git 추적 대상이 아니며 검증기는 원본 checkout의 commit과 clean 상태를 검사한다.

```powershell
git clone --depth 1 --branch v8.4.0 https://github.com/lvgl/lvgl.git .tools/lvgl-8.4.0
./tools/ui/validate-lvgl.ps1
```

검증기는 Windows MSVC·CMake·Ninja를 찾고 필요하면 설치된 Visual Studio Developer PowerShell 환경을 로드한다. [실제 runtime 회귀시험](../../tests/lvgl/test_lvgl.c)은 공식 LVGL 전체 C 소스를 링크하며 가짜 LVGL stub을 사용하지 않는다. 검증 대상은 5개 화면 텍스트 경계/겹침, 독립 quality, 잘못된 enum/range, 모든 분과 윤년, 미확인 요청과 수신 모델 구분, 정차 잠금, overlay hit test, 유휴 진입, callback 재진입, 100회 생성/삭제·2000회 화면 전환의 animation/heap 수명이다.

2026-09-06 Windows MSVC 19.50의 C11 `/W4 /WX` 컴파일과 host 회귀를 통과했다. 이 시험은 Montserrat 14/28과 가상 320×480 display를 사용한다. 최종 한글 subset font, 보드 flush/FPS·PWM, 8시간 soak, 실물 touch, 차량 제어 gate는 미실행이다. 작성자 검증이며 최종 2인 독립 리뷰를 대체하지 않는다.

실제 LVGL draw buffer의 합성 fixture 렌더는 `.tools/lvgl-host-build/drive.bmp`, `audio.bmp`, `fft.bmp`, `automation.bmp`, `settings.bmp`에 생성된다. 최종 한글 폰트가 없어 한글은 대체 glyph로 나타난다. 이 이미지와 upstream 소스·build 결과는 Git에 넣지 않는다.

상세 화면 계약은 [운전자 UI 설계](../../docs/ui/design.md), 자동화는 [자동 제어 로직](../../docs/architecture/automation.md), 무선 명령 수명주기는 [ESP-NOW 프로토콜](../../docs/architecture/protocols/esp-now.md)을 참고한다.
