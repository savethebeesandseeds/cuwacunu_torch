#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <unistd.h>

#include <openssl/evp.h>

#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state_lhs.h"
#include "camahjucunu/data/memory_mapped_dataset.h"
#include "hero/lattice_hero/lattice_catalog.h"
#include "hero/lattice_hero/source_runtime_projection.h"
#include "hero/hashimyei_hero/hashimyei_catalog.h"
#include "camahjucunu/dsl/canonical_path/canonical_path.h"
#include "camahjucunu/dsl/iitepi_wave/iitepi_wave.h"
#include "camahjucunu/dsl/network_design/network_design.h"
#include "camahjucunu/dsl/observation_pipeline/observation_spec.h"
#include "camahjucunu/types/types_data.h"
#include "camahjucunu/types/types_enums.h"
#include "iitepi/board/board.contract.init.h"
#include "iitepi/iitepi.h"
#include "piaabo/dlogs.h"
#include "tsiemene/tsi.type.registry.h"

namespace {

constexpr const char* kStoreRootEnv = "CUWACUNU_HASHIMYEI_STORE_ROOT";
constexpr const char* kDefaultStoreRoot = "/cuwacunu/.hashimyei";
constexpr const char* kDefaultGlobalConfigPath = "/cuwacunu/src/config/.config";
constexpr const char* kDefaultWaveHeroDslPath =
    "/cuwacunu/src/config/instructions/default.hero.lattice.dsl";
constexpr const char* kServerName = "hero_lattice_mcp";
constexpr const char* kServerVersion = "0.1.0";
constexpr const char* kProtocolVersion = "2025-03-26";
constexpr const char* kInitializeInstructions =
    "Use tools/list for tool schemas. Use tools/call with hero.lattice.*.";
constexpr std::size_t kMaxJsonRpcPayloadBytes = 8u << 20;  // 8 MiB

bool g_jsonrpc_use_content_length_framing = false;
int g_protocol_stdout_fd = -1;

struct app_context_t {
  std::filesystem::path store_root{};
  std::filesystem::path config_folder{DEFAULT_CONFIG_FOLDER};
  std::filesystem::path lattice_catalog_path{};
  std::filesystem::path hashimyei_catalog_path{};
  cuwacunu::hero::wave::lattice_catalog_store_t catalog{};
  cuwacunu::hero::hashimyei::hashimyei_catalog_store_t hashimyei_catalog{};
  bool hashimyei_catalog_ready{false};
  bool runtime_initialized{false};
};

struct wave_runtime_defaults_t {
  std::filesystem::path store_root{};
  std::filesystem::path catalog_path{};
  std::filesystem::path hashimyei_catalog_path{};
  std::filesystem::path config_folder{};
};

struct binding_resolution_t {
  std::string board_hash{};
  std::string binding_id{};
  std::string wave_id{};
  cuwacunu::hero::wave::wave_cell_coord_t coord{};
  std::string sampler{};
  std::string record_type{};
  std::string determinism_policy{};
};

struct contract_component_stats_t {
  std::size_t component_count_total{0};
  std::size_t component_count_hashimyei{0};
  std::size_t component_count_non_hashimyei{0};
  std::vector<std::string> canonical_paths{};
  std::map<std::string, std::size_t> component_count_by_tsi_type{};
};

__attribute__((constructor(101))) void redirect_stdout_logs_to_stderr() {
  if (g_protocol_stdout_fd >= 0) return;
  std::fflush(stdout);
  g_protocol_stdout_fd = ::dup(STDOUT_FILENO);
  if (g_protocol_stdout_fd < 0) return;
  (void)::dup2(STDERR_FILENO, STDOUT_FILENO);
}

__attribute__((constructor(102))) void disable_terminal_logs_pre_main() {
  cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);
}

void print_cli_help(const char* argv0) {
  std::cerr << "Usage: " << argv0
            << " [--global-config <path>] [--help]\n"
            << "One-shot tool call: [--tool <name>] [--args-json <json-object>]\n"
            << "Overrides (optional): [--hero-config <path>]"
            << " [--store-root <path>] [--catalog <path>]"
            << " [--hashimyei-catalog <path>] [--config-folder <path>]\n"
            << "(without --tool, server mode expects JSON-RPC input on stdin)\n";
}

[[nodiscard]] std::uint64_t now_ms_utc() {
  const auto now = std::chrono::system_clock::now();
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch())
          .count());
}

[[nodiscard]] std::string trim_ascii(std::string_view in) {
  std::size_t b = 0;
  while (b < in.size() && std::isspace(static_cast<unsigned char>(in[b])) != 0) {
    ++b;
  }
  std::size_t e = in.size();
  while (e > b && std::isspace(static_cast<unsigned char>(in[e - 1])) != 0) {
    --e;
  }
  return std::string(in.substr(b, e - b));
}

[[nodiscard]] std::string lowercase_copy(std::string_view in) {
  std::string out(in);
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return out;
}

[[nodiscard]] bool starts_with(std::string_view text,
                               std::string_view prefix) {
  return text.size() >= prefix.size() &&
         text.substr(0, prefix.size()) == prefix;
}

[[nodiscard]] bool is_hashimyei_hex_token(std::string_view token) {
  if (token.size() < 3) return false;
  if (token[0] != '0' || token[1] != 'x') return false;
  for (std::size_t i = 2; i < token.size(); ++i) {
    const unsigned char c = static_cast<unsigned char>(token[i]);
    const bool hex = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    if (!hex) return false;
  }
  return true;
}

[[nodiscard]] std::string normalize_source_hashimyei_cursor(
    std::string_view canonical_path) {
  const std::string cp = trim_ascii(canonical_path);
  if (!starts_with(cp, "tsi.source.")) return cp;
  std::vector<std::string> parts{};
  std::size_t begin = 0;
  while (begin <= cp.size()) {
    const std::size_t dot = cp.find('.', begin);
    if (dot == std::string::npos) {
      parts.push_back(cp.substr(begin));
      break;
    }
    parts.push_back(cp.substr(begin, dot - begin));
    begin = dot + 1;
  }
  if (parts.size() <= 3) return cp;
  const std::string& tail = parts.back();
  if (is_hashimyei_hex_token(tail)) return cp;
  return parts[0] + "." + parts[1] + "." + parts[2];
}

[[nodiscard]] std::string strip_inline_comment(std::string_view in) {
  std::string out;
  out.reserve(in.size());
  bool in_single = false;
  bool in_double = false;
  for (const char c : in) {
    if (c == '\'' && !in_double) {
      in_single = !in_single;
      out.push_back(c);
      continue;
    }
    if (c == '"' && !in_single) {
      in_double = !in_double;
      out.push_back(c);
      continue;
    }
    if (!in_single && !in_double && (c == '#' || c == ';')) break;
    out.push_back(c);
  }
  return out;
}

[[nodiscard]] std::string unquote_if_wrapped(std::string s) {
  s = trim_ascii(std::move(s));
  if (s.size() >= 2) {
    const char a = s.front();
    const char b = s.back();
    if ((a == '"' && b == '"') || (a == '\'' && b == '\'')) {
      return s.substr(1, s.size() - 2);
    }
  }
  return s;
}

[[nodiscard]] std::optional<std::string> read_ini_value(
    const std::filesystem::path& ini_path, const std::string& section,
    const std::string& key) {
  std::ifstream in(ini_path);
  if (!in) return std::nullopt;
  std::string line;
  std::string current_section;
  while (std::getline(in, line)) {
    const std::string trimmed = trim_ascii(strip_inline_comment(line));
    if (trimmed.empty()) continue;
    if (trimmed.size() >= 2 && trimmed.front() == '[' && trimmed.back() == ']') {
      current_section = trim_ascii(trimmed.substr(1, trimmed.size() - 2));
      continue;
    }
    if (current_section != section) continue;
    const std::size_t eq = trimmed.find('=');
    if (eq == std::string::npos) continue;
    const std::string lhs = trim_ascii(trimmed.substr(0, eq));
    if (lhs != key) continue;
    return trim_ascii(trimmed.substr(eq + 1));
  }
  return std::nullopt;
}

[[nodiscard]] bool parse_bool_token(std::string_view raw, bool* out) {
  if (!out) return false;
  const std::string lowered = lowercase_copy(trim_ascii(raw));
  if (lowered == "true" || lowered == "1" || lowered == "yes" ||
      lowered == "y" || lowered == "on") {
    *out = true;
    return true;
  }
  if (lowered == "false" || lowered == "0" || lowered == "no" ||
      lowered == "n" || lowered == "off") {
    *out = false;
    return true;
  }
  return false;
}

[[nodiscard]] std::string hex_lower(const unsigned char* bytes, std::size_t n) {
  static constexpr std::array<char, 16> kHex{
      '0', '1', '2', '3', '4', '5', '6', '7',
      '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  std::string out;
  out.resize(n * 2);
  for (std::size_t i = 0; i < n; ++i) {
    const unsigned char b = bytes[i];
    out[2 * i + 0] = kHex[(b >> 4) & 0x0F];
    out[2 * i + 1] = kHex[b & 0x0F];
  }
  return out;
}

[[nodiscard]] bool sha256_hex_bytes(std::string_view payload, std::string* out_hex,
                                    std::string* error = nullptr) {
  if (error) error->clear();
  if (!out_hex) {
    if (error) *error = "sha256 output pointer is null";
    return false;
  }
  out_hex->clear();
  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  if (!ctx) {
    if (error) *error = "cannot allocate EVP context";
    return false;
  }

  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int digest_len = 0;
  const bool ok = EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) == 1 &&
                  EVP_DigestUpdate(ctx, payload.data(), payload.size()) == 1 &&
                  EVP_DigestFinal_ex(ctx, digest, &digest_len) == 1;
  EVP_MD_CTX_free(ctx);
  if (!ok) {
    if (error) *error = "EVP sha256 failed";
    return false;
  }
  *out_hex = hex_lower(digest, digest_len);
  return true;
}

[[nodiscard]] bool read_text_file(const std::filesystem::path& path,
                                  std::string* out,
                                  std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "output pointer is null";
    return false;
  }
  out->clear();

  std::ifstream in(path, std::ios::binary);
  if (!in) {
    if (error) *error = "cannot open file: " + path.string();
    return false;
  }
  out->assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
  if (!in.eof() && in.fail()) {
    if (error) *error = "cannot read file: " + path.string();
    return false;
  }
  return true;
}

[[nodiscard]] std::string resolve_path_from_folder(std::string folder,
                                                   std::string path) {
  folder = trim_ascii(folder);
  path = trim_ascii(path);
  if (path.empty()) return {};
  const std::filesystem::path p(path);
  if (p.is_absolute()) return p.string();
  if (folder.empty()) return p.string();
  return (std::filesystem::path(folder) / p).string();
}

[[nodiscard]] bool decode_wave_observation_spec(
    const std::shared_ptr<const cuwacunu::iitepi::wave_record_t>& wave_itself,
    const cuwacunu::camahjucunu::iitepi_wave_t& wave,
    cuwacunu::camahjucunu::observation_spec_t* out,
    std::string* error) {
  if (error) error->clear();
  if (!wave_itself || !out) {
    if (error) *error = "missing wave/observation output while decoding observation payload";
    return false;
  }
  if (wave.sources.size() != 1) {
    if (error) *error = "selected wave must contain exactly one SOURCE block";
    return false;
  }

  const auto& source_decl = wave.sources.front();
  const std::string sources_path = resolve_path_from_folder(
      wave_itself->config_folder, source_decl.sources_dsl_file);
  const std::string channels_path = resolve_path_from_folder(
      wave_itself->config_folder, source_decl.channels_dsl_file);
  if (sources_path.empty() || channels_path.empty()) {
    if (error) *error = "cannot resolve SOURCE DSL file paths";
    return false;
  }
  if (!std::filesystem::exists(sources_path) ||
      !std::filesystem::is_regular_file(sources_path)) {
    if (error) *error = "invalid SOURCE.SOURCES_DSL_FILE path: " + sources_path;
    return false;
  }
  if (!std::filesystem::exists(channels_path) ||
      !std::filesystem::is_regular_file(channels_path)) {
    if (error) *error = "invalid SOURCE.CHANNELS_DSL_FILE path: " + channels_path;
    return false;
  }

  const std::string source_grammar_path = resolve_path_from_folder(
      cuwacunu::iitepi::config_space_t::config_folder,
      cuwacunu::iitepi::config_space_t::get<std::string>(
          "BNF", "observation_sources_grammar_filename"));
  const std::string channel_grammar_path = resolve_path_from_folder(
      cuwacunu::iitepi::config_space_t::config_folder,
      cuwacunu::iitepi::config_space_t::get<std::string>(
          "BNF", "observation_channels_grammar_filename"));
  if (source_grammar_path.empty() || channel_grammar_path.empty()) {
    if (error) *error = "cannot resolve observation grammar file paths";
    return false;
  }
  if (!std::filesystem::exists(source_grammar_path) ||
      !std::filesystem::is_regular_file(source_grammar_path)) {
    if (error) {
      *error = "invalid [BNF].observation_sources_grammar_filename path: " +
               source_grammar_path;
    }
    return false;
  }
  if (!std::filesystem::exists(channel_grammar_path) ||
      !std::filesystem::is_regular_file(channel_grammar_path)) {
    if (error) {
      *error = "invalid [BNF].observation_channels_grammar_filename path: " +
               channel_grammar_path;
    }
    return false;
  }

  const std::string source_instruction =
      cuwacunu::piaabo::dfiles::readFileToString(sources_path);
  const std::string channel_instruction =
      cuwacunu::piaabo::dfiles::readFileToString(channels_path);
  const std::string source_grammar =
      cuwacunu::piaabo::dfiles::readFileToString(source_grammar_path);
  const std::string channel_grammar =
      cuwacunu::piaabo::dfiles::readFileToString(channel_grammar_path);
  if (trim_ascii(source_instruction).empty() ||
      trim_ascii(channel_instruction).empty()) {
    if (error) *error = "selected wave observation DSL payload is empty";
    return false;
  }
  if (trim_ascii(source_grammar).empty() || trim_ascii(channel_grammar).empty()) {
    if (error) *error = "observation grammar payload is empty";
    return false;
  }

  try {
    *out = cuwacunu::camahjucunu::decode_observation_spec_from_split_dsl(
        source_grammar,
        source_instruction,
        channel_grammar,
        channel_instruction);
    return true;
  } catch (const std::exception& e) {
    if (error) {
      *error = std::string("cannot decode selected wave observation DSL: ") +
               e.what();
    }
    return false;
  }
}

