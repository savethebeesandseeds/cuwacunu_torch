#include "piaabo/dencryption.h"

#include "piaabo/dsecurity.h"

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace cuwacunu {
namespace piaabo {
namespace dencryption {
namespace {
constexpr unsigned char kAeadMagic[] = {'C', 'U', 'W', 'A', 'E', 'A', 'D', '1'};
constexpr size_t kAeadMagicLen = sizeof(kAeadMagic);
constexpr uint8_t kAeadVersion = 1;
constexpr size_t kAeadHeaderLen = kAeadMagicLen + 1 + 1 + 1 + 1 + 4;
constexpr uint32_t kAeadMinPbkdf2Iterations = 10000U;
constexpr uint32_t kAeadMaxPbkdf2Iterations = 5000000U;
constexpr const char* kAeadIterationsEnv = "CUWACUNU_AEAD_PBKDF2_ITERATIONS";

uint32_t read_be_u32(const unsigned char* p) {
  return (static_cast<uint32_t>(p[0]) << 24U) | (static_cast<uint32_t>(p[1]) << 16U) |
         (static_cast<uint32_t>(p[2]) << 8U) | static_cast<uint32_t>(p[3]);
}

void write_be_u32(unsigned char* p, uint32_t v) {
  p[0] = static_cast<unsigned char>((v >> 24U) & 0xFFU);
  p[1] = static_cast<unsigned char>((v >> 16U) & 0xFFU);
  p[2] = static_cast<unsigned char>((v >> 8U) & 0xFFU);
  p[3] = static_cast<unsigned char>(v & 0xFFU);
}

uint32_t resolve_aead_pbkdf2_iterations() {
  const char* env_value = std::getenv(kAeadIterationsEnv);
  if (env_value == nullptr || env_value[0] == '\0') {
    return AEAD_DEFAULT_PBKDF2_ITERATIONS;
  }

  errno = 0;
  char* end = nullptr;
  const unsigned long parsed = std::strtoul(env_value, &end, 10);
  if (errno != 0 || end == env_value || *end != '\0') {
    log_warn("Invalid %s value '%s', using default %u.\n", kAeadIterationsEnv, env_value,
             AEAD_DEFAULT_PBKDF2_ITERATIONS);
    return AEAD_DEFAULT_PBKDF2_ITERATIONS;
  }

  uint32_t iterations = static_cast<uint32_t>(
      parsed > static_cast<unsigned long>(std::numeric_limits<uint32_t>::max())
          ? std::numeric_limits<uint32_t>::max()
          : parsed);

  if (iterations < kAeadMinPbkdf2Iterations) {
    log_warn("%s too small (%u), clamping to %u.\n", kAeadIterationsEnv, iterations,
             kAeadMinPbkdf2Iterations);
    iterations = kAeadMinPbkdf2Iterations;
  } else if (iterations > kAeadMaxPbkdf2Iterations) {
    log_warn("%s too large (%u), clamping to %u.\n", kAeadIterationsEnv, iterations,
             kAeadMaxPbkdf2Iterations);
    iterations = kAeadMaxPbkdf2Iterations;
  }
  return iterations;
}

bool derive_pbkdf2_key(const char* passphrase, const unsigned char* salt, int salt_len,
                       uint32_t iterations, unsigned char* key) {
  if (passphrase == nullptr || salt == nullptr || key == nullptr || salt_len <= 0) {
    return false;
  }
  return PKCS5_PBKDF2_HMAC(passphrase, -1, salt, salt_len, static_cast<int>(iterations),
                           EVP_sha256(), AEAD_KEY_LEN, key) == 1;
}

bool parse_aead_blob_header(const unsigned char* blob, size_t blob_len, uint8_t* version,
                            uint8_t* salt_len, uint8_t* nonce_len, uint8_t* tag_len,
                            uint32_t* iterations, size_t* ciphertext_len) {
  if (blob == nullptr || blob_len < kAeadHeaderLen) {
    return false;
  }
  if (memcmp(blob, kAeadMagic, kAeadMagicLen) != 0) {
    return false;
  }

  const uint8_t parsed_version = blob[8];
  const uint8_t parsed_salt_len = blob[9];
  const uint8_t parsed_nonce_len = blob[10];
  const uint8_t parsed_tag_len = blob[11];
  const uint32_t parsed_iterations = read_be_u32(blob + 12);

  if (parsed_version == 0 || parsed_salt_len == 0 || parsed_nonce_len == 0 || parsed_tag_len == 0) {
    return false;
  }
  if (parsed_iterations < kAeadMinPbkdf2Iterations) {
    return false;
  }

  const size_t framing_len = kAeadHeaderLen + static_cast<size_t>(parsed_salt_len) +
                             static_cast<size_t>(parsed_nonce_len) +
                             static_cast<size_t>(parsed_tag_len);
  if (blob_len < framing_len) {
    return false;
  }

  const size_t parsed_ciphertext_len = blob_len - framing_len;
  if (parsed_ciphertext_len == 0) {
    return false;
  }

  if (version != nullptr) {
    *version = parsed_version;
  }
  if (salt_len != nullptr) {
    *salt_len = parsed_salt_len;
  }
  if (nonce_len != nullptr) {
    *nonce_len = parsed_nonce_len;
  }
  if (tag_len != nullptr) {
    *tag_len = parsed_tag_len;
  }
  if (iterations != nullptr) {
    *iterations = parsed_iterations;
  }
  if (ciphertext_len != nullptr) {
    *ciphertext_len = parsed_ciphertext_len;
  }
  return true;
}
} // namespace

bool is_aead_blob(const unsigned char* blob, size_t blob_len) {
  uint8_t version = 0;
  uint8_t salt_len = 0;
  uint8_t nonce_len = 0;
  uint8_t tag_len = 0;
  uint32_t iterations = 0;
  size_t ciphertext_len = 0;
  if (!parse_aead_blob_header(blob, blob_len, &version, &salt_len, &nonce_len, &tag_len,
                              &iterations, &ciphertext_len)) {
    return false;
  }

  return version == kAeadVersion && salt_len == AEAD_SALT_LEN && nonce_len == AEAD_NONCE_LEN &&
         tag_len == AEAD_TAG_LEN && iterations >= kAeadMinPbkdf2Iterations &&
         ciphertext_len > 0;
}

unsigned char* aead_encrypt_blob(const unsigned char* plaintext, size_t plaintext_len,
                                 const char* passphrase, size_t& blob_len) {
  blob_len = 0;
  if (passphrase == nullptr) {
    log_fatal("aead_encrypt_blob received null passphrase.\n");
    return nullptr;
  }
  if (plaintext == nullptr || plaintext_len == 0) {
    log_fatal("aead_encrypt_blob requires non-empty plaintext.\n");
    return nullptr;
  }
  if (plaintext_len > static_cast<size_t>(INT_MAX)) {
    log_fatal("aead_encrypt_blob plaintext is too large for EVP API.\n");
    return nullptr;
  }

  unsigned char salt[AEAD_SALT_LEN];
  unsigned char nonce[AEAD_NONCE_LEN];
  unsigned char tag[AEAD_TAG_LEN];
  unsigned char key[AEAD_KEY_LEN];

  if (RAND_bytes(salt, AEAD_SALT_LEN) != 1) {
    log_fatal("Failed to generate AEAD salt.\n");
    return nullptr;
  }
  if (RAND_bytes(nonce, AEAD_NONCE_LEN) != 1) {
    dsecurity::secure_zero_memory(salt, sizeof(salt));
    log_fatal("Failed to generate AEAD nonce.\n");
    return nullptr;
  }

  const uint32_t iterations = resolve_aead_pbkdf2_iterations();
  if (!derive_pbkdf2_key(passphrase, salt, AEAD_SALT_LEN, iterations, key)) {
    dsecurity::secure_zero_memory(salt, sizeof(salt));
    dsecurity::secure_zero_memory(nonce, sizeof(nonce));
    log_fatal("Failed to derive AEAD encryption key.\n");
    return nullptr;
  }

  unsigned char* ciphertext = dsecurity::secure_allocate<unsigned char>(plaintext_len);
  if (ciphertext == nullptr) {
    dsecurity::secure_zero_memory(salt, sizeof(salt));
    dsecurity::secure_zero_memory(nonce, sizeof(nonce));
    dsecurity::secure_zero_memory(key, sizeof(key));
    log_fatal("Failed to allocate AEAD ciphertext buffer.\n");
    return nullptr;
  }

  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (ctx == nullptr) {
    dsecurity::secure_delete<unsigned char>(ciphertext, plaintext_len);
    dsecurity::secure_zero_memory(salt, sizeof(salt));
    dsecurity::secure_zero_memory(nonce, sizeof(nonce));
    dsecurity::secure_zero_memory(key, sizeof(key));
    log_fatal("Failed to create AEAD cipher context.\n");
    return nullptr;
  }

  int len = 0;
  int ciphertext_len_int = 0;
  if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) ||
      1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, AEAD_NONCE_LEN, nullptr) ||
      1 != EVP_EncryptInit_ex(ctx, nullptr, nullptr, key, nonce) ||
      1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, static_cast<int>(plaintext_len))) {
    EVP_CIPHER_CTX_free(ctx);
    dsecurity::secure_delete<unsigned char>(ciphertext, plaintext_len);
    dsecurity::secure_zero_memory(salt, sizeof(salt));
    dsecurity::secure_zero_memory(nonce, sizeof(nonce));
    dsecurity::secure_zero_memory(key, sizeof(key));
    log_fatal("Failed during AEAD encryption setup/update.\n");
    return nullptr;
  }
  ciphertext_len_int = len;

  if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + ciphertext_len_int, &len)) {
    EVP_CIPHER_CTX_free(ctx);
    dsecurity::secure_delete<unsigned char>(ciphertext, plaintext_len);
    dsecurity::secure_zero_memory(salt, sizeof(salt));
    dsecurity::secure_zero_memory(nonce, sizeof(nonce));
    dsecurity::secure_zero_memory(key, sizeof(key));
    log_fatal("AEAD encryption finalization failed.\n");
    return nullptr;
  }
  ciphertext_len_int += len;

  if (ciphertext_len_int < 0 || static_cast<size_t>(ciphertext_len_int) != plaintext_len) {
    EVP_CIPHER_CTX_free(ctx);
    dsecurity::secure_delete<unsigned char>(ciphertext, plaintext_len);
    dsecurity::secure_zero_memory(salt, sizeof(salt));
    dsecurity::secure_zero_memory(nonce, sizeof(nonce));
    dsecurity::secure_zero_memory(key, sizeof(key));
    log_fatal("AEAD encryption produced unexpected ciphertext length.\n");
    return nullptr;
  }

  if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, AEAD_TAG_LEN, tag)) {
    EVP_CIPHER_CTX_free(ctx);
    dsecurity::secure_delete<unsigned char>(ciphertext, plaintext_len);
    dsecurity::secure_zero_memory(salt, sizeof(salt));
    dsecurity::secure_zero_memory(nonce, sizeof(nonce));
    dsecurity::secure_zero_memory(key, sizeof(key));
    log_fatal("Failed to retrieve AEAD authentication tag.\n");
    return nullptr;
  }
  EVP_CIPHER_CTX_free(ctx);

  if (plaintext_len > (std::numeric_limits<size_t>::max() - kAeadHeaderLen - AEAD_SALT_LEN -
                       AEAD_NONCE_LEN - AEAD_TAG_LEN)) {
    dsecurity::secure_delete<unsigned char>(ciphertext, plaintext_len);
    dsecurity::secure_zero_memory(salt, sizeof(salt));
    dsecurity::secure_zero_memory(nonce, sizeof(nonce));
    dsecurity::secure_zero_memory(tag, sizeof(tag));
    dsecurity::secure_zero_memory(key, sizeof(key));
    log_fatal("AEAD blob length overflow.\n");
    return nullptr;
  }

  blob_len = kAeadHeaderLen + AEAD_SALT_LEN + AEAD_NONCE_LEN + plaintext_len + AEAD_TAG_LEN;
  unsigned char* blob = dsecurity::secure_allocate<unsigned char>(blob_len);
  if (blob == nullptr) {
    dsecurity::secure_delete<unsigned char>(ciphertext, plaintext_len);
    dsecurity::secure_zero_memory(salt, sizeof(salt));
    dsecurity::secure_zero_memory(nonce, sizeof(nonce));
    dsecurity::secure_zero_memory(tag, sizeof(tag));
    dsecurity::secure_zero_memory(key, sizeof(key));
    log_fatal("Failed to allocate AEAD blob buffer.\n");
    return nullptr;
  }

  memcpy(blob, kAeadMagic, kAeadMagicLen);
  blob[8] = kAeadVersion;
  blob[9] = AEAD_SALT_LEN;
  blob[10] = AEAD_NONCE_LEN;
  blob[11] = AEAD_TAG_LEN;
  write_be_u32(blob + 12, iterations);

  size_t cursor = kAeadHeaderLen;
  memcpy(blob + cursor, salt, AEAD_SALT_LEN);
  cursor += AEAD_SALT_LEN;
  memcpy(blob + cursor, nonce, AEAD_NONCE_LEN);
  cursor += AEAD_NONCE_LEN;
  memcpy(blob + cursor, ciphertext, plaintext_len);
  cursor += plaintext_len;
  memcpy(blob + cursor, tag, AEAD_TAG_LEN);

  dsecurity::secure_delete<unsigned char>(ciphertext, plaintext_len);
  dsecurity::secure_zero_memory(salt, sizeof(salt));
  dsecurity::secure_zero_memory(nonce, sizeof(nonce));
  dsecurity::secure_zero_memory(tag, sizeof(tag));
  dsecurity::secure_zero_memory(key, sizeof(key));
  return blob;
}

