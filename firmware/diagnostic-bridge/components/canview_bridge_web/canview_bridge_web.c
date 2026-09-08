/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_bridge_web.h"
#include "bridge_assets.h"
#include "canview_bridge_auth.h"
#include "dns_server.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "cJSON.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "mbedtls/md.h"
#include "psa/crypto.h"

#define CANVIEW_BRIDGE_WEB_TAG "bridge_web"
#define CANVIEW_BRIDGE_WEB_JSON_ARENA_BYTES (16384U)
#define CANVIEW_BRIDGE_WEB_MAX_JSON_DEPTH (8U)
#define CANVIEW_BRIDGE_WEB_MIN_AP_PASSWORD_LENGTH (16U)
#define CANVIEW_BRIDGE_WEB_MAX_AP_PASSWORD_LENGTH (63U)
#define CANVIEW_BRIDGE_WEB_MUTATION_WINDOW_MS (1000U)
#define CANVIEW_BRIDGE_WEB_MUTATION_LIMIT (5U)
#define CANVIEW_BRIDGE_WEB_WS_PROTOCOL "canview-session"
#define CANVIEW_BRIDGE_WEB_WS_TOKEN_PREFIX "canview-session."
#define CANVIEW_BRIDGE_WEB_WS_TOKEN_TEXT_BYTES (22U)
#define CANVIEW_BRIDGE_WEB_ORIGIN_IP "http://192.168.4.1"
#define CANVIEW_BRIDGE_WEB_ORIGIN_HOST "http://canview-diag.local"

typedef union
{
    long double alignment;
    uint8_t bytes[CANVIEW_BRIDGE_WEB_JSON_ARENA_BYTES];
} canview_bridge_json_arena_t;

typedef struct
{
    canview_bridge_auth_t auth;
    canview_bridge_web_config_t config;
    canview_bridge_json_arena_t json_arena;
    SemaphoreHandle_t lock;
    SemaphoreHandle_t request_lock;
    SemaphoreHandle_t ws_io_lock;
    httpd_handle_t server;
    esp_netif_t *wifi_ap_netif;
    esp_netif_t *wifi_sta_netif;
    int active_client_fd;
    bool session_close_pending;
    uint32_t snapshot_revision;
    uint32_t event_sequence;
    size_t json_arena_used;
    uint64_t button_started_ms;
    uint64_t service_window_started_ms;
    uint64_t mutation_window_started_ms;
    uint8_t mutation_count;
    char ap_password[CANVIEW_BRIDGE_WEB_AP_PASSWORD_BYTES];
    char request_body[CANVIEW_BRIDGE_WEB_MAX_JSON_BYTES + 1U];
    char response[CANVIEW_BRIDGE_WEB_MAX_RESPONSE_BYTES];
    char ws_response[CANVIEW_BRIDGE_WEB_MAX_RESPONSE_BYTES];
    uint8_t ws_body[CANVIEW_BRIDGE_WEB_MAX_WS_FRAME_BYTES];
    bool button_down;
    bool button_hold_consumed;
    bool service_window_open;
    bool event_loop_initialized;
    bool wifi_initialized;
    bool wifi_started;
    bool dns_started;
    bool initialized;
} canview_bridge_web_state_t;

static canview_bridge_web_state_t web_state;

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

static size_t bounded_length(const char *text, size_t capacity)
{
    if (text == NULL)
    {
        return 0U;
    }
    size_t length = 0U;
    while (length < capacity && text[length] != '\0')
    {
        ++length;
    }
    return length;
}

static canview_status_t idf_now_ms(void *context, uint64_t *now_ms)
{
    (void)context;
    if (now_ms == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const int64_t now_us = esp_timer_get_time();
    if (now_us < 0)
    {
        return CANVIEW_TIMEOUT;
    }
    *now_ms = (uint64_t)now_us / UINT64_C(1000);
    return CANVIEW_OK;
}

static canview_status_t idf_digest(void *context, const uint8_t *data, size_t data_length,
                                   uint8_t *output, size_t output_length)
{
    (void)context;
    if (data == NULL || data_length == 0U || output == NULL ||
        output_length != CANVIEW_BRIDGE_AUTH_PIN_DIGEST_BYTES)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (info == NULL || mbedtls_md(info, data, data_length, output) != 0)
    {
        return CANVIEW_NOT_IMPLEMENTED;
    }
    return CANVIEW_OK;
}

static canview_status_t idf_mac(void *context, const uint8_t *key, size_t key_length,
                                const uint8_t *data, size_t data_length, uint8_t *output,
                                size_t output_length)
{
    (void)context;
    if (key == NULL || key_length == 0U || data == NULL || data_length == 0U || output == NULL ||
        output_length != CANVIEW_BRIDGE_AUTH_PIN_DIGEST_BYTES)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    static bool psa_ready;
    if (!psa_ready && psa_crypto_init() != PSA_SUCCESS)
    {
        return CANVIEW_NOT_IMPLEMENTED;
    }
    psa_ready = true;
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_type(&attributes, PSA_KEY_TYPE_HMAC);
    if (key_length > SIZE_MAX / 8U)
    {
        return CANVIEW_OVERSIZE;
    }
    psa_set_key_bits(&attributes, key_length * 8U);
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_MESSAGE);
    psa_set_key_algorithm(&attributes, PSA_ALG_HMAC(PSA_ALG_SHA_256));
    psa_key_id_t key_id = PSA_KEY_ID_NULL;
    psa_status_t status = psa_import_key(&attributes, key, key_length, &key_id);
    psa_reset_key_attributes(&attributes);
    size_t actual_length = 0U;
    if (status == PSA_SUCCESS)
    {
        status = psa_mac_compute(key_id, PSA_ALG_HMAC(PSA_ALG_SHA_256), data, data_length, output,
                                 output_length, &actual_length);
        const psa_status_t destroy_status = psa_destroy_key(key_id);
        if (status == PSA_SUCCESS && destroy_status != PSA_SUCCESS)
        {
            status = destroy_status;
        }
    }
    if (status != PSA_SUCCESS || actual_length != output_length)
    {
        return CANVIEW_NOT_IMPLEMENTED;
    }
    return CANVIEW_OK;
}

static canview_status_t idf_random(void *context, uint8_t *output, size_t output_length)
{
    (void)context;
    if (output == NULL || output_length == 0U)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    esp_fill_random(output, output_length);
    return CANVIEW_OK;
}

static void json_reset(void)
{
    secure_zero(web_state.json_arena.bytes, sizeof(web_state.json_arena.bytes));
    web_state.json_arena_used = 0U;
}

static void *json_malloc(size_t size)
{
    if (size == 0U)
    {
        return NULL;
    }
    if (web_state.json_arena_used > sizeof(web_state.json_arena.bytes))
    {
        return NULL;
    }
    const size_t alignment = sizeof(uintptr_t);
    const size_t remainder = web_state.json_arena_used % alignment;
    const size_t padding = remainder == 0U ? 0U : alignment - remainder;
    if (padding > sizeof(web_state.json_arena.bytes) - web_state.json_arena_used ||
        size > sizeof(web_state.json_arena.bytes) - web_state.json_arena_used - padding)
    {
        return NULL;
    }
    web_state.json_arena_used += padding;
    void *result = &web_state.json_arena.bytes[web_state.json_arena_used];
    web_state.json_arena_used += size;
    return result;
}

static void json_free(void *data)
{
    (void)data;
}

static bool json_add_string(cJSON *object, const char *name, const char *value)
{
    return cJSON_AddStringToObject(object, name, value) != NULL;
}

static bool json_add_number(cJSON *object, const char *name, double value)
{
    return cJSON_AddNumberToObject(object, name, value) != NULL;
}

static bool json_add_bool(cJSON *object, const char *name, bool value)
{
    return cJSON_AddBoolToObject(object, name, value) != NULL;
}

static bool json_add_item(cJSON *object, const char *name, cJSON *item)
{
    return item != NULL && cJSON_AddItemToObject(object, name, item) != 0;
}

