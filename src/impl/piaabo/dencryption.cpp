#include "piaabo/dencryption.h"

namespace cuwacunu {
namespace piaabo {
namespace dencryption {
/* Function to derive AES key and IV using a passphrase */
void derive_key_iv(const char* passphrase, unsigned char* key, unsigned char* iv, const unsigned char* salt) {
  const int iterations = AES_KEY_IV_ITERATIONS;

  unsigned char temp_key_iv[AES_KEY_LEN + AES_IV_LEN]; // Temporary buffer

  if (!PKCS5_PBKDF2_HMAC(passphrase, -1, salt, AES_SALT_LEN, iterations, EVP_sha256(), AES_KEY_LEN + AES_IV_LEN, temp_key_iv)) {
    log_fatal("Error deriving AES key and IV.\n");
  }

  memcpy(key, temp_key_iv, AES_KEY_LEN);                   // Copy key portion
  memcpy(iv, temp_key_iv + AES_KEY_LEN, AES_IV_LEN);       // Copy IV portion
}


/* AES encryption function using EVP */
unsigned char* aes_encrypt(const unsigned char* plaintext, size_t plaintext_len, const unsigned char* key, const unsigned char* iv, size_t& ciphertext_len) {
  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) {
    log_fatal("Failed to create EVP_CIPHER_CTX.\n");
    return nullptr;
  }

  int len;
  int ciphertext_len_int = 0;

  /* Allocate memory for ciphertext (plaintext length + block size) */
  unsigned char* ciphertext = dsecurity::secure_allocate<unsigned char>(plaintext_len + AES_BLOCK_SIZE);

  if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
    log_fatal("EVP_EncryptInit_ex failed.\n");
    EVP_CIPHER_CTX_free(ctx);
    dsecurity::secure_delete<unsigned char>(ciphertext, plaintext_len + AES_BLOCK_SIZE);
    return nullptr;
  }

  if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len)) {
    log_fatal("EVP_EncryptUpdate failed.\n");
    EVP_CIPHER_CTX_free(ctx);
    dsecurity::secure_delete<unsigned char>(ciphertext, plaintext_len + AES_BLOCK_SIZE);
    return nullptr;
  }
  ciphertext_len_int = len;

  if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) {
    log_fatal("EVP_EncryptFinal_ex failed.\n");
    EVP_CIPHER_CTX_free(ctx);
    dsecurity::secure_delete<unsigned char>(ciphertext, plaintext_len + AES_BLOCK_SIZE);
    return nullptr;
  }
  ciphertext_len_int += len;

  ciphertext_len = static_cast<size_t>(ciphertext_len_int);

  EVP_CIPHER_CTX_free(ctx);

  return ciphertext;
}


/* AES decryption function using EVP */
unsigned char* aes_decrypt(const unsigned char* ciphertext, size_t ciphertext_len, const unsigned char* key, const unsigned char* iv, size_t& plaintext_len) {
  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) {
    log_fatal("Failed to create EVP_CIPHER_CTX.\n");
    return nullptr;
  }

  int len;
  int plaintext_len_int = 0;

  // Allocate memory for plaintext (ciphertext length)
  unsigned char* plaintext = dsecurity::secure_allocate<unsigned char>(ciphertext_len);

  if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
    log_fatal("EVP_DecryptInit_ex failed.\n");
    EVP_CIPHER_CTX_free(ctx);
    dsecurity::secure_delete<unsigned char>(plaintext, ciphertext_len);
    return nullptr;
  }

  if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len)) {
    log_fatal("EVP_DecryptUpdate failed.\n");
    EVP_CIPHER_CTX_free(ctx);
    dsecurity::secure_delete<unsigned char>(plaintext, ciphertext_len);
    return nullptr;
  }
  plaintext_len_int = len;

  if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) {
    log_fatal("EVP_DecryptFinal_ex failed. Possible incorrect key or corrupted data.\n");
    EVP_CIPHER_CTX_free(ctx);
    dsecurity::secure_delete<unsigned char>(plaintext, ciphertext_len);
    return nullptr;
  }
  plaintext_len_int += len;

  plaintext_len = static_cast<size_t>(plaintext_len_int);

  EVP_CIPHER_CTX_free(ctx);

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

/* function to encode in base64 */
std::string base64Encode(const std::string& data) {
  BIO* bio, *b64;
  BUF_MEM* bufferPtr;

  b64 = BIO_new(BIO_f_base64());
  // Do not use newlines to flush buffer
  BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL); 
  bio = BIO_new(BIO_s_mem());
  bio = BIO_push(b64, bio);

  BIO_write(bio, data.c_str(), data.length());
  BIO_flush(bio);
  BIO_get_mem_ptr(bio, &bufferPtr);

  std::string encodedData(bufferPtr->data, bufferPtr->length);
  BIO_free_all(bio);

  return encodedData;
}

} /* namespace dencryption */
} /* namespace piaabo */
} /* namespace cuwacunu */