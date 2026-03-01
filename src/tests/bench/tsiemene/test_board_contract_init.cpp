// test_board_contract_init.cpp

#include <iostream>
#include <filesystem>
#include <string_view>

#include <torch/torch.h>

#include "camahjucunu/types/types_data.h"
#include "piaabo/dconfig.h"
#include "iitepi/board/board.contract.h"
#include "iitepi/board/board.contract.init.h"
#include "iitepi/board/board.h"

int main() try {
  const char* config_folder = "/cuwacunu/src/config/";
  cuwacunu::iitepi::config_space_t::change_config_file(config_folder);
  cuwacunu::iitepi::config_space_t::update_config();

  const std::string configured_board_path = cuwacunu::iitepi::config_space_t::get<
      std::string>("GENERAL", GENERAL_BOARD_CONFIG_KEY);
  const std::string configured_binding_id = cuwacunu::iitepi::config_space_t::get<
      std::string>("GENERAL", GENERAL_BOARD_BINDING_KEY);
  const std::filesystem::path board_path(configured_board_path);
  const std::string resolved_board_path = board_path.is_absolute()
      ? board_path.string()
      : (std::filesystem::path(cuwacunu::iitepi::config_space_t::config_folder) /
         board_path)
            .string();

  using Datatype = cuwacunu::camahjucunu::exchange::kline_t;
  using Sampler = torch::data::samplers::SequentialSampler;

  const auto init = tsiemene::invoke_board_contract_init_from_file<Datatype, Sampler>(
      resolved_board_path, configured_binding_id, torch::Device(torch::kCPU));
  if (!init.ok) {
    if (!torch::cuda::is_available() &&
        init.error.find("requires CUDA but CUDA is unavailable") != std::string::npos) {
      std::cout << "[test_board_contract_init] skip (CUDA unavailable): " << init.error
                << "\n";
      return 0;
    }
    std::cerr << "[test_board_contract_init] init failed: " << init.error << "\n";
    return 1;
  }
  if (init.canonical_action != std::string(tsiemene::kBoardContractInitCanonicalAction)) {
    std::cerr << "[test_board_contract_init] canonical action mismatch\n";
    return 1;
  }
  if (init.board.contracts.empty()) {
    std::cerr << "[test_board_contract_init] empty contracts\n";
    return 1;
  }

  for (std::size_t i = 0; i < init.board.contracts.size(); ++i) {
    const auto& c = init.board.contracts[i];
    for (const std::string_view key : tsiemene::BoardContract::required_dsl_keys()) {
      const auto* value = c.find_dsl_segment(key);
      if (!value || value->empty()) {
        std::cerr << "[test_board_contract_init] contract[" << i
                  << "] missing required DSL key=" << key << "\n";
        return 1;
      }
    }
  }

  tsiemene::BoardIssue issue{};
  if (!tsiemene::validate_board(init.board, &issue)) {
    std::cerr << "[test_board_contract_init] invalid board: "
              << issue.circuit_issue.what << "\n";
    return 1;
  }

  std::cout << "[test_board_contract_init] pass contracts="
            << init.board.contracts.size()
            << " action=" << init.canonical_action
            << " config=" << init.source_config_path << "\n";
  return 0;
} catch (const std::exception& e) {
  std::cerr << "[test_board_contract_init] exception: " << e.what() << "\n";
  return 1;
}
