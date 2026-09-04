/*
 * canview ESP-NOW wire protocol v1.2 INCOMPLETE DRAFT
 *
 * This header is not a deployable runtime ABI.  It is replaced by the
 * schema-generated v1.3 header in task T-002 before transport integration.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * This file defines byte layouts only. Multi-byte fields are little-endian on
 * the wire. Do not cast untrusted receive buffers directly to these structs;
 * validate length/version/CRC first, then decode with explicit LE helpers.
 */
#ifndef CANVIEW_PROTOCOL_H
#define CANVIEW_PROTOCOL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CANVIEW_PROTOCOL_MAJOR UINT8_C(1)
#define CANVIEW_PROTOCOL_MINOR UINT8_C(2)
#define CANVIEW_HEADER_SIZE UINT8_C(32)
#define CANVIEW_MAX_FRAME_SIZE UINT16_C(240)
#define CANVIEW_MAX_PAYLOAD_SIZE UINT16_C(208)
#define CANVIEW_MAGIC_LE UINT16_C(0x5643) /* Wire bytes 0x43, 0x56: "CV". */

/* Controller-side raw CAN admission limits. The allow-list is deliberately
 * enforced after reception so a new DBC catalog never requires a
 * Communicator firmware change. */
#define CANVIEW_CAN_BUS_ANY UINT8_C(0xFF)
#define CANVIEW_CAN_STANDARD_ID_MAX UINT32_C(0x7FF)
#define CANVIEW_CAN_EXTENDED_ID_MAX UINT32_C(0x1FFFFFFF)
#define CANVIEW_CAN_FILTER_MAX_COUNT UINT8_C(32)
#define CANVIEW_CAN_FILTER_MAX_BATCH_COUNT UINT8_C(8)
#define CANVIEW_CAN_FILTER_MIN_PERIOD_MS UINT16_C(20)
#define CANVIEW_CAN_FILTER_MAX_PERIOD_MS UINT16_C(60000)
#define CANVIEW_CAN_FILTER_MAX_RECORDS_PER_PERIOD UINT8_C(32)
#define CANVIEW_CAN_RX_DEFAULT_BYTES_PER_SECOND UINT32_C(20000)
#define CANVIEW_CAN_RX_MAX_BYTES_PER_SECOND UINT32_C(20000)
#define CANVIEW_CAN_RECORD_WIRE_BYTES UINT16_C(16)
#define CANVIEW_COMMAND_TRACKER_MAX_PENDING UINT8_C(8)
#define CANVIEW_COMMAND_TRACKER_MAX_RETRIES UINT8_C(2)

#if defined(__GNUC__) || defined(__clang__)
#define CANVIEW_PACKED __attribute__((packed))
#else
#define CANVIEW_PACKED
#endif