template <typename Datatype_t>
[[nodiscard]] bool resolve_effective_domain_bounds_for_datatype(
    std::string_view symbol,
    const cuwacunu::camahjucunu::observation_spec_t& observation,
    double* out_domain_from_ms,
    double* out_domain_to_ms,
    std::string* error) {
  if (error) error->clear();
  if (!out_domain_from_ms || !out_domain_to_ms) {
    if (error) *error = "domain output pointers are null";
    return false;
  }
  *out_domain_from_ms = 0.0;
  *out_domain_to_ms = 0.0;

  try {
    auto dataset =
        cuwacunu::camahjucunu::data::create_memory_mapped_concat_dataset<
            Datatype_t>(std::string(symbol), observation,
                        /*force_rebuild_cache=*/false);
    const double domain_left =
        static_cast<double>(dataset.leftmost_key_value_);
    const double domain_right =
        static_cast<double>(dataset.rightmost_key_value_);
    const double step = static_cast<double>(dataset.key_value_step_);
    if (!std::isfinite(domain_left) || !std::isfinite(domain_right) ||
        !std::isfinite(step)) {
      if (error) *error = "effective source bounds are non-finite";
      return false;
    }
    if (!(step > 0.0)) {
      if (error) *error = "effective source key step must be > 0";
      return false;
    }
    *out_domain_from_ms = domain_left;
    *out_domain_to_ms = domain_right + step;  // Half-open [from,to).
    return true;
  } catch (const std::exception& e) {
    if (error) {
      *error =
          std::string("failed to resolve effective source bounds: ") + e.what();
    }
    return false;
  } catch (...) {
    if (error) *error = "failed to resolve effective source bounds";
    return false;
  }
}

[[nodiscard]] bool resolve_effective_domain_bounds_for_record_type(
    std::string_view record_type,
    std::string_view symbol,
    const cuwacunu::camahjucunu::observation_spec_t& observation,
    double* out_domain_from_ms,
    double* out_domain_to_ms,
    std::string* error) {
  if (error) error->clear();
  const std::string rt = lowercase_copy(trim_ascii(record_type));
  if (rt == "kline") {
    return resolve_effective_domain_bounds_for_datatype<
        cuwacunu::camahjucunu::exchange::kline_t>(
        symbol, observation, out_domain_from_ms, out_domain_to_ms, error);
  }
  if (rt == "trade") {
    return resolve_effective_domain_bounds_for_datatype<
        cuwacunu::camahjucunu::exchange::trade_t>(
        symbol, observation, out_domain_from_ms, out_domain_to_ms, error);
  }
  if (rt == "basic") {
    return resolve_effective_domain_bounds_for_datatype<
        cuwacunu::camahjucunu::exchange::basic_t>(
        symbol, observation, out_domain_from_ms, out_domain_to_ms, error);
  }
  if (error) {
    *error = "unsupported record_type for source-runtime projection: " + rt;
  }
  return false;
}

[[nodiscard]] bool parse_bool_ascii_token(std::string_view token, bool* out) {
  if (!out) return false;
  const std::string lowered = lowercase_copy(trim_ascii(token));
  if (lowered == "true" || lowered == "1" || lowered == "yes" ||
      lowered == "on") {
    *out = true;
    return true;
  }
  if (lowered == "false" || lowered == "0" || lowered == "no" ||
      lowered == "off") {
    *out = false;
    return true;
  }
  return false;
}

[[nodiscard]] bool collect_channel_states_for_record_type(
    const cuwacunu::camahjucunu::observation_spec_t& observation,
    std::string_view record_type,
    std::vector<cuwacunu::hero::wave::source_runtime_channel_state_t>* out,
    std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "channel state output pointer is null";
    return false;
  }
  out->clear();

  const std::string target_record_type = lowercase_copy(trim_ascii(record_type));
  for (const auto& ch : observation.channel_forms) {
    const std::string row_record_type =
        lowercase_copy(trim_ascii(ch.record_type));
    if (row_record_type != target_record_type) continue;

    bool active = false;
    if (!parse_bool_ascii_token(ch.active, &active)) {
      if (error) {
        *error = "invalid channel active token for interval '" +
                 cuwacunu::camahjucunu::exchange::enum_to_string(ch.interval) +
                 "'";
      }
      return false;
    }

    out->push_back(cuwacunu::hero::wave::source_runtime_channel_state_t{
        .interval =
            cuwacunu::camahjucunu::exchange::enum_to_string(ch.interval),
        .active = active,
    });
  }

  if (out->empty()) {
    if (error) {
      *error =
          "no channel rows found for record_type '" + target_record_type + "'";
    }
    return false;
  }
  return true;
}

[[nodiscard]] bool build_source_runtime_projection_fragment_for_binding(
    const binding_resolution_t& resolved,
    const cuwacunu::camahjucunu::iitepi_wave_t& wave,
    const cuwacunu::camahjucunu::observation_spec_t& observation,
    cuwacunu::hero::wave::source_runtime_projection_fragment_t* out,
    std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "source projection fragment output pointer is null";
    return false;
  }
  *out = cuwacunu::hero::wave::source_runtime_projection_fragment_t{};

  if (wave.sources.size() != 1U) {
    if (error) {
      *error = "runtime requires exactly one SOURCE block for projection";
    }
    return false;
  }
  const auto& source_decl = wave.sources.front();
  const std::string symbol = trim_ascii(source_decl.symbol);
  if (symbol.empty()) {
    if (error) *error = "SOURCE.RUNTIME.SYMBOL is empty";
    return false;
  }

  double domain_from_ms = 0.0;
  double domain_to_ms = 0.0;
  if (!resolve_effective_domain_bounds_for_record_type(
          resolved.record_type, symbol, observation, &domain_from_ms,
          &domain_to_ms, error)) {
    return false;
  }

  std::vector<cuwacunu::hero::wave::source_runtime_channel_state_t>
      channel_states{};
  if (!collect_channel_states_for_record_type(
          observation, resolved.record_type, &channel_states, error)) {
    return false;
  }

  cuwacunu::hero::wave::source_runtime_projection_input_t input{};
  input.symbol = symbol;
  input.from_date_ddmmyyyy = source_decl.from;
  input.to_date_ddmmyyyy = source_decl.to;
  input.domain_from_ms = domain_from_ms;
  input.domain_to_ms = domain_to_ms;
  input.channel_states = std::move(channel_states);

  if (!cuwacunu::hero::wave::build_source_runtime_projection_fragment(
          input, out, error)) {
    return false;
  }
  return true;
}

[[nodiscard]] std::string json_escape(std::string_view in) {
  std::ostringstream out;
  for (const unsigned char c : in) {
    switch (c) {
      case '"':
        out << "\\\"";
        break;
      case '\\':
        out << "\\\\";
        break;
      case '\b':
        out << "\\b";
        break;
      case '\f':
        out << "\\f";
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
          const char* hex = "0123456789abcdef";
          out << "\\u00";
          out << hex[(c >> 4) & 0x0F] << hex[c & 0x0F];
        } else {
          out << static_cast<char>(c);
        }
        break;
    }
  }
  return out.str();
}

[[nodiscard]] std::string json_quote(std::string_view in) {
  return "\"" + json_escape(in) + "\"";
}

[[nodiscard]] bool parse_json_string_at(const std::string& json, std::size_t pos,
                                        std::string* out,
                                        std::size_t* end_pos) {
  if (pos >= json.size() || json[pos] != '"') return false;
  ++pos;
  std::string value;
  while (pos < json.size()) {
    const char c = json[pos++];
    if (c == '"') {
      if (out) *out = value;
      if (end_pos) *end_pos = pos;
      return true;
    }
    if (c != '\\') {
      value.push_back(c);
      continue;
    }
    if (pos >= json.size()) return false;
    const char esc = json[pos++];
    switch (esc) {
      case '"':
      case '\\':
      case '/':
        value.push_back(esc);
        break;
      case 'b':
        value.push_back('\b');
        break;
      case 'f':
        value.push_back('\f');
        break;
      case 'n':
        value.push_back('\n');
        break;
      case 'r':
        value.push_back('\r');
        break;
      case 't':
        value.push_back('\t');
        break;
      default:
        value.push_back(esc);
        break;
    }
  }
  return false;
}

[[nodiscard]] std::size_t skip_json_whitespace(const std::string& json,
                                               std::size_t pos) {
  while (pos < json.size() &&
         std::isspace(static_cast<unsigned char>(json[pos])) != 0) {
    ++pos;
  }
  return pos;
}

[[nodiscard]] bool find_json_value_end(const std::string& json, std::size_t pos,
                                       std::size_t* out_end_pos) {
  pos = skip_json_whitespace(json, pos);
  if (pos >= json.size()) return false;

  if (json[pos] == '"') {
    std::size_t end_pos = pos;
    if (!parse_json_string_at(json, pos, nullptr, &end_pos)) return false;
    if (out_end_pos) *out_end_pos = end_pos;
    return true;
  }

  if (json[pos] == '{' || json[pos] == '[') {
    std::vector<char> stack;
    stack.reserve(8);
    stack.push_back(json[pos] == '{' ? '}' : ']');
    bool in_string = false;
    bool escape = false;
    for (std::size_t i = pos + 1; i < json.size(); ++i) {
      const char c = json[i];
      if (in_string) {
        if (escape) {
          escape = false;
          continue;
        }
        if (c == '\\') {
          escape = true;
          continue;
        }
        if (c == '"') {
          in_string = false;
        }
        continue;
      }
      if (c == '"') {
        in_string = true;
        continue;
      }
      if (c == '{') {
        stack.push_back('}');
        continue;
      }
      if (c == '[') {
        stack.push_back(']');
        continue;
      }
      if (c == '}' || c == ']') {
        if (stack.empty() || stack.back() != c) return false;
        stack.pop_back();
        if (stack.empty()) {
          if (out_end_pos) *out_end_pos = i + 1;
          return true;
        }
      }
    }
    return false;
  }

  std::size_t end = pos;
  while (end < json.size()) {
    const char c = json[end];
    if (c == ',' || c == '}' || c == ']' ||
        std::isspace(static_cast<unsigned char>(c)) != 0) {
      break;
    }
    ++end;
  }
  if (end <= pos) return false;
  if (out_end_pos) *out_end_pos = end;
  return true;
}

[[nodiscard]] bool extract_json_raw_field(const std::string& json,
                                          const std::string& key,
                                          std::string* out) {
  std::size_t pos = skip_json_whitespace(json, 0);
  if (pos >= json.size() || json[pos] != '{') return false;
  ++pos;

  while (true) {
    pos = skip_json_whitespace(json, pos);
    if (pos >= json.size()) return false;
    if (json[pos] == '}') return false;

    std::string current_key;
    std::size_t after_key = pos;
    if (!parse_json_string_at(json, pos, &current_key, &after_key)) return false;

    pos = skip_json_whitespace(json, after_key);
    if (pos >= json.size() || json[pos] != ':') return false;
    ++pos;
    pos = skip_json_whitespace(json, pos);
    if (pos >= json.size()) return false;

    std::size_t value_end = pos;
    if (!find_json_value_end(json, pos, &value_end)) return false;

    if (current_key == key) {
      if (out) *out = json.substr(pos, value_end - pos);
      return true;
    }

    pos = skip_json_whitespace(json, value_end);
    if (pos >= json.size()) return false;
    if (json[pos] == ',') {
      ++pos;
      continue;
    }
    if (json[pos] == '}') return false;
    return false;
  }
}

[[nodiscard]] bool extract_json_object_field(const std::string& json,
                                             const std::string& key,
                                             std::string* out) {
  std::string raw;
  if (!extract_json_raw_field(json, key, &raw)) return false;
  raw = trim_ascii(raw);
  if (raw.empty() || raw.front() != '{') return false;
  if (out) *out = raw;
  return true;
}

[[nodiscard]] bool extract_json_string_field(const std::string& json,
                                             const std::string& key,
                                             std::string* out) {
  std::string raw;
  if (!extract_json_raw_field(json, key, &raw)) return false;
  std::size_t end_pos = 0;
  if (!parse_json_string_at(raw, 0, out, &end_pos)) return false;
  end_pos = skip_json_whitespace(raw, end_pos);
  return end_pos == raw.size();
}

[[nodiscard]] bool extract_json_bool_field(const std::string& json,
                                           const std::string& key, bool* out) {
  std::string raw;
  if (!extract_json_raw_field(json, key, &raw)) return false;
  const std::string lowered = lowercase_copy(trim_ascii(raw));
  if (lowered == "true") {
    if (out) *out = true;
    return true;
  }
  if (lowered == "false") {
    if (out) *out = false;
    return true;
  }
  return false;
}

[[nodiscard]] bool extract_json_size_field(const std::string& json,
                                           const std::string& key,
                                           std::size_t* out) {
  if (!out) return false;
  std::string raw;
  if (!extract_json_raw_field(json, key, &raw)) return false;
  raw = trim_ascii(raw);
  if (raw.empty()) return false;
  std::uint64_t parsed = 0;
  const auto [ptr, ec] =
      std::from_chars(raw.data(), raw.data() + raw.size(), parsed);
  if (ec != std::errc() || ptr != raw.data() + raw.size()) return false;
  *out = static_cast<std::size_t>(parsed);
  return true;
}

[[nodiscard]] bool extract_json_number_field(const std::string& json,
                                             const std::string& key,
                                             double* out) {
  if (!out) return false;
  std::string raw;
  if (!extract_json_raw_field(json, key, &raw)) return false;
  raw = trim_ascii(raw);
  if (raw.empty()) return false;
  std::size_t pos = 0;
  double value = 0.0;
  try {
    value = std::stod(raw, &pos);
  } catch (...) {
    return false;
  }
  if (pos != raw.size()) return false;
  *out = value;
  return true;
}

[[nodiscard]] bool parse_u64_token(std::string_view text, std::uint64_t* out) {
  if (!out) return false;
  std::string token = trim_ascii(text);
  if (token.empty()) return false;
  const int base = (token.size() > 2 && token[0] == '0' &&
                    (token[1] == 'x' || token[1] == 'X'))
                       ? 16
                       : 10;
  errno = 0;
  char* end = nullptr;
  const unsigned long long parsed = std::strtoull(token.c_str(), &end, base);
  if (errno != 0 || end == nullptr || *end != '\0') return false;
  *out = static_cast<std::uint64_t>(parsed);
  return true;
}

[[nodiscard]] bool extract_json_u64_field(const std::string& json,
                                          const std::string& key,
                                          std::uint64_t* out) {
  if (!out) return false;
  std::string as_text;
  if (extract_json_string_field(json, key, &as_text)) {
    return parse_u64_token(as_text, out);
  }
  double as_number = 0.0;
  if (!extract_json_number_field(json, key, &as_number)) return false;
  if (!std::isfinite(as_number) || as_number < 0.0) return false;
  const double rounded = std::floor(as_number);
  if (rounded != as_number) return false;
  if (rounded > static_cast<double>(std::numeric_limits<std::uint64_t>::max())) {
    return false;
  }
  *out = static_cast<std::uint64_t>(rounded);
  return true;
}

enum class wave_cursor_resolution_e : std::uint8_t {
  Run = 0,
  Episode = 1,
  Batch = 2,
};

