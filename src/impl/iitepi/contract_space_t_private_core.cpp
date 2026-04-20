#include "iitepi/contract_space_t.h"

#include "camahjucunu/dsl/canonical_path/canonical_path.h"
#include "camahjucunu/dsl/jkimyei_specs/jkimyei_specs.h"
#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state.h"
#include "camahjucunu/dsl/network_design/network_design.h"
#include "camahjucunu/dsl/observation_pipeline/observation_spec.h"
#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit.h"
#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit_runtime.h"
#include "camahjucunu/dsl/wave_contract_binding/wave_contract_binding.h"
#include "tsiemene/tsi.type.registry.h"
#include "wikimyei/inference/expected_value/expected_value_network_design.h"
#include "wikimyei/representation/VICReg/vicreg_4d_network_design.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace cuwacunu {
namespace iitepi {

std::mutex contract_config_mutex;

namespace {

using snapshot_ptr_t = std::shared_ptr<const contract_record_t>;
static std::size_t g_registered_contract_count = 0;
static contract_hash_t g_last_registered_contract_hash;
static std::string g_last_registered_contract_path_canonical;
static constexpr const char *kContractSectionDock = "DOCK";
static constexpr const char *kContractSectionAssembly = "ASSEMBLY";
static constexpr const char *kContractSectionAknowledge = "AKNOWLEDGE";
static constexpr const char *kAssemblyKeyCircuitDslFilename =
    "circuit_dsl_filename";
static constexpr const char *kModuleSectionVicreg = "VICReg";
static constexpr const char *kModuleSectionExpectedValue = "EXPECTED_VALUE";
static constexpr const char *kModuleSectionEmbeddingSequenceAnalytics =
    "WIKIMYEI_EVALUATION_EMBEDDING_SEQUENCE_ANALYTICS";
static constexpr const char *kModuleSectionTransferMatrixEvaluation =
    "WIKIMYEI_EVALUATION_TRANSFER_MATRIX_EVALUATION";
static constexpr const char *kContractVariableVicregConfigDslFile =
    "__vicreg_config_dsl_file";
static constexpr const char *kContractVariableExpectedValueConfigDslFile =
    "__expected_value_config_dsl_file";
static constexpr const char
    *kContractVariableEmbeddingSequenceAnalyticsDslFile =
        "__embedding_sequence_analytics_config_dsl_file";
static constexpr const char *kContractVariableTransferMatrixEvaluationDslFile =
    "__transfer_matrix_evaluation_config_dsl_file";
static constexpr const char *kGlobalBnfEmbeddingSequenceAnalyticsGrammarKey =
    "wikimyei_evaluation_embedding_sequence_analytics_grammar_filename";
static constexpr const char *kGlobalBnfTransferMatrixGrammarKey =
    "wikimyei_evaluation_transfer_matrix_evaluation_grammar_filename";

[[nodiscard]] static std::unordered_map<contract_hash_t, snapshot_ptr_t> &
snapshots_by_hash() {
  static std::unordered_map<contract_hash_t, snapshot_ptr_t> registry;
  return registry;
}

[[nodiscard]] static std::unordered_map<std::string, contract_hash_t> &
hash_by_contract_path() {
  static std::unordered_map<std::string, contract_hash_t> registry;
  return registry;
}

[[nodiscard]] static std::string trim_ascii_copy(std::string_view in) {
  std::size_t b = 0;
  while (b < in.size() &&
         std::isspace(static_cast<unsigned char>(in[b])) != 0) {
    ++b;
  }
  std::size_t e = in.size();
  while (e > b && std::isspace(static_cast<unsigned char>(in[e - 1])) != 0) {
    --e;
  }
  return std::string(in.substr(b, e - b));
}

[[nodiscard]] static bool read_all_fd(int fd, std::string *out) {
  if (out == nullptr)
    return false;
  out->clear();
  std::array<char, 4096> buffer{};
  for (;;) {
    const ssize_t n = ::read(fd, buffer.data(), buffer.size());
    if (n < 0) {
      if (errno == EINTR)
        continue;
      return false;
    }
    if (n == 0)
      return true;
    out->append(buffer.data(), static_cast<std::size_t>(n));
  }
}

[[nodiscard]] static std::string strip_comment(std::string_view line,
                                               bool *in_block_comment);
[[nodiscard]] static bool has_non_ws_ascii(const std::string &s);
[[nodiscard]] static std::string trim_ascii_ws_copy(std::string s);
[[nodiscard]] static std::string
strip_comments_from_text_preserve_lines(std::string_view text);
[[nodiscard]] static parsed_config_t parse_config_file(const std::string &path);
[[nodiscard]] static std::vector<std::string>
split_string_items(const std::string &s);
[[nodiscard]] static std::string
resolve_path_from_folder(const std::string &folder, std::string path);
[[nodiscard]] static bool ends_with_ascii(std::string_view text,
                                          std::string_view suffix);
[[nodiscard]] static std::string unquote_if_wrapped(std::string s);
[[nodiscard]] static std::string
canonicalize_path_best_effort(const std::string &path);
[[nodiscard]] static std::string sha256_hex_from_bytes(const void *data,
                                                       std::size_t size);
[[nodiscard]] static std::string sha256_hex_from_file(const std::string &path);
[[nodiscard]] static std::int64_t
file_mtime_ticks(const std::filesystem::path &path);
[[nodiscard]] static contract_file_fingerprint_t
fingerprint_file(const std::string &path);
[[nodiscard]] static std::string compute_manifest_digest_hex(
    const std::vector<contract_file_fingerprint_t> &files);
[[nodiscard]] static std::string json_escape(std::string_view in);
[[nodiscard]] static std::vector<cuwacunu::camahjucunu::dsl_variable_t>
contract_dsl_variables_from_section(const parsed_config_section_t &section);
[[nodiscard]] static std::string canonical_contract_docking_signature_json(
    const contract_docking_signature_t &signature);
[[nodiscard]] static std::string
canonical_contract_signature_json(const contract_signature_t &signature);
[[nodiscard]] static std::optional<contract_component_binding_t>
derive_contract_component_binding(
    const cuwacunu::camahjucunu::tsiemene_instance_decl_t &instance,
    std::string_view contract_hash_for_canonicalization,
    std::string_view vicreg_module_path,
    std::string_view vicreg_module_sha256_hex,
    std::string_view expected_value_module_path,
    std::string_view expected_value_module_sha256_hex, std::string *error);
[[nodiscard]] static std::vector<contract_component_binding_t>
derive_contract_component_bindings_or_fail(
    const cuwacunu::camahjucunu::tsiemene_circuit_instruction_t &instruction,
    std::string_view contract_hash_for_canonicalization,
    std::string_view vicreg_module_path,
    std::string_view vicreg_module_sha256_hex,
    std::string_view expected_value_module_path,
    std::string_view expected_value_module_sha256_hex);
[[nodiscard]] static std::string
contract_required_resolved_path(const parsed_config_t &cfg,
                                const std::string &cfg_folder,
                                const char *section, const char *key);
[[nodiscard]] static std::optional<std::string>
contract_optional_resolved_path(const parsed_config_t &cfg,
                                const std::string &cfg_folder,
                                const char *section, const char *key);
[[nodiscard]] static parsed_config_section_t
merged_contract_variable_section(const parsed_config_t &cfg);
[[nodiscard]] static std::optional<std::string>
contract_optional_resolved_variable_path(const parsed_config_t &cfg,
                                         const std::string &cfg_folder,
                                         const char *variable_name);
[[nodiscard]] static std::string
contract_required_resolved_variable_path(const parsed_config_t &cfg,
                                         const std::string &cfg_folder,
                                         const char *variable_name);
[[nodiscard]] static parsed_config_section_t parse_instruction_file(
    const std::string &path,
    const std::string &latent_lineage_state_grammar_text,
    const parsed_config_section_t *contract_variables = nullptr,
    std::string *out_resolved_text = nullptr);
[[nodiscard]] static bool
module_section_exists(const contract_record_t &snapshot,
                      std::string_view section) noexcept;
[[nodiscard]] static std::shared_ptr<contract_record_t>
build_contract_record_from_contract_path(const std::string &contract_file_path);
[[nodiscard]] static bool
section_supports_instruction_file(std::string_view section) noexcept;
[[nodiscard]] static std::optional<std::string>
module_config_path_for_section(const contract_record_t &snapshot,
                               const std::string &section);
[[nodiscard]] static std::optional<std::string>
contract_instruction_section_value_or_empty(const contract_record_t &snapshot,
                                            const std::string &section,
                                            const std::string &key);
[[nodiscard]] static snapshot_ptr_t
snapshot_ptr_or_fail(const contract_hash_t &hash);
[[nodiscard]] static std::optional<std::string>
global_optional_file_text_by_key(const char *section, const char *key);
[[nodiscard]] static std::string indent_lines(const std::string &text,
                                              std::string_view prefix);
static void normalize_vicreg_module_architecture_from_network_design(
    contract_record_t *record);
static void normalize_expected_value_module_architecture_from_network_design(
    contract_record_t *record);
static bool
validate_contract_config_or_terminate(const parsed_config_t &cfg,
                                      const std::string &cfg_folder);

template <class T> static T parse_scalar_from_string(const std::string &s);

[[nodiscard]] static const char *non_empty_or(const std::string &s,
                                              const char *fallback) {
  return has_non_ws_ascii(s) ? s.c_str() : fallback;
}

[[nodiscard]] static std::string bool_token(bool value) {
  return value ? "true" : "false";
}

/*──────────────────────── shared helpers ─────────────────────────────*/
[[nodiscard]] static std::string strip_comment(std::string_view line,
                                               bool *in_block_comment) {
  bool block_comment =
      (in_block_comment != nullptr) ? *in_block_comment : false;
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
  if (in_block_comment != nullptr)
    *in_block_comment = block_comment;
  return out;
}

[[nodiscard]] static std::vector<std::string>
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

