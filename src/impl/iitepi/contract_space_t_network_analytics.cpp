#include "iitepi/contract_space_t.h"

#include "camahjucunu/dsl/network_design/network_design.h"
#include "piaabo/dfiles.h"
#include "piaabo/torch_compat/network_analytics.h"

#include <cctype>
#include <filesystem>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace cuwacunu {
namespace iitepi {

namespace {

[[nodiscard]] bool has_non_ws_ascii(const std::string &s) {
  for (const unsigned char c : s) {
    if (!std::isspace(c))
      return true;
  }
  return false;
}

[[nodiscard]] std::string trim_ascii_ws_copy(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b])))
    ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])))
    --e;
  return s.substr(b, e - b);
}

[[nodiscard]] std::string resolve_path_from_folder(const std::string &folder,
                                                   std::string path) {
  path = trim_ascii_ws_copy(std::move(path));
  if (path.empty())
    return {};
  const std::filesystem::path p(path);
  if (p.is_absolute())
    return path;
  if (folder.empty())
    return path;
  return (std::filesystem::path(folder) / p).string();
}

[[nodiscard]] std::string parent_folder_from_path(const std::string &path) {
  const std::filesystem::path p(path);
  if (!p.has_parent_path())
    return {};
  return p.parent_path().string();
}

[[nodiscard]] bool ends_with_ascii(std::string_view text,
                                   std::string_view suffix) {
  if (suffix.size() > text.size())
    return false;
  return text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

[[nodiscard]] bool is_single_path_key(std::string_view key) {
  return ends_with_ascii(key, "_filename") || ends_with_ascii(key, "_dsl_file");
}

[[nodiscard]] bool is_list_path_key(std::string_view key) {
  return ends_with_ascii(key, "_filenames") ||
         ends_with_ascii(key, "_dsl_files");
}

[[nodiscard]] std::string unquote_if_wrapped(std::string s) {
  s = trim_ascii_ws_copy(std::move(s));
  if (s.size() >= 2) {
    const char a = s.front();
    const char b = s.back();
    if ((a == '\'' && b == '\'') || (a == '"' && b == '"')) {
      return s.substr(1, s.size() - 2);
    }
  }
  return s;
}

[[nodiscard]] std::string
canonicalize_path_best_effort(const std::string &path) {
  if (!has_non_ws_ascii(path))
    return {};
  std::error_code ec;
  std::filesystem::path p(path);
  if (!p.is_absolute()) {
    p = std::filesystem::absolute(p, ec);
    if (ec)
      ec.clear();
  }
  const std::filesystem::path weak = std::filesystem::weakly_canonical(p, ec);
  if (!ec)
    return weak.string();
  ec.clear();
  const std::filesystem::path normal = p.lexically_normal();
  return normal.string();
}

[[nodiscard]] std::vector<std::string>
split_string_items(const std::string &s) {
  std::vector<std::string> out;
  std::string cur;
  bool in_single = false;
  bool in_double = false;

  auto push_trimmed = [&](std::string &token) {
    std::size_t b = 0;
    std::size_t e = token.size();
    while (b < e && std::isspace(static_cast<unsigned char>(token[b])))
      ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(token[e - 1])))
      --e;
    if (e > b)
      out.emplace_back(token.substr(b, e - b));
    token.clear();
  };

  for (const char ch : s) {
    if (ch == '\'' && !in_double) {
      in_single = !in_single;
      cur.push_back(ch);
      continue;
    }
    if (ch == '"' && !in_single) {
      in_double = !in_double;
      cur.push_back(ch);
      continue;
    }
    if (ch == ',' && !in_single && !in_double) {
      push_trimmed(cur);
      continue;
    }
    cur.push_back(ch);
  }
  push_trimmed(cur);
  return out;
}

