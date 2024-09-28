#include "piaabo/dutils.h"
#include "piaabo/dconfig.h"

int main() {
  const char* config_folder = "/src/config";
  update_config(config_folder);

  /* */
  log_info("%s\n", (*environment_config)["ACTIVE_SYMBOLS"].c_str());
  std::cout << "Press Enter to stop execution...";
  std::cin.get();
  
  /* */
  update_config(config_folder);
  log_info("%s\n", (*environment_config)["ACTIVE_SYMBOLS"].c_str());

  log_info("Test success.\n");
  
  return 0;
}