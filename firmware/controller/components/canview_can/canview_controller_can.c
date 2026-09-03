#include "canview_controller_can.h"

#include <string.h>

static uint16_t read_le16(const uint8_t *bytes)
{
    return (uint16_t)bytes[0] | (uint16_t)((uint16_t)bytes[1] << 8U);
}

static uint32_t read_le32(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] |
           ((uint32_t)bytes[1] << 8U) |
           ((uint32_t)bytes[2] << 16U) |
           ((uint32_t)bytes[3] << 24U);
}

static void write_le16(uint8_t *bytes, uint16_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8U);
}

static void write_le32(uint8_t *bytes, uint32_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8U);
    bytes[2] = (uint8_t)(value >> 16U);
    bytes[3] = (uint8_t)(value >> 24U);
}

static uint8_t record_flags(const canview_can_record_t *record)
{
    return (uint8_t)((record->flags_dlc >> 4U) & UINT8_C(0x0F));
}

static uint8_t record_dlc(const canview_can_record_t *record)
{
    return (uint8_t)(record->flags_dlc & UINT8_C(0x0F));
}

static bool elapsed_at_least(uint32_t now, uint32_t then, uint32_t interval)
{
    return (uint32_t)(now - then) >= interval;
}

static bool stream_config_valid(const canview_controller_can_stream_config_t *config)
{
    return config != NULL &&
           (!config->enabled ||
            (config->period_ms >= CANVIEW_CAN_FILTER_MIN_PERIOD_MS &&
             config->period_ms <= CANVIEW_CAN_FILTER_MAX_PERIOD_MS &&
             config->max_records_per_period > 0U &&
             config->max_records_per_period <= CANVIEW_CAN_FILTER_MAX_RECORDS_PER_PERIOD &&
             config->max_bytes_per_second > 0U &&
             config->max_bytes_per_second <= CANVIEW_CAN_RX_MAX_BYTES_PER_SECOND &&
             config->burst_bytes > 0U &&
             config->burst_bytes <= config->max_bytes_per_second));
}

void canview_controller_can_filter_store_init(
    canview_controller_can_filter_store_t *store)
{
    if (store == NULL) {
        return;
    }
    *store = (canview_controller_can_filter_store_t){
        .config_revision = 1U,
        .stream = {
            .period_ms = 100U,
            .max_records_per_period = 32U,
            .max_bytes_per_second = CANVIEW_CAN_RX_DEFAULT_BYTES_PER_SECOND,
            .burst_bytes = 512U,
            .enabled = true,
        },
    };
}

canview_controller_can_filter_status_t canview_controller_can_filter_validate(
    const canview_controller_can_filter_config_t *config)
{
    if (config == NULL || config->filter_id == 0U || config->can_id > CANVIEW_CAN_EXTENDED_ID_MAX ||
        config->can_id_mask == 0U || config->can_id_mask > CANVIEW_CAN_EXTENDED_ID_MAX ||
        (config->bus_id != CANVIEW_CAN_BUS_ANY && config->bus_id >= 3U) ||
        (config->flags_value & UINT8_C(0xF0)) != 0U ||
        (config->flags_mask & UINT8_C(0xF0)) != 0U ||
        config->min_dlc > config->max_dlc || config->max_dlc > 8U ||
        config->period_ms < CANVIEW_CAN_FILTER_MIN_PERIOD_MS ||
        config->period_ms > CANVIEW_CAN_FILTER_MAX_PERIOD_MS ||
        config->max_records_per_period == 0U ||
        config->max_records_per_period > CANVIEW_CAN_FILTER_MAX_RECORDS_PER_PERIOD) {
        return CANVIEW_CONTROLLER_CAN_FILTER_INVALID;
    }
    return CANVIEW_CONTROLLER_CAN_FILTER_OK;
}

static int find_filter(const canview_controller_can_filter_store_t *store,
                       uint32_t filter_id)
{
    for (size_t i = 0; i < CANVIEW_CAN_FILTER_MAX_COUNT; ++i) {
        if (store->slots[i].in_use && store->slots[i].config.filter_id == filter_id) {
            return (int)i;
        }
    }
    return -1;
}

static int find_free_filter(const canview_controller_can_filter_store_t *store)
{
    for (size_t i = 0; i < CANVIEW_CAN_FILTER_MAX_COUNT; ++i) {
        if (!store->slots[i].in_use) {
            return (int)i;
        }
    }
    return -1;
}

