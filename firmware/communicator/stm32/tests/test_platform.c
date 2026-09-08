/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_build_mode.h"
#include "canview_stm_build.h"
#include "canview_stm_diagnostic.h"
#include "canview_stm_service.h"
#include "canview_stm_stack.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition)                                                                           \
    do                                                                                             \
    {                                                                                              \
        if (!(condition))                                                                          \
        {                                                                                          \
            (void)fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition);                  \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

static uint16_t read_u16_le(const uint8_t *buffer, size_t offset)
{
    return (uint16_t)((uint16_t)buffer[offset] | ((uint16_t)buffer[offset + 1U] << 8U));
}

static uint32_t read_u32_le(const uint8_t *buffer, size_t offset)
{
    return (uint32_t)buffer[offset] | ((uint32_t)buffer[offset + 1U] << 8U) |
           ((uint32_t)buffer[offset + 2U] << 16U) | ((uint32_t)buffer[offset + 3U] << 24U);
}

static void metadata_tests(void)
{
    canview_stm_build_metadata_t metadata;
    CHECK(canview_stm_build_metadata_get(NULL) == CANVIEW_INVALID_ARGUMENT);
    memset(&metadata, 0xff, sizeof(metadata));
    CHECK(canview_stm_build_metadata_get(&metadata) == CANVIEW_OK);
    CHECK(metadata.protocol_schema_sha256 != NULL &&
          strlen(metadata.protocol_schema_sha256) == 64U);
    CHECK(metadata.uart_protocol_schema_sha256 != NULL &&
          strlen(metadata.uart_protocol_schema_sha256) == 64U);
    CHECK(metadata.hardware_digest != NULL && strlen(metadata.hardware_digest) == 64U);
    CHECK(metadata.build_mode != NULL && strcmp(metadata.build_mode, "CAPTURE_ONLY") == 0);
    CHECK(metadata.board_profile != 0U && metadata.build_contract_digest != 0U);
    CHECK(metadata.control_capabilities == 0U && !metadata.tx_permit);
    CHECK(CANVIEW_STM_BUILD_MODE == CANVIEW_STM_BUILD_MODE_CAPTURE_ONLY);
}

static void stack_tests(void)
{
    uint8_t region[512];
    uint8_t small_region[64];
    canview_stm_stack_watermark_t watermark = {0};
    canview_stm_stack_watermark_snapshot_t snapshot = {0U, 0U, false};

    CHECK(canview_stm_stack_watermark_arm(NULL, region, sizeof(region)) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_stack_watermark_arm(&watermark, NULL, sizeof(region)) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_stack_watermark_arm(&watermark, region, 0U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_stack_watermark_arm(&watermark, region,
                                         CANVIEW_STM_STACK_WATERMARK_MIN_BYTES - 1U) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_stack_watermark_arm(&watermark, region,
                                         CANVIEW_STM_STACK_WATERMARK_MAX_BYTES + 1U) ==
          CANVIEW_OVERSIZE);
    CHECK(canview_stm_stack_watermark_sample(&watermark, &snapshot) == CANVIEW_INCOMPLETE);
    CHECK(canview_stm_stack_watermark_sample(&watermark, NULL) == CANVIEW_INVALID_ARGUMENT);

    CHECK(canview_stm_stack_watermark_arm(&watermark, region, sizeof(region)) == CANVIEW_OK);
    for (size_t index = 0U; index < sizeof(region); ++index)
    {
        CHECK(region[index] == CANVIEW_STM_STACK_WATERMARK_PATTERN);
    }
    CHECK(canview_stm_stack_watermark_arm(&watermark, region, sizeof(region)) ==
          CANVIEW_RESOURCE_BUSY);
    CHECK(canview_stm_stack_watermark_sample(&watermark, &snapshot) == CANVIEW_OK);
    CHECK(snapshot.valid && snapshot.current_free_bytes == sizeof(region) &&
          snapshot.minimum_free_bytes == sizeof(region));

    region[sizeof(region) - 1U] = 0U;
    CHECK(canview_stm_stack_watermark_sample(&watermark, &snapshot) == CANVIEW_OK);
    CHECK(snapshot.valid && snapshot.current_free_bytes == sizeof(region) - 1U &&
          snapshot.minimum_free_bytes == sizeof(region) - 1U);
    memset(&region[400U], 0, sizeof(region) - 400U);
    CHECK(canview_stm_stack_watermark_sample(&watermark, &snapshot) == CANVIEW_OK);
    CHECK(snapshot.valid && snapshot.current_free_bytes == 400U &&
          snapshot.minimum_free_bytes == 400U);
    memset(region, 0, 400U);
    CHECK(canview_stm_stack_watermark_sample(&watermark, &snapshot) == CANVIEW_TIMEOUT);
    CHECK(!snapshot.valid && watermark.minimum_free_bytes == 400U);

    canview_stm_stack_watermark_t zero_watermark = {0};
    CHECK(canview_stm_stack_watermark_arm(&zero_watermark, small_region, sizeof(small_region)) ==
          CANVIEW_OK);
    memset(small_region, 0, sizeof(small_region));
    CHECK(canview_stm_stack_watermark_sample(&zero_watermark, &snapshot) == CANVIEW_OK);
    CHECK(snapshot.valid && snapshot.current_free_bytes == 0U &&
          snapshot.minimum_free_bytes == 0U);
    CHECK(canview_stm_stack_watermark_sample(&zero_watermark, &snapshot) == CANVIEW_OK);
    CHECK(snapshot.valid && snapshot.current_free_bytes == 0U);
    zero_watermark.busy = true;
    CHECK(canview_stm_stack_watermark_sample(&zero_watermark, &snapshot) ==
          CANVIEW_RESOURCE_BUSY);
    CHECK(canview_stm_stack_watermark_arm(&zero_watermark, small_region, sizeof(small_region)) ==
          CANVIEW_RESOURCE_BUSY);
    zero_watermark.busy = false;
    zero_watermark.minimum_free_bytes = zero_watermark.region_size + 1U;
    CHECK(canview_stm_stack_watermark_sample(&zero_watermark, &snapshot) == CANVIEW_MALFORMED);
}

static void service_policy_tests(void)
{
    canview_stm_service_root_t root;
    canview_stm_service_decision_t decision;
    CHECK(canview_stm_service_root_init(NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_service_root_init(&root) == CANVIEW_OK);
    CHECK(root.magic == CANVIEW_STM_SERVICE_ROOT_MAGIC &&
          root.version == CANVIEW_STM_SERVICE_ROOT_VERSION &&
          root.byte_size == (uint16_t)sizeof(root));
    CHECK(canview_stm_service_policy_evaluate(NULL, CANVIEW_STM_RESET_REASON_UNKNOWN, &decision) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_service_policy_evaluate(&root, CANVIEW_STM_RESET_REASON_UNKNOWN, NULL) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_service_policy_evaluate(&root, CANVIEW_STM_RESET_REASON_UNKNOWN, &decision) ==
          CANVIEW_OK);
    CHECK(decision.root_page_valid && !decision.authenticity_known &&
          !decision.production_debug_lock_known && !decision.control_policy_satisfied &&
          decision.control_capabilities == 0U && !decision.tx_permit &&
          !decision.erase_on_service_reset);

    root.authenticity_known = true;
    root.authenticity_valid = true;
    root.production_debug_lock_known = true;
    root.production_debug_locked = true;
    CHECK(canview_stm_service_policy_evaluate(&root, CANVIEW_STM_RESET_REASON_SOFTWARE,
                                              &decision) == CANVIEW_OK);
    CHECK(decision.control_policy_satisfied && decision.control_capabilities == 0U &&
          !decision.tx_permit);
    root.service_window_asserted = true;
    CHECK(canview_stm_service_policy_evaluate(&root, CANVIEW_STM_RESET_REASON_SOFTWARE,
                                              &decision) == CANVIEW_OK);
    CHECK(decision.erase_on_service_reset);
    CHECK(canview_stm_service_policy_evaluate(&root, CANVIEW_STM_RESET_REASON_WATCHDOG,
                                              &decision) == CANVIEW_OK &&
          decision.erase_on_service_reset);
    CHECK(canview_stm_service_policy_evaluate(&root, CANVIEW_STM_RESET_REASON_UNKNOWN,
                                              &decision) == CANVIEW_OK &&
          !decision.erase_on_service_reset);
    CHECK(canview_stm_service_policy_evaluate(&root, CANVIEW_STM_RESET_REASON_AMBIGUOUS,
                                              &decision) == CANVIEW_OK &&
          !decision.erase_on_service_reset);

    root.authenticity_valid = false;
    CHECK(canview_stm_service_policy_evaluate(&root, CANVIEW_STM_RESET_REASON_SOFTWARE,
                                              &decision) == CANVIEW_OK &&
          !decision.control_policy_satisfied);
    root.authenticity_valid = true;
    root.production_debug_lock_known = false;
    CHECK(canview_stm_service_policy_evaluate(&root, CANVIEW_STM_RESET_REASON_SOFTWARE,
                                              &decision) == CANVIEW_OK &&
          !decision.control_policy_satisfied);
    root.production_debug_lock_known = true;
    root.production_debug_locked = false;
    CHECK(canview_stm_service_policy_evaluate(&root, CANVIEW_STM_RESET_REASON_SOFTWARE,
                                              &decision) == CANVIEW_OK &&
          !decision.control_policy_satisfied);
    root.production_debug_locked = true;

    const canview_stm_reset_reason_t invalid_reason =
        (canview_stm_reset_reason_t)(CANVIEW_STM_RESET_REASON_MAX + 1);
    CHECK(canview_stm_service_policy_evaluate(&root, invalid_reason, &decision) ==
          CANVIEW_MALFORMED);
    const canview_stm_reset_reason_t negative_reason =
        (canview_stm_reset_reason_t)-1;
    CHECK(canview_stm_service_policy_evaluate(&root, negative_reason, &decision) ==
          CANVIEW_MALFORMED);
    root.magic ^= UINT32_C(1);
    CHECK(canview_stm_service_policy_evaluate(&root, CANVIEW_STM_RESET_REASON_SOFTWARE,
                                              &decision) == CANVIEW_MALFORMED);
    root.magic = CANVIEW_STM_SERVICE_ROOT_MAGIC;
    root.version = UINT16_C(2);
    CHECK(canview_stm_service_policy_evaluate(&root, CANVIEW_STM_RESET_REASON_SOFTWARE,
                                              &decision) == CANVIEW_MALFORMED);
    root.version = CANVIEW_STM_SERVICE_ROOT_VERSION;
    root.byte_size = UINT16_C(1);
    CHECK(canview_stm_service_policy_evaluate(&root, CANVIEW_STM_RESET_REASON_SOFTWARE,
                                              &decision) == CANVIEW_MALFORMED);
}

static void diagnostic_tests(void)
{
    uint8_t buffer[CANVIEW_STM_DIAGNOSTIC_ENCODED_BYTES + 1U];
    uint8_t before[sizeof(buffer)];
    size_t written = 123U;
    canview_stm_diagnostic_t diagnostic = {
        .reset_flags = UINT32_C(0x12345678),
        .reset_reason = CANVIEW_STM_RESET_REASON_WATCHDOG,
        .sysclk_hz = UINT32_C(160000000),
        .peripheral_hz = UINT32_C(80000000),
        .control_capabilities = 0U,
        .tx_permit = false,
        .authenticity_known = true,
        .production_debug_lock_known = true,
        .build_metadata_valid = true,
        .build_contract_digest = UINT32_C(0x10203040),
        .board_profile = UINT32_C(0x91b049a8),
        .stack_free_bytes = UINT32_C(0x11223344),
        .stack_min_free_bytes = UINT32_C(0x55667788),
        .stack_watermark_valid = true,
        .service_reset_erase_pending = true,
        .protocol_schema_sha256 = "protocol",
        .uart_protocol_schema_sha256 = "uart",
        .hardware_digest = "hardware",
        .build_mode = "CAPTURE_ONLY"};

    CHECK(canview_stm_diagnostic_encode(NULL, buffer, sizeof(buffer), &written) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_diagnostic_encode(&diagnostic, NULL, sizeof(buffer), &written) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_diagnostic_encode(&diagnostic, buffer, sizeof(buffer), NULL) ==
          CANVIEW_INVALID_ARGUMENT);
    memset(buffer, 0xcc, sizeof(buffer));
    memcpy(before, buffer, sizeof(buffer));
    written = 123U;
    CHECK(canview_stm_diagnostic_encode(&diagnostic, buffer,
                                        CANVIEW_STM_DIAGNOSTIC_ENCODED_BYTES - 1U,
                                        &written) == CANVIEW_BUFFER_TOO_SMALL);
    CHECK(written == 0U && memcmp(buffer, before, sizeof(buffer)) == 0);

    diagnostic.reset_reason = (canview_stm_reset_reason_t)(CANVIEW_STM_RESET_REASON_MAX + 1);
    CHECK(canview_stm_diagnostic_encode(&diagnostic, buffer, sizeof(buffer), &written) ==
          CANVIEW_MALFORMED);
    diagnostic.reset_reason = (canview_stm_reset_reason_t)-1;
    CHECK(canview_stm_diagnostic_encode(&diagnostic, buffer, sizeof(buffer), &written) ==
          CANVIEW_MALFORMED);
    diagnostic.reset_reason = CANVIEW_STM_RESET_REASON_WATCHDOG;
    diagnostic.tx_permit = true;
    CHECK(canview_stm_diagnostic_encode(&diagnostic, buffer, sizeof(buffer), &written) ==
          CANVIEW_MALFORMED);
    diagnostic.tx_permit = false;
    diagnostic.control_capabilities = 1U;
    CHECK(canview_stm_diagnostic_encode(&diagnostic, buffer, sizeof(buffer), &written) ==
          CANVIEW_MALFORMED);
    diagnostic.control_capabilities = 0U;

#if SIZE_MAX > UINT32_MAX
    diagnostic.stack_free_bytes = (size_t)UINT32_MAX + (size_t)1U;
    CHECK(canview_stm_diagnostic_encode(&diagnostic, buffer, sizeof(buffer), &written) ==
          CANVIEW_MALFORMED);
    diagnostic.stack_free_bytes = UINT32_C(0x11223344);
#endif

    bool *const status_fields[] = {&diagnostic.authenticity_known,
                                   &diagnostic.production_debug_lock_known,
                                   &diagnostic.build_metadata_valid,
                                   &diagnostic.stack_watermark_valid,
                                   &diagnostic.service_reset_erase_pending};
    for (size_t index = 0U; index < sizeof(status_fields) / sizeof(status_fields[0]); ++index)
    {
        *status_fields[index] = false;
        CHECK(canview_stm_diagnostic_encode(&diagnostic, buffer, sizeof(buffer), &written) ==
              CANVIEW_OK);
        *status_fields[index] = true;
    }

    memset(buffer, 0, sizeof(buffer));
    CHECK(canview_stm_diagnostic_encode(&diagnostic, buffer, sizeof(buffer), &written) ==
          CANVIEW_OK);
    CHECK(written == CANVIEW_STM_DIAGNOSTIC_ENCODED_BYTES);
    CHECK(read_u32_le(buffer, 0U) == CANVIEW_STM_DIAGNOSTIC_RECORD_MAGIC);
    CHECK(buffer[4U] == CANVIEW_STM_DIAGNOSTIC_RECORD_VERSION && buffer[5U] == 0U);
    CHECK(read_u16_le(buffer, 6U) ==
          (CANVIEW_STM_DIAGNOSTIC_STATUS_AUTHENTICITY_KNOWN |
           CANVIEW_STM_DIAGNOSTIC_STATUS_DEBUG_LOCK_KNOWN |
           CANVIEW_STM_DIAGNOSTIC_STATUS_BUILD_VALID |
           CANVIEW_STM_DIAGNOSTIC_STATUS_STACK_VALID |
           CANVIEW_STM_DIAGNOSTIC_STATUS_ERASE_PENDING));
    CHECK(read_u32_le(buffer, 8U) == diagnostic.reset_flags);
    CHECK(read_u32_le(buffer, 12U) == diagnostic.sysclk_hz);
    CHECK(read_u32_le(buffer, 16U) == diagnostic.peripheral_hz);
    CHECK(read_u32_le(buffer, 20U) == diagnostic.board_profile);
    CHECK(read_u32_le(buffer, 24U) == diagnostic.build_contract_digest);
    CHECK(read_u32_le(buffer, 28U) == (uint32_t)diagnostic.stack_free_bytes);
    CHECK(read_u32_le(buffer, 32U) == (uint32_t)diagnostic.stack_min_free_bytes);
    CHECK(read_u32_le(buffer, 36U) == 0U);
}

int main(void)
{
    metadata_tests();
    stack_tests();
    service_policy_tests();
    diagnostic_tests();
    (void)puts("PASS: STM32 platform metadata/stack/service/diagnostic C99 tests");
    return 0;
}
