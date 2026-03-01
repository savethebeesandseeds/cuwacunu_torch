/* dsecurity.h */
#pragma once
#include <cstddef>
#include <mutex>
#include <string>
#include <openssl/evp.h>
#include "piaabo/dutils.h"

#pragma GCC push_options
#pragma GCC optimize ("O0")

namespace cuwacunu {
namespace piaabo {
namespace dsecurity {
/*----------------------------------------------------------------------------
 * Process hardening and in-memory secret handling primitives.
 *
 * This module wraps Linux process controls (`mlockall`, `munlockall`, `prctl`)
 * and explicit secret-memory lifecycle utilities used by credential workflows.
 *--------------------------------------------------------------------------*/

/* Enable process-wide page locking for current and future mappings
 * (`mlockall(MCL_CURRENT | MCL_FUTURE)`).
 */
void secure_all_code();

/* Release process-wide page locking (`munlockall`). */
void relax_all_code();

/* Set process dump policy to non-dumpable (`prctl(PR_SET_DUMPABLE, 0)`). */
void secure_code();

/* Restore process dump policy to dumpable (`prctl(PR_SET_DUMPABLE, 1)`). */
void relax_code();

/* Lock a caller-provided memory region into RAM (`mlock`). */
void secure_mlock(const void *data, size_t size);

/* Unlock a caller-provided memory region (`munlock`). */
void secure_munlock(const void *data, size_t size);

/* Zeroize a memory region using a volatile write pattern. */
void secure_zero_memory(void *ptr, size_t size);

/* Allocate an array, initialize to zero, and lock backing pages in RAM. */
template<typename T>
T* secure_allocate(size_t data_size);

/* Zeroize, unlock, and release an array allocated via `secure_allocate`. */
template <typename T>
void secure_delete(T* data, size_t data_size);

/*----------------------------------------------------------------------------
 * SecureStronghold_t
 *
 * Runtime credential container for:
 * - user passphrase capture and verification,
 * - exchange API key decryption lifecycle,
 * - Ed25519 signing through OpenSSL EVP.
 *--------------------------------------------------------------------------*/
class SecureStronghold_t {
private:
  bool is_authenticated;
  std::mutex stronghold_mutex;
  size_t secret_size;
  /* Passphrase buffer used for encrypted PEM unlock and KDF input. */
  char* secret;
  size_t api_key_size;
  /* Decrypted exchange API credential currently held in process memory. */
  char* api_key;
  /* OpenSSL EVP key handle (Ed25519 private key). */
  EVP_PKEY* pkey;
public:
  SecureStronghold_t();
  ~SecureStronghold_t();

  /* Interactive authentication flow:
   * - reads passphrase from TTY with echo suppression (termios),
   * - validates passphrase by loading encrypted Ed25519 PEM material,
   * - derives symmetric material and loads API key into secured memory.
   */
  void authenticate();

  /* Return a snapshot copy of the currently loaded API key value. */
  std::string which_api_key();

  /* Produce a Base64-encoded Ed25519 signature for `message`. */
  std::string Ed25519_signMessage(const std::string& message);
private:
  // std::string rc4(const std::string& value); /* value to be encrypted with RC4 using secret as key */
};

/* Global stronghold singleton used by runtime exchange workflows. */
extern SecureStronghold_t SecureVault;
} /* namespace dsecurity */
} /* namespace piaabo */
} /* namespace cuwacunu */
#pragma GCC pop_options