static void reset_filter_window(canview_controller_can_filter_slot_t *slot)
{
    slot->window_start_ms = 0U;
    slot->records_in_window = 0U;
    slot->window_started = false;
}

canview_controller_can_filter_status_t canview_controller_can_filter_apply(
    canview_controller_can_filter_store_t *store,
    canview_can_filter_action_t action,
    const canview_controller_can_filter_config_t *config)
{
    if (store == NULL) {
        return CANVIEW_CONTROLLER_CAN_FILTER_INVALID;
    }
    if (action == CANVIEW_CAN_FILTER_CLEAR) {
        if (config != NULL) {
            return CANVIEW_CONTROLLER_CAN_FILTER_INVALID;
        }
        memset(store->slots, 0, sizeof(store->slots));
        ++store->config_revision;
        return CANVIEW_CONTROLLER_CAN_FILTER_OK;
    }
    if (config == NULL || config->filter_id == 0U) {
        return CANVIEW_CONTROLLER_CAN_FILTER_INVALID;
    }

    const int existing = find_filter(store, config->filter_id);
    if (action == CANVIEW_CAN_FILTER_DELETE) {
        if (existing < 0) {
            return CANVIEW_CONTROLLER_CAN_FILTER_NOT_FOUND;
        }
        memset(&store->slots[existing], 0, sizeof(store->slots[existing]));
        ++store->config_revision;
        return CANVIEW_CONTROLLER_CAN_FILTER_OK;
    }

    const canview_controller_can_filter_status_t valid =
        canview_controller_can_filter_validate(config);
    if (valid != CANVIEW_CONTROLLER_CAN_FILTER_OK) {
        return valid;
    }
    if (action == CANVIEW_CAN_FILTER_ADD) {
        if (existing >= 0) {
            return CANVIEW_CONTROLLER_CAN_FILTER_CONFLICT;
        }
        const int free_slot = find_free_filter(store);
        if (free_slot < 0) {
            return CANVIEW_CONTROLLER_CAN_FILTER_FULL;
        }
        store->slots[free_slot].in_use = true;
        store->slots[free_slot].config = *config;
        reset_filter_window(&store->slots[free_slot]);
        ++store->config_revision;
        return CANVIEW_CONTROLLER_CAN_FILTER_OK;
    }

    if (action == CANVIEW_CAN_FILTER_REPLACE) {
        if (existing < 0) {
            return CANVIEW_CONTROLLER_CAN_FILTER_NOT_FOUND;
        }
        store->slots[existing].config = *config;
        reset_filter_window(&store->slots[existing]);
        ++store->config_revision;
        return CANVIEW_CONTROLLER_CAN_FILTER_OK;
    }

    return CANVIEW_CONTROLLER_CAN_FILTER_INVALID;
}

bool canview_controller_can_filter_get(
    const canview_controller_can_filter_store_t *store,
    uint32_t filter_id,
    canview_controller_can_filter_config_t *config)
{
    if (store == NULL || config == NULL || filter_id == 0U) {
        return false;
    }
    const int index = find_filter(store, filter_id);
    if (index < 0) {
        return false;
    }
    *config = store->slots[index].config;
    return true;
}

size_t canview_controller_can_filter_list(
    const canview_controller_can_filter_store_t *store,
    canview_controller_can_filter_config_t *configs,
    size_t config_capacity)
{
    if (store == NULL || configs == NULL) {
        return 0U;
    }
    size_t count = 0U;
    for (size_t i = 0; i < CANVIEW_CAN_FILTER_MAX_COUNT && count < config_capacity; ++i) {
        if (store->slots[i].in_use) {
            configs[count++] = store->slots[i].config;
        }
    }
    return count;
}

bool canview_controller_can_filter_set_stream(
    canview_controller_can_filter_store_t *store,
    const canview_controller_can_stream_config_t *config)
{
    if (store == NULL || !stream_config_valid(config)) {
        return false;
    }
    store->stream = *config;
    store->stream_window_started = false;
    store->stream_byte_window_started = false;
    store->stream_records_in_window = 0U;
    store->stream_burst_bytes_in_window = 0U;
    store->stream_bytes_in_window = 0U;
    ++store->config_revision;
    return true;
}

