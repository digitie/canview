/* SPDX-License-Identifier: GPL-3.0-only */
/* 단일 process/단일 owner host Flash 모형. 물리 ECC/erase stall 증거가 아니다. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "canview_boot_flash.h"
#include "canview_boot_identity.h"
#include "flash_layout.h"
#include "flash_map_backend/flash_map_backend.h"
#include "bootutil/bootutil.h"
#include "bootutil/bootutil_public.h"
#include "bootutil/boot_hooks.h"
#include "bootutil/sign_key.h"
#include "mbedtls/platform.h"
#include "tinycrypt/ecc_platform_specific.h"

/* GCC 호환 변환이 FIH 저장 객체의 volatile을 제거하지 않았음을 컴파일로 검사한다. */
_Static_assert(_Generic(&FIH_SUCCESS, volatile int *: 1, default: 0), "FIH global remains volatile");
_Static_assert(_Generic(&((fih_int *)0)->val, volatile int *: 1, default: 0), "FIH value remains volatile");
_Static_assert(_Generic(&((fih_int *)0)->msk, volatile int *: 1, default: 0), "FIH mask remains volatile");
_Static_assert(sizeof(fih_int) == sizeof(canview_fih_result), "FIH return ABI size");

#define PUBLIC_KEY_BYTES 91U
typedef struct
{
    uint8_t bytes[CANVIEW_STM_FLASH_BYTES];
    uint8_t programmed[CANVIEW_STM_FLASH_BYTES / CANVIEW_STM_FLASH_WRITE_BYTES];
    uint8_t public_key[PUBLIC_KEY_BYTES];
    uint32_t writes;
    uint32_t erases;
    uint32_t duplicate_writes;
    uint32_t progress;
} flash_model_t;
static flash_model_t model;
static flash_model_t baseline;
static uint32_t fail_read_length;
static uint32_t fail_read_address;
static int identity_fault;
static struct
{
    jmp_buf reset;
    uint32_t remaining;
    uint32_t events;
    int armed;
} interruption;
static const unsigned int public_key_length = PUBLIC_KEY_BYTES;
/* 제품 기본값이 아니다. 실제 loader는 별도 BSP/provisioning 구현이 필요하다. */
canview_status_t canview_boot_identity_read(canview_ota_identity_t *identity, uint32_t *abi)
{
    static const canview_ota_identity_t synthetic = {
        CANVIEW_OTA_ROLE_COMMUNICATOR, "synthetic-board", "synthetic-layout", 1U, 1U};
    if (identity == NULL || abi == NULL) { return CANVIEW_INVALID_ARGUMENT; }
    if (identity_fault == 1) { return CANVIEW_AUTH_FAILED; }
    *identity = synthetic;
    if (identity_fault == 2) { identity->role = CANVIEW_OTA_ROLE_BRIDGE; }
    *abi = 2U;
    return CANVIEW_OK;
}
/* upstream key ABI. 시작 시 public DER만 로드하고 boot_go 실행 중 불변이다. */
const struct bootutil_key bootutil_keys[] = {{model.public_key, &public_key_length}};
const int bootutil_key_cnt = 1;

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "check failed %s:%d: %s\n", __FILE__, __LINE__, #condition); return 1; } } while (0)

void canview_boot_progress(void)
{
    ++model.progress; /* 호출 횟수만 센다. 실제 watchdog/시간 검증이 아니다. */
}

static void mutation_boundary(void)
{
    ++interruption.events;
    if (interruption.armed != 0)
    {
        if (interruption.remaining == 0U)
        {
            interruption.armed = 0;
            longjmp(interruption.reset, 1);
        }
        --interruption.remaining;
    }
}

static int range_valid(uint32_t address, uint32_t length)
{
    return address >= CANVIEW_STM_PRIMARY_ADDRESS && address <= CANVIEW_STM_POLICY_A_ADDRESS &&
        length != 0U && length <= CANVIEW_STM_POLICY_A_ADDRESS - address;
}

int canview_boot_flash_read(uint32_t address, void *destination, uint32_t length)
{
    if (destination == NULL || !range_valid(address, length) || length == fail_read_length ||
        address == fail_read_address)
    {
        return -1;
    }
    memcpy(destination, &model.bytes[address - CANVIEW_STM_FLASH_BASE], length);
    return 0;
}

