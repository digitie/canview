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

CAN/ESP-NOW task에서 `canview_ui_update()`를 직접 호출하지 않는다. 수신값을 `canview_ui_model_t`로 정규화한 뒤 LVGL task에서 갱신한다. 모든 press는 `CANVIEW_UI_CMD_USER_ACTIVITY`를 발생시켜 유휴 감광을 해제한다.

## 화면 계약

- 주행: 본문 약 75%를 차지하는 세로형 중앙 차량·앞뒤 differential·shaft와 네 wheel의 8단 구동 지수·TPMS, 차량 중앙 순간연비, 하단 DPF·작은 원형 속도/RPM
- 소리: 취침/뒷좌석+ profile, 현재 음량, Cabin FFT. 임의 sound position과 volume ± control 없음
- FFT: `PEAK`와 `LEVEL`을 chart 내부 상단에 둔 대형 spectrum
- 자동화: 원 장식 없는 mode 상태. SPORT red, NORMAL blue, ECO green
- 설정: RTC 시·분, 밝기 slider, toggle, 제한된 `lv_dropdown`. 자유 숫자 text input과 속도 단위 설정 없음
- 전역: 현재 속도와 speed-limit overlay, 일몰 후 미등/전조등 경고. overlay는 click flag가 없어 아래 control을 가로채지 않음

실행 중 telemetry 값은 180–300 ms animation으로 이어지고 screen은 160 ms fade한다. idle 진입은 주행 화면으로 복귀하며, warning overlay는 model의 blink 상태를 그대로 따른다.

## 폰트와 메모리

한글은 `lv_font_conv`로 실제 글리프만 변환해 `font`에 지정한다. 대형 숫자·영문은 `metric_font`에 따로 주입한다. 생략하면 `LV_FONT_DEFAULT`를 사용하므로 기본 빌드에서는 한글이 보이지 않을 수 있다.

최신 공식 데모의 설계 패턴을 참고했지만 코드는 LVGL 8.4 API만 사용하고 `lv_demos`에 runtime 의존하지 않는다. 채용·제외 내역은 [공식 데모 검토](../../docs/ui/lvgl-demo-review.md)에 있다.

## 안전 경계

- UI callback은 `CANVIEW_UI_CMD_*` 의미 명령만 만든다.
- 상위 계층이 정차, 신호 freshness, lease, feedback과 OEM snapshot 복원을 확인한다.
- `four_wd_quality`가 검증되지 않으면 wheel 값을 실제 바퀴 torque로 해석하지 않는다.
- 4WD와 TPMS quality는 독립이며 unavailable은 `0` 대신 `—`다.
- DPF load는 검증된 source가 없으면 unavailable로 둔다.
- FFT dB는 microphone chain 보정 전 상대 레벨이다.
- 자동 밝기는 차량 미등·rheostat CAN만 사용한다.
- 주행 중 설정 control은 상위 계층에서 `LV_STATE_DISABLED`로 전환한다.

상세 화면 계약은 [운전자 UI 설계](../../docs/ui/design.md), 자동화는 [자동 제어 로직](../../docs/architecture/automation.md), 무선 명령 수명주기는 [ESP-NOW 프로토콜](../../docs/architecture/protocols/esp-now.md)을 참고한다.
