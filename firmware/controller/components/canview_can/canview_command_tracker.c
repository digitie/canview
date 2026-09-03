#include "canview_command_tracker.h"

#include <stddef.h>
#include <string.h>

static uint16_t read_le16(const uint8_t *bytes)
{
    return (uint16_t)bytes[0] | (uint16_t)((uint16_t)bytes[1] << 8U);
}

static uint64_t read_le64(const uint8_t *bytes)
{
    uint64_t value = 0U;
    for (uint8_t i = 0; i < 8U; ++i) {
        value |= (uint64_t)bytes[i] << (uint8_t)(i * 8U);
    }
    return value;
}

static int find_slot(const canview_command_tracker_t *tracker, uint64_t token)
{
    /* The tracker is a pending-state table, not the durable idempotency
     * history. Reclaim terminal slots when all pending slots are occupied. */
    for (size_t i = 0; i < CANVIEW_COMMAND_TRACKER_MAX_PENDING; ++i) {
        if (tracker->slots[i].in_use && tracker->slots[i].request_token == token) {
            return (int)i;
        }
    }
    return -1;
}

static int find_free_slot(const canview_command_tracker_t *tracker)
{
    for (size_t i = 0; i < CANVIEW_COMMAND_TRACKER_MAX_PENDING; ++i) {
        if (!tracker->slots[i].in_use) {
            return (int)i;
        }
    }
    for (size_t i = 0; i < CANVIEW_COMMAND_TRACKER_MAX_PENDING; ++i) {
        const canview_command_track_status_t status = tracker->slots[i].status;
        if (status == CANVIEW_COMMAND_TRACK_COMPLETED ||
            status == CANVIEW_COMMAND_TRACK_FAILED ||
            status == CANVIEW_COMMAND_TRACK_EXPIRED) {
            return (int)i;
        }
    }
    return -1;
}

void canview_command_tracker_init(canview_command_tracker_t *tracker)
{
    if (tracker != NULL) {
        memset(tracker, 0, sizeof(*tracker));
    }
}

bool canview_command_tracker_begin(canview_command_tracker_t *tracker,
                                   uint64_t request_token,
                                   uint16_t command_id,
                                   uint16_t ttl_ms,
                                   uint32_t now_ms)
{
    if (tracker == NULL || request_token == 0U || command_id == 0U || ttl_ms == 0U ||
        find_slot(tracker, request_token) >= 0) {
        return false;
    }
    const int index = find_free_slot(tracker);
    if (index < 0) {
        return false;
    }
    tracker->slots[index] = (canview_command_track_slot_t){
        .in_use = true,
        .request_token = request_token,
        .command_id = command_id,
        .ttl_ms = ttl_ms,
        .sent_at_ms = now_ms,
        .last_send_ms = now_ms,
        .status = CANVIEW_COMMAND_TRACK_WAITING_ACK,
    };
    return true;
}

bool canview_command_tracker_on_ack(canview_command_tracker_t *tracker,
                                    uint64_t request_token,
                                    canview_ack_status_t status)
{
    if (tracker == NULL) {
        return false;
    }
    const int index = find_slot(tracker, request_token);
    if (index < 0) {
        return false;
    }
    canview_command_track_slot_t *slot = &tracker->slots[index];
    if (slot->status == CANVIEW_COMMAND_TRACK_COMPLETED ||
        slot->status == CANVIEW_COMMAND_TRACK_FAILED ||
        slot->status == CANVIEW_COMMAND_TRACK_EXPIRED) {
        return status == CANVIEW_ACK_ACCEPTED || status == CANVIEW_ACK_DUPLICATE;
    }
    switch (status) {
    case CANVIEW_ACK_ACCEPTED:
    case CANVIEW_ACK_DUPLICATE:
        slot->status = CANVIEW_COMMAND_TRACK_WAITING_RESULT;
        return true;
    case CANVIEW_ACK_BUSY:
    case CANVIEW_ACK_MALFORMED:
    case CANVIEW_ACK_UNSUPPORTED:
    case CANVIEW_ACK_UNAUTHENTICATED:
    case CANVIEW_ACK_EXPIRED:
    default:
        slot->status = CANVIEW_COMMAND_TRACK_FAILED;
        return false;
    }
}

