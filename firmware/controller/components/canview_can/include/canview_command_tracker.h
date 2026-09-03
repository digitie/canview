#ifndef CANVIEW_COMMAND_TRACKER_H
#define CANVIEW_COMMAND_TRACKER_H

#include <stdbool.h>
#include <stdint.h>

#include "canview_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CANVIEW_COMMAND_TRACK_EMPTY = 0,
    CANVIEW_COMMAND_TRACK_WAITING_ACK,
    CANVIEW_COMMAND_TRACK_WAITING_RESULT,
    CANVIEW_COMMAND_TRACK_COMPLETED,
    CANVIEW_COMMAND_TRACK_FAILED,
    CANVIEW_COMMAND_TRACK_EXPIRED,
} canview_command_track_status_t;

typedef struct {
    bool in_use;
    uint64_t request_token;
    uint16_t command_id;
    uint16_t ttl_ms;
    uint32_t sent_at_ms;
    uint32_t last_send_ms;
    uint8_t retry_count;
    canview_command_track_status_t status;
} canview_command_track_slot_t;

typedef struct {
    canview_command_track_slot_t slots[CANVIEW_COMMAND_TRACKER_MAX_PENDING];
} canview_command_tracker_t;

void canview_command_tracker_init(canview_command_tracker_t *tracker);

bool canview_command_tracker_begin(canview_command_tracker_t *tracker,
                                   uint64_t request_token,
                                   uint16_t command_id,
                                   uint16_t ttl_ms,
                                   uint32_t now_ms);

bool canview_command_tracker_on_ack(canview_command_tracker_t *tracker,
                                    uint64_t request_token,
                                    canview_ack_status_t status);

bool canview_command_tracker_on_result(
    canview_command_tracker_t *tracker,
    const canview_command_result_t *result);

bool canview_command_tracker_should_retry(
    const canview_command_tracker_t *tracker,
    uint64_t request_token,
    uint32_t now_ms);

bool canview_command_tracker_mark_retry(canview_command_tracker_t *tracker,
                                        uint64_t request_token,
                                        uint32_t now_ms);

void canview_command_tracker_expire(canview_command_tracker_t *tracker,
                                    uint32_t now_ms);

canview_command_track_status_t canview_command_tracker_status(
    const canview_command_tracker_t *tracker,
    uint64_t request_token);

#ifdef __cplusplus
}
#endif

#endif
