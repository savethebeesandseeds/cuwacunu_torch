#include <exception>
#include <iostream>
#include <string>

#include "hero/runtime_dev_loop.h"
#include "iitepi/iitepi.h"

namespace {

void print_help(const char* argv0) {
  std::cerr << "Usage: " << argv0 << " [--global-config <path>]\n"
            << "Defaults:\n"
            << "  --global-config " << DEFAULT_GLOBAL_CONFIG_PATH << "\n";
}

}  // namespace

int main(int argc, char** argv) {
  std::string global_config_path = DEFAULT_GLOBAL_CONFIG_PATH;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--global-config" && i + 1 < argc) {
      global_config_path = argv[++i];
      continue;
    }
    if (arg == "--help" || arg == "-h") {
      print_help(argv[0]);
      return 0;
    }
    std::cerr << "Unknown argument: " << arg << "\n";
    print_help(argv[0]);
    return 2;
  }

  try {
    cuwacunu::iitepi::config_space_t::change_config_file(
        global_config_path.c_str());
    cuwacunu::iitepi::config_space_t::update_config();

    cuwacunu::hero::runtime_dev::runtime_reset_result_t reset{};
    std::string error;
    if (!cuwacunu::hero::runtime_dev::reset_runtime_state_from_active_config(
            &reset, &error)) {
      std::cerr << "[runtime_reset] failed: " << error << "\n";
      return 1;
    }

    std::cout << "runtime_reset=ok\n";
    std::cout << "removed_store_entries=" << reset.removed_store_entries << "\n";
    for (const auto& path : reset.removed_store_roots) {
      std::cout << "removed_store_root=" << path.string() << "\n";
    }
    for (const auto& path : reset.removed_catalog_paths) {
      std::cout << "removed_catalog_path=" << path.string() << "\n";
    }
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "[runtime_reset] exception: " << e.what() << "\n";
    return 1;
  }
}
