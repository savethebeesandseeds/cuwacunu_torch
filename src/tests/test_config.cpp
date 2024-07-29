#include "piaabo/dutils.h"
#include "piaabo/config_space.h"

int main() {
  log_info("%s\n", (*environment_config)["ACTIVE_SYMBOLS"].c_str());
  std::cout << "Press Enter to stop execution...";
  std::cin.get();
  update_config();
  log_info("%s\n", (*environment_config)["ACTIVE_SYMBOLS"].c_str());

  log_info("Test success.\n");
  
  return 0;
}