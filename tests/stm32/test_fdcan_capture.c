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
    CHECK(canview_stm_fdcan_profile_validate(NULL) == CANVIEW_INVALID_ARGUMENT);
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
    invalid.bitrate_known = true;
    CHECK(canview_stm_fdcan_profile_validate(&invalid) == CANVIEW_INVALID_ARGUMENT);
    invalid = disabled;
    invalid.transceiver_known = true;
    CHECK(canview_stm_fdcan_profile_validate(&invalid) == CANVIEW_INVALID_ARGUMENT);
    invalid = disabled;
    invalid.transceiver = CANVIEW_STM_FDCAN_TRANSCEIVER_TCAN1046;
    CHECK(canview_stm_fdcan_profile_validate(&invalid) == CANVIEW_INVALID_ARGUMENT);
    invalid = disabled;
    invalid.nominal_bitrate = 500000U;
    CHECK(canview_stm_fdcan_profile_validate(&invalid) == CANVIEW_INVALID_ARGUMENT);
    invalid = disabled;
    invalid.data_bitrate = 1U;
    CHECK(canview_stm_fdcan_profile_validate(&invalid) == CANVIEW_INVALID_ARGUMENT);
    invalid = disabled;
    invalid.nominal_timing.prescaler = 1U;
    CHECK(canview_stm_fdcan_profile_validate(&invalid) == CANVIEW_INVALID_ARGUMENT);
    invalid = disabled;
    invalid.nominal_timing.time_segment1 = 1U;
    CHECK(canview_stm_fdcan_profile_validate(&invalid) == CANVIEW_INVALID_ARGUMENT);
    invalid = disabled;
    invalid.nominal_timing.time_segment2 = 1U;
    CHECK(canview_stm_fdcan_profile_validate(&invalid) == CANVIEW_INVALID_ARGUMENT);
    invalid = disabled;
    invalid.nominal_timing.sync_jump_width = 1U;
    CHECK(canview_stm_fdcan_profile_validate(&invalid) == CANVIEW_INVALID_ARGUMENT);
    invalid = disabled;
    invalid.data_timing.prescaler = 1U;
    CHECK(canview_stm_fdcan_profile_validate(&invalid) == CANVIEW_INVALID_ARGUMENT);
    invalid = disabled;
    invalid.data_timing.time_segment1 = 1U;
    CHECK(canview_stm_fdcan_profile_validate(&invalid) == CANVIEW_INVALID_ARGUMENT);
    invalid = disabled;
    invalid.data_timing.time_segment2 = 1U;
    CHECK(canview_stm_fdcan_profile_validate(&invalid) == CANVIEW_INVALID_ARGUMENT);
    invalid = disabled;
    invalid.data_timing.sync_jump_width = 1U;
    CHECK(canview_stm_fdcan_profile_validate(&invalid) == CANVIEW_INVALID_ARGUMENT);
    invalid = disabled;
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
    invalid = valid_profile(250000U);
    invalid.nominal_timing.prescaler = 0U;
    CHECK(canview_stm_fdcan_profile_validate(&invalid) == CANVIEW_INVALID_ARGUMENT);
    invalid = valid_profile(250000U);
    invalid.nominal_timing.prescaler = 513U;
    CHECK(canview_stm_fdcan_profile_validate(&invalid) == CANVIEW_INVALID_ARGUMENT);
    invalid = valid_profile(250000U);
    invalid.nominal_timing.time_segment1 = 0U;
    CHECK(canview_stm_fdcan_profile_validate(&invalid) == CANVIEW_INVALID_ARGUMENT);
    invalid = valid_profile(250000U);
    invalid.nominal_timing.time_segment1 = 257U;
    CHECK(canview_stm_fdcan_profile_validate(&invalid) == CANVIEW_INVALID_ARGUMENT);
    invalid = valid_profile(250000U);
    invalid.nominal_timing.time_segment2 = 0U;
    CHECK(canview_stm_fdcan_profile_validate(&invalid) == CANVIEW_INVALID_ARGUMENT);
    invalid = valid_profile(250000U);
    invalid.nominal_timing.time_segment2 = 129U;
    CHECK(canview_stm_fdcan_profile_validate(&invalid) == CANVIEW_INVALID_ARGUMENT);
    invalid = valid_profile(250000U);
    invalid.nominal_timing.sync_jump_width = 0U;
    CHECK(canview_stm_fdcan_profile_validate(&invalid) == CANVIEW_INVALID_ARGUMENT);
    invalid = valid_profile(250000U);
    invalid.nominal_timing.sync_jump_width = 129U;
    CHECK(canview_stm_fdcan_profile_validate(&invalid) == CANVIEW_INVALID_ARGUMENT);
    invalid = valid_profile(250000U);
    invalid.nominal_timing.sync_jump_width = 3U;
    CHECK(canview_stm_fdcan_profile_validate(&invalid) == CANVIEW_INVALID_ARGUMENT);
    invalid = valid_profile(250000U);
    invalid.nominal_timing.prescaler = 3U;
    CHECK(canview_stm_fdcan_profile_validate(&invalid) == CANVIEW_INVALID_ARGUMENT);
    invalid = valid_profile(250000U);
    invalid.nominal_timing.prescaler = 10U;
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
    CHECK(canview_stm_fdcan_channel_profile_validate(0U, NULL) == CANVIEW_INVALID_ARGUMENT);
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
    words[1] = (UINT32_C(9) << 16U) | UINT32_C(0x00100000);
    CHECK(canview_stm_fdcan_decode_element(words, 101U, &decoded) == CANVIEW_OK);
    CHECK((decoded.flags & CANVIEW_STM_FDCAN_FRAME_BRS) != 0U && decoded.dlc == 9U);
    words[1] = UINT32_C(9) << 16U;
    CHECK(canview_stm_fdcan_decode_element(words, 102U, &decoded) == CANVIEW_OK);
    CHECK(decoded.flags == 0U && decoded.dlc == 9U && decoded.data[0] == 0U);
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
    CHECK(canview_stm_fdcan_capture_build_batch(NULL, &batch) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_fdcan_capture_build_batch(&capture, NULL) == CANVIEW_INVALID_ARGUMENT);
    canview_stm_fdcan_capture_t uninitialized_batch = {0};
    CHECK(canview_stm_fdcan_capture_build_batch(&uninitialized_batch, &batch) ==
          CANVIEW_INVALID_ARGUMENT);
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

    canview_stm_fdcan_capture_t tie_capture = {0};
    init_capture(&tie_capture, profiles, NULL, NULL);
    for (size_t index = 0U; index < CANVIEW_STM_FDCAN_CHANNEL_COUNT; ++index)
    {
        canview_stm_fdcan_rx_frame_t tie_frame = frame(7U, (uint32_t)(0x180U + index), 0U, 0U);
        CHECK(canview_stm_fdcan_capture_ingest(&tie_capture, index, &tie_frame) == CANVIEW_OK);
    }
    CHECK(canview_stm_fdcan_capture_build_batch(&tie_capture, &batch) == CANVIEW_OK &&
          batch.count == CANVIEW_STM_FDCAN_CHANNEL_COUNT && batch.records[0].bus_id == 0U &&
          batch.records[1].bus_id == 1U && batch.records[2].bus_id == 2U);

    canview_stm_fdcan_channel_stats_t stats = {0};
    CHECK(canview_stm_fdcan_capture_get_stats(&capture, 0U, &stats) == CANVIEW_OK);
    CHECK(stats.accepted_frames == 1U && stats.queued_frames == 0U &&
          stats.state == CANVIEW_STM_FDCAN_BUS_ERROR_ACTIVE);
    CHECK(canview_stm_fdcan_capture_get_stats(NULL, 0U, &stats) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_fdcan_capture_get_stats(&capture, 0U, NULL) == CANVIEW_INVALID_ARGUMENT);
    canview_stm_fdcan_capture_t uninitialized = {0};
    CHECK(canview_stm_fdcan_capture_get_stats(&uninitialized, 0U, &stats) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_fdcan_capture_get_stats(&capture, 3U, &stats) == CANVIEW_INVALID_ARGUMENT);
}

