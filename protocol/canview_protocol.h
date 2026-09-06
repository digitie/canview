/*
 * GENERATED FILE - DO NOT EDIT.
 * Source: protocol/schema/espnow-v1.3.yaml
 * Regenerate with: python tools/generate_protocol.py
 * SPDX-License-Identifier: GPL-3.0-only
 */
#ifndef CANVIEW_PROTOCOL_H
#define CANVIEW_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#define CANVIEW_PROTOCOL_SCHEMA_SHA256 "f65b6167698be2811b1bc80ddc28e1128e550ace1a256fc7c7a714c8a090c6f1"
#define CANVIEW_PROTOCOL_WIRE_NAME "ESP-NOW v1.3"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER)
#define CANVIEW_PACKED
#pragma pack(push, 1)
#elif defined(__GNUC__) || defined(__clang__)
#define CANVIEW_PACKED __attribute__((packed))
#else
#define CANVIEW_PACKED
#endif

#define CANVIEW_PROTOCOL_MAJOR UINT8_C(1)
#define CANVIEW_PROTOCOL_MINOR UINT8_C(3)
#define CANVIEW_HEADER_SIZE UINT8_C(0x20)
#define CANVIEW_MAX_FRAME_SIZE UINT16_C(0xf0)
#define CANVIEW_MAX_PAYLOAD_SIZE UINT16_C(0xd0)
#define CANVIEW_MAGIC_LE UINT16_C(0x5643)
#define CANVIEW_CAN_BUS_ANY UINT8_C(0xff)
#define CANVIEW_CAN_STANDARD_ID_MAX UINT32_C(0x7ff)
#define CANVIEW_CAN_EXTENDED_ID_MAX UINT32_C(0x1fffffff)
#define CANVIEW_CAN_FILTER_MAX_COUNT UINT8_C(0x20)
#define CANVIEW_CAN_FILTER_MAX_BATCH_COUNT UINT8_C(8)
#define CANVIEW_CAN_FILTER_MIN_PERIOD_MS UINT16_C(0x14)
#define CANVIEW_CAN_FILTER_MAX_PERIOD_MS UINT16_C(0xea60)
#define CANVIEW_CAN_FILTER_MAX_RECORDS_PER_PERIOD UINT8_C(0x20)
#define CANVIEW_CAN_RX_DEFAULT_BYTES_PER_SECOND UINT32_C(0x4e20)
#define CANVIEW_CAN_RX_MAX_BYTES_PER_SECOND UINT32_C(0x4e20)
#define CANVIEW_CAN_RECORD_WIRE_BYTES UINT16_C(0x10)
#define CANVIEW_COMMAND_TRACKER_MAX_PENDING UINT8_C(8)
#define CANVIEW_COMMAND_TRACKER_MAX_RETRIES UINT8_C(2)
#define CANVIEW_CONTROL_SCOPE_KNOWN_MASK UINT16_C(0x7ff)
#define CANVIEW_OBSERVER_MAX_RECORDS_PER_SECOND UINT16_C(0xc8)
#define CANVIEW_BULK_MAX_OBJECT_SIZE UINT32_C(0x10000)
#define CANVIEW_BULK_MAX_FRAGMENT_SIZE UINT16_C(0xc0)
#define CANVIEW_BULK_WINDOW_SIZE UINT8_C(4)
#define CANVIEW_COMMAND_TTL_MIN_MS UINT16_C(0x1f4)
#define CANVIEW_COMMAND_TTL_MAX_MS UINT16_C(0x7530)
#define CANVIEW_DIAGNOSTIC_LEASE_DEFAULT_MS UINT32_C(0x7530)
#define CANVIEW_CONFIG_MAX_RECORDS UINT8_C(0x19)
#define CANVIEW_CONFIG_SCHEMA_MAX_BYTES UINT32_C(0x4000)
#define CANVIEW_PROTOCOL_VERSION UINT16_C(0x0103)
#define CANVIEW_PROTOCOL_KNOWN_FLAG_MASK UINT8_C(0x7F)

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
    CANVIEW_MSG_CAN_OBSERVER_CONFIG = 0x28,
    CANVIEW_MSG_CAN_ID_STATS = 0x29,
    CANVIEW_MSG_CAN_CAPTURE_CONTROL = 0x2A,
    CANVIEW_MSG_CAN_CAPTURE_STATUS = 0x2B,
    CANVIEW_MSG_CAN_EVENT_MARKER = 0x2C,
    CANVIEW_MSG_DIAGNOSTIC_LEASE = 0x2D,
    CANVIEW_MSG_COMMAND_REQUEST = 0x30,
    CANVIEW_MSG_COMMAND_RESULT = 0x31,
    CANVIEW_MSG_CONTROL_LEASE_REQUEST = 0x32,
    CANVIEW_MSG_CONTROL_LEASE_STATUS = 0x33,
    CANVIEW_MSG_CONFIG_GET = 0x40,
    CANVIEW_MSG_CONFIG_SET = 0x41,
    CANVIEW_MSG_CONFIG_RESULT = 0x42,
    CANVIEW_MSG_CONFIG_SCHEMA_REQUEST = 0x43,
    CANVIEW_MSG_REMOTE_CONFIG_REQUEST = 0x44,
    CANVIEW_MSG_REMOTE_CONFIG_STATUS = 0x45,
    CANVIEW_MSG_DIAGNOSTIC_COUNTERS = 0x50,
    CANVIEW_MSG_BULK_BEGIN = 0x60,
    CANVIEW_MSG_BULK_FRAGMENT = 0x61,
    CANVIEW_MSG_BULK_ACK = 0x62,
    CANVIEW_MSG_BULK_END = 0x63,
} canview_message_type_t;
#define CANVIEW_MESSAGE_COUNT UINT8_C(43)

typedef enum {
    CANVIEW_ROLE_COMMUNICATOR = UINT32_C(0),
    CANVIEW_ROLE_PRIMARY_CONTROLLER = UINT32_C(1),
    CANVIEW_ROLE_READ_ONLY_CONTROLLER = UINT32_C(2),
    CANVIEW_ROLE_DIAGNOSTIC_BRIDGE = UINT32_C(3),
} canview_role_t;

typedef enum {
    CANVIEW_LINK_DISABLED = UINT32_C(0),
    CANVIEW_LINK_RADIO_INIT = UINT32_C(1),
    CANVIEW_LINK_PEER_RESTORE = UINT32_C(2),
    CANVIEW_LINK_DISCOVERING = UINT32_C(3),
    CANVIEW_LINK_PAIRING = UINT32_C(4),
    CANVIEW_LINK_SECURE_HELLO = UINT32_C(5),
    CANVIEW_LINK_NEGOTIATING = UINT32_C(6),
    CANVIEW_LINK_TIME_SYNC = UINT32_C(7),
    CANVIEW_LINK_STATE_SYNC = UINT32_C(8),
    CANVIEW_LINK_ONLINE = UINT32_C(9),
    CANVIEW_LINK_DEGRADED = UINT32_C(0xa),
    CANVIEW_LINK_RECOVERING = UINT32_C(0xb),
    CANVIEW_LINK_AUTH_BACKOFF = UINT32_C(0xc),
    CANVIEW_LINK_READ_ONLY_INCOMPATIBLE = UINT32_C(0xd),
} canview_link_state_t;

typedef enum {
    CANVIEW_FLAG_ACK_REQUIRED = UINT32_C(1),
    CANVIEW_FLAG_RESPONSE = UINT32_C(2),
    CANVIEW_FLAG_ERROR = UINT32_C(4),
    CANVIEW_FLAG_FRAGMENT = UINT32_C(8),
    CANVIEW_FLAG_LAST_FRAGMENT = UINT32_C(0x10),
    CANVIEW_FLAG_BROADCAST = UINT32_C(0x20),
    CANVIEW_FLAG_READ_ONLY = UINT32_C(0x40),
} canview_frame_flag_t;

typedef enum {
    CANVIEW_PRIORITY_LINK_SAFETY = UINT32_C(0),
    CANVIEW_PRIORITY_COMMAND = UINT32_C(1),
    CANVIEW_PRIORITY_CRITICAL_EVENT = UINT32_C(2),
    CANVIEW_PRIORITY_TELEMETRY = UINT32_C(3),
    CANVIEW_PRIORITY_BULK = UINT32_C(4),
} canview_priority_t;

typedef enum {
    CANVIEW_ACK_ACCEPTED = UINT32_C(0),
    CANVIEW_ACK_DUPLICATE = UINT32_C(1),
    CANVIEW_ACK_BUSY = UINT32_C(2),
    CANVIEW_ACK_MALFORMED = UINT32_C(3),
    CANVIEW_ACK_UNSUPPORTED = UINT32_C(4),
    CANVIEW_ACK_UNAUTHENTICATED = UINT32_C(5),
    CANVIEW_ACK_EXPIRED = UINT32_C(6),
} canview_ack_status_t;

typedef enum {
    CANVIEW_COMMAND_ACCEPTED = UINT32_C(0),
    CANVIEW_COMMAND_EXECUTING = UINT32_C(1),
    CANVIEW_COMMAND_COMPLETED = UINT32_C(2),
    CANVIEW_COMMAND_REJECTED = UINT32_C(3),
    CANVIEW_COMMAND_EXPIRED = UINT32_C(4),
    CANVIEW_COMMAND_CANCELLED = UINT32_C(5),
    CANVIEW_COMMAND_FAILED = UINT32_C(6),
} canview_command_stage_t;

typedef enum {
    CANVIEW_QUALITY_VALID = UINT32_C(0),
    CANVIEW_QUALITY_STALE = UINT32_C(1),
    CANVIEW_QUALITY_UNAVAILABLE = UINT32_C(2),
    CANVIEW_QUALITY_UNVERIFIED = UINT32_C(3),
    CANVIEW_QUALITY_OUT_OF_RANGE = UINT32_C(4),
    CANVIEW_QUALITY_FAULT = UINT32_C(5),
} canview_signal_quality_t;

typedef enum {
    CANVIEW_EVIDENCE_UNKNOWN = UINT32_C(0),
    CANVIEW_EVIDENCE_CANDIDATE = UINT32_C(1),
    CANVIEW_EVIDENCE_OBSERVED = UINT32_C(2),
    CANVIEW_EVIDENCE_VERIFIED = UINT32_C(3),
} canview_evidence_grade_t;

typedef enum {
    CANVIEW_VALUE_BOOL = UINT32_C(0),
    CANVIEW_VALUE_U32 = UINT32_C(1),
    CANVIEW_VALUE_I32 = UINT32_C(2),
    CANVIEW_VALUE_F32 = UINT32_C(3),
    CANVIEW_VALUE_ENUM = UINT32_C(4),
    CANVIEW_VALUE_BITSET = UINT32_C(5),
} canview_value_type_t;

typedef enum {
    CANVIEW_BUS_FLAG_EXTENDED_ID = UINT32_C(1),
    CANVIEW_BUS_FLAG_REMOTE_FRAME = UINT32_C(2),
    CANVIEW_BUS_FLAG_ERROR_FRAME = UINT32_C(4),
    CANVIEW_BUS_FLAG_TX_ECHO = UINT32_C(8),
} canview_can_flag_t;

typedef enum {
    CANVIEW_PRECOND_IGNITION_ON = UINT32_C(1),
    CANVIEW_PRECOND_ENGINE_RUNNING = UINT32_C(2),
    CANVIEW_PRECOND_FORWARD_GEAR = UINT32_C(4),
    CANVIEW_PRECOND_NOT_BRAKING = UINT32_C(8),
    CANVIEW_PRECOND_ABS_ESC_INACTIVE = UINT32_C(0x10),
    CANVIEW_PRECOND_SIGNALS_FRESH = UINT32_C(0x20),
    CANVIEW_PRECOND_CONTROL_LEASE = UINT32_C(0x40),
    CANVIEW_PRECOND_VEHICLE_STOPPED = UINT32_C(0x80),
} canview_precondition_flag_t;

typedef enum {
    CANVIEW_CONTROL_SCOPE_AUDIO_PROFILE = UINT32_C(1),
    CANVIEW_CONTROL_SCOPE_AUDIO_VOLUME_OFFSET = UINT32_C(2),
    CANVIEW_CONTROL_SCOPE_AUDIO_FADER = UINT32_C(4),
    CANVIEW_CONTROL_SCOPE_AUDIO_BALANCE = UINT32_C(8),
    CANVIEW_CONTROL_SCOPE_AUDIO_MUTE = UINT32_C(0x10),
    CANVIEW_CONTROL_SCOPE_AUDIO_REAR_MUTE = UINT32_C(0x20),
    CANVIEW_CONTROL_SCOPE_AUDIO_SDVC = UINT32_C(0x40),
    CANVIEW_CONTROL_SCOPE_AUDIO_RESTORE = UINT32_C(0x80),
    CANVIEW_CONTROL_SCOPE_DRIVE_MODE_PULSE = UINT32_C(0x100),
    CANVIEW_CONTROL_SCOPE_ADAPTIVE_VOLUME_AUTOMATION = UINT32_C(0x200),
    CANVIEW_CONTROL_SCOPE_AUTO_SPORT_AUTOMATION = UINT32_C(0x400),
} canview_control_scope_t;

typedef enum {
    CANVIEW_COMMAND_ID_AUDIO_PROFILE_SET = UINT32_C(0x101),
    CANVIEW_COMMAND_ID_AUDIO_VOLUME_OFFSET_SET = UINT32_C(0x102),
    CANVIEW_COMMAND_ID_AUDIO_RESTORE_SNAPSHOT = UINT32_C(0x103),
    CANVIEW_COMMAND_ID_DRIVE_MODE_BUTTON_PULSE = UINT32_C(0x201),
    CANVIEW_COMMAND_ID_AUTOMATION_ARM = UINT32_C(0x202),
    CANVIEW_COMMAND_ID_AUTOMATION_DISARM = UINT32_C(0x203),
} canview_command_id_t;

typedef enum {
    CANVIEW_AUTOMATION_ADAPTIVE_VOLUME = UINT32_C(1),
    CANVIEW_AUTOMATION_AUTO_SPORT = UINT32_C(2),
} canview_automation_id_t;

typedef enum {
    CANVIEW_CONFIG_SPORT_AUTOMATION_ENABLED = UINT32_C(0x201),
    CANVIEW_CONFIG_SPORT_ENTRY_SPEED_TENTH_KPH = UINT32_C(0x202),
    CANVIEW_CONFIG_SPORT_ACCELERATION_ENABLED = UINT32_C(0x203),
    CANVIEW_CONFIG_RTC_LOCAL_TIME = UINT32_C(0x301),
    CANVIEW_CONFIG_SUNRISE_MINUTES = UINT32_C(0x302),
    CANVIEW_CONFIG_SUNSET_MINUTES = UINT32_C(0x303),
    CANVIEW_CONFIG_HEADLAMP_WARNING_ENABLED = UINT32_C(0x304),
    CANVIEW_CONFIG_CAN_RX_STREAM_PERIOD_MS = UINT32_C(0x401),
    CANVIEW_CONFIG_CAN_RX_STREAM_MAX_RECORDS = UINT32_C(0x402),
    CANVIEW_CONFIG_CAN_RX_BYTES_PER_SECOND = UINT32_C(0x403),
} canview_config_key_t;

typedef enum {
    CANVIEW_CAN_FILTER_ADD = UINT32_C(1),
    CANVIEW_CAN_FILTER_REPLACE = UINT32_C(2),
    CANVIEW_CAN_FILTER_DELETE = UINT32_C(3),
    CANVIEW_CAN_FILTER_CLEAR = UINT32_C(4),
} canview_can_filter_action_t;

typedef enum {
    CANVIEW_CAN_FILTER_APPLIED = UINT32_C(0),
    CANVIEW_CAN_FILTER_INVALID = UINT32_C(1),
    CANVIEW_CAN_FILTER_FULL = UINT32_C(2),
    CANVIEW_CAN_FILTER_NOT_FOUND = UINT32_C(3),
    CANVIEW_CAN_FILTER_CONFLICT = UINT32_C(4),
} canview_can_filter_result_t;

typedef enum {
    CANVIEW_ERROR_TRANSPORT_BASE = UINT32_C(0x100),
    CANVIEW_ERROR_PROTOCOL_BASE = UINT32_C(0x200),
    CANVIEW_ERROR_AUTH_BASE = UINT32_C(0x300),
    CANVIEW_ERROR_COMPAT_BASE = UINT32_C(0x400),
    CANVIEW_ERROR_RESOURCE_BASE = UINT32_C(0x500),
    CANVIEW_ERROR_APPLICATION_BASE = UINT32_C(0x600),
    CANVIEW_ERROR_SAFETY_BASE = UINT32_C(0x700),
    CANVIEW_ERROR_BAD_LENGTH = UINT32_C(0x201),
    CANVIEW_ERROR_BAD_MAGIC = UINT32_C(0x202),
    CANVIEW_ERROR_BAD_CRC = UINT32_C(0x203),
    CANVIEW_ERROR_BAD_RESERVED = UINT32_C(0x204),
    CANVIEW_ERROR_UNSUPPORTED_MESSAGE = UINT32_C(0x401),
    CANVIEW_ERROR_INCOMPATIBLE_MAJOR = UINT32_C(0x402),
    CANVIEW_ERROR_QUEUE_FULL = UINT32_C(0x501),
    CANVIEW_ERROR_AUTH_FAILED = UINT32_C(0x301),
    CANVIEW_ERROR_SESSION_MISMATCH = UINT32_C(0x302),
    CANVIEW_ERROR_LEASE_REQUIRED = UINT32_C(0x701),
    CANVIEW_ERROR_PRECONDITION_FAILED = UINT32_C(0x702),
    CANVIEW_ERROR_SIGNAL_STALE = UINT32_C(0x703),
    CANVIEW_ERROR_TX_DISABLED = UINT32_C(0x704),
} canview_error_code_t;

typedef enum {
    CANVIEW_OBSERVER_OFF = UINT32_C(0),
    CANVIEW_OBSERVER_INVENTORY = UINT32_C(1),
    CANVIEW_OBSERVER_EVENT_DIFF = UINT32_C(2),
    CANVIEW_OBSERVER_FILTERED_RAW = UINT32_C(3),
    CANVIEW_OBSERVER_ARMED_DRIVE = UINT32_C(4),
} canview_observer_mode_t;

typedef enum {
    CANVIEW_CAPTURE_ARM = UINT32_C(1),
    CANVIEW_CAPTURE_START = UINT32_C(2),
    CANVIEW_CAPTURE_STOP = UINT32_C(3),
    CANVIEW_CAPTURE_CANCEL = UINT32_C(4),
} canview_capture_action_t;

typedef enum {
    CANVIEW_CAPTURE_IDLE = UINT32_C(0),
    CANVIEW_CAPTURE_ARMED = UINT32_C(1),
    CANVIEW_CAPTURE_CAPTURING = UINT32_C(2),
    CANVIEW_CAPTURE_FINALIZING = UINT32_C(3),
    CANVIEW_CAPTURE_COMPLETE = UINT32_C(4),
    CANVIEW_CAPTURE_FAILED = UINT32_C(5),
    CANVIEW_CAPTURE_CANCELLED = UINT32_C(6),
} canview_capture_state_t;

typedef enum {
    CANVIEW_MARKER_USER_EVENT = UINT32_C(1),
    CANVIEW_MARKER_LIGHTS = UINT32_C(2),
    CANVIEW_MARKER_AUDIO = UINT32_C(3),
    CANVIEW_MARKER_DRIVE_MODE = UINT32_C(4),
    CANVIEW_MARKER_CUSTOM_TEMPLATE = UINT32_C(5),
} canview_marker_kind_t;

typedef enum {
    CANVIEW_DIAGNOSTIC_LEASE_ACQUIRE = UINT32_C(1),
    CANVIEW_DIAGNOSTIC_LEASE_RENEW = UINT32_C(2),
    CANVIEW_DIAGNOSTIC_LEASE_RELEASE = UINT32_C(3),
} canview_diagnostic_lease_action_t;

typedef enum {
    CANVIEW_DIAGNOSTIC_LEASE_GRANTED = UINT32_C(0),
    CANVIEW_DIAGNOSTIC_LEASE_REJECTED = UINT32_C(1),
    CANVIEW_DIAGNOSTIC_LEASE_NOT_OWNER = UINT32_C(2),
    CANVIEW_DIAGNOSTIC_LEASE_EXPIRED = UINT32_C(3),
} canview_diagnostic_lease_status_t;

typedef enum {
    CANVIEW_REMOTE_CONFIG_RECEIVED = UINT32_C(0),
    CANVIEW_REMOTE_CONFIG_PENDING_CONFIRMATION = UINT32_C(1),
    CANVIEW_REMOTE_CONFIG_APPLIED = UINT32_C(2),
    CANVIEW_REMOTE_CONFIG_REJECTED = UINT32_C(3),
    CANVIEW_REMOTE_CONFIG_EXPIRED = UINT32_C(4),
    CANVIEW_REMOTE_CONFIG_UNCHANGED = UINT32_C(5),
} canview_remote_config_stage_t;

typedef enum {
    CANVIEW_BULK_ACCEPTED = UINT32_C(0),
    CANVIEW_BULK_REJECTED = UINT32_C(1),
    CANVIEW_BULK_ABORTED = UINT32_C(2),
    CANVIEW_BULK_COMPLETE = UINT32_C(3),
    CANVIEW_BULK_EXPIRED = UINT32_C(4),
} canview_bulk_status_t;

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
    uint16_t reserved_le; /* reserved, transmit as zero */
    uint32_t crc32_le;
} canview_frame_header_t;

typedef struct CANVIEW_PACKED {
    uint16_t type_le;
    uint16_t length_le;
} canview_tlv_header_t;

