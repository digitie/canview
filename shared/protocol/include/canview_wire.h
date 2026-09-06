/* SPDX-License-Identifier: GPL-3.0-only */
/** @file canview_wire.h
 * @brief MCU 독립 framing/telemetry API. 승인/인증/차량 송신 기능은 제공하지 않는다.
 */
#ifndef CANVIEW_WIRE_H
#define CANVIEW_WIRE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "canview_status.h"
#include "canview_wire_layout.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    CANVIEW_WIRE_ESPNOW = 0,
    CANVIEW_WIRE_UART = 1
} canview_wire_transport_t;

/* Host representation only. Never memcpy/cast this struct onto the wire. */
typedef struct
{
    uint8_t message_type;
    uint8_t flags;
    uint8_t priority;    /* UART requires zero. */
    uint32_t session_id; /* UART requires zero. */
    uint32_t sequence;
    uint32_t correlation_id;
    uint64_t sender_time; /* ESP-NOW ms <= UINT32_MAX; UART us. */
} canview_wire_header_t;

typedef struct
{
    canview_wire_header_t header;
    const uint8_t *payload;
    size_t payload_size;
} canview_wire_view_t;

typedef struct
{
    uint32_t newest;
    uint64_t seen;
    bool initialized;
} canview_sequence_window_t;

typedef struct
{
    uint16_t delta_us;
    uint8_t bus_id;
    uint8_t flags; /* low nibble: IDE=1, RTR=2, error=4, TX echo=8 */
    uint8_t dlc;
    uint32_t can_id;
    uint8_t data[8];
} canview_wire_can_record_t;

typedef struct
{
    uint64_t base_time_us;
    uint8_t dropped_since_last;
    uint8_t count;
    canview_wire_can_record_t records[CANVIEW_WIRE_CAN_MAX_RECORDS];
} canview_wire_can_batch_t;

typedef struct
{
    size_t used;
    bool discarding;
    uint8_t encoded[CANVIEW_WIRE_UART_MAX_ENCODED];
    uint8_t decoded[CANVIEW_WIRE_UART_MAX_FRAME];
} canview_uart_stream_t;

/** @brief CRC-32/ISO-HDLC. NULL allowed only for size=0. Not authentication.
 * @return CANVIEW_OK or CANVIEW_INVALID_ARGUMENT. Output unchanged on error.
 * @param bytes 입력 바이트. size=0일 때만 NULL 허용.
 * @param size 입력 길이.
 * @param crc 필수 결과 포인터. 오류 시 기존 값 유지.
 */
canview_status_t canview_wire_crc32(const uint8_t *bytes, size_t size, uint32_t *crc);

/** @brief Encode a structural envelope, not a permission/semantic check.
 * Payload is opaque; no CAN/control dispatch, crypto, or session admission.
 * Input/output memory must not overlap. Caller owns all buffers; not ISR work.
 * @return CANVIEW_OK; written=0 on error, output may be changed on error.
 * @param transport ESP-NOW 또는 UART.
 * @param header 필수 host header. wire struct가 아니다.
 * @param payload 호출자 소유 불변 payload. 길이 0일 때 NULL 허용.
 * @param payload_size payload 바이트 수.
 * @param out 입력과 겹치지 않는 출력 버퍼.
 * @param capacity 출력 버퍼 크기.
 * @param written 필수 실제 길이. 실패 시 0.
 */
canview_status_t canview_wire_envelope_encode(canview_wire_transport_t transport,
                                              const canview_wire_header_t *header,
                                              const uint8_t *payload, size_t payload_size,
                                              uint8_t *out, size_t capacity, size_t *written);

/** @brief Validate fixed exact version/size/reserved/CRC and decode envelope.
 * Success means structural bytes ONLY, never authenticated/ONLINE/DELIVER.
 * Version must be ESP-NOW1.3 or UART1.0; newer minor is not implicitly accepted.
 * View borrows input until input changes. View is zeroed on failure.
 * @param transport 정확한 전송 종류.
 * @param bytes 불변 입력 프레임.
 * @param size header를 포함한 전체 바이트 수.
 * @param view 필수 borrowed view. 오류 시 0 초기화.
 * @return 성공은 CANVIEW_OK. 나머지는 상태 enum 및 상세 오류 계약 참조.
 */
canview_status_t canview_wire_envelope_decode(canview_wire_transport_t transport,
                                              const uint8_t *bytes, size_t size,
                                              canview_wire_view_t *view);

/** @brief Encode bounded COBS without delimiter. Non-overlapping buffers.
 * @return written=0 on failure; output may be changed. NULL input only size=0.
 * @param bytes 입력 버퍼. size=0일 때 NULL 허용.
 * @param size 최대 1024바이트.
 * @param out 입력과 겹치지 않는 출력.
 * @param capacity 출력 용량.
 * @param written 필수 실제 길이. 실패 시 0.
 */
canview_status_t canview_wire_cobs_encode(const uint8_t *bytes, size_t size, uint8_t *out,
                                          size_t capacity, size_t *written);

