/* GENERATED FILE - DO NOT EDIT. */
/* Source: protocol/schema/uart-v1.0.yaml */
/* Regenerate with: python tools/generate_uart_protocol.py --write */
/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef CANVIEW_UART_PROTOCOL_H
#define CANVIEW_UART_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CANVIEW_UART_PROTOCOL_SCHEMA_SHA256 "0d6b161fa31b3372ab63a054d60b4b2321387eac205d39e1d1641e7c8a553b02"
#define CANVIEW_UART_PROTOCOL_NAME "Communicator UART v1.0"
#define CANVIEW_UART_PROTOCOL_MAJOR UINT8_C(1)
#define CANVIEW_UART_PROTOCOL_MINOR UINT8_C(0)
#define CANVIEW_UART_HEADER_SIZE UINT16_C(32)
#define CANVIEW_UART_MAX_FRAME_SIZE UINT16_C(1024)
#define CANVIEW_UART_MAX_PAYLOAD_SIZE UINT16_C(992)
#define CANVIEW_UART_MAX_ENCODED_SIZE UINT16_C(1029)
#define CANVIEW_UART_MAX_SERIAL_SIZE (CANVIEW_UART_MAX_ENCODED_SIZE + 1U)
#define CANVIEW_UART_BAUD UINT32_C(4000000)
#define CANVIEW_UART_COMMAND_CACHE_CAPACITY UINT16_C(256)
#define CANVIEW_UART_COMMAND_RESULT_MAX UINT16_C(82)
#define CANVIEW_UART_COMMAND_CACHE_RETENTION_MS UINT32_C(60000)
#define CANVIEW_UART_COMMAND_TTL_MIN_MS UINT16_C(500)
#define CANVIEW_UART_COMMAND_TTL_MAX_MS UINT16_C(30000)
#define CANVIEW_UART_PLAN_MAX_CHUNKS UINT8_C(16)
#define CANVIEW_UART_PLAN_MAX_FILTERS UINT8_C(64)
#define CANVIEW_UART_PLAN_CHUNK_MAX_FILTERS UINT8_C(16)
#define CANVIEW_UART_PLAN_STAGING_TIMEOUT_MS UINT32_C(2000)
#define CANVIEW_UART_PLAN_MIN_RECORDS_PER_SECOND UINT16_C(1)
#define CANVIEW_UART_PLAN_MAX_RECORDS_PER_SECOND UINT16_C(200)
#define CANVIEW_UART_PLAN_MIN_BYTES_PER_SECOND UINT32_C(512)
#define CANVIEW_UART_PLAN_MAX_BYTES_PER_SECOND UINT32_C(20000)
#define CANVIEW_UART_CAN_BATCH_MAX_RECORDS UINT8_C(12)
#define CANVIEW_UART_CAN_ID_STATS_MAX_RECORDS UINT8_C(5)
#define CANVIEW_UART_CONFIG_MAX_RECORDS UINT8_C(25)
#define CANVIEW_UART_DIAGNOSTIC_COUNTER_MAX_RECORDS UINT8_C(12)

#define CANVIEW_UART_FLAG_ACK_REQUIRED UINT8_C(1)
#define CANVIEW_UART_FLAG_RESPONSE UINT8_C(2)
#define CANVIEW_UART_FLAG_ERROR UINT8_C(4)
#define CANVIEW_UART_FLAG_HIGH_PRIORITY UINT8_C(8)
#define CANVIEW_UART_FLAG_SNAPSHOT UINT8_C(16)
#define CANVIEW_UART_FLAG_KNOWN_MASK UINT8_C(0x1F)

#if defined(_MSC_VER)
#define CANVIEW_UART_PACKED
#pragma pack(push, 1)
#elif defined(__GNUC__) || defined(__clang__)
#define CANVIEW_UART_PACKED __attribute__((packed))
#else
#define CANVIEW_UART_PACKED
#endif

typedef enum {
    CANVIEW_UART_MSG_LINK_HELLO = UINT8_C(0x01),
    CANVIEW_UART_MSG_LINK_HELLO_ACK = UINT8_C(0x02),
    CANVIEW_UART_MSG_HEARTBEAT = UINT8_C(0x03),
    CANVIEW_UART_MSG_ACK = UINT8_C(0x04),
    CANVIEW_UART_MSG_ERROR = UINT8_C(0x05),
    CANVIEW_UART_MSG_CAN_RX_BATCH = UINT8_C(0x10),
    CANVIEW_UART_MSG_CAN_BUS_STATUS = UINT8_C(0x11),
    CANVIEW_UART_MSG_SAFETY_SNAPSHOT = UINT8_C(0x12),
    CANVIEW_UART_MSG_CAN_TX_AUDIT = UINT8_C(0x13),
    CANVIEW_UART_MSG_CAN_ID_STATS = UINT8_C(0x14),
    CANVIEW_UART_MSG_CAN_OBSERVER_PLAN = UINT8_C(0x15),
    CANVIEW_UART_MSG_CAN_CAPTURE_CONTROL = UINT8_C(0x16),
    CANVIEW_UART_MSG_CAN_CAPTURE_STATUS = UINT8_C(0x17),
    CANVIEW_UART_MSG_CAN_EVENT_MARKER = UINT8_C(0x18),
    CANVIEW_UART_MSG_COMMAND_REQUEST = UINT8_C(0x20),
    CANVIEW_UART_MSG_COMMAND_RESULT = UINT8_C(0x21),
    CANVIEW_UART_MSG_CONTROL_LEASE = UINT8_C(0x22),
    CANVIEW_UART_MSG_CONFIG_GET = UINT8_C(0x30),
    CANVIEW_UART_MSG_CONFIG_SET = UINT8_C(0x31),
    CANVIEW_UART_MSG_CONFIG_RESULT = UINT8_C(0x32),
    CANVIEW_UART_MSG_DIAGNOSTIC_COUNTERS = UINT8_C(0x40),
    CANVIEW_UART_MSG_FIRMWARE_PREPARE = UINT8_C(0x50),
} canview_uart_message_type_t;
#define CANVIEW_UART_MESSAGE_COUNT UINT8_C(22)

typedef enum {
    CANVIEW_UART_PLAN_OP_BEGIN = UINT8_C(1),
    CANVIEW_UART_PLAN_OP_CHUNK = UINT8_C(2),
    CANVIEW_UART_PLAN_OP_COMMIT = UINT8_C(3),
    CANVIEW_UART_PLAN_OP_ABORT = UINT8_C(4),
} canview_uart_plan_operation_t;

typedef enum {
    CANVIEW_UART_CAPTURE_ARM = UINT8_C(1),
    CANVIEW_UART_CAPTURE_START = UINT8_C(2),
    CANVIEW_UART_CAPTURE_STOP = UINT8_C(3),
    CANVIEW_UART_CAPTURE_CANCEL = UINT8_C(4),
} canview_uart_capture_action_t;

typedef enum {
    CANVIEW_UART_PAYLOAD_FIXED = UINT8_C(0),
    CANVIEW_UART_PAYLOAD_CAN_BATCH = UINT8_C(1),
    CANVIEW_UART_PAYLOAD_BOUNDED = UINT8_C(2),
    CANVIEW_UART_PAYLOAD_SUFFIX = UINT8_C(3),
    CANVIEW_UART_PAYLOAD_VARIANTS = UINT8_C(4),
} canview_uart_payload_kind_t;

typedef enum {
    CANVIEW_UART_DIRECTION_BOTH = UINT8_C(0),
    CANVIEW_UART_DIRECTION_ESP_TO_STM = UINT8_C(1),
    CANVIEW_UART_DIRECTION_STM_TO_ESP = UINT8_C(2),
} canview_uart_message_direction_t;

typedef struct {
    uint8_t message_type;
    uint16_t min_payload;
    uint16_t max_payload;
    uint8_t required_flags;
    uint8_t allowed_flags;
    uint8_t payload_kind;
    uint8_t direction;
    uint8_t supported;
} canview_uart_message_policy_t;

