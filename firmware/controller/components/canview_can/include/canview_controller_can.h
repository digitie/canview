#ifndef CANVIEW_CONTROLLER_CAN_H
#define CANVIEW_CONTROLLER_CAN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "canview_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t filter_id;
    uint32_t can_id;
    uint32_t can_id_mask;
    uint16_t period_ms;
    uint8_t bus_id;
    uint8_t flags_value;
    uint8_t flags_mask;
    uint8_t min_dlc;
    uint8_t max_dlc;
    uint8_t max_records_per_period;
    bool enabled;
} canview_controller_can_filter_config_t;

typedef struct {
    uint16_t period_ms;
    uint8_t max_records_per_period;
    uint32_t max_bytes_per_second;
    uint16_t burst_bytes;
    bool enabled;
} canview_controller_can_stream_config_t;

typedef enum {
    CANVIEW_CONTROLLER_CAN_FILTER_OK = 0,
    CANVIEW_CONTROLLER_CAN_FILTER_INVALID,
    CANVIEW_CONTROLLER_CAN_FILTER_FULL,
    CANVIEW_CONTROLLER_CAN_FILTER_NOT_FOUND,
    CANVIEW_CONTROLLER_CAN_FILTER_CONFLICT,
} canview_controller_can_filter_status_t;

typedef struct {
    bool in_use;
    canview_controller_can_filter_config_t config;
    uint32_t window_start_ms;
    uint8_t records_in_window;
    bool window_started;
} canview_controller_can_filter_slot_t;

typedef struct {
    canview_controller_can_filter_slot_t slots[CANVIEW_CAN_FILTER_MAX_COUNT];
    canview_controller_can_stream_config_t stream;
    uint32_t config_revision;
    uint32_t stream_window_start_ms;
    uint32_t stream_byte_window_start_ms;
    uint8_t stream_records_in_window;
    uint32_t stream_burst_bytes_in_window;
    uint32_t stream_bytes_in_window;
    bool stream_window_started;
    bool stream_byte_window_started;
    uint32_t accepted_records;
    uint32_t rejected_records;
    uint32_t budget_rejected_records;
} canview_controller_can_filter_store_t;

void canview_controller_can_filter_store_init(
    canview_controller_can_filter_store_t *store);

canview_controller_can_filter_status_t canview_controller_can_filter_validate(
    const canview_controller_can_filter_config_t *config);

canview_controller_can_filter_status_t canview_controller_can_filter_apply(
    canview_controller_can_filter_store_t *store,
    canview_can_filter_action_t action,
    const canview_controller_can_filter_config_t *config);

bool canview_controller_can_filter_get(
    const canview_controller_can_filter_store_t *store,
    uint32_t filter_id,
    canview_controller_can_filter_config_t *config);

size_t canview_controller_can_filter_list(
    const canview_controller_can_filter_store_t *store,
    canview_controller_can_filter_config_t *configs,
    size_t config_capacity);

bool canview_controller_can_filter_set_stream(
    canview_controller_can_filter_store_t *store,
    const canview_controller_can_stream_config_t *config);

bool canview_controller_can_filter_from_wire(
    const canview_can_filter_t *wire,
    canview_controller_can_filter_config_t *config);

void canview_controller_can_filter_to_wire(
    const canview_controller_can_filter_config_t *config,
    canview_can_filter_t *wire);

bool canview_controller_can_stream_from_wire(
    const canview_can_stream_config_t *wire,
    canview_controller_can_stream_config_t *config);

void canview_controller_can_stream_to_wire(
    const canview_controller_can_stream_config_t *config,
    canview_can_stream_config_t *wire,
    uint32_t config_revision);

/* Default deny: a raw CAN record is admitted only when one enabled filter
 * matches and both its per-filter and global stream budgets have room. */
bool canview_controller_can_accept_record(
    canview_controller_can_filter_store_t *store,
    const canview_can_record_t *record,
    uint32_t now_ms);

size_t canview_controller_can_filter_batch(
    canview_controller_can_filter_store_t *store,
    const canview_can_record_t *records,
    size_t record_count,
    canview_can_record_t *accepted,
    size_t accepted_capacity,
    uint32_t now_ms,
    uint16_t *rejected_count);

typedef enum {
    CANVIEW_CONTROLLER_SIGNAL_LITTLE_ENDIAN = 0,
    CANVIEW_CONTROLLER_SIGNAL_BIG_ENDIAN = 1,
} canview_controller_signal_byte_order_t;

typedef struct {
    uint16_t signal_id;
    uint8_t bus_id;
    uint32_t can_id;
    uint32_t can_id_mask;
    uint8_t start_bit;
    uint8_t bit_length;
    uint8_t byte_order;
    uint8_t value_type;
    uint8_t quality;
    bool is_signed;
    bool has_range;
    float factor;
    float offset;
    float minimum;
    float maximum;
} canview_controller_signal_descriptor_t;

typedef struct {
    uint16_t signal_id;
    uint8_t value_type;
    uint8_t quality;
    uint16_t age_ms;
    uint64_t raw_value;
    float physical_value;
    uint32_t value_bits;
} canview_controller_decoded_signal_t;

bool canview_controller_decode_signal(
    const canview_can_record_t *record,
    const canview_controller_signal_descriptor_t *descriptor,
    canview_controller_decoded_signal_t *decoded);

#ifdef __cplusplus
}
#endif

#endif
