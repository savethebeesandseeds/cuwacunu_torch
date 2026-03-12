// test_iitepi_board.cpp
// Demonstration: load config -> init board -> run one configured binding.

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "camahjucunu/dsl/canonical_path/canonical_path.h"
#include "iitepi/iitepi.h"
#include "iitepi/board/board.contract.init.h"
#include "iitepi/board/board.contract.h"

namespace {

const char* value_or_empty(const std::string& value) {
  return value.empty() ? "<empty>" : value.c_str();
}

bool expect(bool cond, std::string_view message) {
  if (!cond) {
    log_err("[demo:iitepi_board] FAIL: %s\n", std::string(message).c_str());
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

const cuwacunu::camahjucunu::iitepi_wave_t* find_wave(
    const cuwacunu::camahjucunu::iitepi_wave_set_t& wave_set,
    const std::string& wave_id) {
  for (const auto& wave : wave_set.waves) {
    if (wave.name == wave_id) return &wave;
  }
  return nullptr;
}

void print_run_failure(
    const cuwacunu::iitepi::board_binding_run_record_t& run) {
  log_err("[demo:iitepi_board] run.ok=false\n");
  log_err("[demo:iitepi_board] run.canonical_action=%s\n",
          value_or_empty(run.canonical_action));
  log_err("[demo:iitepi_board] run.error=%s\n", value_or_empty(run.error));
  log_err("[demo:iitepi_board] run.board_hash=%s\n",
          value_or_empty(run.board_hash));
  log_err("[demo:iitepi_board] run.board_binding_id=%s\n",
          value_or_empty(run.board_binding_id));
  log_err("[demo:iitepi_board] run.contract_hash=%s\n",
          value_or_empty(run.contract_hash));
  log_err("[demo:iitepi_board] run.wave_hash=%s\n",
          value_or_empty(run.wave_hash));
  log_err("[demo:iitepi_board] run.record_type=%s\n",
          value_or_empty(run.resolved_record_type));
  log_err("[demo:iitepi_board] run.sampler=%s\n",
          value_or_empty(run.resolved_sampler));
  log_err("[demo:iitepi_board] run.source_config_path=%s\n",
          value_or_empty(run.source_config_path));
  log_err("[demo:iitepi_board] run.total_steps=%llu\n",
          static_cast<unsigned long long>(run.total_steps));
  std::ostringstream contract_steps;
  contract_steps << "[";
  for (std::size_t i = 0; i < run.contract_steps.size(); ++i) {
    if (i > 0) contract_steps << ",";
    contract_steps << run.contract_steps[i];
  }
  contract_steps << "]";
  log_err("[demo:iitepi_board] run.contract_steps=%s\n",
          contract_steps.str().c_str());
  log_err("[demo:iitepi_board] run.board_contracts=%zu\n",
          run.board.contracts.size());
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

  log_info("[demo:iitepi_board] board_hash=%s\n", board_hash.c_str());
  log_info("[demo:iitepi_board] binding=%s\n", binding_id.c_str());

  const auto board_itself =
      cuwacunu::iitepi::board_space_t::board_itself(board_hash);
  const auto& board_instruction = board_itself->board.decoded();
  const auto* bind = find_bind(board_instruction, binding_id);
  bool ok = true;
  ok = ok && expect(bind != nullptr, "binding missing in board instruction");
  if (!ok) return 1;

  const auto* contract_decl = find_contract_decl(board_instruction, bind->contract_ref);
  ok = ok && expect(contract_decl != nullptr, "binding contract missing in board instruction");
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
  const cuwacunu::camahjucunu::iitepi_wave_wikimyei_decl_t* selected_wikimyei =
      nullptr;
  std::string selected_representation_hashimyei{};
  ok = ok && expect(selected_wave != nullptr, "binding wave missing in wave instruction");
  ok = ok && expect(selected_wave && selected_wave->sources.size() == 1,
                    "selected wave must define exactly one SOURCE block");
  ok = ok && expect(selected_wave && selected_wave->probes.empty(),
                    "selected wave must not define removed transfer-matrix PROBE blocks");
  ok = ok && expect(selected_wave && !selected_wave->wikimyeis.empty(),
                    "selected wave must define at least one WIKIMYEI block");
  ok = ok && expect(selected_wave && !selected_wave->sinks.empty(),
                    "selected wave must define at least one SINK block");
  if (selected_wave && !selected_wave->wikimyeis.empty()) {
    for (const auto& wikimyei_decl : selected_wave->wikimyeis) {
      const auto wikimyei_path = cuwacunu::camahjucunu::decode_canonical_path(
          wikimyei_decl.wikimyei_path, contract_hash_for_check);
      if (!wikimyei_path.ok) continue;
      if (wikimyei_path.hashimyei.empty()) continue;
      selected_wikimyei = &wikimyei_decl;
      selected_representation_hashimyei = wikimyei_path.hashimyei;
      break;
    }
    ok = ok && expect(selected_wikimyei != nullptr,
                      "selected wave must include at least one hashimyei-scoped WIKIMYEI");
    ok = ok && expect(!selected_representation_hashimyei.empty(),
                      "selected wave WIKIMYEI missing hashimyei suffix");
  }
  if (!ok) return 1;

  // 3) Initialize runtime board for the selected binding.
  auto init = tsiemene::invoke_board_contract_init_from_snapshot(
      board_hash, binding_id, board_itself);
  if (!init.ok) {
    log_err("[demo:iitepi_board] init.ok=false error=%s\n",
            value_or_empty(init.error));
  }
  ok = ok && expect(init.ok, "board.contract@init failed");
  ok = ok && expect(init.error.empty(), "board.contract@init has errors");
  ok = ok && expect(!init.board.contracts.empty(), "runtime board has no contracts");
  if (!ok) return 1;

  std::string payload_error;
  std::string expected_sources_dsl;
  std::string expected_channels_dsl;
  cuwacunu::camahjucunu::observation_spec_t expected_observation{};
  const bool payload_ok = tsiemene::board_builder::load_wave_dataloader_observation_payloads(
      wave_itself,
      *selected_wave,
      &expected_sources_dsl,
      &expected_channels_dsl,
      &expected_observation,
      &payload_error);
  ok = ok && expect(payload_ok, "wave dataloader observation payload decode failed");
  if (!payload_ok) {
    log_err("[demo:iitepi_board] payload_error=%s\n", payload_error.c_str());
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
                    "sources DSL payload does not match selected wave sources file");
  ok = ok && expect(expected_channels_dsl == read_text_file(resolved_channels_path),
                    "channels DSL payload does not match selected wave channels file");
  if (!ok) return 1;

  const auto* built_circuit_dsl = init.board.contracts.front().find_dsl_segment(
      tsiemene::kBoardContractCircuitDslKey);
  const auto* built_wave_dsl = init.board.contracts.front().find_dsl_segment(
      tsiemene::kBoardContractWaveDslKey);
  ok = ok && expect(built_circuit_dsl != nullptr, "board contract missing circuit DSL segment");
  ok = ok && expect(built_wave_dsl != nullptr, "board contract missing wave DSL segment");
  if (!ok) return 1;

  std::string expected_record_type;
  std::string expected_record_error;
  ok = ok && expect(
      tsiemene::resolve_active_record_type_from_observation(
          expected_observation, &expected_record_type, &expected_record_error),
      "effective record_type inference failed");
  if (!ok) {
    log_err("[demo:iitepi_board] expected_record_error=%s\n",
            expected_record_error.c_str());
    return 1;
  }

  const std::string expected_sampler = lower_ascii(trim_ascii(selected_wave->sampler));
  ok = ok && expect(init.resolved_record_type == expected_record_type,
                    "resolved record_type does not follow wave-selected channels DSL");
  ok = ok && expect(init.resolved_sampler == expected_sampler,
                    "resolved sampler does not follow wave root SAMPLER");
  if (!ok) return 1;

  // 4) Run configured binding explicitly (no implicit binding default in run API).
  const auto run = cuwacunu::iitepi::run_binding(binding_id);
  if (!run.ok) {
    print_run_failure(run);
  }

  ok = ok && expect(run.ok, "binding run failed");
  ok = ok && expect(run.error.empty(), "binding run has runtime error");
  ok = ok && expect(run.board_hash == board_hash, "run record board hash mismatches lock");
  ok = ok && expect(run.board_binding_id == binding_id,
                    "run record binding id mismatches lock");
  ok = ok && expect(!run.contract_hash.empty(), "run record missing contract hash");
  ok = ok && expect(!run.wave_hash.empty(), "run record missing wave hash");
  ok = ok && expect(!run.contract_steps.empty(), "run record missing contract step counts");
  ok = ok && expect(run.total_steps > 0, "run record reports zero executed steps");
  if (!ok) return 1;

  log_info(
      "[demo:iitepi_board] contract_hash=%s wave_hash=%s record_type=%s sampler=%s contracts=%zu total_steps=%llu\n",
      run.contract_hash.c_str(),
      run.wave_hash.c_str(),
      run.resolved_record_type.c_str(),
      run.resolved_sampler.c_str(),
      run.contract_steps.size(),
      static_cast<unsigned long long>(run.total_steps));

  // 5) Transfer-matrix evaluation is now an ephemeral report: no hashimyei-owned
  // persisted report-fragment files are expected as side effects.
  ok = ok && expect(!selected_representation_hashimyei.empty(),
                    "selected wave representation hashimyei is empty");
  if (!ok) return 1;

  return 0;
} catch (const std::exception& e) {
  log_err("[demo:iitepi_board] exception: %s\n", e.what());
  return 1;
}
