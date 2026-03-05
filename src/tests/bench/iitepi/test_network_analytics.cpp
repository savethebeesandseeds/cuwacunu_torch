// test_network_analytics.cpp
// Demonstration: load config -> init board -> emit network design analytics.

#include <iostream>

#include "iitepi/iitepi.h"

int main() try {
  cuwacunu::iitepi::config_space_t::change_config_file("/cuwacunu/src/config/");
  cuwacunu::iitepi::config_space_t::update_config();

  cuwacunu::iitepi::board_space_t::init();
  cuwacunu::iitepi::board_space_t::assert_locked_runtime_intact_or_fail_fast();
  cuwacunu::iitepi::board_space_t::network_analytics(&std::cout, /*beautify=*/true);

  return 0;
} catch (const std::exception& e) {
  std::cerr << "[test_network_analytics] exception: " << e.what() << "\n";
  return 1;
}
