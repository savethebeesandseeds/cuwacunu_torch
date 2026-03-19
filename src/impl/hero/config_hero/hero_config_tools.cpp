#include "hero/config_hero/hero_config_tools.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <optional>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <openssl/evp.h>

#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state_lhs.h"
#include "hero/hashimyei_hero/hashimyei_catalog.h"
#include "hero/config_hero/hero.config.h"
#include "hero/runtime_dev_loop.h"
#include "iitepi/contract_space_t.h"

namespace {
constexpr const char* kDeterministicOnlyMessage =
    "hero.config.ask and hero.config.fix are disabled in deterministic mode";
constexpr const char* kMcpServerName = "hero_config_mcp";
constexpr const char* kMcpServerVersion = "0.5.0";
constexpr const char* kDefaultMcpProtocolVersion = "2025-03-26";
constexpr const char* kMcpInitializeInstructions =
    "Use tools/list for tool schemas. Use tools/call with hero.config.*.";
constexpr const char* kConfigDslScopeErrorTag = "E_CONFIG_DSL_SCOPE";
constexpr int kConfigDslScopeErrorCode = -32041;
constexpr const char* kDefaultGlobalConfigPath = "/cuwacunu/src/config/.config";
constexpr const char* kCatalogFilename = "hashimyei_catalog.idydb";
constexpr std::size_t kMaxJsonRpcPayloadBytes = 8u << 20;  // 8 MiB
bool g_jsonrpc_use_content_length_framing = false;

[[nodiscard]] std::string trim_ascii(std::string_view in) {
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

[[nodiscard]] std::string lowercase_copy(std::string_view in) {
  std::string out(in);
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return out;
}

[[nodiscard]] std::filesystem::path resolve_path_near_config(
    std::string_view raw_path, std::string_view config_path);

[[nodiscard]] bool read_text_file(std::string_view path, std::string* out,
                                  std::string* err) {
  if (err) err->clear();
  if (!out) {
    if (err) *err = "output pointer is null";
    return false;
  }
  std::ifstream in(std::string(path), std::ios::binary);
  if (!in) {
    if (err) *err = "cannot open file: " + std::string(path);
    return false;
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  if (!in.good() && !in.eof()) {
    if (err) *err = "cannot read file: " + std::string(path);
    return false;
  }
  *out = ss.str();
  return true;
}

[[nodiscard]] bool write_text_file_atomic(std::string_view path,
                                          std::string_view content,
                                          std::string* err) {
  namespace fs = std::filesystem;
  if (err) err->clear();
  const fs::path target{std::string(path)};
  if (target.empty()) {
    if (err) *err = "target path is empty";
    return false;
  }
  std::error_code ec{};
  if (target.has_parent_path()) {
    fs::create_directories(target.parent_path(), ec);
    if (ec) {
      if (err) *err = "cannot create parent directory: " + target.parent_path().string();
      return false;
    }
  }
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  const fs::path tmp =
      target.parent_path() /
      (target.filename().string() + ".tmp." +
       std::to_string(static_cast<long long>(stamp)));
  {
    std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
    if (!out) {
      if (err) *err = "cannot open temp file: " + tmp.string();
      return false;
    }
    out << content;
    out.flush();
    if (!out.good()) {
      if (err) *err = "cannot write temp file: " + tmp.string();
      return false;
    }
  }
  fs::rename(tmp, target, ec);
  if (ec) {
    std::error_code rm_ec{};
    fs::remove(tmp, rm_ec);
    if (err) *err = "cannot replace target file: " + target.string();
    return false;
  }
  return true;
}

[[nodiscard]] bool path_is_within(std::filesystem::path base,
                                  std::filesystem::path candidate) {
  std::error_code ec{};
  if (!base.empty()) {
    const auto c = std::filesystem::weakly_canonical(base, ec);
    if (!ec) base = c;
  }
  ec.clear();
  if (!candidate.empty()) {
    const auto c = std::filesystem::weakly_canonical(candidate, ec);
    if (!ec) candidate = c;
  }
  base = base.lexically_normal();
  candidate = candidate.lexically_normal();
  auto b = base.begin();
  auto c = candidate.begin();
  for (; b != base.end() && c != candidate.end(); ++b, ++c) {
    if (*b != *c) return false;
  }
  return b == base.end();
}

[[nodiscard]] std::size_t find_comment_pos_unquoted(std::string_view text) {
  bool in_single = false;
  bool in_double = false;
  bool escape = false;
  for (std::size_t i = 0; i < text.size(); ++i) {
    const char ch = text[i];
    if (escape) {
      escape = false;
      continue;
    }
    if (ch == '\\') {
      escape = true;
      continue;
    }
    if (ch == '\'' && !in_double) {
      in_single = !in_single;
      continue;
    }
    if (ch == '"' && !in_single) {
      in_double = !in_double;
      continue;
    }
    if (in_single || in_double) continue;
    if (ch == '#' || ch == ';') return i;
  }
  return std::string::npos;
}

struct parsed_dsl_line_t {
  bool has_assignment{false};
  std::string leading{};
  std::string lhs{};
  std::string key{};
  std::string declared_type{};
  std::string value{};
  std::string comment{};
};

[[nodiscard]] parsed_dsl_line_t parse_dsl_line(std::string_view line) {
  parsed_dsl_line_t out{};
  const std::string original(line);
  const std::size_t first =
      original.find_first_not_of(" \t\r");
  if (first == std::string::npos) return out;
  const std::string_view trimmed = std::string_view(original).substr(first);
  if (trimmed.rfind("#", 0) == 0 || trimmed.rfind(";", 0) == 0 ||
      trimmed.rfind("//", 0) == 0 || trimmed.rfind("/*", 0) == 0 ||
      trimmed == "*/") {
    return out;
  }

  const std::size_t eq = original.find('=', first);
  if (eq == std::string::npos) return out;

  out.leading = original.substr(0, first);
  out.lhs = trim_ascii(original.substr(first, eq - first));
  if (out.lhs.empty()) return out;

  const auto parsed_lhs = cuwacunu::camahjucunu::dsl::parse_latent_lineage_state_lhs(out.lhs);
  out.key = parsed_lhs.key;
  out.declared_type = parsed_lhs.declared_type;
  if (out.key.empty()) return out;

  const std::string rhs_comment = original.substr(eq + 1);
  const std::size_t comment_pos = find_comment_pos_unquoted(rhs_comment);
  if (comment_pos == std::string::npos) {
    out.value = trim_ascii(rhs_comment);
  } else {
    out.value = trim_ascii(rhs_comment.substr(0, comment_pos));
    out.comment = trim_ascii(rhs_comment.substr(comment_pos));
  }
  out.has_assignment = true;
  return out;
}

enum class dsl_path_resolution_error_t {
  kNone = 0,
  kOutputPointerNull,
  kEmptyPath,
  kEscapesScope,
  kHashimyeiPath,
  kNotDefaultInstructionDsl,
};

[[nodiscard]] bool is_scope_violation(dsl_path_resolution_error_t err) {
  return err == dsl_path_resolution_error_t::kEscapesScope ||
         err == dsl_path_resolution_error_t::kHashimyeiPath ||
         err == dsl_path_resolution_error_t::kNotDefaultInstructionDsl;
}

[[nodiscard]] bool path_has_component(const std::filesystem::path& path,
                                      std::string_view component) {
  for (const auto& part : path.lexically_normal()) {
    if (part == component) return true;
  }
  return false;
}

[[nodiscard]] std::filesystem::path resolve_instruction_root(
    const cuwacunu::hero::mcp::hero_config_store_t& store,
    const std::filesystem::path& scope_path) {
  if (!scope_path.empty()) {
    if (scope_path.filename() == "instructions") return scope_path;
    return scope_path / "instructions";
  }
  const std::filesystem::path cfg_path = store.config_path();
  return cfg_path.parent_path();
}

[[nodiscard]] bool resolve_dsl_path_with_scope(
    const cuwacunu::hero::mcp::hero_config_store_t& store,
    std::string_view request_path, std::filesystem::path* out_path,
    std::string* out_error, dsl_path_resolution_error_t* out_reason) {
  if (out_error) out_error->clear();
  if (out_reason) *out_reason = dsl_path_resolution_error_t::kNone;
  if (!out_path) {
    if (out_error) *out_error = "dsl path output pointer is null";
    if (out_reason) *out_reason = dsl_path_resolution_error_t::kOutputPointerNull;
    return false;
  }
  *out_path = std::filesystem::path{};

  const std::string raw = trim_ascii(request_path);
  if (raw.empty()) {
    if (out_error) *out_error = "dsl path is empty";
    if (out_reason) *out_reason = dsl_path_resolution_error_t::kEmptyPath;
    return false;
  }
  const std::filesystem::path resolved =
      resolve_path_near_config(raw, store.config_path());
  const std::string scope_raw = trim_ascii(store.get_or_default("config_scope_root"));
  const std::filesystem::path scope_path(scope_raw);
  if (path_has_component(resolved, ".hashimyei")) {
    if (out_error) {
      *out_error = std::string(kConfigDslScopeErrorTag) +
                   ": dsl path under .hashimyei is not allowed: " +
                   resolved.string();
    }
    if (out_reason) *out_reason = dsl_path_resolution_error_t::kHashimyeiPath;
    return false;
  }
  if (!scope_raw.empty()) {
    if (!path_is_within(scope_path, resolved)) {
      if (out_error) {
        *out_error = std::string(kConfigDslScopeErrorTag) +
                     ": dsl path escapes config_scope_root: " + resolved.string();
      }
      if (out_reason) *out_reason = dsl_path_resolution_error_t::kEscapesScope;
      return false;
    }
  }

  const std::filesystem::path instruction_root =
      resolve_instruction_root(store, scope_path);
  const std::string filename = resolved.filename().string();
  const bool filename_is_default_dsl =
      resolved.extension() == ".dsl" && filename.rfind("default.", 0) == 0;
  if (!filename_is_default_dsl || !path_is_within(instruction_root, resolved)) {
    if (out_error) {
      *out_error = std::string(kConfigDslScopeErrorTag) +
                   ": dsl path must target instructions/default.*.dsl under " +
                   instruction_root.string() + ": " + resolved.string();
    }
    if (out_reason) {
      *out_reason = dsl_path_resolution_error_t::kNotDefaultInstructionDsl;
    }
    return false;
  }

  *out_path = resolved;
  return true;
}

[[nodiscard]] bool read_ini_general_key(std::string_view ini_path,
                                        std::string_view key,
                                        std::string* out_value) {
  if (out_value) out_value->clear();
  std::ifstream in(std::string(ini_path), std::ios::binary);
  if (!in) return false;

  bool in_general = false;
  std::string line;
  while (std::getline(in, line)) {
    std::string trimmed = trim_ascii(line);
    if (trimmed.empty()) continue;
    if (trimmed.front() == '#' || trimmed.front() == ';') continue;
    if (trimmed.front() == '[' && trimmed.back() == ']') {
      in_general = lowercase_copy(trimmed) == "[general]";
      continue;
    }
    if (!in_general) continue;

    const std::size_t eq = trimmed.find('=');
    if (eq == std::string::npos) continue;
    const std::string lhs = trim_ascii(trimmed.substr(0, eq));
    if (lhs != key) continue;
    std::string rhs = trim_ascii(trimmed.substr(eq + 1));
    const std::size_t comment = rhs.find('#');
    if (comment != std::string::npos) {
      rhs = trim_ascii(rhs.substr(0, comment));
    }
    if (out_value) *out_value = rhs;
    return true;
  }
  return false;
}

[[nodiscard]] std::filesystem::path configured_hashimyei_store_root(
    std::string_view global_config_path) {
  const char* env = std::getenv("CUWACUNU_HASHIMYEI_STORE_ROOT");
  if (env != nullptr && env[0] != '\0') {
    const std::string value = trim_ascii(env);
    if (!value.empty()) return std::filesystem::path(value);
  }

  const std::string cfg_path = trim_ascii(global_config_path);
  const std::string ini_path =
      cfg_path.empty() ? std::string(kDefaultGlobalConfigPath) : cfg_path;
  std::string configured{};
  if (read_ini_general_key(ini_path, "runtime_root", &configured)) {
    configured = trim_ascii(configured);
    if (!configured.empty()) {
      return (std::filesystem::path(configured) / ".hashimyei").lexically_normal();
    }
  }
  return {};
}

[[nodiscard]] std::filesystem::path configured_hashimyei_catalog_path(
    std::string_view global_config_path) {
  return cuwacunu::hashimyei::catalog_db_path(
      configured_hashimyei_store_root(global_config_path));
}

[[nodiscard]] std::string normalized_path_key(
    const std::filesystem::path& path);
[[nodiscard]] bool sha256_hex_file(const std::filesystem::path& path,
                                   std::string* out_hex,
                                   std::string* err);

[[nodiscard]] std::string sanitize_bundle_snapshot_filename(
    std::string_view filename) {
  std::string out{};
  out.reserve(filename.size());
  for (const char ch : filename) {
    const bool ok = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                    (ch >= '0' && ch <= '9') || ch == '.' || ch == '_' ||
                    ch == '-';
    out.push_back(ok ? ch : '_');
  }
  if (out.empty()) out = "dsl";
  return out;
}

[[nodiscard]] bool write_component_founding_dsl_bundle_snapshot(
    const std::filesystem::path& store_root,
    const cuwacunu::hero::hashimyei::component_manifest_t& manifest,
    std::string_view primary_source_path,
    const std::shared_ptr<const cuwacunu::iitepi::contract_record_t>& contract_record,
    std::string* error) {
  if (error) error->clear();
  if (!contract_record) {
    if (error) *error = "missing contract snapshot for founding DSL bundle";
    return false;
  }

  std::vector<std::filesystem::path> source_paths{};
  std::unordered_set<std::string> seen{};
  const auto add_source_path = [&](std::string_view raw_path) {
    const std::string trimmed = trim_ascii(raw_path);
    if (trimmed.empty()) return;
    const std::filesystem::path source_path(trimmed);
    const std::string key = normalized_path_key(source_path);
    if (!seen.insert(key).second) return;
    source_paths.push_back(source_path);
  };

  add_source_path(contract_record->config_file_path_canonical);
  add_source_path(primary_source_path);
  for (const auto& surface : contract_record->docking_signature.surfaces) {
    add_source_path(surface.canonical_path);
  }
  for (const auto& module : contract_record->signature.module_dsl_entries) {
    add_source_path(module.module_dsl_path);
  }

  std::sort(source_paths.begin(), source_paths.end(),
            [](const std::filesystem::path& a, const std::filesystem::path& b) {
              return normalized_path_key(a) < normalized_path_key(b);
            });

  const std::string component_id =
      cuwacunu::hero::hashimyei::compute_component_manifest_id(manifest);
  const auto bundle_dir =
      cuwacunu::hero::hashimyei::founding_dsl_bundle_directory(store_root,
                                                               manifest.canonical_path,
                                                               component_id);
  std::error_code ec{};
  std::filesystem::create_directories(bundle_dir, ec);
  if (ec) {
    if (error) {
      *error = "cannot create founding DSL bundle directory: " +
               bundle_dir.string();
    }
    return false;
  }

  cuwacunu::hero::hashimyei::founding_dsl_bundle_manifest_t bundle_manifest{};
  bundle_manifest.component_id = component_id;
  bundle_manifest.canonical_path = manifest.canonical_path;
  bundle_manifest.hashimyei_name = manifest.hashimyei_identity.name;
  bundle_manifest.files.reserve(source_paths.size());

  for (std::size_t i = 0; i < source_paths.size(); ++i) {
    const auto& source_path = source_paths[i];
    std::string text{};
    if (!read_text_file(source_path.string(), &text, error)) {
      if (error) {
        *error = "cannot read founding DSL source '" + source_path.string() +
                 "': " + *error;
      }
      return false;
    }
    std::ostringstream filename;
    filename << std::setfill('0') << std::setw(4) << i << "_"
             << sanitize_bundle_snapshot_filename(source_path.filename().string());
    const auto snapshot_path = bundle_dir / filename.str();
    if (!write_text_file_atomic(snapshot_path.string(), text, error)) {
      if (error) {
        *error = "cannot write founding DSL snapshot '" + snapshot_path.string() +
                 "': " + *error;
      }
      return false;
    }
    std::string sha256_hex{};
    if (!sha256_hex_file(snapshot_path, &sha256_hex, error)) {
      if (error) {
        *error = "cannot fingerprint founding DSL snapshot '" +
                 snapshot_path.string() + "': " + *error;
      }
      return false;
    }
    bundle_manifest.files.push_back(
        cuwacunu::hero::hashimyei::founding_dsl_bundle_manifest_file_t{
            .source_path = source_path.string(),
            .snapshot_relpath = filename.str(),
            .sha256_hex = sha256_hex,
        });
  }
  bundle_manifest.founding_dsl_bundle_aggregate_sha256_hex =
      cuwacunu::hero::hashimyei::compute_founding_dsl_bundle_aggregate_sha256(
          bundle_manifest);
  if (bundle_manifest.founding_dsl_bundle_aggregate_sha256_hex.empty()) {
    if (error) {
      *error = "cannot compute founding DSL bundle aggregate digest";
    }
    return false;
  }

  if (!cuwacunu::hero::hashimyei::write_founding_dsl_bundle_manifest(
          store_root, bundle_manifest, error)) {
    return false;
  }
  return true;
}

[[nodiscard]] std::string json_escape(std::string_view in) {
  std::ostringstream out;
  for (const unsigned char c : in) {
    switch (c) {
      case '\"':
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
          out << "\\u00";
          const char* hex = "0123456789abcdef";
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
  const std::string trimmed = trim_ascii(raw);
  if (trimmed.empty() || trimmed.front() != '{') return false;
  if (out) *out = trimmed;
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

[[nodiscard]] bool has_json_field(const std::string& json,
                                  const std::string& key) {
  return extract_json_raw_field(json, key, nullptr);
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
      if (content_length == 0) {
        continue;
      }
      if (content_length > kMaxJsonRpcPayloadBytes) return false;
      std::string payload(content_length, '\0');
      in->read(payload.data(), static_cast<std::streamsize>(content_length));
      if (in->gcount() != static_cast<std::streamsize>(content_length)) {
        return false;
      }
      if (out_payload) *out_payload = payload;
      if (out_used_content_length) *out_used_content_length = true;
      return true;
    }

    if (trimmed.front() == '{' || trimmed.front() == '[') {
      if (out_payload) *out_payload = trimmed;
      return true;
    }
  }

  return false;
}

void emit_jsonrpc_payload(std::string_view payload_json) {
  if (g_jsonrpc_use_content_length_framing) {
    std::cout << "Content-Length: " << payload_json.size() << "\r\n\r\n"
              << payload_json;
  } else {
    std::cout << payload_json << '\n';
  }
  std::cout.flush();
}

void emit_jsonrpc_result(std::string_view id_json,
                         std::string_view result_object_json) {
  const std::string payload = std::string("{\"jsonrpc\":\"2.0\",\"id\":") +
                              std::string(id_json) + ",\"result\":" +
                              std::string(result_object_json) + "}";
  emit_jsonrpc_payload(payload);
}

void emit_jsonrpc_error(std::string_view id_json, int code,
                        std::string_view message) {
  const std::string payload =
      std::string("{\"jsonrpc\":\"2.0\",\"id\":") + std::string(id_json) +
      ",\"error\":{\"code\":" + std::to_string(code) +
      ",\"message\":" + json_quote(message) + "}}";
  emit_jsonrpc_payload(payload);
}

[[nodiscard]] std::string bool_json(bool v) { return v ? "true" : "false"; }

[[nodiscard]] std::uint64_t fnv1a64(std::string_view text) {
  std::uint64_t hash = 1469598103934665603ULL;
  for (const unsigned char c : text) {
    hash ^= static_cast<std::uint64_t>(c);
    hash *= 1099511628211ULL;
  }
  return hash;
}

[[nodiscard]] std::string hex_u64(std::uint64_t value) {
  std::ostringstream out;
  out << std::hex << std::setfill('0') << std::setw(16) << value;
  return out.str();
}

[[nodiscard]] std::uint64_t now_ms_utc() {
  const auto now = std::chrono::system_clock::now();
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          now.time_since_epoch())
          .count());
}

[[nodiscard]] bool ends_with_ascii_case_insensitive(std::string_view text,
                                                    std::string_view suffix) {
  if (text.size() < suffix.size()) return false;
  const std::size_t offset = text.size() - suffix.size();
  for (std::size_t i = 0; i < suffix.size(); ++i) {
    const char a = static_cast<char>(
        std::tolower(static_cast<unsigned char>(text[offset + i])));
    const char b = static_cast<char>(
        std::tolower(static_cast<unsigned char>(suffix[i])));
    if (a != b) return false;
  }
  return true;
}

[[nodiscard]] std::string normalize_component_family(std::string canonical_like) {
  canonical_like = trim_ascii(canonical_like);
  if (canonical_like.empty()) return {};

  for (char& ch : canonical_like) {
    if (ch == '/') ch = '.';
  }
  while (!canonical_like.empty() && canonical_like.front() == '.') {
    canonical_like.erase(canonical_like.begin());
  }
  while (!canonical_like.empty() && canonical_like.back() == '.') {
    canonical_like.pop_back();
  }

  const auto strip_suffix = [&](std::string_view suffix) {
    if (ends_with_ascii_case_insensitive(canonical_like, suffix)) {
      canonical_like.resize(canonical_like.size() - suffix.size());
    }
  };

  strip_suffix(".dsl_file");
  strip_suffix("_dsl_file");
  strip_suffix(".dsl_path");
  strip_suffix("_dsl_path");
  strip_suffix(".dsl");
  strip_suffix(".network_design");
  strip_suffix(".sources");
  strip_suffix(".channels");
  strip_suffix(".jkimyei");

  return trim_ascii(canonical_like);
}

[[nodiscard]] std::string maybe_hash_tail(std::string_view canonical) {
  const std::size_t dot = canonical.rfind('.');
  if (dot == std::string::npos || dot + 1 >= canonical.size()) return {};
  const std::string_view tail = canonical.substr(dot + 1);
  if (tail.size() < 3) return {};
  if (tail[0] != '0' || (tail[1] != 'x' && tail[1] != 'X')) return {};
  for (std::size_t i = 2; i < tail.size(); ++i) {
    const char c = tail[i];
    const bool digit = c >= '0' && c <= '9';
    const bool lower = c >= 'a' && c <= 'f';
    const bool upper = c >= 'A' && c <= 'F';
    if (!digit && !lower && !upper) return {};
  }
  return std::string(tail);
}

[[nodiscard]] std::string strip_hash_tail(std::string canonical) {
  const std::string tail = maybe_hash_tail(canonical);
  if (tail.empty()) return canonical;
  const std::string suffix = "." + tail;
  if (canonical.size() > suffix.size() &&
      canonical.compare(canonical.size() - suffix.size(), suffix.size(),
                        suffix) == 0) {
    canonical.resize(canonical.size() - suffix.size());
  }
  return canonical;
}

[[nodiscard]] std::filesystem::path resolve_path_near_config(
    std::string_view raw_path, std::string_view config_path) {
  namespace fs = std::filesystem;
  fs::path raw(trim_ascii(raw_path));
  if (raw.empty()) return raw;

  const auto canonical_or_normalized = [](const fs::path& p) {
    std::error_code ec{};
    const fs::path canonical = fs::weakly_canonical(p, ec);
    return ec ? p.lexically_normal() : canonical;
  };

  if (raw.is_absolute()) {
    return canonical_or_normalized(raw);
  }

  std::vector<fs::path> candidates{};
  candidates.push_back(raw);

  const fs::path config_parent = fs::path(config_path).parent_path();
  if (!config_parent.empty()) candidates.push_back(config_parent / raw);

  constexpr std::string_view kRepoRoot = "/cuwacunu";
  candidates.push_back(fs::path(kRepoRoot) / raw);

  for (const auto& candidate : candidates) {
    std::error_code ec{};
    if (fs::exists(candidate, ec) && !ec) return canonical_or_normalized(candidate);
  }
  return canonical_or_normalized(candidates.front());
}

[[nodiscard]] std::string normalized_path_key(
    const std::filesystem::path& path) {
  if (path.empty()) return {};
  return path.generic_string();
}

[[nodiscard]] std::uint64_t deterministic_cutover_ts_ms(
    std::string_view seed) {
  constexpr std::uint64_t kBase = 1700000000000ULL;
  constexpr std::uint64_t kSpan = 1000000000000ULL;
  return kBase + (fnv1a64(seed) % kSpan);
}

[[nodiscard]] bool sha256_hex_file(const std::filesystem::path& path,
                                   std::string* out_hex,
                                   std::string* out_error) {
  if (out_error) out_error->clear();
  if (!out_hex) {
    if (out_error) *out_error = "sha256 output pointer is null";
    return false;
  }
  out_hex->clear();

  std::ifstream in(path, std::ios::binary);
  if (!in) {
    if (out_error) *out_error = "cannot open file for sha256: " + path.string();
    return false;
  }

  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  if (!ctx) {
    if (out_error) *out_error = "cannot allocate EVP context";
    return false;
  }
  if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
    EVP_MD_CTX_free(ctx);
    if (out_error) *out_error = "EVP_DigestInit_ex failed";
    return false;
  }

  std::array<char, 1 << 14> buf{};
  while (in.good()) {
    in.read(buf.data(), static_cast<std::streamsize>(buf.size()));
    const std::streamsize got = in.gcount();
    if (got <= 0) break;
    if (EVP_DigestUpdate(ctx, buf.data(), static_cast<std::size_t>(got)) != 1) {
      EVP_MD_CTX_free(ctx);
      if (out_error) *out_error = "EVP_DigestUpdate failed";
      return false;
    }
  }

  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int digest_len = 0;
  if (EVP_DigestFinal_ex(ctx, digest, &digest_len) != 1) {
    EVP_MD_CTX_free(ctx);
    if (out_error) *out_error = "EVP_DigestFinal_ex failed";
    return false;
  }
  EVP_MD_CTX_free(ctx);

  static constexpr std::array<char, 16> kHex{
      '0', '1', '2', '3', '4', '5', '6', '7',
      '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  out_hex->resize(static_cast<std::size_t>(digest_len) * 2);
  for (std::size_t i = 0; i < static_cast<std::size_t>(digest_len); ++i) {
    const unsigned char b = digest[i];
    (*out_hex)[2 * i + 0] = kHex[(b >> 4) & 0x0F];
    (*out_hex)[2 * i + 1] = kHex[b & 0x0F];
  }
  return true;
}

[[nodiscard]] std::string derive_family_from_key_or_path(
    std::string_view key, std::string_view dsl_path) {
  const std::string trimmed_key = trim_ascii(key);
  const std::size_t tsi_pos = trimmed_key.find("tsi.");
  if (tsi_pos != std::string::npos) {
    const std::string from_key =
        normalize_component_family(trimmed_key.substr(tsi_pos));
    if (!from_key.empty()) return strip_hash_tail(from_key);
  }

  const std::filesystem::path p(trim_ascii(dsl_path));
  const std::string stem = p.stem().string();
  if (stem.rfind("tsi.", 0) == 0) {
    const std::string from_file = normalize_component_family(stem);
    if (!from_file.empty()) return strip_hash_tail(from_file);
  }
  return {};
}

[[nodiscard]] std::string synthesize_hashimyei_from_sha(std::string_view sha_hex,
                                                        std::string_view fallback) {
  if (sha_hex.size() >= 18) {
    return "0x" + std::string(sha_hex.substr(0, 16));
  }
  if (sha_hex.size() >= 10) {
    return "0x" + std::string(sha_hex.substr(0, 8));
  }
  if (!fallback.empty()) return std::string(fallback);
  return "0x00000000";
}

[[nodiscard]] bool key_looks_like_component_dsl_path(std::string_view key) {
  const std::string lowered = lowercase_copy(trim_ascii(key));
  if (lowered.find("dsl") == std::string::npos) return false;
  if (lowered.find("component") != std::string::npos) return true;
  if (lowered.find("hashimyei") != std::string::npos) return true;
  if (lowered.find("tsi.") != std::string::npos) return true;
  if (lowered.size() >= 9 &&
      lowered.compare(lowered.size() - 9, 9, "_dsl_file") == 0) {
    return true;
  }
  return false;
}

struct touched_component_dsl_diff_t {
  std::string key{};
  std::string action{};
  std::string before_value{};
  std::string after_value{};
  std::string raw_path{};
  std::filesystem::path resolved_path{};
  std::string resolved_norm{};
  std::string dsl_sha256_hex{};
  std::string error{};
};

struct cutover_receipt_entry_t {
  std::string component_dsl_key{};
  std::string action{};
  std::string dsl_path{};
  std::string canonical_path{};
  std::string family{};
  std::string old_hashimyei{};
  std::string new_hashimyei{};
  std::string component_id{};
  std::string status{};
  std::string message{};
  bool inserted{false};
};

[[nodiscard]] std::string build_cutover_receipt_json_impl(
    const cuwacunu::hero::mcp::hero_config_store_t::save_preview_t& preview,
    std::string_view config_path, std::string_view global_config_path) {
  std::vector<touched_component_dsl_diff_t> touched{};
  touched.reserve(preview.diffs.size());
  for (const auto& diff : preview.diffs) {
    if (!key_looks_like_component_dsl_path(diff.key)) continue;
    touched_component_dsl_diff_t t{};
    t.key = diff.key;
    t.action = diff.action;
    t.before_value = diff.before_value;
    t.after_value = diff.after_value;
    t.raw_path = trim_ascii(!diff.after_value.empty() ? diff.after_value
                                                       : diff.before_value);
    if (trim_ascii(t.action) == "removed") {
      t.error =
          "component DSL key was removed; no active revision cutover applied";
      touched.push_back(std::move(t));
      continue;
    }
    if (t.raw_path.empty()) {
      t.error = "component DSL path is empty";
      touched.push_back(std::move(t));
      continue;
    }
    t.resolved_path = resolve_path_near_config(t.raw_path, config_path);
    t.resolved_norm = normalized_path_key(t.resolved_path);
    if (t.resolved_norm.empty()) {
      t.error = "cannot resolve component DSL path";
      touched.push_back(std::move(t));
      continue;
    }
    std::string sha_error;
    if (!sha256_hex_file(t.resolved_path, &t.dsl_sha256_hex, &sha_error)) {
      t.error = sha_error;
    }
    touched.push_back(std::move(t));
  }

  std::sort(touched.begin(), touched.end(),
            [](const touched_component_dsl_diff_t& a,
               const touched_component_dsl_diff_t& b) {
              if (a.key != b.key) return a.key < b.key;
              if (a.resolved_norm != b.resolved_norm) {
                return a.resolved_norm < b.resolved_norm;
              }
              return a.raw_path < b.raw_path;
            });
  touched.erase(
      std::unique(touched.begin(), touched.end(),
                  [](const touched_component_dsl_diff_t& a,
                     const touched_component_dsl_diff_t& b) {
                    return a.key == b.key && a.resolved_norm == b.resolved_norm &&
                           a.action == b.action && a.raw_path == b.raw_path;
                  }),
      touched.end());

  const std::string base_payload =
      preview.proposed_text.empty() ? preview.current_text : preview.proposed_text;
  const std::string founding_revision_id =
      "cfgrev." + hex_u64(fnv1a64(base_payload));
  const std::uint64_t generated_at_ms = now_ms_utc();

  std::vector<cutover_receipt_entry_t> receipts{};
  receipts.reserve(touched.size());

  const std::filesystem::path store_root =
      configured_hashimyei_store_root(global_config_path);
  const std::filesystem::path catalog_path =
      store_root.empty() ? std::filesystem::path{}
                         : configured_hashimyei_catalog_path(global_config_path);
  std::string catalog_error{};
  if (!touched.empty()) {
    if (store_root.empty()) {
      catalog_error = "missing GENERAL.runtime_root in " +
                      std::string(global_config_path.empty()
                                      ? kDefaultGlobalConfigPath
                                      : global_config_path);
    } else if (catalog_path.empty()) {
      catalog_error = "cannot derive hashimyei catalog path from GENERAL.runtime_root";
    }
  }
  std::size_t matched_components_count = 0;
  std::size_t applied_count = 0;
  std::size_t noop_count = 0;
  std::size_t skipped_count = 0;
  std::size_t error_count = 0;

  bool catalog_opened = false;
  cuwacunu::hero::hashimyei::hashimyei_catalog_store_t catalog{};
  std::vector<cuwacunu::hero::hashimyei::component_state_t> components{};
  std::unordered_map<std::string, std::string> component_dsl_norm_cache{};
  std::unordered_map<std::string, std::string> latest_component_id_by_pointer{};
  std::unordered_map<std::string, std::optional<std::string>>
      active_hash_cache_by_pointer{};

  if (!touched.empty() && catalog_error.empty()) {
    cuwacunu::hero::hashimyei::hashimyei_catalog_store_t::options_t options{};
    options.catalog_path = catalog_path;
    options.encrypted = false;
    options.ingest_version = 1;
    if (!catalog.open(options, &catalog_error)) {
      catalog_error = "catalog open failed: " + catalog_error;
    } else {
      catalog_opened = true;
      if (!catalog.list_components("", "", 0, 0, true, &components,
                                   &catalog_error)) {
        catalog_error = "catalog list_components failed: " + catalog_error;
      } else {
        latest_component_id_by_pointer.reserve(components.size());
        for (const auto& component : components) {
          const std::string contract_hash =
              cuwacunu::hero::hashimyei::contract_hash_from_identity(
                  component.manifest.contract_identity);
          const std::string pointer_key =
              component.manifest.canonical_path + "|" + component.manifest.family +
              "|" + contract_hash;
          if (latest_component_id_by_pointer.count(pointer_key) == 0) {
            latest_component_id_by_pointer[pointer_key] = component.component_id;
          }
        }
      }
    }
  }

  std::unordered_set<std::string> emitted_revision_dedup{};
  std::unordered_set<std::string> processed_component_pointer_keys{};
  for (const auto& t : touched) {
    if (!t.error.empty()) {
      receipts.push_back(cutover_receipt_entry_t{
          .component_dsl_key = t.key,
          .action = t.action,
          .dsl_path = t.resolved_norm.empty() ? t.raw_path : t.resolved_norm,
          .status = "error",
          .message = t.error,
      });
      ++error_count;
      continue;
    }

    if (!catalog_opened || !catalog_error.empty()) {
      receipts.push_back(cutover_receipt_entry_t{
          .component_dsl_key = t.key,
          .action = t.action,
          .dsl_path = t.resolved_norm,
          .status = "error",
          .message = catalog_error.empty() ? "catalog unavailable for cutover"
                                           : catalog_error,
      });
      ++error_count;
      continue;
    }

    const std::string target_family =
        derive_family_from_key_or_path(t.key, t.raw_path);
    const std::string target_filename =
        std::filesystem::path(t.resolved_norm).filename().string();

    auto component_dsl_source_key =
        [&](const cuwacunu::hero::hashimyei::component_state_t& component)
            -> const std::string& {
      auto it = component_dsl_norm_cache.find(component.component_id);
      if (it != component_dsl_norm_cache.end()) return it->second;
      const std::string raw_source =
          trim_ascii(component.manifest.founding_dsl_source_path);
      std::string normalized{};
      if (!raw_source.empty()) {
        const std::filesystem::path source_path(raw_source);
        if (source_path.is_absolute()) {
          normalized = normalized_path_key(
              resolve_path_near_config(source_path.string(), config_path));
        } else {
          normalized = source_path.filename().string();
        }
      }
      auto [inserted_it, _] =
          component_dsl_norm_cache.emplace(component.component_id, normalized);
      return inserted_it->second;
    };

    auto component_matches_target_dsl =
        [&](const cuwacunu::hero::hashimyei::component_state_t& component) {
      const std::string& source_key = component_dsl_source_key(component);
      if (source_key.empty()) return false;
      if (source_key == t.resolved_norm) return true;
      if (!target_filename.empty() && source_key == target_filename) {
        return target_family.empty() ||
               component.manifest.family == target_family;
      }
      return false;
    };

    auto is_active_component = [&](const cuwacunu::hero::hashimyei::component_state_t&
                                       component) {
      const std::string contract_hash =
          cuwacunu::hero::hashimyei::contract_hash_from_identity(
              component.manifest.contract_identity);
      const std::string pointer_key =
          component.manifest.canonical_path + "|" + component.manifest.family +
          "|" + contract_hash;
      const auto cache_it = active_hash_cache_by_pointer.find(pointer_key);
      if (cache_it != active_hash_cache_by_pointer.end()) {
        if (!cache_it->second.has_value()) {
          const auto latest_it = latest_component_id_by_pointer.find(pointer_key);
          return latest_it != latest_component_id_by_pointer.end() &&
                 latest_it->second == component.component_id;
        }
        return *cache_it->second == component.manifest.hashimyei_identity.name;
      }

      std::string active_hash{};
      std::string active_error{};
      if (catalog.resolve_active_hashimyei(component.manifest.canonical_path,
                                           component.manifest.family,
                                           contract_hash,
                                           &active_hash,
                                           &active_error)) {
        active_hash_cache_by_pointer.emplace(pointer_key, active_hash);
        return active_hash == component.manifest.hashimyei_identity.name;
      }

      active_hash_cache_by_pointer.emplace(pointer_key, std::nullopt);
      const auto latest_it = latest_component_id_by_pointer.find(pointer_key);
      return latest_it != latest_component_id_by_pointer.end() &&
             latest_it->second == component.component_id;
    };

    std::vector<cuwacunu::hero::hashimyei::component_state_t> matched{};
    matched.reserve(8);
    std::unordered_set<std::string> seen_pointer{};
    for (const auto& component : components) {
      if (!component_matches_target_dsl(component)) continue;
      if (!is_active_component(component)) continue;
      const std::string contract_hash =
          cuwacunu::hero::hashimyei::contract_hash_from_identity(
              component.manifest.contract_identity);
      const std::string pointer_key =
          component.manifest.canonical_path + "|" + component.manifest.family +
          "|" + contract_hash;
      if (seen_pointer.insert(pointer_key).second) matched.push_back(component);
    }

    std::sort(matched.begin(), matched.end(),
              [](const cuwacunu::hero::hashimyei::component_state_t& a,
                 const cuwacunu::hero::hashimyei::component_state_t& b) {
                if (a.manifest.canonical_path != b.manifest.canonical_path) {
                  return a.manifest.canonical_path < b.manifest.canonical_path;
                }
                if (a.manifest.family != b.manifest.family) {
                  return a.manifest.family < b.manifest.family;
                }
                return a.component_id < b.component_id;
              });

    if (matched.empty()) {
      receipts.push_back(cutover_receipt_entry_t{
          .component_dsl_key = t.key,
          .action = t.action,
          .dsl_path = t.resolved_norm,
          .family = target_family,
          .status = "skipped",
          .message =
              "no active catalog component matched this DSL path; no cutover",
      });
      ++skipped_count;
      continue;
    }

    matched_components_count += matched.size();
    for (const auto& active_component : matched) {
      cutover_receipt_entry_t row{};
      row.component_dsl_key = t.key;
      row.action = t.action;
      row.dsl_path = t.resolved_norm;
      row.canonical_path = active_component.manifest.canonical_path;
      row.family = active_component.manifest.family;
      row.old_hashimyei = active_component.manifest.hashimyei_identity.name;
      row.new_hashimyei = active_component.manifest.hashimyei_identity.name;

      const std::string pointer_canonical =
          !active_component.manifest.hashimyei_identity.name.empty() &&
                  !active_component.manifest.family.empty()
              ? active_component.manifest.family
              : active_component.manifest.canonical_path;
      const std::string pointer_key = pointer_canonical + "|" +
                                      active_component.manifest.family + "|" +
                                      cuwacunu::hero::hashimyei::contract_hash_from_identity(
                                          active_component.manifest.contract_identity);
      if (!processed_component_pointer_keys.insert(pointer_key).second) {
        row.status = "noop";
        row.message =
            "component pointer already processed in this save; duplicate cutover suppressed";
        receipts.push_back(std::move(row));
        ++noop_count;
        continue;
      }

      const std::string& old_dsl_key =
          component_dsl_source_key(active_component);
      const bool dsl_path_changed =
          old_dsl_key != t.resolved_norm && old_dsl_key != target_filename;
      const bool dsl_sha_changed =
          lowercase_copy(
              active_component.manifest.founding_dsl_source_sha256_hex) !=
          lowercase_copy(t.dsl_sha256_hex);

      if (!dsl_path_changed && !dsl_sha_changed) {
        row.status = "noop";
        row.message = "active component already matches requested DSL payload";
        receipts.push_back(std::move(row));
        ++noop_count;
        continue;
      }

      cuwacunu::hero::hashimyei::component_manifest_t next_manifest =
          active_component.manifest;
      next_manifest.schema = cuwacunu::hashimyei::kComponentManifestSchemaV2;
      next_manifest.parent_identity = active_component.manifest.hashimyei_identity;
      next_manifest.revision_reason = "dsl_change";
      next_manifest.founding_revision_id = founding_revision_id;
      next_manifest.founding_dsl_source_path =
          target_filename.empty() ? t.resolved_norm : target_filename;
      next_manifest.founding_dsl_source_sha256_hex = t.dsl_sha256_hex;
      next_manifest.lineage_state = "active";
      next_manifest.replaced_by.clear();
      next_manifest.updated_at_ms = deterministic_cutover_ts_ms(
          founding_revision_id + "|" +
          active_component.manifest.canonical_path + "|" +
          t.dsl_sha256_hex + "|" + t.resolved_norm);
      next_manifest.created_at_ms = next_manifest.updated_at_ms;

      if (!active_component.manifest.hashimyei_identity.name.empty()) {
        std::string next_hashimyei = synthesize_hashimyei_from_sha(
            t.dsl_sha256_hex, active_component.manifest.hashimyei_identity.name);
        if (next_hashimyei ==
            active_component.manifest.hashimyei_identity.name) {
          next_hashimyei =
              "0x" + hex_u64(fnv1a64(founding_revision_id + "|" +
                                     active_component.manifest.canonical_path +
                                     "|" + t.dsl_sha256_hex));
        }
        next_manifest.hashimyei_identity.name = next_hashimyei;
        std::uint64_t parsed_ordinal = 0;
        if (!cuwacunu::hashimyei::parse_hex_hash_name_ordinal(next_hashimyei,
                                                               &parsed_ordinal)) {
          row.status = "error";
          row.new_hashimyei = next_hashimyei;
          row.message = "generated hashimyei is not a valid 0x... ordinal";
          receipts.push_back(std::move(row));
          ++error_count;
          continue;
        }
        next_manifest.hashimyei_identity.schema =
            cuwacunu::hashimyei::kIdentitySchemaV2;
        next_manifest.hashimyei_identity.kind =
            cuwacunu::hashimyei::hashimyei_kind_e::TSIEMENE;
        next_manifest.hashimyei_identity.ordinal = parsed_ordinal;
        next_manifest.hashimyei_identity.hash_sha256_hex.clear();
        if (!next_manifest.family.empty()) {
          next_manifest.canonical_path =
              next_manifest.family + "." + next_hashimyei;
        }
      }

      const std::string dedup_seed =
          next_manifest.canonical_path + "|" + next_manifest.family + "|" +
          next_manifest.founding_dsl_source_sha256_hex + "|" +
          next_manifest.founding_dsl_source_path +
          "|" + next_manifest.founding_revision_id;
      if (!emitted_revision_dedup.insert(dedup_seed).second) {
        row.status = "noop";
        row.new_hashimyei = next_manifest.hashimyei_identity.name;
        row.message = "duplicate cutover payload deduplicated in this save";
        receipts.push_back(std::move(row));
        ++noop_count;
        continue;
      }

      const auto contract_snapshot =
          cuwacunu::iitepi::contract_space_t::contract_itself(
              cuwacunu::hero::hashimyei::contract_hash_from_identity(
                  next_manifest.contract_identity));
      std::string founding_bundle_error{};
      if (!write_component_founding_dsl_bundle_snapshot(
              store_root, next_manifest, t.resolved_norm, contract_snapshot,
              &founding_bundle_error)) {
        row.status = "error";
        row.new_hashimyei = next_manifest.hashimyei_identity.name;
        row.message = "cannot persist founding DSL bundle snapshot: " +
                      founding_bundle_error;
        receipts.push_back(std::move(row));
        ++error_count;
        continue;
      }

      std::string register_error;
      std::string component_id;
      bool inserted = false;
      if (!catalog.register_component_manifest(next_manifest, &component_id, &inserted,
                                               &register_error)) {
        row.status = "error";
        row.new_hashimyei = next_manifest.hashimyei_identity.name;
        row.message = register_error;
        receipts.push_back(std::move(row));
        ++error_count;
        continue;
      }

      row.new_hashimyei = next_manifest.hashimyei_identity.name;
      row.component_id = component_id;
      row.inserted = inserted;
      row.status = inserted ? "applied" : "noop";
      row.message = inserted ? "registered new immutable revision and switched active pointer"
                             : "manifest already present; pointer unchanged";
      receipts.push_back(std::move(row));
      if (inserted) {
        ++applied_count;
      } else {
        ++noop_count;
      }
    }
  }

  if (catalog_opened) {
    std::string close_error;
    if (!catalog.close(&close_error) && catalog_error.empty()) {
      catalog_error = "catalog close failed: " + close_error;
    }
  }

  std::ostringstream out;
  out << "{"
      << "\"founding_revision_id\":"
      << json_quote(founding_revision_id) << ","
      << "\"generated_at_ms\":" << generated_at_ms << ","
      << "\"cutover_triggered\":" << bool_json(!touched.empty()) << ","
      << "\"path\":" << json_quote(config_path) << ","
      << "\"catalog_path\":" << json_quote(catalog_path.string()) << ","
      << "\"catalog_opened\":" << bool_json(catalog_opened) << ","
      << "\"catalog_error\":" << json_quote(catalog_error) << ","
      << "\"touched_component_dsl_keys\":[";
  for (std::size_t i = 0; i < touched.size(); ++i) {
    if (i != 0) out << ",";
    out << json_quote(touched[i].key);
  }
  out << "],\"matched_component_count\":" << matched_components_count
      << ",\"applied_count\":" << applied_count << ",\"noop_count\":"
      << noop_count << ",\"skipped_count\":" << skipped_count
      << ",\"error_count\":" << error_count << ",\"receipts\":[";
  for (std::size_t i = 0; i < receipts.size(); ++i) {
    if (i != 0) out << ",";
    out << "{"
        << "\"component_dsl_key\":" << json_quote(receipts[i].component_dsl_key)
        << ",\"action\":" << json_quote(receipts[i].action)
        << ",\"dsl_path\":" << json_quote(receipts[i].dsl_path)
        << ",\"canonical_path\":" << json_quote(receipts[i].canonical_path)
        << ",\"family\":" << json_quote(receipts[i].family)
        << ",\"old_hashimyei\":" << json_quote(receipts[i].old_hashimyei)
        << ",\"new_hashimyei\":" << json_quote(receipts[i].new_hashimyei)
        << ",\"component_id\":" << json_quote(receipts[i].component_id)
        << ",\"status\":" << json_quote(receipts[i].status)
        << ",\"inserted\":" << bool_json(receipts[i].inserted)
        << ",\"message\":" << json_quote(receipts[i].message)
        << ",\"founding_revision_id\":"
        << json_quote(founding_revision_id) << "}";
  }
  out << "]}";
  return out.str();
}

[[nodiscard]] std::string detect_error_tag(std::string_view message) {
  const std::size_t colon = message.find(':');
  if (colon == std::string_view::npos || colon == 0) return {};
  const std::string_view candidate = message.substr(0, colon);
  for (const char c : candidate) {
    if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') {
      continue;
    }
    return {};
  }
  if (candidate.find("E_") != 0) return {};
  return std::string(candidate);
}

[[nodiscard]] std::string make_text_content_result_json(std::string_view text,
                                                        bool is_error,
                                                        int error_code,
                                                        std::string_view error_tag) {
  std::ostringstream out;
  out << "{\"content\":[{\"type\":\"text\",\"text\":" << json_quote(text)
      << "}],\"isError\":" << bool_json(is_error);
  if (is_error) {
    out << ",\"structuredContent\":{\"ok\":false"
        << ",\"error_code\":" << error_code
        << ",\"error_tag\":" << json_quote(error_tag)
        << ",\"message\":" << json_quote(text) << "}";
  }
  out << "}";
  return out.str();
}

[[nodiscard]] std::string make_tool_success_result_json(
    std::string_view tool_name, std::string_view structured_content_json) {
  std::ostringstream out;
  out << "{\"content\":[{\"type\":\"text\",\"text\":"
      << json_quote(std::string("tool ") + std::string(tool_name) +
                    " executed")
      << "}],\"structuredContent\":" << structured_content_json
      << ",\"isError\":false}";
  return out.str();
}

[[nodiscard]] std::string build_status_result(
    const cuwacunu::hero::mcp::hero_config_store_t& store) {
  const auto errors = store.validate();
  std::ostringstream out;
  out << "{";
  out << "\"valid\":" << bool_json(errors.empty()) << ",";
  out << "\"config_path\":" << json_quote(store.config_path()) << ",";
  out << "\"dirty\":" << bool_json(store.dirty()) << ",";
  out << "\"from_template\":" << bool_json(store.from_template()) << ",";
  out << "\"protocol_layer\":"
      << json_quote(store.get_or_default("protocol_layer")) << ",";
  out << "\"backup_enabled\":"
      << json_quote(store.get_or_default("backup_enabled")) << ",";
  out << "\"backup_dir\":" << json_quote(store.get_or_default("backup_dir"))
      << ",";
  out << "\"backup_max_entries\":"
      << json_quote(store.get_or_default("backup_max_entries")) << ",";
  out << "\"errors\":[";
  for (std::size_t i = 0; i < errors.size(); ++i) {
    if (i != 0) out << ",";
    out << json_quote(errors[i]);
  }
  out << "]}";
  return out.str();
}

[[nodiscard]] std::string build_save_preview_result(
    const cuwacunu::hero::mcp::hero_config_store_t::save_preview_t& preview,
    bool include_text) {
  const bool format_only = preview.text_changed && preview.diffs.empty();
  std::ostringstream out;
  out << "{";
  out << "\"file_exists\":" << bool_json(preview.file_exists) << ",";
  out << "\"text_changed\":" << bool_json(preview.text_changed) << ",";
  out << "\"format_only\":" << bool_json(format_only) << ",";
  out << "\"has_changes\":" << bool_json(preview.has_changes) << ",";
  out << "\"diff_count\":" << preview.diff_count << ",";
  out << "\"diffs\":[";
  for (std::size_t i = 0; i < preview.diffs.size(); ++i) {
    const auto& d = preview.diffs[i];
    if (i != 0) out << ",";
    out << "{"
        << "\"key\":" << json_quote(d.key) << ","
        << "\"action\":" << json_quote(d.action) << ","
        << "\"before_domain\":" << json_quote(d.before_declared_domain) << ","
        << "\"before_type\":" << json_quote(d.before_declared_type) << ","
        << "\"before_value\":" << json_quote(d.before_value) << ","
        << "\"after_domain\":" << json_quote(d.after_declared_domain) << ","
        << "\"after_type\":" << json_quote(d.after_declared_type) << ","
        << "\"after_value\":" << json_quote(d.after_value) << "}";
  }
  out << "]";
  if (include_text) {
    out << ",\"current_text\":" << json_quote(preview.current_text);
    out << ",\"proposed_text\":" << json_quote(preview.proposed_text);
  }
  out << "}";
  return out.str();
}

using hero_config_tool_handler_t = bool (*)(
    std::string_view tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store,
    std::string* out_result_json, int* out_error_code,
    std::string* out_error_message);

struct hero_config_tool_descriptor_t {
  std::string_view name;
  std::string_view description;
  std::string_view input_schema_json;
  hero_config_tool_handler_t handler;
};

[[nodiscard]] bool handle_tool_status(
    std::string_view tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store,
    std::string* out_result_json, int* out_error_code,
    std::string* out_error_message);
[[nodiscard]] bool handle_tool_schema(
    std::string_view tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store,
    std::string* out_result_json, int* out_error_code,
    std::string* out_error_message);
[[nodiscard]] bool handle_tool_show(
    std::string_view tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store,
    std::string* out_result_json, int* out_error_code,
    std::string* out_error_message);
[[nodiscard]] bool handle_tool_get(
    std::string_view tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store,
    std::string* out_result_json, int* out_error_code,
    std::string* out_error_message);
[[nodiscard]] bool handle_tool_set(
    std::string_view tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store,
    std::string* out_result_json, int* out_error_code,
    std::string* out_error_message);
[[nodiscard]] bool handle_tool_dsl_get(
    std::string_view tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store,
    std::string* out_result_json, int* out_error_code,
    std::string* out_error_message);
[[nodiscard]] bool handle_tool_dsl_set(
    std::string_view tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store,
    std::string* out_result_json, int* out_error_code,
    std::string* out_error_message);
[[nodiscard]] bool handle_tool_validate(
    std::string_view tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store,
    std::string* out_result_json, int* out_error_code,
    std::string* out_error_message);
[[nodiscard]] bool handle_tool_diff(
    std::string_view tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store,
    std::string* out_result_json, int* out_error_code,
    std::string* out_error_message);
[[nodiscard]] bool handle_tool_backups(
    std::string_view tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store,
    std::string* out_result_json, int* out_error_code,
    std::string* out_error_message);
[[nodiscard]] bool handle_tool_rollback(
    std::string_view tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store,
    std::string* out_result_json, int* out_error_code,
    std::string* out_error_message);
[[nodiscard]] bool handle_tool_save(
    std::string_view tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store,
    std::string* out_result_json, int* out_error_code,
    std::string* out_error_message);
[[nodiscard]] bool handle_tool_reload(
    std::string_view tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store,
    std::string* out_result_json, int* out_error_code,
    std::string* out_error_message);
[[nodiscard]] bool handle_tool_dev_nuke_reset(
    std::string_view tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store,
    std::string* out_result_json, int* out_error_code,
    std::string* out_error_message);

constexpr hero_config_tool_descriptor_t kHeroConfigTools[] = {
#define HERO_CONFIG_TOOL(NAME, DESCRIPTION, INPUT_SCHEMA_JSON, HANDLER) \
  hero_config_tool_descriptor_t{NAME, DESCRIPTION, INPUT_SCHEMA_JSON, HANDLER},
#include "hero/config_hero/hero_config_tools.def"
#undef HERO_CONFIG_TOOL
};

[[nodiscard]] const hero_config_tool_descriptor_t* find_tool_descriptor(
    std::string_view name) {
  for (const auto& descriptor : kHeroConfigTools) {
    if (descriptor.name == name) return &descriptor;
  }
  return nullptr;
}

[[nodiscard]] std::string build_tools_list_result_json_impl() {
  std::ostringstream out;
  out << "{\"tools\":[";
  for (std::size_t i = 0; i < std::size(kHeroConfigTools); ++i) {
    const auto& tool = kHeroConfigTools[i];
    if (i != 0) out << ",";
    out << "{"
        << "\"name\":" << json_quote(tool.name) << ","
        << "\"description\":" << json_quote(tool.description) << ","
        << "\"inputSchema\":" << tool.input_schema_json << "}";
  }
  out << "]}";
  return out.str();
}

[[nodiscard]] std::string build_tools_list_human_text_impl() {
  std::ostringstream out;
  for (const auto& tool : kHeroConfigTools) {
    out << tool.name << "\t" << tool.description << "\n";
  }
  return out.str();
}

[[nodiscard]] std::string path_vector_json(
    const std::vector<std::filesystem::path>& paths) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < paths.size(); ++i) {
    if (i != 0) out << ",";
    out << json_quote(paths[i].string());
  }
  out << "]";
  return out.str();
}

