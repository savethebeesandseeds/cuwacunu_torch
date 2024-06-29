#pragma once
#include <cstring>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include "piaabo/dutils.h"
#include "piaabo/security.h"

#define AES_KEY_IV_ITERATIONS 10000 /* AES-256 key IV iterations */
#define AES_KEY_LEN 256 /* AES-256 key length */
#define AES_SALT_LEN 64 /* AES-256 salt length */

namespace cuwacunu {
namespace piaabo {
namespace encryption {
/* Function to derive AES key and IV using a passphrase */
void derive_key_iv(const char* passphrase, unsigned char* key, unsigned char* iv, const unsigned char* salt);
/* AES encryption function */
unsigned char* aes_encrypt(const unsigned char* plaintext, size_t plaintext_len, const unsigned char* key, const unsigned char* iv, size_t& ciphertext_len);
/* AES decryption function */
unsigned char* aes_decrypt(const unsigned char* ciphertext, size_t ciphertext_len, const unsigned char* key, const unsigned char* iv, size_t& plaintext_len);
/* use openssl to read private key file */
EVP_PKEY* loadPrivateKey(const char* filename, const char* password);
/* free openssl private key */
void freePrivateKey(EVP_PKEY* pkey);
/* Function to sign a message using an Ed25519 private key */
std::string Ed25519_signMessage(const std::string& message, EVP_PKEY* pkey);
} /* namespace encryption */
} /* namespace piaabo */
} /* namespace cuwacunu */