static esp_err_t send_json_locked(httpd_req_t *request, cJSON *root)
{
    if (request == NULL || root == NULL)
    {
        if (root != NULL)
        {
            cJSON_Delete(root);
        }
        secure_zero(web_state.response, sizeof(web_state.response));
        json_reset();
        return ESP_ERR_INVALID_ARG;
    }
    const bool printed = cJSON_PrintPreallocated(root, web_state.response,
                                                  sizeof(web_state.response), false) != 0;
    cJSON_Delete(root);
    json_reset();
    if (!printed)
    {
        secure_zero(web_state.response, sizeof(web_state.response));
        return httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "response too large");
    }
    if (httpd_resp_set_type(request, HTTPD_TYPE_JSON) != ESP_OK ||
        httpd_resp_set_hdr(request, "Cache-Control", "no-store") != ESP_OK ||
        httpd_resp_set_hdr(request, "X-Content-Type-Options", "nosniff") != ESP_OK)
    {
        secure_zero(web_state.response, sizeof(web_state.response));
        return ESP_FAIL;
    }
    const size_t response_length = bounded_length(web_state.response, sizeof(web_state.response));
    if (response_length >= sizeof(web_state.response))
    {
        secure_zero(web_state.response, sizeof(web_state.response));
        return httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "response invalid");
    }
    const esp_err_t result = httpd_resp_send(request, web_state.response, (ssize_t)response_length);
    secure_zero(web_state.response, sizeof(web_state.response));
    return result;
}

static esp_err_t send_custom_status(httpd_req_t *request, const char *status, const char *message)
{
    if (request == NULL || status == NULL || message == NULL ||
        httpd_resp_set_status(request, status) != ESP_OK ||
        httpd_resp_set_type(request, "text/plain; charset=utf-8") != ESP_OK ||
        httpd_resp_set_hdr(request, "Cache-Control", "no-store") != ESP_OK)
    {
        return ESP_FAIL;
    }
    return httpd_resp_sendstr(request, message);
}

static bool state_lock_take(canview_bridge_web_state_t *state)
{
    return state != NULL && state->lock != NULL &&
           xSemaphoreTake(state->lock, portMAX_DELAY) == pdTRUE;
}

static void state_lock_give(canview_bridge_web_state_t *state)
{
    if (state != NULL && state->lock != NULL)
    {
        (void)xSemaphoreGive(state->lock);
    }
}