[[nodiscard]] std::uint64_t wave_cursor_mask_for_resolution(
    wave_cursor_resolution_e resolution) {
  const auto layout =
      cuwacunu::hero::wave::lattice_catalog_store_t::runtime_wave_cursor_layout();
  db::wave_cursor::masked_query_t q{};
  const db::wave_cursor::parts_t zero{};
  std::uint8_t field_mask = db::wave_cursor::field_run;
  if (resolution == wave_cursor_resolution_e::Episode ||
      resolution == wave_cursor_resolution_e::Batch) {
    field_mask = static_cast<std::uint8_t>(field_mask |
                                           db::wave_cursor::field_episode);
  }
  if (resolution == wave_cursor_resolution_e::Batch) {
    field_mask =
        static_cast<std::uint8_t>(field_mask | db::wave_cursor::field_batch);
  }
  (void)db::wave_cursor::build_masked_query(layout, zero, field_mask, &q);
  return q.mask;
}

[[nodiscard]] bool parse_wave_cursor_resolution(
    std::string_view token,
    wave_cursor_resolution_e* out_resolution) {
  if (!out_resolution) return false;
  const std::string lowered = lowercase_copy(trim_ascii(token));
  if (lowered == "run") {
    *out_resolution = wave_cursor_resolution_e::Run;
    return true;
  }
  if (lowered == "episode") {
    *out_resolution = wave_cursor_resolution_e::Episode;
    return true;
  }
  if (lowered == "batch") {
    *out_resolution = wave_cursor_resolution_e::Batch;
    return true;
  }
  return false;
}

[[nodiscard]] const char* wave_cursor_resolution_to_text(
    wave_cursor_resolution_e resolution) {
  switch (resolution) {
    case wave_cursor_resolution_e::Run:
      return "run";
    case wave_cursor_resolution_e::Episode:
      return "episode";
    case wave_cursor_resolution_e::Batch:
      return "batch";
    default:
      return "run";
  }
}

struct intersection_cursor_filter_t {
  std::string hashimyei_cursor{};
  std::uint64_t wave_cursor{0};
};

[[nodiscard]] bool parse_intersection_cursor(
    std::string_view token,
    intersection_cursor_filter_t* out,
    std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "intersection cursor output pointer is null";
    return false;
  }
  *out = intersection_cursor_filter_t{};
  const std::string value = trim_ascii(token);
  if (value.empty()) {
    if (error) *error = "intersection_cursor is empty";
    return false;
  }
  const std::size_t sep1 = value.find('|');
  if (sep1 == std::string::npos) {
    if (error) {
      *error = "intersection_cursor must be hashimyei_cursor|wave_cursor";
    }
    return false;
  }
  if (value.find('|', sep1 + 1) != std::string::npos) {
    if (error) *error = "intersection_cursor has too many separators";
    return false;
  }
  const std::string hashimyei_cursor = trim_ascii(value.substr(0, sep1));
  const std::string wave_cursor_text = trim_ascii(value.substr(sep1 + 1));

  if (hashimyei_cursor.empty()) {
    if (error) *error = "intersection_cursor is missing hashimyei_cursor";
    return false;
  }
  if (wave_cursor_text.empty()) {
    if (error) *error = "intersection_cursor is missing wave_cursor";
    return false;
  }
  if (wave_cursor_text.find('=') != std::string::npos ||
      wave_cursor_text.find('(') != std::string::npos ||
      wave_cursor_text.find(')') != std::string::npos) {
    if (error) {
      *error =
          "intersection_cursor does not encode resolution; use hashimyei_cursor|wave_cursor";
    }
    return false;
  }

  std::uint64_t parsed_wave_cursor = 0;
  if (!parse_u64_token(wave_cursor_text, &parsed_wave_cursor)) {
    if (error) *error = "intersection_cursor wave_cursor is not a valid integer";
    return false;
  }

  out->hashimyei_cursor = normalize_source_hashimyei_cursor(hashimyei_cursor);
  out->wave_cursor = parsed_wave_cursor;
  return true;
}

[[nodiscard]] bool extract_payload_kv(std::string_view payload,
                                      std::string_view key,
                                      std::string* out) {
  if (!out) return false;
  out->clear();
  std::size_t cursor = 0;
  while (cursor < payload.size()) {
    std::size_t line_end = payload.find('\n', cursor);
    if (line_end == std::string_view::npos) line_end = payload.size();
    std::string_view line = payload.substr(cursor, line_end - cursor);
    if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
    const std::size_t eq = line.find('=');
    if (eq != std::string_view::npos && eq > 0) {
      const std::string lhs =
          cuwacunu::camahjucunu::dsl::extract_latent_lineage_state_lhs_key(
              trim_ascii(line.substr(0, eq)));
      if (lhs == key) {
        *out = trim_ascii(line.substr(eq + 1));
        return true;
      }
    }
    if (line_end == payload.size()) break;
    cursor = line_end + 1;
  }
  return false;
}

[[nodiscard]] bool matches_wave_cursor_filter(
    std::uint64_t wave_cursor,
    std::uint64_t expected,
    std::uint64_t mask,
    wave_cursor_resolution_e cursor_resolution) {
  const std::uint64_t comparable_mask =
      mask & wave_cursor_mask_for_resolution(cursor_resolution);
  if (mask != 0 && comparable_mask == 0) return false;
  db::wave_cursor::masked_query_t q{};
  q.value = expected & comparable_mask;
  q.mask = comparable_mask;
  return db::wave_cursor::match_masked(wave_cursor, q);
}

[[nodiscard]] bool extract_wave_cursor_from_payload(
    std::string_view payload,
    std::uint64_t* out_wave_cursor) {
  if (!out_wave_cursor) return false;
  *out_wave_cursor = 0;
  std::size_t cursor = 0;
  while (cursor < payload.size()) {
    std::size_t line_end = payload.find('\n', cursor);
    if (line_end == std::string_view::npos) line_end = payload.size();
    std::string_view line = payload.substr(cursor, line_end - cursor);
    if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
    const std::size_t eq = line.find('=');
    if (eq != std::string_view::npos && eq > 0) {
      const std::string key =
          cuwacunu::camahjucunu::dsl::extract_latent_lineage_state_lhs_key(
              trim_ascii(line.substr(0, eq)));
      if (key == "wave_cursor") {
        return parse_u64_token(line.substr(eq + 1), out_wave_cursor);
      }
    }
    if (line_end == payload.size()) break;
    cursor = line_end + 1;
  }
  return false;
}

[[nodiscard]] bool extract_wave_cursor_resolution_from_payload(
    std::string_view payload,
    wave_cursor_resolution_e* out_resolution) {
  if (!out_resolution) return false;
  *out_resolution = wave_cursor_resolution_e::Run;
  bool has_episode_keys = false;
  bool has_batch_keys = false;
  std::size_t cursor = 0;
  while (cursor < payload.size()) {
    std::size_t line_end = payload.find('\n', cursor);
    if (line_end == std::string_view::npos) line_end = payload.size();
    std::string_view line = payload.substr(cursor, line_end - cursor);
    if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
    const std::size_t eq = line.find('=');
    if (eq != std::string_view::npos && eq > 0) {
      const std::string key =
          cuwacunu::camahjucunu::dsl::extract_latent_lineage_state_lhs_key(
              trim_ascii(line.substr(0, eq)));
      const std::string value = trim_ascii(line.substr(eq + 1));
      if (key == "wave_cursor_resolution") {
        return parse_wave_cursor_resolution(value, out_resolution);
      }
      if (key == "wave.cursor.episode" || key == "episode_k" || key == "episode" ||
          key == "wave_episode" || key == "epoch_k") {
        has_episode_keys = true;
      }
      if (key == "wave.cursor.batch" || key == "batch_j" || key == "batch" ||
          key == "wave_batch") {
        has_batch_keys = true;
      }
    }
    if (line_end == payload.size()) break;
    cursor = line_end + 1;
  }
  if (has_batch_keys) {
    *out_resolution = wave_cursor_resolution_e::Batch;
  } else if (has_episode_keys) {
    *out_resolution = wave_cursor_resolution_e::Episode;
  } else {
    *out_resolution = wave_cursor_resolution_e::Run;
  }
  return true;
}

[[nodiscard]] bool extract_json_id_field(const std::string& json,
                                         std::string* out_id_json) {
  std::string raw_id;
  if (!extract_json_raw_field(json, "id", &raw_id)) {
    if (out_id_json) *out_id_json = "null";
    return true;
  }

  raw_id = trim_ascii(raw_id);
  if (raw_id.empty()) return false;
  if (raw_id.front() == '"') {
    std::string parsed;
    std::size_t end_pos = 0;
    if (!parse_json_string_at(raw_id, 0, &parsed, &end_pos)) return false;
    end_pos = skip_json_whitespace(raw_id, end_pos);
    if (end_pos != raw_id.size()) return false;
    if (out_id_json) *out_id_json = json_quote(parsed);
    return true;
  }

  if (out_id_json) *out_id_json = raw_id;
  return true;
}

[[nodiscard]] bool parse_json_object_string_pairs(
    const std::string& object_json,
    std::vector<std::pair<std::string, std::string>>* out) {
  if (!out) return false;
  out->clear();

  std::size_t pos = skip_json_whitespace(object_json, 0);
  if (pos >= object_json.size() || object_json[pos] != '{') return false;
  ++pos;

  while (true) {
    pos = skip_json_whitespace(object_json, pos);
    if (pos >= object_json.size()) return false;
    if (object_json[pos] == '}') return true;

    std::string key;
    std::size_t after_key = pos;
    if (!parse_json_string_at(object_json, pos, &key, &after_key)) return false;
    pos = skip_json_whitespace(object_json, after_key);
    if (pos >= object_json.size() || object_json[pos] != ':') return false;
    ++pos;
    pos = skip_json_whitespace(object_json, pos);
    if (pos >= object_json.size() || object_json[pos] != '"') return false;

    std::string value;
    std::size_t after_value = pos;
    if (!parse_json_string_at(object_json, pos, &value, &after_value)) return false;
    out->emplace_back(std::move(key), std::move(value));

    pos = skip_json_whitespace(object_json, after_value);
    if (pos >= object_json.size()) return false;
    if (object_json[pos] == ',') {
      ++pos;
      continue;
    }
    if (object_json[pos] == '}') return true;
    return false;
  }
}

[[nodiscard]] bool parse_json_object_number_pairs(
    const std::string& object_json,
    std::vector<std::pair<std::string, double>>* out) {
  if (!out) return false;
  out->clear();

  std::size_t pos = skip_json_whitespace(object_json, 0);
  if (pos >= object_json.size() || object_json[pos] != '{') return false;
  ++pos;

  while (true) {
    pos = skip_json_whitespace(object_json, pos);
    if (pos >= object_json.size()) return false;
    if (object_json[pos] == '}') return true;

    std::string key;
    std::size_t after_key = pos;
    if (!parse_json_string_at(object_json, pos, &key, &after_key)) return false;
    pos = skip_json_whitespace(object_json, after_key);
    if (pos >= object_json.size() || object_json[pos] != ':') return false;
    ++pos;

    std::size_t value_end = pos;
    if (!find_json_value_end(object_json, pos, &value_end)) return false;
    std::string raw_value =
        trim_ascii(std::string_view(object_json).substr(pos, value_end - pos));
    std::size_t parse_pos = 0;
    double value = 0.0;
    try {
      value = std::stod(raw_value, &parse_pos);
    } catch (...) {
      return false;
    }
    if (parse_pos != raw_value.size()) return false;
    out->emplace_back(std::move(key), value);

    pos = skip_json_whitespace(object_json, value_end);
    if (pos >= object_json.size()) return false;
    if (object_json[pos] == ',') {
      ++pos;
      continue;
    }
    if (object_json[pos] == '}') return true;
    return false;
  }
}

[[nodiscard]] bool starts_with_case_insensitive(std::string_view value,
                                                std::string_view prefix) {
  if (value.size() < prefix.size()) return false;
  for (std::size_t i = 0; i < prefix.size(); ++i) {
    const char a = static_cast<char>(
        std::tolower(static_cast<unsigned char>(value[i])));
    const char b = static_cast<char>(
        std::tolower(static_cast<unsigned char>(prefix[i])));
    if (a != b) return false;
  }
  return true;
}

[[nodiscard]] bool parse_content_length_header(std::string_view header_line,
                                               std::size_t* out_length) {
  constexpr std::string_view kPrefix = "content-length:";
  const std::string line = trim_ascii(header_line);
  if (!starts_with_case_insensitive(line, kPrefix)) return false;
  const std::string number = trim_ascii(line.substr(kPrefix.size()));
  if (number.empty()) return false;

  std::uint64_t parsed = 0;
  const auto [ptr, ec] =
      std::from_chars(number.data(), number.data() + number.size(), parsed);
  if (ec != std::errc() || ptr != number.data() + number.size()) return false;
  if (parsed > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
    return false;
  }
  if (out_length) *out_length = static_cast<std::size_t>(parsed);
  return true;
}

[[nodiscard]] bool read_next_jsonrpc_message(std::istream* in,
                                             std::string* out_payload,
                                             bool* out_used_content_length) {
  if (out_used_content_length) *out_used_content_length = false;

  std::string first_line;
  while (std::getline(*in, first_line)) {
    const std::string trimmed = trim_ascii(first_line);
    if (trimmed.empty()) continue;

    std::size_t content_length = 0;
    bool saw_header_block = false;
    if (parse_content_length_header(trimmed, &content_length)) {
      saw_header_block = true;
    } else if (trimmed.front() != '{' && trimmed.front() != '[' &&
               trimmed.find(':') != std::string::npos) {
      saw_header_block = true;
    }

    if (saw_header_block) {
      std::string header_line;
      while (std::getline(*in, header_line)) {
        const std::string header_trimmed = trim_ascii(header_line);
        if (header_trimmed.empty()) break;
        std::size_t parsed_length = 0;
        if (parse_content_length_header(header_trimmed, &parsed_length)) {
          content_length = parsed_length;
        }
      }
      if (content_length == 0) continue;
      if (content_length > kMaxJsonRpcPayloadBytes) return false;

      std::string payload(content_length, '\0');
      in->read(payload.data(), static_cast<std::streamsize>(content_length));
      if (in->gcount() != static_cast<std::streamsize>(content_length)) {
        return false;
      }
      if (out_payload) *out_payload = std::move(payload);
      if (out_used_content_length) *out_used_content_length = true;
      return true;
    }

    if (out_payload) *out_payload = trimmed;
    return true;
  }
  return false;
}

[[nodiscard]] std::filesystem::path default_store_root() {
  if (const char* env = std::getenv(kStoreRootEnv); env && env[0] != '\0') {
    return std::filesystem::path(env);
  }
  return std::filesystem::path(kDefaultStoreRoot);
}

[[nodiscard]] std::filesystem::path default_catalog_path(
    const std::filesystem::path& store_root) {
  return store_root / "catalog" / "lattice_catalog.idydb";
}

[[nodiscard]] std::filesystem::path default_hashimyei_catalog_path(
    const std::filesystem::path& store_root) {
  return store_root / "catalog" / "hashimyei_catalog.idydb";
}

