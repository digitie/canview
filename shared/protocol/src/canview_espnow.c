/* SPDX-License-Identifier: GPL-3.0-only */
/** @file canview_espnow.c
 * @brief ESP-NOW v1.3 codec, session, security adapters and bounded QoS.
 */
#include "canview_espnow.h"

#include <string.h>

#define ESPNOW_HEADER_SIZE (32U)
#define ESPNOW_CRC_OFFSET (28U)
#define ESPNOW_CRC_SIZE (4U)
#define ESPNOW_SEQUENCE_HALF UINT32_C(0x80000000)
#define ESPNOW_CONTROL_COMMAND_ID_OFFSET (8U)
#define ESPNOW_CONTROL_COMMAND_TAG_OFFSET (56U)
#define ESPNOW_CONTROL_COMMAND_PREFIX_SIZE (56U)
#define ESPNOW_CONTROL_LEASE_TAG_OFFSET (36U)
#define ESPNOW_CONTROL_LEASE_PREFIX_SIZE (36U)
#define ESPNOW_CONTROL_COMMAND_ARGUMENT_LENGTH_OFFSET (52U)
#define ESPNOW_CONTROL_COMMAND_PREFIX_PAYLOAD_SIZE (72U)
#define ESPNOW_CONTROL_CANONICAL_DOMAIN "CV-CONTROL-1"
#define ESPNOW_PAIRING_CANONICAL_DOMAIN_SEPARATOR (0U)
#define ESPNOW_DEFAULT_ERROR_RATE_LIMIT (16U)
#define ESPNOW_AUTH_BACKOFF_BASE_MS (1000U)
#define ESPNOW_AUTH_BACKOFF_MAX_MS (60000U)
#define ESPNOW_TIME_SYNC_SAMPLE_TARGET (3U)

static uint16_t read_le16(const uint8_t *bytes)
{
    return (uint16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8U));
}

static uint32_t read_le32(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8U) |
           ((uint32_t)bytes[2] << 16U) | ((uint32_t)bytes[3] << 24U);
}

static uint64_t read_le64(const uint8_t *bytes)
{
    uint64_t value = 0U;
    for (size_t index = 0U; index < 8U; ++index)
    {
        value |= (uint64_t)bytes[index] << (index * 8U);
    }
    return value;
}

static void write_le16(uint8_t *bytes, uint16_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8U);
}

static void write_le32(uint8_t *bytes, uint32_t value)
{
    for (size_t index = 0U; index < 4U; ++index)
    {
        bytes[index] = (uint8_t)(value >> (index * 8U));
    }
}

static uint32_t crc_step(uint32_t crc, uint8_t byte)
{
    crc ^= byte;
    for (uint32_t bit = 0U; bit < 8U; ++bit)
    {
        crc = (crc >> 1U) ^ ((crc & 1U) != 0U ? UINT32_C(0xEDB88320) : 0U);
    }
    return crc;
}

static uint32_t frame_crc(const uint8_t *bytes, size_t length)
{
    uint32_t crc = UINT32_C(0xFFFFFFFF);
    for (size_t index = 0U; index < length; ++index)
    {
        const bool is_crc = index >= ESPNOW_CRC_OFFSET && index < ESPNOW_CRC_OFFSET + ESPNOW_CRC_SIZE;
        crc = crc_step(crc, is_crc ? 0U : bytes[index]);
    }
    return crc ^ UINT32_C(0xFFFFFFFF);
}

static bool constant_time_equal(const uint8_t *left, const uint8_t *right, size_t length)
{
    uint8_t difference = 0U;
    for (size_t index = 0U; index < length; ++index)
    {
        difference |= (uint8_t)(left[index] ^ right[index]);
    }
    return difference == 0U;
}

static bool valid_role(canview_role_t role)
{
    return (uint32_t)role <= (uint32_t)CANVIEW_ROLE_DIAGNOSTIC_BRIDGE;
}

static bool valid_state(canview_link_state_t state)
{
    return (uint32_t)state <= (uint32_t)CANVIEW_LINK_READ_ONLY_INCOMPATIBLE;
}

static uint8_t role_bit(canview_role_t role)
{
    return (uint8_t)(UINT8_C(1) << (uint32_t)role);
}

static uint16_t state_bit(canview_link_state_t state)
{
    return (uint16_t)(UINT16_C(1) << (uint32_t)state);
}

static const canview_espnow_message_contract_t *find_contract(uint8_t message_type)
{
    for (size_t index = 0U; index < canview_espnow_message_contract_count; ++index)
    {
        if (canview_espnow_message_contracts[index].message_type == message_type)
        {
            return &canview_espnow_message_contracts[index];
        }
    }
    return NULL;
}

static bool read_scalar(const uint8_t *bytes, size_t width, uint64_t *value)
{
    if (bytes == NULL || value == NULL ||
        (width != 1U && width != 2U && width != 4U && width != 8U))
    {
        return false;
    }
    *value = 0U;
    for (size_t index = 0U; index < width; ++index)
    {
        *value |= (uint64_t)bytes[index] << (index * 8U);
    }
    return true;
}

static bool contract_variant(
    const canview_espnow_message_contract_t *contract, size_t payload_size,
    uint8_t requested_variant, uint8_t *variant_index)
{
    if (contract == NULL || variant_index == NULL)
    {
        return false;
    }
    if (contract->payload_kind != CANVIEW_CONTRACT_PAYLOAD_VARIANTS)
    {
        if (requested_variant != 0U && requested_variant != CANVIEW_ESPNOW_CONTRACT_VARIANT_NONE)
        {
            return false;
        }
        *variant_index = 0U;
        return payload_size >= contract->payload_min && payload_size <= contract->payload_max;
    }
    size_t matches = 0U;
    uint8_t selected = CANVIEW_ESPNOW_CONTRACT_VARIANT_NONE;
    for (uint8_t index = 0U; index < contract->variant_count; ++index)
    {
        if (payload_size == contract->variant_sizes[index])
        {
            ++matches;
            selected = index;
        }
    }
    if (requested_variant != CANVIEW_ESPNOW_CONTRACT_VARIANT_NONE)
    {
        if (requested_variant >= contract->variant_count ||
            payload_size != contract->variant_sizes[requested_variant])
        {
            return false;
        }
        selected = requested_variant;
        matches = 1U;
    }
    if (matches != 1U)
    {
        return false;
    }
    *variant_index = selected;
    return true;
}

static uint8_t effective_ack_required(
    const canview_espnow_message_contract_t *contract, uint8_t variant_index)
{
    if (contract->payload_kind == CANVIEW_CONTRACT_PAYLOAD_VARIANTS &&
        variant_index < contract->variant_count)
    {
        return contract->variant_ack_required[variant_index];
    }
    return contract->ack_required;
}

static uint8_t effective_response(
    const canview_espnow_message_contract_t *contract, uint8_t variant_index)
{
    if (contract->payload_kind == CANVIEW_CONTRACT_PAYLOAD_VARIANTS &&
        variant_index < contract->variant_count)
    {
        return contract->variant_response[variant_index];
    }
    return contract->response;
}

static bool effective_roles(
    const canview_espnow_message_contract_t *contract, uint8_t variant_index,
    uint8_t *sender_mask, uint8_t *receiver_mask)
{
    if (contract == NULL || sender_mask == NULL || receiver_mask == NULL)
    {
        return false;
    }
    if (contract->payload_kind == CANVIEW_CONTRACT_PAYLOAD_VARIANTS &&
        variant_index < contract->variant_count)
    {
        *sender_mask = contract->variant_sender_role_masks[variant_index];
        *receiver_mask = contract->variant_receiver_role_masks[variant_index];
    }
    else
    {
        *sender_mask = contract->sender_role_mask;
        *receiver_mask = contract->receiver_role_mask;
    }
    return true;
}