bool canview_command_tracker_on_result(
    canview_command_tracker_t *tracker,
    const canview_command_result_t *result)
{
    if (tracker == NULL || result == NULL) {
        return false;
    }
    const uint64_t token = read_le64((const uint8_t *)&result->request_token_le);
    const uint16_t command_id = read_le16((const uint8_t *)&result->command_id_le);
    const int index = find_slot(tracker, token);
    if (index < 0 || tracker->slots[index].command_id != command_id) {
        return false;
    }
    if (tracker->slots[index].status == CANVIEW_COMMAND_TRACK_COMPLETED ||
        tracker->slots[index].status == CANVIEW_COMMAND_TRACK_FAILED ||
        tracker->slots[index].status == CANVIEW_COMMAND_TRACK_EXPIRED) {
        return result->stage == CANVIEW_COMMAND_COMPLETED &&
               tracker->slots[index].status == CANVIEW_COMMAND_TRACK_COMPLETED;
    }
    switch ((canview_command_stage_t)result->stage) {
    case CANVIEW_COMMAND_ACCEPTED:
    case CANVIEW_COMMAND_EXECUTING:
        tracker->slots[index].status = CANVIEW_COMMAND_TRACK_WAITING_RESULT;
        break;
    case CANVIEW_COMMAND_COMPLETED:
        tracker->slots[index].status = CANVIEW_COMMAND_TRACK_COMPLETED;
        break;
    case CANVIEW_COMMAND_EXPIRED:
        tracker->slots[index].status = CANVIEW_COMMAND_TRACK_EXPIRED;
        break;
    case CANVIEW_COMMAND_REJECTED:
    case CANVIEW_COMMAND_CANCELLED:
    case CANVIEW_COMMAND_FAILED:
    default:
        tracker->slots[index].status = CANVIEW_COMMAND_TRACK_FAILED;
        break;
    }
    return true;
}

bool canview_command_tracker_should_retry(
    const canview_command_tracker_t *tracker,
    uint64_t request_token,
    uint32_t now_ms)
{
    if (tracker == NULL) {
        return false;
    }
    const int index = find_slot(tracker, request_token);
    if (index < 0) {
        return false;
    }
    const canview_command_track_slot_t *slot = &tracker->slots[index];
    if (slot->status != CANVIEW_COMMAND_TRACK_WAITING_ACK ||
        slot->retry_count >= CANVIEW_COMMAND_TRACKER_MAX_RETRIES) {
        return false;
    }
    /* ACK retry delays are deliberately independent of result completion.
     * The token remains unchanged across retries; only the packet sequence
     * changes at the transport layer. */
    uint32_t retry_delay = 80U;
    for (uint8_t retry = 0U; retry < slot->retry_count; ++retry) {
        retry_delay = retry_delay > 125U ? 250U : retry_delay * 2U;
    }
    if (retry_delay > 250U) {
        retry_delay = 250U;
    }
    const uint32_t ttl_half = slot->ttl_ms / 2U;
    if (retry_delay > ttl_half) {
        retry_delay = ttl_half;
    }
    return retry_delay > 0U && (uint32_t)(now_ms - slot->last_send_ms) >= retry_delay;
}

bool canview_command_tracker_mark_retry(canview_command_tracker_t *tracker,
                                        uint64_t request_token,
                                        uint32_t now_ms)
{
    if (!canview_command_tracker_should_retry(tracker, request_token, now_ms)) {
        return false;
    }
    const int index = find_slot(tracker, request_token);
    ++tracker->slots[index].retry_count;
    tracker->slots[index].last_send_ms = now_ms;
    return true;
}

void canview_command_tracker_expire(canview_command_tracker_t *tracker,
                                    uint32_t now_ms)
{
    if (tracker == NULL) {
        return;
    }
    for (size_t i = 0; i < CANVIEW_COMMAND_TRACKER_MAX_PENDING; ++i) {
        canview_command_track_slot_t *slot = &tracker->slots[i];
        if (slot->in_use &&
            (slot->status == CANVIEW_COMMAND_TRACK_WAITING_ACK ||
             slot->status == CANVIEW_COMMAND_TRACK_WAITING_RESULT) &&
            (uint32_t)(now_ms - slot->sent_at_ms) >= slot->ttl_ms) {
            slot->status = CANVIEW_COMMAND_TRACK_EXPIRED;
        }
    }
}

canview_command_track_status_t canview_command_tracker_status(
    const canview_command_tracker_t *tracker,
    uint64_t request_token)
{
    if (tracker == NULL) {
        return CANVIEW_COMMAND_TRACK_EMPTY;
    }
    const int index = find_slot(tracker, request_token);
    return index < 0 ? CANVIEW_COMMAND_TRACK_EMPTY : tracker->slots[index].status;
}