  for (auto &item : out) {
    if (item.size() < 2)
      continue;
    const char a = item.front();
    const char b = item.back();
    if ((a == '\'' && b == '\'') || (a == '"' && b == '"')) {
      item = item.substr(1, item.size() - 2);
    }
  }
  return out;
}

[[nodiscard]] static bool has_non_ws_ascii(const std::string &s) {
  for (const unsigned char c : s) {
    if (!std::isspace(c))
      return true;
  }
  return false;
}

[[nodiscard]] static std::string trim_ascii_ws_copy(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b])))
    ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])))
    --e;
  return s.substr(b, e - b);
}

[[nodiscard]] static std::string
strip_comments_from_text_preserve_lines(std::string_view text) {
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

[[nodiscard]] static std::string
resolve_path_from_folder(const std::string &folder, std::string path) {
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

[[nodiscard]] static std::string
parent_folder_from_path(const std::string &path) {
  const std::filesystem::path p(path);
  if (!p.has_parent_path())
    return {};
  return p.parent_path().string();
}

[[nodiscard]] static bool ends_with_ascii(std::string_view text,
                                          std::string_view suffix) {
  if (suffix.size() > text.size())
    return false;
  return text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

[[nodiscard]] static std::string unquote_if_wrapped(std::string s) {
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

[[nodiscard]] static std::string
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

[[nodiscard]] static std::string sha256_hex_from_bytes(const void *data,
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
  const auto ch = [](std::uint32_t x, std::uint32_t y,
                     std::uint32_t z) noexcept { return (x & y) ^ (~x & z); };
  const auto maj = [](std::uint32_t x, std::uint32_t y,
                      std::uint32_t z) noexcept {
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

  const auto transform = [&](sha256_state_t *s, const std::uint8_t *block) {
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
      const std::uint32_t t1 = h + big_sigma1(e) + ch(e, f, g) + k[i] + w[i];
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

  const auto init = [&](sha256_state_t *s) {
    s->h = {0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
            0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u};
    s->block.fill(0);
    s->block_len = 0;
    s->total_bits = 0;
  };

  const auto update = [&](sha256_state_t *s, const std::uint8_t *bytes,
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

  const auto final = [&](sha256_state_t *s, std::array<std::uint8_t, 32> *out) {
    s->total_bits += static_cast<std::uint64_t>(s->block_len) * 8ULL;

    s->block[s->block_len++] = 0x80u;
    if (s->block_len > 56) {
      while (s->block_len < 64)
        s->block[s->block_len++] = 0x00u;
      transform(s, s->block.data());
      s->block_len = 0;
    }
    while (s->block_len < 56)
      s->block[s->block_len++] = 0x00u;

    for (int i = 7; i >= 0; --i) {
      s->block[s->block_len++] =
          static_cast<std::uint8_t>((s->total_bits >> (i * 8)) & 0xFFu);
    }
    transform(s, s->block.data());

    for (std::size_t i = 0; i < 8; ++i) {
      (*out)[i * 4] = static_cast<std::uint8_t>((s->h[i] >> 24U) & 0xFFu);
      (*out)[i * 4 + 1] = static_cast<std::uint8_t>((s->h[i] >> 16U) & 0xFFu);
      (*out)[i * 4 + 2] = static_cast<std::uint8_t>((s->h[i] >> 8U) & 0xFFu);
      (*out)[i * 4 + 3] = static_cast<std::uint8_t>(s->h[i] & 0xFFu);
    }
  };

  sha256_state_t state{};
  init(&state);
  if (size > 0 && data) {
    update(&state, static_cast<const std::uint8_t *>(data), size);
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

[[nodiscard]] static std::string sha256_hex_from_file(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    log_fatal("[dconfig] cannot open file to fingerprint: %s\n", path.c_str());
  }
  std::ostringstream oss;
  oss << in.rdbuf();
  const std::string bytes = oss.str();
  return sha256_hex_from_bytes(bytes.data(), bytes.size());
}

[[nodiscard]] static std::int64_t
file_mtime_ticks(const std::filesystem::path &path) {
  std::error_code ec;
  const auto tp = std::filesystem::last_write_time(path, ec);
  if (ec)
    return 0;
  using ticks_t =
      std::chrono::duration<std::int64_t, std::chrono::nanoseconds::period>;
  return std::chrono::duration_cast<ticks_t>(tp.time_since_epoch()).count();
}

[[nodiscard]] static contract_file_fingerprint_t
fingerprint_file(const std::string &path) {
  const std::string canonical = canonicalize_path_best_effort(path);
  if (!has_non_ws_ascii(canonical)) {
    log_fatal("[dconfig] cannot fingerprint empty path\n");
  }

  const std::filesystem::path p(canonical);
  if (!std::filesystem::exists(p)) {
    log_fatal("[dconfig] fingerprint path does not exist: %s\n",
              canonical.c_str());
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
    const std::vector<contract_file_fingerprint_t> &files) {
  std::vector<std::string> rows;
  rows.reserve(files.size());
  for (const auto &f : files) {
    rows.emplace_back(f.canonical_path + "|" + f.sha256_hex + "\n");
  }
  std::sort(rows.begin(), rows.end());

  std::string payload;
  for (const auto &row : rows)
    payload += row;
  return sha256_hex_from_bytes(payload.data(), payload.size());
}

[[nodiscard]] static std::string json_escape(std::string_view in) {
  std::ostringstream out;
  for (const unsigned char c : in) {
    switch (c) {
    case '"':
      out << "\\\"";
      break;
    case '\\':
      out << "\\\\";
      break;
    case '\n':
      out << "\\n";
      break;
    case '\r':
      out << "\\r";
      break;
    case '\t':
      out << "\\t";
      break;
    default:
      if (c < 0x20) {
        static constexpr char kHex[] = "0123456789abcdef";
        out << "\\u00" << kHex[(c >> 4) & 0x0F] << kHex[c & 0x0F];
      } else {
        out << static_cast<char>(c);
      }
      break;
    }
  }
  return out.str();
}

[[nodiscard]] static std::vector<cuwacunu::camahjucunu::dsl_variable_t>
contract_dsl_variables_from_config(const parsed_config_t &cfg) {
  return contract_dsl_variables_from_section(
      merged_contract_variable_section(cfg));
}

[[nodiscard]] static std::vector<cuwacunu::camahjucunu::dsl_variable_t>
contract_dsl_variables_from_section(const parsed_config_section_t &section) {
  std::vector<cuwacunu::camahjucunu::dsl_variable_t> variables{};
  variables.reserve(section.size());
  for (const auto &[name, value] : section) {
    if (!cuwacunu::camahjucunu::is_dsl_variable_name(name))
      continue;
    variables.push_back({name, value});
  }
  return variables;
}

[[nodiscard]] static std::string resolve_dsl_variables_or_fail(
    std::string_view text,
    const std::vector<cuwacunu::camahjucunu::dsl_variable_t> &variables,
    std::string_view label) {
  if (variables.empty())
    return std::string(text);
  std::string resolved{};
  std::string error{};
  if (!cuwacunu::camahjucunu::resolve_dsl_variables_in_text(
          text, variables, &resolved, &error)) {
    log_fatal("[dconfig] failed to resolve contract DSL variables in %s: %s\n",
              std::string(label).c_str(), error.c_str());
  }
  return resolved;
}

[[nodiscard]] static bool
is_contract_assembly_path_variable(std::string_view name) noexcept {
  return name == "__observation_sources_dsl_file" ||
         name == "__observation_channels_dsl_file" ||
         name == kContractVariableVicregConfigDslFile ||
         name == kContractVariableExpectedValueConfigDslFile ||
         name == kContractVariableEmbeddingSequenceAnalyticsDslFile ||
         name == kContractVariableTransferMatrixEvaluationDslFile;
}

[[nodiscard]] static bool
is_current_dock_contract_variable(std::string_view name) noexcept {
  return name == "__obs_channels" || name == "__obs_seq_length" ||
         name == "__obs_feature_dim" || name == "__embedding_dims" ||
         name == "__future_target_dims";
}

[[nodiscard]] static bool
is_private_assembly_contract_variable(std::string_view name) noexcept {
  return name == "__future_target_weights" || name.rfind("__vicreg_", 0) == 0 ||
         is_contract_assembly_path_variable(name);
}

[[nodiscard]] static std::vector<contract_variable_assignment_t>
identity_contract_variable_assignments(
    const std::vector<contract_variable_assignment_t> &assignments) {
  std::vector<contract_variable_assignment_t> out{};
  out.reserve(assignments.size());
  for (const auto &assignment : assignments) {
    // Path-bearing assembly selectors already contribute via resolved module or
    // staged surface fingerprints. Repeating their raw path text would make
    // identity depend on local path spelling.
    if (is_contract_assembly_path_variable(assignment.name))
      continue;
    out.push_back(assignment);
  }
  return out;
}

[[nodiscard]] static std::vector<contract_variable_assignment_t>
contract_variable_assignments_from_section(
    const parsed_config_section_t &section) {
  std::vector<contract_variable_assignment_t> out{};
  out.reserve(section.size());
  for (const auto &[name, value] : section) {
    if (!cuwacunu::camahjucunu::is_dsl_variable_name(name))
      continue;
    out.push_back({name, value});
  }
  return out;
}

[[nodiscard]] static bool
is_public_docking_surface_id(std::string_view surface_id) noexcept {
  return surface_id == "contract.circuit" ||
         surface_id == "contract.observation.channels";
}

[[nodiscard]] static std::vector<contract_docking_surface_entry_t>
public_docking_contract_surfaces(
    const std::vector<contract_docking_surface_entry_t> &surfaces) {
  std::vector<contract_docking_surface_entry_t> out{};
  out.reserve(surfaces.size());
  for (const auto &surface : surfaces) {
    if (!is_public_docking_surface_id(surface.surface_id))
      continue;
    out.push_back(surface);
  }
  return out;
}

[[nodiscard]] static std::string
canonical_contract_signature_json(const contract_signature_t &signature);

[[nodiscard]] static std::string canonical_contract_docking_signature_json(
    const contract_docking_signature_t &signature) {
  std::vector<std::string> circuits = signature.compatible_circuits;
  std::vector<contract_variable_assignment_t> variables =
      signature.variable_assignments;
  std::vector<contract_docking_surface_entry_t> surfaces =
      public_docking_contract_surfaces(signature.surfaces);
  std::sort(circuits.begin(), circuits.end());
  std::sort(variables.begin(), variables.end(),
            [](const contract_variable_assignment_t &a,
               const contract_variable_assignment_t &b) {
              if (a.name != b.name)
                return a.name < b.name;
              return a.value < b.value;
            });
  std::sort(surfaces.begin(), surfaces.end(),
            [](const contract_docking_surface_entry_t &a,
               const contract_docking_surface_entry_t &b) {
              if (a.surface_id != b.surface_id) {
                return a.surface_id < b.surface_id;
              }
              if (a.sha256_hex != b.sha256_hex)
                return a.sha256_hex < b.sha256_hex;
              return a.canonical_path < b.canonical_path;
            });

  std::ostringstream out;
  out << "{\"schema\":\"" << json_escape(signature.schema)
      << "\",\"compatible_circuits\":[";
  for (std::size_t i = 0; i < circuits.size(); ++i) {
    if (i != 0)
      out << ",";
    out << "\"" << json_escape(circuits[i]) << "\"";
  }
  out << "],\"variables\":[";
  for (std::size_t i = 0; i < variables.size(); ++i) {
    const auto &v = variables[i];
    if (i != 0)
      out << ",";
    out << "{\"name\":\"" << json_escape(v.name) << "\",\"value\":\""
        << json_escape(v.value) << "\"}";
  }
  out << "],\"surfaces\":[";
  for (std::size_t i = 0; i < surfaces.size(); ++i) {
    const auto &s = surfaces[i];
    if (i != 0)
      out << ",";
    out << "{\"surface_id\":\"" << json_escape(s.surface_id)
        << "\",\"sha256_hex\":\"" << json_escape(s.sha256_hex) << "\"}";
  }
  out << "]}";
  return out.str();
}

[[nodiscard]] static std::string
canonical_contract_signature_json(const contract_signature_t &signature) {
  std::vector<contract_component_binding_t> bindings = signature.bindings;
  std::vector<contract_module_signature_entry_t> modules =
      signature.module_dsl_entries;
  std::vector<contract_variable_assignment_t> variables =
      identity_contract_variable_assignments(signature.variable_assignments);
  std::sort(bindings.begin(), bindings.end(),
            [](const contract_component_binding_t &a,
               const contract_component_binding_t &b) {
              if (a.canonical_path != b.canonical_path) {
                return a.canonical_path < b.canonical_path;
              }
              if (a.tsi_type != b.tsi_type)
                return a.tsi_type < b.tsi_type;
              if (a.hashimyei != b.hashimyei)
                return a.hashimyei < b.hashimyei;
              if (a.tsi_dsl_sha256_hex != b.tsi_dsl_sha256_hex) {
                return a.tsi_dsl_sha256_hex < b.tsi_dsl_sha256_hex;
              }
              return a.tsi_dsl_path < b.tsi_dsl_path;
            });
  std::sort(modules.begin(), modules.end(),
            [](const contract_module_signature_entry_t &a,
               const contract_module_signature_entry_t &b) {
              if (a.module_id != b.module_id)
                return a.module_id < b.module_id;
              if (a.module_dsl_sha256_hex != b.module_dsl_sha256_hex) {
                return a.module_dsl_sha256_hex < b.module_dsl_sha256_hex;
              }
              return a.module_dsl_path < b.module_dsl_path;
            });
  std::sort(variables.begin(), variables.end(),
            [](const contract_variable_assignment_t &a,
               const contract_variable_assignment_t &b) {
              if (a.name != b.name)
                return a.name < b.name;
              return a.value < b.value;
            });

  std::ostringstream out;
  out << "{\"circuit_dsl_sha256_hex\":\""
      << json_escape(signature.circuit_dsl_sha256_hex)
      << "\",\"docking_signature_sha256_hex\":\""
      << json_escape(signature.docking_signature_sha256_hex)
      << "\",\"bindings\":[";
  for (std::size_t i = 0; i < bindings.size(); ++i) {
    const auto &b = bindings[i];
    if (i != 0)
      out << ",";
    out << "{\"canonical_path\":\"" << json_escape(b.canonical_path)
        << "\",\"tsi_type\":\"" << json_escape(b.tsi_type)
        << "\",\"hashimyei\":\"" << json_escape(b.hashimyei)
        << "\",\"tsi_dsl_sha256_hex\":\"" << json_escape(b.tsi_dsl_sha256_hex)
        << "\"}";
  }
  out << "],\"modules\":[";
  for (std::size_t i = 0; i < modules.size(); ++i) {
    const auto &m = modules[i];
    if (i != 0)
      out << ",";
    out << "{\"module_id\":\"" << json_escape(m.module_id)
        << "\",\"module_dsl_sha256_hex\":\""
        << json_escape(m.module_dsl_sha256_hex) << "\"}";
  }
  out << "],\"variables\":[";
  for (std::size_t i = 0; i < variables.size(); ++i) {
    const auto &v = variables[i];
    if (i != 0)
      out << ",";
    out << "{\"name\":\"" << json_escape(v.name) << "\",\"value\":\""
        << json_escape(v.value) << "\"}";
  }
  out << "]}";
  return out.str();
}

[[nodiscard]] static std::optional<contract_component_binding_t>
derive_contract_component_binding(
    const cuwacunu::camahjucunu::tsiemene_instance_decl_t &instance,
    std::string_view contract_hash_for_canonicalization,
    std::string_view vicreg_module_path,
    std::string_view vicreg_module_sha256_hex,
    std::string_view expected_value_module_path,
    std::string_view expected_value_module_sha256_hex, std::string *error) {
  if (error)
    error->clear();
  const std::string raw_path = trim_ascii_ws_copy(instance.tsi_type);
  if (!has_non_ws_ascii(raw_path)) {
    if (error)
      *error = "empty circuit instance path for alias '" + instance.alias + "'";
    return std::nullopt;
  }

  const auto decode_path =
      [&](const std::string &text) -> cuwacunu::camahjucunu::canonical_path_t {
    if (has_non_ws_ascii(std::string(contract_hash_for_canonicalization))) {
      return cuwacunu::camahjucunu::decode_canonical_path(
          text, std::string(contract_hash_for_canonicalization));
    }
    return cuwacunu::camahjucunu::decode_canonical_path(text);
  };

  cuwacunu::camahjucunu::canonical_path_t parsed{};
  parsed = decode_path(raw_path);

  contract_component_binding_t binding{};
  bool family_without_hash = false;
  std::string canonical_identity =
      (parsed.ok &&
       parsed.path_kind == cuwacunu::camahjucunu::canonical_path_kind_e::Node &&
       has_non_ws_ascii(parsed.canonical_identity))
          ? parsed.canonical_identity
          : std::string{};
  if (!has_non_ws_ascii(canonical_identity)) {
    const auto synthetic = decode_path(raw_path + ".0x0000");
    if (synthetic.ok &&
        synthetic.path_kind ==
            cuwacunu::camahjucunu::canonical_path_kind_e::Node &&
        has_non_ws_ascii(synthetic.canonical_identity)) {
      const auto synthetic_type =
          tsiemene::parse_tsi_type_id(synthetic.canonical_identity);
      if (synthetic_type.has_value() &&
          tsiemene::tsi_type_instance_policy(*synthetic_type) ==
              tsiemene::TsiInstancePolicy::HashimyeiInstances &&
          raw_path == std::string(tsiemene::tsi_type_token(*synthetic_type))) {
        canonical_identity = synthetic.canonical_identity;
        parsed = synthetic;
        parsed.hashimyei.clear();
        family_without_hash = true;
      }
    }
  }
  if (!has_non_ws_ascii(canonical_identity)) {
    if (error) {
      *error = "invalid contract binding path for alias '" + instance.alias +
               "': " + raw_path;
    }
    return std::nullopt;
  }
  binding.canonical_path =
      family_without_hash ? canonical_identity
                          : ((parsed.ok && has_non_ws_ascii(parsed.canonical))
                                 ? parsed.canonical
                                 : raw_path);

  const auto type_id = tsiemene::parse_tsi_type_id(canonical_identity);
  if (!type_id.has_value()) {
    if (error) {
      *error = "unsupported tsi_type in contract binding path: " +
               binding.canonical_path;
    }
    return std::nullopt;
  }
  binding.tsi_type = std::string(tsiemene::tsi_type_token(*type_id));

  const auto policy = tsiemene::tsi_type_instance_policy(*type_id);
  if (policy == tsiemene::TsiInstancePolicy::HashimyeiInstances) {
    if (!family_without_hash && parsed.ok &&
        has_non_ws_ascii(parsed.hashimyei)) {
      binding.hashimyei = parsed.hashimyei;
    }
  }

  if (*type_id == tsiemene::TsiTypeId::WikimyeiRepresentationVicreg) {
    binding.tsi_dsl_path = trim_ascii_ws_copy(std::string(vicreg_module_path));
  } else if (*type_id == tsiemene::TsiTypeId::WikimyeiInferenceMdn) {
    binding.tsi_dsl_path =
        trim_ascii_ws_copy(std::string(expected_value_module_path));
  }

  if (has_non_ws_ascii(binding.tsi_dsl_path)) {
    binding.tsi_dsl_path = canonicalize_path_best_effort(binding.tsi_dsl_path);
    if (!has_non_ws_ascii(binding.tsi_dsl_path) ||
        !std::filesystem::exists(binding.tsi_dsl_path) ||
        !std::filesystem::is_regular_file(binding.tsi_dsl_path)) {
      if (error) {
        *error =
            "invalid component DSL path in contract signature for binding: " +
            binding.canonical_path;
      }
      return std::nullopt;
    }
    if (*type_id == tsiemene::TsiTypeId::WikimyeiInferenceMdn &&
        has_non_ws_ascii(std::string(expected_value_module_sha256_hex))) {
      binding.tsi_dsl_sha256_hex =
          std::string(expected_value_module_sha256_hex);
    } else if (*type_id == tsiemene::TsiTypeId::WikimyeiRepresentationVicreg &&
               has_non_ws_ascii(std::string(vicreg_module_sha256_hex))) {
      binding.tsi_dsl_sha256_hex = std::string(vicreg_module_sha256_hex);
    } else {
      binding.tsi_dsl_sha256_hex = sha256_hex_from_file(binding.tsi_dsl_path);
    }
  }

  return binding;
}

[[nodiscard]] static std::vector<contract_component_binding_t>
derive_contract_component_bindings_or_fail(
    const cuwacunu::camahjucunu::tsiemene_circuit_instruction_t &instruction,
    std::string_view contract_hash_for_canonicalization,
    std::string_view vicreg_module_path,
    std::string_view vicreg_module_sha256_hex,
    std::string_view expected_value_module_path,
    std::string_view expected_value_module_sha256_hex) {
  std::vector<contract_component_binding_t> out{};
  std::unordered_set<std::string> seen{};
  for (const auto &circuit : instruction.circuits) {
    for (const auto &instance : circuit.instances) {
      std::string local_error{};
      const auto maybe_binding = derive_contract_component_binding(
          instance, contract_hash_for_canonicalization, vicreg_module_path,
          vicreg_module_sha256_hex, expected_value_module_path,
          expected_value_module_sha256_hex, &local_error);
      if (!maybe_binding.has_value()) {
        log_fatal(
            "[dconfig] invalid contract component binding for alias '%s': %s\n",
            instance.alias.c_str(), local_error.c_str());
      }
      const auto &binding = *maybe_binding;
      const std::string dedupe_key =
          binding.canonical_path + "|" + binding.tsi_type + "|" +
          binding.hashimyei + "|" + binding.tsi_dsl_path + "|" +
          binding.tsi_dsl_sha256_hex;
      if (!seen.insert(dedupe_key).second)
        continue;
      out.push_back(binding);
    }
  }
  std::sort(out.begin(), out.end(),
            [](const contract_component_binding_t &a,
               const contract_component_binding_t &b) {
              if (a.canonical_path != b.canonical_path) {
                return a.canonical_path < b.canonical_path;
              }
              if (a.tsi_type != b.tsi_type)
                return a.tsi_type < b.tsi_type;
              if (a.hashimyei != b.hashimyei)
                return a.hashimyei < b.hashimyei;
              if (a.tsi_dsl_path != b.tsi_dsl_path) {
                return a.tsi_dsl_path < b.tsi_dsl_path;
              }
              return a.tsi_dsl_sha256_hex < b.tsi_dsl_sha256_hex;
            });
  return out;
}

[[nodiscard]] static std::string
contract_required_resolved_path(const parsed_config_t &cfg,
                                const std::string &cfg_folder,
                                const char *section, const char *key) {
  const auto sec_it = cfg.find(section);
  if (sec_it == cfg.end()) {
    log_fatal(
        "[dconfig] missing contract section [%s] while building snapshot\n",
        section);
  }
  const auto key_it = sec_it->second.find(key);
  if (key_it == sec_it->second.end()) {
    log_fatal("[dconfig] missing contract key <%s> in section [%s] while "
              "building snapshot\n",
              key, section);
  }
  const std::string raw = trim_ascii_ws_copy(key_it->second);
  if (!has_non_ws_ascii(raw)) {
    log_fatal("[dconfig] empty contract key <%s> in section [%s] while "
              "building snapshot\n",
              key, section);
  }

  const std::string resolved = resolve_path_from_folder(cfg_folder, raw);
  if (!has_non_ws_ascii(resolved)) {
    log_fatal(
        "[dconfig] unable to resolve contract path for <%s> in section [%s]\n",
        key, section);
  }
  if (!std::filesystem::exists(resolved)) {
    log_fatal("[dconfig] contract dependency path does not exist: %s\n",
              resolved.c_str());
  }
  if (!std::filesystem::is_regular_file(resolved)) {
    log_fatal("[dconfig] contract dependency path is not a regular file: %s\n",
              resolved.c_str());
  }
  return resolved;
}

[[nodiscard]] static std::optional<std::string>
contract_optional_resolved_path(const parsed_config_t &cfg,
                                const std::string &cfg_folder,
                                const char *section, const char *key) {
  const auto sec_it = cfg.find(section);
  if (sec_it == cfg.end())
    return std::nullopt;
  const auto key_it = sec_it->second.find(key);
  if (key_it == sec_it->second.end())
    return std::nullopt;

  const std::string raw = trim_ascii_ws_copy(key_it->second);
  if (!has_non_ws_ascii(raw))
    return std::nullopt;

  const std::string resolved = resolve_path_from_folder(cfg_folder, raw);
  if (!has_non_ws_ascii(resolved)) {
    log_fatal("[dconfig] unable to resolve optional contract path for <%s> in "
              "section [%s]\n",
              key, section);
  }
  if (!std::filesystem::exists(resolved)) {
    log_fatal(
        "[dconfig] optional contract dependency path does not exist: %s\n",
        resolved.c_str());
  }
  if (!std::filesystem::is_regular_file(resolved)) {
    log_fatal("[dconfig] optional contract dependency path is not a regular "
              "file: %s\n",
              resolved.c_str());
  }
  return resolved;
}

[[nodiscard]] static std::optional<std::string>
contract_optional_resolved_variable_path(const parsed_config_t &cfg,
                                         const std::string &cfg_folder,
                                         const char *variable_name) {
  const auto assembly_it = cfg.find(kContractSectionAssembly);
  if (assembly_it == cfg.end())
    return std::nullopt;
  const auto value_it = assembly_it->second.find(variable_name);
  if (value_it == assembly_it->second.end())
    return std::nullopt;

  const std::string raw = trim_ascii_ws_copy(value_it->second);
  if (!has_non_ws_ascii(raw))
    return std::nullopt;

  const std::string resolved =
      resolve_path_from_folder(cfg_folder, unquote_if_wrapped(raw));
  if (!has_non_ws_ascii(resolved)) {
    log_fatal("[dconfig] unable to resolve contract assembly path for <%s>\n",
              variable_name);
  }
  if (!std::filesystem::exists(resolved)) {
    log_fatal(
        "[dconfig] contract assembly dependency path does not exist: %s\n",
        resolved.c_str());
  }
  if (!std::filesystem::is_regular_file(resolved)) {
    log_fatal("[dconfig] contract assembly dependency path is not a regular "
              "file: %s\n",
              resolved.c_str());
  }
  return resolved;
}

[[nodiscard]] static std::string
contract_required_resolved_variable_path(const parsed_config_t &cfg,
                                         const std::string &cfg_folder,
                                         const char *variable_name) {
  const auto resolved =
      contract_optional_resolved_variable_path(cfg, cfg_folder, variable_name);
  if (!resolved.has_value()) {
    log_fatal("[dconfig] missing required contract assembly variable <%s> "
              "while building snapshot\n",
              variable_name);
  }
  return *resolved;
}

[[nodiscard]] static std::string
global_required_resolved_path(const char *section, const char *key) {
  std::string raw;
  std::string cfg_folder;
  {
    LOCK_GUARD(config_mutex);
    const auto sec_it = config_space_t::config.find(section);
    if (sec_it == config_space_t::config.end()) {
      log_fatal("[dconfig] missing global section [%s]\n", section);
    }
    const auto key_it = sec_it->second.find(key);
    if (key_it == sec_it->second.end()) {
      log_fatal("[dconfig] missing global key <%s> in section [%s]\n", key,
                section);
    }
    raw = trim_ascii_ws_copy(key_it->second);
    cfg_folder = parent_folder_from_path(config_space_t::config_file_path);
  }
  if (!has_non_ws_ascii(raw)) {
    log_fatal("[dconfig] empty global key <%s> in section [%s]\n", key,
              section);
  }

  const std::string resolved = resolve_path_from_folder(cfg_folder, raw);
  if (!has_non_ws_ascii(resolved)) {
    log_fatal(
        "[dconfig] unable to resolve global path for <%s> in section [%s]\n",
        key, section);
  }
  if (!std::filesystem::exists(resolved)) {
    log_fatal("[dconfig] global dependency path does not exist: %s\n",
              resolved.c_str());
  }
  if (!std::filesystem::is_regular_file(resolved)) {
    log_fatal("[dconfig] global dependency path is not a regular file: %s\n",
              resolved.c_str());
  }
  return resolved;
}

[[nodiscard]] static std::optional<std::string>
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

[[nodiscard]] static parsed_config_section_t
merged_contract_variable_section(const parsed_config_t &cfg) {
  parsed_config_section_t merged{};
  const auto append_section = [&](const char *section_name) {
    const auto section_it = cfg.find(section_name);
    if (section_it == cfg.end())
      return;
    for (const auto &[name, value] : section_it->second) {
      if (!cuwacunu::camahjucunu::is_dsl_variable_name(name))
        continue;
      const auto [it, inserted] = merged.emplace(name, value);
      if (!inserted && it->second != value) {
        log_fatal("[dconfig] duplicate contract variable <%s> across "
                  "DOCK/ASSEMBLY sections\n",
                  name.c_str());
      }
    }
  };
  append_section(kContractSectionDock);
  append_section(kContractSectionAssembly);
  return merged;
}

[[nodiscard]] static std::string indent_lines(const std::string &text,
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

[[nodiscard]] static bool
module_section_exists(const contract_record_t &snapshot,
                      std::string_view section) noexcept {
  return snapshot.module_sections.find(std::string(section)) !=
         snapshot.module_sections.end();
}