static const canview_espnow_tlv_contract_t *find_tlv_contract(
    const canview_espnow_message_contract_t *contract, uint16_t type)
{
    if (contract == NULL ||
        (size_t)contract->tlv_contract_index + (size_t)contract->tlv_contract_count >
            canview_espnow_tlv_contract_count)
    {
        return NULL;
    }
    for (size_t index = 0U; index < contract->tlv_contract_count; ++index)
    {
        const canview_espnow_tlv_contract_t *candidate =
            &canview_espnow_tlv_contracts[contract->tlv_contract_index + index];
        if (candidate->type == type)
        {
            return candidate;
        }
    }
    return NULL;
}

static bool tlv_singleton_seen(
    const uint8_t *payload, size_t first_offset, size_t current_offset, uint16_t type)
{
    size_t offset = first_offset;
    while (offset < current_offset)
    {
        if (current_offset - offset < CANVIEW_ESPNOW_CONTRACT_TLV_HEADER_SIZE)
        {
            return false;
        }
        const uint16_t previous_type = read_le16(payload + offset);
        const uint16_t previous_length = read_le16(
            payload + offset + CANVIEW_ESPNOW_CONTRACT_TLV_HEADER_SIZE - 2U);
        offset += CANVIEW_ESPNOW_CONTRACT_TLV_HEADER_SIZE;
        if ((size_t)previous_length > current_offset - offset)
        {
            return false;
        }
        if (previous_type == type)
        {
            return true;
        }
        offset += (size_t)previous_length;
    }
    return false;
}

static bool valid_tlv_payload(
    const canview_espnow_message_contract_t *contract, const uint8_t *payload,
    size_t payload_size)
{
    if (contract == NULL || payload == NULL || payload_size < contract->prefix_size ||
        (size_t)contract->tlv_contract_index + (size_t)contract->tlv_contract_count >
            canview_espnow_tlv_contract_count)
    {
        return false;
    }
    const size_t first_offset = contract->prefix_size;
    size_t offset = first_offset;
    while (offset < payload_size)
    {
        if (payload_size - offset < CANVIEW_ESPNOW_CONTRACT_TLV_HEADER_SIZE)
        {
            return false;
        }
        const uint16_t type = read_le16(payload + offset);
        const uint16_t value_length = read_le16(
            payload + offset + CANVIEW_ESPNOW_CONTRACT_TLV_HEADER_SIZE - 2U);
        offset += CANVIEW_ESPNOW_CONTRACT_TLV_HEADER_SIZE;
        if ((size_t)value_length > payload_size - offset)
        {
            return false;
        }
        const canview_espnow_tlv_contract_t *definition = find_tlv_contract(contract, type);
        if (definition == NULL)
        {
            if ((type & CANVIEW_ESPNOW_CONTRACT_TLV_CRITICAL_BIT) != 0U)
            {
                return false;
            }
        }
        else if ((definition->size != CANVIEW_ESPNOW_CONTRACT_TLV_VARIABLE_SIZE &&
                  definition->size != value_length) ||
                 (definition->singleton != 0U &&
                  tlv_singleton_seen(payload, first_offset, offset -
                                     CANVIEW_ESPNOW_CONTRACT_TLV_HEADER_SIZE, type)))
        {
            return false;
        }
        offset += (size_t)value_length;
    }
    return offset == payload_size;
}

static bool validate_payload_shape(
    const canview_espnow_message_contract_t *contract, const uint8_t *payload,
    size_t payload_size, uint8_t variant_index)
{
    if (contract == NULL || payload == NULL || payload_size > CANVIEW_MAX_PAYLOAD_SIZE ||
        payload_size < contract->payload_min || payload_size > contract->payload_max)
    {
        return false;
    }
    if (contract->payload_kind == CANVIEW_CONTRACT_PAYLOAD_FIXED)
    {
        return payload_size == contract->payload_min;
    }
    if (contract->payload_kind == CANVIEW_CONTRACT_PAYLOAD_BOUNDED)
    {
        uint64_t count = 0U;
        if (contract->count_width == 0U ||
            !read_scalar(payload + contract->count_offset, contract->count_width, &count) ||
            count > contract->count_max)
        {
            return false;
        }
        const size_t expected = (size_t)contract->prefix_size + (size_t)count * contract->record_size;
        return expected == payload_size;
    }
    if (contract->payload_kind == CANVIEW_CONTRACT_PAYLOAD_SUFFIX)
    {
        uint64_t suffix_length = 0U;
        if (contract->length_width == 0U ||
            !read_scalar(payload + contract->length_offset, contract->length_width, &suffix_length))
        {
            return false;
        }
        return suffix_length == payload_size - contract->prefix_size &&
               (contract->tlv_contract_count == 0U || valid_tlv_payload(contract, payload, payload_size));
    }
    if (contract->payload_kind == CANVIEW_CONTRACT_PAYLOAD_TLV)
    {
        return valid_tlv_payload(contract, payload, payload_size);
    }
    return variant_index < contract->variant_count &&
           payload_size == contract->variant_sizes[variant_index];
}

static bool validate_field_constraints(
    const canview_espnow_message_contract_t *contract, const uint8_t *payload,
    size_t payload_size, uint8_t variant_index)
{
    if (contract == NULL || payload == NULL)
    {
        return false;
    }
    uint64_t count = 0U;
    if (contract->payload_kind == CANVIEW_CONTRACT_PAYLOAD_BOUNDED &&
        !read_scalar(payload + contract->count_offset, contract->count_width, &count))
    {
        return false;
    }
    for (size_t rule_index = 0U; rule_index < canview_espnow_field_constraint_count; ++rule_index)
    {
        const canview_espnow_field_constraint_t *rule = &canview_espnow_field_constraints[rule_index];
        if (rule->message_type != contract->message_type ||
            (rule->variant_index != CANVIEW_ESPNOW_CONTRACT_VARIANT_NONE &&
             rule->variant_index != variant_index))
        {
            continue;
        }
        const size_t repetitions = rule->region == 0U ? 1U : (size_t)count;
        for (size_t repetition = 0U; repetition < repetitions; ++repetition)
        {
            const size_t base = rule->region == 0U
                                    ? 0U
                                    : (size_t)contract->prefix_size + repetition * rule->stride;
            if (base > payload_size || (size_t)rule->offset > payload_size - base ||
                (size_t)rule->width > payload_size - base - rule->offset)
            {
                return false;
            }
            const uint8_t *field = payload + base + rule->offset;
            if (rule->kind == CANVIEW_CONTRACT_FIELD_RESERVED)
            {
                for (size_t byte_index = 0U; byte_index < rule->width; ++byte_index)
                {
                    if (field[byte_index] != 0U)
                    {
                        return false;
                    }
                }
                continue;
            }
            uint64_t value = 0U;
            if (!read_scalar(field, rule->width, &value))
            {
                return false;
            }
            if (rule->kind == CANVIEW_CONTRACT_FIELD_ENUM)
            {
                bool found = false;
                const size_t end = (size_t)rule->allowed_index + rule->allowed_count;
                if (end > canview_espnow_allowed_value_count)
                {
                    return false;
                }
                for (size_t value_index = rule->allowed_index; value_index < end; ++value_index)
                {
                    if (value == canview_espnow_allowed_values[value_index])
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    return false;
                }
            }
            else if (rule->kind == CANVIEW_CONTRACT_FIELD_BITMASK)
            {
                if ((value & (uint64_t)~rule->mask) != 0U)
                {
                    return false;
                }
            }
            else if (rule->kind == CANVIEW_CONTRACT_FIELD_RANGE &&
                     (value < rule->minimum || value > rule->maximum))
            {
                return false;
            }
        }
    }
    return true;
}