static esp_err_t enter_request(httpd_req_t *request, canview_bridge_web_state_t **state)
{
    if (request == NULL || state == NULL || request->user_ctx == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    canview_bridge_web_state_t *candidate = request->user_ctx;
    if (candidate->request_lock == NULL ||
        xSemaphoreTake(candidate->request_lock, portMAX_DELAY) != pdTRUE)
    {
        return send_custom_status(request, "503 Service Unavailable", "service unavailable");
    }
    if (!state_lock_take(candidate))
    {
        (void)xSemaphoreGive(candidate->request_lock);
        return send_custom_status(request, "503 Service Unavailable", "service unavailable");
    }
    if (!candidate->initialized)
    {
        state_lock_give(candidate);
        (void)xSemaphoreGive(candidate->request_lock);
        return send_custom_status(request, "503 Service Unavailable", "service unavailable");
    }
    const int client_fd = httpd_req_to_sockfd(request);
    if (client_fd < 0)
    {
        state_lock_give(candidate);
        (void)xSemaphoreGive(candidate->request_lock);
        return send_custom_status(request, "400 Bad Request", "invalid client");
    }
    if (candidate->session_close_pending)
    {
        state_lock_give(candidate);
        (void)xSemaphoreGive(candidate->request_lock);
        return send_custom_status(request, "503 Service Unavailable", "session closing");
    }
    if (candidate->active_client_fd < 0)
    {
        candidate->active_client_fd = client_fd;
    }
    if (candidate->active_client_fd != client_fd)
    {
        state_lock_give(candidate);
        (void)xSemaphoreGive(candidate->request_lock);
        return send_custom_status(request, "503 Service Unavailable", "one client only");
    }
    state_lock_give(candidate);
    *state = candidate;
    return ESP_OK;
}

static void leave_request(canview_bridge_web_state_t *state)
{
    if (state != NULL && state->request_lock != NULL)
    {
        (void)xSemaphoreGive(state->request_lock);
    }
}

static bool enter_ws_io(canview_bridge_web_state_t *state)
{
    return state != NULL && state->ws_io_lock != NULL &&
           xSemaphoreTake(state->ws_io_lock, portMAX_DELAY) == pdTRUE;
}

static void leave_ws_io(canview_bridge_web_state_t *state)
{
    if (state != NULL && state->ws_io_lock != NULL)
    {
        (void)xSemaphoreGive(state->ws_io_lock);
    }
}

static esp_err_t discard_start_state(void)
{
    esp_err_t cleanup_status = ESP_OK;
    web_state.initialized = false;
    if (web_state.dns_started)
    {
        const esp_err_t status = canview_bridge_dns_stop();
        if (status == ESP_OK)
        {
            web_state.dns_started = false;
        }
        else
        {
            cleanup_status = status;
        }
    }
    if (web_state.server != NULL)
    {
        const esp_err_t status = httpd_stop(web_state.server);
        if (status != ESP_OK && cleanup_status == ESP_OK)
        {
            cleanup_status = status;
        }
        web_state.server = NULL;
    }
    if (web_state.wifi_started)
    {
        const esp_err_t status = esp_wifi_stop();
        if (status != ESP_OK && cleanup_status == ESP_OK)
        {
            cleanup_status = status;
        }
        web_state.wifi_started = false;
    }
    if (web_state.wifi_initialized)
    {
        const esp_err_t status = esp_wifi_deinit();
        if (status != ESP_OK && cleanup_status == ESP_OK)
        {
            cleanup_status = status;
        }
        web_state.wifi_initialized = false;
    }
    if (web_state.wifi_ap_netif != NULL)
    {
        esp_netif_destroy_default_wifi(web_state.wifi_ap_netif);
        web_state.wifi_ap_netif = NULL;
    }
    if (web_state.wifi_sta_netif != NULL)
    {
        esp_netif_destroy_default_wifi(web_state.wifi_sta_netif);
        web_state.wifi_sta_netif = NULL;
    }
    if (web_state.event_loop_initialized)
    {
        const esp_err_t status = esp_event_loop_delete_default();
        if (status != ESP_OK && cleanup_status == ESP_OK)
        {
            cleanup_status = status;
        }
        web_state.event_loop_initialized = false;
    }
    SemaphoreHandle_t ws_io_lock = web_state.ws_io_lock;
    SemaphoreHandle_t request_lock = web_state.request_lock;
    SemaphoreHandle_t lock = web_state.lock;
    web_state.ws_io_lock = NULL;
    web_state.request_lock = NULL;
    web_state.lock = NULL;
    if (ws_io_lock != NULL)
    {
        vSemaphoreDelete(ws_io_lock);
    }
    if (request_lock != NULL)
    {
        vSemaphoreDelete(request_lock);
    }
    if (lock != NULL)
    {
        vSemaphoreDelete(lock);
    }
    const bool dns_pending = web_state.dns_started;
    secure_zero(&web_state, sizeof(web_state));
    web_state.active_client_fd = -1;
    web_state.dns_started = dns_pending;
    return cleanup_status;
}

static bool origin_allowed(httpd_req_t *request, bool required)
{
    if (request == NULL)
    {
        return false;
    }
    const size_t length = httpd_req_get_hdr_value_len(request, "Origin");
    if (length == 0U)
    {
        return !required;
    }
    if (length >= CANVIEW_BRIDGE_WEB_MAX_ORIGIN_BYTES)
    {
        return false;
    }
    char origin[CANVIEW_BRIDGE_WEB_MAX_ORIGIN_BYTES] = {0};
    if (httpd_req_get_hdr_value_str(request, "Origin", origin, sizeof(origin)) != ESP_OK)
    {
        return false;
    }
    return strcmp(origin, CANVIEW_BRIDGE_WEB_ORIGIN_IP) == 0 ||
           strcmp(origin, CANVIEW_BRIDGE_WEB_ORIGIN_HOST) == 0;
}

static int base64_value(char character)
{
    if (character >= 'A' && character <= 'Z')
    {
        return character - 'A';
    }
    if (character >= 'a' && character <= 'z')
    {
        return character - 'a' + 26;
    }
    if (character >= '0' && character <= '9')
    {
        return character - '0' + 52;
    }
    if (character == '-')
    {
        return 62;
    }
    if (character == '_')
    {
        return 63;
    }
    return -1;
}

static bool base64url_encode(const uint8_t *input, size_t input_length, char *output,
                            size_t output_capacity)
{
    if (input == NULL || output == NULL)
    {
        return false;
    }
    const size_t full_groups = input_length / 3U;
    const size_t remainder = input_length % 3U;
    const size_t encoded_length = full_groups * 4U + (remainder == 0U ? 0U : remainder + 1U);
    if (encoded_length >= output_capacity)
    {
        return false;
    }
    static const char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    size_t source = 0U;
    size_t target = 0U;
    while (source + 3U <= input_length)
    {
        const uint32_t value = ((uint32_t)input[source] << 16U) |
                               ((uint32_t)input[source + 1U] << 8U) | input[source + 2U];
        output[target++] = alphabet[(value >> 18U) & 0x3FU];
        output[target++] = alphabet[(value >> 12U) & 0x3FU];
        output[target++] = alphabet[(value >> 6U) & 0x3FU];
        output[target++] = alphabet[value & 0x3FU];
        source += 3U;
    }
    if (remainder == 1U)
    {
        const uint32_t value = (uint32_t)input[source] << 16U;
        output[target++] = alphabet[(value >> 18U) & 0x3FU];
        output[target++] = alphabet[(value >> 12U) & 0x3FU];
    }
    else if (remainder == 2U)
    {
        const uint32_t value = ((uint32_t)input[source] << 16U) |
                               ((uint32_t)input[source + 1U] << 8U);
        output[target++] = alphabet[(value >> 18U) & 0x3FU];
        output[target++] = alphabet[(value >> 12U) & 0x3FU];
        output[target++] = alphabet[(value >> 6U) & 0x3FU];
    }
    output[target] = '\0';
    return target == encoded_length;
}

static bool base64url_decode(const char *input, size_t input_length, uint8_t *output,
                            size_t output_capacity, size_t expected_length)
{
    if (input == NULL || output == NULL || input_length == 0U || expected_length > output_capacity ||
        input_length != CANVIEW_BRIDGE_WEB_WS_TOKEN_TEXT_BYTES || expected_length != 16U)
    {
        return false;
    }
    size_t source = 0U;
    size_t target = 0U;
    while (source + 4U <= input_length)
    {
        const int a = base64_value(input[source]);
        const int b = base64_value(input[source + 1U]);
        const int c = base64_value(input[source + 2U]);
        const int d = base64_value(input[source + 3U]);
        if (a < 0 || b < 0 || c < 0 || d < 0 || target + 3U > output_capacity)
        {
            return false;
        }
        output[target++] = (uint8_t)((a << 2) | (b >> 4));
        output[target++] = (uint8_t)((b << 4) | (c >> 2));
        output[target++] = (uint8_t)((c << 6) | d);
        source += 4U;
    }
    const size_t remaining = input_length - source;
    if (remaining == 2U)
    {
        const int a = base64_value(input[source]);
        const int b = base64_value(input[source + 1U]);
        if (a < 0 || b < 0 || target >= output_capacity || (b & 0x0FU) != 0)
        {
            return false;
        }
        output[target++] = (uint8_t)((a << 2) | (b >> 4));
    }
    else if (remaining == 3U)
    {
        const int a = base64_value(input[source]);
        const int b = base64_value(input[source + 1U]);
        const int c = base64_value(input[source + 2U]);
        if (a < 0 || b < 0 || c < 0 || target + 2U > output_capacity || (c & 0x03) != 0)
        {
            return false;
        }
        output[target++] = (uint8_t)((a << 2) | (b >> 4));
        output[target++] = (uint8_t)((b << 4) | (c >> 2));
    }
    else if (remaining != 0U)
    {
        return false;
    }
    return source + remaining == input_length && target == expected_length;
}

static bool parse_bearer(httpd_req_t *request,
                         uint8_t token[CANVIEW_BRIDGE_AUTH_TOKEN_BYTES])
{
    if (request == NULL || token == NULL)
    {
        return false;
    }
    secure_zero(token, CANVIEW_BRIDGE_AUTH_TOKEN_BYTES);
    const size_t length = httpd_req_get_hdr_value_len(request, "Authorization");
    static const char prefix[] = "Bearer ";
    if (length <= sizeof(prefix) - 1U || length >= CANVIEW_BRIDGE_WEB_MAX_AUTH_HEADER_BYTES)
    {
        return false;
    }
    char header[CANVIEW_BRIDGE_WEB_MAX_AUTH_HEADER_BYTES] = {0};
    const bool header_valid = httpd_req_get_hdr_value_str(request, "Authorization", header,
                                                          sizeof(header)) == ESP_OK &&
                              memcmp(header, prefix, sizeof(prefix) - 1U) == 0;
    bool parsed = false;
    if (header_valid)
    {
        const size_t value_length = length - (sizeof(prefix) - 1U);
        parsed = base64url_decode(header + sizeof(prefix) - 1U, value_length, token,
                                  CANVIEW_BRIDGE_AUTH_TOKEN_BYTES,
                                  CANVIEW_BRIDGE_AUTH_TOKEN_BYTES);
    }
    secure_zero(header, sizeof(header));
    return parsed;
}

static bool authenticated(httpd_req_t *request, canview_bridge_web_state_t *state)
{
    uint8_t token[CANVIEW_BRIDGE_AUTH_TOKEN_BYTES] = {0};
    const bool parsed = parse_bearer(request, token);
    canview_status_t status = CANVIEW_AUTH_FAILED;
    if (parsed && state_lock_take(state))
    {
        status = canview_bridge_auth_check_token(&state->auth, token);
        state_lock_give(state);
    }
    secure_zero(token, sizeof(token));
    return status == CANVIEW_OK;
}

static bool mutation_allowed(canview_bridge_web_state_t *state, uint64_t now_ms)
{
    if (!state_lock_take(state))
    {
        return false;
    }
    if (state->mutation_count == 0U || now_ms < state->mutation_window_started_ms ||
        now_ms - state->mutation_window_started_ms > CANVIEW_BRIDGE_WEB_MUTATION_WINDOW_MS)
    {
        state->mutation_window_started_ms = now_ms;
        state->mutation_count = 0U;
    }
    if (state->mutation_count >= CANVIEW_BRIDGE_WEB_MUTATION_LIMIT)
    {
        state_lock_give(state);
        return false;
    }
    ++state->mutation_count;
    state_lock_give(state);
    return true;
}

static bool service_window_open(const canview_bridge_web_state_t *state)
{
    bool open = false;
    if (state_lock_take((canview_bridge_web_state_t *)state))
    {
        open = state->service_window_open;
        state_lock_give((canview_bridge_web_state_t *)state);
    }
    return open;
}

static canview_status_t issue_challenge(canview_bridge_web_state_t *state,
                                        uint8_t challenge[CANVIEW_BRIDGE_AUTH_CHALLENGE_BYTES])
{
    if (!state_lock_take(state))
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    const canview_status_t status = canview_bridge_auth_issue_challenge(&state->auth, challenge);
    state_lock_give(state);
    return status;
}

static canview_status_t open_session(canview_bridge_web_state_t *state,
                                     const uint8_t challenge[CANVIEW_BRIDGE_AUTH_CHALLENGE_BYTES],
                                     const uint8_t client_nonce[CANVIEW_BRIDGE_AUTH_CLIENT_NONCE_BYTES],
                                     const char *pin, size_t pin_length,
                                     uint8_t token[CANVIEW_BRIDGE_AUTH_TOKEN_BYTES])
{
    if (!state_lock_take(state))
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    const canview_status_t status = canview_bridge_auth_open_session(
        &state->auth, challenge, client_nonce, pin, pin_length, token);
    state_lock_give(state);
    return status;
}

static canview_status_t logout_session(canview_bridge_web_state_t *state)
{
    if (!state_lock_take(state))
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    const canview_status_t status = canview_bridge_auth_logout(&state->auth);
    state_lock_give(state);
    return status;
}

static bool snapshot_revision(const canview_bridge_web_state_t *state, uint32_t *revision)
{
    if (revision == NULL || !state_lock_take((canview_bridge_web_state_t *)state))
    {
        return false;
    }
    *revision = state->snapshot_revision;
    state_lock_give((canview_bridge_web_state_t *)state);
    return true;
}

static bool next_event_values(canview_bridge_web_state_t *state, uint32_t *sequence,
                              uint32_t *revision)
{
    if (sequence == NULL || revision == NULL || !state_lock_take(state))
    {
        return false;
    }
    if (state->event_sequence < UINT32_MAX)
    {
        ++state->event_sequence;
    }
    *sequence = state->event_sequence;
    *revision = state->snapshot_revision;
    state_lock_give(state);
    return true;
}

static esp_err_t receive_json_body(httpd_req_t *request, canview_bridge_web_state_t *state)
{
    if (request == NULL || state == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    secure_zero(state->request_body, sizeof(state->request_body));
    if (request->content_len == 0U)
    {
        return httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "JSON body required");
    }
    if (request->content_len > CANVIEW_BRIDGE_WEB_MAX_JSON_BYTES)
    {
        secure_zero(state->request_body, sizeof(state->request_body));
        return httpd_resp_send_err(request, HTTPD_413_CONTENT_TOO_LARGE, "JSON body too large");
    }
    size_t received_total = 0U;
    while (received_total < request->content_len)
    {
        const size_t remaining = request->content_len - received_total;
        const int received = httpd_req_recv(request, state->request_body + received_total, remaining);
        if (received <= 0 || (size_t)received > remaining)
        {
            secure_zero(state->request_body, sizeof(state->request_body));
            return httpd_resp_send_err(request, HTTPD_408_REQ_TIMEOUT, "request body timeout");
        }
        received_total += (size_t)received;
    }
    state->request_body[received_total] = '\0';
    return ESP_OK;
}

static bool json_nesting_bounded(const char *text, size_t length)
{
    if (text == NULL || length == 0U)
    {
        return false;
    }
    unsigned depth = 0U;
    bool in_string = false;
    bool escaped = false;
    for (size_t index = 0U; index < length; ++index)
    {
        const char character = text[index];
        if (in_string)
        {
            if (escaped)
            {
                escaped = false;
            }
            else if (character == '\\')
            {
                escaped = true;
            }
            else if (character == '"')
            {
                in_string = false;
            }
            continue;
        }
        if (character == '"')
        {
            in_string = true;
        }
        else if (character == '{' || character == '[')
        {
            if (depth >= CANVIEW_BRIDGE_WEB_MAX_JSON_DEPTH)
            {
                return false;
            }
            ++depth;
        }
        else if (character == '}' || character == ']')
        {
            if (depth == 0U)
            {
                return false;
            }
            --depth;
        }
    }
    return !in_string && !escaped && depth == 0U;
}

static bool session_json_fields(const cJSON *root, const cJSON **challenge,
                                const cJSON **nonce, const cJSON **pin)
{
    if (challenge == NULL || nonce == NULL || pin == NULL)
    {
        return false;
    }
    *challenge = NULL;
    *nonce = NULL;
    *pin = NULL;
    if (!cJSON_IsObject(root))
    {
        return false;
    }

    unsigned field_mask = 0U;
    for (const cJSON *item = root->child; item != NULL; item = item->next)
    {
        if (item->string == NULL || !cJSON_IsString(item) || item->valuestring == NULL)
        {
            return false;
        }
        unsigned field_bit = 0U;
        if (strcmp(item->string, "challenge") == 0)
        {
            field_bit = 1U;
            *challenge = item;
        }
        else if (strcmp(item->string, "client_nonce") == 0)
        {
            field_bit = 2U;
            *nonce = item;
        }
        else if (strcmp(item->string, "pin") == 0)
        {
            field_bit = 4U;
            *pin = item;
        }
        else
        {
            return false;
        }
        if ((field_mask & field_bit) != 0U)
        {
            return false;
        }
        field_mask |= field_bit;
    }
    return field_mask == 7U && *challenge != NULL && *nonce != NULL && *pin != NULL;
}

static esp_err_t handle_root(httpd_req_t *request)
{
    canview_bridge_web_state_t *state = NULL;
    esp_err_t result = enter_request(request, &state);
    if (result != ESP_OK)
    {
        return result;
    }
    if (httpd_resp_set_type(request, "text/html; charset=utf-8") != ESP_OK ||
        httpd_resp_set_hdr(request, "Content-Encoding", "gzip") != ESP_OK ||
        httpd_resp_set_hdr(request, "Cache-Control", "no-store") != ESP_OK ||
        httpd_resp_set_hdr(request, "X-Content-Type-Options", "nosniff") != ESP_OK)
    {
        result = ESP_FAIL;
    }
    else
    {
        result = httpd_resp_send(request, (const char *)canview_bridge_index_html_gz,
                                 (ssize_t)canview_bridge_index_html_gz_len);
    }
    leave_request(state);
    return result;
}

static esp_err_t handle_bootstrap(httpd_req_t *request)
{
    canview_bridge_web_state_t *state = NULL;
    esp_err_t result = enter_request(request, &state);
    if (result != ESP_OK)
    {
        return result;
    }
    uint8_t challenge[CANVIEW_BRIDGE_AUTH_CHALLENGE_BYTES] = {0};
    char challenge_text[CANVIEW_BRIDGE_WEB_WS_TOKEN_TEXT_BYTES + 1U] = {0};
    uint32_t revision = 0U;
    uint64_t now_ms = 0U;
    if (idf_now_ms(NULL, &now_ms) != CANVIEW_OK)
    {
        result = httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "clock unavailable");
        leave_request(state);
        return result;
    }
    if (!service_window_open(state))
    {
        result = httpd_resp_send_err(request, HTTPD_403_FORBIDDEN, "service window closed");
        leave_request(state);
        return result;
    }
    if (!mutation_allowed(state, now_ms))
    {
        result = send_custom_status(request, "429 Too Many Requests", "bootstrap rate limited");
        leave_request(state);
        return result;
    }
    if (issue_challenge(state, challenge) != CANVIEW_OK ||
        !base64url_encode(challenge, sizeof(challenge), challenge_text, sizeof(challenge_text)) ||
        !snapshot_revision(state, &revision))
    {
        result = httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "challenge unavailable");
        leave_request(state);
        return result;
    }
    cJSON *root = cJSON_CreateObject();
    if (root == NULL || !json_add_number(root, "schema_version", 1.0) ||
        !json_add_number(root, "snapshot_revision", (double)revision) ||
        !json_add_number(root, "generated_at_ms", (double)now_ms) ||
        !json_add_string(root, "challenge", challenge_text) ||
        !json_add_string(root, "role", "DIAGNOSTIC_BRIDGE") ||
        !json_add_string(root, "session_state", "REAUTH_REQUIRED") ||
        !json_add_bool(root, "read_only", true) || !json_add_number(root, "control_scope", 0.0) ||
        !json_add_bool(root, "vehicle_tx", false))
    {
        if (root != NULL)
        {
            cJSON_Delete(root);
        }
        json_reset();
        result = httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "bootstrap unavailable");
    }
    else
    {
        result = send_json_locked(request, root);
    }
    secure_zero(challenge, sizeof(challenge));
    secure_zero(challenge_text, sizeof(challenge_text));
    leave_request(state);
    return result;
}