static void init_contract_tests(void)
{
    const canview_stm_fdcan_profile_t valid_profiles[3] = {
        valid_profile(500000U), valid_profile(500000U), valid_profile(125000U)};
    const canview_stm_critical_t critical = test_critical();
    canview_stm_fdcan_capture_t capture = {0};
    CHECK(canview_stm_fdcan_capture_init(&capture, NULL, &critical, NULL, NULL) ==
          CANVIEW_INVALID_ARGUMENT);
    canview_stm_critical_t invalid_critical = critical;
    invalid_critical.enter = NULL;
    CHECK(canview_stm_fdcan_capture_init(&capture, valid_profiles, &invalid_critical, NULL, NULL) ==
          CANVIEW_INVALID_ARGUMENT);
    invalid_critical = critical;
    invalid_critical.leave = NULL;
    CHECK(canview_stm_fdcan_capture_init(&capture, valid_profiles, &invalid_critical, NULL, NULL) ==
          CANVIEW_INVALID_ARGUMENT);
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
    canview_stm_fdcan_rx_frame_t uninitialized_frame = frame(0U, 0U, 0U, 0U);
    CHECK(canview_stm_fdcan_capture_ingest(&capture, 0U, &uninitialized_frame) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_fdcan_capture_ingest(NULL, 0U, &uninitialized_frame) ==
          CANVIEW_INVALID_ARGUMENT);
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

    canview_stm_fdcan_capture_t remote_capture = {0};
    init_capture(&remote_capture, profiles, NULL, NULL);
    canview_stm_fdcan_rx_frame_t remote =
        frame(20U, 0x321U, 0U, CANVIEW_STM_FDCAN_FRAME_RTR | CANVIEW_STM_FDCAN_FRAME_ERROR);
    CHECK(canview_stm_fdcan_capture_ingest(&remote_capture, 0U, &remote) == CANVIEW_OK);
    canview_wire_can_batch_t remote_batch = {0};
    CHECK(canview_stm_fdcan_capture_build_batch(&remote_capture, &remote_batch) == CANVIEW_OK);
    CHECK(remote_batch.count == 1U && remote_batch.records[0].flags == 0x06U);

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

    canview_stm_fdcan_capture_t drop_snapshot = {0};
    init_capture(&drop_snapshot, profiles, NULL, NULL);
    drop_snapshot.reported_dropped[0] = 5U;
    drop_snapshot.channels[0].dropped = 2U;
    CHECK(canview_stm_fdcan_capture_build_batch(&drop_snapshot, &batch) == CANVIEW_OK &&
          batch.count == 0U && batch.dropped_since_last == 2U);
    drop_snapshot.reported_dropped[0] = 0U;
    drop_snapshot.channels[0].dropped = UINT32_MAX;
    CHECK(canview_stm_fdcan_capture_build_batch(&drop_snapshot, &batch) == CANVIEW_OK &&
          batch.count == 0U && batch.dropped_since_last == UINT8_MAX);
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

    canview_stm_fdcan_capture_t half_range_forward = {0};
    init_capture(&half_range_forward, profiles, NULL, NULL);
    const canview_stm_fdcan_rx_frame_t half_forward_first = frame(100U, 0x514U, 0U, 0U);
    const canview_stm_fdcan_rx_frame_t half_forward_second =
        frame(100U + CANVIEW_STM_FDCAN_TIMESTAMP_HALF_RANGE, 0x515U, 0U, 0U);
    CHECK(canview_stm_fdcan_capture_ingest(&half_range_forward, 0U, &half_forward_first) ==
          CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_ingest(&half_range_forward, 0U, &half_forward_second) ==
          CANVIEW_MALFORMED);

    canview_stm_fdcan_capture_t half_range_backward = {0};
    init_capture(&half_range_backward, profiles, NULL, NULL);
    const canview_stm_fdcan_rx_frame_t half_backward_first =
        frame(CANVIEW_STM_FDCAN_TIMESTAMP_HALF_RANGE, 0x516U, 0U, 0U);
    const canview_stm_fdcan_rx_frame_t half_backward_second = frame(0U, 0x517U, 0U, 0U);
    CHECK(canview_stm_fdcan_capture_ingest(&half_range_backward, 0U, &half_backward_first) ==
          CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_ingest(&half_range_backward, 0U, &half_backward_second) ==
          CANVIEW_MALFORMED);

    /* 채널 0이 wrap한 직후 채널 1의 이전 epoch frame도 정상적으로 보존한다. */
    canview_stm_fdcan_capture_t cross_wrap = {0};
    init_capture(&cross_wrap, profiles, NULL, NULL);
    canview_stm_fdcan_rx_frame_t cross_before = frame(UINT32_MAX - 15U, 0x520U, 0U, 0U);
    canview_stm_fdcan_rx_frame_t cross_after = frame(16U, 0x521U, 0U, 0U);
    canview_stm_fdcan_rx_frame_t cross_previous = frame(UINT32_MAX - 5U, 0x522U, 0U, 0U);
    canview_stm_fdcan_rx_frame_t cross_next = frame(5U, 0x523U, 0U, 0U);
    CHECK(canview_stm_fdcan_capture_ingest(&cross_wrap, 0U, &cross_before) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_ingest(&cross_wrap, 0U, &cross_after) == CANVIEW_OK);
    CHECK(cross_wrap.extended_timestamp_us == UINT64_C(0x100000010));
    CHECK(canview_stm_fdcan_capture_ingest(&cross_wrap, 1U, &cross_previous) == CANVIEW_OK);
    CHECK(cross_wrap.channels[1].last_timestamp_us == UINT64_C(0xfffffffa));
    CHECK(canview_stm_fdcan_capture_ingest(&cross_wrap, 1U, &cross_next) == CANVIEW_OK);
    CHECK(cross_wrap.channels[1].last_timestamp_us == UINT64_C(0x100000005));

    canview_stm_fdcan_capture_t anchor_capture = {0};
    init_capture(&anchor_capture, profiles, NULL, NULL);
    CHECK(canview_stm_fdcan_capture_set_status(&anchor_capture, 0U,
                                               CANVIEW_STM_FDCAN_BUS_ERROR_ACTIVE, 0U, 0U, 0U,
                                               0U, 0U) == CANVIEW_OK);
    canview_stm_fdcan_rx_frame_t impossible_previous = frame(UINT32_MAX, 0x524U, 0U, 0U);
    CHECK(canview_stm_fdcan_capture_ingest(&anchor_capture, 1U, &impossible_previous) ==
          CANVIEW_MALFORMED);

    canview_stm_fdcan_capture_t previous_epoch_anchor = {0};
    init_capture(&previous_epoch_anchor, profiles, NULL, NULL);
    CHECK(canview_stm_fdcan_capture_set_status(&previous_epoch_anchor, 0U,
                                               CANVIEW_STM_FDCAN_BUS_ERROR_ACTIVE, 0U, 0U, 0U,
                                               0U, UINT32_MAX) == CANVIEW_OK);
    canview_stm_fdcan_rx_frame_t previous_epoch_sample = frame(0U, 0x526U, 0U, 0U);
    CHECK(canview_stm_fdcan_capture_ingest(&previous_epoch_anchor, 1U,
                                           &previous_epoch_sample) == CANVIEW_OK);
    CHECK(previous_epoch_anchor.channels[1].last_timestamp_us == UINT64_C(0x100000000));

    canview_stm_fdcan_capture_t oversize_anchor = {0};
    init_capture(&oversize_anchor, profiles, NULL, NULL);
    CHECK(canview_stm_fdcan_capture_set_status(&oversize_anchor, 0U,
                                               CANVIEW_STM_FDCAN_BUS_ERROR_ACTIVE, 0U, 0U, 0U,
                                               0U, UINT64_MAX) == CANVIEW_OK);
    canview_stm_fdcan_rx_frame_t overflow_sample = frame(0U, 0x525U, 0U, 0U);
    CHECK(canview_stm_fdcan_capture_ingest(&oversize_anchor, 1U, &overflow_sample) ==
          CANVIEW_OVERSIZE);

    canview_stm_fdcan_capture_t oversize_wrap = {0};
    init_capture(&oversize_wrap, profiles, NULL, NULL);
    oversize_wrap.channels[0].timestamp_initialized = true;
    oversize_wrap.channels[0].last_source_timestamp_us = UINT32_MAX;
    oversize_wrap.channels[0].timestamp_epoch_us =
        UINT64_MAX - UINT64_C(0x100000000) + UINT64_C(1);
    oversize_wrap.channels[0].last_timestamp_us = UINT64_MAX;
    canview_stm_fdcan_rx_frame_t wrap_overflow_sample = frame(0U, 0x527U, 0U, 0U);
    CHECK(canview_stm_fdcan_capture_ingest(&oversize_wrap, 0U, &wrap_overflow_sample) ==
          CANVIEW_OVERSIZE);

    canview_stm_fdcan_capture_t oversize_timestamp = {0};
    init_capture(&oversize_timestamp, profiles, NULL, NULL);
    oversize_timestamp.channels[0].timestamp_initialized = true;
    oversize_timestamp.channels[0].last_source_timestamp_us = 100U;
    oversize_timestamp.channels[0].timestamp_epoch_us = UINT64_MAX;
    oversize_timestamp.channels[0].last_timestamp_us = UINT64_MAX;
    canview_stm_fdcan_rx_frame_t timestamp_overflow_sample = frame(101U, 0x528U, 0U, 0U);
    CHECK(canview_stm_fdcan_capture_ingest(&oversize_timestamp, 0U,
                                           &timestamp_overflow_sample) == CANVIEW_OVERSIZE);
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
    CHECK(canview_stm_fdcan_capture_get_inventory(NULL, 0U, &entry) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_fdcan_capture_get_inventory(&capture, 1U, &entry) == CANVIEW_INCOMPLETE);
    CHECK(canview_stm_fdcan_capture_get_inventory(&capture, 0U, NULL) == CANVIEW_INVALID_ARGUMENT);
    canview_stm_fdcan_capture_t uninitialized = {0};
    CHECK(canview_stm_fdcan_capture_get_inventory(&uninitialized, 0U, &entry) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_fdcan_capture_get_inventory_state(NULL, &inventory_count,
                                                        &inventory_dropped) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_fdcan_capture_get_inventory_state(&capture, NULL, &inventory_dropped) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_fdcan_capture_get_inventory_state(&capture, &inventory_count, NULL) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_fdcan_capture_get_inventory_state(&uninitialized, &inventory_count,
                                                        &inventory_dropped) ==
          CANVIEW_INVALID_ARGUMENT);

    canview_stm_fdcan_capture_t period_capture = {0};
    init_capture(&period_capture, profiles, NULL, NULL);
    const uint32_t period_timestamps[] = {100U, 150U, 170U, 250U, 260U};
    for (size_t index = 0U; index < sizeof(period_timestamps) / sizeof(period_timestamps[0]); ++index)
    {
        canview_stm_fdcan_rx_frame_t sample = frame(period_timestamps[index], 0x556U, 0U, 0U);
        CHECK(canview_stm_fdcan_capture_ingest(&period_capture, 0U, &sample) == CANVIEW_OK);
        CHECK(canview_stm_fdcan_capture_build_batch(&period_capture, &batch) == CANVIEW_OK);
    }
    CHECK(canview_stm_fdcan_capture_get_inventory(&period_capture, 0U, &entry) == CANVIEW_OK);
    CHECK(entry.period_sample_count == 4U && entry.period_p50_us == 20U &&
          entry.period_p95_us == 80U);
    for (uint32_t index = 0U; index < 6U; ++index)
    {
        canview_stm_fdcan_rx_frame_t sample =
            frame(300U + index * 10U, 0x556U, 0U, 0U);
        CHECK(canview_stm_fdcan_capture_ingest(&period_capture, 0U, &sample) == CANVIEW_OK);
        CHECK(canview_stm_fdcan_capture_build_batch(&period_capture, &batch) == CANVIEW_OK);
    }
    CHECK(canview_stm_fdcan_capture_get_inventory(&period_capture, 0U, &entry) == CANVIEW_OK);
    CHECK(entry.period_sample_count == CANVIEW_STM_FDCAN_PERIOD_SAMPLE_CAPACITY &&
          entry.period_sample_index != 0U);

    canview_stm_fdcan_capture_t variant_capture = {0};
    init_capture(&variant_capture, profiles, NULL, NULL);
    const canview_stm_fdcan_rx_frame_t variants[] = {
        frame(10U, 0x557U, 0U, 0U),
        frame(20U, 0x557U, 0U, CANVIEW_STM_FDCAN_FRAME_IDE),
        frame(30U, 0x557U, 1U, 0U),
        frame(40U, 0x558U, 0U, 0U)};
    for (size_t index = 0U; index < sizeof(variants) / sizeof(variants[0]); ++index)
    {
        CHECK(canview_stm_fdcan_capture_ingest(&variant_capture, 0U, &variants[index]) ==
              CANVIEW_OK);
        CHECK(canview_stm_fdcan_capture_build_batch(&variant_capture, &batch) == CANVIEW_OK);
    }
    canview_stm_fdcan_rx_frame_t other_bus_variant = frame(50U, 0x557U, 0U, 0U);
    CHECK(canview_stm_fdcan_capture_ingest(&variant_capture, 1U, &other_bus_variant) ==
          CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_build_batch(&variant_capture, &batch) == CANVIEW_OK);

    canview_stm_fdcan_capture_t slow_capture = {0};
    init_capture(&slow_capture, profiles, NULL, NULL);
    canview_stm_fdcan_rx_frame_t slow_first = frame(100U, 0x559U, 0U, 0U);
    canview_stm_fdcan_rx_frame_t slow_second = frame(700100U, 0x559U, 0U, 0U);
    CHECK(canview_stm_fdcan_capture_ingest(&slow_capture, 0U, &slow_first) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_build_batch(&slow_capture, &batch) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_ingest(&slow_capture, 0U, &slow_second) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_build_batch(&slow_capture, &batch) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_get_inventory(&slow_capture, 0U, &entry) == CANVIEW_OK &&
          entry.rate_tenth_hz < UINT16_MAX);

    canview_stm_fdcan_capture_t equal_timestamp_capture = {0};
    init_capture(&equal_timestamp_capture, profiles, NULL, NULL);
    canview_stm_fdcan_rx_frame_t equal_first = frame(80U, 0x55AU, 0U, 0U);
    CHECK(canview_stm_fdcan_capture_ingest(&equal_timestamp_capture, 0U, &equal_first) ==
          CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_build_batch(&equal_timestamp_capture, &batch) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_ingest(&equal_timestamp_capture, 0U, &equal_first) ==
          CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_build_batch(&equal_timestamp_capture, &batch) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_get_inventory(&equal_timestamp_capture, 0U, &entry) ==
          CANVIEW_OK && entry.frame_count == 2U && entry.period_sample_count == 1U);

    canview_stm_fdcan_capture_t long_period_capture = {0};
    init_capture(&long_period_capture, profiles, NULL, NULL);
    canview_stm_fdcan_rx_frame_t long_first = frame(UINT32_MAX - 2U, 0x55BU, 0U, 0U);
    canview_stm_fdcan_rx_frame_t long_second = frame(1U, 0x55BU, 0U, 0U);
    CHECK(canview_stm_fdcan_capture_ingest(&long_period_capture, 0U, &long_first) ==
          CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_build_batch(&long_period_capture, &batch) == CANVIEW_OK);
    long_period_capture.inventory[0].last_timestamp_us = 0U;
    CHECK(canview_stm_fdcan_capture_ingest(&long_period_capture, 0U, &long_second) ==
          CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_build_batch(&long_period_capture, &batch) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_get_inventory(&long_period_capture, 0U, &entry) ==
          CANVIEW_OK && entry.period_samples[0] == UINT32_MAX);

    canview_stm_fdcan_capture_t backward_inventory_capture = {0};
    init_capture(&backward_inventory_capture, profiles, NULL, NULL);
    canview_stm_fdcan_rx_frame_t backward_first = frame(100U, 0x55CU, 0U, 0U);
    canview_stm_fdcan_rx_frame_t backward_second = frame(110U, 0x55CU, 0U, 0U);
    CHECK(canview_stm_fdcan_capture_ingest(&backward_inventory_capture, 0U, &backward_first) ==
          CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_build_batch(&backward_inventory_capture, &batch) ==
          CANVIEW_OK);
    backward_inventory_capture.inventory[0].last_timestamp_us = UINT64_MAX;
    CHECK(canview_stm_fdcan_capture_ingest(&backward_inventory_capture, 0U, &backward_second) ==
          CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_build_batch(&backward_inventory_capture, &batch) ==
          CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_get_inventory(&backward_inventory_capture, 0U, &entry) ==
          CANVIEW_OK && entry.period_sample_count == 0U);

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

    CHECK(canview_stm_fdcan_capture_observe(&split_capture, 65686U) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_get_stats(&split_capture, 0U, &stats) == CANVIEW_OK);
    CHECK(stats.state == CANVIEW_STM_FDCAN_BUS_ERROR_ACTIVE &&
          (stats.status_flags & CANVIEW_STM_FDCAN_STATUS_NO_DATA) == 0U);
    CHECK(canview_stm_fdcan_capture_observe(&split_capture, 65636U +
                                                        CANVIEW_STM_FDCAN_NO_DATA_TIMEOUT_US) ==
          CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_get_stats(&split_capture, 0U, &stats) == CANVIEW_OK);
    CHECK(stats.state == CANVIEW_STM_FDCAN_BUS_NO_DATA &&
          (stats.status_flags & CANVIEW_STM_FDCAN_STATUS_NO_DATA) != 0U);
    CHECK(canview_stm_fdcan_capture_observe(&split_capture, 65636U) == CANVIEW_MALFORMED);
    CHECK(canview_stm_fdcan_capture_observe(NULL, 0U) == CANVIEW_INVALID_ARGUMENT);
    canview_stm_fdcan_capture_t uninitialized_observe = {0};
    CHECK(canview_stm_fdcan_capture_observe(&uninitialized_observe, 0U) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_fdcan_capture_set_status(NULL, 0U, CANVIEW_STM_FDCAN_BUS_NO_DATA, 0U, 0U,
                                               0U, 0U, 0U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_fdcan_capture_set_status(&split_capture, 0U,
                                               CANVIEW_STM_FDCAN_BUS_ERROR_PASSIVE, 5U, 1U, 0U,
                                               7U, 200000U) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_get_stats(&split_capture, 0U, &stats) == CANVIEW_OK);
    CHECK(stats.state == CANVIEW_STM_FDCAN_BUS_ERROR_PASSIVE && stats.rx_error_count == 5U &&
          stats.tx_error_count == 1U && stats.last_error == 7U &&
          stats.last_timestamp_us == 65636U);
    CHECK(canview_stm_fdcan_capture_set_status(&split_capture, 0U,
                                               CANVIEW_STM_FDCAN_BUS_OFF, 9U, 2U, 1U, 8U,
                                               200001U) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_set_status(&split_capture, 0U,
                                               CANVIEW_STM_FDCAN_BUS_STATE_MAX, 0U, 0U, 0U, 0U,
                                               0U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_fdcan_capture_set_status(&split_capture, 0U,
                                               (canview_stm_fdcan_bus_state_t)-1, 0U, 0U, 0U, 0U,
                                               0U) == CANVIEW_INVALID_ARGUMENT);

    const canview_stm_fdcan_profile_t disabled_profiles[3] = {{0}, {0}, {0}};
    canview_stm_fdcan_capture_t status_disabled = {0};
    init_capture(&status_disabled, disabled_profiles, NULL, NULL);
    CHECK(canview_stm_fdcan_capture_set_status(&status_disabled, 0U,
                                               CANVIEW_STM_FDCAN_BUS_ERROR_ACTIVE, 0U, 0U, 0U,
                                               0U, 1U) == CANVIEW_RESOURCE_BUSY);

    canview_stm_fdcan_capture_t state_skip_capture = {0};
    init_capture(&state_skip_capture, profiles, NULL, NULL);
    state_skip_capture.channels[0].state = CANVIEW_STM_FDCAN_BUS_UNKNOWN_BITRATE;
    state_skip_capture.channels[1].state = CANVIEW_STM_FDCAN_BUS_ERROR_PASSIVE;
    state_skip_capture.channels[2].state = CANVIEW_STM_FDCAN_BUS_OFF;
    CHECK(canview_stm_fdcan_capture_observe(&state_skip_capture, 0U) == CANVIEW_OK);
    state_skip_capture.channels[0].state = CANVIEW_STM_FDCAN_BUS_FAULT;
    CHECK(canview_stm_fdcan_capture_observe(&state_skip_capture, 0U) == CANVIEW_OK);

    canview_stm_fdcan_capture_t channel_time_error = {0};
    init_capture(&channel_time_error, profiles, NULL, NULL);
    channel_time_error.channels[0].data_seen = true;
    channel_time_error.channels[0].last_timestamp_us = 100U;
    channel_time_error.channels[0].state = CANVIEW_STM_FDCAN_BUS_ERROR_ACTIVE;
    CHECK(canview_stm_fdcan_capture_observe(&channel_time_error, 50U) == CANVIEW_MALFORMED);
    CHECK(channel_time_error.channels[0].state == CANVIEW_STM_FDCAN_BUS_FAULT);

    canview_stm_fdcan_capture_t no_data_capture = {0};
    init_capture(&no_data_capture, profiles, NULL, NULL);
    CHECK(canview_stm_fdcan_capture_set_status(&no_data_capture, 0U,
                                               CANVIEW_STM_FDCAN_BUS_NO_DATA, 0U, 0U, 0U, 0U,
                                               10U) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_set_status(&no_data_capture, 0U,
                                               CANVIEW_STM_FDCAN_BUS_ERROR_ACTIVE, 0U, 0U, 0U,
                                               0U, 10U) == CANVIEW_OK);

    canview_stm_fdcan_capture_t stale_status_capture = {0};
    init_capture(&stale_status_capture, profiles, NULL, NULL);
    CHECK(canview_stm_fdcan_capture_set_status(
              &stale_status_capture, 0U, CANVIEW_STM_FDCAN_BUS_ERROR_ACTIVE, 0U, 0U, 0U, 0U,
              1000U) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_set_status(
              &stale_status_capture, 0U, CANVIEW_STM_FDCAN_BUS_OFF, 0U, 0U, 1U, 0U,
              2000U) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_set_status(
              &stale_status_capture, 0U, CANVIEW_STM_FDCAN_BUS_ERROR_ACTIVE, 0U, 0U, 0U, 0U,
              1500U) == CANVIEW_STALE);
    CHECK(canview_stm_fdcan_capture_get_stats(&stale_status_capture, 0U, &stats) == CANVIEW_OK);
    CHECK(stats.state == CANVIEW_STM_FDCAN_BUS_OFF && stats.bus_off_count == 1U);

    canview_stm_fdcan_capture_t frame_before_status = {0};
    init_capture(&frame_before_status, profiles, NULL, NULL);
    const canview_stm_fdcan_rx_frame_t newest_frame = frame(3000U, 0x518U, 0U, 0U);
    CHECK(canview_stm_fdcan_capture_ingest(&frame_before_status, 0U, &newest_frame) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_set_status(
              &frame_before_status, 0U, CANVIEW_STM_FDCAN_BUS_ERROR_ACTIVE, 0U, 0U, 0U, 0U,
              2000U) == CANVIEW_STALE);

    canview_stm_fdcan_capture_t sticky_fault_capture = {0};
    init_capture(&sticky_fault_capture, profiles, NULL, NULL);
    CHECK(canview_stm_fdcan_capture_set_status(
              &sticky_fault_capture, 0U, CANVIEW_STM_FDCAN_BUS_ERROR_ACTIVE, 0U, 0U, 0U,
              CANVIEW_STM_FDCAN_ERROR_FIFO_LOSS, 20U) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_get_stats(&sticky_fault_capture, 0U, &stats) == CANVIEW_OK);
    CHECK(stats.state == CANVIEW_STM_FDCAN_BUS_FAULT &&
          stats.hardware_fault_count == 1U &&
          (stats.last_error & CANVIEW_STM_FDCAN_ERROR_FIFO_LOSS) != 0U);
    CHECK(canview_stm_fdcan_capture_set_status(
              &sticky_fault_capture, 0U, CANVIEW_STM_FDCAN_BUS_ERROR_ACTIVE, 0U, 0U, 0U,
              CANVIEW_STM_FDCAN_ERROR_FIFO_LOSS, 21U) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_get_stats(&sticky_fault_capture, 0U, &stats) == CANVIEW_OK);
    CHECK(stats.hardware_fault_count == 1U &&
          (stats.last_error & CANVIEW_STM_FDCAN_ERROR_FIFO_LOSS) != 0U);
    CHECK(canview_stm_fdcan_capture_set_status(
              &sticky_fault_capture, 0U, CANVIEW_STM_FDCAN_BUS_ERROR_ACTIVE, 0U, 0U, 0U,
              0U, 22U) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_get_stats(&sticky_fault_capture, 0U, &stats) == CANVIEW_OK);
    CHECK(stats.state == CANVIEW_STM_FDCAN_BUS_FAULT &&
          stats.hardware_fault_count == 1U &&
          (stats.last_error & CANVIEW_STM_FDCAN_ERROR_FIFO_LOSS) != 0U);
    CHECK(canview_stm_fdcan_capture_set_status(
              &sticky_fault_capture, 0U, CANVIEW_STM_FDCAN_BUS_ERROR_ACTIVE, 0U, 0U, 0U,
              CANVIEW_STM_FDCAN_ERROR_MESSAGE_RAM, 23U) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_get_stats(&sticky_fault_capture, 0U, &stats) == CANVIEW_OK);
    CHECK(stats.hardware_fault_count == 2U &&
          (stats.last_error & (CANVIEW_STM_FDCAN_ERROR_FIFO_LOSS |
                               CANVIEW_STM_FDCAN_ERROR_MESSAGE_RAM)) ==
              (CANVIEW_STM_FDCAN_ERROR_FIFO_LOSS | CANVIEW_STM_FDCAN_ERROR_MESSAGE_RAM));
}

typedef struct
{
    canview_stm_fdcan_capture_t *capture;
    bool reject_bus_one;
    bool reenter;
    bool mutate_header;
    bool mutate_data;
    bool mutated;
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
    if (!fixture->mutated && (fixture->mutate_header || fixture->mutate_data))
    {
        canview_stm_fdcan_record_t *const queued =
            &fixture->capture->channels[record->bus_id].records[
                fixture->capture->channels[record->bus_id].read_index];
        if (fixture->mutate_header)
        {
            queued->can_id ^= 1U;
        }
        else
        {
            queued->data[0] ^= 1U;
        }
        fixture->mutated = true;
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
    canview_stm_fdcan_rx_frame_t input_second = frame(2U, 0x401U, 0U, 0U);
    CHECK(canview_stm_fdcan_capture_ingest(&reentry_capture, 0U, &input) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_ingest(&reentry_capture, 0U, &input_second) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_build_batch(&reentry_capture, &batch) ==
          CANVIEW_RESOURCE_BUSY);
    CHECK(batch.count == 0U);
    CHECK(canview_stm_fdcan_capture_get_stats(&reentry_capture, 0U, &stats) == CANVIEW_OK);
    CHECK(stats.queued_frames == 2U);
    reentry_capture.filter = NULL;
    CHECK(canview_stm_fdcan_capture_build_batch(&reentry_capture, &batch) == CANVIEW_OK);
    CHECK(batch.count == 2U);

    /* Filter reject도 한 호출에서 무한히 ring을 소비하지 않도록 bounded하다. */
    canview_stm_fdcan_capture_t filtered_capture = {0};
    filter_fixture_t filtered_fixture = {0};
    filtered_fixture.capture = &filtered_capture;
    filtered_fixture.reject_bus_one = true;
    init_capture(&filtered_capture, profiles, observer_filter, &filtered_fixture);
    for (uint32_t index = 0U; index < CANVIEW_WIRE_CAN_MAX_RECORDS + 4U; ++index)
    {
        canview_stm_fdcan_rx_frame_t rejected = frame(index, 0x450U + index, 0U, 0U);
        CHECK(canview_stm_fdcan_capture_ingest(&filtered_capture, 1U, &rejected) == CANVIEW_OK);
    }
    CHECK(canview_stm_fdcan_capture_build_batch(&filtered_capture, &batch) == CANVIEW_INCOMPLETE);
    CHECK(canview_stm_fdcan_capture_get_stats(&filtered_capture, 1U, &stats) == CANVIEW_OK);
    CHECK(stats.filtered_frames == CANVIEW_WIRE_CAN_MAX_RECORDS &&
          stats.queued_frames == 4U);
    CHECK(canview_stm_fdcan_capture_build_batch(&filtered_capture, &batch) == CANVIEW_INCOMPLETE);
    CHECK(canview_stm_fdcan_capture_get_stats(&filtered_capture, 1U, &stats) == CANVIEW_OK);
    CHECK(stats.filtered_frames == CANVIEW_WIRE_CAN_MAX_RECORDS + 4U &&
          stats.queued_frames == 0U);

    canview_stm_fdcan_capture_t header_mutation_capture = {0};
    filter_fixture_t header_mutation_fixture = {0};
    header_mutation_fixture.capture = &header_mutation_capture;
    header_mutation_fixture.mutate_header = true;
    init_capture(&header_mutation_capture, profiles, observer_filter, &header_mutation_fixture);
    CHECK(canview_stm_fdcan_capture_ingest(&header_mutation_capture, 0U,
                                           &input) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_build_batch(&header_mutation_capture, &batch) ==
          CANVIEW_MALFORMED);
    CHECK(canview_stm_fdcan_capture_get_stats(&header_mutation_capture, 0U, &stats) ==
          CANVIEW_OK && stats.queued_frames == 1U);
    header_mutation_capture.filter = NULL;
    CHECK(canview_stm_fdcan_capture_build_batch(&header_mutation_capture, &batch) == CANVIEW_OK);

    canview_stm_fdcan_capture_t data_mutation_capture = {0};
    filter_fixture_t data_mutation_fixture = {0};
    data_mutation_fixture.capture = &data_mutation_capture;
    data_mutation_fixture.mutate_data = true;
    init_capture(&data_mutation_capture, profiles, observer_filter, &data_mutation_fixture);
    CHECK(canview_stm_fdcan_capture_ingest(&data_mutation_capture, 0U,
                                           &input) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_build_batch(&data_mutation_capture, &batch) ==
          CANVIEW_MALFORMED);
    CHECK(canview_stm_fdcan_capture_get_stats(&data_mutation_capture, 0U, &stats) ==
          CANVIEW_OK && stats.queued_frames == 1U);
}

static void session_reset_tests(void)
{
    const canview_stm_fdcan_profile_t profiles[3] = {
        valid_profile(500000U), valid_profile(500000U), valid_profile(125000U)};
    canview_stm_fdcan_capture_t capture = {0};
    CHECK(canview_stm_fdcan_capture_reset(NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_fdcan_capture_reset(&capture) == CANVIEW_INVALID_ARGUMENT);
    init_capture(&capture, profiles, NULL, NULL);

    canview_stm_fdcan_rx_frame_t first = frame(100U, 0x601U, 1U, 0U);
    first.data[0] = 0x5AU;
    CHECK(canview_stm_fdcan_capture_ingest(&capture, 0U, &first) == CANVIEW_OK);
    canview_wire_can_batch_t batch = {0};
    CHECK(canview_stm_fdcan_capture_build_batch(&capture, &batch) == CANVIEW_OK);
    CHECK(batch.count == 1U && batch.base_time_us == 100U);

    for (uint32_t index = 0U; index < CANVIEW_STM_FDCAN_RING_CAPACITY; ++index)
    {
        const canview_stm_fdcan_rx_frame_t queued = frame(200U + index, 0x610U, 0U, 0U);
        CHECK(canview_stm_fdcan_capture_ingest(&capture, 1U, &queued) == CANVIEW_OK);
    }
    const canview_stm_fdcan_rx_frame_t dropped = frame(300U, 0x610U, 0U, 0U);
    CHECK(canview_stm_fdcan_capture_ingest(&capture, 1U, &dropped) == CANVIEW_RESOURCE_BUSY);
    CHECK(capture.timestamp_initialized && capture.inventory_count == 1U);
    CHECK(canview_stm_fdcan_capture_set_status(
              &capture, 0U, CANVIEW_STM_FDCAN_BUS_ERROR_ACTIVE, 0U, 0U, 0U,
              CANVIEW_STM_FDCAN_ERROR_FIFO_LOSS, 500U) == CANVIEW_OK);

    capture.building = true;
    CHECK(canview_stm_fdcan_capture_reset(&capture) == CANVIEW_RESOURCE_BUSY);
    CHECK(capture.reentry_requested);
    capture.building = false;
    CHECK(canview_stm_fdcan_capture_reset(&capture) == CANVIEW_OK);
    CHECK(capture.initialized && !capture.building && !capture.reentry_requested &&
          !capture.timestamp_initialized && capture.inventory_count == 0U &&
          capture.inventory_dropped == 0U && capture.last_source_timestamp_us == 0U &&
          capture.extended_timestamp_us == 0U &&
          !capture.channels[0].status_timestamp_initialized &&
          capture.channels[0].last_status_timestamp_us == 0U);

    canview_stm_fdcan_channel_stats_t stats = {0};
    CHECK(canview_stm_fdcan_capture_get_stats(&capture, 0U, &stats) == CANVIEW_OK);
    CHECK(stats.state == CANVIEW_STM_FDCAN_BUS_NO_DATA &&
          stats.status_flags == CANVIEW_STM_FDCAN_STATUS_CONFIGURED &&
          stats.bitrate == 500000U && stats.accepted_frames == 0U &&
          stats.dropped_frames == 0U && stats.hardware_fault_count == 0U &&
          stats.last_error == 0U && stats.queued_frames == 0U &&
          stats.high_water_frames == 0U && stats.last_timestamp_us == 0U);
    CHECK(canview_stm_fdcan_capture_get_stats(&capture, 1U, &stats) == CANVIEW_OK);
    CHECK(stats.state == CANVIEW_STM_FDCAN_BUS_NO_DATA &&
          stats.bitrate == 500000U && stats.accepted_frames == 0U &&
          stats.dropped_frames == 0U && stats.queued_frames == 0U);
    size_t inventory_count = 1U;
    uint32_t inventory_dropped = 1U;
    CHECK(canview_stm_fdcan_capture_get_inventory_state(&capture, &inventory_count,
                                                        &inventory_dropped) == CANVIEW_OK);
    CHECK(inventory_count == 0U && inventory_dropped == 0U);

    const canview_stm_fdcan_rx_frame_t after_reset = frame(10U, 0x602U, 0U, 0U);
    CHECK(canview_stm_fdcan_capture_ingest(&capture, 0U, &after_reset) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_capture_build_batch(&capture, &batch) == CANVIEW_OK &&
          batch.count == 1U && batch.base_time_us == 10U && batch.records[0].can_id == 0x602U);
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
    session_reset_tests();
    (void)puts("PASS: STM32 three-channel FDCAN capture C99 tests");
    return EXIT_SUCCESS;
}
