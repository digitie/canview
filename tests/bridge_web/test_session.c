/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_bridge_web_session.h"

#include <inttypes.h>
#include <stdio.h>

#define CHECK(condition)                                                                          \
    do                                                                                             \
    {                                                                                              \
        if (!(condition))                                                                          \
        {                                                                                          \
            (void)fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition);               \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)

#define TEST_TIMEOUT_MS UINT64_C(300000)

static int test_null_and_empty_state(void)
{
    canview_bridge_web_session_t session = {0};
    canview_bridge_web_session_init(NULL);
    CHECK(!canview_bridge_web_session_is_closing(NULL));
    CHECK(!canview_bridge_web_session_has_other_client(NULL, 1));
    CHECK(canview_bridge_web_session_record_activity(NULL, 1, 0U) ==
          CANVIEW_BRIDGE_WEB_SESSION_INVALID_ARGUMENT);
    CHECK(canview_bridge_web_session_record_activity(&session, -1, 0U) ==
          CANVIEW_BRIDGE_WEB_SESSION_INVALID_ARGUMENT);
    CHECK(!canview_bridge_web_session_idle_expired(NULL, 0U, TEST_TIMEOUT_MS));
    CHECK(!canview_bridge_web_session_begin_close(NULL, NULL));
    CHECK(!canview_bridge_web_session_close(NULL, 1));
    canview_bridge_web_session_init(&session);
    CHECK(session.active_client_fd == -1 && !session.session_close_pending &&
          session.last_activity_ms == 0U && !session.client_activity_valid);
    CHECK(!canview_bridge_web_session_idle_expired(&session, UINT64_MAX, 0U));
    return 0;
}

static int test_activity_and_timeout_boundaries(void)
{
    canview_bridge_web_session_t session = {0};
    canview_bridge_web_session_init(&session);
    CHECK(canview_bridge_web_session_record_activity(&session, 7, 1000U) ==
          CANVIEW_BRIDGE_WEB_SESSION_OK);
    CHECK(session.active_client_fd == 7 && session.last_activity_ms == 1000U &&
          session.client_activity_valid);
    CHECK(!canview_bridge_web_session_has_other_client(&session, 7));
    CHECK(canview_bridge_web_session_has_other_client(&session, 8));
    CHECK(canview_bridge_web_session_record_activity(&session, 8, 1001U) ==
          CANVIEW_BRIDGE_WEB_SESSION_OTHER_CLIENT);
    CHECK(session.last_activity_ms == 1000U);
    CHECK(!canview_bridge_web_session_idle_expired(&session, 1000U + TEST_TIMEOUT_MS - 1U,
                                                   TEST_TIMEOUT_MS));
    CHECK(canview_bridge_web_session_idle_expired(&session, 1000U + TEST_TIMEOUT_MS,
                                                  TEST_TIMEOUT_MS));
    CHECK(canview_bridge_web_session_idle_expired(&session, 999U, TEST_TIMEOUT_MS));
    CHECK(canview_bridge_web_session_record_activity(&session, 7, 2000U) ==
          CANVIEW_BRIDGE_WEB_SESSION_OK);
    CHECK(!canview_bridge_web_session_idle_expired(&session, 2000U + TEST_TIMEOUT_MS - 1U,
                                                   TEST_TIMEOUT_MS));
    return 0;
}

static int test_close_and_fd_reuse(void)
{
    canview_bridge_web_session_t session = {0};
    int client_fd = -1;
    canview_bridge_web_session_init(&session);
    CHECK(canview_bridge_web_session_record_activity(&session, 11, 5000U) ==
          CANVIEW_BRIDGE_WEB_SESSION_OK);
    CHECK(canview_bridge_web_session_begin_close(&session, &client_fd));
    CHECK(client_fd == 11 && canview_bridge_web_session_is_closing(&session));
    CHECK(!session.client_activity_valid && session.last_activity_ms == 0U);
    CHECK(canview_bridge_web_session_record_activity(&session, 11, 5001U) ==
          CANVIEW_BRIDGE_WEB_SESSION_CLOSING);
    CHECK(canview_bridge_web_session_record_activity(&session, 12, 5001U) ==
          CANVIEW_BRIDGE_WEB_SESSION_CLOSING);
    CHECK(!canview_bridge_web_session_begin_close(&session, &client_fd));
    CHECK(!canview_bridge_web_session_close(&session, 12));
    CHECK(canview_bridge_web_session_is_closing(&session));
    CHECK(canview_bridge_web_session_close(&session, 11));
    CHECK(!canview_bridge_web_session_is_closing(&session) && session.active_client_fd == -1);
    CHECK(canview_bridge_web_session_record_activity(&session, 12, 6000U) ==
          CANVIEW_BRIDGE_WEB_SESSION_OK);
    return 0;
}

static int test_close_trigger_failure_is_fail_closed(void)
{
    canview_bridge_web_session_t session = {0};
    int client_fd = -1;
    canview_bridge_web_session_init(&session);
    CHECK(canview_bridge_web_session_record_activity(&session, 21, 9000U) ==
          CANVIEW_BRIDGE_WEB_SESSION_OK);
    CHECK(canview_bridge_web_session_begin_close(&session, &client_fd) && client_fd == 21);

    /* A failed SDK close trigger must leave a bounded retry/stop state, not re-admit a client. */
    CHECK(canview_bridge_web_session_is_closing(&session));
    CHECK(!canview_bridge_web_session_idle_expired(&session, UINT64_MAX, TEST_TIMEOUT_MS));
    CHECK(canview_bridge_web_session_record_activity(&session, 21, 9001U) ==
          CANVIEW_BRIDGE_WEB_SESSION_CLOSING);

    /* The service-stop path zeroizes and reinitializes the seam before a new owner can enter. */
    canview_bridge_web_session_init(&session);
    CHECK(!canview_bridge_web_session_is_closing(&session) && session.active_client_fd == -1);
    CHECK(canview_bridge_web_session_record_activity(&session, 22, 10000U) ==
          CANVIEW_BRIDGE_WEB_SESSION_OK);
    return 0;
}

int main(void)
{
    CHECK(test_null_and_empty_state() == 0);
    CHECK(test_activity_and_timeout_boundaries() == 0);
    CHECK(test_close_and_fd_reuse() == 0);
    CHECK(test_close_trigger_failure_is_fail_closed() == 0);
    (void)puts("PASS: Diagnostic Bridge web session lifecycle");
    return 0;
}
