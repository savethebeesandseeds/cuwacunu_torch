#include "iitepi/config_space_t.h"

#include "iitepi/board_space_t.h"

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
std::string config_space_t::config_folder;
std::string config_space_t::config_file_path;
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
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    if (ec != std::errc()) throw bad_config_access("Invalid integer '" + s + "'");
    (void)ptr;
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

void config_space_t::change_config_file(const char* folder, const char* file) {
  config_folder = folder ? folder : DEFAULT_CONFIG_FOLDER;
  std::string requested_file = file ? file : DEFAULT_CONFIG_FILE;
  if (!has_non_ws_ascii(requested_file)) requested_file = DEFAULT_CONFIG_FILE;
  config_file_path = resolve_path_from_folder(config_folder, requested_file);
  update_config();
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

  const std::string configured_board_path = resolve_path_from_folder(
      config_folder,
      config_space_t::get<std::string>("GENERAL", GENERAL_BOARD_CONFIG_KEY));
  const std::string configured_board_canonical =
      canonicalize_path_best_effort(configured_board_path);
  if (!has_non_ws_ascii(configured_board_canonical)) {
    log_fatal("[dconfig] invalid configured board path: %s\n",
              configured_board_path.c_str());
  }
  const std::string configured_binding_id = trim_ascii_ws_copy(
      config_space_t::get<std::string>("GENERAL", GENERAL_BOARD_BINDING_KEY));
  if (!has_non_ws_ascii(configured_binding_id)) {
    log_fatal("[dconfig] invalid configured board binding id\n");
  }

  if (board_space_t::is_initialized()) {
    board_space_t::init(configured_board_canonical, configured_binding_id);
    board_space_t::assert_locked_runtime_intact_or_fail_fast();
  }
}

