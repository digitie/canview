/* SPDX-License-Identifier: GPL-3.0-only */
/** @file app_main.c
 *  @brief Diagnostic Bridge의 단일 owner boot와 read-only web shell 연결.
 */
#include "board_pins.h"
#include "bridge_button.h"
#include "bridge_bootstrap.h"
#include "canview_board.h"
#include "canview_bridge_auth.h"
#include "canview_bridge_web.h"
#include "canview_esp_pool.h"
#include "canview_esp_runtime.h"
#include "canview_status.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define CANVIEW_BRIDGE_APP_TAG "bridge_app"
#define CANVIEW_BRIDGE_AUTH_NAMESPACE "bridge_auth"
#define CANVIEW_BRIDGE_PIN_DIGEST_KEY "pin_digest"
#define CANVIEW_BRIDGE_AP_PASSWORD_KEY "ap_password"

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

static bool all_zero(const uint8_t *data, size_t length)
{
    if (data == NULL)
    {
        return true;
    }
    uint8_t combined = 0U;
    for (size_t index = 0U; index < length; ++index)
    {
        combined |= data[index];
    }
    return combined == 0U;
}

static bool valid_ap_password(const char *password, size_t length)
{
    if (password == NULL || length < 16U || length > 63U)
    {
        return false;
    }
    for (size_t index = 0U; index < length; ++index)
    {
        const unsigned char character = (unsigned char)password[index];
        if (character < 0x21U || character > 0x7EU)
        {
            return false;
        }
    }
    return true;
}

static esp_err_t stop_web_with_retry(void)
{
    esp_err_t status = canview_bridge_web_stop();
    if (status != ESP_OK)
    {
        ESP_LOGW(CANVIEW_BRIDGE_APP_TAG, "web cleanup retry status=%d", (int)status);
        status = canview_bridge_web_stop();
    }
    return status;
}