bool canview_controller_can_filter_from_wire(
    const canview_can_filter_t *wire,
    canview_controller_can_filter_config_t *config)
{
    if (wire == NULL || config == NULL) {
        return false;
    }
    *config = (canview_controller_can_filter_config_t){
        .filter_id = read_le32((const uint8_t *)&wire->filter_id_le),
        .can_id = read_le32((const uint8_t *)&wire->can_id_le),
        .can_id_mask = read_le32((const uint8_t *)&wire->can_id_mask_le),
        .period_ms = read_le16((const uint8_t *)&wire->period_ms_le),
        .bus_id = wire->bus_id,
        .flags_value = wire->flags_value,
        .flags_mask = wire->flags_mask,
        .min_dlc = wire->min_dlc,
        .max_dlc = wire->max_dlc,
        .max_records_per_period = wire->max_records_per_period,
        .enabled = wire->enabled != 0U,
    };
    return wire->reserved0 == 0U && wire->enabled <= 1U &&
           canview_controller_can_filter_validate(config) ==
           CANVIEW_CONTROLLER_CAN_FILTER_OK;
}

void canview_controller_can_filter_to_wire(
    const canview_controller_can_filter_config_t *config,
    canview_can_filter_t *wire)
{
    if (config == NULL || wire == NULL) {
        return;
    }
    memset(wire, 0, sizeof(*wire));
    write_le32((uint8_t *)&wire->filter_id_le, config->filter_id);
    write_le32((uint8_t *)&wire->can_id_le, config->can_id);
    write_le32((uint8_t *)&wire->can_id_mask_le, config->can_id_mask);
    write_le16((uint8_t *)&wire->period_ms_le, config->period_ms);
    wire->bus_id = config->bus_id;
    wire->flags_value = config->flags_value;
    wire->flags_mask = config->flags_mask;
    wire->min_dlc = config->min_dlc;
    wire->max_dlc = config->max_dlc;
    wire->max_records_per_period = config->max_records_per_period;
    wire->enabled = config->enabled ? 1U : 0U;
}

bool canview_controller_can_stream_from_wire(
    const canview_can_stream_config_t *wire,
    canview_controller_can_stream_config_t *config)
{
    if (wire == NULL || config == NULL) {
        return false;
    }
    *config = (canview_controller_can_stream_config_t){
        .period_ms = read_le16((const uint8_t *)&wire->period_ms_le),
        .max_records_per_period = wire->max_records_per_period,
        .max_bytes_per_second = read_le32((const uint8_t *)&wire->max_bytes_per_second_le),
        .burst_bytes = read_le16((const uint8_t *)&wire->burst_bytes_le),
        .enabled = wire->enabled != 0U,
    };
    return wire->reserved_le == 0U && wire->enabled <= 1U &&
           stream_config_valid(config);
}

void canview_controller_can_stream_to_wire(
    const canview_controller_can_stream_config_t *config,
    canview_can_stream_config_t *wire,
    uint32_t config_revision)
{
    if (config == NULL || wire == NULL) {
        return;
    }
    memset(wire, 0, sizeof(*wire));
    write_le32((uint8_t *)&wire->config_revision_le, config_revision);
    write_le16((uint8_t *)&wire->period_ms_le, config->period_ms);
    wire->max_records_per_period = config->max_records_per_period;
    wire->enabled = config->enabled ? 1U : 0U;
    write_le32((uint8_t *)&wire->max_bytes_per_second_le, config->max_bytes_per_second);
    write_le16((uint8_t *)&wire->burst_bytes_le, config->burst_bytes);
}

static bool filter_matches(const canview_controller_can_filter_config_t *config,
                           const canview_can_record_t *record)
{
    const uint32_t can_id = read_le32((const uint8_t *)&record->can_id_le);
    return (config->bus_id == CANVIEW_CAN_BUS_ANY || config->bus_id == record->bus_id) &&
           (can_id & config->can_id_mask) == (config->can_id & config->can_id_mask) &&
           (record_flags(record) & config->flags_mask) ==
               (config->flags_value & config->flags_mask) &&
           record_dlc(record) >= config->min_dlc && record_dlc(record) <= config->max_dlc;
}