bool config_space_t::validate_config() {
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

  auto require_bool = [&](const char* section, const char* key) {
    std::string value;
    if (!require_value(section, key, &value)) return;
    try {
      (void)parse_scalar_from_string<bool>(value);
    } catch (const std::exception& e) {
      log_warn("Invalid bool value <%s> in section [%s]: %s\n",
               key, section, e.what());
      ok = false;
    }
  };

  auto require_optional_int_with_bounds = [&](const char* section, const char* key,
                                              int64_t min_allowed) {
    const auto sec_it = config.find(section);
    if (sec_it == config.end()) return;
    const auto key_it = sec_it->second.find(key);
    if (key_it == sec_it->second.end()) return;

    const std::string value = trim_ascii_ws_copy(key_it->second);
    if (!has_non_ws_ascii(value)) {
      log_warn("Empty optional field <%s> in section [%s]\n", key, section);
      ok = false;
      return;
    }

    try {
      const int64_t parsed = parse_scalar_from_string<int64_t>(value);
      if (parsed < min_allowed) {
        log_warn(
            "Invalid optional value <%s> in section [%s]: expected >= %lld, got %lld\n",
            key, section, static_cast<long long>(min_allowed),
            static_cast<long long>(parsed));
        ok = false;
      }
    } catch (const std::exception& e) {
      log_warn("Invalid optional integer value <%s> in section [%s]: %s\n",
               key, section, e.what());
      ok = false;
    }
  };

  auto require_optional_double_with_bounds = [&](const char* section, const char* key,
                                                 long double min_allowed,
                                                 bool allow_equal_min) {
    const auto sec_it = config.find(section);
    if (sec_it == config.end()) return;
    const auto key_it = sec_it->second.find(key);
    if (key_it == sec_it->second.end()) return;

    const std::string value = trim_ascii_ws_copy(key_it->second);
    if (!has_non_ws_ascii(value)) {
      log_warn("Empty optional field <%s> in section [%s]\n", key, section);
      ok = false;
      return;
    }

    try {
      const long double parsed = parse_scalar_from_string<long double>(value);
      const bool valid = allow_equal_min ? (parsed >= min_allowed) : (parsed > min_allowed);
      if (!valid) {
        log_warn(
            "Invalid optional value <%s> in section [%s]: expected %s %.3Le, got %.3Le\n",
            key, section, allow_equal_min ? ">=" : ">",
            static_cast<long double>(min_allowed),
            static_cast<long double>(parsed));
        ok = false;
      }
    } catch (const std::exception& e) {
      log_warn("Invalid optional floating value <%s> in section [%s]: %s\n",
               key, section, e.what());
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

  std::string board_cfg_path;
  if (require_value("GENERAL", GENERAL_BOARD_CONFIG_KEY, &board_cfg_path)) {
    const std::string resolved =
        resolve_path_from_folder(config_folder, board_cfg_path);
    if (!has_non_ws_ascii(resolved) || !std::filesystem::exists(resolved)) {
      log_warn(
          "Configured board file does not exist: %s (resolved: %s)\n",
          board_cfg_path.c_str(),
          resolved.c_str());
      ok = false;
    }
  }
  (void)require_value("GENERAL", GENERAL_BOARD_BINDING_KEY, nullptr);

  require_int_with_bounds("GENERAL", "iinuji_logs_buffer_capacity", 1);
  (void)require_value("GENERAL", "hashimyei_store_root", nullptr);
  (void)require_value("GENERAL", "hashimyei_metadata_secret", nullptr);

  require_int_with_bounds("DATA_LOADER", "dataloader_workers", 0);
  require_optional_int_with_bounds("DATA_LOADER", "dataloader_range_warn_batches", 1);
  require_optional_int_with_bounds("DATA_LOADER", "dataloader_csv_bootstrap_deltas", 2);
  require_optional_double_with_bounds("DATA_LOADER", "dataloader_csv_step_abs_tol", 0.0L, false);
  require_optional_double_with_bounds("DATA_LOADER", "dataloader_csv_step_rel_tol", 0.0L, true);
  {
    const auto dl_it = config.find("DATA_LOADER");
    if (dl_it != config.end() &&
        dl_it->second.find("dataloader_batch_size") != dl_it->second.end()) {
      log_warn(
          "[dconfig] DATA_LOADER.dataloader_batch_size is removed; "
          "use WAVE.BATCH_SIZE\n");
      ok = false;
    }
  }

  {
    bool has_force_rebuild_cache = false;
    bool has_force_binarization_alias = false;

    const auto dl_it = config.find("DATA_LOADER");
    if (dl_it != config.end()) {
      const auto fetch_trimmed_if_present = [&](const char* key,
                                                std::string* out) -> bool {
        const auto key_it = dl_it->second.find(key);
        if (key_it == dl_it->second.end()) return false;
        if (out) *out = trim_ascii_ws_copy(key_it->second);
        return true;
      };
      has_force_rebuild_cache = fetch_trimmed_if_present(
          "dataloader_force_rebuild_cache", nullptr);
      has_force_binarization_alias = fetch_trimmed_if_present(
          "dataloader_force_binarization", nullptr);
    }

    if (!has_force_rebuild_cache) {
      log_warn(
          "Missing field <dataloader_force_rebuild_cache> in section [DATA_LOADER]\n");
      ok = false;
    }

    if (has_force_rebuild_cache) {
      require_bool("DATA_LOADER", "dataloader_force_rebuild_cache");
    }
    if (has_force_binarization_alias) {
      log_warn(
          "[dconfig] DATA_LOADER.dataloader_force_binarization is removed; "
          "use DATA_LOADER.dataloader_force_rebuild_cache\n");
      ok = false;
    }
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

  if (!ok) log_terminate_gracefully("Invalid global configuration, aborting.\n");
  return ok;
}

std::string config_space_t::websocket_url() {
  return config_space_t::config[config_space_t::exchange_type == exchange_type_e::REAL
                                    ? "REAL_EXCHANGE"
                                    : "TEST_EXCHANGE"]["websocket_url"];
}

std::string config_space_t::api_key() {
  return config_space_t::config[config_space_t::exchange_type == exchange_type_e::REAL
                                    ? "REAL_EXCHANGE"
                                    : "TEST_EXCHANGE"]["EXCHANGE_api_filename"];
}

std::string config_space_t::Ed25519_pkey() {
  return config_space_t::config[config_space_t::exchange_type == exchange_type_e::REAL
                                    ? "REAL_EXCHANGE"
                                    : "TEST_EXCHANGE"]["Ed25519_pkey"];
}

std::string config_space_t::locked_board_hash() {
  board_space_t::init();
  return board_space_t::locked_board_hash();
}

std::string config_space_t::locked_board_path_canonical() {
  board_space_t::init();
  return board_space_t::locked_board_path_canonical();
}

std::string config_space_t::locked_board_binding_id() {
  board_space_t::init();
  return board_space_t::locked_board_binding_id();
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