static esp_err_t load_credentials(uint8_t pin_digest[CANVIEW_BRIDGE_AUTH_PIN_DIGEST_BYTES],
                                  char ap_password[CANVIEW_BRIDGE_WEB_AP_PASSWORD_BYTES])
{
    if (pin_digest == NULL || ap_password == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    memset(pin_digest, 0, CANVIEW_BRIDGE_AUTH_PIN_DIGEST_BYTES);
    memset(ap_password, 0, CANVIEW_BRIDGE_WEB_AP_PASSWORD_BYTES);

    esp_err_t status = nvs_flash_init();
    if (status != ESP_OK)
    {
        return status;
    }
    nvs_handle_t handle = 0U;
    status = nvs_open(CANVIEW_BRIDGE_AUTH_NAMESPACE, NVS_READONLY, &handle);
    if (status != ESP_OK)
    {
        return status;
    }

    size_t digest_length = CANVIEW_BRIDGE_AUTH_PIN_DIGEST_BYTES;
    status = nvs_get_blob(handle, CANVIEW_BRIDGE_PIN_DIGEST_KEY, pin_digest, &digest_length);
    if (status != ESP_OK || digest_length != CANVIEW_BRIDGE_AUTH_PIN_DIGEST_BYTES ||
        all_zero(pin_digest, CANVIEW_BRIDGE_AUTH_PIN_DIGEST_BYTES))
    {
        nvs_close(handle);
        secure_zero(pin_digest, CANVIEW_BRIDGE_AUTH_PIN_DIGEST_BYTES);
        secure_zero(ap_password, CANVIEW_BRIDGE_WEB_AP_PASSWORD_BYTES);
        return status == ESP_OK ? ESP_ERR_INVALID_SIZE : status;
    }

    size_t password_storage_length = CANVIEW_BRIDGE_WEB_AP_PASSWORD_BYTES;
    status = nvs_get_str(handle, CANVIEW_BRIDGE_AP_PASSWORD_KEY, ap_password,
                         &password_storage_length);
    nvs_close(handle);
    if (status != ESP_OK || password_storage_length == 0U ||
        password_storage_length > CANVIEW_BRIDGE_WEB_AP_PASSWORD_BYTES ||
        ap_password[password_storage_length - 1U] != '\0' ||
        !valid_ap_password(ap_password, password_storage_length - 1U))
    {
        secure_zero(pin_digest, CANVIEW_BRIDGE_AUTH_PIN_DIGEST_BYTES);
        secure_zero(ap_password, CANVIEW_BRIDGE_WEB_AP_PASSWORD_BYTES);
        return status == ESP_OK ? ESP_ERR_INVALID_SIZE : status;
    }
    return ESP_OK;
}

static bool button_pressed(void *context)
{
    (void)context;
    return canview_bridge_button_pressed();
}

static canview_status_t web_poll_status(esp_err_t status)
{
    if (status == ESP_OK)
    {
        return CANVIEW_OK;
    }
    return status == ESP_ERR_TIMEOUT ? CANVIEW_TIMEOUT : CANVIEW_NOT_IMPLEMENTED;
}

void app_main(void)
{
    static canview_esp_core_t core;
    static canview_esp_pool_t pool;
    static canview_esp_runtime_t runtime;
    static uint8_t pin_digest[CANVIEW_BRIDGE_AUTH_PIN_DIGEST_BYTES];
    static char ap_password[CANVIEW_BRIDGE_WEB_AP_PASSWORD_BYTES];
    bool web_started = false;
    canview_esp_runtime_port_t port = {0};
    const canview_platform_port_t board = canview_board_port();
    canview_status_t status = canview_bridge_bootstrap(&core, &pool, &runtime, &port, &board);
    if (status == CANVIEW_OK)
    {
        const esp_err_t credential_status = load_credentials(pin_digest, ap_password);
        if (credential_status != ESP_OK)
        {
            ESP_LOGE(CANVIEW_BRIDGE_APP_TAG, "auth provisioning unavailable status=%d",
                     (int)credential_status);
            status = CANVIEW_NOT_IMPLEMENTED;
        }
    }
    if (status == CANVIEW_OK)
    {
        const canview_bridge_web_config_t config = {pin_digest, ap_password, button_pressed, NULL};
        const esp_err_t web_status = canview_bridge_web_start(&config);
        if (web_status != ESP_OK)
        {
            ESP_LOGE(CANVIEW_BRIDGE_APP_TAG, "read-only web shell start failed status=%d",
                     (int)web_status);
            status = web_poll_status(web_status);
        }
        else if (canview_esp_core_arm_watchdog(&core) != CANVIEW_OK)
        {
            ESP_LOGE(CANVIEW_BRIDGE_APP_TAG, "watchdog arm failed after web startup");
            const esp_err_t stop_status = stop_web_with_retry();
            if (stop_status != ESP_OK)
            {
                ESP_LOGE(CANVIEW_BRIDGE_APP_TAG, "web cleanup failed status=%d", (int)stop_status);
            }
            status = CANVIEW_NOT_IMPLEMENTED;
        }
        else
        {
            web_started = true;
        }
    }
    secure_zero(pin_digest, sizeof(pin_digest));
    secure_zero(ap_password, sizeof(ap_password));

    if (port.report != NULL)
    {
        port.report(port.context, &core, status);
    }
    while (status == CANVIEW_OK)
    {
        if (port.wait == NULL)
        {
            status = CANVIEW_INVALID_ARGUMENT;
            break;
        }
        status = port.wait(port.context);
        if (status == CANVIEW_OK)
        {
            status = web_poll_status(canview_bridge_web_poll());
        }
        if (status == CANVIEW_OK)
        {
            status = canview_esp_core_step(&core);
        }
    }
    if (web_started)
    {
        const esp_err_t stop_status = stop_web_with_retry();
        if (stop_status != ESP_OK)
        {
            ESP_LOGE(CANVIEW_BRIDGE_APP_TAG, "web cleanup failed status=%d", (int)stop_status);
        }
        web_started = false;
    }
    if (port.report != NULL)
    {
        port.report(port.context, &core, status);
    }
    if (board.idle != NULL)
    {
        for (;;)
        {
            board.idle(board.context);
        }
    }
}
