/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_stm_fdcan_capture.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition)                                                                        \
    do                                                                                          \
    {                                                                                           \
        if (!(condition))                                                                       \
        {                                                                                       \
            (void)fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition);              \
            exit(EXIT_FAILURE);                                                                 \
        }                                                                                       \
    } while (0)

static uint32_t test_enter(void *context)
{
    (void)context;
    return 0U;
}

static void test_leave(void *context, uint32_t saved_mask)
{
    (void)context;
    (void)saved_mask;
}

static canview_stm_fdcan_profile_t valid_profile(uint32_t bitrate)
{
    canview_stm_fdcan_profile_t profile = {0};
    CHECK(canview_stm_fdcan_profile_for_bitrate(bitrate, 0U, &profile) == CANVIEW_OK);
    profile.transceiver_known = true;
    profile.transceiver = bitrate == CANVIEW_STM_FDCAN_CAN3_BITRATE
                              ? CANVIEW_STM_FDCAN_TRANSCEIVER_MAX3055
                              : CANVIEW_STM_FDCAN_TRANSCEIVER_TCAN1046;
    CHECK(canview_stm_fdcan_profile_validate(&profile) == CANVIEW_OK);
    return profile;
}

static canview_stm_fdcan_rx_frame_t frame(uint32_t timestamp_us, uint32_t can_id, uint8_t dlc,
                                          uint8_t flags)
{
    canview_stm_fdcan_rx_frame_t result = {0};
    result.source_timestamp_us = timestamp_us;
    result.can_id = can_id;
    result.dlc = dlc;
    result.flags = flags;
    return result;
}

static canview_stm_critical_t test_critical(void)
{
    const canview_stm_critical_t critical = {test_enter, test_leave, NULL};
    return critical;
}

static void init_capture(canview_stm_fdcan_capture_t *capture,
                         const canview_stm_fdcan_profile_t profiles[3],
                         canview_stm_fdcan_filter_fn *filter, void *filter_context)
{
    const canview_stm_critical_t critical = test_critical();
    CHECK(canview_stm_fdcan_capture_init(capture, profiles, &critical, filter, filter_context) ==
          CANVIEW_OK);
}

static void profile_tests(void)
{
    canview_stm_fdcan_profile_t profile = {0};
    CHECK(canview_stm_fdcan_profile_for_bitrate(500000U, 0U, &profile) == CANVIEW_OK);
    CHECK(profile.enabled && profile.bitrate_known && !profile.transceiver_known &&
          profile.nominal_timing.prescaler == 10U && profile.nominal_timing.time_segment1 == 13U &&
          profile.nominal_timing.time_segment2 == 2U && profile.nominal_timing.sync_jump_width == 2U);
    CHECK(canview_stm_fdcan_profile_validate(&profile) == CANVIEW_INVALID_ARGUMENT);
    profile.transceiver_known = true;
    profile.transceiver = CANVIEW_STM_FDCAN_TRANSCEIVER_TCAN1046;
    CHECK(canview_stm_fdcan_profile_validate(&profile) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_profile_for_bitrate(500000U, 2000000U, &profile) ==
          CANVIEW_UNSUPPORTED_MESSAGE);
    CHECK(canview_stm_fdcan_profile_for_bitrate(333333U, 0U, &profile) ==
          CANVIEW_UNSUPPORTED_MESSAGE);
    CHECK(canview_stm_fdcan_profile_for_bitrate(500000U, 0U, NULL) == CANVIEW_INVALID_ARGUMENT);

    const canview_stm_fdcan_profile_t disabled = {0};
    CHECK(canview_stm_fdcan_profile_validate(&disabled) == CANVIEW_OK);
    canview_stm_fdcan_profile_t invalid = disabled;
    invalid.enabled = true;
    CHECK(canview_stm_fdcan_profile_validate(&invalid) == CANVIEW_INVALID_ARGUMENT);
    invalid = valid_profile(125000U);
    invalid.nominal_timing.sync_jump_width = 3U;
    CHECK(canview_stm_fdcan_profile_validate(&invalid) == CANVIEW_INVALID_ARGUMENT);
    invalid = valid_profile(250000U);
    invalid.data_timing.prescaler = 1U;
    CHECK(canview_stm_fdcan_profile_validate(&invalid) == CANVIEW_UNSUPPORTED_MESSAGE);
    invalid = valid_profile(250000U);
    invalid.transceiver = CANVIEW_STM_FDCAN_TRANSCEIVER_MAX;
    CHECK(canview_stm_fdcan_profile_validate(&invalid) == CANVIEW_INVALID_ARGUMENT);
    invalid.transceiver = (canview_stm_fdcan_transceiver_t)-1;
    CHECK(canview_stm_fdcan_profile_validate(&invalid) == CANVIEW_INVALID_ARGUMENT);
}

