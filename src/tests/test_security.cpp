#include "piaabo/dutils.h"
#include "piaabo/security.h"

int main() {
  log_info("waka1\n");
  cuwacunu::piaabo::security::SecureVault.authenticate();
  cuwacunu::piaabo::security::SecureVault.Ed25519_signMessage("waka");
  log_info("waka2\n");

  return 0;
}
