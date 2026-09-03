# CANView LVGL UI

이 디렉터리는 `ESP32-S3-Touch-LCD-3.5`의 320×480 세로 화면을 위한 LVGL 8.4 UI 계층이다. HTML 프로토타입의 정보 구조와 색 토큰을 임베디드 화면으로 옮기되, CAN 프레임 생성·ESP-NOW 재전송·차량 제어 정책은 포함하지 않는다.

## 포함 파일

- `canview_ui.h`: 화면에 주입할 표시 모델과 의미 기반 명령 콜백
- `canview_ui.c`: 주행·소리·FFT·자동화·설정 화면 및 76 px 하단 내비게이션
- `canview_theme.h`: `tokens.css`를 RGB565로 변환하기 전의 sRGB 기준 색상

## 통합 예

```c
#include "canview_ui.h"

static void on_ui_command(const canview_ui_command_t *command, void *user_data)
{
    /* 여기서는 command를 제어 정책 큐에 복사만 한다.
     * 사전조건, lease, ACK, OEM 상태 복원은 상위 계층에서 처리한다. */
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

CAN/ESP-NOW 작업 스레드에서 `canview_ui_update()`를 직접 호출하면 안 된다. 수신 값을 표시 모델로 정규화한 뒤 LVGL 작업 큐 또는 UI 태스크에서 호출한다.

## 폰트와 메모리

소스의 한글 문자열은 프로젝트에서 변환한 한글 폰트가 필요하다. `lv_font_conv`로 실제 화면에 쓰는 글리프만 추출하고 `canview_ui_config_t.font`에 지정한다. 폰트를 지정하지 않으면 `LV_FONT_DEFAULT`를 사용하므로 기본 설정에서는 한글이 보이지 않을 수 있다.

대형 숫자용 영문 폰트는 `metric_font`에 별도로 지정한다. 생략하면 일반 UI 폰트를 사용한다.

## 안전 경계

- UI 콜백은 `CANVIEW_UI_CMD_*` 의미 명령만 생성한다.
- 상위 제어 계층이 정차 여부, 차량 상태, 사용자 수동 조작, lease와 복원 스냅샷을 확인한다.
- `four_wd_quality`가 `VERIFIED`가 아니면 후륜 결합률은 실제 축 토크 배분으로 표현하지 않는다.
- DPF raw code와 상세 신호 검증 상태는 일반 UI가 아니라 서비스 진단에 보존한다.
- FFT의 dB 값은 microphone chain을 보정하기 전에는 상대 레벨로 취급한다.
- 주행 중에는 밝기·단위 설정을 상위 계층에서 `LV_STATE_DISABLED`로 전환한다.

상세 상호작용과 상태 표는 [`../../docs/ui-design.md`](../../docs/ui-design.md), 제어·신호 근거는 [`../../docs/feature-design.md`](../../docs/feature-design.md), 무선 명령 수명주기는 [`../../docs/esp-now-protocol.md`](../../docs/esp-now-protocol.md)를 참고한다.