unsigned char* aead_decrypt_blob(const unsigned char* blob, size_t blob_len, const char* passphrase,
                                 size_t& plaintext_len) {
  plaintext_len = 0;
  if (passphrase == nullptr) {
    log_fatal("aead_decrypt_blob received null passphrase.\n");
    return nullptr;
  }

  uint8_t version = 0;
  uint8_t salt_len = 0;
  uint8_t nonce_len = 0;
  uint8_t tag_len = 0;
  uint32_t iterations = 0;
  size_t ciphertext_len = 0;
  if (!parse_aead_blob_header(blob, blob_len, &version, &salt_len, &nonce_len, &tag_len,
                              &iterations, &ciphertext_len)) {
    log_fatal("Invalid AEAD blob header.\n");
    return nullptr;
  }
  if (version != kAeadVersion || salt_len != AEAD_SALT_LEN || nonce_len != AEAD_NONCE_LEN ||
      tag_len != AEAD_TAG_LEN) {
    log_fatal("Unsupported AEAD blob parameters.\n");
    return nullptr;
  }
  if (ciphertext_len > static_cast<size_t>(INT_MAX)) {
    log_fatal("AEAD ciphertext is too large for EVP API.\n");
    return nullptr;
  }

  const unsigned char* salt = blob + kAeadHeaderLen;
  const unsigned char* nonce = salt + salt_len;
  const unsigned char* ciphertext = nonce + nonce_len;
  const unsigned char* tag = ciphertext + ciphertext_len;

  unsigned char key[AEAD_KEY_LEN];
  if (!derive_pbkdf2_key(passphrase, salt, salt_len, iterations, key)) {
    log_fatal("Failed to derive AEAD decryption key.\n");
    return nullptr;
  }

  unsigned char* plaintext = dsecurity::secure_allocate<unsigned char>(ciphertext_len);
  if (plaintext == nullptr) {
    dsecurity::secure_zero_memory(key, sizeof(key));
    log_fatal("Failed to allocate AEAD plaintext buffer.\n");
    return nullptr;
  }

  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (ctx == nullptr) {
    dsecurity::secure_delete<unsigned char>(plaintext, ciphertext_len);
    dsecurity::secure_zero_memory(key, sizeof(key));
    log_fatal("Failed to create AEAD cipher context.\n");
    return nullptr;
  }

  int len = 0;
  int plaintext_len_int = 0;
  if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) ||
      1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, nonce_len, nullptr) ||
      1 != EVP_DecryptInit_ex(ctx, nullptr, nullptr, key, nonce) ||
      1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, static_cast<int>(ciphertext_len))) {
    EVP_CIPHER_CTX_free(ctx);
    dsecurity::secure_delete<unsigned char>(plaintext, ciphertext_len);
    dsecurity::secure_zero_memory(key, sizeof(key));
    log_fatal("Failed during AEAD decryption setup/update.\n");
    return nullptr;
  }
  plaintext_len_int = len;

  if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, tag_len, const_cast<unsigned char*>(tag))) {
    EVP_CIPHER_CTX_free(ctx);
    dsecurity::secure_delete<unsigned char>(plaintext, ciphertext_len);
    dsecurity::secure_zero_memory(key, sizeof(key));
    log_fatal("Failed to set AEAD authentication tag.\n");
    return nullptr;
  }

  const int final_ok = EVP_DecryptFinal_ex(ctx, plaintext + plaintext_len_int, &len);
  EVP_CIPHER_CTX_free(ctx);
  dsecurity::secure_zero_memory(key, sizeof(key));
  if (final_ok != 1) {
    dsecurity::secure_delete<unsigned char>(plaintext, ciphertext_len);
    log_fatal("AEAD authentication failed while decrypting blob.\n");
    return nullptr;
  }

  plaintext_len_int += len;
  if (plaintext_len_int < 0) {
    dsecurity::secure_delete<unsigned char>(plaintext, ciphertext_len);
    log_fatal("AEAD decryption produced invalid plaintext length.\n");
    return nullptr;
  }

  plaintext_len = static_cast<size_t>(plaintext_len_int);
  return plaintext;
}