static esp_err_t handle_session_post(httpd_req_t *request)
{
    canview_bridge_web_state_t *state = NULL;
    esp_err_t result = enter_request(request, &state);
    if (result != ESP_OK)
    {
        return result;
    }
    uint64_t now_ms = 0U;
    if (!origin_allowed(request, true) || idf_now_ms(NULL, &now_ms) != CANVIEW_OK)
    {
        result = httpd_resp_send_err(request, HTTPD_403_FORBIDDEN, "origin rejected");
        leave_request(state);
        return result;
    }
    if (!mutation_allowed(state, now_ms))
    {
        result = send_custom_status(request, "429 Too Many Requests", "mutation rate limited");
        leave_request(state);
        return result;
    }
    result = receive_json_body(request, state);
    if (result != ESP_OK)
    {
        leave_request(state);
        return result;
    }
    if (!json_nesting_bounded(state->request_body, request->content_len))
    {
        secure_zero(state->request_body, sizeof(state->request_body));
        result = httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "JSON nesting invalid");
        leave_request(state);
        return result;
    }
    json_reset();
    cJSON *root = cJSON_ParseWithLengthOpts(state->request_body, request->content_len + 1U, NULL, true);
    const cJSON *challenge_item = NULL;
    const cJSON *nonce_item = NULL;
    const cJSON *pin_item = NULL;
    const bool fields_valid = session_json_fields(root, &challenge_item, &nonce_item, &pin_item);
    uint8_t challenge[CANVIEW_BRIDGE_AUTH_CHALLENGE_BYTES] = {0};
    uint8_t client_nonce[CANVIEW_BRIDGE_AUTH_CLIENT_NONCE_BYTES] = {0};
    uint8_t token[CANVIEW_BRIDGE_AUTH_TOKEN_BYTES] = {0};
    uint32_t revision = 0U;
    const bool valid_strings = fields_valid && challenge_item != NULL && nonce_item != NULL &&
                               pin_item != NULL;
    const size_t challenge_length = valid_strings ? bounded_length(challenge_item->valuestring, 23U) : 0U;
    const size_t nonce_length = valid_strings ? bounded_length(nonce_item->valuestring, 23U) : 0U;
    const size_t pin_length = valid_strings ? bounded_length(pin_item->valuestring, 9U) : 0U;
    const bool decoded = valid_strings &&
                         base64url_decode(challenge_item->valuestring, challenge_length, challenge,
                                          sizeof(challenge), sizeof(challenge)) &&
                         base64url_decode(nonce_item->valuestring, nonce_length, client_nonce,
                                          sizeof(client_nonce), sizeof(client_nonce));
    const canview_status_t auth_status = decoded
                                             ? open_session(state, challenge, client_nonce,
                                                            pin_item->valuestring, pin_length,
                                                            token)
                                             : CANVIEW_AUTH_FAILED;
    if (root != NULL)
    {
        cJSON_Delete(root);
    }
    json_reset();
    secure_zero(state->request_body, sizeof(state->request_body));
    secure_zero(challenge, sizeof(challenge));
    secure_zero(client_nonce, sizeof(client_nonce));
    if (auth_status != CANVIEW_OK)
    {
        secure_zero(token, sizeof(token));
        result = httpd_resp_send_err(request, HTTPD_401_UNAUTHORIZED, "authentication failed");
        leave_request(state);
        return result;
    }
    char token_text[CANVIEW_BRIDGE_WEB_WS_TOKEN_TEXT_BYTES + 1U] = {0};
    if (!base64url_encode(token, sizeof(token), token_text, sizeof(token_text)))
    {
        secure_zero(token, sizeof(token));
        result = httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "session unavailable");
        leave_request(state);
        return result;
    }
    if (!snapshot_revision(state, &revision))
    {
        secure_zero(token, sizeof(token));
        secure_zero(token_text, sizeof(token_text));
        result = httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR,
                                     "session unavailable");
        leave_request(state);
        return result;
    }
    cJSON *response = cJSON_CreateObject();
    if (response == NULL || !json_add_string(response, "token", token_text) ||
        !json_add_number(response, "expires_in_ms", CANVIEW_BRIDGE_AUTH_TOKEN_TTL_MS) ||
        !json_add_number(response, "snapshot_revision", (double)revision) ||
        !json_add_bool(response, "read_only", true) ||
        !json_add_number(response, "control_scope", 0.0) || !json_add_bool(response, "vehicle_tx", false))
    {
        if (response != NULL)
        {
            cJSON_Delete(response);
        }
        json_reset();
        result = httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "session unavailable");
    }
    else
    {
        result = send_json_locked(request, response);
    }
    secure_zero(token, sizeof(token));
    secure_zero(token_text, sizeof(token_text));
    leave_request(state);
    return result;
}

