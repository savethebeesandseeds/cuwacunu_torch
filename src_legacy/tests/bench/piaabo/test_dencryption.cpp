/* test AEAD encryption/decryption round-trip */
#include "piaabo/dsecurity.h"
#include "piaabo/dencryption.h"

#include <cstring>
#include <iostream>
#include <string>

int main() {
  const char* passphrase = "securepassword";
  const char* data = "Hello, World!";
  const size_t data_len = std::strlen(data);

  size_t encrypted_len = 0;
  unsigned char* encrypted = cuwacunu::piaabo::dencryption::aead_encrypt_blob(
      reinterpret_cast<const unsigned char*>(data), data_len, passphrase, encrypted_len);
  if (encrypted == nullptr || encrypted_len == 0) {
    std::cerr << "AEAD encryption failed." << std::endl;
    return 1;
  }

  if (!cuwacunu::piaabo::dencryption::is_aead_blob(encrypted, encrypted_len)) {
    std::cerr << "Encrypted payload is not a valid AEAD blob." << std::endl;
    cuwacunu::piaabo::dsecurity::secure_delete<unsigned char>(encrypted, encrypted_len);
    return 1;
  }

  size_t decrypted_len = 0;
  unsigned char* decrypted = cuwacunu::piaabo::dencryption::aead_decrypt_blob(
      encrypted, encrypted_len, passphrase, decrypted_len);
  if (decrypted == nullptr) {
    std::cerr << "AEAD decryption failed." << std::endl;
    cuwacunu::piaabo::dsecurity::secure_delete<unsigned char>(encrypted, encrypted_len);
    return 1;
  }

  std::string result(reinterpret_cast<char*>(decrypted), decrypted_len);
  std::cout << "Original  text: " << std::string(data) << std::endl;
  std::cout << "Decrypted text: " << result << std::endl;

  cuwacunu::piaabo::dsecurity::secure_delete<unsigned char>(encrypted, encrypted_len);
  cuwacunu::piaabo::dsecurity::secure_delete<unsigned char>(decrypted, decrypted_len);
  return result == data ? 0 : 2;
}
