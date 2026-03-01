#include "piaabo/dsecurity.h"

#include "piaabo/dconfig.h"
#include "piaabo/dencryption.h"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <new>
#include <termios.h>
#include <thread>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/prctl.h>

#define AUTHENTICATION_PROMPT "Enter Password:"
#define WRONG_AUTH "Wrong password."
#define CORRECT_AUTH "Authentication success."
#define FATAL_AUTH "Password was too long and has been truncated, fatal termination."
#define FATAL_NO_AUTH_SIGNATURE "Trying to sign a message without prior authentication, fatal termination."
#define FATAL_NON_TTY_AUTH "Authentication requires an interactive TTY terminal."
#define MAX_PASSWORD_SIZE 1024

RUNTIME_WARNING("(dsecurity.cpp)[] secure the secret with hardware \n");
RUNTIME_WARNING("(dsecurity.cpp)[] add second factor authentication \n");

namespace cuwacunu {
namespace piaabo {
namespace dsecurity {
/* secure all current and future memory */
void secure_all_code() {
  if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
    log_secure_fatal("Total Memory locking failed. \n");
  } else {
    log_secure_dbg("Locking all program memory. \n");
  }
}

/* Reset all current and future memory */
void relax_all_code() {
  if (munlockall() != 0) {
    log_secure_fatal("Failed to unlock all memory. \n");
  } else {
    log_secure_warn("Unlocking all program memory. \n");
  }
}

/* Set process as non-dumpable */
void secure_code() {
  if (prctl(PR_SET_DUMPABLE, 0) != 0) {
    log_secure_fatal("Failed to set process as non-dumpable. \n");
  }
}

/* Reset process dumpable status */
void relax_code() {
  if (prctl(PR_SET_DUMPABLE, 1) != 0) {
    log_secure_fatal("Failed to reset process dumpable status. \n");
  }
}

/* Lock the memory to prevent swapping */
void secure_mlock(const void* data, size_t size) {
  if (data == nullptr || size == 0) {
    return;
  }
  if (mlock(data, size) != 0) {
    log_secure_fatal("Memory locking failed. \n");
  }
}

/* Unlock the memory */
void secure_munlock(const void* data, size_t size) {
  if (data == nullptr || size == 0) {
    return;
  }
  if (munlock(data, size) != 0) {
    log_secure_fatal("Memory unlocking failed. \n");
  }
}

/* Secure set memory to zero */
void secure_zero_memory(void* ptr, size_t size) {
  if (ptr == nullptr || size == 0) {
    return;
  }
  volatile unsigned char* volatile_ptr = reinterpret_cast<volatile unsigned char*>(ptr);
  while (size-- > 0) {
    *volatile_ptr++ = 0;
  }
}

/* secure allocate variable */
template <typename T>
T* secure_allocate(size_t data_size) {
  if (data_size == 0) {
    return nullptr;
  }
  if (data_size > (std::numeric_limits<size_t>::max() / sizeof(T))) {
    log_secure_fatal("(secure_allocate) Memory allocation size overflow.\n");
  }

  const size_t bytes = data_size * sizeof(T);
  T* data = new (std::nothrow) T[data_size];
  if (data == nullptr) {
    log_secure_fatal("(secure_allocate) Memory allocation failed.\n");
  }

  secure_zero_memory(data, bytes);
  secure_mlock(data, bytes);
  return data;
}

/* secure dealocate variable */
template <typename T>
void secure_delete(T* data, size_t data_size) {
  if (data == nullptr || data_size == 0) {
    return;
  }
  if (data_size > (std::numeric_limits<size_t>::max() / sizeof(T))) {
    log_secure_fatal("(secure_delete) Memory wipe size overflow.\n");
  }

  const size_t bytes = data_size * sizeof(T);
  secure_zero_memory(data, bytes);
  secure_munlock(data, bytes);
  delete[] data;
}

namespace {
class TerminalEchoGuard {
public:
  TerminalEchoGuard() : active_(false) {
    if (tcgetattr(STDIN_FILENO, &oldt_) != 0) {
      log_secure_fatal("Failed to read terminal settings.\n");
    }
    struct termios newt = oldt_;
    newt.c_lflag &= static_cast<tcflag_t>(~ECHO);
    if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) != 0) {
      log_secure_fatal("Failed to disable terminal echo.\n");
    }
    active_ = true;
  }

  ~TerminalEchoGuard() {
    if (active_ && tcsetattr(STDIN_FILENO, TCSANOW, &oldt_) != 0) {
      log_secure_error("Failed to restore terminal settings.\n");
    }
  }

  TerminalEchoGuard(const TerminalEchoGuard&) = delete;
  TerminalEchoGuard& operator=(const TerminalEchoGuard&) = delete;

private:
  struct termios oldt_ {};
  bool active_;
};

template <typename T>
class ScopedSecureArray {
public:
  ScopedSecureArray() = default;