static bool validate_payload_semantics(
    const canview_espnow_message_contract_t *contract, const uint8_t *payload,
    size_t payload_size, uint8_t variant_index, const canview_peer_session_t *session,
    canview_role_t sender_role, uint32_t now_ms)
{
    if (!validate_payload_shape(contract, payload, payload_size, variant_index) ||
        !validate_field_constraints(contract, payload, payload_size, variant_index))
    {
        return false;
    }
    if (contract->message_type == CANVIEW_MSG_HELLO && payload[32U] != (uint8_t)sender_role)
    {
        return false;
    }
    if (contract->message_type == CANVIEW_MSG_CAPABILITIES)
    {
        if (payload[16U] != (uint8_t)sender_role)
        {
            return false;
        }
        const uint16_t scope = read_le16(payload + 24U);
        if ((payload[16U] == (uint8_t)CANVIEW_ROLE_READ_ONLY_CONTROLLER ||
             payload[16U] == (uint8_t)CANVIEW_ROLE_DIAGNOSTIC_BRIDGE) &&
            scope != 0U)
        {
            return false;
        }
    }
    if (session != NULL && session->local_authorization_valid &&
        (contract->message_type == CANVIEW_MSG_PAIR_CHALLENGE ||
         contract->message_type == CANVIEW_MSG_PAIR_CONFIRM ||
         contract->message_type == CANVIEW_MSG_PAIR_RESULT))
    {
        size_t role_offset = 0U;
        size_t scope_offset = 0U;
        bool has_scope = false;
        if (contract->message_type == CANVIEW_MSG_PAIR_CHALLENGE)
        {
            role_offset = 84U;
            scope_offset = 86U;
            has_scope = true;
        }
        else if (contract->message_type == CANVIEW_MSG_PAIR_CONFIRM)
        {
            role_offset = 34U;
        }
        else
        {
            role_offset = 9U;
        }
        if (payload[role_offset] != (uint8_t)session->authorized_role ||
            (has_scope && read_le16(payload + scope_offset) != session->authorized_scope))
        {
            return false;
        }
    }
    if (contract->message_type == CANVIEW_MSG_COMMAND_REQUEST && session != NULL &&
        !canview_control_time_sync_is_valid(&session->control_time_sync, now_ms))
    {
        return false;
    }
    return true;
}

static void header_from_wire(const uint8_t *bytes, canview_wire_header_t *header)
{
    header->message_type = bytes[5U];
    header->flags = bytes[6U];
    header->priority = bytes[7U];
    header->session_id = read_le32(bytes + 8U);
    header->sequence = read_le32(bytes + 12U);
    header->sender_time = read_le32(bytes + 16U);
    header->correlation_id = read_le32(bytes + 20U);
}

static void header_to_wire(const canview_wire_header_t *header, uint8_t *bytes, size_t payload_size)
{
    write_le16(bytes, CANVIEW_MAGIC_LE);
    bytes[2U] = CANVIEW_PROTOCOL_MAJOR;
    bytes[3U] = CANVIEW_PROTOCOL_MINOR;
    bytes[4U] = (uint8_t)CANVIEW_HEADER_SIZE;
    bytes[5U] = header->message_type;
    bytes[6U] = header->flags;
    bytes[7U] = header->priority;
    write_le32(bytes + 8U, header->session_id);
    write_le32(bytes + 12U, header->sequence);
    write_le32(bytes + 16U, (uint32_t)header->sender_time);
    write_le32(bytes + 20U, header->correlation_id);
    write_le16(bytes + 24U, (uint16_t)payload_size);
    write_le16(bytes + 26U, 0U);
    write_le32(bytes + 28U, 0U);
}

static canview_decode_reason_t validate_policy(
    const canview_transport_meta_t *meta, const canview_peer_session_t *session,
    const canview_espnow_message_contract_t *contract,
    const canview_wire_header_t *header, const uint8_t *payload, size_t payload_size,
    uint8_t variant_index)
{
    if (meta == NULL || contract == NULL || header == NULL || payload == NULL ||
        !valid_role(meta->sender_role) || !valid_role(meta->receiver_role) ||
        !valid_state(meta->link_state))
    {
        return CANVIEW_DECODE_REASON_INVALID_ARGUMENT;
    }
    uint8_t sender_mask = 0U;
    uint8_t receiver_mask = 0U;
    if (!effective_roles(contract, variant_index, &sender_mask, &receiver_mask) ||
        (sender_mask & role_bit(meta->sender_role)) == 0U ||
        (receiver_mask & role_bit(meta->receiver_role)) == 0U)
    {
        return CANVIEW_DECODE_REASON_ROLE_MISMATCH;
    }
    if ((contract->state_mask & state_bit(meta->link_state)) == 0U)
    {
        return CANVIEW_DECODE_REASON_STATE_MISMATCH;
    }
    const uint8_t ack_required = effective_ack_required(contract, variant_index);
    const bool response_flag = (header->flags & CANVIEW_FLAG_RESPONSE) != 0U;
    const bool response_required = effective_response(contract, variant_index) != 0U;
    if (((header->flags & CANVIEW_FLAG_ACK_REQUIRED) != 0U) != (ack_required != 0U) ||
        ((header->flags & CANVIEW_FLAG_BROADCAST) != 0U) != (contract->broadcast != 0U) ||
        ((header->flags & CANVIEW_FLAG_FRAGMENT) != 0U) != (contract->fragment != 0U) ||
        ((header->flags & CANVIEW_FLAG_LAST_FRAGMENT) != 0U && !contract->fragment) ||
        (header->flags & (uint8_t)~CANVIEW_PROTOCOL_KNOWN_FLAG_MASK) != 0U ||
        header->priority != contract->priority ||
        (contract->payload_kind == CANVIEW_CONTRACT_PAYLOAD_VARIANTS &&
         response_flag != response_required))
    {
        return CANVIEW_DECODE_REASON_BAD_FLAGS;
    }
    if ((header->flags & CANVIEW_FLAG_ACK_REQUIRED) != 0U &&
        (header->message_type == CANVIEW_MSG_ACK || header->message_type == CANVIEW_MSG_ERROR))
    {
        return CANVIEW_DECODE_REASON_BAD_FLAGS;
    }
    if ((header->flags & CANVIEW_FLAG_READ_ONLY) != 0U &&
        meta->sender_role != CANVIEW_ROLE_READ_ONLY_CONTROLLER &&
        meta->sender_role != CANVIEW_ROLE_DIAGNOSTIC_BRIDGE)
    {
        return CANVIEW_DECODE_REASON_ROLE_MISMATCH;
    }
    if (contract->session_zero)
    {
        if (header->session_id != 0U)
        {
            return CANVIEW_DECODE_REASON_SESSION_MISMATCH;
        }
        if (contract->authenticated_session_zero &&
            (!meta->encrypted || !meta->peer_authenticated))
        {
            return CANVIEW_DECODE_REASON_ENCRYPTION_REQUIRED;
        }
    }
    else
    {
        const uint32_t expected_session = meta->expected_session_id != 0U
                                               ? meta->expected_session_id
                                               : (session != NULL ? session->session_id : 0U);
        if (header->session_id == 0U || (expected_session != 0U && header->session_id != expected_session))
        {
            return CANVIEW_DECODE_REASON_SESSION_MISMATCH;
        }
    }
    if (contract->encrypted && (!meta->encrypted || !meta->peer_authenticated || !meta->peer_known))
    {
        return CANVIEW_DECODE_REASON_ENCRYPTION_REQUIRED;
    }
    if (!validate_payload_semantics(contract, payload, payload_size, variant_index, session,
                                    meta->sender_role, meta->now_ms))
    {
        if (contract->message_type == CANVIEW_MSG_COMMAND_REQUEST && session != NULL &&
            !canview_control_time_sync_is_valid(&session->control_time_sync, meta->now_ms))
        {
            return CANVIEW_DECODE_REASON_TIME_SYNC_STALE;
        }
        return CANVIEW_DECODE_REASON_SEMANTIC;
    }
    return CANVIEW_DECODE_REASON_NONE;
}

