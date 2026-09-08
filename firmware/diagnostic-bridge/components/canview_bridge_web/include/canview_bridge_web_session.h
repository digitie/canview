/* SPDX-License-Identifier: GPL-3.0-only */
/** @file canview_bridge_web_session.h
 *  @brief Diagnostic Bridge web client ownership and timeout state.
 */
#ifndef CANVIEW_BRIDGE_WEB_SESSION_H
#define CANVIEW_BRIDGE_WEB_SESSION_H

#include <stdbool.h>
#include <stdint.h>

/** @brief SDK-independent state for the single-client web service. */
typedef struct
{
    int active_client_fd;
    bool session_close_pending;
    uint64_t last_activity_ms;
    bool client_activity_valid;
} canview_bridge_web_session_t;

/** @brief Result of recording an HTTP or WebSocket activity event. */
typedef enum
{
    CANVIEW_BRIDGE_WEB_SESSION_OK = 0,
    CANVIEW_BRIDGE_WEB_SESSION_INVALID_ARGUMENT,
    CANVIEW_BRIDGE_WEB_SESSION_CLOSING,
    CANVIEW_BRIDGE_WEB_SESSION_OTHER_CLIENT
} canview_bridge_web_session_result_t;

/** @brief Initialize an empty session. */
void canview_bridge_web_session_init(canview_bridge_web_session_t *session);

/** @brief Return whether a previous client is being closed. */
bool canview_bridge_web_session_is_closing(const canview_bridge_web_session_t *session);

/** @brief Return whether client_fd conflicts with the current owner. */
bool canview_bridge_web_session_has_other_client(const canview_bridge_web_session_t *session,
                                                 int client_fd);

/** @brief Claim the client or refresh its activity timestamp. */
canview_bridge_web_session_result_t canview_bridge_web_session_record_activity(
    canview_bridge_web_session_t *session, int client_fd, uint64_t now_ms);

/** @brief Determine whether the client expired or the clock moved backwards. */
bool canview_bridge_web_session_idle_expired(const canview_bridge_web_session_t *session,
                                             uint64_t now_ms, uint64_t timeout_ms);

/** @brief Mark the owner for close and return its descriptor. */
bool canview_bridge_web_session_begin_close(canview_bridge_web_session_t *session,
                                            int *client_fd);

/** @brief Complete close callback cleanup for the matching descriptor. */
bool canview_bridge_web_session_close(canview_bridge_web_session_t *session, int client_fd);

#endif
