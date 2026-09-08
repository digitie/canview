/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_bridge_auth.h"
#include <string.h>

static void secure_zero(void *data, size_t length)
{
    volatile uint8_t *bytes = data;
    while (length > 0U)
    {
        *bytes = 0U;
        ++bytes;
        --length;
    }
}

static bool context_ready(const canview_bridge_auth_t *auth)
{
    return auth != NULL && auth->initialized && auth->callbacks.now_ms != NULL &&
           auth->callbacks.digest != NULL && auth->callbacks.mac != NULL &&
           auth->callbacks.random != NULL;
}

static bool elapsed_within(uint64_t now_ms, uint64_t started_ms, uint64_t window_ms)
{
    return now_ms >= started_ms && now_ms - started_ms <= window_ms;
}

static bool all_zero(const uint8_t *data, size_t length)
{
    uint8_t combined = 0U;
    for (size_t index = 0U; index < length; ++index)
    {
        combined |= data[index];
    }
    return combined == 0U;
}

static bool constant_time_equal(const uint8_t *left, const uint8_t *right, size_t length)
{
    uint8_t difference = 0U;
    for (size_t index = 0U; index < length; ++index)
    {
        difference |= (uint8_t)(left[index] ^ right[index]);
    }
    return difference == 0U;
}

static canview_status_t now(const canview_bridge_auth_t *auth, uint64_t *now_ms)
{
    if (auth == NULL || now_ms == NULL || auth->callbacks.now_ms == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    return auth->callbacks.now_ms(auth->callbacks.context, now_ms);
}

static void clear_token(canview_bridge_auth_t *auth)
{
    secure_zero(auth->token, sizeof(auth->token));
    auth->token_issued_ms = 0U;
    auth->token_valid = false;
}

static void clear_challenge(canview_bridge_auth_t *auth)
{
    secure_zero(auth->challenge, sizeof(auth->challenge));
    auth->challenge_issued_ms = 0U;
    auth->challenge_valid = false;
}

static void refresh_lockout(canview_bridge_auth_t *auth, uint64_t now_ms)
{
    if (auth->locked &&
        (now_ms < auth->lockout_started_ms ||
         elapsed_within(now_ms, auth->lockout_started_ms, CANVIEW_BRIDGE_AUTH_LOCKOUT_MS)))
    {
        return;
    }
    if (auth->locked)
    {
        auth->locked = false;
        auth->failure_count = 0U;
        auth->failure_window_started_ms = now_ms;
    }
}

static void record_failure(canview_bridge_auth_t *auth, uint64_t now_ms)
{
    refresh_lockout(auth, now_ms);
    if (auth->failure_count == 0U ||
        !elapsed_within(now_ms, auth->failure_window_started_ms,
                        CANVIEW_BRIDGE_AUTH_FAILURE_WINDOW_MS))
    {
        auth->failure_count = 0U;
        auth->failure_window_started_ms = now_ms;
    }
    if (auth->failure_count < UINT8_MAX)
    {
        ++auth->failure_count;
    }
    if (auth->failure_count >= CANVIEW_BRIDGE_AUTH_FAILURE_LIMIT)
    {
        auth->locked = true;
        auth->lockout_started_ms = now_ms;
    }
}

static bool pin_is_valid(const char *pin, size_t pin_length)
{
    if (pin == NULL || pin_length < 6U || pin_length > 8U)
    {
        return false;
    }
    for (size_t index = 0U; index < pin_length; ++index)
    {
        if (pin[index] < '0' || pin[index] > '9')
        {
            return false;
        }
    }
    return true;
}

static void append_u64_le(uint8_t output[8], uint64_t value)
{
    for (size_t index = 0U; index < 8U; ++index)
    {
        output[index] = (uint8_t)(value >> (index * 8U));
    }
}

static canview_status_t auth_failure(canview_bridge_auth_t *auth, uint64_t now_ms)
{
    record_failure(auth, now_ms);
    return CANVIEW_AUTH_FAILED;
}

canview_status_t canview_bridge_auth_reconcile(canview_bridge_auth_t *auth, uint64_t now_ms)
{
    if (!context_ready(auth))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    refresh_lockout(auth, now_ms);
    if (auth->challenge_valid &&
        !elapsed_within(now_ms, auth->challenge_issued_ms,
                        CANVIEW_BRIDGE_AUTH_CHALLENGE_TTL_MS))
    {
        clear_challenge(auth);
    }
    if (auth->token_valid &&
        (!auth->service_window_open ||
         !elapsed_within(now_ms, auth->token_issued_ms, CANVIEW_BRIDGE_AUTH_TOKEN_TTL_MS)))
    {
        clear_token(auth);
    }
    return CANVIEW_OK;
}

canview_status_t canview_bridge_auth_init(canview_bridge_auth_t *auth,
                                          const uint8_t *pin_digest,
                                          const canview_bridge_auth_callbacks_t *callbacks)
{
    if (auth == NULL || pin_digest == NULL || callbacks == NULL || callbacks->now_ms == NULL ||
        callbacks->digest == NULL || callbacks->mac == NULL || callbacks->random == NULL ||
        all_zero(pin_digest, CANVIEW_BRIDGE_AUTH_PIN_DIGEST_BYTES))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    /* The input is const caller-owned storage; copy it before the caller zeroizes it. */
    memset(auth, 0, sizeof(*auth));
    memcpy(auth->pin_digest, pin_digest, sizeof(auth->pin_digest));
    auth->callbacks = *callbacks;
    auth->initialized = true;
    return CANVIEW_OK;
}

canview_status_t canview_bridge_auth_issue_challenge(
    canview_bridge_auth_t *auth, uint8_t challenge[CANVIEW_BRIDGE_AUTH_CHALLENGE_BYTES])
{
    if (challenge == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    secure_zero(challenge, CANVIEW_BRIDGE_AUTH_CHALLENGE_BYTES);
    if (!context_ready(auth))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    clear_challenge(auth);
    uint64_t now_ms = 0U;
    if (now(auth, &now_ms) != CANVIEW_OK)
    {
        return CANVIEW_TIMEOUT;
    }
    uint8_t generated[CANVIEW_BRIDGE_AUTH_CHALLENGE_BYTES] = {0};
    const canview_status_t random_status =
        auth->callbacks.random(auth->callbacks.context, generated, sizeof(generated));
    if (random_status != CANVIEW_OK || all_zero(generated, sizeof(generated)))
    {
        secure_zero(generated, sizeof(generated));
        return random_status == CANVIEW_OK ? CANVIEW_NOT_IMPLEMENTED : random_status;
    }
    memcpy(auth->challenge, generated, sizeof(auth->challenge));
    memcpy(challenge, generated, sizeof(auth->challenge));
    secure_zero(generated, sizeof(generated));
    auth->challenge_issued_ms = now_ms;
    auth->challenge_valid = true;
    return CANVIEW_OK;
}

canview_status_t canview_bridge_auth_set_service_window(canview_bridge_auth_t *auth, bool open)
{
    if (!context_ready(auth))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    auth->service_window_open = open;
    if (!open)
    {
        clear_token(auth);
        clear_challenge(auth);
    }
    return CANVIEW_OK;
}

canview_status_t canview_bridge_auth_open_session(
    canview_bridge_auth_t *auth,
    const uint8_t challenge[CANVIEW_BRIDGE_AUTH_CHALLENGE_BYTES],
    const uint8_t client_nonce[CANVIEW_BRIDGE_AUTH_CLIENT_NONCE_BYTES], const char *pin,
    size_t pin_length, uint8_t token[CANVIEW_BRIDGE_AUTH_TOKEN_BYTES])
{
    if (token == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    secure_zero(token, CANVIEW_BRIDGE_AUTH_TOKEN_BYTES);
    if (!context_ready(auth) || challenge == NULL || client_nonce == NULL || pin == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    uint64_t now_ms = 0U;
    if (now(auth, &now_ms) != CANVIEW_OK)
    {
        return CANVIEW_TIMEOUT;
    }
    if (canview_bridge_auth_reconcile(auth, now_ms) != CANVIEW_OK)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (auth->locked || !auth->service_window_open)
    {
        return CANVIEW_AUTH_FAILED;
    }
    if (!auth->challenge_valid ||
        !elapsed_within(now_ms, auth->challenge_issued_ms,
                        CANVIEW_BRIDGE_AUTH_CHALLENGE_TTL_MS) ||
        !constant_time_equal(challenge, auth->challenge, sizeof(auth->challenge)) ||
        all_zero(client_nonce, CANVIEW_BRIDGE_AUTH_CLIENT_NONCE_BYTES) ||
        !pin_is_valid(pin, pin_length))
    {
        return auth_failure(auth, now_ms);
    }

    uint8_t supplied_digest[CANVIEW_BRIDGE_AUTH_PIN_DIGEST_BYTES] = {0};
    const canview_status_t digest_status = auth->callbacks.digest(
        auth->callbacks.context, (const uint8_t *)pin, pin_length, supplied_digest,
        sizeof(supplied_digest));
    if (digest_status != CANVIEW_OK ||
        !constant_time_equal(supplied_digest, auth->pin_digest, sizeof(supplied_digest)))
    {
        secure_zero(supplied_digest, sizeof(supplied_digest));
        return digest_status == CANVIEW_OK ? auth_failure(auth, now_ms) : CANVIEW_NOT_IMPLEMENTED;
    }
    secure_zero(supplied_digest, sizeof(supplied_digest));

    uint8_t message[CANVIEW_BRIDGE_AUTH_CHALLENGE_BYTES +
                    CANVIEW_BRIDGE_AUTH_CLIENT_NONCE_BYTES + sizeof(uint64_t)] = {0};
    memcpy(message, challenge, CANVIEW_BRIDGE_AUTH_CHALLENGE_BYTES);
    memcpy(message + CANVIEW_BRIDGE_AUTH_CHALLENGE_BYTES, client_nonce,
           CANVIEW_BRIDGE_AUTH_CLIENT_NONCE_BYTES);
    append_u64_le(message + CANVIEW_BRIDGE_AUTH_CHALLENGE_BYTES +
                      CANVIEW_BRIDGE_AUTH_CLIENT_NONCE_BYTES,
                  auth->challenge_issued_ms);

    uint8_t digest[CANVIEW_BRIDGE_AUTH_PIN_DIGEST_BYTES] = {0};
    const canview_status_t mac_status = auth->callbacks.mac(
        auth->callbacks.context, auth->pin_digest, sizeof(auth->pin_digest), message,
        sizeof(message), digest, sizeof(digest));
    secure_zero(message, sizeof(message));
    if (mac_status != CANVIEW_OK)
    {
        secure_zero(digest, sizeof(digest));
        return CANVIEW_NOT_IMPLEMENTED;
    }
    memcpy(auth->token, digest, sizeof(auth->token));
    memcpy(token, digest, sizeof(auth->token));
    secure_zero(digest, sizeof(digest));
    clear_challenge(auth);
    auth->token_issued_ms = now_ms;
    auth->token_valid = true;
    auth->failure_count = 0U;
    auth->failure_window_started_ms = now_ms;
    return CANVIEW_OK;
}

canview_status_t canview_bridge_auth_check_token(
    canview_bridge_auth_t *auth, const uint8_t token[CANVIEW_BRIDGE_AUTH_TOKEN_BYTES])
{
    if (!context_ready(auth) || token == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    uint64_t now_ms = 0U;
    if (now(auth, &now_ms) != CANVIEW_OK)
    {
        return CANVIEW_TIMEOUT;
    }
    if (canview_bridge_auth_reconcile(auth, now_ms) != CANVIEW_OK)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (!auth->service_window_open || !auth->token_valid ||
        !constant_time_equal(token, auth->token, sizeof(auth->token)))
    {
        return CANVIEW_AUTH_FAILED;
    }
    return CANVIEW_OK;
}

canview_status_t canview_bridge_auth_logout(canview_bridge_auth_t *auth)
{
    if (!context_ready(auth))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    clear_token(auth);
    clear_challenge(auth);
    return CANVIEW_OK;
}

bool canview_bridge_auth_is_locked(canview_bridge_auth_t *auth)
{
    if (!context_ready(auth))
    {
        return false;
    }
    uint64_t now_ms = 0U;
    if (now(auth, &now_ms) != CANVIEW_OK)
    {
        return true;
    }
    refresh_lockout(auth, now_ms);
    return auth->locked;
}
