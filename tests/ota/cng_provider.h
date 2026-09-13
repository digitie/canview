/* SPDX-License-Identifier: GPL-3.0-only */
/** @file cng_provider.h @brief Windows host 시험 전용 실제 암호 provider. */
#ifndef CANVIEW_TEST_CNG_PROVIDER_H
#define CANVIEW_TEST_CNG_PROVIDER_H
#include "body.h"

/** @brief {0} 초기화. native handle은 reset 성공 전 외부에서 변경하지 않는다. */
typedef struct
{
    void *handle;
} canview_test_cng_hash_t;

/** @brief context의 신뢰된 X[32]||Y[32]로 P256/SHA256을 검증한다. */
canview_status_t canview_test_p256_verify(void *context, const uint8_t *message, size_t message_size,
    const uint8_t signature[CANVIEW_OTA_ENVELOPE_SIGNATURE_BYTES]);

/** @brief Windows SDK SHA-256 함수표. context는 body reset까지 유효해야 한다. */
canview_ota_hash_t canview_test_cng_hash_provider(canview_test_cng_hash_t *context);
#endif
