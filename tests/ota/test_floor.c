/* SPDX-License-Identifier: GPL-3.0-only */
/** @file test_floor.c @brief 버전 하한/동일 버전 복구의 순수 C 경계 시험. 실물 증거 아님. */
#include <stdio.h>
#include <string.h>
#include "floor.h"

#define CHECK(condition) do { if (!(condition)) { \
    (void)fprintf(stderr, "floor check failed at line %d\n", __LINE__); return 1; } } while (0)

typedef struct
{
    canview_ota_manifest_t manifest;
    canview_ota_floor_t floor;
} test_fixture_t;

static test_fixture_t test_fixture(canview_ota_target_t target)
{
    test_fixture_t data = {0};
    const canview_ota_role_t role = target == CANVIEW_OTA_TARGET_CONTROLLER ? CANVIEW_OTA_ROLE_CONTROLLER :
        (target == CANVIEW_OTA_TARGET_BRIDGE ? CANVIEW_OTA_ROLE_BRIDGE : CANVIEW_OTA_ROLE_COMMUNICATOR);
    data.manifest.role = role;
    (void)memcpy(data.manifest.board_revision, "test-board", sizeof("test-board"));
    (void)memcpy(data.manifest.layout_id, "test-layout", sizeof("test-layout"));
    data.manifest.security_epoch = 7U;
    data.manifest.image_count = 1U;
    data.manifest.images[0].target = target;
    data.manifest.images[0].sha256[0] = 0xA5U;
    data.floor.ready = true;
    data.floor.identity.role = role;
    data.floor.identity.security_epoch = 7U;
    (void)memcpy(data.floor.identity.board_revision, data.manifest.board_revision, CANVIEW_OTA_TEXT_BYTES);
    (void)memcpy(data.floor.identity.layout_id, data.manifest.layout_id, CANVIEW_OTA_TEXT_BYTES);
    data.floor.count = 1U;
    data.floor.records[0].target = target;
    data.floor.records[0].confirmed_digest[0] = 0xA5U;
    data.floor.records[0].observed_digest[0] = 0xA5U;
    data.floor.records[0].installed = CANVIEW_OTA_INSTALLED_BOOTABLE;
    return data;
}

static bool test_result(const test_fixture_t *data, canview_status_t expected,
    canview_ota_floor_action_t first, canview_ota_floor_action_t second, uint32_t *cases)
{
    uint8_t before[sizeof(*data)];
    canview_ota_floor_result_t result;
    (void)memcpy(before, data, sizeof(before));
    (void)memset(&result, 0xA5, sizeof(result));
    const canview_status_t actual = canview_ota_floor_check(&data->manifest, &data->floor, &result);
    ++*cases;
    return actual == expected && result.images[0] == first && result.images[1] == second &&
        memcmp(before, data, sizeof(before)) == 0;
}

#define REJECT(data, status) CHECK(test_result(&(data), (status), \
    CANVIEW_OTA_FLOOR_UNCHECKED, CANVIEW_OTA_FLOOR_UNCHECKED, &cases))
#define ACCEPT(data, action) CHECK(test_result(&(data), CANVIEW_OK, \
    (action), CANVIEW_OTA_FLOOR_UNCHECKED, &cases))

