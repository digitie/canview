/* SPDX-License-Identifier: GPL-3.0-only */
/** @file stage.c @brief Parser와 동기 BSP 수신 저장의 fail-closed 호출 순서. */
#include <string.h>
#include "stage.h"

static bool stage_overlaps(const canview_ota_stage_t *stage, const void *input, size_t size)
{
    const uintptr_t owner = (uintptr_t)stage;
    const uintptr_t address = (uintptr_t)input;
    if (input == NULL || size == 0U) { return false; }
    if (size - 1U > UINTPTR_MAX - address) { return true; }
    return address >= owner ? address - owner < sizeof(*stage) : owner - address < size;
}

static canview_status_t stage_fail(canview_ota_stage_t *stage, canview_status_t error)
{
    if (stage->state != CANVIEW_OTA_STAGE_FAILED) { stage->error = error; }
    stage->state = CANVIEW_OTA_STAGE_FAILED;
    return stage->error;
}

canview_status_t canview_ota_stage_open(canview_ota_stage_t *stage,
    const uint8_t *prefix, size_t size, const canview_ota_identity_t *identity,
    const canview_ota_runtime_t *runtime, const canview_ota_floor_t *floor,
    canview_ota_manifest_verify_fn verify, void *verify_context,
    const canview_ota_hash_t *hash, const canview_ota_storage_t *storage)
{
    if (stage == NULL) { return CANVIEW_INVALID_ARGUMENT; }
    if (stage->busy || stage->state != CANVIEW_OTA_STAGE_EMPTY) { return CANVIEW_RESOURCE_BUSY; }
    if (stage_overlaps(stage, prefix, size) || stage_overlaps(stage, identity, sizeof(*identity)) ||
        stage_overlaps(stage, runtime, sizeof(*runtime)) || stage_overlaps(stage, floor, sizeof(*floor)) ||
        stage_overlaps(stage, hash, sizeof(*hash)) || stage_overlaps(stage, storage, sizeof(*storage)) ||
        stage_overlaps(stage, verify_context, 1U))
    {
        return stage_fail(stage, CANVIEW_INVALID_ARGUMENT);
    }
    if (storage == NULL || storage->context == NULL || storage->begin == NULL ||
        storage->write == NULL || storage->verify == NULL || storage->close == NULL ||
        stage_overlaps(stage, storage->context, 1U) ||
        (hash != NULL && stage_overlaps(stage, hash->context, 1U)))
    {
        return stage_fail(stage, CANVIEW_INVALID_ARGUMENT);
    }
    stage->busy = true;
    canview_status_t status = canview_ota_body_open(&stage->body, prefix, size, identity,
        runtime, floor, verify, verify_context, hash);
    if (status == CANVIEW_OK)
    {
        stage->storage = *storage;
        stage->storage_live = true; /* partial begin 오류도 close 대상이다. */
        status = stage->storage.begin(stage->storage.context, &stage->body.manifest, prefix, size);
    }
    if (status == CANVIEW_OK) { stage->state = CANVIEW_OTA_STAGE_RECEIVING; }
    else { status = stage_fail(stage, status); }
    stage->busy = false;
    return status;
}

canview_status_t canview_ota_stage_feed(canview_ota_stage_t *stage, uint32_t offset,
    const uint8_t *data, size_t size)
{
    if (stage == NULL) { return CANVIEW_INVALID_ARGUMENT; }
    if (stage->busy) { return CANVIEW_RESOURCE_BUSY; }
    if (stage->state == CANVIEW_OTA_STAGE_FAILED) { return stage->error; }
    if (stage->state != CANVIEW_OTA_STAGE_RECEIVING) { return stage_fail(stage, CANVIEW_INVALID_ARGUMENT); }
    if (stage_overlaps(stage, data, size)) { return stage_fail(stage, CANVIEW_INVALID_ARGUMENT); }
    stage->busy = true;
    canview_status_t status = canview_ota_body_feed(&stage->body, offset, data, size);
    if (status == CANVIEW_OK && size != 0U)
    {
        status = stage->storage.write(stage->storage.context, offset, data, size);
    }
    if (status != CANVIEW_OK) { status = stage_fail(stage, status); }
    stage->busy = false;
    return status;
}

canview_status_t canview_ota_stage_finish(canview_ota_stage_t *stage)
{
    if (stage == NULL) { return CANVIEW_INVALID_ARGUMENT; }
    if (stage->busy) { return CANVIEW_RESOURCE_BUSY; }
    if (stage->state == CANVIEW_OTA_STAGE_FAILED) { return stage->error; }
    if (stage->state != CANVIEW_OTA_STAGE_RECEIVING) { return CANVIEW_INVALID_ARGUMENT; }
    stage->busy = true;
    canview_status_t status = canview_ota_body_finish(&stage->body);
    if (status == CANVIEW_OK)
    {
        status = stage->storage.verify(stage->storage.context, &stage->body.manifest);
    }
    if (status == CANVIEW_OK) { stage->state = CANVIEW_OTA_STAGE_NATIVE_MATCHED; }
    else { status = stage_fail(stage, status); }
    stage->busy = false;
    return status;
}

canview_status_t canview_ota_stage_reset(canview_ota_stage_t *stage)
{
    if (stage == NULL) { return CANVIEW_INVALID_ARGUMENT; }
    if (stage->busy) { return CANVIEW_RESOURCE_BUSY; }
    stage->busy = true;
    canview_status_t status = CANVIEW_OK;
    if (stage->storage_live)
    {
        status = stage->storage.close(stage->storage.context);
        if (status == CANVIEW_OK) { stage->storage_live = false; }
    }
    if (status == CANVIEW_OK) { status = canview_ota_body_reset(&stage->body); }
    if (status == CANVIEW_OK) { (void)memset(stage, 0, sizeof(*stage)); }
    else
    {
        (void)stage_fail(stage, status);
        stage->cleanup_error = status;
        stage->busy = false;
    }
    return status;
}
