/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_stm_fdcan_capture.h"

#include <string.h>

#define CANVIEW_STM_FDCAN_TIMING_MAX_PRESCALER (512U)
#define CANVIEW_STM_FDCAN_TIMING_MAX_SEGMENT1 (256U)
#define CANVIEW_STM_FDCAN_TIMING_MAX_SEGMENT2 (128U)
#define CANVIEW_STM_FDCAN_TIMING_MAX_SJW (128U)
#define CANVIEW_STM_FDCAN_TIMESTAMP_MAX_DELTA (UINT32_C(0x7fffffff))
#define CANVIEW_STM_FDCAN_TIMESTAMP_WRAP_INCREMENT (UINT64_C(0x100000000))
#define CANVIEW_STM_FDCAN_WIRE_IDE (UINT8_C(0x01))
#define CANVIEW_STM_FDCAN_WIRE_RTR (UINT8_C(0x02))
#define CANVIEW_STM_FDCAN_WIRE_ERROR (UINT8_C(0x04))
#define CANVIEW_STM_FDCAN_ELEMENT_EXTENDED (UINT32_C(0x40000000))
#define CANVIEW_STM_FDCAN_ELEMENT_REMOTE (UINT32_C(0x20000000))
#define CANVIEW_STM_FDCAN_ELEMENT_EXTENDED_ID_MASK (UINT32_C(0x1fffffff))
#define CANVIEW_STM_FDCAN_ELEMENT_STANDARD_ID_MASK (UINT32_C(0x7ff))
#define CANVIEW_STM_FDCAN_ELEMENT_FDF (UINT32_C(0x00200000))
#define CANVIEW_STM_FDCAN_ELEMENT_BRS (UINT32_C(0x00100000))
#define CANVIEW_STM_FDCAN_ELEMENT_DLC_MASK (UINT32_C(0x000f0000))

typedef struct
{
    uint32_t bitrate;
    canview_stm_fdcan_timing_t timing;
} canview_stm_fdcan_rate_entry_t;

/* 80 MHz kernel clock, 16 TQ, 87.5% sample point. */
static const canview_stm_fdcan_rate_entry_t rate_table[] = {
    {UINT32_C(1000000), {UINT16_C(5), UINT16_C(13), UINT8_C(2), UINT8_C(2)}},
    {UINT32_C(500000), {UINT16_C(10), UINT16_C(13), UINT8_C(2), UINT8_C(2)}},
    {UINT32_C(250000), {UINT16_C(20), UINT16_C(13), UINT8_C(2), UINT8_C(2)}},
    {UINT32_C(125000), {UINT16_C(40), UINT16_C(13), UINT8_C(2), UINT8_C(2)}}};

typedef char canview_stm_fdcan_record_fits_queue[
    (sizeof(canview_stm_fdcan_record_t) <= CANVIEW_STM_QUEUE_RECORD_MAX) ? 1 : -1];
typedef char canview_stm_fdcan_record_fits_wire[
    (CANVIEW_STM_FDCAN_MAX_DATA_BYTES == sizeof(((canview_stm_fdcan_record_t *)0)->data)) ? 1
                                                                                           : -1];

typedef struct
{
    uint32_t last_source_timestamp_us;
    uint64_t timestamp_epoch_us;
    uint64_t extended_timestamp_us;
    bool initialized;
} canview_stm_fdcan_timestamp_state_t;

static bool timing_is_zero(const canview_stm_fdcan_timing_t *timing)
{
    return timing != NULL && timing->prescaler == 0U && timing->time_segment1 == 0U &&
           timing->time_segment2 == 0U && timing->sync_jump_width == 0U;
}

static bool timing_matches_rate(const canview_stm_fdcan_timing_t *timing, uint32_t bitrate)
{
    if (timing == NULL || bitrate == 0U || timing->prescaler == 0U ||
        timing->prescaler > CANVIEW_STM_FDCAN_TIMING_MAX_PRESCALER ||
        timing->time_segment1 == 0U || timing->time_segment1 > CANVIEW_STM_FDCAN_TIMING_MAX_SEGMENT1 ||
        timing->time_segment2 == 0U || timing->time_segment2 > CANVIEW_STM_FDCAN_TIMING_MAX_SEGMENT2 ||
        timing->sync_jump_width == 0U || timing->sync_jump_width > CANVIEW_STM_FDCAN_TIMING_MAX_SJW ||
        timing->sync_jump_width > timing->time_segment2)
    {
        return false;
    }
    const uint64_t time_quanta = UINT64_C(1) + (uint64_t)timing->time_segment1 +
                                 (uint64_t)timing->time_segment2;
    const uint64_t divisor = time_quanta * (uint64_t)timing->prescaler;
    return divisor != 0U && (uint64_t)CANVIEW_STM_FDCAN_KERNEL_CLOCK_HZ % divisor == 0U &&
           (uint64_t)CANVIEW_STM_FDCAN_KERNEL_CLOCK_HZ / divisor == (uint64_t)bitrate;
}