  explicit ScopedSecureArray(size_t size) {
    reset(size);
  }

  ~ScopedSecureArray() {
    clear();
  }

  ScopedSecureArray(const ScopedSecureArray&) = delete;
  ScopedSecureArray& operator=(const ScopedSecureArray&) = delete;

  void reset(size_t size) {
    clear();
    if (size == 0) {
      return;
    }
    data_ = secure_allocate<T>(size);
    size_ = size;
  }

  void adopt(T* data, size_t size) {
    clear();
    data_ = data;
    size_ = size;
  }

  T* data() const {
    return data_;
  }

  size_t size() const {
    return size_;
  }

  bool empty() const {
    return data_ == nullptr || size_ == 0;
  }

  void clear() {
    if (data_ != nullptr && size_ > 0) {
      secure_delete<T>(data_, size_);
    }
    data_ = nullptr;
    size_ = 0;
  }

private:
  T* data_ = nullptr;
  size_t size_ = 0;
};

/* Write to file securing memory */
void secure_write_to_file(const char* filename, const unsigned char* data, size_t size) {
  std::ofstream file(filename, std::ios::out | std::ios::binary | std::ios::trunc);
  if (!file) {
    log_secure_fatal("(secure_write_to_file) Cannot open file to write: %s\n", filename);
  }

  if (size > 0 && data == nullptr) {
    log_secure_fatal("(secure_write_to_file) Null data with non-zero size for file: %s\n", filename);
  }

  if (size > 0) {
    file.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
    if (!file) {
      log_secure_fatal("(secure_write_to_file) Cannot write to file: %s\n", filename);
    }
  }

  file.close();
}

/* Read from file securing memory */
unsigned char* secure_read_from_file(const char* filename, size_t& size) {
  size = 0;
  std::ifstream file(filename, std::ios::in | std::ios::binary);
  if (!file) {
    log_secure_fatal("(secure_read_from_file) Cannot open file to read: %s\n", filename);
  }

  file.seekg(0, std::ios::end);
  const std::streampos end_pos = file.tellg();
  if (end_pos < 0) {
    log_secure_fatal("(secure_read_from_file) Failed to determine file size: %s\n", filename);
  }

  size = static_cast<size_t>(end_pos);
  file.seekg(0, std::ios::beg);

  if (size == 0) {
    file.close();
    return nullptr;
  }

  unsigned char* file_contents = secure_allocate<unsigned char>(size);
  if (file_contents == nullptr) {
    log_secure_fatal("(secure_read_from_file) Memory allocation failed.\n");
  }

  file.read(reinterpret_cast<char*>(file_contents), static_cast<std::streamsize>(size));
  if (!file) {
    log_secure_fatal("(secure_read_from_file) Cannot read from file: %s\n", filename);
  }

  file.close();
  return file_contents;
}

void restore_dumpable_state() {
  try {
    relax_code();
  } catch (...) {
    log_secure_error("Failed to restore dumpable state.\n");
  }
}
} // namespace

/* secure data struct */
SecureStronghold_t::SecureStronghold_t()
    : is_authenticated(false),
      stronghold_mutex(),
      secret_size(MAX_PASSWORD_SIZE),
      secret(secure_allocate<char>(MAX_PASSWORD_SIZE)),
      api_key_size(0),
      api_key(nullptr),
      pkey(nullptr) {
  secure_code();
  try {
    secure_mlock(this, sizeof(*this));
    relax_code();
  } catch (...) {
    restore_dumpable_state();
    throw;
  }
}

SecureStronghold_t::~SecureStronghold_t() {
  secure_code();
  try {
    std::unique_lock<std::mutex> lock(stronghold_mutex);
    is_authenticated = false;

    if (pkey != nullptr) {
      dencryption::freePrivateKey(pkey);
      pkey = nullptr;
    }

    secure_delete<char>(api_key, api_key_size);
    api_key = nullptr;
    api_key_size = 0;

    secure_delete<char>(secret, secret_size);
    secret = nullptr;
    secret_size = 0;

    lock.unlock();
    secure_munlock(this, sizeof(*this));
    relax_code();
  } catch (...) {
    restore_dumpable_state();
  }
}