static esp_err_t handle_session_delete(httpd_req_t *request)
{
    canview_bridge_web_state_t *state = NULL;
    esp_err_t result = enter_request(request, &state);
    if (result != ESP_OK)
    {
        return result;
    }
    uint64_t now_ms = 0U;
    if (!origin_allowed(request, true) || idf_now_ms(NULL, &now_ms) != CANVIEW_OK)
    {
        result = httpd_resp_send_err(request, HTTPD_403_FORBIDDEN, "origin rejected");
    }
    else if (!mutation_allowed(state, now_ms))
    {
        result = send_custom_status(request, "429 Too Many Requests", "mutation rate limited");
    }
    else if (!authenticated(request, state))
    {
        result = httpd_resp_send_err(request, HTTPD_401_UNAUTHORIZED, "authentication required");
    }
    else if (logout_session(state) != CANVIEW_OK)
    {
        result = httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "logout unavailable");
    }
    else
    {
        cJSON *response = cJSON_CreateObject();
        if (response == NULL || !json_add_string(response, "state", "REVOKED") ||
            !json_add_bool(response, "read_only", true))
        {
            if (response != NULL)
            {
                cJSON_Delete(response);
            }
            json_reset();
            result = httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "logout unavailable");
        }
        else
        {
            result = send_json_locked(request, response);
        }
    }
    leave_request(state);
    return result;
}

static esp_err_t handle_system(httpd_req_t *request)
{
    canview_bridge_web_state_t *state = NULL;
    esp_err_t result = enter_request(request, &state);
    if (result != ESP_OK)
    {
        return result;
    }
    if (!authenticated(request, state))
    {
        result = httpd_resp_send_err(request, HTTPD_401_UNAUTHORIZED, "authentication required");
        leave_request(state);
        return result;
    }
    uint32_t revision = 0U;
    if (!snapshot_revision(state, &revision))
    {
        result = httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR,
                                     "system unavailable");
        leave_request(state);
        return result;
    }
    cJSON *root = cJSON_CreateObject();
    if (root == NULL || !json_add_string(root, "role", "DIAGNOSTIC_BRIDGE") ||
        !json_add_string(root, "observer_state", "NOT_IMPLEMENTED") ||
        !json_add_string(root, "vehicle_state", "UNKNOWN") ||
        !json_add_number(root, "snapshot_revision", (double)revision) ||
        !json_add_number(root, "wifi_channel", CANVIEW_BRIDGE_WEB_WIFI_CHANNEL) ||
        !json_add_number(root, "client_limit", 1.0) || !json_add_bool(root, "read_only", true) ||
        !json_add_number(root, "control_scope", 0.0) || !json_add_bool(root, "vehicle_tx", false))
    {
        if (root != NULL)
        {
            cJSON_Delete(root);
        }
        json_reset();
        result = httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "system unavailable");
    }
    else
    {
        result = send_json_locked(request, root);
    }
    leave_request(state);
    return result;
}

static esp_err_t handle_collection(httpd_req_t *request)
{
    canview_bridge_web_state_t *state = NULL;
    esp_err_t result = enter_request(request, &state);
    if (result != ESP_OK)
    {
        return result;
    }
    if (!authenticated(request, state))
    {
        result = httpd_resp_send_err(request, HTTPD_401_UNAUTHORIZED, "authentication required");
        leave_request(state);
        return result;
    }
    uint32_t revision = 0U;
    if (!snapshot_revision(state, &revision))
    {
        result = httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR,
                                     "snapshot unavailable");
        leave_request(state);
        return result;
    }
    cJSON *root = cJSON_CreateObject();
    cJSON *items = cJSON_CreateArray();
    if (root == NULL || items == NULL || !json_add_item(root, "items", items) ||
        !json_add_string(root, "state", "NOT_IMPLEMENTED") ||
        !json_add_number(root, "snapshot_revision", (double)revision) ||
        !json_add_bool(root, "read_only", true) || !json_add_number(root, "control_scope", 0.0) ||
        !json_add_bool(root, "vehicle_tx", false))
    {
        if (root != NULL)
        {
            cJSON_Delete(root);
        }
        else if (items != NULL)
        {
            cJSON_Delete(items);
        }
        json_reset();
        result = httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "snapshot unavailable");
    }
    else
    {
        result = send_json_locked(request, root);
    }
    leave_request(state);
    return result;
}