[[nodiscard]] std::optional<std::string>
global_optional_file_text_by_key(const char *section, const char *key) {
  std::string raw;
  std::string cfg_folder;
  {
    LOCK_GUARD(config_mutex);
    const auto sec_it = config_space_t::config.find(section);
    if (sec_it == config_space_t::config.end())
      return std::nullopt;
    const auto key_it = sec_it->second.find(key);
    if (key_it == sec_it->second.end())
      return std::nullopt;
    raw = trim_ascii_ws_copy(key_it->second);
    cfg_folder = parent_folder_from_path(config_space_t::config_file_path);
  }
  if (!has_non_ws_ascii(raw))
    return std::nullopt;
  const std::string resolved = resolve_path_from_folder(cfg_folder, raw);
  if (!has_non_ws_ascii(resolved) || !std::filesystem::exists(resolved) ||
      !std::filesystem::is_regular_file(resolved)) {
    return std::nullopt;
  }
  const std::string text = piaabo::dfiles::readFileToString(resolved);
  if (!has_non_ws_ascii(text))
    return std::nullopt;
  return text;
}

[[nodiscard]] std::string indent_lines(const std::string &text,
                                       std::string_view prefix) {
  if (text.empty() || prefix.empty())
    return text;
  std::string out;
  out.reserve(text.size() + prefix.size() * 8);
  bool at_line_start = true;
  for (const char ch : text) {
    if (at_line_start)
      out.append(prefix);
    out.push_back(ch);
    at_line_start = (ch == '\n');
  }
  return out;
}

[[nodiscard]] const char *non_empty_or(const std::string &s,
                                       const char *fallback) {
  return has_non_ws_ascii(s) ? s.c_str() : fallback;
}

} // namespace

