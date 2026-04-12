#include "hero_config_tools_internal.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <sstream>
#include <unordered_set>
#include <vector>

#include "camahjucunu/dsl/iitepi_campaign/iitepi_campaign.h"
#include "camahjucunu/dsl/iitepi_wave/iitepi_wave.h"
#include "camahjucunu/dsl/jkimyei_specs/jkimyei_specs.h"
#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state.h"
#include "camahjucunu/dsl/network_design/network_design.h"
#include "camahjucunu/dsl/observation_pipeline/observation_channels_decoder.h"
#include "camahjucunu/dsl/observation_pipeline/observation_sources_decoder.h"
#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit.h"
#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit_runtime.h"
#include "camahjucunu/dsl/marshal_objective/marshal_objective.h"
#include "hero/hashimyei_hero/hero_hashimyei_tools.h"
#include "hero/lattice_hero/hero_lattice_tools.h"
#include "hero/runtime_hero/hero_runtime_tools.h"
#include "hero/marshal_hero/hero_marshal_tools.h"
#include "iitepi/contract_space_t.h"

namespace cuwacunu::hero::mcp::detail {

enum class instruction_dsl_validation_family_e : std::uint8_t {
  Unsupported = 0,
  LatentLineageState,
  MarshalObjective,
  NetworkDesign,
  Jkimyei,
  IitepiCircuit,
  IitepiContract,
  IitepiWave,
  IitepiCampaign,
  ObservationChannels,
  ObservationSources,
};

[[nodiscard]] std::string instruction_dsl_validation_family_name(
    instruction_dsl_validation_family_e family) {
  switch (family) {
    case instruction_dsl_validation_family_e::LatentLineageState:
      return "latent_lineage_state";
    case instruction_dsl_validation_family_e::MarshalObjective:
      return "marshal_objective";
    case instruction_dsl_validation_family_e::NetworkDesign:
      return "network_design";
    case instruction_dsl_validation_family_e::Jkimyei:
      return "jkimyei";
    case instruction_dsl_validation_family_e::IitepiCircuit:
      return "iitepi_circuit";
    case instruction_dsl_validation_family_e::IitepiContract:
      return "iitepi_contract";
    case instruction_dsl_validation_family_e::IitepiWave:
      return "iitepi_wave";
    case instruction_dsl_validation_family_e::IitepiCampaign:
      return "iitepi_campaign";
    case instruction_dsl_validation_family_e::ObservationChannels:
      return "observation_channels";
    case instruction_dsl_validation_family_e::ObservationSources:
      return "observation_sources";
    case instruction_dsl_validation_family_e::Unsupported:
    default:
      return "unsupported";
  }
}

[[nodiscard]] std::filesystem::path resolve_config_root_from_global_config(
    const hero_config_store_t& store) {
  std::string repo_root{};
  if (read_ini_general_key(store.global_config_path(), "repo_root",
                           &repo_root)) {
    repo_root = trim_ascii(repo_root);
    if (!repo_root.empty()) {
      return (resolve_path_near_config(repo_root, store.global_config_path()) /
              "src/config")
          .lexically_normal();
    }
  }

  const std::string scope_raw =
      trim_ascii(store.get_or_default("config_scope_root"));
  if (!scope_raw.empty()) {
    return resolve_path_near_config(scope_raw, store.config_path())
        .lexically_normal();
  }
  return std::filesystem::path(store.config_path())
      .parent_path()
      .lexically_normal();
}

[[nodiscard]] instruction_dsl_validation_family_e
detect_instruction_dsl_validation_family(const std::filesystem::path& dsl_path) {
  const std::string file_name = lowercase_copy(dsl_path.filename().string());
  if (lowercase_copy(dsl_path.extension().string()) != ".dsl") {
    return instruction_dsl_validation_family_e::Unsupported;
  }
  if (file_name.find("tsi.source.dataloader.channels") != std::string::npos) {
    return instruction_dsl_validation_family_e::ObservationChannels;
  }
  if (file_name.find("tsi.source.dataloader.sources") != std::string::npos) {
    return instruction_dsl_validation_family_e::ObservationSources;
  }
  if (is_marshal_objective_dsl_path(dsl_path)) {
    return instruction_dsl_validation_family_e::MarshalObjective;
  }
  if (file_name.find("network_design") != std::string::npos) {
    return instruction_dsl_validation_family_e::NetworkDesign;
  }
  if (file_name.find("jkimyei") != std::string::npos) {
    return instruction_dsl_validation_family_e::Jkimyei;
  }
  if (file_name.rfind("iitepi.contract", 0) == 0 ||
      file_name.find(".contract.") != std::string::npos) {
    return instruction_dsl_validation_family_e::IitepiContract;
  }
  if (file_name.find("iitepi.circuit") != std::string::npos) {
    return instruction_dsl_validation_family_e::IitepiCircuit;
  }
  if (file_name == "iitepi.waves.dsl" ||
      file_name.rfind("default.iitepi.wave", 0) == 0 ||
      file_name.rfind("iitepi.wave", 0) == 0 ||
      file_name.rfind("iitepi.waves", 0) == 0) {
    return instruction_dsl_validation_family_e::IitepiWave;
  }
  if (is_campaign_dsl_path(dsl_path)) {
    return instruction_dsl_validation_family_e::IitepiCampaign;
  }
  return instruction_dsl_validation_family_e::LatentLineageState;
}

[[nodiscard]] std::filesystem::path grammar_path_for_instruction_dsl_family(
    const std::filesystem::path& config_root,
    instruction_dsl_validation_family_e family) {
  switch (family) {
    case instruction_dsl_validation_family_e::LatentLineageState:
      return config_root / "bnf" / "latent_lineage_state.authored.bnf";
    case instruction_dsl_validation_family_e::MarshalObjective:
      return config_root / "bnf" / "objective.marshal.bnf";
    case instruction_dsl_validation_family_e::NetworkDesign:
      return config_root / "bnf" / "network_design.bnf";
    case instruction_dsl_validation_family_e::Jkimyei:
      return config_root / "bnf" / "jkimyei.bnf";
    case instruction_dsl_validation_family_e::IitepiCircuit:
      return config_root / "bnf" / "iitepi.circuit.bnf";
    case instruction_dsl_validation_family_e::IitepiContract:
      return config_root / "bnf" / "iitepi.contract.bnf";
    case instruction_dsl_validation_family_e::IitepiWave:
      return config_root / "bnf" / "iitepi.wave.bnf";
    case instruction_dsl_validation_family_e::IitepiCampaign:
      return config_root / "bnf" / "iitepi.campaign.bnf";
    case instruction_dsl_validation_family_e::ObservationChannels:
      return config_root / "bnf" / "tsi.source.dataloader.channels.bnf";
    case instruction_dsl_validation_family_e::ObservationSources:
      return config_root / "bnf" / "tsi.source.dataloader.sources.bnf";
    case instruction_dsl_validation_family_e::Unsupported:
    default:
      return {};
  }
}

[[nodiscard]] std::vector<std::string> associated_man_name_candidates(
    const std::filesystem::path& dsl_path,
    instruction_dsl_validation_family_e family) {
  std::vector<std::string> candidates{};
  auto push_candidate = [&](std::string name) {
    name = trim_ascii(name);
    if (name.empty()) return;
    if (std::find(candidates.begin(), candidates.end(), name) !=
        candidates.end()) {
      return;
    }
    candidates.push_back(std::move(name));
  };

  std::string file_name = dsl_path.filename().string();
  if (file_name.rfind("default.", 0) == 0) {
    file_name.erase(0, std::string("default.").size());
  }
  if (file_name.size() > 4 &&
      file_name.compare(file_name.size() - 4, 4, ".dsl") == 0) {
    push_candidate(file_name.substr(0, file_name.size() - 4) + ".man");
  }

  const std::string lowered = lowercase_copy(file_name);
  if (lowered.rfind("hero.config", 0) == 0) {
    push_candidate("hero.config.man");
  } else if (lowered.rfind("hero.runtime", 0) == 0) {
    push_candidate("hero.runtime.config.man");
  } else if (lowered.rfind("hero.marshal", 0) == 0) {
    push_candidate("hero.marshal.config.man");
  } else if (lowered.rfind("hero.human", 0) == 0) {
    push_candidate("hero.human.config.man");
  } else if (lowered.rfind("hero.hashimyei", 0) == 0) {
    push_candidate("hero.hashimyei.config.man");
  } else if (lowered.rfind("hero.lattice", 0) == 0) {
    push_candidate("hero.lattice.config.man");
  }

  if (lowered.find("iitepi.circuit") != std::string::npos) {
    push_candidate("iitepi.circuit.man");
  }
  if (lowered.find("tsi.source.dataloader.channels") != std::string::npos) {
    push_candidate("tsi.source.dataloader.channels.man");
  }
  if (lowered.find("tsi.source.dataloader.sources") != std::string::npos) {
    push_candidate("tsi.source.dataloader.sources.man");
  }
  if (lowered.find("tsi.wikimyei.representation.vicreg") !=
      std::string::npos) {
    push_candidate("tsi.wikimyei.representation.vicreg.man");
  }
  if (lowered.find("tsodao") != std::string::npos) {
    push_candidate("tsodao.man");
  }

  switch (family) {
    case instruction_dsl_validation_family_e::MarshalObjective:
      push_candidate("marshal.objective.man");
      break;
    case instruction_dsl_validation_family_e::IitepiContract:
      push_candidate("iitepi.contract.man");
      break;
    case instruction_dsl_validation_family_e::IitepiCircuit:
      push_candidate("iitepi.circuit.man");
      break;
    case instruction_dsl_validation_family_e::IitepiWave:
      push_candidate("iitepi.wave.man");
      break;
    case instruction_dsl_validation_family_e::IitepiCampaign:
      push_candidate("iitepi.campaign.man");
      break;
    case instruction_dsl_validation_family_e::Jkimyei:
      push_candidate("jkimyei.man");
      break;
    case instruction_dsl_validation_family_e::ObservationChannels:
      push_candidate("tsi.source.dataloader.channels.man");
      break;
    case instruction_dsl_validation_family_e::ObservationSources:
      push_candidate("tsi.source.dataloader.sources.man");
      break;
    case instruction_dsl_validation_family_e::NetworkDesign:
      push_candidate("network_design.man");
      break;
    case instruction_dsl_validation_family_e::LatentLineageState:
    case instruction_dsl_validation_family_e::Unsupported:
    default:
      break;
  }
  return candidates;
}

[[nodiscard]] std::filesystem::path find_associated_man_path(
    const std::filesystem::path& config_root,
    const std::filesystem::path& dsl_path,
    instruction_dsl_validation_family_e family) {
  if (config_root.empty()) return {};
  const std::filesystem::path man_root = config_root / "man";
  std::error_code ec{};
  if (!std::filesystem::exists(man_root, ec) ||
      !std::filesystem::is_directory(man_root, ec)) {
    return {};
  }
  const auto candidates = associated_man_name_candidates(dsl_path, family);
  for (const auto& candidate : candidates) {
    const std::filesystem::path man_path = man_root / candidate;
    ec.clear();
    if (std::filesystem::exists(man_path, ec) &&
        std::filesystem::is_regular_file(man_path, ec)) {
      return man_path;
    }
  }
  return {};
}

[[nodiscard]] bool append_associated_man_fields(
    std::ostringstream* out, const std::filesystem::path& man_path,
    bool include_content, std::string_view warning, std::string* out_error) {
  if (out == nullptr) return false;
  std::string man_content{};
  if (!man_path.empty() && include_content) {
    if (!read_text_file(man_path.string(), &man_content, out_error))
      return false;
  }
  *out << ",\"man_path\":"
       << (man_path.empty() ? "null" : json_quote(man_path.string()))
       << ",\"man_available\":" << bool_json(!man_path.empty());
  if (include_content) {
    *out << ",\"man_content\":"
         << (man_path.empty() ? "null" : json_quote(man_content));
  }
  if (!warning.empty()) {
    *out << ",\"warning\":" << json_quote(std::string(warning));
  }
  return true;
}

[[nodiscard]] std::string missing_associated_man_warning(
    const std::filesystem::path& dsl_path) {
  return "no associated .man found for " + dsl_path.string();
}

[[nodiscard]] bool should_warn_missing_associated_man(
    const std::filesystem::path& dsl_path,
    instruction_dsl_validation_family_e family) {
  (void)family;
  const std::string extension = lowercase_copy(dsl_path.extension().string());
  return extension == ".dsl";
}

void log_config_warning(std::string_view warning) {
  if (warning.empty()) return;
  static std::mutex warning_mutex{};
  static std::unordered_set<std::string> seen_warnings{};
  {
    std::lock_guard<std::mutex> lock(warning_mutex);
    const auto [it, inserted] = seen_warnings.emplace(warning);
    (void)it;
    if (!inserted) return;
  }
  const std::string line = "[" + std::string(kMcpServerName) + "][warning] " +
                           std::string(warning) + "\n";
  (void)write_all_fd(STDERR_FILENO, line.data(), line.size());
}

[[nodiscard]] std::filesystem::path resolve_default_project_config_root() {
  std::string repo_root{};
  if (read_ini_general_key(kDefaultGlobalConfigPath, "repo_root", &repo_root)) {
    repo_root = trim_ascii(repo_root);
    if (!repo_root.empty()) {
      return (resolve_path_near_config(repo_root, kDefaultGlobalConfigPath) /
              "src/config")
          .lexically_normal();
    }
  }
  return std::filesystem::path("/cuwacunu/src/config").lexically_normal();
}

[[nodiscard]] std::filesystem::path find_associated_man_path_with_fallback(
    const std::filesystem::path& primary_config_root,
    const std::filesystem::path& dsl_path,
    instruction_dsl_validation_family_e family) {
  std::filesystem::path man_path =
      find_associated_man_path(primary_config_root, dsl_path, family);
  if (!man_path.empty()) return man_path;
  const std::filesystem::path fallback_config_root =
      resolve_default_project_config_root();
  if (!fallback_config_root.empty() &&
      fallback_config_root.lexically_normal() !=
          primary_config_root.lexically_normal()) {
    man_path = find_associated_man_path(fallback_config_root, dsl_path, family);
  }
  return man_path;
}

[[nodiscard]] bool validate_basic_iitepi_contract_wrapper(
    std::string_view text, std::string* out_error) {
  if (out_error) out_error->clear();
  static constexpr std::string_view kBegin = "-----BEGIN IITEPI CONTRACT-----";
  static constexpr std::string_view kEnd = "-----END IITEPI CONTRACT-----";
  const std::size_t begin_pos = text.find(kBegin);
  if (begin_pos == std::string_view::npos) {
    if (out_error) {
      *out_error = "missing -----BEGIN IITEPI CONTRACT----- marker";
    }
    return false;
  }
  const std::size_t end_pos = text.find(kEnd, begin_pos + kBegin.size());
  if (end_pos == std::string_view::npos) {
    if (out_error) *out_error = "missing -----END IITEPI CONTRACT----- marker";
    return false;
  }
  if (end_pos <= begin_pos) {
    if (out_error) *out_error = "invalid IITEPI contract marker ordering";
    return false;
  }

  const std::size_t body_begin = begin_pos + kBegin.size();
  const std::string body(text.substr(body_begin, end_pos - body_begin));
  std::istringstream input(body);
  std::string raw_line{};
  bool circuit_seen = false;
  std::size_t statement_count = 0;
  while (std::getline(input, raw_line)) {
    std::string line = trim_ascii(raw_line);
    if (line.empty()) continue;
    if (line.rfind("//", 0) == 0 || line.rfind("#", 0) == 0) continue;
    ++statement_count;
    if (line.back() != ';') {
      if (out_error) {
        *out_error =
            "invalid IITEPI contract statement (missing ';'): " + line;
      }
      return false;
    }
    if (line.rfind("CIRCUIT_FILE:", 0) == 0) circuit_seen = true;
  }
  if (statement_count == 0) {
    if (out_error) *out_error = "empty IITEPI contract body";
    return false;
  }
  if (!circuit_seen) {
    if (out_error) *out_error = "missing CIRCUIT_FILE statement";
    return false;
  }
  return true;
}

[[nodiscard]] bool validate_instruction_dsl_replacement(
    const hero_config_store_t& store, const std::filesystem::path& dsl_path,
    std::string_view replacement_text, std::string* out_family_name,
    std::filesystem::path* out_grammar_path, std::string* out_error) {
  if (out_error) out_error->clear();
  if (out_family_name) out_family_name->clear();
  if (out_grammar_path) *out_grammar_path = std::filesystem::path{};

  const instruction_dsl_validation_family_e family =
      detect_instruction_dsl_validation_family(dsl_path);
  const std::string family_name = instruction_dsl_validation_family_name(family);
  if (out_family_name) *out_family_name = family_name;
  if (family == instruction_dsl_validation_family_e::Unsupported) {
    if (out_error) {
      *out_error = "instruction dsl replace validation is unsupported for this "
                   "DSL family: " +
                   dsl_path.string();
    }
    return false;
  }

  const std::filesystem::path config_root =
      resolve_config_root_from_global_config(store);
  if (config_root.empty()) {
    if (out_error) {
      *out_error = "cannot resolve config root for instruction dsl validation";
    }
    return false;
  }
  const std::filesystem::path grammar_path =
      grammar_path_for_instruction_dsl_family(config_root, family);
  if (out_grammar_path) *out_grammar_path = grammar_path;
  if (grammar_path.empty()) {
    if (out_error) {
      *out_error = "cannot resolve grammar path for instruction dsl validation";
    }
    return false;
  }

  std::string grammar_text{};
  if (!read_text_file(grammar_path.string(), &grammar_text, out_error))
    return false;

  try {
    const std::string text(replacement_text);
    switch (family) {
      case instruction_dsl_validation_family_e::LatentLineageState:
        (void)cuwacunu::camahjucunu::dsl::decode_latent_lineage_state_from_dsl(
            grammar_text, text);
        break;
      case instruction_dsl_validation_family_e::MarshalObjective:
        (void)cuwacunu::camahjucunu::dsl::decode_marshal_objective_from_dsl(
            grammar_text, text);
        break;
      case instruction_dsl_validation_family_e::NetworkDesign:
        (void)cuwacunu::camahjucunu::dsl::decode_network_design_from_dsl(
            grammar_text, text);
        break;
      case instruction_dsl_validation_family_e::Jkimyei:
        (void)cuwacunu::camahjucunu::dsl::decode_jkimyei_specs_from_dsl(
            grammar_text, text, dsl_path.string());
        break;
      case instruction_dsl_validation_family_e::IitepiCircuit: {
        cuwacunu::camahjucunu::dsl::tsiemeneCircuits decoder(grammar_text);
        const auto instruction = decoder.decode(text);
        std::string semantic_error{};
        if (!cuwacunu::camahjucunu::validate_circuit_instruction(
                instruction, &semantic_error)) {
          if (out_error) {
            *out_error = "instruction dsl replace validation failed (" +
                         family_name + "): " + semantic_error;
          }
          return false;
        }
        break;
      }
      case instruction_dsl_validation_family_e::IitepiContract: {
        std::string basic_contract_error{};
        if (!validate_basic_iitepi_contract_wrapper(text,
                                                    &basic_contract_error)) {
          if (out_error) {
            *out_error = "instruction dsl replace validation failed (" +
                         family_name + "): " + basic_contract_error;
          }
          return false;
        }
        std::error_code ec{};
        std::filesystem::create_directories(dsl_path.parent_path(), ec);
        if (ec) {
          if (out_error) {
            *out_error = "cannot prepare contract validation directory: " +
                         dsl_path.parent_path().string();
          }
          return false;
        }
        const bool existed_before = std::filesystem::exists(dsl_path, ec) &&
                                    std::filesystem::is_regular_file(dsl_path,
                                                                     ec);
        std::string previous_text{};
        if (existed_before) {
          std::string previous_read_error{};
          if (!read_text_file(dsl_path.string(), &previous_text,
                              &previous_read_error)) {
            if (out_error) {
              *out_error = "cannot read current contract file for validation: " +
                           previous_read_error;
            }
            return false;
          }
        }
        struct contract_validation_restore_guard_t {
          std::filesystem::path path;
          bool existed_before{false};
          std::string previous_text{};
          bool restore_needed{true};
          std::string* out_error{nullptr};
          ~contract_validation_restore_guard_t() {
            if (!restore_needed || path.empty()) return;
            std::string restore_error{};
            if (existed_before) {
              if (!write_text_file_atomic(path.string(), previous_text,
                                          &restore_error)) {
                if (out_error != nullptr && out_error->empty()) {
                  *out_error =
                      "failed to restore original contract file after "
                      "validation: " +
                      restore_error;
                }
              }
              return;
            }
            std::error_code cleanup_ec{};
            std::filesystem::remove(path, cleanup_ec);
            if (cleanup_ec && out_error != nullptr && out_error->empty()) {
              *out_error =
                  "failed to remove temporary contract file after validation: " +
                  path.string();
            }
          }
        } restore_guard{dsl_path, existed_before, previous_text, true,
                        out_error};
        std::string stage_write_error{};
        if (!write_text_file_atomic(dsl_path.string(), text,
                                    &stage_write_error)) {
          if (out_error) {
            *out_error = "cannot stage contract file for validation: " +
                         stage_write_error;
          }
          return false;
        }
        std::string contract_error{};
        if (!cuwacunu::iitepi::contract_space_t::validate_contract_file_isolated(
                dsl_path.string(), store.global_config_path(),
                &contract_error)) {
          if (out_error) {
            *out_error = "instruction dsl replace validation failed (" +
                         family_name + "): " + contract_error;
          }
          return false;
        }
        restore_guard.restore_needed = true;
        break;
      }
      case instruction_dsl_validation_family_e::IitepiWave:
        (void)cuwacunu::camahjucunu::dsl::decode_iitepi_wave_from_dsl(
            grammar_text, text);
        break;
      case instruction_dsl_validation_family_e::IitepiCampaign:
        (void)cuwacunu::camahjucunu::dsl::decode_iitepi_campaign_from_dsl(
            grammar_text, text);
        break;
      case instruction_dsl_validation_family_e::ObservationChannels: {
        cuwacunu::camahjucunu::dsl::observationChannelsDecoder decoder(
            grammar_text);
        (void)decoder.decode(text);
        break;
      }
      case instruction_dsl_validation_family_e::ObservationSources: {
        cuwacunu::camahjucunu::dsl::observationSourcesDecoder decoder(
            grammar_text);
        (void)decoder.decode(text);
        break;
      }
      case instruction_dsl_validation_family_e::Unsupported:
      default:
        if (out_error) {
          *out_error = "instruction dsl replace validation is unsupported for "
                       "this DSL family: " +
                       dsl_path.string();
        }
        return false;
    }
  } catch (const std::exception& ex) {
    if (out_error) {
      *out_error = "instruction dsl replace validation failed (" +
                   family_name + "): " + ex.what();
    }
    return false;
  } catch (...) {
    if (out_error) {
      *out_error = "instruction dsl replace validation failed (" +
                   family_name + "): unknown decoder error";
    }
    return false;
  }
  return true;
}

struct scoped_file_cleanup_t {
  std::filesystem::path path{};
  ~scoped_file_cleanup_t() {
    if (path.empty()) return;
    std::error_code ec{};
    std::filesystem::remove(path, ec);
  }
};

[[nodiscard]] std::string join_messages(
    const std::vector<std::string>& messages, std::string_view delimiter) {
  std::ostringstream out;
  for (std::size_t i = 0; i < messages.size(); ++i) {
    if (i != 0) out << delimiter;
    out << messages[i];
  }
  return out.str();
}

[[nodiscard]] std::filesystem::path make_validation_snapshot_path(
    const std::filesystem::path& dsl_path) {
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  return dsl_path.parent_path() /
         (dsl_path.filename().string() + ".validate." +
          std::to_string(static_cast<long long>(stamp)));
}

[[nodiscard]] bool write_validation_snapshot(
    const std::filesystem::path& dsl_path, std::string_view content,
    std::filesystem::path* out_snapshot_path, std::string* out_error) {
  if (out_error) out_error->clear();
  if (!out_snapshot_path) {
    if (out_error) *out_error = "validation snapshot output pointer is null";
    return false;
  }
  const std::filesystem::path snapshot_path =
      make_validation_snapshot_path(dsl_path);
  if (!write_text_file_atomic(snapshot_path.string(), content, out_error)) {
    return false;
  }
  *out_snapshot_path = snapshot_path;
  return true;
}

[[nodiscard]] bool validate_default_dsl_replacement_semantics(
    const hero_config_store_t& store, const std::filesystem::path& dsl_path,
    std::string_view replacement_text, std::string* out_error) {
  if (out_error) out_error->clear();
  const std::string file_name = lowercase_copy(dsl_path.filename().string());
  if (file_name != "default.hero.config.dsl" &&
      file_name != "default.hero.runtime.dsl" &&
      file_name != "default.hero.marshal.dsl" &&
      file_name != "default.hero.hashimyei.dsl" &&
      file_name != "default.hero.lattice.dsl") {
    return true;
  }

  std::filesystem::path snapshot_path{};
  if (!write_validation_snapshot(dsl_path, replacement_text, &snapshot_path,
                                 out_error)) {
    return false;
  }
  const scoped_file_cleanup_t cleanup{snapshot_path};

  if (file_name == "default.hero.config.dsl") {
    hero_config_store_t candidate(snapshot_path.string(),
                                  store.global_config_path());
    std::string load_error{};
    if (!candidate.load(&load_error)) {
      if (out_error) {
        *out_error = "default hero config validation failed: " + load_error;
      }
      return false;
    }
    const std::vector<std::string> validation_errors = candidate.validate();
    if (!validation_errors.empty()) {
      if (out_error) {
        *out_error = "default hero config validation failed: " +
                     join_messages(validation_errors, "; ");
      }
      return false;
    }
    return true;
  }

  if (file_name == "default.hero.runtime.dsl") {
    cuwacunu::hero::runtime_mcp::runtime_defaults_t defaults{};
    std::string validation_error{};
    if (!cuwacunu::hero::runtime_mcp::load_runtime_defaults(
            snapshot_path, store.global_config_path(), &defaults,
            &validation_error)) {
      if (out_error) {
        *out_error = "default hero runtime validation failed: " +
                     validation_error;
      }
      return false;
    }
    return true;
  }

  if (file_name == "default.hero.marshal.dsl") {
    cuwacunu::hero::marshal_mcp::marshal_defaults_t defaults{};
    std::string validation_error{};
    if (!cuwacunu::hero::marshal_mcp::load_marshal_defaults(
            snapshot_path, store.global_config_path(), &defaults,
            &validation_error)) {
      if (out_error) {
        *out_error = "default hero marshal validation failed: " +
                     validation_error;
      }
      return false;
    }
    return true;
  }

  if (file_name == "default.hero.hashimyei.dsl") {
    cuwacunu::hero::hashimyei_mcp::hashimyei_runtime_defaults_t defaults{};
    std::string validation_error{};
    if (!cuwacunu::hero::hashimyei_mcp::load_hashimyei_runtime_defaults(
            snapshot_path, store.global_config_path(), &defaults,
            &validation_error)) {
      if (out_error) {
        *out_error = "default hero hashimyei validation failed: " +
                     validation_error;
      }
      return false;
    }
    return true;
  }

  cuwacunu::hero::lattice_mcp::wave_runtime_defaults_t defaults{};
  std::string validation_error{};
  if (!cuwacunu::hero::lattice_mcp::load_wave_runtime_defaults(
          snapshot_path, store.global_config_path(), &defaults,
          &validation_error)) {
    if (out_error) {
      *out_error = "default hero lattice validation failed: " +
                   validation_error;
    }
    return false;
  }
  return true;
}

[[nodiscard]] std::string default_mutation_warning() {
  return "mutating defaults changes shared truth for objectives that inherit them";
}

[[nodiscard]] bool extract_objective_request_paths(
    std::string_view tool_name, const std::string& request_json,
    std::string* out_objective_root, std::string* out_path,
    int* out_error_code, std::string* out_error_message) {
  if (out_objective_root) out_objective_root->clear();
  if (out_path) out_path->clear();
  std::string objective_root{};
  std::string path{};
  if (!extract_json_string_field(request_json, "objective_root", &objective_root) ||
      objective_root.empty()) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message =
          std::string(tool_name) + " requires argument objective_root";
    }
    return false;
  }
  if (!extract_json_string_field(request_json, "path", &path) || path.empty()) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument path";
    }
    return false;
  }
  if (out_objective_root) *out_objective_root = std::move(objective_root);
  if (out_path) *out_path = std::move(path);
  return true;
}