static void board_profile_tests(void)
{
    const canview_stm_fdcan_profile_t tcan = valid_profile(500000U);
    canview_stm_fdcan_profile_t max = valid_profile(CANVIEW_STM_FDCAN_CAN3_BITRATE);
    max.transceiver = CANVIEW_STM_FDCAN_TRANSCEIVER_MAX3055;
    CHECK(canview_stm_fdcan_channel_profile_validate(0U, &tcan) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_channel_profile_validate(1U, &tcan) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_channel_profile_validate(2U, &max) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_channel_profile_validate(2U, &tcan) == CANVIEW_INVALID_ARGUMENT);
    canview_stm_fdcan_profile_t wrong_rate = max;
    wrong_rate.nominal_bitrate = 500000U;
    CHECK(canview_stm_fdcan_channel_profile_validate(2U, &wrong_rate) ==
          CANVIEW_INVALID_ARGUMENT);
    canview_stm_fdcan_profile_t wrong_phy = max;
    wrong_phy.transceiver = CANVIEW_STM_FDCAN_TRANSCEIVER_TCAN1046;
    CHECK(canview_stm_fdcan_channel_profile_validate(2U, &wrong_phy) ==
          CANVIEW_INVALID_ARGUMENT);
    const canview_stm_fdcan_profile_t disabled = {0};
    CHECK(canview_stm_fdcan_channel_profile_validate(2U, &disabled) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_channel_profile_validate(3U, &disabled) ==
          CANVIEW_INVALID_ARGUMENT);
}

static void decoder_tests(void)
{
    uint32_t words[4] = {0};
    canview_stm_fdcan_rx_frame_t decoded = {0};
    CHECK(canview_stm_fdcan_decode_element(NULL, 0U, &decoded) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_fdcan_decode_element(words, 0U, NULL) == CANVIEW_INVALID_ARGUMENT);
    words[0] = 0x123U << 18U;
    words[1] = 3U << 16U;
    words[2] = UINT32_C(0x00332211);
    words[3] = UINT32_C(0x88776655);
    CHECK(canview_stm_fdcan_decode_element(words, 77U, &decoded) == CANVIEW_OK);
    CHECK(decoded.source_timestamp_us == 77U && decoded.can_id == 0x123U && decoded.dlc == 3U &&
          decoded.flags == 0U && decoded.data[0] == 0x11U && decoded.data[1] == 0x22U &&
          decoded.data[2] == 0x33U && decoded.data[3] == 0U);
    words[0] = UINT32_C(0x40000000) | UINT32_C(0x1ABCDE);
    words[1] = 8U << 16U;
    words[2] = UINT32_C(0x04030201);
    words[3] = UINT32_C(0x08070605);
    CHECK(canview_stm_fdcan_decode_element(words, 88U, &decoded) == CANVIEW_OK);
    CHECK(decoded.can_id == 0x1ABCDEU && decoded.flags == CANVIEW_STM_FDCAN_FRAME_IDE &&
          decoded.data[7] == 8U);
    words[0] |= UINT32_C(0x20000000);
    CHECK(canview_stm_fdcan_decode_element(words, 99U, &decoded) == CANVIEW_OK);
    CHECK((decoded.flags & CANVIEW_STM_FDCAN_FRAME_RTR) != 0U && decoded.data[0] == 0U &&
          decoded.data[7] == 0U);
    words[0] = 0x123U << 18U;
    words[1] = (UINT32_C(9) << 16U) | UINT32_C(0x00200000);
    CHECK(canview_stm_fdcan_decode_element(words, 100U, &decoded) == CANVIEW_OK);
    CHECK((decoded.flags & CANVIEW_STM_FDCAN_FRAME_FD) != 0U && decoded.dlc == 9U);
}

