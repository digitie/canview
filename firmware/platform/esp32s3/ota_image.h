/* SPDX-License-Identifier: GPL-3.0-only */
/** @file ota_image.h @brief BSP 전용 ESP-IDF native image 읽기·서명 검사. */
#ifndef CANVIEW_ESP_OTA_IMAGE_H
#define CANVIEW_ESP_OTA_IMAGE_H
#include <stdint.h>
#if defined(CANVIEW_ESP_IMAGE_SDK_TEST)
#include "esp_image_sdk_fixture.h"
#else
#include "esp_app_desc.h"
#include "esp_partition.h"
#endif

#define CANVIEW_ESP_IMAGE_CUSTOM_BYTES (168U)
#define CANVIEW_ESP_IMAGE_DIGEST_BYTES (32U)

/** @brief 서명된 native bytes. CANView role/board/floor 판정은 별도 core 책임이다. */
typedef struct
{
    esp_app_desc_t app;
    uint8_t custom[CANVIEW_ESP_IMAGE_CUSTOM_BYTES];
} canview_esp_image_info_t;

/** @brief SDK로 전체 file hash와 native RSA Secure Boot V2 서명을 검사한다.
 * @param partition BSP가 제공하는 실제 내부 Flash app 또는 bundle_stage partition.
 * @param offset partition 안의 image 시작. 64KiB 정렬이며 웹 입력 주소가 아니다.
 * @param size 서명된 native file 전체 길이. signature sector 포함, 최대4MiB.
 * @param digest 인증된 manifest의 전체 file SHA-256. 호출 중 불변, NULL 불가.
 * @param out 성공 시 app/custom descriptor, 실패 시0. 다른 인자와 겹침 금지.
 * @return ESP_OK 또는 실제 SDK 오류. 서명 비활성 설정은 ESP_ERR_NOT_SUPPORTED.
 * @details 동기/읽기 전용이다. caller는 호출 동안 Flash bytes와 인자를 불변으로
 * 보장하고 모든 bootloader_mmap/OTA 검증 호출을 단일 task에서 직렬화해야 한다.
 * ISR/동시 호출/재진입 금지. SDK 내부 hash/mmap resource는 SDK가 소유한다.
 * 주소를 보존하거나 erase/write/boot selector/eFuse를 호출하지 않는다.
 * 성공도 role/board/layout/sequence/floor 또는 설치 승인 판정은 아니다.
 */
esp_err_t canview_esp_image_verify(const esp_partition_t *partition, uint32_t offset,
    uint32_t size, const uint8_t digest[CANVIEW_ESP_IMAGE_DIGEST_BYTES], canview_esp_image_info_t *out);
#endif
