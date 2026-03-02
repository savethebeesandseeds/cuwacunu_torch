// test_iitepi_board.cpp
// Demonstration: load config -> init board -> run one configured binding.

#include <iostream>
#include <optional>
#include <string>
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
  const auto default_binding_id =
      cuwacunu::iitepi::board_space_t::locked_board_binding_id();
  const auto board_itself = cuwacunu::iitepi::board_space_t::board_itself(board_hash);
  const auto& board_instruction = board_itself->board.decoded();

  std::optional<std::string> alternate_binding_id;
  for (const auto& bind : board_instruction.binds) {
    if (bind.id != default_binding_id) {
      alternate_binding_id = bind.id;
      break;
    }
  }

  std::cout << "[demo:iitepi_board] board_hash=" << board_hash << "\n";
  std::cout << "[demo:iitepi_board] default_binding=" << default_binding_id
            << "\n";
  if (alternate_binding_id.has_value()) {
    std::cout << "[demo:iitepi_board] alternate_binding="
              << *alternate_binding_id << "\n";
  }

  // 3) Run default binding with inferred runtime typing.
  const auto run = tsiemene::invoke_board_binding_run_from_locked_runtime();

  bool ok = true;
  ok = ok && expect(run.ok, "binding run succeeded");
  ok = ok && expect(run.error.empty(), "binding run has no runtime error");
  ok = ok && expect(run.board_hash == board_hash, "run record board hash matches lock");
  ok = ok && expect(run.board_binding_id == default_binding_id,
                    "run record binding id matches lock");
  ok = ok && expect(!run.contract_hash.empty(), "run record contains contract hash");
  ok = ok && expect(!run.wave_hash.empty(), "run record contains wave hash");
  ok = ok && expect(!run.contract_steps.empty(), "run record contains contract step counts");
  ok = ok && expect(run.total_steps > 0, "run record reports executed steps");
  if (!ok) return 1;

  std::cout << "[demo:iitepi_board] default contract_hash=" << run.contract_hash
            << " wave_hash=" << run.wave_hash
            << " record_type=" << run.resolved_record_type
            << " sampler=" << run.resolved_sampler
            << " contracts=" << run.contract_steps.size()
            << " total_steps=" << run.total_steps << "\n";

  // 4) Optionally run a second binding explicitly to demonstrate no binding lock.
  if (alternate_binding_id.has_value()) {
    const auto run_alt = tsiemene::invoke_board_binding_run_from_locked_runtime(
        *alternate_binding_id);
    ok = true;
    ok = ok && expect(run_alt.ok, "alternate binding run succeeded");
    ok = ok && expect(run_alt.error.empty(),
                      "alternate binding run has no runtime error");
    ok = ok && expect(run_alt.board_hash == board_hash,
                      "alternate run board hash matches lock");
    ok = ok && expect(run_alt.board_binding_id == *alternate_binding_id,
                      "alternate run binding id matches request");
    ok = ok && expect(run_alt.total_steps > 0,
                      "alternate run reports executed steps");
    if (!ok) return 1;

    std::cout << "[demo:iitepi_board] alternate contract_hash="
              << run_alt.contract_hash << " wave_hash=" << run_alt.wave_hash
              << " record_type=" << run_alt.resolved_record_type
              << " sampler=" << run_alt.resolved_sampler
              << " contracts=" << run_alt.contract_steps.size()
              << " total_steps=" << run_alt.total_steps << "\n";
  }

  return 0;
} catch (const std::exception& e) {
  std::cerr << "[demo:iitepi_board] exception: " << e.what() << "\n";
  return 1;
}