[[nodiscard]] bool handle_tool_status(
    std::string_view tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store,
    std::string* out_result_json, int* out_error_code,
    std::string* out_error_message) {
  (void)tool_name;
  (void)request_json;
  (void)out_error_code;
  (void)out_error_message;
  if (out_result_json) *out_result_json = build_status_result(*store);
  return true;
}

[[nodiscard]] bool handle_tool_schema(
    std::string_view tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store,
    std::string* out_result_json, int* out_error_code,
    std::string* out_error_message) {
  (void)tool_name;
  (void)request_json;
  (void)store;
  (void)out_error_code;
  (void)out_error_message;
  std::ostringstream out;
  out << "{\"keys\":[";
  for (std::size_t i = 0;
       i < cuwacunu::hero::config::kRuntimeKeyDescriptors.size(); ++i) {
    const auto& d = cuwacunu::hero::config::kRuntimeKeyDescriptors[i];
    if (i != 0) out << ",";
    out << "{"
        << "\"key\":" << json_quote(d.key) << ","
        << "\"type\":" << json_quote(d.type) << ","
        << "\"required\":" << bool_json(d.required) << ","
        << "\"default\":" << json_quote(d.default_value) << ","
        << "\"range\":" << json_quote(d.range) << ","
        << "\"allowed\":" << json_quote(d.allowed) << "}";
  }
  out << "]}";
  if (out_result_json) *out_result_json = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_show(
    std::string_view tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store,
    std::string* out_result_json, int* out_error_code,
    std::string* out_error_message) {
  (void)tool_name;
  (void)request_json;
  (void)out_error_code;
  (void)out_error_message;
  const auto entries = store->entries_snapshot();
  std::ostringstream out;
  out << "{\"entries\":[";
  for (std::size_t i = 0; i < entries.size(); ++i) {
    if (i != 0) out << ",";
    out << "{"
        << "\"key\":" << json_quote(entries[i].key) << ","
        << "\"domain\":" << json_quote(entries[i].declared_domain) << ","
        << "\"type\":" << json_quote(entries[i].declared_type) << ","
        << "\"value\":" << json_quote(entries[i].value) << "}";
  }
  out << "]}";
  if (out_result_json) *out_result_json = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_get(
    std::string_view tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store,
    std::string* out_result_json, int* out_error_code,
    std::string* out_error_message) {
  std::string key;
  if (!extract_json_string_field(request_json, "key", &key) || key.empty()) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument key";
    }
    return false;
  }
  const auto value = store->get_value(key);
  if (!value.has_value()) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) *out_error_message = "key not found: " + key;
    return false;
  }
  if (out_result_json) {
    *out_result_json = "{\"key\":" + json_quote(key) + ",\"value\":" +
                       json_quote(*value) + "}";
  }
  return true;
}