/* use openssl to read private key file */
EVP_PKEY* loadPrivateKey(const char* filename, const char* password) {
  if (filename == nullptr || password == nullptr) {
    log_warn("loadPrivateKey received invalid input.\n");
    return nullptr;
  }

  FILE* file = fopen(filename, "r");
  if (!file) {
    log_warn("Unable to open file %s\n", filename);
    return nullptr;
  }

  EVP_PKEY* pkey = PEM_read_PrivateKey(file, nullptr, nullptr, const_cast<char*>(password));
  fclose(file);

  if (!pkey) {
    log_warn("Error reading private key from PEM file: %s\n", filename);
  }
  return pkey;
}

/* free openssl private key */
void freePrivateKey(EVP_PKEY* pkey) {
  if (pkey != nullptr) {
    EVP_PKEY_free(pkey);
  }
}

/* Function to sign a message using an Ed25519 private key */
std::string Ed25519_signMessage(const std::string& message, EVP_PKEY* pkey) {
  if (pkey == nullptr) {
    log_fatal("(Ed25519_signMessage) Null private key provided.\n");
    return "";
  }

  EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
  if (!mdctx) {
    log_fatal("(Ed25519_signMessage) Failed to create EVP_MD_CTX.\n");
    return "";
  }

  if (1 != EVP_DigestSignInit(mdctx, nullptr, nullptr, nullptr, pkey)) {
    EVP_MD_CTX_free(mdctx);
    log_fatal("(Ed25519_signMessage) DigestSignInit failed.\n");
    return "";
  }

  size_t siglen = 0;
  if (1 != EVP_DigestSign(mdctx, nullptr, &siglen,
                          reinterpret_cast<const unsigned char*>(message.data()),
                          message.length())) {
    EVP_MD_CTX_free(mdctx);
    log_fatal("(Ed25519_signMessage) DigestSign failed to determine buffer length.\n");
    return "";
  }
  if (siglen == 0) {
    EVP_MD_CTX_free(mdctx);
    log_fatal("(Ed25519_signMessage) DigestSign returned empty signature.\n");
    return "";
  }

  unsigned char* sig = static_cast<unsigned char*>(malloc(siglen));
  if (sig == nullptr) {
    EVP_MD_CTX_free(mdctx);
    log_fatal("(Ed25519_signMessage) Failed to allocate signature buffer.\n");
    return "";
  }

  if (1 != EVP_DigestSign(mdctx, sig, &siglen, reinterpret_cast<const unsigned char*>(message.data()),
                          message.length())) {
    free(sig);
    EVP_MD_CTX_free(mdctx);
    log_fatal("(Ed25519_signMessage) DigestSign failed.\n");
    return "";
  }

  std::string signature(reinterpret_cast<char*>(sig), siglen);
  dsecurity::secure_zero_memory(sig, siglen);
  free(sig);
  EVP_MD_CTX_free(mdctx);
  return signature;
}