typedef struct CANVIEW_PACKED {
    uint64_t installation_id_le;
    uint64_t endpoint_id_le;
    uint8_t mac[6];
    uint8_t reserved0[2]; /* reserved, transmit as zero */
    uint8_t nonce[16];
    uint8_t channel;
    uint8_t proposed_major;
    uint8_t proposed_minor_min;
    uint8_t proposed_minor_max;
    uint32_t link_key_generation_le;
    uint32_t expires_at_ms_le;
    uint8_t hmac_tag[16];
} canview_discovery_payload_t;

typedef struct CANVIEW_PACKED {
    uint64_t request_token_le;
    uint8_t discovery_digest[16];
    uint64_t requester_endpoint_id_le;
    uint8_t requester_mac[6];
    uint8_t reserved0[2]; /* reserved, transmit as zero */
    uint8_t controller_nonce[16];
    uint8_t requested_role;
    uint8_t requested_major;
    uint8_t requested_minor;
    uint8_t reserved1; /* reserved, transmit as zero */
    uint32_t expires_at_ms_le;
    uint8_t hmac_tag[16];
} canview_pair_request_payload_t;

typedef struct CANVIEW_PACKED {
    uint8_t discovery_digest[16];
    uint8_t request_digest[16];
    uint64_t endpoint0_id_le;
    uint64_t endpoint1_id_le;
    uint8_t nonce0[16];
    uint8_t nonce1[16];
    uint8_t selected_major;
    uint8_t selected_minor;
    uint8_t channel;
    uint8_t reserved0; /* reserved, transmit as zero */
    uint8_t authorized_role;
    uint8_t reserved1; /* reserved, transmit as zero */
    uint16_t authorized_scope_le;
    uint32_t allowed_message_classes_le;
    uint32_t link_key_generation_le;
    uint8_t hmac_tag[16];
} canview_pair_challenge_payload_t;

typedef struct CANVIEW_PACKED {
    uint8_t transcript_hash[32];
    uint8_t selected_major;
    uint8_t selected_minor;
    uint8_t authorized_role;
    uint8_t reserved0; /* reserved, transmit as zero */
    uint32_t link_key_generation_le;
    uint8_t confirm_nonce[16];
    uint8_t hmac_tag[16];
} canview_pair_confirm_payload_t;

typedef struct CANVIEW_PACKED {
    uint64_t request_token_le;
    uint8_t status;
    uint8_t assigned_role;
    uint16_t reserved0_le; /* reserved, transmit as zero */
    uint32_t link_key_generation_le;
    uint64_t peer_id_le;
    uint8_t transcript_hash[16];
    uint32_t expires_at_ms_le;
    uint8_t hmac_tag[16];
} canview_pair_result_payload_t;

typedef struct CANVIEW_PACKED {
    uint64_t device_id_le;
    uint64_t boot_id_le;
    uint8_t nonce[16];
    uint8_t role;
    uint8_t major;
    uint8_t minor;
    uint8_t reserved0; /* reserved, transmit as zero */
    uint16_t max_frame_le;
    uint8_t build_id_digest[16];
    uint8_t challenge[16];
    uint8_t capability_digest[16];
} canview_hello_payload_t;

typedef struct CANVIEW_PACKED {
    uint64_t device_id_le;
    uint64_t boot_id_le;
    uint8_t role;
    uint8_t selected_major;
    uint8_t selected_minor;
    uint8_t reserved0; /* reserved, transmit as zero */
    uint16_t max_frame_le;
    uint8_t bus_count;
    uint8_t support_flags;
    uint16_t control_scope_le;
    uint8_t max_filters;
    uint8_t max_peers;
    uint8_t max_batch_records;
    uint8_t reserved1; /* reserved, transmit as zero */
    uint32_t profile_id_le;
    uint32_t catalog_revision_le;
    uint16_t config_schema_version_le;
    uint16_t reserved2_le; /* reserved, transmit as zero */
    uint8_t profile_digest[32];
    uint8_t catalog_digest[32];
    uint8_t build_id_digest[16];
} canview_capabilities_prefix_t;

typedef struct CANVIEW_PACKED {
    uint64_t request_token_le;
    uint64_t t1_sender_us_le;
    uint32_t sync_generation_le;
    uint32_t reserved0_le; /* reserved, transmit as zero */
} canview_time_sync_request_payload_t;

typedef struct CANVIEW_PACKED {
    uint64_t request_token_le;
    uint64_t t1_sender_us_le;
    uint64_t t2_receiver_us_le;
    uint64_t t3_receiver_send_us_le;
    uint32_t sync_generation_le;
    uint32_t uncertainty_us_le;
    uint32_t reserved0_le; /* reserved, transmit as zero */
} canview_time_sync_response_payload_t;

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
    uint64_t telemetry_dropped_le;
    uint64_t protocol_error_count_le;
    uint16_t safety_inhibit_le;
    uint16_t reserved0_le; /* reserved, transmit as zero */
} canview_heartbeat_payload_t;

typedef struct CANVIEW_PACKED {
    uint32_t acknowledged_sequence_le;
    uint64_t request_token_le;
    uint16_t status_le;
    uint16_t detail_le;
    uint32_t receiver_time_ms_le;
} canview_ack_payload_t;

typedef struct CANVIEW_PACKED {
    uint16_t code_le;
    uint8_t severity;
    uint8_t origin;
    uint8_t offending_message_type;
    uint8_t reserved0; /* reserved, transmit as zero */
    uint16_t detail_le;
    uint32_t offending_sequence_le;
    uint32_t retry_after_ms_le;
} canview_error_payload_t;

typedef struct CANVIEW_PACKED {
    uint64_t request_token_le;
    uint32_t known_state_revision_le;
    uint64_t known_boot_id_le;
    uint32_t reserved0_le; /* reserved, transmit as zero */
} canview_state_snapshot_request_payload_t;

typedef struct CANVIEW_PACKED {
    uint64_t stm_boot_id_le;
    uint32_t state_revision_le;
    uint32_t profile_revision_le;
    uint8_t profile_digest[16];
    uint8_t build_mode;
    uint8_t tx_gate;
    uint8_t active_bus_mask;
    uint8_t bus_error_mask;
    uint8_t control_lease_state;
    uint8_t link_state;
    uint8_t audio_quality;
    uint8_t sport_state;
    uint16_t safety_inhibit_le;
    uint16_t reserved0_le; /* reserved, transmit as zero */
    uint32_t audio_snapshot_revision_le;
    uint32_t completed_time_ms_le;
    uint32_t vehicle_profile_id_le;
    uint32_t control_generation_le;
} canview_state_snapshot_payload_t;

typedef struct CANVIEW_PACKED {
    uint64_t base_time_us_le;
    uint8_t count;
    uint8_t dropped_since_last;
    uint16_t reserved_le; /* reserved, transmit as zero */
} canview_can_batch_header_t;

typedef struct CANVIEW_PACKED {
    uint16_t delta_us_le;
    uint8_t bus_id;
    uint8_t flags_dlc;
    uint32_t can_id_le;
    uint8_t data[8];
} canview_can_record_t;

typedef struct CANVIEW_PACKED {
    uint64_t sample_time_us_le;
    uint8_t count;
    uint8_t reserved0[3]; /* reserved, transmit as zero */
    uint32_t catalog_revision_le;
} canview_signal_batch_header_t;

typedef struct CANVIEW_PACKED {
    uint16_t signal_id_le;
    uint8_t value_type;
    uint8_t quality;
    uint16_t age_ms_le;
    uint8_t evidence_grade;
    uint8_t reserved; /* reserved, transmit as zero */
    uint32_t value_bits_le;
} canview_signal_record_t;

typedef struct CANVIEW_PACKED {
    uint64_t boot_id_le;
    uint8_t count;
    uint8_t reserved0[3]; /* reserved, transmit as zero */
} canview_bus_status_header_t;

typedef struct CANVIEW_PACKED {
    uint8_t bus_id;
    uint8_t state;
    uint16_t flags_le;
    uint32_t bitrate_le;
    uint64_t error_count_le;
    uint64_t rx_count_le;
    uint8_t tx_inhibit;
    uint8_t reserved0[3]; /* reserved, transmit as zero */
    uint64_t bus_off_count_le;
} canview_bus_status_record_t;

typedef struct CANVIEW_PACKED {
    uint32_t config_revision_le;
    uint32_t filter_id_le;
} canview_can_filter_get_payload_t;

typedef struct CANVIEW_PACKED {
    uint8_t action;
    uint8_t count;
    uint16_t reserved_le; /* reserved, transmit as zero */
    uint32_t config_revision_le;
} canview_can_filter_batch_header_t;

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
    uint8_t reserved0; /* reserved, transmit as zero */
} canview_can_filter_t;

typedef struct CANVIEW_PACKED {
    uint32_t config_revision_le;
    uint32_t filter_id_le;
    uint8_t action;
    uint8_t result;
    uint16_t detail_le;
} canview_can_filter_result_payload_t;

typedef struct CANVIEW_PACKED {
    uint32_t config_revision_le;
    uint16_t period_ms_le;
    uint8_t max_records_per_period;
    uint8_t enabled;
    uint32_t max_bytes_per_second_le;
    uint16_t burst_bytes_le;
    uint16_t reserved_le; /* reserved, transmit as zero */
} canview_can_stream_config_t;

typedef struct CANVIEW_PACKED {
    uint32_t config_revision_le;
    uint16_t accepted_records_le;
    uint16_t rejected_records_le;
    uint32_t dropped_by_budget_le;
    uint32_t reserved_le; /* reserved, transmit as zero */
} canview_can_stream_status_t;

typedef struct CANVIEW_PACKED {
    uint64_t request_token_le;
    uint32_t expected_observer_revision_le;
    uint32_t filter_revision_le;
    uint8_t mode;
    uint8_t bus_mask;
    uint16_t flags_le;
    uint16_t stats_period_ms_le;
    uint16_t max_records_per_second_le;
    uint32_t max_bytes_per_second_le;
    uint32_t duration_ms_le;
    uint16_t pretrigger_ms_le;
    uint16_t posttrigger_ms_le;
    uint32_t reserved0_le; /* reserved, transmit as zero */
} canview_observer_config_payload_t;

typedef struct CANVIEW_PACKED {
    uint32_t window_end_ms_le;
    uint8_t count;
    uint8_t part_index;
    uint8_t part_count;
    uint8_t reserved0; /* reserved, transmit as zero */
} canview_can_id_stats_header_t;

typedef struct CANVIEW_PACKED {
    uint8_t bus_id;
    uint8_t flags_dlc;
    uint16_t rate_tenth_hz_le;
    uint32_t can_id_le;
    uint32_t frame_count_le;
    uint32_t change_count_le;
    uint32_t period_p50_us_le;
    uint32_t period_p95_us_le;
    uint64_t bit_change_mask_le;
    uint8_t last_data[8];
} canview_can_id_stats_record_t;

typedef struct CANVIEW_PACKED {
    uint64_t request_token_le;
    uint64_t capture_id_le;
    uint8_t action;
    uint8_t reserved0; /* reserved, transmit as zero */
    uint16_t flags_le;
    uint32_t requested_time_ms_le;
    uint32_t reserved1_le; /* reserved, transmit as zero */
} canview_capture_control_payload_t;

typedef struct CANVIEW_PACKED {
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
    uint32_t reserved1_le; /* reserved, transmit as zero */
} canview_capture_status_payload_t;

typedef struct CANVIEW_PACKED {
    uint64_t capture_id_le;
    uint32_t marker_id_le;
    uint32_t sender_time_ms_le;
    uint16_t marker_kind_le;
    uint16_t flags_le;
    uint16_t label_code_le;
    uint16_t reserved0_le; /* reserved, transmit as zero */
    uint32_t time_uncertainty_us_le;
} canview_event_marker_payload_t;

typedef struct CANVIEW_PACKED {
    uint64_t request_token_le;
    uint8_t action;
    uint8_t reserved0[3]; /* reserved, transmit as zero */
    uint32_t requested_ms_le;
    uint64_t lease_id_le;
} canview_diagnostic_lease_request_t;

typedef struct CANVIEW_PACKED {
    uint64_t request_token_le;
    uint64_t lease_id_le;
    uint8_t status;
    uint8_t reserved0[3]; /* reserved, transmit as zero */
    uint32_t granted_ms_le;
    uint32_t expires_at_ms_le;
    uint64_t owner_device_id_le;
} canview_diagnostic_lease_response_t;

typedef struct CANVIEW_PACKED {
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
    uint16_t reserved_le; /* reserved, transmit as zero */
    uint8_t control_tag[16];
} canview_command_request_t;

typedef struct CANVIEW_PACKED {
    uint64_t request_token_le;
    uint16_t command_id_le;
    uint8_t stage;
    uint8_t reserved0; /* reserved, transmit as zero */
    uint16_t reason_le;
    uint32_t state_revision_le;
    uint32_t completed_time_ms_le;
    uint32_t request_sequence_le;
    uint8_t control_tag[16];
    uint32_t feedback_revision_le;
    uint32_t feedback_time_ms_le;
} canview_command_result_t;

typedef struct CANVIEW_PACKED {
    uint64_t request_token_le;
    uint8_t action;
    uint8_t reserved0[3]; /* reserved, transmit as zero */
    uint32_t requested_ms_le;
    uint64_t lease_id_le;
    uint16_t requested_scope_le;
    uint16_t reserved1_le; /* reserved, transmit as zero */
    uint32_t expected_state_revision_le;
    uint32_t control_generation_le;
    uint8_t control_tag[16];
} canview_control_lease_request_t;

typedef struct CANVIEW_PACKED {
    uint64_t request_token_le;
    uint64_t lease_id_le;
    uint8_t status;
    uint8_t reserved0[3]; /* reserved, transmit as zero */
    uint32_t granted_ms_le;
    uint32_t expires_at_ms_le;
    uint64_t owner_device_id_le;
    uint16_t granted_scope_le;
    uint16_t reserved1_le; /* reserved, transmit as zero */
    uint32_t control_generation_le;
    uint8_t control_tag[16];
} canview_control_lease_status_t;

typedef struct CANVIEW_PACKED {
    uint64_t request_token_le;
    uint32_t expected_state_revision_le;
    uint16_t schema_version_le;
    uint16_t reserved0_le; /* reserved, transmit as zero */
    uint16_t key_le;
    uint16_t reserved1_le; /* reserved, transmit as zero */
    uint32_t known_revision_le;
} canview_config_get_payload_t;

typedef struct CANVIEW_PACKED {
    uint16_t schema_version_le;
    uint8_t count;
    uint8_t reserved; /* reserved, transmit as zero */
} canview_config_batch_header_t;

typedef struct CANVIEW_PACKED {
    uint16_t key_le;
    uint8_t value_type;
    uint8_t reserved; /* reserved, transmit as zero */
    uint32_t value_bits_le;
} canview_config_record_t;

typedef struct CANVIEW_PACKED {
    uint64_t request_token_le;
    uint8_t stage;
    uint8_t applied_count;
    uint8_t pending_count;
    uint8_t reserved0; /* reserved, transmit as zero */
    uint16_t reason_le;
    uint16_t reserved1_le; /* reserved, transmit as zero */
    uint32_t state_revision_le;
    uint32_t completed_time_ms_le;
    uint32_t detail_le;
} canview_config_result_payload_t;

typedef struct CANVIEW_PACKED {
    uint64_t request_token_le;
    uint16_t known_schema_version_le;
    uint16_t reserved0_le; /* reserved, transmit as zero */
    uint32_t known_digest_prefix_le;
} canview_config_schema_request_payload_t;

typedef struct CANVIEW_PACKED {
    uint64_t request_token_le;
    uint32_t expected_state_revision_le;
    uint16_t schema_version_le;
    uint8_t count;
    uint8_t flags;
    uint16_t ttl_ms_le;
    uint16_t reserved0_le; /* reserved, transmit as zero */
} canview_remote_config_request_prefix_t;

typedef struct CANVIEW_PACKED {
    uint64_t request_token_le;
    uint8_t stage;
    uint8_t applied_count;
    uint8_t pending_count;
    uint8_t reserved0; /* reserved, transmit as zero */
    uint16_t reason_le;
    uint16_t reserved1_le; /* reserved, transmit as zero */
    uint32_t state_revision_le;
    uint32_t completed_time_ms_le;
    uint32_t detail_le;
} canview_remote_config_status_payload_t;

typedef struct CANVIEW_PACKED {
    uint64_t boot_id_le;
    uint32_t state_revision_le;
    uint8_t count;
    uint8_t reserved0[3]; /* reserved, transmit as zero */
} canview_diagnostic_counters_header_t;

typedef struct CANVIEW_PACKED {
    uint16_t counter_id_le;
    uint8_t category;
    uint8_t reserved0; /* reserved, transmit as zero */
    uint64_t value_le;
    uint8_t saturated;
    uint8_t reserved1[3]; /* reserved, transmit as zero */
} canview_diagnostic_counter_record_t;

typedef struct CANVIEW_PACKED {
    uint64_t request_token_le;
    uint8_t object_id[16];
    uint16_t object_type_le;
    uint16_t flags_le;
    uint32_t total_size_le;
    uint16_t fragment_size_le;
    uint8_t window_size;
    uint8_t reserved0; /* reserved, transmit as zero */
    uint8_t sha256[32];
    uint32_t timeout_ms_le;
} canview_bulk_begin_payload_t;

typedef struct CANVIEW_PACKED {
    uint8_t object_id[16];
    uint32_t fragment_index_le;
    uint32_t total_fragments_le;
    uint16_t payload_len_le;
    uint16_t flags_le;
} canview_bulk_fragment_prefix_t;

typedef struct CANVIEW_PACKED {
    uint8_t object_id[16];
    uint32_t base_fragment_le;
    uint32_t received_bitmap_le;
    uint32_t received_bytes_le;
    uint8_t window_size;
    uint8_t status;
    uint16_t reserved0_le; /* reserved, transmit as zero */
} canview_bulk_ack_payload_t;

typedef struct CANVIEW_PACKED {
    uint8_t object_id[16];
    uint8_t status;
    uint8_t reserved0[3]; /* reserved, transmit as zero */
    uint32_t total_size_le;
    uint8_t sha256[32];
    uint16_t reason_le;
    uint16_t reserved1_le; /* reserved, transmit as zero */
} canview_bulk_end_payload_t;