static bool parse_ws_token(httpd_req_t *request,
                           uint8_t token[CANVIEW_BRIDGE_AUTH_TOKEN_BYTES])
{
    if (request == NULL || token == NULL)
    {
        return false;
    }
    secure_zero(token, CANVIEW_BRIDGE_AUTH_TOKEN_BYTES);
    const size_t length = httpd_req_get_hdr_value_len(request, "Sec-WebSocket-Protocol");
    if (length == 0U || length >= CANVIEW_BRIDGE_WEB_MAX_AUTH_HEADER_BYTES)
    {
        return false;
    }
    char protocols[CANVIEW_BRIDGE_WEB_MAX_AUTH_HEADER_BYTES] = {0};
    if (httpd_req_get_hdr_value_str(request, "Sec-WebSocket-Protocol", protocols,
                                    sizeof(protocols)) != ESP_OK)
    {
        return false;
    }
    const size_t prefix_length = sizeof(CANVIEW_BRIDGE_WEB_WS_TOKEN_PREFIX) - 1U;
    const size_t supported_length = sizeof(CANVIEW_BRIDGE_WEB_WS_PROTOCOL) - 1U;
    bool supported = false;
    bool token_found = false;
    bool valid = true;
    size_t begin = 0U;
    while (begin < length)
    {
        while (begin < length && (protocols[begin] == ' ' || protocols[begin] == '\t'))
        {
            ++begin;
        }
        if (begin >= length)
        {
            valid = false;
            break;
        }
        size_t end = begin;
        while (end < length && protocols[end] != ',')
        {
            ++end;
        }
        const size_t item_end = end;
        while (end > begin && (protocols[end - 1U] == ' ' || protocols[end - 1U] == '\t'))
        {
            --end;
        }
        const size_t item_length = end - begin;
        if (item_length == 0U)
        {
            valid = false;
            break;
        }
        if (item_length == supported_length &&
            memcmp(protocols + begin, CANVIEW_BRIDGE_WEB_WS_PROTOCOL, supported_length) == 0)
        {
            if (supported)
            {
                valid = false;
                break;
            }
            supported = true;
        }
        else if (item_length >= prefix_length &&
                 memcmp(protocols + begin, CANVIEW_BRIDGE_WEB_WS_TOKEN_PREFIX, prefix_length) == 0)
        {
            if (item_length != prefix_length + CANVIEW_BRIDGE_WEB_WS_TOKEN_TEXT_BYTES || token_found ||
                !base64url_decode(protocols + begin + prefix_length,
                                  CANVIEW_BRIDGE_WEB_WS_TOKEN_TEXT_BYTES, token,
                                  CANVIEW_BRIDGE_AUTH_TOKEN_BYTES, CANVIEW_BRIDGE_AUTH_TOKEN_BYTES))
            {
                valid = false;
                break;
            }
            token_found = true;
        }
        if (item_end == length)
        {
            break;
        }
        begin = item_end + 1U;
        if (begin >= length)
        {
            valid = false;
            break;
        }
    }
    const bool parsed = valid && supported && token_found;
    secure_zero(protocols, sizeof(protocols));
    return parsed;
}

static bool ws_token_authenticated(canview_bridge_web_state_t *state,
                                   const uint8_t token[CANVIEW_BRIDGE_AUTH_TOKEN_BYTES])
{
    if (token == NULL || !state_lock_take(state))
    {
        return false;
    }
    const bool valid = canview_bridge_auth_check_token(&state->auth, token) == CANVIEW_OK;
    state_lock_give(state);
    return valid;
}

static esp_err_t ws_pre_handshake(httpd_req_t *request)
{
    canview_bridge_web_state_t *state = NULL;
    if (enter_request(request, &state) != ESP_OK)
    {
        return ESP_FAIL;
    }
    uint8_t token[CANVIEW_BRIDGE_AUTH_TOKEN_BYTES] = {0};
    const bool allowed = origin_allowed(request, true) && parse_ws_token(request, token) &&
                         ws_token_authenticated(state, token);
    secure_zero(token, sizeof(token));
    leave_request(state);
    return allowed ? ESP_OK : ESP_FAIL;
}

static cJSON *make_live_event(uint32_t sequence, uint32_t revision, uint64_t now_ms)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *payload = cJSON_CreateObject();
    if (root == NULL || payload == NULL || !json_add_string(root, "type", "system.summary") ||
        !json_add_number(root, "seq", (double)sequence) ||
        !json_add_number(root, "server_time_ms", (double)now_ms) ||
        !json_add_number(root, "snapshot_revision", (double)revision) ||
        !json_add_bool(payload, "read_only", true) || !json_add_number(payload, "control_scope", 0.0) ||
        !json_add_bool(payload, "vehicle_tx", false) ||
        !json_add_string(payload, "observer_state", "NOT_IMPLEMENTED") ||
        !json_add_item(root, "payload", payload))
    {
        if (root != NULL)
        {
            cJSON_Delete(root);
        }
        else if (payload != NULL)
        {
            cJSON_Delete(payload);
        }
        json_reset();
        return NULL;
    }
    return root;
}

static esp_err_t ws_post_handshake(httpd_req_t *request)
{
    canview_bridge_web_state_t *state = NULL;
    esp_err_t result = enter_request(request, &state);
    if (result != ESP_OK)
    {
        return result;
    }
    uint8_t token[CANVIEW_BRIDGE_AUTH_TOKEN_BYTES] = {0};
    uint32_t sequence = 0U;
    uint32_t revision = 0U;
    secure_zero(state->ws_response, sizeof(state->ws_response));
    if (!origin_allowed(request, true) || !parse_ws_token(request, token) ||
        !ws_token_authenticated(state, token))
    {
        secure_zero(token, sizeof(token));
        leave_request(state);
        return ESP_FAIL;
    }
    secure_zero(token, sizeof(token));
    uint64_t now_ms = 0U;
    if (idf_now_ms(NULL, &now_ms) != CANVIEW_OK)
    {
        leave_request(state);
        return ESP_FAIL;
    }
    if (!next_event_values(state, &sequence, &revision))
    {
        leave_request(state);
        return ESP_FAIL;
    }
    cJSON *event = make_live_event(sequence, revision, now_ms);
    if (event == NULL)
    {
        secure_zero(state->response, sizeof(state->response));
        secure_zero(state->ws_response, sizeof(state->ws_response));
        leave_request(state);
        return ESP_FAIL;
    }
    const bool printed = cJSON_PrintPreallocated(event, state->ws_response,
                                                  sizeof(state->ws_response), false) != 0;
    cJSON_Delete(event);
    json_reset();
    if (!printed)
    {
        secure_zero(state->response, sizeof(state->response));
        secure_zero(state->ws_response, sizeof(state->ws_response));
        leave_request(state);
        return ESP_FAIL;
    }
    const size_t response_length = bounded_length(state->ws_response, sizeof(state->ws_response));
    if (response_length >= sizeof(state->ws_response))
    {
        secure_zero(state->response, sizeof(state->response));
        secure_zero(state->ws_response, sizeof(state->ws_response));
        leave_request(state);
        return ESP_FAIL;
    }
    secure_zero(state->response, sizeof(state->response));
    if (!enter_ws_io(state))
    {
        secure_zero(state->ws_response, sizeof(state->ws_response));
        leave_request(state);
        return ESP_FAIL;
    }
    httpd_ws_frame_t frame = {0};
    frame.type = HTTPD_WS_TYPE_TEXT;
    frame.payload = (uint8_t *)state->ws_response;
    frame.len = response_length;
    result = httpd_ws_send_frame(request, &frame);
    leave_ws_io(state);
    secure_zero(state->ws_response, sizeof(state->ws_response));
    leave_request(state);
    return result;
}

