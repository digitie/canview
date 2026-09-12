/* SPDX-License-Identifier: GPL-3.0-only */
/** @file test_envelope.c @brief Portable 경계/실패 전파 시험. 실제 서명 시험은 별도다. */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "envelope.h"

#define CHECK(condition) do { if (!(condition)) { \
    (void)fprintf(stderr, "envelope check failed at line %d\n", __LINE__); \
    return 1; } } while (0)
#define TEST_PREFIX_BYTES (89U)

typedef struct
{
    const uint8_t *prefix;
    size_t calls;
    canview_status_t result;
    bool reenter;
} test_verify_t;

static canview_status_t test_verify(void *context, const uint8_t *message,
                                    size_t size, const uint8_t signature[64])
{
    test_verify_t *state = context;
    if (state == NULL || message == NULL || signature == NULL || size != 1U || message[0] != 0xA0U)
    {
        return CANVIEW_MALFORMED;
    }
    ++state->calls;
    if (state->reenter)
    {
        canview_ota_envelope_t nested;
        state->reenter = false;
        if (canview_ota_envelope_check(state->prefix, TEST_PREFIX_BYTES, test_verify, state, &nested) != CANVIEW_OK ||
            nested.manifest_size != 1U)
        {
            return CANVIEW_MALFORMED;
        }
    }
    return state->result;
}

static bool test_cleared(const canview_ota_envelope_t *view)
{
    return view->manifest_offset == 0U && view->manifest_size == 0U &&
           view->images_offset == 0U && view->declared_total_size == 0U &&
           view->declared_image_count == 0U;
}

int main(void)
{
    uint8_t prefix[TEST_PREFIX_BYTES] = {
        'C', 'V', 'O', 'T', 'A', '0', '0', '1', 1U, 0U, 24U, 0U,
        1U, 0U, 0U, 0U, 64U, 0U, 1U, 0U, 90U, 0U, 0U, 0U, 0xA0U
    };
    test_verify_t state = {prefix, 0U, CANVIEW_OK, false};
    canview_ota_envelope_t view;
    CHECK(canview_ota_envelope_check(prefix, sizeof(prefix), test_verify, &state, NULL) == CANVIEW_INVALID_ARGUMENT);
    (void)memset(&view, 0xA5, sizeof(view));
    CHECK(canview_ota_envelope_check(NULL, 0U, test_verify, &state, &view) == CANVIEW_INVALID_ARGUMENT);
    CHECK(test_cleared(&view));
    CHECK(canview_ota_envelope_check(prefix, sizeof(prefix), NULL, &state, &view) == CANVIEW_INVALID_ARGUMENT);
    CHECK(test_cleared(&view));
    CHECK(canview_ota_envelope_check(prefix, SIZE_MAX, test_verify, &state, &view) == CANVIEW_OVERSIZE);
    CHECK(test_cleared(&view));
    for (size_t size = 0U; size < sizeof(prefix); ++size)
    {
        CHECK(canview_ota_envelope_check(prefix, size, test_verify, &state, &view) == CANVIEW_INCOMPLETE);
        CHECK(test_cleared(&view));
    }
    CHECK(state.calls == 0U);
    CHECK(canview_ota_envelope_check(prefix, sizeof(prefix), test_verify, &state, &view) == CANVIEW_OK);
    CHECK(state.calls == 1U && view.manifest_offset == 24U && view.manifest_size == 1U);
    CHECK(view.images_offset == sizeof(prefix) && view.declared_total_size == 90U && view.declared_image_count == 1U);
    state.result = CANVIEW_AUTH_FAILED;
    CHECK(canview_ota_envelope_check(prefix, sizeof(prefix), test_verify, &state, &view) == CANVIEW_AUTH_FAILED);
    CHECK(state.calls == 2U && test_cleared(&view));
    state.result = CANVIEW_RESOURCE_BUSY;
    CHECK(canview_ota_envelope_check(prefix, sizeof(prefix), test_verify, &state, &view) == CANVIEW_RESOURCE_BUSY);
    CHECK(state.calls == 3U && test_cleared(&view));
    state.result = CANVIEW_OK;
    state.reenter = true;
    CHECK(canview_ota_envelope_check(prefix, sizeof(prefix), test_verify, &state, &view) == CANVIEW_OK);
    CHECK(state.calls == 5U && !state.reenter);
    prefix[24] = 0U;
    CHECK(canview_ota_envelope_check(prefix, sizeof(prefix), test_verify, &state, &view) == CANVIEW_MALFORMED);
    CHECK(state.calls == 5U && test_cleared(&view));
    prefix[24] = 0xA0U;
    CHECK(canview_ota_envelope_check(prefix, sizeof(prefix), test_verify, &state, &view) == CANVIEW_OK);
    CHECK(state.calls == 6U);
    prefix[18] = 2U;
    CHECK(canview_ota_envelope_check(prefix, sizeof(prefix), test_verify, &state, &view) == CANVIEW_MALFORMED);
    CHECK(state.calls == 6U && test_cleared(&view));
    return 0;
}
