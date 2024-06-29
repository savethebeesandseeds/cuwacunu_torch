/* test the AES encryption/decription */
#include "piaabo/security.h"
#include "piaabo/encryption.h"

int main() {
  unsigned char* salt      = cuwacunu::piaabo::security::secure_allocate<unsigned char>(AES_SALT_LEN);
  unsigned char* key       = cuwacunu::piaabo::security::secure_allocate<unsigned char>(AES_KEY_LEN);
  unsigned char* iv        = cuwacunu::piaabo::security::secure_allocate<unsigned char>(AES_BLOCK_SIZE);
  unsigned char* iv_backup = cuwacunu::piaabo::security::secure_allocate<unsigned char>(AES_BLOCK_SIZE);

  const char* passphrase = "securepassword";
  memcpy(salt, "saltsalt", AES_SALT_LEN); // RAND_bytes(salt, AES_SALT_LEN);

  cuwacunu::piaabo::encryption::derive_key_iv(passphrase, key, iv_backup, salt);

  const char* data = "Hello, World!";
  size_t data_len = strlen(data);
  size_t encrypted_len, decrypted_len;

  /* Encrypt */
  memcpy(iv, iv_backup, AES_BLOCK_SIZE);   /* Restore IV from backup before decryption */
  unsigned char* encrypted = cuwacunu::piaabo::encryption::aes_encrypt(reinterpret_cast<const unsigned char*>(data), data_len, key, iv, encrypted_len);

  /* Decrypt */
  memcpy(iv, iv_backup, AES_BLOCK_SIZE);   /* Restore IV from backup before decryption */
  unsigned char* decrypted = cuwacunu::piaabo::encryption::aes_decrypt(encrypted, encrypted_len, key, iv, decrypted_len);

  std::string result(reinterpret_cast<char*>(decrypted), decrypted_len);
  std::cout << "Decrypted text: " << result << std::endl;

  /* Clean up */
  cuwacunu::piaabo::security::secure_delete<unsigned char>(salt, AES_SALT_LEN);
  cuwacunu::piaabo::security::secure_delete<unsigned char>(key, AES_KEY_LEN);
  cuwacunu::piaabo::security::secure_delete<unsigned char>(iv, AES_BLOCK_SIZE);
  cuwacunu::piaabo::security::secure_delete<unsigned char>(iv_backup, AES_BLOCK_SIZE);
  
  cuwacunu::piaabo::security::secure_delete<unsigned char>(encrypted, encrypted_len);
  cuwacunu::piaabo::security::secure_delete<unsigned char>(decrypted, decrypted_len);

  return 0;
}