int canview_boot_flash_write(uint32_t address, const void *source, uint32_t length)
{
    uint32_t offset;
    uint32_t index;
    if (source == NULL || !range_valid(address, length) || address % 8U != 0U || length % 8U != 0U)
    {
        return -1;
    }
    offset = address - CANVIEW_STM_FLASH_BASE;
    for (index = offset / 8U; index < (offset + length) / 8U; ++index)
    {
        if (model.programmed[index] != 0U)
        {
            ++model.duplicate_writes;
            return -1;
        }
    }
    mutation_boundary();
    memset(&model.programmed[offset / 8U], 1, length / 8U);
    memcpy(&model.bytes[offset], source, length);
    ++model.writes;
    mutation_boundary();
    return 0;
}

int canview_boot_flash_erase(uint32_t address, uint32_t length)
{
    uint32_t offset;
    if (!range_valid(address, length) || address % 2048U != 0U || length % 2048U != 0U)
    {
        return -1;
    }
    offset = address - CANVIEW_STM_FLASH_BASE;
    mutation_boundary();
    memset(&model.bytes[offset], UINT8_MAX, length);
    memset(&model.programmed[offset / 8U], 0, length / 8U);
    ++model.erases;
    mutation_boundary();
    return 0;
}

static int load_file(const char *path, uint8_t *destination, size_t capacity, size_t *length)
{
    FILE *file = NULL;
    int extra;
    int error;
#if defined(_MSC_VER)
    if (fopen_s(&file, path, "rb") != 0)
    {
        return -1;
    }
#else
    file = fopen(path, "rb");
#endif
    if (file == NULL)
    {
        return -1;
    }
    *length = fread(destination, 1U, capacity, file);
    extra = fgetc(file);
    error = ferror(file);
    if (fclose(file) != 0 || error != 0 || extra != EOF)
    {
        return -1;
    }
    return 0;
}

static int load_image(const char *path, uint32_t address)
{
    size_t length;
    uint32_t offset = address - CANVIEW_STM_FLASH_BASE;
    if (load_file(path, &model.bytes[offset], CANVIEW_STM_SIGNED_IMAGE_MAX_BYTES, &length) != 0)
    {
        return -1;
    }
    memset(&model.programmed[offset / 8U], 1, (length + 7U) / 8U);
    return 0;
}

static int boot_version(int major)
{
    struct boot_rsp response = {0};
    FIH_DECLARE(result, FIH_FAILURE);
    _Static_assert(_Generic(&result, volatile int *: 1, default: 0), "FIH local remains volatile");
    FIH_CALL(boot_go, result, &response);
    if (major < 0)
    {
        CHECK(FIH_NOT_EQ(result, FIH_SUCCESS));
        return 0;
    }
    CHECK(FIH_EQ(result, FIH_SUCCESS));
    CHECK(response.br_hdr != NULL);
    CHECK(major == 0 ? (response.br_hdr->ih_ver.iv_major == 1 || response.br_hdr->ih_ver.iv_major == 2) :
        response.br_hdr->ih_ver.iv_major == major);
    CHECK(response.br_hdr->ih_hdr_size == CANVIEW_STM_IMAGE_HEADER_BYTES);
    CHECK(response.br_flash_dev_id == 0U);
    CHECK(response.br_image_off == CANVIEW_STM_PRIMARY_ADDRESS - CANVIEW_STM_FLASH_BASE);
    return 0;
}

static int cut_sweep(int reverting)
{
    uint32_t cut;
    uint32_t total;
    CHECK(boot_set_confirmed() == 0);
    CHECK(boot_set_pending(0) == 0);
    if (reverting != 0)
    {
        CHECK(boot_version(2) == 0);
    }
    baseline = model;
    interruption.events = 0U;
    CHECK(boot_version(reverting != 0 ? 1 : 2) == 0);
    total = interruption.events;
    CHECK(total > 0U);
    for (cut = 0U; cut < total; ++cut)
    {
        model = baseline;
        interruption.events = 0U;
        interruption.remaining = cut;
        interruption.armed = 1;
        if (setjmp(interruption.reset) == 0)
        {
            CHECK(boot_version(reverting != 0 ? 1 : 2) == 0);
            CHECK(0); /* 모든 선택 경계는 실제로 중단되어야 한다. */
        }
        /* reset은 RAM CFI 상태도 초기화한다. Flash/프로그램 이력만 보존한다. */
        _fih_cfi_ctr = fih_int_encode(0);
        CHECK(interruption.events == cut + 1U);
        CHECK(boot_version(reverting != 0 ? 1 : 0) == 0);
        CHECK(boot_version(1) == 0);
        CHECK(model.duplicate_writes == 0U);
    }
    printf("deterministic API pre/post cut points=%lu PASS\n", (unsigned long)total);
    return 0;
}

