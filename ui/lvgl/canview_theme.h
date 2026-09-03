#ifndef CANVIEW_THEME_H
#define CANVIEW_THEME_H

#include "lvgl.h"

/* tokens.css의 sRGB 근사값. 실제 패널 감마와 밝기에 맞춰 최종 보정한다. */
#define CANVIEW_COLOR_PAPER        lv_color_hex(0x04080C)
#define CANVIEW_COLOR_PAPER_2      lv_color_hex(0x0A1015)
#define CANVIEW_COLOR_PAPER_3      lv_color_hex(0x141B22)
#define CANVIEW_COLOR_PAPER_4      lv_color_hex(0x1F282F)
#define CANVIEW_COLOR_INK          lv_color_hex(0xE9F0F3)
#define CANVIEW_COLOR_INK_2        lv_color_hex(0xBEC5CA)
#define CANVIEW_COLOR_MUTED        lv_color_hex(0x8F969C)
#define CANVIEW_COLOR_RULE         lv_color_hex(0x2A3138)
#define CANVIEW_COLOR_RULE_2       lv_color_hex(0x1B2329)
#define CANVIEW_COLOR_ACCENT       lv_color_hex(0x36CAF1)
#define CANVIEW_COLOR_ACCENT_WASH  lv_color_hex(0x062128)
#define CANVIEW_COLOR_ACCENT_LINE  lv_color_hex(0x206173)
#define CANVIEW_COLOR_WARNING      lv_color_hex(0xEDB333)
#define CANVIEW_COLOR_WARNING_WASH lv_color_hex(0x241C0C)
#define CANVIEW_COLOR_ERROR        lv_color_hex(0xFA6863)
#define CANVIEW_COLOR_ERROR_WASH   lv_color_hex(0x2B1614)

#define CANVIEW_RADIUS_SM 8
#define CANVIEW_RADIUS_MD 12
#define CANVIEW_RADIUS_LG 18
#define CANVIEW_TOUCH_PRIMARY 76
#define CANVIEW_TOUCH_SECONDARY 48

#endif