static void stream_tests(void)
{
    const canview_stm_fdcan_profile_t profiles[3] = {
        valid_profile(500000U), valid_profile(250000U), valid_profile(125000U)};
    canview_stm_fdcan_capture_t capture = {0};
    const canview_stm_critical_t critical = test_critical();
    CHECK(canview_stm_fdcan_capture_init(NULL, profiles, &critical, NULL, NULL) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_fdcan_capture_init(&capture, profiles, NULL, NULL, NULL) ==
          CANVIEW_INVALID_ARGUMENT);
    init_capture(&capture, profiles, NULL, NULL);
    CHECK(canview_stm_fdcan_capture_init(&capture, profiles, &critical, NULL, NULL) ==
          CANVIEW_RESOURCE_BUSY);

    canview_stm_fdcan_rx_frame_t first = frame(100U, 0x100U, 1U, 0U);
    first.data[0] = 0x11U;
    canview_stm_fdcan_rx_frame_t second = frame(110U, 0x1ABCDEU, 2U,
                                                CANVIEW_STM_FDCAN_FRAME_IDE);
    second.data[0] = 0x22U;
    second.data[1] = 0x33U;
    canview_stm_fdcan_rx_frame_t third = frame(105U, 0x7FFU, 0U, 0U);
    CHECK(canview_stm_fdcan_capture_ingest(&capture, 0U, &first) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_ingest(&capture, 1U, &second) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_ingest(&capture, 2U, &third) == CANVIEW_OK);

    canview_wire_can_batch_t batch = {0};
    CHECK(canview_stm_fdcan_capture_build_batch(&capture, &batch) == CANVIEW_OK);
    CHECK(batch.base_time_us == 100U && batch.count == 3U && batch.dropped_since_last == 0U);
    CHECK(batch.records[0].bus_id == 0U && batch.records[0].can_id == 0x100U &&
          batch.records[0].delta_us == 0U && batch.records[0].data[0] == 0x11U);
    CHECK(batch.records[1].bus_id == 2U && batch.records[1].can_id == 0x7FFU &&
          batch.records[1].delta_us == 5U);
    CHECK(batch.records[2].bus_id == 1U && batch.records[2].can_id == 0x1ABCDEU &&
          batch.records[2].flags == 1U && batch.records[2].delta_us == 10U);
    uint8_t encoded[CANVIEW_WIRE_CAN_PREFIX_SIZE +
                    CANVIEW_WIRE_CAN_MAX_RECORDS * CANVIEW_WIRE_CAN_RECORD_SIZE];
    size_t written = 0U;
    CHECK(canview_wire_can_batch_encode(&batch, encoded, sizeof(encoded), &written) == CANVIEW_OK);
    CHECK(written == CANVIEW_WIRE_CAN_PREFIX_SIZE + 3U * CANVIEW_WIRE_CAN_RECORD_SIZE);
    CHECK(canview_stm_fdcan_capture_build_batch(&capture, &batch) == CANVIEW_INCOMPLETE);
    CHECK(batch.count == 0U && batch.dropped_since_last == 0U);

    canview_stm_fdcan_channel_stats_t stats = {0};
    CHECK(canview_stm_fdcan_capture_get_stats(&capture, 0U, &stats) == CANVIEW_OK);
    CHECK(stats.accepted_frames == 1U && stats.queued_frames == 0U &&
          stats.state == CANVIEW_STM_FDCAN_BUS_ERROR_ACTIVE);
    CHECK(canview_stm_fdcan_capture_get_stats(&capture, 3U, &stats) == CANVIEW_INVALID_ARGUMENT);
}

