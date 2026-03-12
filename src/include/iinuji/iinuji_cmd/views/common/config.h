#pragma once

#include <filesystem>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "iinuji/iinuji_cmd/views/common/base.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline ConfigTabData make_text_tab(std::string id,
                                   std::string title,
                                   std::string path) {
  ConfigTabData tab{};
  tab.id = std::move(id);
  tab.title = std::move(title);
  tab.path = std::move(path);
  tab.ok = read_text_file_safe(tab.path, &tab.content, &tab.error);
  return tab;
}

inline ConfigTabData make_secrets_tab() {
  ConfigTabData tab{};
  tab.id = "secrets";
  tab.title = "secrets";
  tab.path = "(computed)";

  std::ostringstream oss;
  oss << "# secrets summary\n";
  oss << "# values are masked; file paths and sizes are shown\n\n";

  const std::vector<std::pair<std::string, std::vector<std::string>>> section_keys{
      {"TEST_EXCHANGE", {"Ed25519_pkey", "EXCHANGE_api_filename"}},
      {"REAL_EXCHANGE", {"Ed25519_pkey", "EXCHANGE_api_filename"}},
      {"REAL_HERO", {"OPENAI_api_filename"}},
  };

  for (const auto& section_entry : section_keys) {
    const std::string& sec = section_entry.first;
    const std::vector<std::string>& keys = section_entry.second;
    oss << "[" << sec << "]\n";
    for (const std::string& key : keys) {
      std::string path;
      if (!lookup_global_config_value(sec, key, &path)) {
        oss << "  " << key << ": <missing in config>\n";
        continue;
      }
      oss << "  " << key << ": " << format_file_status(path);
      if (key == "EXCHANGE_api_filename" || key == "OPENAI_api_filename") {
        std::string content;
        std::string err;
        if (read_text_file_safe(path, &content, &err)) {
          std::string first_line;
          std::istringstream iss(content);
          std::getline(iss, first_line);
          oss << " preview=" << masked_preview(first_line);
        } else {
          oss << " preview=<unreadable>";
        }
      }
      oss << "\n";
    }
    oss << "\n";
  }

  tab.ok = true;
  tab.content = oss.str();
  return tab;
}

inline ConfigState load_config_view_from_config(
    const cuwacunu::iitepi::contract_hash_t& contract_hash) {
  ConfigState out{};
  if (contract_hash.empty()) {
    out.ok = false;
    out.error = "missing contract hash for config view";
    return out;
  }

  const std::string global_path = cuwacunu::iitepi::config_space_t::config_file_path;
  const std::string contract_path =
      cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash)
          ->config_file_path;
  out.tabs.push_back(make_text_tab("global", "global .config", global_path));
  out.tabs.push_back(make_text_tab("contract", "board contract", contract_path));

  struct TabSpec {
    const char* id;
    const char* title;
    const char* section;
    const char* key;
  };
  static constexpr TabSpec specs[] = {
      {"tsi.source.dataloader.sources.bnf", "tsi.source.dataloader.sources.bnf",
       "BNF", "observation_sources_grammar_filename"},
      {"tsi.source.dataloader.channels.bnf", "tsi.source.dataloader.channels.bnf",
       "BNF", "observation_channels_grammar_filename"},
      {"jkimyei.bnf", "jkimyei.bnf",
       "BNF", "jkimyei_specs_grammar_filename"},
      {"iitepi.contract.circuit.bnf", "iitepi.contract.circuit.bnf",
       "BNF", "tsiemene_circuit_grammar_filename"},
      {"default.iitepi.contract.circuit.dsl", "default.iitepi.contract.circuit.dsl",
       "CONTRACT", "circuit_dsl_filename"},
      {"iitepi.wave.bnf", "iitepi.wave.bnf",
       "BNF", "iitepi_wave_grammar_filename"},
      {"default.iitepi.wave.dsl", "default.iitepi.wave.dsl",
       "DSL", "iitepi_wave_dsl_filename"},
      {"canonical_path.bnf", "canonical_path.bnf",
       "BNF", "canonical_path_grammar_filename"},
  };

  for (const auto& s : specs) {
    std::string path;
    if (!lookup_config_value(s.section, s.key, contract_hash, &path)) {
      ConfigTabData tab{};
      tab.id = s.id;
      tab.title = s.title;
      tab.ok = false;
      tab.error = "missing [" + std::string(s.section) + "]." + std::string(s.key);
      out.tabs.push_back(std::move(tab));
      continue;
    }
    if (path.empty()) {
      ConfigTabData tab{};
      tab.id = s.id;
      tab.title = s.title;
      tab.ok = false;
      tab.error = "empty [" + std::string(s.section) + "] path";
      out.tabs.push_back(std::move(tab));
      continue;
    }
    out.tabs.push_back(make_text_tab(s.id, s.title, path));
  }

  out.tabs.push_back(make_secrets_tab());
  out.ok = !out.tabs.empty();
  if (!out.ok) out.error = "no tabs";
  return out;
}

inline ConfigState load_config_view_from_config() {
  return load_config_view_from_config(resolve_configured_board_contract_hash());
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
