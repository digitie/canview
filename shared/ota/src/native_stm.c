/* SPDX-License-Identifier: GPL-3.0-only */
/** @file native_stm.c @brief MCUboot v2.4.0 비압축·비암호화 P256 profile. */
#include <string.h>
#include "native_stm.h"

#define STM_MAGIC (UINT32_C(0x96F3B83D))
#define STM_BASE_HEADER_BYTES (32U)
#define STM_PROTECTED_BYTES (4U + 4U + CANVIEW_OTA_NATIVE_METADATA_BYTES)
#define STM_TLV_INFO_MAGIC (0x6907U)
#define STM_TLV_PROTECTED_MAGIC (0x6908U)
#define STM_TLV_SHA256 (0x0010U)
#define STM_TLV_KEYHASH (0x0001U)
#define STM_TLV_SIGNATURE (0x0022U)
#define STM_DER_MAX (72U)
#define STM_DER_MIN (8U)
#define STM_COORDINATE_BYTES (32U)

static uint16_t stm_u16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8U));
}
static uint32_t stm_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8U) | ((uint32_t)p[2] << 16U) | ((uint32_t)p[3] << 24U);
}
static uint64_t stm_u64(const uint8_t *p)
{
    return (uint64_t)stm_u32(p) | ((uint64_t)stm_u32(p + 4U) << 32U);
}

static bool stm_text_equal(const uint8_t *wire, const char *local)
{
    size_t end = 0U;
    while (end < CANVIEW_OTA_TEXT_BYTES && wire[end] != 0U)
    {
        if (wire[end] < 0x20U || wire[end] > 0x7EU || wire[end] != (uint8_t)local[end])
        {
            return false;
        }
        ++end;
    }
    if (end == 0U || end == CANVIEW_OTA_TEXT_BYTES || local[end] != '\0')
    {
        return false;
    }
    for (; end < CANVIEW_OTA_TEXT_BYTES; ++end)
    {
        if (wire[end] != 0U) { return false; }
    }
    return true;
}

static bool stm_version(const uint8_t *header, const char version[CANVIEW_OTA_TEXT_BYTES])
{
    const uint32_t fields[] = {header[20], header[21], stm_u16(header + 22U), stm_u32(header + 24U)};
    const char separators[] = {'.', '.', '+', '\0'};
    size_t offset = 0U;
    for (size_t field = 0U; field < sizeof(fields) / sizeof(fields[0]); ++field)
    {
        uint32_t value = 0U;
        const size_t start = offset;
        while (offset < CANVIEW_OTA_TEXT_BYTES && version[offset] >= '0' && version[offset] <= '9')
        {
            const uint32_t digit = (uint32_t)(version[offset] - '0');
            if (value > (UINT32_MAX - digit) / 10U) { return false; }
            value = value * 10U + digit;
            ++offset;
        }
        if (offset == start || offset >= CANVIEW_OTA_TEXT_BYTES ||
            (offset - start > 1U && version[start] == '0') ||
            value != fields[field] || version[offset] != separators[field])
        {
            return false;
        }
        ++offset;
    }
    return true;
}

/* DER 정수를 고정32byte로 옮길 뿐, 암호 연산은 SDK에 위임한다. */
static bool stm_der_integer(const uint8_t *der, size_t size, size_t *offset, uint8_t *out)
{
    if (*offset > size || size - *offset < 3U || der[*offset] != 0x02U)
    {
        return false;
    }
    size_t count = der[*offset + 1U];
    *offset += 2U;
    if (count == 0U || count > STM_COORDINATE_BYTES + 1U || count > size - *offset ||
        (der[*offset] & 0x80U) != 0U)
    {
        return false;
    }
    if (der[*offset] == 0U && count > 1U)
    {
        if ((der[*offset + 1U] & 0x80U) == 0U) { return false; }
        ++*offset;
        --count;
    }
    if (count > STM_COORDINATE_BYTES) { return false; }
    (void)memset(out, 0, STM_COORDINATE_BYTES);
    (void)memcpy(out + STM_COORDINATE_BYTES - count, der + *offset, count);
    *offset += count;
    return true;
}

static bool stm_signature(const uint8_t *der, size_t size, uint8_t raw[CANVIEW_OTA_ENVELOPE_SIGNATURE_BYTES])
{
    size_t offset = 2U;
    return size >= STM_DER_MIN && size <= STM_DER_MAX && der[0] == 0x30U &&
        der[1] == size - 2U && stm_der_integer(der, size, &offset, raw) &&
        stm_der_integer(der, size, &offset, raw + STM_COORDINATE_BYTES) && offset == size;
}

static bool stm_metadata(const uint8_t *metadata, const canview_ota_identity_t *identity,
                         const canview_ota_image_t *expected)
{
    static const uint8_t magic[8] = {'C', 'V', 'I', 'M', 'G', '0', '0', '1'};
    return memcmp(metadata, magic, sizeof(magic)) == 0 && stm_u16(metadata + 8U) == 1U &&
        stm_u16(metadata + 10U) == CANVIEW_OTA_NATIVE_METADATA_BYTES &&
        stm_u32(metadata + 12U) == (uint32_t)identity->role &&
        stm_u32(metadata + 16U) == (uint32_t)expected->target &&
        stm_u32(metadata + 20U) == identity->security_epoch &&
        stm_u32(metadata + 24U) == expected->abi && stm_u32(metadata + 28U) == 0U &&
        stm_u64(metadata + 32U) == expected->release_sequence &&
        stm_text_equal(metadata + 40U, identity->board_revision) &&
        stm_text_equal(metadata + 104U, identity->layout_id);
}