typedef enum {
    CANVIEW_MSG_DISCOVERY = 0x01,
    CANVIEW_MSG_PAIR_REQUEST = 0x02,
    CANVIEW_MSG_PAIR_CHALLENGE = 0x03,
    CANVIEW_MSG_PAIR_CONFIRM = 0x04,
    CANVIEW_MSG_PAIR_RESULT = 0x05,

    CANVIEW_MSG_HELLO = 0x10,
    CANVIEW_MSG_CAPABILITIES = 0x11,
    CANVIEW_MSG_TIME_SYNC_REQUEST = 0x12,
    CANVIEW_MSG_TIME_SYNC_RESPONSE = 0x13,
    CANVIEW_MSG_HEARTBEAT = 0x14,
    CANVIEW_MSG_ACK = 0x15,
    CANVIEW_MSG_ERROR = 0x16,
    CANVIEW_MSG_STATE_SNAPSHOT_REQUEST = 0x17,
    CANVIEW_MSG_STATE_SNAPSHOT = 0x18,

    CANVIEW_MSG_CAN_BATCH = 0x20,
    CANVIEW_MSG_SIGNAL_BATCH = 0x21,
    CANVIEW_MSG_BUS_STATUS = 0x22,
    CANVIEW_MSG_CAN_FILTER_GET = 0x23,
    CANVIEW_MSG_CAN_FILTER_SET = 0x24,
    CANVIEW_MSG_CAN_FILTER_RESULT = 0x25,
    CANVIEW_MSG_CAN_STREAM_CONFIG = 0x26,
    CANVIEW_MSG_CAN_STREAM_STATUS = 0x27,

    CANVIEW_MSG_COMMAND_REQUEST = 0x30,
    CANVIEW_MSG_COMMAND_RESULT = 0x31,
    CANVIEW_MSG_CONTROL_LEASE_REQUEST = 0x32,
    CANVIEW_MSG_CONTROL_LEASE_STATUS = 0x33,

    CANVIEW_MSG_CONFIG_GET = 0x40,
    CANVIEW_MSG_CONFIG_SET = 0x41,
    CANVIEW_MSG_CONFIG_RESULT = 0x42,
    CANVIEW_MSG_DIAGNOSTIC_COUNTERS = 0x50,

    CANVIEW_MSG_BULK_BEGIN = 0x60,
    CANVIEW_MSG_BULK_FRAGMENT = 0x61,
    CANVIEW_MSG_BULK_ACK = 0x62,
    CANVIEW_MSG_BULK_END = 0x63,
} canview_message_type_t;

typedef enum {
    CANVIEW_FLAG_ACK_REQUIRED = 1u << 0,
    CANVIEW_FLAG_RESPONSE = 1u << 1,
    CANVIEW_FLAG_ERROR = 1u << 2,
    CANVIEW_FLAG_FRAGMENT = 1u << 3,
    CANVIEW_FLAG_LAST_FRAGMENT = 1u << 4,
    CANVIEW_FLAG_BROADCAST = 1u << 5,
    CANVIEW_FLAG_READ_ONLY = 1u << 6,
} canview_frame_flag_t;

typedef enum {
    CANVIEW_PRIORITY_LINK_SAFETY = 0,
    CANVIEW_PRIORITY_COMMAND = 1,
    CANVIEW_PRIORITY_CRITICAL_EVENT = 2,
    CANVIEW_PRIORITY_TELEMETRY = 3,
    CANVIEW_PRIORITY_BULK = 4,
} canview_priority_t;

typedef enum {
    CANVIEW_ACK_ACCEPTED = 0,
    CANVIEW_ACK_DUPLICATE = 1,
    CANVIEW_ACK_BUSY = 2,
    CANVIEW_ACK_MALFORMED = 3,
    CANVIEW_ACK_UNSUPPORTED = 4,
    CANVIEW_ACK_UNAUTHENTICATED = 5,
    CANVIEW_ACK_EXPIRED = 6,
} canview_ack_status_t;

typedef enum {
    CANVIEW_COMMAND_ACCEPTED = 0,
    CANVIEW_COMMAND_EXECUTING = 1,
    CANVIEW_COMMAND_COMPLETED = 2,
    CANVIEW_COMMAND_REJECTED = 3,
    CANVIEW_COMMAND_EXPIRED = 4,
    CANVIEW_COMMAND_CANCELLED = 5,
    CANVIEW_COMMAND_FAILED = 6,
} canview_command_stage_t;

typedef enum {
    CANVIEW_QUALITY_VALID = 0,
    CANVIEW_QUALITY_STALE = 1,
    CANVIEW_QUALITY_UNAVAILABLE = 2,
    CANVIEW_QUALITY_UNVERIFIED = 3,
    CANVIEW_QUALITY_OUT_OF_RANGE = 4,
    CANVIEW_QUALITY_FAULT = 5,
} canview_signal_quality_t;

typedef enum {
    CANVIEW_VALUE_BOOL = 0,
    CANVIEW_VALUE_U32 = 1,
    CANVIEW_VALUE_I32 = 2,
    CANVIEW_VALUE_F32 = 3,
    CANVIEW_VALUE_ENUM = 4,
    CANVIEW_VALUE_BITSET = 5,
} canview_value_type_t;