[[nodiscard]] bool open_hashimyei_catalog_for_snapshot(app_context_t* app,
                                                       std::string* error) {
  if (error) error->clear();
  if (!app) {
    if (error) *error = "app context is null";
    return false;
  }
  if (app->hashimyei_catalog_ready) return true;
  if (app->hashimyei_catalog_path.empty()) {
    if (error) *error = "hashimyei catalog path is empty";
    return false;
  }

  cuwacunu::hero::hashimyei::hashimyei_catalog_store_t::options_t hash_opts{};
  hash_opts.catalog_path = app->hashimyei_catalog_path;
  hash_opts.encrypted = false;
  hash_opts.ingest_version = 2;
  std::string open_error;
  if (!app->hashimyei_catalog.open(hash_opts, &open_error)) {
    if (error) {
      *error = "cannot open hashimyei catalog at " +
               app->hashimyei_catalog_path.string() + ": " + open_error;
    }
    return false;
  }
  app->hashimyei_catalog_ready = true;
  return true;
}

void close_hashimyei_catalog_snapshot(app_context_t* app) {
  if (!app || !app->hashimyei_catalog_ready) return;
  std::string ignored;
  (void)app->hashimyei_catalog.close(&ignored);
  app->hashimyei_catalog_ready = false;
}

[[nodiscard]] std::filesystem::path resolve_lattice_hero_dsl_path(
    const std::filesystem::path& global_config_path) {
  const std::optional<std::string> configured = read_ini_value(
      global_config_path, "REAL_HERO", "lattice_hero_dsl_filename");
  if (!configured.has_value()) {
    return std::filesystem::path(kDefaultWaveHeroDslPath);
  }
  const std::string resolved = resolve_path_from_folder(
      global_config_path.parent_path().string(), *configured);
  if (resolved.empty()) {
    return std::filesystem::path(kDefaultWaveHeroDslPath);
  }
  return std::filesystem::path(resolved);
}

[[nodiscard]] bool load_wave_runtime_defaults(
    const std::filesystem::path& hero_dsl_path, wave_runtime_defaults_t* out,
    std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "missing destination for wave runtime defaults";
    return false;
  }

  std::error_code ec;
  if (!std::filesystem::exists(hero_dsl_path, ec) ||
      !std::filesystem::is_regular_file(hero_dsl_path, ec)) {
    if (error) *error = "hero lattice DSL file not found: " + hero_dsl_path.string();
    return false;
  }

  std::ifstream in(hero_dsl_path);
  if (!in) {
    if (error) *error = "cannot open hero lattice DSL file: " + hero_dsl_path.string();
    return false;
  }

  std::unordered_map<std::string, std::string> values{};
  std::string line;
  bool in_block_comment = false;
  while (std::getline(in, line)) {
    std::string candidate = line;
    if (in_block_comment) {
      const std::size_t end_block = candidate.find("*/");
      if (end_block == std::string::npos) continue;
      candidate = candidate.substr(end_block + 2);
      in_block_comment = false;
    }
    const std::size_t start_block = candidate.find("/*");
    if (start_block != std::string::npos) {
      const std::size_t end_block = candidate.find("*/", start_block + 2);
      if (end_block == std::string::npos) {
        candidate = candidate.substr(0, start_block);
        in_block_comment = true;
      } else {
        candidate.erase(start_block, end_block - start_block + 2);
      }
    }
    candidate = trim_ascii(strip_inline_comment(candidate));
    if (candidate.empty()) continue;
    const std::size_t eq = candidate.find('=');
    if (eq == std::string::npos) continue;
    std::string lhs = trim_ascii(candidate.substr(0, eq));
    const std::string value = unquote_if_wrapped(
        trim_ascii(candidate.substr(eq + 1)));
    lhs = cuwacunu::camahjucunu::dsl::extract_latent_lineage_state_lhs_key(lhs);
    if (lhs.empty()) continue;
    values[lhs] = value;
  }

  const auto it_store_root = values.find("store_root");
  if (it_store_root != values.end()) {
    out->store_root = resolve_path_from_folder(
        hero_dsl_path.parent_path().string(), it_store_root->second);
  }
  const auto it_catalog_path = values.find("catalog_path");
  if (it_catalog_path != values.end()) {
    out->catalog_path = resolve_path_from_folder(
        hero_dsl_path.parent_path().string(), it_catalog_path->second);
  }
  const auto it_hash_catalog = values.find("hashimyei_catalog_path");
  if (it_hash_catalog != values.end()) {
    out->hashimyei_catalog_path = resolve_path_from_folder(
        hero_dsl_path.parent_path().string(), it_hash_catalog->second);
  }
  const auto it_config_folder = values.find("config_folder");
  if (it_config_folder != values.end()) {
    out->config_folder = resolve_path_from_folder(
        hero_dsl_path.parent_path().string(), it_config_folder->second);
  }
  const auto it_encrypted = values.find("encrypted");
  if (it_encrypted != values.end()) {
    bool parsed = false;
    if (!parse_bool_token(it_encrypted->second, &parsed)) {
      if (error) *error = "invalid bool for key encrypted in " + hero_dsl_path.string();
      return false;
    }
    if (parsed) {
      if (error) {
        *error = "encrypted=true is not supported in " + hero_dsl_path.string() +
                 "; set encrypted=false";
      }
      return false;
    }
  }
  return true;
}

bool write_all_fd(int fd, std::string_view bytes) {
  const char* data = bytes.data();
  std::size_t remaining = bytes.size();
  while (remaining > 0) {
    const ssize_t wrote = ::write(fd, data, remaining);
    if (wrote < 0) return false;
    if (wrote == 0) return false;
    data += static_cast<std::size_t>(wrote);
    remaining -= static_cast<std::size_t>(wrote);
  }
  return true;
}

void write_jsonrpc_payload(std::string_view payload) {
  if (g_protocol_stdout_fd < 0) g_protocol_stdout_fd = STDOUT_FILENO;
  if (g_jsonrpc_use_content_length_framing) {
    std::ostringstream framed;
    framed << "Content-Length: " << payload.size() << "\r\n\r\n" << payload;
    (void)write_all_fd(g_protocol_stdout_fd, framed.str());
  } else {
    (void)write_all_fd(g_protocol_stdout_fd, std::string(payload) + "\n");
  }
}

void write_jsonrpc_result(std::string_view id_json, std::string_view result_json) {
  std::ostringstream out;
  out << "{\"jsonrpc\":\"2.0\",\"id\":" << id_json
      << ",\"result\":" << result_json << "}";
  write_jsonrpc_payload(out.str());
}

void write_jsonrpc_error(std::string_view id_json, int code,
                         std::string_view message) {
  std::ostringstream out;
  out << "{\"jsonrpc\":\"2.0\",\"id\":" << id_json << ",\"error\":{"
      << "\"code\":" << code << ",\"message\":" << json_quote(message)
      << "}}";
  write_jsonrpc_payload(out.str());
}

[[nodiscard]] std::string canonical_path_for_cell(
    const cuwacunu::hero::wave::wave_cell_t& cell) {
  return "wave.cell." + cell.coord.contract_hash + "." + cell.coord.wave_hash;
}

[[nodiscard]] std::string artifact_link_to_json(
    const cuwacunu::hero::wave::wave_artifact_link_t& link) {
  std::ostringstream out;
  out << "{"
      << "\"aggregate_schema\":" << json_quote(link.aggregate_schema) << ","
      << "\"aggregate_sha256\":" << json_quote(link.aggregate_sha256) << ","
      << "\"artifact_ids\":[";
  for (std::size_t i = 0; i < link.artifact_ids.size(); ++i) {
    if (i != 0) out << ",";
    out << json_quote(link.artifact_ids[i]);
  }
  out << "],\"numeric_summary\":[";
  for (std::size_t i = 0; i < link.numeric_summary.size(); ++i) {
    if (i != 0) out << ",";
    out << "{\"key\":" << json_quote(link.numeric_summary[i].first)
        << ",\"value\":" << link.numeric_summary[i].second << "}";
  }
  out << "],\"text_summary\":[";
  for (std::size_t i = 0; i < link.text_summary.size(); ++i) {
    if (i != 0) out << ",";
    out << "{\"key\":" << json_quote(link.text_summary[i].first)
        << ",\"value\":" << json_quote(link.text_summary[i].second) << "}";
  }
  out << "],\"joined_kv_report\":" << json_quote(link.joined_kv_report)
      << "}";
  return out.str();
}

[[nodiscard]] std::string execution_profile_to_json(
    const cuwacunu::hero::wave::wave_execution_profile_t& p) {
  std::ostringstream out;
  out << "{"
      << "\"binding_id\":" << json_quote(p.binding_id) << ","
      << "\"wave_id\":" << json_quote(p.wave_id) << ","
      << "\"device\":" << json_quote(p.device) << ","
      << "\"sampler\":" << json_quote(p.sampler) << ","
      << "\"record_type\":" << json_quote(p.record_type) << ","
      << "\"dtype\":" << json_quote(p.dtype) << ","
      << "\"seed\":" << json_quote(p.seed) << ","
      << "\"determinism_policy\":" << json_quote(p.determinism_policy)
      << "}";
  return out.str();
}

[[nodiscard]] std::string cell_to_json(
    const cuwacunu::hero::wave::wave_cell_t& cell) {
  std::ostringstream out;
  out << "{"
      << "\"cell_id\":" << json_quote(cell.cell_id) << ","
      << "\"canonical_path\":" << json_quote(canonical_path_for_cell(cell))
      << ","
      << "\"coord\":{"
      << "\"contract_hash\":" << json_quote(cell.coord.contract_hash) << ","
      << "\"wave_hash\":" << json_quote(cell.coord.wave_hash) << "},"
      << "\"execution_profile\":" << execution_profile_to_json(cell.execution_profile)
      << ","
      << "\"state\":" << json_quote(cell.state) << ","
      << "\"trial_count\":" << cell.trial_count << ","
      << "\"last_trial_id\":" << json_quote(cell.last_trial_id) << ","
      << "\"projection_version\":" << cell.projection_version << ","
      << "\"created_at_ms\":" << cell.created_at_ms << ","
      << "\"updated_at_ms\":" << cell.updated_at_ms << ","
      << "\"artifact_link\":" << artifact_link_to_json(cell.artifact_link)
      << "}";
  return out.str();
}

[[nodiscard]] std::string trial_to_json(
    const cuwacunu::hero::wave::wave_trial_t& t) {
  std::ostringstream out;
  out << "{"
      << "\"trial_id\":" << json_quote(t.trial_id) << ","
      << "\"cell_id\":" << json_quote(t.cell_id) << ","
      << "\"state_snapshot_id\":" << json_quote(t.state_snapshot_id) << ","
      << "\"started_at_ms\":" << t.started_at_ms << ","
      << "\"finished_at_ms\":" << t.finished_at_ms << ","
      << "\"ok\":" << (t.ok ? "true" : "false") << ","
      << "\"error\":" << json_quote(t.error) << ","
      << "\"total_steps\":" << t.total_steps << ","
      << "\"board_hash\":" << json_quote(t.board_hash) << ","
      << "\"run_id\":" << json_quote(t.run_id) << "}";
  return out.str();
}

[[nodiscard]] std::string hashimyei_identity_to_json(
    const cuwacunu::hashimyei::hashimyei_t& id) {
  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(id.schema) << ","
      << "\"kind\":"
      << json_quote(cuwacunu::hashimyei::hashimyei_kind_to_string(id.kind)) << ","
      << "\"name\":" << json_quote(id.name) << ","
      << "\"ordinal\":" << id.ordinal << ","
      << "\"hash_sha256_hex\":" << json_quote(id.hash_sha256_hex) << "}";
  return out.str();
}

[[nodiscard]] std::string wave_contract_binding_to_json(
    const cuwacunu::hero::hashimyei::wave_contract_binding_t& binding) {
  std::ostringstream out;
  out << "{"
      << "\"identity\":" << hashimyei_identity_to_json(binding.identity) << ","
      << "\"contract\":" << hashimyei_identity_to_json(binding.contract) << ","
      << "\"wave\":" << hashimyei_identity_to_json(binding.wave) << ","
      << "\"binding_alias\":" << json_quote(binding.binding_alias) << "}";
  return out.str();
}

[[nodiscard]] std::string run_manifest_to_json(
    const cuwacunu::hero::hashimyei::run_manifest_t& run) {
  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(run.schema) << ","
      << "\"run_id\":" << json_quote(run.run_id) << ","
      << "\"started_at_ms\":" << run.started_at_ms << ","
      << "\"board_identity\":"
      << hashimyei_identity_to_json(run.board_identity) << ","
      << "\"wave_contract_binding\":"
      << wave_contract_binding_to_json(run.wave_contract_binding) << ","
      << "\"sampler\":" << json_quote(run.sampler) << ","
      << "\"record_type\":" << json_quote(run.record_type) << ","
      << "\"seed\":" << json_quote(run.seed) << ","
      << "\"device\":" << json_quote(run.device) << ","
      << "\"dtype\":" << json_quote(run.dtype) << ","
      << "\"dependency_files\":[";
  for (std::size_t i = 0; i < run.dependency_files.size(); ++i) {
    if (i != 0) out << ",";
    out << "{\"canonical_path\":"
        << json_quote(run.dependency_files[i].canonical_path)
        << ",\"sha256\":" << json_quote(run.dependency_files[i].sha256_hex)
        << "}";
  }
  out << "],\"components\":[";
  for (std::size_t i = 0; i < run.components.size(); ++i) {
    if (i != 0) out << ",";
    out << "{\"canonical_path\":" << json_quote(run.components[i].canonical_path)
        << ",\"tsi_type\":" << json_quote(run.components[i].tsi_type)
        << ",\"hashimyei\":" << json_quote(run.components[i].hashimyei) << "}";
  }
  out << "]}";
  return out.str();
}

[[nodiscard]] std::string artifact_to_json(
    const cuwacunu::hero::hashimyei::artifact_entry_t& artifact) {
  const std::string canonical_path =
      normalize_source_hashimyei_cursor(artifact.canonical_path);
  std::string wave_cursor_text;
  if (!extract_payload_kv(artifact.payload_json, "wave_cursor", &wave_cursor_text)) {
    wave_cursor_text = "0";
  }
  std::string wave_cursor_resolution_text;
  if (!extract_payload_kv(artifact.payload_json, "wave_cursor_resolution",
                          &wave_cursor_resolution_text)) {
    wave_cursor_resolution_text = "run";
  }
  std::string hashimyei_cursor;
  if (!extract_payload_kv(artifact.payload_json, "hashimyei_cursor",
                          &hashimyei_cursor)) {
    hashimyei_cursor = canonical_path;
  }
  hashimyei_cursor = normalize_source_hashimyei_cursor(hashimyei_cursor);
  const std::string intersection_cursor =
      hashimyei_cursor + "|" + wave_cursor_text;
  std::ostringstream out;
  out << "{"
      << "\"artifact_id\":" << json_quote(artifact.artifact_id) << ","
      << "\"run_id\":" << json_quote(artifact.run_id) << ","
      << "\"wave_cursor\":" << json_quote(wave_cursor_text) << ","
      << "\"wave_cursor_resolution\":"
      << json_quote(wave_cursor_resolution_text) << ","
      << "\"canonical_path\":" << json_quote(canonical_path) << ","
      << "\"hashimyei_cursor\":" << json_quote(hashimyei_cursor) << ","
      << "\"intersection_cursor\":" << json_quote(intersection_cursor) << ","
      << "\"hashimyei\":" << json_quote(artifact.hashimyei) << ","
      << "\"schema\":" << json_quote(artifact.schema) << ","
      << "\"artifact_sha256\":" << json_quote(artifact.artifact_sha256) << ","
      << "\"path\":" << json_quote(artifact.path) << ","
      << "\"ts_ms\":" << artifact.ts_ms
      << "}";
  return out.str();
}

