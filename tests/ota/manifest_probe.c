/* SPDX-License-Identifier: GPL-3.0-only */
/** @file manifest_probe.c @brief Host typed parser oracle. 서명 mock은 구조 시험에만 사용한다. */
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#endif
#include "manifest.h"

typedef struct
{
    const uint8_t *prefix;
    size_t size;
    const canview_ota_identity_t *identity;
    uint32_t calls;
    canview_status_t result;
    bool reenter;
} probe_context_t;

static canview_status_t probe_verify(void *context, const uint8_t *message, size_t size,
    const uint8_t signature[CANVIEW_OTA_ENVELOPE_SIGNATURE_BYTES])
{
    if (context != NULL)
    {
        probe_context_t *state = context;
        ++state->calls;
        if (state->reenter)
        {
            canview_ota_manifest_t nested;
            state->reenter = false;
            if (canview_ota_manifest_check(state->prefix, state->size, state->identity,
                                          probe_verify, state, &nested) != CANVIEW_OK)
            {
                return CANVIEW_MALFORMED;
            }
        }
        return state->result;
    }
    return message != NULL && size != 0U && signature != NULL ? CANVIEW_OK : CANVIEW_INVALID_ARGUMENT;
}

static bool probe_zero(const canview_ota_manifest_t *view)
{
    const uint8_t *bytes = (const uint8_t *)view;
    for (size_t index = 0U; index < sizeof(*view); ++index)
    {
        if (bytes[index] != 0U)
        {
            return false;
        }
    }
    return true;
}

int main(int argc, char **argv)
{
    uint8_t data[CANVIEW_OTA_ENVELOPE_PREFIX_MAX + 1U];
    uint8_t size_bytes[4];
    canview_ota_identity_t identity = {CANVIEW_OTA_ROLE_COMMUNICATOR, "synthetic-board", "synthetic-layout", 7U, 11U};
    canview_ota_manifest_t view;
    if (argc != 2)
    {
        return 1;
    }
    if (strcmp(argv[1], "2") == 0)
    {
        identity.role = CANVIEW_OTA_ROLE_CONTROLLER;
    }
    else if (strcmp(argv[1], "3") == 0)
    {
        identity.role = CANVIEW_OTA_ROLE_BRIDGE;
    }
    else if (strcmp(argv[1], "1") != 0)
    {
        return 1;
    }
    /* NULL/oversize/local identity 오류는 입력 읽기 전 실패하고 out을 지워야 한다. */
    (void)memset(&view, 0xA5, sizeof(view));
    if (canview_ota_manifest_check(NULL, 0U, &identity, probe_verify, NULL, NULL) != CANVIEW_INVALID_ARGUMENT ||
        canview_ota_manifest_check(NULL, 0U, &identity, probe_verify, NULL, &view) != CANVIEW_INVALID_ARGUMENT || !probe_zero(&view) ||
        canview_ota_manifest_check(data, SIZE_MAX, &identity, probe_verify, NULL, &view) != CANVIEW_OVERSIZE || !probe_zero(&view) ||
        canview_ota_manifest_check(data, 0U, NULL, probe_verify, NULL, &view) != CANVIEW_INVALID_ARGUMENT || !probe_zero(&view))
    {
        return 1;
    }
    const canview_ota_role_t role = identity.role;
    identity.role = (canview_ota_role_t)0;
    if (canview_ota_manifest_check(data, 0U, &identity, probe_verify, NULL, &view) != CANVIEW_INVALID_ARGUMENT || !probe_zero(&view))
    {
        return 1;
    }
    identity.role = role;
    (void)memset(identity.board_revision, 'A', sizeof(identity.board_revision));
    if (canview_ota_manifest_check(data, 0U, &identity, probe_verify, NULL, &view) != CANVIEW_INVALID_ARGUMENT || !probe_zero(&view))
    {
        return 1;
    }
    (void)memset(identity.board_revision, 0, sizeof(identity.board_revision));
    if (canview_ota_manifest_check(data, 0U, &identity, probe_verify, NULL, &view) != CANVIEW_INVALID_ARGUMENT || !probe_zero(&view))
    {
        return 1;
    }
    (void)memcpy(identity.board_revision, "synthetic-board", sizeof("synthetic-board"));
#if defined(_WIN32)
    if (_setmode(_fileno(stdin), _O_BINARY) < 0)
    {
        return 1;
    }
#endif
    for (;;)
    {
        const size_t count = fread(size_bytes, 1U, sizeof(size_bytes), stdin);
        if (count == 0U && feof(stdin) != 0 && ferror(stdin) == 0)
        {
            return fflush(stdout) == 0 ? 0 : 1;
        }
        if (count != sizeof(size_bytes))
        {
            return 1;
        }
        const uint32_t size = (uint32_t)size_bytes[0] | ((uint32_t)size_bytes[1] << 8U) |
            ((uint32_t)size_bytes[2] << 16U) | ((uint32_t)size_bytes[3] << 24U);
        if (size > sizeof(data) || fread(data, 1U, size, stdin) != size)
        {
            return 1;
        }
        (void)memset(&view, 0xA5, sizeof(view));
        const canview_status_t status = canview_ota_manifest_check(data, size, &identity, probe_verify, NULL, &view);
        if (status != CANVIEW_OK && !probe_zero(&view))
        {
            return 1;
        }
        if (status == CANVIEW_OK)
        {
            canview_ota_manifest_t second;
            probe_context_t state = {data, size, &identity, 0U, CANVIEW_OK, true};
            if (canview_ota_manifest_check(data, size, &identity, probe_verify, &state, &second) != CANVIEW_OK ||
                state.calls != 2U || state.reenter || second.total_size != view.total_size)
            {
                return 1;
            }
            state.result = CANVIEW_AUTH_FAILED;
            if (canview_ota_manifest_check(data, size, &identity, probe_verify, &state, &second) != CANVIEW_AUTH_FAILED ||
                state.calls != 3U || !probe_zero(&second))
            {
                return 1;
            }
        }
        if (printf("%u %u %u", (unsigned int)status, (unsigned int)view.image_count, (unsigned int)view.total_size) < 0)
        {
            return 1;
        }
        for (uint32_t index = 0U; index < view.image_count; ++index)
        {
            const canview_ota_image_t *image = &view.images[index];
            if (printf(" %u %u %u %" PRIu64, (unsigned int)image->target,
                (unsigned int)image->offset, (unsigned int)image->length, image->release_sequence) < 0)
            {
                return 1;
            }
        }
        if (putchar('\n') == EOF)
        {
            return 1;
        }
    }
}
