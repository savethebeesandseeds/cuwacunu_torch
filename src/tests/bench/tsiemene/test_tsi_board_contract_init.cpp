// test_tsi_board_contract_init.cpp

#include <iostream>
#include <string_view>

#include <torch/torch.h>

#include "camahjucunu/types/types_data.h"
#include "piaabo/dconfig.h"
#include "tsiemene/board.contract.h"
#include "tsiemene/board.contract.init.h"
#include "tsiemene/board.h"

int main() try {
  const char* config_folder = "/cuwacunu/src/config/";
  cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
  cuwacunu::piaabo::dconfig::config_space_t::update_config();

  using Datatype = cuwacunu::camahjucunu::exchange::kline_t;
  using Sampler = torch::data::samplers::SequentialSampler;

  const auto init = tsiemene::invoke_board_contract_init_from_default_config<Datatype, Sampler>(
      torch::Device(torch::kCPU));
  if (!init.ok) {
    std::cerr << "[test_tsi_board_contract_init] init failed: " << init.error << "\n";
    return 1;
  }
  if (init.canonical_action != std::string(tsiemene::kBoardContractInitCanonicalAction)) {
    std::cerr << "[test_tsi_board_contract_init] canonical action mismatch\n";
    return 1;
  }
  if (init.board.contracts.empty()) {
    std::cerr << "[test_tsi_board_contract_init] empty contracts\n";
    return 1;
  }

  for (std::size_t i = 0; i < init.board.contracts.size(); ++i) {
    const auto& c = init.board.contracts[i];
    for (const std::string_view key : tsiemene::BoardContract::required_dsl_keys()) {
      const auto* value = c.find_dsl_segment(key);
      if (!value || value->empty()) {
        std::cerr << "[test_tsi_board_contract_init] contract[" << i
                  << "] missing required DSL key=" << key << "\n";
        return 1;
      }
    }
  }

  tsiemene::BoardIssue issue{};
  if (!tsiemene::validate_board(init.board, &issue)) {
    std::cerr << "[test_tsi_board_contract_init] invalid board: "
              << issue.circuit_issue.what << "\n";
    return 1;
  }

  std::cout << "[test_tsi_board_contract_init] pass contracts="
            << init.board.contracts.size()
            << " action=" << init.canonical_action
            << " config=" << init.source_config_path << "\n";
  return 0;
} catch (const std::exception& e) {
  std::cerr << "[test_tsi_board_contract_init] exception: " << e.what() << "\n";
  return 1;
}

