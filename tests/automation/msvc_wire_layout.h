#ifndef CANVIEW_AUTOMATION_MSVC_WIRE_LAYOUT_H
#define CANVIEW_AUTOMATION_MSVC_WIRE_LAYOUT_H

/* v1.2 draft의 GNU packed에 대응하는 host 시험 전용 경계다.
 * runtime ABI 이식 완료를 뜻하지 않으며 protocol 생성기 변경은 T-002 범위다. */
#include <stdint.h>
#pragma pack(push, 1)
#include "canview_protocol.h"
#pragma pack(pop)

#endif
