#include "canview_controller_can.h"

#include <string.h>

static uint32_t read_le32(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] |
           ((uint32_t)bytes[1] << 8U) |
           ((uint32_t)bytes[2] << 16U) |
           ((uint32_t)bytes[3] << 24U);
}

static uint32_t mask_for_length(uint8_t bit_length)
{
    if (bit_length >= 32U) {
        return UINT32_MAX;
    }
    return (UINT32_C(1) << bit_length) - 1U;
}

static bool signal_matches(const canview_can_record_t *record,
                           const canview_controller_signal_descriptor_t *descriptor)
{
    const uint32_t can_id = read_le32((const uint8_t *)&record->can_id_le);
    return (descriptor->bus_id == CANVIEW_CAN_BUS_ANY ||
            descriptor->bus_id == record->bus_id) &&
           (can_id & descriptor->can_id_mask) ==
               (descriptor->can_id & descriptor->can_id_mask);
}

static bool extract_little_endian(const uint8_t *data, uint8_t data_len,
                                  uint8_t start_bit, uint8_t bit_length,
                                  uint64_t *raw)
{
    if (bit_length == 0U || bit_length > 32U || start_bit >= 64U ||
        (uint32_t)start_bit + (uint32_t)bit_length > (uint32_t)data_len * 8U) {
        return false;
    }
    uint64_t value = 0U;
    for (uint8_t i = 0; i < bit_length; ++i) {
        const uint8_t bit = (uint8_t)(start_bit + i);
        value |= (uint64_t)((data[bit / 8U] >> (bit % 8U)) & 1U) << i;
    }
    *raw = value;
    return true;
}

static bool extract_big_endian(const uint8_t *data, uint8_t data_len,
                               uint8_t start_bit, uint8_t bit_length,
                               uint64_t *raw)
{
    if (bit_length == 0U || bit_length > 32U || start_bit >= 64U) {
        return false;
    }
    uint8_t byte = (uint8_t)(start_bit / 8U);
    int8_t bit = (int8_t)(start_bit % 8U);
    uint64_t value = 0U;
    for (uint8_t i = 0; i < bit_length; ++i) {
        if (byte >= data_len) {
            return false;
        }
        value = (value << 1U) | (uint64_t)((data[byte] >> bit) & 1U);
        if (bit == 0) {
            ++byte;
            bit = 7;
        } else {
            --bit;
        }
    }
    *raw = value;
    return true;
}

static int32_t sign_extend(uint64_t raw, uint8_t bit_length)
{
    const uint32_t value = (uint32_t)raw;
    const uint32_t mask = mask_for_length(bit_length);
    const uint32_t sign = UINT32_C(1) << (bit_length - 1U);
    const uint32_t extended = (value & sign) != 0U ? value | ~mask : value;
    return (int32_t)extended;
}

bool canview_controller_decode_signal(
    const canview_can_record_t *record,
    const canview_controller_signal_descriptor_t *descriptor,
    canview_controller_decoded_signal_t *decoded)
{
    if (record == NULL || descriptor == NULL || decoded == NULL ||
        descriptor->can_id_mask == 0U ||
        descriptor->bit_length == 0U || descriptor->bit_length > 32U ||
        descriptor->byte_order > CANVIEW_CONTROLLER_SIGNAL_BIG_ENDIAN ||
        descriptor->factor == 0.0F || !signal_matches(record, descriptor)) {
        return false;
    }

    const uint8_t data_len = (uint8_t)(record->flags_dlc & UINT8_C(0x0F));
    if (data_len > 8U) {
        return false;
    }
    uint64_t raw = 0U;
    const bool extracted = descriptor->byte_order == CANVIEW_CONTROLLER_SIGNAL_LITTLE_ENDIAN
                               ? extract_little_endian(record->data, data_len,
                                                        descriptor->start_bit,
                                                        descriptor->bit_length, &raw)
                               : extract_big_endian(record->data, data_len,
                                                    descriptor->start_bit,
                                                    descriptor->bit_length, &raw);
    if (!extracted) {
        return false;
    }

    const float physical = (descriptor->is_signed
                                ? (float)sign_extend(raw, descriptor->bit_length)
                                : (float)(uint32_t)raw) * descriptor->factor +
                           descriptor->offset;
    *decoded = (canview_controller_decoded_signal_t){
        .signal_id = descriptor->signal_id,
        .value_type = descriptor->value_type,
        .quality = descriptor->quality,
        .age_ms = 0U,
        .raw_value = raw,
        .physical_value = physical,
    };
    if (descriptor->has_range &&
        (physical < descriptor->minimum || physical > descriptor->maximum)) {
        decoded->quality = CANVIEW_QUALITY_OUT_OF_RANGE;
    }

    if (descriptor->value_type == CANVIEW_VALUE_F32) {
        uint32_t bits = 0U;
        memcpy(&bits, &physical, sizeof(bits));
        decoded->value_bits = bits;
    } else if (descriptor->value_type == CANVIEW_VALUE_I32) {
        decoded->value_bits = (uint32_t)(int32_t)physical;
    } else if (descriptor->value_type == CANVIEW_VALUE_BOOL) {
        decoded->value_bits = physical != 0.0F ? 1U : 0U;
    } else {
        decoded->value_bits = (uint32_t)physical;
    }
    return true;
}
