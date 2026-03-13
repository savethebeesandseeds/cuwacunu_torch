#include <exception>
#include <iostream>
#include <string>

#include "hero/runtime_dev_loop.h"
#include "iitepi/board/board.contract.init.h"
#include "iitepi/iitepi.h"

namespace {

const char* value_or_empty(const std::string& value) {
  return value.empty() ? "<empty>" : value.c_str();
}

void print_help(const char* argv0) {
  std::cerr << "Usage: " << argv0
            << " [--config-folder <path>] [--binding <binding_id>]"
            << " [--reset-runtime-state]\n"
            << "Defaults:\n"
            << "  --config-folder " << DEFAULT_CONFIG_FOLDER << "\n";
}

}  // namespace

int main(int argc, char** argv) {
  std::string config_folder = DEFAULT_CONFIG_FOLDER;
  std::string binding_override;
  bool reset_runtime_state_flag = false;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--config-folder" && i + 1 < argc) {
      config_folder = argv[++i];
      continue;
    }
    if (arg == "--binding" && i + 1 < argc) {
      binding_override = argv[++i];
      continue;
    }
    if (arg == "--reset-runtime-state") {
      reset_runtime_state_flag = true;
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
    cuwacunu::iitepi::config_space_t::change_config_file(config_folder.c_str());
    cuwacunu::iitepi::config_space_t::update_config();

    const bool reset_runtime_state =
        reset_runtime_state_flag ||
        cuwacunu::iitepi::config_space_t::get<bool>(
            "GENERAL", "reset_runtime_state_at_start", false);
    if (reset_runtime_state) {
      cuwacunu::hero::runtime_dev::runtime_reset_result_t reset{};
      std::string reset_error;
      if (!cuwacunu::hero::runtime_dev::reset_runtime_state_from_active_config(
              &reset, &reset_error)) {
        std::cerr << "[main_board] runtime reset failed: " << reset_error << "\n";
        return 1;
      }
      log_info(
          "[main_board] runtime reset ok removed_store_roots=%zu removed_catalogs=%zu removed_entries=%llu\n",
          reset.removed_store_roots.size(),
          reset.removed_catalog_paths.size(),
          static_cast<unsigned long long>(reset.removed_store_entries));
      for (const auto& path : reset.removed_store_roots) {
        log_info("[main_board] runtime reset store_root=%s\n", path.string().c_str());
      }
      for (const auto& path : reset.removed_catalog_paths) {
        log_info("[main_board] runtime reset catalog_path=%s\n",
                 path.string().c_str());
      }
    }

    cuwacunu::iitepi::board_space_t::init();
    cuwacunu::iitepi::board_space_t::assert_locked_runtime_intact_or_fail_fast();

    const std::string board_hash =
        cuwacunu::iitepi::board_space_t::locked_board_hash();
    const std::string binding_id =
        binding_override.empty()
            ? cuwacunu::iitepi::board_space_t::locked_board_binding_id()
            : binding_override;

    log_info("[main_board] board_hash=%s binding=%s\n",
             value_or_empty(board_hash), value_or_empty(binding_id));

    const auto run = cuwacunu::iitepi::run_binding(binding_id);
    if (!run.ok) {
      log_err(
          "[main_board] run failed board_hash=%s binding=%s error=%s\n",
          value_or_empty(run.board_hash),
          value_or_empty(run.board_binding_id),
          value_or_empty(run.error));
      log_err(
          "[main_board] contract_hash=%s wave_hash=%s record_type=%s sampler=%s total_steps=%llu\n",
          value_or_empty(run.contract_hash),
          value_or_empty(run.wave_hash),
          value_or_empty(run.resolved_record_type),
          value_or_empty(run.resolved_sampler),
          static_cast<unsigned long long>(run.total_steps));
      return 1;
    }

    log_info(
        "[main_board] run ok board_hash=%s binding=%s contract_hash=%s wave_hash=%s record_type=%s sampler=%s contracts=%zu total_steps=%llu\n",
        value_or_empty(run.board_hash),
        value_or_empty(run.board_binding_id),
        value_or_empty(run.contract_hash),
        value_or_empty(run.wave_hash),
        value_or_empty(run.resolved_record_type),
        value_or_empty(run.resolved_sampler),
        run.contract_steps.size(),
        static_cast<unsigned long long>(run.total_steps));
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "[main_board] exception: " << e.what() << "\n";
    return 1;
  }
}
