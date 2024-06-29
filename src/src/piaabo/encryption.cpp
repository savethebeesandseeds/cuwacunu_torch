#include "piaabo/encryption.h"

namespace cuwacunu {
namespace piaabo {
namespace encryption {
/* Function to derive AES key and IV using a passphrase */
void derive_key_iv(const char* passphrase, unsigned char* key, unsigned char* iv, const unsigned char* salt) {
  const size_t iterations = AES_KEY_IV_ITERATIONS;
  const size_t key_len = AES_KEY_LEN;

  if (!PKCS5_PBKDF2_HMAC(passphrase, -1, salt, AES_SALT_LEN, iterations, EVP_sha256(), key_len, key)) {
    log_fatal("Error deriving for AES key. \n");
  }

  memcpy(iv, key + 16, AES_BLOCK_SIZE);  /* Use part of the key for the IV */
}

/* AES encryption function */
unsigned char* aes_encrypt(const unsigned char* plaintext, size_t plaintext_len, const unsigned char* key, const unsigned char* iv, size_t& ciphertext_len) {
  AES_KEY enc_key;
  AES_set_encrypt_key(key, 256, &enc_key);

  size_t len = ((plaintext_len + AES_BLOCK_SIZE) / AES_BLOCK_SIZE) * AES_BLOCK_SIZE;
  unsigned char* ciphertext = security::secure_allocate<unsigned char>(len);

  AES_cbc_encrypt(plaintext, ciphertext, plaintext_len, &enc_key, const_cast<unsigned char*>(iv), AES_ENCRYPT);
  ciphertext_len = len;
  return ciphertext;
}

/* AES decryption function */
unsigned char* aes_decrypt(const unsigned char* ciphertext, size_t ciphertext_len, const unsigned char* key, const unsigned char* iv, size_t& plaintext_len) {
  AES_KEY dec_key;
  AES_set_decrypt_key(key, 256, &dec_key);

  unsigned char* plaintext = security::secure_allocate<unsigned char>(ciphertext_len);
  AES_cbc_encrypt(ciphertext, plaintext, ciphertext_len, &dec_key, const_cast<unsigned char*>(iv), AES_DECRYPT);

  /* Remove padding */
  size_t pad = plaintext[ciphertext_len - 1];
  plaintext_len = (pad < 1 || pad > AES_BLOCK_SIZE) ? ciphertext_len : ciphertext_len - pad;
  return plaintext;
}

/* use openssl to read private key file */
EVP_PKEY* loadPrivateKey(const char* filename, const char* password) {
  FILE* file = fopen(filename, "r");
  if (!file) {
    log_warn("Unable to open file %s\n", filename);
    return NULL;
  }

  EVP_PKEY* pkey = PEM_read_PrivateKey(file, NULL, NULL, (void*)password);
  fclose(file);

  if (!pkey) { log_warn("Error reading private key from PEM file: %s\n", filename); }

  return pkey;
}

/* free openssl private key */
void freePrivateKey(EVP_PKEY* pkey) {
  EVP_PKEY_free(pkey);
}

/* Function to sign a message using an Ed25519 private key */
std::string Ed25519_signMessage(const std::string& message, EVP_PKEY* pkey) {
  EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
  if (!mdctx) {
    log_fatal("(Ed25519_signMessage) Failed to create EVP_MD_CTX.\n");
    return "";
  }

  if (1 != EVP_DigestSignInit(mdctx, NULL, NULL, NULL, pkey)) {
    EVP_MD_CTX_free(mdctx);
    log_fatal("(Ed25519_signMessage) DigestSignInit failed.\n");
    return "";
  }

  size_t siglen;
  if (1 != EVP_DigestSign(mdctx, NULL, &siglen, (const unsigned char*)message.c_str(), message.length())) {
    EVP_MD_CTX_free(mdctx);
    log_fatal("(Ed25519_signMessage) DigestSign failed to determine buffer length.\n");
    return "";
  }

  unsigned char* sig = (unsigned char*)malloc(siglen);
  if (1 != EVP_DigestSign(mdctx, sig, &siglen, (const unsigned char*)message.c_str(), message.length())) {
    free(sig);
    EVP_MD_CTX_free(mdctx);
    log_fatal("(Ed25519_signMessage) DigestSign failed.\n");
    return "";
  }

  std::string signature((char*)sig, siglen);
  free(sig);
  EVP_MD_CTX_free(mdctx);

  return signature;
}

} /* encryption */
} /* namespace piaabo */
} /* namespace cuwacunu */