[[nodiscard]] std::string ensure_trailing_newline(std::string value) {
  if (!value.empty() && value.back() != '\n') value.push_back('\n');
  return value;
}

[[nodiscard]] std::string joined_kv_report(
    std::string_view canonical_path,
    const std::vector<cuwacunu::hero::hashimyei::artifact_entry_t>& artifacts) {
  std::ostringstream out;
  out << "# " << cuwacunu::hashimyei::kJoinedReportSchemaV1 << "\n";
  out << "report_schema=" << cuwacunu::hashimyei::kJoinedReportSchemaV1 << "\n";
  out << "canonical_path=" << canonical_path << "\n";
  out << "artifact_count=" << artifacts.size() << "\n";
  for (std::size_t i = 0; i < artifacts.size(); ++i) {
    const auto& a = artifacts[i];
    std::uint64_t wave_cursor = 0;
    (void)extract_wave_cursor_from_payload(a.payload_json, &wave_cursor);
    wave_cursor_resolution_e wave_cursor_resolution = wave_cursor_resolution_e::Run;
    (void)extract_wave_cursor_resolution_from_payload(
        a.payload_json, &wave_cursor_resolution);
    const std::string wave_cursor_resolution_text =
        wave_cursor_resolution_to_text(wave_cursor_resolution);
    std::string hashimyei_cursor;
    if (!extract_payload_kv(a.payload_json, "hashimyei_cursor",
                            &hashimyei_cursor)) {
      hashimyei_cursor = a.canonical_path;
    }
    hashimyei_cursor = normalize_source_hashimyei_cursor(hashimyei_cursor);
    const std::string intersection_cursor =
        hashimyei_cursor + "|" + std::to_string(wave_cursor);
    out << "\n# --- artifact[" << i << "] ---\n";
    out << "artifact_index=" << i << "\n";
    out << "artifact_id=" << a.artifact_id << "\n";
    out << "schema=" << a.schema << "\n";
    out << "run_id=" << a.run_id << "\n";
    out << "wave_cursor=" << wave_cursor << "\n";
    out << "wave_cursor_resolution=" << wave_cursor_resolution_text << "\n";
    out << "hashimyei_cursor=" << hashimyei_cursor << "\n";
    out << "intersection_cursor=" << intersection_cursor << "\n";
    out << "hashimyei=" << a.hashimyei << "\n";
    out << "ts_ms=" << a.ts_ms << "\n";
    out << "path=" << a.path << "\n";
    out << "artifact_sha256=" << a.artifact_sha256 << "\n";
    out << "payload_begin=1\n";
    out << ensure_trailing_newline(a.payload_json);
    out << "payload_end=1\n";
  }
  return out.str();
}

[[nodiscard]] bool refresh_runtime_report_catalog(app_context_t* app,
                                                  std::string* out_error) {
  if (out_error) out_error->clear();
  if (!app) {
    if (out_error) *out_error = "app context is null";
    return false;
  }
  return app->catalog.ingest_runtime_reports(app->store_root, out_error);
}

[[nodiscard]] bool reset_lattice_catalog(app_context_t* app,
                                         bool reingest_runtime,
                                         std::string* out_error) {
  if (out_error) out_error->clear();
  if (!app) {
    if (out_error) *out_error = "app context is null";
    return false;
  }
  if (app->lattice_catalog_path.empty()) {
    if (out_error) *out_error = "lattice catalog path is empty";
    return false;
  }

  if (app->catalog.opened()) {
    std::string close_error;
    if (!app->catalog.close(&close_error)) {
      if (out_error) *out_error = "catalog close failed: " + close_error;
      return false;
    }
  }

  std::error_code ec{};
  std::filesystem::remove(app->lattice_catalog_path, ec);
  if (ec && std::filesystem::exists(app->lattice_catalog_path)) {
    if (out_error) {
      *out_error = "cannot remove lattice catalog file: " +
                   app->lattice_catalog_path.string();
    }
    return false;
  }

  cuwacunu::hero::wave::lattice_catalog_store_t::options_t opts{};
  opts.catalog_path = app->lattice_catalog_path;
  opts.encrypted = false;
  opts.projection_version = 2;
  if (!app->catalog.open(opts, out_error)) return false;
  if (!reingest_runtime) return true;
  return refresh_runtime_report_catalog(app, out_error);
}

[[nodiscard]] bool reject_legacy_binding_args(const std::string& arguments_json,
                                              std::string* out_error) {
  std::string ignored;
  if (extract_json_string_field(arguments_json, "contract_hash", &ignored)) {
    if (out_error) {
      *out_error =
          "argument contract_hash is not supported in v2; use contract_hashimyei";
    }
    return false;
  }
  if (extract_json_string_field(arguments_json, "wave_hash", &ignored)) {
    if (out_error) {
      *out_error = "argument wave_hash is not supported in v2; use wave_hashimyei";
    }
    return false;
  }
  if (extract_json_string_field(arguments_json, "binding_id", &ignored)) {
    if (out_error) {
      *out_error =
          "argument binding_id is not supported in v2; use binding_hashimyei";
    }
    return false;
  }
  return true;
}

[[nodiscard]] std::string make_tool_result_json(std::string_view text,
                                                std::string_view structured_json,
                                                bool is_error) {
  std::ostringstream out;
  out << "{\"content\":[{\"type\":\"text\",\"text\":" << json_quote(text)
      << "}],\"structuredContent\":" << structured_json
      << ",\"isError\":" << (is_error ? "true" : "false") << "}";
  return out.str();
}

[[nodiscard]] std::string normalize_determinism_policy(std::string value) {
  value = lowercase_copy(trim_ascii(value));
  if (value == "deterministic" || value == "det") return "deterministic";
  if (value == "non_deterministic" || value == "nondeterministic" ||
      value == "non-deterministic" || value == "stochastic") {
    return "non_deterministic";
  }
  if (value.empty()) return "non_deterministic";
  return value;
}

[[nodiscard]] bool parse_device_or_default(const std::string& device_text,
                                           torch::Device* out,
                                           std::string* error) {
  if (!out) {
    if (error) *error = "device output pointer is null";
    return false;
  }
  if (error) error->clear();

  const std::string lower = lowercase_copy(trim_ascii(device_text));
  if (lower.empty() || lower == "cpu") {
    *out = torch::Device(torch::kCPU);
    return true;
  }
  if (lower.rfind("cuda", 0) == 0) {
    try {
      *out = torch::Device(device_text);
      return true;
    } catch (const std::exception& e) {
      if (error) *error = "invalid torch device '" + device_text + "': " + e.what();
      return false;
    }
  }
  if (error) *error = "unsupported device '" + device_text + "'";
  return false;
}

[[nodiscard]] bool parse_kv_payload(
    std::string_view payload,
    std::unordered_map<std::string, std::string>* out) {
  if (!out) return false;
  out->clear();
  std::size_t cursor = 0;
  while (cursor < payload.size()) {
    std::size_t line_end = payload.find('\n', cursor);
    if (line_end == std::string_view::npos) line_end = payload.size();
    std::string_view line = payload.substr(cursor, line_end - cursor);
    if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
    const std::string trimmed = trim_ascii(line);
    if (!trimmed.empty() && trimmed.front() != '#') {
      const std::size_t eq = trimmed.find('=');
      if (eq != std::string::npos && eq > 0) {
        const std::string key =
            cuwacunu::camahjucunu::dsl::extract_latent_lineage_state_lhs_key(
                trimmed.substr(0, eq));
        const std::string val = trim_ascii(trimmed.substr(eq + 1));
        if (!key.empty()) (*out)[key] = val;
      }
    }
    if (line_end == payload.size()) break;
    cursor = line_end + 1;
  }
  return true;
}

[[nodiscard]] bool collect_wave_sink_descriptors(
    std::string_view contract_hash,
    std::string_view wave_hash,
    std::string_view wave_id,
    std::vector<std::tuple<std::string, std::string, std::string>>* out,
    std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "sink descriptors output pointer is null";
    return false;
  }
  out->clear();

  const auto wave_itself = cuwacunu::iitepi::wave_space_t::wave_itself(
      std::string(wave_hash));
  if (!wave_itself) {
    if (error) *error = "wave record is null for hash " + std::string(wave_hash);
    return false;
  }
  const auto& wave_set = wave_itself->wave.decoded();
  const auto* wave = ::tsiemene::find_wave_by_id(wave_set, std::string(wave_id));
  if (!wave) {
    if (error) {
      *error = "wave id '" + std::string(wave_id) + "' not found in wave_hash " +
               std::string(wave_hash);
    }
    return false;
  }

  for (const auto& sink_decl : wave->sinks) {
    const std::string raw_path = trim_ascii(sink_decl.sink_path);
    if (raw_path.empty()) continue;

    const auto parsed = cuwacunu::camahjucunu::decode_canonical_path(
        raw_path, std::string(contract_hash));
    const std::string canonical_path =
        (parsed.ok && !parsed.canonical.empty()) ? parsed.canonical : raw_path;
    const std::string canonical_identity =
        (parsed.ok && !parsed.canonical_identity.empty())
            ? parsed.canonical_identity
            : canonical_path;
    const auto type_id = tsiemene::parse_tsi_type_id(canonical_identity);
    const std::string sink_type = type_id.has_value()
                                      ? std::string(tsiemene::tsi_type_token(*type_id))
                                      : canonical_identity;
    out->emplace_back(raw_path, canonical_path, sink_type);
  }

  std::sort(out->begin(), out->end(),
            [](const auto& a, const auto& b) {
              if (std::get<1>(a) != std::get<1>(b)) {
                return std::get<1>(a) < std::get<1>(b);
              }
              return std::get<2>(a) < std::get<2>(b);
            });
  return true;
}

[[nodiscard]] bool compute_state_snapshot_id(
    app_context_t* app,
    const binding_resolution_t& resolved,
    std::string* out_snapshot_id,
    bool* out_strict,
    std::string* error) {
  if (error) error->clear();
  if (!app || !out_snapshot_id || !out_strict) {
    if (error) *error = "snapshot outputs are null";
    return false;
  }
  out_snapshot_id->clear();
  *out_strict = false;

  const auto contract_itself =
      cuwacunu::iitepi::contract_space_t::contract_itself(resolved.coord.contract_hash);
  if (!contract_itself) {
    if (error) {
      *error = "contract record is null for hash " + resolved.coord.contract_hash;
    }
    return false;
  }

  std::vector<cuwacunu::iitepi::contract_component_binding_t> bindings =
      contract_itself->signature.bindings;
  std::sort(bindings.begin(), bindings.end(),
            [](const cuwacunu::iitepi::contract_component_binding_t& a,
               const cuwacunu::iitepi::contract_component_binding_t& b) {
              if (a.canonical_path != b.canonical_path) {
                return a.canonical_path < b.canonical_path;
              }
              if (a.tsi_type != b.tsi_type) return a.tsi_type < b.tsi_type;
              if (a.hashimyei != b.hashimyei) return a.hashimyei < b.hashimyei;
              if (a.tsi_dsl_path != b.tsi_dsl_path) {
                return a.tsi_dsl_path < b.tsi_dsl_path;
              }
              return a.tsi_dsl_sha256_hex < b.tsi_dsl_sha256_hex;
            });

  std::ostringstream payload;
  payload << "schema=wave.state_snapshot.v1\n";
  payload << "contract_hash=" << resolved.coord.contract_hash << "\n";
  payload << "wave_hash=" << resolved.coord.wave_hash << "\n";
  payload << "binding_id=" << resolved.binding_id << "\n";
  payload << "binding_count=" << bindings.size() << "\n";

  bool hash_catalog_attempted = false;
  bool has_hashimyei = false;
  bool strict = true;
  for (std::size_t i = 0; i < bindings.size(); ++i) {
    const auto& b = bindings[i];
    payload << "binding_" << i << "_canonical_path=" << b.canonical_path << "\n";
    payload << "binding_" << i << "_tsi_type=" << b.tsi_type << "\n";
    payload << "binding_" << i << "_hashimyei=" << b.hashimyei << "\n";
    payload << "binding_" << i << "_tsi_dsl_sha256=" << b.tsi_dsl_sha256_hex
            << "\n";
    if (b.hashimyei.empty()) continue;

    has_hashimyei = true;
    if (!hash_catalog_attempted) {
      hash_catalog_attempted = true;
      if (!open_hashimyei_catalog_for_snapshot(app, nullptr)) {
        strict = false;
      }
    }
    if (!app->hashimyei_catalog_ready) {
      strict = false;
      payload << "binding_" << i << "_state=catalog_unavailable\n";
      continue;
    }

    cuwacunu::hero::hashimyei::component_state_t component{};
    std::string resolve_error;
    if (!app->hashimyei_catalog.resolve_component(
            b.canonical_path, b.hashimyei, &component, &resolve_error)) {
      strict = false;
      payload << "binding_" << i << "_state=component_unresolved\n";
      continue;
    }

    payload << "binding_" << i << "_state=ok\n";
    payload << "binding_" << i << "_component_id=" << component.component_id << "\n";
    payload << "binding_" << i << "_component_ts_ms=" << component.ts_ms << "\n";
    payload << "binding_" << i << "_component_artifact_sha256="
            << component.artifact_sha256 << "\n";
    payload << "binding_" << i << "_component_manifest_path="
            << component.manifest_path << "\n";
    payload << "binding_" << i << "_component_dsl_sha256="
            << component.manifest.dsl_sha256_hex << "\n";
  }

  if (hash_catalog_attempted) close_hashimyei_catalog_snapshot(app);
  if (!has_hashimyei) strict = true;
  if (!sha256_hex_bytes(payload.str(), out_snapshot_id, error)) return false;
  *out_strict = strict;
  return true;
}

