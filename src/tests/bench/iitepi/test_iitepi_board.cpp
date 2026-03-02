// test_iitepi_board.cpp
// Demonstration: load config -> init board -> run one configured binding.

#include <iostream>
#include <string_view>

#include "iitepi/iitepi.h"
#include "iitepi/board/board.contract.init.h"

namespace {

bool expect(bool cond, std::string_view message) {
  if (!cond) {
    std::cerr << "[demo:iitepi_board] FAIL: " << message << "\n";
    return false;
  }
  return true;
}

}  // namespace

int main() try {
  // 1) Load config.
  cuwacunu::iitepi::config_space_t::change_config_file("/cuwacunu/src/config/");
  cuwacunu::iitepi::config_space_t::update_config();

  // 2) Init board from config lock.
  cuwacunu::iitepi::board_space_t::init();
  cuwacunu::iitepi::board_space_t::assert_locked_runtime_intact_or_fail_fast();

  const auto board_hash = cuwacunu::iitepi::board_space_t::locked_board_hash();
  const auto binding_id =
      cuwacunu::iitepi::board_space_t::locked_board_binding_id();

  std::cout << "[demo:iitepi_board] board_hash=" << board_hash << "\n";
  std::cout << "[demo:iitepi_board] binding=" << binding_id << "\n";

  // 3) Run configured binding explicitly (no implicit binding default in run API).
  const auto run = cuwacunu::iitepi::run_binding(binding_id);

  bool ok = true;
  ok = ok && expect(run.ok, "binding run succeeded");
  ok = ok && expect(run.error.empty(), "binding run has no runtime error");
  ok = ok && expect(run.board_hash == board_hash, "run record board hash matches lock");
  ok = ok && expect(run.board_binding_id == binding_id,
                    "run record binding id matches lock");
  ok = ok && expect(!run.contract_hash.empty(), "run record contains contract hash");
  ok = ok && expect(!run.wave_hash.empty(), "run record contains wave hash");
  ok = ok && expect(!run.contract_steps.empty(), "run record contains contract step counts");
  ok = ok && expect(run.total_steps > 0, "run record reports executed steps");
  if (!ok) return 1;

  std::cout << "[demo:iitepi_board] contract_hash=" << run.contract_hash
            << " wave_hash=" << run.wave_hash
            << " record_type=" << run.resolved_record_type
            << " sampler=" << run.resolved_sampler
            << " contracts=" << run.contract_steps.size()
            << " total_steps=" << run.total_steps << "\n";

  return 0;
} catch (const std::exception& e) {
  std::cerr << "[demo:iitepi_board] exception: " << e.what() << "\n";
  return 1;
}
