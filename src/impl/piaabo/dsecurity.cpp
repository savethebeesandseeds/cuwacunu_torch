#include "piaabo/dsecurity.h"

#define AUTHENTICATION_PROMPT "Enter Password:"
#define WRONG_AUTH "Wrong password."
#define CORRECT_AUTH "Authentication success."
#define FATAL_AUTH "Password was too long and has been truncated, fatal termination."
#define FATAL_NO_AUTH_SIGNATURE "Trying to sign a message without prior authentication, fatal termination."
#define MAX_PASSWORD_SIZE 1024
#define FILESALT "SEED"

RUNTIME_WARNING("(dsecurity.h)[] secure the secret with hardware \n");
RUNTIME_WARNING("(dsecurity.h)[] add second factor authentication \n");

namespace cuwacunu {
namespace piaabo {
namespace dsecurity {
/* secure all current and future memory */ void secure_all_code()                       { if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) { log_secure_fatal("Total Memory locking failed. \n"); } else { log_secure_dbg("Locking all program memory. \n"); } }
/* Reset all current and future memory */  void relax_all_code()                        { if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) { log_secure_fatal("Failed to unlock all Memory. \n"); } else { log_secure_warn("Unlocking all program memory. \n"); } }
/* Set process as non-dumpable */          void secure_non_dumpable_code()              { if (prctl(PR_SET_DUMPABLE, 0) != 0) { log_secure_fatal("Failed to set process as non-dumpable. \n"); } }
/* Reset process dumpable status */        void relax_non_dumpable_code()               { if (prctl(PR_SET_DUMPABLE, 1) != 0) { log_secure_fatal("Failed to reset process dumpable status. \n"); } }
/* Lock the memory to prevent swapping */  void secure_mlock(const void *data, size_t size)   { if (mlock(data, size)   != 0) { log_secure_fatal("Memory locking failed. \n"); } }
/* Unlock the memory */                    void secure_munlock(const void *data, size_t size) { if (munlock(data, size) != 0) { log_secure_fatal("Memory unlocking failed. \n"); } }

/* Secure set memory to zero */
void secure_zero_memory(void *ptr, size_t size) {
  volatile char* volatile_ptr = reinterpret_cast<volatile char*>(ptr);
  while (size--) { *volatile_ptr++ = 0; }
}

/* secure allocate variable */
template <typename T>
T* secure_allocate(size_t data_size) {
  T* data = new T[data_size];
  if (!data) {
    log_secure_fatal("(secure_allocate) Memory allocation failed.\n");
    return nullptr;
  }
  secure_zero_memory(data, data_size * sizeof(T));
  secure_mlock(data, data_size * sizeof(T));
  return data;
}

/* secure dealocate variable */
template <typename T>
void secure_delete(T* data, size_t data_size) {
  if (data != nullptr) {
    secure_zero_memory(data, data_size * sizeof(T));
    secure_munlock(data, data_size * sizeof(T));
    delete[] data;
  }
}

/* Write to file securing memory */
void secure_write_to_file(const char* filename, const unsigned char* data, size_t size) {
  std::ofstream file(filename, std::ios::out | std::ios::binary | std::ios::trunc);
  if (!file) { log_secure_fatal("(secure_write_to_file) Cannot open file to write: %s\n", filename); }

  file.write(reinterpret_cast<const char*>(data), size);
  if (!file) { log_secure_fatal("(secure_write_to_file) Cannot write to file: %s\n", filename); }
  file.close();
}

/* Read from file securing memory */
unsigned char* secure_read_from_file(const char* filename, size_t& size) {
  std::ifstream file(filename, std::ios::in | std::ios::binary);
  if (!file) { log_secure_fatal("(secure_read_from_file) Cannot open file to read: %s\n", filename); }

  file.seekg(0, std::ios::end);
  size = file.tellg();
  file.seekg(0, std::ios::beg);

  unsigned char* fileContents = secure_allocate<unsigned char>(size);

  if (!fileContents) { log_secure_fatal("(secure_read_from_file) Memory allocation failed.\n"); }

  file.read(reinterpret_cast<char*>(fileContents), size);
  if (!file) { log_secure_fatal("(secure_read_from_file) Cannot read from file: %s\n", filename); }
  file.close();
  return fileContents;
}

/* secure data struct */
SecureStronghold_t::SecureStronghold_t() 
  : is_authenticated(false),
    stronghold_mutex(), 
    secret_size(MAX_PASSWORD_SIZE), 
    secret(secure_allocate<char>(secret_size)),
    api_key(secure_allocate<char>(secret_size)) {
  /* constructor */
  secure_non_dumpable_code();
  secure_mlock(this, sizeof(*this));
  relax_non_dumpable_code();
}

SecureStronghold_t::~SecureStronghold_t() {
  secure_non_dumpable_code();
  /* Securely wipe the variables */
  dencryption::freePrivateKey(pkey);
  secure_delete<char>(secret, secret_size);
  secure_delete<char>(api_key, secret_size);
  secret_size = 0;
  munlock(this, sizeof(*this));
  relax_non_dumpable_code();
}

/* Safely set secret value from user prompt */
void SecureStronghold_t::authenticate() {
  secure_non_dumpable_code();
  is_authenticated = false;
  stronghold_mutex.lock();
  /* local variables */
  struct termios oldt, newt;
  auto startTime = std::chrono::steady_clock::now();
  /* reset the secret */
  memset(secret, 0, secret_size);
  /* --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- -- */
  /*                   request user prompt                          */
  /* --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- -- */
  /* Save current terminal settings */
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~ECHO;  /* Disable terminal typing echoing */
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  /* --- --- Prompt user for input --- --- */
  log_info("%s", AUTHENTICATION_PROMPT);
  std::cin.getline(secret, secret_size);
  /* --- --- --- --- - --- --- --- --- --- */
  
  /* Validate user prompt */
  if (std::cin.fail()) {
    std::cin.clear(); /* Clears the error flag */
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); /* Ignores the rest of the line */
    stronghold_mutex.unlock();
    /* return on fatal */
    log_secure_fatal("%s\n", FATAL_AUTH);
    
    /* reucurrent call */
    stronghold_mutex.unlock();
    authenticate();
    return;
  }
  