/** @brief Decode COBS without delimiter. Empty encoded packet is malformed.
 * Non-overlapping buffers; written=0 on failure, output may be changed.
 * @param bytes delimiter를 제외한 비어 있지 않은 COBS 입력.
 * @param size 입력 바이트 수.
 * @param out 입력과 겹치지 않는 출력.
 * @param capacity 출력 용량.
 * @param written 필수 실제 길이. 실패 시 0.
 * @return 성공은 CANVIEW_OK. 나머지는 상태 enum 및 상세 오류 계약 참조.
 */
canview_status_t canview_wire_cobs_decode(const uint8_t *bytes, size_t size, uint8_t *out,
                                          size_t capacity, size_t *written);

/** @brief Initialize/reset caller-owned UART parser. NULL is rejected.
 * @param stream 필수 caller 소유 context. 기존 view 수명 종료.
 * @return 성공은 CANVIEW_OK. 나머지는 상태 enum 및 상세 오류 계약 참조.
 */
canview_status_t canview_uart_stream_reset(canview_uart_stream_t *stream);

/** @brief Feed one UART byte in worker context; no heap, callbacks or TX.
 * OK only on a complete structurally valid envelope. INCOMPLETE includes empty
 * delimiters. OVERSIZE once per excessive packet, then discard until delimiter.
 * View borrows stream.decoded and expires on next feed/reset. Single owner.
 * Caller MUST validate message/auth/session/role before dispatch or ACK.
 * @param stream reset을 거친 단일 worker 소유 parser.
 * @param byte 다음 UART 바이트. 0은 경계.
 * @param view 필수 결과. 다음 feed/reset 전까지만 유효.
 * @return 성공은 CANVIEW_OK. 나머지는 상태 enum 및 상세 오류 계약 참조.
 */
canview_status_t canview_uart_stream_feed(canview_uart_stream_t *stream, uint8_t byte,
                                          canview_wire_view_t *view);

/** @brief Encode UART envelope + COBS + delimiter with caller scratch.
 * scratch/out/payload must not overlap. No serial I/O or retry occurs.
 * @param header UART host header. session_id와 priority는 0.
 * @param payload 입력 payload. 길이 0일 때 NULL 허용.
 * @param payload_size 최대 992바이트.
 * @param scratch 출력/payload와 겹치지 않는 임시 프레임 버퍼.
 * @param scratch_size 임시 버퍼 크기. 최대 프레임에는 1024 필요.
 * @param out delimiter를 포함한 직렬화 결과.
 * @param capacity 최대 프레임용 권장 1030바이트.
 * @param written 필수 실제 길이. 실패 시 0.
 * @return 성공은 CANVIEW_OK. 나머지는 상태 enum 및 상세 오류 계약 참조.
 */
canview_status_t canview_uart_packet_encode(const canview_wire_header_t *header,
                                            const uint8_t *payload, size_t payload_size,
                                            uint8_t *scratch, size_t scratch_size, uint8_t *out,
                                            size_t capacity, size_t *written);

/** @brief Decode classic CAN batch (12 prefix + up to12*16).
 * Reject invalid ID/DLC/bus/reserved/padding. Output zeroed on error.
 * This is telemetry only; it never creates a vehicle transmit request.
 * @param bytes telemetry batch 입력.
 * @param size 정확히 12+count*16바이트.
 * @param batch 필수 caller 소유 결과. 오류 시 0 초기화.
 * @return 성공은 CANVIEW_OK. 나머지는 상태 enum 및 상세 오류 계약 참조.
 */
canview_status_t canview_wire_can_batch_decode(const uint8_t *bytes, size_t size,
                                               canview_wire_can_batch_t *batch);

/** @brief Encode canonical classic CAN batch; trailing data bytes must be zero.
 * Non-overlapping buffers. written=0 on failure.
 * @param batch 검증할 입력. RTR 및 DLC 이후 data는 0.
 * @param out 입력과 겹치지 않는 출력.
 * @param capacity 출력 용량. 최대 204바이트.
 * @param written 필수 실제 길이. 실패 시 0.
 * @return 성공은 CANVIEW_OK. 나머지는 상태 enum 및 상세 오류 계약 참조.
 */
canview_status_t canview_wire_can_batch_encode(const canview_wire_can_batch_t *batch, uint8_t *out,
                                               size_t capacity, size_t *written);

/** @brief Initialize/reset per-session caller-owned replay window.
 * @param window 필수 session별 caller 소유 상태.
 * @return 성공은 CANVIEW_OK. 나머지는 상태 enum 및 상세 오류 계약 참조.
 */
canview_status_t canview_sequence_window_reset(canview_sequence_window_t *window);

/** @brief Commit an ALREADY authenticated/validated sequence into 64-bit window.
 * Single owner. Caller can probe a COPY before transactional queue admission;
 * commit original only after admission. Duplicate/stale leaves state unchanged.
 * Exactly half-range is ambiguous and rejected; reset on boot/session change.
 * @param window 필수 인증된 session별 단일-owner 상태.
 * @param sequence 인증 및 queue admission이 완료된 sequence.
 * @return 성공은 CANVIEW_OK. 나머지는 상태 enum 및 상세 오류 계약 참조.
 */
canview_status_t canview_sequence_window_accept(canview_sequence_window_t *window,
                                                uint32_t sequence);

#ifdef __cplusplus
}
#endif
#endif