typedef enum {
    CANVIEW_BUS_FLAG_EXTENDED_ID = 1u << 0,
    CANVIEW_BUS_FLAG_REMOTE_FRAME = 1u << 1,
    CANVIEW_BUS_FLAG_ERROR_FRAME = 1u << 2,
    CANVIEW_BUS_FLAG_TX_ECHO = 1u << 3,
} canview_can_flag_t;

typedef enum {
    CANVIEW_PRECOND_IGNITION_ON = 1u << 0,
    CANVIEW_PRECOND_ENGINE_RUNNING = 1u << 1,
    CANVIEW_PRECOND_FORWARD_GEAR = 1u << 2,
    CANVIEW_PRECOND_NOT_BRAKING = 1u << 3,
    CANVIEW_PRECOND_ABS_ESC_INACTIVE = 1u << 4,
    CANVIEW_PRECOND_SIGNALS_FRESH = 1u << 5,
    CANVIEW_PRECOND_CONTROL_LEASE = 1u << 6,
    CANVIEW_PRECOND_VEHICLE_STOPPED = 1u << 7,
} canview_precondition_flag_t;

/* Negotiated Primary Controller control scopes.  The wire capability is a
 * little-endian 16-bit bitset.  Read-only Controllers and Diagnostic Bridges
 * must advertise zero.  AUDIO_PROFILE_SET additionally requires every scope
 * used by the selected vehicle-profile transaction. */
typedef enum {
    CANVIEW_CONTROL_SCOPE_AUDIO_PROFILE = 1u << 0,
    CANVIEW_CONTROL_SCOPE_AUDIO_VOLUME_OFFSET = 1u << 1,
    CANVIEW_CONTROL_SCOPE_AUDIO_FADER = 1u << 2,
    CANVIEW_CONTROL_SCOPE_AUDIO_BALANCE = 1u << 3,
    CANVIEW_CONTROL_SCOPE_AUDIO_MUTE = 1u << 4,
    CANVIEW_CONTROL_SCOPE_AUDIO_REAR_MUTE = 1u << 5,
    CANVIEW_CONTROL_SCOPE_AUDIO_SDVC = 1u << 6,
    CANVIEW_CONTROL_SCOPE_AUDIO_RESTORE = 1u << 7,
    CANVIEW_CONTROL_SCOPE_DRIVE_MODE_PULSE = 1u << 8,
    CANVIEW_CONTROL_SCOPE_ADAPTIVE_VOLUME_AUTOMATION = 1u << 9,
    CANVIEW_CONTROL_SCOPE_AUTO_SPORT_AUTOMATION = 1u << 10,
} canview_control_scope_t;

#define CANVIEW_CONTROL_SCOPE_KNOWN_MASK UINT16_C(0x07FF)

typedef enum {
    CANVIEW_COMMAND_AUDIO_PROFILE_SET = 0x0101,
    CANVIEW_COMMAND_AUDIO_VOLUME_OFFSET_SET = 0x0102,
    CANVIEW_COMMAND_AUDIO_RESTORE_SNAPSHOT = 0x0103,
    CANVIEW_COMMAND_DRIVE_MODE_BUTTON_PULSE = 0x0201,
    CANVIEW_COMMAND_AUTOMATION_ARM = 0x0202,
    CANVIEW_COMMAND_AUTOMATION_DISARM = 0x0203,
    CANVIEW_COMMAND_RTC_SET_LOCAL_TIME = 0x0301,
} canview_command_id_t;

typedef enum {
    CANVIEW_AUTOMATION_ADAPTIVE_VOLUME = 1,
    CANVIEW_AUTOMATION_AUTO_SPORT = 2,
} canview_automation_id_t;