/* Safely set secret value from user prompt */
void SecureStronghold_t::authenticate() {
  secure_code();
  try {
    std::unique_lock<std::mutex> lock(stronghold_mutex);

    if (!isatty(STDIN_FILENO)) {
      log_secure_fatal("%s\n", FATAL_NON_TTY_AUTH);
    }

    is_authenticated = false;

    if (pkey != nullptr) {
      dencryption::freePrivateKey(pkey);
      pkey = nullptr;
    }

    secure_delete<char>(api_key, api_key_size);
    api_key = nullptr;
    api_key_size = 0;

    while (true) {
      const auto start_time = std::chrono::steady_clock::now();
      secure_zero_memory(secret, secret_size);

      log_info("%s", AUTHENTICATION_PROMPT);
      {
        TerminalEchoGuard no_echo;
        std::cin.getline(secret, static_cast<std::streamsize>(secret_size));
      }
      log_info("\n");

      if (std::cin.fail()) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        log_secure_fatal("%s\n", FATAL_AUTH);
      }

      EVP_PKEY* loaded_key = dencryption::loadPrivateKey(
          cuwacunu::piaabo::dconfig::config_space_t::Ed25519_pkey().c_str(),
          static_cast<const char*>(secret));

      if (loaded_key == nullptr) {
        const auto min_duration = std::chrono::seconds(3);
        const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start_time);
        if (elapsed < min_duration) {
          std::this_thread::sleep_for(min_duration - elapsed);
        }
        log_secure_warn("%s\n", WRONG_AUTH);
        continue;
      }

      pkey = loaded_key;
      break;
    }

    uint32_t random_sleep = 0;
    if (RAND_bytes(reinterpret_cast<unsigned char*>(&random_sleep), sizeof(random_sleep)) != 1) {
      random_sleep = 0;
    }
    std::this_thread::sleep_for(
        std::chrono::milliseconds(11 + static_cast<int>(random_sleep % 311)));

    log_secure_info("%s\n", CORRECT_AUTH);

    const std::string api_key_filename = cuwacunu::piaabo::dconfig::config_space_t::api_key();

    size_t api_key_filesize = 0;

    ScopedSecureArray<unsigned char> api_key_filecontents;
    api_key_filecontents.adopt(
        secure_read_from_file(api_key_filename.c_str(), api_key_filesize), api_key_filesize);

    if (api_key_filesize == 0 || api_key_filecontents.data() == nullptr) {
      log_secure_fatal(
          "Empty Exchange API Key file: %s, please follow instructions on ../config/README.md\n",
          api_key_filename.c_str());
    }
    if (!dencryption::is_aead_blob(api_key_filecontents.data(), api_key_filesize)) {
      log_secure_fatal(
          "Non-compliant Exchange API Key file: %s, expected AEAD envelope format\n",
          api_key_filename.c_str());
    }

    size_t decrypted_len = 0;
    ScopedSecureArray<unsigned char> decrypted;
    unsigned char* decrypted_ptr = dencryption::aead_decrypt_blob(
        api_key_filecontents.data(), api_key_filesize, static_cast<const char*>(secret),
        decrypted_len);
    if (decrypted_ptr == nullptr) {
      log_secure_fatal("Failed to decrypt AEAD API key file: %s\n", api_key_filename.c_str());
    }
    decrypted.adopt(decrypted_ptr, decrypted_len);

    const size_t new_api_key_size = decrypted_len + 1;
    char* new_api_key = secure_allocate<char>(new_api_key_size);
    if (decrypted_len > 0) {
      memcpy(new_api_key, reinterpret_cast<const char*>(decrypted.data()), decrypted_len);
    }
    new_api_key[decrypted_len] = '\0';
    api_key = new_api_key;
    api_key_size = new_api_key_size;

    size_t encrypted_len = 0;
    unsigned char* encrypted_ptr = dencryption::aead_encrypt_blob(
        decrypted.data(), decrypted_len, static_cast<const char*>(secret), encrypted_len);
    if (encrypted_ptr == nullptr) {
      log_secure_fatal("Failed to encrypt API key file: %s\n", api_key_filename.c_str());
    }

    ScopedSecureArray<unsigned char> encrypted;
    encrypted.adopt(encrypted_ptr, encrypted_len);

    secure_write_to_file(api_key_filename.c_str(), encrypted.data(), encrypted_len);

    is_authenticated = true;
    relax_code();
  } catch (...) {
    restore_dumpable_state();
    throw;
  }
}

/* retrive the api_key */
std::string SecureStronghold_t::which_api_key() {
  std::lock_guard<std::mutex> lock(stronghold_mutex);
  if (api_key == nullptr || api_key_size == 0) {
    return "";
  }
  return std::string(api_key);
}

/* Safely sign key data from key file */
std::string SecureStronghold_t::Ed25519_signMessage(const std::string& message) {
  secure_code();
  try {
    std::string signature;
    {
      std::unique_lock<std::mutex> lock(stronghold_mutex);
      if (!is_authenticated || pkey == nullptr) {
        log_secure_fatal("%s\n", FATAL_NO_AUTH_SIGNATURE);
      }
      signature = cuwacunu::piaabo::dencryption::Ed25519_signMessage(message, pkey);
    }
    relax_code();
    return cuwacunu::piaabo::dencryption::base64Encode(signature);
  } catch (...) {
    restore_dumpable_state();
    throw;
  }
}

/* initialize */
SecureStronghold_t SecureVault = SecureStronghold_t();
} /* namespace dsecurity */
} /* namespace piaabo */
} /* namespace cuwacunu */
