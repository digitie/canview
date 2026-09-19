/* SPDX-License-Identifier: GPL-3.0-only */
/** @file psa_crypto_fixture.h @brief PSA 호출/실패 주입 모형. 실제 암호 구현이 아니다. */
#ifndef CANVIEW_PSA_CRYPTO_FIXTURE_H
#define CANVIEW_PSA_CRYPTO_FIXTURE_H
#include <stddef.h>
#include <stdint.h>
typedef int32_t psa_status_t;
typedef uint32_t mbedtls_svc_key_id_t;
typedef uint32_t psa_algorithm_t;
typedef struct { uint32_t active; } psa_hash_operation_t;
typedef struct { uint32_t type; size_t bits; uint32_t usage; psa_algorithm_t algorithm; uint32_t lifetime; } psa_key_attributes_t;
#define PSA_SUCCESS (0)
#define PSA_ERROR_INVALID_SIGNATURE (-149)
#define PSA_ERROR_INVALID_ARGUMENT (-135)
#define PSA_ERROR_NOT_SUPPORTED (-134)
#define PSA_ERROR_HARDWARE_FAILURE (-147)
#define PSA_ERROR_INSUFFICIENT_MEMORY (-141)
#define PSA_ERROR_BAD_STATE (-137)
#define PSA_ALG_SHA_256 (UINT32_C(0x02000009))
#define PSA_ALG_ECDSA(hash) (UINT32_C(0x06000600) | ((hash) & UINT32_C(0xff)))
#define PSA_ECC_FAMILY_SECP_R1 (0x12U)
#define PSA_KEY_TYPE_ECC_PUBLIC_KEY(family) (0x4100U | (family))
#define PSA_KEY_USAGE_VERIFY_MESSAGE (0x1000U)
#define PSA_KEY_LIFETIME_VOLATILE (0U)
#define PSA_KEY_ATTRIBUTES_INIT {0}
static inline int mbedtls_svc_key_id_is_null(mbedtls_svc_key_id_t key) { return key == 0U; }
static inline psa_hash_operation_t psa_hash_operation_init(void) { return (psa_hash_operation_t){0}; }
static inline void psa_set_key_type(psa_key_attributes_t *a, uint32_t value) { a->type = value; }
static inline void psa_set_key_bits(psa_key_attributes_t *a, size_t value) { a->bits = value; }
static inline void psa_set_key_usage_flags(psa_key_attributes_t *a, uint32_t value) { a->usage = value; }
static inline void psa_set_key_algorithm(psa_key_attributes_t *a, psa_algorithm_t value) { a->algorithm = value; }
static inline void psa_set_key_lifetime(psa_key_attributes_t *a, uint32_t value) { a->lifetime = value; }
static inline void psa_reset_key_attributes(psa_key_attributes_t *a) { *a = (psa_key_attributes_t){0}; }
psa_status_t psa_crypto_init(void);
psa_status_t psa_import_key(const psa_key_attributes_t *a, const uint8_t *data, size_t size, mbedtls_svc_key_id_t *key);
psa_status_t psa_destroy_key(mbedtls_svc_key_id_t key);
psa_status_t psa_verify_message(mbedtls_svc_key_id_t key, psa_algorithm_t algorithm,
    const uint8_t *message, size_t size, const uint8_t *signature, size_t signature_size);
psa_status_t psa_hash_setup(psa_hash_operation_t *operation, psa_algorithm_t algorithm);
psa_status_t psa_hash_update(psa_hash_operation_t *operation, const uint8_t *data, size_t size);
psa_status_t psa_hash_finish(psa_hash_operation_t *operation, uint8_t *digest, size_t capacity, size_t *size);
psa_status_t psa_hash_abort(psa_hash_operation_t *operation);
#endif
