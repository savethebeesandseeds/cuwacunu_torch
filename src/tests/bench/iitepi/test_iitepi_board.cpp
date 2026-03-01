// test_iitepi_board.cpp
// Demonstration: explicit board init, bind resolution, and compatibility checks.

#include <iostream>
#include <string>
#include <string_view>

#include "camahjucunu/dsl/tsiemene_board/tsiemene_board.h"
#include "iitepi/iitepi.h"
#include "iitepi/board/board.builder.h"
#include "iitepi/board/board.validation.h"

namespace {

bool expect(bool cond, std::string_view message) {
  if (!cond) {
    std::cerr << "[demo:iitepi_board] FAIL: " << message << "\n";
    return false;
  }
  return true;
}

const cuwacunu::camahjucunu::tsiemene_board_bind_decl_t* find_bind(
    const cuwacunu::camahjucunu::tsiemene_board_instruction_t& instruction,
    const std::string& binding_id) {
  for (const auto& bind : instruction.binds) {
    if (bind.id == binding_id) return &bind;
  }
  return nullptr;
}

const cuwacunu::camahjucunu::tsiemene_board_contract_decl_t* find_contract_decl(
    const cuwacunu::camahjucunu::tsiemene_board_instruction_t& instruction,
    const std::string& contract_id) {
  for (const auto& decl : instruction.contracts) {
    if (decl.id == contract_id) return &decl;
  }
  return nullptr;
}

}  // namespace

int main() try {
  cuwacunu::iitepi::config_space_t::change_config_file("/cuwacunu/src/config/");
  cuwacunu::iitepi::config_space_t::update_config();
  cuwacunu::iitepi::board_space_t::init();
  cuwacunu::iitepi::board_space_t::assert_locked_runtime_intact_or_fail_fast();

  const std::string board_hash = cuwacunu::iitepi::board_space_t::locked_board_hash();
  const std::string binding_id =
      cuwacunu::iitepi::board_space_t::locked_board_binding_id();
  const auto board_itself =
      cuwacunu::iitepi::board_space_t::board_itself(board_hash);
  const auto& board_instruction = board_itself->board.decoded();
  const auto* bind = find_bind(board_instruction, binding_id);

  bool ok = true;
  ok = ok && expect(bind != nullptr, "selected board binding exists");
  if (!ok) return 1;

  const auto* contract_decl = find_contract_decl(board_instruction, bind->contract_ref);
  ok = ok && expect(contract_decl != nullptr, "binding CONTRACT exists");
  if (!ok) return 1;

  const auto contract_hash =
      cuwacunu::iitepi::board_space_t::contract_hash_for_binding(
          board_hash, binding_id);
  const auto wave_hash =
      cuwacunu::iitepi::board_space_t::wave_hash_for_binding(
          board_hash, binding_id);
  const auto contract_itself =
      cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash);
  const auto wave_itself =
      cuwacunu::iitepi::wave_space_t::wave_itself(wave_hash);

  const auto& circuit_instruction = contract_itself->circuit.decoded();
  const auto& wave_set = wave_itself->wave.decoded();
  const auto* selected_wave =
      tsiemene::board_builder::select_wave_by_id(wave_set, bind->wave_ref);

  ok = ok && expect(!circuit_instruction.circuits.empty(), "contract has at least one circuit");
  ok = ok && expect(selected_wave != nullptr, "selected wave exists");
  ok = ok && expect(selected_wave && selected_wave->epochs > 0, "wave epochs > 0");
  ok = ok && expect(selected_wave && selected_wave->batch_size > 0, "wave batch_size > 0");
  if (!ok) return 1;

  const auto contract_report =
      tsiemene::validate_contract_definition(circuit_instruction, contract_hash);
  const auto wave_report =
      tsiemene::validate_wave_definition(*selected_wave, contract_hash);
  const auto compat_report = tsiemene::validate_wave_contract_compatibility(
      circuit_instruction,
      *selected_wave,
      contract_hash,
      &contract_itself->jkimyei.decoded(),
      contract_decl->id,
      selected_wave->name);
  ok = ok && expect(contract_report.ok, "contract validation is ok");
  ok = ok && expect(wave_report.ok, "wave validation is ok");
  ok = ok && expect(compat_report.ok, "compatibility validation is ok");
  if (!ok) return 1;

  std::cout << "[demo:iitepi_board] board_hash=" << board_hash << "\n";
  std::cout << "[demo:iitepi_board] binding=" << binding_id << "\n";
  std::cout << "[demo:iitepi_board] contract_hash=" << contract_hash
            << " wave_hash=" << wave_hash << "\n";
  std::cout << "[demo:iitepi_board] circuits=" << circuit_instruction.circuits.size()
            << " wave=" << selected_wave->name
            << " epochs=" << selected_wave->epochs
            << " batch_size=" << selected_wave->batch_size
            << " max_batches_per_epoch=" << selected_wave->max_batches_per_epoch
            << "\n";
  return 0;
} catch (const std::exception& e) {
  std::cerr << "[demo:iitepi_board] exception: " << e.what() << "\n";
  return 1;
}