static esp_err_t handle_live(httpd_req_t *request)
{
    canview_bridge_web_state_t *state = NULL;
    esp_err_t result = enter_request(request, &state);
    if (result != ESP_OK)
    {
        return result;
    }
    uint8_t token[CANVIEW_BRIDGE_AUTH_TOKEN_BYTES] = {0};
    const bool allowed = origin_allowed(request, true) && parse_ws_token(request, token) &&
                         ws_token_authenticated(state, token);
    secure_zero(token, sizeof(token));
    if (!allowed)
    {
        leave_request(state);
        return ESP_FAIL;
    }
    leave_request(state);
    if (!enter_ws_io(state))
    {
        return ESP_FAIL;
    }
    httpd_ws_frame_t frame = {0};
    frame.type = HTTPD_WS_TYPE_TEXT;
    result = httpd_ws_recv_frame(request, &frame, 0U);
    if (result == ESP_OK && frame.len > CANVIEW_BRIDGE_WEB_MAX_WS_FRAME_BYTES)
    {
        result = ESP_FAIL;
    }
    else if (result == ESP_OK && frame.len > 0U)
    {
        frame.payload = state->ws_body;
        result = httpd_ws_recv_frame(request, &frame, frame.len);
    }
    secure_zero(state->ws_body, sizeof(state->ws_body));
    leave_ws_io(state);
    return result;
}