[[nodiscard]] bool collect_wave_sink_artifact_link(
    const binding_resolution_t& resolved,
    const cuwacunu::hero::wave::wave_execution_profile_t& profile,
    const cuwacunu::iitepi::board_binding_run_record_t& run,
    const cuwacunu::hero::wave::wave_trial_t& trial,
    std::string_view source_runtime_projection_lls,
    cuwacunu::hero::wave::wave_artifact_link_t* out,
    std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "artifact link output pointer is null";
    return false;
  }
  *out = cuwacunu::hero::wave::wave_artifact_link_t{};

  std::vector<std::tuple<std::string, std::string, std::string>> sinks{};
  if (!collect_wave_sink_descriptors(resolved.coord.contract_hash,
                                     resolved.coord.wave_hash,
                                     resolved.wave_id,
                                     &sinks,
                                     error)) {
    return false;
  }

  std::unordered_map<std::string, double> numeric{};
  std::unordered_map<std::string, std::string> text{};
  std::ostringstream joined;
  joined << "# wave.sink.joined_report.v1\n";
  joined << "report_schema=wave.sink.joined_report.v1\n";
  joined << "aggregate_schema=wave.sink.artifact_link.v1\n";
  joined << "contract_hash=" << resolved.coord.contract_hash << "\n";
  joined << "wave_hash=" << resolved.coord.wave_hash << "\n";
  joined << "binding_id=" << profile.binding_id << "\n";
  joined << "wave_id=" << profile.wave_id << "\n";
  joined << "board_hash=" << run.board_hash << "\n";
  joined << "run_id=" << trial.run_id << "\n";
  joined << "trial_id=" << trial.trial_id << "\n";
  joined << "state_snapshot_id=" << trial.state_snapshot_id << "\n";
  joined << "run_ok=" << (run.ok ? 1 : 0) << "\n";
  joined << "total_steps=" << run.total_steps << "\n";
  const std::string source_runtime_projection =
      trim_ascii(source_runtime_projection_lls);
  if (!source_runtime_projection.empty()) {
    joined << "\n# --- source.runtime.projection.v2 ---\n";
    joined << "source.runtime.projection.run_id=" << trial.run_id << "\n";
    joined << source_runtime_projection_lls;
    if (source_runtime_projection_lls.back() != '\n') joined << "\n";
  }
  joined << "sink_count=" << sinks.size() << "\n";
  joined << "error=" << run.error << "\n";

  numeric["run.ok"] = run.ok ? 1.0 : 0.0;
  numeric["run.total_steps"] = static_cast<double>(run.total_steps);
  numeric["run.duration_ms"] =
      static_cast<double>(trial.finished_at_ms - trial.started_at_ms);
  numeric["sink.count"] = static_cast<double>(sinks.size());

  text["run.error"] = run.error;
  text["run.outcome"] = run.ok ? "ok" : "error";
  text["binding_id"] = profile.binding_id;
  text["wave_id"] = profile.wave_id;
  text["source.runtime.projection.present"] =
      source_runtime_projection.empty() ? "false" : "true";

  for (std::size_t i = 0; i < sinks.size(); ++i) {
    const auto& sink = sinks[i];
    const std::string& raw_path = std::get<0>(sink);
    const std::string& canonical_path = std::get<1>(sink);
    const std::string& sink_type = std::get<2>(sink);

    const std::string sink_schema =
        (sink_type == "tsi.sink.null") ? "tsi.sink.null.report.v1"
                                       : "tsi.sink.report.v1";

    std::ostringstream sink_payload;
    sink_payload << "schema=" << sink_schema << "\n";
    sink_payload << "sink_path=" << canonical_path << "\n";
    sink_payload << "sink_type=" << sink_type << "\n";
    sink_payload << "run_id=" << trial.run_id << "\n";
    sink_payload << "run_ok=" << (run.ok ? 1 : 0) << "\n";
    sink_payload << "total_steps=" << run.total_steps << "\n";
    if (sink_type == "tsi.sink.null") {
      sink_payload << "sink_status=empty\n";
      sink_payload << "note=tsi.sink.null emits no terminal metrics\n";
    } else {
      sink_payload << "sink_status=report_not_wired\n";
      sink_payload << "note=terminal sink metrics pending implementation\n";
    }

    std::string artifact_id;
    const std::string artifact_seed =
        resolved.coord.contract_hash + "|" + resolved.coord.wave_hash + "|" +
        profile.binding_id + "|" + profile.wave_id + "|" + trial.trial_id + "|" +
        canonical_path + "|" + sink_payload.str();
    if (!sha256_hex_bytes(artifact_seed, &artifact_id, error)) return false;
    out->artifact_ids.push_back(artifact_id);

    std::ostringstream prefix;
    prefix << "sink." << std::setw(4) << std::setfill('0') << i << ".";
    const std::string key_prefix = prefix.str();

    text[key_prefix + "path"] = canonical_path;
    text[key_prefix + "raw_path"] = raw_path;
    text[key_prefix + "type"] = sink_type;
    text[key_prefix + "schema"] = sink_schema;

    std::unordered_map<std::string, std::string> kv{};
    (void)parse_kv_payload(sink_payload.str(), &kv);
    for (const auto& [k, v] : kv) {
      std::size_t parsed_chars = 0;
      double num = 0.0;
      try {
        num = std::stod(v, &parsed_chars);
      } catch (...) {
        parsed_chars = 0;
      }
      if (parsed_chars == v.size() && std::isfinite(num)) {
        numeric[key_prefix + k] = num;
      } else {
        text[key_prefix + k] = v;
      }
    }

    joined << "\n# --- sink[" << i << "] ---\n";
    joined << "sink_index=" << i << "\n";
    joined << "sink_artifact_id=" << artifact_id << "\n";
    joined << "sink_path=" << canonical_path << "\n";
    joined << "sink_raw_path=" << raw_path << "\n";
    joined << "sink_type=" << sink_type << "\n";
    joined << "sink_payload_begin=1\n";
    joined << sink_payload.str();
    joined << "sink_payload_end=1\n";
  }

  for (const auto& [k, v] : numeric) {
    out->numeric_summary.emplace_back(k, v);
  }
  for (const auto& [k, v] : text) {
    out->text_summary.emplace_back(k, v);
  }

  std::sort(out->numeric_summary.begin(), out->numeric_summary.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });
  std::sort(out->text_summary.begin(), out->text_summary.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });

  out->joined_kv_report = joined.str();
  if (!sha256_hex_bytes(out->joined_kv_report, &out->aggregate_sha256, error)) {
    return false;
  }
  out->aggregate_schema = "wave.sink.artifact_link.v1";
  return true;
}

[[nodiscard]] bool ensure_runtime_initialized(app_context_t* app,
                                              std::string* error) {
  if (error) error->clear();
  if (!app) {
    if (error) *error = "app context is null";
    return false;
  }
  if (app->runtime_initialized) return true;

  try {
    cuwacunu::iitepi::config_space_t::change_config_file(
        app->config_folder.string().c_str());
    cuwacunu::iitepi::config_space_t::update_config();
    cuwacunu::iitepi::board_space_t::init();
    cuwacunu::iitepi::board_space_t::assert_locked_runtime_intact_or_fail_fast();
    app->runtime_initialized = true;
    return true;
  } catch (const std::exception& e) {
    if (error) *error = std::string("runtime init failed: ") + e.what();
    return false;
  } catch (...) {
    if (error) *error = "runtime init failed: unknown exception";
    return false;
  }
}

[[nodiscard]] bool resolve_binding(app_context_t* app,
                                   std::string_view requested_contract_hash,
                                   std::string_view requested_wave_hash,
                                   std::string_view requested_binding_id,
                                   std::string_view requested_wave_id,
                                   binding_resolution_t* out,
                                   std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "binding resolution output is null";
    return false;
  }
  *out = binding_resolution_t{};

  if (!ensure_runtime_initialized(app, error)) return false;

  try {
    out->board_hash = cuwacunu::iitepi::board_space_t::locked_board_hash();
    const auto board = cuwacunu::iitepi::board_space_t::board_itself(out->board_hash);
    if (!board) {
      if (error) *error = "board record is null";
      return false;
    }

    const auto& instruction = board->board.decoded();

    struct candidate_t {
      std::string binding_id;
      std::string wave_id;
      std::string contract_hash;
      std::string wave_hash;
    };
    std::vector<candidate_t> candidates{};

    for (const auto& bind : instruction.binds) {
      if (!requested_binding_id.empty() && bind.id != requested_binding_id) continue;
      if (!requested_wave_id.empty() && bind.wave_ref != requested_wave_id) continue;

      const std::string contract_hash =
          cuwacunu::iitepi::board_space_t::contract_hash_for_binding(out->board_hash,
                                                                     bind.id);
      const std::string wave_hash =
          cuwacunu::iitepi::board_space_t::wave_hash_for_binding(out->board_hash,
                                                                 bind.id);

      if (!requested_contract_hash.empty() && contract_hash != requested_contract_hash) {
        continue;
      }
      if (!requested_wave_hash.empty() && wave_hash != requested_wave_hash) {
        continue;
      }

      candidates.push_back(
          candidate_t{bind.id, bind.wave_ref, contract_hash, wave_hash});
    }

    if (candidates.empty()) {
      if (error) {
        *error = "no board binding matches requested coordinate (contract_hash=" +
                 std::string(requested_contract_hash) + ", wave_hash=" +
                 std::string(requested_wave_hash) + ")";
      }
      return false;
    }
    if (candidates.size() > 1) {
      std::ostringstream ids;
      for (std::size_t i = 0; i < candidates.size(); ++i) {
        if (i != 0) ids << ",";
        ids << candidates[i].binding_id;
      }
      if (error) {
        *error = "ambiguous binding resolution, candidates=" + ids.str() +
                 " (provide binding_id or wave_id)";
      }
      return false;
    }

    const candidate_t& c = candidates.front();
    out->binding_id = c.binding_id;
    out->wave_id = c.wave_id;
    out->coord.contract_hash = c.contract_hash;
    out->coord.wave_hash = c.wave_hash;

    const auto wave_itself =
        cuwacunu::iitepi::wave_space_t::wave_itself(out->coord.wave_hash);
    if (!wave_itself) {
      if (error) *error = "wave record is null for hash " + out->coord.wave_hash;
      return false;
    }

    const auto& wave_set = wave_itself->wave.decoded();
    const auto* wave =
        ::tsiemene::find_wave_by_id(wave_set, out->wave_id);
    if (!wave) {
      if (error) {
        *error = "wave id '" + out->wave_id + "' not found in wave_hash " +
                 out->coord.wave_hash;
      }
      return false;
    }

    out->sampler = wave->sampler;
    out->determinism_policy = normalize_determinism_policy(wave->determinism_policy);

    const auto contract_itself =
        cuwacunu::iitepi::contract_space_t::contract_itself(out->coord.contract_hash);
    if (!contract_itself) {
      if (error) {
        *error = "contract record is null for hash " + out->coord.contract_hash;
      }
      return false;
    }

    std::string record_error;
    cuwacunu::camahjucunu::observation_spec_t observation{};
    if (!decode_wave_observation_spec(
            wave_itself, *wave, &observation, &record_error)) {
      if (error) {
        *error = "cannot resolve observation payload from selected wave: " +
                 record_error;
      }
      return false;
    }
    if (!::tsiemene::resolve_active_record_type_from_observation(
            observation, &out->record_type, &record_error)) {
      if (error) {
        *error = "cannot resolve record_type from observation: " + record_error;
      }
      return false;
    }
    return true;
  } catch (const std::exception& e) {
    if (error) *error = std::string("binding resolution failed: ") + e.what();
    return false;
  } catch (...) {
    if (error) *error = "binding resolution failed: unknown exception";
    return false;
  }
}

[[nodiscard]] bool collect_contract_component_stats(
    const std::shared_ptr<const cuwacunu::iitepi::contract_record_t>& contract_itself,
    std::string_view contract_hash,
    contract_component_stats_t* out,
    std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "component stats output pointer is null";
    return false;
  }
  *out = contract_component_stats_t{};
  if (!contract_itself) {
    if (error) *error = "contract record is null";
    return false;
  }

  try {
    const auto& instruction = contract_itself->circuit.decoded();
    std::unordered_map<std::string, bool> seen{};
    const bool has_active = !instruction.active_circuit_name.empty();
    for (const auto& circuit : instruction.circuits) {
      if (has_active && circuit.name != instruction.active_circuit_name) continue;
      for (const auto& instance : circuit.instances) {
        const std::string raw = trim_ascii(instance.tsi_type);
        if (raw.empty()) continue;

        const auto parsed = cuwacunu::camahjucunu::decode_canonical_path(
            raw, std::string(contract_hash));
        const std::string canonical_path =
            (parsed.ok && !parsed.canonical.empty()) ? parsed.canonical : raw;
        const std::string canonical_identity =
            (parsed.ok && !parsed.canonical_identity.empty())
                ? parsed.canonical_identity
                : canonical_path;
        const auto type_id = tsiemene::parse_tsi_type_id(canonical_identity);
        const std::string tsi_type = type_id.has_value()
                                         ? std::string(tsiemene::tsi_type_token(*type_id))
                                         : canonical_identity;

        const std::string dedupe = canonical_path + "|" + tsi_type;
        if (seen.find(dedupe) != seen.end()) continue;
        seen.emplace(dedupe, true);

        const bool has_hashimyei =
            parsed.ok && !parsed.hashimyei.empty();
        out->component_count_total += 1;
        if (has_hashimyei) {
          out->component_count_hashimyei += 1;
        } else {
          out->component_count_non_hashimyei += 1;
        }
        out->canonical_paths.push_back(canonical_path);
        out->component_count_by_tsi_type[tsi_type] += 1;
      }
    }
    std::sort(out->canonical_paths.begin(), out->canonical_paths.end());
    return true;
  } catch (const std::exception& e) {
    if (error) *error = std::string("component stats failed: ") + e.what();
    return false;
  } catch (...) {
    if (error) *error = "component stats failed: unknown exception";
    return false;
  }
}

[[nodiscard]] std::string json_string_array(
    const std::vector<std::string>& values) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) out << ",";
    out << json_quote(values[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string format_projection_double(double v) {
  std::ostringstream oss;
  oss.setf(std::ios::fixed);
  oss << std::setprecision(12) << v;
  return oss.str();
}

[[nodiscard]] std::string build_projection_lls_payload(
    const cuwacunu::hero::wave::wave_projection_t& projection) {
  std::vector<std::pair<std::string, double>> projection_num =
      projection.projection_num;
  std::vector<std::pair<std::string, std::string>> projection_txt =
      projection.projection_txt;

  std::sort(projection_num.begin(), projection_num.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });
  std::sort(projection_txt.begin(), projection_txt.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });

  std::ostringstream payload;
  payload << "# wave.projection.lls.v2\n";
  payload << "schema=wave.projection.lls.v2\n";
  payload << "projection_version=" << projection.projection_version << "\n";
  payload << "projector_build_id=" << projection.projector_build_id << "\n";

  payload << "\n# section.projection_num\n";
  for (const auto& [k, v] : projection_num) {
    payload << k << "=" << format_projection_double(v) << "\n";
  }

  payload << "\n# section.projection_txt\n";
  for (const auto& [k, v] : projection_txt) {
    payload << k << "=" << v << "\n";
  }

  return cuwacunu::camahjucunu::dsl::convert_latent_lineage_state_payload_to_lattice_state(
      payload.str());
}

