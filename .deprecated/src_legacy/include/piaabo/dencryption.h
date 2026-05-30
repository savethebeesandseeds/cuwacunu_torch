#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/pem.h>
#include "piaabo/dutils.h"

/*----------------------------------------------------------------------------
 * Cryptographic profile constants (OpenSSL EVP backend).
 *--------------------------------------------------------------------------*/
#define AEAD_KEY_LEN 32             /* AES-256 key length (bytes) */
#define AEAD_NONCE_LEN 12           /* AES-GCM nonce length (bytes) */
#define AEAD_TAG_LEN 16             /* AES-GCM authentication tag length (bytes) */
#define AEAD_SALT_LEN 16            /* KDF salt length for AEAD blob (bytes) */
#define AEAD_DEFAULT_PBKDF2_ITERATIONS 210000U /* default PBKDF2 iterations for AEAD */

namespace cuwacunu {
namespace piaabo {
namespace dencryption {
/*----------------------------------------------------------------------------
 * OpenSSL-based encryption/signing helpers.
 *
 * Algorithms and protocols:
 * - KDF: PBKDF2-HMAC-SHA256
 * - Symmetric cipher: AES-256-GCM (versioned AEAD blob)
 * - Asymmetric signature: Ed25519 (RFC 8032 semantics via EVP_PKEY)
 * - Transport encoding: Base64 without newlines (RFC 4648 style)
 *--------------------------------------------------------------------------*/

/* Detect whether `blob` is encoded in the native versioned AEAD envelope
 * format managed by this module.
 */
bool is_aead_blob(const unsigned char* blob, size_t blob_len);

/* Encrypt plaintext into a versioned AEAD blob:
 * - KDF: PBKDF2-HMAC-SHA256(passphrase, salt, iterations)
 * - Cipher: AES-256-GCM(key, nonce)
 * Output buffer is heap-allocated and returned to caller.
 */
unsigned char* aead_encrypt_blob(const unsigned char* plaintext, size_t plaintext_len,
                                 const char* passphrase, size_t& blob_len);

/* Decrypt and authenticate a versioned AEAD blob produced by
 * `aead_encrypt_blob`. Returns plaintext buffer on success.
 */
unsigned char* aead_decrypt_blob(const unsigned char* blob, size_t blob_len,
                                 const char* passphrase, size_t& plaintext_len);

/* Load a private key from PEM container (e.g., encrypted PKCS#8 PEM) using
 * passphrase-mediated OpenSSL key loading.
 */
EVP_PKEY* loadPrivateKey(const char* filename, const char* password);

/* Release an EVP_PKEY handle allocated by OpenSSL. */
void freePrivateKey(EVP_PKEY* pkey);

/* Sign `message` with an Ed25519 private key and return raw signature bytes
 * as `std::string` binary payload.
 */
std::string Ed25519_signMessage(const std::string& message, EVP_PKEY* pkey);

/* Encode arbitrary binary payload into Base64 text (no newline framing). */
std::string base64Encode(const std::string& data);
} /* namespace dencryption */
} /* namespace piaabo */
} /* namespace cuwacunu */
