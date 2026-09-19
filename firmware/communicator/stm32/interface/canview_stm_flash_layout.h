/* SPDX-License-Identifier: GPL-3.0-only */
/** @file canview_stm_flash_layout.h
 * @brief BSP 소유 Flash 배치 조회와 범위 검사. Flash IO/OTA 권한 부여 API가 아니다.
 *
 * heap, callback, mutable state가 없는 재진입 가능 C99 API다. 출력은 성공할 때만
 * 갱신한다. 호출자는 유효한 출력 객체를 소유하며 동시 출력 쓰기를 직렬화한다.
 * program/erase 성공도 profile, 서명, 활성 슬롯 보호, ECC, 단전 복구를 보증하지
 * 않는다. 실제 드라이버는 그 검사를 별도로 완료해야 한다.
 */
#ifndef CANVIEW_STM_FLASH_LAYOUT_H
#define CANVIEW_STM_FLASH_LAYOUT_H

#include <stdint.h>
#include "canview_status.h"

/** @brief 지역 enum. MCUboot flash_area ID나 wire 값을 직접 cast하지 않는다. */
typedef enum
{
    CANVIEW_STM_FLASH_BOOT = 0,
    CANVIEW_STM_FLASH_PRIMARY,
    CANVIEW_STM_FLASH_SECONDARY,
    CANVIEW_STM_FLASH_POLICY_A,
    CANVIEW_STM_FLASH_POLICY_B,
    CANVIEW_STM_FLASH_CONFIG_A,
    CANVIEW_STM_FLASH_CONFIG_B,
    CANVIEW_STM_FLASH_RESERVED,
    CANVIEW_STM_FLASH_REGION_COUNT
} canview_stm_flash_region_t;

/** @brief 범위 검사할 동작. erase는 page 단위, program은 doubleword 단위다. */
typedef enum
{
    CANVIEW_STM_FLASH_READ = 0,
    CANVIEW_STM_FLASH_PROGRAM,
    CANVIEW_STM_FLASH_ERASE
} canview_stm_flash_operation_t;

/** @brief 배치 값의 복사본. 값을 수정해도 BSP 정본은 변경되지 않는다. */
typedef struct
{
    uint32_t address; /**< 절대 시작 주소. */
    uint32_t size; /**< byte 길이. */
} canview_stm_flash_area_t;

/** @brief 고정 영역 조회. null/잘못된 enum은 INVALID_ARGUMENT, 그 외 OK. */
canview_status_t canview_stm_flash_area(canview_stm_flash_region_t region,
    canview_stm_flash_area_t *area);

/** @brief 상대 offset/length를 검사한 뒤 절대 주소를 반환한다.
 * @param region BSP 영역. BOOT/RESERVED의 program/erase는 항상 AUTH_FAILED.
 * @param operation READ/PROGRAM/ERASE 중 하나.
 * @param offset 영역 시작으로부터 byte offset.
 * @param length 0은 거절한다. 영역 밖/overflow는 OVERSIZE, 정렬 오류는 INVALID_ARGUMENT.
 * @param address 성공 시에만 갱신하는 절대 주소. null은 INVALID_ARGUMENT.
 * @return CANVIEW_OK는 기하학적 범위만 유효함을 뜻하며 쓰기 권한이 아니다.
 */
canview_status_t canview_stm_flash_range(canview_stm_flash_region_t region,
    canview_stm_flash_operation_t operation, uint32_t offset, uint32_t length,
    uint32_t *address);

#endif
