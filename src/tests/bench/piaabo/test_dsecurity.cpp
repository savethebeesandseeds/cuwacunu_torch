#include "piaabo/dutils.h"
#include "piaabo/dconfig.h"
#include "piaabo/dsecurity.h"

int main() {
  std::string message = "test";
  /* update config */
  const char* config_folder = "/cuwacunu/src/config/";
  cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
  cuwacunu::piaabo::dconfig::config_space_t::update_config();

  /* authenticate user */
  log_warn("\n\t password is: test\n\n");
  cuwacunu::piaabo::dsecurity::SecureVault.authenticate();

  /* print */
  TICK(sign_message);
  std::string signature = cuwacunu::piaabo::dsecurity::SecureVault.Ed25519_signMessage(message);
  PRINT_TOCK_ns(sign_message);

  log_info("Message: [%s], under key: [%s] has signature: [%s].\n", 
    message.c_str(), cuwacunu::piaabo::dconfig::config_space_t::api_key().c_str(), signature.c_str());

  return 0;
}