/* function to encode in base64 */
std::string base64Encode(const std::string& data) {
  if (data.empty()) {
    return "";
  }
  if (data.length() > static_cast<size_t>(INT_MAX)) {
    log_fatal("base64Encode input too large.\n");
    return "";
  }

  BIO* b64 = BIO_new(BIO_f_base64());
  BIO* bio = BIO_new(BIO_s_mem());
  if (b64 == nullptr || bio == nullptr) {
    if (b64 != nullptr) {
      BIO_free(b64);
    }
    if (bio != nullptr) {
      BIO_free(bio);
    }
    log_fatal("Failed to allocate BIO objects for base64 encoding.\n");
    return "";
  }

  BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
  bio = BIO_push(b64, bio);

  if (BIO_write(bio, data.data(), static_cast<int>(data.length())) <= 0) {
    BIO_free_all(bio);
    log_fatal("Failed to write base64 buffer.\n");
    return "";
  }
  if (BIO_flush(bio) != 1) {
    BIO_free_all(bio);
    log_fatal("Failed to flush base64 buffer.\n");
    return "";
  }

  BUF_MEM* buffer_ptr = nullptr;
  BIO_get_mem_ptr(bio, &buffer_ptr);
  if (buffer_ptr == nullptr || buffer_ptr->data == nullptr) {
    BIO_free_all(bio);
    log_fatal("Failed to read base64 buffer.\n");
    return "";
  }

  const std::string encoded_data(buffer_ptr->data, buffer_ptr->length);
  BIO_free_all(bio);
  return encoded_data;
}

} /* namespace dencryption */
} /* namespace piaabo */
} /* namespace cuwacunu */