static int adapter_tests(void)
{
    FIH_DECLARE(hook_result, FIH_FAILURE);
    const struct flash_area *area = NULL;
    struct flash_area forged;
    struct flash_sector sectors[97];
    uint8_t data[16] = {0};
    uintptr_t base = 1U;
    uint32_t count = 95U;
    uint32_t index;
    FIH_CALL(boot_image_check_hook, hook_result, -1, 0);
    CHECK(FIH_EQ(hook_result, FIH_FAILURE));
    FIH_CALL(boot_image_check_hook, hook_result, 1, 0);
    CHECK(FIH_EQ(hook_result, FIH_FAILURE));
    FIH_CALL(boot_image_check_hook, hook_result, 0, -1);
    CHECK(FIH_EQ(hook_result, FIH_FAILURE));
    FIH_CALL(boot_image_check_hook, hook_result, 0, 2);
    CHECK(FIH_EQ(hook_result, FIH_FAILURE));
    CHECK(flash_device_base(1U, &base) != 0 && base == 1U);
    CHECK(flash_device_base(0U, NULL) != 0);
    CHECK(flash_device_base(0U, &base) == 0 && base == CANVIEW_STM_FLASH_BASE);
    for (index = 0U; index <= UINT8_MAX; ++index)
    {
        if (index != 1U && index != 2U)
        {
            CHECK(flash_area_open((uint8_t)index, &area) != 0 && area == NULL);
        }
    }
    CHECK(flash_area_open(1U, NULL) != 0);
    CHECK(flash_area_open(1U, &area) == 0);
    forged = *area;
    CHECK(flash_area_write(&forged, 0U, data, 8U) != 0);
    CHECK(flash_area_write(area, 0U, NULL, 8U) != 0);
    CHECK(flash_area_read(NULL, 0U, data, 8U) != 0);
    CHECK(flash_area_read(area, 0U, NULL, 8U) != 0);
    CHECK(flash_area_read(area, 0U, data, 0U) != 0);
    CHECK(flash_area_read(area, UINT32_MAX, data, 8U) != 0);
    CHECK(flash_area_read(area, area->fa_size, data, 1U) != 0);
    CHECK(flash_area_write(area, 1U, data, 8U) != 0);
    CHECK(flash_area_write(area, 0U, data, 7U) != 0);
    CHECK(flash_area_erase(area, 0U, 2047U) != 0);
    CHECK(flash_area_erase(area, 1U, 2048U) != 0);
    CHECK(flash_area_get_sectors(1, &count, sectors) != 0 && count == 95U);
    count = 97U;
    CHECK(flash_area_get_sectors(-1, &count, sectors) != 0);
    CHECK(flash_area_get_sectors(256, &count, sectors) != 0);
    CHECK(flash_area_get_sectors(0, &count, sectors) != 0);
    CHECK(flash_area_get_sectors(1, NULL, sectors) != 0);
    CHECK(flash_area_get_sectors(1, &count, NULL) != 0);
    CHECK(count == 97U);
    CHECK(flash_area_get_sectors(2, &count, sectors) == 0 && count == 97U);
    CHECK(sectors[96].fs_off == 96U * 2048U && sectors[96].fs_size == 2048U);
    CHECK(flash_area_get_sector(area, area->fa_size, &sectors[0]) != 0);
    CHECK(flash_area_get_sector(&forged, 0U, &sectors[0]) != 0);
    CHECK(flash_area_get_sector(area, 0U, NULL) != 0);
    CHECK(flash_area_get_sector(area, area->fa_size - 1U, &sectors[0]) == 0);
    CHECK(sectors[0].fs_off == 95U * 2048U);
    CHECK(flash_area_id_from_multi_image_slot(1, 0) == -1);
    CHECK(flash_area_id_from_multi_image_slot(0, -1) == -1);
    CHECK(flash_area_id_from_image_slot(0) == 1);
    CHECK(flash_area_id_to_multi_image_slot(0, 2) == 1);
    CHECK(flash_area_id_to_multi_image_slot(0, 1) == 0);
    CHECK(flash_area_id_to_multi_image_slot(0, 0) == -1);
    CHECK(flash_area_id_to_multi_image_slot(1, 2) == -1);
    CHECK(flash_area_align(&forged) == 0U);
    CHECK(model.writes == 0U && model.erases == 0U);
    CHECK(flash_area_write(area, 0U, data, 8U) == 0);
    CHECK(flash_area_write(area, 0U, data, 8U) != 0);
    CHECK(flash_area_erase(area, 0U, 2048U) == 0);
    memset(data, UINT8_MAX, sizeof(data));
    CHECK(flash_area_write(area, 0U, data, 8U) == 0);
    CHECK(flash_area_write(area, 0U, data, 8U) != 0); /* 모두 ff라도 ECC 재프로그램 금지 */
    return 0;
}