[[nodiscard]] bool build_projection(
    const binding_resolution_t& resolved,
    const cuwacunu::hero::wave::wave_execution_profile_t& profile,
    cuwacunu::hero::wave::wave_projection_t* out,
    std::string* out_source_runtime_projection_lls,
    std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "projection output pointer is null";
    return false;
  }
  *out = cuwacunu::hero::wave::wave_projection_t{};
  out->projection_version = 2;
  out->projector_build_id = "wave.projector.v2";
  if (out_source_runtime_projection_lls) out_source_runtime_projection_lls->clear();

  try {
    const auto wave_itself =
        cuwacunu::iitepi::wave_space_t::wave_itself(resolved.coord.wave_hash);
    const auto contract_itself =
        cuwacunu::iitepi::contract_space_t::contract_itself(resolved.coord.contract_hash);
    if (!wave_itself || !contract_itself) {
      if (error) *error = "cannot resolve wave/contract records for projection";
      return false;
    }

    const auto& wave_set = wave_itself->wave.decoded();
    const auto* wave = ::tsiemene::find_wave_by_id(wave_set, resolved.wave_id);
    if (!wave) {
      if (error) {
        *error = "cannot find wave id '" + resolved.wave_id + "' in wave_hash " +
                 resolved.coord.wave_hash;
      }
      return false;
    }

    cuwacunu::camahjucunu::observation_spec_t observation{};
    std::string observation_error{};
    if (!decode_wave_observation_spec(
            wave_itself, *wave, &observation, &observation_error)) {
      if (error) {
        *error = "cannot resolve observation payload from selected wave: " +
                 observation_error;
      }
      return false;
    }
    cuwacunu::hero::wave::source_runtime_projection_fragment_t
        source_runtime_fragment{};
    std::string source_runtime_error{};
    if (!build_source_runtime_projection_fragment_for_binding(
            resolved, *wave, observation, &source_runtime_fragment,
            &source_runtime_error)) {
      if (error) {
        *error = "cannot build source-runtime projection fragment: " +
                 source_runtime_error;
      }
      return false;
    }
    if (out_source_runtime_projection_lls) {
      *out_source_runtime_projection_lls = source_runtime_fragment.projection_lls;
    }
    const auto& design_blob = contract_itself->vicreg_network_design;

    out->projection_num.push_back({"mode_flags", static_cast<double>(wave->mode_flags)});
    out->projection_num.push_back({"epochs", static_cast<double>(wave->epochs)});
    out->projection_num.push_back({"batch_size", static_cast<double>(wave->batch_size)});
    out->projection_num.push_back(
        {"max_batches_per_epoch", static_cast<double>(wave->max_batches_per_epoch)});
    out->projection_num.push_back(
        {"source_count", static_cast<double>(wave->sources.size())});
    out->projection_num.push_back(
        {"wikimyei_count", static_cast<double>(wave->wikimyeis.size())});
    out->projection_num.push_back(
        {"probe_count", static_cast<double>(wave->probes.size())});

    std::uint64_t workers_sum = 0;
    for (const auto& src : wave->sources) {
      workers_sum += src.workers;
    }
    out->projection_num.push_back(
        {"source_workers_sum", static_cast<double>(workers_sum)});

    contract_component_stats_t component_stats{};
    if (!collect_contract_component_stats(contract_itself,
                                          resolved.coord.contract_hash,
                                          &component_stats,
                                          error)) {
      return false;
    }
    out->projection_num.push_back(
        {"component_count_total",
         static_cast<double>(component_stats.component_count_total)});
    out->projection_num.push_back(
        {"component_count_hashimyei",
         static_cast<double>(component_stats.component_count_hashimyei)});
    out->projection_num.push_back(
        {"component_count_non_hashimyei",
         static_cast<double>(component_stats.component_count_non_hashimyei)});
    for (const auto& [tsi_type, count] : component_stats.component_count_by_tsi_type) {
      out->projection_num.push_back(
          {"component_count_by_tsi_type." + tsi_type,
           static_cast<double>(count)});
    }

    auto obs = observation;
    out->projection_num.push_back({"observation.channel_count",
                                   static_cast<double>(obs.count_channels())});
    out->projection_num.push_back({"observation.max_seq_len",
                                   static_cast<double>(obs.max_sequence_length())});
    out->projection_num.push_back(
        {"observation.max_future_seq_len",
         static_cast<double>(obs.max_future_sequence_length())});

    std::string join_policy = "none";
    double node_count = 0.0;
    double export_count = 0.0;
    if (design_blob.has_payload()) {
      const auto& design = design_blob.decoded();
      join_policy = design.join_policy;
      node_count = static_cast<double>(design.nodes.size());
      export_count = static_cast<double>(design.exports.size());
    }
    out->projection_num.push_back({"network.node_count", node_count});
    out->projection_num.push_back({"network.export_count", export_count});
    for (const auto& [k, v] : source_runtime_fragment.projection_num) {
      out->projection_num.push_back({k, v});
    }

    out->projection_txt.push_back(
        {"mode", cuwacunu::camahjucunu::canonical_iitepi_wave_mode(wave->mode_flags)});
    out->projection_txt.push_back({"sampler", wave->sampler});
    out->projection_txt.push_back({"network.join_policy", join_policy});
    for (const auto& [k, v] : source_runtime_fragment.projection_txt) {
      out->projection_txt.push_back({k, v});
    }

    out->projection_txt.push_back({"binding_id", profile.binding_id});
    out->projection_txt.push_back({"wave_id", profile.wave_id});
    out->projection_txt.push_back({"determinism_policy", profile.determinism_policy});
    out->projection_txt.push_back({"record_type", profile.record_type});
    out->projection_txt.push_back({"device", profile.device});
    out->projection_txt.push_back({"dtype", profile.dtype});
    out->projection_txt.push_back({"seed", profile.seed});
    out->projection_lls = build_projection_lls_payload(*out);
    return true;
  } catch (const std::exception& e) {
    if (error) *error = std::string("projection build failed: ") + e.what();
    return false;
  } catch (...) {
    if (error) *error = "projection build failed: unknown exception";
    return false;
  }
}

[[nodiscard]] std::string tools_list_result_json() {
  std::ostringstream out;
  out << "{\"tools\":[";
  bool first = true;
#define HERO_LATTICE_MCP_TOOL(NAME, DESCRIPTION, INPUT_SCHEMA_JSON)                 \
  do {                                                                               \
    if (!first) out << ",";                                                          \
    first = false;                                                                   \
    out << "{\"name\":" << json_quote(NAME)                                          \
        << ",\"description\":" << json_quote(DESCRIPTION)                           \
        << ",\"inputSchema\":" << INPUT_SCHEMA_JSON << "}";                         \
  } while (false)
#include "hero_lattice_tools.def"
#undef HERO_LATTICE_MCP_TOOL
  out << "]}";
  return out.str();
}

[[nodiscard]] std::string initialize_result_json() {
  std::ostringstream out;
  out << "{"
      << "\"protocolVersion\":" << json_quote(kProtocolVersion) << ","
      << "\"serverInfo\":{"
      << "\"name\":" << json_quote(kServerName) << ","
      << "\"version\":" << json_quote(kServerVersion)
      << "},"
      << "\"capabilities\":{\"tools\":{\"listChanged\":false}},"
      << "\"instructions\":" << json_quote(kInitializeInstructions)
      << "}";
  return out.str();
}

