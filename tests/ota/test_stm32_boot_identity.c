/* SPDX-License-Identifier: GPL-3.0-only */
/* 제품 BSP를 직접 link한다. 출력은 Python이 독립 입력 DER/상수와 대조한다. */
#include "canview_boot_identity.h"
#include "bootutil/sign_key.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define CHECK(test) do { if (!(test)) { \
    fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #test); return 1; } } while (0)

int main(void)
{
    canview_ota_identity_t identity;
    uint32_t abi = UINT32_MAX;
    (void)memset(&identity, 0xa5, sizeof(identity));
    uint8_t before[sizeof(identity)];
    (void)memcpy(before, &identity, sizeof(before));
    CHECK(canview_boot_identity_read(NULL, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_boot_identity_read(NULL, &abi) == CANVIEW_INVALID_ARGUMENT);
    CHECK(abi == UINT32_MAX);
    CHECK(canview_boot_identity_read(&identity, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(memcmp(before, &identity, sizeof(before)) == 0);
    CHECK(canview_boot_identity_read(&identity, &abi) == CANVIEW_OK);
    CHECK(identity.role == CANVIEW_OTA_ROLE_COMMUNICATOR);
    CHECK(strcmp(identity.board_revision, "comm-r2-n16r8") == 0);
    CHECK(strcmp(identity.layout_id, "communicator-ota-layout-v1") == 0);
    const uint32_t epoch = identity.security_epoch;
    const uint32_t key_id = identity.key_id;
    const uint32_t expected_abi = abi;
    for (uint32_t call = 0U; call < 100U; ++call)
    {
        (void)memset(&identity, 0, sizeof(identity));
        abi = 0U;
        CHECK(canview_boot_identity_read(&identity, &abi) == CANVIEW_OK);
        CHECK(identity.role == CANVIEW_OTA_ROLE_COMMUNICATOR);
        CHECK(strcmp(identity.board_revision, "comm-r2-n16r8") == 0);
        CHECK(strcmp(identity.layout_id, "communicator-ota-layout-v1") == 0);
        CHECK(identity.security_epoch == epoch && identity.key_id == key_id && abi == expected_abi);
    }
    CHECK(bootutil_key_cnt == 1 && bootutil_keys[0].key != NULL && bootutil_keys[0].len != NULL);
    CHECK(*bootutil_keys[0].len == 91U);
    CHECK(printf("%" PRIu32 " %" PRIu32 " %" PRIu32 "\n", epoch, key_id, abi) > 0);
    for (size_t index = 0U; index < *bootutil_keys[0].len; ++index)
    {
        CHECK(printf("%02x", (unsigned int)bootutil_keys[0].key[index]) == 2);
    }
    CHECK(putchar('\n') != EOF);
    return 0;
}