[[nodiscard]] bool handle_tool_set(
    std::string_view tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store,
    std::string* out_result_json, int* out_error_code,
    std::string* out_error_message) {
  std::string key;
  std::string value;
  if (!extract_json_string_field(request_json, "key", &key) || key.empty()) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument key";
    }
    return false;
  }
  if (!extract_json_string_field(request_json, "value", &value)) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument value";
    }
    return false;
  }
  std::string err;
  if (!store->set_value(key, value, &err)) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) *out_error_message = err;
    return false;
  }
  if (out_result_json) {
    *out_result_json = "{\"updated\":true,\"key\":" + json_quote(key) +
                       ",\"value\":" + json_quote(value) + "}";
  }
  return true;
}

[[nodiscard]] bool handle_tool_dsl_get(
    std::string_view tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store,
    std::string* out_result_json, int* out_error_code,
    std::string* out_error_message) {
  std::string dsl_path_raw;
  std::string key;
  if (!extract_json_string_field(request_json, "path", &dsl_path_raw) ||
      dsl_path_raw.empty()) {
    (void)extract_json_string_field(request_json, "dsl_path", &dsl_path_raw);
  }
  if (dsl_path_raw.empty()) dsl_path_raw = store->config_path();
  if (!extract_json_string_field(request_json, "key", &key) || key.empty()) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument key";
    }
    return false;
  }

  std::filesystem::path dsl_path{};
  std::string err{};
  dsl_path_resolution_error_t dsl_err = dsl_path_resolution_error_t::kNone;
  if (!resolve_dsl_path_with_scope(*store, dsl_path_raw, &dsl_path, &err,
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

  std::istringstream input(text);
  std::string line{};
  std::string value{};
  std::string declared_type{};
  std::size_t line_no = 0;
  bool found = false;
  while (std::getline(input, line)) {
    ++line_no;
    const parsed_dsl_line_t parsed = parse_dsl_line(line);
    if (!parsed.has_assignment) continue;
    if (parsed.key != key) continue;
    value = parsed.value;
    declared_type = parsed.declared_type;
    found = true;
    break;
  }
  if (!found) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message =
          "key not found in dsl file: " + key + " @ " + dsl_path.string();
    }
    return false;
  }
  if (out_result_json) {
    std::ostringstream out;
    out << "{\"dsl_path\":" << json_quote(dsl_path.string())
        << ",\"key\":" << json_quote(key)
        << ",\"declared_type\":" << json_quote(declared_type)
        << ",\"value\":" << json_quote(value)
        << ",\"line\":" << line_no << "}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool handle_tool_dsl_set(
    std::string_view tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store,
    std::string* out_result_json, int* out_error_code,
    std::string* out_error_message) {
  std::string dsl_path_raw;
  std::string key;
  std::string value;
  if (!extract_json_string_field(request_json, "path", &dsl_path_raw) ||
      dsl_path_raw.empty()) {
    (void)extract_json_string_field(request_json, "dsl_path", &dsl_path_raw);
  }
  if (dsl_path_raw.empty()) dsl_path_raw = store->config_path();
  if (!extract_json_string_field(request_json, "key", &key) || key.empty()) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument key";
    }
    return false;
  }
  if (!extract_json_string_field(request_json, "value", &value)) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message = std::string(tool_name) + " requires argument value";
    }
    return false;
  }

  std::filesystem::path dsl_path{};
  std::string err{};
  dsl_path_resolution_error_t dsl_err = dsl_path_resolution_error_t::kNone;
  if (!resolve_dsl_path_with_scope(*store, dsl_path_raw, &dsl_path, &err,
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

  std::istringstream input(text);
  std::vector<std::string> lines{};
  std::string line{};
  while (std::getline(input, line)) {
    lines.push_back(line);
  }

  bool updated = false;
  std::size_t updated_line = 0;
  std::string before_value{};
  for (std::size_t i = 0; i < lines.size(); ++i) {
    const parsed_dsl_line_t parsed = parse_dsl_line(lines[i]);
    if (!parsed.has_assignment) continue;
    if (parsed.key != key) continue;
    before_value = parsed.value;
    std::string next = parsed.leading + parsed.lhs + " = " + value;
    if (!parsed.comment.empty()) next += " " + parsed.comment;
    lines[i] = std::move(next);
    updated = true;
    updated_line = i + 1;
    break;
  }
  if (!updated) {
    if (out_error_code) *out_error_code = -32602;
    if (out_error_message) {
      *out_error_message =
          "key not found in dsl file: " + key + " @ " + dsl_path.string();
    }
    return false;
  }

  std::ostringstream rebuilt{};
  for (std::size_t i = 0; i < lines.size(); ++i) {
    rebuilt << lines[i];
    if (i + 1 < lines.size()) rebuilt << "\n";
  }
  if (text.empty() || text.back() == '\n') rebuilt << "\n";

  if (!write_text_file_atomic(dsl_path.string(), rebuilt.str(), &err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) *out_error_message = err;
    return false;
  }
  if (out_result_json) {
    std::ostringstream out;
    out << "{\"updated\":true,\"dsl_path\":"
        << json_quote(dsl_path.string()) << ",\"key\":" << json_quote(key)
        << ",\"before_value\":" << json_quote(before_value)
        << ",\"value\":" << json_quote(value)
        << ",\"line\":" << updated_line << "}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool handle_tool_validate(
    std::string_view tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store,
    std::string* out_result_json, int* out_error_code,
    std::string* out_error_message) {
  (void)tool_name;
  (void)request_json;
  (void)out_error_code;
  (void)out_error_message;
  const auto errors = store->validate();
  std::ostringstream out;
  out << "{\"valid\":" << bool_json(errors.empty()) << ",\"errors\":[";
  for (std::size_t i = 0; i < errors.size(); ++i) {
    if (i != 0) out << ",";
    out << json_quote(errors[i]);
  }
  out << "]}";
  if (out_result_json) *out_result_json = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_diff(
    std::string_view tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store,
    std::string* out_result_json, int* out_error_code,
    std::string* out_error_message) {
  bool include_text = false;
  bool parsed_include_text = false;
  if (extract_json_raw_field(request_json, "include_text", nullptr)) {
    if (!extract_json_bool_field(request_json, "include_text", &include_text)) {
      if (out_error_code) *out_error_code = -32602;
      if (out_error_message) {
        *out_error_message =
            std::string(tool_name) + " include_text must be boolean";
      }
      return false;
    }
    parsed_include_text = true;
  }
  if (!parsed_include_text &&
      extract_json_bool_field(request_json, "includeText", &include_text)) {
    parsed_include_text = true;
  }
  (void)parsed_include_text;

  cuwacunu::hero::mcp::hero_config_store_t::save_preview_t preview;
  std::string err;
  if (!store->preview_save(&preview, &err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) *out_error_message = err;
    return false;
  }
  if (out_result_json) {
    *out_result_json = build_save_preview_result(preview, include_text);
  }
  return true;
}

[[nodiscard]] bool handle_tool_backups(
    std::string_view tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store,
    std::string* out_result_json, int* out_error_code,
    std::string* out_error_message) {
  (void)tool_name;
  (void)request_json;
  std::vector<cuwacunu::hero::mcp::hero_config_store_t::backup_entry_t>
      backups;
  std::string err;
  if (!store->list_backups(&backups, &err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) *out_error_message = err;
    return false;
  }
  std::ostringstream out;
  out << "{\"count\":" << backups.size() << ",\"backups\":[";
  for (std::size_t i = 0; i < backups.size(); ++i) {
    if (i != 0) out << ",";
    out << "{"
        << "\"index\":" << i << ","
        << "\"filename\":" << json_quote(backups[i].filename) << ","
        << "\"path\":" << json_quote(backups[i].path) << "}";
  }
  out << "]}";
  if (out_result_json) *out_result_json = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_rollback(
    std::string_view tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store,
    std::string* out_result_json, int* out_error_code,
    std::string* out_error_message) {
  std::string selector;
  if (extract_json_raw_field(request_json, "backup", nullptr)) {
    if (!extract_json_string_field(request_json, "backup", &selector)) {
      if (out_error_code) *out_error_code = -32602;
      if (out_error_message) {
        *out_error_message = std::string(tool_name) + " backup must be string";
      }
      return false;
    }
  }

  std::string selected;
  std::string err;
  if (!store->rollback_to_backup(selector, &selected, &err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) *out_error_message = err;
    return false;
  }
  if (out_result_json) {
    *out_result_json = "{\"rolled_back\":true,\"path\":" +
                       json_quote(store->config_path()) +
                       ",\"selected_backup\":" + json_quote(selected) + "}";
  }
  return true;
}

[[nodiscard]] bool handle_tool_save(
    std::string_view tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store,
    std::string* out_result_json, int* out_error_code,
    std::string* out_error_message) {
  (void)tool_name;
  (void)request_json;
  cuwacunu::hero::mcp::hero_config_store_t::save_preview_t preview;
  std::string err;
  if (!store->preview_save(&preview, &err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) *out_error_message = err;
    return false;
  }
  if (!store->save(&err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) *out_error_message = err;
    return false;
  }
  if (out_result_json) {
    *out_result_json = "{\"saved\":true,\"path\":" +
                       json_quote(store->config_path()) + ",\"cutover\":" +
                       build_cutover_receipt_json_impl(
                           preview, store->config_path(),
                           store->global_config_path()) +
                       "}";
  }
  return true;
}

[[nodiscard]] bool handle_tool_reload(
    std::string_view tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store,
    std::string* out_result_json, int* out_error_code,
    std::string* out_error_message) {
  (void)tool_name;
  (void)request_json;
  std::string err;
  if (!store->load(&err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) *out_error_message = err;
    return false;
  }
  if (out_result_json) {
    *out_result_json = "{\"reloaded\":true,\"path\":" +
                       json_quote(store->config_path()) + "}";
  }
  return true;
}

[[nodiscard]] bool handle_tool_dev_nuke_reset(
    std::string_view tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store,
    std::string* out_result_json, int* out_error_code,
    std::string* out_error_message) {
  (void)tool_name;
  (void)request_json;
  cuwacunu::hero::runtime_dev::runtime_reset_targets_t targets{};
  std::string err;
  if (!cuwacunu::hero::runtime_dev::resolve_runtime_reset_targets_from_global_config(
          store->global_config_path(), &targets, &err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) *out_error_message = err;
    return false;
  }

  const bool dirty_before_reset = store->dirty();
  cuwacunu::hero::runtime_dev::runtime_reset_result_t reset_result{};
  if (!cuwacunu::hero::runtime_dev::reset_runtime_state(targets, &reset_result,
                                                        &err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) *out_error_message = err;
    return false;
  }

  if (!store->load(&err)) {
    if (out_error_code) *out_error_code = -32603;
    if (out_error_message) {
      *out_error_message =
          "runtime state reset succeeded but config reload failed: " + err;
    }
    return false;
  }

  if (out_result_json) {
    std::ostringstream out;
    out << "{"
        << "\"reset\":true,"
        << "\"config_path\":" << json_quote(store->config_path()) << ","
        << "\"global_config_path\":"
        << json_quote(store->global_config_path()) << ","
        << "\"dirty_before_reset\":" << bool_json(dirty_before_reset) << ","
        << "\"resolved_from_saved_global_config\":true,"
        << "\"hashimyei_hero_dsl_path\":"
        << json_quote(targets.hashimyei_hero_dsl_path.string()) << ","
        << "\"lattice_hero_dsl_path\":"
        << json_quote(targets.lattice_hero_dsl_path.string()) << ","
        << "\"runtime_hero_dsl_path\":"
        << json_quote(targets.runtime_hero_dsl_path.string()) << ","
        << "\"runtime_campaigns_root\":"
        << json_quote(targets.runtime_campaigns_root.string()) << ","
        << "\"target_store_roots\":" << path_vector_json(targets.store_roots)
        << ","
        << "\"target_catalog_paths\":"
        << path_vector_json(targets.catalog_paths) << ","
        << "\"removed_store_roots\":"
        << path_vector_json(reset_result.removed_store_roots) << ","
        << "\"removed_catalog_paths\":"
        << path_vector_json(reset_result.removed_catalog_paths) << ","
        << "\"removed_store_entries\":" << reset_result.removed_store_entries
        << ",\"reloaded\":true"
        << "}";
    *out_result_json = out.str();
  }
  return true;
}

[[nodiscard]] bool dispatch_tool_jsonrpc(
    const std::string& tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store, std::string* out_result_json,
    int* out_error_code, std::string* out_error_message) {
  if (out_error_code) *out_error_code = -32603;
  if (out_error_message) *out_error_message = "internal error";

  const auto* descriptor = find_tool_descriptor(tool_name);
  if (descriptor != nullptr) {
    return descriptor->handler(tool_name, request_json, store, out_result_json,
                               out_error_code, out_error_message);
  }
  if (tool_name == "hero.config.ask" || tool_name == "hero.config.fix") {
    if (out_error_code) *out_error_code = -32601;
    if (out_error_message) *out_error_message = kDeterministicOnlyMessage;
    return false;
  }

  if (out_error_code) *out_error_code = -32601;
  if (out_error_message) *out_error_message = "unknown tool: " + tool_name;
  return false;
}

}  // namespace

namespace cuwacunu {
namespace hero {
namespace mcp {

[[nodiscard]] std::string build_tools_list_result_json() {
  return build_tools_list_result_json_impl();
}

[[nodiscard]] std::string build_tools_list_human_text() {
  return build_tools_list_human_text_impl();
}

bool execute_tool_json(const std::string& tool_name, std::string arguments_json,
                       hero_config_store_t* store,
                       std::string* out_tool_result_json,
                       std::string* out_error_message) {
  if (out_tool_result_json) out_tool_result_json->clear();
  if (out_error_message) out_error_message->clear();

  if (!store) {
    if (out_error_message) *out_error_message = "store is null";
    return false;
  }

  const std::string trimmed_name = trim_ascii(tool_name);
  if (trimmed_name.empty()) {
    if (out_error_message) *out_error_message = "tool name is empty";
    return false;
  }

  arguments_json = trim_ascii(arguments_json);
  if (arguments_json.empty()) arguments_json = "{}";
  if (arguments_json.front() != '{') {
    if (out_error_message) {
      *out_error_message = "tool arguments must be a JSON object";
    }
    return false;
  }

  std::string structured_result_json;
  int error_code = -32603;
  std::string error_message;
  if (!dispatch_tool_jsonrpc(trimmed_name, arguments_json, store,
                             &structured_result_json, &error_code,
                             &error_message)) {
    if (out_tool_result_json) {
      const std::string text = std::string("tool error: ") + error_message;
      *out_tool_result_json = make_text_content_result_json(
          text, true, error_code, detect_error_tag(error_message));
    }
    if (out_error_message) *out_error_message = error_message;
    return false;
  }

  if (out_tool_result_json) {
    *out_tool_result_json =
        make_tool_success_result_json(trimmed_name, structured_result_json);
  }
  return true;
}

void run_jsonrpc_stdio_loop(hero_config_store_t* store) {
  std::string payload;
  while (true) {
    bool used_content_length = false;
    if (!read_next_jsonrpc_message(&std::cin, &payload,
                                   &used_content_length)) {
      break;
    }
    g_jsonrpc_use_content_length_framing = used_content_length;

    const std::string trimmed = trim_ascii(payload);
    if (trimmed.empty()) continue;

    const bool has_id = has_json_field(trimmed, "id");

    std::string id_json;
    if (!extract_json_id_field(trimmed, &id_json)) {
      if (has_id) {
        emit_jsonrpc_error("null", -32700, "invalid request: unable to parse id");
      }
      continue;
    }

    std::string method;
    if (!extract_json_string_field(trimmed, "method", &method) ||
        method.empty()) {
      if (has_id) {
        emit_jsonrpc_error(id_json, -32600, "invalid request: missing method");
      }
      continue;
    }

    if (method.rfind("notifications/", 0) == 0) {
      continue;
    }

    if (method == "initialize") {
      std::string protocol_version = kDefaultMcpProtocolVersion;
      std::string params_json;
      if (extract_json_object_field(trimmed, "params", &params_json)) {
        std::string protocol_candidate;
        if (extract_json_string_field(params_json, "protocolVersion",
                                      &protocol_candidate) &&
            !protocol_candidate.empty()) {
          protocol_version = protocol_candidate;
        }
      } else {
        // Fallback for initialize requests that omit the params object.
        std::string protocol_candidate;
        if (extract_json_string_field(trimmed, "protocolVersion",
                                      &protocol_candidate) &&
            !protocol_candidate.empty()) {
          protocol_version = protocol_candidate;
        }
      }
      if (has_id) {
        emit_jsonrpc_result(
            id_json, std::string("{\"protocolVersion\":") +
                         json_quote(protocol_version) +
                         ",\"capabilities\":{\"tools\":{\"listChanged\":false}},"
                         "\"serverInfo\":{\"name\":" +
                         json_quote(kMcpServerName) +
                         ",\"version\":" + json_quote(kMcpServerVersion) +
                         "},\"instructions\":" +
                         json_quote(kMcpInitializeInstructions) + "}");
      }
      continue;
    }
    if (method == "ping") {
      if (has_id) {
        emit_jsonrpc_result(id_json, "{}");
      }
      continue;
    }
    if (method == "tools/list") {
      if (has_id) {
        emit_jsonrpc_result(id_json, build_tools_list_result_json_impl());
      }
      continue;
    }

    if (method == "tools/call" || method.rfind("hero.", 0) == 0) {
      std::string tool_name = method;
      std::string dispatch_json = trimmed;
      if (method == "tools/call") {
        std::string params_json;
        if (!extract_json_object_field(trimmed, "params", &params_json)) {
          if (has_id) {
            emit_jsonrpc_error(id_json, -32602,
                               "tools/call requires params object");
          }
          continue;
        }
        if (!extract_json_string_field(params_json, "name", &tool_name) ||
            tool_name.empty()) {
          if (has_id) {
            emit_jsonrpc_error(id_json, -32602,
                               "tools/call requires params.name");
          }
          continue;
        }

        std::string arguments_json;
        if (extract_json_raw_field(params_json, "arguments", &arguments_json)) {
          arguments_json = trim_ascii(arguments_json);
          if (arguments_json.empty() || arguments_json.front() != '{') {
            if (has_id) {
              emit_jsonrpc_error(id_json, -32602,
                                 "tools/call requires params.arguments object");
            }
            continue;
          }
          dispatch_json = arguments_json;
        } else {
          dispatch_json = "{}";
        }
      } else {
        std::string params_json;
        if (extract_json_object_field(trimmed, "params", &params_json)) {
          dispatch_json = params_json;
        }
      }

      std::string result_json;
      int error_code = -32603;
      std::string error_message;
      if (!dispatch_tool_jsonrpc(tool_name, dispatch_json, store, &result_json,
                                 &error_code, &error_message)) {
        if (has_id) {
          if (method == "tools/call") {
            const std::string text = std::string("tool error: ") + error_message;
            emit_jsonrpc_result(id_json,
                                make_text_content_result_json(
                                    text, true, error_code,
                                    detect_error_tag(error_message)));
          } else {
            emit_jsonrpc_error(id_json, error_code, error_message);
          }
        }
        continue;
      }
      if (has_id) {
        if (method == "tools/call") {
          emit_jsonrpc_result(
              id_json, make_tool_success_result_json(tool_name, result_json));
        } else {
          emit_jsonrpc_result(id_json, result_json);
        }
      }
      continue;
    }

    if (has_id) {
      emit_jsonrpc_error(id_json, -32601, "method not found: " + method);
    }
  }
}

}  // namespace mcp
}  // namespace hero
}  // namespace cuwacunu