canview_status_t canview_ota_stm_image_check(
    const uint8_t *data, size_t size, const canview_ota_identity_t *identity,
    const canview_ota_image_t *expected, const canview_ota_native_crypto_t *crypto)
{
    if (data == NULL || identity == NULL || expected == NULL || crypto == NULL ||
        crypto->context == NULL || crypto->sha256 == NULL || crypto->verify == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (size > CANVIEW_OTA_STM_IMAGE_MAX) { return CANVIEW_OVERSIZE; }
    if (identity->role != CANVIEW_OTA_ROLE_COMMUNICATOR || expected->target != CANVIEW_OTA_TARGET_COMM_STM ||
        expected->signature != CANVIEW_OTA_IMAGE_MCUBOOT_P256 || size != expected->length)
    {
        return CANVIEW_AUTH_FAILED;
    }
    if (size < CANVIEW_OTA_STM_HEADER_BYTES + 8U + STM_PROTECTED_BYTES + 4U)
    {
        return CANVIEW_INCOMPLETE;
    }
    if (stm_u32(data) != STM_MAGIC || stm_u32(data + 4U) != 0U ||
        stm_u16(data + 8U) != CANVIEW_OTA_STM_HEADER_BYTES ||
        stm_u16(data + 10U) != STM_PROTECTED_BYTES || stm_u32(data + 16U) != 0U || stm_u32(data + 28U) != 0U)
    {
        return CANVIEW_MALFORMED;
    }
    const uint32_t image_size = stm_u32(data + 12U);
    if (image_size < 8U || image_size > size - CANVIEW_OTA_STM_HEADER_BYTES - STM_PROTECTED_BYTES - 4U)
    {
        return CANVIEW_MALFORMED;
    }
    for (size_t index = STM_BASE_HEADER_BYTES; index < CANVIEW_OTA_STM_HEADER_BYTES; ++index)
    {
        if (data[index] != 0U) { return CANVIEW_MALFORMED; }
    }
    const size_t protected_offset = CANVIEW_OTA_STM_HEADER_BYTES + image_size;
    const uint8_t *protected_tlv = data + protected_offset;
    if (stm_u16(protected_tlv) != STM_TLV_PROTECTED_MAGIC || stm_u16(protected_tlv + 2U) != STM_PROTECTED_BYTES ||
        stm_u16(protected_tlv + 4U) != CANVIEW_OTA_NATIVE_TLV ||
        stm_u16(protected_tlv + 6U) != CANVIEW_OTA_NATIVE_METADATA_BYTES)
    {
        return CANVIEW_MALFORMED;
    }
    if (!stm_metadata(protected_tlv + 8U, identity, expected) || !stm_version(data, expected->version))
    {
        return CANVIEW_AUTH_FAILED;
    }
    const size_t signed_size = protected_offset + STM_PROTECTED_BYTES;
    const uint8_t *tlv = data + signed_size;
    const size_t tlv_size = size - signed_size;
    /* SHA256(36), KEYHASH(36), signature header(4), info(4), DER(8..72). */
    if (tlv_size < 80U + STM_DER_MIN || tlv_size > 80U + STM_DER_MAX ||
        stm_u16(tlv) != STM_TLV_INFO_MAGIC || stm_u16(tlv + 2U) != tlv_size ||
        stm_u16(tlv + 4U) != STM_TLV_SHA256 || stm_u16(tlv + 6U) != CANVIEW_OTA_DIGEST_BYTES ||
        stm_u16(tlv + 40U) != STM_TLV_KEYHASH || stm_u16(tlv + 42U) != CANVIEW_OTA_DIGEST_BYTES ||
        stm_u16(tlv + 76U) != STM_TLV_SIGNATURE || stm_u16(tlv + 78U) != tlv_size - 80U)
    {
        return CANVIEW_MALFORMED;
    }
    uint8_t raw[CANVIEW_OTA_ENVELOPE_SIGNATURE_BYTES];
    uint8_t digest[CANVIEW_OTA_DIGEST_BYTES];
    if (!stm_signature(tlv + 80U, tlv_size - 80U, raw)) { return CANVIEW_MALFORMED; }
    if (memcmp(tlv + 44U, crypto->key_digest, CANVIEW_OTA_DIGEST_BYTES) != 0) { return CANVIEW_AUTH_FAILED; }
    canview_status_t status = crypto->sha256(crypto->context, data, size, digest);
    if (status != CANVIEW_OK) { return status; }
    if (memcmp(digest, expected->sha256, sizeof(digest)) != 0) { return CANVIEW_AUTH_FAILED; }
    status = crypto->sha256(crypto->context, data, signed_size, digest);
    if (status != CANVIEW_OK) { return status; }
    if (memcmp(digest, tlv + 8U, sizeof(digest)) != 0) { return CANVIEW_AUTH_FAILED; }
    return crypto->verify(crypto->context, digest, raw);
}
