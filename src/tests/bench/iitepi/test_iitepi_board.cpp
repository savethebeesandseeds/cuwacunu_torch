// test_iitepi_board.cpp
// Demonstration: load config -> init board -> run one configured binding.

#include <iostream>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "iitepi/iitepi.h"
#include "iitepi/board/board.contract.init.h"
#include "iitepi/board/board.contract.h"

namespace {

bool expect(bool cond, std::string_view message) {
  if (!cond) {
    std::cerr << "[demo:iitepi_board] FAIL: " << message << "\n";
    return false;
  }
  return true;
}

std::string read_text_file(const std::string& path) {
  std::ifstream in(path);
  if (!in.is_open()) {
    throw std::runtime_error("failed to open file: " + path);
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

std::string trim_ascii(std::string s) {
  std::size_t b = 0;
  while (b < s.size() &&
         static_cast<unsigned char>(s[b]) <= static_cast<unsigned char>(' ')) {
    ++b;
  }
  std::size_t e = s.size();
  while (e > b &&
         static_cast<unsigned char>(s[e - 1]) <=
             static_cast<unsigned char>(' ')) {
    --e;
  }
  return s.substr(b, e - b);
}

std::string lower_ascii(std::string s) {
  for (char& ch : s) {
    if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
  }
  return s;
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

const cuwacunu::camahjucunu::tsiemene_wave_t* find_wave(
    const cuwacunu::camahjucunu::tsiemene_wave_set_t& wave_set,
    const std::string& wave_id) {
  for (const auto& wave : wave_set.waves) {
    if (wave.name == wave_id) return &wave;
  }
  return nullptr;
}

void print_run_failure(
    const cuwacunu::iitepi::board_binding_run_record_t& run) {
  std::cerr << "[demo:iitepi_board] run.ok=false\n";
  std::cerr << "[demo:iitepi_board] run.canonical_action="
            << run.canonical_action << "\n";
  std::cerr << "[demo:iitepi_board] run.error="
            << (run.error.empty() ? "<empty>" : run.error) << "\n";
  std::cerr << "[demo:iitepi_board] run.board_hash="
            << (run.board_hash.empty() ? "<empty>" : run.board_hash) << "\n";
  std::cerr << "[demo:iitepi_board] run.board_binding_id="
            << (run.board_binding_id.empty() ? "<empty>" : run.board_binding_id)
            << "\n";
  std::cerr << "[demo:iitepi_board] run.contract_hash="
            << (run.contract_hash.empty() ? "<empty>" : run.contract_hash)
            << "\n";
  std::cerr << "[demo:iitepi_board] run.wave_hash="
            << (run.wave_hash.empty() ? "<empty>" : run.wave_hash) << "\n";
  std::cerr << "[demo:iitepi_board] run.record_type="
            << (run.resolved_record_type.empty() ? "<empty>"
                                                : run.resolved_record_type)
            << "\n";
  std::cerr << "[demo:iitepi_board] run.sampler="
            << (run.resolved_sampler.empty() ? "<empty>" : run.resolved_sampler)
            << "\n";
  std::cerr << "[demo:iitepi_board] run.source_config_path="
            << (run.source_config_path.empty() ? "<empty>"
                                               : run.source_config_path)
            << "\n";
  std::cerr << "[demo:iitepi_board] run.total_steps=" << run.total_steps << "\n";
  std::cerr << "[demo:iitepi_board] run.contract_steps=[";
  for (std::size_t i = 0; i < run.contract_steps.size(); ++i) {
    if (i > 0) std::cerr << ",";
    std::cerr << run.contract_steps[i];
  }
  std::cerr << "]\n";
  std::cerr << "[demo:iitepi_board] run.board_contracts="
            << run.board.contracts.size() << "\n";
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

  const auto board_itself =
      cuwacunu::iitepi::board_space_t::board_itself(board_hash);
  const auto& board_instruction = board_itself->board.decoded();
  const auto* bind = find_bind(board_instruction, binding_id);
  bool ok = true;
  ok = ok && expect(bind != nullptr, "binding exists in board instruction");
  if (!ok) return 1;

  const auto* contract_decl = find_contract_decl(board_instruction, bind->contract_ref);
  ok = ok && expect(contract_decl != nullptr, "binding contract exists in board instruction");
  if (!ok) return 1;

  const std::string contract_path =
      (std::filesystem::path(board_itself->config_folder) / contract_decl->file).string();
  const auto contract_hash_for_check =
      cuwacunu::iitepi::contract_space_t::register_contract_file(contract_path);
  const auto wave_hash_for_check =
      cuwacunu::iitepi::board_space_t::wave_hash_for_binding(board_hash, binding_id);
  const auto contract_itself =
      cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash_for_check);
  const auto wave_itself =
      cuwacunu::iitepi::wave_space_t::wave_itself(wave_hash_for_check);
  const auto& wave_set = wave_itself->wave.decoded();
  const auto* selected_wave = find_wave(wave_set, bind->wave_ref);
  ok = ok && expect(selected_wave != nullptr, "binding wave exists in wave instruction");
  ok = ok && expect(selected_wave && selected_wave->sources.size() == 1,
                    "selected wave has exactly one SOURCE block");
  if (!ok) return 1;

  auto init = tsiemene::invoke_board_contract_init_from_snapshot(
      board_hash, binding_id, board_itself);
  if (!init.ok) {
    std::cerr << "[demo:iitepi_board] init.ok=false error="
              << (init.error.empty() ? "<empty>" : init.error) << "\n";
  }
  ok = ok && expect(init.ok, "board.contract@init succeeded");
  ok = ok && expect(init.error.empty(), "board.contract@init has no error");
  ok = ok && expect(!init.board.contracts.empty(), "runtime board has contracts");
  if (!ok) return 1;

  std::string payload_error;
  std::string expected_sources_dsl;
  std::string expected_channels_dsl;
  cuwacunu::camahjucunu::observation_spec_t expected_observation{};
  const bool payload_ok = tsiemene::board_builder::load_wave_dataloader_observation_payloads(
      contract_itself,
      wave_itself,
      *selected_wave,
      &expected_sources_dsl,
      &expected_channels_dsl,
      &expected_observation,
      &payload_error);
  ok = ok && expect(payload_ok, "wave dataloader observation payload decode succeeded");
  if (!payload_ok) {
    std::cerr << "[demo:iitepi_board] payload_error=" << payload_error << "\n";
    return 1;
  }

  const std::string resolved_sources_path =
      (std::filesystem::path(wave_itself->config_folder) /
       selected_wave->sources.front().sources_dsl_file)
          .lexically_normal()
          .string();
  const std::string resolved_channels_path =
      (std::filesystem::path(wave_itself->config_folder) /
       selected_wave->sources.front().channels_dsl_file)
          .lexically_normal()
          .string();
  ok = ok && expect(expected_sources_dsl == read_text_file(resolved_sources_path),
                    "sources DSL payload matches selected wave sources file");
  ok = ok && expect(expected_channels_dsl == read_text_file(resolved_channels_path),
                    "channels DSL payload matches selected wave channels file");
  if (!ok) return 1;

  const auto* built_sources_dsl = init.board.contracts.front().find_dsl_segment(
      tsiemene::kBoardContractObservationSourcesDslKey);
  const auto* built_channels_dsl = init.board.contracts.front().find_dsl_segment(
      tsiemene::kBoardContractObservationChannelsDslKey);
  ok = ok && expect(built_sources_dsl != nullptr, "board contract stores sources DSL segment");
  ok = ok && expect(built_channels_dsl != nullptr, "board contract stores channels DSL segment");
  ok = ok && expect(built_sources_dsl && *built_sources_dsl == expected_sources_dsl,
                    "board contract sources DSL segment uses wave-selected payload");
  ok = ok && expect(built_channels_dsl && *built_channels_dsl == expected_channels_dsl,
                    "board contract channels DSL segment uses wave-selected payload");
  if (!ok) return 1;

  std::string expected_record_type;
  std::string expected_record_error;
  ok = ok && expect(
      tsiemene::resolve_active_record_type_from_observation(
          expected_observation, &expected_record_type, &expected_record_error),
      "effective record_type inference succeeded");
  if (!ok) {
    std::cerr << "[demo:iitepi_board] expected_record_error="
              << expected_record_error << "\n";
    return 1;
  }

  const std::string expected_sampler = lower_ascii(trim_ascii(selected_wave->sampler));
  ok = ok && expect(init.resolved_record_type == expected_record_type,
                    "resolved record_type follows wave-selected channels DSL");
  ok = ok && expect(init.resolved_sampler == expected_sampler,
                    "resolved sampler follows wave root SAMPLER");
  if (!ok) return 1;

  // 3) Run configured binding explicitly (no implicit binding default in run API).
  const auto run = cuwacunu::iitepi::run_binding(binding_id);
  if (!run.ok) {
    print_run_failure(run);
  }

  ok = ok && expect(run.ok, "binding run did not succeeded");
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
