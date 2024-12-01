/* dsecurity.h */
#pragma once
#include <iostream>
#include <mutex>
#include <fstream>
#include <cstring>
#include <termios.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/xattr.h>
#include <cerrno>
#include <chrono>
#include <thread>
#include "piaabo/dutils.h"
#include "piaabo/dencryption.h"
#include "piaabo/dconfig.h"

#pragma GCC push_options
#pragma GCC optimize ("O0")

namespace cuwacunu {
namespace piaabo {
namespace dsecurity {
/* secure all current and future memory */
void secure_all_code();
/* Reset all current and future memory */
void relax_all_code();
/* Set process as non-dumpable */
void secure_code();
/* Reset process dumpable status */
void relax_code();
/* Lock the memory to prevent swapping */
void secure_mlock(const void *data, size_t size);
/* Unlock the memory */
void secure_munlock(const void *data, size_t size);
/* Secure set memory to zero */
void secure_zero_memory(void *ptr, size_t size);
/* secure allocate variable */
template<typename T>
T* secure_allocate(size_t data_size);
/* secure dealocate variable */
template <typename T>
void secure_delete(T* data, size_t data_size);

class SecureStronghold_t {
private:
  bool is_authenticated;
  std::mutex stronghold_mutex;
  size_t secret_size;
  char* secret;   /* secret is the global password */
  char* api_key;  /* api_key is the exchange key */
  EVP_PKEY* pkey;
public:
  SecureStronghold_t();
  ~SecureStronghold_t();
  void authenticate();  /* prompt user for the secret */
  std::string which_api_key();
  std::string Ed25519_signMessage(const std::string& message); /* sign a message with Ed25519 */
private:
  // std::string rc4(const std::string& value); /* value to be encrypted with RC4 using secret as key */
};
extern SecureStronghold_t SecureVault;
} /* namespace dsecurity */
} /* namespace piaabo */
} /* namespace cuwacunu */
#pragma GCC pop_options