int main(int argc, char **argv)
{
    size_t key_length;
    uint32_t index;
    CHECK(argc == 5);
    CHECK(mbedtls_calloc(1U, 32U) == NULL);
    mbedtls_free(NULL);
    CHECK(default_CSPRNG(NULL, 0U) == 0);
    memset(model.bytes, UINT8_MAX, sizeof(model.bytes));
    CHECK(adapter_tests() == 0);
    memset(&model, 0, sizeof(model));
    memset(model.bytes, UINT8_MAX, sizeof(model.bytes));
    CHECK(load_file(argv[2], model.public_key, sizeof(model.public_key), &key_length) == 0);
    CHECK(key_length == PUBLIC_KEY_BYTES);
    CHECK(load_image(argv[3], CANVIEW_STM_PRIMARY_ADDRESS) == 0);
    CHECK(load_image(argv[4], CANVIEW_STM_SECONDARY_IMAGE) == 0);
    if (strcmp(argv[1], "identity-missing") == 0) { identity_fault = 1; }
    if (strcmp(argv[1], "identity-role") == 0) { identity_fault = 2; }
    if (strcmp(argv[1], "read-header") == 0) { fail_read_length = 512U; }
    if (strcmp(argv[1], "read-metadata") == 0) { fail_read_length = 176U; }
    if (strcmp(argv[1], "read-tlv") == 0) { fail_read_length = 4U; }
    if (strcmp(argv[1], "read-tlv-body") == 0)
    {
        fail_read_address = CANVIEW_STM_PRIMARY_ADDRESS + 512U + 8192U + 176U + 4U;
    }
    if (strcmp(argv[1], "reject") == 0 || identity_fault != 0 || fail_read_length != 0U ||
        fail_read_address != 0U)
    {
        FIH_DECLARE(profile_result, FIH_FAILURE);
        FIH_CALL(boot_image_check_hook, profile_result, 0, 0);
        /* native signature 실패 case는 여기서 REGULAR가 정상이다. SUCCESS는 절대 금지. */
        CHECK(FIH_NOT_EQ(profile_result, FIH_SUCCESS));
        CHECK(boot_version(-1) == 0);
    }
    else if (strcmp(argv[1], "boot") == 0)
    {
        CHECK(boot_version(1) == 0);
    }
    else if (strcmp(argv[1], "swap") == 0 || strcmp(argv[1], "confirm") == 0)
    {
        CHECK(boot_set_confirmed() == 0);
        CHECK(boot_set_pending(0) == 0);
        CHECK(boot_version(2) == 0);
        if (strcmp(argv[1], "confirm") == 0)
        {
            CHECK(boot_set_confirmed() == 0);
            CHECK(boot_version(2) == 0);
        }
        else
        {
            CHECK(boot_version(1) == 0);
        }
    }
    else if (strcmp(argv[1], "cuts") == 0 || strcmp(argv[1], "cuts-revert") == 0)
    {
        CHECK(cut_sweep(strcmp(argv[1], "cuts-revert") == 0) == 0);
    }
    else if (strcmp(argv[1], "bad-secondary") == 0)
    {
        CHECK(boot_set_confirmed() == 0);
        CHECK(boot_set_pending(0) == 0);
        CHECK(boot_version(1) == 0);
    }
    else
    {
        CHECK(0);
    }
    CHECK(model.duplicate_writes == 0U);
    for (index = 0U; index < CANVIEW_STM_BOOT_BYTES; ++index)
    {
        CHECK(model.bytes[index] == UINT8_MAX);
    }
    for (index = CANVIEW_STM_POLICY_A_ADDRESS - CANVIEW_STM_FLASH_BASE;
        index < CANVIEW_STM_FLASH_BYTES; ++index)
    {
        CHECK(model.bytes[index] == UINT8_MAX);
    }
    printf("MCUboot C model %s PASS writes=%lu erases=%lu; physical/HIL NOT_RUN\n",
        argv[1], (unsigned long)model.writes, (unsigned long)model.erases);
    return 0;
}
