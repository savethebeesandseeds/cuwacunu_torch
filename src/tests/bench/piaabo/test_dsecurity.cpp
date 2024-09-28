#include "piaabo/dutils.h"
#include "piaabo/dconfig.h"
#include "piaabo/dsecurity.h"

int main() {
  /* update config */
  const char* config_folder = "/src/config";
  update_config(config_folder);

  /* update config */
  cuwacunu::piaabo::dsecurity::SecureVault.authenticate();
  cuwacunu::piaabo::dsecurity::SecureVault.Ed25519_signMessage("test");

  return 0;
}