typedef enum {
    CANVIEW_CONFIG_SPORT_AUTOMATION_ENABLED = 0x0201,
    CANVIEW_CONFIG_SPORT_ENTRY_SPEED_TENTH_KPH = 0x0202,
    CANVIEW_CONFIG_SPORT_ACCELERATION_ENABLED = 0x0203,
    CANVIEW_CONFIG_RTC_LOCAL_TIME = 0x0301,
    CANVIEW_CONFIG_SUNRISE_MINUTES = 0x0302,
    CANVIEW_CONFIG_SUNSET_MINUTES = 0x0303,
    CANVIEW_CONFIG_HEADLAMP_WARNING_ENABLED = 0x0304,
    CANVIEW_CONFIG_CAN_RX_STREAM_PERIOD_MS = 0x0401,
    CANVIEW_CONFIG_CAN_RX_STREAM_MAX_RECORDS = 0x0402,
    CANVIEW_CONFIG_CAN_RX_BYTES_PER_SECOND = 0x0403,
} canview_config_key_t;

typedef enum {
    CANVIEW_CAN_FILTER_ADD = 1,
    CANVIEW_CAN_FILTER_REPLACE = 2,
    CANVIEW_CAN_FILTER_DELETE = 3,
    CANVIEW_CAN_FILTER_CLEAR = 4,
} canview_can_filter_action_t;

typedef enum {
    CANVIEW_CAN_FILTER_APPLIED = 0,
    CANVIEW_CAN_FILTER_INVALID = 1,
    CANVIEW_CAN_FILTER_FULL = 2,
    CANVIEW_CAN_FILTER_NOT_FOUND = 3,
    CANVIEW_CAN_FILTER_CONFLICT = 4,
} canview_can_filter_result_t;

typedef enum {
    CANVIEW_ERROR_TRANSPORT_BASE = 0x0100,
    CANVIEW_ERROR_PROTOCOL_BASE = 0x0200,
    CANVIEW_ERROR_AUTH_BASE = 0x0300,
    CANVIEW_ERROR_COMPAT_BASE = 0x0400,
    CANVIEW_ERROR_RESOURCE_BASE = 0x0500,
    CANVIEW_ERROR_APPLICATION_BASE = 0x0600,
    CANVIEW_ERROR_SAFETY_BASE = 0x0700,

    CANVIEW_ERROR_BAD_LENGTH = 0x0201,
    CANVIEW_ERROR_BAD_MAGIC = 0x0202,
    CANVIEW_ERROR_BAD_CRC = 0x0203,
    CANVIEW_ERROR_BAD_RESERVED = 0x0204,
    CANVIEW_ERROR_UNSUPPORTED_MESSAGE = 0x0401,
    CANVIEW_ERROR_INCOMPATIBLE_MAJOR = 0x0402,
    CANVIEW_ERROR_QUEUE_FULL = 0x0501,
    CANVIEW_ERROR_AUTH_FAILED = 0x0301,
    CANVIEW_ERROR_SESSION_MISMATCH = 0x0302,
    CANVIEW_ERROR_LEASE_REQUIRED = 0x0701,
    CANVIEW_ERROR_PRECONDITION_FAILED = 0x0702,
    CANVIEW_ERROR_SIGNAL_STALE = 0x0703,
    CANVIEW_ERROR_TX_DISABLED = 0x0704,
} canview_error_code_t;

typedef struct CANVIEW_PACKED {
    uint16_t magic_le;
    uint8_t major;
    uint8_t minor;
    uint8_t header_len;
    uint8_t message_type;
    uint8_t flags;
    uint8_t priority;
    uint32_t session_id_le;
    uint32_t sequence_le;
    uint32_t sender_time_ms_le;
    uint32_t correlation_id_le;
    uint16_t payload_len_le;
    uint16_t reserved_le;
    uint32_t crc32_le;
} canview_frame_header_t;

typedef struct CANVIEW_PACKED {
    uint16_t type_le;
    uint16_t length_le;
} canview_tlv_header_t;

typedef struct CANVIEW_PACKED {
    uint32_t acknowledged_sequence_le;
    uint16_t status_le;
    uint16_t detail_le;
    uint32_t receiver_time_ms_le;
} canview_ack_payload_t;