canview_status_t canview_stm_fdcan_profile_for_bitrate(
    uint32_t nominal_bitrate, uint32_t data_bitrate, canview_stm_fdcan_profile_t *profile)
{
    if (profile == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (data_bitrate != 0U)
    {
        return CANVIEW_UNSUPPORTED_MESSAGE;
    }
    for (size_t index = 0U; index < sizeof(rate_table) / sizeof(rate_table[0]); ++index)
    {
        if (rate_table[index].bitrate == nominal_bitrate)
        {
            const canview_stm_fdcan_profile_t candidate = {
                .enabled = true,
                .bitrate_known = true,
                .transceiver_known = false,
                .transceiver = CANVIEW_STM_FDCAN_TRANSCEIVER_UNKNOWN,
                .nominal_bitrate = nominal_bitrate,
                .data_bitrate = 0U,
                .nominal_timing = rate_table[index].timing,
                .data_timing = {0U, 0U, 0U, 0U}};
            *profile = candidate;
            return CANVIEW_OK;
        }
    }
    return CANVIEW_UNSUPPORTED_MESSAGE;
}

canview_status_t canview_stm_fdcan_profile_validate(const canview_stm_fdcan_profile_t *profile)
{
    if (profile == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (!profile->enabled)
    {
        if (profile->bitrate_known || profile->transceiver_known ||
            profile->transceiver != CANVIEW_STM_FDCAN_TRANSCEIVER_UNKNOWN ||
            profile->nominal_bitrate != 0U ||
            profile->data_bitrate != 0U || !timing_is_zero(&profile->nominal_timing) ||
            !timing_is_zero(&profile->data_timing))
        {
            return CANVIEW_INVALID_ARGUMENT;
        }
        return CANVIEW_OK;
    }
    const int transceiver_value = (int)profile->transceiver;
    if (!profile->bitrate_known || !profile->transceiver_known || transceiver_value <= 0 ||
        transceiver_value >= (int)CANVIEW_STM_FDCAN_TRANSCEIVER_MAX ||
        profile->nominal_bitrate == 0U)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (profile->data_bitrate != 0U || !timing_is_zero(&profile->data_timing))
    {
        return CANVIEW_UNSUPPORTED_MESSAGE;
    }
    if (!timing_matches_rate(&profile->nominal_timing, profile->nominal_bitrate))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    return CANVIEW_OK;
}

canview_status_t canview_stm_fdcan_channel_profile_validate(
    size_t channel, const canview_stm_fdcan_profile_t *profile)
{
    if (profile == NULL || channel >= CANVIEW_STM_FDCAN_CHANNEL_COUNT)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const canview_status_t profile_status = canview_stm_fdcan_profile_validate(profile);
    if (profile_status != CANVIEW_OK || !profile->enabled)
    {
        return profile_status;
    }
    if (channel < 2U && profile->transceiver != CANVIEW_STM_FDCAN_TRANSCEIVER_TCAN1046)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (channel == 2U &&
        (profile->transceiver != CANVIEW_STM_FDCAN_TRANSCEIVER_MAX3055 ||
         profile->nominal_bitrate != CANVIEW_STM_FDCAN_CAN3_BITRATE))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    return CANVIEW_OK;
}

static bool bus_state_valid(canview_stm_fdcan_bus_state_t state)
{
    const int state_value = (int)state;
    return state_value >= 0 && state_value < (int)CANVIEW_STM_FDCAN_BUS_STATE_MAX;
}

static void increment_saturating(uint32_t *value)
{
    if (*value != UINT32_MAX)
    {
        ++*value;
    }
}

static void add_saturating(uint32_t *value, uint32_t amount)
{
    if (amount > UINT32_MAX - *value)
    {
        *value = UINT32_MAX;
    }
    else
    {
        *value += amount;
    }
}

static canview_status_t validate_input_frame(const canview_stm_fdcan_rx_frame_t *frame)
{
    if (frame == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if ((frame->flags & (uint8_t)~CANVIEW_STM_FDCAN_FRAME_FLAGS_MASK) != 0U)
    {
        return CANVIEW_MALFORMED;
    }
    if ((frame->flags & CANVIEW_STM_FDCAN_FRAME_FD) != 0U)
    {
        return CANVIEW_UNSUPPORTED_MESSAGE;
    }
    if ((frame->flags & CANVIEW_STM_FDCAN_FRAME_BRS) != 0U ||
        frame->dlc > CANVIEW_STM_FDCAN_MAX_DATA_BYTES)
    {
        return CANVIEW_MALFORMED;
    }
    const uint32_t maximum_id = (frame->flags & CANVIEW_STM_FDCAN_FRAME_IDE) != 0U
                                    ? CANVIEW_WIRE_CAN_EXTENDED_ID_MAX
                                    : CANVIEW_WIRE_CAN_STANDARD_ID_MAX;
    if (frame->can_id > maximum_id)
    {
        return CANVIEW_MALFORMED;
    }
    const size_t first_padding = (frame->flags & CANVIEW_STM_FDCAN_FRAME_RTR) != 0U
                                     ? 0U
                                     : (size_t)frame->dlc;
    for (size_t index = first_padding; index < CANVIEW_STM_FDCAN_MAX_DATA_BYTES; ++index)
    {
        if (frame->data[index] != 0U)
        {
            return CANVIEW_MALFORMED;
        }
    }
    return CANVIEW_OK;
}

static canview_status_t extend_timestamp(canview_stm_fdcan_timestamp_state_t *state,
                                         uint32_t source_timestamp_us, uint64_t *timestamp_us,
                                         bool *wrapped)
{
    if (state == NULL || timestamp_us == NULL || wrapped == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    *wrapped = false;
    if (!state->initialized)
    {
        state->initialized = true;
        state->last_source_timestamp_us = source_timestamp_us;
        state->timestamp_epoch_us = 0U;
        state->extended_timestamp_us = (uint64_t)source_timestamp_us;
        *timestamp_us = (uint64_t)source_timestamp_us;
        return CANVIEW_OK;
    }
    const uint32_t previous_source = state->last_source_timestamp_us;
    const bool source_is_lower = source_timestamp_us < previous_source;
    const uint32_t distance = source_is_lower ? previous_source - source_timestamp_us
                                              : source_timestamp_us - previous_source;
    if (distance > CANVIEW_STM_FDCAN_TIMESTAMP_MAX_DELTA)
    {
        if (!source_is_lower || state->timestamp_epoch_us >
                                   UINT64_MAX - CANVIEW_STM_FDCAN_TIMESTAMP_WRAP_INCREMENT)
        {
            return source_is_lower ? CANVIEW_OVERSIZE : CANVIEW_MALFORMED;
        }
        state->timestamp_epoch_us += CANVIEW_STM_FDCAN_TIMESTAMP_WRAP_INCREMENT;
        *wrapped = true;
    }
    if (!source_is_lower || *wrapped)
    {
        state->last_source_timestamp_us = source_timestamp_us;
    }
    if (state->timestamp_epoch_us > UINT64_MAX - (uint64_t)source_timestamp_us)
    {
        return CANVIEW_OVERSIZE;
    }
    *timestamp_us = state->timestamp_epoch_us + (uint64_t)source_timestamp_us;
    if (*timestamp_us > state->extended_timestamp_us)
    {
        state->extended_timestamp_us = *timestamp_us;
    }
    return CANVIEW_OK;
}

static uint8_t wire_flags(uint8_t input_flags)
{
    uint8_t result = 0U;
    if ((input_flags & CANVIEW_STM_FDCAN_FRAME_IDE) != 0U)
    {
        result |= CANVIEW_STM_FDCAN_WIRE_IDE;
    }
    if ((input_flags & CANVIEW_STM_FDCAN_FRAME_RTR) != 0U)
    {
        result |= CANVIEW_STM_FDCAN_WIRE_RTR;
    }
    if ((input_flags & CANVIEW_STM_FDCAN_FRAME_ERROR) != 0U)
    {
        result |= CANVIEW_STM_FDCAN_WIRE_ERROR;
    }
    return result;
}

static void inventory_percentiles(const canview_stm_fdcan_inventory_entry_t *source,
                                  uint32_t *p50_us, uint32_t *p95_us)
{
    uint32_t sorted[CANVIEW_STM_FDCAN_PERIOD_SAMPLE_CAPACITY] = {0U};
    const size_t count = source->period_sample_count;
    for (size_t index = 0U; index < count; ++index)
    {
        sorted[index] = source->period_samples[index];
    }
    for (size_t index = 1U; index < count; ++index)
    {
        const uint32_t value = sorted[index];
        size_t position = index;
        while (position != 0U && sorted[position - 1U] > value)
        {
            sorted[position] = sorted[position - 1U];
            --position;
        }
        sorted[position] = value;
    }
    if (count == 0U)
    {
        *p50_us = 0U;
        *p95_us = 0U;
        return;
    }
    *p50_us = sorted[(count - 1U) / 2U];
    *p95_us = sorted[(count * 95U + 99U) / 100U - 1U];
}

static void inventory_update(canview_stm_fdcan_capture_t *capture,
                             const canview_stm_fdcan_record_t *record)
{
    const uint32_t mask = capture->critical.enter(capture->critical.context);
    canview_stm_fdcan_inventory_entry_t *entry = NULL;
    for (size_t index = 0U; index < capture->inventory_count; ++index)
    {
        canview_stm_fdcan_inventory_entry_t *const candidate = &capture->inventory[index];
        if (candidate->valid && candidate->bus_id == record->bus_id &&
            candidate->flags == record->flags && candidate->dlc == record->dlc &&
            candidate->can_id == record->can_id)
        {
            entry = candidate;
            break;
        }
    }
    if (entry == NULL)
    {
        if (capture->inventory_count >= CANVIEW_STM_FDCAN_INVENTORY_CAPACITY)
        {
            increment_saturating(&capture->inventory_dropped);
            capture->channels[record->bus_id].status_flags |=
                CANVIEW_STM_FDCAN_STATUS_INVENTORY_FULL;
            capture->critical.leave(capture->critical.context, mask);
            return;
        }
        entry = &capture->inventory[capture->inventory_count];
        ++capture->inventory_count;
        memset(entry, 0, sizeof(*entry));
        entry->valid = true;
        entry->bus_id = record->bus_id;
        entry->flags = record->flags;
        entry->dlc = record->dlc;
        entry->can_id = record->can_id;
        entry->first_timestamp_us = record->timestamp_us;
        entry->last_timestamp_us = record->timestamp_us;
        memcpy(entry->last_data, record->data, sizeof(entry->last_data));
        entry->frame_count = 1U;
        capture->critical.leave(capture->critical.context, mask);
        return;
    }

    if (record->timestamp_us >= entry->last_timestamp_us && entry->frame_count != 0U)
    {
        const uint64_t delta = record->timestamp_us - entry->last_timestamp_us;
        const uint32_t period = delta > UINT32_MAX ? UINT32_MAX : (uint32_t)delta;
        if (entry->period_sample_count < CANVIEW_STM_FDCAN_PERIOD_SAMPLE_CAPACITY)
        {
            entry->period_samples[entry->period_sample_count] = period;
            ++entry->period_sample_count;
        }
        else
        {
            entry->period_samples[entry->period_sample_index] = period;
            entry->period_sample_index = (uint8_t)((entry->period_sample_index + 1U) %
                                                   CANVIEW_STM_FDCAN_PERIOD_SAMPLE_CAPACITY);
        }
    }
    uint64_t bit_change_mask = 0U;
    for (size_t index = 0U; index < CANVIEW_STM_FDCAN_MAX_DATA_BYTES; ++index)
    {
        bit_change_mask |= (uint64_t)(entry->last_data[index] ^ record->data[index]) << (index * 8U);
    }
    if (bit_change_mask != 0U)
    {
        increment_saturating(&entry->change_count);
        entry->bit_change_mask |= bit_change_mask;
    }
    increment_saturating(&entry->frame_count);
    entry->last_timestamp_us = record->timestamp_us;
    memcpy(entry->last_data, record->data, sizeof(entry->last_data));
    if (entry->frame_count > 1U && entry->last_timestamp_us >= entry->first_timestamp_us)
    {
        const uint64_t duration = entry->last_timestamp_us - entry->first_timestamp_us;
        if (duration != 0U)
        {
            const uint64_t intervals = (uint64_t)entry->frame_count - 1U;
            const uint64_t rate = (intervals * UINT64_C(10000000)) / duration;
            entry->rate_tenth_hz = rate > UINT16_MAX ? UINT16_MAX : (uint16_t)rate;
        }
    }
    capture->critical.leave(capture->critical.context, mask);
}

canview_status_t canview_stm_fdcan_decode_element(const uint32_t words[4],
                                                  uint32_t source_timestamp_us,
                                                  canview_stm_fdcan_rx_frame_t *frame)
{
    if (words == NULL || frame == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const uint32_t identifier_word = words[0];
    const uint32_t control_word = words[1];
    const bool extended = (identifier_word & CANVIEW_STM_FDCAN_ELEMENT_EXTENDED) != 0U;
    const bool remote = (identifier_word & CANVIEW_STM_FDCAN_ELEMENT_REMOTE) != 0U;
    const bool fd = (control_word & CANVIEW_STM_FDCAN_ELEMENT_FDF) != 0U;
    const bool bit_rate_switch = (control_word & CANVIEW_STM_FDCAN_ELEMENT_BRS) != 0U;
    const uint8_t dlc = (uint8_t)((control_word & CANVIEW_STM_FDCAN_ELEMENT_DLC_MASK) >> 16U);
    const canview_stm_fdcan_rx_frame_t decoded = {
        .source_timestamp_us = source_timestamp_us,
        .can_id = extended ? identifier_word & CANVIEW_STM_FDCAN_ELEMENT_EXTENDED_ID_MASK
                           : (identifier_word >> 18U) & CANVIEW_STM_FDCAN_ELEMENT_STANDARD_ID_MASK,
        .dlc = dlc,
        .flags = (uint8_t)((extended ? CANVIEW_STM_FDCAN_FRAME_IDE : 0U) |
                           (remote ? CANVIEW_STM_FDCAN_FRAME_RTR : 0U) |
                           (fd ? CANVIEW_STM_FDCAN_FRAME_FD : 0U) |
                           (bit_rate_switch ? CANVIEW_STM_FDCAN_FRAME_BRS : 0U)),
        .data = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}};
    *frame = decoded;
    if (!fd && dlc <= CANVIEW_STM_FDCAN_MAX_DATA_BYTES && !remote)
    {
        for (size_t index = 0U; index < CANVIEW_STM_FDCAN_MAX_DATA_BYTES; ++index)
        {
            const uint32_t word = index < 4U ? words[2] : words[3];
            const uint32_t shift = ((uint32_t)index % 4U) * 8U;
            if (index < (size_t)dlc)
            {
                frame->data[index] = (uint8_t)(word >> shift);
            }
        }
    }
    return CANVIEW_OK;
}

canview_status_t canview_stm_fdcan_capture_init(
    canview_stm_fdcan_capture_t *capture,
    const canview_stm_fdcan_profile_t profiles[CANVIEW_STM_FDCAN_CHANNEL_COUNT],
    const canview_stm_critical_t *critical, canview_stm_fdcan_filter_fn *filter,
    void *filter_context)
{
    if (capture == NULL || profiles == NULL || critical == NULL || critical->enter == NULL ||
        critical->leave == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (capture->initialized)
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    for (size_t index = 0U; index < CANVIEW_STM_FDCAN_CHANNEL_COUNT; ++index)
    {
        const canview_status_t profile_status =
            canview_stm_fdcan_channel_profile_validate(index, &profiles[index]);
        if (profile_status != CANVIEW_OK)
        {
            return profile_status;
        }
    }
    const canview_stm_critical_t copied_critical = *critical;
    memset(capture, 0, sizeof(*capture));
    capture->critical = copied_critical;
    capture->filter = filter;
    capture->filter_context = filter_context;
    for (size_t index = 0U; index < CANVIEW_STM_FDCAN_CHANNEL_COUNT; ++index)
    {
        canview_stm_fdcan_channel_t *const channel = &capture->channels[index];
        channel->enabled = profiles[index].enabled;
        channel->bitrate = profiles[index].nominal_bitrate;
        channel->state = profiles[index].enabled ? CANVIEW_STM_FDCAN_BUS_NO_DATA
                                                  : CANVIEW_STM_FDCAN_BUS_UNKNOWN_BITRATE;
        channel->status_flags = profiles[index].enabled ? CANVIEW_STM_FDCAN_STATUS_CONFIGURED : 0U;
    }
    capture->initialized = true;
    return CANVIEW_OK;
}

canview_status_t canview_stm_fdcan_capture_ingest(canview_stm_fdcan_capture_t *capture,
                                                  size_t channel_index,
                                                  const canview_stm_fdcan_rx_frame_t *frame)
{
    if (capture == NULL || frame == NULL || channel_index >= CANVIEW_STM_FDCAN_CHANNEL_COUNT ||
        !capture->initialized)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    canview_stm_fdcan_channel_t *const channel = &capture->channels[channel_index];
    if (!channel->enabled)
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    const canview_status_t input_status = validate_input_frame(frame);
    const uint32_t mask = capture->critical.enter(capture->critical.context);
    if (input_status == CANVIEW_UNSUPPORTED_MESSAGE)
    {
        increment_saturating(&channel->unsupported);
        channel->status_flags |= CANVIEW_STM_FDCAN_STATUS_FD_UNSUPPORTED;
        capture->critical.leave(capture->critical.context, mask);
        return input_status;
    }
    if (input_status != CANVIEW_OK)
    {
        increment_saturating(&channel->malformed);
        channel->status_flags |= CANVIEW_STM_FDCAN_STATUS_MALFORMED;
        channel->state = CANVIEW_STM_FDCAN_BUS_FAULT;
        capture->critical.leave(capture->critical.context, mask);
        return input_status;
    }
    /* A rejected frame must not advance the timestamp high-water mark. */
    if (channel->count >= CANVIEW_STM_FDCAN_RING_CAPACITY)
    {
        increment_saturating(&channel->dropped);
        channel->status_flags |= CANVIEW_STM_FDCAN_STATUS_DROPPED;
        capture->critical.leave(capture->critical.context, mask);
        return CANVIEW_RESOURCE_BUSY;
    }
    canview_stm_fdcan_timestamp_state_t timestamp_state = {
        .last_source_timestamp_us = capture->last_source_timestamp_us,
        .timestamp_epoch_us = capture->timestamp_epoch_us,
        .extended_timestamp_us = capture->extended_timestamp_us,
        .initialized = capture->timestamp_initialized};
    uint64_t timestamp_us = 0U;
    bool wrapped = false;
    const canview_status_t timestamp_status =
        extend_timestamp(&timestamp_state, frame->source_timestamp_us, &timestamp_us, &wrapped);
    if (timestamp_status != CANVIEW_OK)
    {
        increment_saturating(&channel->malformed);
        channel->status_flags |= CANVIEW_STM_FDCAN_STATUS_MALFORMED;
        channel->state = CANVIEW_STM_FDCAN_BUS_FAULT;
        capture->critical.leave(capture->critical.context, mask);
        return timestamp_status;
    }
    if (channel->timestamp_initialized && timestamp_us < channel->last_timestamp_us)
    {
        increment_saturating(&channel->malformed);
        channel->status_flags |= CANVIEW_STM_FDCAN_STATUS_MALFORMED;
        channel->state = CANVIEW_STM_FDCAN_BUS_FAULT;
        capture->critical.leave(capture->critical.context, mask);
        return CANVIEW_MALFORMED;
    }
    capture->last_source_timestamp_us = timestamp_state.last_source_timestamp_us;
    capture->timestamp_epoch_us = timestamp_state.timestamp_epoch_us;
    capture->extended_timestamp_us = timestamp_state.extended_timestamp_us;
    capture->timestamp_initialized = timestamp_state.initialized;
    canview_stm_fdcan_record_t record = {0};
    record.timestamp_us = timestamp_us;
    record.can_id = frame->can_id;
    record.bus_id = (uint8_t)channel_index;
    record.flags = wire_flags(frame->flags);
    record.dlc = frame->dlc;
    for (size_t index = 0U; index < CANVIEW_STM_FDCAN_MAX_DATA_BYTES; ++index)
    {
        record.data[index] = frame->data[index];
    }
    channel->records[channel->write_index] = record;
    channel->write_index = (channel->write_index + 1U) % CANVIEW_STM_FDCAN_RING_CAPACITY;
    ++channel->count;
    if (channel->count > channel->high_water)
    {
        channel->high_water = channel->count;
    }
    increment_saturating(&channel->accepted);
    channel->last_timestamp_us = timestamp_us;
    channel->timestamp_initialized = true;
    channel->data_seen = true;
    channel->status_flags |= CANVIEW_STM_FDCAN_STATUS_DATA_SEEN;
    if (wrapped)
    {
        channel->status_flags |= CANVIEW_STM_FDCAN_STATUS_TIMESTAMP_WRAP;
    }
    channel->status_flags &= (uint8_t)~CANVIEW_STM_FDCAN_STATUS_NO_DATA;
    if (channel->state == CANVIEW_STM_FDCAN_BUS_NO_DATA ||
        channel->state == CANVIEW_STM_FDCAN_BUS_ERROR_ACTIVE)
    {
        channel->state = CANVIEW_STM_FDCAN_BUS_ERROR_ACTIVE;
    }
    capture->critical.leave(capture->critical.context, mask);
    return CANVIEW_OK;
}

canview_status_t canview_stm_fdcan_capture_record_drops(canview_stm_fdcan_capture_t *capture,
                                                         size_t channel_index,
                                                         uint32_t dropped)
{
    if (capture == NULL || !capture->initialized ||
        channel_index >= CANVIEW_STM_FDCAN_CHANNEL_COUNT)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (dropped == 0U)
    {
        return CANVIEW_OK;
    }
    const uint32_t mask = capture->critical.enter(capture->critical.context);
    canview_stm_fdcan_channel_t *const channel = &capture->channels[channel_index];
    if (!channel->enabled)
    {
        capture->critical.leave(capture->critical.context, mask);
        return CANVIEW_RESOURCE_BUSY;
    }
    add_saturating(&channel->dropped, dropped);
    channel->status_flags |= CANVIEW_STM_FDCAN_STATUS_DROPPED;
    capture->critical.leave(capture->critical.context, mask);
    return CANVIEW_OK;
}

static bool load_pending(canview_stm_fdcan_capture_t *capture, size_t channel_index)
{
    if (capture->pending_valid[channel_index])
    {
        return true;
    }
    const uint32_t mask = capture->critical.enter(capture->critical.context);
    const canview_stm_fdcan_channel_t *const channel = &capture->channels[channel_index];
    const bool available = channel->count != 0U;
    if (available)
    {
        capture->pending[channel_index] = channel->records[channel->read_index];
        capture->pending_valid[channel_index] = true;
    }
    capture->critical.leave(capture->critical.context, mask);
    return available;
}

static bool record_equal(const canview_stm_fdcan_record_t *first,
                         const canview_stm_fdcan_record_t *second)
{
    if (first->timestamp_us != second->timestamp_us || first->can_id != second->can_id ||
        first->bus_id != second->bus_id || first->flags != second->flags || first->dlc != second->dlc)
    {
        return false;
    }
    for (size_t index = 0U; index < CANVIEW_STM_FDCAN_MAX_DATA_BYTES; ++index)
    {
        if (first->data[index] != second->data[index])
        {
            return false;
        }
    }
    return true;
}

static canview_status_t remove_pending(canview_stm_fdcan_capture_t *capture, size_t channel_index,
                                       const canview_stm_fdcan_record_t *record)
{
    const uint32_t mask = capture->critical.enter(capture->critical.context);
    canview_stm_fdcan_channel_t *const channel = &capture->channels[channel_index];
    canview_status_t status = CANVIEW_INCOMPLETE;
    if (channel->count != 0U && record_equal(&channel->records[channel->read_index], record))
    {
        channel->read_index = (channel->read_index + 1U) % CANVIEW_STM_FDCAN_RING_CAPACITY;
        --channel->count;
        capture->pending_valid[channel_index] = false;
        status = CANVIEW_OK;
    }
    else if (channel->count != 0U)
    {
        status = CANVIEW_MALFORMED;
    }
    capture->critical.leave(capture->critical.context, mask);
    return status;
}

static bool select_oldest(canview_stm_fdcan_capture_t *capture, size_t *channel_index)
{
    bool found = false;
    size_t selected = 0U;
    for (size_t index = 0U; index < CANVIEW_STM_FDCAN_CHANNEL_COUNT; ++index)
    {
        if (!load_pending(capture, index))
        {
            continue;
        }
        if (!found || capture->pending[index].timestamp_us < capture->pending[selected].timestamp_us ||
            (capture->pending[index].timestamp_us == capture->pending[selected].timestamp_us &&
             index < selected))
        {
            selected = index;
            found = true;
        }
    }
    if (found)
    {
        *channel_index = selected;
    }
    return found;
}

static uint32_t current_dropped(const canview_stm_fdcan_capture_t *capture, size_t channel_index)
{
    canview_stm_fdcan_capture_t *const mutable_capture = (canview_stm_fdcan_capture_t *)capture;
    const uint32_t mask = mutable_capture->critical.enter(mutable_capture->critical.context);
    const uint32_t dropped = mutable_capture->channels[channel_index].dropped;
    mutable_capture->critical.leave(mutable_capture->critical.context, mask);
    return dropped;
}

static uint8_t dropped_since_last(canview_stm_fdcan_capture_t *capture)
{
    uint64_t total = 0U;
    for (size_t index = 0U; index < CANVIEW_STM_FDCAN_CHANNEL_COUNT; ++index)
    {
        const uint32_t current = current_dropped(capture, index);
        const uint32_t previous = capture->reported_dropped[index];
        total += (uint64_t)(current >= previous ? current - previous : current);
        capture->reported_dropped[index] = current;
    }
    return (uint8_t)(total > UINT8_MAX ? UINT8_MAX : total);
}

static void increment_filtered(canview_stm_fdcan_capture_t *capture, size_t channel_index)
{
    const uint32_t mask = capture->critical.enter(capture->critical.context);
    increment_saturating(&capture->channels[channel_index].filtered);
    capture->critical.leave(capture->critical.context, mask);
}

static void convert_record(const canview_stm_fdcan_record_t *source, uint64_t base_time_us,
                           canview_wire_can_record_t *destination)
{
    const uint64_t delta = source->timestamp_us - base_time_us;
    destination->delta_us = (uint16_t)delta;
    destination->bus_id = source->bus_id;
    destination->flags = source->flags;
    destination->dlc = source->dlc;
    destination->can_id = source->can_id;
    for (size_t index = 0U; index < CANVIEW_STM_FDCAN_MAX_DATA_BYTES; ++index)
    {
        destination->data[index] = source->data[index];
    }
}

canview_status_t canview_stm_fdcan_capture_build_batch(canview_stm_fdcan_capture_t *capture,
                                                        canview_wire_can_batch_t *batch)
{
    if (capture == NULL || batch == NULL || !capture->initialized)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (capture->building)
    {
        capture->reentry_requested = true;
        return CANVIEW_RESOURCE_BUSY;
    }
    capture->building = true;
    capture->reentry_requested = false;
    canview_wire_can_batch_t result = {0};
    bool base_set = false;
    while ((size_t)result.count < CANVIEW_WIRE_CAN_MAX_RECORDS)
    {
        size_t selected = 0U;
        if (!select_oldest(capture, &selected))
        {
            break;
        }
        const canview_stm_fdcan_record_t candidate = capture->pending[selected];
        bool accepted = true;
        if (capture->filter != NULL)
        {
            accepted = capture->filter(&candidate, capture->filter_context);
            if (capture->reentry_requested)
            {
                capture->building = false;
                memset(batch, 0, sizeof(*batch));
                return CANVIEW_RESOURCE_BUSY;
            }
        }
        if (!accepted)
        {
            const canview_status_t remove_status = remove_pending(capture, selected, &candidate);
            if (remove_status != CANVIEW_OK)
            {
                capture->building = false;
                memset(batch, 0, sizeof(*batch));
                return remove_status;
            }
            increment_filtered(capture, selected);
            inventory_update(capture, &candidate);
            continue;
        }
        if (!base_set)
        {
            result.base_time_us = candidate.timestamp_us;
            base_set = true;
        }
        else
        {
            if (candidate.timestamp_us < result.base_time_us)
            {
                capture->building = false;
                memset(batch, 0, sizeof(*batch));
                return CANVIEW_MALFORMED;
            }
            if (candidate.timestamp_us - result.base_time_us > UINT16_MAX)
            {
                break;
            }
        }
        const canview_status_t remove_status = remove_pending(capture, selected, &candidate);
        if (remove_status != CANVIEW_OK)
        {
            capture->building = false;
            memset(batch, 0, sizeof(*batch));
            return remove_status;
        }
        inventory_update(capture, &candidate);
        convert_record(&candidate, result.base_time_us, &result.records[result.count]);
        ++result.count;
        if (capture->reentry_requested)
        {
            capture->building = false;
            memset(batch, 0, sizeof(*batch));
            return CANVIEW_RESOURCE_BUSY;
        }
    }
    const uint8_t drops = dropped_since_last(capture);
    result.dropped_since_last = drops;
    capture->building = false;
    if (result.count == 0U && drops == 0U)
    {
        memset(batch, 0, sizeof(*batch));
        return CANVIEW_INCOMPLETE;
    }
    *batch = result;
    return CANVIEW_OK;
}

canview_status_t canview_stm_fdcan_capture_observe(canview_stm_fdcan_capture_t *capture,
                                                   uint64_t now_us)
{
    if (capture == NULL || !capture->initialized)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const uint32_t mask = capture->critical.enter(capture->critical.context);
    if (capture->timestamp_initialized && now_us < capture->extended_timestamp_us)
    {
        for (size_t index = 0U; index < CANVIEW_STM_FDCAN_CHANNEL_COUNT; ++index)
        {
            if (capture->channels[index].enabled)
            {
                capture->channels[index].state = CANVIEW_STM_FDCAN_BUS_FAULT;
                capture->channels[index].status_flags |= CANVIEW_STM_FDCAN_STATUS_MALFORMED;
            }
        }
        capture->critical.leave(capture->critical.context, mask);
        return CANVIEW_MALFORMED;
    }
    if (capture->timestamp_initialized && now_us > capture->extended_timestamp_us)
    {
        capture->extended_timestamp_us = now_us;
    }
    bool time_error = false;
    for (size_t index = 0U; index < CANVIEW_STM_FDCAN_CHANNEL_COUNT; ++index)
    {
        canview_stm_fdcan_channel_t *const channel = &capture->channels[index];
        if (!channel->enabled || channel->state == CANVIEW_STM_FDCAN_BUS_UNKNOWN_BITRATE ||
            channel->state == CANVIEW_STM_FDCAN_BUS_ERROR_PASSIVE ||
            channel->state == CANVIEW_STM_FDCAN_BUS_OFF ||
            channel->state == CANVIEW_STM_FDCAN_BUS_FAULT)
        {
            continue;
        }
        if (channel->data_seen && now_us < channel->last_timestamp_us)
        {
            channel->state = CANVIEW_STM_FDCAN_BUS_FAULT;
            channel->status_flags |= CANVIEW_STM_FDCAN_STATUS_MALFORMED;
            time_error = true;
            continue;
        }
        if (!channel->data_seen || now_us - channel->last_timestamp_us >=
                                      CANVIEW_STM_FDCAN_NO_DATA_TIMEOUT_US)
        {
            channel->state = CANVIEW_STM_FDCAN_BUS_NO_DATA;
            channel->status_flags |= CANVIEW_STM_FDCAN_STATUS_NO_DATA;
        }
        else
        {
            channel->state = CANVIEW_STM_FDCAN_BUS_ERROR_ACTIVE;
            channel->status_flags &= (uint8_t)~CANVIEW_STM_FDCAN_STATUS_NO_DATA;
        }
    }
    capture->critical.leave(capture->critical.context, mask);
    return time_error ? CANVIEW_MALFORMED : CANVIEW_OK;
}

canview_status_t canview_stm_fdcan_capture_set_status(
    canview_stm_fdcan_capture_t *capture, size_t channel_index, canview_stm_fdcan_bus_state_t state,
    uint16_t rx_error_count, uint16_t tx_error_count, uint32_t bus_off_count,
    uint32_t last_error, uint64_t timestamp_us)
{
    if (capture == NULL || !capture->initialized || channel_index >= CANVIEW_STM_FDCAN_CHANNEL_COUNT ||
        !bus_state_valid(state))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const uint32_t mask = capture->critical.enter(capture->critical.context);
    canview_stm_fdcan_channel_t *const channel = &capture->channels[channel_index];
    if (!channel->enabled && state != CANVIEW_STM_FDCAN_BUS_UNKNOWN_BITRATE)
    {
        capture->critical.leave(capture->critical.context, mask);
        return CANVIEW_RESOURCE_BUSY;
    }
    if (channel->timestamp_initialized && timestamp_us < channel->last_timestamp_us)
    {
        channel->state = CANVIEW_STM_FDCAN_BUS_FAULT;
        channel->status_flags |= CANVIEW_STM_FDCAN_STATUS_MALFORMED;
        capture->critical.leave(capture->critical.context, mask);
        return CANVIEW_MALFORMED;
    }
    if (capture->timestamp_initialized && timestamp_us < capture->extended_timestamp_us)
    {
        channel->state = CANVIEW_STM_FDCAN_BUS_FAULT;
        channel->status_flags |= CANVIEW_STM_FDCAN_STATUS_MALFORMED;
        capture->critical.leave(capture->critical.context, mask);
        return CANVIEW_MALFORMED;
    }
    if (!capture->timestamp_initialized || timestamp_us > capture->extended_timestamp_us)
    {
        capture->timestamp_initialized = true;
        capture->last_source_timestamp_us = (uint32_t)timestamp_us;
        capture->timestamp_epoch_us = timestamp_us & UINT64_C(0xffffffff00000000);
        capture->extended_timestamp_us = timestamp_us;
    }
    channel->state = state;
    channel->rx_error_count = rx_error_count;
    channel->tx_error_count = tx_error_count;
    channel->bus_off_count = bus_off_count;
    channel->last_error = last_error;
    channel->last_timestamp_us = timestamp_us;
    channel->timestamp_initialized = true;
    if (state == CANVIEW_STM_FDCAN_BUS_NO_DATA)
    {
        channel->status_flags |= CANVIEW_STM_FDCAN_STATUS_NO_DATA;
    }
    else
    {
        channel->status_flags &= (uint8_t)~CANVIEW_STM_FDCAN_STATUS_NO_DATA;
    }
    capture->critical.leave(capture->critical.context, mask);
    return CANVIEW_OK;
}

canview_status_t canview_stm_fdcan_capture_get_stats(
    const canview_stm_fdcan_capture_t *capture, size_t channel_index,
    canview_stm_fdcan_channel_stats_t *stats)
{
    if (capture == NULL || stats == NULL || !capture->initialized ||
        channel_index >= CANVIEW_STM_FDCAN_CHANNEL_COUNT)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    canview_stm_fdcan_capture_t *const mutable_capture = (canview_stm_fdcan_capture_t *)capture;
    const uint32_t mask = mutable_capture->critical.enter(mutable_capture->critical.context);
    const canview_stm_fdcan_channel_t *const channel = &mutable_capture->channels[channel_index];
    const canview_stm_fdcan_channel_stats_t snapshot = {
        .state = channel->state,
        .status_flags = channel->status_flags,
        .rx_error_count = channel->rx_error_count,
        .tx_error_count = channel->tx_error_count,
        .bitrate = channel->bitrate,
        .bus_off_count = channel->bus_off_count,
        .last_error = channel->last_error,
        .last_timestamp_us = channel->last_timestamp_us,
        .accepted_frames = channel->accepted,
        .dropped_frames = channel->dropped,
        .unsupported_frames = channel->unsupported,
        .malformed_frames = channel->malformed,
        .filtered_frames = channel->filtered,
        .queued_frames = channel->count,
        .high_water_frames = channel->high_water};
    *stats = snapshot;
    mutable_capture->critical.leave(mutable_capture->critical.context, mask);
    return CANVIEW_OK;
}

canview_status_t canview_stm_fdcan_capture_get_inventory(
    const canview_stm_fdcan_capture_t *capture, size_t index,
    canview_stm_fdcan_inventory_entry_t *entry)
{
    if (capture == NULL || entry == NULL || !capture->initialized)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    canview_stm_fdcan_capture_t *const mutable_capture = (canview_stm_fdcan_capture_t *)capture;
    const uint32_t mask = mutable_capture->critical.enter(mutable_capture->critical.context);
    if (index >= mutable_capture->inventory_count)
    {
        mutable_capture->critical.leave(mutable_capture->critical.context, mask);
        return CANVIEW_INCOMPLETE;
    }
    const canview_stm_fdcan_inventory_entry_t snapshot = mutable_capture->inventory[index];
    mutable_capture->critical.leave(mutable_capture->critical.context, mask);
    *entry = snapshot;
    inventory_percentiles(entry, &entry->period_p50_us, &entry->period_p95_us);
    return CANVIEW_OK;
}

canview_status_t canview_stm_fdcan_capture_get_inventory_state(
    const canview_stm_fdcan_capture_t *capture, size_t *count, uint32_t *dropped)
{
    if (capture == NULL || count == NULL || dropped == NULL || !capture->initialized)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    canview_stm_fdcan_capture_t *const mutable_capture = (canview_stm_fdcan_capture_t *)capture;
    const uint32_t mask = mutable_capture->critical.enter(mutable_capture->critical.context);
    *count = mutable_capture->inventory_count;
    *dropped = mutable_capture->inventory_dropped;
    mutable_capture->critical.leave(mutable_capture->critical.context, mask);
    return CANVIEW_OK;
}