int main(void)
{
    static const uint64_t sequences[] = {0U, 1U, 2U, UINT32_MAX, (uint64_t)UINT32_MAX + 1U,
        UINT64_C(9007199254740991), UINT64_C(9007199254740992), UINT64_C(9007199254740993),
        UINT64_C(9223372036854775807), UINT64_C(9223372036854775808), UINT64_MAX - 1U, UINT64_MAX};
    uint32_t cases = 0U;
    canview_ota_floor_result_t out;
    test_fixture_t base = test_fixture(CANVIEW_OTA_TARGET_COMM_ESP);
    CHECK(canview_ota_floor_check(NULL, NULL, NULL) == CANVIEW_INVALID_ARGUMENT);
    for (uint32_t which = 0U; which < 2U; ++which)
    {
        (void)memset(&out, 0xA5, sizeof(out));
        CHECK(canview_ota_floor_check(which == 0U ? NULL : &base.manifest,
            which == 1U ? NULL : &base.floor, &out) == CANVIEW_INVALID_ARGUMENT);
        CHECK(out.images[0] == CANVIEW_OTA_FLOOR_UNCHECKED && out.images[1] == CANVIEW_OTA_FLOOR_UNCHECKED);
    }
    for (uint32_t target = 1U; target <= 4U; ++target)
    {
        base = test_fixture((canview_ota_target_t)target);
        for (size_t low = 0U; low < sizeof(sequences) / sizeof(sequences[0]); ++low)
        {
            for (size_t candidate = 0U; candidate < sizeof(sequences) / sizeof(sequences[0]); ++candidate)
            {
                test_fixture_t data = base;
                data.floor.records[0].minimum_sequence = sequences[low];
                data.floor.records[0].observed_sequence = sequences[low];
                data.manifest.images[0].release_sequence = sequences[candidate];
                if (sequences[candidate] < sequences[low]) { REJECT(data, CANVIEW_STALE); }
                else if (sequences[candidate] > sequences[low]) { ACCEPT(data, CANVIEW_OTA_FLOOR_UPGRADE); }
                else
                {
                    ACCEPT(data, CANVIEW_OTA_FLOOR_ALREADY_INSTALLED);
                    data.floor.records[0].installed = CANVIEW_OTA_INSTALLED_DAMAGED;
                    ACCEPT(data, CANVIEW_OTA_FLOOR_REPAIR_REQUIRED);
                    data.floor.records[0].installed = CANVIEW_OTA_INSTALLED_UNKNOWN;
                    REJECT(data, CANVIEW_INCOMPLETE);
                }
            }
        }
        for (uint32_t byte = 0U; byte < CANVIEW_OTA_DIGEST_BYTES; ++byte)
        {
            for (uint32_t bit = 0U; bit < 8U; ++bit)
            {
                test_fixture_t data = base;
                data.manifest.images[0].sha256[byte] ^= (uint8_t)(1U << bit);
                REJECT(data, CANVIEW_AUTH_FAILED);
                data.floor.records[0].installed = CANVIEW_OTA_INSTALLED_DAMAGED;
                REJECT(data, CANVIEW_AUTH_FAILED);
                data = base;
                data.floor.records[0].observed_digest[byte] ^= (uint8_t)(1U << bit);
                REJECT(data, CANVIEW_INCOMPLETE);
            }
        }
        for (uint32_t mutation = 0U; mutation < 23U; ++mutation)
        {
            test_fixture_t data = base;
            canview_status_t expected = CANVIEW_MALFORMED;
            switch (mutation)
            {
                case 0U: data.floor.ready = false; expected = CANVIEW_INCOMPLETE; break;
                case 1U: data.floor = (canview_ota_floor_t){0}; expected = CANVIEW_INCOMPLETE; break;
                case 2U: data.floor.count = 0U; expected = CANVIEW_INCOMPLETE; break;
                case 3U: data.floor.count = UINT32_MAX; break;
                case 4U: data.manifest.image_count = 0U; break;
                case 5U: data.manifest.image_count = UINT32_MAX; break;
                case 6U: data.floor.records[0].target = (canview_ota_target_t)0; break;
                case 7U: data.manifest.images[0].target = (canview_ota_target_t)5; break;
                case 8U: data.floor.records[0].installed = (canview_ota_installed_state_t)3; break;
                case 9U: data.floor.records[0].installed = (canview_ota_installed_state_t)-1; break;
                case 10U: data.floor.count = 2U; data.floor.records[1] = data.floor.records[0]; break;
                case 11U: data.manifest.image_count = 2U; data.manifest.images[1] = data.manifest.images[0]; break;
                case 12U: data.floor.identity.security_epoch++; expected = CANVIEW_AUTH_FAILED; break;
                case 13U: data.floor.identity.role = (canview_ota_role_t)0; expected = CANVIEW_AUTH_FAILED; break;
                case 14U: data.floor.identity.board_revision[0] = 'x'; expected = CANVIEW_AUTH_FAILED; break;
                case 15U: data.floor.identity.layout_id[0] = 'x'; expected = CANVIEW_AUTH_FAILED; break;
                case 16U: data.floor.records[0].observed_sequence = UINT64_MAX; expected = CANVIEW_INCOMPLETE; break;
                case 17U:
                    (void)memset(data.manifest.board_revision, 'a', CANVIEW_OTA_TEXT_BYTES);
                    (void)memset(data.floor.identity.board_revision, 'a', CANVIEW_OTA_TEXT_BYTES);
                    expected = CANVIEW_AUTH_FAILED; break;
                case 18U: data.manifest.board_revision[0] = '\0'; data.floor.identity.board_revision[0] = '\0';
                    expected = CANVIEW_AUTH_FAILED; break;
                case 19U: data.manifest.board_revision[0] = '\x1F'; data.floor.identity.board_revision[0] = '\x1F';
                    expected = CANVIEW_AUTH_FAILED; break;
                case 20U: data.manifest.board_revision[0] = '\x7F'; data.floor.identity.board_revision[0] = '\x7F';
                    expected = CANVIEW_AUTH_FAILED; break;
                case 21U: data.manifest.role = (canview_ota_role_t)0; data.floor.identity.role = (canview_ota_role_t)0; break;
                default: data.floor.records[0].target = (canview_ota_target_t)-1; break;
            }
            REJECT(data, expected);
        }
        test_fixture_t data = base;
        data.manifest.images[0].release_sequence = 1U;
        data.floor.records[0].installed = CANVIEW_OTA_INSTALLED_UNKNOWN;
        ACCEPT(data, CANVIEW_OTA_FLOOR_UPGRADE);
        data.floor.identity.key_id = UINT32_MAX;
        ACCEPT(data, CANVIEW_OTA_FLOOR_UPGRADE);
    }
    /* 두 MCU의 기록 순서와 image 순서는 독립. 뒤 target 실패도 앞 판정을 지운다. */
    base = test_fixture(CANVIEW_OTA_TARGET_COMM_ESP);
    base.manifest.image_count = 2U;
    base.manifest.images[1] = base.manifest.images[0];
    base.manifest.images[1].target = CANVIEW_OTA_TARGET_COMM_STM;
    REJECT(base, CANVIEW_INCOMPLETE);
    base.floor.count = 2U;
    base.floor.records[1] = base.floor.records[0];
    base.floor.records[0].target = CANVIEW_OTA_TARGET_COMM_STM;
    base.floor.records[0].installed = CANVIEW_OTA_INSTALLED_DAMAGED;
    CHECK(test_result(&base, CANVIEW_OK, CANVIEW_OTA_FLOOR_ALREADY_INSTALLED,
        CANVIEW_OTA_FLOOR_REPAIR_REQUIRED, &cases));
    base.manifest.images[1].sha256[31] ^= 1U;
    REJECT(base, CANVIEW_AUTH_FAILED);
    return printf("OTA floor cases: %u; no Flash/boot mutation\n", (unsigned int)cases) < 0 ? 1 : 0;
}