typedef struct CANVIEW_PACKED {
    uint64_t base_time_us_le;
    uint8_t count;
    uint8_t dropped_since_last;
    uint16_t reserved_le;
} canview_can_batch_header_t;

typedef struct CANVIEW_PACKED {
    uint16_t delta_us_le;
    uint8_t bus_id;
    uint8_t flags_dlc; /* High nibble flags, low nibble DLC (0..8). */
    uint32_t can_id_le;
    uint8_t data[8];
} canview_can_record_t;

/* A Controller allow-list entry. Matching is
 * (arbitration_id & can_id_mask) == (can_id & can_id_mask). A non-zero mask,
 * valid DLC range, and bounded period/count are mandatory. */
typedef struct CANVIEW_PACKED {
    uint32_t filter_id_le;
    uint32_t can_id_le;
    uint32_t can_id_mask_le;
    uint16_t period_ms_le;
    uint8_t bus_id;
    uint8_t flags_value;
    uint8_t flags_mask;
    uint8_t min_dlc;
    uint8_t max_dlc;
    uint8_t max_records_per_period;
    uint8_t enabled;
    uint8_t reserved0;
} canview_can_filter_t;

typedef struct CANVIEW_PACKED {
    uint8_t action;
    uint8_t count;
    uint16_t reserved_le;
    uint32_t config_revision_le;
} canview_can_filter_batch_header_t;

typedef struct CANVIEW_PACKED {
    uint32_t config_revision_le;
    uint32_t filter_id_le; /* 0 requests the complete snapshot. */
} canview_can_filter_get_payload_t;

typedef struct CANVIEW_PACKED {
    uint32_t config_revision_le;
    uint32_t filter_id_le;
    uint8_t action;
    uint8_t result;
    uint16_t detail_le;
} canview_can_filter_result_payload_t;

/* Global stream budget complements per-filter period/count. It is applied at
 * the Controller receive boundary and is not a request to transmit arbitrary
 * CAN frames from the Communicator. */
typedef struct CANVIEW_PACKED {
    uint32_t config_revision_le;
    uint16_t period_ms_le;
    uint8_t max_records_per_period;
    uint8_t enabled;
    uint32_t max_bytes_per_second_le;
    uint16_t burst_bytes_le; /* Maximum raw bytes in one period window. */
    uint16_t reserved_le;
} canview_can_stream_config_t;

typedef struct CANVIEW_PACKED {
    uint32_t config_revision_le;
    uint16_t accepted_records_le;
    uint16_t rejected_records_le;
    uint32_t dropped_by_budget_le;
    uint32_t reserved_le;
} canview_can_stream_status_t;

typedef struct CANVIEW_PACKED {
    uint64_t sample_time_us_le;
    uint8_t count;
    uint8_t catalog_revision;
    uint16_t reserved_le;
} canview_signal_batch_header_t;

typedef struct CANVIEW_PACKED {
    uint16_t signal_id_le;
    uint8_t value_type;
    uint8_t quality;
    uint16_t age_ms_le;
    uint16_t reserved_le;
    uint32_t value_bits_le;
} canview_signal_record_t;

typedef struct CANVIEW_PACKED {
    uint64_t request_token_le;
    uint16_t command_id_le;
    uint16_t ttl_ms_le;
    uint32_t expected_state_revision_le;
    uint32_t precondition_flags_le;
    uint16_t argument_tlv_length_le;
    uint16_t reserved_le;
} canview_command_request_t;

typedef struct CANVIEW_PACKED {
    uint64_t request_token_le;
    uint16_t command_id_le;
    uint8_t stage;
    uint8_t reason;
    uint32_t state_revision_le;
    uint32_t completed_time_ms_le;
} canview_command_result_t;

/* The UI sets hour/minute while the Controller preserves the currently known
 * calendar date. A service/host may populate the complete date fields later. */