static esp_err_t handle_static_or_not_found(httpd_req_t *request)
{
    if (request == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (bounded_length(request->uri, CANVIEW_BRIDGE_WEB_MAX_URI_BYTES + 1U) >=
        CANVIEW_BRIDGE_WEB_MAX_URI_BYTES + 1U)
    {
        return httpd_resp_send_err(request, HTTPD_414_URI_TOO_LONG, "URI too long");
    }
    if (strncmp(request->uri, "/api/", sizeof("/api/") - 1U) == 0)
    {
        return httpd_resp_send_err(request, HTTPD_404_NOT_FOUND, "not found");
    }
    return handle_root(request);
}

static bool valid_ap_password(const char *password)
{
    const size_t length = bounded_length(password, CANVIEW_BRIDGE_WEB_AP_PASSWORD_BYTES);
    if (length < CANVIEW_BRIDGE_WEB_MIN_AP_PASSWORD_LENGTH ||
        length > CANVIEW_BRIDGE_WEB_MAX_AP_PASSWORD_LENGTH)
    {
        return false;
    }
    for (size_t index = 0U; index < length; ++index)
    {
        if ((unsigned char)password[index] < 0x21U || (unsigned char)password[index] > 0x7EU)
        {
            return false;
        }
    }
    return true;
}

static esp_err_t start_wifi(void)
{
    esp_err_t status = ESP_OK;
    wifi_config_t station_config = {0};
    wifi_config_t ap_config = {0};
    uint8_t mac[6] = {0};
    status = esp_netif_init();
    if (status != ESP_OK && status != ESP_ERR_INVALID_STATE)
    {
        goto cleanup;
    }
    status = esp_event_loop_create_default();
    if (status != ESP_OK && status != ESP_ERR_INVALID_STATE)
    {
        goto cleanup;
    }
    web_state.event_loop_initialized = status == ESP_OK;
    const wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    status = esp_wifi_init(&init_config);
    if (status != ESP_OK)
    {
        goto cleanup;
    }
    web_state.wifi_initialized = status == ESP_OK;
    web_state.wifi_ap_netif = esp_netif_create_default_wifi_ap();
    if (web_state.wifi_ap_netif == NULL)
    {
        status = ESP_ERR_NO_MEM;
        goto cleanup;
    }
    web_state.wifi_sta_netif = esp_netif_create_default_wifi_sta();
    if (web_state.wifi_sta_netif == NULL)
    {
        status = ESP_ERR_NO_MEM;
        goto cleanup;
    }
    status = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (status != ESP_OK)
    {
        goto cleanup;
    }
    status = esp_wifi_set_mode(WIFI_MODE_APSTA);
    if (status != ESP_OK)
    {
        goto cleanup;
    }
    status = esp_wifi_set_country_code("KR", false);
    if (status != ESP_OK)
    {
        goto cleanup;
    }
    status = esp_wifi_set_config(WIFI_IF_STA, &station_config);
    if (status != ESP_OK)
    {
        goto cleanup;
    }
    status = esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    if (status != ESP_OK)
    {
        goto cleanup;
    }
    const int ssid_length = snprintf((char *)ap_config.ap.ssid, sizeof(ap_config.ap.ssid),
                                     "CANView-DIAG-%02X%02X", mac[4], mac[5]);
    if (ssid_length <= 0 || (size_t)ssid_length >= sizeof(ap_config.ap.ssid))
    {
        status = ESP_ERR_INVALID_ARG;
        goto cleanup;
    }
    const size_t password_length = bounded_length(web_state.ap_password,
                                                 sizeof(web_state.ap_password));
    memcpy(ap_config.ap.password, web_state.ap_password, password_length + 1U);
    ap_config.ap.ssid_len = (uint8_t)ssid_length;
    ap_config.ap.channel = CANVIEW_BRIDGE_WEB_WIFI_CHANNEL;
    ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    ap_config.ap.max_connection = 1U;
    ap_config.ap.beacon_interval = 100U;
    ap_config.ap.pmf_cfg.capable = true;
    ap_config.ap.pmf_cfg.required = false;
    status = esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    secure_zero(&ap_config, sizeof(ap_config));
    secure_zero(mac, sizeof(mac));
    if (status != ESP_OK)
    {
        goto cleanup;
    }
    status = esp_wifi_start();
    if (status != ESP_OK)
    {
        goto cleanup;
    }
    web_state.wifi_started = true;
    status = esp_wifi_set_channel(CANVIEW_BRIDGE_WEB_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

cleanup:
    secure_zero(&station_config, sizeof(station_config));
    secure_zero(&ap_config, sizeof(ap_config));
    secure_zero(mac, sizeof(mac));
    return status;
}

static bool web_service_window_expired(canview_bridge_web_state_t *state, uint64_t now_ms)
{
    return state->service_window_open &&
           (now_ms < state->service_window_started_ms ||
            now_ms - state->service_window_started_ms > CANVIEW_BRIDGE_WEB_SERVICE_WINDOW_MS);
}

static void close_session(httpd_handle_t server, int client_fd)
{
    (void)server;
    if (!state_lock_take(&web_state))
    {
        return;
    }
    if (web_state.active_client_fd == client_fd)
    {
        web_state.active_client_fd = -1;
        web_state.session_close_pending = false;
        (void)canview_bridge_auth_logout(&web_state.auth);
    }
    state_lock_give(&web_state);
}

static const httpd_uri_t uri_root = {.uri = "/", .method = HTTP_GET, .handler = handle_root,
                                     .user_ctx = &web_state};
static const httpd_uri_t uri_bootstrap = {.uri = "/api/v1/bootstrap",
                                          .method = HTTP_GET,
                                          .handler = handle_bootstrap,
                                          .user_ctx = &web_state};
static const httpd_uri_t uri_session_post = {.uri = "/api/v1/session",
                                             .method = HTTP_POST,
                                             .handler = handle_session_post,
                                             .user_ctx = &web_state};
static const httpd_uri_t uri_session_delete = {.uri = "/api/v1/session",
                                               .method = HTTP_DELETE,
                                               .handler = handle_session_delete,
                                               .user_ctx = &web_state};
static const httpd_uri_t uri_system = {.uri = "/api/v1/system",
                                       .method = HTTP_GET,
                                       .handler = handle_system,
                                       .user_ctx = &web_state};
static const httpd_uri_t uri_peers = {.uri = "/api/v1/peers",
                                      .method = HTTP_GET,
                                      .handler = handle_collection,
                                      .user_ctx = &web_state};
static const httpd_uri_t uri_buses = {.uri = "/api/v1/buses",
                                      .method = HTTP_GET,
                                      .handler = handle_collection,
                                      .user_ctx = &web_state};
static const httpd_uri_t uri_frames = {.uri = "/api/v1/frames",
                                       .method = HTTP_GET,
                                       .handler = handle_collection,
                                       .user_ctx = &web_state};
static const httpd_uri_t uri_filters = {.uri = "/api/v1/filters",
                                        .method = HTTP_GET,
                                        .handler = handle_collection,
                                        .user_ctx = &web_state};
static const httpd_uri_t uri_captures = {.uri = "/api/v1/captures",
                                         .method = HTTP_GET,
                                         .handler = handle_collection,
                                         .user_ctx = &web_state};
static const httpd_uri_t uri_candidates = {.uri = "/api/v1/candidates",
                                           .method = HTTP_GET,
                                           .handler = handle_collection,
                                           .user_ctx = &web_state};
static const httpd_uri_t uri_config_targets = {.uri = "/api/v1/config-targets",
                                               .method = HTTP_GET,
                                               .handler = handle_collection,
                                               .user_ctx = &web_state};
static const httpd_uri_t uri_live = {
    .uri = "/api/v1/live",
    .method = HTTP_GET,
    .handler = handle_live,
    .user_ctx = &web_state,
    .is_websocket = true,
    .handle_ws_control_frames = true,
    .supported_subprotocol = CANVIEW_BRIDGE_WEB_WS_PROTOCOL,
#if CONFIG_HTTPD_WS_PRE_HANDSHAKE_CB_SUPPORT
    .ws_pre_handshake_cb = ws_pre_handshake,
#endif
#if CONFIG_HTTPD_WS_POST_HANDSHAKE_CB_SUPPORT
    .ws_post_handshake_cb = ws_post_handshake,
#endif
};
static const httpd_uri_t uri_fallback = {.uri = "/*",
                                         .method = HTTP_GET,
                                         .handler = handle_static_or_not_found,
                                         .user_ctx = &web_state};

static esp_err_t register_uri(httpd_handle_t server, const httpd_uri_t *uri)
{
    return httpd_register_uri_handler(server, uri);
}

esp_err_t canview_bridge_web_start(const canview_bridge_web_config_t *config)
{
    if (config == NULL || config->pin_digest == NULL || !valid_ap_password(config->ap_password) ||
        config->button_pressed == NULL || web_state.initialized)
    {
        return ESP_ERR_INVALID_ARG;
    }
    const esp_err_t stale_cleanup_status = discard_start_state();
    if (stale_cleanup_status != ESP_OK)
    {
        return stale_cleanup_status;
    }
    memset(&web_state, 0, sizeof(web_state));
    web_state.active_client_fd = -1;
    web_state.snapshot_revision = 1U;
    web_state.config = *config;
    const size_t password_length = bounded_length(config->ap_password,
                                                 CANVIEW_BRIDGE_WEB_AP_PASSWORD_BYTES);
    memcpy(web_state.ap_password, config->ap_password, password_length + 1U);
    web_state.lock = xSemaphoreCreateMutex();
    if (web_state.lock == NULL)
    {
        discard_start_state();
        return ESP_ERR_NO_MEM;
    }
    web_state.request_lock = xSemaphoreCreateMutex();
    if (web_state.request_lock == NULL)
    {
        discard_start_state();
        return ESP_ERR_NO_MEM;
    }
    web_state.ws_io_lock = xSemaphoreCreateMutex();
    if (web_state.ws_io_lock == NULL)
    {
        discard_start_state();
        return ESP_ERR_NO_MEM;
    }
    const canview_bridge_auth_callbacks_t callbacks = {idf_now_ms, idf_digest, idf_mac, idf_random,
                                                       NULL};
    if (canview_bridge_auth_init(&web_state.auth, config->pin_digest, &callbacks) != CANVIEW_OK)
    {
        discard_start_state();
        return ESP_ERR_INVALID_ARG;
    }
    web_state.config.pin_digest = NULL;
    web_state.config.ap_password = NULL;
    cJSON_Hooks hooks = {.malloc_fn = json_malloc, .free_fn = json_free};
    cJSON_InitHooks(&hooks);
    json_reset();
    esp_err_t status = start_wifi();
    secure_zero(web_state.ap_password, sizeof(web_state.ap_password));
    if (status != ESP_OK)
    {
        discard_start_state();
        return status;
    }
    httpd_config_t http_config = HTTPD_DEFAULT_CONFIG();
    http_config.stack_size = 6144U;
    http_config.max_req_hdr_len = 1024U;
    http_config.max_uri_len = CANVIEW_BRIDGE_WEB_MAX_URI_BYTES;
    http_config.max_open_sockets = 1U;
    http_config.max_uri_handlers = 20U;
    http_config.max_resp_headers = 6U;
    http_config.backlog_conn = 1U;
    http_config.lru_purge_enable = true;
    http_config.recv_wait_timeout = 5U;
    http_config.send_wait_timeout = 5U;
    http_config.close_fn = close_session;
    status = httpd_start(&web_state.server, &http_config);
    if (status != ESP_OK)
    {
        discard_start_state();
        return status;
    }
    const httpd_uri_t *uris[] = {&uri_root,       &uri_bootstrap, &uri_session_post,
                                 &uri_session_delete, &uri_system,    &uri_peers,
                                 &uri_buses,      &uri_frames,    &uri_filters,
                                 &uri_captures,   &uri_candidates, &uri_config_targets,
                                 &uri_live,       &uri_fallback};
    for (size_t index = 0U; index < sizeof(uris) / sizeof(uris[0]); ++index)
    {
        status = register_uri(web_state.server, uris[index]);
        if (status != ESP_OK)
        {
            (void)httpd_stop(web_state.server);
            web_state.server = NULL;
            discard_start_state();
            return status;
        }
    }
    status = canview_bridge_dns_start();
    if (status != ESP_OK)
    {
        (void)httpd_stop(web_state.server);
        web_state.server = NULL;
        discard_start_state();
        return status;
    }
    web_state.dns_started = true;
    web_state.initialized = true;
    ESP_LOGI(CANVIEW_BRIDGE_WEB_TAG, "read-only local shell ready channel=%u client-limit=1 tx=0",
             CANVIEW_BRIDGE_WEB_WIFI_CHANNEL);
    return ESP_OK;
}

esp_err_t canview_bridge_web_stop(void)
{
    return discard_start_state();
}

esp_err_t canview_bridge_web_poll(void)
{
    if (!web_state.initialized || web_state.lock == NULL || web_state.config.button_pressed == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    uint64_t now_ms = 0U;
    if (idf_now_ms(NULL, &now_ms) != CANVIEW_OK)
    {
        return ESP_ERR_INVALID_STATE;
    }
    const bool pressed = web_state.config.button_pressed(web_state.config.button_context);
    if (!state_lock_take(&web_state))
    {
        return ESP_ERR_TIMEOUT;
    }
    httpd_handle_t expired_server = NULL;
    int expired_client_fd = -1;
    if (pressed)
    {
        if (!web_state.button_down)
        {
            web_state.button_down = true;
            web_state.button_started_ms = now_ms;
            web_state.button_hold_consumed = false;
        }
        if (!web_state.service_window_open && !web_state.button_hold_consumed &&
            now_ms >= web_state.button_started_ms &&
            now_ms - web_state.button_started_ms >= CANVIEW_BRIDGE_WEB_SERVICE_HOLD_MS)
        {
            web_state.button_hold_consumed = true;
            web_state.service_window_open = true;
            web_state.service_window_started_ms = now_ms;
            if (canview_bridge_auth_set_service_window(&web_state.auth, true) != CANVIEW_OK)
            {
                web_state.service_window_open = false;
            }
        }
    }
    else
    {
        web_state.button_down = false;
        web_state.button_hold_consumed = false;
    }
    if (web_service_window_expired(&web_state, now_ms))
    {
        web_state.service_window_open = false;
        web_state.button_hold_consumed = true;
        (void)canview_bridge_auth_set_service_window(&web_state.auth, false);
        if (web_state.active_client_fd >= 0 && web_state.server != NULL)
        {
            expired_server = web_state.server;
            expired_client_fd = web_state.active_client_fd;
            web_state.session_close_pending = true;
            (void)canview_bridge_auth_logout(&web_state.auth);
        }
    }
    state_lock_give(&web_state);
    if (expired_server != NULL && expired_client_fd >= 0)
    {
        (void)httpd_sess_trigger_close(expired_server, expired_client_fd);
    }
    return ESP_OK;
}

bool canview_bridge_web_service_window_open(void)
{
    if (!web_state.initialized || !state_lock_take(&web_state))
    {
        return false;
    }
    const bool open = web_state.service_window_open;
    state_lock_give(&web_state);
    return open;
}
