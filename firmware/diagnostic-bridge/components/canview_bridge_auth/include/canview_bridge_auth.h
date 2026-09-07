/* SPDX-License-Identifier: GPL-3.0-only */
/** @file canview_bridge_auth.h
 *  @brief Diagnostic Bridge의 SDK 독립적인 service-session 상태기계.
 */
#ifndef CANVIEW_BRIDGE_AUTH_H
#define CANVIEW_BRIDGE_AUTH_H

#include "canview_status.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CANVIEW_BRIDGE_AUTH_CHALLENGE_BYTES (16U)
#define CANVIEW_BRIDGE_AUTH_CLIENT_NONCE_BYTES (16U)
#define CANVIEW_BRIDGE_AUTH_TOKEN_BYTES (16U)
#define CANVIEW_BRIDGE_AUTH_PIN_DIGEST_BYTES (32U)
#define CANVIEW_BRIDGE_AUTH_CHALLENGE_TTL_MS (60000U)
#define CANVIEW_BRIDGE_AUTH_TOKEN_TTL_MS (600000U)
#define CANVIEW_BRIDGE_AUTH_FAILURE_LIMIT (5U)
#define CANVIEW_BRIDGE_AUTH_FAILURE_WINDOW_MS (60000U)
#define CANVIEW_BRIDGE_AUTH_LOCKOUT_MS (60000U)

typedef canview_status_t canview_bridge_auth_now_fn(void *context, uint64_t *now_ms);
typedef canview_status_t canview_bridge_auth_digest_fn(void *context, const uint8_t *data,
                                                       size_t data_length, uint8_t *output,
                                                       size_t output_length);
typedef canview_status_t canview_bridge_auth_mac_fn(void *context, const uint8_t *key,
                                                    size_t key_length, const uint8_t *data,
                                                    size_t data_length, uint8_t *output,
                                                    size_t output_length);
typedef canview_status_t canview_bridge_auth_random_fn(void *context, uint8_t *output,
                                                       size_t output_length);

typedef struct
{
    canview_bridge_auth_now_fn *now_ms;
    canview_bridge_auth_digest_fn *digest;
    canview_bridge_auth_mac_fn *mac;
    canview_bridge_auth_random_fn *random;
    void *context;
} canview_bridge_auth_callbacks_t;

/**
 * @brief Auth state owned by the web service task. Do not serialize or expose it.
 *
 * The PIN digest, challenge, and token are RAM-only state. Initialization clears the supplied
 * storage and the caller must keep it alive until the web service stops. All public functions
 * are non-reentrant; the caller owns serialization.
 */
typedef struct
{
    uint8_t pin_digest[CANVIEW_BRIDGE_AUTH_PIN_DIGEST_BYTES];
    uint8_t challenge[CANVIEW_BRIDGE_AUTH_CHALLENGE_BYTES];
    uint8_t token[CANVIEW_BRIDGE_AUTH_TOKEN_BYTES];
    canview_bridge_auth_callbacks_t callbacks;
    uint64_t challenge_issued_ms;
    uint64_t token_issued_ms;
    uint64_t failure_window_started_ms;
    uint64_t lockout_started_ms;
    uint8_t failure_count;
    bool challenge_valid;
    bool token_valid;
    bool service_window_open;
    bool locked;
    bool initialized;
} canview_bridge_auth_t;

/** @brief provisioned digest와 SDK adapter를 등록한다. PIN은 받지 않는다. */
canview_status_t canview_bridge_auth_init(canview_bridge_auth_t *auth,
                                          const uint8_t *pin_digest,
                                          const canview_bridge_auth_callbacks_t *callbacks);

/** @brief 새 challenge를 발급하고 이전 미사용 challenge를 폐기한다. */
canview_status_t canview_bridge_auth_issue_challenge(
    canview_bridge_auth_t *auth, uint8_t challenge[CANVIEW_BRIDGE_AUTH_CHALLENGE_BYTES]);

/** @brief 물리 service window 상태를 갱신한다. 닫으면 활성 token을 폐기한다. */
canview_status_t canview_bridge_auth_set_service_window(canview_bridge_auth_t *auth, bool open);

/**
 * @brief challenge·nonce·PIN으로 session token을 만든다.
 *
 * PIN은 6–8자리 decimal 문자열이어야 하며, 성공한 challenge는 즉시 one-time 폐기된다.
 * 실패 상세를 구분하지 않아 원격 PIN 추측에 내부 상태를 노출하지 않는다.
 */
canview_status_t canview_bridge_auth_open_session(
    canview_bridge_auth_t *auth,
    const uint8_t challenge[CANVIEW_BRIDGE_AUTH_CHALLENGE_BYTES],
    const uint8_t client_nonce[CANVIEW_BRIDGE_AUTH_CLIENT_NONCE_BYTES], const char *pin,
    size_t pin_length, uint8_t token[CANVIEW_BRIDGE_AUTH_TOKEN_BYTES]);

/** @brief memory-only token의 유효성·만료·service window를 검사한다. */
canview_status_t canview_bridge_auth_check_token(
    canview_bridge_auth_t *auth, const uint8_t token[CANVIEW_BRIDGE_AUTH_TOKEN_BYTES]);

/** @brief token과 미사용 challenge를 폐기한다. */
canview_status_t canview_bridge_auth_logout(canview_bridge_auth_t *auth);

/** @brief 현재 lockout 상태를 반환한다. 잘못된 context는 false로 처리한다. */
bool canview_bridge_auth_is_locked(canview_bridge_auth_t *auth);

#endif