[[nodiscard]] bool handle_tool_get_runs(app_context_t* app,
                                        const std::string& arguments_json,
                                        std::string* out_structured,
                                        std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();
  if (!reject_legacy_binding_args(arguments_json, out_error)) return false;

  std::string contract_hashimyei;
  std::string wave_hashimyei;
  std::string binding_hashimyei;
  (void)extract_json_string_field(arguments_json, "contract_hashimyei",
                                  &contract_hashimyei);
  (void)extract_json_string_field(arguments_json, "wave_hashimyei",
                                  &wave_hashimyei);
  (void)extract_json_string_field(arguments_json, "binding_hashimyei",
                                  &binding_hashimyei);

  std::size_t limit = 0;
  std::size_t offset = 0;
  (void)extract_json_size_field(arguments_json, "limit", &limit);
  (void)extract_json_size_field(arguments_json, "offset", &offset);

  if (!refresh_runtime_report_catalog(app, out_error)) return false;

  std::vector<cuwacunu::hero::hashimyei::run_manifest_t> runs{};
  std::size_t total = 0;
  if (!app->catalog.list_runtime_runs_by_binding(
          contract_hashimyei, wave_hashimyei, binding_hashimyei, &runs, out_error)) {
    return false;
  }
  total = runs.size();
  const std::size_t off = std::min(offset, runs.size());
  std::size_t count = limit;
  if (count == 0) count = runs.size() - off;
  count = std::min(count, runs.size() - off);
  const auto begin = runs.begin() + static_cast<std::ptrdiff_t>(off);
  const auto end = begin + static_cast<std::ptrdiff_t>(count);
  runs.assign(begin, end);

  std::ostringstream runs_json;
  runs_json << "[";
  for (std::size_t i = 0; i < runs.size(); ++i) {
    if (i != 0) runs_json << ",";
    runs_json << run_manifest_to_json(runs[i]);
  }
  runs_json << "]";

  std::ostringstream out;
  out << "{\"canonical_path\":\"\""
      << ",\"count\":" << runs.size()
      << ",\"total\":" << total
      << ",\"contract_hashimyei\":" << json_quote(contract_hashimyei)
      << ",\"wave_hashimyei\":" << json_quote(wave_hashimyei)
      << ",\"binding_hashimyei\":" << json_quote(binding_hashimyei)
      << ",\"runs\":" << runs_json.str() << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_get_history(app_context_t* app,
                                           const std::string& arguments_json,
                                           std::string* out_structured,
                                           std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string canonical_path;
  std::string intersection_cursor_arg;
  (void)extract_json_string_field(arguments_json, "canonical_path", &canonical_path);
  if (canonical_path.empty()) {
    (void)extract_json_string_field(arguments_json, "hashimyei_cursor",
                                    &canonical_path);
  }
  (void)extract_json_string_field(arguments_json, "intersection_cursor",
                                  &intersection_cursor_arg);
  canonical_path = normalize_source_hashimyei_cursor(canonical_path);
  intersection_cursor_arg = trim_ascii(intersection_cursor_arg);

  std::string schema;
  (void)extract_json_string_field(arguments_json, "schema", &schema);
  std::uint64_t wave_cursor = 0;
  std::uint64_t wave_cursor_mask =
      cuwacunu::hero::wave::lattice_catalog_store_t::runtime_wave_cursor_full_mask();
  bool use_wave_cursor = false;
  if (!intersection_cursor_arg.empty()) {
    intersection_cursor_filter_t parsed{};
    if (!parse_intersection_cursor(intersection_cursor_arg, &parsed, out_error)) {
      return false;
    }
    canonical_path = parsed.hashimyei_cursor;
    use_wave_cursor = true;
    wave_cursor = parsed.wave_cursor;
    wave_cursor_mask =
        cuwacunu::hero::wave::lattice_catalog_store_t::runtime_wave_cursor_full_mask();
    intersection_cursor_arg = canonical_path + "|" + std::to_string(wave_cursor);
  } else if (extract_json_u64_field(arguments_json, "wave_cursor", &wave_cursor)) {
    use_wave_cursor = true;
    (void)extract_json_u64_field(arguments_json, "wave_cursor_mask",
                                 &wave_cursor_mask);
  } else {
    std::uint64_t mask_only = 0;
    if (extract_json_u64_field(arguments_json, "wave_cursor_mask", &mask_only)) {
      *out_error = "wave_cursor_mask requires wave_cursor";
      return false;
    }
  }
  if (canonical_path.empty()) {
    *out_error =
        "get_history requires arguments.canonical_path, arguments.hashimyei_cursor, or arguments.intersection_cursor";
    return false;
  }

  std::size_t limit = 20;
  std::size_t offset = 0;
  (void)extract_json_size_field(arguments_json, "limit", &limit);
  (void)extract_json_size_field(arguments_json, "offset", &offset);

  if (!refresh_runtime_report_catalog(app, out_error)) return false;

  std::vector<cuwacunu::hero::hashimyei::artifact_entry_t> rows{};
  const std::size_t query_limit = use_wave_cursor ? 0 : limit;
  const std::size_t query_offset = use_wave_cursor ? 0 : offset;
  if (!app->catalog.list_runtime_artifacts(canonical_path, schema, query_limit,
                                           query_offset, true,
                                           &rows, out_error)) {
    return false;
  }
  if (use_wave_cursor) {
    std::vector<cuwacunu::hero::hashimyei::artifact_entry_t> filtered{};
    filtered.reserve(rows.size());
    for (const auto& row : rows) {
      std::uint64_t row_wave_cursor = 0;
      if (!extract_wave_cursor_from_payload(row.payload_json, &row_wave_cursor)) {
        continue;
      }
      wave_cursor_resolution_e row_cursor_resolution =
          wave_cursor_resolution_e::Run;
      (void)extract_wave_cursor_resolution_from_payload(
          row.payload_json, &row_cursor_resolution);
      if (!matches_wave_cursor_filter(row_wave_cursor, wave_cursor,
                                      wave_cursor_mask,
                                      row_cursor_resolution)) {
        continue;
      }
      filtered.push_back(row);
    }
    rows.swap(filtered);
    const std::size_t off = std::min(offset, rows.size());
    std::size_t count = limit;
    if (count == 0) count = rows.size() - off;
    count = std::min(count, rows.size() - off);
    rows.assign(rows.begin() + static_cast<std::ptrdiff_t>(off),
                rows.begin() + static_cast<std::ptrdiff_t>(off + count));
  }

  std::ostringstream history;
  history << "[";
  for (std::size_t i = 0; i < rows.size(); ++i) {
    if (i != 0) history << ",";
    history << artifact_to_json(rows[i]);
  }
  history << "]";
  if (!rows.empty()) {
    canonical_path = normalize_source_hashimyei_cursor(rows.front().canonical_path);
  }

  std::ostringstream out;
  std::string normalized_intersection = intersection_cursor_arg;
  if (normalized_intersection.empty() && use_wave_cursor) {
    normalized_intersection =
        canonical_path + "|" + std::to_string(wave_cursor);
  }
  out << "{\"canonical_path\":" << json_quote(canonical_path)
      << ",\"hashimyei_cursor\":" << json_quote(canonical_path)
      << ",\"intersection_cursor\":"
      << (normalized_intersection.empty() ? "null"
                                          : json_quote(normalized_intersection))
      << ",\"count\":" << rows.size()
      << ",\"wave_cursor\":"
      << (use_wave_cursor ? json_quote(std::to_string(wave_cursor)) : "null")
      << ",\"wave_cursor_mask\":"
      << (use_wave_cursor ? json_quote(std::to_string(wave_cursor_mask))
                          : "null")
      << ",\"history\":" << history.str() << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_get_report_lls(app_context_t* app,
                                              const std::string& arguments_json,
                                              std::string* out_structured,
                                              std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  std::string canonical_path;
  std::string intersection_cursor_arg;
  (void)extract_json_string_field(arguments_json, "canonical_path", &canonical_path);
  if (canonical_path.empty()) {
    (void)extract_json_string_field(arguments_json, "hashimyei_cursor",
                                    &canonical_path);
  }
  (void)extract_json_string_field(arguments_json, "intersection_cursor",
                                  &intersection_cursor_arg);
  canonical_path = normalize_source_hashimyei_cursor(canonical_path);
  intersection_cursor_arg = trim_ascii(intersection_cursor_arg);

  std::string schema;
  std::string run_id;
  (void)extract_json_string_field(arguments_json, "schema", &schema);
  (void)extract_json_string_field(arguments_json, "run_id", &run_id);

  std::size_t limit = 0;
  std::size_t offset = 0;
  bool newest_first = false;
  (void)extract_json_size_field(arguments_json, "limit", &limit);
  (void)extract_json_size_field(arguments_json, "offset", &offset);
  (void)extract_json_bool_field(arguments_json, "newest_first", &newest_first);
  std::uint64_t wave_cursor = 0;
  std::uint64_t wave_cursor_mask =
      cuwacunu::hero::wave::lattice_catalog_store_t::runtime_wave_cursor_full_mask();
  bool use_wave_cursor = false;
  if (!intersection_cursor_arg.empty()) {
    intersection_cursor_filter_t parsed{};
    if (!parse_intersection_cursor(intersection_cursor_arg, &parsed, out_error)) {
      return false;
    }
    canonical_path = parsed.hashimyei_cursor;
    use_wave_cursor = true;
    wave_cursor = parsed.wave_cursor;
    wave_cursor_mask =
        cuwacunu::hero::wave::lattice_catalog_store_t::runtime_wave_cursor_full_mask();
    intersection_cursor_arg = canonical_path + "|" + std::to_string(wave_cursor);
  } else if (extract_json_u64_field(arguments_json, "wave_cursor", &wave_cursor)) {
    use_wave_cursor = true;
    (void)extract_json_u64_field(arguments_json, "wave_cursor_mask",
                                 &wave_cursor_mask);
  } else {
    std::uint64_t mask_only = 0;
    if (extract_json_u64_field(arguments_json, "wave_cursor_mask", &mask_only)) {
      *out_error = "wave_cursor_mask requires wave_cursor";
      return false;
    }
  }
  if (canonical_path.empty()) {
    *out_error =
        "get_report_lls requires arguments.canonical_path, arguments.hashimyei_cursor, or arguments.intersection_cursor";
    return false;
  }

  if (!refresh_runtime_report_catalog(app, out_error)) return false;

  std::vector<cuwacunu::hero::hashimyei::artifact_entry_t> selected{};
  std::vector<cuwacunu::hero::hashimyei::artifact_entry_t> rows{};
  if (!app->catalog.list_runtime_artifacts(canonical_path, schema, 0, 0, newest_first,
                                           &rows, out_error)) {
    return false;
  }
  std::vector<cuwacunu::hero::hashimyei::artifact_entry_t> filtered{};
  filtered.reserve(rows.size());
  for (const auto& row : rows) {
    if (!run_id.empty() && row.run_id != run_id) continue;
    if (use_wave_cursor) {
      std::uint64_t row_wave_cursor = 0;
      if (!extract_wave_cursor_from_payload(row.payload_json, &row_wave_cursor)) {
        continue;
      }
      wave_cursor_resolution_e row_cursor_resolution =
          wave_cursor_resolution_e::Run;
      (void)extract_wave_cursor_resolution_from_payload(
          row.payload_json, &row_cursor_resolution);
      if (!matches_wave_cursor_filter(row_wave_cursor, wave_cursor,
                                      wave_cursor_mask,
                                      row_cursor_resolution)) {
        continue;
      }
    }
    filtered.push_back(row);
  }
  const std::size_t off = std::min(offset, filtered.size());
  std::size_t count = limit;
  if (count == 0) count = filtered.size() - off;
  count = std::min(count, filtered.size() - off);
  selected.assign(filtered.begin() + static_cast<std::ptrdiff_t>(off),
                  filtered.begin() + static_cast<std::ptrdiff_t>(off + count));

  std::ostringstream artifacts_json;
  artifacts_json << "[";
  for (std::size_t i = 0; i < selected.size(); ++i) {
    if (i != 0) artifacts_json << ",";
    artifacts_json << artifact_to_json(selected[i]);
  }
  artifacts_json << "]";
  if (!selected.empty()) {
    canonical_path = normalize_source_hashimyei_cursor(
        selected.front().canonical_path);
  }

  const std::string report_text = joined_kv_report(canonical_path, selected);
  std::ostringstream out;
  std::string normalized_intersection = intersection_cursor_arg;
  if (normalized_intersection.empty() && use_wave_cursor) {
    normalized_intersection =
        canonical_path + "|" + std::to_string(wave_cursor);
  }
  out << "{\"canonical_path\":" << json_quote(canonical_path)
      << ",\"hashimyei_cursor\":" << json_quote(canonical_path)
      << ",\"intersection_cursor\":"
      << (normalized_intersection.empty() ? "null"
                                          : json_quote(normalized_intersection))
      << ",\"run_id\":" << json_quote(run_id)
      << ",\"schema\":" << json_quote(schema)
      << ",\"wave_cursor\":"
      << (use_wave_cursor ? json_quote(std::to_string(wave_cursor)) : "null")
      << ",\"wave_cursor_mask\":"
      << (use_wave_cursor ? json_quote(std::to_string(wave_cursor_mask))
                          : "null")
      << ",\"count\":" << selected.size()
      << ",\"artifacts\":" << artifacts_json.str()
      << ",\"report_lls\":" << json_quote(report_text) << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_reset_catalog(app_context_t* app,
                                             const std::string& arguments_json,
                                             std::string* out_structured,
                                             std::string* out_error) {
  if (!app || !out_structured || !out_error) return false;
  out_error->clear();

  bool reingest_runtime = true;
  (void)extract_json_bool_field(arguments_json, "reingest_runtime",
                                &reingest_runtime);
  if (!reset_lattice_catalog(app, reingest_runtime, out_error)) return false;

  std::vector<cuwacunu::hero::hashimyei::artifact_entry_t> artifacts{};
  if (!app->catalog.list_runtime_artifacts("", "", 0, 0, true, &artifacts,
                                           out_error)) {
    return false;
  }

  std::ostringstream out;
  out << "{\"canonical_path\":\"\""
      << ",\"catalog_path\":" << json_quote(app->lattice_catalog_path.string())
      << ",\"reingest_runtime\":"
      << (reingest_runtime ? "true" : "false")
      << ",\"runtime_artifact_count\":" << artifacts.size() << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_call(app_context_t* app,
                                    const std::string& tool_name,
                                    const std::string& arguments_json,
                                    std::string* out_result_json,
                                    std::string* out_error_json) {
  if (!app || !out_result_json || !out_error_json) return false;

  std::string structured;
  std::string err;
  bool ok = false;
  if (tool_name == "hero.lattice.get_runs") {
    ok = handle_tool_get_runs(app, arguments_json, &structured, &err);
  } else if (tool_name == "hero.lattice.get_history") {
    ok = handle_tool_get_history(app, arguments_json, &structured, &err);
  } else if (tool_name == "hero.lattice.get_report_lls") {
    ok = handle_tool_get_report_lls(app, arguments_json, &structured, &err);
  } else if (tool_name == "hero.lattice.reset_catalog") {
    ok = handle_tool_reset_catalog(app, arguments_json, &structured, &err);
  } else {
    *out_error_json = "unknown tool: " + tool_name;
    return false;
  }

  if (!ok) {
    const std::string fallback =
        "{\"canonical_path\":\"\",\"error\":" + json_quote(err) + "}";
    *out_result_json = make_tool_result_json(err, fallback, true);
    return true;
  }

  *out_result_json = make_tool_result_json("ok", structured, false);
  return true;
}

int run_mcp_server(app_context_t* app) {
  if (!app) return 2;

  for (;;) {
    std::string message;
    bool used_content_length = false;
    if (!read_next_jsonrpc_message(&std::cin, &message, &used_content_length)) {
      break;
    }
    if (message.empty()) continue;

    if (used_content_length) {
      g_jsonrpc_use_content_length_framing = true;
    }

    std::string id_json;
    if (!extract_json_id_field(message, &id_json)) {
      write_jsonrpc_error("null", -32700, "invalid JSON-RPC id");
      continue;
    }

    std::string method;
    if (!extract_json_string_field(message, "method", &method)) {
      write_jsonrpc_error(id_json, -32600, "missing method");
      continue;
    }

    if (method.rfind("notifications/", 0) == 0) {
      continue;
    }
    if (method == "initialize") {
      write_jsonrpc_result(id_json, initialize_result_json());
      continue;
    }
    if (method == "ping") {
      write_jsonrpc_result(id_json, "{}");
      continue;
    }
    if (method == "tools/list") {
      write_jsonrpc_result(id_json, tools_list_result_json());
      continue;
    }
    if (method != "tools/call") {
      write_jsonrpc_error(id_json, -32601, "method not found");
      continue;
    }

    std::string params;
    if (!extract_json_object_field(message, "params", &params)) {
      write_jsonrpc_error(id_json, -32602, "tools/call requires params object");
      continue;
    }

    std::string name;
    if (!extract_json_string_field(params, "name", &name) || name.empty()) {
      write_jsonrpc_error(id_json, -32602, "tools/call requires params.name");
      continue;
    }

    std::string arguments = "{}";
    (void)extract_json_object_field(params, "arguments", &arguments);

    std::string tool_result;
    std::string tool_error;
    if (!handle_tool_call(app, name, arguments, &tool_result, &tool_error)) {
      write_jsonrpc_error(id_json, -32601, tool_error);
      continue;
    }
    write_jsonrpc_result(id_json, tool_result);
  }

  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  app_context_t app{};
  std::filesystem::path global_config_path = kDefaultGlobalConfigPath;
  std::filesystem::path hero_config_path{};
  app.store_root.clear();
  app.config_folder = DEFAULT_CONFIG_FOLDER;
  std::filesystem::path catalog_path{};
  std::filesystem::path hashimyei_catalog_path{};
  std::string direct_tool_name{};
  std::string direct_tool_args_json = "{}";
  bool direct_tool_mode = false;
  bool direct_tool_args_overridden = false;
  bool store_root_overridden = false;
  bool catalog_overridden = false;
  bool hash_catalog_overridden = false;
  bool config_folder_overridden = false;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--hero-config" && i + 1 < argc) {
      hero_config_path = argv[++i];
      continue;
    }
    if (arg == "--global-config" && i + 1 < argc) {
      global_config_path = argv[++i];
      continue;
    }
    if (arg == "--store-root" && i + 1 < argc) {
      app.store_root = argv[++i];
      store_root_overridden = true;
      continue;
    }
    if (arg == "--catalog" && i + 1 < argc) {
      catalog_path = argv[++i];
      catalog_overridden = true;
      continue;
    }
    if (arg == "--hashimyei-catalog" && i + 1 < argc) {
      hashimyei_catalog_path = argv[++i];
      hash_catalog_overridden = true;
      continue;
    }
    if (arg == "--tool" && i + 1 < argc) {
      direct_tool_name = argv[++i];
      direct_tool_mode = true;
      continue;
    }
    if (arg == "--args-json" && i + 1 < argc) {
      direct_tool_args_json = argv[++i];
      direct_tool_args_overridden = true;
      continue;
    }
    if (arg == "--config-folder" && i + 1 < argc) {
      app.config_folder = argv[++i];
      config_folder_overridden = true;
      continue;
    }
    if (arg == "--help" || arg == "-h") {
      print_cli_help(argv[0]);
      return 0;
    }
    std::cerr << "Unknown argument: " << arg << "\n";
    print_cli_help(argv[0]);
    return 2;
  }

  if (direct_tool_args_overridden && !direct_tool_mode) {
    std::cerr << "--args-json requires --tool\n";
    return 2;
  }
  if (direct_tool_mode) {
    direct_tool_name = trim_ascii(direct_tool_name);
    if (direct_tool_name.empty()) {
      std::cerr << "--tool requires a non-empty name\n";
      return 2;
    }
    direct_tool_args_json = trim_ascii(direct_tool_args_json);
    if (direct_tool_args_json.empty()) direct_tool_args_json = "{}";
    if (direct_tool_args_json.front() != '{') {
      std::cerr << "--args-json must be a JSON object\n";
      return 2;
    }
  }
  if (!direct_tool_mode && ::isatty(STDIN_FILENO) != 0) {
    std::cerr << "[" << kServerName
              << "] no --tool provided and stdin is a terminal; "
                 "server mode expects JSON-RPC input on stdin.\n";
    print_cli_help(argv[0]);
    return 2;
  }

  if (hero_config_path.empty()) {
    hero_config_path = resolve_lattice_hero_dsl_path(global_config_path);
  }
  wave_runtime_defaults_t defaults{};
  std::string defaults_error;
  if (!load_wave_runtime_defaults(hero_config_path, &defaults, &defaults_error)) {
    std::cerr << "[" << kServerName << "] " << defaults_error << "\n";
    return 2;
  }

  if (!store_root_overridden) {
    if (!defaults.store_root.empty()) {
      app.store_root = defaults.store_root;
    } else {
      app.store_root = default_store_root();
    }
  } else if (app.store_root.empty()) {
    app.store_root = default_store_root();
  }
  if (!catalog_overridden) {
    if (!defaults.catalog_path.empty()) {
      catalog_path = defaults.catalog_path;
    } else {
      catalog_path = default_catalog_path(app.store_root);
    }
  } else if (catalog_path.empty()) {
    catalog_path = default_catalog_path(app.store_root);
  }
  if (!hash_catalog_overridden) {
    if (!defaults.hashimyei_catalog_path.empty()) {
      hashimyei_catalog_path = defaults.hashimyei_catalog_path;
    } else {
      hashimyei_catalog_path = default_hashimyei_catalog_path(app.store_root);
    }
  } else if (hashimyei_catalog_path.empty()) {
    hashimyei_catalog_path = default_hashimyei_catalog_path(app.store_root);
  }
  if (!config_folder_overridden && !defaults.config_folder.empty()) {
    app.config_folder = defaults.config_folder;
  }
  app.lattice_catalog_path = catalog_path;
  app.hashimyei_catalog_path = hashimyei_catalog_path;

  cuwacunu::hero::wave::lattice_catalog_store_t::options_t opts{};
  opts.catalog_path = catalog_path;
  opts.encrypted = false;
  opts.projection_version = 2;

  std::string error;
  if (!app.catalog.open(opts, &error)) {
    std::cerr << "[" << kServerName << "] cannot open catalog: " << error << "\n";
    return 1;
  }

  if (direct_tool_mode) {
    std::string tool_result;
    std::string tool_error;
    if (!handle_tool_call(&app, direct_tool_name, direct_tool_args_json,
                          &tool_result, &tool_error)) {
      std::cerr << "[" << kServerName << "] " << tool_error << "\n";
      return 2;
    }
    write_jsonrpc_payload(tool_result);
    bool is_error = false;
    (void)extract_json_bool_field(tool_result, "isError", &is_error);
    if (g_protocol_stdout_fd >= 0 && g_protocol_stdout_fd != STDOUT_FILENO) {
      (void)::close(g_protocol_stdout_fd);
      g_protocol_stdout_fd = -1;
    }
    return is_error ? 1 : 0;
  }

  const int rc = run_mcp_server(&app);
  if (g_protocol_stdout_fd >= 0 && g_protocol_stdout_fd != STDOUT_FILENO) {
    (void)::close(g_protocol_stdout_fd);
    g_protocol_stdout_fd = -1;
  }
  return rc;
}
