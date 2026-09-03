#ifndef CANVIEW_THEME_H
#define CANVIEW_THEME_H

#include "lvgl.h"

/* tokens.css의 sRGB 근사값. 실제 패널 감마와 밝기에 맞춰 최종 보정한다. */
#define CANVIEW_COLOR_PAPER        lv_color_hex(0x02060C)
#define CANVIEW_COLOR_PAPER_2      lv_color_hex(0x06101C)
#define CANVIEW_COLOR_PAPER_3      lv_color_hex(0x0C1A29)
#define CANVIEW_COLOR_PAPER_4      lv_color_hex(0x142639)
#define CANVIEW_COLOR_INK          lv_color_hex(0xEDF4F7)
#define CANVIEW_COLOR_INK_2        lv_color_hex(0xBDC9D0)
#define CANVIEW_COLOR_MUTED        lv_color_hex(0x828F99)
#define CANVIEW_COLOR_RULE         lv_color_hex(0x24394B)
#define CANVIEW_COLOR_RULE_2       lv_color_hex(0x15293A)
#define CANVIEW_COLOR_ACCENT       lv_color_hex(0x16ACE2)
#define CANVIEW_COLOR_ACCENT_WASH  lv_color_hex(0x062740)
#define CANVIEW_COLOR_ACCENT_LINE  lv_color_hex(0x197BA4)
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
