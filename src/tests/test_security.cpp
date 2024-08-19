#include "piaabo/dutils.h"
#include "piaabo/security.h"

int main() {
  cuwacunu::piaabo::security::SecureVault.authenticate();
  cuwacunu::piaabo::security::SecureVault.Ed25519_signMessage("test");

  return 0;
}