#if defined(__cplusplus)
static_assert(sizeof(canview_frame_header_t) == 32U, "canview_frame_header_t wire size");
static_assert(offsetof(canview_frame_header_t, magic_le) == 0U, "canview_frame_header_t.magic_le offset");
static_assert(offsetof(canview_frame_header_t, major) == 2U, "canview_frame_header_t.major offset");
static_assert(offsetof(canview_frame_header_t, minor) == 3U, "canview_frame_header_t.minor offset");
static_assert(offsetof(canview_frame_header_t, header_len) == 4U, "canview_frame_header_t.header_len offset");
static_assert(offsetof(canview_frame_header_t, message_type) == 5U, "canview_frame_header_t.message_type offset");
static_assert(offsetof(canview_frame_header_t, flags) == 6U, "canview_frame_header_t.flags offset");
static_assert(offsetof(canview_frame_header_t, priority) == 7U, "canview_frame_header_t.priority offset");
static_assert(offsetof(canview_frame_header_t, session_id_le) == 8U, "canview_frame_header_t.session_id_le offset");
static_assert(offsetof(canview_frame_header_t, sequence_le) == 12U, "canview_frame_header_t.sequence_le offset");
static_assert(offsetof(canview_frame_header_t, sender_time_ms_le) == 16U, "canview_frame_header_t.sender_time_ms_le offset");
static_assert(offsetof(canview_frame_header_t, correlation_id_le) == 20U, "canview_frame_header_t.correlation_id_le offset");
static_assert(offsetof(canview_frame_header_t, payload_len_le) == 24U, "canview_frame_header_t.payload_len_le offset");
static_assert(offsetof(canview_frame_header_t, reserved_le) == 26U, "canview_frame_header_t.reserved_le offset");
static_assert(offsetof(canview_frame_header_t, crc32_le) == 28U, "canview_frame_header_t.crc32_le offset");
static_assert(sizeof(canview_tlv_header_t) == 4U, "canview_tlv_header_t wire size");
static_assert(offsetof(canview_tlv_header_t, type_le) == 0U, "canview_tlv_header_t.type_le offset");
static_assert(offsetof(canview_tlv_header_t, length_le) == 2U, "canview_tlv_header_t.length_le offset");
static_assert(sizeof(canview_discovery_payload_t) == 68U, "canview_discovery_payload_t wire size");
static_assert(offsetof(canview_discovery_payload_t, installation_id_le) == 0U, "canview_discovery_payload_t.installation_id_le offset");
static_assert(offsetof(canview_discovery_payload_t, endpoint_id_le) == 8U, "canview_discovery_payload_t.endpoint_id_le offset");
static_assert(offsetof(canview_discovery_payload_t, mac) == 16U, "canview_discovery_payload_t.mac offset");
static_assert(offsetof(canview_discovery_payload_t, reserved0) == 22U, "canview_discovery_payload_t.reserved0 offset");
static_assert(offsetof(canview_discovery_payload_t, nonce) == 24U, "canview_discovery_payload_t.nonce offset");
static_assert(offsetof(canview_discovery_payload_t, channel) == 40U, "canview_discovery_payload_t.channel offset");
static_assert(offsetof(canview_discovery_payload_t, proposed_major) == 41U, "canview_discovery_payload_t.proposed_major offset");
static_assert(offsetof(canview_discovery_payload_t, proposed_minor_min) == 42U, "canview_discovery_payload_t.proposed_minor_min offset");
static_assert(offsetof(canview_discovery_payload_t, proposed_minor_max) == 43U, "canview_discovery_payload_t.proposed_minor_max offset");
static_assert(offsetof(canview_discovery_payload_t, link_key_generation_le) == 44U, "canview_discovery_payload_t.link_key_generation_le offset");
static_assert(offsetof(canview_discovery_payload_t, expires_at_ms_le) == 48U, "canview_discovery_payload_t.expires_at_ms_le offset");
static_assert(offsetof(canview_discovery_payload_t, hmac_tag) == 52U, "canview_discovery_payload_t.hmac_tag offset");
static_assert(sizeof(canview_pair_request_payload_t) == 80U, "canview_pair_request_payload_t wire size");
static_assert(offsetof(canview_pair_request_payload_t, request_token_le) == 0U, "canview_pair_request_payload_t.request_token_le offset");
static_assert(offsetof(canview_pair_request_payload_t, discovery_digest) == 8U, "canview_pair_request_payload_t.discovery_digest offset");
static_assert(offsetof(canview_pair_request_payload_t, requester_endpoint_id_le) == 24U, "canview_pair_request_payload_t.requester_endpoint_id_le offset");
static_assert(offsetof(canview_pair_request_payload_t, requester_mac) == 32U, "canview_pair_request_payload_t.requester_mac offset");
static_assert(offsetof(canview_pair_request_payload_t, reserved0) == 38U, "canview_pair_request_payload_t.reserved0 offset");
static_assert(offsetof(canview_pair_request_payload_t, controller_nonce) == 40U, "canview_pair_request_payload_t.controller_nonce offset");
static_assert(offsetof(canview_pair_request_payload_t, requested_role) == 56U, "canview_pair_request_payload_t.requested_role offset");
static_assert(offsetof(canview_pair_request_payload_t, requested_major) == 57U, "canview_pair_request_payload_t.requested_major offset");
static_assert(offsetof(canview_pair_request_payload_t, requested_minor) == 58U, "canview_pair_request_payload_t.requested_minor offset");
static_assert(offsetof(canview_pair_request_payload_t, reserved1) == 59U, "canview_pair_request_payload_t.reserved1 offset");
static_assert(offsetof(canview_pair_request_payload_t, expires_at_ms_le) == 60U, "canview_pair_request_payload_t.expires_at_ms_le offset");
static_assert(offsetof(canview_pair_request_payload_t, hmac_tag) == 64U, "canview_pair_request_payload_t.hmac_tag offset");
static_assert(sizeof(canview_pair_challenge_payload_t) == 112U, "canview_pair_challenge_payload_t wire size");
static_assert(offsetof(canview_pair_challenge_payload_t, discovery_digest) == 0U, "canview_pair_challenge_payload_t.discovery_digest offset");
static_assert(offsetof(canview_pair_challenge_payload_t, request_digest) == 16U, "canview_pair_challenge_payload_t.request_digest offset");
static_assert(offsetof(canview_pair_challenge_payload_t, endpoint0_id_le) == 32U, "canview_pair_challenge_payload_t.endpoint0_id_le offset");
static_assert(offsetof(canview_pair_challenge_payload_t, endpoint1_id_le) == 40U, "canview_pair_challenge_payload_t.endpoint1_id_le offset");
static_assert(offsetof(canview_pair_challenge_payload_t, nonce0) == 48U, "canview_pair_challenge_payload_t.nonce0 offset");
static_assert(offsetof(canview_pair_challenge_payload_t, nonce1) == 64U, "canview_pair_challenge_payload_t.nonce1 offset");
static_assert(offsetof(canview_pair_challenge_payload_t, selected_major) == 80U, "canview_pair_challenge_payload_t.selected_major offset");
static_assert(offsetof(canview_pair_challenge_payload_t, selected_minor) == 81U, "canview_pair_challenge_payload_t.selected_minor offset");
static_assert(offsetof(canview_pair_challenge_payload_t, channel) == 82U, "canview_pair_challenge_payload_t.channel offset");
static_assert(offsetof(canview_pair_challenge_payload_t, reserved0) == 83U, "canview_pair_challenge_payload_t.reserved0 offset");
static_assert(offsetof(canview_pair_challenge_payload_t, authorized_role) == 84U, "canview_pair_challenge_payload_t.authorized_role offset");
static_assert(offsetof(canview_pair_challenge_payload_t, reserved1) == 85U, "canview_pair_challenge_payload_t.reserved1 offset");
static_assert(offsetof(canview_pair_challenge_payload_t, authorized_scope_le) == 86U, "canview_pair_challenge_payload_t.authorized_scope_le offset");
static_assert(offsetof(canview_pair_challenge_payload_t, allowed_message_classes_le) == 88U, "canview_pair_challenge_payload_t.allowed_message_classes_le offset");
static_assert(offsetof(canview_pair_challenge_payload_t, link_key_generation_le) == 92U, "canview_pair_challenge_payload_t.link_key_generation_le offset");
static_assert(offsetof(canview_pair_challenge_payload_t, hmac_tag) == 96U, "canview_pair_challenge_payload_t.hmac_tag offset");
static_assert(sizeof(canview_pair_confirm_payload_t) == 72U, "canview_pair_confirm_payload_t wire size");
static_assert(offsetof(canview_pair_confirm_payload_t, transcript_hash) == 0U, "canview_pair_confirm_payload_t.transcript_hash offset");
static_assert(offsetof(canview_pair_confirm_payload_t, selected_major) == 32U, "canview_pair_confirm_payload_t.selected_major offset");
static_assert(offsetof(canview_pair_confirm_payload_t, selected_minor) == 33U, "canview_pair_confirm_payload_t.selected_minor offset");
static_assert(offsetof(canview_pair_confirm_payload_t, authorized_role) == 34U, "canview_pair_confirm_payload_t.authorized_role offset");
static_assert(offsetof(canview_pair_confirm_payload_t, reserved0) == 35U, "canview_pair_confirm_payload_t.reserved0 offset");
static_assert(offsetof(canview_pair_confirm_payload_t, link_key_generation_le) == 36U, "canview_pair_confirm_payload_t.link_key_generation_le offset");
static_assert(offsetof(canview_pair_confirm_payload_t, confirm_nonce) == 40U, "canview_pair_confirm_payload_t.confirm_nonce offset");
static_assert(offsetof(canview_pair_confirm_payload_t, hmac_tag) == 56U, "canview_pair_confirm_payload_t.hmac_tag offset");
static_assert(sizeof(canview_pair_result_payload_t) == 60U, "canview_pair_result_payload_t wire size");
static_assert(offsetof(canview_pair_result_payload_t, request_token_le) == 0U, "canview_pair_result_payload_t.request_token_le offset");
static_assert(offsetof(canview_pair_result_payload_t, status) == 8U, "canview_pair_result_payload_t.status offset");
static_assert(offsetof(canview_pair_result_payload_t, assigned_role) == 9U, "canview_pair_result_payload_t.assigned_role offset");
static_assert(offsetof(canview_pair_result_payload_t, reserved0_le) == 10U, "canview_pair_result_payload_t.reserved0_le offset");
static_assert(offsetof(canview_pair_result_payload_t, link_key_generation_le) == 12U, "canview_pair_result_payload_t.link_key_generation_le offset");
static_assert(offsetof(canview_pair_result_payload_t, peer_id_le) == 16U, "canview_pair_result_payload_t.peer_id_le offset");
static_assert(offsetof(canview_pair_result_payload_t, transcript_hash) == 24U, "canview_pair_result_payload_t.transcript_hash offset");
static_assert(offsetof(canview_pair_result_payload_t, expires_at_ms_le) == 40U, "canview_pair_result_payload_t.expires_at_ms_le offset");
static_assert(offsetof(canview_pair_result_payload_t, hmac_tag) == 44U, "canview_pair_result_payload_t.hmac_tag offset");
static_assert(sizeof(canview_hello_payload_t) == 86U, "canview_hello_payload_t wire size");
static_assert(offsetof(canview_hello_payload_t, device_id_le) == 0U, "canview_hello_payload_t.device_id_le offset");
static_assert(offsetof(canview_hello_payload_t, boot_id_le) == 8U, "canview_hello_payload_t.boot_id_le offset");
static_assert(offsetof(canview_hello_payload_t, nonce) == 16U, "canview_hello_payload_t.nonce offset");
static_assert(offsetof(canview_hello_payload_t, role) == 32U, "canview_hello_payload_t.role offset");
static_assert(offsetof(canview_hello_payload_t, major) == 33U, "canview_hello_payload_t.major offset");
static_assert(offsetof(canview_hello_payload_t, minor) == 34U, "canview_hello_payload_t.minor offset");
static_assert(offsetof(canview_hello_payload_t, reserved0) == 35U, "canview_hello_payload_t.reserved0 offset");
static_assert(offsetof(canview_hello_payload_t, max_frame_le) == 36U, "canview_hello_payload_t.max_frame_le offset");
static_assert(offsetof(canview_hello_payload_t, build_id_digest) == 38U, "canview_hello_payload_t.build_id_digest offset");
static_assert(offsetof(canview_hello_payload_t, challenge) == 54U, "canview_hello_payload_t.challenge offset");
static_assert(offsetof(canview_hello_payload_t, capability_digest) == 70U, "canview_hello_payload_t.capability_digest offset");
static_assert(sizeof(canview_capabilities_prefix_t) == 122U, "canview_capabilities_prefix_t wire size");
static_assert(offsetof(canview_capabilities_prefix_t, device_id_le) == 0U, "canview_capabilities_prefix_t.device_id_le offset");
static_assert(offsetof(canview_capabilities_prefix_t, boot_id_le) == 8U, "canview_capabilities_prefix_t.boot_id_le offset");
static_assert(offsetof(canview_capabilities_prefix_t, role) == 16U, "canview_capabilities_prefix_t.role offset");
static_assert(offsetof(canview_capabilities_prefix_t, selected_major) == 17U, "canview_capabilities_prefix_t.selected_major offset");
static_assert(offsetof(canview_capabilities_prefix_t, selected_minor) == 18U, "canview_capabilities_prefix_t.selected_minor offset");
static_assert(offsetof(canview_capabilities_prefix_t, reserved0) == 19U, "canview_capabilities_prefix_t.reserved0 offset");
static_assert(offsetof(canview_capabilities_prefix_t, max_frame_le) == 20U, "canview_capabilities_prefix_t.max_frame_le offset");
static_assert(offsetof(canview_capabilities_prefix_t, bus_count) == 22U, "canview_capabilities_prefix_t.bus_count offset");
static_assert(offsetof(canview_capabilities_prefix_t, support_flags) == 23U, "canview_capabilities_prefix_t.support_flags offset");
static_assert(offsetof(canview_capabilities_prefix_t, control_scope_le) == 24U, "canview_capabilities_prefix_t.control_scope_le offset");
static_assert(offsetof(canview_capabilities_prefix_t, max_filters) == 26U, "canview_capabilities_prefix_t.max_filters offset");
static_assert(offsetof(canview_capabilities_prefix_t, max_peers) == 27U, "canview_capabilities_prefix_t.max_peers offset");
static_assert(offsetof(canview_capabilities_prefix_t, max_batch_records) == 28U, "canview_capabilities_prefix_t.max_batch_records offset");
static_assert(offsetof(canview_capabilities_prefix_t, reserved1) == 29U, "canview_capabilities_prefix_t.reserved1 offset");
static_assert(offsetof(canview_capabilities_prefix_t, profile_id_le) == 30U, "canview_capabilities_prefix_t.profile_id_le offset");
static_assert(offsetof(canview_capabilities_prefix_t, catalog_revision_le) == 34U, "canview_capabilities_prefix_t.catalog_revision_le offset");
static_assert(offsetof(canview_capabilities_prefix_t, config_schema_version_le) == 38U, "canview_capabilities_prefix_t.config_schema_version_le offset");
static_assert(offsetof(canview_capabilities_prefix_t, reserved2_le) == 40U, "canview_capabilities_prefix_t.reserved2_le offset");
static_assert(offsetof(canview_capabilities_prefix_t, profile_digest) == 42U, "canview_capabilities_prefix_t.profile_digest offset");
static_assert(offsetof(canview_capabilities_prefix_t, catalog_digest) == 74U, "canview_capabilities_prefix_t.catalog_digest offset");
static_assert(offsetof(canview_capabilities_prefix_t, build_id_digest) == 106U, "canview_capabilities_prefix_t.build_id_digest offset");
static_assert(sizeof(canview_time_sync_request_payload_t) == 24U, "canview_time_sync_request_payload_t wire size");
static_assert(offsetof(canview_time_sync_request_payload_t, request_token_le) == 0U, "canview_time_sync_request_payload_t.request_token_le offset");
static_assert(offsetof(canview_time_sync_request_payload_t, t1_sender_us_le) == 8U, "canview_time_sync_request_payload_t.t1_sender_us_le offset");
static_assert(offsetof(canview_time_sync_request_payload_t, sync_generation_le) == 16U, "canview_time_sync_request_payload_t.sync_generation_le offset");
static_assert(offsetof(canview_time_sync_request_payload_t, reserved0_le) == 20U, "canview_time_sync_request_payload_t.reserved0_le offset");
static_assert(sizeof(canview_time_sync_response_payload_t) == 44U, "canview_time_sync_response_payload_t wire size");
static_assert(offsetof(canview_time_sync_response_payload_t, request_token_le) == 0U, "canview_time_sync_response_payload_t.request_token_le offset");
static_assert(offsetof(canview_time_sync_response_payload_t, t1_sender_us_le) == 8U, "canview_time_sync_response_payload_t.t1_sender_us_le offset");
static_assert(offsetof(canview_time_sync_response_payload_t, t2_receiver_us_le) == 16U, "canview_time_sync_response_payload_t.t2_receiver_us_le offset");
static_assert(offsetof(canview_time_sync_response_payload_t, t3_receiver_send_us_le) == 24U, "canview_time_sync_response_payload_t.t3_receiver_send_us_le offset");
static_assert(offsetof(canview_time_sync_response_payload_t, sync_generation_le) == 32U, "canview_time_sync_response_payload_t.sync_generation_le offset");
static_assert(offsetof(canview_time_sync_response_payload_t, uncertainty_us_le) == 36U, "canview_time_sync_response_payload_t.uncertainty_us_le offset");
static_assert(offsetof(canview_time_sync_response_payload_t, reserved0_le) == 40U, "canview_time_sync_response_payload_t.reserved0_le offset");
static_assert(sizeof(canview_heartbeat_payload_t) == 44U, "canview_heartbeat_payload_t wire size");
static_assert(offsetof(canview_heartbeat_payload_t, boot_id_le) == 0U, "canview_heartbeat_payload_t.boot_id_le offset");
static_assert(offsetof(canview_heartbeat_payload_t, state_revision_le) == 8U, "canview_heartbeat_payload_t.state_revision_le offset");
static_assert(offsetof(canview_heartbeat_payload_t, uptime_ms_le) == 12U, "canview_heartbeat_payload_t.uptime_ms_le offset");
static_assert(offsetof(canview_heartbeat_payload_t, heartbeat_interval_ms_le) == 16U, "canview_heartbeat_payload_t.heartbeat_interval_ms_le offset");
static_assert(offsetof(canview_heartbeat_payload_t, last_rssi_dbm) == 18U, "canview_heartbeat_payload_t.last_rssi_dbm offset");
static_assert(offsetof(canview_heartbeat_payload_t, link_state) == 19U, "canview_heartbeat_payload_t.link_state offset");
static_assert(offsetof(canview_heartbeat_payload_t, active_bus_mask) == 20U, "canview_heartbeat_payload_t.active_bus_mask offset");
static_assert(offsetof(canview_heartbeat_payload_t, bus_error_mask) == 21U, "canview_heartbeat_payload_t.bus_error_mask offset");
static_assert(offsetof(canview_heartbeat_payload_t, rx_queue_depth_le) == 22U, "canview_heartbeat_payload_t.rx_queue_depth_le offset");
static_assert(offsetof(canview_heartbeat_payload_t, telemetry_dropped_le) == 24U, "canview_heartbeat_payload_t.telemetry_dropped_le offset");
static_assert(offsetof(canview_heartbeat_payload_t, protocol_error_count_le) == 32U, "canview_heartbeat_payload_t.protocol_error_count_le offset");
static_assert(offsetof(canview_heartbeat_payload_t, safety_inhibit_le) == 40U, "canview_heartbeat_payload_t.safety_inhibit_le offset");
static_assert(offsetof(canview_heartbeat_payload_t, reserved0_le) == 42U, "canview_heartbeat_payload_t.reserved0_le offset");
static_assert(sizeof(canview_ack_payload_t) == 20U, "canview_ack_payload_t wire size");
static_assert(offsetof(canview_ack_payload_t, acknowledged_sequence_le) == 0U, "canview_ack_payload_t.acknowledged_sequence_le offset");
static_assert(offsetof(canview_ack_payload_t, request_token_le) == 4U, "canview_ack_payload_t.request_token_le offset");
static_assert(offsetof(canview_ack_payload_t, status_le) == 12U, "canview_ack_payload_t.status_le offset");
static_assert(offsetof(canview_ack_payload_t, detail_le) == 14U, "canview_ack_payload_t.detail_le offset");
static_assert(offsetof(canview_ack_payload_t, receiver_time_ms_le) == 16U, "canview_ack_payload_t.receiver_time_ms_le offset");
static_assert(sizeof(canview_error_payload_t) == 16U, "canview_error_payload_t wire size");
static_assert(offsetof(canview_error_payload_t, code_le) == 0U, "canview_error_payload_t.code_le offset");
static_assert(offsetof(canview_error_payload_t, severity) == 2U, "canview_error_payload_t.severity offset");
static_assert(offsetof(canview_error_payload_t, origin) == 3U, "canview_error_payload_t.origin offset");
static_assert(offsetof(canview_error_payload_t, offending_message_type) == 4U, "canview_error_payload_t.offending_message_type offset");
static_assert(offsetof(canview_error_payload_t, reserved0) == 5U, "canview_error_payload_t.reserved0 offset");
static_assert(offsetof(canview_error_payload_t, detail_le) == 6U, "canview_error_payload_t.detail_le offset");
static_assert(offsetof(canview_error_payload_t, offending_sequence_le) == 8U, "canview_error_payload_t.offending_sequence_le offset");
static_assert(offsetof(canview_error_payload_t, retry_after_ms_le) == 12U, "canview_error_payload_t.retry_after_ms_le offset");
static_assert(sizeof(canview_state_snapshot_request_payload_t) == 24U, "canview_state_snapshot_request_payload_t wire size");
static_assert(offsetof(canview_state_snapshot_request_payload_t, request_token_le) == 0U, "canview_state_snapshot_request_payload_t.request_token_le offset");
static_assert(offsetof(canview_state_snapshot_request_payload_t, known_state_revision_le) == 8U, "canview_state_snapshot_request_payload_t.known_state_revision_le offset");
static_assert(offsetof(canview_state_snapshot_request_payload_t, known_boot_id_le) == 12U, "canview_state_snapshot_request_payload_t.known_boot_id_le offset");
static_assert(offsetof(canview_state_snapshot_request_payload_t, reserved0_le) == 20U, "canview_state_snapshot_request_payload_t.reserved0_le offset");
static_assert(sizeof(canview_state_snapshot_payload_t) == 60U, "canview_state_snapshot_payload_t wire size");
static_assert(offsetof(canview_state_snapshot_payload_t, stm_boot_id_le) == 0U, "canview_state_snapshot_payload_t.stm_boot_id_le offset");
static_assert(offsetof(canview_state_snapshot_payload_t, state_revision_le) == 8U, "canview_state_snapshot_payload_t.state_revision_le offset");
static_assert(offsetof(canview_state_snapshot_payload_t, profile_revision_le) == 12U, "canview_state_snapshot_payload_t.profile_revision_le offset");
static_assert(offsetof(canview_state_snapshot_payload_t, profile_digest) == 16U, "canview_state_snapshot_payload_t.profile_digest offset");
static_assert(offsetof(canview_state_snapshot_payload_t, build_mode) == 32U, "canview_state_snapshot_payload_t.build_mode offset");
static_assert(offsetof(canview_state_snapshot_payload_t, tx_gate) == 33U, "canview_state_snapshot_payload_t.tx_gate offset");
static_assert(offsetof(canview_state_snapshot_payload_t, active_bus_mask) == 34U, "canview_state_snapshot_payload_t.active_bus_mask offset");
static_assert(offsetof(canview_state_snapshot_payload_t, bus_error_mask) == 35U, "canview_state_snapshot_payload_t.bus_error_mask offset");
static_assert(offsetof(canview_state_snapshot_payload_t, control_lease_state) == 36U, "canview_state_snapshot_payload_t.control_lease_state offset");
static_assert(offsetof(canview_state_snapshot_payload_t, link_state) == 37U, "canview_state_snapshot_payload_t.link_state offset");
static_assert(offsetof(canview_state_snapshot_payload_t, audio_quality) == 38U, "canview_state_snapshot_payload_t.audio_quality offset");
static_assert(offsetof(canview_state_snapshot_payload_t, sport_state) == 39U, "canview_state_snapshot_payload_t.sport_state offset");
static_assert(offsetof(canview_state_snapshot_payload_t, safety_inhibit_le) == 40U, "canview_state_snapshot_payload_t.safety_inhibit_le offset");
static_assert(offsetof(canview_state_snapshot_payload_t, reserved0_le) == 42U, "canview_state_snapshot_payload_t.reserved0_le offset");
static_assert(offsetof(canview_state_snapshot_payload_t, audio_snapshot_revision_le) == 44U, "canview_state_snapshot_payload_t.audio_snapshot_revision_le offset");
static_assert(offsetof(canview_state_snapshot_payload_t, completed_time_ms_le) == 48U, "canview_state_snapshot_payload_t.completed_time_ms_le offset");
static_assert(offsetof(canview_state_snapshot_payload_t, vehicle_profile_id_le) == 52U, "canview_state_snapshot_payload_t.vehicle_profile_id_le offset");
static_assert(offsetof(canview_state_snapshot_payload_t, control_generation_le) == 56U, "canview_state_snapshot_payload_t.control_generation_le offset");
static_assert(sizeof(canview_can_batch_header_t) == 12U, "canview_can_batch_header_t wire size");
static_assert(offsetof(canview_can_batch_header_t, base_time_us_le) == 0U, "canview_can_batch_header_t.base_time_us_le offset");
static_assert(offsetof(canview_can_batch_header_t, count) == 8U, "canview_can_batch_header_t.count offset");
static_assert(offsetof(canview_can_batch_header_t, dropped_since_last) == 9U, "canview_can_batch_header_t.dropped_since_last offset");
static_assert(offsetof(canview_can_batch_header_t, reserved_le) == 10U, "canview_can_batch_header_t.reserved_le offset");
static_assert(sizeof(canview_can_record_t) == 16U, "canview_can_record_t wire size");
static_assert(offsetof(canview_can_record_t, delta_us_le) == 0U, "canview_can_record_t.delta_us_le offset");
static_assert(offsetof(canview_can_record_t, bus_id) == 2U, "canview_can_record_t.bus_id offset");
static_assert(offsetof(canview_can_record_t, flags_dlc) == 3U, "canview_can_record_t.flags_dlc offset");
static_assert(offsetof(canview_can_record_t, can_id_le) == 4U, "canview_can_record_t.can_id_le offset");
static_assert(offsetof(canview_can_record_t, data) == 8U, "canview_can_record_t.data offset");
static_assert(sizeof(canview_signal_batch_header_t) == 16U, "canview_signal_batch_header_t wire size");
static_assert(offsetof(canview_signal_batch_header_t, sample_time_us_le) == 0U, "canview_signal_batch_header_t.sample_time_us_le offset");
static_assert(offsetof(canview_signal_batch_header_t, count) == 8U, "canview_signal_batch_header_t.count offset");
static_assert(offsetof(canview_signal_batch_header_t, reserved0) == 9U, "canview_signal_batch_header_t.reserved0 offset");
static_assert(offsetof(canview_signal_batch_header_t, catalog_revision_le) == 12U, "canview_signal_batch_header_t.catalog_revision_le offset");
static_assert(sizeof(canview_signal_record_t) == 12U, "canview_signal_record_t wire size");
static_assert(offsetof(canview_signal_record_t, signal_id_le) == 0U, "canview_signal_record_t.signal_id_le offset");
static_assert(offsetof(canview_signal_record_t, value_type) == 2U, "canview_signal_record_t.value_type offset");
static_assert(offsetof(canview_signal_record_t, quality) == 3U, "canview_signal_record_t.quality offset");
static_assert(offsetof(canview_signal_record_t, age_ms_le) == 4U, "canview_signal_record_t.age_ms_le offset");
static_assert(offsetof(canview_signal_record_t, evidence_grade) == 6U, "canview_signal_record_t.evidence_grade offset");
static_assert(offsetof(canview_signal_record_t, reserved) == 7U, "canview_signal_record_t.reserved offset");
static_assert(offsetof(canview_signal_record_t, value_bits_le) == 8U, "canview_signal_record_t.value_bits_le offset");
static_assert(sizeof(canview_bus_status_header_t) == 12U, "canview_bus_status_header_t wire size");
static_assert(offsetof(canview_bus_status_header_t, boot_id_le) == 0U, "canview_bus_status_header_t.boot_id_le offset");
static_assert(offsetof(canview_bus_status_header_t, count) == 8U, "canview_bus_status_header_t.count offset");
static_assert(offsetof(canview_bus_status_header_t, reserved0) == 9U, "canview_bus_status_header_t.reserved0 offset");
static_assert(sizeof(canview_bus_status_record_t) == 36U, "canview_bus_status_record_t wire size");
static_assert(offsetof(canview_bus_status_record_t, bus_id) == 0U, "canview_bus_status_record_t.bus_id offset");
static_assert(offsetof(canview_bus_status_record_t, state) == 1U, "canview_bus_status_record_t.state offset");
static_assert(offsetof(canview_bus_status_record_t, flags_le) == 2U, "canview_bus_status_record_t.flags_le offset");
static_assert(offsetof(canview_bus_status_record_t, bitrate_le) == 4U, "canview_bus_status_record_t.bitrate_le offset");
static_assert(offsetof(canview_bus_status_record_t, error_count_le) == 8U, "canview_bus_status_record_t.error_count_le offset");
static_assert(offsetof(canview_bus_status_record_t, rx_count_le) == 16U, "canview_bus_status_record_t.rx_count_le offset");
static_assert(offsetof(canview_bus_status_record_t, tx_inhibit) == 24U, "canview_bus_status_record_t.tx_inhibit offset");
static_assert(offsetof(canview_bus_status_record_t, reserved0) == 25U, "canview_bus_status_record_t.reserved0 offset");
static_assert(offsetof(canview_bus_status_record_t, bus_off_count_le) == 28U, "canview_bus_status_record_t.bus_off_count_le offset");
static_assert(sizeof(canview_can_filter_get_payload_t) == 8U, "canview_can_filter_get_payload_t wire size");
static_assert(offsetof(canview_can_filter_get_payload_t, config_revision_le) == 0U, "canview_can_filter_get_payload_t.config_revision_le offset");
static_assert(offsetof(canview_can_filter_get_payload_t, filter_id_le) == 4U, "canview_can_filter_get_payload_t.filter_id_le offset");
static_assert(sizeof(canview_can_filter_batch_header_t) == 8U, "canview_can_filter_batch_header_t wire size");
static_assert(offsetof(canview_can_filter_batch_header_t, action) == 0U, "canview_can_filter_batch_header_t.action offset");
static_assert(offsetof(canview_can_filter_batch_header_t, count) == 1U, "canview_can_filter_batch_header_t.count offset");
static_assert(offsetof(canview_can_filter_batch_header_t, reserved_le) == 2U, "canview_can_filter_batch_header_t.reserved_le offset");
static_assert(offsetof(canview_can_filter_batch_header_t, config_revision_le) == 4U, "canview_can_filter_batch_header_t.config_revision_le offset");
static_assert(sizeof(canview_can_filter_t) == 22U, "canview_can_filter_t wire size");
static_assert(offsetof(canview_can_filter_t, filter_id_le) == 0U, "canview_can_filter_t.filter_id_le offset");
static_assert(offsetof(canview_can_filter_t, can_id_le) == 4U, "canview_can_filter_t.can_id_le offset");
static_assert(offsetof(canview_can_filter_t, can_id_mask_le) == 8U, "canview_can_filter_t.can_id_mask_le offset");
static_assert(offsetof(canview_can_filter_t, period_ms_le) == 12U, "canview_can_filter_t.period_ms_le offset");
static_assert(offsetof(canview_can_filter_t, bus_id) == 14U, "canview_can_filter_t.bus_id offset");
static_assert(offsetof(canview_can_filter_t, flags_value) == 15U, "canview_can_filter_t.flags_value offset");
static_assert(offsetof(canview_can_filter_t, flags_mask) == 16U, "canview_can_filter_t.flags_mask offset");
static_assert(offsetof(canview_can_filter_t, min_dlc) == 17U, "canview_can_filter_t.min_dlc offset");
static_assert(offsetof(canview_can_filter_t, max_dlc) == 18U, "canview_can_filter_t.max_dlc offset");
static_assert(offsetof(canview_can_filter_t, max_records_per_period) == 19U, "canview_can_filter_t.max_records_per_period offset");
static_assert(offsetof(canview_can_filter_t, enabled) == 20U, "canview_can_filter_t.enabled offset");
static_assert(offsetof(canview_can_filter_t, reserved0) == 21U, "canview_can_filter_t.reserved0 offset");
static_assert(sizeof(canview_can_filter_result_payload_t) == 12U, "canview_can_filter_result_payload_t wire size");
static_assert(offsetof(canview_can_filter_result_payload_t, config_revision_le) == 0U, "canview_can_filter_result_payload_t.config_revision_le offset");
static_assert(offsetof(canview_can_filter_result_payload_t, filter_id_le) == 4U, "canview_can_filter_result_payload_t.filter_id_le offset");
static_assert(offsetof(canview_can_filter_result_payload_t, action) == 8U, "canview_can_filter_result_payload_t.action offset");
static_assert(offsetof(canview_can_filter_result_payload_t, result) == 9U, "canview_can_filter_result_payload_t.result offset");
static_assert(offsetof(canview_can_filter_result_payload_t, detail_le) == 10U, "canview_can_filter_result_payload_t.detail_le offset");
static_assert(sizeof(canview_can_stream_config_t) == 16U, "canview_can_stream_config_t wire size");
static_assert(offsetof(canview_can_stream_config_t, config_revision_le) == 0U, "canview_can_stream_config_t.config_revision_le offset");
static_assert(offsetof(canview_can_stream_config_t, period_ms_le) == 4U, "canview_can_stream_config_t.period_ms_le offset");
static_assert(offsetof(canview_can_stream_config_t, max_records_per_period) == 6U, "canview_can_stream_config_t.max_records_per_period offset");
static_assert(offsetof(canview_can_stream_config_t, enabled) == 7U, "canview_can_stream_config_t.enabled offset");
static_assert(offsetof(canview_can_stream_config_t, max_bytes_per_second_le) == 8U, "canview_can_stream_config_t.max_bytes_per_second_le offset");
static_assert(offsetof(canview_can_stream_config_t, burst_bytes_le) == 12U, "canview_can_stream_config_t.burst_bytes_le offset");
static_assert(offsetof(canview_can_stream_config_t, reserved_le) == 14U, "canview_can_stream_config_t.reserved_le offset");
static_assert(sizeof(canview_can_stream_status_t) == 16U, "canview_can_stream_status_t wire size");
static_assert(offsetof(canview_can_stream_status_t, config_revision_le) == 0U, "canview_can_stream_status_t.config_revision_le offset");
static_assert(offsetof(canview_can_stream_status_t, accepted_records_le) == 4U, "canview_can_stream_status_t.accepted_records_le offset");
static_assert(offsetof(canview_can_stream_status_t, rejected_records_le) == 6U, "canview_can_stream_status_t.rejected_records_le offset");
static_assert(offsetof(canview_can_stream_status_t, dropped_by_budget_le) == 8U, "canview_can_stream_status_t.dropped_by_budget_le offset");
static_assert(offsetof(canview_can_stream_status_t, reserved_le) == 12U, "canview_can_stream_status_t.reserved_le offset");
static_assert(sizeof(canview_observer_config_payload_t) == 40U, "canview_observer_config_payload_t wire size");
static_assert(offsetof(canview_observer_config_payload_t, request_token_le) == 0U, "canview_observer_config_payload_t.request_token_le offset");
static_assert(offsetof(canview_observer_config_payload_t, expected_observer_revision_le) == 8U, "canview_observer_config_payload_t.expected_observer_revision_le offset");
static_assert(offsetof(canview_observer_config_payload_t, filter_revision_le) == 12U, "canview_observer_config_payload_t.filter_revision_le offset");
static_assert(offsetof(canview_observer_config_payload_t, mode) == 16U, "canview_observer_config_payload_t.mode offset");
static_assert(offsetof(canview_observer_config_payload_t, bus_mask) == 17U, "canview_observer_config_payload_t.bus_mask offset");
static_assert(offsetof(canview_observer_config_payload_t, flags_le) == 18U, "canview_observer_config_payload_t.flags_le offset");
static_assert(offsetof(canview_observer_config_payload_t, stats_period_ms_le) == 20U, "canview_observer_config_payload_t.stats_period_ms_le offset");
static_assert(offsetof(canview_observer_config_payload_t, max_records_per_second_le) == 22U, "canview_observer_config_payload_t.max_records_per_second_le offset");
static_assert(offsetof(canview_observer_config_payload_t, max_bytes_per_second_le) == 24U, "canview_observer_config_payload_t.max_bytes_per_second_le offset");
static_assert(offsetof(canview_observer_config_payload_t, duration_ms_le) == 28U, "canview_observer_config_payload_t.duration_ms_le offset");
static_assert(offsetof(canview_observer_config_payload_t, pretrigger_ms_le) == 32U, "canview_observer_config_payload_t.pretrigger_ms_le offset");
static_assert(offsetof(canview_observer_config_payload_t, posttrigger_ms_le) == 34U, "canview_observer_config_payload_t.posttrigger_ms_le offset");
static_assert(offsetof(canview_observer_config_payload_t, reserved0_le) == 36U, "canview_observer_config_payload_t.reserved0_le offset");
static_assert(sizeof(canview_can_id_stats_header_t) == 8U, "canview_can_id_stats_header_t wire size");
static_assert(offsetof(canview_can_id_stats_header_t, window_end_ms_le) == 0U, "canview_can_id_stats_header_t.window_end_ms_le offset");
static_assert(offsetof(canview_can_id_stats_header_t, count) == 4U, "canview_can_id_stats_header_t.count offset");
static_assert(offsetof(canview_can_id_stats_header_t, part_index) == 5U, "canview_can_id_stats_header_t.part_index offset");
static_assert(offsetof(canview_can_id_stats_header_t, part_count) == 6U, "canview_can_id_stats_header_t.part_count offset");
static_assert(offsetof(canview_can_id_stats_header_t, reserved0) == 7U, "canview_can_id_stats_header_t.reserved0 offset");
static_assert(sizeof(canview_can_id_stats_record_t) == 40U, "canview_can_id_stats_record_t wire size");
static_assert(offsetof(canview_can_id_stats_record_t, bus_id) == 0U, "canview_can_id_stats_record_t.bus_id offset");
static_assert(offsetof(canview_can_id_stats_record_t, flags_dlc) == 1U, "canview_can_id_stats_record_t.flags_dlc offset");
static_assert(offsetof(canview_can_id_stats_record_t, rate_tenth_hz_le) == 2U, "canview_can_id_stats_record_t.rate_tenth_hz_le offset");
static_assert(offsetof(canview_can_id_stats_record_t, can_id_le) == 4U, "canview_can_id_stats_record_t.can_id_le offset");
static_assert(offsetof(canview_can_id_stats_record_t, frame_count_le) == 8U, "canview_can_id_stats_record_t.frame_count_le offset");
static_assert(offsetof(canview_can_id_stats_record_t, change_count_le) == 12U, "canview_can_id_stats_record_t.change_count_le offset");
static_assert(offsetof(canview_can_id_stats_record_t, period_p50_us_le) == 16U, "canview_can_id_stats_record_t.period_p50_us_le offset");
static_assert(offsetof(canview_can_id_stats_record_t, period_p95_us_le) == 20U, "canview_can_id_stats_record_t.period_p95_us_le offset");
static_assert(offsetof(canview_can_id_stats_record_t, bit_change_mask_le) == 24U, "canview_can_id_stats_record_t.bit_change_mask_le offset");
static_assert(offsetof(canview_can_id_stats_record_t, last_data) == 32U, "canview_can_id_stats_record_t.last_data offset");
static_assert(sizeof(canview_capture_control_payload_t) == 28U, "canview_capture_control_payload_t wire size");
static_assert(offsetof(canview_capture_control_payload_t, request_token_le) == 0U, "canview_capture_control_payload_t.request_token_le offset");
static_assert(offsetof(canview_capture_control_payload_t, capture_id_le) == 8U, "canview_capture_control_payload_t.capture_id_le offset");
static_assert(offsetof(canview_capture_control_payload_t, action) == 16U, "canview_capture_control_payload_t.action offset");
static_assert(offsetof(canview_capture_control_payload_t, reserved0) == 17U, "canview_capture_control_payload_t.reserved0 offset");
static_assert(offsetof(canview_capture_control_payload_t, flags_le) == 18U, "canview_capture_control_payload_t.flags_le offset");
static_assert(offsetof(canview_capture_control_payload_t, requested_time_ms_le) == 20U, "canview_capture_control_payload_t.requested_time_ms_le offset");
static_assert(offsetof(canview_capture_control_payload_t, reserved1_le) == 24U, "canview_capture_control_payload_t.reserved1_le offset");
static_assert(sizeof(canview_capture_status_payload_t) == 44U, "canview_capture_status_payload_t wire size");
static_assert(offsetof(canview_capture_status_payload_t, request_token_le) == 0U, "canview_capture_status_payload_t.request_token_le offset");
static_assert(offsetof(canview_capture_status_payload_t, capture_id_le) == 8U, "canview_capture_status_payload_t.capture_id_le offset");
static_assert(offsetof(canview_capture_status_payload_t, state) == 16U, "canview_capture_status_payload_t.state offset");
static_assert(offsetof(canview_capture_status_payload_t, reason_le) == 17U, "canview_capture_status_payload_t.reason_le offset");
static_assert(offsetof(canview_capture_status_payload_t, bus_mask) == 19U, "canview_capture_status_payload_t.bus_mask offset");
static_assert(offsetof(canview_capture_status_payload_t, accepted_records_le) == 20U, "canview_capture_status_payload_t.accepted_records_le offset");
static_assert(offsetof(canview_capture_status_payload_t, dropped_records_le) == 24U, "canview_capture_status_payload_t.dropped_records_le offset");
static_assert(offsetof(canview_capture_status_payload_t, stored_bytes_le) == 28U, "canview_capture_status_payload_t.stored_bytes_le offset");
static_assert(offsetof(canview_capture_status_payload_t, remaining_ms_le) == 32U, "canview_capture_status_payload_t.remaining_ms_le offset");
static_assert(offsetof(canview_capture_status_payload_t, effective_filter_revision_le) == 36U, "canview_capture_status_payload_t.effective_filter_revision_le offset");
static_assert(offsetof(canview_capture_status_payload_t, reserved1_le) == 40U, "canview_capture_status_payload_t.reserved1_le offset");
static_assert(sizeof(canview_event_marker_payload_t) == 28U, "canview_event_marker_payload_t wire size");
static_assert(offsetof(canview_event_marker_payload_t, capture_id_le) == 0U, "canview_event_marker_payload_t.capture_id_le offset");
static_assert(offsetof(canview_event_marker_payload_t, marker_id_le) == 8U, "canview_event_marker_payload_t.marker_id_le offset");
static_assert(offsetof(canview_event_marker_payload_t, sender_time_ms_le) == 12U, "canview_event_marker_payload_t.sender_time_ms_le offset");
static_assert(offsetof(canview_event_marker_payload_t, marker_kind_le) == 16U, "canview_event_marker_payload_t.marker_kind_le offset");
static_assert(offsetof(canview_event_marker_payload_t, flags_le) == 18U, "canview_event_marker_payload_t.flags_le offset");
static_assert(offsetof(canview_event_marker_payload_t, label_code_le) == 20U, "canview_event_marker_payload_t.label_code_le offset");
static_assert(offsetof(canview_event_marker_payload_t, reserved0_le) == 22U, "canview_event_marker_payload_t.reserved0_le offset");
static_assert(offsetof(canview_event_marker_payload_t, time_uncertainty_us_le) == 24U, "canview_event_marker_payload_t.time_uncertainty_us_le offset");
static_assert(sizeof(canview_diagnostic_lease_request_t) == 24U, "canview_diagnostic_lease_request_t wire size");
static_assert(offsetof(canview_diagnostic_lease_request_t, request_token_le) == 0U, "canview_diagnostic_lease_request_t.request_token_le offset");
static_assert(offsetof(canview_diagnostic_lease_request_t, action) == 8U, "canview_diagnostic_lease_request_t.action offset");
static_assert(offsetof(canview_diagnostic_lease_request_t, reserved0) == 9U, "canview_diagnostic_lease_request_t.reserved0 offset");
static_assert(offsetof(canview_diagnostic_lease_request_t, requested_ms_le) == 12U, "canview_diagnostic_lease_request_t.requested_ms_le offset");
static_assert(offsetof(canview_diagnostic_lease_request_t, lease_id_le) == 16U, "canview_diagnostic_lease_request_t.lease_id_le offset");
static_assert(sizeof(canview_diagnostic_lease_response_t) == 36U, "canview_diagnostic_lease_response_t wire size");
static_assert(offsetof(canview_diagnostic_lease_response_t, request_token_le) == 0U, "canview_diagnostic_lease_response_t.request_token_le offset");
static_assert(offsetof(canview_diagnostic_lease_response_t, lease_id_le) == 8U, "canview_diagnostic_lease_response_t.lease_id_le offset");
static_assert(offsetof(canview_diagnostic_lease_response_t, status) == 16U, "canview_diagnostic_lease_response_t.status offset");
static_assert(offsetof(canview_diagnostic_lease_response_t, reserved0) == 17U, "canview_diagnostic_lease_response_t.reserved0 offset");
static_assert(offsetof(canview_diagnostic_lease_response_t, granted_ms_le) == 20U, "canview_diagnostic_lease_response_t.granted_ms_le offset");
static_assert(offsetof(canview_diagnostic_lease_response_t, expires_at_ms_le) == 24U, "canview_diagnostic_lease_response_t.expires_at_ms_le offset");
static_assert(offsetof(canview_diagnostic_lease_response_t, owner_device_id_le) == 28U, "canview_diagnostic_lease_response_t.owner_device_id_le offset");
static_assert(sizeof(canview_command_request_t) == 72U, "canview_command_request_t wire size");
static_assert(offsetof(canview_command_request_t, request_token_le) == 0U, "canview_command_request_t.request_token_le offset");
static_assert(offsetof(canview_command_request_t, command_id_le) == 8U, "canview_command_request_t.command_id_le offset");
static_assert(offsetof(canview_command_request_t, ttl_ms_le) == 10U, "canview_command_request_t.ttl_ms_le offset");
static_assert(offsetof(canview_command_request_t, origin_device_id_le) == 12U, "canview_command_request_t.origin_device_id_le offset");
static_assert(offsetof(canview_command_request_t, origin_boot_id_le) == 20U, "canview_command_request_t.origin_boot_id_le offset");
static_assert(offsetof(canview_command_request_t, wireless_session_id_le) == 28U, "canview_command_request_t.wireless_session_id_le offset");
static_assert(offsetof(canview_command_request_t, control_generation_le) == 32U, "canview_command_request_t.control_generation_le offset");
static_assert(offsetof(canview_command_request_t, issued_at_controller_ms_le) == 36U, "canview_command_request_t.issued_at_controller_ms_le offset");
static_assert(offsetof(canview_command_request_t, control_sync_generation_le) == 40U, "canview_command_request_t.control_sync_generation_le offset");
static_assert(offsetof(canview_command_request_t, expected_state_revision_le) == 44U, "canview_command_request_t.expected_state_revision_le offset");
static_assert(offsetof(canview_command_request_t, precondition_flags_le) == 48U, "canview_command_request_t.precondition_flags_le offset");
static_assert(offsetof(canview_command_request_t, argument_tlv_length_le) == 52U, "canview_command_request_t.argument_tlv_length_le offset");
static_assert(offsetof(canview_command_request_t, reserved_le) == 54U, "canview_command_request_t.reserved_le offset");
static_assert(offsetof(canview_command_request_t, control_tag) == 56U, "canview_command_request_t.control_tag offset");
static_assert(sizeof(canview_command_result_t) == 50U, "canview_command_result_t wire size");
static_assert(offsetof(canview_command_result_t, request_token_le) == 0U, "canview_command_result_t.request_token_le offset");
static_assert(offsetof(canview_command_result_t, command_id_le) == 8U, "canview_command_result_t.command_id_le offset");
static_assert(offsetof(canview_command_result_t, stage) == 10U, "canview_command_result_t.stage offset");
static_assert(offsetof(canview_command_result_t, reserved0) == 11U, "canview_command_result_t.reserved0 offset");
static_assert(offsetof(canview_command_result_t, reason_le) == 12U, "canview_command_result_t.reason_le offset");
static_assert(offsetof(canview_command_result_t, state_revision_le) == 14U, "canview_command_result_t.state_revision_le offset");
static_assert(offsetof(canview_command_result_t, completed_time_ms_le) == 18U, "canview_command_result_t.completed_time_ms_le offset");
static_assert(offsetof(canview_command_result_t, request_sequence_le) == 22U, "canview_command_result_t.request_sequence_le offset");
static_assert(offsetof(canview_command_result_t, control_tag) == 26U, "canview_command_result_t.control_tag offset");
static_assert(offsetof(canview_command_result_t, feedback_revision_le) == 42U, "canview_command_result_t.feedback_revision_le offset");
static_assert(offsetof(canview_command_result_t, feedback_time_ms_le) == 46U, "canview_command_result_t.feedback_time_ms_le offset");
static_assert(sizeof(canview_control_lease_request_t) == 52U, "canview_control_lease_request_t wire size");
static_assert(offsetof(canview_control_lease_request_t, request_token_le) == 0U, "canview_control_lease_request_t.request_token_le offset");
static_assert(offsetof(canview_control_lease_request_t, action) == 8U, "canview_control_lease_request_t.action offset");
static_assert(offsetof(canview_control_lease_request_t, reserved0) == 9U, "canview_control_lease_request_t.reserved0 offset");
static_assert(offsetof(canview_control_lease_request_t, requested_ms_le) == 12U, "canview_control_lease_request_t.requested_ms_le offset");
static_assert(offsetof(canview_control_lease_request_t, lease_id_le) == 16U, "canview_control_lease_request_t.lease_id_le offset");
static_assert(offsetof(canview_control_lease_request_t, requested_scope_le) == 24U, "canview_control_lease_request_t.requested_scope_le offset");
static_assert(offsetof(canview_control_lease_request_t, reserved1_le) == 26U, "canview_control_lease_request_t.reserved1_le offset");
static_assert(offsetof(canview_control_lease_request_t, expected_state_revision_le) == 28U, "canview_control_lease_request_t.expected_state_revision_le offset");
static_assert(offsetof(canview_control_lease_request_t, control_generation_le) == 32U, "canview_control_lease_request_t.control_generation_le offset");
static_assert(offsetof(canview_control_lease_request_t, control_tag) == 36U, "canview_control_lease_request_t.control_tag offset");
static_assert(sizeof(canview_control_lease_status_t) == 60U, "canview_control_lease_status_t wire size");
static_assert(offsetof(canview_control_lease_status_t, request_token_le) == 0U, "canview_control_lease_status_t.request_token_le offset");
static_assert(offsetof(canview_control_lease_status_t, lease_id_le) == 8U, "canview_control_lease_status_t.lease_id_le offset");
static_assert(offsetof(canview_control_lease_status_t, status) == 16U, "canview_control_lease_status_t.status offset");
static_assert(offsetof(canview_control_lease_status_t, reserved0) == 17U, "canview_control_lease_status_t.reserved0 offset");
static_assert(offsetof(canview_control_lease_status_t, granted_ms_le) == 20U, "canview_control_lease_status_t.granted_ms_le offset");
static_assert(offsetof(canview_control_lease_status_t, expires_at_ms_le) == 24U, "canview_control_lease_status_t.expires_at_ms_le offset");
static_assert(offsetof(canview_control_lease_status_t, owner_device_id_le) == 28U, "canview_control_lease_status_t.owner_device_id_le offset");
static_assert(offsetof(canview_control_lease_status_t, granted_scope_le) == 36U, "canview_control_lease_status_t.granted_scope_le offset");
static_assert(offsetof(canview_control_lease_status_t, reserved1_le) == 38U, "canview_control_lease_status_t.reserved1_le offset");
static_assert(offsetof(canview_control_lease_status_t, control_generation_le) == 40U, "canview_control_lease_status_t.control_generation_le offset");
static_assert(offsetof(canview_control_lease_status_t, control_tag) == 44U, "canview_control_lease_status_t.control_tag offset");
static_assert(sizeof(canview_config_get_payload_t) == 24U, "canview_config_get_payload_t wire size");
static_assert(offsetof(canview_config_get_payload_t, request_token_le) == 0U, "canview_config_get_payload_t.request_token_le offset");
static_assert(offsetof(canview_config_get_payload_t, expected_state_revision_le) == 8U, "canview_config_get_payload_t.expected_state_revision_le offset");
static_assert(offsetof(canview_config_get_payload_t, schema_version_le) == 12U, "canview_config_get_payload_t.schema_version_le offset");
static_assert(offsetof(canview_config_get_payload_t, reserved0_le) == 14U, "canview_config_get_payload_t.reserved0_le offset");
static_assert(offsetof(canview_config_get_payload_t, key_le) == 16U, "canview_config_get_payload_t.key_le offset");
static_assert(offsetof(canview_config_get_payload_t, reserved1_le) == 18U, "canview_config_get_payload_t.reserved1_le offset");
static_assert(offsetof(canview_config_get_payload_t, known_revision_le) == 20U, "canview_config_get_payload_t.known_revision_le offset");
static_assert(sizeof(canview_config_batch_header_t) == 4U, "canview_config_batch_header_t wire size");
static_assert(offsetof(canview_config_batch_header_t, schema_version_le) == 0U, "canview_config_batch_header_t.schema_version_le offset");
static_assert(offsetof(canview_config_batch_header_t, count) == 2U, "canview_config_batch_header_t.count offset");
static_assert(offsetof(canview_config_batch_header_t, reserved) == 3U, "canview_config_batch_header_t.reserved offset");
static_assert(sizeof(canview_config_record_t) == 8U, "canview_config_record_t wire size");
static_assert(offsetof(canview_config_record_t, key_le) == 0U, "canview_config_record_t.key_le offset");
static_assert(offsetof(canview_config_record_t, value_type) == 2U, "canview_config_record_t.value_type offset");
static_assert(offsetof(canview_config_record_t, reserved) == 3U, "canview_config_record_t.reserved offset");
static_assert(offsetof(canview_config_record_t, value_bits_le) == 4U, "canview_config_record_t.value_bits_le offset");
static_assert(sizeof(canview_config_result_payload_t) == 28U, "canview_config_result_payload_t wire size");
static_assert(offsetof(canview_config_result_payload_t, request_token_le) == 0U, "canview_config_result_payload_t.request_token_le offset");
static_assert(offsetof(canview_config_result_payload_t, stage) == 8U, "canview_config_result_payload_t.stage offset");
static_assert(offsetof(canview_config_result_payload_t, applied_count) == 9U, "canview_config_result_payload_t.applied_count offset");
static_assert(offsetof(canview_config_result_payload_t, pending_count) == 10U, "canview_config_result_payload_t.pending_count offset");
static_assert(offsetof(canview_config_result_payload_t, reserved0) == 11U, "canview_config_result_payload_t.reserved0 offset");
static_assert(offsetof(canview_config_result_payload_t, reason_le) == 12U, "canview_config_result_payload_t.reason_le offset");
static_assert(offsetof(canview_config_result_payload_t, reserved1_le) == 14U, "canview_config_result_payload_t.reserved1_le offset");
static_assert(offsetof(canview_config_result_payload_t, state_revision_le) == 16U, "canview_config_result_payload_t.state_revision_le offset");
static_assert(offsetof(canview_config_result_payload_t, completed_time_ms_le) == 20U, "canview_config_result_payload_t.completed_time_ms_le offset");
static_assert(offsetof(canview_config_result_payload_t, detail_le) == 24U, "canview_config_result_payload_t.detail_le offset");
static_assert(sizeof(canview_config_schema_request_payload_t) == 16U, "canview_config_schema_request_payload_t wire size");
static_assert(offsetof(canview_config_schema_request_payload_t, request_token_le) == 0U, "canview_config_schema_request_payload_t.request_token_le offset");
static_assert(offsetof(canview_config_schema_request_payload_t, known_schema_version_le) == 8U, "canview_config_schema_request_payload_t.known_schema_version_le offset");
static_assert(offsetof(canview_config_schema_request_payload_t, reserved0_le) == 10U, "canview_config_schema_request_payload_t.reserved0_le offset");
static_assert(offsetof(canview_config_schema_request_payload_t, known_digest_prefix_le) == 12U, "canview_config_schema_request_payload_t.known_digest_prefix_le offset");
static_assert(sizeof(canview_remote_config_request_prefix_t) == 20U, "canview_remote_config_request_prefix_t wire size");
static_assert(offsetof(canview_remote_config_request_prefix_t, request_token_le) == 0U, "canview_remote_config_request_prefix_t.request_token_le offset");
static_assert(offsetof(canview_remote_config_request_prefix_t, expected_state_revision_le) == 8U, "canview_remote_config_request_prefix_t.expected_state_revision_le offset");
static_assert(offsetof(canview_remote_config_request_prefix_t, schema_version_le) == 12U, "canview_remote_config_request_prefix_t.schema_version_le offset");
static_assert(offsetof(canview_remote_config_request_prefix_t, count) == 14U, "canview_remote_config_request_prefix_t.count offset");
static_assert(offsetof(canview_remote_config_request_prefix_t, flags) == 15U, "canview_remote_config_request_prefix_t.flags offset");
static_assert(offsetof(canview_remote_config_request_prefix_t, ttl_ms_le) == 16U, "canview_remote_config_request_prefix_t.ttl_ms_le offset");
static_assert(offsetof(canview_remote_config_request_prefix_t, reserved0_le) == 18U, "canview_remote_config_request_prefix_t.reserved0_le offset");
static_assert(sizeof(canview_remote_config_status_payload_t) == 28U, "canview_remote_config_status_payload_t wire size");
static_assert(offsetof(canview_remote_config_status_payload_t, request_token_le) == 0U, "canview_remote_config_status_payload_t.request_token_le offset");
static_assert(offsetof(canview_remote_config_status_payload_t, stage) == 8U, "canview_remote_config_status_payload_t.stage offset");
static_assert(offsetof(canview_remote_config_status_payload_t, applied_count) == 9U, "canview_remote_config_status_payload_t.applied_count offset");
static_assert(offsetof(canview_remote_config_status_payload_t, pending_count) == 10U, "canview_remote_config_status_payload_t.pending_count offset");
static_assert(offsetof(canview_remote_config_status_payload_t, reserved0) == 11U, "canview_remote_config_status_payload_t.reserved0 offset");
static_assert(offsetof(canview_remote_config_status_payload_t, reason_le) == 12U, "canview_remote_config_status_payload_t.reason_le offset");
static_assert(offsetof(canview_remote_config_status_payload_t, reserved1_le) == 14U, "canview_remote_config_status_payload_t.reserved1_le offset");
static_assert(offsetof(canview_remote_config_status_payload_t, state_revision_le) == 16U, "canview_remote_config_status_payload_t.state_revision_le offset");
static_assert(offsetof(canview_remote_config_status_payload_t, completed_time_ms_le) == 20U, "canview_remote_config_status_payload_t.completed_time_ms_le offset");
static_assert(offsetof(canview_remote_config_status_payload_t, detail_le) == 24U, "canview_remote_config_status_payload_t.detail_le offset");
static_assert(sizeof(canview_diagnostic_counters_header_t) == 16U, "canview_diagnostic_counters_header_t wire size");
static_assert(offsetof(canview_diagnostic_counters_header_t, boot_id_le) == 0U, "canview_diagnostic_counters_header_t.boot_id_le offset");
static_assert(offsetof(canview_diagnostic_counters_header_t, state_revision_le) == 8U, "canview_diagnostic_counters_header_t.state_revision_le offset");
static_assert(offsetof(canview_diagnostic_counters_header_t, count) == 12U, "canview_diagnostic_counters_header_t.count offset");
static_assert(offsetof(canview_diagnostic_counters_header_t, reserved0) == 13U, "canview_diagnostic_counters_header_t.reserved0 offset");
static_assert(sizeof(canview_diagnostic_counter_record_t) == 16U, "canview_diagnostic_counter_record_t wire size");
static_assert(offsetof(canview_diagnostic_counter_record_t, counter_id_le) == 0U, "canview_diagnostic_counter_record_t.counter_id_le offset");
static_assert(offsetof(canview_diagnostic_counter_record_t, category) == 2U, "canview_diagnostic_counter_record_t.category offset");
static_assert(offsetof(canview_diagnostic_counter_record_t, reserved0) == 3U, "canview_diagnostic_counter_record_t.reserved0 offset");
static_assert(offsetof(canview_diagnostic_counter_record_t, value_le) == 4U, "canview_diagnostic_counter_record_t.value_le offset");
static_assert(offsetof(canview_diagnostic_counter_record_t, saturated) == 12U, "canview_diagnostic_counter_record_t.saturated offset");
static_assert(offsetof(canview_diagnostic_counter_record_t, reserved1) == 13U, "canview_diagnostic_counter_record_t.reserved1 offset");
static_assert(sizeof(canview_bulk_begin_payload_t) == 72U, "canview_bulk_begin_payload_t wire size");
static_assert(offsetof(canview_bulk_begin_payload_t, request_token_le) == 0U, "canview_bulk_begin_payload_t.request_token_le offset");
static_assert(offsetof(canview_bulk_begin_payload_t, object_id) == 8U, "canview_bulk_begin_payload_t.object_id offset");
static_assert(offsetof(canview_bulk_begin_payload_t, object_type_le) == 24U, "canview_bulk_begin_payload_t.object_type_le offset");
static_assert(offsetof(canview_bulk_begin_payload_t, flags_le) == 26U, "canview_bulk_begin_payload_t.flags_le offset");
static_assert(offsetof(canview_bulk_begin_payload_t, total_size_le) == 28U, "canview_bulk_begin_payload_t.total_size_le offset");
static_assert(offsetof(canview_bulk_begin_payload_t, fragment_size_le) == 32U, "canview_bulk_begin_payload_t.fragment_size_le offset");
static_assert(offsetof(canview_bulk_begin_payload_t, window_size) == 34U, "canview_bulk_begin_payload_t.window_size offset");
static_assert(offsetof(canview_bulk_begin_payload_t, reserved0) == 35U, "canview_bulk_begin_payload_t.reserved0 offset");
static_assert(offsetof(canview_bulk_begin_payload_t, sha256) == 36U, "canview_bulk_begin_payload_t.sha256 offset");
static_assert(offsetof(canview_bulk_begin_payload_t, timeout_ms_le) == 68U, "canview_bulk_begin_payload_t.timeout_ms_le offset");
static_assert(sizeof(canview_bulk_fragment_prefix_t) == 28U, "canview_bulk_fragment_prefix_t wire size");
static_assert(offsetof(canview_bulk_fragment_prefix_t, object_id) == 0U, "canview_bulk_fragment_prefix_t.object_id offset");
static_assert(offsetof(canview_bulk_fragment_prefix_t, fragment_index_le) == 16U, "canview_bulk_fragment_prefix_t.fragment_index_le offset");
static_assert(offsetof(canview_bulk_fragment_prefix_t, total_fragments_le) == 20U, "canview_bulk_fragment_prefix_t.total_fragments_le offset");
static_assert(offsetof(canview_bulk_fragment_prefix_t, payload_len_le) == 24U, "canview_bulk_fragment_prefix_t.payload_len_le offset");
static_assert(offsetof(canview_bulk_fragment_prefix_t, flags_le) == 26U, "canview_bulk_fragment_prefix_t.flags_le offset");
static_assert(sizeof(canview_bulk_ack_payload_t) == 32U, "canview_bulk_ack_payload_t wire size");
static_assert(offsetof(canview_bulk_ack_payload_t, object_id) == 0U, "canview_bulk_ack_payload_t.object_id offset");
static_assert(offsetof(canview_bulk_ack_payload_t, base_fragment_le) == 16U, "canview_bulk_ack_payload_t.base_fragment_le offset");
static_assert(offsetof(canview_bulk_ack_payload_t, received_bitmap_le) == 20U, "canview_bulk_ack_payload_t.received_bitmap_le offset");
static_assert(offsetof(canview_bulk_ack_payload_t, received_bytes_le) == 24U, "canview_bulk_ack_payload_t.received_bytes_le offset");
static_assert(offsetof(canview_bulk_ack_payload_t, window_size) == 28U, "canview_bulk_ack_payload_t.window_size offset");
static_assert(offsetof(canview_bulk_ack_payload_t, status) == 29U, "canview_bulk_ack_payload_t.status offset");
static_assert(offsetof(canview_bulk_ack_payload_t, reserved0_le) == 30U, "canview_bulk_ack_payload_t.reserved0_le offset");
static_assert(sizeof(canview_bulk_end_payload_t) == 60U, "canview_bulk_end_payload_t wire size");
static_assert(offsetof(canview_bulk_end_payload_t, object_id) == 0U, "canview_bulk_end_payload_t.object_id offset");
static_assert(offsetof(canview_bulk_end_payload_t, status) == 16U, "canview_bulk_end_payload_t.status offset");
static_assert(offsetof(canview_bulk_end_payload_t, reserved0) == 17U, "canview_bulk_end_payload_t.reserved0 offset");
static_assert(offsetof(canview_bulk_end_payload_t, total_size_le) == 20U, "canview_bulk_end_payload_t.total_size_le offset");
static_assert(offsetof(canview_bulk_end_payload_t, sha256) == 24U, "canview_bulk_end_payload_t.sha256 offset");
static_assert(offsetof(canview_bulk_end_payload_t, reason_le) == 56U, "canview_bulk_end_payload_t.reason_le offset");
static_assert(offsetof(canview_bulk_end_payload_t, reserved1_le) == 58U, "canview_bulk_end_payload_t.reserved1_le offset");
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(canview_frame_header_t) == 32U, "canview_frame_header_t wire size");
_Static_assert(offsetof(canview_frame_header_t, magic_le) == 0U, "canview_frame_header_t.magic_le offset");
_Static_assert(offsetof(canview_frame_header_t, major) == 2U, "canview_frame_header_t.major offset");
_Static_assert(offsetof(canview_frame_header_t, minor) == 3U, "canview_frame_header_t.minor offset");
_Static_assert(offsetof(canview_frame_header_t, header_len) == 4U, "canview_frame_header_t.header_len offset");
_Static_assert(offsetof(canview_frame_header_t, message_type) == 5U, "canview_frame_header_t.message_type offset");
_Static_assert(offsetof(canview_frame_header_t, flags) == 6U, "canview_frame_header_t.flags offset");
_Static_assert(offsetof(canview_frame_header_t, priority) == 7U, "canview_frame_header_t.priority offset");
_Static_assert(offsetof(canview_frame_header_t, session_id_le) == 8U, "canview_frame_header_t.session_id_le offset");
_Static_assert(offsetof(canview_frame_header_t, sequence_le) == 12U, "canview_frame_header_t.sequence_le offset");
_Static_assert(offsetof(canview_frame_header_t, sender_time_ms_le) == 16U, "canview_frame_header_t.sender_time_ms_le offset");
_Static_assert(offsetof(canview_frame_header_t, correlation_id_le) == 20U, "canview_frame_header_t.correlation_id_le offset");
_Static_assert(offsetof(canview_frame_header_t, payload_len_le) == 24U, "canview_frame_header_t.payload_len_le offset");
_Static_assert(offsetof(canview_frame_header_t, reserved_le) == 26U, "canview_frame_header_t.reserved_le offset");
_Static_assert(offsetof(canview_frame_header_t, crc32_le) == 28U, "canview_frame_header_t.crc32_le offset");
_Static_assert(sizeof(canview_tlv_header_t) == 4U, "canview_tlv_header_t wire size");
_Static_assert(offsetof(canview_tlv_header_t, type_le) == 0U, "canview_tlv_header_t.type_le offset");
_Static_assert(offsetof(canview_tlv_header_t, length_le) == 2U, "canview_tlv_header_t.length_le offset");
_Static_assert(sizeof(canview_discovery_payload_t) == 68U, "canview_discovery_payload_t wire size");
_Static_assert(offsetof(canview_discovery_payload_t, installation_id_le) == 0U, "canview_discovery_payload_t.installation_id_le offset");
_Static_assert(offsetof(canview_discovery_payload_t, endpoint_id_le) == 8U, "canview_discovery_payload_t.endpoint_id_le offset");
_Static_assert(offsetof(canview_discovery_payload_t, mac) == 16U, "canview_discovery_payload_t.mac offset");
_Static_assert(offsetof(canview_discovery_payload_t, reserved0) == 22U, "canview_discovery_payload_t.reserved0 offset");
_Static_assert(offsetof(canview_discovery_payload_t, nonce) == 24U, "canview_discovery_payload_t.nonce offset");
_Static_assert(offsetof(canview_discovery_payload_t, channel) == 40U, "canview_discovery_payload_t.channel offset");
_Static_assert(offsetof(canview_discovery_payload_t, proposed_major) == 41U, "canview_discovery_payload_t.proposed_major offset");
_Static_assert(offsetof(canview_discovery_payload_t, proposed_minor_min) == 42U, "canview_discovery_payload_t.proposed_minor_min offset");
_Static_assert(offsetof(canview_discovery_payload_t, proposed_minor_max) == 43U, "canview_discovery_payload_t.proposed_minor_max offset");
_Static_assert(offsetof(canview_discovery_payload_t, link_key_generation_le) == 44U, "canview_discovery_payload_t.link_key_generation_le offset");
_Static_assert(offsetof(canview_discovery_payload_t, expires_at_ms_le) == 48U, "canview_discovery_payload_t.expires_at_ms_le offset");
_Static_assert(offsetof(canview_discovery_payload_t, hmac_tag) == 52U, "canview_discovery_payload_t.hmac_tag offset");
_Static_assert(sizeof(canview_pair_request_payload_t) == 80U, "canview_pair_request_payload_t wire size");
_Static_assert(offsetof(canview_pair_request_payload_t, request_token_le) == 0U, "canview_pair_request_payload_t.request_token_le offset");
_Static_assert(offsetof(canview_pair_request_payload_t, discovery_digest) == 8U, "canview_pair_request_payload_t.discovery_digest offset");
_Static_assert(offsetof(canview_pair_request_payload_t, requester_endpoint_id_le) == 24U, "canview_pair_request_payload_t.requester_endpoint_id_le offset");
_Static_assert(offsetof(canview_pair_request_payload_t, requester_mac) == 32U, "canview_pair_request_payload_t.requester_mac offset");
_Static_assert(offsetof(canview_pair_request_payload_t, reserved0) == 38U, "canview_pair_request_payload_t.reserved0 offset");
_Static_assert(offsetof(canview_pair_request_payload_t, controller_nonce) == 40U, "canview_pair_request_payload_t.controller_nonce offset");
_Static_assert(offsetof(canview_pair_request_payload_t, requested_role) == 56U, "canview_pair_request_payload_t.requested_role offset");
_Static_assert(offsetof(canview_pair_request_payload_t, requested_major) == 57U, "canview_pair_request_payload_t.requested_major offset");
_Static_assert(offsetof(canview_pair_request_payload_t, requested_minor) == 58U, "canview_pair_request_payload_t.requested_minor offset");
_Static_assert(offsetof(canview_pair_request_payload_t, reserved1) == 59U, "canview_pair_request_payload_t.reserved1 offset");
_Static_assert(offsetof(canview_pair_request_payload_t, expires_at_ms_le) == 60U, "canview_pair_request_payload_t.expires_at_ms_le offset");
_Static_assert(offsetof(canview_pair_request_payload_t, hmac_tag) == 64U, "canview_pair_request_payload_t.hmac_tag offset");
_Static_assert(sizeof(canview_pair_challenge_payload_t) == 112U, "canview_pair_challenge_payload_t wire size");
_Static_assert(offsetof(canview_pair_challenge_payload_t, discovery_digest) == 0U, "canview_pair_challenge_payload_t.discovery_digest offset");
_Static_assert(offsetof(canview_pair_challenge_payload_t, request_digest) == 16U, "canview_pair_challenge_payload_t.request_digest offset");
_Static_assert(offsetof(canview_pair_challenge_payload_t, endpoint0_id_le) == 32U, "canview_pair_challenge_payload_t.endpoint0_id_le offset");
_Static_assert(offsetof(canview_pair_challenge_payload_t, endpoint1_id_le) == 40U, "canview_pair_challenge_payload_t.endpoint1_id_le offset");
_Static_assert(offsetof(canview_pair_challenge_payload_t, nonce0) == 48U, "canview_pair_challenge_payload_t.nonce0 offset");
_Static_assert(offsetof(canview_pair_challenge_payload_t, nonce1) == 64U, "canview_pair_challenge_payload_t.nonce1 offset");
_Static_assert(offsetof(canview_pair_challenge_payload_t, selected_major) == 80U, "canview_pair_challenge_payload_t.selected_major offset");
_Static_assert(offsetof(canview_pair_challenge_payload_t, selected_minor) == 81U, "canview_pair_challenge_payload_t.selected_minor offset");
_Static_assert(offsetof(canview_pair_challenge_payload_t, channel) == 82U, "canview_pair_challenge_payload_t.channel offset");
_Static_assert(offsetof(canview_pair_challenge_payload_t, reserved0) == 83U, "canview_pair_challenge_payload_t.reserved0 offset");
_Static_assert(offsetof(canview_pair_challenge_payload_t, authorized_role) == 84U, "canview_pair_challenge_payload_t.authorized_role offset");
_Static_assert(offsetof(canview_pair_challenge_payload_t, reserved1) == 85U, "canview_pair_challenge_payload_t.reserved1 offset");
_Static_assert(offsetof(canview_pair_challenge_payload_t, authorized_scope_le) == 86U, "canview_pair_challenge_payload_t.authorized_scope_le offset");
_Static_assert(offsetof(canview_pair_challenge_payload_t, allowed_message_classes_le) == 88U, "canview_pair_challenge_payload_t.allowed_message_classes_le offset");
_Static_assert(offsetof(canview_pair_challenge_payload_t, link_key_generation_le) == 92U, "canview_pair_challenge_payload_t.link_key_generation_le offset");
_Static_assert(offsetof(canview_pair_challenge_payload_t, hmac_tag) == 96U, "canview_pair_challenge_payload_t.hmac_tag offset");
_Static_assert(sizeof(canview_pair_confirm_payload_t) == 72U, "canview_pair_confirm_payload_t wire size");
_Static_assert(offsetof(canview_pair_confirm_payload_t, transcript_hash) == 0U, "canview_pair_confirm_payload_t.transcript_hash offset");
_Static_assert(offsetof(canview_pair_confirm_payload_t, selected_major) == 32U, "canview_pair_confirm_payload_t.selected_major offset");
_Static_assert(offsetof(canview_pair_confirm_payload_t, selected_minor) == 33U, "canview_pair_confirm_payload_t.selected_minor offset");
_Static_assert(offsetof(canview_pair_confirm_payload_t, authorized_role) == 34U, "canview_pair_confirm_payload_t.authorized_role offset");
_Static_assert(offsetof(canview_pair_confirm_payload_t, reserved0) == 35U, "canview_pair_confirm_payload_t.reserved0 offset");
_Static_assert(offsetof(canview_pair_confirm_payload_t, link_key_generation_le) == 36U, "canview_pair_confirm_payload_t.link_key_generation_le offset");
_Static_assert(offsetof(canview_pair_confirm_payload_t, confirm_nonce) == 40U, "canview_pair_confirm_payload_t.confirm_nonce offset");
_Static_assert(offsetof(canview_pair_confirm_payload_t, hmac_tag) == 56U, "canview_pair_confirm_payload_t.hmac_tag offset");
_Static_assert(sizeof(canview_pair_result_payload_t) == 60U, "canview_pair_result_payload_t wire size");
_Static_assert(offsetof(canview_pair_result_payload_t, request_token_le) == 0U, "canview_pair_result_payload_t.request_token_le offset");
_Static_assert(offsetof(canview_pair_result_payload_t, status) == 8U, "canview_pair_result_payload_t.status offset");
_Static_assert(offsetof(canview_pair_result_payload_t, assigned_role) == 9U, "canview_pair_result_payload_t.assigned_role offset");
_Static_assert(offsetof(canview_pair_result_payload_t, reserved0_le) == 10U, "canview_pair_result_payload_t.reserved0_le offset");
_Static_assert(offsetof(canview_pair_result_payload_t, link_key_generation_le) == 12U, "canview_pair_result_payload_t.link_key_generation_le offset");
_Static_assert(offsetof(canview_pair_result_payload_t, peer_id_le) == 16U, "canview_pair_result_payload_t.peer_id_le offset");
_Static_assert(offsetof(canview_pair_result_payload_t, transcript_hash) == 24U, "canview_pair_result_payload_t.transcript_hash offset");
_Static_assert(offsetof(canview_pair_result_payload_t, expires_at_ms_le) == 40U, "canview_pair_result_payload_t.expires_at_ms_le offset");
_Static_assert(offsetof(canview_pair_result_payload_t, hmac_tag) == 44U, "canview_pair_result_payload_t.hmac_tag offset");
_Static_assert(sizeof(canview_hello_payload_t) == 86U, "canview_hello_payload_t wire size");
_Static_assert(offsetof(canview_hello_payload_t, device_id_le) == 0U, "canview_hello_payload_t.device_id_le offset");
_Static_assert(offsetof(canview_hello_payload_t, boot_id_le) == 8U, "canview_hello_payload_t.boot_id_le offset");
_Static_assert(offsetof(canview_hello_payload_t, nonce) == 16U, "canview_hello_payload_t.nonce offset");
_Static_assert(offsetof(canview_hello_payload_t, role) == 32U, "canview_hello_payload_t.role offset");
_Static_assert(offsetof(canview_hello_payload_t, major) == 33U, "canview_hello_payload_t.major offset");
_Static_assert(offsetof(canview_hello_payload_t, minor) == 34U, "canview_hello_payload_t.minor offset");
_Static_assert(offsetof(canview_hello_payload_t, reserved0) == 35U, "canview_hello_payload_t.reserved0 offset");
_Static_assert(offsetof(canview_hello_payload_t, max_frame_le) == 36U, "canview_hello_payload_t.max_frame_le offset");
_Static_assert(offsetof(canview_hello_payload_t, build_id_digest) == 38U, "canview_hello_payload_t.build_id_digest offset");
_Static_assert(offsetof(canview_hello_payload_t, challenge) == 54U, "canview_hello_payload_t.challenge offset");
_Static_assert(offsetof(canview_hello_payload_t, capability_digest) == 70U, "canview_hello_payload_t.capability_digest offset");
_Static_assert(sizeof(canview_capabilities_prefix_t) == 122U, "canview_capabilities_prefix_t wire size");
_Static_assert(offsetof(canview_capabilities_prefix_t, device_id_le) == 0U, "canview_capabilities_prefix_t.device_id_le offset");
_Static_assert(offsetof(canview_capabilities_prefix_t, boot_id_le) == 8U, "canview_capabilities_prefix_t.boot_id_le offset");
_Static_assert(offsetof(canview_capabilities_prefix_t, role) == 16U, "canview_capabilities_prefix_t.role offset");
_Static_assert(offsetof(canview_capabilities_prefix_t, selected_major) == 17U, "canview_capabilities_prefix_t.selected_major offset");
_Static_assert(offsetof(canview_capabilities_prefix_t, selected_minor) == 18U, "canview_capabilities_prefix_t.selected_minor offset");
_Static_assert(offsetof(canview_capabilities_prefix_t, reserved0) == 19U, "canview_capabilities_prefix_t.reserved0 offset");
_Static_assert(offsetof(canview_capabilities_prefix_t, max_frame_le) == 20U, "canview_capabilities_prefix_t.max_frame_le offset");
_Static_assert(offsetof(canview_capabilities_prefix_t, bus_count) == 22U, "canview_capabilities_prefix_t.bus_count offset");
_Static_assert(offsetof(canview_capabilities_prefix_t, support_flags) == 23U, "canview_capabilities_prefix_t.support_flags offset");
_Static_assert(offsetof(canview_capabilities_prefix_t, control_scope_le) == 24U, "canview_capabilities_prefix_t.control_scope_le offset");
_Static_assert(offsetof(canview_capabilities_prefix_t, max_filters) == 26U, "canview_capabilities_prefix_t.max_filters offset");
_Static_assert(offsetof(canview_capabilities_prefix_t, max_peers) == 27U, "canview_capabilities_prefix_t.max_peers offset");
_Static_assert(offsetof(canview_capabilities_prefix_t, max_batch_records) == 28U, "canview_capabilities_prefix_t.max_batch_records offset");
_Static_assert(offsetof(canview_capabilities_prefix_t, reserved1) == 29U, "canview_capabilities_prefix_t.reserved1 offset");
_Static_assert(offsetof(canview_capabilities_prefix_t, profile_id_le) == 30U, "canview_capabilities_prefix_t.profile_id_le offset");
_Static_assert(offsetof(canview_capabilities_prefix_t, catalog_revision_le) == 34U, "canview_capabilities_prefix_t.catalog_revision_le offset");
_Static_assert(offsetof(canview_capabilities_prefix_t, config_schema_version_le) == 38U, "canview_capabilities_prefix_t.config_schema_version_le offset");
_Static_assert(offsetof(canview_capabilities_prefix_t, reserved2_le) == 40U, "canview_capabilities_prefix_t.reserved2_le offset");
_Static_assert(offsetof(canview_capabilities_prefix_t, profile_digest) == 42U, "canview_capabilities_prefix_t.profile_digest offset");
_Static_assert(offsetof(canview_capabilities_prefix_t, catalog_digest) == 74U, "canview_capabilities_prefix_t.catalog_digest offset");
_Static_assert(offsetof(canview_capabilities_prefix_t, build_id_digest) == 106U, "canview_capabilities_prefix_t.build_id_digest offset");
_Static_assert(sizeof(canview_time_sync_request_payload_t) == 24U, "canview_time_sync_request_payload_t wire size");
_Static_assert(offsetof(canview_time_sync_request_payload_t, request_token_le) == 0U, "canview_time_sync_request_payload_t.request_token_le offset");
_Static_assert(offsetof(canview_time_sync_request_payload_t, t1_sender_us_le) == 8U, "canview_time_sync_request_payload_t.t1_sender_us_le offset");
_Static_assert(offsetof(canview_time_sync_request_payload_t, sync_generation_le) == 16U, "canview_time_sync_request_payload_t.sync_generation_le offset");
_Static_assert(offsetof(canview_time_sync_request_payload_t, reserved0_le) == 20U, "canview_time_sync_request_payload_t.reserved0_le offset");
_Static_assert(sizeof(canview_time_sync_response_payload_t) == 44U, "canview_time_sync_response_payload_t wire size");
_Static_assert(offsetof(canview_time_sync_response_payload_t, request_token_le) == 0U, "canview_time_sync_response_payload_t.request_token_le offset");
_Static_assert(offsetof(canview_time_sync_response_payload_t, t1_sender_us_le) == 8U, "canview_time_sync_response_payload_t.t1_sender_us_le offset");
_Static_assert(offsetof(canview_time_sync_response_payload_t, t2_receiver_us_le) == 16U, "canview_time_sync_response_payload_t.t2_receiver_us_le offset");
_Static_assert(offsetof(canview_time_sync_response_payload_t, t3_receiver_send_us_le) == 24U, "canview_time_sync_response_payload_t.t3_receiver_send_us_le offset");
_Static_assert(offsetof(canview_time_sync_response_payload_t, sync_generation_le) == 32U, "canview_time_sync_response_payload_t.sync_generation_le offset");
_Static_assert(offsetof(canview_time_sync_response_payload_t, uncertainty_us_le) == 36U, "canview_time_sync_response_payload_t.uncertainty_us_le offset");
_Static_assert(offsetof(canview_time_sync_response_payload_t, reserved0_le) == 40U, "canview_time_sync_response_payload_t.reserved0_le offset");
_Static_assert(sizeof(canview_heartbeat_payload_t) == 44U, "canview_heartbeat_payload_t wire size");
_Static_assert(offsetof(canview_heartbeat_payload_t, boot_id_le) == 0U, "canview_heartbeat_payload_t.boot_id_le offset");
_Static_assert(offsetof(canview_heartbeat_payload_t, state_revision_le) == 8U, "canview_heartbeat_payload_t.state_revision_le offset");
_Static_assert(offsetof(canview_heartbeat_payload_t, uptime_ms_le) == 12U, "canview_heartbeat_payload_t.uptime_ms_le offset");
_Static_assert(offsetof(canview_heartbeat_payload_t, heartbeat_interval_ms_le) == 16U, "canview_heartbeat_payload_t.heartbeat_interval_ms_le offset");
_Static_assert(offsetof(canview_heartbeat_payload_t, last_rssi_dbm) == 18U, "canview_heartbeat_payload_t.last_rssi_dbm offset");
_Static_assert(offsetof(canview_heartbeat_payload_t, link_state) == 19U, "canview_heartbeat_payload_t.link_state offset");
_Static_assert(offsetof(canview_heartbeat_payload_t, active_bus_mask) == 20U, "canview_heartbeat_payload_t.active_bus_mask offset");
_Static_assert(offsetof(canview_heartbeat_payload_t, bus_error_mask) == 21U, "canview_heartbeat_payload_t.bus_error_mask offset");
_Static_assert(offsetof(canview_heartbeat_payload_t, rx_queue_depth_le) == 22U, "canview_heartbeat_payload_t.rx_queue_depth_le offset");
_Static_assert(offsetof(canview_heartbeat_payload_t, telemetry_dropped_le) == 24U, "canview_heartbeat_payload_t.telemetry_dropped_le offset");
_Static_assert(offsetof(canview_heartbeat_payload_t, protocol_error_count_le) == 32U, "canview_heartbeat_payload_t.protocol_error_count_le offset");
_Static_assert(offsetof(canview_heartbeat_payload_t, safety_inhibit_le) == 40U, "canview_heartbeat_payload_t.safety_inhibit_le offset");
_Static_assert(offsetof(canview_heartbeat_payload_t, reserved0_le) == 42U, "canview_heartbeat_payload_t.reserved0_le offset");
_Static_assert(sizeof(canview_ack_payload_t) == 20U, "canview_ack_payload_t wire size");
_Static_assert(offsetof(canview_ack_payload_t, acknowledged_sequence_le) == 0U, "canview_ack_payload_t.acknowledged_sequence_le offset");
_Static_assert(offsetof(canview_ack_payload_t, request_token_le) == 4U, "canview_ack_payload_t.request_token_le offset");
_Static_assert(offsetof(canview_ack_payload_t, status_le) == 12U, "canview_ack_payload_t.status_le offset");
_Static_assert(offsetof(canview_ack_payload_t, detail_le) == 14U, "canview_ack_payload_t.detail_le offset");
_Static_assert(offsetof(canview_ack_payload_t, receiver_time_ms_le) == 16U, "canview_ack_payload_t.receiver_time_ms_le offset");
_Static_assert(sizeof(canview_error_payload_t) == 16U, "canview_error_payload_t wire size");
_Static_assert(offsetof(canview_error_payload_t, code_le) == 0U, "canview_error_payload_t.code_le offset");
_Static_assert(offsetof(canview_error_payload_t, severity) == 2U, "canview_error_payload_t.severity offset");
_Static_assert(offsetof(canview_error_payload_t, origin) == 3U, "canview_error_payload_t.origin offset");
_Static_assert(offsetof(canview_error_payload_t, offending_message_type) == 4U, "canview_error_payload_t.offending_message_type offset");
_Static_assert(offsetof(canview_error_payload_t, reserved0) == 5U, "canview_error_payload_t.reserved0 offset");
_Static_assert(offsetof(canview_error_payload_t, detail_le) == 6U, "canview_error_payload_t.detail_le offset");
_Static_assert(offsetof(canview_error_payload_t, offending_sequence_le) == 8U, "canview_error_payload_t.offending_sequence_le offset");
_Static_assert(offsetof(canview_error_payload_t, retry_after_ms_le) == 12U, "canview_error_payload_t.retry_after_ms_le offset");
_Static_assert(sizeof(canview_state_snapshot_request_payload_t) == 24U, "canview_state_snapshot_request_payload_t wire size");
_Static_assert(offsetof(canview_state_snapshot_request_payload_t, request_token_le) == 0U, "canview_state_snapshot_request_payload_t.request_token_le offset");
_Static_assert(offsetof(canview_state_snapshot_request_payload_t, known_state_revision_le) == 8U, "canview_state_snapshot_request_payload_t.known_state_revision_le offset");
_Static_assert(offsetof(canview_state_snapshot_request_payload_t, known_boot_id_le) == 12U, "canview_state_snapshot_request_payload_t.known_boot_id_le offset");
_Static_assert(offsetof(canview_state_snapshot_request_payload_t, reserved0_le) == 20U, "canview_state_snapshot_request_payload_t.reserved0_le offset");
_Static_assert(sizeof(canview_state_snapshot_payload_t) == 60U, "canview_state_snapshot_payload_t wire size");
_Static_assert(offsetof(canview_state_snapshot_payload_t, stm_boot_id_le) == 0U, "canview_state_snapshot_payload_t.stm_boot_id_le offset");
_Static_assert(offsetof(canview_state_snapshot_payload_t, state_revision_le) == 8U, "canview_state_snapshot_payload_t.state_revision_le offset");
_Static_assert(offsetof(canview_state_snapshot_payload_t, profile_revision_le) == 12U, "canview_state_snapshot_payload_t.profile_revision_le offset");
_Static_assert(offsetof(canview_state_snapshot_payload_t, profile_digest) == 16U, "canview_state_snapshot_payload_t.profile_digest offset");
_Static_assert(offsetof(canview_state_snapshot_payload_t, build_mode) == 32U, "canview_state_snapshot_payload_t.build_mode offset");
_Static_assert(offsetof(canview_state_snapshot_payload_t, tx_gate) == 33U, "canview_state_snapshot_payload_t.tx_gate offset");
_Static_assert(offsetof(canview_state_snapshot_payload_t, active_bus_mask) == 34U, "canview_state_snapshot_payload_t.active_bus_mask offset");
_Static_assert(offsetof(canview_state_snapshot_payload_t, bus_error_mask) == 35U, "canview_state_snapshot_payload_t.bus_error_mask offset");
_Static_assert(offsetof(canview_state_snapshot_payload_t, control_lease_state) == 36U, "canview_state_snapshot_payload_t.control_lease_state offset");
_Static_assert(offsetof(canview_state_snapshot_payload_t, link_state) == 37U, "canview_state_snapshot_payload_t.link_state offset");
_Static_assert(offsetof(canview_state_snapshot_payload_t, audio_quality) == 38U, "canview_state_snapshot_payload_t.audio_quality offset");
_Static_assert(offsetof(canview_state_snapshot_payload_t, sport_state) == 39U, "canview_state_snapshot_payload_t.sport_state offset");
_Static_assert(offsetof(canview_state_snapshot_payload_t, safety_inhibit_le) == 40U, "canview_state_snapshot_payload_t.safety_inhibit_le offset");
_Static_assert(offsetof(canview_state_snapshot_payload_t, reserved0_le) == 42U, "canview_state_snapshot_payload_t.reserved0_le offset");
_Static_assert(offsetof(canview_state_snapshot_payload_t, audio_snapshot_revision_le) == 44U, "canview_state_snapshot_payload_t.audio_snapshot_revision_le offset");
_Static_assert(offsetof(canview_state_snapshot_payload_t, completed_time_ms_le) == 48U, "canview_state_snapshot_payload_t.completed_time_ms_le offset");
_Static_assert(offsetof(canview_state_snapshot_payload_t, vehicle_profile_id_le) == 52U, "canview_state_snapshot_payload_t.vehicle_profile_id_le offset");
_Static_assert(offsetof(canview_state_snapshot_payload_t, control_generation_le) == 56U, "canview_state_snapshot_payload_t.control_generation_le offset");
_Static_assert(sizeof(canview_can_batch_header_t) == 12U, "canview_can_batch_header_t wire size");
_Static_assert(offsetof(canview_can_batch_header_t, base_time_us_le) == 0U, "canview_can_batch_header_t.base_time_us_le offset");
_Static_assert(offsetof(canview_can_batch_header_t, count) == 8U, "canview_can_batch_header_t.count offset");
_Static_assert(offsetof(canview_can_batch_header_t, dropped_since_last) == 9U, "canview_can_batch_header_t.dropped_since_last offset");
_Static_assert(offsetof(canview_can_batch_header_t, reserved_le) == 10U, "canview_can_batch_header_t.reserved_le offset");
_Static_assert(sizeof(canview_can_record_t) == 16U, "canview_can_record_t wire size");
_Static_assert(offsetof(canview_can_record_t, delta_us_le) == 0U, "canview_can_record_t.delta_us_le offset");
_Static_assert(offsetof(canview_can_record_t, bus_id) == 2U, "canview_can_record_t.bus_id offset");
_Static_assert(offsetof(canview_can_record_t, flags_dlc) == 3U, "canview_can_record_t.flags_dlc offset");
_Static_assert(offsetof(canview_can_record_t, can_id_le) == 4U, "canview_can_record_t.can_id_le offset");
_Static_assert(offsetof(canview_can_record_t, data) == 8U, "canview_can_record_t.data offset");
_Static_assert(sizeof(canview_signal_batch_header_t) == 16U, "canview_signal_batch_header_t wire size");
_Static_assert(offsetof(canview_signal_batch_header_t, sample_time_us_le) == 0U, "canview_signal_batch_header_t.sample_time_us_le offset");
_Static_assert(offsetof(canview_signal_batch_header_t, count) == 8U, "canview_signal_batch_header_t.count offset");
_Static_assert(offsetof(canview_signal_batch_header_t, reserved0) == 9U, "canview_signal_batch_header_t.reserved0 offset");
_Static_assert(offsetof(canview_signal_batch_header_t, catalog_revision_le) == 12U, "canview_signal_batch_header_t.catalog_revision_le offset");
_Static_assert(sizeof(canview_signal_record_t) == 12U, "canview_signal_record_t wire size");
_Static_assert(offsetof(canview_signal_record_t, signal_id_le) == 0U, "canview_signal_record_t.signal_id_le offset");
_Static_assert(offsetof(canview_signal_record_t, value_type) == 2U, "canview_signal_record_t.value_type offset");
_Static_assert(offsetof(canview_signal_record_t, quality) == 3U, "canview_signal_record_t.quality offset");
_Static_assert(offsetof(canview_signal_record_t, age_ms_le) == 4U, "canview_signal_record_t.age_ms_le offset");
_Static_assert(offsetof(canview_signal_record_t, evidence_grade) == 6U, "canview_signal_record_t.evidence_grade offset");
_Static_assert(offsetof(canview_signal_record_t, reserved) == 7U, "canview_signal_record_t.reserved offset");
_Static_assert(offsetof(canview_signal_record_t, value_bits_le) == 8U, "canview_signal_record_t.value_bits_le offset");
_Static_assert(sizeof(canview_bus_status_header_t) == 12U, "canview_bus_status_header_t wire size");
_Static_assert(offsetof(canview_bus_status_header_t, boot_id_le) == 0U, "canview_bus_status_header_t.boot_id_le offset");
_Static_assert(offsetof(canview_bus_status_header_t, count) == 8U, "canview_bus_status_header_t.count offset");
_Static_assert(offsetof(canview_bus_status_header_t, reserved0) == 9U, "canview_bus_status_header_t.reserved0 offset");
_Static_assert(sizeof(canview_bus_status_record_t) == 36U, "canview_bus_status_record_t wire size");
_Static_assert(offsetof(canview_bus_status_record_t, bus_id) == 0U, "canview_bus_status_record_t.bus_id offset");
_Static_assert(offsetof(canview_bus_status_record_t, state) == 1U, "canview_bus_status_record_t.state offset");
_Static_assert(offsetof(canview_bus_status_record_t, flags_le) == 2U, "canview_bus_status_record_t.flags_le offset");
_Static_assert(offsetof(canview_bus_status_record_t, bitrate_le) == 4U, "canview_bus_status_record_t.bitrate_le offset");
_Static_assert(offsetof(canview_bus_status_record_t, error_count_le) == 8U, "canview_bus_status_record_t.error_count_le offset");
_Static_assert(offsetof(canview_bus_status_record_t, rx_count_le) == 16U, "canview_bus_status_record_t.rx_count_le offset");
_Static_assert(offsetof(canview_bus_status_record_t, tx_inhibit) == 24U, "canview_bus_status_record_t.tx_inhibit offset");
_Static_assert(offsetof(canview_bus_status_record_t, reserved0) == 25U, "canview_bus_status_record_t.reserved0 offset");
_Static_assert(offsetof(canview_bus_status_record_t, bus_off_count_le) == 28U, "canview_bus_status_record_t.bus_off_count_le offset");
_Static_assert(sizeof(canview_can_filter_get_payload_t) == 8U, "canview_can_filter_get_payload_t wire size");
_Static_assert(offsetof(canview_can_filter_get_payload_t, config_revision_le) == 0U, "canview_can_filter_get_payload_t.config_revision_le offset");
_Static_assert(offsetof(canview_can_filter_get_payload_t, filter_id_le) == 4U, "canview_can_filter_get_payload_t.filter_id_le offset");
_Static_assert(sizeof(canview_can_filter_batch_header_t) == 8U, "canview_can_filter_batch_header_t wire size");
_Static_assert(offsetof(canview_can_filter_batch_header_t, action) == 0U, "canview_can_filter_batch_header_t.action offset");
_Static_assert(offsetof(canview_can_filter_batch_header_t, count) == 1U, "canview_can_filter_batch_header_t.count offset");
_Static_assert(offsetof(canview_can_filter_batch_header_t, reserved_le) == 2U, "canview_can_filter_batch_header_t.reserved_le offset");
_Static_assert(offsetof(canview_can_filter_batch_header_t, config_revision_le) == 4U, "canview_can_filter_batch_header_t.config_revision_le offset");
_Static_assert(sizeof(canview_can_filter_t) == 22U, "canview_can_filter_t wire size");
_Static_assert(offsetof(canview_can_filter_t, filter_id_le) == 0U, "canview_can_filter_t.filter_id_le offset");
_Static_assert(offsetof(canview_can_filter_t, can_id_le) == 4U, "canview_can_filter_t.can_id_le offset");
_Static_assert(offsetof(canview_can_filter_t, can_id_mask_le) == 8U, "canview_can_filter_t.can_id_mask_le offset");
_Static_assert(offsetof(canview_can_filter_t, period_ms_le) == 12U, "canview_can_filter_t.period_ms_le offset");
_Static_assert(offsetof(canview_can_filter_t, bus_id) == 14U, "canview_can_filter_t.bus_id offset");
_Static_assert(offsetof(canview_can_filter_t, flags_value) == 15U, "canview_can_filter_t.flags_value offset");
_Static_assert(offsetof(canview_can_filter_t, flags_mask) == 16U, "canview_can_filter_t.flags_mask offset");
_Static_assert(offsetof(canview_can_filter_t, min_dlc) == 17U, "canview_can_filter_t.min_dlc offset");
_Static_assert(offsetof(canview_can_filter_t, max_dlc) == 18U, "canview_can_filter_t.max_dlc offset");
_Static_assert(offsetof(canview_can_filter_t, max_records_per_period) == 19U, "canview_can_filter_t.max_records_per_period offset");
_Static_assert(offsetof(canview_can_filter_t, enabled) == 20U, "canview_can_filter_t.enabled offset");
_Static_assert(offsetof(canview_can_filter_t, reserved0) == 21U, "canview_can_filter_t.reserved0 offset");
_Static_assert(sizeof(canview_can_filter_result_payload_t) == 12U, "canview_can_filter_result_payload_t wire size");
_Static_assert(offsetof(canview_can_filter_result_payload_t, config_revision_le) == 0U, "canview_can_filter_result_payload_t.config_revision_le offset");
_Static_assert(offsetof(canview_can_filter_result_payload_t, filter_id_le) == 4U, "canview_can_filter_result_payload_t.filter_id_le offset");
_Static_assert(offsetof(canview_can_filter_result_payload_t, action) == 8U, "canview_can_filter_result_payload_t.action offset");
_Static_assert(offsetof(canview_can_filter_result_payload_t, result) == 9U, "canview_can_filter_result_payload_t.result offset");
_Static_assert(offsetof(canview_can_filter_result_payload_t, detail_le) == 10U, "canview_can_filter_result_payload_t.detail_le offset");
_Static_assert(sizeof(canview_can_stream_config_t) == 16U, "canview_can_stream_config_t wire size");
_Static_assert(offsetof(canview_can_stream_config_t, config_revision_le) == 0U, "canview_can_stream_config_t.config_revision_le offset");
_Static_assert(offsetof(canview_can_stream_config_t, period_ms_le) == 4U, "canview_can_stream_config_t.period_ms_le offset");
_Static_assert(offsetof(canview_can_stream_config_t, max_records_per_period) == 6U, "canview_can_stream_config_t.max_records_per_period offset");
_Static_assert(offsetof(canview_can_stream_config_t, enabled) == 7U, "canview_can_stream_config_t.enabled offset");
_Static_assert(offsetof(canview_can_stream_config_t, max_bytes_per_second_le) == 8U, "canview_can_stream_config_t.max_bytes_per_second_le offset");
_Static_assert(offsetof(canview_can_stream_config_t, burst_bytes_le) == 12U, "canview_can_stream_config_t.burst_bytes_le offset");
_Static_assert(offsetof(canview_can_stream_config_t, reserved_le) == 14U, "canview_can_stream_config_t.reserved_le offset");
_Static_assert(sizeof(canview_can_stream_status_t) == 16U, "canview_can_stream_status_t wire size");
_Static_assert(offsetof(canview_can_stream_status_t, config_revision_le) == 0U, "canview_can_stream_status_t.config_revision_le offset");
_Static_assert(offsetof(canview_can_stream_status_t, accepted_records_le) == 4U, "canview_can_stream_status_t.accepted_records_le offset");
_Static_assert(offsetof(canview_can_stream_status_t, rejected_records_le) == 6U, "canview_can_stream_status_t.rejected_records_le offset");
_Static_assert(offsetof(canview_can_stream_status_t, dropped_by_budget_le) == 8U, "canview_can_stream_status_t.dropped_by_budget_le offset");
_Static_assert(offsetof(canview_can_stream_status_t, reserved_le) == 12U, "canview_can_stream_status_t.reserved_le offset");
_Static_assert(sizeof(canview_observer_config_payload_t) == 40U, "canview_observer_config_payload_t wire size");
_Static_assert(offsetof(canview_observer_config_payload_t, request_token_le) == 0U, "canview_observer_config_payload_t.request_token_le offset");
_Static_assert(offsetof(canview_observer_config_payload_t, expected_observer_revision_le) == 8U, "canview_observer_config_payload_t.expected_observer_revision_le offset");
_Static_assert(offsetof(canview_observer_config_payload_t, filter_revision_le) == 12U, "canview_observer_config_payload_t.filter_revision_le offset");
_Static_assert(offsetof(canview_observer_config_payload_t, mode) == 16U, "canview_observer_config_payload_t.mode offset");
_Static_assert(offsetof(canview_observer_config_payload_t, bus_mask) == 17U, "canview_observer_config_payload_t.bus_mask offset");
_Static_assert(offsetof(canview_observer_config_payload_t, flags_le) == 18U, "canview_observer_config_payload_t.flags_le offset");
_Static_assert(offsetof(canview_observer_config_payload_t, stats_period_ms_le) == 20U, "canview_observer_config_payload_t.stats_period_ms_le offset");
_Static_assert(offsetof(canview_observer_config_payload_t, max_records_per_second_le) == 22U, "canview_observer_config_payload_t.max_records_per_second_le offset");
_Static_assert(offsetof(canview_observer_config_payload_t, max_bytes_per_second_le) == 24U, "canview_observer_config_payload_t.max_bytes_per_second_le offset");
_Static_assert(offsetof(canview_observer_config_payload_t, duration_ms_le) == 28U, "canview_observer_config_payload_t.duration_ms_le offset");
_Static_assert(offsetof(canview_observer_config_payload_t, pretrigger_ms_le) == 32U, "canview_observer_config_payload_t.pretrigger_ms_le offset");
_Static_assert(offsetof(canview_observer_config_payload_t, posttrigger_ms_le) == 34U, "canview_observer_config_payload_t.posttrigger_ms_le offset");
_Static_assert(offsetof(canview_observer_config_payload_t, reserved0_le) == 36U, "canview_observer_config_payload_t.reserved0_le offset");
_Static_assert(sizeof(canview_can_id_stats_header_t) == 8U, "canview_can_id_stats_header_t wire size");
_Static_assert(offsetof(canview_can_id_stats_header_t, window_end_ms_le) == 0U, "canview_can_id_stats_header_t.window_end_ms_le offset");
_Static_assert(offsetof(canview_can_id_stats_header_t, count) == 4U, "canview_can_id_stats_header_t.count offset");
_Static_assert(offsetof(canview_can_id_stats_header_t, part_index) == 5U, "canview_can_id_stats_header_t.part_index offset");
_Static_assert(offsetof(canview_can_id_stats_header_t, part_count) == 6U, "canview_can_id_stats_header_t.part_count offset");
_Static_assert(offsetof(canview_can_id_stats_header_t, reserved0) == 7U, "canview_can_id_stats_header_t.reserved0 offset");
_Static_assert(sizeof(canview_can_id_stats_record_t) == 40U, "canview_can_id_stats_record_t wire size");
_Static_assert(offsetof(canview_can_id_stats_record_t, bus_id) == 0U, "canview_can_id_stats_record_t.bus_id offset");
_Static_assert(offsetof(canview_can_id_stats_record_t, flags_dlc) == 1U, "canview_can_id_stats_record_t.flags_dlc offset");
_Static_assert(offsetof(canview_can_id_stats_record_t, rate_tenth_hz_le) == 2U, "canview_can_id_stats_record_t.rate_tenth_hz_le offset");
_Static_assert(offsetof(canview_can_id_stats_record_t, can_id_le) == 4U, "canview_can_id_stats_record_t.can_id_le offset");
_Static_assert(offsetof(canview_can_id_stats_record_t, frame_count_le) == 8U, "canview_can_id_stats_record_t.frame_count_le offset");
_Static_assert(offsetof(canview_can_id_stats_record_t, change_count_le) == 12U, "canview_can_id_stats_record_t.change_count_le offset");
_Static_assert(offsetof(canview_can_id_stats_record_t, period_p50_us_le) == 16U, "canview_can_id_stats_record_t.period_p50_us_le offset");
_Static_assert(offsetof(canview_can_id_stats_record_t, period_p95_us_le) == 20U, "canview_can_id_stats_record_t.period_p95_us_le offset");
_Static_assert(offsetof(canview_can_id_stats_record_t, bit_change_mask_le) == 24U, "canview_can_id_stats_record_t.bit_change_mask_le offset");
_Static_assert(offsetof(canview_can_id_stats_record_t, last_data) == 32U, "canview_can_id_stats_record_t.last_data offset");
_Static_assert(sizeof(canview_capture_control_payload_t) == 28U, "canview_capture_control_payload_t wire size");
_Static_assert(offsetof(canview_capture_control_payload_t, request_token_le) == 0U, "canview_capture_control_payload_t.request_token_le offset");
_Static_assert(offsetof(canview_capture_control_payload_t, capture_id_le) == 8U, "canview_capture_control_payload_t.capture_id_le offset");
_Static_assert(offsetof(canview_capture_control_payload_t, action) == 16U, "canview_capture_control_payload_t.action offset");
_Static_assert(offsetof(canview_capture_control_payload_t, reserved0) == 17U, "canview_capture_control_payload_t.reserved0 offset");
_Static_assert(offsetof(canview_capture_control_payload_t, flags_le) == 18U, "canview_capture_control_payload_t.flags_le offset");
_Static_assert(offsetof(canview_capture_control_payload_t, requested_time_ms_le) == 20U, "canview_capture_control_payload_t.requested_time_ms_le offset");
_Static_assert(offsetof(canview_capture_control_payload_t, reserved1_le) == 24U, "canview_capture_control_payload_t.reserved1_le offset");
_Static_assert(sizeof(canview_capture_status_payload_t) == 44U, "canview_capture_status_payload_t wire size");
_Static_assert(offsetof(canview_capture_status_payload_t, request_token_le) == 0U, "canview_capture_status_payload_t.request_token_le offset");
_Static_assert(offsetof(canview_capture_status_payload_t, capture_id_le) == 8U, "canview_capture_status_payload_t.capture_id_le offset");
_Static_assert(offsetof(canview_capture_status_payload_t, state) == 16U, "canview_capture_status_payload_t.state offset");
_Static_assert(offsetof(canview_capture_status_payload_t, reason_le) == 17U, "canview_capture_status_payload_t.reason_le offset");
_Static_assert(offsetof(canview_capture_status_payload_t, bus_mask) == 19U, "canview_capture_status_payload_t.bus_mask offset");
_Static_assert(offsetof(canview_capture_status_payload_t, accepted_records_le) == 20U, "canview_capture_status_payload_t.accepted_records_le offset");
_Static_assert(offsetof(canview_capture_status_payload_t, dropped_records_le) == 24U, "canview_capture_status_payload_t.dropped_records_le offset");
_Static_assert(offsetof(canview_capture_status_payload_t, stored_bytes_le) == 28U, "canview_capture_status_payload_t.stored_bytes_le offset");
_Static_assert(offsetof(canview_capture_status_payload_t, remaining_ms_le) == 32U, "canview_capture_status_payload_t.remaining_ms_le offset");
_Static_assert(offsetof(canview_capture_status_payload_t, effective_filter_revision_le) == 36U, "canview_capture_status_payload_t.effective_filter_revision_le offset");
_Static_assert(offsetof(canview_capture_status_payload_t, reserved1_le) == 40U, "canview_capture_status_payload_t.reserved1_le offset");
_Static_assert(sizeof(canview_event_marker_payload_t) == 28U, "canview_event_marker_payload_t wire size");
_Static_assert(offsetof(canview_event_marker_payload_t, capture_id_le) == 0U, "canview_event_marker_payload_t.capture_id_le offset");
_Static_assert(offsetof(canview_event_marker_payload_t, marker_id_le) == 8U, "canview_event_marker_payload_t.marker_id_le offset");
_Static_assert(offsetof(canview_event_marker_payload_t, sender_time_ms_le) == 12U, "canview_event_marker_payload_t.sender_time_ms_le offset");
_Static_assert(offsetof(canview_event_marker_payload_t, marker_kind_le) == 16U, "canview_event_marker_payload_t.marker_kind_le offset");
_Static_assert(offsetof(canview_event_marker_payload_t, flags_le) == 18U, "canview_event_marker_payload_t.flags_le offset");
_Static_assert(offsetof(canview_event_marker_payload_t, label_code_le) == 20U, "canview_event_marker_payload_t.label_code_le offset");
_Static_assert(offsetof(canview_event_marker_payload_t, reserved0_le) == 22U, "canview_event_marker_payload_t.reserved0_le offset");
_Static_assert(offsetof(canview_event_marker_payload_t, time_uncertainty_us_le) == 24U, "canview_event_marker_payload_t.time_uncertainty_us_le offset");
_Static_assert(sizeof(canview_diagnostic_lease_request_t) == 24U, "canview_diagnostic_lease_request_t wire size");
_Static_assert(offsetof(canview_diagnostic_lease_request_t, request_token_le) == 0U, "canview_diagnostic_lease_request_t.request_token_le offset");
_Static_assert(offsetof(canview_diagnostic_lease_request_t, action) == 8U, "canview_diagnostic_lease_request_t.action offset");
_Static_assert(offsetof(canview_diagnostic_lease_request_t, reserved0) == 9U, "canview_diagnostic_lease_request_t.reserved0 offset");
_Static_assert(offsetof(canview_diagnostic_lease_request_t, requested_ms_le) == 12U, "canview_diagnostic_lease_request_t.requested_ms_le offset");
_Static_assert(offsetof(canview_diagnostic_lease_request_t, lease_id_le) == 16U, "canview_diagnostic_lease_request_t.lease_id_le offset");
_Static_assert(sizeof(canview_diagnostic_lease_response_t) == 36U, "canview_diagnostic_lease_response_t wire size");
_Static_assert(offsetof(canview_diagnostic_lease_response_t, request_token_le) == 0U, "canview_diagnostic_lease_response_t.request_token_le offset");
_Static_assert(offsetof(canview_diagnostic_lease_response_t, lease_id_le) == 8U, "canview_diagnostic_lease_response_t.lease_id_le offset");
_Static_assert(offsetof(canview_diagnostic_lease_response_t, status) == 16U, "canview_diagnostic_lease_response_t.status offset");
_Static_assert(offsetof(canview_diagnostic_lease_response_t, reserved0) == 17U, "canview_diagnostic_lease_response_t.reserved0 offset");
_Static_assert(offsetof(canview_diagnostic_lease_response_t, granted_ms_le) == 20U, "canview_diagnostic_lease_response_t.granted_ms_le offset");
_Static_assert(offsetof(canview_diagnostic_lease_response_t, expires_at_ms_le) == 24U, "canview_diagnostic_lease_response_t.expires_at_ms_le offset");
_Static_assert(offsetof(canview_diagnostic_lease_response_t, owner_device_id_le) == 28U, "canview_diagnostic_lease_response_t.owner_device_id_le offset");
_Static_assert(sizeof(canview_command_request_t) == 72U, "canview_command_request_t wire size");
_Static_assert(offsetof(canview_command_request_t, request_token_le) == 0U, "canview_command_request_t.request_token_le offset");
_Static_assert(offsetof(canview_command_request_t, command_id_le) == 8U, "canview_command_request_t.command_id_le offset");
_Static_assert(offsetof(canview_command_request_t, ttl_ms_le) == 10U, "canview_command_request_t.ttl_ms_le offset");
_Static_assert(offsetof(canview_command_request_t, origin_device_id_le) == 12U, "canview_command_request_t.origin_device_id_le offset");
_Static_assert(offsetof(canview_command_request_t, origin_boot_id_le) == 20U, "canview_command_request_t.origin_boot_id_le offset");
_Static_assert(offsetof(canview_command_request_t, wireless_session_id_le) == 28U, "canview_command_request_t.wireless_session_id_le offset");
_Static_assert(offsetof(canview_command_request_t, control_generation_le) == 32U, "canview_command_request_t.control_generation_le offset");
_Static_assert(offsetof(canview_command_request_t, issued_at_controller_ms_le) == 36U, "canview_command_request_t.issued_at_controller_ms_le offset");
_Static_assert(offsetof(canview_command_request_t, control_sync_generation_le) == 40U, "canview_command_request_t.control_sync_generation_le offset");
_Static_assert(offsetof(canview_command_request_t, expected_state_revision_le) == 44U, "canview_command_request_t.expected_state_revision_le offset");
_Static_assert(offsetof(canview_command_request_t, precondition_flags_le) == 48U, "canview_command_request_t.precondition_flags_le offset");
_Static_assert(offsetof(canview_command_request_t, argument_tlv_length_le) == 52U, "canview_command_request_t.argument_tlv_length_le offset");
_Static_assert(offsetof(canview_command_request_t, reserved_le) == 54U, "canview_command_request_t.reserved_le offset");
_Static_assert(offsetof(canview_command_request_t, control_tag) == 56U, "canview_command_request_t.control_tag offset");
_Static_assert(sizeof(canview_command_result_t) == 50U, "canview_command_result_t wire size");
_Static_assert(offsetof(canview_command_result_t, request_token_le) == 0U, "canview_command_result_t.request_token_le offset");
_Static_assert(offsetof(canview_command_result_t, command_id_le) == 8U, "canview_command_result_t.command_id_le offset");
_Static_assert(offsetof(canview_command_result_t, stage) == 10U, "canview_command_result_t.stage offset");
_Static_assert(offsetof(canview_command_result_t, reserved0) == 11U, "canview_command_result_t.reserved0 offset");
_Static_assert(offsetof(canview_command_result_t, reason_le) == 12U, "canview_command_result_t.reason_le offset");
_Static_assert(offsetof(canview_command_result_t, state_revision_le) == 14U, "canview_command_result_t.state_revision_le offset");
_Static_assert(offsetof(canview_command_result_t, completed_time_ms_le) == 18U, "canview_command_result_t.completed_time_ms_le offset");
_Static_assert(offsetof(canview_command_result_t, request_sequence_le) == 22U, "canview_command_result_t.request_sequence_le offset");
_Static_assert(offsetof(canview_command_result_t, control_tag) == 26U, "canview_command_result_t.control_tag offset");
_Static_assert(offsetof(canview_command_result_t, feedback_revision_le) == 42U, "canview_command_result_t.feedback_revision_le offset");
_Static_assert(offsetof(canview_command_result_t, feedback_time_ms_le) == 46U, "canview_command_result_t.feedback_time_ms_le offset");
_Static_assert(sizeof(canview_control_lease_request_t) == 52U, "canview_control_lease_request_t wire size");
_Static_assert(offsetof(canview_control_lease_request_t, request_token_le) == 0U, "canview_control_lease_request_t.request_token_le offset");
_Static_assert(offsetof(canview_control_lease_request_t, action) == 8U, "canview_control_lease_request_t.action offset");
_Static_assert(offsetof(canview_control_lease_request_t, reserved0) == 9U, "canview_control_lease_request_t.reserved0 offset");
_Static_assert(offsetof(canview_control_lease_request_t, requested_ms_le) == 12U, "canview_control_lease_request_t.requested_ms_le offset");
_Static_assert(offsetof(canview_control_lease_request_t, lease_id_le) == 16U, "canview_control_lease_request_t.lease_id_le offset");
_Static_assert(offsetof(canview_control_lease_request_t, requested_scope_le) == 24U, "canview_control_lease_request_t.requested_scope_le offset");
_Static_assert(offsetof(canview_control_lease_request_t, reserved1_le) == 26U, "canview_control_lease_request_t.reserved1_le offset");
_Static_assert(offsetof(canview_control_lease_request_t, expected_state_revision_le) == 28U, "canview_control_lease_request_t.expected_state_revision_le offset");
_Static_assert(offsetof(canview_control_lease_request_t, control_generation_le) == 32U, "canview_control_lease_request_t.control_generation_le offset");
_Static_assert(offsetof(canview_control_lease_request_t, control_tag) == 36U, "canview_control_lease_request_t.control_tag offset");
_Static_assert(sizeof(canview_control_lease_status_t) == 60U, "canview_control_lease_status_t wire size");
_Static_assert(offsetof(canview_control_lease_status_t, request_token_le) == 0U, "canview_control_lease_status_t.request_token_le offset");
_Static_assert(offsetof(canview_control_lease_status_t, lease_id_le) == 8U, "canview_control_lease_status_t.lease_id_le offset");
_Static_assert(offsetof(canview_control_lease_status_t, status) == 16U, "canview_control_lease_status_t.status offset");
_Static_assert(offsetof(canview_control_lease_status_t, reserved0) == 17U, "canview_control_lease_status_t.reserved0 offset");
_Static_assert(offsetof(canview_control_lease_status_t, granted_ms_le) == 20U, "canview_control_lease_status_t.granted_ms_le offset");
_Static_assert(offsetof(canview_control_lease_status_t, expires_at_ms_le) == 24U, "canview_control_lease_status_t.expires_at_ms_le offset");
_Static_assert(offsetof(canview_control_lease_status_t, owner_device_id_le) == 28U, "canview_control_lease_status_t.owner_device_id_le offset");
_Static_assert(offsetof(canview_control_lease_status_t, granted_scope_le) == 36U, "canview_control_lease_status_t.granted_scope_le offset");
_Static_assert(offsetof(canview_control_lease_status_t, reserved1_le) == 38U, "canview_control_lease_status_t.reserved1_le offset");
_Static_assert(offsetof(canview_control_lease_status_t, control_generation_le) == 40U, "canview_control_lease_status_t.control_generation_le offset");
_Static_assert(offsetof(canview_control_lease_status_t, control_tag) == 44U, "canview_control_lease_status_t.control_tag offset");
_Static_assert(sizeof(canview_config_get_payload_t) == 24U, "canview_config_get_payload_t wire size");
_Static_assert(offsetof(canview_config_get_payload_t, request_token_le) == 0U, "canview_config_get_payload_t.request_token_le offset");
_Static_assert(offsetof(canview_config_get_payload_t, expected_state_revision_le) == 8U, "canview_config_get_payload_t.expected_state_revision_le offset");
_Static_assert(offsetof(canview_config_get_payload_t, schema_version_le) == 12U, "canview_config_get_payload_t.schema_version_le offset");
_Static_assert(offsetof(canview_config_get_payload_t, reserved0_le) == 14U, "canview_config_get_payload_t.reserved0_le offset");
_Static_assert(offsetof(canview_config_get_payload_t, key_le) == 16U, "canview_config_get_payload_t.key_le offset");
_Static_assert(offsetof(canview_config_get_payload_t, reserved1_le) == 18U, "canview_config_get_payload_t.reserved1_le offset");
_Static_assert(offsetof(canview_config_get_payload_t, known_revision_le) == 20U, "canview_config_get_payload_t.known_revision_le offset");
_Static_assert(sizeof(canview_config_batch_header_t) == 4U, "canview_config_batch_header_t wire size");
_Static_assert(offsetof(canview_config_batch_header_t, schema_version_le) == 0U, "canview_config_batch_header_t.schema_version_le offset");
_Static_assert(offsetof(canview_config_batch_header_t, count) == 2U, "canview_config_batch_header_t.count offset");
_Static_assert(offsetof(canview_config_batch_header_t, reserved) == 3U, "canview_config_batch_header_t.reserved offset");
_Static_assert(sizeof(canview_config_record_t) == 8U, "canview_config_record_t wire size");
_Static_assert(offsetof(canview_config_record_t, key_le) == 0U, "canview_config_record_t.key_le offset");
_Static_assert(offsetof(canview_config_record_t, value_type) == 2U, "canview_config_record_t.value_type offset");
_Static_assert(offsetof(canview_config_record_t, reserved) == 3U, "canview_config_record_t.reserved offset");
_Static_assert(offsetof(canview_config_record_t, value_bits_le) == 4U, "canview_config_record_t.value_bits_le offset");
_Static_assert(sizeof(canview_config_result_payload_t) == 28U, "canview_config_result_payload_t wire size");
_Static_assert(offsetof(canview_config_result_payload_t, request_token_le) == 0U, "canview_config_result_payload_t.request_token_le offset");
_Static_assert(offsetof(canview_config_result_payload_t, stage) == 8U, "canview_config_result_payload_t.stage offset");
_Static_assert(offsetof(canview_config_result_payload_t, applied_count) == 9U, "canview_config_result_payload_t.applied_count offset");
_Static_assert(offsetof(canview_config_result_payload_t, pending_count) == 10U, "canview_config_result_payload_t.pending_count offset");
_Static_assert(offsetof(canview_config_result_payload_t, reserved0) == 11U, "canview_config_result_payload_t.reserved0 offset");
_Static_assert(offsetof(canview_config_result_payload_t, reason_le) == 12U, "canview_config_result_payload_t.reason_le offset");
_Static_assert(offsetof(canview_config_result_payload_t, reserved1_le) == 14U, "canview_config_result_payload_t.reserved1_le offset");
_Static_assert(offsetof(canview_config_result_payload_t, state_revision_le) == 16U, "canview_config_result_payload_t.state_revision_le offset");
_Static_assert(offsetof(canview_config_result_payload_t, completed_time_ms_le) == 20U, "canview_config_result_payload_t.completed_time_ms_le offset");
_Static_assert(offsetof(canview_config_result_payload_t, detail_le) == 24U, "canview_config_result_payload_t.detail_le offset");
_Static_assert(sizeof(canview_config_schema_request_payload_t) == 16U, "canview_config_schema_request_payload_t wire size");
_Static_assert(offsetof(canview_config_schema_request_payload_t, request_token_le) == 0U, "canview_config_schema_request_payload_t.request_token_le offset");
_Static_assert(offsetof(canview_config_schema_request_payload_t, known_schema_version_le) == 8U, "canview_config_schema_request_payload_t.known_schema_version_le offset");
_Static_assert(offsetof(canview_config_schema_request_payload_t, reserved0_le) == 10U, "canview_config_schema_request_payload_t.reserved0_le offset");
_Static_assert(offsetof(canview_config_schema_request_payload_t, known_digest_prefix_le) == 12U, "canview_config_schema_request_payload_t.known_digest_prefix_le offset");
_Static_assert(sizeof(canview_remote_config_request_prefix_t) == 20U, "canview_remote_config_request_prefix_t wire size");
_Static_assert(offsetof(canview_remote_config_request_prefix_t, request_token_le) == 0U, "canview_remote_config_request_prefix_t.request_token_le offset");
_Static_assert(offsetof(canview_remote_config_request_prefix_t, expected_state_revision_le) == 8U, "canview_remote_config_request_prefix_t.expected_state_revision_le offset");
_Static_assert(offsetof(canview_remote_config_request_prefix_t, schema_version_le) == 12U, "canview_remote_config_request_prefix_t.schema_version_le offset");
_Static_assert(offsetof(canview_remote_config_request_prefix_t, count) == 14U, "canview_remote_config_request_prefix_t.count offset");
_Static_assert(offsetof(canview_remote_config_request_prefix_t, flags) == 15U, "canview_remote_config_request_prefix_t.flags offset");
_Static_assert(offsetof(canview_remote_config_request_prefix_t, ttl_ms_le) == 16U, "canview_remote_config_request_prefix_t.ttl_ms_le offset");
_Static_assert(offsetof(canview_remote_config_request_prefix_t, reserved0_le) == 18U, "canview_remote_config_request_prefix_t.reserved0_le offset");
_Static_assert(sizeof(canview_remote_config_status_payload_t) == 28U, "canview_remote_config_status_payload_t wire size");
_Static_assert(offsetof(canview_remote_config_status_payload_t, request_token_le) == 0U, "canview_remote_config_status_payload_t.request_token_le offset");
_Static_assert(offsetof(canview_remote_config_status_payload_t, stage) == 8U, "canview_remote_config_status_payload_t.stage offset");
_Static_assert(offsetof(canview_remote_config_status_payload_t, applied_count) == 9U, "canview_remote_config_status_payload_t.applied_count offset");
_Static_assert(offsetof(canview_remote_config_status_payload_t, pending_count) == 10U, "canview_remote_config_status_payload_t.pending_count offset");
_Static_assert(offsetof(canview_remote_config_status_payload_t, reserved0) == 11U, "canview_remote_config_status_payload_t.reserved0 offset");
_Static_assert(offsetof(canview_remote_config_status_payload_t, reason_le) == 12U, "canview_remote_config_status_payload_t.reason_le offset");
_Static_assert(offsetof(canview_remote_config_status_payload_t, reserved1_le) == 14U, "canview_remote_config_status_payload_t.reserved1_le offset");
_Static_assert(offsetof(canview_remote_config_status_payload_t, state_revision_le) == 16U, "canview_remote_config_status_payload_t.state_revision_le offset");
_Static_assert(offsetof(canview_remote_config_status_payload_t, completed_time_ms_le) == 20U, "canview_remote_config_status_payload_t.completed_time_ms_le offset");
_Static_assert(offsetof(canview_remote_config_status_payload_t, detail_le) == 24U, "canview_remote_config_status_payload_t.detail_le offset");
_Static_assert(sizeof(canview_diagnostic_counters_header_t) == 16U, "canview_diagnostic_counters_header_t wire size");
_Static_assert(offsetof(canview_diagnostic_counters_header_t, boot_id_le) == 0U, "canview_diagnostic_counters_header_t.boot_id_le offset");
_Static_assert(offsetof(canview_diagnostic_counters_header_t, state_revision_le) == 8U, "canview_diagnostic_counters_header_t.state_revision_le offset");
_Static_assert(offsetof(canview_diagnostic_counters_header_t, count) == 12U, "canview_diagnostic_counters_header_t.count offset");
_Static_assert(offsetof(canview_diagnostic_counters_header_t, reserved0) == 13U, "canview_diagnostic_counters_header_t.reserved0 offset");
_Static_assert(sizeof(canview_diagnostic_counter_record_t) == 16U, "canview_diagnostic_counter_record_t wire size");
_Static_assert(offsetof(canview_diagnostic_counter_record_t, counter_id_le) == 0U, "canview_diagnostic_counter_record_t.counter_id_le offset");
_Static_assert(offsetof(canview_diagnostic_counter_record_t, category) == 2U, "canview_diagnostic_counter_record_t.category offset");
_Static_assert(offsetof(canview_diagnostic_counter_record_t, reserved0) == 3U, "canview_diagnostic_counter_record_t.reserved0 offset");
_Static_assert(offsetof(canview_diagnostic_counter_record_t, value_le) == 4U, "canview_diagnostic_counter_record_t.value_le offset");
_Static_assert(offsetof(canview_diagnostic_counter_record_t, saturated) == 12U, "canview_diagnostic_counter_record_t.saturated offset");
_Static_assert(offsetof(canview_diagnostic_counter_record_t, reserved1) == 13U, "canview_diagnostic_counter_record_t.reserved1 offset");
_Static_assert(sizeof(canview_bulk_begin_payload_t) == 72U, "canview_bulk_begin_payload_t wire size");
_Static_assert(offsetof(canview_bulk_begin_payload_t, request_token_le) == 0U, "canview_bulk_begin_payload_t.request_token_le offset");
_Static_assert(offsetof(canview_bulk_begin_payload_t, object_id) == 8U, "canview_bulk_begin_payload_t.object_id offset");
_Static_assert(offsetof(canview_bulk_begin_payload_t, object_type_le) == 24U, "canview_bulk_begin_payload_t.object_type_le offset");
_Static_assert(offsetof(canview_bulk_begin_payload_t, flags_le) == 26U, "canview_bulk_begin_payload_t.flags_le offset");
_Static_assert(offsetof(canview_bulk_begin_payload_t, total_size_le) == 28U, "canview_bulk_begin_payload_t.total_size_le offset");
_Static_assert(offsetof(canview_bulk_begin_payload_t, fragment_size_le) == 32U, "canview_bulk_begin_payload_t.fragment_size_le offset");
_Static_assert(offsetof(canview_bulk_begin_payload_t, window_size) == 34U, "canview_bulk_begin_payload_t.window_size offset");
_Static_assert(offsetof(canview_bulk_begin_payload_t, reserved0) == 35U, "canview_bulk_begin_payload_t.reserved0 offset");
_Static_assert(offsetof(canview_bulk_begin_payload_t, sha256) == 36U, "canview_bulk_begin_payload_t.sha256 offset");
_Static_assert(offsetof(canview_bulk_begin_payload_t, timeout_ms_le) == 68U, "canview_bulk_begin_payload_t.timeout_ms_le offset");
_Static_assert(sizeof(canview_bulk_fragment_prefix_t) == 28U, "canview_bulk_fragment_prefix_t wire size");
_Static_assert(offsetof(canview_bulk_fragment_prefix_t, object_id) == 0U, "canview_bulk_fragment_prefix_t.object_id offset");
_Static_assert(offsetof(canview_bulk_fragment_prefix_t, fragment_index_le) == 16U, "canview_bulk_fragment_prefix_t.fragment_index_le offset");
_Static_assert(offsetof(canview_bulk_fragment_prefix_t, total_fragments_le) == 20U, "canview_bulk_fragment_prefix_t.total_fragments_le offset");
_Static_assert(offsetof(canview_bulk_fragment_prefix_t, payload_len_le) == 24U, "canview_bulk_fragment_prefix_t.payload_len_le offset");
_Static_assert(offsetof(canview_bulk_fragment_prefix_t, flags_le) == 26U, "canview_bulk_fragment_prefix_t.flags_le offset");
_Static_assert(sizeof(canview_bulk_ack_payload_t) == 32U, "canview_bulk_ack_payload_t wire size");
_Static_assert(offsetof(canview_bulk_ack_payload_t, object_id) == 0U, "canview_bulk_ack_payload_t.object_id offset");
_Static_assert(offsetof(canview_bulk_ack_payload_t, base_fragment_le) == 16U, "canview_bulk_ack_payload_t.base_fragment_le offset");
_Static_assert(offsetof(canview_bulk_ack_payload_t, received_bitmap_le) == 20U, "canview_bulk_ack_payload_t.received_bitmap_le offset");
_Static_assert(offsetof(canview_bulk_ack_payload_t, received_bytes_le) == 24U, "canview_bulk_ack_payload_t.received_bytes_le offset");
_Static_assert(offsetof(canview_bulk_ack_payload_t, window_size) == 28U, "canview_bulk_ack_payload_t.window_size offset");
_Static_assert(offsetof(canview_bulk_ack_payload_t, status) == 29U, "canview_bulk_ack_payload_t.status offset");
_Static_assert(offsetof(canview_bulk_ack_payload_t, reserved0_le) == 30U, "canview_bulk_ack_payload_t.reserved0_le offset");
_Static_assert(sizeof(canview_bulk_end_payload_t) == 60U, "canview_bulk_end_payload_t wire size");
_Static_assert(offsetof(canview_bulk_end_payload_t, object_id) == 0U, "canview_bulk_end_payload_t.object_id offset");
_Static_assert(offsetof(canview_bulk_end_payload_t, status) == 16U, "canview_bulk_end_payload_t.status offset");
_Static_assert(offsetof(canview_bulk_end_payload_t, reserved0) == 17U, "canview_bulk_end_payload_t.reserved0 offset");
_Static_assert(offsetof(canview_bulk_end_payload_t, total_size_le) == 20U, "canview_bulk_end_payload_t.total_size_le offset");
_Static_assert(offsetof(canview_bulk_end_payload_t, sha256) == 24U, "canview_bulk_end_payload_t.sha256 offset");
_Static_assert(offsetof(canview_bulk_end_payload_t, reason_le) == 56U, "canview_bulk_end_payload_t.reason_le offset");
_Static_assert(offsetof(canview_bulk_end_payload_t, reserved1_le) == 58U, "canview_bulk_end_payload_t.reserved1_le offset");
#endif

#if defined(_MSC_VER)
#pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* CANVIEW_PROTOCOL_H */
