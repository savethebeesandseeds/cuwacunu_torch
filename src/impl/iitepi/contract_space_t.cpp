#include "iitepi/contract_space_t.h"

#include "camahjucunu/dsl/jkimyei_specs/jkimyei_specs.h"
#include "camahjucunu/dsl/observation_pipeline/observation_spec.h"
#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit.h"
#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit_runtime.h"

#include <array>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <initializer_list>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace cuwacunu {
namespace iitepi {

std::mutex contract_config_mutex;

namespace {

using snapshot_ptr_t = std::shared_ptr<const contract_record_t>;

[[nodiscard]] static std::unordered_map<contract_hash_t, snapshot_ptr_t>&
snapshots_by_hash() {
  static std::unordered_map<contract_hash_t, snapshot_ptr_t> registry;
  return registry;
}

[[nodiscard]] static std::unordered_map<std::string, contract_hash_t>&
hash_by_contract_path() {
  static std::unordered_map<std::string, contract_hash_t> registry;
  return registry;
}

[[nodiscard]] static std::string strip_comment(std::string_view line,
                                               bool* in_block_comment);
[[nodiscard]] static bool has_non_ws_ascii(const std::string& s);
[[nodiscard]] static std::string trim_ascii_ws_copy(std::string s);
[[nodiscard]] static std::string strip_comments_from_text_preserve_lines(
    std::string_view text);
[[nodiscard]] static std::string decode_escaped_text(std::string s);
[[nodiscard]] static parsed_config_t parse_config_file(const std::string& path);
[[nodiscard]] static std::vector<std::string> split_string_items(
    const std::string& s);
[[nodiscard]] static std::string resolve_path_from_folder(
    const std::string& folder, std::string path);
[[nodiscard]] static std::string canonicalize_path_best_effort(
    const std::string& path);
[[nodiscard]] static std::string sha256_hex_from_bytes(const void* data,
                                                       std::size_t size);
[[nodiscard]] static std::string sha256_hex_from_file(const std::string& path);
[[nodiscard]] static std::int64_t file_mtime_ticks(
    const std::filesystem::path& path);
[[nodiscard]] static contract_file_fingerprint_t fingerprint_file(
    const std::string& path);
[[nodiscard]] static std::string compute_manifest_digest_hex(
    const std::vector<contract_file_fingerprint_t>& files);
[[nodiscard]] static std::string contract_required_resolved_path(
    const parsed_config_t& cfg, const std::string& cfg_folder,
    const char* section, const char* key);
[[nodiscard]] static std::string snapshot_contract_dsl_value_or_empty(
    const parsed_config_t& cfg, const char* key);
[[nodiscard]] static parsed_config_section_t parse_instruction_file(
    const std::string& path);
[[nodiscard]] static bool module_section_exists(
    const contract_record_t& snapshot, std::string_view section) noexcept;
[[nodiscard]] static std::shared_ptr<contract_record_t>
build_contract_record_from_contract_path(const std::string& contract_file_path);
[[nodiscard]] static bool section_supports_instruction_file(
    std::string_view section) noexcept;
[[nodiscard]] static std::optional<std::string>
module_config_path_for_section(const contract_record_t& snapshot,
                               const std::string& section);
[[nodiscard]] static std::string parse_instruction_lhs_key(std::string lhs);
[[nodiscard]] static std::optional<std::string>
contract_instruction_section_value_or_empty(const contract_record_t& snapshot,
                                            const std::string& section,
                                            const std::string& key);
[[nodiscard]] static snapshot_ptr_t
snapshot_ptr_or_fail(const contract_hash_t& hash);
[[nodiscard]] static std::vector<snapshot_ptr_t>
registry_snapshots_copy_locked();
static bool validate_contract_config_or_terminate(const parsed_config_t& cfg,
                                                  const std::string& cfg_folder);

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

[[nodiscard]] static std::string strip_comments_from_text_preserve_lines(
    std::string_view text) {
  std::istringstream input{std::string(text)};
  std::string line;
  std::string out;
  bool in_block_comment = false;
  while (std::getline(input, line)) {
    out.append(strip_comment(line, &in_block_comment));
    out.push_back('\n');
  }
  return out;
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

[[nodiscard]] static contract_file_fingerprint_t fingerprint_file(
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

  contract_file_fingerprint_t fp{};
  fp.canonical_path = canonical;
  fp.file_size_bytes = std::filesystem::file_size(p);
  fp.mtime_ticks = file_mtime_ticks(p);
  fp.sha256_hex = sha256_hex_from_file(canonical);
  return fp;
}

[[nodiscard]] static std::string compute_manifest_digest_hex(
    const std::vector<contract_file_fingerprint_t>& files) {
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

[[nodiscard]] static std::string contract_required_resolved_path(
    const parsed_config_t& cfg, const std::string& cfg_folder, const char* section,
    const char* key) {
  const auto sec_it = cfg.find(section);
  if (sec_it == cfg.end()) {
    log_fatal("[dconfig] missing contract section [%s] while building snapshot\n",
              section);
  }
  const auto key_it = sec_it->second.find(key);
  if (key_it == sec_it->second.end()) {
    log_fatal(
        "[dconfig] missing contract key <%s> in section [%s] while building snapshot\n",
        key, section);
  }
  const std::string raw = trim_ascii_ws_copy(key_it->second);
  if (!has_non_ws_ascii(raw)) {
    log_fatal(
        "[dconfig] empty contract key <%s> in section [%s] while building snapshot\n",
        key, section);
  }

  const std::string resolved = resolve_path_from_folder(cfg_folder, raw);
  if (!has_non_ws_ascii(resolved)) {
    log_fatal(
        "[dconfig] unable to resolve contract path for <%s> in section [%s]\n", key,
        section);
  }
  if (!std::filesystem::exists(resolved)) {
    log_fatal("[dconfig] contract dependency path does not exist: %s\n",
              resolved.c_str());
  }
  return resolved;
}

[[nodiscard]] static std::optional<std::string> contract_optional_resolved_path(
    const parsed_config_t& cfg, const std::string& cfg_folder, const char* section,
    const char* key) {
  const auto sec_it = cfg.find(section);
  if (sec_it == cfg.end()) return std::nullopt;
  const auto key_it = sec_it->second.find(key);
  if (key_it == sec_it->second.end()) return std::nullopt;

  const std::string raw = trim_ascii_ws_copy(key_it->second);
  if (!has_non_ws_ascii(raw)) return std::nullopt;

  const std::string resolved = resolve_path_from_folder(cfg_folder, raw);
  if (!has_non_ws_ascii(resolved)) {
    log_fatal(
        "[dconfig] unable to resolve optional contract path for <%s> in section [%s]\n",
        key, section);
  }
  if (!std::filesystem::exists(resolved)) {
    log_fatal("[dconfig] optional contract dependency path does not exist: %s\n",
              resolved.c_str());
  }
  return resolved;
}

[[nodiscard]] static std::string snapshot_contract_dsl_value_or_empty(
    const parsed_config_t& cfg, const char* key) {
  const auto sec_it = cfg.find("BOARD_CONTRACT_DSL");
  if (sec_it == cfg.end()) return {};
  const auto key_it = sec_it->second.find(key);
  if (key_it == sec_it->second.end()) return {};
  const std::string raw = key_it->second;
  if (!has_non_ws_ascii(raw)) return {};
  return decode_escaped_text(raw);
}

[[nodiscard]] static bool module_section_exists(const contract_record_t& snapshot,
                                                std::string_view section) noexcept {
  return snapshot.module_sections.find(std::string(section)) !=
         snapshot.module_sections.end();
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

[[nodiscard]] static bool section_supports_instruction_file(
    std::string_view section) noexcept {
  return section == "VICReg" || section == "VALUE_ESTIMATION";
}

[[nodiscard]] static std::optional<std::string>
module_config_path_for_section(const contract_record_t& snapshot,
                               const std::string& section) {
  if (!section_supports_instruction_file(section)) return std::nullopt;
  const auto path_it = snapshot.module_section_paths.find(section);
  if (path_it == snapshot.module_section_paths.end()) return std::nullopt;
  if (!has_non_ws_ascii(path_it->second)) return std::nullopt;
  return path_it->second;
}

[[nodiscard]] static std::string parse_instruction_lhs_key(std::string lhs) {
  lhs = piaabo::trim_string(std::move(lhs));
  const std::size_t colon = lhs.find(':');
  if (colon == std::string::npos) return lhs;
  return piaabo::trim_string(lhs.substr(0, colon));
}

[[nodiscard]] static parsed_config_section_t parse_instruction_file(
    const std::string& path) {
  parsed_config_section_t parsed;
  std::ifstream file = cuwacunu::piaabo::dfiles::readFileToStream(path);
  if (!file) {
    log_warn("[dconfig] cannot open module config file: %s\n", path.c_str());
    return parsed;
  }

  std::string raw_line;
  bool in_block_comment = false;
  while (std::getline(file, raw_line)) {
    const std::string line =
        piaabo::trim_string(strip_comment(raw_line, &in_block_comment));
    if (line.empty()) continue;

    const std::size_t pos = line.find('=');
    if (pos == std::string::npos) {
      log_warn("[dconfig] skipping malformed module config line in %s: %s\n",
               path.c_str(), raw_line.c_str());
      continue;
    }

    const std::string lhs = piaabo::trim_string(line.substr(0, pos));
    const std::string rhs = piaabo::trim_string(line.substr(pos + 1));
    const std::string key = parse_instruction_lhs_key(lhs);
    if (key.empty()) continue;
    parsed[key] = rhs;
  }
  return parsed;
}

[[nodiscard]] static std::optional<std::string>
contract_instruction_section_value_or_empty(const contract_record_t& snapshot,
                                            const std::string& section,
                                            const std::string& key) {
  const auto sec_it = snapshot.module_sections.find(section);
  if (sec_it == snapshot.module_sections.end()) return std::nullopt;
  const auto value_it = sec_it->second.find(key);
  if (value_it == sec_it->second.end()) return std::nullopt;
  return value_it->second;
}

static bool validate_contract_config_or_terminate(const parsed_config_t& cfg,
                                                  const std::string& cfg_folder) {
  bool ok = true;

  const auto require_value =
      [&](const char* section, const char* key, std::string* out_value) -> bool {
    const auto sec_it = cfg.find(section);
    if (sec_it == cfg.end()) {
      log_warn("Missing contract section [%s]\n", section);
      ok = false;
      return false;
    }

    const auto key_it = sec_it->second.find(key);
    if (key_it == sec_it->second.end()) {
      log_warn("Missing field <%s> in contract section [%s]\n", key, section);
      ok = false;
      return false;
    }

    const std::string value = trim_ascii_ws_copy(key_it->second);
    if (!has_non_ws_ascii(value)) {
      log_warn("Empty field <%s> in contract section [%s]\n", key, section);
      ok = false;
      return false;
    }
    if (out_value) *out_value = value;
    return true;
  };

  const auto require_existing_path =
      [&](const char* section, const char* key, std::string* out_path) -> bool {
    std::string raw_path;
    if (!require_value(section, key, &raw_path)) return false;

    const std::string resolved = resolve_path_from_folder(cfg_folder, raw_path);
    if (!has_non_ws_ascii(resolved)) {
      log_warn("Unable to resolve path for <%s> in contract section [%s]\n",
               key, section);
      ok = false;
      return false;
    }
    if (!std::filesystem::exists(resolved)) {
      log_warn(
          "Configured path does not exist for <%s> in contract section [%s]: %s\n",
          key, section, resolved.c_str());
      ok = false;
      return false;
    }
    if (out_path) *out_path = resolved;
    return true;
  };

  const auto resolve_existing_path_if_present =
      [&](const char* section, const char* key, std::string* out_path) -> bool {
    const auto sec_it = cfg.find(section);
    if (sec_it == cfg.end()) return false;
    const auto key_it = sec_it->second.find(key);
    if (key_it == sec_it->second.end()) return false;

    const std::string raw_path = trim_ascii_ws_copy(key_it->second);
    if (!has_non_ws_ascii(raw_path)) return false;

    const std::string resolved = resolve_path_from_folder(cfg_folder, raw_path);
    if (!has_non_ws_ascii(resolved)) {
      log_warn(
          "Unable to resolve optional path for <%s> in contract section [%s]\n",
          key, section);
      ok = false;
      return false;
    }
    if (!std::filesystem::exists(resolved)) {
      log_warn(
          "Configured optional path does not exist for <%s> in contract section [%s]: %s\n",
          key, section, resolved.c_str());
      ok = false;
      return false;
    }
    if (out_path) *out_path = resolved;
    return true;
  };

  std::string vicreg_config_path;
  std::string value_estimation_config_path;
  (void)require_existing_path("SPECS", "vicreg_config_filename",
                              &vicreg_config_path);
  (void)resolve_existing_path_if_present("SPECS",
                                         "value_estimation_config_filename",
                                         &value_estimation_config_path);
  constexpr const char* kRequiredDslPathKeys[] = {
      "observation_sources_grammar_filename",
      "observation_sources_dsl_filename",
      "observation_channels_grammar_filename",
      "observation_channels_dsl_filename",
      "jkimyei_specs_grammar_filename",
      "jkimyei_specs_dsl_filename",
      "tsiemene_circuit_grammar_filename",
      "tsiemene_circuit_dsl_filename",
      "canonical_path_grammar_filename",
  };
  for (const char* key : kRequiredDslPathKeys) {
    (void)require_existing_path("DSL", key, nullptr);
  }

  const bool has_train_circuit = resolve_existing_path_if_present(
      "DSL", "tsiemene_circuit_train_dsl_filename", nullptr);
  const bool has_run_circuit = resolve_existing_path_if_present(
      "DSL", "tsiemene_circuit_run_dsl_filename", nullptr);
  if (has_train_circuit || has_run_circuit) {
    log_warn(
        "[dconfig] split circuit keys "
        "<tsiemene_circuit_train_dsl_filename>/<tsiemene_circuit_run_dsl_filename> "
        "are removed; use [DSL].tsiemene_circuit_dsl_filename\n");
    ok = false;
  }

  {
    const auto inline_it = cfg.find("BOARD_CONTRACT_DSL");
    if (inline_it != cfg.end()) {
      const auto reject_removed_inline = [&](const char* key) {
        const auto key_it = inline_it->second.find(key);
        if (key_it == inline_it->second.end()) return;
        if (!has_non_ws_ascii(trim_ascii_ws_copy(key_it->second))) return;
        log_warn(
            "[dconfig] BOARD_CONTRACT_DSL.%s is removed; "
            "use canonical BOARD_CONTRACT_DSL.tsiemene_*_dsl_text keys.\n",
            key);
        ok = false;
      };
      reject_removed_inline("tsiemene_circuit_train_dsl_text");
      reject_removed_inline("tsiemene_circuit_run_dsl_text");
    }
  }

  const auto validate_module_instruction_file =
      [&](const char* module_name, const std::string& module_path,
          std::initializer_list<const char*> string_keys,
          std::initializer_list<const char*> int_keys,
          std::initializer_list<const char*> float_keys,
          std::initializer_list<const char*> bool_keys,
          std::initializer_list<const char*> arr_int_keys,
          std::initializer_list<const char*> arr_float_keys) {
    if (!has_non_ws_ascii(module_path)) return;

    const parsed_config_section_t module_values = parse_instruction_file(module_path);
    const auto lower_trim_ascii = [](std::string value) -> std::string {
      value = trim_ascii_ws_copy(std::move(value));
      std::transform(value.begin(), value.end(), value.begin(),
                     [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
      return value;
    };
    const auto strip_type_prefixes = [&](std::string value) -> std::string {
      value = lower_trim_ascii(std::move(value));
      if (value.rfind("torch::", 0) == 0) value = value.substr(7);
      if (value.rfind("at::", 0) == 0) value = value.substr(4);
      if (value.rfind("k", 0) == 0 && value.size() > 1 &&
          std::isalpha(static_cast<unsigned char>(value[1]))) {
        value = value.substr(1);
      }
      return value;
    };
    const auto is_valid_dtype_token = [&](std::string value) -> bool {
      value = strip_type_prefixes(std::move(value));
      return value == "bool" || value == "int8" || value == "int16" ||
             value == "int32" || value == "int64" || value == "float16" ||
             value == "half" || value == "f16" || value == "float32" ||
             value == "float" || value == "f32" || value == "float64" ||
             value == "double" || value == "f64";
    };
    const auto is_valid_device_token = [&](std::string value) -> bool {
      value = lower_trim_ascii(std::move(value));
      if (value == "cpu" || value == "cuda" || value == "gpu") return true;
      if (value == "torch::kcpu" || value == "kcpu" || value == "torch::kcuda" ||
          value == "kcuda") {
        return true;
      }
      if (value.rfind("cuda:", 0) == 0 || value.rfind("gpu:", 0) == 0) {
        const std::string suffix = value.substr(value.find(':') + 1);
        return !suffix.empty() &&
               std::all_of(suffix.begin(), suffix.end(), [](unsigned char c) {
                 return std::isdigit(c);
               });
      }
      return false;
    };

    const auto require_module_value =
        [&](const char* key, std::string* out_value) -> bool {
      const auto it = module_values.find(key);
      if (it == module_values.end()) {
        log_warn("Missing key <%s> in module config [%s] file: %s\n", key,
                 module_name, module_path.c_str());
        ok = false;
        return false;
      }

      const std::string value = trim_ascii_ws_copy(it->second);
      if (!has_non_ws_ascii(value)) {
        log_warn("Empty key <%s> in module config [%s] file: %s\n", key,
                 module_name, module_path.c_str());
        ok = false;
        return false;
      }

      if (out_value) *out_value = value;
      return true;
    };

    for (const char* key : string_keys) {
      (void)require_module_value(key, nullptr);
    }
    {
      std::string dtype_value;
      if (require_module_value("dtype", &dtype_value) &&
          !is_valid_dtype_token(dtype_value)) {
        log_warn("Invalid dtype token in module config [%s] file %s: %s\n",
                 module_name, module_path.c_str(), dtype_value.c_str());
        ok = false;
      }
    }
    {
      std::string device_value;
      if (require_module_value("device", &device_value) &&
          !is_valid_device_token(device_value)) {
        log_warn("Invalid device token in module config [%s] file %s: %s\n",
                 module_name, module_path.c_str(), device_value.c_str());
        ok = false;
      }
    }

    for (const char* key : int_keys) {
      std::string value;
      if (!require_module_value(key, &value)) continue;
      try {
        (void)parse_scalar_from_string<int64_t>(value);
      } catch (const std::exception& e) {
        log_warn("Invalid int value for <%s> in module config [%s] file %s: %s\n",
                 key, module_name, module_path.c_str(), e.what());
        ok = false;
      }
    }

    for (const char* key : float_keys) {
      std::string value;
      if (!require_module_value(key, &value)) continue;
      try {
        (void)parse_scalar_from_string<double>(value);
      } catch (const std::exception& e) {
        log_warn(
            "Invalid float value for <%s> in module config [%s] file %s: %s\n",
            key, module_name, module_path.c_str(), e.what());
        ok = false;
      }
    }

    for (const char* key : bool_keys) {
      std::string value;
      if (!require_module_value(key, &value)) continue;
      try {
        (void)parse_scalar_from_string<bool>(value);
      } catch (const std::exception& e) {
        log_warn("Invalid bool value for <%s> in module config [%s] file %s: %s\n",
                 key, module_name, module_path.c_str(), e.what());
        ok = false;
      }
    }

    for (const char* key : arr_int_keys) {
      std::string value;
      if (!require_module_value(key, &value)) continue;
      const auto items = split_string_items(value);
      if (items.empty()) {
        log_warn("Empty integer array for <%s> in module config [%s] file: %s\n",
                 key, module_name, module_path.c_str());
        ok = false;
        continue;
      }
      for (const auto& item : items) {
        try {
          (void)parse_scalar_from_string<int64_t>(item);
        } catch (const std::exception& e) {
          log_warn(
              "Invalid integer array item for <%s> in module config [%s] file %s: %s\n",
              key, module_name, module_path.c_str(), e.what());
          ok = false;
        }
      }
    }

    for (const char* key : arr_float_keys) {
      std::string value;
      if (!require_module_value(key, &value)) continue;
      const auto items = split_string_items(value);
      if (items.empty()) {
        log_warn("Empty float array for <%s> in module config [%s] file: %s\n",
                 key, module_name, module_path.c_str());
        ok = false;
        continue;
      }
      for (const auto& item : items) {
        try {
          (void)parse_scalar_from_string<double>(item);
        } catch (const std::exception& e) {
          log_warn(
              "Invalid float array item for <%s> in module config [%s] file %s: %s\n",
              key, module_name, module_path.c_str(), e.what());
          ok = false;
        }
      }
    }
  };

  validate_module_instruction_file(
      "VICReg", vicreg_config_path,
      /* string_keys */
      {"model_path", "projector_mlp_spec", "projector_norm",
       "projector_activation", "dtype", "device"},
      /* int_keys */
      {"n_epochs", "n_iters", "swa_start_iter", "encoding_dims",
       "channel_expansion_dim", "fused_feature_dim", "encoder_hidden_dims",
       "encoder_depth", "optimizer_threshold_reset"},
      /* float_keys */
      {},
      /* bool_keys */
      {"verbose_train", "projector_hidden_bias", "projector_last_bias",
       "projector_bn_in_fp32", "enable_buffer_averaging"},
      /* arr_int_keys */
      {},
      /* arr_float_keys */
      {});

  validate_module_instruction_file(
      "VALUE_ESTIMATION", value_estimation_config_path,
      /* string_keys */
      {"model_path", "dtype", "device"},
      /* int_keys */
      {"n_epochs", "n_iters", "telemetry_every", "mixture_comps",
       "features_hidden", "residual_depth", "optimizer_threshold_reset"},
      /* float_keys */
      {"grad_clip"},
      /* bool_keys */
      {"verbose_train"},
      /* arr_int_keys */
      {"target_dims"},
      /* arr_float_keys */
      {"target_weights"});

  if (!ok) {
    log_terminate_gracefully("Invalid board contract configuration, aborting.\n");
  }
  return ok;
}

[[nodiscard]] static std::shared_ptr<contract_record_t>
build_contract_record_from_contract_path(const std::string& contract_file_path) {
  const std::string resolved_contract_path =
      canonicalize_path_best_effort(contract_file_path);
  if (!has_non_ws_ascii(resolved_contract_path)) {
    log_fatal("[dconfig] cannot resolve board contract file path from: %s\n",
              contract_file_path.c_str());
  }
  if (!std::filesystem::exists(resolved_contract_path)) {
    log_fatal("[dconfig] contract file path does not exist: %s\n",
              resolved_contract_path.c_str());
  }

  std::string contract_folder;
  const std::filesystem::path contract_path(resolved_contract_path);
  if (contract_path.has_parent_path()) {
    contract_folder = contract_path.parent_path().string();
  }

  parsed_config_t parsed = parse_config_file(resolved_contract_path);
  (void)validate_contract_config_or_terminate(parsed, contract_folder);

  auto record = std::make_shared<contract_record_t>();
  record->config_folder = contract_folder;
  record->config_file_path = resolved_contract_path;
  record->config_file_path_canonical =
      canonicalize_path_best_effort(resolved_contract_path);
  record->config = parsed;

  const std::string vicreg_path = contract_required_resolved_path(
      record->config, record->config_folder, "SPECS", "vicreg_config_filename");
  const auto value_estimation_path = contract_optional_resolved_path(
      record->config, record->config_folder, "SPECS",
      "value_estimation_config_filename");
  record->module_section_paths["VICReg"] = vicreg_path;
  record->module_sections["VICReg"] = parse_instruction_file(vicreg_path);
  if (value_estimation_path.has_value()) {
    record->module_section_paths["VALUE_ESTIMATION"] = *value_estimation_path;
    record->module_sections["VALUE_ESTIMATION"] =
        parse_instruction_file(*value_estimation_path);
  }

  constexpr const char* kRequiredDslPathKeys[] = {
      "observation_sources_grammar_filename",
      "observation_sources_dsl_filename",
      "observation_channels_grammar_filename",
      "observation_channels_dsl_filename",
      "jkimyei_specs_grammar_filename",
      "jkimyei_specs_dsl_filename",
      "tsiemene_circuit_grammar_filename",
      "tsiemene_circuit_dsl_filename",
      "canonical_path_grammar_filename",
  };

  std::set<std::string> dependency_paths;
  dependency_paths.insert(record->config_file_path_canonical);
  dependency_paths.insert(canonicalize_path_best_effort(vicreg_path));
  if (value_estimation_path.has_value()) {
    dependency_paths.insert(canonicalize_path_best_effort(*value_estimation_path));
  }

  std::unordered_map<std::string, std::string> dsl_asset_text_by_key{};
  for (const char* key : kRequiredDslPathKeys) {
    const std::string path = contract_required_resolved_path(
        record->config, record->config_folder, "DSL", key);
    dsl_asset_text_by_key[key] = piaabo::dfiles::readFileToString(path);
    dependency_paths.insert(canonicalize_path_best_effort(path));
  }

  const auto dsl_asset_text_or_fail = [&](const char* key) -> const std::string& {
    const auto it = dsl_asset_text_by_key.find(key);
    if (it == dsl_asset_text_by_key.end()) {
      log_fatal("[dconfig] missing required DSL/grammar asset <%s> while building contract record\n",
                key);
    }
    return it->second;
  };

  record->observation.sources.grammar =
      dsl_asset_text_or_fail("observation_sources_grammar_filename");
  record->observation.sources.dsl =
      snapshot_contract_dsl_value_or_empty(record->config, "observation_sources_dsl_text");
  if (!has_non_ws_ascii(record->observation.sources.dsl)) {
    record->observation.sources.dsl =
        dsl_asset_text_or_fail("observation_sources_dsl_filename");
  }

  record->observation.channels.grammar =
      dsl_asset_text_or_fail("observation_channels_grammar_filename");
  record->observation.channels.dsl =
      snapshot_contract_dsl_value_or_empty(record->config, "observation_channels_dsl_text");
  if (!has_non_ws_ascii(record->observation.channels.dsl)) {
    record->observation.channels.dsl =
        dsl_asset_text_or_fail("observation_channels_dsl_filename");
  }

  record->jkimyei.grammar =
      dsl_asset_text_or_fail("jkimyei_specs_grammar_filename");
  record->jkimyei.dsl =
      snapshot_contract_dsl_value_or_empty(record->config, "jkimyei_specs_dsl_text");
  if (!has_non_ws_ascii(record->jkimyei.dsl)) {
    record->jkimyei.dsl = dsl_asset_text_or_fail("jkimyei_specs_dsl_filename");
  }

  record->circuit.grammar =
      dsl_asset_text_or_fail("tsiemene_circuit_grammar_filename");
  record->circuit.dsl =
      snapshot_contract_dsl_value_or_empty(record->config, "tsiemene_circuit_dsl_text");
  if (!has_non_ws_ascii(record->circuit.dsl)) {
    record->circuit.dsl = dsl_asset_text_or_fail("tsiemene_circuit_dsl_filename");
  }

  record->canonical_path.grammar =
      dsl_asset_text_or_fail("canonical_path_grammar_filename");

  const std::string removed_circuit_train_text =
      snapshot_contract_dsl_value_or_empty(record->config,
                                           "tsiemene_circuit_train_dsl_text");
  const std::string removed_circuit_run_text =
      snapshot_contract_dsl_value_or_empty(record->config,
                                           "tsiemene_circuit_run_dsl_text");
  if (has_non_ws_ascii(removed_circuit_train_text) ||
      has_non_ws_ascii(removed_circuit_run_text)) {
    log_fatal(
        "[dconfig] mode-split BOARD_CONTRACT_DSL inline keys are removed; "
        "use tsiemene_circuit_dsl_text\n");
  }

  if (!has_non_ws_ascii(record->circuit.dsl)) {
    log_fatal(
        "[dconfig] missing effective circuit DSL payload\n");
  }
  for (const auto& dep_path : dependency_paths) {
    if (!has_non_ws_ascii(dep_path)) continue;
    record->dependency_manifest.files.push_back(fingerprint_file(dep_path));
  }
  record->dependency_manifest.aggregate_sha256_hex =
      compute_manifest_digest_hex(record->dependency_manifest.files);
  return record;
}

[[nodiscard]] static snapshot_ptr_t
snapshot_ptr_or_fail(const contract_hash_t& hash) {
  LOCK_GUARD(contract_config_mutex);
  auto& snapshots = snapshots_by_hash();
  const auto it = snapshots.find(hash);
  if (it == snapshots.end() || !it->second) {
    log_fatal(
        "[dconfig] contract hash lookup failed: hash=%s is not registered in runtime registry\n",
        hash.c_str());
  }
  return it->second;
}

[[nodiscard]] static std::vector<snapshot_ptr_t>
registry_snapshots_copy_locked() {
  auto& snapshots = snapshots_by_hash();
  std::vector<snapshot_ptr_t> out;
  out.reserve(snapshots.size());
  for (const auto& [_, ptr] : snapshots) {
    if (ptr) out.push_back(ptr);
  }
  return out;
}

}  // namespace

/*──────────────────────── contract registry space ─────────────────────────*/
contract_hash_t contract_space_t::register_contract_file(const std::string& path) {
  const std::string canonical_path = canonicalize_path_best_effort(path);
  if (!has_non_ws_ascii(canonical_path)) {
    log_fatal("[dconfig] register_contract_file received empty/invalid path: %s\n",
              path.c_str());
  }

  std::optional<contract_hash_t> existing_hash;
  {
    LOCK_GUARD(contract_config_mutex);
    auto& path_to_hash = hash_by_contract_path();
    auto& snapshots = snapshots_by_hash();
    const auto existing = path_to_hash.find(canonical_path);
    if (existing != path_to_hash.end()) {
      const auto snap_it = snapshots.find(existing->second);
      if (snap_it == snapshots.end() || !snap_it->second) {
        log_fatal(
            "[dconfig] contract registry corruption: path is mapped but snapshot is missing (%s)\n",
            canonical_path.c_str());
      }
      existing_hash = existing->second;
    }
  }
  if (existing_hash.has_value()) {
    assert_intact_or_fail_fast(*existing_hash);
    return *existing_hash;
  }

  const auto built_snapshot =
      build_contract_record_from_contract_path(canonical_path);
  if (!built_snapshot) {
    log_fatal("[dconfig] failed to build contract snapshot from: %s\n",
              canonical_path.c_str());
  }
  const contract_hash_t built_hash =
      built_snapshot->dependency_manifest.aggregate_sha256_hex;
  if (!has_non_ws_ascii(built_hash)) {
    log_fatal("[dconfig] built contract snapshot has empty manifest hash for: %s\n",
              canonical_path.c_str());
  }

  {
    LOCK_GUARD(contract_config_mutex);
    auto& path_to_hash = hash_by_contract_path();
    auto& snapshots = snapshots_by_hash();
    const auto existing = path_to_hash.find(canonical_path);
    if (existing != path_to_hash.end()) {
      if (existing->second != built_hash) {
        log_fatal(
            "[dconfig] immutable contract lock violation: attempted to rebind contract path %s from hash %s to %s\n",
            canonical_path.c_str(),
            existing->second.c_str(),
            built_hash.c_str());
      }
      const auto snap_it = snapshots.find(existing->second);
      if (snap_it == snapshots.end() || !snap_it->second) {
        log_fatal(
            "[dconfig] contract registry corruption: path is mapped but snapshot is missing (%s)\n",
            canonical_path.c_str());
      }
      existing_hash = existing->second;
    } else {
      auto by_hash = snapshots.find(built_hash);
      if (by_hash == snapshots.end()) {
        snapshots.emplace(built_hash, built_snapshot);
      } else if (!by_hash->second) {
        by_hash->second = built_snapshot;
      }
      path_to_hash[canonical_path] = built_hash;
    }
  }

  if (existing_hash.has_value()) {
    assert_intact_or_fail_fast(*existing_hash);
    return *existing_hash;
  }
  return built_hash;
}

std::shared_ptr<const contract_record_t> contract_space_t::contract_itself(
    const contract_hash_t& hash) {
  return snapshot_ptr_or_fail(hash);
}

void contract_space_t::assert_intact_or_fail_fast(const contract_hash_t& hash) {
  const auto snap = snapshot_ptr_or_fail(hash);

  std::vector<contract_file_fingerprint_t> refreshed;
  refreshed.reserve(snap->dependency_manifest.files.size());

  for (const auto& expected : snap->dependency_manifest.files) {
    const std::filesystem::path p(expected.canonical_path);
    if (!std::filesystem::exists(p) || !std::filesystem::is_regular_file(p)) {
      log_fatal(
          "[dconfig] immutable contract lock violation: dependency missing or invalid: %s\n",
          expected.canonical_path.c_str());
    }

    contract_file_fingerprint_t current = expected;
    current.file_size_bytes = std::filesystem::file_size(p);
    current.mtime_ticks = file_mtime_ticks(p);

    if (current.file_size_bytes != expected.file_size_bytes ||
        current.mtime_ticks != expected.mtime_ticks) {
      current.sha256_hex = sha256_hex_from_file(expected.canonical_path);
      if (current.sha256_hex != expected.sha256_hex) {
        log_fatal(
            "[dconfig] immutable contract lock violation: contract dependency changed "
            "mid-run: %s\n",
            expected.canonical_path.c_str());
      }
    }
    refreshed.push_back(std::move(current));
  }

  const std::string digest = compute_manifest_digest_hex(refreshed);
  if (digest != snap->dependency_manifest.aggregate_sha256_hex) {
    log_fatal(
        "[dconfig] immutable contract lock violation: dependency manifest digest "
        "mismatch mid-run\n");
  }
}

void contract_space_t::assert_registry_intact_or_fail_fast() {
  std::vector<snapshot_ptr_t> snapshots;
  {
    LOCK_GUARD(contract_config_mutex);
    snapshots = registry_snapshots_copy_locked();
  }
  for (const auto& ptr : snapshots) {
    if (!ptr) continue;
    assert_intact_or_fail_fast(ptr->dependency_manifest.aggregate_sha256_hex);
  }
}

bool contract_space_t::has_contract(const contract_hash_t& hash) noexcept {
  LOCK_GUARD(contract_config_mutex);
  const auto& snapshots = snapshots_by_hash();
  const auto it = snapshots.find(hash);
  return it != snapshots.end() && static_cast<bool>(it->second);
}

std::vector<contract_hash_t> contract_space_t::registered_hashes() {
  LOCK_GUARD(contract_config_mutex);
  const auto& snapshots = snapshots_by_hash();
  std::vector<contract_hash_t> out{};
  out.reserve(snapshots.size());
  for (const auto& [hash, ptr] : snapshots) {
    if (ptr) out.push_back(hash);
  }
  std::sort(out.begin(), out.end());
  return out;
}

std::string contract_record_t::raw(const std::string& section,
                                   const std::string& key) const {
  const auto s = config.find(section);
  if (s != config.end()) {
    const auto k = s->second.find(key);
    if (k != s->second.end()) return k->second;
  }
  if (const auto instruction_value =
          contract_instruction_section_value_or_empty(*this, section, key);
      instruction_value.has_value()) {
    return *instruction_value;
  }
  if (s == config.end()) {
    if (module_config_path_for_section(*this, section).has_value() ||
        module_section_exists(*this, section)) {
      throw bad_config_access("Missing key <" + key + "> in [" + section + "]");
    }
    throw bad_config_access("Missing section [" + section + "]");
  }
  throw bad_config_access("Missing key <" + key + "> in [" + section + "]");
}

template <class T>
T contract_record_t::from_string(const std::string& s) {
  return parse_scalar_from_string<T>(s);
}

template <class T>
T contract_record_t::get(const std::string& section,
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
std::vector<T> contract_record_t::get_arr(
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

const cuwacunu::camahjucunu::tsiemene_circuit_instruction_t&
contract_record::circuit_blob_t::decoded() const {
  std::call_once(decode_once_, [&]() {
    try {
      auto parser = cuwacunu::camahjucunu::dsl::tsiemeneCircuits(grammar);
      std::string effective_dsl = strip_comments_from_text_preserve_lines(dsl);
      const std::size_t first_non_ws =
          effective_dsl.find_first_not_of(" \t\r\n");
      if (first_non_ws == std::string::npos) {
        throw std::runtime_error("circuit DSL is empty after comment stripping");
      }
      if (first_non_ws > 0) effective_dsl.erase(0, first_non_ws);
      auto inst = parser.decode(effective_dsl);
      std::string semantic_error;
      if (!cuwacunu::camahjucunu::validate_circuit_instruction(
              inst, &semantic_error)) {
        throw std::runtime_error(
            "circuit semantic validation failed: " + semantic_error);
      }
      decoded_cache_ = std::make_shared<cuwacunu::camahjucunu::tsiemene_circuit_instruction_t>(
          std::move(inst));
    } catch (const std::exception& e) {
      throw std::runtime_error(
          std::string("failed to decode circuit DSL: ") + e.what());
    }
  });
  if (!decoded_cache_) {
    throw std::runtime_error("failed to decode circuit DSL: empty decoded cache");
  }
  return *decoded_cache_;
}

const cuwacunu::camahjucunu::observation_spec_t&
contract_record::observation_blob_t::decoded() const {
  std::call_once(decode_once_, [&]() {
    try {
      auto inst = cuwacunu::camahjucunu::decode_observation_spec_from_split_dsl(
          sources.grammar, sources.dsl, channels.grammar, channels.dsl);
      decoded_cache_ =
          std::make_shared<cuwacunu::camahjucunu::observation_spec_t>(
              std::move(inst));
    } catch (const std::exception& e) {
      throw std::runtime_error(
          std::string("failed to decode observation DSL: ") + e.what());
    }
  });
  if (!decoded_cache_) {
    throw std::runtime_error(
        "failed to decode observation DSL: empty decoded cache");
  }
  return *decoded_cache_;
}

const cuwacunu::camahjucunu::jkimyei_specs_t&
contract_record::jkimyei_blob_t::decoded() const {
  std::call_once(decode_once_, [&]() {
    try {
      auto inst = cuwacunu::camahjucunu::dsl::decode_jkimyei_specs_from_dsl(
          grammar, dsl);
      decoded_cache_ =
          std::make_shared<cuwacunu::camahjucunu::jkimyei_specs_t>(
              std::move(inst));
    } catch (const std::exception& e) {
      throw std::runtime_error(
          std::string("failed to decode jkimyei DSL: ") + e.what());
    }
  });
  if (!decoded_cache_) {
    throw std::runtime_error("failed to decode jkimyei DSL: empty decoded cache");
  }
  return *decoded_cache_;
}

template int64_t contract_record_t::get<int64_t>(
    const std::string&, const std::string&, std::optional<int64_t>) const;
template int contract_record_t::get<int>(
    const std::string&, const std::string&, std::optional<int>) const;
template double contract_record_t::get<double>(
    const std::string&, const std::string&, std::optional<double>) const;
template float contract_record_t::get<float>(
    const std::string&, const std::string&, std::optional<float>) const;
template bool contract_record_t::get<bool>(
    const std::string&, const std::string&, std::optional<bool>) const;
template std::string contract_record_t::get<std::string>(
    const std::string&, const std::string&, std::optional<std::string>) const;

template std::vector<int64_t> contract_record_t::get_arr<int64_t>(
    const std::string&, const std::string&,
    std::optional<std::vector<int64_t>>) const;
template std::vector<int> contract_record_t::get_arr<int>(
    const std::string&, const std::string&,
    std::optional<std::vector<int>>) const;
template std::vector<double> contract_record_t::get_arr<double>(
    const std::string&, const std::string&,
    std::optional<std::vector<double>>) const;
template std::vector<float> contract_record_t::get_arr<float>(
    const std::string&, const std::string&,
    std::optional<std::vector<float>>) const;
template std::vector<bool> contract_record_t::get_arr<bool>(
    const std::string&, const std::string&,
    std::optional<std::vector<bool>>) const;
template std::vector<std::string> contract_record_t::get_arr<std::string>(
    const std::string&, const std::string&,
    std::optional<std::vector<std::string>>) const;

}  // namespace iitepi
}  // namespace cuwacunu