static void init_contract_tests(void)
{
    const canview_stm_fdcan_profile_t valid_profiles[3] = {
        valid_profile(500000U), valid_profile(500000U), valid_profile(125000U)};
    const canview_stm_critical_t critical = test_critical();
    canview_stm_fdcan_capture_t capture = {0};
    canview_stm_fdcan_profile_t wrong_profiles[3] = {
        valid_profiles[0], valid_profiles[1], valid_profiles[2]};
    wrong_profiles[0].transceiver = CANVIEW_STM_FDCAN_TRANSCEIVER_MAX3055;
    CHECK(canview_stm_fdcan_capture_init(&capture, wrong_profiles, &critical, NULL, NULL) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(!capture.initialized);
    CHECK(canview_stm_fdcan_capture_init(&capture, valid_profiles, &critical, NULL, NULL) ==
          CANVIEW_OK);
}

static void validation_tests(void)
{
    const canview_stm_fdcan_profile_t profiles[3] = {
        valid_profile(500000U), valid_profile(500000U), valid_profile(125000U)};
    canview_stm_fdcan_capture_t capture = {0};
    init_capture(&capture, profiles, NULL, NULL);
    canview_stm_fdcan_rx_frame_t bad = frame(1U, 2048U, 0U, 0U);
    CHECK(canview_stm_fdcan_capture_ingest(&capture, 0U, &bad) == CANVIEW_MALFORMED);
    bad = frame(2U, 536870912U, 0U, CANVIEW_STM_FDCAN_FRAME_IDE);
    CHECK(canview_stm_fdcan_capture_ingest(&capture, 0U, &bad) == CANVIEW_MALFORMED);
    bad = frame(3U, 1U, 9U, 0U);
    CHECK(canview_stm_fdcan_capture_ingest(&capture, 0U, &bad) == CANVIEW_MALFORMED);
    bad = frame(4U, 1U, 0U, UINT8_C(0x80));
    CHECK(canview_stm_fdcan_capture_ingest(&capture, 0U, &bad) == CANVIEW_MALFORMED);
    bad = frame(5U, 1U, 0U, CANVIEW_STM_FDCAN_FRAME_BRS);
    CHECK(canview_stm_fdcan_capture_ingest(&capture, 0U, &bad) == CANVIEW_MALFORMED);
    bad = frame(6U, 1U, 1U, 0U);
    bad.data[1] = 0xFFU;
    CHECK(canview_stm_fdcan_capture_ingest(&capture, 0U, &bad) == CANVIEW_MALFORMED);
    bad = frame(7U, 1U, 0U, CANVIEW_STM_FDCAN_FRAME_RTR);
    bad.data[0] = 0x01U;
    CHECK(canview_stm_fdcan_capture_ingest(&capture, 0U, &bad) == CANVIEW_MALFORMED);
    bad = frame(8U, 1U, 15U, CANVIEW_STM_FDCAN_FRAME_FD);
    CHECK(canview_stm_fdcan_capture_ingest(&capture, 0U, &bad) ==
          CANVIEW_UNSUPPORTED_MESSAGE);
    CHECK(canview_stm_fdcan_capture_ingest(&capture, 3U, &bad) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_fdcan_capture_ingest(&capture, 0U, NULL) == CANVIEW_INVALID_ARGUMENT);

    canview_stm_fdcan_channel_stats_t stats = {0};
    CHECK(canview_stm_fdcan_capture_get_stats(&capture, 0U, &stats) == CANVIEW_OK);
    CHECK(stats.malformed_frames == 7U && stats.unsupported_frames == 1U &&
          (stats.status_flags & CANVIEW_STM_FDCAN_STATUS_MALFORMED) != 0U &&
          (stats.status_flags & CANVIEW_STM_FDCAN_STATUS_FD_UNSUPPORTED) != 0U &&
          stats.queued_frames == 0U);

    const canview_stm_fdcan_profile_t disabled_profiles[3] = {{0}, {0}, {0}};
    canview_stm_fdcan_capture_t disabled_capture = {0};
    init_capture(&disabled_capture, disabled_profiles, NULL, NULL);
    CHECK(canview_stm_fdcan_capture_ingest(&disabled_capture, 0U, &bad) ==
          CANVIEW_RESOURCE_BUSY);
    CHECK(canview_stm_fdcan_capture_observe(&disabled_capture, 0U) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_get_stats(&disabled_capture, 0U, &stats) == CANVIEW_OK);
    CHECK(stats.state == CANVIEW_STM_FDCAN_BUS_UNKNOWN_BITRATE);
}

static void ring_drop_tests(void)
{
    const canview_stm_fdcan_profile_t profiles[3] = {
        valid_profile(500000U), valid_profile(500000U), valid_profile(125000U)};
    canview_stm_fdcan_capture_t capture = {0};
    init_capture(&capture, profiles, NULL, NULL);
    for (uint32_t index = 0U; index < CANVIEW_STM_FDCAN_RING_CAPACITY; ++index)
    {
        canview_stm_fdcan_rx_frame_t input = frame(index, 0x200U, 0U, 0U);
        CHECK(canview_stm_fdcan_capture_ingest(&capture, 0U, &input) == CANVIEW_OK);
    }
    canview_stm_fdcan_rx_frame_t full = frame(64U, 0x200U, 0U, 0U);
    CHECK(canview_stm_fdcan_capture_ingest(&capture, 0U, &full) == CANVIEW_RESOURCE_BUSY);
    canview_stm_fdcan_channel_stats_t stats = {0};
    CHECK(canview_stm_fdcan_capture_get_stats(&capture, 0U, &stats) == CANVIEW_OK);
    CHECK(stats.accepted_frames == CANVIEW_STM_FDCAN_RING_CAPACITY &&
          stats.dropped_frames == 1U && stats.queued_frames == CANVIEW_STM_FDCAN_RING_CAPACITY &&
          stats.high_water_frames == CANVIEW_STM_FDCAN_RING_CAPACITY);
    CHECK(capture.timestamp_initialized && capture.last_source_timestamp_us == 63U &&
          capture.extended_timestamp_us == 63U);
    canview_wire_can_batch_t batch = {0};
    CHECK(canview_stm_fdcan_capture_build_batch(&capture, &batch) == CANVIEW_OK);
    CHECK(batch.count == CANVIEW_WIRE_CAN_MAX_RECORDS && batch.dropped_since_last == 1U);
    CHECK(canview_stm_fdcan_capture_build_batch(&capture, &batch) == CANVIEW_OK);
    CHECK(batch.dropped_since_last == 0U);
}

static void drop_accounting_tests(void)
{
    const canview_stm_fdcan_profile_t profiles[3] = {
        valid_profile(500000U), valid_profile(500000U), valid_profile(125000U)};
    canview_stm_fdcan_capture_t capture = {0};
    init_capture(&capture, profiles, NULL, NULL);
    CHECK(canview_stm_fdcan_capture_record_drops(NULL, 0U, 1U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_fdcan_capture_record_drops(&capture, 3U, 1U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_fdcan_capture_record_drops(&capture, 0U, 0U) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_record_drops(&capture, 0U, 3U) == CANVIEW_OK);

    canview_stm_fdcan_channel_stats_t stats = {0};
    CHECK(canview_stm_fdcan_capture_get_stats(&capture, 0U, &stats) == CANVIEW_OK);
    CHECK(stats.dropped_frames == 3U &&
          (stats.status_flags & CANVIEW_STM_FDCAN_STATUS_DROPPED) != 0U);
    canview_wire_can_batch_t batch = {0};
    CHECK(canview_stm_fdcan_capture_build_batch(&capture, &batch) == CANVIEW_OK &&
          batch.count == 0U && batch.dropped_since_last == 3U);

    canview_stm_fdcan_capture_t saturated = {0};
    init_capture(&saturated, profiles, NULL, NULL);
    saturated.channels[0].dropped = UINT32_MAX - 1U;
    CHECK(canview_stm_fdcan_capture_record_drops(&saturated, 0U, 2U) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_get_stats(&saturated, 0U, &stats) == CANVIEW_OK &&
          stats.dropped_frames == UINT32_MAX);

    const canview_stm_fdcan_profile_t disabled_profiles[3] = {{0}, {0}, {0}};
    canview_stm_fdcan_capture_t disabled = {0};
    init_capture(&disabled, disabled_profiles, NULL, NULL);
    CHECK(canview_stm_fdcan_capture_record_drops(&disabled, 0U, 1U) == CANVIEW_RESOURCE_BUSY);
}

static void timestamp_transaction_tests(void)
{
    const canview_stm_fdcan_profile_t profiles[3] = {
        valid_profile(500000U), valid_profile(500000U), valid_profile(125000U)};
    canview_stm_fdcan_capture_t capture = {0};
    init_capture(&capture, profiles, NULL, NULL);

    canview_stm_fdcan_rx_frame_t first = frame(100U, 0x510U, 0U, 0U);
    canview_stm_fdcan_rx_frame_t backward = frame(50U, 0x511U, 0U, 0U);
    CHECK(canview_stm_fdcan_capture_ingest(&capture, 0U, &first) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_ingest(&capture, 0U, &backward) == CANVIEW_MALFORMED);
    CHECK(capture.last_source_timestamp_us == 100U && capture.timestamp_epoch_us == 0U &&
          capture.extended_timestamp_us == 100U);

    canview_stm_fdcan_channel_stats_t stats = {0};
    CHECK(canview_stm_fdcan_capture_get_stats(&capture, 0U, &stats) == CANVIEW_OK);
    CHECK(stats.accepted_frames == 1U && stats.malformed_frames == 1U && stats.queued_frames == 1U &&
          stats.state == CANVIEW_STM_FDCAN_BUS_FAULT);

    /* 다른 bus의 작은 timestamp는 cross-channel reorder로 허용하되 high-water는 유지한다. */
    canview_stm_fdcan_rx_frame_t other_bus = frame(60U, 0x512U, 0U, 0U);
    CHECK(canview_stm_fdcan_capture_ingest(&capture, 1U, &other_bus) == CANVIEW_OK);
    CHECK(capture.last_source_timestamp_us == 100U && capture.extended_timestamp_us == 100U);

    canview_stm_fdcan_rx_frame_t forward_gap = frame(UINT32_MAX, 0x513U, 0U, 0U);
    CHECK(canview_stm_fdcan_capture_ingest(&capture, 1U, &forward_gap) == CANVIEW_MALFORMED);
    CHECK(capture.last_source_timestamp_us == 100U && capture.extended_timestamp_us == 100U);
}

static void inventory_tests(void)
{
    const canview_stm_fdcan_profile_t profiles[3] = {
        valid_profile(500000U), valid_profile(500000U), valid_profile(125000U)};
    canview_stm_fdcan_capture_t capture = {0};
    init_capture(&capture, profiles, NULL, NULL);
    canview_wire_can_batch_t batch = {0};

    canview_stm_fdcan_rx_frame_t first = frame(100U, 0x555U, 2U, 0U);
    first.data[0] = 0x01U;
    CHECK(canview_stm_fdcan_capture_ingest(&capture, 0U, &first) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_build_batch(&capture, &batch) == CANVIEW_OK &&
          batch.count == 1U);

    canview_stm_fdcan_rx_frame_t changed = frame(120U, 0x555U, 2U, 0U);
    changed.data[0] = 0x03U;
    changed.data[1] = 0x04U;
    CHECK(canview_stm_fdcan_capture_ingest(&capture, 0U, &changed) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_build_batch(&capture, &batch) == CANVIEW_OK &&
          batch.count == 1U);

    canview_stm_fdcan_rx_frame_t repeated = frame(140U, 0x555U, 2U, 0U);
    repeated.data[0] = 0x03U;
    repeated.data[1] = 0x04U;
    CHECK(canview_stm_fdcan_capture_ingest(&capture, 0U, &repeated) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_build_batch(&capture, &batch) == CANVIEW_OK &&
          batch.count == 1U);

    size_t inventory_count = 0U;
    uint32_t inventory_dropped = 0U;
    CHECK(canview_stm_fdcan_capture_get_inventory_state(&capture, &inventory_count,
                                                        &inventory_dropped) == CANVIEW_OK);
    CHECK(inventory_count == 1U && inventory_dropped == 0U);
    canview_stm_fdcan_inventory_entry_t entry = {0};
    CHECK(canview_stm_fdcan_capture_get_inventory(&capture, 0U, &entry) == CANVIEW_OK);
    CHECK(entry.valid && entry.bus_id == 0U && entry.can_id == 0x555U && entry.dlc == 2U &&
          entry.frame_count == 3U && entry.change_count == 1U && entry.bit_change_mask == 0x0402U &&
          entry.first_timestamp_us == 100U && entry.last_timestamp_us == 140U &&
          entry.period_sample_count == 2U && entry.period_p50_us == 20U &&
          entry.period_p95_us == 20U && entry.rate_tenth_hz == UINT16_MAX);
    CHECK(canview_stm_fdcan_capture_get_inventory(&capture, 1U, &entry) == CANVIEW_INCOMPLETE);
    CHECK(canview_stm_fdcan_capture_get_inventory(&capture, 0U, NULL) == CANVIEW_INVALID_ARGUMENT);

    canview_stm_fdcan_capture_t saturated_capture = {0};
    init_capture(&saturated_capture, profiles, NULL, NULL);
    for (size_t index = 0U; index < CANVIEW_STM_FDCAN_INVENTORY_CAPACITY + 1U; ++index)
    {
        canview_stm_fdcan_rx_frame_t unique =
            frame((uint32_t)(100U + index), (uint32_t)(0x600U + index), 0U, 0U);
        CHECK(canview_stm_fdcan_capture_ingest(&saturated_capture, 0U, &unique) == CANVIEW_OK);
        CHECK(canview_stm_fdcan_capture_build_batch(&saturated_capture, &batch) == CANVIEW_OK &&
              batch.count == 1U);
    }
    canview_stm_fdcan_channel_stats_t stats = {0};
    CHECK(canview_stm_fdcan_capture_get_inventory_state(&saturated_capture, &inventory_count,
                                                        &inventory_dropped) == CANVIEW_OK);
    CHECK(inventory_count == CANVIEW_STM_FDCAN_INVENTORY_CAPACITY && inventory_dropped == 1U);
    CHECK(canview_stm_fdcan_capture_get_stats(&saturated_capture, 0U, &stats) == CANVIEW_OK);
    CHECK(stats.accepted_frames == CANVIEW_STM_FDCAN_INVENTORY_CAPACITY + 1U &&
          (stats.status_flags & CANVIEW_STM_FDCAN_STATUS_INVENTORY_FULL) != 0U);
}

static void timestamp_tests(void)
{
    const canview_stm_fdcan_profile_t profiles[3] = {
        valid_profile(500000U), valid_profile(500000U), valid_profile(125000U)};
    canview_stm_fdcan_capture_t capture = {0};
    init_capture(&capture, profiles, NULL, NULL);
    canview_stm_fdcan_rx_frame_t before_wrap = frame(UINT32_MAX - 2U, 1U, 0U, 0U);
    canview_stm_fdcan_rx_frame_t after_wrap = frame(1U, 2U, 0U, 0U);
    CHECK(canview_stm_fdcan_capture_ingest(&capture, 0U, &before_wrap) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_ingest(&capture, 0U, &after_wrap) == CANVIEW_OK);
    canview_wire_can_batch_t batch = {0};
    CHECK(canview_stm_fdcan_capture_build_batch(&capture, &batch) == CANVIEW_OK);
    CHECK(batch.count == 2U && batch.records[1].delta_us == 4U);
    canview_stm_fdcan_channel_stats_t stats = {0};
    CHECK(canview_stm_fdcan_capture_get_stats(&capture, 0U, &stats) == CANVIEW_OK);
    CHECK((stats.status_flags & CANVIEW_STM_FDCAN_STATUS_TIMESTAMP_WRAP) != 0U);

    canview_stm_fdcan_capture_t split_capture = {0};
    init_capture(&split_capture, profiles, NULL, NULL);
    canview_stm_fdcan_rx_frame_t near = frame(100U, 3U, 0U, 0U);
    canview_stm_fdcan_rx_frame_t far = frame(65636U, 4U, 0U, 0U);
    CHECK(canview_stm_fdcan_capture_ingest(&split_capture, 0U, &near) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_ingest(&split_capture, 0U, &far) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_build_batch(&split_capture, &batch) == CANVIEW_OK);
    CHECK(batch.count == 1U && batch.base_time_us == 100U);
    CHECK(canview_stm_fdcan_capture_build_batch(&split_capture, &batch) == CANVIEW_OK);
    CHECK(batch.count == 1U && batch.base_time_us == 65636U && batch.records[0].delta_us == 0U);

    CHECK(canview_stm_fdcan_capture_observe(&split_capture, 65636U +
                                                        CANVIEW_STM_FDCAN_NO_DATA_TIMEOUT_US) ==
          CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_get_stats(&split_capture, 0U, &stats) == CANVIEW_OK);
    CHECK(stats.state == CANVIEW_STM_FDCAN_BUS_NO_DATA &&
          (stats.status_flags & CANVIEW_STM_FDCAN_STATUS_NO_DATA) != 0U);
    CHECK(canview_stm_fdcan_capture_observe(&split_capture, 65636U) == CANVIEW_MALFORMED);
    CHECK(canview_stm_fdcan_capture_set_status(&split_capture, 0U,
                                               CANVIEW_STM_FDCAN_BUS_ERROR_PASSIVE, 5U, 1U, 0U,
                                               7U, 200000U) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_get_stats(&split_capture, 0U, &stats) == CANVIEW_OK);
    CHECK(stats.state == CANVIEW_STM_FDCAN_BUS_ERROR_PASSIVE && stats.rx_error_count == 5U &&
          stats.tx_error_count == 1U && stats.last_error == 7U);
    CHECK(canview_stm_fdcan_capture_set_status(&split_capture, 0U,
                                               CANVIEW_STM_FDCAN_BUS_OFF, 9U, 2U, 1U, 8U,
                                               200001U) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_set_status(&split_capture, 0U,
                                               CANVIEW_STM_FDCAN_BUS_STATE_MAX, 0U, 0U, 0U, 0U,
                                               0U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_fdcan_capture_set_status(&split_capture, 0U,
                                               (canview_stm_fdcan_bus_state_t)-1, 0U, 0U, 0U, 0U,
                                               0U) == CANVIEW_INVALID_ARGUMENT);
}

typedef struct
{
    canview_stm_fdcan_capture_t *capture;
    bool reject_bus_one;
    bool reenter;
    uint32_t calls;
} filter_fixture_t;

static bool observer_filter(const canview_stm_fdcan_record_t *record, void *context)
{
    filter_fixture_t *const fixture = (filter_fixture_t *)context;
    ++fixture->calls;
    if (fixture->reenter)
    {
        canview_wire_can_batch_t nested = {0};
        CHECK(canview_stm_fdcan_capture_build_batch(fixture->capture, &nested) ==
              CANVIEW_RESOURCE_BUSY);
    }
    return !(fixture->reject_bus_one && record->bus_id == 1U);
}

static void filter_tests(void)
{
    const canview_stm_fdcan_profile_t profiles[3] = {
        valid_profile(500000U), valid_profile(500000U), valid_profile(125000U)};
    filter_fixture_t fixture = {0};
    canview_stm_fdcan_capture_t capture = {0};
    fixture.capture = &capture;
    fixture.reject_bus_one = true;
    init_capture(&capture, profiles, observer_filter, &fixture);
    for (size_t index = 0U; index < 3U; ++index)
    {
        canview_stm_fdcan_rx_frame_t input = frame((uint32_t)(100U + index * 10U),
                                                   (uint32_t)(0x300U + index), 0U, 0U);
        CHECK(canview_stm_fdcan_capture_ingest(&capture, index, &input) == CANVIEW_OK);
    }
    canview_wire_can_batch_t batch = {0};
    CHECK(canview_stm_fdcan_capture_build_batch(&capture, &batch) == CANVIEW_OK);
    CHECK(batch.count == 2U && fixture.calls == 3U);
    canview_stm_fdcan_channel_stats_t stats = {0};
    CHECK(canview_stm_fdcan_capture_get_stats(&capture, 1U, &stats) == CANVIEW_OK);
    CHECK(stats.filtered_frames == 1U && stats.queued_frames == 0U);

    canview_stm_fdcan_capture_t reentry_capture = {0};
    filter_fixture_t reentry_fixture = {0};
    reentry_fixture.capture = &reentry_capture;
    reentry_fixture.reenter = true;
    init_capture(&reentry_capture, profiles, observer_filter, &reentry_fixture);
    canview_stm_fdcan_rx_frame_t input = frame(1U, 0x400U, 0U, 0U);
    CHECK(canview_stm_fdcan_capture_ingest(&reentry_capture, 0U, &input) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_build_batch(&reentry_capture, &batch) ==
          CANVIEW_RESOURCE_BUSY);
    CHECK(batch.count == 0U);
    CHECK(canview_stm_fdcan_capture_get_stats(&reentry_capture, 0U, &stats) == CANVIEW_OK);
    CHECK(stats.queued_frames == 1U);
    reentry_capture.filter = NULL;
    CHECK(canview_stm_fdcan_capture_build_batch(&reentry_capture, &batch) == CANVIEW_OK);
    CHECK(batch.count == 1U);
}

int main(void)
{
    profile_tests();
    board_profile_tests();
    decoder_tests();
    init_contract_tests();
    stream_tests();
    validation_tests();
    ring_drop_tests();
    timestamp_tests();
    timestamp_transaction_tests();
    inventory_tests();
    drop_accounting_tests();
    filter_tests();
    (void)puts("PASS: STM32 three-channel FDCAN capture C99 tests");
    return EXIT_SUCCESS;
}
