/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_bridge_auth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition)                                                                          \
    do                                                                                             \
    {                                                                                              \
        if (!(condition))                                                                          \
        {                                                                                          \
            (void)fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition);               \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)

typedef struct
{
    uint64_t now_ms;
    uint8_t next_random;
    bool clock_fail;
    bool random_zero;
    bool digest_fail;
    bool mac_fail;
} fake_t;

static canview_status_t fake_now(void *context, uint64_t *now_ms)
{
    fake_t *fake = context;
    CHECK(fake != NULL && now_ms != NULL);
    if (fake->clock_fail)
    {
        return CANVIEW_TIMEOUT;
    }
    *now_ms = fake->now_ms;
    return CANVIEW_OK;
}

static canview_status_t fake_random(void *context, uint8_t *output, size_t output_length)
{
    fake_t *fake = context;
    CHECK(fake != NULL && output != NULL && output_length > 0U);
    if (fake->random_zero)
    {
        memset(output, 0, output_length);
        return CANVIEW_OK;
    }
    for (size_t index = 0U; index < output_length; ++index)
    {
        output[index] = (uint8_t)(fake->next_random + index + 1U);
    }
    fake->next_random = (uint8_t)(fake->next_random + 1U);
    return CANVIEW_OK;
}

static canview_status_t fake_digest(void *context, const uint8_t *data, size_t data_length,
                                    uint8_t *output, size_t output_length)
{
    CHECK(data != NULL && data_length > 0U && output != NULL);
    CHECK(output_length == CANVIEW_BRIDGE_AUTH_PIN_DIGEST_BYTES);
    if (context != NULL && ((fake_t *)context)->digest_fail)
    {
        return CANVIEW_NOT_IMPLEMENTED;
    }
    uint8_t state = 0xA7U;
    for (size_t index = 0U; index < data_length; ++index)
    {
        state = (uint8_t)((state << 1U) | (state >> 7U));
        state = (uint8_t)(state ^ data[index]);
    }
    for (size_t index = 0U; index < output_length; ++index)
    {
        output[index] = (uint8_t)(state + (uint8_t)(index * 17U));
    }
    return CANVIEW_OK;
}

