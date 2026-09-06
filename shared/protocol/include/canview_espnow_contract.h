/* GENERATED FILE - DO NOT EDIT.
 * Source: protocol/schema/espnow-v1.3.yaml
 * Regenerate with: python tools/generate_protocol.py
 * SPDX-License-Identifier: GPL-3.0-only */
#ifndef CANVIEW_ESPNOW_CONTRACT_H
#define CANVIEW_ESPNOW_CONTRACT_H

#include <stddef.h>
#include <stdint.h>

#define CANVIEW_ESPNOW_CONTRACT_SCHEMA_SHA256 "02ce0eea119158151a038c1b1b89600cc38a7e125df96193f09ff05518ed4108"
#define CANVIEW_ESPNOW_CONTRACT_VARIANT_NONE UINT8_C(0xFF)

typedef enum {
    CANVIEW_CONTRACT_PAYLOAD_FIXED = 0,
    CANVIEW_CONTRACT_PAYLOAD_BOUNDED = 1,
    CANVIEW_CONTRACT_PAYLOAD_SUFFIX = 2,
    CANVIEW_CONTRACT_PAYLOAD_TLV = 3,
    CANVIEW_CONTRACT_PAYLOAD_VARIANTS = 4
} canview_contract_payload_kind_t;

typedef enum {
    CANVIEW_CONTRACT_QOS_NONE = 0,
    CANVIEW_CONTRACT_QOS_0 = 1,
    CANVIEW_CONTRACT_QOS_1 = 2,
    CANVIEW_CONTRACT_QOS_1_WINDOW = 3
} canview_contract_qos_t;

typedef enum {
    CANVIEW_CONTRACT_FIELD_RESERVED = 0,
    CANVIEW_CONTRACT_FIELD_ENUM = 1,
    CANVIEW_CONTRACT_FIELD_BITMASK = 2,
    CANVIEW_CONTRACT_FIELD_RANGE = 3
} canview_contract_field_kind_t;

typedef struct {
    uint8_t message_type;
    uint8_t payload_kind;
    uint8_t qos;
    uint8_t priority;
    uint8_t ack_required;
    uint8_t broadcast;
    uint8_t encrypted;
    uint8_t response;
    uint8_t fragment;
    uint8_t session_zero;
    uint8_t authenticated_session_zero;
    uint8_t sender_role_mask;
    uint8_t receiver_role_mask;
    uint16_t state_mask;
    uint16_t payload_min;
    uint16_t payload_max;
    uint16_t prefix_size;
    uint16_t record_size;
    uint16_t count_max;
    uint8_t count_offset;
    uint8_t count_width;
    uint8_t length_offset;
    uint8_t length_width;
    uint8_t variant_count;
    uint16_t variant_sizes[2];
    uint8_t variant_sender_role_masks[2];
    uint8_t variant_receiver_role_masks[2];
    uint8_t variant_response[2];
    uint8_t variant_ack_required[2];
} canview_espnow_message_contract_t;

typedef struct {
    uint8_t message_type;
    uint8_t region;
    uint8_t variant_index;
    uint8_t kind;
    uint8_t width;
    uint16_t offset;
    uint16_t stride;
    uint32_t minimum;
    uint32_t maximum;
    uint32_t mask;
    uint16_t allowed_index;
    uint8_t allowed_count;
} canview_espnow_field_constraint_t;

typedef struct {
    uint8_t message_type;
    const char *domain;
    uint8_t field_count;
    uint16_t offsets[16];
    uint16_t lengths[16];
} canview_espnow_pairing_contract_t;

extern const canview_espnow_message_contract_t canview_espnow_message_contracts[];
extern const size_t canview_espnow_message_contract_count;
extern const canview_espnow_field_constraint_t canview_espnow_field_constraints[];
extern const size_t canview_espnow_field_constraint_count;
extern const uint32_t canview_espnow_allowed_values[];
extern const size_t canview_espnow_allowed_value_count;
extern const canview_espnow_pairing_contract_t canview_espnow_pairing_contracts[];
extern const size_t canview_espnow_pairing_contract_count;

#endif /* CANVIEW_ESPNOW_CONTRACT_H */
