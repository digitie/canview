/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_bridge_web_session.h"

#include <stddef.h>

void canview_bridge_web_session_init(canview_bridge_web_session_t *session)
{
    if (session == NULL)
    {
        return;
    }
    session->active_client_fd = -1;
    session->session_close_pending = false;
    session->last_activity_ms = 0U;
    session->client_activity_valid = false;
}

bool canview_bridge_web_session_is_closing(const canview_bridge_web_session_t *session)
{
    return session != NULL && session->session_close_pending;
}

bool canview_bridge_web_session_has_other_client(const canview_bridge_web_session_t *session,
                                                 int client_fd)
{
    return session != NULL && client_fd >= 0 && session->active_client_fd >= 0 &&
           session->active_client_fd != client_fd;
}

canview_bridge_web_session_result_t canview_bridge_web_session_record_activity(
    canview_bridge_web_session_t *session, int client_fd, uint64_t now_ms)
{
    if (session == NULL || client_fd < 0)
    {
        return CANVIEW_BRIDGE_WEB_SESSION_INVALID_ARGUMENT;
    }
    if (session->session_close_pending)
    {
        return CANVIEW_BRIDGE_WEB_SESSION_CLOSING;
    }
    if (canview_bridge_web_session_has_other_client(session, client_fd))
    {
        return CANVIEW_BRIDGE_WEB_SESSION_OTHER_CLIENT;
    }
    if (session->active_client_fd < 0)
    {
        session->active_client_fd = client_fd;
    }
    session->last_activity_ms = now_ms;
    session->client_activity_valid = true;
    return CANVIEW_BRIDGE_WEB_SESSION_OK;
}

bool canview_bridge_web_session_idle_expired(const canview_bridge_web_session_t *session,
                                             uint64_t now_ms, uint64_t timeout_ms)
{
    if (session == NULL || session->active_client_fd < 0 || !session->client_activity_valid)
    {
        return false;
    }
    return now_ms < session->last_activity_ms ||
           now_ms - session->last_activity_ms >= timeout_ms;
}

bool canview_bridge_web_session_begin_close(canview_bridge_web_session_t *session,
                                            int *client_fd)
{
    if (session == NULL || client_fd == NULL || session->active_client_fd < 0 ||
        session->session_close_pending)
    {
        return false;
    }
    *client_fd = session->active_client_fd;
    session->session_close_pending = true;
    session->last_activity_ms = 0U;
    session->client_activity_valid = false;
    return true;
}

bool canview_bridge_web_session_close(canview_bridge_web_session_t *session, int client_fd)
{
    if (session == NULL || client_fd < 0 || session->active_client_fd != client_fd)
    {
        return false;
    }
    canview_bridge_web_session_init(session);
    return true;
}