static canview_decode_result_t decode_failure(
    const canview_transport_meta_t *meta, canview_peer_session_t *session,
    canview_decoded_frame_t *out, canview_decode_reason_t reason)
{
    if (out != NULL)
    {
        out->reason = reason;
    }
    if (meta == NULL || meta->source_is_broadcast || session == NULL ||
        !canview_error_rate_limiter_allow(&session->error_limiter, meta->now_ms))
    {
        return CANVIEW_DECODE_DROP_SILENT;
    }
    return CANVIEW_DECODE_ERROR_RATE_LIMITED;
}

canview_status_t canview_frame_recalculate_crc(uint8_t *bytes, size_t length)
{
    if (bytes == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (length < ESPNOW_HEADER_SIZE || length > CANVIEW_MAX_FRAME_SIZE)
    {
        return CANVIEW_MALFORMED;
    }
    write_le32(bytes + ESPNOW_CRC_OFFSET, 0U);
    write_le32(bytes + ESPNOW_CRC_OFFSET, frame_crc(bytes, length));
    return CANVIEW_OK;
}

canview_decode_result_t canview_frame_decode(
    const canview_transport_meta_t *meta, const uint8_t *bytes, size_t length,
    canview_peer_session_t *session, canview_decoded_frame_t *out)
{
    if (out == NULL)
    {
        return CANVIEW_DECODE_DROP_SILENT;
    }
    memset(out, 0, sizeof(*out));
    out->variant_index = CANVIEW_ESPNOW_CONTRACT_VARIANT_NONE;
    out->reason = CANVIEW_DECODE_REASON_INVALID_ARGUMENT;
    if (meta == NULL || bytes == NULL || session == NULL)
    {
        return decode_failure(meta, session, out, CANVIEW_DECODE_REASON_INVALID_ARGUMENT);
    }
    if (session->rx_resource_limit == 0U)
    {
        session->rx_resource_limit = CANVIEW_ESPNOW_RX_RESOURCE_LIMIT;
        (void)canview_error_rate_limiter_reset(&session->error_limiter, ESPNOW_DEFAULT_ERROR_RATE_LIMIT);
    }
    if (!canview_peer_session_auth_allowed(session, meta->now_ms))
    {
        return decode_failure(meta, session, out, CANVIEW_DECODE_REASON_AUTH_BACKOFF);
    }
    if (length < ESPNOW_HEADER_SIZE || length > CANVIEW_MAX_FRAME_SIZE)
    {
        return decode_failure(meta, session, out, CANVIEW_DECODE_REASON_BAD_LENGTH);
    }
    if (read_le16(bytes) != CANVIEW_MAGIC_LE)
    {
        return decode_failure(meta, session, out, CANVIEW_DECODE_REASON_BAD_MAGIC);
    }
    if (bytes[2U] != CANVIEW_PROTOCOL_MAJOR)
    {
        return decode_failure(meta, session, out, CANVIEW_DECODE_REASON_INCOMPATIBLE_MAJOR);
    }
    if (bytes[3U] < CANVIEW_PROTOCOL_MINOR || bytes[4U] != CANVIEW_HEADER_SIZE ||
        read_le16(bytes + 26U) != 0U ||
        (bytes[6U] & (uint8_t)~CANVIEW_PROTOCOL_KNOWN_FLAG_MASK) != 0U ||
        bytes[7U] > CANVIEW_PRIORITY_BULK)
    {
        return decode_failure(meta, session, out, CANVIEW_DECODE_REASON_BAD_RESERVED);
    }
    const size_t payload_size = read_le16(bytes + 24U);
    if (payload_size != length - ESPNOW_HEADER_SIZE || payload_size > CANVIEW_MAX_PAYLOAD_SIZE)
    {
        return decode_failure(meta, session, out, CANVIEW_DECODE_REASON_BAD_LENGTH);
    }
    if (read_le32(bytes + ESPNOW_CRC_OFFSET) != frame_crc(bytes, length))
    {
        return decode_failure(meta, session, out, CANVIEW_DECODE_REASON_BAD_CRC);
    }
    header_from_wire(bytes, &out->header);
    out->payload = bytes + ESPNOW_HEADER_SIZE;
    out->payload_size = payload_size;
    out->contract = find_contract(out->header.message_type);
    if (out->contract == NULL)
    {
        return decode_failure(meta, session, out, CANVIEW_DECODE_REASON_UNSUPPORTED_MESSAGE);
    }
    if (!contract_variant(out->contract, payload_size, CANVIEW_ESPNOW_CONTRACT_VARIANT_NONE,
                          &out->variant_index))
    {
        return decode_failure(meta, session, out, CANVIEW_DECODE_REASON_BAD_LENGTH);
    }
    const canview_decode_reason_t policy_reason = validate_policy(
        meta, session, out->contract, &out->header, out->payload, payload_size, out->variant_index);
    if (policy_reason != CANVIEW_DECODE_REASON_NONE)
    {
        if (policy_reason == CANVIEW_DECODE_REASON_ENCRYPTION_REQUIRED)
        {
            canview_peer_session_auth_failure(session, meta->now_ms);
        }
        return decode_failure(meta, session, out, policy_reason);
    }
    canview_sequence_window_t candidate = session->rx_window;
    const canview_status_t sequence_status = canview_sequence_window_accept(
        &candidate, out->header.sequence);
    if (sequence_status == CANVIEW_DUPLICATE)
    {
        out->duplicate = true;
        out->reason = CANVIEW_DECODE_REASON_REPLAY;
        return effective_ack_required(out->contract, out->variant_index) != 0U
                   ? CANVIEW_DECODE_ACK_REQUIRED
                   : CANVIEW_DECODE_DROP_SILENT;
    }
    if (sequence_status == CANVIEW_STALE)
    {
        return decode_failure(meta, session, out, CANVIEW_DECODE_REASON_STALE);
    }
    if (sequence_status != CANVIEW_OK || canview_peer_session_reserve_rx(session) != CANVIEW_OK)
    {
        return decode_failure(meta, session, out, CANVIEW_DECODE_REASON_RESOURCE_BUSY);
    }
    session->rx_window = candidate;
    session->last_valid_ms = meta->now_ms;
    canview_peer_session_auth_success(session);
    if (session->initialized)
    {
        (void)canview_peer_session_on_message(session, out->header.message_type, meta->now_ms);
    }
    out->reason = CANVIEW_DECODE_REASON_NONE;
    return effective_ack_required(out->contract, out->variant_index) != 0U
               ? CANVIEW_DECODE_ACK_REQUIRED
               : CANVIEW_DECODE_DELIVER;
}

static canview_encode_result_t encode_error(canview_decode_reason_t reason)
{
    switch (reason)
    {
    case CANVIEW_DECODE_REASON_INVALID_ARGUMENT:
        return CANVIEW_ENCODE_INVALID_ARGUMENT;
    case CANVIEW_DECODE_REASON_BAD_LENGTH:
    case CANVIEW_DECODE_REASON_BAD_FLAGS:
    case CANVIEW_DECODE_REASON_BAD_RESERVED:
    case CANVIEW_DECODE_REASON_SEMANTIC:
        return CANVIEW_ENCODE_MALFORMED;
    case CANVIEW_DECODE_REASON_INCOMPATIBLE_MAJOR:
        return CANVIEW_ENCODE_UNSUPPORTED_VERSION;
    case CANVIEW_DECODE_REASON_UNSUPPORTED_MESSAGE:
        return CANVIEW_ENCODE_UNSUPPORTED_MESSAGE;
    case CANVIEW_DECODE_REASON_ENCRYPTION_REQUIRED:
        return CANVIEW_ENCODE_UNAUTHENTICATED;
    case CANVIEW_DECODE_REASON_ROLE_MISMATCH:
        return CANVIEW_ENCODE_ROLE_MISMATCH;
    case CANVIEW_DECODE_REASON_STATE_MISMATCH:
        return CANVIEW_ENCODE_STATE_MISMATCH;
    case CANVIEW_DECODE_REASON_SESSION_MISMATCH:
        return CANVIEW_ENCODE_SESSION_MISMATCH;
    default:
        return CANVIEW_ENCODE_MALFORMED;
    }
}

canview_encode_result_t canview_frame_encode(
    const canview_encode_request_t *request, uint8_t *out, size_t capacity, size_t *written)
{
    if (written == NULL)
    {
        return CANVIEW_ENCODE_INVALID_ARGUMENT;
    }
    *written = 0U;
    if (request == NULL || out == NULL || request->payload == NULL ||
        request->payload_size > CANVIEW_MAX_PAYLOAD_SIZE || capacity < ESPNOW_HEADER_SIZE)
    {
        return CANVIEW_ENCODE_INVALID_ARGUMENT;
    }
    const canview_espnow_message_contract_t *contract = find_contract(request->header.message_type);
    if (contract == NULL)
    {
        return CANVIEW_ENCODE_UNSUPPORTED_MESSAGE;
    }
    uint8_t variant_index = CANVIEW_ESPNOW_CONTRACT_VARIANT_NONE;
    if (!contract_variant(contract, request->payload_size, request->variant_index, &variant_index))
    {
        return CANVIEW_ENCODE_MALFORMED;
    }
    const canview_decode_reason_t policy_reason = validate_policy(
        &request->meta, NULL, contract, &request->header, request->payload,
        request->payload_size, variant_index);
    if (policy_reason != CANVIEW_DECODE_REASON_NONE)
    {
        return encode_error(policy_reason);
    }
    if (request->header.sender_time > UINT32_MAX ||
        request->payload_size + ESPNOW_HEADER_SIZE > capacity ||
        request->payload_size + ESPNOW_HEADER_SIZE > CANVIEW_MAX_FRAME_SIZE)
    {
        return request->header.sender_time > UINT32_MAX
                   ? CANVIEW_ENCODE_MALFORMED
                   : CANVIEW_ENCODE_BUFFER_TOO_SMALL;
    }
    header_to_wire(&request->header, out, request->payload_size);
    memcpy(out + ESPNOW_HEADER_SIZE, request->payload, request->payload_size);
    write_le32(out + ESPNOW_CRC_OFFSET, frame_crc(out, ESPNOW_HEADER_SIZE + request->payload_size));
    *written = ESPNOW_HEADER_SIZE + request->payload_size;
    return CANVIEW_ENCODE_OK;
}

canview_status_t canview_peer_session_reset(canview_peer_session_t *session)
{
    if (session == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    memset(session, 0, sizeof(*session));
    session->state = CANVIEW_LINK_DISABLED;
    session->rx_resource_limit = CANVIEW_ESPNOW_RX_RESOURCE_LIMIT;
    session->next_sequence = 1U;
    (void)canview_sequence_window_reset(&session->rx_window);
    return canview_error_rate_limiter_reset(&session->error_limiter, ESPNOW_DEFAULT_ERROR_RATE_LIMIT);
}

canview_status_t canview_peer_session_start(
    canview_peer_session_t *session, canview_role_t local_role, uint32_t session_id,
    bool encrypted, bool authenticated, uint32_t now_ms)
{
    if (session == NULL || !valid_role(local_role) || session_id == 0U)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    (void)canview_peer_session_reset(session);
    session->initialized = true;
    session->session_id = session_id;
    session->local_role = local_role;
    session->state = CANVIEW_LINK_SECURE_HELLO;
    session->encrypted = encrypted;
    session->authenticated = authenticated;
    session->last_valid_ms = now_ms;
    return CANVIEW_OK;
}

canview_status_t canview_peer_session_authorize(
    canview_peer_session_t *session, canview_role_t peer_role, uint16_t scope,
    uint32_t message_classes, uint32_t link_key_generation)
{
    if (session == NULL || !valid_role(peer_role) ||
        (scope & (uint16_t)~CANVIEW_CONTROL_SCOPE_KNOWN_MASK) != 0U ||
        ((peer_role == CANVIEW_ROLE_READ_ONLY_CONTROLLER || peer_role == CANVIEW_ROLE_DIAGNOSTIC_BRIDGE) &&
         scope != 0U) || link_key_generation == 0U)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    session->peer_role = peer_role;
    session->authorized_role = peer_role;
    session->authorized_scope = scope;
    session->authorized_message_classes = message_classes;
    session->link_key_generation = link_key_generation;
    session->local_authorization_valid = true;
    return CANVIEW_OK;
}

canview_status_t canview_peer_session_on_message(
    canview_peer_session_t *session, uint8_t message_type, uint32_t now_ms)
{
    if (session == NULL || !session->initialized)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    session->last_valid_ms = now_ms;
    switch (session->state)
    {
    case CANVIEW_LINK_SECURE_HELLO:
        if (message_type == CANVIEW_MSG_HELLO)
        {
            session->state = CANVIEW_LINK_NEGOTIATING;
        }
        break;
    case CANVIEW_LINK_NEGOTIATING:
        if (message_type == CANVIEW_MSG_CAPABILITIES)
        {
            session->state = CANVIEW_LINK_TIME_SYNC;
            session->time_sync_samples = 0U;
        }
        break;
    case CANVIEW_LINK_TIME_SYNC:
        if (message_type == CANVIEW_MSG_TIME_SYNC_REQUEST ||
            message_type == CANVIEW_MSG_TIME_SYNC_RESPONSE)
        {
            if (session->time_sync_samples < UINT8_MAX)
            {
                ++session->time_sync_samples;
            }
            if (session->time_sync_samples >= ESPNOW_TIME_SYNC_SAMPLE_TARGET)
            {
                session->state = CANVIEW_LINK_STATE_SYNC;
            }
        }
        break;
    case CANVIEW_LINK_STATE_SYNC:
        if (message_type == CANVIEW_MSG_STATE_SNAPSHOT)
        {
            if (!session->encrypted || !session->authenticated)
            {
                return CANVIEW_INVALID_ARGUMENT;
            }
            session->state = CANVIEW_LINK_ONLINE;
        }
        break;
    case CANVIEW_LINK_DEGRADED:
    case CANVIEW_LINK_RECOVERING:
        if (message_type == CANVIEW_MSG_HEARTBEAT)
        {
            session->state = CANVIEW_LINK_ONLINE;
            session->heartbeat_misses = 0U;
        }
        break;
    case CANVIEW_LINK_ONLINE:
        break;
    default:
        break;
    }
    return CANVIEW_OK;
}

canview_status_t canview_peer_session_reserve_rx(canview_peer_session_t *session)
{
    if (session == NULL || session->rx_reserved >= session->rx_resource_limit)
    {
        return CANVIEW_BUFFER_TOO_SMALL;
    }
    ++session->rx_reserved;
    return CANVIEW_OK;
}

canview_status_t canview_peer_session_release_rx(canview_peer_session_t *session)
{
    if (session == NULL || session->rx_reserved == 0U)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    --session->rx_reserved;
    return CANVIEW_OK;
}

bool canview_peer_session_auth_allowed(const canview_peer_session_t *session, uint32_t now_ms)
{
    return session != NULL &&
           (session->auth_failures < 5U ||
            now_ms - session->auth_backoff_until_ms < UINT32_C(0x80000000));
}

void canview_peer_session_auth_failure(canview_peer_session_t *session, uint32_t now_ms)
{
    if (session == NULL)
    {
        return;
    }
    if (session->auth_failures < UINT8_MAX)
    {
        ++session->auth_failures;
    }
    if (session->auth_failures >= 5U)
    {
        const uint8_t shift = (uint8_t)((session->auth_failures - 5U) > 5U
                                            ? 5U
                                            : session->auth_failures - 5U);
        uint32_t delay = ESPNOW_AUTH_BACKOFF_BASE_MS << shift;
        if (delay > ESPNOW_AUTH_BACKOFF_MAX_MS)
        {
            delay = ESPNOW_AUTH_BACKOFF_MAX_MS;
        }
        session->auth_backoff_until_ms = now_ms + delay;
    }
}

void canview_peer_session_auth_success(canview_peer_session_t *session)
{
    if (session != NULL)
    {
        session->auth_failures = 0U;
        session->auth_backoff_until_ms = 0U;
    }
}

canview_status_t canview_control_time_sync_update(
    canview_control_time_sync_t *sync, uint32_t generation, uint32_t uncertainty_us,
    uint32_t now_ms, uint64_t controller_boot_id, uint64_t stm_boot_id)
{
    if (sync == NULL || generation == 0U || uncertainty_us > CANVIEW_ESPNOW_CONTROL_TIME_MAX_UNCERTAINTY_US)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (sync->valid)
    {
        const uint32_t difference = generation - sync->generation;
        if (difference == 0U)
        {
            return CANVIEW_DUPLICATE;
        }
        if (difference >= ESPNOW_SEQUENCE_HALF)
        {
            return CANVIEW_STALE;
        }
    }
    sync->valid = true;
    sync->generation = generation;
    sync->uncertainty_us = uncertainty_us;
    sync->updated_ms = now_ms;
    sync->controller_boot_id = controller_boot_id;
    sync->stm_boot_id = stm_boot_id;
    return CANVIEW_OK;
}

void canview_control_time_sync_invalidate(canview_control_time_sync_t *sync)
{
    if (sync != NULL)
    {
        memset(sync, 0, sizeof(*sync));
    }
}

bool canview_control_time_sync_is_valid(
    const canview_control_time_sync_t *sync, uint32_t now_ms)
{
    return sync != NULL && sync->valid && sync->uncertainty_us <= CANVIEW_ESPNOW_CONTROL_TIME_MAX_UNCERTAINTY_US &&
           now_ms - sync->updated_ms <= CANVIEW_ESPNOW_CONTROL_TIME_MAX_AGE_MS;
}

canview_status_t canview_error_rate_limiter_reset(
    canview_error_rate_limiter_t *limiter, uint16_t per_second_limit)
{
    if (limiter == NULL || per_second_limit == 0U)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    memset(limiter, 0, sizeof(*limiter));
    limiter->per_second_limit = per_second_limit;
    return CANVIEW_OK;
}

bool canview_error_rate_limiter_allow(
    canview_error_rate_limiter_t *limiter, uint32_t now_ms)
{
    if (limiter == NULL || limiter->per_second_limit == 0U)
    {
        return false;
    }
    if (now_ms - limiter->window_start_ms >= 1000U)
    {
        limiter->window_start_ms = now_ms;
        limiter->emitted = 0U;
        limiter->suppressed = 0U;
    }
    if (limiter->emitted >= limiter->per_second_limit)
    {
        if (limiter->suppressed < UINT16_MAX)
        {
            ++limiter->suppressed;
        }
        return false;
    }
    ++limiter->emitted;
    return true;
}

canview_status_t canview_frame_pool_reset(canview_frame_pool_t *pool)
{
    if (pool == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    memset(pool, 0, sizeof(*pool));
    return CANVIEW_OK;
}

canview_status_t canview_frame_pool_acquire(
    canview_frame_pool_t *pool, uint8_t *slot_index, uint8_t **buffer)
{
    if (pool == NULL || slot_index == NULL || buffer == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    for (uint8_t index = 0U; index < CANVIEW_ESPNOW_RX_RESOURCE_LIMIT; ++index)
    {
        if (!pool->slots[index].in_use)
        {
            pool->slots[index].in_use = true;
            *slot_index = index;
            *buffer = pool->slots[index].bytes;
            return CANVIEW_OK;
        }
    }
    return CANVIEW_BUFFER_TOO_SMALL;
}

canview_status_t canview_frame_pool_release(canview_frame_pool_t *pool, uint8_t slot_index)
{
    if (pool == NULL || slot_index >= CANVIEW_ESPNOW_RX_RESOURCE_LIMIT ||
        !pool->slots[slot_index].in_use)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    pool->slots[slot_index].in_use = false;
    return CANVIEW_OK;
}

static const canview_espnow_pairing_contract_t *find_pairing_contract(uint8_t message_type)
{
    for (size_t index = 0U; index < canview_espnow_pairing_contract_count; ++index)
    {
        if (canview_espnow_pairing_contracts[index].message_type == message_type)
        {
            return &canview_espnow_pairing_contracts[index];
        }
    }
    return NULL;
}

canview_status_t canview_pairing_transcript_encode(
    uint8_t message_type, const uint8_t *payload, size_t payload_size,
    uint8_t *output, size_t capacity, size_t *written)
{
    if (written == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    *written = 0U;
    if (payload == NULL || output == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const canview_espnow_pairing_contract_t *contract = find_pairing_contract(message_type);
    if (contract == NULL)
    {
        return CANVIEW_UNSUPPORTED_MESSAGE;
    }
    size_t domain_length = 0U;
    while (contract->domain[domain_length] != '\0')
    {
        ++domain_length;
    }
    if (domain_length > UINT8_MAX)
    {
        return CANVIEW_MALFORMED;
    }
    size_t required = 1U + domain_length + 1U;
    for (uint8_t index = 0U; index < contract->field_count; ++index)
    {
        const size_t offset = contract->offsets[index];
        const size_t field_length = contract->lengths[index];
        if (offset > payload_size || field_length > payload_size - offset ||
            required > SIZE_MAX - 2U - field_length)
        {
            return CANVIEW_MALFORMED;
        }
        required += 2U + field_length;
    }
    if (required > capacity || required > CANVIEW_ESPNOW_MAX_CANONICAL_BYTES)
    {
        return CANVIEW_BUFFER_TOO_SMALL;
    }
    output[0] = (uint8_t)domain_length;
    memcpy(output + 1U, contract->domain, domain_length);
    output[1U + domain_length] = ESPNOW_PAIRING_CANONICAL_DOMAIN_SEPARATOR;
    size_t cursor = 2U + domain_length;
    for (uint8_t index = 0U; index < contract->field_count; ++index)
    {
        const size_t offset = contract->offsets[index];
        const size_t field_length = contract->lengths[index];
        write_le16(output + cursor, (uint16_t)field_length);
        cursor += 2U;
        memcpy(output + cursor, payload + offset, field_length);
        cursor += field_length;
    }
    *written = cursor;
    return CANVIEW_OK;
}

static bool crypto_hmac_available(const canview_crypto_adapter_t *crypto)
{
    return crypto != NULL && crypto->hmac_sha256 != NULL;
}

static canview_status_t hmac_tag16(
    const canview_crypto_adapter_t *crypto, const uint8_t key[CANVIEW_ESPNOW_LINK_ROOT_BYTES],
    const uint8_t *data, size_t data_length, uint8_t tag[CANVIEW_ESPNOW_CONTROL_TAG_BYTES])
{
    if (!crypto_hmac_available(crypto) || key == NULL || data == NULL || tag == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    uint8_t digest[CANVIEW_ESPNOW_HASH_BYTES] = {0};
    if (crypto->hmac_sha256(crypto->context, key, CANVIEW_ESPNOW_LINK_ROOT_BYTES,
                            data, data_length, digest) != 0)
    {
        return CANVIEW_AUTH_FAILED;
    }
    memcpy(tag, digest, CANVIEW_ESPNOW_CONTROL_TAG_BYTES);
    return CANVIEW_OK;
}

canview_status_t canview_pairing_tag_compute(
    const canview_crypto_adapter_t *crypto, const uint8_t link_root[CANVIEW_ESPNOW_LINK_ROOT_BYTES],
    uint8_t message_type, const uint8_t *payload, size_t payload_size,
    uint8_t tag[CANVIEW_ESPNOW_CONTROL_TAG_BYTES])
{
    if (link_root == NULL || tag == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    uint8_t canonical[CANVIEW_ESPNOW_MAX_CANONICAL_BYTES] = {0};
    size_t canonical_size = 0U;
    const canview_status_t status = canview_pairing_transcript_encode(
        message_type, payload, payload_size, canonical, sizeof(canonical), &canonical_size);
    if (status != CANVIEW_OK)
    {
        return status;
    }
    return hmac_tag16(crypto, link_root, canonical, canonical_size, tag);
}

canview_status_t canview_pairing_tag_verify(
    const canview_crypto_adapter_t *crypto, const uint8_t link_root[CANVIEW_ESPNOW_LINK_ROOT_BYTES],
    uint8_t message_type, const uint8_t *payload, size_t payload_size,
    const uint8_t tag[CANVIEW_ESPNOW_CONTROL_TAG_BYTES])
{
    if (tag == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    uint8_t expected[CANVIEW_ESPNOW_CONTROL_TAG_BYTES] = {0};
    const canview_status_t status = canview_pairing_tag_compute(
        crypto, link_root, message_type, payload, payload_size, expected);
    if (status != CANVIEW_OK)
    {
        return status;
    }
    return constant_time_equal(expected, tag, sizeof(expected)) ? CANVIEW_OK : CANVIEW_AUTH_FAILED;
}

canview_status_t canview_pairing_replay_accept(
    canview_peer_session_t *session,
    const uint8_t nonce[CANVIEW_ESPNOW_PAIRING_NONCE_BYTES],
    const uint8_t transcript_hash[CANVIEW_ESPNOW_HASH_BYTES])
{
    if (session == NULL || nonce == NULL || transcript_hash == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if ((session->pairing_nonce_seen &&
         constant_time_equal(session->pairing_nonce, nonce, CANVIEW_ESPNOW_PAIRING_NONCE_BYTES)) ||
        (session->transcript_seen &&
         constant_time_equal(session->transcript_hash, transcript_hash, CANVIEW_ESPNOW_HASH_BYTES)))
    {
        return CANVIEW_DUPLICATE;
    }
    memcpy(session->pairing_nonce, nonce, CANVIEW_ESPNOW_PAIRING_NONCE_BYTES);
    memcpy(session->transcript_hash, transcript_hash, CANVIEW_ESPNOW_HASH_BYTES);
    session->pairing_nonce_seen = true;
    session->transcript_seen = true;
    return CANVIEW_OK;
}

static canview_status_t control_canonical_encode(
    uint32_t session_id, uint8_t message_type, const uint8_t *payload, size_t payload_size,
    uint8_t *output, size_t capacity, size_t *written)
{
    if (payload == NULL || output == NULL || written == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    size_t domain_length = 0U;
    while (ESPNOW_CONTROL_CANONICAL_DOMAIN[domain_length] != '\0')
    {
        ++domain_length;
    }
    size_t prefix_size = 0U;
    size_t tag_offset = 0U;
    size_t argument_length = 0U;
    if (message_type == CANVIEW_MSG_COMMAND_REQUEST)
    {
        if (payload_size < ESPNOW_CONTROL_COMMAND_PREFIX_PAYLOAD_SIZE)
        {
            return CANVIEW_MALFORMED;
        }
        prefix_size = ESPNOW_CONTROL_COMMAND_PREFIX_SIZE;
        tag_offset = ESPNOW_CONTROL_COMMAND_TAG_OFFSET;
        argument_length = read_le16(payload + ESPNOW_CONTROL_COMMAND_ARGUMENT_LENGTH_OFFSET);
        if (argument_length > payload_size - ESPNOW_CONTROL_COMMAND_PREFIX_PAYLOAD_SIZE ||
            payload_size != ESPNOW_CONTROL_COMMAND_PREFIX_PAYLOAD_SIZE + argument_length)
        {
            return CANVIEW_MALFORMED;
        }
    }
    else if (message_type == CANVIEW_MSG_CONTROL_LEASE_REQUEST)
    {
        if (payload_size != 52U)
        {
            return CANVIEW_MALFORMED;
        }
        prefix_size = ESPNOW_CONTROL_LEASE_PREFIX_SIZE;
        tag_offset = ESPNOW_CONTROL_LEASE_TAG_OFFSET;
    }
    else
    {
        return CANVIEW_UNSUPPORTED_MESSAGE;
    }
    if (tag_offset != prefix_size || domain_length > UINT8_MAX)
    {
        return CANVIEW_MALFORMED;
    }
    const size_t required = 1U + domain_length + 1U + 1U + 4U + 2U + prefix_size + argument_length;
    if (required > capacity || required > CANVIEW_ESPNOW_MAX_CANONICAL_BYTES)
    {
        return CANVIEW_BUFFER_TOO_SMALL;
    }
    output[0] = (uint8_t)domain_length;
    memcpy(output + 1U, ESPNOW_CONTROL_CANONICAL_DOMAIN, domain_length);
    size_t cursor = 1U + domain_length;
    output[cursor++] = 0U;
    output[cursor++] = message_type;
    write_le32(output + cursor, session_id);
    cursor += 4U;
    write_le16(output + cursor, (uint16_t)payload_size);
    cursor += 2U;
    memcpy(output + cursor, payload, prefix_size);
    cursor += prefix_size;
    if (argument_length != 0U)
    {
        memcpy(output + cursor, payload + ESPNOW_CONTROL_COMMAND_PREFIX_PAYLOAD_SIZE, argument_length);
        cursor += argument_length;
    }
    *written = cursor;
    return CANVIEW_OK;
}

canview_status_t canview_control_tag_compute(
    const canview_crypto_adapter_t *crypto,
    const uint8_t control_root[CANVIEW_ESPNOW_LINK_ROOT_BYTES],
    uint32_t session_id, uint8_t message_type, const uint8_t *payload, size_t payload_size,
    uint8_t tag[CANVIEW_ESPNOW_CONTROL_TAG_BYTES])
{
    if (control_root == NULL || tag == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    uint8_t canonical[CANVIEW_ESPNOW_MAX_CANONICAL_BYTES] = {0};
    size_t canonical_size = 0U;
    const canview_status_t status = control_canonical_encode(
        session_id, message_type, payload, payload_size,
        canonical, sizeof(canonical), &canonical_size);
    if (status != CANVIEW_OK)
    {
        return status;
    }
    return hmac_tag16(crypto, control_root, canonical, canonical_size, tag);
}

canview_status_t canview_control_tag_verify(
    const canview_crypto_adapter_t *crypto,
    const uint8_t control_root[CANVIEW_ESPNOW_LINK_ROOT_BYTES],
    uint32_t session_id, uint8_t message_type, const uint8_t *payload, size_t payload_size,
    const uint8_t tag[CANVIEW_ESPNOW_CONTROL_TAG_BYTES])
{
    if (tag == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    uint8_t expected[CANVIEW_ESPNOW_CONTROL_TAG_BYTES] = {0};
    const canview_status_t status = canview_control_tag_compute(
        crypto, control_root, session_id, message_type, payload, payload_size, expected);
    if (status != CANVIEW_OK)
    {
        return status;
    }
    return constant_time_equal(expected, tag, sizeof(expected)) ? CANVIEW_OK : CANVIEW_AUTH_FAILED;
}

canview_status_t canview_link_key_store_reset(canview_link_key_store_t *store)
{
    if (store == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    memset(store, 0, sizeof(*store));
    return CANVIEW_OK;
}

canview_status_t canview_link_key_store_stage(
    canview_link_key_store_t *store, uint32_t generation,
    const uint8_t link_root[CANVIEW_ESPNOW_LINK_ROOT_BYTES])
{
    if (store == NULL || link_root == NULL || generation == 0U ||
        (store->active.valid &&
         (generation - store->active.generation == 0U ||
          generation - store->active.generation >= ESPNOW_SEQUENCE_HALF)))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    memset(&store->staging, 0, sizeof(store->staging));
    store->staging.valid = true;
    store->staging.generation = generation;
    memcpy(store->staging.link_root, link_root, sizeof(store->staging.link_root));
    store->staging_ready = false;
    return CANVIEW_OK;
}

canview_status_t canview_link_key_store_mark_ready(canview_link_key_store_t *store)
{
    if (store == NULL || !store->staging.valid)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    store->staging_ready = true;
    return CANVIEW_OK;
}

canview_status_t canview_link_key_store_commit(canview_link_key_store_t *store)
{
    if (store == NULL || !store->staging.valid || !store->staging_ready)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    store->active = store->staging;
    memset(&store->staging, 0, sizeof(store->staging));
    store->staging_ready = false;
    return CANVIEW_OK;
}

canview_status_t canview_link_key_store_recover(canview_link_key_store_t *store)
{
    if (store == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (store->staging_ready && store->staging.valid)
    {
        store->active = store->staging;
    }
    memset(&store->staging, 0, sizeof(store->staging));
    store->staging_ready = false;
    return CANVIEW_OK;
}

static bool time_reached(uint32_t now_ms, uint32_t deadline_ms)
{
    return (int32_t)(now_ms - deadline_ms) >= 0;
}

static uint32_t qos_ack_timeout(const canview_qos_scheduler_t *scheduler)
{
    uint32_t timeout = CANVIEW_ESPNOW_INITIAL_ACK_TIMEOUT_MS;
    if (scheduler->has_srtt)
    {
        timeout = scheduler->srtt_ms > (UINT32_MAX - 20U) / 2U
                      ? UINT32_MAX
                      : scheduler->srtt_ms * 2U + 20U;
    }
    if (timeout < CANVIEW_ESPNOW_MIN_ACK_TIMEOUT_MS)
    {
        timeout = CANVIEW_ESPNOW_MIN_ACK_TIMEOUT_MS;
    }
    if (timeout > CANVIEW_ESPNOW_MAX_ACK_TIMEOUT_MS)
    {
        timeout = CANVIEW_ESPNOW_MAX_ACK_TIMEOUT_MS;
    }
    return timeout;
}

canview_status_t canview_qos_scheduler_reset(
    canview_qos_scheduler_t *scheduler, uint32_t first_sequence)
{
    if (scheduler == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    memset(scheduler, 0, sizeof(*scheduler));
    scheduler->next_sequence = first_sequence;
    return CANVIEW_OK;
}

canview_status_t canview_qos_submit(
    canview_qos_scheduler_t *scheduler, const canview_encode_request_t *request,
    uint32_t now_ms, uint16_t ttl_ms, uint64_t request_token,
    const uint8_t **frame, size_t *frame_size)
{
    if (frame == NULL || frame_size == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    *frame = NULL;
    *frame_size = 0U;
    if (scheduler == NULL || request == NULL || request->payload == NULL || request_token == 0U ||
        ttl_ms < CANVIEW_COMMAND_TTL_MIN_MS || ttl_ms > CANVIEW_COMMAND_TTL_MAX_MS ||
        (request->header.flags & CANVIEW_FLAG_ACK_REQUIRED) == 0U)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    canview_qos_pending_t *slot = NULL;
    for (size_t index = 0U; index < CANVIEW_ESPNOW_MAX_PENDING; ++index)
    {
        if (!scheduler->pending[index].in_use)
        {
            slot = &scheduler->pending[index];
            break;
        }
    }
    if (slot == NULL)
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    canview_encode_request_t encoded_request = *request;
    encoded_request.header.sequence = scheduler->next_sequence;
    ++scheduler->next_sequence;
    size_t encoded_size = 0U;
    const canview_encode_result_t encode_result = canview_frame_encode(
        &encoded_request, slot->frame, sizeof(slot->frame), &encoded_size);
    if (encode_result != CANVIEW_ENCODE_OK)
    {
        return encode_result == CANVIEW_ENCODE_BUFFER_TOO_SMALL
                   ? CANVIEW_BUFFER_TOO_SMALL
                   : CANVIEW_MALFORMED;
    }
    if (request->header.message_type == CANVIEW_MSG_COMMAND_REQUEST)
    {
        if (request->payload_size < ESPNOW_CONTROL_COMMAND_PREFIX_PAYLOAD_SIZE ||
            read_le64(request->payload) != request_token ||
            read_le16(request->payload + 10U) != ttl_ms)
        {
            return CANVIEW_MALFORMED;
        }
    }
    memset(slot->frame + encoded_size, 0, sizeof(slot->frame) - encoded_size);
    slot->in_use = true;
    slot->frame_size = encoded_size;
    slot->request_token = request_token;
    slot->first_sequence = encoded_request.header.sequence;
    slot->current_sequence = encoded_request.header.sequence;
    slot->issued_at_ms = now_ms;
    slot->expires_at_ms = now_ms + (uint32_t)ttl_ms;
    slot->last_sent_ms = now_ms;
    slot->attempts = 1U;
    uint32_t delay = qos_ack_timeout(scheduler);
    const uint32_t half_ttl = (uint32_t)ttl_ms / 2U;
    if (delay > half_ttl)
    {
        delay = half_ttl;
    }
    if (delay == 0U)
    {
        slot->in_use = false;
        return CANVIEW_TIMEOUT;
    }
    slot->next_deadline_ms = now_ms + delay;
    *frame = slot->frame;
    *frame_size = slot->frame_size;
    return CANVIEW_OK;
}

canview_qos_event_t canview_qos_poll(
    canview_qos_scheduler_t *scheduler, uint32_t now_ms,
    const uint8_t **frame, size_t *frame_size)
{
    if (frame != NULL)
    {
        *frame = NULL;
    }
    if (frame_size != NULL)
    {
        *frame_size = 0U;
    }
    if (scheduler == NULL || frame == NULL || frame_size == NULL)
    {
        return CANVIEW_QOS_NO_EVENT;
    }
    for (size_t index = 0U; index < CANVIEW_ESPNOW_MAX_PENDING; ++index)
    {
        canview_qos_pending_t *slot = &scheduler->pending[index];
        if (!slot->in_use || !time_reached(now_ms, slot->next_deadline_ms))
        {
            continue;
        }
        if (time_reached(now_ms, slot->expires_at_ms) || slot->attempts >= CANVIEW_ESPNOW_MAX_ATTEMPTS)
        {
            slot->in_use = false;
            return CANVIEW_QOS_EXPIRED;
        }
        const uint32_t remaining = slot->expires_at_ms - now_ms;
        uint32_t delay = qos_ack_timeout(scheduler);
        const uint32_t half_remaining = remaining / 2U;
        if (delay > half_remaining)
        {
            delay = half_remaining;
        }
        if (delay == 0U)
        {
            slot->in_use = false;
            return CANVIEW_QOS_EXPIRED;
        }
        slot->current_sequence = scheduler->next_sequence;
        ++scheduler->next_sequence;
        write_le32(slot->frame + 12U, slot->current_sequence);
        (void)canview_frame_recalculate_crc(slot->frame, slot->frame_size);
        ++slot->attempts;
        slot->last_sent_ms = now_ms;
        slot->next_deadline_ms = now_ms + delay;
        *frame = slot->frame;
        *frame_size = slot->frame_size;
        return CANVIEW_QOS_RETRY_READY;
    }
    return CANVIEW_QOS_NO_EVENT;
}

canview_status_t canview_qos_ack(
    canview_qos_scheduler_t *scheduler, uint64_t request_token,
    uint32_t acknowledged_sequence, uint32_t now_ms)
{
    if (scheduler == NULL || request_token == 0U)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    for (size_t index = 0U; index < CANVIEW_ESPNOW_MAX_PENDING; ++index)
    {
        canview_qos_pending_t *slot = &scheduler->pending[index];
        if (!slot->in_use || slot->request_token != request_token)
        {
            continue;
        }
        if (acknowledged_sequence != slot->current_sequence)
        {
            return CANVIEW_STALE;
        }
        const uint32_t sample = now_ms - slot->last_sent_ms;
        if (!scheduler->has_srtt)
        {
            scheduler->srtt_ms = sample;
            scheduler->has_srtt = true;
        }
        else
        {
            scheduler->srtt_ms = (scheduler->srtt_ms * 7U + sample) / 8U;
        }
        slot->in_use = false;
        return CANVIEW_OK;
    }
    return CANVIEW_STALE;
}