typedef struct CANVIEW_PACKED {
    uint16_t year_le;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t weekday;
    uint8_t reserved[4];
} canview_rtc_time_payload_t;

typedef struct CANVIEW_PACKED {
    uint16_t schema_version_le;
    uint8_t count;
    uint8_t reserved;
} canview_config_batch_header_t;

typedef struct CANVIEW_PACKED {
    uint16_t key_le;
    uint8_t value_type;
    uint8_t reserved;
    uint32_t value_bits_le;
} canview_config_record_t;

typedef struct CANVIEW_PACKED {
    uint16_t code_le;
    uint8_t severity;
    uint8_t origin;
    uint8_t offending_message_type;
    uint8_t reserved0;
    uint16_t detail_le;
    uint32_t offending_sequence_le;
    uint32_t retry_after_ms_le;
} canview_error_payload_t;

typedef struct CANVIEW_PACKED {
    uint64_t boot_id_le;
    uint32_t state_revision_le;
    uint32_t uptime_ms_le;
    uint16_t heartbeat_interval_ms_le;
    int8_t last_rssi_dbm;
    uint8_t link_state;
    uint8_t active_bus_mask;
    uint8_t bus_error_mask;
    uint16_t rx_queue_depth_le;
    uint16_t telemetry_dropped_le;
    uint16_t protocol_error_count_le;
    uint16_t reserved_le;
} canview_heartbeat_payload_t;

#if defined(__cplusplus)
static_assert(sizeof(canview_frame_header_t) == 32, "wire header must be 32 bytes");
static_assert(sizeof(canview_can_record_t) == 16, "CAN record must be 16 bytes");
static_assert(sizeof(canview_can_filter_t) == 22, "CAN filter must be 22 bytes");
static_assert(sizeof(canview_can_filter_batch_header_t) == 8,
              "CAN filter batch header must be 8 bytes");
static_assert(sizeof(canview_can_filter_get_payload_t) == 8,
              "CAN filter get payload must be 8 bytes");
static_assert(sizeof(canview_can_filter_result_payload_t) == 12,
              "CAN filter result must be 12 bytes");
static_assert(sizeof(canview_can_stream_config_t) == 16,
              "CAN stream config must be 16 bytes");
static_assert(sizeof(canview_can_stream_status_t) == 16,
              "CAN stream status must be 16 bytes");
static_assert(sizeof(canview_rtc_time_payload_t) == 12,
              "RTC payload must be 12 bytes");
static_assert(sizeof(canview_signal_record_t) == 12, "signal record must be 12 bytes");
static_assert(sizeof(canview_config_record_t) == 8, "config record must be 8 bytes");
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(canview_frame_header_t) == 32, "wire header must be 32 bytes");
_Static_assert(sizeof(canview_can_record_t) == 16, "CAN record must be 16 bytes");
_Static_assert(sizeof(canview_can_filter_t) == 22, "CAN filter must be 22 bytes");
_Static_assert(sizeof(canview_can_filter_batch_header_t) == 8,
               "CAN filter batch header must be 8 bytes");
_Static_assert(sizeof(canview_can_filter_get_payload_t) == 8,
               "CAN filter get payload must be 8 bytes");
_Static_assert(sizeof(canview_can_filter_result_payload_t) == 12,
               "CAN filter result must be 12 bytes");
_Static_assert(sizeof(canview_can_stream_config_t) == 16,
               "CAN stream config must be 16 bytes");
_Static_assert(sizeof(canview_can_stream_status_t) == 16,
               "CAN stream status must be 16 bytes");
_Static_assert(sizeof(canview_rtc_time_payload_t) == 12,
               "RTC payload must be 12 bytes");
_Static_assert(sizeof(canview_signal_record_t) == 12, "signal record must be 12 bytes");
_Static_assert(sizeof(canview_config_record_t) == 8, "config record must be 8 bytes");
#endif

#ifdef __cplusplus
}
#endif

#endif /* CANVIEW_PROTOCOL_H */