  /* Input has been retrived - Restore old terminal settings */
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  log_info("\n");

  /* --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- -- */
  /*            validate against ssl private file                   */
  /* --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- -- */
  /* read private key file */
  pkey = dencryption::loadPrivateKey(cuwacunu::piaabo::dconfig::config_space_t::Ed25519_pkey().c_str(), static_cast<const char*>(secret));

  if (!pkey) {
    auto minDuration = std::chrono::seconds(3);
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - startTime);
    if (elapsed < minDuration) {
      std::this_thread::sleep_for(minDuration - elapsed);
    }
    log_secure_warn("%s\n", WRONG_AUTH);
    
    /* reucurrent call */
    stronghold_mutex.unlock();
    authenticate();
    return;
  }

  /* short delay to avoid side-channel timing attacks */
  unsigned int random_sleep;
  std::this_thread::sleep_for(std::chrono::milliseconds(11 + RAND_bytes(reinterpret_cast<unsigned char*>(&random_sleep), sizeof(random_sleep)) && (random_sleep % 311)));

  /* notify correct auth */
  log_secure_info("%s\n", CORRECT_AUTH);
  /* --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- -- */
  /*               Read and decode api key file                     */
  /* --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- -- */
  std::string api_key_filename = cuwacunu::piaabo::dconfig::config_space_t::api_key();
  std::string aes_salt_filename = cuwacunu::piaabo::dconfig::config_space_t::aes_salt();
  
  unsigned char* key       = secure_allocate<unsigned char>(AES_KEY_LEN);
  unsigned char* salt      = secure_allocate<unsigned char>(AES_SALT_LEN);
  unsigned char* iv        = secure_allocate<unsigned char>(AES_BLOCK_SIZE);
  unsigned char* iv_backup = secure_allocate<unsigned char>(AES_BLOCK_SIZE);
  
  size_t api_key_filesize = 0, aes_salt_filesize = 0;

  /* read api key contents */
  unsigned char* api_key_filecontents = secure_read_from_file(api_key_filename.c_str(), api_key_filesize);
  unsigned char* aes_salt_filecontents = secure_read_from_file(aes_salt_filename.c_str(), aes_salt_filesize);

  /* encription variables */
  unsigned char* decrypted;
  unsigned char* encrypted;
  size_t decrypted_len = 0, encrypted_len = 0;

  if(aes_salt_filesize == 0) {
    /* fresh file, should contain the plain text api key */
    decrypted_len = api_key_filesize;
    decrypted = secure_allocate<unsigned char>(decrypted_len);
    
    /* no need to decrypt the api key */
    memcpy(decrypted, api_key_filecontents, api_key_filesize);
  } else {
    /* known file, should contain the AES encrypted api key */
    if(aes_salt_filesize != AES_SALT_LEN) {
      secure_delete<unsigned char>(key, AES_KEY_LEN);
      secure_delete<unsigned char>(salt, AES_SALT_LEN);
      secure_delete<unsigned char>(iv, AES_BLOCK_SIZE);
      secure_delete<unsigned char>(iv_backup, AES_BLOCK_SIZE);
      api_key_filesize  = 0;
      aes_salt_filesize = 0;

      secure_delete<unsigned char>(api_key_filecontents, api_key_filesize);
      secure_delete<unsigned char>(aes_salt_filecontents, aes_salt_filesize);
      api_key_filesize  = 0;
      aes_salt_filesize = 0;

      stronghold_mutex.unlock();
      relax_non_dumpable_code();
      log_secure_fatal("Non-complient Exchange API Key file: %s, please follow instructions on ../config/README.md\n", api_key_filename.c_str());
    }
    /* assign the salt */
    memcpy(salt, aes_salt_filecontents, AES_SALT_LEN);

    /* decrypt the api key*/
    dencryption::derive_key_iv(static_cast<const char*>(secret), key, iv_backup, salt);
    memcpy(iv, iv_backup, AES_BLOCK_SIZE);   /* Restore IV from backup before decryption */
    decrypted = dencryption::aes_decrypt(api_key_filecontents, api_key_filesize, key, iv, decrypted_len);
  }

  /* save the value of the api_key */
  memcpy(api_key, reinterpret_cast<char*>(decrypted), decrypted_len);
  api_key[decrypted_len] = '\0';

  /* Encrypt the file back with a new salt (this makes the file change everytime is read) */
  RAND_bytes(salt, AES_SALT_LEN);
  
  /* encrypt the api key */
  dencryption::derive_key_iv(static_cast<const char*>(secret), key, iv_backup, salt);
  memcpy(iv, iv_backup, AES_BLOCK_SIZE);   /* Restore IV from backup before decryption */
  encrypted = dencryption::aes_encrypt(decrypted, decrypted_len, key, iv, encrypted_len);

  /* save the key back to the file */
  secure_write_to_file(api_key_filename.c_str(), encrypted, encrypted_len);

  /* save the salt to a file */
  secure_write_to_file(aes_salt_filename.c_str(), salt, AES_SALT_LEN);

  /* --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- -- */
  /*                      Finalize                                  */
  /* --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- -- */
  secure_delete<unsigned char>(key, AES_KEY_LEN);
  secure_delete<unsigned char>(salt, AES_SALT_LEN);
  secure_delete<unsigned char>(iv, AES_BLOCK_SIZE);
  secure_delete<unsigned char>(iv_backup, AES_BLOCK_SIZE);
  api_key_filesize  = 0;
  aes_salt_filesize = 0;

  secure_delete<unsigned char>(decrypted, decrypted_len);
  secure_delete<unsigned char>(encrypted, encrypted_len);
  decrypted_len = 0;
  encrypted_len = 0;

  secure_delete<unsigned char>(api_key_filecontents, api_key_filesize);
  secure_delete<unsigned char>(aes_salt_filecontents, aes_salt_filesize);
  api_key_filesize  = 0;
  aes_salt_filesize = 0;

  is_authenticated = true;
  stronghold_mutex.unlock();
  relax_non_dumpable_code();
}

/* retrive the api_key */
std::string SecureStronghold_t::which_api_key() {
  return std::string(api_key);
}

/* Safely sign key data from key file */
std::string SecureStronghold_t::Ed25519_signMessage(const std::string& message) {
  secure_non_dumpable_code();
  if(!is_authenticated) {
    log_secure_fatal("%s\n", FATAL_NO_AUTH_SIGNATURE);
  }
  stronghold_mutex.lock();
  std::string signature = cuwacunu::piaabo::dencryption::Ed25519_signMessage(message, pkey);
  stronghold_mutex.unlock();
  relax_non_dumpable_code();

  return cuwacunu::piaabo::dencryption::base64Encode(signature);
}
/* initialize */
SecureStronghold_t SecureVault = SecureStronghold_t();
} /* namespace dsecurity */
} /* namespace piaabo */
} /* namespace cuwacunu */
