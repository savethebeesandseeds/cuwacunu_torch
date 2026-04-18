#include "iitepi/runtime_binding/runtime_binding_space_t.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "camahjucunu/dsl/iitepi_campaign/iitepi_campaign.h"
#include "camahjucunu/dsl/runtime_binding_instruction/runtime_binding_instruction.h"
#include "camahjucunu/dsl/iitepi_wave/iitepi_wave.h"
#include "hero/wave_contract_binding_runtime.h"
#include "iitepi/contract_space_t.h"
#include "iitepi/wave_space_t.h"

namespace cuwacunu {
namespace iitepi {

std::mutex runtime_binding_config_mutex;

namespace {

using runtime_binding_ptr_t = std::shared_ptr<const runtime_binding_record_t>;

static bool g_runtime_binding_runtime_lock_initialized = false;
static std::string g_locked_campaign_path_canonical;
static runtime_binding_hash_t g_locked_runtime_binding_hash;
static std::string g_locked_binding_id;
static std::string g_locked_runtime_source_sha256;
static std::string g_last_requested_binding_id;
static std::size_t g_registered_runtime_binding_count = 0;
static runtime_binding_hash_t g_last_registered_runtime_binding_hash;
static std::string g_last_registered_runtime_binding_path_canonical;
[[nodiscard]] static std::unordered_map<runtime_binding_hash_t, runtime_binding_ptr_t>&
runtime_bindings_by_hash() {
  static std::unordered_map<runtime_binding_hash_t, runtime_binding_ptr_t> registry;
  return registry;
}

[[nodiscard]] static std::unordered_map<std::string, runtime_binding_hash_t>&
hash_by_runtime_binding_path() {
  static std::unordered_map<std::string, runtime_binding_hash_t> registry;
  return registry;
}

[[nodiscard]] static std::string strip_comment(std::string_view line,
                                               bool* in_block_comment);
[[nodiscard]] static bool has_non_ws_ascii(const std::string& s);
[[nodiscard]] static std::string trim_ascii_ws_copy(std::string s);
[[nodiscard]] static std::string decode_escaped_text(std::string s);
[[nodiscard]] static parsed_config_t parse_config_file(const std::string& path);
[[nodiscard]] static std::vector<std::string> split_string_items(
    const std::string& s);
[[nodiscard]] static bool has_suffix_case_insensitive(std::string_view text,
                                                      std::string_view suffix);
[[nodiscard]] static std::string resolve_path_from_folder(
    const std::string& folder,
    std::string path);
[[nodiscard]] static std::string canonicalize_path_best_effort(
    const std::string& path);
[[nodiscard]] static std::string sha256_hex_from_bytes(const void* data,
                                                       std::size_t size);
[[nodiscard]] static std::string sha256_hex_from_file(const std::string& path);
[[nodiscard]] static std::int64_t file_mtime_ticks(
    const std::filesystem::path& path);
[[nodiscard]] static runtime_binding_file_fingerprint_t fingerprint_file(
    const std::string& path);
[[nodiscard]] static std::string compute_manifest_digest_hex(
    const std::vector<runtime_binding_file_fingerprint_t>& files);
[[nodiscard]] static std::string runtime_binding_required_resolved_path(
    const parsed_config_t& cfg,
    const std::string& cfg_folder,
    const char* section,
    const char* key);
[[nodiscard]] static std::string snapshot_runtime_binding_dsl_value_or_empty(
    const parsed_config_t& cfg,
    const char* key);
[[nodiscard]] static std::shared_ptr<runtime_binding_record_t>
build_runtime_binding_record_from_path(const std::string& runtime_binding_file_path);
[[nodiscard]] static runtime_binding_ptr_t runtime_binding_ptr_or_fail(const runtime_binding_hash_t& hash);
[[nodiscard]] static std::vector<runtime_binding_ptr_t> registry_runtime_bindings_copy_locked();
[[nodiscard]] static std::string active_bind_id_or_fail(
    const runtime_binding_ptr_t& runtime_binding_itself);
[[nodiscard]] static std::string read_text_file_or_throw(
    const std::filesystem::path& path);
[[nodiscard]] static bool looks_like_campaign_dsl(std::string_view text);
[[nodiscard]] static std::string default_campaign_grammar_path();
[[nodiscard]] static std::string select_campaign_binding_id_or_throw(
    const cuwacunu::camahjucunu::iitepi_campaign_instruction_t& instruction,
    std::string binding_override);
[[nodiscard]] static std::string build_internal_runtime_binding_text_from_campaign(
    const std::filesystem::path& campaign_path,
    const cuwacunu::camahjucunu::iitepi_campaign_instruction_t& instruction,
    const std::string& binding_id);
[[nodiscard]] static std::string materialize_internal_runtime_binding_snapshot_from_campaign(
    const std::string& campaign_path, const std::string& binding_override,
    std::string* out_binding_id);
static bool validate_runtime_binding_config_or_terminate(const parsed_config_t& cfg,
                                               const std::string& cfg_folder);

template <class T>
static T parse_scalar_from_string(const std::string& s);

[[nodiscard]] static const char* non_empty_or(const std::string& s,
                                              const char* fallback) {
  return has_non_ws_ascii(s) ? s.c_str() : fallback;
}

[[nodiscard]] static std::string read_text_file_or_throw(
    const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("cannot open file for read: " + path.string());
  }
  return std::string(std::istreambuf_iterator<char>(in),
                     std::istreambuf_iterator<char>());
}

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

[[nodiscard]] static bool has_suffix_case_insensitive(std::string_view text,
                                                      std::string_view suffix) {
  if (suffix.size() > text.size()) return false;
  const std::size_t offset = text.size() - suffix.size();
  for (std::size_t i = 0; i < suffix.size(); ++i) {
    const unsigned char a = static_cast<unsigned char>(text[offset + i]);
    const unsigned char b = static_cast<unsigned char>(suffix[i]);
    if (std::tolower(a) != std::tolower(b)) return false;
  }
  return true;
}

[[nodiscard]] static bool has_non_ws_ascii(const std::string& s) {
  for (const unsigned char c : s) {
    if (!std::isspace(c)) return true;
  }
  return false;
}

[[nodiscard]] static bool looks_like_campaign_dsl(std::string_view text) {
  bool in_block_comment = false;
  std::string first_token{};
  for (std::size_t pos = 0; pos <= text.size();) {
    const std::size_t end = text.find('\n', pos);
    const std::string raw_line =
        (end == std::string::npos) ? std::string(text.substr(pos))
                                   : std::string(text.substr(pos, end - pos));
    const std::string line =
        trim_ascii_ws_copy(strip_comment(raw_line, &in_block_comment));
    if (has_non_ws_ascii(line)) {
      for (const char ch : line) {
        if (std::isspace(static_cast<unsigned char>(ch)) != 0 || ch == '{' ||
            ch == ';') {
          break;
        }
        first_token.push_back(ch);
      }
      break;
    }
    if (end == std::string::npos) break;
    pos = end + 1;
  }
  return first_token == "CAMPAIGN";
}

[[nodiscard]] static std::string trim_ascii_ws_copy(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

[[nodiscard]] static std::string unquote_if_wrapped(std::string s) {
  s = trim_ascii_ws_copy(std::move(s));
  if (s.size() >= 2) {
    const char a = s.front();
    const char b = s.back();
    if ((a == '"' && b == '"') || (a == '\'' && b == '\'')) {
      return s.substr(1, s.size() - 2);
    }
  }
  return s;
}

[[nodiscard]] static std::string decode_escaped_text(std::string s) {
  s = unquote_if_wrapped(std::move(s));
  std::string out;
  out.reserve(s.size());
  for (std::size_t i = 0; i < s.size(); ++i) {
    const char c = s[i];
    if (c == '\\' && i + 1 < s.size()) {
      const char n = s[++i];
      switch (n) {
        case 'n':
          out.push_back('\n');
          break;
        case 'r':
          out.push_back('\r');
          break;
        case 't':
          out.push_back('\t');
          break;
        case '\\':
          out.push_back('\\');
          break;
        case '"':
          out.push_back('"');
          break;
        case '\'':
          out.push_back('\'');
          break;
        default:
          out.push_back(n);
          break;
      }
      continue;
    }
    out.push_back(c);
  }
  return out;
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

[[nodiscard]] static std::string parent_folder_from_path(const std::string& path) {
  const std::filesystem::path p(path);
  if (!p.has_parent_path()) return {};
  return p.parent_path().string();
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

[[nodiscard]] static std::string default_campaign_grammar_path() {
  const std::string raw = trim_ascii_ws_copy(config_space_t::get<std::string>(
      "BNF", "iitepi_campaign_grammar_filename", std::string{}));
  if (!has_non_ws_ascii(raw)) {
    log_fatal(
        "[dconfig] missing [BNF].iitepi_campaign_grammar_filename in active global config\n");
  }
  const std::string resolved = resolve_path_from_folder(
      parent_folder_from_path(config_space_t::config_file_path), raw);
  if (!has_non_ws_ascii(resolved)) {
    log_fatal("[dconfig] unable to resolve [BNF].iitepi_campaign_grammar_filename\n");
  }
  return resolved;
}

[[nodiscard]] static std::string select_campaign_binding_id_or_throw(
    const cuwacunu::camahjucunu::iitepi_campaign_instruction_t& instruction,
    std::string binding_override) {
  binding_override = trim_ascii_ws_copy(std::move(binding_override));
  if (has_non_ws_ascii(binding_override)) return binding_override;
  if (!instruction.runs.empty() &&
      has_non_ws_ascii(instruction.runs.front().bind_ref)) {
    return instruction.runs.front().bind_ref;
  }
  throw std::runtime_error("campaign does not define any RUN entries");
}

[[nodiscard]] static std::string build_internal_runtime_binding_text_from_campaign(
    const std::filesystem::path& campaign_path,
    const cuwacunu::camahjucunu::iitepi_campaign_instruction_t& instruction,
    const std::string& binding_id) {
  const auto bind_it = std::find_if(
      instruction.binds.begin(), instruction.binds.end(),
      [&](const auto& bind) { return bind.id == binding_id; });
  if (bind_it == instruction.binds.end()) {
    throw std::runtime_error("campaign references unknown binding id: " +
                             binding_id);
  }
  const auto contract_it = std::find_if(
      instruction.contracts.begin(), instruction.contracts.end(),
      [&](const auto& contract) { return contract.id == bind_it->contract_ref; });
  if (contract_it == instruction.contracts.end()) {
    throw std::runtime_error("binding references unknown contract id: " +
                             bind_it->contract_ref);
  }

  std::ostringstream out;
  out << "ACTIVE_BIND " << binding_id << ";\n\n";
  out << "RUNTIME_BINDING {\n";
  for (const auto& contract : instruction.contracts) {
    out << "  IMPORT_CONTRACT \""
        << resolve_path_from_folder(campaign_path.parent_path().string(),
                                    contract.file)
        << "\" AS " << contract.id << ";\n";
  }
  out << "\n";
  for (const auto& wave : instruction.waves) {
    out << "  FROM \""
        << resolve_path_from_folder(campaign_path.parent_path().string(),
                                    wave.file)
        << "\" IMPORT_WAVE " << wave.id << ";\n";
  }
  out << "\n";
  out << "  BIND " << bind_it->id << " {\n";
  for (const auto& variable : bind_it->variables) {
    out << "    " << variable.name << " = " << variable.value << ";\n";
  }
  out << "    CONTRACT = " << bind_it->contract_ref << ";\n";
  out << "    WAVE = " << bind_it->wave_ref << ";\n";
  out << "  };\n";
  out << "}\n";
  return out.str();
}

[[nodiscard]] static std::string materialize_internal_runtime_binding_snapshot_from_campaign(
    const std::string& campaign_path, const std::string& binding_override,
    std::string* out_binding_id) {
  if (out_binding_id) out_binding_id->clear();
  const std::filesystem::path campaign_file_path(campaign_path);
  const std::string campaign_text = read_text_file_or_throw(campaign_file_path);
  const auto instruction =
      cuwacunu::camahjucunu::dsl::decode_iitepi_campaign_from_dsl(
          read_text_file_or_throw(default_campaign_grammar_path()), campaign_text);
  const std::string binding_id =
      select_campaign_binding_id_or_throw(instruction, binding_override);
  const std::string identity_seed =
      canonicalize_path_best_effort(campaign_path) + "|" + binding_id;
  const std::string unique =
      sha256_hex_from_bytes(identity_seed.data(), identity_seed.size()).substr(0,
                                                                              16);
  const std::filesystem::path campaign_snapshot_root =
      std::filesystem::temp_directory_path() /
      ("iitepi.internal." + unique + ".campaign_snapshot");
  cuwacunu::hero::wave_contract_binding_runtime::
      resolved_wave_contract_binding_snapshot_t campaign_snapshot{};
  std::string campaign_snapshot_error{};
  if (!cuwacunu::hero::wave_contract_binding_runtime::
          prepare_campaign_binding_snapshot(campaign_file_path, binding_id,
                                            campaign_snapshot_root,
                                            &campaign_snapshot,
                                            &campaign_snapshot_error)) {
    throw std::runtime_error("cannot materialize bind-mounted campaign snapshot: " +
                             campaign_snapshot_error);
  }
  const std::string snapshot_campaign_text =
      read_text_file_or_throw(campaign_snapshot.campaign_dsl_path);
  const auto snapshot_instruction =
      cuwacunu::camahjucunu::dsl::decode_iitepi_campaign_from_dsl(
          read_text_file_or_throw(default_campaign_grammar_path()),
          snapshot_campaign_text);
  const std::string runtime_binding_text =
      build_internal_runtime_binding_text_from_campaign(
          std::filesystem::path(campaign_snapshot.campaign_dsl_path),
          snapshot_instruction, binding_id);
  const std::filesystem::path snapshot_path =
      std::filesystem::temp_directory_path() /
      ("iitepi.internal." + unique + ".runtime_binding.dsl");
  std::ofstream out(snapshot_path, std::ios::binary | std::ios::trunc);
  if (!out) {
    throw std::runtime_error("cannot create internal runtime-binding snapshot: " +
                             snapshot_path.string());
  }
  out << runtime_binding_text;
  out.flush();
  if (!out.good()) {
    throw std::runtime_error("cannot write internal runtime-binding snapshot: " +
                             snapshot_path.string());
  }
  if (out_binding_id) *out_binding_id = binding_id;
  return snapshot_path.string();
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

[[nodiscard]] static std::string sha256_hex_from_bytes(const void* data,
                                                       std::size_t size) {
  struct sha256_state_t {
    std::array<std::uint32_t, 8> h{};
    std::array<std::uint8_t, 64> block{};
    std::size_t block_len{0};
    std::uint64_t total_bits{0};
  };

  constexpr std::array<std::uint32_t, 64> k = {
      0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu,
      0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u, 0xd807aa98u, 0x12835b01u,
      0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u,
      0xc19bf174u, 0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
      0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau, 0x983e5152u,
      0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u,
      0x06ca6351u, 0x14292967u, 0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu,
      0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
      0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u,
      0xd6990624u, 0xf40e3585u, 0x106aa070u, 0x19a4c116u, 0x1e376c08u,
      0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu,
      0x682e6ff3u, 0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
      0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u};

  const auto rotr = [](std::uint32_t v, std::uint32_t n) noexcept {
    return (v >> n) | (v << (32U - n));
  };
  const auto ch = [](std::uint32_t x, std::uint32_t y, std::uint32_t z) noexcept {
    return (x & y) ^ (~x & z);
  };
  const auto maj = [](std::uint32_t x, std::uint32_t y, std::uint32_t z) noexcept {
    return (x & y) ^ (x & z) ^ (y & z);
  };
  const auto big_sigma0 = [&](std::uint32_t x) noexcept {
    return rotr(x, 2U) ^ rotr(x, 13U) ^ rotr(x, 22U);
  };
  const auto big_sigma1 = [&](std::uint32_t x) noexcept {
    return rotr(x, 6U) ^ rotr(x, 11U) ^ rotr(x, 25U);
  };
  const auto small_sigma0 = [&](std::uint32_t x) noexcept {
    return rotr(x, 7U) ^ rotr(x, 18U) ^ (x >> 3U);
  };
  const auto small_sigma1 = [&](std::uint32_t x) noexcept {
    return rotr(x, 17U) ^ rotr(x, 19U) ^ (x >> 10U);
  };

  const auto transform = [&](sha256_state_t* s, const std::uint8_t* block) {
    std::array<std::uint32_t, 64> w{};
    for (std::size_t i = 0; i < 16; ++i) {
      const std::size_t j = i * 4;
      w[i] = (static_cast<std::uint32_t>(block[j]) << 24U) |
             (static_cast<std::uint32_t>(block[j + 1]) << 16U) |
             (static_cast<std::uint32_t>(block[j + 2]) << 8U) |
             (static_cast<std::uint32_t>(block[j + 3]));
    }
    for (std::size_t i = 16; i < 64; ++i) {
      w[i] = small_sigma1(w[i - 2]) + w[i - 7] + small_sigma0(w[i - 15]) +
             w[i - 16];
    }

    std::uint32_t a = s->h[0];
    std::uint32_t b = s->h[1];
    std::uint32_t c = s->h[2];
    std::uint32_t d = s->h[3];
    std::uint32_t e = s->h[4];
    std::uint32_t f = s->h[5];
    std::uint32_t g = s->h[6];
    std::uint32_t h = s->h[7];

    for (std::size_t i = 0; i < 64; ++i) {
      const std::uint32_t t1 =
          h + big_sigma1(e) + ch(e, f, g) + k[i] + w[i];
      const std::uint32_t t2 = big_sigma0(a) + maj(a, b, c);
      h = g;
      g = f;
      f = e;
      e = d + t1;
      d = c;
      c = b;
      b = a;
      a = t1 + t2;
    }

    s->h[0] += a;
    s->h[1] += b;
    s->h[2] += c;
    s->h[3] += d;
    s->h[4] += e;
    s->h[5] += f;
    s->h[6] += g;
    s->h[7] += h;
  };

  const auto init = [&](sha256_state_t* s) {
    s->h = {0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
            0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u};
    s->block.fill(0);
    s->block_len = 0;
    s->total_bits = 0;
  };

  const auto update = [&](sha256_state_t* s, const std::uint8_t* bytes,
                          std::size_t len) {
    for (std::size_t i = 0; i < len; ++i) {
      s->block[s->block_len++] = bytes[i];
      if (s->block_len == 64) {
        transform(s, s->block.data());
        s->total_bits += 512;
        s->block_len = 0;
      }
    }
  };

  const auto final = [&](sha256_state_t* s, std::array<std::uint8_t, 32>* out) {
    s->total_bits += static_cast<std::uint64_t>(s->block_len) * 8ULL;

    s->block[s->block_len++] = 0x80u;
    if (s->block_len > 56) {
      while (s->block_len < 64) s->block[s->block_len++] = 0x00u;
      transform(s, s->block.data());
      s->block_len = 0;
    }
    while (s->block_len < 56) s->block[s->block_len++] = 0x00u;

    for (int i = 7; i >= 0; --i) {
      s->block[s->block_len++] =
          static_cast<std::uint8_t>((s->total_bits >> (i * 8)) & 0xFFu);
    }
    transform(s, s->block.data());

    for (std::size_t i = 0; i < 8; ++i) {
      (*out)[i * 4] = static_cast<std::uint8_t>((s->h[i] >> 24U) & 0xFFu);
      (*out)[i * 4 + 1] =
          static_cast<std::uint8_t>((s->h[i] >> 16U) & 0xFFu);
      (*out)[i * 4 + 2] = static_cast<std::uint8_t>((s->h[i] >> 8U) & 0xFFu);
      (*out)[i * 4 + 3] = static_cast<std::uint8_t>(s->h[i] & 0xFFu);
    }
  };

  sha256_state_t state{};
  init(&state);
  if (size > 0 && data) {
    update(&state, static_cast<const std::uint8_t*>(data), size);
  }
  std::array<std::uint8_t, 32> digest{};
  final(&state, &digest);

  static constexpr char kHex[] = "0123456789abcdef";
  std::string out;
  out.resize(digest.size() * 2);
  for (std::size_t i = 0; i < digest.size(); ++i) {
    const unsigned char b = digest[i];
    out[2 * i] = kHex[(b >> 4) & 0x0F];
    out[2 * i + 1] = kHex[b & 0x0F];
  }
  return out;
}

[[nodiscard]] static std::string sha256_hex_from_file(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    log_fatal("[dconfig] cannot open file to fingerprint: %s\n", path.c_str());
  }
  std::ostringstream oss;
  oss << in.rdbuf();
  const std::string bytes = oss.str();
  return sha256_hex_from_bytes(bytes.data(), bytes.size());
}

[[nodiscard]] static std::int64_t file_mtime_ticks(
    const std::filesystem::path& path) {
  std::error_code ec;
  const auto tp = std::filesystem::last_write_time(path, ec);
  if (ec) return 0;
  using ticks_t =
      std::chrono::duration<std::int64_t, std::chrono::nanoseconds::period>;
  return std::chrono::duration_cast<ticks_t>(tp.time_since_epoch()).count();
}

[[nodiscard]] static runtime_binding_file_fingerprint_t fingerprint_file(
    const std::string& path) {
  const std::string canonical = canonicalize_path_best_effort(path);
  if (!has_non_ws_ascii(canonical)) {
    log_fatal("[dconfig] cannot fingerprint empty path\n");
  }

  const std::filesystem::path p(canonical);
  if (!std::filesystem::exists(p)) {
    log_fatal("[dconfig] fingerprint path does not exist: %s\n", canonical.c_str());
  }
  if (!std::filesystem::is_regular_file(p)) {
    log_fatal("[dconfig] fingerprint path is not a regular file: %s\n",
              canonical.c_str());
  }

  runtime_binding_file_fingerprint_t fp{};
  fp.canonical_path = canonical;
  fp.file_size_bytes = std::filesystem::file_size(p);
  fp.mtime_ticks = file_mtime_ticks(p);
  fp.sha256_hex = sha256_hex_from_file(canonical);
  return fp;
}

[[nodiscard]] static std::string compute_manifest_digest_hex(
    const std::vector<runtime_binding_file_fingerprint_t>& files) {
  std::vector<std::string> rows;
  rows.reserve(files.size());
  for (const auto& f : files) {
    rows.emplace_back(f.canonical_path + "|" + f.sha256_hex + "\n");
  }
  std::sort(rows.begin(), rows.end());

  std::string payload;
  for (const auto& row : rows) payload += row;
  return sha256_hex_from_bytes(payload.data(), payload.size());
}

[[nodiscard]] static std::string runtime_binding_required_resolved_path(
    const parsed_config_t& cfg,
    const std::string& cfg_folder,
    const char* section,
    const char* key) {
  const auto sec_it = cfg.find(section);
  if (sec_it == cfg.end()) {
    log_fatal("[dconfig] missing runtime-binding section [%s]\n", section);
  }
  const auto key_it = sec_it->second.find(key);
  if (key_it == sec_it->second.end()) {
    log_fatal("[dconfig] missing runtime-binding key <%s> in section [%s]\n", key, section);
  }
  const std::string raw = trim_ascii_ws_copy(key_it->second);
  if (!has_non_ws_ascii(raw)) {
    log_fatal("[dconfig] empty runtime-binding key <%s> in section [%s]\n", key, section);
  }
  const std::string resolved = resolve_path_from_folder(cfg_folder, raw);
  if (!has_non_ws_ascii(resolved)) {
    log_fatal("[dconfig] unable to resolve runtime-binding path <%s> in [%s]\n", key, section);
  }
  if (!std::filesystem::exists(resolved)) {
    log_fatal("[dconfig] runtime-binding dependency path does not exist: %s\n",
              resolved.c_str());
  }
  return resolved;
}

[[nodiscard]] static std::string global_required_resolved_path(
    const char* section,
    const char* key) {
  const auto sec_it = config_space_t::config.find(section);
  if (sec_it == config_space_t::config.end()) {
    log_fatal("[dconfig] missing global section [%s]\n", section);
  }
  const auto key_it = sec_it->second.find(key);
  if (key_it == sec_it->second.end()) {
    log_fatal("[dconfig] missing global key <%s> in section [%s]\n", key, section);
  }
  const std::string raw = trim_ascii_ws_copy(key_it->second);
  if (!has_non_ws_ascii(raw)) {
    log_fatal("[dconfig] empty global key <%s> in section [%s]\n", key, section);
  }
  const std::string resolved =
      resolve_path_from_folder(parent_folder_from_path(config_space_t::config_file_path),
                               raw);
  if (!has_non_ws_ascii(resolved)) {
    log_fatal("[dconfig] unable to resolve global path <%s> in [%s]\n", key, section);
  }
  if (!std::filesystem::exists(resolved)) {
    log_fatal("[dconfig] global dependency path does not exist: %s\n",
              resolved.c_str());
  }
  return resolved;
}

[[nodiscard]] static std::string resolve_internal_runtime_binding_grammar_path() {
  const auto sec_it = config_space_t::config.find("BNF");
  if (sec_it != config_space_t::config.end()) {
    const auto key_it = sec_it->second.find("iitepi_runtime_binding_grammar_filename");
    if (key_it != sec_it->second.end()) {
      const std::string raw = trim_ascii_ws_copy(key_it->second);
      if (has_non_ws_ascii(raw)) {
        const std::string resolved =
            resolve_path_from_folder(
                parent_folder_from_path(config_space_t::config_file_path), raw);
        if (has_non_ws_ascii(resolved) && std::filesystem::exists(resolved)) {
          return resolved;
        }
      }
    }
  }
  log_fatal(
      "[dconfig] missing/invalid [BNF].iitepi_runtime_binding_grammar_filename in active global config\n");
  return {};
}

[[nodiscard]] static std::string snapshot_runtime_binding_dsl_value_or_empty(
    const parsed_config_t& cfg,
    const char* key) {
  const auto sec_it = cfg.find("RUNTIME_BINDING_DSL");
  if (sec_it == cfg.end()) return {};
  const auto key_it = sec_it->second.find(key);
  if (key_it == sec_it->second.end()) return {};
  const std::string raw = key_it->second;
  if (!has_non_ws_ascii(raw)) return {};
  return decode_escaped_text(raw);
}

static bool validate_runtime_binding_config_or_terminate(const parsed_config_t& cfg,
                                               const std::string& cfg_folder) {
  bool ok = true;
  const auto require_existing_path =
      [&](const char* section, const char* key) {
        const auto sec_it = cfg.find(section);
        if (sec_it == cfg.end()) {
          log_warn("Missing runtime-binding section [%s]\n", section);
          ok = false;
          return;
        }
        const auto key_it = sec_it->second.find(key);
        if (key_it == sec_it->second.end()) {
          log_warn("Missing runtime-binding key <%s> in section [%s]\n", key, section);
          ok = false;
          return;
        }
        const std::string raw = trim_ascii_ws_copy(key_it->second);
        if (!has_non_ws_ascii(raw)) {
          log_warn("Empty runtime-binding key <%s> in section [%s]\n", key, section);
          ok = false;
          return;
        }
        const std::string resolved = resolve_path_from_folder(cfg_folder, raw);
        if (!has_non_ws_ascii(resolved) || !std::filesystem::exists(resolved)) {
          log_warn("Configured runtime-binding path does not exist for <%s> in [%s]: %s\n",
                   key, section, resolved.c_str());
          ok = false;
        }
      };

  if (!std::filesystem::exists(resolve_internal_runtime_binding_grammar_path())) {
    log_warn("Internal runtime-binding grammar path does not exist: %s\n",
             resolve_internal_runtime_binding_grammar_path().c_str());
    ok = false;
  }
  require_existing_path("DSL", "iitepi_runtime_binding_dsl_filename");

  if (!ok) {
    log_terminate_gracefully("Invalid runtime-binding configuration, aborting.\n");
  }
  return ok;
}

[[nodiscard]] static std::shared_ptr<runtime_binding_record_t>
build_runtime_binding_record_from_path(const std::string& runtime_binding_file_path) {
  const std::string resolved_runtime_binding_path =
      canonicalize_path_best_effort(runtime_binding_file_path);
  if (!has_non_ws_ascii(resolved_runtime_binding_path)) {
    log_fatal("[dconfig] cannot resolve runtime-binding path from: %s\n",
              runtime_binding_file_path.c_str());
  }
  if (!std::filesystem::exists(resolved_runtime_binding_path)) {
    log_fatal("[dconfig] runtime-binding path does not exist: %s\n",
              resolved_runtime_binding_path.c_str());
  }

  std::string runtime_binding_folder;
  const std::filesystem::path runtime_binding_path(resolved_runtime_binding_path);
  if (runtime_binding_path.has_parent_path()) {
    runtime_binding_folder = runtime_binding_path.parent_path().string();
  }

  const bool runtime_binding_path_is_dsl =
      has_suffix_case_insensitive(resolved_runtime_binding_path, ".dsl");

  auto record = std::make_shared<runtime_binding_record_t>();
  record->config_file_path = resolved_runtime_binding_path;
  record->config_file_path_canonical =
      canonicalize_path_best_effort(resolved_runtime_binding_path);
  std::string dsl_path = resolved_runtime_binding_path;
  if (!runtime_binding_path_is_dsl) {
    parsed_config_t parsed = parse_config_file(resolved_runtime_binding_path);
    (void)validate_runtime_binding_config_or_terminate(parsed, runtime_binding_folder);
    record->config = parsed;
    dsl_path = runtime_binding_required_resolved_path(
        record->config,
        runtime_binding_folder,
        "DSL",
        "iitepi_runtime_binding_dsl_filename");
  }

  const std::string grammar_path = resolve_internal_runtime_binding_grammar_path();

  std::set<std::string> dependency_paths;
  dependency_paths.insert(record->config_file_path_canonical);
  dependency_paths.insert(canonicalize_path_best_effort(grammar_path));
  if (!runtime_binding_path_is_dsl) {
    dependency_paths.insert(canonicalize_path_best_effort(dsl_path));
  }

  record->runtime_binding.grammar = piaabo::dfiles::readFileToString(grammar_path);
  if (runtime_binding_path_is_dsl) {
    record->runtime_binding.dsl = piaabo::dfiles::readFileToString(dsl_path);
  } else {
    record->runtime_binding.dsl =
        snapshot_runtime_binding_dsl_value_or_empty(record->config, "iitepi_runtime_binding_dsl_text");
    if (!has_non_ws_ascii(record->runtime_binding.dsl)) {
      record->runtime_binding.dsl = piaabo::dfiles::readFileToString(dsl_path);
    }
  }

  if (!has_non_ws_ascii(record->runtime_binding.grammar)) {
    log_fatal("[dconfig] missing effective runtime-binding grammar payload\n");
  }
  if (!has_non_ws_ascii(record->runtime_binding.dsl)) {
    log_fatal("[dconfig] missing effective runtime-binding DSL payload\n");
  }

  for (const auto& dep_path : dependency_paths) {
    if (!has_non_ws_ascii(dep_path)) continue;
    record->dependency_manifest.files.push_back(fingerprint_file(dep_path));
  }
  record->dependency_manifest.dependency_manifest_aggregate_sha256_hex =
      compute_manifest_digest_hex(record->dependency_manifest.files);
  return record;
}

[[nodiscard]] static runtime_binding_ptr_t runtime_binding_ptr_or_fail(const runtime_binding_hash_t& hash) {
  LOCK_GUARD(runtime_binding_config_mutex);
  auto& bindings_by_hash = runtime_bindings_by_hash();
  const auto it = bindings_by_hash.find(hash);
  if (it == bindings_by_hash.end() || !it->second) {
    log_fatal(
        "[dconfig] runtime-binding hash lookup failed: hash=%s is not registered in runtime registry\n",
        hash.c_str());
  }
  return it->second;
}

[[nodiscard]] static std::vector<runtime_binding_ptr_t> registry_runtime_bindings_copy_locked() {
  auto& bindings_by_hash = runtime_bindings_by_hash();
  std::vector<runtime_binding_ptr_t> out;
  out.reserve(bindings_by_hash.size());
  for (const auto& [_, ptr] : bindings_by_hash) {
    if (ptr) out.push_back(ptr);
  }
  return out;
}

[[nodiscard]] static const cuwacunu::camahjucunu::runtime_binding_bind_decl_t*
find_bind_by_id(const cuwacunu::camahjucunu::runtime_binding_instruction_t& instruction,
                const std::string& binding_id) {
  for (const auto& bind : instruction.binds) {
    if (bind.id == binding_id) return &bind;
  }
  return nullptr;
}

[[nodiscard]] static const cuwacunu::camahjucunu::runtime_binding_contract_decl_t*
find_contract_by_id(
    const cuwacunu::camahjucunu::runtime_binding_instruction_t& instruction,
    const std::string& contract_id) {
  for (const auto& contract_decl : instruction.contracts) {
    if (contract_decl.id == contract_id) return &contract_decl;
  }
  return nullptr;
}

[[nodiscard]] static const cuwacunu::camahjucunu::runtime_binding_wave_decl_t*
find_wave_import_by_id(
    const cuwacunu::camahjucunu::runtime_binding_instruction_t& instruction,
    const std::string& wave_id) {
  for (const auto& wave_decl : instruction.waves) {
    if (wave_decl.id == wave_id) return &wave_decl;
  }
  return nullptr;
}

[[nodiscard]] static std::string active_bind_id_or_fail(
    const runtime_binding_ptr_t& runtime_binding_itself) {
  if (!runtime_binding_itself) {
    log_fatal("[iitepi] runtime-binding ACTIVE_BIND resolve received null runtime binding record\n");
  }
  const auto& runtime_binding_instruction = runtime_binding_itself->runtime_binding.decoded();
  const std::string active_bind_id =
      trim_ascii_ws_copy(runtime_binding_instruction.active_bind_id);
  if (!has_non_ws_ascii(active_bind_id)) {
    log_fatal("[iitepi] runtime-binding DSL missing ACTIVE_BIND directive\n");
  }
  if (!find_bind_by_id(runtime_binding_instruction, active_bind_id)) {
    log_fatal("[iitepi] runtime-binding ACTIVE_BIND references unknown BIND id: %s\n",
              active_bind_id.c_str());
  }
  return active_bind_id;
}

static void resolve_and_assert_runtime_binding_dependencies(
    const runtime_binding_ptr_t& runtime_binding_itself,
    const std::optional<std::string>& selected_binding_id,
    const runtime_binding_hash_t& runtime_binding_hash) {
  if (!runtime_binding_itself) {
    log_fatal("[iitepi] runtime-binding dependency resolve received null runtime binding record\n");
  }
  const auto& runtime_binding_instruction = runtime_binding_itself->runtime_binding.decoded();
  const cuwacunu::camahjucunu::runtime_binding_bind_decl_t* selected_bind =
      nullptr;
  if (selected_binding_id.has_value()) {
    const std::string selected_id = trim_ascii_ws_copy(*selected_binding_id);
    selected_bind = find_bind_by_id(runtime_binding_instruction, selected_id);
    if (!selected_bind) {
      log_fatal(
          "[iitepi] configured runtime binding id not found in runtime-binding DSL: %s\n",
          selected_id.c_str());
    }
  }

  std::unordered_map<std::string, std::string> contract_hash_by_id{};
  for (const auto& contract_decl : runtime_binding_instruction.contracts) {
    const std::string resolved_contract_path =
        resolve_path_from_folder(
            parent_folder_from_path(runtime_binding_itself->config_file_path),
            contract_decl.file);
    const auto contract_hash =
        contract_space_t::register_contract_file(resolved_contract_path);
    contract_space_t::assert_intact_or_fail_fast(contract_hash);
    contract_hash_by_id[contract_decl.id] = contract_hash;
  }

  for (const auto& wave_decl : runtime_binding_instruction.waves) {
    const std::string resolved_wave_path =
        resolve_path_from_folder(
            parent_folder_from_path(runtime_binding_itself->config_file_path),
            wave_decl.file);
    const auto wave_hash = wave_space_t::register_wave_file(resolved_wave_path);
    wave_space_t::assert_intact_or_fail_fast(wave_hash);
  }

  for (const auto& bind : runtime_binding_instruction.binds) {
    const auto contract_it = contract_hash_by_id.find(bind.contract_ref);
    if (contract_it == contract_hash_by_id.end()) {
      log_fatal("[iitepi] runtime-binding bind references unknown CONTRACT id: %s\n",
                bind.contract_ref.c_str());
    }
    const auto bound_wave_hash = runtime_binding_space_t::wave_hash_for_binding(runtime_binding_hash, bind.id);
    wave_space_t::assert_intact_or_fail_fast(bound_wave_hash);
  }

  if (selected_bind) {
    const auto selected_contract_hash =
        runtime_binding_space_t::contract_hash_for_binding(runtime_binding_hash, selected_bind->id);
    const auto selected_wave_hash =
        runtime_binding_space_t::wave_hash_for_binding(runtime_binding_hash, selected_bind->id);
    contract_space_t::assert_intact_or_fail_fast(selected_contract_hash);
    wave_space_t::assert_intact_or_fail_fast(selected_wave_hash);
  }
}

}  // namespace

runtime_binding_space_t::_init runtime_binding_space_t::_initializer{};

void runtime_binding_space_t::lifecycle_init() {
  log_info(
      "[runtime_binding_space_t] initializing static-global runtime-binding lock "
      "(single locked runtime-binding hash/path/binding, selected lazily via "
      "runtime_binding_space_t::init)\n");
}

void runtime_binding_space_t::lifecycle_finit() {
  bool lock_initialized = false;
  std::string locked_hash;
  std::string locked_path;
  std::string locked_binding_id;
  std::string last_binding_id;
  std::size_t registered_count = 0;
  std::string last_runtime_binding_hash;
  std::string last_runtime_binding_path;
  {
    LOCK_GUARD(runtime_binding_config_mutex);
    lock_initialized = g_runtime_binding_runtime_lock_initialized;
    locked_hash = g_locked_runtime_binding_hash;
    locked_path = g_locked_campaign_path_canonical;
    locked_binding_id = g_locked_binding_id;
    last_binding_id = g_last_requested_binding_id;
    registered_count = g_registered_runtime_binding_count;
    last_runtime_binding_hash = g_last_registered_runtime_binding_hash;
    last_runtime_binding_path = g_last_registered_runtime_binding_path_canonical;
  }

  log_info(
      "[runtime_binding_space_t] finalizing static-global runtime-binding lock "
      "(lock_initialized=%s, locked_hash=%s, locked_path=%s, "
      "locked_binding_id=%s, last_binding_id=%s, registered_runtime_bindings=%zu, "
      "last_runtime_binding_hash=%s, last_runtime_binding_path=%s)\n",
      lock_initialized ? "true" : "false",
      non_empty_or(locked_hash, "<none>"),
      non_empty_or(locked_path, "<none>"),
      non_empty_or(locked_binding_id, "<none>"),
      non_empty_or(last_binding_id, "<none>"),
      registered_count,
      non_empty_or(last_runtime_binding_hash, "<none>"),
      non_empty_or(last_runtime_binding_path, "<none>"));
}

void runtime_binding_space_t::init() {
  init(config_space_t::effective_campaign_dsl_path());
}

void runtime_binding_space_t::init(const std::string& runtime_binding_file_path) {
  const std::string configured_path_canonical =
      canonicalize_path_best_effort(runtime_binding_file_path);
  if (!has_non_ws_ascii(configured_path_canonical)) {
    log_fatal("[iitepi] invalid configured campaign/runtime-binding path: %s\n",
              runtime_binding_file_path.c_str());
  }

  std::string binding_id{};
  try {
    const std::string text = read_text_file_or_throw(configured_path_canonical);
    if (looks_like_campaign_dsl(text)) {
      binding_id = select_campaign_binding_id_or_throw(
          cuwacunu::camahjucunu::dsl::decode_iitepi_campaign_from_dsl(
              read_text_file_or_throw(default_campaign_grammar_path()), text),
          "");
    } else {
      const runtime_binding_hash_t runtime_binding_hash = register_runtime_binding_file(configured_path_canonical);
      const auto runtime_binding_itself = runtime_binding_space_t::runtime_binding_itself(runtime_binding_hash);
      binding_id = active_bind_id_or_fail(runtime_binding_itself);
    }
  } catch (const std::exception& e) {
    log_fatal("[iitepi] cannot initialize runtime from %s: %s\n",
              configured_path_canonical.c_str(), e.what());
  }
  init(configured_path_canonical, binding_id);
}

void runtime_binding_space_t::init(const std::string& runtime_binding_file_path,
                         const std::string& binding_id) {
  const std::string configured_path_canonical =
      canonicalize_path_best_effort(runtime_binding_file_path);
  if (!has_non_ws_ascii(configured_path_canonical)) {
    log_fatal("[iitepi] invalid configured campaign/runtime-binding path: %s\n",
              runtime_binding_file_path.c_str());
  }
  std::string runtime_binding_path = configured_path_canonical;
  std::string configured_binding_id = trim_ascii_ws_copy(binding_id);
  const std::string configured_source_sha256 =
      sha256_hex_from_file(configured_path_canonical);
  try {
    const std::string text = read_text_file_or_throw(configured_path_canonical);
    if (looks_like_campaign_dsl(text)) {
      runtime_binding_path = materialize_internal_runtime_binding_snapshot_from_campaign(
          configured_path_canonical, configured_binding_id,
          &configured_binding_id);
    }
  } catch (const std::exception& e) {
    log_fatal("[iitepi] cannot initialize runtime from %s: %s\n",
              configured_path_canonical.c_str(), e.what());
  }
  if (!has_non_ws_ascii(configured_binding_id)) {
    log_fatal("[iitepi] invalid configured runtime binding id\n");
  }

  const std::string runtime_binding_canonical =
      canonicalize_path_best_effort(runtime_binding_path);
  const runtime_binding_hash_t runtime_binding_hash = register_runtime_binding_file(runtime_binding_canonical);

  {
    LOCK_GUARD(runtime_binding_config_mutex);
    g_last_requested_binding_id = configured_binding_id;
    if (!g_runtime_binding_runtime_lock_initialized) {
      g_locked_runtime_binding_hash = runtime_binding_hash;
      g_locked_campaign_path_canonical = configured_path_canonical;
      g_locked_binding_id = configured_binding_id;
      g_locked_runtime_source_sha256 = configured_source_sha256;
      g_runtime_binding_runtime_lock_initialized = true;
    } else {
      if (configured_path_canonical != g_locked_campaign_path_canonical) {
        log_fatal(
            "[iitepi] immutable runtime lock violation: configured campaign changed "
            "mid-run (configured=%s, locked=%s)\n",
            configured_path_canonical.c_str(),
            g_locked_campaign_path_canonical.c_str());
      }
      if (runtime_binding_hash != g_locked_runtime_binding_hash) {
        log_fatal(
            "[iitepi] immutable runtime lock violation: campaign hash changed "
            "mid-run (configured=%s, locked=%s)\n",
            runtime_binding_hash.c_str(),
            g_locked_runtime_binding_hash.c_str());
      }
      if (configured_source_sha256 != g_locked_runtime_source_sha256) {
        log_fatal(
            "[iitepi] immutable runtime lock violation: runtime source changed "
            "mid-run (configured=%s, locked=%s)\n",
            configured_source_sha256.c_str(),
            g_locked_runtime_source_sha256.c_str());
      }
      if (configured_binding_id != g_locked_binding_id) {
        log_fatal(
            "[iitepi] immutable runtime-binding lock violation: runtime binding changed "
            "mid-run (configured=%s, locked=%s)\n",
            configured_binding_id.c_str(),
            g_locked_binding_id.c_str());
      }
    }
  }

  const auto runtime_binding_itself = runtime_binding_space_t::runtime_binding_itself(runtime_binding_hash);
  resolve_and_assert_runtime_binding_dependencies(
      runtime_binding_itself, std::optional<std::string>{configured_binding_id},
      runtime_binding_hash);
  assert_locked_runtime_intact_or_fail_fast();
}

bool runtime_binding_space_t::is_initialized() noexcept {
  LOCK_GUARD(runtime_binding_config_mutex);
  return g_runtime_binding_runtime_lock_initialized &&
         has_non_ws_ascii(g_locked_runtime_binding_hash) &&
         has_non_ws_ascii(g_locked_campaign_path_canonical) &&
         has_non_ws_ascii(g_locked_binding_id);
}

runtime_binding_hash_t runtime_binding_space_t::locked_runtime_binding_hash() {
  LOCK_GUARD(runtime_binding_config_mutex);
  if (!g_runtime_binding_runtime_lock_initialized || !has_non_ws_ascii(g_locked_runtime_binding_hash)) {
    log_fatal("[iitepi] locked runtime-binding hash requested before runtime_binding_space_t::init\n");
  }
  return g_locked_runtime_binding_hash;
}

std::string runtime_binding_space_t::locked_campaign_path_canonical() {
  LOCK_GUARD(runtime_binding_config_mutex);
  if (!g_runtime_binding_runtime_lock_initialized ||
      !has_non_ws_ascii(g_locked_campaign_path_canonical)) {
    log_fatal("[iitepi] locked campaign path requested before runtime_binding_space_t::init\n");
  }
  return g_locked_campaign_path_canonical;
}

std::string runtime_binding_space_t::locked_binding_id() {
  LOCK_GUARD(runtime_binding_config_mutex);
  if (!g_runtime_binding_runtime_lock_initialized ||
      !has_non_ws_ascii(g_locked_runtime_binding_hash) ||
      !has_non_ws_ascii(g_locked_binding_id)) {
    log_fatal("[iitepi] locked binding requested before runtime_binding_space_t::init\n");
  }
  return g_locked_binding_id;
}

void runtime_binding_space_t::assert_locked_runtime_intact_or_fail_fast() {
  runtime_binding_hash_t locked_hash;
  std::string locked_path;
  std::string locked_binding_id;
  std::string locked_source_sha256;
  {
    LOCK_GUARD(runtime_binding_config_mutex);
    if (!g_runtime_binding_runtime_lock_initialized ||
        !has_non_ws_ascii(g_locked_runtime_binding_hash) ||
        !has_non_ws_ascii(g_locked_binding_id)) {
      log_fatal("[iitepi] locked runtime integrity requested before runtime-binding init\n");
    }
    locked_hash = g_locked_runtime_binding_hash;
    locked_path = g_locked_campaign_path_canonical;
    locked_binding_id = g_locked_binding_id;
    locked_source_sha256 = g_locked_runtime_source_sha256;
  }
  if (has_non_ws_ascii(locked_path) && has_non_ws_ascii(locked_source_sha256)) {
    const std::string current_source_sha256 = sha256_hex_from_file(locked_path);
    if (current_source_sha256 != locked_source_sha256) {
      log_fatal(
          "[iitepi] immutable runtime lock violation: locked source changed "
          "mid-run (current=%s, locked=%s, path=%s)\n",
          current_source_sha256.c_str(),
          locked_source_sha256.c_str(),
          locked_path.c_str());
    }
  }
  runtime_binding_space_t::assert_intact_or_fail_fast(locked_hash);
  const auto runtime_binding_itself = runtime_binding_space_t::runtime_binding_itself(locked_hash);
  resolve_and_assert_runtime_binding_dependencies(
      runtime_binding_itself, std::optional<std::string>{locked_binding_id}, locked_hash);
  runtime_binding_space_t::assert_registry_intact_or_fail_fast();
  contract_space_t::assert_registry_intact_or_fail_fast();
  wave_space_t::assert_registry_intact_or_fail_fast();
}

runtime_binding_hash_t runtime_binding_space_t::register_runtime_binding_file(const std::string& path) {
  const std::string canonical_path = canonicalize_path_best_effort(path);
  if (!has_non_ws_ascii(canonical_path)) {
    log_fatal("[dconfig] register_runtime_binding_file received empty/invalid path: %s\n",
              path.c_str());
  }

  std::optional<runtime_binding_hash_t> existing_hash;
  {
    LOCK_GUARD(runtime_binding_config_mutex);
    auto& path_to_hash = hash_by_runtime_binding_path();
    auto& bindings_by_hash = runtime_bindings_by_hash();
    const auto existing = path_to_hash.find(canonical_path);
    if (existing != path_to_hash.end()) {
      const auto runtime_binding_it = bindings_by_hash.find(existing->second);
      if (runtime_binding_it == bindings_by_hash.end() || !runtime_binding_it->second) {
        log_fatal(
            "[dconfig] runtime-binding registry corruption: path is mapped but runtime binding record is missing (%s)\n",
            canonical_path.c_str());
      }
      existing_hash = existing->second;
      g_registered_runtime_binding_count = bindings_by_hash.size();
      g_last_registered_runtime_binding_hash = existing->second;
      g_last_registered_runtime_binding_path_canonical = canonical_path;
    }
  }
  if (existing_hash.has_value()) {
    assert_intact_or_fail_fast(*existing_hash);
    return *existing_hash;
  }

  const auto built_runtime_binding = build_runtime_binding_record_from_path(canonical_path);
  if (!built_runtime_binding) {
    log_fatal("[dconfig] failed to build runtime binding record from: %s\n",
              canonical_path.c_str());
  }
  const runtime_binding_hash_t built_hash =
      built_runtime_binding->dependency_manifest.dependency_manifest_aggregate_sha256_hex;
  if (!has_non_ws_ascii(built_hash)) {
    log_fatal("[dconfig] built runtime binding record has empty manifest hash for: %s\n",
              canonical_path.c_str());
  }

  {
    LOCK_GUARD(runtime_binding_config_mutex);
    auto& path_to_hash = hash_by_runtime_binding_path();
    auto& bindings_by_hash = runtime_bindings_by_hash();
    const auto existing = path_to_hash.find(canonical_path);
    if (existing != path_to_hash.end()) {
      if (existing->second != built_hash) {
        log_fatal(
            "[dconfig] immutable runtime-binding lock violation: attempted to rebind runtime-binding path %s from hash %s to %s\n",
            canonical_path.c_str(),
            existing->second.c_str(),
            built_hash.c_str());
      }
      const auto runtime_binding_it = bindings_by_hash.find(existing->second);
      if (runtime_binding_it == bindings_by_hash.end() || !runtime_binding_it->second) {
        log_fatal(
            "[dconfig] runtime-binding registry corruption: path is mapped but runtime binding record is missing (%s)\n",
            canonical_path.c_str());
      }
      existing_hash = existing->second;
    } else {
      auto by_hash = bindings_by_hash.find(built_hash);
      if (by_hash == bindings_by_hash.end()) {
        bindings_by_hash.emplace(built_hash, built_runtime_binding);
      } else if (!by_hash->second) {
        by_hash->second = built_runtime_binding;
      }
      path_to_hash[canonical_path] = built_hash;
    }
    g_registered_runtime_binding_count = bindings_by_hash.size();
    g_last_registered_runtime_binding_hash = existing_hash.has_value() ? *existing_hash
                                                              : built_hash;
    g_last_registered_runtime_binding_path_canonical = canonical_path;
  }

  if (existing_hash.has_value()) {
    assert_intact_or_fail_fast(*existing_hash);
    return *existing_hash;
  }
  return built_hash;
}

std::shared_ptr<const runtime_binding_record_t> runtime_binding_space_t::runtime_binding_itself(
    const runtime_binding_hash_t& hash) {
  return runtime_binding_ptr_or_fail(hash);
}

std::string runtime_binding_space_t::contract_hash_for_binding(
    const runtime_binding_hash_t& hash,
    const std::string& binding_id) {
  const auto runtime_binding = runtime_binding_ptr_or_fail(hash);
  const auto& instruction = runtime_binding->runtime_binding.decoded();
  const auto* bind = find_bind_by_id(instruction, binding_id);
  if (!bind) {
    log_fatal("[dconfig] runtime binding id not found: %s\n", binding_id.c_str());
  }
  const auto* contract_decl = find_contract_by_id(instruction, bind->contract_ref);
  if (!contract_decl) {
    log_fatal("[dconfig] runtime binding references unknown CONTRACT id: %s\n",
              bind->contract_ref.c_str());
  }
  const std::string contract_path =
      resolve_path_from_folder(
          parent_folder_from_path(runtime_binding->config_file_path),
          contract_decl->file);
  return contract_space_t::register_contract_file(contract_path);
}

std::string runtime_binding_space_t::wave_hash_for_binding(const runtime_binding_hash_t& hash,
                                                 const std::string& binding_id) {
  const auto runtime_binding = runtime_binding_ptr_or_fail(hash);
  const auto& instruction = runtime_binding->runtime_binding.decoded();
  const auto* bind = find_bind_by_id(instruction, binding_id);
  if (!bind) {
    log_fatal("[dconfig] runtime binding id not found: %s\n", binding_id.c_str());
  }

  const std::string bind_wave_id = trim_ascii_ws_copy(bind->wave_ref);
  const auto* wave_decl = find_wave_import_by_id(instruction, bind_wave_id);
  if (!wave_decl) {
    log_fatal(
        "[dconfig] runtime binding references unknown imported WAVE id: %s\n",
        bind_wave_id.c_str());
  }
  const std::string wave_path =
      resolve_path_from_folder(
          parent_folder_from_path(runtime_binding->config_file_path),
          wave_decl->file);
  const auto wave_hash = wave_space_t::register_wave_file(wave_path);
  const auto wave_itself = wave_space_t::wave_itself(wave_hash);
  const auto& wave_set = wave_itself->wave.decoded();
  const auto matches_wave = std::any_of(
      wave_set.waves.begin(), wave_set.waves.end(), [&](const auto& wave) {
        return trim_ascii_ws_copy(wave.name) == bind_wave_id;
      });
  if (!matches_wave) {
    log_fatal(
        "[dconfig] imported WAVE id not found in declared wave file: %s (%s)\n",
        bind_wave_id.c_str(), wave_path.c_str());
  }
  return wave_hash;
}

void runtime_binding_space_t::assert_intact_or_fail_fast(const runtime_binding_hash_t& hash) {
  const auto runtime_binding = runtime_binding_ptr_or_fail(hash);

  std::vector<runtime_binding_file_fingerprint_t> refreshed;
  refreshed.reserve(runtime_binding->dependency_manifest.files.size());

  for (const auto& expected : runtime_binding->dependency_manifest.files) {
    const std::filesystem::path p(expected.canonical_path);
    if (!std::filesystem::exists(p) || !std::filesystem::is_regular_file(p)) {
      log_fatal(
          "[dconfig] immutable runtime-binding lock violation: dependency missing or invalid: %s\n",
          expected.canonical_path.c_str());
    }

    runtime_binding_file_fingerprint_t current = expected;
    current.file_size_bytes = std::filesystem::file_size(p);
    current.mtime_ticks = file_mtime_ticks(p);

    if (current.file_size_bytes != expected.file_size_bytes ||
        current.mtime_ticks != expected.mtime_ticks) {
      current.sha256_hex = sha256_hex_from_file(expected.canonical_path);
      if (current.sha256_hex != expected.sha256_hex) {
        log_fatal(
            "[dconfig] immutable runtime-binding lock violation: runtime-binding dependency changed "
            "mid-run: %s\n",
            expected.canonical_path.c_str());
      }
    }
    refreshed.push_back(std::move(current));
  }

  const std::string digest = compute_manifest_digest_hex(refreshed);
  if (digest !=
      runtime_binding->dependency_manifest
          .dependency_manifest_aggregate_sha256_hex) {
    log_fatal(
        "[dconfig] immutable runtime-binding lock violation: dependency manifest digest "
        "mismatch mid-run\n");
  }
}

void runtime_binding_space_t::assert_registry_intact_or_fail_fast() {
  std::vector<runtime_binding_ptr_t> runtime_bindings;
  {
    LOCK_GUARD(runtime_binding_config_mutex);
    runtime_bindings = registry_runtime_bindings_copy_locked();
  }
  for (const auto& ptr : runtime_bindings) {
    if (!ptr) continue;
    assert_intact_or_fail_fast(
        ptr->dependency_manifest.dependency_manifest_aggregate_sha256_hex);
  }
}

bool runtime_binding_space_t::has_runtime_binding(const runtime_binding_hash_t& hash) noexcept {
  LOCK_GUARD(runtime_binding_config_mutex);
  const auto& runtime_bindings = runtime_bindings_by_hash();
  const auto it = runtime_bindings.find(hash);
  return it != runtime_bindings.end() && static_cast<bool>(it->second);
}

std::vector<runtime_binding_hash_t> runtime_binding_space_t::registered_hashes() {
  LOCK_GUARD(runtime_binding_config_mutex);
  const auto& runtime_bindings = runtime_bindings_by_hash();
  std::vector<runtime_binding_hash_t> out{};
  out.reserve(runtime_bindings.size());
  for (const auto& [hash, ptr] : runtime_bindings) {
    if (ptr) out.push_back(hash);
  }
  std::sort(out.begin(), out.end());
  return out;
}

std::string runtime_binding_record_t::raw(const std::string& section,
                                const std::string& key) const {
  const auto s = config.find(section);
  if (s == config.end()) throw bad_config_access("Missing section [" + section + "]");
  const auto k = s->second.find(key);
  if (k == s->second.end()) {
    throw bad_config_access("Missing key <" + key + "> in [" + section + "]");
  }
  return k->second;
}

template <class T>
T runtime_binding_record_t::from_string(const std::string& s) {
  return parse_scalar_from_string<T>(s);
}

template <class T>
T runtime_binding_record_t::get(const std::string& section,
                      const std::string& key,
                      std::optional<T> fallback) const {
  try {
    return from_string<T>(raw(section, key));
  } catch (const bad_config_access&) {
    if (fallback) return *fallback;
    throw;
  }
}

template <class T>
std::vector<T> runtime_binding_record_t::get_arr(
    const std::string& section,
    const std::string& key,
    std::optional<std::vector<T>> fallback) const {
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

const cuwacunu::camahjucunu::runtime_binding_instruction_t&
runtime_binding_record::runtime_binding_blob_t::decoded() const {
  std::call_once(decode_once_, [&]() {
    try {
      auto inst = cuwacunu::camahjucunu::dsl::decode_runtime_binding_from_dsl(
          grammar,
          dsl);
      decoded_cache_ =
          std::make_shared<cuwacunu::camahjucunu::runtime_binding_instruction_t>(
              std::move(inst));
    } catch (const std::exception& e) {
      throw std::runtime_error(
          std::string("failed to decode runtime-binding DSL: ") + e.what());
    }
  });
  if (!decoded_cache_) {
    throw std::runtime_error("failed to decode runtime-binding DSL: empty decoded cache");
  }
  return *decoded_cache_;
}

template int64_t runtime_binding_record_t::get<int64_t>(
    const std::string&, const std::string&, std::optional<int64_t>) const;
template int runtime_binding_record_t::get<int>(
    const std::string&, const std::string&, std::optional<int>) const;
template double runtime_binding_record_t::get<double>(
    const std::string&, const std::string&, std::optional<double>) const;
template float runtime_binding_record_t::get<float>(
    const std::string&, const std::string&, std::optional<float>) const;
template bool runtime_binding_record_t::get<bool>(
    const std::string&, const std::string&, std::optional<bool>) const;
template std::string runtime_binding_record_t::get<std::string>(
    const std::string&, const std::string&, std::optional<std::string>) const;

template std::vector<int64_t> runtime_binding_record_t::get_arr<int64_t>(
    const std::string&, const std::string&, std::optional<std::vector<int64_t>>) const;
template std::vector<int> runtime_binding_record_t::get_arr<int>(
    const std::string&, const std::string&, std::optional<std::vector<int>>) const;
template std::vector<double> runtime_binding_record_t::get_arr<double>(
    const std::string&, const std::string&, std::optional<std::vector<double>>) const;
template std::vector<float> runtime_binding_record_t::get_arr<float>(
    const std::string&, const std::string&, std::optional<std::vector<float>>) const;
template std::vector<bool> runtime_binding_record_t::get_arr<bool>(
    const std::string&, const std::string&, std::optional<std::vector<bool>>) const;
template std::vector<std::string> runtime_binding_record_t::get_arr<std::string>(
    const std::string&, const std::string&, std::optional<std::vector<std::string>>) const;

}  // namespace iitepi
}  // namespace cuwacunu