void contract_space_t::network_topology_analytics(const contract_hash_t &hash,
                                                  std::ostream *out,
                                                  bool beautify) {
  std::ostream &os = (out != nullptr) ? *out : std::cout;
  const auto snapshot = contract_itself(hash);
  if (!snapshot) {
    os << "[iitepi::contract_space_t::network_topology_analytics] error=null "
          "contract snapshot\n";
    return;
  }

  os << "[iitepi::contract_space_t::network_topology_analytics] contract_hash="
     << hash << " path=" << snapshot->config_file_path_canonical << "\n";

  const std::optional<std::string> fallback_network_design_grammar =
      global_optional_file_text_by_key("BNF",
                                       "network_design_grammar_filename");

  struct decoded_network_entry_t {
    std::string source_label{};
    cuwacunu::piaabo::torch_compat::network_design_analytics_report_t report{};
  };

  std::vector<decoded_network_entry_t> decoded_networks{};
  std::unordered_set<std::string> seen_network_sources{};
  std::unordered_set<std::string> seen_network_paths{};

  const auto append_decoded =
      [&](std::string source_label,
          const cuwacunu::camahjucunu::network_design_instruction_t &design) {
        if (!seen_network_sources.insert(source_label).second)
          return;
        decoded_network_entry_t entry{};
        entry.source_label = std::move(source_label);
        entry.report =
            cuwacunu::piaabo::torch_compat::summarize_network_design_analytics(
                design);
        decoded_networks.push_back(std::move(entry));
      };

  std::string vicreg_network_design_path{};
  {
    const auto module_path_it = snapshot->module_section_paths.find("VICReg");
    const auto module_sec_it = snapshot->module_sections.find("VICReg");
    if (module_path_it != snapshot->module_section_paths.end() &&
        module_sec_it != snapshot->module_sections.end()) {
      const auto net_it = module_sec_it->second.find("network_design_dsl_file");
      if (net_it != module_sec_it->second.end()) {
        std::string module_folder{};
        const std::filesystem::path module_path(module_path_it->second);
        if (module_path.has_parent_path()) {
          module_folder = module_path.parent_path().string();
        }
        vicreg_network_design_path = resolve_path_from_folder(
            module_folder,
            unquote_if_wrapped(trim_ascii_ws_copy(net_it->second)));
        const std::string canonical =
            canonicalize_path_best_effort(vicreg_network_design_path);
        if (has_non_ws_ascii(canonical)) {
          vicreg_network_design_path = canonical;
        }
      }
    }
  }

  if (snapshot->vicreg_network_design.has_payload()) {
    try {
      const auto &design = snapshot->vicreg_network_design.decoded();
      std::string source_label = hash;
      source_label += ":VICReg.network_design_dsl_file:";
      source_label += has_non_ws_ascii(vicreg_network_design_path)
                          ? vicreg_network_design_path
                          : std::string("<embedded>");
      append_decoded(source_label, design);
      if (has_non_ws_ascii(vicreg_network_design_path) &&
          ends_with_ascii(vicreg_network_design_path, ".dsl")) {
        seen_network_paths.insert(vicreg_network_design_path);
      }
    } catch (const std::exception &e) {
      os << "[iitepi::contract_space_t::network_topology_analytics] error="
         << e.what() << "\n";
    }
  }

  const std::string discovery_grammar =
      has_non_ws_ascii(snapshot->vicreg_network_design.grammar)
          ? snapshot->vicreg_network_design.grammar
          : (fallback_network_design_grammar.has_value()
                 ? *fallback_network_design_grammar
                 : std::string{});

  if (has_non_ws_ascii(discovery_grammar)) {
    for (const char *section_name_c : {"ASSEMBLY"}) {
      const std::string section_name = section_name_c;
      const auto sec_it = snapshot->config.find(section_name);
      if (sec_it == snapshot->config.end())
        continue;
      for (const auto &[key, raw_value] : sec_it->second) {
        const bool key_is_single_path = is_single_path_key(key);
        const bool key_is_list_path = is_list_path_key(key);
        if (!key_is_single_path && !key_is_list_path)
          continue;

        std::vector<std::string> path_values{};
        if (key_is_list_path) {
          std::string list_raw = trim_ascii_ws_copy(raw_value);
          if (list_raw.size() >= 2 && list_raw.front() == '[' &&
              list_raw.back() == ']') {
            list_raw =
                trim_ascii_ws_copy(list_raw.substr(1, list_raw.size() - 2));
          }
          path_values = split_string_items(list_raw);
        } else {
          path_values.push_back(raw_value);
        }

        for (auto raw_path : path_values) {
          raw_path =
              unquote_if_wrapped(trim_ascii_ws_copy(std::move(raw_path)));
          if (!has_non_ws_ascii(raw_path))
            continue;
          const std::string resolved_path = resolve_path_from_folder(
              parent_folder_from_path(snapshot->config_file_path), raw_path);
          if (!has_non_ws_ascii(resolved_path))
            continue;
          if (!std::filesystem::exists(resolved_path) ||
              !std::filesystem::is_regular_file(resolved_path)) {
            continue;
          }

          const std::string canonical_path =
              canonicalize_path_best_effort(resolved_path);
          const std::string path_key =
              has_non_ws_ascii(canonical_path) ? canonical_path : resolved_path;
          if (!ends_with_ascii(path_key, ".dsl"))
            continue;
          if (!seen_network_paths.insert(path_key).second)
            continue;

          const std::string dsl_text =
              piaabo::dfiles::readFileToString(resolved_path);
          if (!has_non_ws_ascii(dsl_text))
            continue;

          try {
            const auto design =
                cuwacunu::camahjucunu::dsl::decode_network_design_from_dsl(
                    discovery_grammar, dsl_text);
            std::string source_label = hash;
            source_label += ":";
            source_label += section_name;
            source_label += ".";
            source_label += key;
            source_label += ":";
            source_label += path_key;
            append_decoded(source_label, design);
          } catch (...) {
            // not a network-design DSL payload; skip.
          }
        }
      }
    }
  }

  if (decoded_networks.empty()) {
    if (beautify) {
      os << "\x1b[1;96mNetwork Design Analytics Report\x1b[0m\n";
      os << "\t\x1b[90msource_label\x1b[0m : \x1b[97m" << hash << ":"
         << snapshot->config_file_path_canonical << "\x1b[0m\n";
      os << "\t\x1b[90mnetwork_design\x1b[0m : \x1b[93mabsent\x1b[0m\n";
    } else {
      os << "schema="
         << cuwacunu::piaabo::torch_compat::kNetworkDesignAnalyticsSchemaCurrent
         << "\n";
      os << "source_label=" << hash << ":"
         << snapshot->config_file_path_canonical << "\n";
      os << "network_design=absent\n";
    }
    return;
  }

  if (beautify) {
    os << "\t\x1b[1;95mnetworks_detected\x1b[0m : \x1b[97m"
       << decoded_networks.size() << "\x1b[0m\n";
  }

  for (std::size_t i = 0; i < decoded_networks.size(); ++i) {
    const auto &entry = decoded_networks[i];
    const std::string payload =
        beautify ? cuwacunu::piaabo::torch_compat::
                       network_design_analytics_to_pretty_text(
                           entry.report, entry.source_label, /*use_color=*/true)
                 : cuwacunu::piaabo::torch_compat::
                       network_design_analytics_to_latent_lineage_state_text(
                           entry.report, entry.source_label);
    if (beautify) {
      os << "\t\x1b[1;95mnetwork[" << (i + 1) << "/" << decoded_networks.size()
         << "]\x1b[0m\n";
      os << indent_lines(payload, "\t");
    } else {
      os << payload;
    }
    if (!payload.empty() && payload.back() != '\n')
      os << "\n";
  }
}

