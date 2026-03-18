#include "iitepi/config_space_t.h"

#include "iitepi/runtime_binding_space_t.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <filesystem>
#include <iterator>
#include <string_view>

namespace cuwacunu {
namespace iitepi {

std::mutex config_mutex;
exchange_type_e config_space_t::exchange_type;
std::string config_space_t::config_file_path;
std::string config_space_t::runtime_campaign_dsl_override_path;
parsed_config_t config_space_t::config;

[[nodiscard]] static std::string strip_comment(std::string_view line,
                                               bool* in_block_comment);
[[nodiscard]] static bool has_non_ws_ascii(const std::string& s);
[[nodiscard]] static std::string trim_ascii_ws_copy(std::string s);
[[nodiscard]] static parsed_config_t parse_config_file(const std::string& path);
[[nodiscard]] static std::vector<std::string> split_string_items(
    const std::string& s);
[[nodiscard]] static std::string resolve_path_from_folder(
    const std::string& folder, std::string path);
[[nodiscard]] static std::string canonicalize_path_best_effort(
    const std::string& path);

template <class T>
static T parse_scalar_from_string(const std::string& s);

/*──────────────────────── shared helpers ─────────────────────────────*/
[[nodiscard]] static std::string strip_comment(std::string_view line,
                                               bool* in_block_comment) {
  bool block_comment = (in_block_comment != nullptr) ? *in_block_comment : false;
  bool in_single = false;
  bool in_double = false;
  std::string out;
  out.reserve(line.size());

  for (std::size_t i = 0; i < line.size();) {
    const char c = line[i];
    const char next = (i + 1 < line.size()) ? line[i + 1] : '\0';

    if (block_comment) {
      if (c == '*' && next == '/') {
        block_comment = false;
        i += 2;
      } else {
        ++i;
      }
      continue;
    }

    if (!in_single && !in_double && c == '/' && next == '*') {
      block_comment = true;
      i += 2;
      continue;
    }

    if (c == '\'' && !in_double) {
      in_single = !in_single;
      out.push_back(c);
      ++i;
    } else if (c == '"' && !in_single) {
      in_double = !in_double;
      out.push_back(c);
      ++i;
    } else if ((c == '#' || c == ';') && !in_single && !in_double) {
      break;
    } else {
      out.push_back(c);
      ++i;
    }
  }
  if (in_block_comment != nullptr) *in_block_comment = block_comment;
  return out;
}

[[nodiscard]] static std::vector<std::string> split_string_items(
    const std::string& s) {
  std::vector<std::string> out;
  std::string cur;
  bool in_single = false;
  bool in_double = false;

  auto push_trimmed = [&](std::string& token) {
    std::size_t b = 0;
    std::size_t e = token.size();
    while (b < e && std::isspace(static_cast<unsigned char>(token[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(token[e - 1]))) --e;
    if (e > b) out.emplace_back(token.substr(b, e - b));
    token.clear();
  };

  for (std::size_t i = 0; i < s.size(); ++i) {
    const char c = s[i];
    if (c == '\'' && !in_double) {
      in_single = !in_single;
      cur.push_back(c);
      continue;
    }
    if (c == '"' && !in_single) {
      in_double = !in_double;
      cur.push_back(c);
      continue;
    }
    if (c == ',' && !in_single && !in_double) {
      push_trimmed(cur);
    } else {
      cur.push_back(c);
    }
  }
  push_trimmed(cur);

  for (auto& item : out) {
    if (item.size() < 2) continue;
    const char a = item.front();
    const char b = item.back();
    if ((a == '\'' && b == '\'') || (a == '"' && b == '"')) {
      item = item.substr(1, item.size() - 2);
    }
  }
  return out;
}

[[nodiscard]] static bool has_non_ws_ascii(const std::string& s) {
  for (const unsigned char c : s) {
    if (!std::isspace(c)) return true;
  }
  return false;
}

[[nodiscard]] static std::string trim_ascii_ws_copy(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

[[nodiscard]] static std::string resolve_path_from_folder(const std::string& folder,
                                                          std::string path) {
  path = trim_ascii_ws_copy(std::move(path));
  if (path.empty()) return {};
  const std::filesystem::path p(path);
  if (p.is_absolute()) return path;
  if (folder.empty()) return path;
  return (std::filesystem::path(folder) / p).string();
}

[[nodiscard]] static std::string canonicalize_path_best_effort(
    const std::string& path) {
  if (!has_non_ws_ascii(path)) return {};
  std::error_code ec;
  std::filesystem::path p(path);
  if (!p.is_absolute()) {
    p = std::filesystem::absolute(p, ec);
    if (ec) ec.clear();
  }
  const std::filesystem::path weak = std::filesystem::weakly_canonical(p, ec);
  if (!ec) return weak.string();
  ec.clear();
  const std::filesystem::path normal = p.lexically_normal();
  return normal.string();
}

[[nodiscard]] static std::string global_config_parent_path(
    const std::string& config_path) {
  const std::string trimmed = trim_ascii_ws_copy(config_path);
  if (!has_non_ws_ascii(trimmed)) return {};
  const std::filesystem::path p(trimmed);
  if (!p.has_parent_path()) return {};
  return p.parent_path().string();
}

[[nodiscard]] static parsed_config_t parse_config_file(const std::string& path) {
  std::ifstream file = cuwacunu::piaabo::dfiles::readFileToStream(path);
  if (!file) {
    log_fatal("Cannot open config file: %s\n", path.c_str());
  }

  parsed_config_t parsed;
  std::string raw;
  std::string current;
  bool in_block_comment = false;
  while (std::getline(file, raw)) {
    std::string line = piaabo::trim_string(strip_comment(raw, &in_block_comment));
    if (line.empty()) continue;

    if (line.front() == '[' && line.back() == ']') {
      current = piaabo::trim_string(line.substr(1, line.size() - 2));
      parsed[current];
      continue;
    }

    const std::size_t pos = line.find('=');
    if (pos == std::string::npos) {
      log_warn("Skipping malformed line in %s: %s\n", path.c_str(), raw.c_str());
      continue;
    }

    std::string key = piaabo::trim_string(line.substr(0, pos));
    std::string value = piaabo::trim_string(line.substr(pos + 1));
    parsed[current][key] = value;
  }
  return parsed;
}

template <class T>
static T parse_scalar_from_string(const std::string& s) {
  if constexpr (std::is_same_v<T, std::string>) {
    return s;
  } else if constexpr (std::is_same_v<T, bool>) {
    const std::string trimmed = trim_ascii_ws_copy(s);
    std::string v;
    v.reserve(trimmed.size());
    std::transform(
        trimmed.begin(), trimmed.end(), std::back_inserter(v),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (v == "1" || v == "true" || v == "yes" || v == "y" || v == "on") {
      return true;
    }
    if (v == "0" || v == "false" || v == "no" || v == "n" || v == "off") {
      return false;
    }
    throw bad_config_access("Invalid bool '" + s + "'");
  } else if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>) {
    T v{};
    const auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    if (ec != std::errc() || ptr != s.data() + s.size()) {
      throw bad_config_access("Invalid integer '" + s + "'");
    }
    return v;
  } else if constexpr (std::is_floating_point_v<T>) {
    char* end{};
    errno = 0;
    const long double ld = std::strtold(s.c_str(), &end);
    if (errno || end != s.data() + s.size()) {
      throw bad_config_access("Invalid float '" + s + "'");
    }
    return static_cast<T>(ld);
  } else {
    static_assert(!std::is_same_v<T, T>,
                  "from_string<T>() not specialized for this T");
  }
}

/*──────────────────────── global config space ───────────────────────────*/
std::string config_space_t::raw(const std::string& section, const std::string& key) {
  LOCK_GUARD(config_mutex);
  const auto s = config.find(section);
  if (s == config.end()) throw bad_config_access("Missing section [" + section + "]");
  const auto k = s->second.find(key);
  if (k == s->second.end()) {
    throw bad_config_access("Missing key <" + key + "> in [" + section + "]");
  }
  return k->second;
}

template <class T>
T config_space_t::from_string(const std::string& s) {
  return parse_scalar_from_string<T>(s);
}

template <class T>
T config_space_t::get(const std::string& section, const std::string& key,
                      std::optional<T> fallback) {
  try {
    return from_string<T>(raw(section, key));
  } catch (const bad_config_access&) {
    if (fallback) return *fallback;
    throw;
  }
}

template <class T>
std::vector<T> config_space_t::get_arr(const std::string& section,
                                       const std::string& key,
                                       std::optional<std::vector<T>> fallback) {
  try {
    const std::string s = raw(section, key);
    const std::vector<std::string> items = split_string_items(s);
    std::vector<T> result;
    result.reserve(items.size());
    for (const auto& it : items) result.emplace_back(from_string<T>(it));
    return result;
  } catch (const bad_config_access&) {
    if (fallback) return *fallback;
    throw;
  }
}

parsed_config_t config_space_t::read_config(const std::string& path) {
  return parse_config_file(path);
}

void config_space_t::change_config_file(const char* config_path) {
  std::string requested = config_path ? config_path : DEFAULT_GLOBAL_CONFIG_PATH;
  requested = trim_ascii_ws_copy(std::move(requested));
  if (!has_non_ws_ascii(requested)) requested = DEFAULT_GLOBAL_CONFIG_PATH;
  config_file_path = canonicalize_path_best_effort(requested);
  update_config();
}

void config_space_t::set_runtime_campaign_dsl_override(const std::string& path) {
  LOCK_GUARD(config_mutex);
  runtime_campaign_dsl_override_path = trim_ascii_ws_copy(path);
}

void config_space_t::clear_runtime_campaign_dsl_override() {
  LOCK_GUARD(config_mutex);
  runtime_campaign_dsl_override_path.clear();
}

std::string config_space_t::effective_campaign_dsl_path() {
  std::string config_path_copy{};
  std::string override_copy{};
  std::string configured_campaign_path{};
  {
    LOCK_GUARD(config_mutex);
    config_path_copy = config_file_path;
    override_copy = runtime_campaign_dsl_override_path;
    const auto sec_it = config.find("GENERAL");
    if (sec_it != config.end()) {
      const auto key_it =
          sec_it->second.find(GENERAL_DEFAULT_IITEPI_CAMPAIGN_DSL_KEY);
      if (key_it != sec_it->second.end()) {
        configured_campaign_path = key_it->second;
      }
    }
  }
  const std::string chosen =
      has_non_ws_ascii(override_copy)
          ? override_copy
          : (has_non_ws_ascii(configured_campaign_path)
                 ? configured_campaign_path
                 : DEFAULT_IITEPI_CAMPAIGN_DSL_FILE);
  return resolve_path_from_folder(global_config_parent_path(config_path_copy), chosen);
}

std::string config_space_t::locked_campaign_hash() {
  runtime_binding_space_t::init();
  return runtime_binding_space_t::locked_runtime_binding_hash();
}

std::string config_space_t::locked_campaign_path_canonical() {
  runtime_binding_space_t::init();
  return runtime_binding_space_t::locked_campaign_path_canonical();
}

std::string config_space_t::locked_binding_id() {
  runtime_binding_space_t::init();
  return runtime_binding_space_t::locked_binding_id();
}

void config_space_t::update_config() {
  if (!std::filesystem::exists(config_file_path)) {
    log_warn("[dconfig] global config file %s does not exist\n",
             config_file_path.c_str());
  }
  parsed_config_t parsed = read_config(config_file_path);
  {
    LOCK_GUARD(config_mutex);
    config = std::move(parsed);
  }
  validate_config();

  const std::string t = config_space_t::get<std::string>("GENERAL", "exchange_type");
  exchange_type_e exchange_type_temp = exchange_type_e::NONE;
  if (t == "REAL") {
    exchange_type_temp = exchange_type_e::REAL;
  } else if (t == "TEST") {
    exchange_type_temp = exchange_type_e::TEST;
  } else {
    log_fatal("[dconfig] invalid GENERAL.exchange_type value: %s\n", t.c_str());
  }
  log_info("[dconfig] exchange_type = %s\n", t.c_str());

  if (exchange_type != exchange_type_e::NONE && exchange_type_temp != exchange_type) {
    log_fatal(
        "(config_space_t.cpp)[update_config] mid-course changes to exchange_type are "
        "not permitted\n");
  }
  exchange_type = exchange_type_temp;

  const std::string configured_campaign_path = effective_campaign_dsl_path();
  const std::string configured_campaign_canonical =
      canonicalize_path_best_effort(configured_campaign_path);
  if (!has_non_ws_ascii(configured_campaign_canonical)) {
    log_fatal("[dconfig] invalid configured campaign path: %s\n",
              configured_campaign_path.c_str());
  }

  if (runtime_binding_space_t::is_initialized()) {
    const std::string locked_campaign_canonical =
        runtime_binding_space_t::locked_campaign_path_canonical();
    if (configured_campaign_canonical != locked_campaign_canonical) {
      log_fatal(
          "[dconfig] immutable runtime lock violation: configured campaign path "
          "changed mid-run (configured=%s, locked=%s)\n",
          configured_campaign_canonical.c_str(),
          locked_campaign_canonical.c_str());
    }
    runtime_binding_space_t::assert_locked_runtime_intact_or_fail_fast();
  }
}

bool config_space_t::validate_config() {
  const std::string config_base_folder = global_config_parent_path(config_file_path);
  bool ok = true;

  const auto require_value =
      [&](const char* section, const char* key, std::string* out_value) -> bool {
    const auto sec_it = config.find(section);
    if (sec_it == config.end()) {
      log_warn("Missing section [%s]\n", section);
      ok = false;
      return false;
    }

    const auto key_it = sec_it->second.find(key);
    if (key_it == sec_it->second.end()) {
      log_warn("Missing field <%s> in section [%s]\n", key, section);
      ok = false;
      return false;
    }

    const std::string value = trim_ascii_ws_copy(key_it->second);
    if (!has_non_ws_ascii(value)) {
      log_warn("Empty field <%s> in section [%s]\n", key, section);
      ok = false;
      return false;
    }

    if (out_value) *out_value = value;
    return true;
  };

  auto require_int_with_bounds = [&](const char* section, const char* key,
                                     int64_t min_allowed) {
    std::string value;
    if (!require_value(section, key, &value)) return;
    try {
      const int64_t parsed = parse_scalar_from_string<int64_t>(value);
      if (parsed < min_allowed) {
        log_warn(
            "Invalid value <%s> in section [%s]: expected >= %lld, got %lld\n",
            key, section, static_cast<long long>(min_allowed),
            static_cast<long long>(parsed));
        ok = false;
      }
    } catch (const std::exception& e) {
      log_warn("Invalid integer value <%s> in section [%s]: %s\n",
               key, section, e.what());
      ok = false;
    }
  };

  auto validate_optional_bool = [&](const char* section, const char* key) {
    const auto sec_it = config.find(section);
    if (sec_it == config.end()) return;
    const auto key_it = sec_it->second.find(key);
    if (key_it == sec_it->second.end()) return;
    const std::string value = trim_ascii_ws_copy(key_it->second);
    if (!has_non_ws_ascii(value)) return;
    try {
      (void)parse_scalar_from_string<bool>(value);
    } catch (const std::exception& e) {
      log_warn("Invalid bool value <%s> in section [%s]: %s\n",
               key, section, e.what());
      ok = false;
    }
  };

  auto validate_optional_logs_metadata_filter = [&](const char* section,
                                                    const char* key) {
    const auto sec_it = config.find(section);
    if (sec_it == config.end()) return;
    const auto key_it = sec_it->second.find(key);
    if (key_it == sec_it->second.end()) return;
    std::string value = trim_ascii_ws_copy(key_it->second);
    std::transform(
        value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (!has_non_ws_ascii(value)) return;
    if (value == "any" ||
        value == "meta+" || value == "meta" || value == "any_meta" || value == "with_any_metadata" ||
        value == "fn+" || value == "function" || value == "with_function" ||
        value == "path+" || value == "path" || value == "with_path" ||
        value == "callsite+" || value == "callsite" || value == "with_callsite") {
      return;
    }
    log_warn(
        "Invalid value <%s> in section [%s]: expected any|any_meta|function|path|callsite\n",
        key, section);
    ok = false;
  };

  const auto require_existing_path =
      [&](const char* section, const char* key, const std::string& base_folder) {
        std::string raw_path;
        if (!require_value(section, key, &raw_path)) return;

        const std::string resolved = resolve_path_from_folder(base_folder, raw_path);
        if (!has_non_ws_ascii(resolved)) {
          log_warn("Unable to resolve path for <%s> in section [%s]\n", key, section);
          ok = false;
          return;
        }
        if (!std::filesystem::exists(resolved)) {
          log_warn("Configured path does not exist for <%s> in section [%s]: %s\n",
                   key, section, resolved.c_str());
          ok = false;
          return;
        }
        if (!std::filesystem::is_regular_file(resolved)) {
          log_warn("Configured path is not a regular file for <%s> in section [%s]: %s\n",
                   key, section, resolved.c_str());
          ok = false;
        }
      };

  std::string exchange_type_value;
  if (require_value("GENERAL", "exchange_type", &exchange_type_value)) {
    if (exchange_type_value != "TEST" && exchange_type_value != "REAL") {
      log_warn(
          "Invalid value <exchange_type> in section [GENERAL]: expected TEST or REAL, got %s\n",
          exchange_type_value.c_str());
      ok = false;
    }
  }

  std::string campaign_cfg_path = DEFAULT_IITEPI_CAMPAIGN_DSL_FILE;
  if (require_value("GENERAL", GENERAL_DEFAULT_IITEPI_CAMPAIGN_DSL_KEY,
                    &campaign_cfg_path)) {
    const std::string resolved =
        resolve_path_from_folder(config_base_folder, campaign_cfg_path);
    if (!has_non_ws_ascii(resolved) || !std::filesystem::exists(resolved)) {
      log_warn(
          "Configured campaign file does not exist: %s (resolved: %s)\n",
          campaign_cfg_path.c_str(),
          resolved.c_str());
      ok = false;
    } else if (!std::filesystem::is_regular_file(resolved)) {
      log_warn(
          "Configured campaign file is not a regular file: %s (resolved: %s)\n",
          campaign_cfg_path.c_str(),
          resolved.c_str());
      ok = false;
    }
  }
  {
    LOCK_GUARD(config_mutex);
    if (has_non_ws_ascii(runtime_campaign_dsl_override_path)) {
      const std::string resolved = resolve_path_from_folder(
          config_base_folder, runtime_campaign_dsl_override_path);
      if (!has_non_ws_ascii(resolved) || !std::filesystem::exists(resolved)) {
        log_warn("Runtime campaign override does not exist: %s (resolved: %s)\n",
                 runtime_campaign_dsl_override_path.c_str(), resolved.c_str());
        ok = false;
      } else if (!std::filesystem::is_regular_file(resolved)) {
        log_warn(
            "Runtime campaign override is not a regular file: %s (resolved: %s)\n",
            runtime_campaign_dsl_override_path.c_str(), resolved.c_str());
        ok = false;
      }
    }
  }

  require_int_with_bounds("GUI", "iinuji_logs_buffer_capacity", 1);
  validate_optional_bool("GUI", "iinuji_logs_show_date");
  validate_optional_bool("GUI", "iinuji_logs_show_thread");
  validate_optional_bool("GUI", "iinuji_logs_show_metadata");
  validate_optional_bool("GUI", "iinuji_logs_show_color");
  validate_optional_bool("GUI", "iinuji_logs_auto_follow");
  validate_optional_bool("GUI", "iinuji_logs_mouse_capture");
  validate_optional_logs_metadata_filter(
      "GUI", "iinuji_logs_metadata_filter");
  (void)require_value("GENERAL", "hashimyei_store_root", nullptr);
  (void)require_value("GENERAL", "hashimyei_metadata_secret", nullptr);

  constexpr const char* kBnfKeys[] = {
      "iitepi_campaign_grammar_filename",
      "iitepi_runtime_binding_grammar_filename",
      "iitepi_wave_grammar_filename",
      "vicreg_grammar_filename",
      "value_estimation_grammar_filename",
      "wikimyei_evaluation_embedding_sequence_analytics_grammar_filename",
      "wikimyei_evaluation_transfer_matrix_evaluation_grammar_filename",
      "observation_sources_grammar_filename",
      "observation_channels_grammar_filename",
      "jkimyei_specs_grammar_filename",
      "tsiemene_circuit_grammar_filename",
      "canonical_path_grammar_filename",
  };
  for (const char* key : kBnfKeys) {
    require_existing_path("BNF", key, config_base_folder);
  }

  constexpr const char* kExchangeKeys[] = {
      "Ed25519_pkey",
      "EXCHANGE_api_filename",
      "websocket_url",
  };
  for (const char* key : kExchangeKeys) {
    (void)require_value("TEST_EXCHANGE", key, nullptr);
    (void)require_value("REAL_EXCHANGE", key, nullptr);
  }

  std::string hero_mode_value;
  if (require_value("REAL_HERO", "mode", &hero_mode_value)) {
    std::string mode_normalized = hero_mode_value;
    std::transform(mode_normalized.begin(), mode_normalized.end(),
                   mode_normalized.begin(), [](unsigned char c) {
                     return static_cast<char>(std::tolower(c));
                   });
    if (mode_normalized != "openai" && mode_normalized != "selfhosted") {
      log_warn("Invalid value <mode> in section [REAL_HERO]: expected openai or selfhosted, got %s\n",
               mode_normalized.c_str());
      ok = false;
    } else if (mode_normalized == "selfhosted") {
      log_warn("Invalid value <mode> in section [REAL_HERO]: isufficient founds for self hosted model deployment, please change mode to openai.\n");
      ok = false;
    }
  }

  std::string hero_transport;
  if (require_value("REAL_HERO", "transport", &hero_transport)) {
    std::transform(hero_transport.begin(), hero_transport.end(),
                   hero_transport.begin(), [](unsigned char c) {
                     return static_cast<char>(std::tolower(c));
                   });
    if (hero_transport != "curl") {
      log_warn("Invalid value <transport> in section [REAL_HERO]: expected curl, got %s\n",
               hero_transport.c_str());
      ok = false;
    }
  }
  (void)require_value("REAL_HERO", "OPENAI_api_filename", nullptr);
  (void)require_value("REAL_HERO", "endpoint", nullptr);
  require_existing_path("REAL_HERO", "config_hero_dsl_filename", config_base_folder);
  require_existing_path("REAL_HERO", "hashimyei_hero_dsl_filename", config_base_folder);
  require_existing_path("REAL_HERO", "lattice_hero_dsl_filename", config_base_folder);
  require_existing_path("REAL_HERO", "runtime_hero_dsl_filename", config_base_folder);

  if (!ok) log_terminate_gracefully("Invalid global configuration, aborting.\n");
  return ok;
}

std::string config_space_t::websocket_url() {
  LOCK_GUARD(config_mutex);
  const char* section = (config_space_t::exchange_type == exchange_type_e::REAL)
                            ? "REAL_EXCHANGE"
                            : "TEST_EXCHANGE";
  const auto sec_it = config_space_t::config.find(section);
  if (sec_it == config_space_t::config.end()) {
    throw bad_config_access("Missing section [" + std::string(section) + "]");
  }
  const auto key_it = sec_it->second.find("websocket_url");
  if (key_it == sec_it->second.end()) {
    throw bad_config_access("Missing key <websocket_url> in [" + std::string(section) + "]");
  }
  return key_it->second;
}

std::string config_space_t::api_key() {
  LOCK_GUARD(config_mutex);
  const char* section = (config_space_t::exchange_type == exchange_type_e::REAL)
                            ? "REAL_EXCHANGE"
                            : "TEST_EXCHANGE";
  const auto sec_it = config_space_t::config.find(section);
  if (sec_it == config_space_t::config.end()) {
    throw bad_config_access("Missing section [" + std::string(section) + "]");
  }
  const auto key_it = sec_it->second.find("EXCHANGE_api_filename");
  if (key_it == sec_it->second.end()) {
    throw bad_config_access("Missing key <EXCHANGE_api_filename> in [" +
                            std::string(section) + "]");
  }
  return key_it->second;
}

std::string config_space_t::Ed25519_pkey() {
  LOCK_GUARD(config_mutex);
  const char* section = (config_space_t::exchange_type == exchange_type_e::REAL)
                            ? "REAL_EXCHANGE"
                            : "TEST_EXCHANGE";
  const auto sec_it = config_space_t::config.find(section);
  if (sec_it == config_space_t::config.end()) {
    throw bad_config_access("Missing section [" + std::string(section) + "]");
  }
  const auto key_it = sec_it->second.find("Ed25519_pkey");
  if (key_it == sec_it->second.end()) {
    throw bad_config_access("Missing key <Ed25519_pkey> in [" + std::string(section) + "]");
  }
  return key_it->second;
}

std::string config_space_t::hero_mode() {
  LOCK_GUARD(config_mutex);
  const auto sec_it = config_space_t::config.find("REAL_HERO");
  if (sec_it == config_space_t::config.end()) {
    throw bad_config_access("Missing section [REAL_HERO]");
  }
  const auto key_it = sec_it->second.find("mode");
  if (key_it == sec_it->second.end()) {
    throw bad_config_access("Missing key <mode> in [REAL_HERO]");
  }
  return key_it->second;
}

std::string config_space_t::hero_api_key_filename() {
  LOCK_GUARD(config_mutex);
  const auto sec_it = config_space_t::config.find("REAL_HERO");
  if (sec_it == config_space_t::config.end()) {
    throw bad_config_access("Missing section [REAL_HERO]");
  }
  const auto key_it = sec_it->second.find("OPENAI_api_filename");
  if (key_it == sec_it->second.end()) {
    throw bad_config_access("Missing key <OPENAI_api_filename> in [REAL_HERO]");
  }
  return key_it->second;
}

std::string config_space_t::locked_runtime_binding_hash() {
  return locked_campaign_hash();
}

/*──────────────────────────── life-cycle hooks ───────────────────────────*/
void config_space_t::finit() { log_info("[dconfig] finalizing\n"); }
void config_space_t::init() {
  log_info("[dconfig] initializing\n");
  exchange_type = exchange_type_e::NONE;
  change_config_file();  // defaults to global .config
}
config_space_t::_init config_space_t::_initializer;

/*──────────────── explicit template instantiations ───────────────────────*/
template int64_t config_space_t::get<int64_t>(const std::string&, const std::string&,
                                              std::optional<int64_t>);
template int config_space_t::get<int>(const std::string&, const std::string&,
                                      std::optional<int>);
template double config_space_t::get<double>(const std::string&, const std::string&,
                                            std::optional<double>);
template float config_space_t::get<float>(const std::string&, const std::string&,
                                          std::optional<float>);
template bool config_space_t::get<bool>(const std::string&, const std::string&,
                                        std::optional<bool>);
template std::string config_space_t::get<std::string>(
    const std::string&, const std::string&, std::optional<std::string>);

template std::vector<int64_t> config_space_t::get_arr<int64_t>(
    const std::string&, const std::string&, std::optional<std::vector<int64_t>>);
template std::vector<int> config_space_t::get_arr<int>(
    const std::string&, const std::string&, std::optional<std::vector<int>>);
template std::vector<double> config_space_t::get_arr<double>(
    const std::string&, const std::string&, std::optional<std::vector<double>>);
template std::vector<float> config_space_t::get_arr<float>(
    const std::string&, const std::string&, std::optional<std::vector<float>>);
template std::vector<bool> config_space_t::get_arr<bool>(
    const std::string&, const std::string&, std::optional<std::vector<bool>>);
template std::vector<std::string> config_space_t::get_arr<std::string>(
    const std::string&, const std::string&, std::optional<std::vector<std::string>>);

}  // namespace iitepi
}  // namespace cuwacunu
