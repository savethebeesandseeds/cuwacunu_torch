#include <exception>
#include <iostream>
#include <string>

#include "iitepi/board/board.contract.init.h"
#include "iitepi/iitepi.h"

namespace {

const char* value_or_empty(const std::string& value) {
  return value.empty() ? "<empty>" : value.c_str();
}

void print_help(const char* argv0) {
  std::cerr << "Usage: " << argv0
            << " [--config-folder <path>] [--binding <binding_id>]\n"
            << "Defaults:\n"
            << "  --config-folder " << DEFAULT_CONFIG_FOLDER << "\n";
}

}  // namespace

int main(int argc, char** argv) {
  std::string config_folder = DEFAULT_CONFIG_FOLDER;
  std::string binding_override;

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

    cuwacunu::iitepi::board_space_t::init();
    cuwacunu::iitepi::board_space_t::assert_locked_runtime_intact_or_fail_fast();

    const std::string board_hash =
        cuwacunu::iitepi::board_space_t::locked_board_hash();
    const std::string binding_id =
        binding_override.empty()
            ? cuwacunu::iitepi::board_space_t::locked_board_binding_id()
            : binding_override;

    log_info("[hero_board] board_hash=%s binding=%s\n",
             value_or_empty(board_hash), value_or_empty(binding_id));

    const auto run = cuwacunu::iitepi::run_binding(binding_id);
    if (!run.ok) {
      log_err(
          "[hero_board] run failed board_hash=%s binding=%s error=%s\n",
          value_or_empty(run.board_hash),
          value_or_empty(run.board_binding_id),
          value_or_empty(run.error));
      log_err(
          "[hero_board] contract_hash=%s wave_hash=%s record_type=%s sampler=%s total_steps=%llu\n",
          value_or_empty(run.contract_hash),
          value_or_empty(run.wave_hash),
          value_or_empty(run.resolved_record_type),
          value_or_empty(run.resolved_sampler),
          static_cast<unsigned long long>(run.total_steps));
      return 1;
    }

    log_info(
        "[hero_board] run ok board_hash=%s binding=%s contract_hash=%s wave_hash=%s record_type=%s sampler=%s contracts=%zu total_steps=%llu\n",
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
    std::cerr << "[hero_board] exception: " << e.what() << "\n";
    return 1;
  }
}