typedef struct CANVIEW_UART_PACKED {
    uint64_t boot_id_le;
    uint8_t protocol_min_major;
    uint8_t protocol_min_minor;
    uint8_t protocol_max_major;
    uint8_t protocol_max_minor;
    uint16_t hardware_revision_le;
    uint16_t firmware_major_le;
    uint16_t firmware_minor_le;
    uint16_t firmware_patch_le;
    uint64_t capability_bits_le;
    uint16_t max_packet_le;
    uint16_t reserved0_le;
    uint8_t build_id_digest[16U];
    uint64_t device_id_le;
    uint8_t capability_digest[16U];
} canview_uart_link_hello_payload_t;
typedef char canview_uart_assert_canview_uart_link_hello_payload_t_size[(sizeof(canview_uart_link_hello_payload_t) == 72U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_link_hello_payload_t_boot_id_le_offset[(offsetof(canview_uart_link_hello_payload_t, boot_id_le) == 0U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_link_hello_payload_t_protocol_min_major_offset[(offsetof(canview_uart_link_hello_payload_t, protocol_min_major) == 8U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_link_hello_payload_t_protocol_min_minor_offset[(offsetof(canview_uart_link_hello_payload_t, protocol_min_minor) == 9U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_link_hello_payload_t_protocol_max_major_offset[(offsetof(canview_uart_link_hello_payload_t, protocol_max_major) == 10U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_link_hello_payload_t_protocol_max_minor_offset[(offsetof(canview_uart_link_hello_payload_t, protocol_max_minor) == 11U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_link_hello_payload_t_hardware_revision_le_offset[(offsetof(canview_uart_link_hello_payload_t, hardware_revision_le) == 12U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_link_hello_payload_t_firmware_major_le_offset[(offsetof(canview_uart_link_hello_payload_t, firmware_major_le) == 14U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_link_hello_payload_t_firmware_minor_le_offset[(offsetof(canview_uart_link_hello_payload_t, firmware_minor_le) == 16U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_link_hello_payload_t_firmware_patch_le_offset[(offsetof(canview_uart_link_hello_payload_t, firmware_patch_le) == 18U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_link_hello_payload_t_capability_bits_le_offset[(offsetof(canview_uart_link_hello_payload_t, capability_bits_le) == 20U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_link_hello_payload_t_max_packet_le_offset[(offsetof(canview_uart_link_hello_payload_t, max_packet_le) == 28U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_link_hello_payload_t_reserved0_le_offset[(offsetof(canview_uart_link_hello_payload_t, reserved0_le) == 30U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_link_hello_payload_t_build_id_digest_offset[(offsetof(canview_uart_link_hello_payload_t, build_id_digest) == 32U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_link_hello_payload_t_device_id_le_offset[(offsetof(canview_uart_link_hello_payload_t, device_id_le) == 48U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_link_hello_payload_t_capability_digest_offset[(offsetof(canview_uart_link_hello_payload_t, capability_digest) == 56U) ? 1 : -1];

typedef struct CANVIEW_UART_PACKED {
    uint64_t local_boot_id_le;
    uint64_t peer_boot_id_le;
    uint8_t selected_major;
    uint8_t selected_minor;
    uint8_t result;
    uint8_t reserved0;
    uint64_t capability_bits_le;
    uint16_t max_packet_le;
    uint16_t reserved1_le;
    uint8_t negotiation_digest[8U];
} canview_uart_link_hello_ack_payload_t;
typedef char canview_uart_assert_canview_uart_link_hello_ack_payload_t_size[(sizeof(canview_uart_link_hello_ack_payload_t) == 40U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_link_hello_ack_payload_t_local_boot_id_le_offset[(offsetof(canview_uart_link_hello_ack_payload_t, local_boot_id_le) == 0U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_link_hello_ack_payload_t_peer_boot_id_le_offset[(offsetof(canview_uart_link_hello_ack_payload_t, peer_boot_id_le) == 8U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_link_hello_ack_payload_t_selected_major_offset[(offsetof(canview_uart_link_hello_ack_payload_t, selected_major) == 16U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_link_hello_ack_payload_t_selected_minor_offset[(offsetof(canview_uart_link_hello_ack_payload_t, selected_minor) == 17U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_link_hello_ack_payload_t_result_offset[(offsetof(canview_uart_link_hello_ack_payload_t, result) == 18U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_link_hello_ack_payload_t_reserved0_offset[(offsetof(canview_uart_link_hello_ack_payload_t, reserved0) == 19U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_link_hello_ack_payload_t_capability_bits_le_offset[(offsetof(canview_uart_link_hello_ack_payload_t, capability_bits_le) == 20U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_link_hello_ack_payload_t_max_packet_le_offset[(offsetof(canview_uart_link_hello_ack_payload_t, max_packet_le) == 28U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_link_hello_ack_payload_t_reserved1_le_offset[(offsetof(canview_uart_link_hello_ack_payload_t, reserved1_le) == 30U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_link_hello_ack_payload_t_negotiation_digest_offset[(offsetof(canview_uart_link_hello_ack_payload_t, negotiation_digest) == 32U) ? 1 : -1];

typedef struct CANVIEW_UART_PACKED {
    uint64_t boot_id_le;
    uint64_t uptime_us_le;
    uint8_t link_state;
    uint8_t flags;
    uint16_t safety_queue_depth_le;
    uint16_t state_queue_depth_le;
    uint16_t raw_queue_depth_le;
    uint32_t telemetry_dropped_le;
    uint32_t protocol_errors_le;
    uint32_t cts_blocked_ms_le;
    uint16_t safety_inhibit_le;
    uint8_t active_bus_mask;
    uint8_t bus_error_mask;
    uint32_t state_revision_le;
    uint32_t reserved0_le;
} canview_uart_heartbeat_payload_t;
typedef char canview_uart_assert_canview_uart_heartbeat_payload_t_size[(sizeof(canview_uart_heartbeat_payload_t) == 48U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_heartbeat_payload_t_boot_id_le_offset[(offsetof(canview_uart_heartbeat_payload_t, boot_id_le) == 0U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_heartbeat_payload_t_uptime_us_le_offset[(offsetof(canview_uart_heartbeat_payload_t, uptime_us_le) == 8U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_heartbeat_payload_t_link_state_offset[(offsetof(canview_uart_heartbeat_payload_t, link_state) == 16U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_heartbeat_payload_t_flags_offset[(offsetof(canview_uart_heartbeat_payload_t, flags) == 17U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_heartbeat_payload_t_safety_queue_depth_le_offset[(offsetof(canview_uart_heartbeat_payload_t, safety_queue_depth_le) == 18U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_heartbeat_payload_t_state_queue_depth_le_offset[(offsetof(canview_uart_heartbeat_payload_t, state_queue_depth_le) == 20U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_heartbeat_payload_t_raw_queue_depth_le_offset[(offsetof(canview_uart_heartbeat_payload_t, raw_queue_depth_le) == 22U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_heartbeat_payload_t_telemetry_dropped_le_offset[(offsetof(canview_uart_heartbeat_payload_t, telemetry_dropped_le) == 24U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_heartbeat_payload_t_protocol_errors_le_offset[(offsetof(canview_uart_heartbeat_payload_t, protocol_errors_le) == 28U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_heartbeat_payload_t_cts_blocked_ms_le_offset[(offsetof(canview_uart_heartbeat_payload_t, cts_blocked_ms_le) == 32U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_heartbeat_payload_t_safety_inhibit_le_offset[(offsetof(canview_uart_heartbeat_payload_t, safety_inhibit_le) == 36U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_heartbeat_payload_t_active_bus_mask_offset[(offsetof(canview_uart_heartbeat_payload_t, active_bus_mask) == 38U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_heartbeat_payload_t_bus_error_mask_offset[(offsetof(canview_uart_heartbeat_payload_t, bus_error_mask) == 39U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_heartbeat_payload_t_state_revision_le_offset[(offsetof(canview_uart_heartbeat_payload_t, state_revision_le) == 40U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_heartbeat_payload_t_reserved0_le_offset[(offsetof(canview_uart_heartbeat_payload_t, reserved0_le) == 44U) ? 1 : -1];

typedef struct CANVIEW_UART_PACKED {
    uint32_t acknowledged_sequence_le;
    uint16_t detail_le;
    uint16_t status_le;
    uint64_t request_token_le;
    uint32_t receiver_time_ms_le;
} canview_uart_ack_payload_t;
typedef char canview_uart_assert_canview_uart_ack_payload_t_size[(sizeof(canview_uart_ack_payload_t) == 20U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_ack_payload_t_acknowledged_sequence_le_offset[(offsetof(canview_uart_ack_payload_t, acknowledged_sequence_le) == 0U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_ack_payload_t_detail_le_offset[(offsetof(canview_uart_ack_payload_t, detail_le) == 4U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_ack_payload_t_status_le_offset[(offsetof(canview_uart_ack_payload_t, status_le) == 6U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_ack_payload_t_request_token_le_offset[(offsetof(canview_uart_ack_payload_t, request_token_le) == 8U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_ack_payload_t_receiver_time_ms_le_offset[(offsetof(canview_uart_ack_payload_t, receiver_time_ms_le) == 16U) ? 1 : -1];

typedef struct CANVIEW_UART_PACKED {
    uint16_t code_le;
    uint8_t severity;
    uint8_t origin;
    uint8_t offending_message_type;
    uint8_t reserved0;
    uint16_t detail_le;
    uint32_t offending_sequence_le;
    uint32_t retry_after_ms_le;
    uint32_t reserved1_le;
} canview_uart_error_payload_t;
typedef char canview_uart_assert_canview_uart_error_payload_t_size[(sizeof(canview_uart_error_payload_t) == 20U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_error_payload_t_code_le_offset[(offsetof(canview_uart_error_payload_t, code_le) == 0U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_error_payload_t_severity_offset[(offsetof(canview_uart_error_payload_t, severity) == 2U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_error_payload_t_origin_offset[(offsetof(canview_uart_error_payload_t, origin) == 3U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_error_payload_t_offending_message_type_offset[(offsetof(canview_uart_error_payload_t, offending_message_type) == 4U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_error_payload_t_reserved0_offset[(offsetof(canview_uart_error_payload_t, reserved0) == 5U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_error_payload_t_detail_le_offset[(offsetof(canview_uart_error_payload_t, detail_le) == 6U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_error_payload_t_offending_sequence_le_offset[(offsetof(canview_uart_error_payload_t, offending_sequence_le) == 8U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_error_payload_t_retry_after_ms_le_offset[(offsetof(canview_uart_error_payload_t, retry_after_ms_le) == 12U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_error_payload_t_reserved1_le_offset[(offsetof(canview_uart_error_payload_t, reserved1_le) == 16U) ? 1 : -1];

typedef struct CANVIEW_UART_PACKED {
    uint8_t bus_id;
    uint8_t state;
    uint16_t flags_le;
    uint32_t bitrate_le;
    uint16_t rx_error_le;
    uint16_t tx_error_le;
    uint32_t bus_off_count_le;
    uint16_t last_error_code_le;
    uint16_t reserved0_le;
    uint64_t timestamp_us_le;
} canview_uart_can_bus_status_payload_t;
typedef char canview_uart_assert_canview_uart_can_bus_status_payload_t_size[(sizeof(canview_uart_can_bus_status_payload_t) == 28U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_bus_status_payload_t_bus_id_offset[(offsetof(canview_uart_can_bus_status_payload_t, bus_id) == 0U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_bus_status_payload_t_state_offset[(offsetof(canview_uart_can_bus_status_payload_t, state) == 1U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_bus_status_payload_t_flags_le_offset[(offsetof(canview_uart_can_bus_status_payload_t, flags_le) == 2U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_bus_status_payload_t_bitrate_le_offset[(offsetof(canview_uart_can_bus_status_payload_t, bitrate_le) == 4U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_bus_status_payload_t_rx_error_le_offset[(offsetof(canview_uart_can_bus_status_payload_t, rx_error_le) == 8U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_bus_status_payload_t_tx_error_le_offset[(offsetof(canview_uart_can_bus_status_payload_t, tx_error_le) == 10U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_bus_status_payload_t_bus_off_count_le_offset[(offsetof(canview_uart_can_bus_status_payload_t, bus_off_count_le) == 12U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_bus_status_payload_t_last_error_code_le_offset[(offsetof(canview_uart_can_bus_status_payload_t, last_error_code_le) == 16U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_bus_status_payload_t_reserved0_le_offset[(offsetof(canview_uart_can_bus_status_payload_t, reserved0_le) == 18U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_bus_status_payload_t_timestamp_us_le_offset[(offsetof(canview_uart_can_bus_status_payload_t, timestamp_us_le) == 20U) ? 1 : -1];

typedef struct CANVIEW_UART_PACKED {
    uint64_t boot_id_le;
    uint32_t state_revision_le;
    uint32_t profile_revision_le;
    uint32_t safety_flags_le;
    uint16_t inhibit_reason_le;
    uint8_t control_lease_state;
    uint8_t tx_gate;
    uint8_t active_bus_mask;
    uint8_t bus_error_mask;
    uint16_t reserved0_le;
    uint32_t can_profile_id_le;
    uint64_t snapshot_time_us_le;
    uint8_t vehicle_state_digest[8U];
} canview_uart_safety_snapshot_payload_t;
typedef char canview_uart_assert_canview_uart_safety_snapshot_payload_t_size[(sizeof(canview_uart_safety_snapshot_payload_t) == 48U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_safety_snapshot_payload_t_boot_id_le_offset[(offsetof(canview_uart_safety_snapshot_payload_t, boot_id_le) == 0U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_safety_snapshot_payload_t_state_revision_le_offset[(offsetof(canview_uart_safety_snapshot_payload_t, state_revision_le) == 8U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_safety_snapshot_payload_t_profile_revision_le_offset[(offsetof(canview_uart_safety_snapshot_payload_t, profile_revision_le) == 12U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_safety_snapshot_payload_t_safety_flags_le_offset[(offsetof(canview_uart_safety_snapshot_payload_t, safety_flags_le) == 16U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_safety_snapshot_payload_t_inhibit_reason_le_offset[(offsetof(canview_uart_safety_snapshot_payload_t, inhibit_reason_le) == 20U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_safety_snapshot_payload_t_control_lease_state_offset[(offsetof(canview_uart_safety_snapshot_payload_t, control_lease_state) == 22U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_safety_snapshot_payload_t_tx_gate_offset[(offsetof(canview_uart_safety_snapshot_payload_t, tx_gate) == 23U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_safety_snapshot_payload_t_active_bus_mask_offset[(offsetof(canview_uart_safety_snapshot_payload_t, active_bus_mask) == 24U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_safety_snapshot_payload_t_bus_error_mask_offset[(offsetof(canview_uart_safety_snapshot_payload_t, bus_error_mask) == 25U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_safety_snapshot_payload_t_reserved0_le_offset[(offsetof(canview_uart_safety_snapshot_payload_t, reserved0_le) == 26U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_safety_snapshot_payload_t_can_profile_id_le_offset[(offsetof(canview_uart_safety_snapshot_payload_t, can_profile_id_le) == 28U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_safety_snapshot_payload_t_snapshot_time_us_le_offset[(offsetof(canview_uart_safety_snapshot_payload_t, snapshot_time_us_le) == 32U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_safety_snapshot_payload_t_vehicle_state_digest_offset[(offsetof(canview_uart_safety_snapshot_payload_t, vehicle_state_digest) == 40U) ? 1 : -1];

typedef struct CANVIEW_UART_PACKED {
    uint64_t request_token_le;
    uint16_t command_id_le;
    uint8_t status;
    uint8_t reserved0;
    uint32_t can_id_le;
    uint8_t bus_id;
    uint8_t dlc;
    uint16_t flags_le;
    uint64_t tx_time_us_le;
    uint32_t safety_revision_le;
    uint32_t feedback_revision_le;
    uint32_t reserved1_le;
} canview_uart_can_tx_audit_payload_t;
typedef char canview_uart_assert_canview_uart_can_tx_audit_payload_t_size[(sizeof(canview_uart_can_tx_audit_payload_t) == 40U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_tx_audit_payload_t_request_token_le_offset[(offsetof(canview_uart_can_tx_audit_payload_t, request_token_le) == 0U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_tx_audit_payload_t_command_id_le_offset[(offsetof(canview_uart_can_tx_audit_payload_t, command_id_le) == 8U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_tx_audit_payload_t_status_offset[(offsetof(canview_uart_can_tx_audit_payload_t, status) == 10U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_tx_audit_payload_t_reserved0_offset[(offsetof(canview_uart_can_tx_audit_payload_t, reserved0) == 11U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_tx_audit_payload_t_can_id_le_offset[(offsetof(canview_uart_can_tx_audit_payload_t, can_id_le) == 12U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_tx_audit_payload_t_bus_id_offset[(offsetof(canview_uart_can_tx_audit_payload_t, bus_id) == 16U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_tx_audit_payload_t_dlc_offset[(offsetof(canview_uart_can_tx_audit_payload_t, dlc) == 17U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_tx_audit_payload_t_flags_le_offset[(offsetof(canview_uart_can_tx_audit_payload_t, flags_le) == 18U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_tx_audit_payload_t_tx_time_us_le_offset[(offsetof(canview_uart_can_tx_audit_payload_t, tx_time_us_le) == 20U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_tx_audit_payload_t_safety_revision_le_offset[(offsetof(canview_uart_can_tx_audit_payload_t, safety_revision_le) == 28U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_tx_audit_payload_t_feedback_revision_le_offset[(offsetof(canview_uart_can_tx_audit_payload_t, feedback_revision_le) == 32U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_tx_audit_payload_t_reserved1_le_offset[(offsetof(canview_uart_can_tx_audit_payload_t, reserved1_le) == 36U) ? 1 : -1];

typedef struct CANVIEW_UART_PACKED {
    uint32_t window_end_ms_le;
    uint8_t count;
    uint8_t part_index;
    uint8_t part_count;
    uint8_t reserved0;
} canview_uart_can_id_stats_prefix_t;
typedef char canview_uart_assert_canview_uart_can_id_stats_prefix_t_size[(sizeof(canview_uart_can_id_stats_prefix_t) == 8U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_id_stats_prefix_t_window_end_ms_le_offset[(offsetof(canview_uart_can_id_stats_prefix_t, window_end_ms_le) == 0U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_id_stats_prefix_t_count_offset[(offsetof(canview_uart_can_id_stats_prefix_t, count) == 4U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_id_stats_prefix_t_part_index_offset[(offsetof(canview_uart_can_id_stats_prefix_t, part_index) == 5U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_id_stats_prefix_t_part_count_offset[(offsetof(canview_uart_can_id_stats_prefix_t, part_count) == 6U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_id_stats_prefix_t_reserved0_offset[(offsetof(canview_uart_can_id_stats_prefix_t, reserved0) == 7U) ? 1 : -1];

typedef struct CANVIEW_UART_PACKED {
    uint8_t bus_id;
    uint8_t flags_dlc;
    uint16_t rate_tenth_hz_le;
    uint32_t can_id_le;
    uint32_t frame_count_le;
    uint32_t change_count_le;
    uint32_t period_p50_us_le;
    uint32_t period_p95_us_le;
    uint64_t bit_change_mask_le;
    uint8_t last_data[8U];
} canview_uart_can_id_stats_record_t;
typedef char canview_uart_assert_canview_uart_can_id_stats_record_t_size[(sizeof(canview_uart_can_id_stats_record_t) == 40U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_id_stats_record_t_bus_id_offset[(offsetof(canview_uart_can_id_stats_record_t, bus_id) == 0U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_id_stats_record_t_flags_dlc_offset[(offsetof(canview_uart_can_id_stats_record_t, flags_dlc) == 1U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_id_stats_record_t_rate_tenth_hz_le_offset[(offsetof(canview_uart_can_id_stats_record_t, rate_tenth_hz_le) == 2U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_id_stats_record_t_can_id_le_offset[(offsetof(canview_uart_can_id_stats_record_t, can_id_le) == 4U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_id_stats_record_t_frame_count_le_offset[(offsetof(canview_uart_can_id_stats_record_t, frame_count_le) == 8U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_id_stats_record_t_change_count_le_offset[(offsetof(canview_uart_can_id_stats_record_t, change_count_le) == 12U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_id_stats_record_t_period_p50_us_le_offset[(offsetof(canview_uart_can_id_stats_record_t, period_p50_us_le) == 16U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_id_stats_record_t_period_p95_us_le_offset[(offsetof(canview_uart_can_id_stats_record_t, period_p95_us_le) == 20U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_id_stats_record_t_bit_change_mask_le_offset[(offsetof(canview_uart_can_id_stats_record_t, bit_change_mask_le) == 24U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_can_id_stats_record_t_last_data_offset[(offsetof(canview_uart_can_id_stats_record_t, last_data) == 32U) ? 1 : -1];

typedef struct CANVIEW_UART_PACKED {
    uint8_t operation;
    uint8_t reserved0[3U];
    uint64_t request_token_le;
    uint32_t plan_revision_le;
    uint32_t expected_active_revision_le;
    uint16_t total_chunks_le;
    uint16_t filter_count_le;
    uint16_t max_records_per_second_le;
    uint32_t max_bytes_per_second_le;
    uint8_t bus_mask;
    uint8_t reserved1;
} canview_uart_observer_plan_begin_t;
typedef char canview_uart_assert_canview_uart_observer_plan_begin_t_size[(sizeof(canview_uart_observer_plan_begin_t) == 32U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_plan_begin_t_operation_offset[(offsetof(canview_uart_observer_plan_begin_t, operation) == 0U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_plan_begin_t_reserved0_offset[(offsetof(canview_uart_observer_plan_begin_t, reserved0) == 1U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_plan_begin_t_request_token_le_offset[(offsetof(canview_uart_observer_plan_begin_t, request_token_le) == 4U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_plan_begin_t_plan_revision_le_offset[(offsetof(canview_uart_observer_plan_begin_t, plan_revision_le) == 12U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_plan_begin_t_expected_active_revision_le_offset[(offsetof(canview_uart_observer_plan_begin_t, expected_active_revision_le) == 16U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_plan_begin_t_total_chunks_le_offset[(offsetof(canview_uart_observer_plan_begin_t, total_chunks_le) == 20U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_plan_begin_t_filter_count_le_offset[(offsetof(canview_uart_observer_plan_begin_t, filter_count_le) == 22U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_plan_begin_t_max_records_per_second_le_offset[(offsetof(canview_uart_observer_plan_begin_t, max_records_per_second_le) == 24U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_plan_begin_t_max_bytes_per_second_le_offset[(offsetof(canview_uart_observer_plan_begin_t, max_bytes_per_second_le) == 26U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_plan_begin_t_bus_mask_offset[(offsetof(canview_uart_observer_plan_begin_t, bus_mask) == 30U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_plan_begin_t_reserved1_offset[(offsetof(canview_uart_observer_plan_begin_t, reserved1) == 31U) ? 1 : -1];

typedef struct CANVIEW_UART_PACKED {
    uint8_t operation;
    uint8_t reserved0[3U];
    uint64_t request_token_le;
    uint32_t plan_revision_le;
    uint16_t chunk_index_le;
    uint16_t chunk_count_le;
    uint8_t record_count;
    uint8_t reserved1[3U];
} canview_uart_observer_plan_chunk_t;
typedef char canview_uart_assert_canview_uart_observer_plan_chunk_t_size[(sizeof(canview_uart_observer_plan_chunk_t) == 24U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_plan_chunk_t_operation_offset[(offsetof(canview_uart_observer_plan_chunk_t, operation) == 0U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_plan_chunk_t_reserved0_offset[(offsetof(canview_uart_observer_plan_chunk_t, reserved0) == 1U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_plan_chunk_t_request_token_le_offset[(offsetof(canview_uart_observer_plan_chunk_t, request_token_le) == 4U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_plan_chunk_t_plan_revision_le_offset[(offsetof(canview_uart_observer_plan_chunk_t, plan_revision_le) == 12U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_plan_chunk_t_chunk_index_le_offset[(offsetof(canview_uart_observer_plan_chunk_t, chunk_index_le) == 16U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_plan_chunk_t_chunk_count_le_offset[(offsetof(canview_uart_observer_plan_chunk_t, chunk_count_le) == 18U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_plan_chunk_t_record_count_offset[(offsetof(canview_uart_observer_plan_chunk_t, record_count) == 20U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_plan_chunk_t_reserved1_offset[(offsetof(canview_uart_observer_plan_chunk_t, reserved1) == 21U) ? 1 : -1];

typedef struct CANVIEW_UART_PACKED {
    uint8_t bus_id;
    uint8_t flags;
    uint16_t reserved0_le;
    uint32_t can_id_le;
    uint32_t can_mask_le;
} canview_uart_observer_filter_record_t;
typedef char canview_uart_assert_canview_uart_observer_filter_record_t_size[(sizeof(canview_uart_observer_filter_record_t) == 12U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_filter_record_t_bus_id_offset[(offsetof(canview_uart_observer_filter_record_t, bus_id) == 0U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_filter_record_t_flags_offset[(offsetof(canview_uart_observer_filter_record_t, flags) == 1U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_filter_record_t_reserved0_le_offset[(offsetof(canview_uart_observer_filter_record_t, reserved0_le) == 2U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_filter_record_t_can_id_le_offset[(offsetof(canview_uart_observer_filter_record_t, can_id_le) == 4U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_filter_record_t_can_mask_le_offset[(offsetof(canview_uart_observer_filter_record_t, can_mask_le) == 8U) ? 1 : -1];

typedef struct CANVIEW_UART_PACKED {
    uint8_t operation;
    uint8_t reserved0[3U];
    uint64_t request_token_le;
    uint32_t plan_revision_le;
    uint8_t plan_digest[32U];
} canview_uart_observer_plan_commit_t;
typedef char canview_uart_assert_canview_uart_observer_plan_commit_t_size[(sizeof(canview_uart_observer_plan_commit_t) == 48U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_plan_commit_t_operation_offset[(offsetof(canview_uart_observer_plan_commit_t, operation) == 0U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_plan_commit_t_reserved0_offset[(offsetof(canview_uart_observer_plan_commit_t, reserved0) == 1U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_plan_commit_t_request_token_le_offset[(offsetof(canview_uart_observer_plan_commit_t, request_token_le) == 4U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_plan_commit_t_plan_revision_le_offset[(offsetof(canview_uart_observer_plan_commit_t, plan_revision_le) == 12U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_plan_commit_t_plan_digest_offset[(offsetof(canview_uart_observer_plan_commit_t, plan_digest) == 16U) ? 1 : -1];

typedef struct CANVIEW_UART_PACKED {
    uint8_t operation;
    uint8_t reserved0[3U];
    uint64_t request_token_le;
    uint32_t plan_revision_le;
    uint16_t reason_le;
    uint16_t reserved1_le;
} canview_uart_observer_plan_abort_t;
typedef char canview_uart_assert_canview_uart_observer_plan_abort_t_size[(sizeof(canview_uart_observer_plan_abort_t) == 20U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_plan_abort_t_operation_offset[(offsetof(canview_uart_observer_plan_abort_t, operation) == 0U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_plan_abort_t_reserved0_offset[(offsetof(canview_uart_observer_plan_abort_t, reserved0) == 1U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_plan_abort_t_request_token_le_offset[(offsetof(canview_uart_observer_plan_abort_t, request_token_le) == 4U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_plan_abort_t_plan_revision_le_offset[(offsetof(canview_uart_observer_plan_abort_t, plan_revision_le) == 12U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_plan_abort_t_reason_le_offset[(offsetof(canview_uart_observer_plan_abort_t, reason_le) == 16U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_observer_plan_abort_t_reserved1_le_offset[(offsetof(canview_uart_observer_plan_abort_t, reserved1_le) == 18U) ? 1 : -1];

typedef struct CANVIEW_UART_PACKED {
    uint64_t request_token_le;
    uint64_t capture_id_le;
    uint8_t action;
    uint8_t reserved0;
    uint16_t flags_le;
    uint32_t requested_time_ms_le;
    uint32_t reserved1_le;
} canview_uart_capture_control_payload_t;
typedef char canview_uart_assert_canview_uart_capture_control_payload_t_size[(sizeof(canview_uart_capture_control_payload_t) == 28U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_capture_control_payload_t_request_token_le_offset[(offsetof(canview_uart_capture_control_payload_t, request_token_le) == 0U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_capture_control_payload_t_capture_id_le_offset[(offsetof(canview_uart_capture_control_payload_t, capture_id_le) == 8U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_capture_control_payload_t_action_offset[(offsetof(canview_uart_capture_control_payload_t, action) == 16U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_capture_control_payload_t_reserved0_offset[(offsetof(canview_uart_capture_control_payload_t, reserved0) == 17U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_capture_control_payload_t_flags_le_offset[(offsetof(canview_uart_capture_control_payload_t, flags_le) == 18U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_capture_control_payload_t_requested_time_ms_le_offset[(offsetof(canview_uart_capture_control_payload_t, requested_time_ms_le) == 20U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_capture_control_payload_t_reserved1_le_offset[(offsetof(canview_uart_capture_control_payload_t, reserved1_le) == 24U) ? 1 : -1];

typedef struct CANVIEW_UART_PACKED {
    uint64_t request_token_le;
    uint64_t capture_id_le;
    uint8_t state;
    uint16_t reason_le;
    uint8_t bus_mask;
    uint32_t accepted_records_le;
    uint32_t dropped_records_le;
    uint32_t stored_bytes_le;
    uint32_t remaining_ms_le;
    uint32_t effective_filter_revision_le;
    uint32_t reserved0_le;
} canview_uart_capture_status_payload_t;
typedef char canview_uart_assert_canview_uart_capture_status_payload_t_size[(sizeof(canview_uart_capture_status_payload_t) == 44U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_capture_status_payload_t_request_token_le_offset[(offsetof(canview_uart_capture_status_payload_t, request_token_le) == 0U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_capture_status_payload_t_capture_id_le_offset[(offsetof(canview_uart_capture_status_payload_t, capture_id_le) == 8U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_capture_status_payload_t_state_offset[(offsetof(canview_uart_capture_status_payload_t, state) == 16U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_capture_status_payload_t_reason_le_offset[(offsetof(canview_uart_capture_status_payload_t, reason_le) == 17U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_capture_status_payload_t_bus_mask_offset[(offsetof(canview_uart_capture_status_payload_t, bus_mask) == 19U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_capture_status_payload_t_accepted_records_le_offset[(offsetof(canview_uart_capture_status_payload_t, accepted_records_le) == 20U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_capture_status_payload_t_dropped_records_le_offset[(offsetof(canview_uart_capture_status_payload_t, dropped_records_le) == 24U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_capture_status_payload_t_stored_bytes_le_offset[(offsetof(canview_uart_capture_status_payload_t, stored_bytes_le) == 28U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_capture_status_payload_t_remaining_ms_le_offset[(offsetof(canview_uart_capture_status_payload_t, remaining_ms_le) == 32U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_capture_status_payload_t_effective_filter_revision_le_offset[(offsetof(canview_uart_capture_status_payload_t, effective_filter_revision_le) == 36U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_capture_status_payload_t_reserved0_le_offset[(offsetof(canview_uart_capture_status_payload_t, reserved0_le) == 40U) ? 1 : -1];

typedef struct CANVIEW_UART_PACKED {
    uint64_t capture_id_le;
    uint32_t marker_id_le;
    uint32_t sender_time_ms_le;
    uint16_t marker_kind_le;
    uint16_t flags_le;
    uint16_t label_code_le;
    uint16_t reserved0_le;
    uint32_t time_uncertainty_us_le;
} canview_uart_event_marker_payload_t;
typedef char canview_uart_assert_canview_uart_event_marker_payload_t_size[(sizeof(canview_uart_event_marker_payload_t) == 28U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_event_marker_payload_t_capture_id_le_offset[(offsetof(canview_uart_event_marker_payload_t, capture_id_le) == 0U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_event_marker_payload_t_marker_id_le_offset[(offsetof(canview_uart_event_marker_payload_t, marker_id_le) == 8U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_event_marker_payload_t_sender_time_ms_le_offset[(offsetof(canview_uart_event_marker_payload_t, sender_time_ms_le) == 12U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_event_marker_payload_t_marker_kind_le_offset[(offsetof(canview_uart_event_marker_payload_t, marker_kind_le) == 16U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_event_marker_payload_t_flags_le_offset[(offsetof(canview_uart_event_marker_payload_t, flags_le) == 18U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_event_marker_payload_t_label_code_le_offset[(offsetof(canview_uart_event_marker_payload_t, label_code_le) == 20U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_event_marker_payload_t_reserved0_le_offset[(offsetof(canview_uart_event_marker_payload_t, reserved0_le) == 22U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_event_marker_payload_t_time_uncertainty_us_le_offset[(offsetof(canview_uart_event_marker_payload_t, time_uncertainty_us_le) == 24U) ? 1 : -1];

typedef struct CANVIEW_UART_PACKED {
    uint64_t request_token_le;
    uint16_t command_id_le;
    uint16_t ttl_ms_le;
    uint64_t origin_device_id_le;
    uint64_t origin_boot_id_le;
    uint32_t wireless_session_id_le;
    uint32_t control_generation_le;
    uint32_t issued_at_controller_ms_le;
    uint32_t control_sync_generation_le;
    uint32_t expected_state_revision_le;
    uint32_t precondition_flags_le;
    uint16_t argument_tlv_length_le;
    uint16_t reserved0_le;
    uint8_t canonical_argument_digest[32U];
    uint8_t control_tag[16U];
} canview_uart_command_request_t;
typedef char canview_uart_assert_canview_uart_command_request_t_size[(sizeof(canview_uart_command_request_t) == 104U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_command_request_t_request_token_le_offset[(offsetof(canview_uart_command_request_t, request_token_le) == 0U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_command_request_t_command_id_le_offset[(offsetof(canview_uart_command_request_t, command_id_le) == 8U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_command_request_t_ttl_ms_le_offset[(offsetof(canview_uart_command_request_t, ttl_ms_le) == 10U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_command_request_t_origin_device_id_le_offset[(offsetof(canview_uart_command_request_t, origin_device_id_le) == 12U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_command_request_t_origin_boot_id_le_offset[(offsetof(canview_uart_command_request_t, origin_boot_id_le) == 20U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_command_request_t_wireless_session_id_le_offset[(offsetof(canview_uart_command_request_t, wireless_session_id_le) == 28U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_command_request_t_control_generation_le_offset[(offsetof(canview_uart_command_request_t, control_generation_le) == 32U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_command_request_t_issued_at_controller_ms_le_offset[(offsetof(canview_uart_command_request_t, issued_at_controller_ms_le) == 36U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_command_request_t_control_sync_generation_le_offset[(offsetof(canview_uart_command_request_t, control_sync_generation_le) == 40U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_command_request_t_expected_state_revision_le_offset[(offsetof(canview_uart_command_request_t, expected_state_revision_le) == 44U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_command_request_t_precondition_flags_le_offset[(offsetof(canview_uart_command_request_t, precondition_flags_le) == 48U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_command_request_t_argument_tlv_length_le_offset[(offsetof(canview_uart_command_request_t, argument_tlv_length_le) == 52U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_command_request_t_reserved0_le_offset[(offsetof(canview_uart_command_request_t, reserved0_le) == 54U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_command_request_t_canonical_argument_digest_offset[(offsetof(canview_uart_command_request_t, canonical_argument_digest) == 56U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_command_request_t_control_tag_offset[(offsetof(canview_uart_command_request_t, control_tag) == 88U) ? 1 : -1];

typedef struct CANVIEW_UART_PACKED {
    uint64_t request_token_le;
    uint16_t command_id_le;
    uint8_t stage;
    uint8_t reserved0;
    uint16_t reason_le;
    uint32_t state_revision_le;
    uint32_t completed_time_ms_le;
    uint32_t request_sequence_le;
    uint8_t control_tag[16U];
    uint32_t feedback_revision_le;
    uint32_t feedback_time_ms_le;
    uint8_t canonical_argument_digest[32U];
} canview_uart_command_result_t;
typedef char canview_uart_assert_canview_uart_command_result_t_size[(sizeof(canview_uart_command_result_t) == 82U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_command_result_t_request_token_le_offset[(offsetof(canview_uart_command_result_t, request_token_le) == 0U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_command_result_t_command_id_le_offset[(offsetof(canview_uart_command_result_t, command_id_le) == 8U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_command_result_t_stage_offset[(offsetof(canview_uart_command_result_t, stage) == 10U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_command_result_t_reserved0_offset[(offsetof(canview_uart_command_result_t, reserved0) == 11U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_command_result_t_reason_le_offset[(offsetof(canview_uart_command_result_t, reason_le) == 12U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_command_result_t_state_revision_le_offset[(offsetof(canview_uart_command_result_t, state_revision_le) == 14U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_command_result_t_completed_time_ms_le_offset[(offsetof(canview_uart_command_result_t, completed_time_ms_le) == 18U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_command_result_t_request_sequence_le_offset[(offsetof(canview_uart_command_result_t, request_sequence_le) == 22U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_command_result_t_control_tag_offset[(offsetof(canview_uart_command_result_t, control_tag) == 26U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_command_result_t_feedback_revision_le_offset[(offsetof(canview_uart_command_result_t, feedback_revision_le) == 42U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_command_result_t_feedback_time_ms_le_offset[(offsetof(canview_uart_command_result_t, feedback_time_ms_le) == 46U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_command_result_t_canonical_argument_digest_offset[(offsetof(canview_uart_command_result_t, canonical_argument_digest) == 50U) ? 1 : -1];

typedef struct CANVIEW_UART_PACKED {
    uint64_t request_token_le;
    uint8_t action;
    uint8_t reserved0[3U];
    uint32_t requested_ms_le;
    uint64_t lease_id_le;
    uint16_t requested_scope_le;
    uint16_t reserved1_le;
    uint32_t expected_state_revision_le;
    uint32_t control_generation_le;
    uint8_t control_tag[16U];
} canview_uart_control_lease_payload_t;
typedef char canview_uart_assert_canview_uart_control_lease_payload_t_size[(sizeof(canview_uart_control_lease_payload_t) == 52U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_control_lease_payload_t_request_token_le_offset[(offsetof(canview_uart_control_lease_payload_t, request_token_le) == 0U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_control_lease_payload_t_action_offset[(offsetof(canview_uart_control_lease_payload_t, action) == 8U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_control_lease_payload_t_reserved0_offset[(offsetof(canview_uart_control_lease_payload_t, reserved0) == 9U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_control_lease_payload_t_requested_ms_le_offset[(offsetof(canview_uart_control_lease_payload_t, requested_ms_le) == 12U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_control_lease_payload_t_lease_id_le_offset[(offsetof(canview_uart_control_lease_payload_t, lease_id_le) == 16U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_control_lease_payload_t_requested_scope_le_offset[(offsetof(canview_uart_control_lease_payload_t, requested_scope_le) == 24U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_control_lease_payload_t_reserved1_le_offset[(offsetof(canview_uart_control_lease_payload_t, reserved1_le) == 26U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_control_lease_payload_t_expected_state_revision_le_offset[(offsetof(canview_uart_control_lease_payload_t, expected_state_revision_le) == 28U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_control_lease_payload_t_control_generation_le_offset[(offsetof(canview_uart_control_lease_payload_t, control_generation_le) == 32U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_control_lease_payload_t_control_tag_offset[(offsetof(canview_uart_control_lease_payload_t, control_tag) == 36U) ? 1 : -1];

typedef struct CANVIEW_UART_PACKED {
    uint64_t request_token_le;
    uint32_t expected_state_revision_le;
    uint16_t schema_version_le;
    uint16_t reserved0_le;
    uint16_t key_le;
    uint16_t reserved1_le;
    uint32_t known_revision_le;
} canview_uart_config_get_payload_t;
typedef char canview_uart_assert_canview_uart_config_get_payload_t_size[(sizeof(canview_uart_config_get_payload_t) == 24U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_config_get_payload_t_request_token_le_offset[(offsetof(canview_uart_config_get_payload_t, request_token_le) == 0U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_config_get_payload_t_expected_state_revision_le_offset[(offsetof(canview_uart_config_get_payload_t, expected_state_revision_le) == 8U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_config_get_payload_t_schema_version_le_offset[(offsetof(canview_uart_config_get_payload_t, schema_version_le) == 12U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_config_get_payload_t_reserved0_le_offset[(offsetof(canview_uart_config_get_payload_t, reserved0_le) == 14U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_config_get_payload_t_key_le_offset[(offsetof(canview_uart_config_get_payload_t, key_le) == 16U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_config_get_payload_t_reserved1_le_offset[(offsetof(canview_uart_config_get_payload_t, reserved1_le) == 18U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_config_get_payload_t_known_revision_le_offset[(offsetof(canview_uart_config_get_payload_t, known_revision_le) == 20U) ? 1 : -1];

typedef struct CANVIEW_UART_PACKED {
    uint16_t schema_version_le;
    uint8_t count;
    uint8_t reserved0;
} canview_uart_config_batch_prefix_t;
typedef char canview_uart_assert_canview_uart_config_batch_prefix_t_size[(sizeof(canview_uart_config_batch_prefix_t) == 4U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_config_batch_prefix_t_schema_version_le_offset[(offsetof(canview_uart_config_batch_prefix_t, schema_version_le) == 0U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_config_batch_prefix_t_count_offset[(offsetof(canview_uart_config_batch_prefix_t, count) == 2U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_config_batch_prefix_t_reserved0_offset[(offsetof(canview_uart_config_batch_prefix_t, reserved0) == 3U) ? 1 : -1];

typedef struct CANVIEW_UART_PACKED {
    uint16_t key_le;
    uint8_t value_type;
    uint8_t reserved0;
    uint32_t value_bits_le;
} canview_uart_config_record_t;
typedef char canview_uart_assert_canview_uart_config_record_t_size[(sizeof(canview_uart_config_record_t) == 8U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_config_record_t_key_le_offset[(offsetof(canview_uart_config_record_t, key_le) == 0U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_config_record_t_value_type_offset[(offsetof(canview_uart_config_record_t, value_type) == 2U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_config_record_t_reserved0_offset[(offsetof(canview_uart_config_record_t, reserved0) == 3U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_config_record_t_value_bits_le_offset[(offsetof(canview_uart_config_record_t, value_bits_le) == 4U) ? 1 : -1];

typedef struct CANVIEW_UART_PACKED {
    uint64_t request_token_le;
    uint8_t stage;
    uint8_t applied_count;
    uint8_t pending_count;
    uint8_t reserved0;
    uint16_t reason_le;
    uint16_t reserved1_le;
    uint32_t state_revision_le;
    uint32_t completed_time_ms_le;
    uint32_t detail_le;
} canview_uart_config_result_payload_t;
typedef char canview_uart_assert_canview_uart_config_result_payload_t_size[(sizeof(canview_uart_config_result_payload_t) == 28U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_config_result_payload_t_request_token_le_offset[(offsetof(canview_uart_config_result_payload_t, request_token_le) == 0U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_config_result_payload_t_stage_offset[(offsetof(canview_uart_config_result_payload_t, stage) == 8U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_config_result_payload_t_applied_count_offset[(offsetof(canview_uart_config_result_payload_t, applied_count) == 9U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_config_result_payload_t_pending_count_offset[(offsetof(canview_uart_config_result_payload_t, pending_count) == 10U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_config_result_payload_t_reserved0_offset[(offsetof(canview_uart_config_result_payload_t, reserved0) == 11U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_config_result_payload_t_reason_le_offset[(offsetof(canview_uart_config_result_payload_t, reason_le) == 12U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_config_result_payload_t_reserved1_le_offset[(offsetof(canview_uart_config_result_payload_t, reserved1_le) == 14U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_config_result_payload_t_state_revision_le_offset[(offsetof(canview_uart_config_result_payload_t, state_revision_le) == 16U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_config_result_payload_t_completed_time_ms_le_offset[(offsetof(canview_uart_config_result_payload_t, completed_time_ms_le) == 20U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_config_result_payload_t_detail_le_offset[(offsetof(canview_uart_config_result_payload_t, detail_le) == 24U) ? 1 : -1];

typedef struct CANVIEW_UART_PACKED {
    uint64_t boot_id_le;
    uint32_t state_revision_le;
    uint8_t count;
    uint8_t reserved0[3U];
} canview_uart_diagnostic_counters_prefix_t;
typedef char canview_uart_assert_canview_uart_diagnostic_counters_prefix_t_size[(sizeof(canview_uart_diagnostic_counters_prefix_t) == 16U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_diagnostic_counters_prefix_t_boot_id_le_offset[(offsetof(canview_uart_diagnostic_counters_prefix_t, boot_id_le) == 0U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_diagnostic_counters_prefix_t_state_revision_le_offset[(offsetof(canview_uart_diagnostic_counters_prefix_t, state_revision_le) == 8U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_diagnostic_counters_prefix_t_count_offset[(offsetof(canview_uart_diagnostic_counters_prefix_t, count) == 12U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_diagnostic_counters_prefix_t_reserved0_offset[(offsetof(canview_uart_diagnostic_counters_prefix_t, reserved0) == 13U) ? 1 : -1];

typedef struct CANVIEW_UART_PACKED {
    uint16_t counter_id_le;
    uint8_t category;
    uint8_t reserved0;
    uint64_t value_le;
    uint8_t saturated;
    uint8_t reserved1[3U];
} canview_uart_diagnostic_counter_record_t;
typedef char canview_uart_assert_canview_uart_diagnostic_counter_record_t_size[(sizeof(canview_uart_diagnostic_counter_record_t) == 16U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_diagnostic_counter_record_t_counter_id_le_offset[(offsetof(canview_uart_diagnostic_counter_record_t, counter_id_le) == 0U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_diagnostic_counter_record_t_category_offset[(offsetof(canview_uart_diagnostic_counter_record_t, category) == 2U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_diagnostic_counter_record_t_reserved0_offset[(offsetof(canview_uart_diagnostic_counter_record_t, reserved0) == 3U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_diagnostic_counter_record_t_value_le_offset[(offsetof(canview_uart_diagnostic_counter_record_t, value_le) == 4U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_diagnostic_counter_record_t_saturated_offset[(offsetof(canview_uart_diagnostic_counter_record_t, saturated) == 12U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_diagnostic_counter_record_t_reserved1_offset[(offsetof(canview_uart_diagnostic_counter_record_t, reserved1) == 13U) ? 1 : -1];

typedef struct CANVIEW_UART_PACKED {
    uint64_t request_token_le;
    uint8_t target;
    uint8_t action;
    uint16_t reserved0_le;
    uint32_t image_size_le;
    uint8_t image_digest[16U];
    uint32_t requested_generation_le;
} canview_uart_firmware_prepare_payload_t;
typedef char canview_uart_assert_canview_uart_firmware_prepare_payload_t_size[(sizeof(canview_uart_firmware_prepare_payload_t) == 36U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_firmware_prepare_payload_t_request_token_le_offset[(offsetof(canview_uart_firmware_prepare_payload_t, request_token_le) == 0U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_firmware_prepare_payload_t_target_offset[(offsetof(canview_uart_firmware_prepare_payload_t, target) == 8U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_firmware_prepare_payload_t_action_offset[(offsetof(canview_uart_firmware_prepare_payload_t, action) == 9U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_firmware_prepare_payload_t_reserved0_le_offset[(offsetof(canview_uart_firmware_prepare_payload_t, reserved0_le) == 10U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_firmware_prepare_payload_t_image_size_le_offset[(offsetof(canview_uart_firmware_prepare_payload_t, image_size_le) == 12U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_firmware_prepare_payload_t_image_digest_offset[(offsetof(canview_uart_firmware_prepare_payload_t, image_digest) == 16U) ? 1 : -1];
typedef char canview_uart_assert_canview_uart_firmware_prepare_payload_t_requested_generation_le_offset[(offsetof(canview_uart_firmware_prepare_payload_t, requested_generation_le) == 32U) ? 1 : -1];

#if defined(_MSC_VER)
#pragma pack(pop)
#endif

/* The following table is metadata, not a permission grant. Runtime code
 * still applies role, link, safety and lease checks before dispatch. */
static const canview_uart_message_policy_t CANVIEW_UART_MESSAGE_POLICIES[] = {
    {CANVIEW_UART_MSG_LINK_HELLO, 72U, 72U, UINT8_C(0), UINT8_C(0), CANVIEW_UART_PAYLOAD_FIXED, CANVIEW_UART_DIRECTION_BOTH, UINT8_C(1)},
    {CANVIEW_UART_MSG_LINK_HELLO_ACK, 40U, 40U, UINT8_C(2), UINT8_C(2), CANVIEW_UART_PAYLOAD_FIXED, CANVIEW_UART_DIRECTION_BOTH, UINT8_C(1)},
    {CANVIEW_UART_MSG_HEARTBEAT, 48U, 48U, UINT8_C(0), UINT8_C(16), CANVIEW_UART_PAYLOAD_FIXED, CANVIEW_UART_DIRECTION_BOTH, UINT8_C(1)},
    {CANVIEW_UART_MSG_ACK, 20U, 20U, UINT8_C(2), UINT8_C(2), CANVIEW_UART_PAYLOAD_FIXED, CANVIEW_UART_DIRECTION_BOTH, UINT8_C(1)},
    {CANVIEW_UART_MSG_ERROR, 20U, 20U, UINT8_C(4), UINT8_C(6), CANVIEW_UART_PAYLOAD_FIXED, CANVIEW_UART_DIRECTION_BOTH, UINT8_C(1)},
    {CANVIEW_UART_MSG_CAN_RX_BATCH, 12U, 204U, UINT8_C(0), UINT8_C(16), CANVIEW_UART_PAYLOAD_CAN_BATCH, CANVIEW_UART_DIRECTION_STM_TO_ESP, UINT8_C(1)},
    {CANVIEW_UART_MSG_CAN_BUS_STATUS, 28U, 28U, UINT8_C(16), UINT8_C(16), CANVIEW_UART_PAYLOAD_FIXED, CANVIEW_UART_DIRECTION_STM_TO_ESP, UINT8_C(1)},
    {CANVIEW_UART_MSG_SAFETY_SNAPSHOT, 48U, 48U, UINT8_C(16), UINT8_C(16), CANVIEW_UART_PAYLOAD_FIXED, CANVIEW_UART_DIRECTION_STM_TO_ESP, UINT8_C(1)},
    {CANVIEW_UART_MSG_CAN_TX_AUDIT, 40U, 40U, UINT8_C(2), UINT8_C(18), CANVIEW_UART_PAYLOAD_FIXED, CANVIEW_UART_DIRECTION_STM_TO_ESP, UINT8_C(1)},
    {CANVIEW_UART_MSG_CAN_ID_STATS, 8U, 208U, UINT8_C(16), UINT8_C(16), CANVIEW_UART_PAYLOAD_BOUNDED, CANVIEW_UART_DIRECTION_STM_TO_ESP, UINT8_C(1)},
    {CANVIEW_UART_MSG_CAN_OBSERVER_PLAN, 20U, 216U, UINT8_C(1), UINT8_C(9), CANVIEW_UART_PAYLOAD_VARIANTS, CANVIEW_UART_DIRECTION_ESP_TO_STM, UINT8_C(1)},
    {CANVIEW_UART_MSG_CAN_CAPTURE_CONTROL, 28U, 28U, UINT8_C(1), UINT8_C(9), CANVIEW_UART_PAYLOAD_FIXED, CANVIEW_UART_DIRECTION_ESP_TO_STM, UINT8_C(1)},
    {CANVIEW_UART_MSG_CAN_CAPTURE_STATUS, 44U, 44U, UINT8_C(2), UINT8_C(18), CANVIEW_UART_PAYLOAD_FIXED, CANVIEW_UART_DIRECTION_STM_TO_ESP, UINT8_C(1)},
    {CANVIEW_UART_MSG_CAN_EVENT_MARKER, 28U, 28U, UINT8_C(1), UINT8_C(9), CANVIEW_UART_PAYLOAD_FIXED, CANVIEW_UART_DIRECTION_ESP_TO_STM, UINT8_C(1)},
    {CANVIEW_UART_MSG_COMMAND_REQUEST, 104U, 240U, UINT8_C(1), UINT8_C(9), CANVIEW_UART_PAYLOAD_SUFFIX, CANVIEW_UART_DIRECTION_ESP_TO_STM, UINT8_C(1)},
    {CANVIEW_UART_MSG_COMMAND_RESULT, 82U, 82U, UINT8_C(2), UINT8_C(18), CANVIEW_UART_PAYLOAD_FIXED, CANVIEW_UART_DIRECTION_STM_TO_ESP, UINT8_C(1)},
    {CANVIEW_UART_MSG_CONTROL_LEASE, 52U, 52U, UINT8_C(1), UINT8_C(9), CANVIEW_UART_PAYLOAD_FIXED, CANVIEW_UART_DIRECTION_ESP_TO_STM, UINT8_C(1)},
    {CANVIEW_UART_MSG_CONFIG_GET, 24U, 24U, UINT8_C(1), UINT8_C(9), CANVIEW_UART_PAYLOAD_FIXED, CANVIEW_UART_DIRECTION_ESP_TO_STM, UINT8_C(1)},
    {CANVIEW_UART_MSG_CONFIG_SET, 4U, 204U, UINT8_C(1), UINT8_C(9), CANVIEW_UART_PAYLOAD_BOUNDED, CANVIEW_UART_DIRECTION_ESP_TO_STM, UINT8_C(1)},
    {CANVIEW_UART_MSG_CONFIG_RESULT, 28U, 28U, UINT8_C(2), UINT8_C(18), CANVIEW_UART_PAYLOAD_FIXED, CANVIEW_UART_DIRECTION_STM_TO_ESP, UINT8_C(1)},
    {CANVIEW_UART_MSG_DIAGNOSTIC_COUNTERS, 16U, 208U, UINT8_C(0), UINT8_C(16), CANVIEW_UART_PAYLOAD_BOUNDED, CANVIEW_UART_DIRECTION_BOTH, UINT8_C(1)},
    {CANVIEW_UART_MSG_FIRMWARE_PREPARE, 36U, 36U, UINT8_C(1), UINT8_C(9), CANVIEW_UART_PAYLOAD_FIXED, CANVIEW_UART_DIRECTION_ESP_TO_STM, UINT8_C(0)},
};
#define CANVIEW_UART_MESSAGE_POLICY_COUNT (sizeof(CANVIEW_UART_MESSAGE_POLICIES) / sizeof(CANVIEW_UART_MESSAGE_POLICIES[0]))

typedef char canview_uart_assert_protocol_max[
    (CANVIEW_UART_MAX_PAYLOAD_SIZE + CANVIEW_UART_HEADER_SIZE == CANVIEW_UART_MAX_FRAME_SIZE) ? 1 : -1];
typedef char canview_uart_assert_protocol_encoded[
    (CANVIEW_UART_MAX_ENCODED_SIZE == 1029U) ? 1 : -1];

#ifdef __cplusplus
}
#endif
#endif
