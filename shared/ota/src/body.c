/* SPDX-License-Identifier: GPL-3.0-only */
/** @file body.c @brief SDK hash를 사용하는 순차 이미지 검사. I/O·Flash 권한 없음. */
#include <string.h>
#include "body.h"

static canview_status_t body_close_hash(canview_ota_body_t *body)
{
    if (body->hash_live)
    {
        const canview_status_t status = body->hash.reset(body->hash.context);
        body->cleanup_error = status;
        if (status != CANVIEW_OK)
        {
            return status;
        }
        body->hash_live = false;
    }
    return CANVIEW_OK;
}

static canview_status_t body_mark_failed(canview_ota_body_t *body, canview_status_t error)
{
    body->state = CANVIEW_OTA_BODY_FAILED;
    body->error = error;
    (void)memset(&body->manifest, 0, sizeof(body->manifest));
    body->image_index = 0U;
    body->image_bytes = 0U;
    body->next_offset = 0U;
    return error;
}

static canview_status_t body_fail(canview_ota_body_t *body, canview_status_t error)
{
    body->cleanup_error = body_close_hash(body);
    return body_mark_failed(body, error);
}

static canview_status_t body_start_hash(canview_ota_body_t *body)
{
    /* start 내부 partial initialization도 reset 대상이다. */
    body->hash_live = true;
    return body->hash.start(body->hash.context);
}

canview_status_t canview_ota_body_open(
    canview_ota_body_t *body, const uint8_t *prefix, size_t size,
    const canview_ota_identity_t *identity, canview_ota_manifest_verify_fn verify,
    void *verify_context, const canview_ota_hash_t *hash)
{
    canview_status_t status;
    if (body == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (body->busy || body->state != CANVIEW_OTA_BODY_EMPTY)
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    if (hash == NULL || hash->context == NULL || hash->start == NULL ||
        hash->update == NULL || hash->finish == NULL || hash->reset == NULL)
    {
        return body_mark_failed(body, CANVIEW_INVALID_ARGUMENT);
    }
    body->busy = true;
    status = canview_ota_manifest_check(prefix, size, identity, verify, verify_context, &body->manifest);
    if (status == CANVIEW_OK)
    {
        body->hash = *hash;
        body->next_offset = body->manifest.images[0].offset;
        status = body_start_hash(body);
    }
    if (status == CANVIEW_OK)
    {
        body->state = CANVIEW_OTA_BODY_RECEIVING;
    }
    else
    {
        status = body_fail(body, status);
    }
    body->busy = false;
    return status;
}

static canview_status_t body_image_finish(canview_ota_body_t *body)
{
    uint8_t digest[CANVIEW_OTA_DIGEST_BYTES] = {0};
    canview_status_t status = body->hash.finish(body->hash.context, digest);
    if (status != CANVIEW_OK)
    {
        return body_fail(body, status);
    }
    if (memcmp(digest, body->manifest.images[body->image_index].sha256, sizeof(digest)) != 0)
    {
        return body_fail(body, CANVIEW_AUTH_FAILED);
    }
    status = body_close_hash(body);
    if (status != CANVIEW_OK)
    {
        return body_mark_failed(body, status);
    }
    ++body->image_index;
    body->image_bytes = 0U;
    if (body->image_index == body->manifest.image_count)
    {
        body->state = CANVIEW_OTA_BODY_HASHES_MATCHED;
        return CANVIEW_OK;
    }
    status = body_start_hash(body);
    return status == CANVIEW_OK ? CANVIEW_OK : body_fail(body, status);
}

static canview_status_t body_consume(canview_ota_body_t *body, uint32_t offset,
                                    const uint8_t *data, size_t size)
{
    if (body->state == CANVIEW_OTA_BODY_FAILED)
    {
        return body->error;
    }
    if (body->state != CANVIEW_OTA_BODY_RECEIVING && body->state != CANVIEW_OTA_BODY_HASHES_MATCHED)
    {
        return body_fail(body, CANVIEW_STALE);
    }
    if (data == NULL && size != 0U)
    {
        return body_fail(body, CANVIEW_INVALID_ARGUMENT);
    }
    if (size > CANVIEW_OTA_BODY_CHUNK_MAX)
    {
        return body_fail(body, CANVIEW_OVERSIZE);
    }
    if (offset != body->next_offset)
    {
        return body_fail(body, offset < body->next_offset ? CANVIEW_DUPLICATE : CANVIEW_MALFORMED);
    }
    if (size > body->manifest.total_size - body->next_offset)
    {
        return body_fail(body, CANVIEW_OVERSIZE);
    }
    size_t used = 0U;
    while (used < size)
    {
        const uint32_t remaining = body->manifest.images[body->image_index].length - body->image_bytes;
        const size_t count = size - used < remaining ? size - used : remaining;
        canview_status_t status = body->hash.update(body->hash.context, data + used, count);
        if (status != CANVIEW_OK)
        {
            return body_fail(body, status);
        }
        used += count;
        body->next_offset += (uint32_t)count;
        body->image_bytes += (uint32_t)count;
        if (body->image_bytes == body->manifest.images[body->image_index].length)
        {
            status = body_image_finish(body);
            if (status != CANVIEW_OK)
            {
                return status;
            }
        }
    }
    return CANVIEW_OK;
}

canview_status_t canview_ota_body_feed(canview_ota_body_t *body, uint32_t offset,
                                     const uint8_t *data, size_t size)
{
    if (body == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (body->busy)
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    body->busy = true;
    const canview_status_t status = body_consume(body, offset, data, size);
    body->busy = false;
    return status;
}

canview_status_t canview_ota_body_finish(canview_ota_body_t *body)
{
    canview_status_t status;
    if (body == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (body->busy)
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    body->busy = true;
    if (body->state == CANVIEW_OTA_BODY_HASHES_MATCHED)
    {
        status = CANVIEW_OK;
    }
    else if (body->state == CANVIEW_OTA_BODY_FAILED)
    {
        status = body->error;
    }
    else
    {
        status = body_fail(body, CANVIEW_INCOMPLETE);
    }
    body->busy = false;
    return status;
}

canview_status_t canview_ota_body_reset(canview_ota_body_t *body)
{
    if (body == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (body->busy)
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    body->busy = true;
    const canview_status_t status = body_close_hash(body);
    if (status == CANVIEW_OK)
    {
        *body = (canview_ota_body_t){0};
    }
    else
    {
        const canview_status_t error = body_mark_failed(body, status);
        body->busy = false;
        return error;
    }
    return status;
}
