/* SPDX-License-Identifier: GPL-3.0-only */
#include "fdcan_capture.h"

#include "fake_hardware.h"

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

typedef struct
{
    uint32_t frames;
    uint32_t drops;
    uint32_t statuses;
    size_t last_channel;
    canview_stm_fdcan_rx_frame_t last_frame;
    canview_stm_fdcan_bus_state_t last_state;
    uint32_t last_error;
    bool fail_frame;
    bool fail_drop;
    canview_stm_fdcan_platform_t *platform;
    bool reenter_service;
    canview_status_t reentry_status;
} sink_fixture_t;

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

static canview_status_t frame_sink(void *context, size_t channel,
                                   const canview_stm_fdcan_rx_frame_t *frame)
{
    sink_fixture_t *const fixture = (sink_fixture_t *)context;
    if (fixture == NULL || frame == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    ++fixture->frames;
    fixture->last_channel = channel;
    fixture->last_frame = *frame;
    if (fixture->reenter_service && fixture->platform != NULL)
    {
        fixture->reenter_service = false;
        fixture->reentry_status =
            canview_stm_fdcan_platform_service(fixture->platform, frame->source_timestamp_us);
    }
    return fixture->fail_frame ? CANVIEW_TIMEOUT : CANVIEW_OK;
}

static canview_status_t drop_sink(void *context, size_t channel, uint32_t dropped)
{
    sink_fixture_t *const fixture = (sink_fixture_t *)context;
    if (fixture == NULL || dropped == 0U)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (fixture->fail_drop)
    {
        return CANVIEW_TIMEOUT;
    }
    fixture->last_channel = channel;
    fixture->drops += dropped;
    return fixture->fail_drop ? CANVIEW_TIMEOUT : CANVIEW_OK;
}

static void status_sink(void *context, size_t channel, canview_stm_fdcan_bus_state_t state,
                        uint16_t rx_error_count, uint16_t tx_error_count, uint32_t bus_off_count,
                        uint32_t last_error, uint32_t source_timestamp_us)
{
    sink_fixture_t *const fixture = (sink_fixture_t *)context;
    (void)rx_error_count;
    (void)tx_error_count;
    (void)bus_off_count;
    (void)source_timestamp_us;
    if (fixture == NULL)
    {
        return;
    }
    ++fixture->statuses;
    fixture->last_channel = channel;
    fixture->last_state = state;
    fixture->last_error = last_error;
}

static canview_stm_fdcan_platform_config_t config_for(sink_fixture_t *fixture)
{
    const canview_stm_fdcan_platform_config_t config = {
        .profiles = {valid_profile(500000U), valid_profile(500000U), valid_profile(125000U)},
        .frame_sink = frame_sink,
        .drop_sink = drop_sink,
        .status_sink = status_sink,
        .sink_context = fixture};
    return config;
}

static canview_stm_fdcan_platform_config_t config_for_profiles(
    sink_fixture_t *fixture, const canview_stm_fdcan_profile_t profiles[3])
{
    canview_stm_fdcan_platform_config_t config = config_for(fixture);
    for (size_t index = 0U; index < 3U; ++index)
    {
        config.profiles[index] = profiles[index];
    }
    return config;
}

static void put_fifo_word(size_t channel, uint32_t identifier, uint32_t control,
                          uint32_t data_low, uint32_t data_high)
{
    const size_t offset = 176U + channel * 848U;
    uint32_t *const element = (uint32_t *)(void *)(fake_sramcan + offset);
    element[0] = identifier;
    element[1] = control;
    element[2] = data_low;
    element[3] = data_high;
}

static void start_platform(canview_stm_fdcan_platform_t *platform,
                           sink_fixture_t *fixture)
{
    const canview_stm_fdcan_platform_config_t config = config_for(fixture);
    CHECK(canview_stm_fdcan_platform_init(platform, &config) == CANVIEW_OK);
    fake_hardware_ready();
    CHECK(canview_stm_fdcan_platform_start(platform) == CANVIEW_OK);
}

static void lifecycle_tests(void)
{
    fake_hardware_reset();
    sink_fixture_t first_sink = {0};
    sink_fixture_t second_sink = {0};
    canview_stm_fdcan_platform_t first = {0};
    canview_stm_fdcan_platform_t second = {0};
    const canview_stm_fdcan_platform_config_t first_config = config_for(&first_sink);
    const canview_stm_fdcan_platform_config_t second_config = config_for(&second_sink);
    CHECK(canview_stm_fdcan_platform_init(NULL, &first_config) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_fdcan_platform_init(&first, NULL) == CANVIEW_INVALID_ARGUMENT);
    canview_stm_fdcan_platform_t invalid = {0};
    canview_stm_fdcan_platform_config_t invalid_config = first_config;
    invalid_config.frame_sink = NULL;
    CHECK(canview_stm_fdcan_platform_init(&invalid, &invalid_config) == CANVIEW_INVALID_ARGUMENT);
    invalid_config = first_config;
    invalid_config.drop_sink = NULL;
    CHECK(canview_stm_fdcan_platform_init(&invalid, &invalid_config) == CANVIEW_INVALID_ARGUMENT);
    invalid_config = first_config;
    invalid_config.status_sink = NULL;
    CHECK(canview_stm_fdcan_platform_init(&invalid, &invalid_config) == CANVIEW_INVALID_ARGUMENT);
    invalid_config = first_config;
    invalid_config.profiles[0].transceiver = CANVIEW_STM_FDCAN_TRANSCEIVER_MAX3055;
    CHECK(canview_stm_fdcan_platform_init(&invalid, &invalid_config) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_fdcan_platform_init(&first, &first_config) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_platform_init(&second, &second_config) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_platform_start(NULL) == CANVIEW_INVALID_ARGUMENT);
    canview_stm_fdcan_platform_t uninitialized = {0};
    CHECK(canview_stm_fdcan_platform_start(&uninitialized) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_fdcan_platform_start(&first) == CANVIEW_INVALID_ARGUMENT);
    fake_hardware_ready();
    CHECK(canview_stm_fdcan_platform_start(&first) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_platform_init(&first, &first_config) == CANVIEW_RESOURCE_BUSY);
    CHECK((fake_fdcan1.CCCR & (FDCAN_CCCR_MON | FDCAN_CCCR_DAR)) ==
          (FDCAN_CCCR_MON | FDCAN_CCCR_DAR));
    CHECK(canview_stm_fdcan_platform_start(&first) == CANVIEW_RESOURCE_BUSY);
    const uint32_t output_calls = fake_output_calls;
    CHECK(canview_stm_fdcan_platform_stop(&second) == CANVIEW_RESOURCE_BUSY);
    CHECK(fake_output_calls == output_calls);
    canview_stm_fdcan_platform_t active_conflict = {0};
    sink_fixture_t active_conflict_sink = {0};
    const canview_stm_fdcan_platform_config_t active_conflict_config =
        config_for(&active_conflict_sink);
    CHECK(canview_stm_fdcan_platform_init(&active_conflict, &active_conflict_config) ==
          CANVIEW_RESOURCE_BUSY);
    FDCAN2_IT0_IRQHandler();
    FDCAN3_IT0_IRQHandler();
    CHECK(canview_stm_fdcan_platform_stop(&first) == CANVIEW_OK);
    CHECK(first.raw_read_index[0] == 0U && first.raw_write_index[0] == 0U &&
          first.raw_drops[0] == 0U && first.pending_interrupts[0] == 0U);
    CHECK(canview_stm_fdcan_platform_start(&first) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_platform_stop(&first) == CANVIEW_OK);
    first.started[0] = true;
    CHECK(canview_stm_fdcan_platform_start(&first) == CANVIEW_RESOURCE_BUSY);
    first.started[0] = false;
    CHECK(canview_stm_fdcan_platform_stop(NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_fdcan_platform_stop(&uninitialized) == CANVIEW_INVALID_ARGUMENT);

    fake_hardware_reset();
    fake_rcc.CCIPR = RCC_CCIPR_FDCANSEL_0;
    CHECK(canview_stm_fdcan_platform_start(&first) == CANVIEW_INVALID_ARGUMENT);
    fake_hardware_ready();
    fake_rcc.CFGR = 0U;
    CHECK(canview_stm_fdcan_platform_start(&first) == CANVIEW_INVALID_ARGUMENT);
    fake_hardware_ready();
    fake_tim2.CR1 = 0U;
    CHECK(canview_stm_fdcan_platform_start(&first) == CANVIEW_INVALID_ARGUMENT);
    fake_hardware_ready();
    fake_tim2.PSC = 0U;
    CHECK(canview_stm_fdcan_platform_start(&first) == CANVIEW_INVALID_ARGUMENT);
    fake_hardware_ready();
    fake_tim2.ARR = 0U;
    CHECK(canview_stm_fdcan_platform_start(&first) == CANVIEW_INVALID_ARGUMENT);
    fake_hardware_ready();
    CHECK(canview_stm_fdcan_platform_start(&first) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_platform_stop(&first) == CANVIEW_OK);

    fake_hardware_reset();
    sink_fixture_t disabled_sink = {0};
    canview_stm_fdcan_platform_t disabled = {0};
    const canview_stm_fdcan_profile_t disabled_profiles[3] = {
        valid_profile(500000U), {0}, {0}};
    const canview_stm_fdcan_platform_config_t disabled_config =
        config_for_profiles(&disabled_sink, disabled_profiles);
    CHECK(canview_stm_fdcan_platform_init(&disabled, &disabled_config) == CANVIEW_OK);
    fake_hardware_ready();
    CHECK(canview_stm_fdcan_platform_start(&disabled) == CANVIEW_OK);
    CHECK(!fake_outputs[0][4] && fake_outputs[0][5] && !fake_outputs[0][6]);
    CHECK(canview_stm_fdcan_platform_stop(&disabled) == CANVIEW_OK);

    fake_hardware_reset();
    sink_fixture_t first_disabled_sink = {0};
    canview_stm_fdcan_platform_t first_disabled = {0};
    const canview_stm_fdcan_profile_t first_disabled_profiles[3] = {
        {0}, valid_profile(500000U), {0}};
    const canview_stm_fdcan_platform_config_t first_disabled_config =
        config_for_profiles(&first_disabled_sink, first_disabled_profiles);
    CHECK(canview_stm_fdcan_platform_init(&first_disabled, &first_disabled_config) ==
          CANVIEW_OK);
    fake_hardware_ready();
    CHECK(canview_stm_fdcan_platform_start(&first_disabled) == CANVIEW_OK);
    CHECK(fake_outputs[0][4] && !fake_outputs[0][5] && !fake_outputs[0][6]);
    CHECK(canview_stm_fdcan_platform_stop(&first_disabled) == CANVIEW_OK);

    fake_hardware_reset();
    sink_fixture_t all_disabled_sink = {0};
    canview_stm_fdcan_platform_t all_disabled = {0};
    const canview_stm_fdcan_profile_t all_disabled_profiles[3] = {{0}, {0}, {0}};
    const canview_stm_fdcan_platform_config_t all_disabled_config =
        config_for_profiles(&all_disabled_sink, all_disabled_profiles);
    CHECK(canview_stm_fdcan_platform_init(&all_disabled, &all_disabled_config) == CANVIEW_OK);
    fake_hardware_ready();
    CHECK(canview_stm_fdcan_platform_start(&all_disabled) == CANVIEW_INVALID_ARGUMENT);

    fake_hardware_reset();
    canview_stm_fdcan_platform_t failed = {0};
    sink_fixture_t failed_sink = {0};
    const canview_stm_fdcan_platform_config_t failed_config = config_for(&failed_sink);
    CHECK(canview_stm_fdcan_platform_init(&failed, &failed_config) == CANVIEW_OK);
    fake_hardware_ready();
    fake_output_fail_call = 1U;
    CHECK(canview_stm_fdcan_platform_start(&failed) == CANVIEW_TIMEOUT);
    CHECK(fake_output_calls == 6U);
    CHECK(!failed.started[0] && !failed.started[1] && !failed.started[2]);
    fake_output_fail_call = 0U;
    CHECK(canview_stm_fdcan_platform_start(&failed) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_platform_stop(&failed) == CANVIEW_OK);

    fake_hardware_reset();
    sink_fixture_t receive_failed_sink = {0};
    canview_stm_fdcan_platform_t receive_failed = {0};
    const canview_stm_fdcan_platform_config_t receive_failed_config =
        config_for(&receive_failed_sink);
    CHECK(canview_stm_fdcan_platform_init(&receive_failed, &receive_failed_config) == CANVIEW_OK);
    fake_hardware_ready();
    fake_output_fail_call = 7U;
    CHECK(canview_stm_fdcan_platform_start(&receive_failed) == CANVIEW_TIMEOUT);
    CHECK(!receive_failed.started[0] && !receive_failed.started[1] &&
          !receive_failed.started[2]);

    fake_hardware_reset();
    sink_fixture_t rollback_failed_sink = {0};
    canview_stm_fdcan_platform_t rollback_failed = {0};
    const canview_stm_fdcan_platform_config_t rollback_failed_config =
        config_for(&rollback_failed_sink);
    CHECK(canview_stm_fdcan_platform_init(&rollback_failed, &rollback_failed_config) ==
          CANVIEW_OK);
    fake_hardware_ready();
    fake_output_fail_call = 8U;
    fake_output_fail_call_2 = 9U;
    CHECK(canview_stm_fdcan_platform_start(&rollback_failed) == CANVIEW_TIMEOUT);
    CHECK(!rollback_failed.started[0] && !rollback_failed.started[1] &&
          !rollback_failed.started[2]);
    fake_output_fail_call = 0U;
    fake_output_fail_call_2 = 0U;

    fake_hardware_reset();
    sink_fixture_t configure_failed_sink = {0};
    canview_stm_fdcan_platform_t configure_failed = {0};
    const canview_stm_fdcan_platform_config_t configure_failed_config =
        config_for(&configure_failed_sink);
    CHECK(canview_stm_fdcan_platform_init(&configure_failed, &configure_failed_config) ==
          CANVIEW_OK);
    fake_hardware_ready();
    fake_wait_fail_call = 4U;
    CHECK(canview_stm_fdcan_platform_start(&configure_failed) == CANVIEW_TIMEOUT);
    CHECK(!configure_failed.started[0] && !configure_failed.started[1] &&
          !configure_failed.started[2]);
    fake_wait_fail_call = 0U;

    fake_hardware_reset();
    sink_fixture_t stop_output_failed_sink = {0};
    canview_stm_fdcan_platform_t stop_output_failed = {0};
    const canview_stm_fdcan_platform_config_t stop_output_failed_config =
        config_for(&stop_output_failed_sink);
    CHECK(canview_stm_fdcan_platform_init(&stop_output_failed, &stop_output_failed_config) ==
          CANVIEW_OK);
    fake_hardware_ready();
    CHECK(canview_stm_fdcan_platform_start(&stop_output_failed) == CANVIEW_OK);
    fake_output_fail_call = fake_output_calls + 1U;
    CHECK(canview_stm_fdcan_platform_stop(&stop_output_failed) == CANVIEW_TIMEOUT);
    fake_output_fail_call = 0U;

    fake_hardware_reset();
    sink_fixture_t stop_wait_failed_sink = {0};
    canview_stm_fdcan_platform_t stop_wait_failed = {0};
    const canview_stm_fdcan_platform_config_t stop_wait_failed_config =
        config_for(&stop_wait_failed_sink);
    CHECK(canview_stm_fdcan_platform_init(&stop_wait_failed, &stop_wait_failed_config) ==
          CANVIEW_OK);
    fake_hardware_ready();
    CHECK(canview_stm_fdcan_platform_start(&stop_wait_failed) == CANVIEW_OK);
    fake_wait_fail_call = fake_wait_calls + 1U;
    CHECK(canview_stm_fdcan_platform_stop(&stop_wait_failed) == CANVIEW_TIMEOUT);
    fake_wait_fail_call = 0U;

    fake_hardware_reset();
    sink_fixture_t poll_failed_sink = {0};
    canview_stm_fdcan_platform_t poll_failed = {0};
    const canview_stm_fdcan_platform_config_t poll_failed_config =
        config_for(&poll_failed_sink);
    CHECK(canview_stm_fdcan_platform_init(&poll_failed, &poll_failed_config) == CANVIEW_OK);
    fake_hardware_ready();
    fake_wait_poll_mismatch_call = 1U;
    CHECK(canview_stm_fdcan_platform_start(&poll_failed) == CANVIEW_TIMEOUT);
    CHECK(!poll_failed.started[0] && !poll_failed.started[1] && !poll_failed.started[2]);
    fake_wait_poll_mismatch_call = 0U;
}

static void fifo_and_loss_tests(void)
{
    fake_hardware_reset();
    sink_fixture_t fixture = {0};
    canview_stm_fdcan_platform_t platform = {0};
    start_platform(&platform, &fixture);
    CHECK(canview_stm_fdcan_platform_service(NULL, 0U) == CANVIEW_INVALID_ARGUMENT);
    canview_stm_fdcan_platform_t uninitialized = {0};
    CHECK(canview_stm_fdcan_platform_service(&uninitialized, 0U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_fdcan_platform_service(&platform, 1U) == CANVIEW_OK);
    canview_stm_fdcan_frame_sink_fn *const saved_frame_sink = platform.config.frame_sink;
    platform.config.frame_sink = NULL;
    CHECK(canview_stm_fdcan_platform_service(&platform, 1U) == CANVIEW_INVALID_ARGUMENT);
    platform.config.frame_sink = saved_frame_sink;
    canview_stm_fdcan_drop_sink_fn *const saved_drop_sink = platform.config.drop_sink;
    platform.config.drop_sink = NULL;
    CHECK(canview_stm_fdcan_platform_service(&platform, 1U) == CANVIEW_INVALID_ARGUMENT);
    platform.config.drop_sink = saved_drop_sink;
    canview_stm_fdcan_status_sink_fn *const saved_status_sink = platform.config.status_sink;
    platform.config.status_sink = NULL;
    CHECK(canview_stm_fdcan_platform_service(&platform, 1U) == CANVIEW_INVALID_ARGUMENT);
    platform.config.status_sink = saved_status_sink;

    fake_fdcan1.RXF0S = 0U;
    fake_fdcan1.IR = FDCAN_IR_RF0N;
    FDCAN1_IT0_IRQHandler();
    CHECK(canview_stm_fdcan_platform_service(&platform, 2U) == CANVIEW_OK);

    put_fifo_word(0U, UINT32_C(0x123) << 18U, UINT32_C(2) << 16U,
                  UINT32_C(0x00332211), UINT32_C(0x88776655));
    fake_fdcan1.RXF0S = UINT32_C(3);
    fake_fdcan1.IR = FDCAN_IR_RF0N;
    FDCAN1_IT0_IRQHandler();
    CHECK(canview_stm_fdcan_platform_service(&platform, 99U) == CANVIEW_OK);
    CHECK(fixture.frames == 3U && fixture.last_channel == 0U &&
          fixture.last_frame.can_id == 0x123U && fixture.last_frame.dlc == 2U &&
          fixture.last_frame.data[0] == 0x11U && fixture.last_frame.data[1] == 0x22U);

    fake_fdcan2.RXF0S = UINT32_C(1) | (UINT32_C(3) << 8U);
    fake_fdcan2.IR = FDCAN_IR_RF0N;
    FDCAN2_IT0_IRQHandler();
    CHECK(canview_stm_fdcan_platform_service(&platform, 99U) == CANVIEW_OK);
    CHECK(fixture.last_state == CANVIEW_STM_FDCAN_BUS_FAULT &&
          (fixture.last_error & CANVIEW_STM_FDCAN_PLATFORM_ERROR_FIFO_LOSS) != 0U);

    fake_fdcan3.PSR = FDCAN_PSR_EP;
    fake_fdcan3.ECR = UINT32_C(0x1234);
    fake_fdcan3.IR = FDCAN_IR_EP | FDCAN_IR_EW;
    FDCAN3_IT0_IRQHandler();
    CHECK(canview_stm_fdcan_platform_service(&platform, 100U) == CANVIEW_OK);
    CHECK(fixture.last_state == CANVIEW_STM_FDCAN_BUS_ERROR_PASSIVE &&
          fixture.last_error == (FDCAN_IR_EP | FDCAN_IR_EW));
    fake_fdcan3.PSR = FDCAN_PSR_BO;
    fake_fdcan3.IR = FDCAN_IR_BO;
    FDCAN3_IT0_IRQHandler();
    CHECK(canview_stm_fdcan_platform_service(&platform, 101U) == CANVIEW_OK);
    CHECK(fixture.last_state == CANVIEW_STM_FDCAN_BUS_OFF &&
          platform.bus_off_count[2] == 1U);
    fake_fdcan3.IR = FDCAN_IR_BO;
    FDCAN3_IT0_IRQHandler();
    CHECK(canview_stm_fdcan_platform_service(&platform, 102U) == CANVIEW_OK);
    CHECK(platform.bus_off_count[2] == 1U);
    platform.previous_state[2] = CANVIEW_STM_FDCAN_BUS_ERROR_ACTIVE;
    platform.bus_off_count[2] = UINT32_MAX;
    fake_fdcan3.IR = FDCAN_IR_BO;
    FDCAN3_IT0_IRQHandler();
    CHECK(canview_stm_fdcan_platform_service(&platform, 103U) == CANVIEW_OK);
    CHECK(platform.bus_off_count[2] == UINT32_MAX);

    fake_fdcan1.RXF0S = UINT32_C(4);
    fake_fdcan1.IR = FDCAN_IR_RF0N;
    FDCAN1_IT0_IRQHandler();
    CHECK(canview_stm_fdcan_platform_service(&platform, 100U) == CANVIEW_OK);
    CHECK(fixture.last_state == CANVIEW_STM_FDCAN_BUS_FAULT &&
          (fixture.last_error & CANVIEW_STM_FDCAN_PLATFORM_ERROR_FIFO_LOSS) != 0U);

    fake_fdcan1.RXF0S = UINT32_C(3);
    for (size_t call = 0U; call < 6U; ++call)
    {
        fake_fdcan1.IR = FDCAN_IR_RF0N;
        FDCAN1_IT0_IRQHandler();
    }
    fixture.fail_drop = true;
    CHECK(canview_stm_fdcan_platform_service(&platform, 104U) == CANVIEW_TIMEOUT);
    fixture.fail_drop = false;
    CHECK(canview_stm_fdcan_platform_service(&platform, 105U) == CANVIEW_OK);
    CHECK(fixture.frames == 19U && fixture.drops == 2U);
    CHECK((fixture.last_error & CANVIEW_STM_FDCAN_PLATFORM_ERROR_RAW_RING_OVERFLOW) != 0U);

    platform.reported_raw_drops[0] = UINT32_MAX;
    CHECK(canview_stm_fdcan_platform_service(&platform, 106U) == CANVIEW_OK);
    CHECK(fixture.drops == 4U);

    put_fifo_word(0U, UINT32_C(0x321) << 18U, UINT32_C(1) << 16U, 0U, 0U);
    fake_fdcan1.RXF0S = 1U;
    fake_fdcan1.IR = FDCAN_IR_RF0N;
    FDCAN1_IT0_IRQHandler();
    fixture.fail_frame = true;
    CHECK(canview_stm_fdcan_platform_service(&platform, 107U) == CANVIEW_TIMEOUT);
    fixture.fail_frame = false;
    CHECK(platform.sink_failures[0] != 0U);
    platform.raw_read_index[0] = 0U;
    platform.raw_write_index[0] = CANVIEW_STM_FDCAN_PLATFORM_RAW_RING_CAPACITY;
    platform.raw_drops[0] = UINT32_MAX;
    platform.reported_raw_drops[0] = UINT32_MAX;
    fake_fdcan1.RXF0S = 1U;
    fake_fdcan1.IR = FDCAN_IR_RF0N;
    FDCAN1_IT0_IRQHandler();
    platform.raw_read_index[0] = platform.raw_write_index[0];
    CHECK(canview_stm_fdcan_platform_service(&platform, 108U) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_platform_stop(&platform) == CANVIEW_OK);

    FDCAN1_IT0_IRQHandler();
    CHECK(canview_stm_fdcan_platform_service(&platform, 108U) == CANVIEW_OK);

    /* stop() must discard the previous session's raw snapshots. */
    CHECK(canview_stm_fdcan_platform_start(&platform) == CANVIEW_OK);
    CHECK(canview_stm_fdcan_platform_service(&platform, 102U) == CANVIEW_OK);
    CHECK(fixture.frames == 20U && fixture.drops == 4U);
    CHECK(canview_stm_fdcan_platform_stop(&platform) == CANVIEW_OK);
}

static void hardware_loss_tests(void)
{
    fake_hardware_reset();
    sink_fixture_t fixture = {0};
    canview_stm_fdcan_platform_t platform = {0};
    start_platform(&platform, &fixture);

    fake_fdcan1.RXF0S = UINT32_C(1) | FDCAN_RXF0S_RF0L;
    fake_fdcan1.IR = FDCAN_IR_RF0L;
    FDCAN1_IT0_IRQHandler();
    CHECK(canview_stm_fdcan_platform_service(&platform, 200U) == CANVIEW_OK);
    CHECK(fixture.last_state == CANVIEW_STM_FDCAN_BUS_FAULT &&
          (fixture.last_error & CANVIEW_STM_FDCAN_PLATFORM_ERROR_FIFO_LOSS) != 0U);

    fake_fdcan1.RXF0S = 0U;
    fake_fdcan1.IR = FDCAN_IR_MRAF;
    FDCAN1_IT0_IRQHandler();
    CHECK(canview_stm_fdcan_platform_service(&platform, 201U) == CANVIEW_OK);
    CHECK(fixture.last_state == CANVIEW_STM_FDCAN_BUS_FAULT &&
          (fixture.last_error & CANVIEW_STM_FDCAN_PLATFORM_ERROR_MESSAGE_RAM) != 0U);

    fake_fdcan1.IR = FDCAN_IR_RF0F;
    FDCAN1_IT0_IRQHandler();
    CHECK(canview_stm_fdcan_platform_service(&platform, 202U) == CANVIEW_OK);
    CHECK(fixture.last_state == CANVIEW_STM_FDCAN_BUS_FAULT &&
          (fixture.last_error & CANVIEW_STM_FDCAN_PLATFORM_ERROR_FIFO_LOSS) != 0U &&
          (fixture.last_error & CANVIEW_STM_FDCAN_PLATFORM_ERROR_MESSAGE_RAM) != 0U &&
          (fixture.last_error & FDCAN_IR_RF0F) != 0U &&
          platform.session_fault_flags[0] ==
              (CANVIEW_STM_FDCAN_ERROR_FIFO_LOSS | CANVIEW_STM_FDCAN_ERROR_MESSAGE_RAM));
    CHECK(canview_stm_fdcan_platform_stop(&platform) == CANVIEW_OK);
}

static void callback_safety_tests(void)
{
    fake_hardware_reset();
    sink_fixture_t fixture = {0};
    canview_stm_fdcan_platform_t platform = {0};
    start_platform(&platform, &fixture);
    fixture.platform = &platform;
    fixture.reenter_service = true;
    put_fifo_word(0U, UINT32_C(0x321) << 18U, UINT32_C(1) << 16U, 0U, 0U);
    fake_fdcan1.RXF0S = 1U;
    fake_fdcan1.IR = FDCAN_IR_RF0N;
    FDCAN1_IT0_IRQHandler();
    CHECK(canview_stm_fdcan_platform_service(&platform, 300U) == CANVIEW_OK);
    CHECK(fixture.reentry_status == CANVIEW_RESOURCE_BUSY);
    CHECK(fixture.frames == 3U);

    put_fifo_word(0U, UINT32_C(0x322) << 18U, UINT32_C(1) << 16U, 0U, 0U);
    fake_fdcan1.RXF0S = 1U;
    fake_fdcan1.IR = FDCAN_IR_RF0N;
    FDCAN1_IT0_IRQHandler();
    const uint8_t read_index = platform.raw_read_index[0];
    fixture.fail_frame = true;
    CHECK(canview_stm_fdcan_platform_service(&platform, 301U) == CANVIEW_TIMEOUT);
    CHECK(platform.raw_read_index[0] == read_index && fixture.frames == 4U);
    fixture.fail_frame = false;
    CHECK(canview_stm_fdcan_platform_service(&platform, 302U) == CANVIEW_OK);
    CHECK(platform.raw_read_index[0] != read_index && fixture.frames == 7U);
    CHECK(canview_stm_fdcan_platform_stop(&platform) == CANVIEW_OK);
}

int main(void)
{
    lifecycle_tests();
    fifo_and_loss_tests();
    hardware_loss_tests();
    callback_safety_tests();
    (void)puts("PASS: STM32 FDCAN CMSIS adapter fake-register tests");
    return EXIT_SUCCESS;
}
