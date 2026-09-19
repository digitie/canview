/* SPDX-License-Identifier: GPL-3.0-only */
/** @file metadata.c @brief 서명 golden 준비용 합성 descriptor. 제품 identity가 아니다. */
#include <stddef.h>
#include <stdint.h>
#include "native_metadata.h"

#define FIXTURE_METADATA_VERSION (1U)
#define FIXTURE_EPOCH (7U)
#define FIXTURE_ABI (2U)
#define FIXTURE_BOARD_OFFSET (40U)
#define FIXTURE_LAYOUT_OFFSET (104U)

/* ESP-IDF가 보존하는 rodata byte열만 생성한다. portable parser의 struct cast가 아니다. */
typedef struct
{
    uint8_t magic[8];
    uint16_t version;
    uint16_t size;
    uint32_t role;
    uint32_t target;
    uint32_t epoch;
    uint32_t abi;
    uint32_t reserved;
    uint64_t sequence;
    char board[CANVIEW_OTA_TEXT_BYTES];
    char layout[CANVIEW_OTA_TEXT_BYTES];
} fixture_metadata_t;

_Static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__, "fixture metadata requires little endian");
_Static_assert(sizeof(fixture_metadata_t) == CANVIEW_OTA_NATIVE_METADATA_BYTES, "metadata size drift");
_Static_assert(offsetof(fixture_metadata_t, board) == FIXTURE_BOARD_OFFSET, "board offset drift");
_Static_assert(offsetof(fixture_metadata_t, layout) == FIXTURE_LAYOUT_OFFSET, "layout offset drift");

/* Linker -u로 보존하는 const symbol. main task/Flash/키에 대한 권한은 없다. */
extern const fixture_metadata_t canview_ota_fixture_metadata;
const fixture_metadata_t canview_ota_fixture_metadata
    __attribute__((section(".rodata_custom_desc"), used)) =
{
    .magic = {'C', 'V', 'I', 'M', 'G', '0', '0', '1'},
    .version = FIXTURE_METADATA_VERSION,
    .size = CANVIEW_OTA_NATIVE_METADATA_BYTES,
    .role = CANVIEW_OTA_ROLE_COMMUNICATOR,
    .target = CANVIEW_OTA_TARGET_COMM_ESP,
    .epoch = FIXTURE_EPOCH,
    .abi = FIXTURE_ABI,
    .reserved = 0U,
    .sequence = UINT64_MAX,
    .board = "synthetic-board",
    .layout = "synthetic-layout"
};
