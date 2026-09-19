/* SPDX-License-Identifier: GPL-3.0-only */
/** @file image_hooks.c @brief MCUboot 서명 검증 앞의 고정 CANView image profile 검사. */
#include "bootutil/boot_public_hooks.h"
#include "flash_map_backend/flash_map_backend.h"
#include "canview_boot_identity.h"
#include "native_stm.h"
#include "flash_layout.h"

#define PROTECTED_BYTES (8U + CANVIEW_OTA_NATIVE_METADATA_BYTES)
#define TLV_BYTES_MIN (88U)
#define TLV_BYTES_MAX (152U)
#define METADATA_SEQUENCE_OFFSET (32U)

static uint16_t image_u16(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8U));
}

static uint32_t image_u32(const uint8_t *data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8U) |
        ((uint32_t)data[2] << 16U) | ((uint32_t)data[3] << 24U);
}

/* 인증 완료로 취급하지 않는다. profile 일치 후에도 native hash/signature가 필수다. */
static bool image_profile_matches(const struct flash_area *area, uint32_t start)
{
    uint8_t header[CANVIEW_STM_IMAGE_HEADER_BYTES];
    uint8_t protected_tlv[PROTECTED_BYTES];
    uint8_t ordinary[TLV_BYTES_MAX];
    canview_ota_identity_t identity = {0};
    canview_ota_image_t expected = {0};
    uint32_t payload;
    uint32_t protected_offset;
    uint32_t ordinary_offset;
    uint16_t ordinary_bytes;
    if (canview_boot_identity_read(&identity, &expected.abi) != CANVIEW_OK ||
        identity.role != CANVIEW_OTA_ROLE_COMMUNICATOR ||
        flash_area_read(area, start, header, sizeof(header)) != 0)
    {
        return false;
    }
    if (image_u32(header) != IMAGE_MAGIC || image_u32(header + 4U) != 0U ||
        image_u16(header + 8U) != sizeof(header) || image_u16(header + 10U) != PROTECTED_BYTES ||
        image_u32(header + 16U) != 0U || image_u32(header + 28U) != 0U)
    {
        return false;
    }
    for (size_t index = 32U; index < sizeof(header); ++index)
    {
        if (header[index] != 0U) { return false; }
    }
    payload = image_u32(header + 12U);
    if (payload < 8U || payload > CANVIEW_STM_SIGNED_IMAGE_MAX_BYTES -
        CANVIEW_STM_IMAGE_HEADER_BYTES - PROTECTED_BYTES - TLV_BYTES_MIN)
    {
        return false;
    }
    /* start는 hook에서 0 또는 고정 secondary offset page로 제한한다. */
    protected_offset = start + CANVIEW_STM_IMAGE_HEADER_BYTES + payload;
    ordinary_offset = protected_offset + PROTECTED_BYTES;
    if (flash_area_read(area, protected_offset, protected_tlv, sizeof(protected_tlv)) != 0 ||
        image_u16(protected_tlv) != IMAGE_TLV_PROT_INFO_MAGIC ||
        image_u16(protected_tlv + 2U) != PROTECTED_BYTES ||
        image_u16(protected_tlv + 4U) != CANVIEW_OTA_NATIVE_TLV ||
        image_u16(protected_tlv + 6U) != CANVIEW_OTA_NATIVE_METADATA_BYTES ||
        flash_area_read(area, ordinary_offset, ordinary, 4U) != 0)
    {
        return false;
    }
    ordinary_bytes = image_u16(ordinary + 2U);
    if (image_u16(ordinary) != IMAGE_TLV_INFO_MAGIC || ordinary_bytes < TLV_BYTES_MIN ||
        ordinary_bytes > TLV_BYTES_MAX ||
        ordinary_bytes > CANVIEW_STM_SIGNED_IMAGE_MAX_BYTES - (ordinary_offset - start))
    {
        return false;
    }
    if (flash_area_read(area, ordinary_offset + 4U, ordinary + 4U, ordinary_bytes - 4U) != 0 ||
        image_u16(ordinary + 4U) != IMAGE_TLV_SHA256 || image_u16(ordinary + 6U) != 32U ||
        image_u16(ordinary + 40U) != IMAGE_TLV_KEYHASH || image_u16(ordinary + 42U) != 32U ||
        image_u16(ordinary + 76U) != IMAGE_TLV_ECDSA_SIG ||
        image_u16(ordinary + 78U) != ordinary_bytes - 80U)
    {
        return false;
    }
    expected.target = CANVIEW_OTA_TARGET_COMM_STM;
    expected.signature = CANVIEW_OTA_IMAGE_MCUBOOT_P256;
    /* 여기서는 identity만 대조한다. sequence는 서명으로 결합되지만 floor/manifest
     * 대조는 T-205 activation/confirmation 정책의 별도 gate이며 완료된 것이 아니다. */
    const uint8_t *metadata = protected_tlv + 8U;
    expected.release_sequence = (uint64_t)image_u32(metadata + METADATA_SEQUENCE_OFFSET) |
        ((uint64_t)image_u32(metadata + METADATA_SEQUENCE_OFFSET + 4U) << 32U);
    return canview_ota_native_metadata_check(metadata, CANVIEW_OTA_NATIVE_METADATA_BYTES,
        &identity, &expected) == CANVIEW_OK;
}

/* 공식 hook ABI: 성공이 아니라 REGULAR를 반환하여 native 검증을 반드시 이어간다.
 * FIH 객체/RET는 upstream 규칙을 따르며 반환형만 build 사본의 무수식 int ABI다. */
int boot_image_check_hook(int img_index, int slot)
{
    const struct flash_area *area = NULL;
    FIH_DECLARE(result, FIH_FAILURE);
    if (img_index != 0 || (slot != 0 && slot != 1) ||
        flash_area_open((uint8_t)(slot + 1), &area) != 0)
    {
        FIH_RET(result);
    }
    const uint32_t start = boot_get_state_secondary_offset(boot_get_loader_state(), area);
    if ((start == 0U || (slot == 1 && start == CANVIEW_STM_FLASH_PAGE_BYTES)) &&
        image_profile_matches(area, start))
    {
        result = FIH_BOOT_HOOK_REGULAR;
    }
    flash_area_close(area);
    FIH_RET(result);
}

int boot_read_image_header_hook(int img_index, int slot, struct image_header *header)
{
    (void)img_index;
    (void)slot;
    (void)header;
    return BOOT_HOOK_REGULAR;
}

int boot_perform_update_hook(int img_index, struct image_header *header, const struct flash_area *area)
{
    (void)img_index;
    (void)header;
    (void)area;
    return BOOT_HOOK_REGULAR;
}

int boot_read_swap_state_primary_slot_hook(int img_index, struct boot_swap_state *state)
{
    (void)img_index;
    (void)state;
    return BOOT_HOOK_REGULAR;
}

int boot_copy_region_post_hook(int img_index, const struct flash_area *area, size_t size)
{
    (void)img_index;
    (void)area;
    (void)size;
    return 0; /* 추가 후처리 없음. upstream 복사/검증을 대체하지 않는다. */
}