static void expected_digest(const char *pin, uint8_t output[CANVIEW_BRIDGE_AUTH_PIN_DIGEST_BYTES])
{
    (void)fake_digest(NULL, (const uint8_t *)pin, strlen(pin), output,
                      CANVIEW_BRIDGE_AUTH_PIN_DIGEST_BYTES);
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

static canview_status_t fake_mac(void *context, const uint8_t *key, size_t key_length,
                                 const uint8_t *data, size_t data_length, uint8_t *output,
                                 size_t output_length)
{
    CHECK(key != NULL && key_length == CANVIEW_BRIDGE_AUTH_PIN_DIGEST_BYTES);
    CHECK(data != NULL && data_length > 0U && output != NULL);
    CHECK(output_length == CANVIEW_BRIDGE_AUTH_PIN_DIGEST_BYTES);
    if (context != NULL && ((fake_t *)context)->mac_fail)
    {
        return CANVIEW_NOT_IMPLEMENTED;
    }
    uint8_t state = 0x5AU;
    for (size_t index = 0U; index < key_length; ++index)
    {
        state = (uint8_t)(state ^ key[index] ^ (uint8_t)index);
    }
    for (size_t index = 0U; index < data_length; ++index)
    {
        state = (uint8_t)((state << 1U) | (state >> 7U));
        state = (uint8_t)(state ^ data[index]);
    }
    for (size_t index = 0U; index < output_length; ++index)
    {
        output[index] = (uint8_t)(state + (uint8_t)(index * 29U));
    }
    return CANVIEW_OK;
}

static int test_lifecycle(void)
{
    fake_t fake = {1000U, 0U, false, false, false, false};
    uint8_t digest[CANVIEW_BRIDGE_AUTH_PIN_DIGEST_BYTES] = {0};
    expected_digest("123456", digest);
    const canview_bridge_auth_callbacks_t callbacks = {fake_now, fake_digest, fake_mac, fake_random,
                                                       &fake};
    canview_bridge_auth_t auth;
    uint8_t challenge[CANVIEW_BRIDGE_AUTH_CHALLENGE_BYTES] = {0};
    uint8_t nonce[CANVIEW_BRIDGE_AUTH_CLIENT_NONCE_BYTES] = {0xA5U};
    uint8_t token[CANVIEW_BRIDGE_AUTH_TOKEN_BYTES] = {0};
    CHECK(canview_bridge_auth_init(&auth, digest, &callbacks) == CANVIEW_OK);
    CHECK(canview_bridge_auth_issue_challenge(&auth, challenge) == CANVIEW_OK);
    CHECK(canview_bridge_auth_open_session(&auth, challenge, nonce, "123456", 6U, token) ==
          CANVIEW_AUTH_FAILED);
    CHECK(canview_bridge_auth_set_service_window(&auth, true) == CANVIEW_OK);
    CHECK(canview_bridge_auth_open_session(&auth, challenge, nonce, "123456", 6U, token) ==
          CANVIEW_OK);
    CHECK(!canview_bridge_auth_is_locked(&auth));
    CHECK(canview_bridge_auth_check_token(&auth, token) == CANVIEW_OK);
    challenge[0] ^= 1U;
    CHECK(canview_bridge_auth_open_session(&auth, challenge, nonce, "123456", 6U, token) ==
          CANVIEW_AUTH_FAILED);
    CHECK(canview_bridge_auth_logout(&auth) == CANVIEW_OK);
    CHECK(canview_bridge_auth_check_token(&auth, token) == CANVIEW_AUTH_FAILED);
    CHECK(canview_bridge_auth_set_service_window(&auth, false) == CANVIEW_OK);
    return 0;
}

static int test_bounds_and_replay(void)
{
    fake_t fake = {2000U, 20U, false, false, false, false};
    uint8_t digest[CANVIEW_BRIDGE_AUTH_PIN_DIGEST_BYTES] = {0};
    expected_digest("123456", digest);
    const canview_bridge_auth_callbacks_t callbacks = {fake_now, fake_digest, fake_mac, fake_random,
                                                       &fake};
    canview_bridge_auth_t auth = {0};
    uint8_t challenge[CANVIEW_BRIDGE_AUTH_CHALLENGE_BYTES] = {0};
    uint8_t nonce[CANVIEW_BRIDGE_AUTH_CLIENT_NONCE_BYTES] = {0x11U};
    uint8_t token[CANVIEW_BRIDGE_AUTH_TOKEN_BYTES] = {0};
    CHECK(canview_bridge_auth_init(&auth, digest, &callbacks) == CANVIEW_OK);
    CHECK(canview_bridge_auth_set_service_window(&auth, true) == CANVIEW_OK);
    CHECK(canview_bridge_auth_issue_challenge(&auth, challenge) == CANVIEW_OK);
    memset(token, 0xA5, sizeof(token));
    CHECK(canview_bridge_auth_open_session(&auth, challenge, nonce, "12345", 5U, token) ==
          CANVIEW_AUTH_FAILED);
    CHECK(all_zero(token, sizeof(token)));
    CHECK(canview_bridge_auth_open_session(&auth, challenge, nonce, "123456789", 9U, token) ==
          CANVIEW_AUTH_FAILED);
    CHECK(canview_bridge_auth_open_session(&auth, challenge, nonce, "12A456", 6U, token) ==
          CANVIEW_AUTH_FAILED);
    CHECK(canview_bridge_auth_open_session(&auth, challenge, nonce, "123456", 6U, token) ==
          CANVIEW_OK);
    CHECK(canview_bridge_auth_open_session(&auth, challenge, nonce, "123456", 6U, token) ==
          CANVIEW_AUTH_FAILED);
    fake.now_ms += CANVIEW_BRIDGE_AUTH_TOKEN_TTL_MS + 1U;
    CHECK(canview_bridge_auth_check_token(&auth, token) == CANVIEW_AUTH_FAILED);
    return 0;
}

static int test_lockout_and_expiry(void)
{
    fake_t fake = {3000U, 40U, false, false, false, false};
    uint8_t digest[CANVIEW_BRIDGE_AUTH_PIN_DIGEST_BYTES] = {0};
    expected_digest("123456", digest);
    const canview_bridge_auth_callbacks_t callbacks = {fake_now, fake_digest, fake_mac, fake_random,
                                                       &fake};
    canview_bridge_auth_t auth = {0};
    uint8_t challenge[CANVIEW_BRIDGE_AUTH_CHALLENGE_BYTES] = {0};
    uint8_t nonce[CANVIEW_BRIDGE_AUTH_CLIENT_NONCE_BYTES] = {0x22U};
    uint8_t token[CANVIEW_BRIDGE_AUTH_TOKEN_BYTES] = {0};
    CHECK(canview_bridge_auth_init(&auth, digest, &callbacks) == CANVIEW_OK);
    CHECK(canview_bridge_auth_set_service_window(&auth, true) == CANVIEW_OK);
    CHECK(canview_bridge_auth_issue_challenge(&auth, challenge) == CANVIEW_OK);
    for (unsigned count = 0U; count < CANVIEW_BRIDGE_AUTH_FAILURE_LIMIT; ++count)
    {
        CHECK(canview_bridge_auth_open_session(&auth, challenge, nonce, "000000", 6U, token) ==
              CANVIEW_AUTH_FAILED);
    }
    CHECK(canview_bridge_auth_is_locked(&auth));
    fake.now_ms = 2999U;
    CHECK(canview_bridge_auth_is_locked(&auth));
    CHECK(canview_bridge_auth_open_session(&auth, challenge, nonce, "123456", 6U, token) ==
          CANVIEW_AUTH_FAILED);
    fake.now_ms = 3000U + CANVIEW_BRIDGE_AUTH_LOCKOUT_MS + 1U;
    CHECK(!canview_bridge_auth_is_locked(&auth));
    CHECK(canview_bridge_auth_issue_challenge(&auth, challenge) == CANVIEW_OK);
    CHECK(canview_bridge_auth_open_session(&auth, challenge, nonce, "123456", 6U, token) ==
          CANVIEW_OK);
    fake.now_ms += CANVIEW_BRIDGE_AUTH_TOKEN_TTL_MS + 1U;
    CHECK(canview_bridge_auth_check_token(&auth, token) == CANVIEW_AUTH_FAILED);
    return 0;
}

static int test_reconcile_expiry(void)
{
    fake_t fake = {5000U, 100U, false, false, false, false};
    uint8_t digest[CANVIEW_BRIDGE_AUTH_PIN_DIGEST_BYTES] = {0};
    expected_digest("123456", digest);
    const canview_bridge_auth_callbacks_t callbacks = {fake_now, fake_digest, fake_mac, fake_random,
                                                       &fake};
    canview_bridge_auth_t auth = {0};
    uint8_t challenge[CANVIEW_BRIDGE_AUTH_CHALLENGE_BYTES] = {0};
    uint8_t nonce[CANVIEW_BRIDGE_AUTH_CLIENT_NONCE_BYTES] = {0x44U};
    uint8_t token[CANVIEW_BRIDGE_AUTH_TOKEN_BYTES] = {0};
    CHECK(canview_bridge_auth_init(&auth, digest, &callbacks) == CANVIEW_OK);
    CHECK(canview_bridge_auth_set_service_window(&auth, true) == CANVIEW_OK);
    CHECK(canview_bridge_auth_issue_challenge(&auth, challenge) == CANVIEW_OK);
    CHECK(canview_bridge_auth_reconcile(&auth, fake.now_ms) == CANVIEW_OK);
    CHECK(auth.challenge_valid);
    CHECK(canview_bridge_auth_open_session(&auth, challenge, nonce, "123456", 6U, token) ==
          CANVIEW_OK);
    CHECK(auth.token_valid);
    CHECK(canview_bridge_auth_reconcile(&auth, fake.now_ms + CANVIEW_BRIDGE_AUTH_TOKEN_TTL_MS + 1U) ==
          CANVIEW_OK);
    CHECK(!auth.token_valid);
    CHECK(canview_bridge_auth_reconcile(&auth, fake.now_ms - 1U) == CANVIEW_OK);
    CHECK(!auth.token_valid);
    return 0;
}

static int test_callback_failures_and_expiry(void)
{
    fake_t fake = {4000U, 80U, false, false, false, false};
    uint8_t digest[CANVIEW_BRIDGE_AUTH_PIN_DIGEST_BYTES] = {0};
    expected_digest("12345678", digest);
    const canview_bridge_auth_callbacks_t callbacks = {fake_now, fake_digest, fake_mac, fake_random,
                                                       &fake};
    canview_bridge_auth_t auth;
    uint8_t challenge[CANVIEW_BRIDGE_AUTH_CHALLENGE_BYTES] = {0};
    uint8_t nonce[CANVIEW_BRIDGE_AUTH_CLIENT_NONCE_BYTES] = {0x33U};
    uint8_t token[CANVIEW_BRIDGE_AUTH_TOKEN_BYTES] = {0};
    CHECK(canview_bridge_auth_init(NULL, digest, &callbacks) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_bridge_auth_init(&auth, digest, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_bridge_auth_init(&auth, digest, &callbacks) == CANVIEW_OK);
    CHECK(canview_bridge_auth_issue_challenge(NULL, challenge) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_bridge_auth_issue_challenge(&auth, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_bridge_auth_check_token(&auth, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_bridge_auth_logout(NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_bridge_auth_set_service_window(NULL, true) == CANVIEW_INVALID_ARGUMENT);
    CHECK(!canview_bridge_auth_is_locked(NULL));
    CHECK(canview_bridge_auth_set_service_window(&auth, true) == CANVIEW_OK);

    fake.random_zero = true;
    memset(challenge, 0xA5, sizeof(challenge));
    CHECK(canview_bridge_auth_issue_challenge(&auth, challenge) == CANVIEW_NOT_IMPLEMENTED);
    CHECK(all_zero(challenge, sizeof(challenge)));
    fake.random_zero = false;
    CHECK(canview_bridge_auth_issue_challenge(&auth, challenge) == CANVIEW_OK);
    uint8_t previous_challenge[CANVIEW_BRIDGE_AUTH_CHALLENGE_BYTES] = {0};
    memcpy(previous_challenge, challenge, sizeof(previous_challenge));
    fake.random_zero = true;
    memset(challenge, 0xA5, sizeof(challenge));
    CHECK(canview_bridge_auth_issue_challenge(&auth, challenge) == CANVIEW_NOT_IMPLEMENTED);
    CHECK(all_zero(challenge, sizeof(challenge)));
    fake.random_zero = false;
    CHECK(canview_bridge_auth_open_session(&auth, previous_challenge, nonce, "12345678", 8U,
                                           token) == CANVIEW_AUTH_FAILED);
    memset(previous_challenge, 0, sizeof(previous_challenge));
    fake.now_ms += CANVIEW_BRIDGE_AUTH_CHALLENGE_TTL_MS + 1U;
    CHECK(canview_bridge_auth_open_session(&auth, challenge, nonce, "12345678", 8U, token) ==
          CANVIEW_AUTH_FAILED);

    CHECK(canview_bridge_auth_issue_challenge(&auth, challenge) == CANVIEW_OK);
    fake.clock_fail = true;
    CHECK(canview_bridge_auth_open_session(&auth, challenge, nonce, "12345678", 8U, token) ==
          CANVIEW_TIMEOUT);
    CHECK(canview_bridge_auth_check_token(&auth, token) == CANVIEW_TIMEOUT);
    CHECK(canview_bridge_auth_is_locked(&auth));
    fake.clock_fail = false;

    fake.digest_fail = true;
    CHECK(canview_bridge_auth_open_session(&auth, challenge, nonce, "12345678", 8U, token) ==
          CANVIEW_NOT_IMPLEMENTED);
    fake.digest_fail = false;
    fake.mac_fail = true;
    CHECK(canview_bridge_auth_open_session(&auth, challenge, nonce, "12345678", 8U, token) ==
          CANVIEW_NOT_IMPLEMENTED);
    fake.mac_fail = false;
    CHECK(canview_bridge_auth_open_session(&auth, challenge, nonce, "12345678", 8U, token) ==
          CANVIEW_OK);
    return 0;
}

int main(void)
{
    CHECK(test_lifecycle() == 0);
    CHECK(test_bounds_and_replay() == 0);
    CHECK(test_lockout_and_expiry() == 0);
    CHECK(test_reconcile_expiry() == 0);
    CHECK(test_callback_failures_and_expiry() == 0);
    (void)puts("PASS: bridge auth lifecycle, bounds, replay, lockout, expiry");
    return 0;
}