void contract_space_t::network_parameter_analytics(const contract_hash_t &hash,
                                                   std::ostream *out,
                                                   bool beautify) {
  std::ostream &os = (out != nullptr) ? *out : std::cout;
  const auto snapshot = contract_itself(hash);
  if (!snapshot) {
    os << "[iitepi::contract_space_t::network_parameter_analytics] error=null "
          "contract snapshot\n";
    return;
  }

  os << "[iitepi::contract_space_t::network_parameter_analytics] contract_hash="
     << hash << " path=" << snapshot->config_file_path_canonical << "\n";

  struct parameter_payload_t {
    std::string source_label{};
    std::string report_file{};
    std::string report_schema{};
    bool report_schema_supported{false};
    std::string payload{};
  };

  std::vector<parameter_payload_t> reports{};
  std::unordered_set<std::string> seen_report_files{};
  std::vector<std::string> missing_sidecars{};
  std::unordered_set<std::string> seen_missing{};

  const auto append_if_sidecar_exists = [&](std::string source_label,
                                            const std::string &candidate_path) {
    if (!has_non_ws_ascii(candidate_path))
      return;
    std::filesystem::path report_path(candidate_path);
    bool from_checkpoint = false;
    if (report_path.extension() == ".pt") {
      from_checkpoint = true;
      report_path.replace_extension(".network_analytics.lls");
    } else if (!ends_with_ascii(report_path.string(),
                                ".network_analytics.lls")) {
      return;
    }

    std::error_code ec{};
    if (!std::filesystem::exists(report_path, ec) ||
        !std::filesystem::is_regular_file(report_path, ec)) {
      if (from_checkpoint) {
        const std::string missing = report_path.string();
        if (seen_missing.insert(missing).second) {
          missing_sidecars.push_back(missing);
        }
      }
      return;
    }

    std::string canonical = canonicalize_path_best_effort(report_path.string());
    if (!has_non_ws_ascii(canonical))
      canonical = report_path.string();
    if (!seen_report_files.insert(canonical).second)
      return;

    const std::string payload = piaabo::dfiles::readFileToString(canonical);
    if (!has_non_ws_ascii(payload))
      return;
    const std::string report_schema =
        cuwacunu::piaabo::torch_compat::extract_analytics_kv_schema(payload);
    const bool report_schema_supported =
        cuwacunu::piaabo::torch_compat::is_supported_network_analytics_schema(
            report_schema);
    reports.push_back(parameter_payload_t{
        .source_label = std::move(source_label),
        .report_file = canonical,
        .report_schema = report_schema,
        .report_schema_supported = report_schema_supported,
        .payload = payload,
    });
  };

  const auto scan_keyed_paths = [&](const std::string &section_name,
                                    const parsed_config_section_t &section) {
    for (const auto &[key, raw_value] : section) {
      const bool key_is_single_path = is_single_path_key(key);
      const bool key_is_list_path = is_list_path_key(key);
      if (!key_is_single_path && !key_is_list_path)
        continue;

      std::vector<std::string> path_values{};
      if (key_is_list_path) {
        std::string list_raw = trim_ascii_ws_copy(raw_value);
        if (list_raw.size() >= 2 && list_raw.front() == '[' &&
            list_raw.back() == ']') {
          list_raw =
              trim_ascii_ws_copy(list_raw.substr(1, list_raw.size() - 2));
        }
        path_values = split_string_items(list_raw);
      } else {
        path_values.push_back(raw_value);
      }

      for (auto raw_path : path_values) {
        raw_path = unquote_if_wrapped(trim_ascii_ws_copy(std::move(raw_path)));
        if (!has_non_ws_ascii(raw_path))
          continue;
        const std::string resolved = resolve_path_from_folder(
            parent_folder_from_path(snapshot->config_file_path), raw_path);
        if (!has_non_ws_ascii(resolved))
          continue;
        if (!std::filesystem::exists(resolved) ||
            !std::filesystem::is_regular_file(resolved)) {
          continue;
        }

        std::string source = hash;
        source += ":";
        source += section_name;
        source += ".";
        source += key;
        source += ":";
        source += canonicalize_path_best_effort(resolved);
        append_if_sidecar_exists(source, resolved);
      }
    }
  };

  for (const auto &section_name : {"ASSEMBLY"}) {
    const auto sec_it = snapshot->config.find(section_name);
    if (sec_it == snapshot->config.end())
      continue;
    scan_keyed_paths(section_name, sec_it->second);
  }
  for (const auto &[section_name, section] : snapshot->module_sections) {
    scan_keyed_paths(section_name, section);
  }

  if (reports.empty()) {
    if (beautify) {
      os << "\x1b[1;96mNetwork Parameter Analytics Report\x1b[0m\n";
      os << "\t\x1b[90msource_label\x1b[0m : \x1b[97m" << hash << ":"
         << snapshot->config_file_path_canonical << "\x1b[0m\n";
      os << "\t\x1b[90mparameter_analytics\x1b[0m : \x1b[93mabsent\x1b[0m\n";
    } else {
      os << "schema="
         << cuwacunu::piaabo::torch_compat::kNetworkAnalyticsSchemaCurrent
         << "\n";
      os << "source_label=" << hash << ":"
         << snapshot->config_file_path_canonical << "\n";
      os << "parameter_analytics=absent\n";
    }
    for (const auto &missing : missing_sidecars) {
      os << "[iitepi::contract_space_t::network_parameter_analytics] "
            "missing_sidecar="
         << missing << "\n";
    }
    return;
  }

  if (beautify) {
    os << "\t\x1b[1;95mparameter_reports_detected\x1b[0m : \x1b[97m"
       << reports.size() << "\x1b[0m\n";
  }

  for (std::size_t i = 0; i < reports.size(); ++i) {
    const auto &item = reports[i];
    if (beautify) {
      os << "\t\x1b[1;95mparameter_report[" << (i + 1) << "/" << reports.size()
         << "]\x1b[0m\n";
      os << "\t\t\x1b[90msource_label\x1b[0m : \x1b[97m" << item.source_label
         << "\x1b[0m\n";
      os << "\t\t\x1b[90mreport_file\x1b[0m : \x1b[97m" << item.report_file
         << "\x1b[0m\n";
      os << "\t\t\x1b[90mreport_schema\x1b[0m : \x1b[97m"
         << non_empty_or(item.report_schema, "<missing>") << "\x1b[0m\n";
      os << "\t\t\x1b[90mreport_schema_supported\x1b[0m : \x1b[97m"
         << (item.report_schema_supported ? "true" : "false") << "\x1b[0m\n";
      os << indent_lines(item.payload, "\t\t");
      if (!item.payload.empty() && item.payload.back() != '\n')
        os << "\n";
    } else {
      os << "source_label=" << item.source_label << "\n";
      os << "report_file=" << item.report_file << "\n";
      os << "report_schema=" << item.report_schema << "\n";
      os << "report_schema_supported="
         << (item.report_schema_supported ? "true" : "false") << "\n";
      os << item.payload;
      if (!item.payload.empty() && item.payload.back() != '\n')
        os << "\n";
    }
  }

  for (const auto &missing : missing_sidecars) {
    os << "[iitepi::contract_space_t::network_parameter_analytics] "
          "missing_sidecar="
       << missing << "\n";
  }
}

void contract_space_t::network_analytics(const contract_hash_t &hash,
                                         std::ostream *out, bool beautify,
                                         network_analytics_mode_e mode) {
  std::ostream &os = (out != nullptr) ? *out : std::cout;
  switch (mode) {
  case network_analytics_mode_e::Topology:
    network_topology_analytics(hash, &os, beautify);
    return;
  case network_analytics_mode_e::Parameters:
    network_parameter_analytics(hash, &os, beautify);
    return;
  case network_analytics_mode_e::Both:
    network_topology_analytics(hash, &os, beautify);
    if (beautify) {
      os << "\t\x1b[1;94mParameter Analytics\x1b[0m\n";
    }
    network_parameter_analytics(hash, &os, beautify);
    return;
  }
}

} // namespace iitepi
} // namespace cuwacunu
