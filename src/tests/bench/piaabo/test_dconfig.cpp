#include "piaabo/dutils.h"
#include "piaabo/dconfig.h"

int main() {
  const char* config_folder = "/src/config/";
  cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
  cuwacunu::piaabo::dconfig::config_space_t::update_config();

  /* */
  log_info("Reading any configuration field: %s\n", cuwacunu::piaabo::dconfig::config_space_t::websocket_url().c_str());
  std::cout << "Press Enter to stop execution...";
  std::cin.get();
  
  /* */
  cuwacunu::piaabo::dconfig::config_space_t::update_config();
  log_info("Reading any configuration field: %s\n", cuwacunu::piaabo::dconfig::config_space_t::websocket_url().c_str());

  log_info("Test success.\n");
  
  return 0;
}