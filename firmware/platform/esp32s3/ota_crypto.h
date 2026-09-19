/* SPDX-License-Identifier: GPL-3.0-only */
/** @file ota_crypto.h @brief BSP 전용 PSA Crypto manifest/SHA256 provider. */
#ifndef CANVIEW_ESP_OTA_CRYPTO_H
#define CANVIEW_ESP_OTA_CRYPTO_H

#include "body.h"
#if defined(CANVIEW_OTA_PSA_TEST)
#include "psa_crypto_fixture.h"
#else
#include "psa/crypto.h"
#endif

#define CANVIEW_ESP_OTA_PUBLIC_KEY_BYTES (65U)

/** @brief 단일 task 소유 SDK 자원. 처음 {0}, close 성공 전 복사/memset/해제 금지.
 * @details volatile public key 한 개와 SHA256 operation 한 개만 소유한다.
 * busy는 SDK 호출 재진입 방어이며 thread lock이 아니다. ISR/동시 호출 금지.
 * 멤버는 adapter 전용이다. SDK 내부 자원 부족은 오류로 전파하며 직접 heap을 쓰지 않는다.
 */
typedef struct
{
    psa_hash_operation_t hash;
    mbedtls_svc_key_id_t key;
    bool ready;
    bool hash_dirty;
    bool hash_active;
    bool closing;
    bool busy;
} canview_esp_ota_crypto_t;

/** @brief 신뢰된 BSP의 역할별 P256 공개키를 volatile VERIFY_MESSAGE 전용으로 import.
 * @param context {0} 또는 close 성공한 단일 owner storage. NULL 불가.
 * @param public_key 0x04||X[32]||Y[32], 호출 중만 빌리는65B. context와 비중첩.
 * package/Web의 공개키를 전달하지 않는다. 개인키/영속 key/provisioning을 다루지 않는다.
 * @return OK 또는 실제 SDK 오류. 실패 뒤에도 close 성공 전 context를 버리지 않는다.
 */
canview_status_t canview_esp_ota_crypto_init(canview_esp_ota_crypto_t *context,
    const uint8_t public_key[CANVIEW_ESP_OTA_PUBLIC_KEY_BYTES]);

/** @brief hash abort와 public key destroy. 실패하면 handle을 보존하고 재시도한다.
 * @details 시작하면 검증/새 hash를 차단한다. body_reset 완료 후 호출해야 한다.
 * 성공 때만 context가 초기 상태로 돌아간다. {0} 또는 반복 close는 OK다.
 */
canview_status_t canview_esp_ota_crypto_close(canview_esp_ota_crypto_t *context);

/** @brief 기존 manifest verify callback. CBOR1..16KiB와 raw r||s64B를 SDK로 검증한다.
 * @details 인자는 context와 비중첩이며 호출 중만 유효하다. ready/소유권·크기 검사를
 * 거친 뒤 PSA ECDSA(SHA256)를 호출한다. OK는 서명 일치일 뿐 native/설치 승인이 아니다.
 */
canview_status_t canview_esp_ota_manifest_verify(void *context, const uint8_t *message,
    size_t size, const uint8_t signature[CANVIEW_OTA_ENVELOPE_SIGNATURE_BYTES]);

/** @brief body용 함수표. context는 body_reset/crypto_close 성공까지 owner가 보존한다.
 * @details 각 callback이 ready/순서/재진입을 검사한다. NULL context도 실패용 함수표를
 * 반환한다. 입력 chunk는0..16KiB, 출력 digest32B이며 context와 비중첩이다.
 * setup/update/finish 실패 뒤에는 reset 성공 전 새 hash를 시작하지 않는다.
 */
canview_ota_hash_t canview_esp_ota_hash_provider(canview_esp_ota_crypto_t *context);

#endif