[[nodiscard]] bool handle_tool_default_read(
    std::string_view tool_name, const std::string& request_json,
    hero_config_store_t* store, std::string* out_result_json,
    int* out_error_code, std::string* out_error_message) {
  std::string dsl_path_raw{};
  if (!extract_json_string_field(request_json, "path", &dsl_path_raw) ||
      dsl_path_raw.empty()) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument path";
    }
    return false;
  }

  std::filesystem::path dsl_path{};
  std::string err{};
  dsl_path_resolution_error_t dsl_err = dsl_path_resolution_error_t::kNone;
  if (!resolve_default_dsl_path_with_scope(*store, dsl_path_raw, &dsl_path, &err,
                                           &dsl_err)) {
    if (out_error_code) {
      *out_error_code =
          is_scope_violation(dsl_err) ? kConfigDslScopeErrorCode : -32602;
    }
    if (out_error_message) *out_error_message = err;
    return false;
  }
  std::string text{};
  if (!read_text_file(dsl_path.string(), &text, &err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) *out_error_message = err;
    return false;
  }
  std::string sha256_hex{};
  if (!sha256_hex_file(dsl_path, &sha256_hex, &err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) *out_error_message = err;
    return false;
  }
  const auto validation_family_enum =
      detect_instruction_dsl_validation_family(dsl_path);
  const std::string validation_family =
      instruction_dsl_validation_family_name(validation_family_enum);
  const std::filesystem::path config_root =
      resolve_config_root_from_global_config(*store);
  const std::filesystem::path man_path = find_associated_man_path_with_fallback(
      config_root, dsl_path, validation_family_enum);
  const std::string warning =
      man_path.empty() &&
              should_warn_missing_associated_man(dsl_path, validation_family_enum)
          ? missing_associated_man_warning(dsl_path)
          : "";
  if (!warning.empty()) log_config_warning(warning);

  if (out_result_json) {
    std::ostringstream out;
    out << "{\"path\":" << json_quote(dsl_path.string())
        << ",\"bytes\":" << text.size()
        << ",\"sha256\":" << json_quote(sha256_hex)
        << ",\"validation_family\":" << json_quote(validation_family)
        << ",\"replace_supported\":"
        << bool_json(validation_family != "unsupported")
        << ",\"content\":" << json_quote(text);
    if (!append_associated_man_fields(&out, man_path,
                                      /*include_content=*/true, warning,
                                      &err)) {
      if (out_error_code) *out_error_code = -32603;
      if (out_error_message) *out_error_message = err;
      return false;
    }
    out << "}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool handle_tool_default_list(
    std::string_view tool_name, const std::string& request_json,
    hero_config_store_t* store, std::string* out_result_json,
    int* out_error_code, std::string* out_error_message) {
  bool include_man = false;
  bool parsed_include_man = false;
  if (extract_json_raw_field(request_json, "include_man", nullptr)) {
    if (!extract_json_bool_field(request_json, "include_man", &include_man)) {
      if (out_error_code) *out_error_code = -32602;
      if (out_error_message) {
        *out_error_message =
            std::string(tool_name) + " include_man must be boolean";
      }
      return false;
    }
    parsed_include_man = true;
  }
  if (!parsed_include_man &&
      extract_json_bool_field(request_json, "includeMan", &include_man)) {
    parsed_include_man = true;
  }
  (void)parsed_include_man;
  std::string err{};
  std::vector<std::filesystem::path> default_roots{};
  if (!collect_configured_root_paths(*store, "default_roots", &default_roots,
                                     &err)) {
    if (out_error_code) *out_error_code = kConfigDslScopeErrorCode;
    if (out_error_message) *out_error_message = err;
    return false;
  }
  std::vector<std::string> allowed_extensions{};
  if (!collect_allowed_extensions(*store, &allowed_extensions, &err)) {
    if (out_error_code) *out_error_code = kConfigDslScopeErrorCode;
    if (out_error_message) *out_error_message = err;
    return false;
  }

  std::vector<std::filesystem::path> files{};
  for (const auto& root : default_roots) {
    std::vector<std::filesystem::path> root_files{};
    if (!list_instruction_files_under_root(root, allowed_extensions, &root_files,
                                           &err)) {
      if (out_error_code) *out_error_code = -32603;
      if (out_error_message) *out_error_message = err;
      return false;
    }
    files.insert(files.end(), root_files.begin(), root_files.end());
  }
  const std::filesystem::path config_root =
      resolve_config_root_from_global_config(*store);
  std::sort(files.begin(), files.end(),
            [](const std::filesystem::path& a, const std::filesystem::path& b) {
              return a.string() < b.string();
            });
  files.erase(std::unique(files.begin(), files.end(),
                          [](const std::filesystem::path& a,
                             const std::filesystem::path& b) {
                            return a.string() == b.string();
                          }),
              files.end());

  if (out_result_json) {
    std::ostringstream out;
    out << "{\"roots\":" << path_vector_json(default_roots)
        << ",\"files\":[";
    for (std::size_t i = 0; i < files.size(); ++i) {
      const auto& path = files[i];
      const auto validation_family_enum =
          detect_instruction_dsl_validation_family(path);
      const std::string validation_family =
          instruction_dsl_validation_family_name(validation_family_enum);
      const std::filesystem::path man_path =
          find_associated_man_path_with_fallback(config_root, path,
                                                 validation_family_enum);
      const std::string warning =
          man_path.empty() &&
                  should_warn_missing_associated_man(path,
                                                     validation_family_enum)
              ? missing_associated_man_warning(path)
              : "";
      if (!warning.empty()) log_config_warning(warning);
      std::string matched_root{};
      std::string relative_path{};
      for (const auto& root : default_roots) {
        if (!path_is_within(root, path)) continue;
        matched_root = root.string();
        std::error_code ec{};
        relative_path = std::filesystem::relative(path, root, ec).string();
        if (ec) relative_path.clear();
        break;
      }
      if (i != 0) out << ",";
      out << "{\"root\":" << json_quote(matched_root)
          << ",\"path\":" << json_quote(path.string())
          << ",\"relative_path\":" << json_quote(relative_path)
          << ",\"validation_family\":" << json_quote(validation_family)
          << ",\"replace_supported\":"
          << bool_json(validation_family != "unsupported");
      if (!append_associated_man_fields(&out, man_path, include_man, warning,
                                        &err)) {
        if (out_error_code) *out_error_code = -32603;
        if (out_error_message) *out_error_message = err;
        return false;
      }
      out << "}";
    }
    out << "],\"count\":" << files.size() << "}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool handle_tool_default_create(
    std::string_view tool_name, const std::string& request_json,
    hero_config_store_t* store, std::string* out_result_json,
    int* out_error_code, std::string* out_error_message) {
  std::string dsl_path_raw{};
  std::string content{};
  if (!extract_json_string_field(request_json, "path", &dsl_path_raw) ||
      dsl_path_raw.empty()) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument path";
    }
    return false;
  }
  if (!extract_json_string_field(request_json, "content", &content)) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument content";
    }
    return false;
  }

  std::filesystem::path dsl_path{};
  std::string err{};
  dsl_path_resolution_error_t dsl_err = dsl_path_resolution_error_t::kNone;
  if (!resolve_default_dsl_path_with_scope(*store, dsl_path_raw, &dsl_path, &err,
                                           &dsl_err)) {
    if (out_error_code) {
      *out_error_code =
          is_scope_violation(dsl_err) ? kConfigDslScopeErrorCode : -32602;
    }
    if (out_error_message) *out_error_message = err;
    return false;
  }
  if (!enforce_write_target_allowed(*store, dsl_path, &err)) {
    if (out_error_code) *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message) *out_error_message = err;
    return false;
  }
  std::error_code ec{};
  if (std::filesystem::exists(dsl_path, ec)) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = "default file already exists: " + dsl_path.string();
    }
    return false;
  }

  std::string validation_family{};
  std::filesystem::path grammar_path{};
  if (!validate_instruction_dsl_replacement(*store, dsl_path, content,
                                            &validation_family, &grammar_path,
                                            &err)) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) *out_error_message = err;
    return false;
  }
  if (!validate_default_dsl_replacement_semantics(*store, dsl_path, content,
                                                  &err)) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) *out_error_message = err;
    return false;
  }

  if (!write_text_file_atomic(dsl_path.string(), content, &err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) *out_error_message = err;
    return false;
  }
  std::string after_sha256{};
  if (!sha256_hex_file(dsl_path, &after_sha256, &err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) *out_error_message = err;
    return false;
  }

  if (out_result_json) {
    std::ostringstream out;
    out << "{\"created\":true,\"path\":" << json_quote(dsl_path.string())
        << ",\"bytes\":" << content.size()
        << ",\"sha256\":" << json_quote(after_sha256)
        << ",\"validation_family\":" << json_quote(validation_family)
        << ",\"grammar_path\":" << json_quote(grammar_path.string())
        << ",\"warning\":" << json_quote(default_mutation_warning()) << "}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool handle_tool_default_replace(
    std::string_view tool_name, const std::string& request_json,
    hero_config_store_t* store, std::string* out_result_json,
    int* out_error_code, std::string* out_error_message) {
  std::string dsl_path_raw{};
  std::string content{};
  std::string expected_sha256{};
  if (!extract_json_string_field(request_json, "path", &dsl_path_raw) ||
      dsl_path_raw.empty()) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument path";
    }
    return false;
  }
  if (!extract_json_string_field(request_json, "content", &content)) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument content";
    }
    return false;
  }
  (void)extract_json_string_field(request_json, "expected_sha256",
                                  &expected_sha256);

  std::filesystem::path dsl_path{};
  std::string err{};
  dsl_path_resolution_error_t dsl_err = dsl_path_resolution_error_t::kNone;
  if (!resolve_default_dsl_path_with_scope(*store, dsl_path_raw, &dsl_path, &err,
                                           &dsl_err)) {
    if (out_error_code) {
      *out_error_code =
          is_scope_violation(dsl_err) ? kConfigDslScopeErrorCode : -32602;
    }
    if (out_error_message) *out_error_message = err;
    return false;
  }
  if (!enforce_write_target_allowed(*store, dsl_path, &err)) {
    if (out_error_code) *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message) *out_error_message = err;
    return false;
  }

  std::error_code ec{};
  const bool existed = std::filesystem::exists(dsl_path, ec) &&
                       std::filesystem::is_regular_file(dsl_path, ec);
  if (!existed) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = "default file does not exist: " + dsl_path.string();
    }
    return false;
  }
  std::string before_sha256{};
  if (!sha256_hex_file(dsl_path, &before_sha256, &err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) *out_error_message = err;
    return false;
  }
  expected_sha256 = lowercase_copy(trim_ascii(expected_sha256));
  if (!expected_sha256.empty() &&
      lowercase_copy(before_sha256) != expected_sha256) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message =
          "expected_sha256 does not match current default content: " +
          dsl_path.string();
    }
    return false;
  }

  std::string validation_family{};
  std::filesystem::path grammar_path{};
  if (!validate_instruction_dsl_replacement(*store, dsl_path, content,
                                            &validation_family, &grammar_path,
                                            &err)) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) *out_error_message = err;
    return false;
  }
  if (!validate_default_dsl_replacement_semantics(*store, dsl_path, content,
                                                  &err)) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) *out_error_message = err;
    return false;
  }

  if (!backup_previous_write_target_with_cap(*store, dsl_path, &err)) {
    if (out_error_code) *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message) *out_error_message = err;
    return false;
  }

  if (!write_text_file_atomic(dsl_path.string(), content, &err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) *out_error_message = err;
    return false;
  }
  std::string after_sha256{};
  if (!sha256_hex_file(dsl_path, &after_sha256, &err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) *out_error_message = err;
    return false;
  }

  if (out_result_json) {
    std::ostringstream out;
    out << "{\"replaced\":true,\"path\":" << json_quote(dsl_path.string())
        << ",\"bytes\":" << content.size()
        << ",\"before_sha256\":" << json_quote(before_sha256)
        << ",\"sha256\":" << json_quote(after_sha256)
        << ",\"validation_family\":" << json_quote(validation_family)
        << ",\"grammar_path\":" << json_quote(grammar_path.string())
        << ",\"warning\":" << json_quote(default_mutation_warning()) << "}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool handle_tool_default_delete(
    std::string_view tool_name, const std::string& request_json,
    hero_config_store_t* store, std::string* out_result_json,
    int* out_error_code, std::string* out_error_message) {
  std::string dsl_path_raw{};
  std::string expected_sha256{};
  if (!extract_json_string_field(request_json, "path", &dsl_path_raw) ||
      dsl_path_raw.empty()) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument path";
    }
    return false;
  }
  (void)extract_json_string_field(request_json, "expected_sha256",
                                  &expected_sha256);

  std::filesystem::path dsl_path{};
  std::string err{};
  dsl_path_resolution_error_t dsl_err = dsl_path_resolution_error_t::kNone;
  if (!resolve_default_dsl_path_with_scope(*store, dsl_path_raw, &dsl_path, &err,
                                           &dsl_err)) {
    if (out_error_code) {
      *out_error_code =
          is_scope_violation(dsl_err) ? kConfigDslScopeErrorCode : -32602;
    }
    if (out_error_message) *out_error_message = err;
    return false;
  }
  if (!enforce_write_target_allowed(*store, dsl_path, &err)) {
    if (out_error_code) *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message) *out_error_message = err;
    return false;
  }

  std::string before_sha256{};
  if (!sha256_hex_file(dsl_path, &before_sha256, &err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) *out_error_message = err;
    return false;
  }
  expected_sha256 = lowercase_copy(trim_ascii(expected_sha256));
  if (!expected_sha256.empty() &&
      lowercase_copy(before_sha256) != expected_sha256) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message =
          "expected_sha256 does not match current default content: " +
          dsl_path.string();
    }
    return false;
  }

  if (!backup_previous_write_target_with_cap(*store, dsl_path, &err)) {
    if (out_error_code) *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message) *out_error_message = err;
    return false;
  }

  std::error_code ec{};
  if (!std::filesystem::remove(dsl_path, ec) || ec) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) {
      *out_error_message = "failed to delete default file: " + dsl_path.string();
    }
    return false;
  }

  if (out_result_json) {
    std::ostringstream out;
    out << "{\"deleted\":true,\"path\":" << json_quote(dsl_path.string())
        << ",\"before_sha256\":" << json_quote(before_sha256)
        << ",\"warning\":" << json_quote(default_mutation_warning()) << "}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool handle_tool_objective_read(
    std::string_view tool_name, const std::string& request_json,
    hero_config_store_t* store, std::string* out_result_json,
    int* out_error_code, std::string* out_error_message) {
  std::string objective_root_raw{};
  std::string path_raw{};
  if (!extract_objective_request_paths(tool_name, request_json,
                                       &objective_root_raw, &path_raw,
                                       out_error_code,
                                       out_error_message)) {
    return false;
  }

  std::filesystem::path dsl_path{};
  std::string err{};
  dsl_path_resolution_error_t dsl_err = dsl_path_resolution_error_t::kNone;
  if (!resolve_objective_path_with_scope(*store, objective_root_raw, path_raw,
                                         /*allow_missing_target=*/false,
                                         &dsl_path, &err, &dsl_err)) {
    if (out_error_code) {
      *out_error_code =
          is_scope_violation(dsl_err) ? kConfigDslScopeErrorCode : -32602;
    }
    if (out_error_message) *out_error_message = err;
    return false;
  }

  std::string content{};
  if (!read_text_file(dsl_path.string(), &content, &err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) *out_error_message = err;
    return false;
  }
  std::string sha256_hex{};
  if (!sha256_hex_file(dsl_path, &sha256_hex, &err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) *out_error_message = err;
    return false;
  }
  const auto validation_family_enum =
      detect_instruction_dsl_validation_family(dsl_path);
  const std::string validation_family =
      instruction_dsl_validation_family_name(validation_family_enum);
  const std::filesystem::path config_root =
      resolve_config_root_from_global_config(*store);
  const std::filesystem::path man_path = find_associated_man_path_with_fallback(
      config_root, dsl_path, validation_family_enum);
  const std::string warning =
      man_path.empty() &&
              should_warn_missing_associated_man(dsl_path, validation_family_enum)
          ? missing_associated_man_warning(dsl_path)
          : "";
  if (!warning.empty()) log_config_warning(warning);

  if (out_result_json) {
    std::ostringstream out;
    out << "{\"objective_root\":" << json_quote(objective_root_raw)
        << ",\"path\":" << json_quote(dsl_path.string())
        << ",\"bytes\":" << content.size()
        << ",\"sha256\":" << json_quote(sha256_hex)
        << ",\"validation_family\":" << json_quote(validation_family)
        << ",\"replace_supported\":"
        << bool_json(validation_family != "unsupported")
        << ",\"content\":" << json_quote(content);
    if (!append_associated_man_fields(&out, man_path,
                                      /*include_content=*/true, warning,
                                      &err)) {
      if (out_error_code) *out_error_code = -32603;
      if (out_error_message) *out_error_message = err;
      return false;
    }
    out << "}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool handle_tool_objective_list(
    std::string_view tool_name, const std::string& request_json,
    hero_config_store_t* store, std::string* out_result_json,
    int* out_error_code, std::string* out_error_message) {
  bool include_man = false;
  bool parsed_include_man = false;
  if (extract_json_raw_field(request_json, "include_man", nullptr)) {
    if (!extract_json_bool_field(request_json, "include_man", &include_man)) {
      if (out_error_code) *out_error_code = -32602;
      if (out_error_message) {
        *out_error_message =
            std::string(tool_name) + " include_man must be boolean";
      }
      return false;
    }
    parsed_include_man = true;
  }
  if (!parsed_include_man &&
      extract_json_bool_field(request_json, "includeMan", &include_man)) {
    parsed_include_man = true;
  }
  (void)parsed_include_man;
  std::string objective_root_raw{};
  if (!extract_json_string_field(request_json, "objective_root", &objective_root_raw) ||
      objective_root_raw.empty()) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message =
          std::string(tool_name) + " requires argument objective_root";
    }
    return false;
  }

  std::filesystem::path objective_root{};
  std::string err{};
  dsl_path_resolution_error_t dsl_err = dsl_path_resolution_error_t::kNone;
  if (!resolve_objective_root_with_scope(*store, objective_root_raw,
                                         &objective_root, &err, &dsl_err)) {
    if (out_error_code) {
      *out_error_code =
          is_scope_violation(dsl_err) ? kConfigDslScopeErrorCode : -32602;
    }
    if (out_error_message) *out_error_message = err;
    return false;
  }

  std::vector<std::string> allowed_extensions{};
  if (!collect_allowed_extensions(*store, &allowed_extensions, &err)) {
    if (out_error_code) *out_error_code = kConfigDslScopeErrorCode;
    if (out_error_message) *out_error_message = err;
    return false;
  }
  std::vector<std::filesystem::path> files{};
  if (!list_instruction_files_under_root(objective_root, allowed_extensions,
                                         &files, &err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) *out_error_message = err;
    return false;
  }
  const std::filesystem::path config_root =
      resolve_config_root_from_global_config(*store);

  if (out_result_json) {
    std::ostringstream out;
    out << "{\"objective_root\":" << json_quote(objective_root_raw)
        << ",\"files\":[";
    for (std::size_t i = 0; i < files.size(); ++i) {
      const auto& path = files[i];
      std::error_code ec{};
      const std::string relative_path =
          std::filesystem::relative(path, objective_root, ec).string();
      const auto validation_family_enum =
          detect_instruction_dsl_validation_family(path);
      const std::string validation_family =
          instruction_dsl_validation_family_name(validation_family_enum);
      const std::filesystem::path man_path =
          find_associated_man_path_with_fallback(config_root, path,
                                                 validation_family_enum);
      const std::string warning =
          man_path.empty() &&
                  should_warn_missing_associated_man(path,
                                                     validation_family_enum)
              ? missing_associated_man_warning(path)
              : "";
      if (!warning.empty()) log_config_warning(warning);
      if (i != 0) out << ",";
      out << "{\"path\":" << json_quote(path.string())
          << ",\"relative_path\":" << json_quote(relative_path)
          << ",\"validation_family\":" << json_quote(validation_family)
          << ",\"replace_supported\":"
          << bool_json(validation_family != "unsupported");
      if (!append_associated_man_fields(&out, man_path, include_man, warning,
                                        &err)) {
        if (out_error_code) *out_error_code = -32603;
        if (out_error_message) *out_error_message = err;
        return false;
      }
      out << "}";
    }
    out << "],\"count\":" << files.size() << "}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool handle_tool_objective_create(
    std::string_view tool_name, const std::string& request_json,
    hero_config_store_t* store, std::string* out_result_json,
    int* out_error_code, std::string* out_error_message) {
  std::string objective_root_raw{};
  std::string path_raw{};
  std::string content{};
  if (!extract_objective_request_paths(tool_name, request_json,
                                       &objective_root_raw, &path_raw,
                                       out_error_code,
                                       out_error_message)) {
    return false;
  }
  if (!extract_json_string_field(request_json, "content", &content)) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument content";
    }
    return false;
  }

  std::filesystem::path dsl_path{};
  std::string err{};
  dsl_path_resolution_error_t dsl_err = dsl_path_resolution_error_t::kNone;
  if (!resolve_objective_path_with_scope(*store, objective_root_raw, path_raw,
                                         /*allow_missing_target=*/true,
                                         &dsl_path, &err, &dsl_err)) {
    if (out_error_code) {
      *out_error_code =
          is_scope_violation(dsl_err) ? kConfigDslScopeErrorCode : -32602;
    }
    if (out_error_message) *out_error_message = err;
    return false;
  }
  if (!enforce_write_target_allowed(*store, dsl_path, &err)) {
    if (out_error_code) *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message) *out_error_message = err;
    return false;
  }
  std::error_code ec{};
  if (std::filesystem::exists(dsl_path, ec)) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = "objective file already exists: " + dsl_path.string();
    }
    return false;
  }

  std::string validation_family{};
  std::filesystem::path grammar_path{};
  if (!validate_instruction_dsl_replacement(*store, dsl_path, content,
                                            &validation_family, &grammar_path,
                                            &err)) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) *out_error_message = err;
    return false;
  }

  if (!write_text_file_atomic(dsl_path.string(), content, &err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) *out_error_message = err;
    return false;
  }
  std::string after_sha256{};
  if (!sha256_hex_file(dsl_path, &after_sha256, &err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) *out_error_message = err;
    return false;
  }

  if (out_result_json) {
    std::ostringstream out;
    out << "{\"created\":true"
        << ",\"objective_root\":" << json_quote(objective_root_raw)
        << ",\"path\":" << json_quote(dsl_path.string())
        << ",\"bytes\":" << content.size()
        << ",\"sha256\":" << json_quote(after_sha256)
        << ",\"validation_family\":" << json_quote(validation_family)
        << ",\"grammar_path\":" << json_quote(grammar_path.string()) << "}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool handle_tool_objective_replace(
    std::string_view tool_name, const std::string& request_json,
    hero_config_store_t* store, std::string* out_result_json,
    int* out_error_code, std::string* out_error_message) {
  std::string objective_root_raw{};
  std::string path_raw{};
  std::string content{};
  std::string expected_sha256{};
  if (!extract_objective_request_paths(tool_name, request_json,
                                       &objective_root_raw, &path_raw,
                                       out_error_code,
                                       out_error_message)) {
    return false;
  }
  if (!extract_json_string_field(request_json, "content", &content)) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument content";
    }
    return false;
  }
  (void)extract_json_string_field(request_json, "expected_sha256",
                                  &expected_sha256);

  std::filesystem::path dsl_path{};
  std::string err{};
  dsl_path_resolution_error_t dsl_err = dsl_path_resolution_error_t::kNone;
  if (!resolve_objective_path_with_scope(*store, objective_root_raw, path_raw,
                                         /*allow_missing_target=*/true,
                                         &dsl_path, &err, &dsl_err)) {
    if (out_error_code) {
      *out_error_code =
          is_scope_violation(dsl_err) ? kConfigDslScopeErrorCode : -32602;
    }
    if (out_error_message) *out_error_message = err;
    return false;
  }
  if (!enforce_write_target_allowed(*store, dsl_path, &err)) {
    if (out_error_code) *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message) *out_error_message = err;
    return false;
  }

  std::error_code ec{};
  const bool existed = std::filesystem::exists(dsl_path, ec) &&
                       std::filesystem::is_regular_file(dsl_path, ec);
  if (!existed) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = "objective file does not exist: " + dsl_path.string();
    }
    return false;
  }
  std::string before_sha256{};
  if (!sha256_hex_file(dsl_path, &before_sha256, &err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) *out_error_message = err;
    return false;
  }
  expected_sha256 = lowercase_copy(trim_ascii(expected_sha256));
  if (!expected_sha256.empty() &&
      lowercase_copy(before_sha256) != expected_sha256) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message =
          "expected_sha256 does not match current objective content: " +
          dsl_path.string();
    }
    return false;
  }

  std::string validation_family{};
  std::filesystem::path grammar_path{};
  if (!validate_instruction_dsl_replacement(*store, dsl_path, content,
                                            &validation_family, &grammar_path,
                                            &err)) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) *out_error_message = err;
    return false;
  }

  if (!backup_previous_write_target_with_cap(*store, dsl_path, &err)) {
    if (out_error_code) *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message) *out_error_message = err;
    return false;
  }

  if (!write_text_file_atomic(dsl_path.string(), content, &err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) *out_error_message = err;
    return false;
  }
  std::string after_sha256{};
  if (!sha256_hex_file(dsl_path, &after_sha256, &err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) *out_error_message = err;
    return false;
  }

  if (out_result_json) {
    std::ostringstream out;
    out << "{\"replaced\":true,\"created\":" << bool_json(!existed)
        << ",\"objective_root\":" << json_quote(objective_root_raw)
        << ",\"path\":" << json_quote(dsl_path.string())
        << ",\"bytes\":" << content.size()
        << ",\"before_sha256\":" << json_quote(before_sha256)
        << ",\"sha256\":" << json_quote(after_sha256)
        << ",\"validation_family\":" << json_quote(validation_family)
        << ",\"grammar_path\":" << json_quote(grammar_path.string()) << "}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool handle_tool_objective_delete(
    std::string_view tool_name, const std::string& request_json,
    hero_config_store_t* store, std::string* out_result_json,
    int* out_error_code, std::string* out_error_message) {
  std::string objective_root_raw{};
  std::string path_raw{};
  std::string expected_sha256{};
  if (!extract_objective_request_paths(tool_name, request_json,
                                       &objective_root_raw, &path_raw,
                                       out_error_code,
                                       out_error_message)) {
    return false;
  }
  (void)extract_json_string_field(request_json, "expected_sha256",
                                  &expected_sha256);

  std::filesystem::path dsl_path{};
  std::string err{};
  dsl_path_resolution_error_t dsl_err = dsl_path_resolution_error_t::kNone;
  if (!resolve_objective_path_with_scope(*store, objective_root_raw, path_raw,
                                         /*allow_missing_target=*/false,
                                         &dsl_path, &err, &dsl_err)) {
    if (out_error_code) {
      *out_error_code =
          is_scope_violation(dsl_err) ? kConfigDslScopeErrorCode : -32602;
    }
    if (out_error_message) *out_error_message = err;
    return false;
  }
  if (!enforce_write_target_allowed(*store, dsl_path, &err)) {
    if (out_error_code) *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message) *out_error_message = err;
    return false;
  }

  std::string before_sha256{};
  if (!sha256_hex_file(dsl_path, &before_sha256, &err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) *out_error_message = err;
    return false;
  }
  expected_sha256 = lowercase_copy(trim_ascii(expected_sha256));
  if (!expected_sha256.empty() &&
      lowercase_copy(before_sha256) != expected_sha256) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message =
          "expected_sha256 does not match current objective content: " +
          dsl_path.string();
    }
    return false;
  }

  if (!backup_previous_write_target_with_cap(*store, dsl_path, &err)) {
    if (out_error_code) *out_error_code = kConfigWritePolicyErrorCode;
    if (out_error_message) *out_error_message = err;
    return false;
  }

  std::error_code ec{};
  if (!std::filesystem::remove(dsl_path, ec) || ec) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) {
      *out_error_message = "failed to delete objective file: " + dsl_path.string();
    }
    return false;
  }

  if (out_result_json) {
    std::ostringstream out;
    out << "{\"deleted\":true"
        << ",\"objective_root\":" << json_quote(objective_root_raw)
        << ",\"path\":" << json_quote(dsl_path.string())
        << ",\"before_sha256\":" << json_quote(before_sha256) << "}";
    *out_result_json = out.str();
  }
  return true;
}

}  // namespace cuwacunu::hero::mcp::detail