static bool record_is_valid(const canview_can_record_t *record)
{
    const uint32_t can_id = read_le32((const uint8_t *)&record->can_id_le);
    const uint8_t flags = record_flags(record);
    const uint32_t id_limit = (flags & CANVIEW_BUS_FLAG_EXTENDED_ID) != 0U
                                  ? CANVIEW_CAN_EXTENDED_ID_MAX
                                  : CANVIEW_CAN_STANDARD_ID_MAX;
    return record->bus_id < 3U && record_dlc(record) <= 8U && can_id <= id_limit;
}

static bool filter_quota_available(canview_controller_can_filter_slot_t *slot,
                                   uint32_t now_ms)
{
    if (!slot->window_started ||
        elapsed_at_least(now_ms, slot->window_start_ms, slot->config.period_ms)) {
        slot->window_started = true;
        slot->window_start_ms = now_ms;
        slot->records_in_window = 0U;
    }
    return slot->records_in_window < slot->config.max_records_per_period;
}

static bool stream_quota_available(canview_controller_can_filter_store_t *store,
                                   uint32_t now_ms)
{
    const canview_controller_can_stream_config_t *stream = &store->stream;
    if (!stream->enabled) {
        return false;
    }
    if (!store->stream_window_started ||
        elapsed_at_least(now_ms, store->stream_window_start_ms, stream->period_ms)) {
        store->stream_window_started = true;
        store->stream_window_start_ms = now_ms;
        store->stream_records_in_window = 0U;
        store->stream_burst_bytes_in_window = 0U;
    }
    if (!store->stream_byte_window_started ||
        elapsed_at_least(now_ms, store->stream_byte_window_start_ms, 1000U)) {
        store->stream_byte_window_started = true;
        store->stream_byte_window_start_ms = now_ms;
        store->stream_bytes_in_window = 0U;
    }
    return store->stream_records_in_window < stream->max_records_per_period &&
           store->stream_burst_bytes_in_window + CANVIEW_CAN_RECORD_WIRE_BYTES <=
               stream->burst_bytes &&
           store->stream_bytes_in_window + CANVIEW_CAN_RECORD_WIRE_BYTES <=
               stream->max_bytes_per_second;
}

bool canview_controller_can_accept_record(
    canview_controller_can_filter_store_t *store,
    const canview_can_record_t *record,
    uint32_t now_ms)
{
    if (store == NULL || record == NULL || !record_is_valid(record)) {
        if (store != NULL) {
            ++store->rejected_records;
        }
        return false;
    }

    int matching_slot = -1;
    for (size_t i = 0; i < CANVIEW_CAN_FILTER_MAX_COUNT; ++i) {
        canview_controller_can_filter_slot_t *slot = &store->slots[i];
        if (slot->in_use && slot->config.enabled && filter_matches(&slot->config, record) &&
            filter_quota_available(slot, now_ms)) {
            matching_slot = (int)i;
            break;
        }
    }
    if (matching_slot < 0) {
        ++store->rejected_records;
        return false;
    }
    if (!stream_quota_available(store, now_ms)) {
        ++store->rejected_records;
        ++store->budget_rejected_records;
        return false;
    }

    canview_controller_can_filter_slot_t *slot = &store->slots[matching_slot];
    ++slot->records_in_window;
    ++store->stream_records_in_window;
    store->stream_burst_bytes_in_window += CANVIEW_CAN_RECORD_WIRE_BYTES;
    store->stream_bytes_in_window += CANVIEW_CAN_RECORD_WIRE_BYTES;
    ++store->accepted_records;
    return true;
}

size_t canview_controller_can_filter_batch(
    canview_controller_can_filter_store_t *store,
    const canview_can_record_t *records,
    size_t record_count,
    canview_can_record_t *accepted,
    size_t accepted_capacity,
    uint32_t now_ms,
    uint16_t *rejected_count)
{
    size_t accepted_count = 0U;
    uint16_t rejected = 0U;
    if (store == NULL || records == NULL || accepted == NULL) {
        if (rejected_count != NULL) {
            *rejected_count = 0U;
        }
        return 0U;
    }
    for (size_t i = 0; i < record_count; ++i) {
        if (accepted_count < accepted_capacity &&
            canview_controller_can_accept_record(store, &records[i], now_ms)) {
            accepted[accepted_count++] = records[i];
        } else {
            if (accepted_count >= accepted_capacity) {
                ++store->rejected_records;
            }
            if (rejected < UINT16_MAX) {
                ++rejected;
            }
        }
    }
    if (rejected_count != NULL) {
        *rejected_count = rejected;
    }
    return accepted_count;
}
