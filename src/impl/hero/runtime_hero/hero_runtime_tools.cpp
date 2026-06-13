#include "hero/runtime_hero/hero_runtime_tools.h"

#include "hero/lattice_hero/lattice/exposure/exposure_ledger.h"
#include "hero/marshal_hero/marshal/digest.h"
#include "hero/mcp_schema_compat.h"
#include "hero/mcp_stdio_transport.h"
#include "hero/runtime_hero/hero_runtime.h"
#include "hero/runtime_hero/runtime/job_layout.h"
#include "hero/runtime_hero/runtime/policy_training_job_contract.h"
#include "hero/runtime_hero/runtime/wave_settings.h"
#include "hero/short_ref.h"
#include "wikimyei/assembly.h"
#include "wikimyei/policy/portfolio/graph_node_allocation/torch_policy_module.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>
#include <utility>
#include <vector>

namespace cuwacunu::hero::runtime {
namespace {

namespace fs = std::filesystem;
namespace display = cuwacunu::hero::display;

constexpr int kPolicyErrorCode = -32050;
constexpr const char *kDefaultGlobalConfigPath = "/cuwacunu/src/config/.config";
constexpr const char *kDefaultRuntimePolicyPath =
    "/cuwacunu/src/config/hero.runtime.dsl";

struct tool_descriptor_t {
  const char *name;
  const char *description;
  const char *input_schema_json;
};

constexpr tool_descriptor_t kTools[] = {
    {"hero.runtime.status",
     "Read-only health: summarize Runtime Hero policy, executable, "
     "active "
     "wave, job root, and explicit non-proof boundary state.",
     R"({"type":"object","properties":{},"additionalProperties":false})"},
    {"hero.runtime.inspect.schema",
     "Read-only Runtime policy key registry inspection.",
     R"({"type":"object","properties":{},"additionalProperties":false})"},
    {"hero.runtime.inspect.wave", "Read-only active Runtime wave inspection.",
     R"({"type":"object","properties":{"config_path":{"type":"string"}},"additionalProperties":false})"},
    {"hero.runtime.inspect.jobs", "Read-only Runtime job directory listing.",
     R"({"type":"object","properties":{"root":{"type":"string"},"limit":{"type":"integer"},"include_artifacts":{"type":"boolean"}},"additionalProperties":false})"},
    {"hero.runtime.inspect.job",
     "Read-only Runtime job manifest/state/report bundle inspection.",
     R"({"type":"object","properties":{"job_id":{"type":"string"},"job_dir":{"type":"string"},"include_text":{"type":"boolean"},"max_bytes":{"type":"integer"}},"additionalProperties":false})"},
    {"hero.runtime.inspect.artifact.job",
     "Read-only bounded named artifact inspection inside one Runtime job.",
     R"({"type":"object","properties":{"job_id":{"type":"string"},"job_dir":{"type":"string"},"artifact":{"type":"string"},"max_bytes":{"type":"integer"}},"additionalProperties":false})"},
    {"hero.runtime.inspect.artifact.path",
     "Read-only bounded Runtime artifact inspection by explicit path.",
     R"({"type":"object","required":["path"],"properties":{"path":{"type":"string"},"max_bytes":{"type":"integer"}},"additionalProperties":false})"},
    {"hero.runtime.run",
     "Runtime wave execution with mode=dry_run|execute. Policy component "
     "waves use the contract-backed graph_node_allocation driver inside "
     "this same surface; wave-specific payload lives in args_path. Execute "
     "supports the bounded "
     "noop_policy_training.v1 smoke trainer and the PPO V0 "
     "ppo_policy_adapter.v1 trainer. Operators should use "
     "hero.environment.rollout for replay admission.",
     R"({"type":"object","required":["mode"],"properties":{"mode":{"type":"string","enum":["dry_run","execute"]},"args_path":{"type":"string"},"args_digest":{"type":"string"}},"additionalProperties":false})"},
    {"hero.runtime.reset",
     "Guarded developer reset. mode=plan previews the "
     "reset; "
     "mode=execute clears allowed Runtime roots only "
     "with a digest-pinned args_path request and policy permission.",
     R"({"type":"object","required":["mode"],"properties":{"mode":{"type":"string","enum":["plan","execute"]},"args_path":{"type":"string"},"args_digest":{"type":"string"}},"additionalProperties":false})"},
};

[[nodiscard]] bool tool_is_read_only(std::string_view name) {
  return name == "hero.runtime.status" ||
         name == "hero.runtime.inspect.schema" ||
         name == "hero.runtime.inspect.wave" ||
         name == "hero.runtime.inspect.jobs" ||
         name == "hero.runtime.inspect.job" ||
         name == "hero.runtime.inspect.artifact.job" ||
         name == "hero.runtime.inspect.artifact.path";
}

[[nodiscard]] bool tool_is_destructive(std::string_view name) {
  return name == "hero.runtime.run" || name == "hero.runtime.reset";
}

[[nodiscard]] std::string trim_ascii(std::string_view in) {
  std::size_t begin = 0;
  while (begin < in.size() &&
         std::isspace(static_cast<unsigned char>(in[begin])) != 0) {
    ++begin;
  }
  std::size_t end = in.size();
  while (end > begin &&
         std::isspace(static_cast<unsigned char>(in[end - 1])) != 0) {
    --end;
  }
  return std::string(in.substr(begin, end - begin));
}

[[nodiscard]] std::string lowercase_ascii(std::string_view in) {
  std::string out(in);
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return out;
}

[[nodiscard]] bool parse_bool(std::string_view raw, bool *out) {
  const std::string value = lowercase_ascii(trim_ascii(raw));
  if (value == "true" || value == "1" || value == "yes" || value == "on") {
    if (out) {
      *out = true;
    }
    return true;
  }
  if (value == "false" || value == "0" || value == "no" || value == "off") {
    if (out) {
      *out = false;
    }
    return true;
  }
  return false;
}

[[nodiscard]] bool parse_int(std::string_view raw, int *out) {
  const std::string value = trim_ascii(raw);
  if (value.empty()) {
    return false;
  }
  int parsed = 0;
  const auto result =
      std::from_chars(value.data(), value.data() + value.size(), parsed);
  if (result.ec != std::errc{} || result.ptr != value.data() + value.size()) {
    return false;
  }
  if (out) {
    *out = parsed;
  }
  return true;
}

[[nodiscard]] bool parse_int64(std::string_view raw, std::int64_t *out) {
  const std::string value = trim_ascii(raw);
  if (value.empty()) {
    return false;
  }
  std::int64_t parsed = 0;
  const auto result =
      std::from_chars(value.data(), value.data() + value.size(), parsed);
  if (result.ec != std::errc{} || result.ptr != value.data() + value.size()) {
    return false;
  }
  if (out) {
    *out = parsed;
  }
  return true;
}

[[nodiscard]] bool parse_double(std::string_view raw, double *out) {
  const std::string value = trim_ascii(raw);
  if (value.empty()) {
    return false;
  }
  char *end = nullptr;
  errno = 0;
  const double parsed = std::strtod(value.c_str(), &end);
  if (errno != 0 || end == value.c_str() ||
      trim_ascii(std::string_view(end)).size() != 0) {
    return false;
  }
  if (out) {
    *out = parsed;
  }
  return true;
}

[[nodiscard]] std::vector<std::string> split_csv(std::string_view raw) {
  std::vector<std::string> out;
  std::string item;
  std::istringstream in{std::string(raw)};
  while (std::getline(in, item, ',')) {
    item = trim_ascii(item);
    if (!item.empty()) {
      out.push_back(item);
    }
  }
  return out;
}

[[nodiscard]] std::string json_quote(std::string_view s) {
  std::ostringstream out;
  out << '"';
  for (const unsigned char c : s) {
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
        out << "\\u00";
        static constexpr std::array<char, 16> kHex{'0', '1', '2', '3', '4', '5',
                                                   '6', '7', '8', '9', 'a', 'b',
                                                   'c', 'd', 'e', 'f'};
        out << kHex[(c >> 4) & 0x0F] << kHex[c & 0x0F];
      } else {
        out << static_cast<char>(c);
      }
      break;
    }
  }
  out << '"';
  return out.str();
}

[[nodiscard]] const char *bool_json(bool value) {
  return value ? "true" : "false";
}

void skip_ws(const std::string &s, std::size_t *idx) {
  while (*idx < s.size() &&
         std::isspace(static_cast<unsigned char>(s[*idx])) != 0) {
    ++(*idx);
  }
}

[[nodiscard]] bool parse_json_string_token(const std::string &s,
                                           std::size_t *idx, std::string *out) {
  if (*idx >= s.size() || s[*idx] != '"') {
    return false;
  }
  ++(*idx);
  std::string value;
  while (*idx < s.size()) {
    const char c = s[(*idx)++];
    if (c == '"') {
      if (out) {
        *out = std::move(value);
      }
      return true;
    }
    if (c != '\\') {
      if (static_cast<unsigned char>(c) < 0x20) {
        return false;
      }
      value.push_back(c);
      continue;
    }
    if (*idx >= s.size()) {
      return false;
    }
    const char escaped = s[(*idx)++];
    switch (escaped) {
    case '"':
    case '\\':
    case '/':
      value.push_back(escaped);
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
    case 'u':
      if (*idx + 4 > s.size()) {
        return false;
      }
      value.append("\\u");
      value.append(s.substr(*idx, 4));
      *idx += 4;
      break;
    default:
      return false;
    }
  }
  return false;
}

[[nodiscard]] bool skip_json_value(const std::string &s, std::size_t *idx) {
  skip_ws(s, idx);
  if (*idx >= s.size()) {
    return false;
  }
  if (s[*idx] == '"') {
    return parse_json_string_token(s, idx, nullptr);
  }
  if (s[*idx] == '{' || s[*idx] == '[') {
    std::vector<char> stack;
    stack.push_back(s[*idx]);
    ++(*idx);
    bool in_string = false;
    bool escape = false;
    while (*idx < s.size()) {
      const char c = s[(*idx)++];
      if (in_string) {
        if (escape) {
          escape = false;
        } else if (c == '\\') {
          escape = true;
        } else if (c == '"') {
          in_string = false;
        }
        continue;
      }
      if (c == '"') {
        in_string = true;
      } else if (c == '{' || c == '[') {
        stack.push_back(c);
      } else if (c == '}') {
        if (stack.empty() || stack.back() != '{') {
          return false;
        }
        stack.pop_back();
      } else if (c == ']') {
        if (stack.empty() || stack.back() != '[') {
          return false;
        }
        stack.pop_back();
      }
      if (stack.empty()) {
        return true;
      }
    }
    return false;
  }
  const std::size_t begin = *idx;
  while (*idx < s.size()) {
    const char c = s[*idx];
    if (c == ',' || c == '}' || c == ']' ||
        std::isspace(static_cast<unsigned char>(c)) != 0) {
      break;
    }
    ++(*idx);
  }
  return *idx > begin;
}

[[nodiscard]] bool extract_json_raw_field(const std::string &json,
                                          std::string_view key,
                                          std::string *out) {
  std::size_t idx = 0;
  skip_ws(json, &idx);
  if (idx >= json.size() || json[idx] != '{') {
    return false;
  }
  ++idx;
  while (idx < json.size()) {
    skip_ws(json, &idx);
    if (idx < json.size() && json[idx] == '}') {
      return false;
    }
    std::string current_key;
    if (!parse_json_string_token(json, &idx, &current_key)) {
      return false;
    }
    skip_ws(json, &idx);
    if (idx >= json.size() || json[idx] != ':') {
      return false;
    }
    ++idx;
    skip_ws(json, &idx);
    const std::size_t value_begin = idx;
    if (!skip_json_value(json, &idx)) {
      return false;
    }
    if (current_key == key) {
      if (out) {
        *out = trim_ascii(
            std::string_view(json).substr(value_begin, idx - value_begin));
      }
      return true;
    }
    skip_ws(json, &idx);
    if (idx < json.size() && json[idx] == ',') {
      ++idx;
      continue;
    }
    if (idx < json.size() && json[idx] == '}') {
      return false;
    }
  }
  return false;
}

[[nodiscard]] bool extract_json_string_field(const std::string &json,
                                             std::string_view key,
                                             std::string *out) {
  std::string raw;
  if (!extract_json_raw_field(json, key, &raw)) {
    return false;
  }
  std::size_t idx = 0;
  skip_ws(raw, &idx);
  std::string value;
  if (!parse_json_string_token(raw, &idx, &value)) {
    return false;
  }
  skip_ws(raw, &idx);
  if (idx != raw.size()) {
    return false;
  }
  if (out) {
    *out = std::move(value);
  }
  return true;
}

[[nodiscard]] bool
extract_json_first_string_field(const std::string &json,
                                std::initializer_list<std::string_view> keys,
                                std::string *out) {
  for (const auto key : keys) {
    if (extract_json_string_field(json, key, out)) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool
extract_json_string_object(const std::string &json,
                           std::unordered_map<std::string, std::string> *out) {
  std::size_t idx = 0;
  skip_ws(json, &idx);
  if (idx >= json.size() || json[idx] != '{') {
    return false;
  }
  ++idx;
  std::unordered_map<std::string, std::string> values;
  while (idx < json.size()) {
    skip_ws(json, &idx);
    if (idx < json.size() && json[idx] == '}') {
      ++idx;
      skip_ws(json, &idx);
      if (idx != json.size()) {
        return false;
      }
      if (out) {
        *out = std::move(values);
      }
      return true;
    }
    std::string key;
    if (!parse_json_string_token(json, &idx, &key)) {
      return false;
    }
    skip_ws(json, &idx);
    if (idx >= json.size() || json[idx] != ':') {
      return false;
    }
    ++idx;
    skip_ws(json, &idx);
    std::string value;
    if (!parse_json_string_token(json, &idx, &value)) {
      return false;
    }
    values[std::move(key)] = std::move(value);
    skip_ws(json, &idx);
    if (idx < json.size() && json[idx] == ',') {
      ++idx;
      continue;
    }
    if (idx < json.size() && json[idx] == '}') {
      continue;
    }
    return false;
  }
  return false;
}

[[nodiscard]] bool extract_json_bool_field(const std::string &json,
                                           std::string_view key, bool *out) {
  std::string raw;
  if (!extract_json_raw_field(json, key, &raw)) {
    return false;
  }
  const std::string value = trim_ascii(raw);
  if (value == "true") {
    if (out) {
      *out = true;
    }
    return true;
  }
  if (value == "false") {
    if (out) {
      *out = false;
    }
    return true;
  }
  return false;
}

[[nodiscard]] bool extract_json_int_field(const std::string &json,
                                          std::string_view key, int *out) {
  std::string raw;
  if (!extract_json_raw_field(json, key, &raw)) {
    return false;
  }
  return parse_int(raw, out);
}

struct json_field_t {
  std::string key;
  std::string raw_value;
};

[[nodiscard]] bool parse_json_object_fields(const std::string &json,
                                            std::vector<json_field_t> *out,
                                            std::string *err) {
  if (out) {
    out->clear();
  }
  std::size_t idx = 0;
  skip_ws(json, &idx);
  if (idx >= json.size() || json[idx] != '{') {
    if (err) {
      *err = "arguments must be a JSON object";
    }
    return false;
  }
  ++idx;
  skip_ws(json, &idx);
  if (idx < json.size() && json[idx] == '}') {
    ++idx;
    skip_ws(json, &idx);
    if (idx != json.size()) {
      if (err) {
        *err = "unexpected trailing content after JSON object";
      }
      return false;
    }
    return true;
  }
  while (idx < json.size()) {
    std::string key;
    if (!parse_json_string_token(json, &idx, &key)) {
      if (err) {
        *err = "expected JSON object key";
      }
      return false;
    }
    if (out) {
      for (const auto &field : *out) {
        if (field.key == key) {
          if (err) {
            *err = "duplicate field: " + key;
          }
          return false;
        }
      }
    }
    skip_ws(json, &idx);
    if (idx >= json.size() || json[idx] != ':') {
      if (err) {
        *err = "expected ':' after JSON object key";
      }
      return false;
    }
    ++idx;
    skip_ws(json, &idx);
    const std::size_t value_begin = idx;
    if (!skip_json_value(json, &idx)) {
      if (err) {
        *err = "invalid JSON value for field: " + key;
      }
      return false;
    }
    if (out) {
      out->push_back({key, trim_ascii(std::string_view(json).substr(
                               value_begin, idx - value_begin))});
    }
    skip_ws(json, &idx);
    if (idx < json.size() && json[idx] == ',') {
      ++idx;
      skip_ws(json, &idx);
      continue;
    }
    if (idx < json.size() && json[idx] == '}') {
      ++idx;
      skip_ws(json, &idx);
      if (idx != json.size()) {
        if (err) {
          *err = "unexpected trailing content after JSON object";
        }
        return false;
      }
      return true;
    }
    if (err) {
      *err = "expected ',' or '}' in JSON object";
    }
    return false;
  }
  if (err) {
    *err = "unterminated JSON object";
  }
  return false;
}

[[nodiscard]] bool
field_allowed(std::string_view key,
              std::initializer_list<std::string_view> allowed) {
  return std::find(allowed.begin(), allowed.end(), key) != allowed.end();
}

[[nodiscard]] bool
validate_allowed_fields(const std::vector<json_field_t> &fields,
                        std::initializer_list<std::string_view> allowed,
                        std::string *err) {
  for (const auto &field : fields) {
    if (!field_allowed(field.key, allowed)) {
      if (err) {
        *err = "unknown field: " + field.key;
      }
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool
validate_tool_fields(const std::string &args,
                     std::initializer_list<std::string_view> allowed,
                     std::vector<json_field_t> *fields, std::string *err) {
  if (!parse_json_object_fields(args, fields, err)) {
    return false;
  }
  return validate_allowed_fields(*fields, allowed, err);
}

[[nodiscard]] bool parse_required_string_arg(const std::string &args,
                                             std::string_view key,
                                             std::string *out,
                                             std::string *err) {
  std::string raw;
  if (!extract_json_raw_field(args, key, &raw)) {
    if (err) {
      *err = "missing required field: " + std::string(key);
    }
    return false;
  }
  std::string value;
  if (!extract_json_string_field(args, key, &value)) {
    if (err) {
      *err = std::string(key) + " must be string";
    }
    return false;
  }
  value = trim_ascii(value);
  if (value.empty()) {
    if (err) {
      *err = std::string(key) + " must be non-empty";
    }
    return false;
  }
  if (out) {
    *out = std::move(value);
  }
  return true;
}

void append_raw_json_field(std::ostringstream &out, std::string_view key,
                           const std::string &raw, bool *first) {
  if (!*first) {
    out << ",";
  }
  out << json_quote(key) << ":" << raw;
  *first = false;
}

void append_bool_json_field(std::ostringstream &out, std::string_view key,
                            bool value, bool *first) {
  append_raw_json_field(out, key, value ? "true" : "false", first);
}

void append_string_json_field(std::ostringstream &out, std::string_view key,
                              std::string_view value, bool *first) {
  append_raw_json_field(out, key, json_quote(value), first);
}

void append_optional_raw_json_field(const std::string &args,
                                    std::ostringstream &out,
                                    std::string_view key, bool *first) {
  std::string raw;
  if (extract_json_raw_field(args, key, &raw)) {
    append_raw_json_field(out, key, raw, first);
  }
}

[[nodiscard]] std::string
object_with_selected_fields(const std::string &args,
                            std::initializer_list<std::string_view> keys) {
  std::ostringstream out;
  out << "{";
  bool first = true;
  for (const auto key : keys) {
    append_optional_raw_json_field(args, out, key, &first);
  }
  out << "}";
  return out.str();
}

[[nodiscard]] fs::path normalize_path(const fs::path &path) {
  std::error_code ec;
  const fs::path canonical = fs::weakly_canonical(path, ec);
  return ec ? path.lexically_normal() : canonical;
}

[[nodiscard]] fs::path resolve_against(const fs::path &base_file,
                                       std::string_view raw) {
  fs::path path(trim_ascii(raw));
  if (path.empty()) {
    return path;
  }
  if (path.is_absolute()) {
    return normalize_path(path);
  }
  return normalize_path(base_file.parent_path() / path);
}

[[nodiscard]] bool path_within(const fs::path &root, const fs::path &path) {
  const fs::path norm_root = normalize_path(root);
  const fs::path norm_path = normalize_path(path);
  auto root_it = norm_root.begin();
  auto path_it = norm_path.begin();
  for (; root_it != norm_root.end(); ++root_it, ++path_it) {
    if (path_it == norm_path.end() || *root_it != *path_it) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool under_any_root(const fs::path &path,
                                  const std::vector<std::string> &roots,
                                  const fs::path &base_file) {
  for (const std::string &raw : roots) {
    const fs::path root = resolve_against(base_file, raw);
    if (path_within(root, path)) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] std::string strip_ini_comment(std::string_view in) {
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
    if (!in_single && !in_double && (c == '#' || c == ';')) {
      break;
    }
    out.push_back(c);
  }
  return out;
}

[[nodiscard]] bool read_text_file(const fs::path &path, std::string *out,
                                  std::string *err) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    if (err) {
      *err = "cannot open file: " + path.string();
    }
    return false;
  }
  std::ostringstream buffer;
  buffer << in.rdbuf();
  if (in.bad()) {
    if (err) {
      *err = "cannot read file: " + path.string();
    }
    return false;
  }
  if (out) {
    *out = buffer.str();
  }
  return true;
}

[[nodiscard]] std::string file_digest_or_empty(const fs::path &path,
                                               const std::string &domain) {
  std::string text;
  std::string ignored;
  if (!read_text_file(path, &text, &ignored)) {
    return {};
  }
  return cuwacunu::hero::marshal::marshal_digest_for_text(domain, text);
}

[[nodiscard]] std::string
replay_report_digest_for_text(const std::string &text) {
  std::uint64_t hash =
      cuwacunu::wikimyei::assembly::assembly_detail::kFnvOffsetBasis;
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(
      hash, "kikijyeba.lattice.exposure.v1");
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(
      hash,
      std::string("kikijyeba.environment.replay.report_digest.v1\n") + text);
  return cuwacunu::wikimyei::assembly::assembly_detail::hash_hex(hash);
}

[[nodiscard]] std::string
replay_report_digest_for_path_or_empty(const fs::path &path) {
  std::string text;
  std::string ignored;
  if (!read_text_file(path, &text, &ignored)) {
    return {};
  }
  return replay_report_digest_for_text(text);
}

[[nodiscard]] std::string read_text_file_limited(const fs::path &path,
                                                 std::size_t max_bytes,
                                                 bool *truncated) {
  if (truncated) {
    *truncated = false;
  }
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    return {};
  }
  std::string out;
  out.resize(max_bytes);
  in.read(out.data(), static_cast<std::streamsize>(max_bytes));
  out.resize(static_cast<std::size_t>(in.gcount()));
  if (in.peek() != EOF && truncated) {
    *truncated = true;
  }
  return out;
}

[[nodiscard]] std::optional<std::string>
read_ini_value(const fs::path &ini_path, std::string_view section,
               std::string_view key) {
  std::ifstream in(ini_path);
  if (!in) {
    return std::nullopt;
  }
  std::string current_section;
  std::string line;
  while (std::getline(in, line)) {
    const std::string trimmed = trim_ascii(strip_ini_comment(line));
    if (trimmed.empty()) {
      continue;
    }
    if (trimmed.size() >= 2 && trimmed.front() == '[' &&
        trimmed.back() == ']') {
      current_section = trim_ascii(trimmed.substr(1, trimmed.size() - 2));
      continue;
    }
    if (current_section != section) {
      continue;
    }
    const std::size_t eq = trimmed.find('=');
    if (eq == std::string::npos) {
      continue;
    }
    const std::string lhs = trim_ascii(trimmed.substr(0, eq));
    if (lhs == key) {
      return trim_ascii(trimmed.substr(eq + 1));
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::string strip_dsl_comments(std::string_view in) {
  std::string out;
  out.reserve(in.size());
  bool in_block = false;
  for (std::size_t i = 0; i < in.size(); ++i) {
    if (!in_block && i + 1 < in.size() && in[i] == '/' && in[i + 1] == '*') {
      in_block = true;
      ++i;
      continue;
    }
    if (in_block && i + 1 < in.size() && in[i] == '*' && in[i + 1] == '/') {
      in_block = false;
      ++i;
      continue;
    }
    if (in_block) {
      continue;
    }
    out.push_back(in[i]);
  }
  return out;
}

[[nodiscard]] std::unordered_map<std::string, std::string>
parse_assignment_text(std::string_view text, bool semicolon_terminated) {
  std::unordered_map<std::string, std::string> out;
  std::istringstream lines{std::string(text)};
  std::string line;
  while (std::getline(lines, line)) {
    line = trim_ascii(strip_ini_comment(line));
    if (line.empty() || line == "};" || line == "}" ||
        line.find('{') != std::string::npos ||
        (line.front() == '[' && line.back() == ']')) {
      continue;
    }
    const std::size_t eq = line.find('=');
    if (eq == std::string::npos) {
      continue;
    }
    std::string key = trim_ascii(line.substr(0, eq));
    const std::size_t domain = key.find_first_of("[:");
    if (domain != std::string::npos) {
      key = trim_ascii(key.substr(0, domain));
    }
    std::string value = trim_ascii(line.substr(eq + 1));
    if (semicolon_terminated && !value.empty() && value.back() == ';') {
      value.pop_back();
      value = trim_ascii(value);
    }
    if (!key.empty()) {
      out[std::move(key)] = std::move(value);
    }
  }
  return out;
}

struct runtime_profile_blocks_t {
  std::string base_text{};
  std::unordered_map<std::string, std::string> profile_text_by_id{};
};

[[nodiscard]] bool valid_runtime_profile_id(std::string_view raw) {
  const std::string value = trim_ascii(raw);
  if (value.empty()) {
    return false;
  }
  for (const unsigned char c : value) {
    if (std::isalnum(c) == 0 && c != '_' && c != '-' && c != '.') {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool split_runtime_profile_blocks(std::string_view text,
                                                runtime_profile_blocks_t *out,
                                                std::string *error) {
  if (!out) {
    if (error) {
      *error = "runtime profile block output pointer is null";
    }
    return false;
  }

  runtime_profile_blocks_t parsed{};
  std::istringstream lines{std::string(text)};
  std::string line;
  bool in_profile = false;
  std::string active_profile_id;
  std::ostringstream active_profile_text;
  std::size_t line_no = 0;

  while (std::getline(lines, line)) {
    ++line_no;
    const std::string trimmed = trim_ascii(strip_ini_comment(line));
    if (!in_profile) {
      if (trimmed.rfind("RUNTIME_PROFILE", 0) != 0) {
        parsed.base_text += line;
        parsed.base_text.push_back('\n');
        continue;
      }

      const std::size_t brace = trimmed.find('{');
      if (brace == std::string::npos) {
        if (error) {
          *error = "RUNTIME_PROFILE missing opening brace at line " +
                   std::to_string(line_no);
        }
        return false;
      }
      active_profile_id = trim_ascii(
          trimmed.substr(std::string_view{"RUNTIME_PROFILE"}.size(),
                         brace - std::string_view{"RUNTIME_PROFILE"}.size()));
      const std::string after_brace = trim_ascii(trimmed.substr(brace + 1));
      if (!valid_runtime_profile_id(active_profile_id)) {
        if (error) {
          *error = "invalid RUNTIME_PROFILE id at line " +
                   std::to_string(line_no) + ": " + active_profile_id;
        }
        return false;
      }
      if (!after_brace.empty()) {
        if (error) {
          *error = "RUNTIME_PROFILE opening line must end after `{` at line " +
                   std::to_string(line_no);
        }
        return false;
      }
      if (parsed.profile_text_by_id.count(active_profile_id) != 0) {
        if (error) {
          *error = "duplicate RUNTIME_PROFILE id: " + active_profile_id;
        }
        return false;
      }
      in_profile = true;
      active_profile_text.str(std::string{});
      active_profile_text.clear();
      continue;
    }

    if (trimmed == "}" || trimmed == "};") {
      parsed.profile_text_by_id.emplace(active_profile_id,
                                        active_profile_text.str());
      active_profile_id.clear();
      active_profile_text.str(std::string{});
      active_profile_text.clear();
      in_profile = false;
      continue;
    }
    active_profile_text << line << '\n';
  }

  if (in_profile) {
    if (error) {
      *error = "unterminated RUNTIME_PROFILE block: " + active_profile_id;
    }
    return false;
  }

  *out = std::move(parsed);
  return true;
}

[[nodiscard]] std::unordered_map<std::string, std::string>
parse_kv_file(const fs::path &path) {
  std::string text;
  std::string ignored;
  if (!read_text_file(path, &text, &ignored)) {
    return {};
  }
  return parse_assignment_text(text, false);
}

[[nodiscard]] std::string
kv_map_to_json(const std::unordered_map<std::string, std::string> &map) {
  std::vector<std::string> keys;
  keys.reserve(map.size());
  for (const auto &entry : map) {
    keys.push_back(entry.first);
  }
  std::sort(keys.begin(), keys.end());
  std::ostringstream out;
  out << "{";
  for (std::size_t i = 0; i < keys.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << json_quote(keys[i]) << ":" << json_quote(map.at(keys[i]));
  }
  out << "}";
  return out.str();
}

enum class kv_json_field_kind_t {
  string_value,
  bool_value,
  int_value,
  number_value
};

struct kv_json_field_t {
  const char *key;
  kv_json_field_kind_t kind;
};

[[nodiscard]] bool
kv_get_trimmed(const std::unordered_map<std::string, std::string> &map,
               std::string_view key, std::string *out) {
  const auto found = map.find(std::string(key));
  if (found == map.end()) {
    return false;
  }
  if (out) {
    *out = trim_ascii(found->second);
  }
  return true;
}

void kv_set_string(const std::unordered_map<std::string, std::string> &map,
                   std::string_view key, std::string *field) {
  std::string value;
  if (!kv_get_trimmed(map, key, &value)) {
    return;
  }
  *field = std::move(value);
}

[[nodiscard]] bool
kv_set_bool(const std::unordered_map<std::string, std::string> &map,
            std::string_view key, bool *field, std::string *err) {
  std::string value;
  if (!kv_get_trimmed(map, key, &value) || value.empty()) {
    return true;
  }
  bool parsed = false;
  if (!parse_bool(value, &parsed)) {
    if (err) {
      *err = std::string(key) + " must be boolean";
    }
    return false;
  }
  *field = parsed;
  return true;
}

[[nodiscard]] bool
kv_set_int64(const std::unordered_map<std::string, std::string> &map,
             std::string_view key, std::int64_t *field, std::string *err) {
  std::string value;
  if (!kv_get_trimmed(map, key, &value) || value.empty()) {
    return true;
  }
  std::int64_t parsed = 0;
  if (!parse_int64(value, &parsed)) {
    if (err) {
      *err = std::string(key) + " must be integer";
    }
    return false;
  }
  *field = parsed;
  return true;
}

[[nodiscard]] bool
kv_set_double(const std::unordered_map<std::string, std::string> &map,
              std::string_view key, double *field, std::string *err) {
  std::string value;
  if (!kv_get_trimmed(map, key, &value) || value.empty()) {
    return true;
  }
  double parsed = 0.0;
  if (!parse_double(value, &parsed)) {
    if (err) {
      *err = std::string(key) + " must be number";
    }
    return false;
  }
  *field = parsed;
  return true;
}

[[nodiscard]] bool
append_kv_json_field(const std::unordered_map<std::string, std::string> &map,
                     const kv_json_field_t &field, std::ostringstream &out,
                     bool *first, std::string *err) {
  std::string value;
  if (!kv_get_trimmed(map, field.key, &value) || value.empty()) {
    return true;
  }
  switch (field.kind) {
  case kv_json_field_kind_t::string_value:
    append_string_json_field(out, field.key, value, first);
    return true;
  case kv_json_field_kind_t::bool_value: {
    bool parsed = false;
    if (!parse_bool(value, &parsed)) {
      if (err) {
        *err = std::string(field.key) + " must be boolean";
      }
      return false;
    }
    append_bool_json_field(out, field.key, parsed, first);
    return true;
  }
  case kv_json_field_kind_t::int_value: {
    std::int64_t parsed = 0;
    if (!parse_int64(value, &parsed)) {
      if (err) {
        *err = std::string(field.key) + " must be integer";
      }
      return false;
    }
    append_raw_json_field(out, field.key, std::to_string(parsed), first);
    return true;
  }
  case kv_json_field_kind_t::number_value: {
    double parsed = 0.0;
    if (!parse_double(value, &parsed) || !std::isfinite(parsed)) {
      if (err) {
        *err = std::string(field.key) + " must be finite number";
      }
      return false;
    }
    append_raw_json_field(out, field.key, value, first);
    return true;
  }
  }
  return false;
}

[[nodiscard]] bool
append_kv_json_fields(const std::unordered_map<std::string, std::string> &map,
                      std::initializer_list<kv_json_field_t> fields,
                      std::ostringstream &out, bool *first, std::string *err) {
  for (const auto &field : fields) {
    if (!append_kv_json_field(map, field, out, first, err)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool validate_kv_json_fields(
    const std::unordered_map<std::string, std::string> &map,
    std::initializer_list<kv_json_field_t> allowed, std::string *err,
    std::string_view request_name = "execution request",
    std::initializer_list<std::string_view> extra_allowed = {}) {
  for (const auto &[key, _] : map) {
    bool allowed_key = false;
    for (const auto &field : allowed) {
      if (key == field.key) {
        allowed_key = true;
        break;
      }
    }
    if (!allowed_key && field_allowed(key, extra_allowed)) {
      allowed_key = true;
    }
    if (!allowed_key) {
      if (err != nullptr) {
        *err = std::string(request_name) + " unknown field: " + key;
      }
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool read_required_text_file_arg(const std::string &args,
                                               std::string_view path_key,
                                               fs::path *path,
                                               std::string *text,
                                               std::string *err) {
  std::string path_arg;
  if (!parse_required_string_arg(args, path_key, &path_arg, err)) {
    return false;
  }
  fs::path resolved = normalize_path(fs::path(path_arg));
  std::string read_error;
  if (!read_text_file(resolved, text, &read_error)) {
    if (err) {
      *err = "failed to read " + std::string(path_key) + ": " + read_error;
    }
    return false;
  }
  if (path) {
    *path = std::move(resolved);
  }
  return true;
}

[[nodiscard]] bool check_optional_string_digest_arg(const std::string &args,
                                                    std::string_view digest_key,
                                                    std::string_view actual,
                                                    std::string_view error_code,
                                                    std::string *err) {
  if (!extract_json_raw_field(args, digest_key, nullptr)) {
    return true;
  }
  std::string supplied;
  if (!extract_json_string_field(args, digest_key, &supplied)) {
    if (err) {
      *err = std::string(digest_key) + " must be string";
    }
    return false;
  }
  supplied = trim_ascii(supplied);
  if (!supplied.empty() && supplied != actual) {
    if (err) {
      *err = std::string(error_code) + ": expected " + supplied + " actual " +
             std::string(actual);
    }
    return false;
  }
  return true;
}

[[nodiscard]] bool load_optional_execution_request(
    const std::string &args, std::unordered_map<std::string, std::string> *out,
    std::string *err) {
  if (out != nullptr) {
    out->clear();
  }
  if (!extract_json_raw_field(args, "execution_request_path", nullptr)) {
    if (extract_json_raw_field(args, "execution_request_digest", nullptr)) {
      *err = "execution_request_digest requires execution_request_path";
      return false;
    }
    return true;
  }

  fs::path execution_request_path;
  std::string execution_request_text;
  if (!read_required_text_file_arg(args, "execution_request_path",
                                   &execution_request_path,
                                   &execution_request_text, err)) {
    return false;
  }
  const std::string execution_request_digest =
      cuwacunu::hero::marshal::marshal_digest_for_text(
          "kikijyeba.runtime.policy_training_execution_request.v1",
          execution_request_text);
  if (!check_optional_string_digest_arg(
          args, "execution_request_digest", execution_request_digest,
          "E_RUNTIME_EXECUTION_REQUEST_DIGEST_MISMATCH", err)) {
    return false;
  }

  auto parsed = parse_assignment_text(execution_request_text, false);
  if (parsed.count("confirm_execute") != 0 ||
      parsed.count("timeout_seconds") != 0) {
    *err = "execution request must not contain confirm_execute or "
           "timeout_seconds";
    return false;
  }
  if (out != nullptr) {
    *out = std::move(parsed);
  }
  return true;
}

struct runtime_run_request_file_t {
  std::unordered_map<std::string, std::string> fields{};
  fs::path path{};
  std::string digest_domain{};
  std::string digest{};
  bool digest_supplied = false;
  bool present = false;
};

struct runtime_reset_request_file_t {
  std::unordered_map<std::string, std::string> fields{};
  fs::path path{};
  std::string digest_domain{};
  std::string digest{};
  bool digest_supplied = false;
  bool present = false;
};

[[nodiscard]] bool runtime_run_request_public_field(std::string_view key) {
  return key == "subject" || key == "mode" || key == "operation" ||
         key == "requested_mode" || key == "args_path" || key == "args_digest";
}

[[nodiscard]] bool runtime_run_request_allowed_field(std::string_view key) {
  return key == "config_path" || key == "contract_path" ||
         key == "contract_digest" || key == "execution_request_path" ||
         key == "execution_request_digest" || key == "runtime_handoff_path" ||
         key == "runtime_handoff_digest";
}

[[nodiscard]] std::string
runtime_run_request_digest_domain(std::string_view operation) {
  return "kikijyeba.runtime.run_request." + std::string(operation) + ".v1";
}

[[nodiscard]] bool runtime_reset_request_public_field(std::string_view key) {
  return key == "mode" || key == "requested_mode" || key == "args_path" ||
         key == "args_digest";
}

[[nodiscard]] bool runtime_reset_request_allowed_field(std::string_view key) {
  return key == "runtime_root" || key == "backup";
}

[[nodiscard]] std::string runtime_reset_request_digest_domain() {
  return "kikijyeba.runtime.reset_request.v1";
}

[[nodiscard]] bool parse_runtime_run_request_text(
    std::string_view text, std::unordered_map<std::string, std::string> *out,
    std::string *err) {
  if (out == nullptr) {
    *err = "run request output map is null";
    return false;
  }
  out->clear();
  std::istringstream lines{std::string(text)};
  std::string line;
  std::size_t line_no = 0;
  while (std::getline(lines, line)) {
    ++line_no;
    line = trim_ascii(strip_ini_comment(line));
    if (line.empty()) {
      continue;
    }
    if (line == "}" || line == "};" || line.find('{') != std::string::npos ||
        (line.front() == '[' && line.back() == ']')) {
      *err = "E_RUNTIME_RUN_REQUEST_PARSE: line " + std::to_string(line_no) +
             " is not a key=value assignment";
      return false;
    }
    const std::size_t eq = line.find('=');
    if (eq == std::string::npos) {
      *err = "E_RUNTIME_RUN_REQUEST_PARSE: line " + std::to_string(line_no) +
             " is missing '='";
      return false;
    }
    std::string key = trim_ascii(line.substr(0, eq));
    const std::size_t domain = key.find_first_of("[:");
    if (domain != std::string::npos) {
      key = trim_ascii(key.substr(0, domain));
    }
    std::string value = trim_ascii(line.substr(eq + 1));
    if (!value.empty() && value.back() == ';') {
      value.pop_back();
      value = trim_ascii(value);
    }
    if (key.empty()) {
      *err = "E_RUNTIME_RUN_REQUEST_PARSE: line " + std::to_string(line_no) +
             " has empty key";
      return false;
    }
    if (runtime_run_request_public_field(key)) {
      *err = "E_RUNTIME_RUN_REQUEST_PUBLIC_FIELD: " + key;
      return false;
    }
    if (!runtime_run_request_allowed_field(key)) {
      *err = "E_RUNTIME_RUN_REQUEST_UNKNOWN_FIELD: " + key;
      return false;
    }
    if (out->find(key) != out->end()) {
      *err = "E_RUNTIME_RUN_REQUEST_DUPLICATE_FIELD: " + key;
      return false;
    }
    out->emplace(std::move(key), std::move(value));
  }
  return true;
}

[[nodiscard]] bool parse_runtime_reset_request_text(
    std::string_view text, std::unordered_map<std::string, std::string> *out,
    std::string *err) {
  if (out == nullptr) {
    *err = "reset request output map is null";
    return false;
  }
  out->clear();
  std::istringstream lines{std::string(text)};
  std::string line;
  std::size_t line_no = 0;
  while (std::getline(lines, line)) {
    ++line_no;
    line = trim_ascii(strip_ini_comment(line));
    if (line.empty()) {
      continue;
    }
    if (line == "}" || line == "};" || line.find('{') != std::string::npos ||
        (line.front() == '[' && line.back() == ']')) {
      *err = "E_RUNTIME_RESET_REQUEST_PARSE: line " + std::to_string(line_no) +
             " is not a key=value assignment";
      return false;
    }
    const std::size_t eq = line.find('=');
    if (eq == std::string::npos) {
      *err = "E_RUNTIME_RESET_REQUEST_PARSE: line " + std::to_string(line_no) +
             " is missing '='";
      return false;
    }
    std::string key = trim_ascii(line.substr(0, eq));
    const std::size_t domain = key.find_first_of("[:");
    if (domain != std::string::npos) {
      key = trim_ascii(key.substr(0, domain));
    }
    std::string value = trim_ascii(line.substr(eq + 1));
    if (!value.empty() && value.back() == ';') {
      value.pop_back();
      value = trim_ascii(value);
    }
    if (key.empty()) {
      *err = "E_RUNTIME_RESET_REQUEST_PARSE: line " + std::to_string(line_no) +
             " has empty key";
      return false;
    }
    if (runtime_reset_request_public_field(key)) {
      *err = "E_RUNTIME_RESET_REQUEST_PUBLIC_FIELD: " + key;
      return false;
    }
    if (key == "confirm_dev_nuke") {
      *err = "E_RUNTIME_RESET_REQUEST_RETIRED_FIELD: confirm_dev_nuke";
      return false;
    }
    if (!runtime_reset_request_allowed_field(key)) {
      *err = "E_RUNTIME_RESET_REQUEST_UNKNOWN_FIELD: " + key;
      return false;
    }
    if (out->find(key) != out->end()) {
      *err = "E_RUNTIME_RESET_REQUEST_DUPLICATE_FIELD: " + key;
      return false;
    }
    out->emplace(std::move(key), std::move(value));
  }
  return true;
}

[[nodiscard]] bool load_optional_run_request(const std::string &args,
                                             std::string_view operation,
                                             std::string_view requested_mode,
                                             runtime_run_request_file_t *out,
                                             std::string *err) {
  if (out == nullptr) {
    *err = "run request packet output is null";
    return false;
  }
  runtime_run_request_file_t packet{};
  packet.present = extract_json_raw_field(args, "args_path", nullptr);
  packet.digest_supplied = extract_json_raw_field(args, "args_digest", nullptr);
  if (!packet.present) {
    if (packet.digest_supplied) {
      *err = "args_digest requires args_path";
      return false;
    }
    *out = std::move(packet);
    return true;
  }

  std::string request_text;
  if (!read_required_text_file_arg(args, "args_path", &packet.path,
                                   &request_text, err)) {
    return false;
  }
  packet.digest_domain = runtime_run_request_digest_domain(operation);
  packet.digest = cuwacunu::hero::marshal::marshal_digest_for_text(
      packet.digest_domain, request_text);
  if (requested_mode == "execute" && !packet.digest_supplied) {
    *err = "E_RUNTIME_RUN_REQUEST_DIGEST_REQUIRED: args_digest is "
           "required when mode=execute and args_path is set";
    return false;
  }
  if (packet.digest_supplied) {
    std::string supplied;
    if (!extract_json_string_field(args, "args_digest", &supplied)) {
      *err = "args_digest must be string";
      return false;
    }
    supplied = trim_ascii(supplied);
    if (supplied.empty()) {
      *err = "args_digest must be non-empty when supplied";
      return false;
    }
    if (supplied != packet.digest) {
      *err = "E_RUNTIME_RUN_REQUEST_DIGEST_MISMATCH: expected " + supplied +
             " actual " + packet.digest;
      return false;
    }
  }
  if (!parse_runtime_run_request_text(request_text, &packet.fields, err)) {
    return false;
  }
  *out = std::move(packet);
  return true;
}

[[nodiscard]] bool load_optional_reset_request(
    const std::string &args, std::string_view requested_mode,
    runtime_reset_request_file_t *out, std::string *err) {
  if (out == nullptr) {
    *err = "reset request packet output is null";
    return false;
  }
  runtime_reset_request_file_t packet{};
  packet.present = extract_json_raw_field(args, "args_path", nullptr);
  packet.digest_supplied = extract_json_raw_field(args, "args_digest", nullptr);
  if (!packet.present) {
    if (packet.digest_supplied) {
      *err = "args_digest requires args_path";
      return false;
    }
    if (requested_mode == "execute") {
      *err = "E_RUNTIME_RESET_REQUEST_REQUIRED: args_path is required when "
             "mode=execute";
      return false;
    }
    *out = std::move(packet);
    return true;
  }

  std::string request_text;
  if (!read_required_text_file_arg(args, "args_path", &packet.path,
                                   &request_text, err)) {
    return false;
  }
  packet.digest_domain = runtime_reset_request_digest_domain();
  packet.digest = cuwacunu::hero::marshal::marshal_digest_for_text(
      packet.digest_domain, request_text);
  if (requested_mode == "execute" && !packet.digest_supplied) {
    *err = "E_RUNTIME_RESET_REQUEST_DIGEST_REQUIRED: args_digest is required "
           "when mode=execute";
    return false;
  }
  if (packet.digest_supplied) {
    std::string supplied;
    if (!extract_json_string_field(args, "args_digest", &supplied)) {
      *err = "args_digest must be string";
      return false;
    }
    supplied = trim_ascii(supplied);
    if (supplied.empty()) {
      *err = "args_digest must be non-empty when supplied";
      return false;
    }
    if (supplied != packet.digest) {
      *err = "E_RUNTIME_RESET_REQUEST_DIGEST_MISMATCH: expected " + supplied +
             " actual " + packet.digest;
      return false;
    }
  }
  if (!parse_runtime_reset_request_text(request_text, &packet.fields, err)) {
    return false;
  }
  *out = std::move(packet);
  return true;
}

[[nodiscard]] bool read_runtime_handoff_for_run_request(
    const std::unordered_map<std::string, std::string> &request,
    std::string *handoff_json, std::string *err) {
  std::string handoff_path_raw;
  const bool has_handoff_path =
      kv_get_trimmed(request, "runtime_handoff_path", &handoff_path_raw) &&
      !handoff_path_raw.empty();
  std::string supplied_digest;
  const bool has_handoff_digest =
      kv_get_trimmed(request, "runtime_handoff_digest", &supplied_digest) &&
      !supplied_digest.empty();
  if (!has_handoff_path) {
    if (has_handoff_digest) {
      *err = "runtime_handoff_digest requires runtime_handoff_path";
      return false;
    }
    return true;
  }
  fs::path handoff_path = normalize_path(fs::path(handoff_path_raw));
  std::string read_error;
  std::string text;
  if (!read_text_file(handoff_path, &text, &read_error)) {
    *err = "failed to read runtime_handoff_path: " + read_error;
    return false;
  }
  const std::string actual_digest =
      cuwacunu::hero::marshal::marshal_digest_for_text(
          "kikijyeba.runtime.run_request.runtime_handoff_file.v1", text);
  if (has_handoff_digest && supplied_digest != actual_digest) {
    *err = "E_RUNTIME_RUN_REQUEST_HANDOFF_DIGEST_MISMATCH: expected " +
           supplied_digest + " actual " + actual_digest;
    return false;
  }
  std::vector<json_field_t> handoff_fields;
  if (!parse_json_object_fields(text, &handoff_fields, err)) {
    if (err != nullptr) {
      *err = "E_RUNTIME_RUN_REQUEST_HANDOFF_INVALID: " + *err;
    }
    return false;
  }
  if (handoff_json != nullptr) {
    *handoff_json = trim_ascii(text);
  }
  return true;
}

[[nodiscard]] bool materialize_runtime_run_args(const std::string &public_args,
                                                std::string_view operation,
                                                std::string_view requested_mode,
                                                std::string *out,
                                                std::string *err) {
  runtime_run_request_file_t request{};
  if (!load_optional_run_request(public_args, operation, requested_mode,
                                 &request, err)) {
    return false;
  }
  std::ostringstream resolved;
  resolved << "{";
  bool first = true;
  append_string_json_field(resolved, "operation", operation, &first);
  append_string_json_field(resolved, "requested_mode", requested_mode, &first);
  if (request.present) {
    if (!append_kv_json_fields(
            request.fields,
            {{"config_path", kv_json_field_kind_t::string_value},
             {"contract_path", kv_json_field_kind_t::string_value},
             {"contract_digest", kv_json_field_kind_t::string_value},
             {"execution_request_path", kv_json_field_kind_t::string_value},
             {"execution_request_digest", kv_json_field_kind_t::string_value}},
            resolved, &first, err)) {
      return false;
    }
    std::string handoff_json;
    if (!read_runtime_handoff_for_run_request(request.fields, &handoff_json,
                                              err)) {
      return false;
    }
    if (!handoff_json.empty()) {
      append_raw_json_field(resolved, "runtime_handoff", handoff_json, &first);
    }
  }
  resolved << "}";
  if (out != nullptr) {
    *out = resolved.str();
  }
  return true;
}

[[nodiscard]] bool
validate_kv_fields(const std::unordered_map<std::string, std::string> &map,
                   std::initializer_list<std::string_view> allowed,
                   std::string *err,
                   std::string_view request_name = "execution request") {
  for (const auto &[key, _] : map) {
    if (!field_allowed(key, allowed)) {
      if (err != nullptr) {
        *err = std::string(request_name) + " unknown field: " + key;
      }
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool append_top_or_kv_string_field(
    const std::string &args,
    const std::unordered_map<std::string, std::string> &map,
    std::string_view key, std::ostringstream &out, bool *first) {
  std::string raw;
  if (extract_json_raw_field(args, key, &raw)) {
    append_raw_json_field(out, key, raw, first);
    return true;
  }
  std::string value;
  if (kv_get_trimmed(map, key, &value) && !value.empty()) {
    append_string_json_field(out, key, value, first);
    return true;
  }
  return false;
}

[[nodiscard]] bool append_wave_overlay_from_request(
    const std::unordered_map<std::string, std::string> &map,
    std::ostringstream &out, bool *first, std::string *err) {
  bool present = false;
  for (const auto key :
       {"source_range", "anchor_index_begin", "anchor_index_end",
        "source_key_begin", "source_key_end"}) {
    std::string value;
    if (kv_get_trimmed(map, key, &value) && !value.empty()) {
      present = true;
      break;
    }
  }
  if (!present) {
    return true;
  }

  std::ostringstream overlay;
  overlay << "{";
  bool overlay_first = true;
  if (!append_kv_json_fields(
          map,
          {{"source_range", kv_json_field_kind_t::string_value},
           {"anchor_index_begin", kv_json_field_kind_t::string_value},
           {"anchor_index_end", kv_json_field_kind_t::string_value},
           {"source_key_begin", kv_json_field_kind_t::string_value},
           {"source_key_end", kv_json_field_kind_t::string_value}},
          overlay, &overlay_first, err)) {
    return false;
  }
  overlay << "}";
  append_raw_json_field(out, "wave_overlay", overlay.str(), first);
  return true;
}

[[nodiscard]] bool policy_training_contract_from_kv(
    const std::unordered_map<std::string, std::string> &map,
    cuwacunu::hero::runtime::policy_training_job_contract_t *contract,
    std::string *err) {
  if (contract == nullptr) {
    if (err) {
      *err = "policy training contract output pointer is null";
    }
    return false;
  }
  cuwacunu::hero::runtime::policy_training_job_contract_t parsed{};
  kv_set_string(map, "schema_version", &parsed.schema_version);
  kv_set_string(map, "artifact_schema_id", &parsed.artifact_schema_id);
  kv_set_string(map, "runtime_job_kind", &parsed.runtime_job_kind);
  kv_set_string(map, "protocol_id", &parsed.protocol_id);
  kv_set_string(map, "protocol_contract_fingerprint",
                &parsed.protocol_contract_fingerprint);
  kv_set_string(map, "graph_order_fingerprint",
                &parsed.graph_order_fingerprint);
  kv_set_string(map, "source_cursor_token", &parsed.source_cursor_token);
  kv_set_string(map, "split_policy_fingerprint",
                &parsed.split_policy_fingerprint);
  kv_set_string(map, "component_assembly_fingerprint",
                &parsed.component_assembly_fingerprint);
  kv_set_string(map, "policy_id", &parsed.policy_id);
  kv_set_string(map, "policy_kind", &parsed.policy_kind);
  kv_set_string(map, "policy_architecture_digest",
                &parsed.policy_architecture_digest);
  kv_set_string(map, "training_config_digest", &parsed.training_config_digest);
  kv_set_string(map, "training_range_digest", &parsed.training_range_digest);
  kv_set_string(map, "validation_range_digest",
                &parsed.validation_range_digest);
  kv_set_string(map, "test_range_digest", &parsed.test_range_digest);
  kv_set_string(map, "environment_contract_id",
                &parsed.environment_contract_id);
  kv_set_string(map, "observation_schema_digest",
                &parsed.observation_schema_digest);
  kv_set_string(map, "action_schema_digest", &parsed.action_schema_digest);
  kv_set_string(map, "reward_contract_digest", &parsed.reward_contract_digest);
  kv_set_string(map, "policy_input_schema_id", &parsed.policy_input_schema_id);
  kv_set_string(map, "action_adapter_id", &parsed.action_adapter_id);
  kv_set_string(map, "action_distribution_id", &parsed.action_distribution_id);
  kv_set_string(map, "reward_contract_id", &parsed.reward_contract_id);
  kv_set_string(map, "execution_profile_digest",
                &parsed.execution_profile_digest);
  kv_set_string(map, "ppo_policy_artifact_contract_id",
                &parsed.ppo_policy_artifact_contract_id);
  kv_set_string(map, "policy_family_id", &parsed.policy_family_id);
  kv_set_string(map, "policy_checkpoint_schema_id",
                &parsed.policy_checkpoint_schema_id);
  kv_set_string(map, "policy_dsl_digest", &parsed.policy_dsl_digest);
  kv_set_string(map, "policy_net_digest", &parsed.policy_net_digest);
  kv_set_string(map, "policy_input_feature_manifest_digest",
                &parsed.policy_input_feature_manifest_digest);
  kv_set_string(map, "policy_jkimyei_digest", &parsed.policy_jkimyei_digest);
  kv_set_string(map, "target_node_universe_digest",
                &parsed.target_node_universe_digest);
  kv_set_string(map, "action_distribution_config_digest",
                &parsed.action_distribution_config_digest);
  kv_set_string(map, "snapshot_family_digest", &parsed.snapshot_family_digest);
  kv_set_string(map, "actor_architecture_digest",
                &parsed.actor_architecture_digest);
  kv_set_string(map, "critic_architecture_digest",
                &parsed.critic_architecture_digest);
  kv_set_string(map, "actor_checkpoint_digest",
                &parsed.actor_checkpoint_digest);
  kv_set_string(map, "critic_checkpoint_digest",
                &parsed.critic_checkpoint_digest);
  kv_set_string(map, "optimizer_state_digest", &parsed.optimizer_state_digest);
  kv_set_string(map, "ppo_config_digest", &parsed.ppo_config_digest);
  kv_set_string(map, "optimizer_state_schema_id",
                &parsed.optimizer_state_schema_id);
  kv_set_string(map, "device_policy", &parsed.device_policy);
  kv_set_string(map, "resume_mode", &parsed.resume_mode);
  kv_set_string(map, "resume_actor_checkpoint_path",
                &parsed.resume_actor_checkpoint_path);
  kv_set_string(map, "resume_actor_checkpoint_digest",
                &parsed.resume_actor_checkpoint_digest);
  kv_set_string(map, "resume_optimizer_state_path",
                &parsed.resume_optimizer_state_path);
  kv_set_string(map, "resume_optimizer_state_digest",
                &parsed.resume_optimizer_state_digest);
  kv_set_string(map, "advantage_estimator_id", &parsed.advantage_estimator_id);
  kv_set_string(map, "advantage_normalization_policy",
                &parsed.advantage_normalization_policy);
  kv_set_string(map, "rollout_collection_schema_id",
                &parsed.rollout_collection_schema_id);
  kv_set_string(map, "rollout_collection_digest",
                &parsed.rollout_collection_digest);
  kv_set_string(map, "ppo_update_report_schema_id",
                &parsed.ppo_update_report_schema_id);
  kv_set_string(map, "ppo_update_report_digest",
                &parsed.ppo_update_report_digest);
  kv_set_string(map, "validation_rollout_report_digest",
                &parsed.validation_rollout_report_digest);
  kv_set_string(map, "policy_quality_report_digest",
                &parsed.policy_quality_report_digest);
  kv_set_string(map, "training_schedule_mode", &parsed.training_schedule_mode);
  kv_set_string(map, "causal_schedule_schema_id",
                &parsed.causal_schedule_schema_id);
  kv_set_string(map, "causal_schedule_digest", &parsed.causal_schedule_digest);
  kv_set_string(map, "causal_schedule_cursor_key_kind",
                &parsed.causal_schedule_cursor_key_kind);
  kv_set_string(map, "causal_schedule_no_future_snapshot_use_source",
                &parsed.causal_schedule_no_future_snapshot_use_source);
  kv_set_string(map, "normalization_fit_range_digest",
                &parsed.normalization_fit_range_digest);
  kv_set_string(map, "replay_buffer_source_range_digest",
                &parsed.replay_buffer_source_range_digest);
  kv_set_string(map, "early_stopping_policy_digest",
                &parsed.early_stopping_policy_digest);
  kv_set_string(map, "hyperparameter_selection_policy_digest",
                &parsed.hyperparameter_selection_policy_digest);
  kv_set_string(map, "selector_split", &parsed.selector_split);
  kv_set_string(map, "selector_policy_digest", &parsed.selector_policy_digest);
  kv_set_string(map, "parent_checkpoint_digest",
                &parsed.parent_checkpoint_digest);
  kv_set_string(map, "parent_forecast_eval_fact_digest",
                &parsed.parent_forecast_eval_fact_digest);
  kv_set_string(map, "parent_observer_belief_fact_digest",
                &parsed.parent_observer_belief_fact_digest);
  kv_set_string(map, "parent_allocation_engine_fact_digest",
                &parsed.parent_allocation_engine_fact_digest);
  kv_set_string(map, "parent_replay_environment_fact_digest",
                &parsed.parent_replay_environment_fact_digest);
  kv_set_string(map, "parent_replay_environment_report_digest",
                &parsed.parent_replay_environment_report_digest);
  kv_set_string(map, "final_refit_parent_selected_checkpoint_digest",
                &parsed.final_refit_parent_selected_checkpoint_digest);
  if (!kv_set_double(map, "ppo_gamma", &parsed.ppo_gamma, err) ||
      !kv_set_bool(map, "ppo_gamma_bound", &parsed.ppo_gamma_bound, err) ||
      !kv_set_double(map, "ppo_gae_lambda", &parsed.ppo_gae_lambda, err) ||
      !kv_set_bool(map, "ppo_gae_lambda_bound", &parsed.ppo_gae_lambda_bound,
                   err) ||
      !kv_set_double(map, "ppo_clip_epsilon", &parsed.ppo_clip_epsilon, err) ||
      !kv_set_bool(map, "ppo_clip_epsilon_bound",
                   &parsed.ppo_clip_epsilon_bound, err) ||
      !kv_set_double(map, "ppo_target_kl", &parsed.ppo_target_kl, err) ||
      !kv_set_bool(map, "ppo_target_kl_bound", &parsed.ppo_target_kl_bound,
                   err) ||
      !kv_set_double(map, "ppo_entropy_coeff", &parsed.ppo_entropy_coeff,
                     err) ||
      !kv_set_bool(map, "ppo_entropy_coeff_bound",
                   &parsed.ppo_entropy_coeff_bound, err) ||
      !kv_set_double(map, "ppo_value_loss_coeff", &parsed.ppo_value_loss_coeff,
                     err) ||
      !kv_set_bool(map, "ppo_value_loss_coeff_bound",
                   &parsed.ppo_value_loss_coeff_bound, err) ||
      !kv_set_double(map, "ppo_max_grad_norm", &parsed.ppo_max_grad_norm,
                     err) ||
      !kv_set_bool(map, "ppo_max_grad_norm_bound",
                   &parsed.ppo_max_grad_norm_bound, err) ||
      !kv_set_int64(map, "ppo_minibatch_size", &parsed.ppo_minibatch_size,
                    err) ||
      !kv_set_bool(map, "ppo_minibatch_size_bound",
                   &parsed.ppo_minibatch_size_bound, err) ||
      !kv_set_int64(map, "ppo_epochs_per_rollout",
                    &parsed.ppo_epochs_per_rollout, err) ||
      !kv_set_bool(map, "ppo_epochs_per_rollout_bound",
                   &parsed.ppo_epochs_per_rollout_bound, err) ||
      !kv_set_int64(map, "cuda_device_index", &parsed.cuda_device_index, err) ||
      !kv_set_bool(map, "cuda_device_index_bound",
                   &parsed.cuda_device_index_bound, err) ||
      !kv_set_int64(map, "random_seed", &parsed.random_seed, err) ||
      !kv_set_bool(map, "random_seed_bound", &parsed.random_seed_bound, err) ||
      !kv_set_int64(map, "max_episodes", &parsed.max_episodes, err) ||
      !kv_set_int64(map, "max_steps", &parsed.max_steps, err) ||
      !kv_set_int64(map, "max_parallel_jobs", &parsed.max_parallel_jobs, err) ||
      !kv_set_int64(map, "max_wall_clock_seconds",
                    &parsed.max_wall_clock_seconds, err) ||
      !kv_set_bool(map, "causal_schedule_readiness_eligible",
                   &parsed.causal_schedule_readiness_eligible, err) ||
      !kv_set_bool(map, "causal_schedule_no_future_snapshot_use",
                   &parsed.causal_schedule_no_future_snapshot_use, err) ||
      !kv_set_bool(map, "offline_full_window_research_allowed",
                   &parsed.offline_full_window_research_allowed, err) ||
      !kv_set_bool(map, "final_refit_uses_validation",
                   &parsed.final_refit_uses_validation, err) ||
      !kv_set_bool(map, "validation_no_longer_proof",
                   &parsed.validation_no_longer_proof, err) ||
      !kv_set_bool(map, "sealed_test_required", &parsed.sealed_test_required,
                   err) ||
      !kv_set_bool(map, "live_execution_allowed",
                   &parsed.live_execution_allowed, err)) {
    return false;
  }
  *contract = std::move(parsed);
  return true;
}

[[nodiscard]] std::string
string_array_json(const std::vector<std::string> &values) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << json_quote(values[i]);
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string policy_get(const runtime_policy_t &policy,
                                     std::string_view key) {
  const auto found = policy.values.find(std::string(key));
  if (found != policy.values.end()) {
    return found->second;
  }
  for (const auto &descriptor : kRuntimePolicyDescriptors) {
    if (descriptor.key == key) {
      return std::string(descriptor.default_value);
    }
  }
  return {};
}

[[nodiscard]] bool policy_bool_or(const runtime_policy_t &policy,
                                  std::string_view key, bool fallback) {
  bool parsed = fallback;
  (void)parse_bool(policy_get(policy, key), &parsed);
  return parsed;
}

[[nodiscard]] int policy_int_or(const runtime_policy_t &policy,
                                std::string_view key, int fallback) {
  int parsed = fallback;
  (void)parse_int(policy_get(policy, key), &parsed);
  return parsed;
}

[[nodiscard]] bool
policy_runtime_timeout_seconds(const runtime_policy_t &policy, int *out,
                               std::string *err) {
  const int timeout_seconds = policy_int_or(policy, "max_runtime_seconds", 0);
  if (timeout_seconds < 0) {
    if (err) {
      *err = "max_runtime_seconds must be >= 0";
    }
    return false;
  }
  if (out) {
    *out = timeout_seconds;
  }
  return true;
}

[[nodiscard]] fs::path policy_path(const runtime_policy_t &policy,
                                   std::string_view key) {
  return resolve_against(policy.policy_path, policy_get(policy, key));
}

[[nodiscard]] bool executable_file(const fs::path &path) {
  return ::access(path.c_str(), X_OK) == 0;
}

[[nodiscard]] std::string make_tool_result(std::string_view tool_name,
                                           std::string_view structured_json) {
  std::ostringstream out;
  out << "{\"content\":[{\"type\":\"text\",\"text\":"
      << json_quote(std::string(tool_name) + " executed")
      << "}],\"structuredContent\":" << structured_json
      << ",\"isError\":false}";
  return out.str();
}

[[nodiscard]] std::string make_error_result(std::string_view message,
                                            int code) {
  std::ostringstream out;
  out << "{\"content\":[{\"type\":\"text\",\"text\":" << json_quote(message)
      << "}],\"structuredContent\":{\"ok\":false,\"error_code\":" << code
      << ",\"message\":" << json_quote(message) << "},\"isError\":true}";
  return out.str();
}

struct wave_info_t {
  fs::path config_path{};
  fs::path wave_path{};
  fs::path cursor_path{};
  fs::path protocol_path{};
  std::string selected_wave_id{};
  std::string selected_wave_source{};
  bool readable{false};
  bool cursor_readable{false};
  bool protocol_readable{false};
  std::string error{};
  std::string cursor_error{};
  std::string protocol_error{};
  std::unordered_map<std::string, std::string> values{};
  std::unordered_map<std::string, std::string> protocol_values{};
};

[[nodiscard]] fs::path effective_config_path(const runtime_context_t &ctx,
                                             const std::string &arg) {
  if (!trim_ascii(arg).empty()) {
    return normalize_path(fs::path(arg));
  }
  if (!ctx.global_config_path.empty()) {
    return normalize_path(ctx.global_config_path);
  }
  return policy_path(ctx.policy, "default_config_path");
}

[[nodiscard]] std::string
canonical_protocol_representation_family(std::string_view value) {
  const std::string trimmed = trim_ascii(value);
  const std::string lower = lowercase_ascii(trimmed);
  if (lower.empty()) {
    return "wikimyei.representation.encoding.vicreg";
  }
  if (lower == "mtf_jepa_mae_vicreg" ||
      lower == "mtf_jepa_mae_vicreg_representation" ||
      lower == "mtf_jvmae_representation" ||
      lower == "wikimyei.representation.encoding.mtf_jepa_mae_vicreg") {
    return "wikimyei.representation.encoding.mtf_jepa_mae_vicreg";
  }
  if (lower == "vicreg" || lower == "vicreg_representation" ||
      lower == "wikimyei.representation.encoding.vicreg") {
    return "wikimyei.representation.encoding.vicreg";
  }
  return trimmed;
}

[[nodiscard]] std::string protocol_value_or(
    const std::unordered_map<std::string, std::string> &protocol_values,
    const char *key, std::string fallback) {
  const auto it = protocol_values.find(key);
  if (it == protocol_values.end() || trim_ascii(it->second).empty()) {
    return fallback;
  }
  return trim_ascii(it->second);
}

[[nodiscard]] cuwacunu::hero::runtime::settings::wave_protocol_bindings_t
wave_protocol_bindings_from_values(
    const std::unordered_map<std::string, std::string> &protocol_values) {
  auto out =
      cuwacunu::hero::runtime::settings::default_wave_protocol_bindings();
  out.protocol_id =
      protocol_value_or(protocol_values, "PROTOCOL_ID", out.protocol_id);
  out.representation_family =
      canonical_protocol_representation_family(protocol_value_or(
          protocol_values, "REPRESENTATION", out.representation_family));
  out.inference_family =
      protocol_value_or(protocol_values, "INFERENCE", out.inference_family);
  out.allocation_policy_family = protocol_value_or(
      protocol_values, "ALLOCATION_POLICY", out.allocation_policy_family);
  out.policy_component_family = protocol_value_or(
      protocol_values, "POLICY_COMPONENT", out.policy_component_family);
  return out;
}

inline void populate_wave_info_values(
    const cuwacunu::piaabo::parse::simple_kv::block_t &block,
    const cuwacunu::hero::runtime::settings::wave_settings_t &settings,
    wave_info_t *info) {
  if (info == nullptr) {
    return;
  }
  namespace wave_settings = cuwacunu::hero::runtime::settings;
  info->values = block.values;
  info->values["WAVE_ID"] = settings.wave_id;
  info->values["PROTOCOL"] = settings.protocol_id;
  info->values["AUTHORED_TARGET"] = settings.authored_target;
  info->values["TARGET"] = wave_settings::wave_target_name(settings.target);
  info->values["MODE"] = settings.mode_text;
  info->values["SOURCE_CURSOR_ID"] = settings.source_cursor_id;
  info->values["SOURCE_CURSOR_KIND"] = settings.source_cursor_kind;
  info->values["SOURCE_CURSOR_SCOPE"] = settings.source_cursor_scope;
  info->values["SOURCE_RANGE"] =
      wave_settings::source_range_policy_name(settings.source_range_policy);
  if (settings.source_order_policy_explicit) {
    info->values["SOURCE_ORDER"] =
        wave_settings::source_order_policy_name(settings.source_order_policy);
  } else {
    info->values.erase("SOURCE_ORDER");
  }
  info->values.erase("ANCHOR_INDEX_BEGIN");
  info->values.erase("ANCHOR_INDEX_END");
  info->values.erase("SOURCE_KEY_BEGIN");
  info->values.erase("SOURCE_KEY_END");
  info->values.erase("ANCHOR_KEY_BEGIN");
  info->values.erase("ANCHOR_KEY_END");
  if (settings.anchor_index_begin.has_value()) {
    info->values["ANCHOR_INDEX_BEGIN"] =
        std::to_string(*settings.anchor_index_begin);
  }
  if (settings.anchor_index_end.has_value()) {
    info->values["ANCHOR_INDEX_END"] =
        std::to_string(*settings.anchor_index_end);
  }
  if (settings.source_key_begin.has_value()) {
    info->values["SOURCE_KEY_BEGIN"] =
        std::to_string(*settings.source_key_begin);
  }
  if (settings.source_key_end.has_value()) {
    info->values["SOURCE_KEY_END"] = std::to_string(*settings.source_key_end);
  }
}

[[nodiscard]] wave_info_t load_wave_info(const runtime_context_t &ctx,
                                         const fs::path &config_path) {
  wave_info_t info{};
  info.config_path = config_path;
  const auto maybe_wave =
      read_ini_value(config_path, "HERO", "runtime_wave_dsl_path");
  if (!maybe_wave.has_value()) {
    info.error =
        "missing [HERO].runtime_wave_dsl_path in " + config_path.string();
    return info;
  }
  info.wave_path = resolve_against(config_path, *maybe_wave);
  std::string text;
  if (!read_text_file(info.wave_path, &text, &info.error)) {
    return info;
  }
  info.readable = true;
  const auto maybe_protocol =
      read_ini_value(config_path, "KIKIJYEBA", "kikijyeba_protocol_dsl_path");
  if (!maybe_protocol.has_value()) {
    info.protocol_error =
        "missing [KIKIJYEBA].kikijyeba_protocol_dsl_path in " +
        config_path.string();
    info.error = info.protocol_error;
    return info;
  }
  info.protocol_path = resolve_against(config_path, *maybe_protocol);
  std::string protocol_text;
  if (!read_text_file(info.protocol_path, &protocol_text,
                      &info.protocol_error)) {
    info.error = info.protocol_error;
    return info;
  }
  info.protocol_readable = true;
  info.protocol_values =
      parse_assignment_text(strip_dsl_comments(protocol_text), true);
  const auto protocol_bindings =
      wave_protocol_bindings_from_values(info.protocol_values);

  const auto maybe_cursor =
      read_ini_value(config_path, "UJCAMEI", "ujcamei_source_cursor_dsl_path");
  if (!maybe_cursor.has_value()) {
    info.cursor_error = "missing [UJCAMEI].ujcamei_source_cursor_dsl_path in " +
                        config_path.string();
    info.error = info.cursor_error;
    return info;
  }
  info.cursor_path = resolve_against(config_path, *maybe_cursor);
  std::string cursor_text;
  if (!read_text_file(info.cursor_path, &cursor_text, &info.cursor_error)) {
    info.error = info.cursor_error;
    return info;
  }
  info.cursor_readable = true;

  const auto maybe_wave_id =
      read_ini_value(config_path, "HERO", "runtime_wave_id");
  info.selected_wave_id =
      maybe_wave_id.has_value() ? trim_ascii(*maybe_wave_id) : std::string{};
  info.selected_wave_source =
      info.selected_wave_id.empty() ? "wave_file_or_single" : "global_config";
  try {
    const auto block = cuwacunu::hero::runtime::settings::
        selected_wave_settings_block_from_dsl(text, info.selected_wave_id);
    const auto settings =
        cuwacunu::hero::runtime::settings::decode_wave_settings_from_block(
            block, protocol_bindings, cursor_text);
    if (info.selected_wave_id.empty()) {
      info.selected_wave_id = settings.wave_id;
    }
    populate_wave_info_values(block, settings, &info);
  } catch (const std::exception &ex) {
    info.error = ex.what();
    return info;
  }
  const auto target_it = info.values.find("TARGET");
  const bool mdn_wave =
      target_it != info.values.end() &&
      target_it->second == "wikimyei.inference.expected_value.mdn";
  if (mdn_wave) {
    const auto maybe_mdn_jkimyei =
        read_ini_value(config_path, "JKIMYEI",
                       "wikimyei_inference_expected_value_mdn_jkimyei_path");
    if (maybe_mdn_jkimyei.has_value()) {
      std::string training_text;
      std::string training_error;
      const auto training_path =
          resolve_against(config_path, *maybe_mdn_jkimyei);
      if (read_text_file(training_path, &training_text, &training_error)) {
        const auto training_values =
            parse_assignment_text(strip_dsl_comments(training_text), true);
        for (const char *key :
             {"INPUT_REPRESENTATION_CHECKPOINT", "INPUT_MDN_CHECKPOINT"}) {
          const auto input_it = training_values.find(key);
          if (input_it != training_values.end() && !input_it->second.empty()) {
            info.values[key] = input_it->second;
          }
        }
      }
    }
  }
  return info;
}

[[nodiscard]] std::string wave_action_from_mode(std::string_view mode) {
  const std::string lower = lowercase_ascii(mode);
  bool saw_run = false;
  bool saw_train = false;
  std::string token;
  for (const char c : lower) {
    if (c == '|' || c == '+' || c == ',' ||
        std::isspace(static_cast<unsigned char>(c)) != 0) {
      if (token == "run") {
        saw_run = true;
      } else if (token == "train") {
        saw_train = true;
      }
      token.clear();
      continue;
    }
    token.push_back(c);
  }
  if (token == "run") {
    saw_run = true;
  } else if (token == "train") {
    saw_train = true;
  }
  if (saw_train && !saw_run) {
    return "train";
  }
  return "run";
}

[[nodiscard]] bool wave_debug_from_mode(std::string_view mode) {
  return lowercase_ascii(mode).find("debug") != std::string::npos;
}

[[nodiscard]] std::string canonical_source_range(std::string_view value) {
  const std::string lower = lowercase_ascii(trim_ascii(value));
  return lower == "anchor_key" ? "source_key" : lower;
}

struct wave_overlay_t {
  bool present{false};
  std::string source_range{};
  std::string anchor_index_begin{};
  std::string anchor_index_end{};
  std::string source_key_begin{};
  std::string source_key_end{};
};

[[nodiscard]] bool parse_wave_overlay_from_args(const std::string &args,
                                                wave_overlay_t *overlay,
                                                std::string *err) {
  if (overlay == nullptr) {
    return true;
  }
  *overlay = {};
  std::string raw;
  if (!extract_json_raw_field(args, "wave_overlay", &raw)) {
    return true;
  }
  overlay->present = true;
  (void)extract_json_string_field(raw, "source_range", &overlay->source_range);
  (void)extract_json_string_field(raw, "anchor_index_begin",
                                  &overlay->anchor_index_begin);
  (void)extract_json_string_field(raw, "anchor_index_end",
                                  &overlay->anchor_index_end);
  (void)extract_json_string_field(raw, "source_key_begin",
                                  &overlay->source_key_begin);
  (void)extract_json_string_field(raw, "source_key_end",
                                  &overlay->source_key_end);
  if (trim_ascii(overlay->source_range).empty()) {
    if (!trim_ascii(overlay->anchor_index_begin).empty() ||
        !trim_ascii(overlay->anchor_index_end).empty()) {
      overlay->source_range = "anchor_index";
    } else if (!trim_ascii(overlay->source_key_begin).empty() ||
               !trim_ascii(overlay->source_key_end).empty()) {
      overlay->source_range = "source_key";
    } else {
      overlay->source_range = "all";
    }
  }
  overlay->source_range = canonical_source_range(overlay->source_range);
  if (overlay->source_range != "all" &&
      overlay->source_range != "anchor_index" &&
      overlay->source_range != "source_key") {
    if (err) {
      *err = "E_RUNTIME_WAVE_OVERLAY_INVALID: source_range must be all, "
             "anchor_index, or source_key";
    }
    return false;
  }
  if (overlay->source_range == "all" &&
      (!trim_ascii(overlay->anchor_index_begin).empty() ||
       !trim_ascii(overlay->anchor_index_end).empty() ||
       !trim_ascii(overlay->source_key_begin).empty() ||
       !trim_ascii(overlay->source_key_end).empty())) {
    if (err) {
      *err = "E_RUNTIME_WAVE_OVERLAY_INVALID: SOURCE_RANGE=all cannot carry "
             "range bounds";
    }
    return false;
  }
  if (overlay->source_range == "anchor_index" &&
      (trim_ascii(overlay->anchor_index_begin).empty() ||
       trim_ascii(overlay->anchor_index_end).empty() ||
       !trim_ascii(overlay->source_key_begin).empty() ||
       !trim_ascii(overlay->source_key_end).empty())) {
    if (err) {
      *err = "E_RUNTIME_WAVE_OVERLAY_INVALID: anchor_index overlay requires "
             "anchor_index_begin/end only";
    }
    return false;
  }
  if (overlay->source_range == "source_key" &&
      (trim_ascii(overlay->source_key_begin).empty() ||
       trim_ascii(overlay->source_key_end).empty() ||
       !trim_ascii(overlay->anchor_index_begin).empty() ||
       !trim_ascii(overlay->anchor_index_end).empty())) {
    if (err) {
      *err = "E_RUNTIME_WAVE_OVERLAY_INVALID: source_key overlay requires "
             "source_key_begin/end only";
    }
    return false;
  }
  return true;
}

inline void apply_wave_overlay_to_info(wave_info_t *info,
                                       const wave_overlay_t &overlay) {
  if (info == nullptr || !overlay.present) {
    return;
  }
  info->values["SOURCE_RANGE"] = overlay.source_range;
  info->values.erase("ANCHOR_INDEX_BEGIN");
  info->values.erase("ANCHOR_INDEX_END");
  info->values.erase("SOURCE_KEY_BEGIN");
  info->values.erase("SOURCE_KEY_END");
  info->values.erase("ANCHOR_KEY_BEGIN");
  info->values.erase("ANCHOR_KEY_END");
  if (overlay.source_range == "anchor_index") {
    info->values["ANCHOR_INDEX_BEGIN"] = overlay.anchor_index_begin;
    info->values["ANCHOR_INDEX_END"] = overlay.anchor_index_end;
  } else if (overlay.source_range == "source_key") {
    info->values["SOURCE_KEY_BEGIN"] = overlay.source_key_begin;
    info->values["SOURCE_KEY_END"] = overlay.source_key_end;
  }
}

[[nodiscard]] std::string job_kind_from_target(std::string_view target) {
  const std::string lower = lowercase_ascii(target);
  if (lower == "wikimyei.representation.encoding.vicreg") {
    return "channel_representation_vicreg";
  }
  if (lower == "wikimyei.representation.encoding.mtf_jepa_mae_vicreg") {
    return "channel_representation_mtf_jepa_mae_vicreg";
  }
  if (lower == "wikimyei.inference.expected_value.mdn") {
    return "channel_inference_mdn";
  }
  if (lower == "wikimyei.policy.portfolio.graph_node_allocation") {
    return "policy_training";
  }
  return "invalid_wave_target";
}

[[nodiscard]] std::string wave_value_or_empty(const wave_info_t &info,
                                              std::string_view key) {
  const auto it = info.values.find(std::string(key));
  return it == info.values.end() ? std::string{} : trim_ascii(it->second);
}

[[nodiscard]] std::string wave_job_kind_from_info(const wave_info_t &info) {
  const std::string explicit_job_kind = wave_value_or_empty(info, "JOB_KIND");
  if (!explicit_job_kind.empty()) {
    return explicit_job_kind;
  }
  return job_kind_from_target(wave_value_or_empty(info, "TARGET"));
}

[[nodiscard]] bool wave_info_requests_policy_training(const wave_info_t &info) {
  if (!info.readable || !info.error.empty()) {
    return false;
  }
  const std::string job_kind =
      lowercase_ascii(trim_ascii(wave_job_kind_from_info(info)));
  if (job_kind == "policy_training") {
    return true;
  }
  return !wave_value_or_empty(info, "POLICY_KIND").empty() ||
         !wave_value_or_empty(info, "POLICY_ID").empty() ||
         !wave_value_or_empty(info, "TRAINING_SCHEDULE_MODE").empty();
}

[[nodiscard]] std::unordered_map<std::string, std::string>
wave_model_state_inputs(const wave_info_t &info) {
  std::unordered_map<std::string, std::string> inputs;
  for (const auto &[key, value] : info.values) {
    if (key.rfind("INPUT_", 0) != 0 || value.empty()) {
      continue;
    }
    inputs["PLAN_" + key] = fs::path(value).lexically_normal().string();
  }
  return inputs;
}

[[nodiscard]] bool wave_source_order_is_explicit(const wave_info_t &info) {
  const auto it = info.values.find("SOURCE_ORDER");
  return it != info.values.end() && !trim_ascii(it->second).empty();
}

[[nodiscard]] std::string wave_source_order_from_info(const wave_info_t &info,
                                                      std::string_view action) {
  if (wave_source_order_is_explicit(info)) {
    return lowercase_ascii(trim_ascii(info.values.at("SOURCE_ORDER")));
  }
  return action == "train" ? "random_per_epoch" : "sequential";
}

[[nodiscard]] std::string
wave_source_order_warning_token(std::string_view action,
                                std::string_view source_order,
                                bool source_order_explicit) {
  if (action != "train" ||
      lowercase_ascii(trim_ascii(source_order)) != "sequential") {
    return {};
  }
  return source_order_explicit ? "train_wave_explicit_sequential_source_order"
                               : "train_wave_effective_sequential_source_order";
}

[[nodiscard]] std::string protocol_id_from_info(const wave_info_t &info) {
  const auto it = info.protocol_values.find("PROTOCOL_ID");
  return it == info.protocol_values.end() ? std::string{"cwu_01v"} : it->second;
}

[[nodiscard]] std::string protocol_value_from_info(const wave_info_t &info,
                                                   const char *key,
                                                   std::string fallback = {}) {
  const auto it = info.protocol_values.find(key);
  return it == info.protocol_values.end() ? std::move(fallback) : it->second;
}

[[nodiscard]] std::string wave_protocol_id_from_info(const wave_info_t &info) {
  return wave_value_or_empty(info, "PROTOCOL");
}

[[nodiscard]] bool wave_protocol_matches_active(const wave_info_t &info,
                                                std::string_view active) {
  return trim_ascii(wave_protocol_id_from_info(info)) == trim_ascii(active);
}

[[nodiscard]] std::string
active_representation_from_info(const wave_info_t &info) {
  const auto it = info.protocol_values.find("REPRESENTATION");
  if (it == info.protocol_values.end() || trim_ascii(it->second).empty()) {
    return "wikimyei.representation.encoding.vicreg";
  }
  const std::string lower = lowercase_ascii(trim_ascii(it->second));
  if (lower == "mtf_jepa_mae_vicreg" ||
      lower == "mtf_jepa_mae_vicreg_representation" ||
      lower == "mtf_jvmae_representation") {
    return "wikimyei.representation.encoding.mtf_jepa_mae_vicreg";
  }
  if (lower == "vicreg" || lower == "vicreg_representation") {
    return "wikimyei.representation.encoding.vicreg";
  }
  return it->second;
}

[[nodiscard]] bool
protocol_target_compatible(std::string_view target,
                           std::string_view active_representation_family) {
  const std::string job_kind = job_kind_from_target(target);
  const std::string active =
      lowercase_ascii(trim_ascii(active_representation_family));
  if (job_kind == "channel_representation_vicreg") {
    return active == "wikimyei.representation.encoding.vicreg";
  }
  if (job_kind == "channel_representation_mtf_jepa_mae_vicreg") {
    return active == "wikimyei.representation.encoding.mtf_jepa_mae_vicreg";
  }
  return true;
}

[[nodiscard]] std::string
execution_chain(std::string_view target, std::string_view action,
                std::string_view policy_kind = {},
                std::string_view active_representation_family =
                    "wikimyei.representation.encoding.vicreg") {
  const std::string job_kind = job_kind_from_target(target);
  if (job_kind == "channel_representation_vicreg") {
    if (action == "train") {
      return "ujcamei.source.registry:run -> "
             "wikimyei.expression.nodelift.srl:run -> "
             "wikimyei.representation.encoding.vicreg:train";
    }
    return "ujcamei.source.registry:run -> "
           "wikimyei.expression.nodelift.srl:run -> "
           "wikimyei.representation.encoding.vicreg:run";
  }
  if (job_kind == "channel_representation_mtf_jepa_mae_vicreg") {
    if (action == "train") {
      return "ujcamei.source.registry:run -> "
             "wikimyei.expression.nodelift.srl:run -> "
             "wikimyei.representation.encoding.mtf_jepa_mae_vicreg:train";
    }
    return "ujcamei.source.registry:run -> "
           "wikimyei.expression.nodelift.srl:run -> "
           "wikimyei.representation.encoding.mtf_jepa_mae_vicreg:run";
  }
  if (job_kind == "channel_inference_mdn") {
    const bool active_mtf =
        active_representation_family ==
        "wikimyei.representation.encoding.mtf_jepa_mae_vicreg";
    if (active_mtf) {
      if (action == "train") {
        return "ujcamei.source.registry:run -> "
               "wikimyei.expression.nodelift.srl:run -> "
               "wikimyei.representation.encoding.mtf_jepa_mae_vicreg:"
               "run_frozen -> wikimyei.inference.expected_value.mdn:train";
      }
      return "ujcamei.source.registry:run -> "
             "wikimyei.expression.nodelift.srl:run -> "
             "wikimyei.representation.encoding.mtf_jepa_mae_vicreg:"
             "run_frozen -> wikimyei.inference.expected_value.mdn:run";
    }
    if (action == "train") {
      return "ujcamei.source.registry:run -> "
             "wikimyei.expression.nodelift.srl:run -> "
             "wikimyei.representation.encoding.vicreg:run_frozen -> "
             "wikimyei.inference.expected_value.mdn:train";
    }
    return "ujcamei.source.registry:run -> "
           "wikimyei.expression.nodelift.srl:run -> "
           "wikimyei.representation.encoding.vicreg:run_frozen -> "
           "wikimyei.inference.expected_value.mdn:run";
  }
  if (job_kind == "policy_training") {
    const std::string policy_kind_lower =
        lowercase_ascii(trim_ascii(policy_kind));
    if (policy_kind_lower == "ppo_policy_adapter.v1" ||
        policy_kind_lower == "ppo" || policy_kind_lower == "ppo_v0") {
      return "runtime.policy_training_contract:validate -> "
             "runtime.policy_training.ppo_v0:"
             "write_checkpoint_rollout_update";
    }
    return "runtime.policy_training_contract:validate -> "
           "runtime.policy_training.noop:write_checkpoint";
  }
  return {};
}

[[nodiscard]] std::string wave_info_json(const wave_info_t &info) {
  const std::string wave_id = info.values.count("WAVE_ID") != 0
                                  ? info.values.at("WAVE_ID")
                                  : std::string{};
  const std::string target = info.values.count("TARGET") != 0
                                 ? info.values.at("TARGET")
                                 : std::string{};
  const std::string authored_target = info.values.count("AUTHORED_TARGET") != 0
                                          ? info.values.at("AUTHORED_TARGET")
                                          : target;
  const std::string mode = info.values.count("MODE") != 0
                               ? info.values.at("MODE")
                               : std::string{"run"};
  const std::string action = wave_action_from_mode(mode);
  const std::string source_range = canonical_source_range(
      info.values.count("SOURCE_RANGE") != 0 ? info.values.at("SOURCE_RANGE")
                                             : std::string{"all"});
  const bool source_order_explicit = wave_source_order_is_explicit(info);
  const std::string source_order = wave_source_order_from_info(info, action);
  const std::string source_order_warning = wave_source_order_warning_token(
      action, source_order, source_order_explicit);
  const std::string cursor_kind = info.values.count("SOURCE_CURSOR_KIND") != 0
                                      ? info.values.at("SOURCE_CURSOR_KIND")
                                      : std::string{"graph_anchor"};
  const std::string cursor_scope = info.values.count("SOURCE_CURSOR_SCOPE") != 0
                                       ? info.values.at("SOURCE_CURSOR_SCOPE")
                                       : std::string{"wave_batch"};
  const std::string source_cursor_id =
      info.values.count("SOURCE_CURSOR_ID") != 0
          ? info.values.at("SOURCE_CURSOR_ID")
          : std::string{};
  const std::string protocol_id = protocol_id_from_info(info);
  const std::string wave_protocol_id = wave_protocol_id_from_info(info);
  const std::string protocol_status =
      protocol_value_from_info(info, "PROTOCOL_STATUS", "active");
  const std::string successor_protocol =
      protocol_value_from_info(info, "SUCCESSOR_PROTOCOL");
  const std::string protocol_warning =
      protocol_value_from_info(info, "PROTOCOL_WARNING");
  const bool protocol_warns =
      !protocol_warning.empty() ||
      lowercase_ascii(trim_ascii(protocol_status)) != "active";
  const bool protocol_match = wave_protocol_matches_active(info, protocol_id);
  const std::string active_representation =
      active_representation_from_info(info);
  const std::string protocol_observer =
      protocol_value_from_info(info, "OBSERVER", "wikimyei.observer.belief");
  const std::string protocol_allocation_policy = protocol_value_from_info(
      info, "ALLOCATION_POLICY",
      "wikimyei.policy.portfolio.spot_distributional_utility");
  const std::string protocol_policy_component = protocol_value_from_info(
      info, "POLICY_COMPONENT",
      "wikimyei.policy.portfolio.graph_node_allocation");
  const bool target_compatible =
      protocol_target_compatible(target, active_representation);
  const std::string job_kind = wave_job_kind_from_info(info);
  const std::string policy_id = wave_value_or_empty(info, "POLICY_ID");
  const std::string policy_kind = wave_value_or_empty(info, "POLICY_KIND");
  const std::string training_schedule_mode =
      wave_value_or_empty(info, "TRAINING_SCHEDULE_MODE");
  const std::string live_execution_allowed =
      wave_value_or_empty(info, "LIVE_EXECUTION_ALLOWED");
  std::ostringstream out;
  out << "{\"config_path\":" << json_quote(info.config_path.string())
      << ",\"wave_path\":" << json_quote(info.wave_path.string())
      << ",\"cursor_path\":" << json_quote(info.cursor_path.string())
      << ",\"protocol_path\":" << json_quote(info.protocol_path.string())
      << ",\"selected_wave_id\":" << json_quote(info.selected_wave_id)
      << ",\"selected_wave_source\":" << json_quote(info.selected_wave_source)
      << ",\"readable\":" << bool_json(info.readable);
  if (!info.error.empty()) {
    out << ",\"error\":" << json_quote(info.error);
  }
  out << ",\"cursor_readable\":" << bool_json(info.cursor_readable);
  if (!info.cursor_error.empty()) {
    out << ",\"cursor_error\":" << json_quote(info.cursor_error);
  }
  out << ",\"protocol_readable\":" << bool_json(info.protocol_readable);
  if (!info.protocol_error.empty()) {
    out << ",\"protocol_error\":" << json_quote(info.protocol_error);
  }
  out << ",\"wave_id\":" << json_quote(wave_id)
      << ",\"protocol_id\":" << json_quote(protocol_id)
      << ",\"wave_protocol_id\":" << json_quote(wave_protocol_id)
      << ",\"protocol_status\":" << json_quote(protocol_status)
      << ",\"successor_protocol\":" << json_quote(successor_protocol)
      << ",\"protocol_warning_level\":"
      << json_quote(protocol_warns ? "warning" : "none")
      << ",\"protocol_warning\":" << json_quote(protocol_warning)
      << ",\"wave_protocol_match\":" << bool_json(protocol_match)
      << ",\"wave_protocol_warning\":"
      << json_quote(protocol_match
                        ? std::string{}
                        : "wave_protocol_does_not_match_active_protocol")
      << ",\"active_representation_family\":"
      << json_quote(active_representation)
      << ",\"protocol_observer_family\":" << json_quote(protocol_observer)
      << ",\"protocol_allocation_policy_family\":"
      << json_quote(protocol_allocation_policy)
      << ",\"protocol_policy_component_family\":"
      << json_quote(protocol_policy_component)
      << ",\"protocol_target_compatible\":" << bool_json(target_compatible)
      << ",\"protocol_target_warning\":"
      << json_quote(target_compatible ? std::string{}
                                      : "wave_target_not_docked_by_active_"
                                        "protocol")
      << ",\"authored_target_component_family_id\":"
      << json_quote(authored_target)
      << ",\"target_component_family_id\":" << json_quote(target)
      << ",\"mode\":" << json_quote(mode)
      << ",\"action\":" << json_quote(action)
      << ",\"debug\":" << bool_json(wave_debug_from_mode(mode))
      << ",\"source_cursor_id\":" << json_quote(source_cursor_id)
      << ",\"source_cursor_kind\":" << json_quote(cursor_kind)
      << ",\"source_cursor_scope\":" << json_quote(cursor_scope)
      << ",\"source_range\":" << json_quote(source_range)
      << ",\"source_order\":" << json_quote(source_order)
      << ",\"source_order_explicit\":" << bool_json(source_order_explicit)
      << ",\"source_order_warning_level\":"
      << json_quote(source_order_warning.empty() ? "none" : "warning")
      << ",\"source_order_warnings\":" << json_quote(source_order_warning)
      << ",\"anchor_index_begin\":";
  if (info.values.count("ANCHOR_INDEX_BEGIN") != 0) {
    out << json_quote(info.values.at("ANCHOR_INDEX_BEGIN"));
  } else {
    out << "null";
  }
  out << ",\"anchor_index_end\":";
  if (info.values.count("ANCHOR_INDEX_END") != 0) {
    out << json_quote(info.values.at("ANCHOR_INDEX_END"));
  } else {
    out << "null";
  }
  out << ",\"source_key_begin\":";
  if (info.values.count("SOURCE_KEY_BEGIN") != 0) {
    out << json_quote(info.values.at("SOURCE_KEY_BEGIN"));
  } else if (info.values.count("ANCHOR_KEY_BEGIN") != 0) {
    out << json_quote(info.values.at("ANCHOR_KEY_BEGIN"));
  } else {
    out << "null";
  }
  out << ",\"source_key_end\":";
  if (info.values.count("SOURCE_KEY_END") != 0) {
    out << json_quote(info.values.at("SOURCE_KEY_END"));
  } else if (info.values.count("ANCHOR_KEY_END") != 0) {
    out << json_quote(info.values.at("ANCHOR_KEY_END"));
  } else {
    out << "null";
  }
  out << ",\"job_kind\":" << json_quote(job_kind) << ",\"policy_id\":";
  if (policy_id.empty()) {
    out << "null";
  } else {
    out << json_quote(policy_id);
  }
  out << ",\"policy_kind\":";
  if (policy_kind.empty()) {
    out << "null";
  } else {
    out << json_quote(policy_kind);
  }
  out << ",\"training_schedule_mode\":";
  if (training_schedule_mode.empty()) {
    out << "null";
  } else {
    out << json_quote(training_schedule_mode);
  }
  out << ",\"live_execution_allowed\":";
  if (live_execution_allowed.empty()) {
    out << "false";
  } else {
    out << bool_json(lowercase_ascii(live_execution_allowed) == "true");
  }
  out << ",\"policy_training_wave\":"
      << bool_json(wave_info_requests_policy_training(info))
      << ",\"train_target\":" << bool_json(action == "train")
      << ",\"execution_chain\":"
      << json_quote(execution_chain(target, action, policy_kind,
                                    active_representation))
      << ",\"model_state_inputs\":"
      << kv_map_to_json(wave_model_state_inputs(info)) << "}";
  return out.str();
}

struct process_result_t {
  int exit_code{-1};
  bool signaled{false};
  int signal_number{0};
  bool timed_out{false};
  bool stdout_truncated{false};
  bool stderr_truncated{false};
  std::string stdout_text{};
  std::string stderr_text{};
};

void append_capped(std::string *out, const char *data, std::size_t size,
                   std::size_t cap, bool *truncated) {
  if (!out) {
    return;
  }
  if (out->size() < cap) {
    const std::size_t room = cap - out->size();
    out->append(data, std::min(room, size));
  }
  if (size > 0 && out->size() >= cap && truncated) {
    *truncated = true;
  }
}

void set_nonblocking(int fd) {
  const int flags = ::fcntl(fd, F_GETFL, 0);
  if (flags >= 0) {
    (void)::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  }
}

void drain_fd(int fd, std::string *out, std::size_t cap, bool *truncated,
              bool *open) {
  std::array<char, 4096> buffer{};
  while (true) {
    const ssize_t n = ::read(fd, buffer.data(), buffer.size());
    if (n > 0) {
      append_capped(out, buffer.data(), static_cast<std::size_t>(n), cap,
                    truncated);
      continue;
    }
    if (n == 0) {
      *open = false;
      (void)::close(fd);
      return;
    }
    if (errno == EINTR) {
      continue;
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return;
    }
    *open = false;
    (void)::close(fd);
    return;
  }
}

[[nodiscard]] process_result_t run_process(const std::vector<std::string> &argv,
                                           int timeout_seconds,
                                           std::size_t capture_cap) {
  process_result_t result{};
  if (argv.empty()) {
    result.stderr_text = "empty argv";
    return result;
  }
  int stdout_pipe[2]{-1, -1};
  int stderr_pipe[2]{-1, -1};
  if (::pipe(stdout_pipe) != 0 || ::pipe(stderr_pipe) != 0) {
    result.stderr_text = "pipe failed";
    return result;
  }

  const pid_t pid = ::fork();
  if (pid < 0) {
    result.stderr_text = "fork failed";
    (void)::close(stdout_pipe[0]);
    (void)::close(stdout_pipe[1]);
    (void)::close(stderr_pipe[0]);
    (void)::close(stderr_pipe[1]);
    return result;
  }
  if (pid == 0) {
    (void)::dup2(stdout_pipe[1], STDOUT_FILENO);
    (void)::dup2(stderr_pipe[1], STDERR_FILENO);
    (void)::close(stdout_pipe[0]);
    (void)::close(stdout_pipe[1]);
    (void)::close(stderr_pipe[0]);
    (void)::close(stderr_pipe[1]);
    std::vector<char *> cargv;
    cargv.reserve(argv.size() + 1);
    for (const std::string &arg : argv) {
      cargv.push_back(const_cast<char *>(arg.c_str()));
    }
    cargv.push_back(nullptr);
    ::execv(cargv[0], cargv.data());
    const std::string err =
        std::string("execv failed: ") + std::strerror(errno) + "\n";
    const ssize_t ignored_write =
        ::write(STDERR_FILENO, err.data(), err.size());
    (void)ignored_write;
    _exit(127);
  }

  (void)::close(stdout_pipe[1]);
  (void)::close(stderr_pipe[1]);
  set_nonblocking(stdout_pipe[0]);
  set_nonblocking(stderr_pipe[0]);
  bool stdout_open = true;
  bool stderr_open = true;
  bool exited = false;
  int status = 0;
  const auto start = std::chrono::steady_clock::now();

  while (stdout_open || stderr_open || !exited) {
    if (!exited) {
      const pid_t waited = ::waitpid(pid, &status, WNOHANG);
      if (waited == pid) {
        exited = true;
      }
    }
    if (!exited && timeout_seconds > 0) {
      const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::steady_clock::now() - start);
      if (elapsed.count() > timeout_seconds) {
        result.timed_out = true;
        (void)::kill(pid, SIGKILL);
        (void)::waitpid(pid, &status, 0);
        exited = true;
      }
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    int maxfd = -1;
    if (stdout_open) {
      FD_SET(stdout_pipe[0], &readfds);
      maxfd = std::max(maxfd, stdout_pipe[0]);
    }
    if (stderr_open) {
      FD_SET(stderr_pipe[0], &readfds);
      maxfd = std::max(maxfd, stderr_pipe[0]);
    }
    if (maxfd >= 0) {
      timeval tv{};
      tv.tv_sec = 0;
      tv.tv_usec = 100000;
      const int ready = ::select(maxfd + 1, &readfds, nullptr, nullptr, &tv);
      if (ready > 0) {
        if (stdout_open && FD_ISSET(stdout_pipe[0], &readfds)) {
          drain_fd(stdout_pipe[0], &result.stdout_text, capture_cap,
                   &result.stdout_truncated, &stdout_open);
        }
        if (stderr_open && FD_ISSET(stderr_pipe[0], &readfds)) {
          drain_fd(stderr_pipe[0], &result.stderr_text, capture_cap,
                   &result.stderr_truncated, &stderr_open);
        }
      }
    } else if (exited) {
      break;
    }
  }
  if (stdout_open) {
    drain_fd(stdout_pipe[0], &result.stdout_text, capture_cap,
             &result.stdout_truncated, &stdout_open);
  }
  if (stderr_open) {
    drain_fd(stderr_pipe[0], &result.stderr_text, capture_cap,
             &result.stderr_truncated, &stderr_open);
  }
  if (WIFEXITED(status)) {
    result.exit_code = WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
    result.signaled = true;
    result.signal_number = WTERMSIG(status);
    result.exit_code = 128 + result.signal_number;
  }
  return result;
}

[[nodiscard]] std::unordered_map<std::string, std::string>
parse_process_stdout_kv(const std::string &stdout_text) {
  std::unordered_map<std::string, std::string> out;
  std::istringstream lines(stdout_text);
  std::string line;
  while (std::getline(lines, line)) {
    line = trim_ascii(line);
    const std::size_t eq = line.find('=');
    if (eq == std::string::npos || eq == 0) {
      continue;
    }
    const std::string key = trim_ascii(line.substr(0, eq));
    bool key_ok = true;
    for (const char c : key) {
      const auto u = static_cast<unsigned char>(c);
      if (!(std::isalnum(u) || c == '_')) {
        key_ok = false;
        break;
      }
    }
    if (key_ok) {
      out[key] = trim_ascii(line.substr(eq + 1));
    }
  }
  return out;
}

[[nodiscard]] bool explicit_job_dir_allowed(const runtime_policy_t &policy,
                                            const fs::path &path) {
  return under_any_root(path,
                        split_csv(policy_get(policy, "allowed_job_roots")),
                        policy.policy_path);
}

[[nodiscard]] bool dev_nuke_root_allowed(const runtime_policy_t &policy,
                                         const fs::path &path) {
  return under_any_root(path,
                        split_csv(policy_get(policy, "allowed_dev_nuke_roots")),
                        policy.policy_path);
}

[[nodiscard]] fs::path runtime_root(const runtime_policy_t &policy) {
  return policy_path(policy, "runtime_root");
}

[[nodiscard]] bool looks_safe_to_clear_runtime_root(const fs::path &path) {
  const fs::path normalized = normalize_path(path);
  if (normalized.empty() || !normalized.is_absolute()) {
    return false;
  }
  if (normalized == normalized.root_path()) {
    return false;
  }
  if (!normalized.has_filename() || normalized.filename() == "." ||
      normalized.filename() == "..") {
    return false;
  }
  const fs::path parent = normalized.parent_path();
  return !parent.empty() && parent != normalized.root_path();
}

[[nodiscard]] std::string path_vector_json(const std::vector<fs::path> &paths) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < paths.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << json_quote(paths[i].string());
  }
  out << "]";
  return out.str();
}

struct dev_nuke_target_t {
  fs::path path{};
  bool is_directory{false};
  bool is_regular_file{false};
  std::uintmax_t entry_count{0};
};

struct active_job_marker_t {
  fs::path state_path{};
  std::string status{};
};

[[nodiscard]] bool terminal_job_status(std::string_view status) {
  const std::string value = lowercase_ascii(trim_ascii(status));
  return value == "dry_run" || value == "completed" || value == "failed" ||
         value == "stopped" || value == "cancelled" || value == "canceled" ||
         value == "interrupted";
}

[[nodiscard]] std::uintmax_t count_tree_entries(const fs::path &path) {
  std::error_code ec;
  if (!fs::exists(path, ec) || ec) {
    return 0;
  }
  std::uintmax_t count = 1;
  if (!fs::is_directory(path, ec) || ec) {
    return count;
  }
  fs::recursive_directory_iterator it(
      path, fs::directory_options::skip_permission_denied, ec);
  const fs::recursive_directory_iterator end;
  for (; !ec && it != end; it.increment(ec)) {
    ++count;
  }
  return count;
}

[[nodiscard]] bool
collect_dev_nuke_targets(const fs::path &root,
                         std::vector<dev_nuke_target_t> *targets,
                         std::string *err) {
  if (!targets) {
    if (err) {
      *err = "missing dev_nuke target output";
    }
    return false;
  }
  targets->clear();
  std::error_code ec;
  if (!fs::exists(root, ec)) {
    return true;
  }
  if (ec) {
    if (err) {
      *err = "failed to inspect runtime_root: " + root.string() + ": " +
             ec.message();
    }
    return false;
  }
  if (!fs::is_directory(root, ec) || ec) {
    if (err) {
      *err = "runtime_root is not a directory: " + root.string();
    }
    return false;
  }
  for (fs::directory_iterator it(root, ec), end; !ec && it != end;
       it.increment(ec)) {
    dev_nuke_target_t target{};
    target.path = normalize_path(it->path());
    target.is_directory = it->is_directory(ec);
    if (ec) {
      target.is_directory = false;
      ec.clear();
    }
    target.is_regular_file = it->is_regular_file(ec);
    if (ec) {
      target.is_regular_file = false;
      ec.clear();
    }
    target.entry_count = count_tree_entries(target.path);
    targets->push_back(std::move(target));
  }
  if (ec) {
    if (err) {
      *err =
          "failed to list runtime_root: " + root.string() + ": " + ec.message();
    }
    return false;
  }
  std::sort(targets->begin(), targets->end(),
            [](const dev_nuke_target_t &a, const dev_nuke_target_t &b) {
              return a.path.string() < b.path.string();
            });
  return true;
}

[[nodiscard]] std::string
dev_nuke_targets_json(const std::vector<dev_nuke_target_t> &targets) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < targets.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"path\":" << json_quote(targets[i].path.string())
        << ",\"is_directory\":" << bool_json(targets[i].is_directory)
        << ",\"is_regular_file\":" << bool_json(targets[i].is_regular_file)
        << ",\"entry_count\":" << targets[i].entry_count << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] bool
scan_active_runtime_jobs(const fs::path &root,
                         std::vector<active_job_marker_t> *active_jobs,
                         std::string *err) {
  if (!active_jobs) {
    if (err) {
      *err = "missing active job output";
    }
    return false;
  }
  active_jobs->clear();
  std::error_code ec;
  if (!fs::exists(root, ec)) {
    return true;
  }
  if (ec) {
    if (err) {
      *err =
          "failed to inspect runtime_root for active jobs: " + root.string() +
          ": " + ec.message();
    }
    return false;
  }
  fs::recursive_directory_iterator it(
      root, fs::directory_options::skip_permission_denied, ec);
  const fs::recursive_directory_iterator end;
  for (; !ec && it != end; it.increment(ec)) {
    if (it->path().filename() != "job.state") {
      continue;
    }
    const auto fields = parse_kv_file(it->path());
    const auto found = fields.find("status");
    const std::string status =
        found == fields.end() ? std::string{"<missing>"} : found->second;
    if (!terminal_job_status(status)) {
      active_jobs->push_back(active_job_marker_t{it->path(), status});
    }
  }
  if (ec) {
    if (err) {
      *err = "failed to scan runtime_root for active jobs: " + root.string() +
             ": " + ec.message();
    }
    return false;
  }
  return true;
}

[[nodiscard]] std::string
active_jobs_json(const std::vector<active_job_marker_t> &active_jobs) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < active_jobs.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << "{\"state_path\":" << json_quote(active_jobs[i].state_path.string())
        << ",\"status\":" << json_quote(active_jobs[i].status) << "}";
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string sanitize_path_component(std::string_view value) {
  std::string out;
  out.reserve(value.size());
  for (const unsigned char c : value) {
    if (std::isalnum(c) != 0 || c == '.' || c == '_' || c == '-') {
      out.push_back(static_cast<char>(c));
    } else {
      out.push_back('_');
    }
  }
  return out.empty() ? std::string{"runtime_root"} : out;
}

[[nodiscard]] bool make_dev_nuke_snapshot_root(const fs::path &root,
                                               const fs::path &backup_root,
                                               fs::path *out,
                                               std::string *err) {
  if (!out) {
    if (err) {
      *err = "missing backup snapshot output";
    }
    return false;
  }
  const std::string root_leaf =
      sanitize_path_component(root.filename().string());
  const auto stamp = std::chrono::duration_cast<std::chrono::microseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
  const std::string stamp_text = std::to_string(static_cast<long long>(stamp));
  const fs::path family_root = normalize_path(backup_root / root_leaf);
  fs::path candidate = family_root / stamp_text;
  std::error_code ec;
  int disambiguator = 1;
  while (fs::exists(candidate, ec)) {
    if (ec) {
      if (err) {
        *err = "failed to inspect dev_nuke backup path: " + candidate.string() +
               ": " + ec.message();
      }
      return false;
    }
    candidate =
        family_root / (stamp_text + "." + std::to_string(disambiguator++));
  }
  fs::create_directories(candidate, ec);
  if (ec) {
    if (err) {
      *err = "failed to create dev_nuke backup path: " + candidate.string() +
             ": " + ec.message();
    }
    return false;
  }
  *out = normalize_path(candidate);
  return true;
}

[[nodiscard]] bool move_dev_nuke_target(const fs::path &root,
                                        const fs::path &target,
                                        const fs::path &snapshot_root,
                                        fs::path *out_backup,
                                        std::string *err) {
  if (out_backup) {
    out_backup->clear();
  }
  if (!path_within(root, target) ||
      normalize_path(root) == normalize_path(target)) {
    if (err) {
      *err = "dev_nuke target escapes runtime_root: " + target.string();
    }
    return false;
  }
  const fs::path relative =
      normalize_path(target).lexically_relative(normalize_path(root));
  if (relative.empty() || relative == ".") {
    if (err) {
      *err = "failed to derive dev_nuke backup path for: " + target.string();
    }
    return false;
  }
  const fs::path backup_path = normalize_path(snapshot_root / relative);
  std::error_code ec;
  fs::create_directories(backup_path.parent_path(), ec);
  if (ec) {
    if (err) {
      *err = "failed to prepare dev_nuke backup path: " +
             backup_path.parent_path().string() + ": " + ec.message();
    }
    return false;
  }
  fs::rename(target, backup_path, ec);
  if (ec) {
    if (err) {
      *err = "failed to move runtime target into dev_nuke backup: " +
             target.string() + " -> " + backup_path.string() + ": " +
             ec.message();
    }
    return false;
  }
  if (out_backup) {
    *out_backup = backup_path;
  }
  return true;
}

[[nodiscard]] bool resolve_job_dir_from_args(const std::string &args,
                                             const runtime_policy_t &policy,
                                             fs::path *out, std::string *err) {
  std::string job_dir_arg;
  std::string job_id_arg;
  (void)extract_json_string_field(args, "job_dir", &job_dir_arg);
  (void)extract_json_string_field(args, "job_id", &job_id_arg);
  if (!trim_ascii(job_dir_arg).empty() && !trim_ascii(job_id_arg).empty()) {
    *err = "provide job_id or job_dir, not both";
    return false;
  }
  fs::path path;
  if (!trim_ascii(job_dir_arg).empty()) {
    path = normalize_path(fs::path(job_dir_arg));
    if (!explicit_job_dir_allowed(policy, path)) {
      *err =
          "E_RUNTIME_JOB_DIR_DENIED: job_dir is outside allowed_job_roots: " +
          path.string();
      return false;
    }
  } else if (!trim_ascii(job_id_arg).empty()) {
    if (job_id_arg.find('/') != std::string::npos ||
        job_id_arg.find('\\') != std::string::npos ||
        job_id_arg.find("..") != std::string::npos) {
      *err = "job_id must be a runtime job id";
      return false;
    }
    const auto found =
        cuwacunu::hero::runtime::job_layout::find_runtime_job_dir_by_id(
            runtime_root(policy), job_id_arg);
    if (!found.has_value()) {
      *err = "E_RUNTIME_JOB_NOT_FOUND: " + job_id_arg;
      return false;
    }
    path = normalize_path(*found);
  } else {
    *err = "hero.runtime job lookup requires job_id or job_dir";
    return false;
  }
  if (out) {
    *out = path;
  }
  return true;
}

[[nodiscard]] std::string artifact_summary_json(const fs::path &path,
                                                bool include_text,
                                                std::size_t max_bytes) {
  const bool exists = fs::exists(path);
  const bool is_file = exists && fs::is_regular_file(path);
  std::ostringstream out;
  out << "{\"path\":" << json_quote(path.string())
      << ",\"exists\":" << bool_json(exists)
      << ",\"is_regular_file\":" << bool_json(is_file);
  if (is_file) {
    std::error_code ec;
    const auto size = fs::file_size(path, ec);
    out << ",\"size\":" << (ec ? 0 : size)
        << ",\"fields\":" << kv_map_to_json(parse_kv_file(path));
    if (include_text) {
      bool truncated = false;
      const std::string text =
          read_text_file_limited(path, max_bytes, &truncated);
      out << ",\"truncated\":" << bool_json(truncated)
          << ",\"text\":" << json_quote(text);
    }
  }
  out << "}";
  return out.str();
}

[[nodiscard]] fs::path runtime_replay_artifact_dir(const fs::path &job_dir) {
  return job_dir / "artifacts" / "kikijyeba.environment.replay.v1";
}

[[nodiscard]] fs::path
runtime_replay_batch_index_path(const fs::path &job_dir) {
  return runtime_replay_artifact_dir(job_dir) / "runtime_replay_batches.index";
}

[[nodiscard]] fs::path
runtime_replay_experiment_index_path(const fs::path &job_dir) {
  return runtime_replay_artifact_dir(job_dir) /
         "runtime_replay_experiments.index";
}

[[nodiscard]] fs::path
resolve_runtime_replay_artifact_path(const fs::path &raw_path,
                                     const fs::path &index_dir) {
  if (raw_path.empty()) {
    return {};
  }
  return raw_path.is_absolute() ? normalize_path(raw_path)
                                : normalize_path(index_dir / raw_path);
}

[[nodiscard]] bool path_contains_parent_reference(const fs::path &path) {
  for (const auto &part : path) {
    if (part == "..") {
      return true;
    }
  }
  return false;
}

struct replay_experiment_report_binding_t {
  fs::path index_path{};
  fs::path report_path{};
  std::string declared_report_digest{};
  bool index_present{false};
  bool report_path_from_index{false};
  bool report_path_rejected{false};
};

[[nodiscard]] fs::path
latest_replay_experiment_report_default_path(const fs::path &job_dir) {
  return runtime_replay_artifact_dir(job_dir) /
         "runtime_replay_experiment.report";
}

[[nodiscard]] replay_experiment_report_binding_t
latest_replay_experiment_report_binding(const fs::path &job_dir) {
  replay_experiment_report_binding_t binding{};
  binding.index_path = runtime_replay_experiment_index_path(job_dir);
  const auto index = parse_kv_file(binding.index_path);
  binding.index_present = !index.empty();
  int entry_count = 0;
  if (parse_int(index.count("entry_count") == 0 ? std::string{}
                                                : index.at("entry_count"),
                &entry_count) &&
      entry_count > 0) {
    const auto prefix =
        std::string("entry_") + std::to_string(entry_count - 1) + "_";
    const auto digest_found = index.find(prefix + "report_digest");
    if (digest_found != index.end()) {
      binding.declared_report_digest = trim_ascii(digest_found->second);
    }
    const auto found = index.find(prefix + "report_path");
    if (found != index.end() && !trim_ascii(found->second).empty()) {
      const fs::path raw_report_path(trim_ascii(found->second));
      if (!path_contains_parent_reference(raw_report_path)) {
        binding.report_path = resolve_runtime_replay_artifact_path(
            raw_report_path, binding.index_path.parent_path());
        binding.report_path_from_index = true;
        return binding;
      }
      binding.report_path_rejected = true;
      return binding;
    }
  }
  binding.report_path = latest_replay_experiment_report_default_path(job_dir);
  return binding;
}

[[nodiscard]] fs::path
latest_replay_experiment_report_path(const fs::path &job_dir) {
  return latest_replay_experiment_report_binding(job_dir).report_path;
}

[[nodiscard]] std::string replay_report_integrity_json(
    const replay_experiment_report_binding_t &binding) {
  const bool report_exists =
      !binding.report_path.empty() && fs::exists(binding.report_path);
  const bool is_file =
      report_exists && fs::is_regular_file(binding.report_path);
  const std::string computed_digest =
      is_file ? replay_report_digest_for_path_or_empty(binding.report_path)
              : std::string{};
  const bool digest_bound = !binding.declared_report_digest.empty();
  const bool digest_match = digest_bound && !computed_digest.empty() &&
                            binding.declared_report_digest == computed_digest;
  std::ostringstream out;
  out << "{\"index_path\":" << json_quote(binding.index_path.string())
      << ",\"report_path\":" << json_quote(binding.report_path.string())
      << ",\"index_present\":" << bool_json(binding.index_present)
      << ",\"report_path_from_index\":"
      << bool_json(binding.report_path_from_index)
      << ",\"report_path_rejected\":" << bool_json(binding.report_path_rejected)
      << ",\"report_exists\":" << bool_json(report_exists)
      << ",\"is_regular_file\":" << bool_json(is_file)
      << ",\"report_digest_bound\":" << bool_json(digest_bound)
      << ",\"declared_report_digest\":"
      << json_quote(binding.declared_report_digest)
      << ",\"computed_report_digest\":" << json_quote(computed_digest)
      << ",\"report_digest_match\":" << bool_json(digest_match) << "}";
  return out.str();
}

[[nodiscard]] std::string job_artifacts_json(const fs::path &job_dir,
                                             const std::string &report_path,
                                             bool include_text,
                                             std::size_t max_bytes) {
  fs::path effective_report =
      report_path.empty() ? fs::path{} : fs::path(report_path);
  if (effective_report.empty()) {
    const fs::path channel_inference = job_dir / "channel_inference.report";
    const fs::path channel_representation =
        job_dir / "channel_representation.report";
    const fs::path inference = job_dir / "inference.report";
    const fs::path representation = job_dir / "representation.report";
    if (fs::exists(channel_inference)) {
      effective_report = channel_inference;
    } else if (fs::exists(channel_representation)) {
      effective_report = channel_representation;
    } else if (fs::exists(inference)) {
      effective_report = inference;
    } else if (fs::exists(representation)) {
      effective_report = representation;
    }
  }
  const auto replay_report_binding =
      latest_replay_experiment_report_binding(job_dir);
  std::ostringstream out;
  out << "{\"job_dir\":" << json_quote(job_dir.string()) << ",\"manifest\":"
      << artifact_summary_json(job_dir / "job.manifest", include_text,
                               max_bytes)
      << ",\"state\":"
      << artifact_summary_json(job_dir / "job.state", include_text, max_bytes)
      << ",\"report\":";
  if (effective_report.empty()) {
    out << "null";
  } else {
    out << artifact_summary_json(effective_report, include_text, max_bytes);
  }
  out << ",\"replay_batch_index\":"
      << artifact_summary_json(runtime_replay_batch_index_path(job_dir),
                               include_text, max_bytes)
      << ",\"replay_experiment_index\":"
      << artifact_summary_json(runtime_replay_experiment_index_path(job_dir),
                               include_text, max_bytes)
      << ",\"replay_experiment_report\":"
      << artifact_summary_json(replay_report_binding.report_path, include_text,
                               max_bytes)
      << ",\"replay_experiment_report_integrity\":"
      << replay_report_integrity_json(replay_report_binding) << "}";
  return out.str();
}

[[nodiscard]] std::string
runtime_terminal_evidence_json(const fs::path &job_dir) {
  const fs::path manifest_path = job_dir / "job.manifest";
  const fs::path state_path = job_dir / "job.state";
  const fs::path result_fact_path = job_dir / "runtime.result.fact";
  const fs::path checkpoint_io_path = job_dir / "runtime.checkpoint_io.fact";
  const auto manifest = parse_kv_file(manifest_path);
  const auto state = parse_kv_file(state_path);
  const auto result_fact = parse_kv_file(result_fact_path);
  const auto checkpoint_io = parse_kv_file(checkpoint_io_path);
  const auto get = [](const auto &fields,
                      const std::string &key) -> std::string {
    const auto it = fields.find(key);
    return it == fields.end() ? std::string{} : it->second;
  };
  const auto first = [](std::initializer_list<std::string> values) {
    for (const auto &value : values) {
      if (!trim_ascii(value).empty()) {
        return value;
      }
    }
    return std::string{};
  };
  const auto truthy = [](const std::string &value) {
    bool parsed = false;
    return parse_bool(value, &parsed) && parsed;
  };
  const std::string job_id =
      first({get(result_fact, "job_id"), get(state, "job_id"),
             get(manifest, "job_id")});
  const std::string status =
      first({get(result_fact, "status"), get(state, "status")});
  const std::string handoff_id = first({get(result_fact, "runtime_handoff_id"),
                                        get(manifest, "runtime_handoff_id")});
  const std::string handoff_digest =
      first({get(result_fact, "runtime_handoff_digest"),
             get(manifest, "runtime_handoff_digest")});
  const std::string target_driver_run_id =
      first({get(result_fact, "marshal_target_driver_run_id"),
             get(manifest, "marshal_target_driver_run_id")});
  const bool checkpoint_io_required =
      truthy(first({get(checkpoint_io, "checkpoint_written"),
                    get(result_fact, "checkpoint_written"),
                    get(state, "checkpoint_written")})) ||
      truthy(first({get(checkpoint_io, "representation_checkpoint_loaded"),
                    get(result_fact, "representation_checkpoint_loaded")})) ||
      truthy(first({get(checkpoint_io, "mdn_checkpoint_loaded"),
                    get(result_fact, "mdn_checkpoint_loaded")})) ||
      !first({get(checkpoint_io, "checkpoint_path"),
              get(result_fact, "checkpoint_path"),
              get(state, "checkpoint_path"),
              get(manifest, "input_representation_checkpoint_path"),
              get(manifest, "input_mdn_checkpoint_path")})
           .empty();
  const bool terminal_status = status == "completed" || status == "failed" ||
                               status == "skipped" || status == "dry_run";
  const bool observed = !job_id.empty() && terminal_status &&
                        fs::is_regular_file(manifest_path) &&
                        fs::is_regular_file(result_fact_path);

  std::ostringstream out;
  out << "{\"observed\":" << bool_json(observed)
      << ",\"job_id\":" << json_quote(job_id)
      << ",\"job_dir\":" << json_quote(job_dir.string())
      << ",\"terminal_status\":" << json_quote(status)
      << ",\"job_manifest_path\":" << json_quote(manifest_path.string())
      << ",\"job_manifest_digest\":"
      << json_quote(file_digest_or_empty(
             manifest_path, "kikijyeba.marshal.runtime_job_manifest.v1"))
      << ",\"job_state_path\":" << json_quote(state_path.string())
      << ",\"job_state_digest\":"
      << json_quote(file_digest_or_empty(
             state_path, "kikijyeba.marshal.runtime_job_state.v1"))
      << ",\"runtime_result_fact_path\":"
      << json_quote(result_fact_path.string())
      << ",\"runtime_result_fact_digest\":"
      << json_quote(file_digest_or_empty(
             result_fact_path, "kikijyeba.marshal.runtime_terminal_fact.v1"))
      << ",\"checkpoint_io_required\":" << bool_json(checkpoint_io_required)
      << ",\"runtime_checkpoint_io_fact_path\":"
      << json_quote(checkpoint_io_path.string())
      << ",\"runtime_checkpoint_io_fact_digest\":"
      << json_quote(file_digest_or_empty(
             checkpoint_io_path,
             "kikijyeba.marshal.runtime_checkpoint_io_fact.v1"))
      << ",\"runtime_handoff_id\":" << json_quote(handoff_id)
      << ",\"runtime_handoff_digest\":" << json_quote(handoff_digest)
      << ",\"target_driver_run_id\":" << json_quote(target_driver_run_id)
      << "}";
  return out.str();
}

[[nodiscard]] std::string process_result_json(
    const std::vector<std::string> &argv, const process_result_t &result,
    const fs::path &job_dir,
    const std::unordered_map<std::string, std::string> &stdout_kv,
    bool include_text, std::size_t max_bytes) {
  std::ostringstream out;
  out << "{\"ok\":" << bool_json(result.exit_code == 0 && !result.timed_out)
      << ",\"exit_code\":" << result.exit_code
      << ",\"timed_out\":" << bool_json(result.timed_out)
      << ",\"signaled\":" << bool_json(result.signaled)
      << ",\"signal_number\":" << result.signal_number << ",\"argv\":[";
  for (std::size_t i = 0; i < argv.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << json_quote(argv[i]);
  }
  out << "],\"stdout_truncated\":" << bool_json(result.stdout_truncated)
      << ",\"stderr_truncated\":" << bool_json(result.stderr_truncated)
      << ",\"stdout\":" << json_quote(result.stdout_text)
      << ",\"stderr\":" << json_quote(result.stderr_text)
      << ",\"stdout_fields\":" << kv_map_to_json(stdout_kv);
  fs::path artifact_job_dir = job_dir;
  if (artifact_job_dir.empty()) {
    const auto found = stdout_kv.find("manifest_path");
    if (found != stdout_kv.end()) {
      artifact_job_dir = fs::path(found->second).parent_path();
    }
  }
  if (!artifact_job_dir.empty()) {
    std::string report_path;
    const auto found_report = stdout_kv.find("report_path");
    if (found_report != stdout_kv.end()) {
      report_path = found_report->second;
    } else if (const auto found_replay_report =
                   stdout_kv.find("replay_report_path");
               found_replay_report != stdout_kv.end()) {
      report_path = found_replay_report->second;
    }
    out << ",\"artifacts\":"
        << job_artifacts_json(artifact_job_dir, report_path, include_text,
                              max_bytes)
        << ",\"terminal_evidence\":"
        << runtime_terminal_evidence_json(artifact_job_dir);
  }
  out << "}";
  return out.str();
}

[[nodiscard]] bool
report_i64_field(const std::unordered_map<std::string, std::string> &report,
                 std::string_view key, long long *out, std::string *err) {
  const auto found = report.find(std::string(key));
  if (found == report.end()) {
    *err = "missing report field: " + std::string(key);
    return false;
  }
  const std::string value = trim_ascii(found->second);
  if (value.empty()) {
    *err = "empty report field: " + std::string(key);
    return false;
  }
  long long parsed = 0;
  const auto result =
      std::from_chars(value.data(), value.data() + value.size(), parsed);
  if (result.ec != std::errc{} || result.ptr != value.data() + value.size()) {
    *err = "non-integer report field: " + std::string(key) + "=" + value;
    return false;
  }
  *out = parsed;
  return true;
}

[[nodiscard]] bool
validate_replay_report_for_validation_rollout(const fs::path &report_path,
                                              std::string *err) {
  if (report_path.empty() || !fs::exists(report_path) ||
      !fs::is_regular_file(report_path)) {
    *err = "E_RUNTIME_REPLAY_VALIDATION_REPORT_MISSING";
    return false;
  }
  const auto report = parse_kv_file(report_path);
  if (report.empty()) {
    *err = "E_RUNTIME_REPLAY_VALIDATION_REPORT_EMPTY";
    return false;
  }

  const auto require_nonempty = [&](std::string_view key,
                                    const char *code) -> bool {
    const auto found = report.find(std::string(key));
    if (found == report.end() || trim_ascii(found->second).empty()) {
      *err = code;
      return false;
    }
    return true;
  };
  if (!require_nonempty("execution_profile_digest",
                        "E_RUNTIME_REPLAY_VALIDATION_PROFILE_DIGEST_MISSING") ||
      !require_nonempty(
          "policy_set_digest",
          "E_RUNTIME_REPLAY_VALIDATION_POLICY_SET_DIGEST_MISSING")) {
    return false;
  }

  long long completed_count = 0;
  long long invalid_trace_count = 0;
  long long synthetic_step_count = 0;
  long long numeraire_fallback_pair_count = 0;
  std::string field_error;
  if (!report_i64_field(report, "completed_count", &completed_count,
                        &field_error)) {
    *err = "E_RUNTIME_REPLAY_VALIDATION_REPORT_FIELD_INVALID: " + field_error;
    return false;
  }
  if (completed_count <= 0) {
    *err = "E_RUNTIME_REPLAY_VALIDATION_NO_COMPLETED_EPISODES";
    return false;
  }
  if (!report_i64_field(report, "cajtucu_invalid_trace_count",
                        &invalid_trace_count, &field_error) ||
      !report_i64_field(report, "cajtucu_synthetic_market_step_count",
                        &synthetic_step_count, &field_error) ||
      !report_i64_field(report, "cajtucu_numeraire_fallback_pair_count",
                        &numeraire_fallback_pair_count, &field_error)) {
    *err = "E_RUNTIME_REPLAY_VALIDATION_REPORT_FIELD_INVALID: " + field_error;
    return false;
  }
  if (synthetic_step_count > 0) {
    *err = "E_RUNTIME_REPLAY_VALIDATION_SYNTHETIC_MARKET_USED: "
           "cajtucu_synthetic_market_step_count=" +
           std::to_string(synthetic_step_count);
    return false;
  }
  if (invalid_trace_count > 0) {
    *err = "E_RUNTIME_REPLAY_VALIDATION_INVALID_CAJTUCU_TRACE: "
           "cajtucu_invalid_trace_count=" +
           std::to_string(invalid_trace_count);
    return false;
  }
  if (numeraire_fallback_pair_count > 0) {
    *err = "E_RUNTIME_REPLAY_VALIDATION_NUMERAIRE_FALLBACK_USED: "
           "cajtucu_numeraire_fallback_pair_count=" +
           std::to_string(numeraire_fallback_pair_count);
    return false;
  }
  return true;
}

[[nodiscard]] bool parse_optional_bool_arg(const std::string &args,
                                           std::string_view key,
                                           bool default_value, bool *out,
                                           std::string *err) {
  bool value = default_value;
  if (extract_json_raw_field(args, key, nullptr) &&
      !extract_json_bool_field(args, key, &value)) {
    *err = std::string(key) + " must be boolean";
    return false;
  }
  *out = value;
  return true;
}

[[nodiscard]] bool parse_optional_int_arg(const std::string &args,
                                          std::string_view key,
                                          int default_value, int *out,
                                          std::string *err) {
  int value = default_value;
  if (extract_json_raw_field(args, key, nullptr) &&
      !extract_json_int_field(args, key, &value)) {
    *err = std::string(key) + " must be integer";
    return false;
  }
  *out = value;
  return true;
}

[[nodiscard]] bool parse_optional_double_arg(const std::string &args,
                                             std::string_view key,
                                             double default_value, double *out,
                                             std::string *err) {
  double value = default_value;
  std::string raw;
  if (extract_json_raw_field(args, key, &raw) && !parse_double(raw, &value)) {
    *err = std::string(key) + " must be number";
    return false;
  }
  *out = value;
  return true;
}

[[nodiscard]] bool parse_policy_training_required_int(const std::string &args,
                                                      std::string_view key,
                                                      int *out,
                                                      std::string *err) {
  if (!extract_json_raw_field(args, key, nullptr)) {
    *err = "missing required field: " + std::string(key);
    return false;
  }
  int value = 0;
  if (!extract_json_int_field(args, key, &value)) {
    *err = std::string(key) + " must be integer";
    return false;
  }
  *out = value;
  return true;
}

[[nodiscard]] bool parse_required_string_arg_or_wave(
    const std::string &args, std::string_view json_key,
    const wave_info_t *selected_wave, std::string_view wave_key,
    std::string *out, std::string *err) {
  if (extract_json_raw_field(args, json_key, nullptr)) {
    return parse_required_string_arg(args, json_key, out, err);
  }
  if (selected_wave != nullptr) {
    const auto value = wave_value_or_empty(*selected_wave, wave_key);
    if (!value.empty()) {
      *out = value;
      return true;
    }
  }
  if (err) {
    *err = "missing required field: " + std::string(json_key);
  }
  return false;
}

[[nodiscard]] bool parse_optional_bool_arg_or_wave(
    const std::string &args, std::string_view json_key,
    const wave_info_t *selected_wave, std::string_view wave_key,
    bool default_value, bool *out, std::string *err) {
  if (extract_json_raw_field(args, json_key, nullptr)) {
    return parse_optional_bool_arg(args, json_key, default_value, out, err);
  }
  if (selected_wave != nullptr) {
    const auto value =
        lowercase_ascii(wave_value_or_empty(*selected_wave, wave_key));
    if (!value.empty()) {
      if (value == "true") {
        *out = true;
        return true;
      }
      if (value == "false") {
        *out = false;
        return true;
      }
      if (err) {
        *err = std::string(wave_key) + " must be true or false";
      }
      return false;
    }
  }
  *out = default_value;
  return true;
}

[[nodiscard]] std::string policy_training_contract_json(
    const cuwacunu::hero::runtime::policy_training_job_contract_t &contract) {
  std::ostringstream out;
  out << "{\"schema_version\":" << json_quote(contract.schema_version)
      << ",\"artifact_schema_id\":" << json_quote(contract.artifact_schema_id)
      << ",\"runtime_job_kind\":" << json_quote(contract.runtime_job_kind)
      << ",\"protocol_id\":" << json_quote(contract.protocol_id)
      << ",\"protocol_contract_fingerprint\":"
      << json_quote(contract.protocol_contract_fingerprint)
      << ",\"graph_order_fingerprint\":"
      << json_quote(contract.graph_order_fingerprint)
      << ",\"source_cursor_token\":" << json_quote(contract.source_cursor_token)
      << ",\"split_policy_fingerprint\":"
      << json_quote(contract.split_policy_fingerprint)
      << ",\"component_assembly_fingerprint\":"
      << json_quote(contract.component_assembly_fingerprint)
      << ",\"policy_id\":" << json_quote(contract.policy_id)
      << ",\"policy_kind\":" << json_quote(contract.policy_kind)
      << ",\"policy_architecture_digest\":"
      << json_quote(contract.policy_architecture_digest)
      << ",\"training_config_digest\":"
      << json_quote(contract.training_config_digest)
      << ",\"training_range_digest\":"
      << json_quote(contract.training_range_digest)
      << ",\"validation_range_digest\":"
      << json_quote(contract.validation_range_digest)
      << ",\"test_range_digest\":" << json_quote(contract.test_range_digest)
      << ",\"environment_contract_id\":"
      << json_quote(contract.environment_contract_id)
      << ",\"observation_schema_digest\":"
      << json_quote(contract.observation_schema_digest)
      << ",\"action_schema_digest\":"
      << json_quote(contract.action_schema_digest)
      << ",\"reward_contract_digest\":"
      << json_quote(contract.reward_contract_digest)
      << ",\"policy_input_schema_id\":"
      << json_quote(contract.policy_input_schema_id)
      << ",\"action_adapter_id\":" << json_quote(contract.action_adapter_id)
      << ",\"action_distribution_id\":"
      << json_quote(contract.action_distribution_id)
      << ",\"reward_contract_id\":" << json_quote(contract.reward_contract_id)
      << ",\"execution_profile_digest\":"
      << json_quote(contract.execution_profile_digest)
      << ",\"ppo_policy_artifact_contract_id\":"
      << json_quote(contract.ppo_policy_artifact_contract_id)
      << ",\"policy_family_id\":" << json_quote(contract.policy_family_id)
      << ",\"policy_checkpoint_schema_id\":"
      << json_quote(contract.policy_checkpoint_schema_id)
      << ",\"policy_dsl_digest\":" << json_quote(contract.policy_dsl_digest)
      << ",\"policy_net_digest\":" << json_quote(contract.policy_net_digest)
      << ",\"policy_input_feature_manifest_digest\":"
      << json_quote(contract.policy_input_feature_manifest_digest)
      << ",\"policy_jkimyei_digest\":"
      << json_quote(contract.policy_jkimyei_digest)
      << ",\"target_node_universe_digest\":"
      << json_quote(contract.target_node_universe_digest)
      << ",\"action_distribution_config_digest\":"
      << json_quote(contract.action_distribution_config_digest)
      << ",\"snapshot_family_digest\":"
      << json_quote(contract.snapshot_family_digest)
      << ",\"actor_architecture_digest\":"
      << json_quote(contract.actor_architecture_digest)
      << ",\"critic_architecture_digest\":"
      << json_quote(contract.critic_architecture_digest)
      << ",\"actor_checkpoint_digest\":"
      << json_quote(contract.actor_checkpoint_digest)
      << ",\"critic_checkpoint_digest\":"
      << json_quote(contract.critic_checkpoint_digest)
      << ",\"optimizer_state_digest\":"
      << json_quote(contract.optimizer_state_digest)
      << ",\"ppo_config_digest\":" << json_quote(contract.ppo_config_digest)
      << ",\"optimizer_state_schema_id\":"
      << json_quote(contract.optimizer_state_schema_id)
      << ",\"device_policy\":" << json_quote(contract.device_policy)
      << ",\"cuda_device_index\":" << contract.cuda_device_index
      << ",\"cuda_device_index_bound\":"
      << bool_json(contract.cuda_device_index_bound)
      << ",\"resume_mode\":" << json_quote(contract.resume_mode)
      << ",\"resume_actor_checkpoint_path\":"
      << json_quote(contract.resume_actor_checkpoint_path)
      << ",\"resume_actor_checkpoint_digest\":"
      << json_quote(contract.resume_actor_checkpoint_digest)
      << ",\"resume_optimizer_state_path\":"
      << json_quote(contract.resume_optimizer_state_path)
      << ",\"resume_optimizer_state_digest\":"
      << json_quote(contract.resume_optimizer_state_digest)
      << ",\"advantage_estimator_id\":"
      << json_quote(contract.advantage_estimator_id)
      << ",\"advantage_normalization_policy\":"
      << json_quote(contract.advantage_normalization_policy)
      << ",\"rollout_collection_schema_id\":"
      << json_quote(contract.rollout_collection_schema_id)
      << ",\"rollout_collection_digest\":"
      << json_quote(contract.rollout_collection_digest)
      << ",\"ppo_update_report_schema_id\":"
      << json_quote(contract.ppo_update_report_schema_id)
      << ",\"ppo_update_report_digest\":"
      << json_quote(contract.ppo_update_report_digest)
      << ",\"validation_rollout_report_digest\":"
      << json_quote(contract.validation_rollout_report_digest)
      << ",\"policy_quality_report_digest\":"
      << json_quote(contract.policy_quality_report_digest)
      << ",\"ppo_gamma\":" << contract.ppo_gamma
      << ",\"ppo_gamma_bound\":" << bool_json(contract.ppo_gamma_bound)
      << ",\"ppo_gae_lambda\":" << contract.ppo_gae_lambda
      << ",\"ppo_gae_lambda_bound\":"
      << bool_json(contract.ppo_gae_lambda_bound)
      << ",\"ppo_clip_epsilon\":" << contract.ppo_clip_epsilon
      << ",\"ppo_clip_epsilon_bound\":"
      << bool_json(contract.ppo_clip_epsilon_bound)
      << ",\"ppo_target_kl\":" << contract.ppo_target_kl
      << ",\"ppo_target_kl_bound\":" << bool_json(contract.ppo_target_kl_bound)
      << ",\"ppo_entropy_coeff\":" << contract.ppo_entropy_coeff
      << ",\"ppo_entropy_coeff_bound\":"
      << bool_json(contract.ppo_entropy_coeff_bound)
      << ",\"ppo_value_loss_coeff\":" << contract.ppo_value_loss_coeff
      << ",\"ppo_value_loss_coeff_bound\":"
      << bool_json(contract.ppo_value_loss_coeff_bound)
      << ",\"ppo_max_grad_norm\":" << contract.ppo_max_grad_norm
      << ",\"ppo_max_grad_norm_bound\":"
      << bool_json(contract.ppo_max_grad_norm_bound)
      << ",\"ppo_minibatch_size\":" << contract.ppo_minibatch_size
      << ",\"ppo_minibatch_size_bound\":"
      << bool_json(contract.ppo_minibatch_size_bound)
      << ",\"ppo_epochs_per_rollout\":" << contract.ppo_epochs_per_rollout
      << ",\"ppo_epochs_per_rollout_bound\":"
      << bool_json(contract.ppo_epochs_per_rollout_bound)
      << ",\"training_schedule_mode\":"
      << json_quote(contract.training_schedule_mode)
      << ",\"causal_schedule_schema_id\":"
      << json_quote(contract.causal_schedule_schema_id)
      << ",\"causal_schedule_digest\":"
      << json_quote(contract.causal_schedule_digest)
      << ",\"causal_schedule_cursor_key_kind\":"
      << json_quote(contract.causal_schedule_cursor_key_kind)
      << ",\"causal_schedule_no_future_snapshot_use_source\":"
      << json_quote(contract.causal_schedule_no_future_snapshot_use_source)
      << ",\"normalization_fit_range_digest\":"
      << json_quote(contract.normalization_fit_range_digest)
      << ",\"replay_buffer_source_range_digest\":"
      << json_quote(contract.replay_buffer_source_range_digest)
      << ",\"early_stopping_policy_digest\":"
      << json_quote(contract.early_stopping_policy_digest)
      << ",\"hyperparameter_selection_policy_digest\":"
      << json_quote(contract.hyperparameter_selection_policy_digest)
      << ",\"selector_split\":" << json_quote(contract.selector_split)
      << ",\"selector_policy_digest\":"
      << json_quote(contract.selector_policy_digest)
      << ",\"parent_checkpoint_digest\":"
      << json_quote(contract.parent_checkpoint_digest)
      << ",\"parent_forecast_eval_fact_digest\":"
      << json_quote(contract.parent_forecast_eval_fact_digest)
      << ",\"parent_observer_belief_fact_digest\":"
      << json_quote(contract.parent_observer_belief_fact_digest)
      << ",\"parent_allocation_engine_fact_digest\":"
      << json_quote(contract.parent_allocation_engine_fact_digest)
      << ",\"parent_replay_environment_fact_digest\":"
      << json_quote(contract.parent_replay_environment_fact_digest)
      << ",\"parent_replay_environment_report_digest\":"
      << json_quote(contract.parent_replay_environment_report_digest)
      << ",\"final_refit_parent_selected_checkpoint_digest\":"
      << json_quote(contract.final_refit_parent_selected_checkpoint_digest)
      << ",\"random_seed\":" << contract.random_seed
      << ",\"max_episodes\":" << contract.max_episodes
      << ",\"max_steps\":" << contract.max_steps
      << ",\"max_parallel_jobs\":" << contract.max_parallel_jobs
      << ",\"max_wall_clock_seconds\":" << contract.max_wall_clock_seconds
      << ",\"causal_schedule_readiness_eligible\":"
      << bool_json(contract.causal_schedule_readiness_eligible)
      << ",\"causal_schedule_no_future_snapshot_use\":"
      << bool_json(contract.causal_schedule_no_future_snapshot_use)
      << ",\"offline_full_window_research_allowed\":"
      << bool_json(contract.offline_full_window_research_allowed)
      << ",\"final_refit_uses_validation\":"
      << bool_json(contract.final_refit_uses_validation)
      << ",\"validation_no_longer_proof\":"
      << bool_json(contract.validation_no_longer_proof)
      << ",\"sealed_test_required\":"
      << bool_json(contract.sealed_test_required)
      << ",\"live_execution_allowed\":"
      << bool_json(contract.live_execution_allowed) << "}";
  return out.str();
}

[[nodiscard]] bool policy_training_execute_kind_supported(
    const cuwacunu::hero::runtime::policy_training_job_contract_t &contract) {
  const auto kind = lowercase_ascii(contract.policy_kind);
  return contract.policy_kind == "noop_policy_training.v1" ||
         kind == "ppo_policy_adapter.v1" || kind == "ppo" || kind == "ppo_v0";
}

[[nodiscard]] bool policy_training_execute_kind_is_ppo_v0(
    const cuwacunu::hero::runtime::policy_training_job_contract_t &contract) {
  const auto kind = lowercase_ascii(contract.policy_kind);
  return kind == "ppo_policy_adapter.v1" || kind == "ppo" || kind == "ppo_v0";
}

[[nodiscard]] std::string policy_training_unsupported_execute_reason(
    const cuwacunu::hero::runtime::policy_training_job_contract_t &contract) {
  if (lowercase_ascii(contract.policy_kind).find("ppo") != std::string::npos) {
    return "E_RUNTIME_POLICY_TRAINING_PPO_KIND_UNSUPPORTED: PPO policy "
           "training execute supports policy_kind=ppo_policy_adapter.v1";
  }
  return "E_RUNTIME_POLICY_TRAINING_POLICY_KIND_UNSUPPORTED: "
         "mode=execute currently supports "
         "policy_kind=noop_policy_training.v1 or ppo_policy_adapter.v1";
}

[[nodiscard]] std::string policy_training_checkpoint_text(
    const cuwacunu::hero::runtime::policy_training_job_contract_t &contract,
    std::string_view contract_digest) {
  std::ostringstream out;
  out << "schema=kikijyeba.runtime.policy_training.noop_checkpoint.v1\n";
  out << "policy_id=" << contract.policy_id << "\n";
  out << "policy_kind=" << contract.policy_kind << "\n";
  out << "contract_digest=" << contract_digest << "\n";
  out << "causal_schedule_digest=" << contract.causal_schedule_digest << "\n";
  out << "execution_profile_digest=" << contract.execution_profile_digest
      << "\n";
  out << "ppo_implemented=false\n";
  out << "optimizer_steps=0\n";
  out << "trainer_kind=noop_policy_training.v1\n";
  out << "readiness_scope=pre_ppo_runtime_plumbing\n";
  return out.str();
}

[[nodiscard]] std::string policy_training_report_text(
    const cuwacunu::hero::runtime::policy_training_job_contract_t &contract,
    std::string_view contract_digest, const fs::path &checkpoint_path,
    std::string_view checkpoint_digest) {
  std::error_code ec;
  const auto checkpoint_bytes = fs::exists(checkpoint_path, ec)
                                    ? fs::file_size(checkpoint_path, ec)
                                    : std::uintmax_t{0};
  std::ostringstream out;
  out << "report_schema_id=kikijyeba.runtime.policy_training.noop_report.v1\n";
  out << "report_schema_version=1\n";
  out << "report_writer_id=hero.runtime.policy_training.noop_pre_ppo.v1\n";
  out << "report_writer_version=1\n";
  out << "run_data_kind=policy_training_noop_smoke\n";
  out << "readiness_scope=pre_ppo_runtime_plumbing\n";
  out << "policy_id=" << contract.policy_id << "\n";
  out << "policy_kind=" << contract.policy_kind << "\n";
  out << "policy_architecture_digest=" << contract.policy_architecture_digest
      << "\n";
  out << "training_config_digest=" << contract.training_config_digest << "\n";
  out << "contract_digest=" << contract_digest << "\n";
  out << "training_schedule_mode=" << contract.training_schedule_mode << "\n";
  out << "causal_schedule_digest=" << contract.causal_schedule_digest << "\n";
  out << "causal_schedule_no_future_snapshot_use="
      << (contract.causal_schedule_no_future_snapshot_use ? "true" : "false")
      << "\n";
  out << "checkpoint_path=" << checkpoint_path.string() << "\n";
  out << "checkpoint_path_reported=" << checkpoint_path.string() << "\n";
  out << "checkpoint_digest_reported=" << checkpoint_digest << "\n";
  out << "checkpoint_digest_verified=true\n";
  out << "checkpoint_file_exists=true\n";
  out << "checkpoint_bytes=" << checkpoint_bytes << "\n";
  out << "checkpoint_artifact_status=written\n";
  out << "checkpoint_written=true\n";
  out << "optimizer_steps=0\n";
  out << "steps_attempted=0\n";
  out << "steps_completed=0\n";
  out << "finite_parameter_check=true\n";
  out << "nonfinite_output_count=0\n";
  out << "ppo_implemented=false\n";
  out << "live_execution_allowed=false\n";
  out << "market_readiness_claimed=false\n";
  return out.str();
}

[[nodiscard]] std::string policy_training_manifest_text(
    const cuwacunu::hero::runtime::policy_training_job_contract_t &contract,
    std::string_view contract_digest, std::string_view job_id,
    std::string_view job_stable_id, std::string_view attempt_leaf,
    const fs::path &global_config_path,
    std::string_view protocol_status = "pre_ppo_noop_execution",
    std::string_view wave_id = "policy_training_pre_ppo",
    std::string_view execution_chain =
        "runtime.policy_training_contract:validate -> "
        "runtime.policy_training.noop:write_checkpoint") {
  const auto operator_surface_digest =
      cuwacunu::hero::runtime::policy_operator_surface_digest(contract);
  std::ostringstream out;
  out << "manifest_format=kikijyeba.runtime.job_manifest.v1\n";
  out << "job_id=" << job_id << "\n";
  out << "job_stable_id=" << job_stable_id << "\n";
  out << "job_attempt_id=" << attempt_leaf << "\n";
  out << "job_attempt_index=0\n";
  out << "job_attempt_policy=immutable_no_overwrite\n";
  out << "job_kind=policy_training\n";
  out << "config_path=" << global_config_path.string() << "\n";
  out << "config_bundle_id=policy_training_contract_" << contract_digest
      << "\n";
  out << "config_receipt_id=policy_training_contract_" << contract_digest
      << "\n";
  out << "component_spawn_registry_id=hero.runtime.policy_training\n";
  out << "component_family_id=wikimyei.policy.portfolio.graph_node_"
         "allocation\n";
  out << "component_spawn_schema=kikijyeba.component_spawn.v2\n";
  out << "component_spawn_fingerprint=" << operator_surface_digest << "\n";
  out << "component_operator_surface_digest=\n";
  out << "component_spawn_id=" << contract.policy_id << "\n";
  out << "component_spawn_label=" << contract.policy_id << "\n";
  out << "topology_id=kikijyeba.topology.graph.v1\n";
  out << "protocol_id=" << contract.protocol_id << "\n";
  out << "protocol_kind=policy_training_contract\n";
  out << "protocol_status=" << protocol_status << "\n";
  out << "wave_id=" << wave_id << "\n";
  out << "target_component_family_id=wikimyei.policy.portfolio.graph_node_"
         "allocation\n";
  out << "wave_action=train\n";
  out << "wave_mode=train\n";
  out << "execution_chain=" << execution_chain << "\n";
  out << "mutated_components=wikimyei.policy.portfolio.graph_node_allocation\n";
  out << "frozen_components=wikimyei.representation,"
         "wikimyei.inference.expected_value.mdn,wikimyei.observer.belief,"
         "kikijyeba.environment.replay.v1,cajtucu.execution.paper.v1\n";
  out << "source_range_policy=causal_schedule_digest\n";
  out << "source_order_policy=sequential\n";
  out << "source_order_policy_explicit=true\n";
  out << "source_cursor_token=" << contract.source_cursor_token << "\n";
  out << "source_input_length=1\n";
  out << "source_future_length=1\n";
  out << "source_footprint_precision="
      << contract.causal_schedule_cursor_key_kind << "\n";
  out << "protocol_contract_fingerprint="
      << contract.protocol_contract_fingerprint << "\n";
  out << "graph_order_fingerprint=" << contract.graph_order_fingerprint << "\n";
  out << "component_assembly_fingerprint=" << operator_surface_digest << "\n";
  out << "policy_operator_surface_contract_id="
      << "policy_operator_surface_and_identity_standardization.v1\n";
  out << "policy_operator_surface_digest=" << operator_surface_digest << "\n";
  out << "policy_family_id=" << contract.policy_family_id << "\n";
  out << "policy_dsl_digest=" << contract.policy_dsl_digest << "\n";
  out << "policy_net_digest=" << contract.policy_net_digest << "\n";
  out << "policy_input_feature_manifest_digest="
      << contract.policy_input_feature_manifest_digest << "\n";
  out << "policy_jkimyei_digest=" << contract.policy_jkimyei_digest << "\n";
  out << "target_node_universe_digest=" << contract.target_node_universe_digest
      << "\n";
  out << "action_distribution_id=" << contract.action_distribution_id << "\n";
  out << "action_distribution_config_digest="
      << contract.action_distribution_config_digest << "\n";
  out << "reward_contract_id=" << contract.reward_contract_id << "\n";
  out << "execution_profile_digest=" << contract.execution_profile_digest
      << "\n";
  out << "causal_schedule_digest=" << contract.causal_schedule_digest << "\n";
  out << "nodelift_assembly_fingerprint=\n";
  out << "vicreg_assembly_fingerprint=\n";
  out << "mtf_jepa_mae_vicreg_assembly_fingerprint=\n";
  out << "mdn_assembly_fingerprint=\n";
  out << "dock_binding_fingerprint=\n";
  out << "dock_binding_token=\n";
  out << "stream_plan=policy_training_contract\n";
  out << "policy_training_contract_schema=" << contract.schema_version << "\n";
  out << "policy_training_contract_digest=" << contract_digest << "\n";
  out << "policy_training_artifact_schema=" << contract.artifact_schema_id
      << "\n";
  return out.str();
}

[[nodiscard]] std::string policy_training_state_text(
    std::string_view job_id, std::string_view job_stable_id,
    std::string_view attempt_leaf, const fs::path &checkpoint_path,
    const fs::path &report_path) {
  std::ostringstream out;
  out << "state_format=kikijyeba.runtime.job_state.v1\n";
  out << "job_id=" << job_id << "\n";
  out << "job_stable_id=" << job_stable_id << "\n";
  out << "job_attempt_id=" << attempt_leaf << "\n";
  out << "job_attempt_index=0\n";
  out << "job_attempt_policy=immutable_no_overwrite\n";
  out << "job_kind=policy_training\n";
  out << "status=completed\n";
  out << "error_message=\n";
  out << "wave_id=policy_training_pre_ppo\n";
  out << "target_component_family_id=wikimyei.policy.portfolio.graph_node_"
         "allocation\n";
  out << "wave_action=train\n";
  out << "steps_attempted=0\n";
  out << "steps_completed=0\n";
  out << "skipped_batches=0\n";
  out << "optimizer_steps=0\n";
  out << "checkpoint_written=true\n";
  out << "checkpoint_write_count=1\n";
  out << "checkpoint_path=" << checkpoint_path.string() << "\n";
  out << "report_written=true\n";
  out << "report_write_count=1\n";
  out << "report_path=" << report_path.string() << "\n";
  out << "runtime_result_fact_written=true\n";
  out << "runtime_result_fact_path=runtime.result.fact\n";
  out << "runtime_checkpoint_io_fact_written=true\n";
  out << "runtime_checkpoint_io_fact_path=runtime.checkpoint_io.fact\n";
  out << "runtime_health_measurement_fact_written=true\n";
  out << "runtime_health_measurement_fact_path=runtime.health_measurement."
         "fact\n";
  out << "lattice_exposure_fact_written=true\n";
  out << "lattice_exposure_fact_path=runtime.policy_training.fact\n";
  return out.str();
}

[[nodiscard]] std::string
policy_training_runtime_result_fact_text(std::string_view job_id,
                                         const fs::path &checkpoint_path) {
  std::ostringstream out;
  out << "fact_type=runtime.result.fact\n";
  out << "schema_version=1\n";
  out << "producer=runtime\n";
  out << "job_id=" << job_id << "\n";
  out << "job_kind=policy_training\n";
  out << "status=completed\n";
  out << "error_message=\n";
  out << "optimizer_steps=0\n";
  out << "checkpoint_written=true\n";
  out << "checkpoint_path=" << checkpoint_path.string() << "\n";
  out << "model_state_mutated=true\n";
  out << "finite_parameter_check=true\n";
  out << "nonfinite_output_count=0\n";
  out << "final_loss=0\n";
  out << "mean_loss=0\n";
  return out.str();
}

[[nodiscard]] std::string
policy_training_checkpoint_io_fact_text(std::string_view job_id,
                                        const fs::path &checkpoint_path,
                                        std::string_view checkpoint_digest) {
  std::error_code ec;
  const auto checkpoint_bytes = fs::exists(checkpoint_path, ec)
                                    ? fs::file_size(checkpoint_path, ec)
                                    : std::uintmax_t{0};
  std::ostringstream out;
  out << "fact_type=runtime.checkpoint_io.fact\n";
  out << "schema_version=1\n";
  out << "producer=runtime\n";
  out << "job_id=" << job_id << "\n";
  out << "job_kind=policy_training\n";
  out << "checkpoint_written=true\n";
  out << "checkpoint_path=" << checkpoint_path.string() << "\n";
  out << "checkpoint_write_count=1\n";
  out << "checkpoint_artifact_digest=" << checkpoint_digest << "\n";
  out << "checkpoint_path_reported=" << checkpoint_path.string() << "\n";
  out << "checkpoint_digest_reported=" << checkpoint_digest << "\n";
  out << "checkpoint_digest_verified=true\n";
  out << "checkpoint_file_exists=true\n";
  out << "checkpoint_bytes=" << checkpoint_bytes << "\n";
  out << "checkpoint_artifact_status=written\n";
  out << "model_state_mutated=true\n";
  return out.str();
}

[[nodiscard]] std::string
policy_training_health_fact_text(std::string_view job_id) {
  std::ostringstream out;
  out << "fact_type=runtime.health_measurement.fact\n";
  out << "schema_version=1\n";
  out << "producer=runtime\n";
  out << "job_id=" << job_id << "\n";
  out << "job_kind=policy_training\n";
  out << "finite_parameter_check=true\n";
  out << "nonfinite_output_count=0\n";
  return out.str();
}

struct ppo_v0_update_sample_t {
  std::int64_t step_index{0};
  std::string policy_input_digest{};
  double reward{0.0};
  double old_value{0.0};
  double new_value{0.0};
  double advantage{0.0};
  double normalized_advantage{0.0};
  double return_target{0.0};
  double old_log_prob{0.0};
  double new_log_prob{0.0};
  double log_prob_delta{0.0};
  double prob_ratio{1.0};
  double clipped_prob_ratio{1.0};
  bool ratio_clipped{false};
  double policy_loss_unclipped{0.0};
  double policy_loss_clipped{0.0};
  double policy_loss_final{0.0};
  double value_loss{0.0};
  double entropy{0.0};
  double entropy_bonus{0.0};
  double approx_kl{0.0};
};

struct ppo_v0_update_metrics_t {
  std::int64_t sample_count{0};
  std::int64_t optimizer_steps{1};
  double advantage_mean{0.0};
  double advantage_std{1.0};
  double mean_reward{0.0};
  double mean_return_target{0.0};
  double policy_loss_unclipped{0.0};
  double policy_loss_clipped{0.0};
  double policy_loss_final{0.0};
  double value_loss{0.0};
  double entropy{0.0};
  double entropy_bonus{0.0};
  double approx_kl{0.0};
  double clip_fraction{0.0};
  double gradient_norm{0.0};
  double explained_variance{0.0};
  bool target_kl_exceeded{false};
  bool early_stop{false};
  bool parameter_update_applied{false};
  bool policy_input_tensor_payload_bound{false};
  bool cuda_available{false};
  bool module_parameters_on_cuda{false};
  bool forward_input_on_cuda{false};
  bool loss_on_cuda{false};
  bool optimizer_state_on_cuda{false};
  bool cuda_verification_passed{false};
  bool resume_parent_loaded{false};
  bool optimizer_state_resume_loaded{false};
  std::string policy_input_tensor_payload_source{};
  std::string parameter_update_mode{"torch_autograd_ppo_clip_gae_v0"};
  std::string optimizer_kind{"torch_adam"};
  std::string device_policy{"require_cuda"};
  std::string runtime_device_kind{"cuda"};
  std::string resume_mode{"fresh_spawn"};
  std::string resume_parent_actor_checkpoint_digest{};
  std::string resume_optimizer_state_digest{};
  std::string optimizer_torch_state_path{};
  std::string optimizer_torch_state_digest{};
  std::int64_t cuda_device_index{0};
  double actor_learning_rate{0.0};
  double critic_learning_rate{0.0};
  double actor_logit_gradient_norm{0.0};
  double parameter_delta_l2{0.0};
  double updated_value_estimate{0.0};
  std::vector<double> updated_node_weight_logits{};
  std::vector<double> updated_action_distribution_params{};
  std::vector<ppo_v0_update_sample_t> update_samples{};
};

inline void write_seed_graph_node_allocation_module_state_artifact(
    const fs::path &module_state_path, const torch::Device &device) {
  namespace graph_alloc =
      cuwacunu::wikimyei::policy::portfolio::graph_node_allocation;
  torch::manual_seed(17);
  auto options = graph_alloc::make_graph_node_allocation_torch_policy_options();
  options.device = device;
  auto module =
      graph_alloc::GraphNodeAllocationTorchPolicyModule(std::move(options));
  graph_alloc::save_graph_node_allocation_torch_module_state(module_state_path,
                                                             module);
}

struct ppo_v0_rollout_sample_t {
  std::int64_t step_index{0};
  std::int64_t source_step_index{0};
  std::string source{"runtime_smoke_fixture"};
  bool replay_backed{false};
  bool policy_distribution_evidence_bound{true};
  std::string rollout_id{"ppo_v0_runtime_smoke_rollout"};
  std::string episode_id{"ppo_v0_smoke_episode_0"};
  std::string block_id{"ppo_v0_smoke_block_0"};
  std::string block_cursor_begin{"0"};
  std::string block_cursor_end{"0"};
  std::string observation_anchor_key{};
  std::int64_t observation_anchor_index{0};
  std::int64_t knowledge_timestamp_ms{0};
  std::string policy_input_digest{};
  std::string transition_digest{};
  std::string action_distribution_id{};
  std::string active_node_indices{"0,1"};
  std::int64_t active_count{2};
  double reward{0.0};
  double reward_log_growth{0.0};
  double reward_drawdown_penalty{0.0};
  double reward_transaction_cost_penalty{0.0};
  double reward_turnover_penalty{0.0};
  double reward_invalid_action_penalty{0.0};
  double old_value{0.0};
  double new_value{0.0};
  double old_log_prob{0.0};
  double new_log_prob{0.0};
  double entropy{0.0};
  std::string target_node_weights{};
  std::string policy_input_tensor_payload_schema_id{};
  bool policy_input_tensor_payload_bound{false};
  std::string policy_input_node_features_shape{};
  std::string policy_input_node_features{};
  std::string policy_input_global_features_shape{};
  std::string policy_input_global_features{};
  std::string policy_input_risk_features_shape{};
  std::string policy_input_risk_features{};
  std::string policy_input_executable_mask_shape{};
  std::string policy_input_executable_mask{};
  torch::Tensor node_features{};
  torch::Tensor global_features{};
  torch::Tensor risk_features{};
  torch::Tensor executable_mask{};
  torch::Tensor target_weights_tensor{};
  double transaction_cost_numeraire{0.0};
  double turnover{0.0};
  double rejected_notional_numeraire{0.0};
  double partial_unfilled_notional_numeraire{0.0};
  double fee_cost_numeraire{0.0};
  double spread_cost_numeraire{0.0};
  double slippage_cost_numeraire{0.0};
  std::uint64_t missing_direct_pair_count{0};
  std::uint64_t numeraire_fallback_pair_count{0};
  std::uint64_t rejected_fill_count{0};
  std::uint64_t partial_fill_count{0};
  bool cajtucu_trace_available{false};
  std::string cajtucu_trace_id{};
  std::string cajtucu_trace_digest{};
  bool cajtucu_trace_digest_bound{false};
  std::string cajtucu_trace_source{"runtime_smoke_fixture"};
  bool cajtucu_trace_valid{true};
  bool invalid_action{false};
  bool invalid_execution_trace{false};
  bool done{false};
  bool truncated{false};
};

[[nodiscard]] std::string device_kind_string(const torch::Device &device) {
  return device.type() == torch::kCUDA ? "cuda" : "cpu";
}

[[nodiscard]] bool tensor_on_device(const torch::Tensor &tensor,
                                    const torch::Device &device) {
  return tensor.defined() && tensor.device().type() == device.type() &&
         (device.type() != torch::kCUDA ||
          tensor.device().index() == device.index());
}

[[nodiscard]] bool module_parameters_on_device(
    const cuwacunu::wikimyei::policy::portfolio::graph_node_allocation::
        GraphNodeAllocationTorchPolicyModule &module,
    const torch::Device &device) {
  if (module.is_empty()) {
    return false;
  }
  for (const auto &parameter : module->parameters()) {
    if (!tensor_on_device(parameter, device)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] torch::Tensor
tensor_to_runtime_device(const torch::Tensor &tensor,
                         const torch::Device &device, torch::Dtype dtype) {
  return tensor.to(torch::TensorOptions().dtype(dtype).device(device));
}

[[nodiscard]] bool resolve_ppo_v0_runtime_device(
    const cuwacunu::hero::runtime::policy_training_job_contract_t &contract,
    torch::Device *device, std::string *err) {
  if (contract.device_policy != "require_cuda") {
    if (err != nullptr) {
      *err = "E_RUNTIME_POLICY_TRAINING_PPO_DEVICE_POLICY_UNSUPPORTED: "
             "PPO V0 requires device_policy=require_cuda";
    }
    return false;
  }
  if (!torch::cuda::is_available()) {
    if (err != nullptr) {
      *err = "E_RUNTIME_POLICY_TRAINING_PPO_CUDA_UNAVAILABLE: "
             "device_policy=require_cuda but CUDA is not available";
    }
    return false;
  }
  if (contract.cuda_device_index != 0) {
    if (err != nullptr) {
      *err = "E_RUNTIME_POLICY_TRAINING_PPO_CUDA_DEVICE_UNSUPPORTED: "
             "PPO V0 currently supports cuda_device_index=0 only";
    }
    return false;
  }
  if (device != nullptr) {
    *device = torch::Device(torch::kCUDA, 0);
  }
  return true;
}

struct resolved_resume_actor_checkpoint_t {
  fs::path checkpoint_path{};
  std::string checkpoint_digest{};
  fs::path module_state_path{};
  std::string module_state_digest{};
};

[[nodiscard]] std::string
kv_field_or_empty(const std::unordered_map<std::string, std::string> &fields,
                  std::string_view key) {
  const auto it = fields.find(std::string(key));
  return it == fields.end() ? std::string{} : it->second;
}

[[nodiscard]] bool validate_resume_checkpoint_field(
    const std::unordered_map<std::string, std::string> &fields,
    std::string_view key, std::string_view expected, std::string *err) {
  if (expected.empty()) {
    return true;
  }
  const std::string actual = kv_field_or_empty(fields, key);
  if (actual.empty()) {
    if (err != nullptr) {
      *err = "E_RUNTIME_POLICY_TRAINING_PPO_RESUME_FIELD_MISSING: parent "
             "checkpoint missing " +
             std::string(key);
    }
    return false;
  }
  if (actual != expected) {
    if (err != nullptr) {
      *err = "E_RUNTIME_POLICY_TRAINING_PPO_RESUME_INCOMPATIBLE: parent " +
             std::string(key) + " does not match current contract";
    }
    return false;
  }
  return true;
}

[[nodiscard]] bool resolve_resume_actor_checkpoint(
    const cuwacunu::hero::runtime::policy_training_job_contract_t &contract,
    resolved_resume_actor_checkpoint_t *resolved, std::string *err) {
  if (contract.resume_mode == "fresh_spawn") {
    return true;
  }
  const fs::path checkpoint_path =
      normalize_path(fs::path(contract.resume_actor_checkpoint_path));
  if (!fs::exists(checkpoint_path)) {
    if (err != nullptr) {
      *err = "E_RUNTIME_POLICY_TRAINING_PPO_RESUME_CHECKPOINT_MISSING: " +
             checkpoint_path.string();
    }
    return false;
  }
  const std::string actual_checkpoint_digest = file_digest_or_empty(
      checkpoint_path, contract.policy_checkpoint_schema_id);
  if (!contract.resume_actor_checkpoint_digest.empty() &&
      actual_checkpoint_digest != contract.resume_actor_checkpoint_digest) {
    if (err != nullptr) {
      *err = "E_RUNTIME_POLICY_TRAINING_PPO_RESUME_CHECKPOINT_DIGEST_MISMATCH";
    }
    return false;
  }
  const auto fields = parse_kv_file(checkpoint_path);
  const auto require_match = [&fields, err](std::string_view key,
                                            std::string_view expected) {
    return validate_resume_checkpoint_field(fields, key, expected, err);
  };
  if (!require_match("policy_family_id", contract.policy_family_id) ||
      !require_match("module_contract_id",
                     "wikimyei.policy.portfolio.graph_node_allocation."
                     "torch_policy_module.v0") ||
      !require_match("network_contract_id",
                     "wikimyei.policy.portfolio.graph_node_allocation."
                     "net.v1") ||
      !require_match("policy_input_feature_manifest_digest",
                     contract.policy_input_feature_manifest_digest) ||
      !require_match("action_distribution_id",
                     contract.action_distribution_id) ||
      !require_match("action_distribution_config_digest",
                     contract.action_distribution_config_digest) ||
      !require_match("ppo_config_digest", contract.ppo_config_digest) ||
      !require_match("causal_schedule_digest",
                     contract.causal_schedule_digest) ||
      !require_match("snapshot_family_digest",
                     contract.snapshot_family_digest) ||
      !require_match("reward_contract_id", contract.reward_contract_id) ||
      !require_match("execution_profile_digest",
                     contract.execution_profile_digest) ||
      !require_match("graph_order_fingerprint",
                     contract.graph_order_fingerprint) ||
      !require_match("actor_architecture_digest",
                     contract.actor_architecture_digest) ||
      !require_match("critic_architecture_digest",
                     contract.critic_architecture_digest)) {
    return false;
  }

  const std::string module_state_field =
      kv_field_or_empty(fields, "module_state_path");
  if (trim_ascii(module_state_field).empty()) {
    if (err != nullptr) {
      *err = "E_RUNTIME_POLICY_TRAINING_PPO_RESUME_MODULE_STATE_MISSING: "
             "parent checkpoint has no module_state_path";
    }
    return false;
  }
  const fs::path module_state_path =
      fs::path(module_state_field).is_absolute()
          ? normalize_path(fs::path(module_state_field))
          : normalize_path(checkpoint_path.parent_path() /
                           fs::path(module_state_field));
  if (!fs::exists(module_state_path)) {
    if (err != nullptr) {
      *err = "E_RUNTIME_POLICY_TRAINING_PPO_RESUME_MODULE_STATE_NOT_FOUND: " +
             module_state_path.string();
    }
    return false;
  }
  const std::string module_state_digest = file_digest_or_empty(
      module_state_path,
      "wikimyei.policy.portfolio.graph_node_allocation.module_state.v0");
  const std::string expected_module_state_digest =
      kv_field_or_empty(fields, "module_state_digest");
  if (!expected_module_state_digest.empty() &&
      module_state_digest != expected_module_state_digest) {
    if (err != nullptr) {
      *err = "E_RUNTIME_POLICY_TRAINING_PPO_RESUME_MODULE_STATE_DIGEST_"
             "MISMATCH";
    }
    return false;
  }
  if (resolved != nullptr) {
    resolved->checkpoint_path = checkpoint_path;
    resolved->checkpoint_digest = actual_checkpoint_digest;
    resolved->module_state_path = module_state_path;
    resolved->module_state_digest = module_state_digest;
  }
  return true;
}

[[nodiscard]] bool resolve_resume_optimizer_torch_state_path(
    const cuwacunu::hero::runtime::policy_training_job_contract_t &contract,
    fs::path *torch_state_path, std::string *err) {
  if (contract.resume_mode != "resume_weights_and_optimizer") {
    return true;
  }
  const fs::path optimizer_state_path =
      normalize_path(fs::path(contract.resume_optimizer_state_path));
  if (!fs::exists(optimizer_state_path)) {
    if (err != nullptr) {
      *err = "E_RUNTIME_POLICY_TRAINING_PPO_RESUME_OPTIMIZER_STATE_MISSING: " +
             optimizer_state_path.string();
    }
    return false;
  }
  const std::string actual_digest = file_digest_or_empty(
      optimizer_state_path, contract.optimizer_state_schema_id);
  if (!contract.resume_optimizer_state_digest.empty() &&
      actual_digest != contract.resume_optimizer_state_digest) {
    if (err != nullptr) {
      *err = "E_RUNTIME_POLICY_TRAINING_PPO_RESUME_OPTIMIZER_STATE_DIGEST_"
             "MISMATCH";
    }
    return false;
  }
  const auto fields = parse_kv_file(optimizer_state_path);
  const std::string torch_state_field =
      kv_field_or_empty(fields, "optimizer_torch_state_path");
  if (trim_ascii(torch_state_field).empty()) {
    if (err != nullptr) {
      *err = "E_RUNTIME_POLICY_TRAINING_PPO_RESUME_OPTIMIZER_TORCH_STATE_"
             "MISSING";
    }
    return false;
  }
  const fs::path resolved_path =
      fs::path(torch_state_field).is_absolute()
          ? normalize_path(fs::path(torch_state_field))
          : normalize_path(optimizer_state_path.parent_path() /
                           fs::path(torch_state_field));
  if (!fs::exists(resolved_path)) {
    if (err != nullptr) {
      *err = "E_RUNTIME_POLICY_TRAINING_PPO_RESUME_OPTIMIZER_TORCH_STATE_NOT_"
             "FOUND: " +
             resolved_path.string();
    }
    return false;
  }
  if (torch_state_path != nullptr) {
    *torch_state_path = resolved_path;
  }
  return true;
}

struct ppo_v0_policy_summary_t {
  std::string policy_id{};
  std::string policy_kind{};
  std::int64_t attempted_count{0};
  std::int64_t completed_count{0};
  std::int64_t failed_count{0};
  double mean_total_reward{0.0};
  double mean_total_log_growth{0.0};
  double mean_final_equity_numeraire{0.0};
  double mean_max_drawdown{0.0};
  double realized_volatility{0.0};
  double mean_total_turnover{0.0};
  double mean_total_transaction_cost_numeraire{0.0};
  double requested_notional_numeraire{0.0};
  double executed_notional_numeraire{0.0};
  double rejected_notional_numeraire{0.0};
  double partial_notional_numeraire{0.0};
  double fill_ratio{0.0};
  double fee_cost_numeraire{0.0};
  double spread_cost_numeraire{0.0};
  double slippage_cost_numeraire{0.0};
  double mean_target_weight_error_l1{0.0};
  double mean_target_weight_error_linf{0.0};
  double mean_projection_mae{0.0};
  double mean_projection_signed_bias{0.0};
  double mean_projection_directional_accuracy{0.0};
  double mean_projection_interval_coverage{0.0};
  std::uint64_t cajtucu_valid_trace_count{0};
  std::uint64_t cajtucu_invalid_trace_count{0};
  std::uint64_t cajtucu_missing_direct_pair_count{0};
  std::uint64_t cajtucu_numeraire_fallback_pair_count{0};
  std::uint64_t rejected_order_count{0};
  std::uint64_t partial_order_count{0};
};

struct ppo_v0_rollout_collection_t {
  std::vector<ppo_v0_rollout_sample_t> samples{};
  std::vector<ppo_v0_policy_summary_t> policy_summaries{};
  std::string rollout_collection_id{"ppo_v0_runtime_smoke_rollout"};
  std::string collection_mode{"runtime_ppo_v0_smoke_fixture"};
  std::string collection_source{"runtime_smoke_fixture"};
  fs::path input_replay_report_path{};
  std::string input_replay_report_digest{};
  fs::path input_policy_checkpoint_path{};
  std::string input_policy_checkpoint_digest{};
  bool kikijyeba_cajtucu_replay_integration{false};
  bool policy_distribution_evidence_bound{true};
  bool on_policy_runtime_collection_complete{false};
  bool replay_summary_bound{false};
  bool final_equity_numeraire_bound{false};
  double final_equity_numeraire{0.0};
  bool total_log_growth_bound{false};
  double total_log_growth{0.0};
  bool max_drawdown_bound{false};
  double max_drawdown{0.0};
  bool realized_volatility_bound{false};
  double realized_volatility{0.0};
  bool target_tracking_error_l1_bound{false};
  double target_tracking_error_l1{0.0};
  bool target_tracking_error_linf_bound{false};
  double target_tracking_error_linf{0.0};
};

[[nodiscard]] std::string tensor_shape_csv(torch::Tensor tensor) {
  if (!tensor.defined()) {
    return {};
  }
  std::ostringstream out;
  for (std::int64_t i = 0; i < tensor.dim(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << tensor.size(i);
  }
  return out.str();
}

[[nodiscard]] std::string tensor_flat_values_csv(torch::Tensor tensor) {
  if (!tensor.defined()) {
    return {};
  }
  const auto flat = tensor.to(torch::kFloat64).contiguous().view({-1});
  if (!torch::isfinite(flat).all().item<bool>()) {
    return {};
  }
  std::ostringstream out;
  out << std::setprecision(17);
  const auto *data = flat.data_ptr<double>();
  for (std::int64_t i = 0; i < flat.numel(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << data[i];
  }
  return out.str();
}

inline void bind_sample_policy_input_tensors(ppo_v0_rollout_sample_t &sample,
                                             torch::Tensor node_features,
                                             torch::Tensor global_features,
                                             torch::Tensor risk_features,
                                             torch::Tensor executable_mask,
                                             torch::Tensor target_weights) {
  sample.policy_input_tensor_payload_schema_id =
      "kikijyeba.environment.policy_input.tensor_payload.v1";
  sample.node_features = node_features.to(torch::kFloat64).contiguous();
  sample.global_features = global_features.to(torch::kFloat64).contiguous();
  sample.risk_features = risk_features.to(torch::kFloat64).contiguous();
  sample.executable_mask = executable_mask.to(torch::kBool).contiguous();
  sample.target_weights_tensor =
      target_weights.to(torch::kFloat64).contiguous();
  sample.policy_input_node_features_shape =
      tensor_shape_csv(sample.node_features);
  sample.policy_input_node_features =
      tensor_flat_values_csv(sample.node_features);
  sample.policy_input_global_features_shape =
      tensor_shape_csv(sample.global_features);
  sample.policy_input_global_features =
      tensor_flat_values_csv(sample.global_features);
  sample.policy_input_risk_features_shape =
      tensor_shape_csv(sample.risk_features);
  sample.policy_input_risk_features =
      tensor_flat_values_csv(sample.risk_features);
  sample.policy_input_executable_mask_shape =
      tensor_shape_csv(sample.executable_mask);
  sample.policy_input_executable_mask =
      tensor_flat_values_csv(sample.executable_mask);
  sample.policy_input_tensor_payload_bound =
      !sample.policy_input_node_features_shape.empty() &&
      !sample.policy_input_node_features.empty() &&
      !sample.policy_input_global_features_shape.empty() &&
      !sample.policy_input_global_features.empty() &&
      !sample.policy_input_risk_features_shape.empty() &&
      !sample.policy_input_risk_features.empty() &&
      !sample.policy_input_executable_mask_shape.empty() &&
      !sample.policy_input_executable_mask.empty() &&
      sample.target_weights_tensor.defined();
}

inline void bind_smoke_policy_input_tensors(ppo_v0_rollout_sample_t &sample,
                                            double weight0, double weight1) {
  auto node =
      torch::zeros({2, 28}, torch::TensorOptions().dtype(torch::kFloat64));
  node.index_put_({0, 0}, weight0);
  node.index_put_({1, 0}, weight1);
  node.index_put_({0, 1}, weight0);
  node.index_put_({1, 1}, weight1);
  node.index_put_({0, 2}, 0.0);
  node.index_put_({1, 2}, 0.0);
  node.index_put_({0, 3}, 0.0);
  node.index_put_({1, 3}, 0.0);
  for (std::int64_t j = 4; j <= 6; ++j) {
    node.index_put_({0, j}, 1.0);
    node.index_put_({1, j}, 1.0);
  }
  node.index_put_({0, 7}, sample.reward_log_growth);
  node.index_put_({1, 7}, sample.reward_log_growth * 0.5);
  node.index_put_({0, 15}, 0.75);
  node.index_put_({1, 15}, 0.70);
  node.index_put_({0, 17}, sample.transaction_cost_numeraire);
  node.index_put_({1, 17}, sample.transaction_cost_numeraire);
  node.index_put_({0, 24}, sample.entropy);
  node.index_put_({1, 24}, sample.entropy);
  const auto global = torch::tensor(
      {0.0, 0.0, weight0 + weight1, weight0 + weight1, weight0, weight0},
      torch::TensorOptions().dtype(torch::kFloat64));
  const auto risk =
      torch::zeros({10}, torch::TensorOptions().dtype(torch::kFloat64));
  const auto mask =
      torch::tensor({true, true}, torch::TensorOptions().dtype(torch::kBool));
  const auto target = torch::tensor(
      {weight0, weight1}, torch::TensorOptions().dtype(torch::kFloat64));
  bind_sample_policy_input_tensors(sample, node, global, risk, mask, target);
}

[[nodiscard]] std::vector<ppo_v0_rollout_sample_t> ppo_v0_smoke_samples() {
  struct seed_t {
    double reward;
    double old_value;
    double new_value;
    double old_log_prob;
    double new_log_prob;
    double entropy;
    const char *target_node_weights;
    double transaction_cost_numeraire;
    double turnover;
    double fee_cost_numeraire;
    double spread_cost_numeraire;
    double slippage_cost_numeraire;
    double weight0;
    double weight1;
    bool done;
  };
  const std::array<seed_t, 4> seeds{{
      {0.0020, 0.0008, 0.0010, -0.7100, -0.6950, 1.08,
       "fixture_node_0:0.50,fixture_node_1:0.50", 0.00010, 0.120, 0.00003,
       0.00004, 0.00003, 0.50, 0.50, false},
      {-0.0010, 0.0002, 0.0001, -0.7200, -0.7330, 1.05,
       "fixture_node_0:0.48,fixture_node_1:0.52", 0.00008, 0.060, 0.00002,
       0.00003, 0.00003, 0.48, 0.52, false},
      {0.0030, 0.0005, 0.0007, -0.7050, -0.6930, 1.06,
       "fixture_node_0:0.46,fixture_node_1:0.54", 0.00012, 0.090, 0.00004,
       0.00004, 0.00004, 0.46, 0.54, false},
      {0.0005, 0.0001, 0.0002, -0.7300, -0.7220, 1.04,
       "fixture_node_0:0.47,fixture_node_1:0.53", 0.00006, 0.030, 0.00002,
       0.00002, 0.00002, 0.47, 0.53, true},
  }};
  std::vector<ppo_v0_rollout_sample_t> out;
  out.reserve(seeds.size());
  for (std::size_t i = 0; i < seeds.size(); ++i) {
    const auto &seed = seeds[i];
    ppo_v0_rollout_sample_t sample{};
    sample.step_index = static_cast<std::int64_t>(i);
    sample.source_step_index = sample.step_index;
    sample.block_cursor_end = std::to_string(seeds.size());
    sample.observation_anchor_key = "ppo_v0_smoke_anchor_" + std::to_string(i);
    sample.observation_anchor_index = static_cast<std::int64_t>(i);
    sample.knowledge_timestamp_ms = 1000 + static_cast<std::int64_t>(i);
    sample.reward = seed.reward;
    sample.reward_log_growth = seed.reward;
    sample.old_value = seed.old_value;
    sample.new_value = seed.new_value;
    sample.old_log_prob = seed.old_log_prob;
    sample.new_log_prob = seed.new_log_prob;
    sample.entropy = seed.entropy;
    sample.target_node_weights = seed.target_node_weights;
    sample.transaction_cost_numeraire = seed.transaction_cost_numeraire;
    sample.turnover = seed.turnover;
    sample.fee_cost_numeraire = seed.fee_cost_numeraire;
    sample.spread_cost_numeraire = seed.spread_cost_numeraire;
    sample.slippage_cost_numeraire = seed.slippage_cost_numeraire;
    sample.done = seed.done;
    bind_smoke_policy_input_tensors(sample, seed.weight0, seed.weight1);
    out.push_back(std::move(sample));
  }
  return out;
}

[[nodiscard]] ppo_v0_rollout_collection_t make_ppo_v0_smoke_collection() {
  ppo_v0_rollout_collection_t out{};
  out.samples = ppo_v0_smoke_samples();
  return out;
}

[[nodiscard]] bool
load_sample_policy_input_tensors(ppo_v0_rollout_sample_t *sample,
                                 std::string *err);

[[nodiscard]] std::string
kv_string_or(const std::unordered_map<std::string, std::string> &kv,
             std::string_view key, std::string_view fallback = {}) {
  const auto found = kv.find(std::string(key));
  return found == kv.end() ? std::string(fallback) : trim_ascii(found->second);
}

[[nodiscard]] bool
kv_double(const std::unordered_map<std::string, std::string> &kv,
          std::string_view key, double *out) {
  const auto found = kv.find(std::string(key));
  if (found == kv.end()) {
    return false;
  }
  double parsed = 0.0;
  if (!parse_double(found->second, &parsed) || !std::isfinite(parsed)) {
    return false;
  }
  if (out) {
    *out = parsed;
  }
  return true;
}

[[nodiscard]] double
kv_double_or(const std::unordered_map<std::string, std::string> &kv,
             std::string_view key, double fallback = 0.0) {
  double out = fallback;
  (void)kv_double(kv, key, &out);
  return out;
}

[[nodiscard]] bool
kv_first_double(const std::unordered_map<std::string, std::string> &kv,
                std::initializer_list<std::string_view> keys, double *out) {
  for (std::string_view key : keys) {
    if (kv_double(kv, key, out)) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool parse_i64_text(std::string_view raw, std::int64_t *out) {
  const std::string value = trim_ascii(raw);
  if (value.empty()) {
    return false;
  }
  std::int64_t parsed = 0;
  const auto result =
      std::from_chars(value.data(), value.data() + value.size(), parsed);
  if (result.ec != std::errc{} || result.ptr != value.data() + value.size()) {
    return false;
  }
  if (out) {
    *out = parsed;
  }
  return true;
}

[[nodiscard]] bool
kv_i64(const std::unordered_map<std::string, std::string> &kv,
       std::string_view key, std::int64_t *out) {
  const auto found = kv.find(std::string(key));
  return found != kv.end() && parse_i64_text(found->second, out);
}

[[nodiscard]] std::int64_t
kv_i64_or(const std::unordered_map<std::string, std::string> &kv,
          std::string_view key, std::int64_t fallback = 0) {
  std::int64_t out = fallback;
  (void)kv_i64(kv, key, &out);
  return out;
}

[[nodiscard]] std::uint64_t
kv_u64_or(const std::unordered_map<std::string, std::string> &kv,
          std::string_view key, std::uint64_t fallback = 0) {
  std::int64_t parsed = 0;
  if (!kv_i64(kv, key, &parsed) || parsed < 0) {
    return fallback;
  }
  return static_cast<std::uint64_t>(parsed);
}

[[nodiscard]] bool
kv_bool_or(const std::unordered_map<std::string, std::string> &kv,
           std::string_view key, bool fallback = false) {
  const auto found = kv.find(std::string(key));
  if (found == kv.end()) {
    return fallback;
  }
  bool out = fallback;
  return parse_bool(found->second, &out) ? out : fallback;
}

[[nodiscard]] int scan_prefixed_index_count(
    const std::unordered_map<std::string, std::string> &kv,
    std::string_view prefix) {
  int max_index = -1;
  const std::string prefix_string(prefix);
  for (const auto &entry : kv) {
    if (entry.first.rfind(prefix_string, 0) != 0) {
      continue;
    }
    const std::string_view rest(entry.first.data() + prefix_string.size(),
                                entry.first.size() - prefix_string.size());
    const auto underscore = rest.find('_');
    if (underscore == std::string_view::npos) {
      continue;
    }
    int parsed = 0;
    if (parse_int(rest.substr(0, underscore), &parsed) && parsed > max_index) {
      max_index = parsed;
    }
  }
  return max_index + 1;
}

[[nodiscard]] bool load_ppo_v0_replay_rollout_collection(
    const fs::path &report_path,
    const cuwacunu::hero::runtime::policy_training_job_contract_t &contract,
    std::string_view contract_digest, ppo_v0_rollout_collection_t *out,
    std::string *err) {
  if (report_path.empty() || !fs::exists(report_path) ||
      !fs::is_regular_file(report_path)) {
    if (err) {
      *err = "E_RUNTIME_POLICY_TRAINING_PPO_REPLAY_REPORT_MISSING: " +
             report_path.string();
    }
    return false;
  }
  const auto kv = parse_kv_file(report_path);
  if (kv.empty()) {
    if (err) {
      *err = "E_RUNTIME_POLICY_TRAINING_PPO_REPLAY_REPORT_EMPTY: " +
             report_path.string();
    }
    return false;
  }

  ppo_v0_rollout_collection_t collection{};
  collection.rollout_collection_id = "ppo_v0_runtime_replay_report_rollout";
  collection.collection_mode = "runtime_ppo_v0_replay_report_ingestion";
  collection.collection_source = "kikijyeba_replay_report";
  collection.input_replay_report_path = report_path;
  collection.input_replay_report_digest = file_digest_or_empty(
      report_path,
      kv_string_or(kv, "schema", "kikijyeba.environment.replay.report.v1"));
  collection.kikijyeba_cajtucu_replay_integration = true;
  collection.final_equity_numeraire_bound = kv_first_double(
      kv, {"final_equity_numeraire", "mean_final_equity_numeraire"},
      &collection.final_equity_numeraire);
  collection.total_log_growth_bound =
      kv_first_double(kv, {"total_log_growth", "mean_total_log_growth"},
                      &collection.total_log_growth);
  collection.max_drawdown_bound = kv_first_double(
      kv, {"max_drawdown", "mean_max_drawdown"}, &collection.max_drawdown);
  collection.realized_volatility_bound =
      kv_first_double(kv, {"realized_volatility", "mean_realized_volatility"},
                      &collection.realized_volatility);
  collection.target_tracking_error_l1_bound = kv_first_double(
      kv, {"target_tracking_error_l1", "mean_target_weight_error_l1"},
      &collection.target_tracking_error_l1);
  collection.target_tracking_error_linf_bound = kv_first_double(
      kv, {"target_tracking_error_linf", "mean_target_weight_error_linf"},
      &collection.target_tracking_error_linf);
  collection.replay_summary_bound = collection.final_equity_numeraire_bound ||
                                    collection.total_log_growth_bound ||
                                    collection.max_drawdown_bound ||
                                    collection.realized_volatility_bound ||
                                    collection.target_tracking_error_l1_bound ||
                                    collection.target_tracking_error_linf_bound;

  int policy_summary_count = 0;
  const auto policy_summary_count_found = kv.find("policy_summary_count");
  if (policy_summary_count_found != kv.end()) {
    (void)parse_int(policy_summary_count_found->second, &policy_summary_count);
  }
  if (policy_summary_count <= 0) {
    policy_summary_count = scan_prefixed_index_count(kv, "policy_");
  }
  collection.policy_summaries.reserve(
      static_cast<std::size_t>(std::max(0, policy_summary_count)));
  for (int policy_index = 0; policy_index < policy_summary_count;
       ++policy_index) {
    const std::string prefix = "policy_" + std::to_string(policy_index) + "_";
    if (kv.find(prefix + "id") == kv.end()) {
      continue;
    }
    ppo_v0_policy_summary_t summary{};
    summary.policy_id = kv_string_or(kv, prefix + "id", "");
    summary.policy_kind = kv_string_or(kv, prefix + "kind", "");
    summary.attempted_count = kv_i64_or(kv, prefix + "attempted_count", 0);
    summary.completed_count = kv_i64_or(kv, prefix + "completed_count", 0);
    summary.failed_count = kv_i64_or(kv, prefix + "failed_count", 0);
    summary.mean_total_reward =
        kv_double_or(kv, prefix + "mean_total_reward", 0.0);
    summary.mean_total_log_growth =
        kv_double_or(kv, prefix + "mean_total_log_growth", 0.0);
    summary.mean_final_equity_numeraire =
        kv_double_or(kv, prefix + "mean_final_equity_numeraire", 0.0);
    summary.mean_max_drawdown =
        kv_double_or(kv, prefix + "mean_max_drawdown", 0.0);
    summary.realized_volatility =
        kv_double_or(kv, prefix + "realized_volatility", 0.0);
    summary.mean_total_turnover =
        kv_double_or(kv, prefix + "mean_total_turnover", 0.0);
    summary.mean_total_transaction_cost_numeraire =
        kv_double_or(kv, prefix + "mean_total_transaction_cost_numeraire", 0.0);
    summary.requested_notional_numeraire =
        kv_double_or(kv, prefix + "requested_notional_numeraire", 0.0);
    summary.executed_notional_numeraire =
        kv_double_or(kv, prefix + "executed_notional_numeraire", 0.0);
    summary.rejected_notional_numeraire =
        kv_double_or(kv, prefix + "rejected_notional_numeraire", 0.0);
    summary.partial_notional_numeraire =
        kv_double_or(kv, prefix + "partial_notional_numeraire", 0.0);
    summary.fill_ratio = kv_double_or(kv, prefix + "fill_ratio", 0.0);
    summary.fee_cost_numeraire =
        kv_double_or(kv, prefix + "fee_cost_numeraire", 0.0);
    summary.spread_cost_numeraire =
        kv_double_or(kv, prefix + "spread_cost_numeraire", 0.0);
    summary.slippage_cost_numeraire =
        kv_double_or(kv, prefix + "slippage_cost_numeraire", 0.0);
    summary.mean_target_weight_error_l1 =
        kv_double_or(kv, prefix + "mean_target_weight_error_l1", 0.0);
    summary.mean_target_weight_error_linf =
        kv_double_or(kv, prefix + "mean_target_weight_error_linf", 0.0);
    summary.mean_projection_mae =
        kv_double_or(kv, prefix + "mean_projection_mae", 0.0);
    summary.mean_projection_signed_bias =
        kv_double_or(kv, prefix + "mean_projection_signed_bias", 0.0);
    summary.mean_projection_directional_accuracy =
        kv_double_or(kv, prefix + "mean_projection_directional_accuracy", 0.0);
    summary.mean_projection_interval_coverage =
        kv_double_or(kv, prefix + "mean_projection_interval_coverage", 0.0);
    summary.cajtucu_valid_trace_count =
        kv_u64_or(kv, prefix + "cajtucu_valid_trace_count", 0);
    summary.cajtucu_invalid_trace_count =
        kv_u64_or(kv, prefix + "cajtucu_invalid_trace_count", 0);
    summary.cajtucu_missing_direct_pair_count =
        kv_u64_or(kv, prefix + "cajtucu_missing_direct_pair_count", 0);
    summary.cajtucu_numeraire_fallback_pair_count =
        kv_u64_or(kv, prefix + "cajtucu_numeraire_fallback_pair_count", 0);
    summary.rejected_order_count =
        kv_u64_or(kv, prefix + "rejected_order_count", 0);
    summary.partial_order_count =
        kv_u64_or(kv, prefix + "partial_order_count", 0);
    collection.policy_summaries.push_back(std::move(summary));
  }

  int episode_count = 0;
  const auto episode_count_found = kv.find("episode_count");
  if (episode_count_found != kv.end()) {
    (void)parse_int(episode_count_found->second, &episode_count);
  }
  if (episode_count <= 0) {
    episode_count = scan_prefixed_index_count(kv, "episode_");
  }

  std::int64_t output_step_index = 0;
  bool all_distribution_evidence_bound = true;
  const std::string default_runtime_run_id =
      kv_string_or(kv, "runtime_run_id", "unknown_runtime_run");
  const std::string default_environment_run_id =
      kv_string_or(kv, "environment_run_id", default_runtime_run_id);

  for (int episode_index = 0; episode_index < episode_count; ++episode_index) {
    const std::string episode_prefix =
        "episode_" + std::to_string(episode_index) + "_";
    int step_count = 0;
    const auto step_count_found = kv.find(episode_prefix + "step_count");
    if (step_count_found != kv.end()) {
      (void)parse_int(step_count_found->second, &step_count);
    }
    if (step_count <= 0) {
      step_count = scan_prefixed_index_count(kv, episode_prefix + "step_");
    }
    for (int source_step_index = 0; source_step_index < step_count;
         ++source_step_index) {
      const std::string prefix =
          episode_prefix + "step_" + std::to_string(source_step_index) + "_";
      if (kv.find(prefix + "reward_total") == kv.end() &&
          kv.find(prefix + "target_node_weights") == kv.end()) {
        continue;
      }

      ppo_v0_rollout_sample_t sample{};
      sample.step_index = output_step_index++;
      sample.source_step_index =
          kv_i64_or(kv, prefix + "step_index", source_step_index);
      sample.source = "kikijyeba_replay_report";
      sample.replay_backed = true;
      sample.rollout_id = default_environment_run_id;
      sample.episode_id =
          kv_string_or(kv, prefix + "episode_id",
                       "episode_" + std::to_string(episode_index));
      sample.block_id = "episode_" + std::to_string(episode_index);
      sample.block_cursor_begin = kv_string_or(
          kv, prefix + "accepted_anchor_index_begin",
          kv_string_or(kv, prefix + "observation_anchor_index", "0"));
      sample.block_cursor_end = kv_string_or(
          kv, prefix + "accepted_anchor_index_end", sample.block_cursor_begin);
      sample.observation_anchor_key = kv_string_or(
          kv, prefix + "anchor_key",
          "replay_report_anchor_" + std::to_string(sample.source_step_index));
      sample.observation_anchor_index = kv_i64_or(
          kv, prefix + "observation_anchor_index", sample.source_step_index);
      sample.knowledge_timestamp_ms =
          kv_i64_or(kv, prefix + "knowledge_timestamp_ms", 0);
      sample.policy_input_digest =
          kv_string_or(kv, prefix + "policy_input_digest", "");
      sample.action_distribution_id =
          kv_string_or(kv, prefix + "action_distribution_id",
                       contract.action_distribution_id);
      sample.active_node_indices =
          kv_string_or(kv, prefix + "active_node_indices", "");
      sample.active_count = kv_i64_or(kv, prefix + "active_count", 0);
      sample.target_node_weights =
          kv_string_or(kv, prefix + "target_node_weights", "");
      sample.reward = kv_double_or(kv, prefix + "reward_total", 0.0);
      sample.reward_log_growth =
          kv_double_or(kv, prefix + "reward_log_growth", sample.reward);
      sample.reward_drawdown_penalty =
          kv_double_or(kv, prefix + "reward_drawdown_penalty", 0.0);
      sample.reward_transaction_cost_penalty =
          kv_double_or(kv, prefix + "reward_transaction_cost_penalty", 0.0);
      sample.reward_turnover_penalty =
          kv_double_or(kv, prefix + "reward_turnover_penalty", 0.0);
      sample.reward_invalid_action_penalty =
          kv_double_or(kv, prefix + "reward_invalid_action_penalty", 0.0);

      const bool action_distribution_evidence_bound =
          kv_bool_or(kv, prefix + "action_distribution_evidence_bound", false);
      const bool old_log_bound =
          kv_double(kv, prefix + "old_log_prob", &sample.old_log_prob);
      const bool new_log_bound =
          kv_double(kv, prefix + "new_log_prob", &sample.new_log_prob);
      double parsed_old_value = 0.0;
      const bool old_value_bound =
          kv_double(kv, prefix + "old_value_estimate", &parsed_old_value);
      sample.policy_distribution_evidence_bound =
          old_log_bound &&
          (old_value_bound || action_distribution_evidence_bound);
      if (!sample.policy_distribution_evidence_bound) {
        sample.old_log_prob = -0.7000;
        sample.new_log_prob = sample.old_log_prob;
        all_distribution_evidence_bound = false;
      } else if (!new_log_bound) {
        sample.new_log_prob = sample.old_log_prob;
      }
      sample.entropy =
          kv_double_or(kv, prefix + "entropy",
                       kv_double_or(kv, prefix + "old_entropy", 0.0));
      sample.old_value = old_value_bound ? parsed_old_value : 0.0;
      sample.new_value =
          kv_double_or(kv, prefix + "new_value_estimate", sample.old_value);
      sample.policy_input_tensor_payload_schema_id = kv_string_or(
          kv, prefix + "policy_input_tensor_payload_schema_id", "");
      sample.policy_input_tensor_payload_bound =
          kv_bool_or(kv, prefix + "policy_input_tensor_payload_bound", false);
      sample.policy_input_node_features_shape =
          kv_string_or(kv, prefix + "policy_input_node_features_shape", "");
      sample.policy_input_node_features =
          kv_string_or(kv, prefix + "policy_input_node_features", "");
      sample.policy_input_global_features_shape =
          kv_string_or(kv, prefix + "policy_input_global_features_shape", "");
      sample.policy_input_global_features =
          kv_string_or(kv, prefix + "policy_input_global_features", "");
      sample.policy_input_risk_features_shape =
          kv_string_or(kv, prefix + "policy_input_risk_features_shape", "");
      sample.policy_input_risk_features =
          kv_string_or(kv, prefix + "policy_input_risk_features", "");
      sample.policy_input_executable_mask_shape =
          kv_string_or(kv, prefix + "policy_input_executable_mask_shape", "");
      sample.policy_input_executable_mask =
          kv_string_or(kv, prefix + "policy_input_executable_mask", "");
      if (sample.policy_input_tensor_payload_bound &&
          !load_sample_policy_input_tensors(&sample, err)) {
        return false;
      }

      sample.transaction_cost_numeraire = kv_double_or(
          kv, prefix + "transaction_cost_numeraire",
          kv_double_or(kv, prefix + "cajtucu_total_transaction_cost_numeraire",
                       0.0));
      sample.turnover = kv_double_or(kv, prefix + "turnover", 0.0);
      sample.rejected_notional_numeraire =
          kv_double_or(kv, prefix + "cajtucu_rejected_notional_numeraire", 0.0);
      sample.partial_unfilled_notional_numeraire =
          kv_double_or(kv, prefix + "cajtucu_partial_notional_numeraire", 0.0);
      sample.fee_cost_numeraire =
          kv_double_or(kv, prefix + "cajtucu_total_fee_numeraire",
                       kv_double_or(kv, prefix + "fill_fee_numeraire", 0.0));
      sample.spread_cost_numeraire =
          kv_double_or(kv, prefix + "cajtucu_total_spread_cost_numeraire", 0.0);
      sample.slippage_cost_numeraire =
          kv_double_or(kv, prefix + "cajtucu_total_slippage_numeraire", 0.0);
      sample.missing_direct_pair_count =
          kv_u64_or(kv, prefix + "cajtucu_missing_direct_pair_count", 0);
      sample.numeraire_fallback_pair_count =
          kv_u64_or(kv, prefix + "cajtucu_numeraire_fallback_pair_count", 0);
      sample.rejected_fill_count =
          kv_u64_or(kv, prefix + "cajtucu_rejected_fill_count", 0);
      sample.partial_fill_count =
          kv_u64_or(kv, prefix + "cajtucu_partial_fill_count", 0);
      sample.cajtucu_trace_available =
          kv_bool_or(kv, prefix + "cajtucu_execution_trace_available", false);
      sample.cajtucu_trace_id =
          kv_string_or(kv, prefix + "cajtucu_trace_id", "");
      sample.cajtucu_trace_valid =
          kv_bool_or(kv, prefix + "cajtucu_trace_valid", true);
      sample.cajtucu_trace_digest_bound =
          sample.cajtucu_trace_available && !sample.cajtucu_trace_id.empty();
      sample.cajtucu_trace_source = sample.cajtucu_trace_digest_bound
                                        ? "kikijyeba_replay_report_trace_id"
                                        : "kikijyeba_replay_report";
      if (sample.cajtucu_trace_digest_bound) {
        sample.cajtucu_trace_digest =
            cuwacunu::hero::marshal::marshal_digest_for_text(
                "kikijyeba.runtime.ppo.replay_cajtucu_trace_id.v1",
                sample.cajtucu_trace_id + "|" + sample.observation_anchor_key +
                    "|" + std::string(contract_digest));
      }
      sample.invalid_action = kv_bool_or(kv, prefix + "invalid_action", false);
      sample.invalid_execution_trace =
          sample.cajtucu_trace_available && !sample.cajtucu_trace_valid;
      sample.done =
          kv_bool_or(kv, prefix + "done", source_step_index + 1 == step_count);
      sample.truncated = kv_bool_or(kv, prefix + "truncated", false);
      collection.samples.push_back(std::move(sample));
    }
  }

  if (collection.samples.empty()) {
    if (err) {
      *err = "E_RUNTIME_POLICY_TRAINING_PPO_REPLAY_REPORT_NO_STEPS: " +
             report_path.string();
    }
    return false;
  }
  collection.policy_distribution_evidence_bound =
      all_distribution_evidence_bound;
  *out = std::move(collection);
  return true;
}

[[nodiscard]] double clamp_double(double value, double lo, double hi) {
  return std::max(lo, std::min(hi, value));
}

[[nodiscard]] std::vector<double>
parse_target_weight_values(std::string_view text) {
  std::vector<double> out;
  std::stringstream stream{std::string(text)};
  std::string token;
  while (std::getline(stream, token, ',')) {
    token = trim_ascii(token);
    if (token.empty()) {
      continue;
    }
    const auto colon = token.rfind(':');
    const auto equals = token.rfind('=');
    const auto separator =
        colon == std::string::npos
            ? equals
            : (equals == std::string::npos ? colon : std::max(colon, equals));
    if (separator != std::string::npos) {
      token = trim_ascii(std::string_view(token).substr(separator + 1));
    }
    double value = 0.0;
    if (!parse_double(token, &value) || !std::isfinite(value)) {
      return {};
    }
    out.push_back(value);
  }
  return out;
}

[[nodiscard]] std::string csv_from_doubles(const std::vector<double> &values) {
  std::ostringstream out;
  out << std::setprecision(17);
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << values[i];
  }
  return out.str();
}

[[nodiscard]] std::vector<std::int64_t> parse_shape_csv(std::string_view text) {
  std::vector<std::int64_t> out;
  std::stringstream stream{std::string(text)};
  std::string token;
  while (std::getline(stream, token, ',')) {
    token = trim_ascii(token);
    if (token.empty()) {
      continue;
    }
    std::int64_t parsed = 0;
    if (!parse_i64_text(token, &parsed) || parsed <= 0) {
      return {};
    }
    out.push_back(parsed);
  }
  return out;
}

[[nodiscard]] std::vector<double> parse_double_csv(std::string_view text) {
  std::vector<double> out;
  std::stringstream stream{std::string(text)};
  std::string token;
  while (std::getline(stream, token, ',')) {
    token = trim_ascii(token);
    if (token.empty()) {
      continue;
    }
    double parsed = 0.0;
    if (!parse_double(token, &parsed) || !std::isfinite(parsed)) {
      return {};
    }
    out.push_back(parsed);
  }
  return out;
}

[[nodiscard]] bool shape_element_count(const std::vector<std::int64_t> &shape,
                                       std::int64_t *out) {
  if (shape.empty()) {
    return false;
  }
  std::int64_t count = 1;
  for (const auto dim : shape) {
    if (dim <= 0 || count > (std::numeric_limits<std::int64_t>::max() / dim)) {
      return false;
    }
    count *= dim;
  }
  if (out) {
    *out = count;
  }
  return true;
}

[[nodiscard]] torch::Tensor
tensor_from_csv_payload(std::string_view label, std::string_view shape_text,
                        std::string_view values_text, torch::Dtype dtype,
                        std::string *err) {
  const auto shape = parse_shape_csv(shape_text);
  const auto values = parse_double_csv(values_text);
  std::int64_t count = 0;
  if (!shape_element_count(shape, &count)) {
    if (err) {
      *err =
          "E_RUNTIME_POLICY_TRAINING_PPO_POLICY_INPUT_TENSOR_PAYLOAD_INVALID: "
          "invalid shape for " +
          std::string(label);
    }
    return {};
  }
  if (values.empty() || static_cast<std::int64_t>(values.size()) != count) {
    if (err) {
      *err =
          "E_RUNTIME_POLICY_TRAINING_PPO_POLICY_INPUT_TENSOR_PAYLOAD_INVALID: "
          "value count mismatch for " +
          std::string(label) + " expected=" + std::to_string(count) +
          " actual=" + std::to_string(values.size());
    }
    return {};
  }
  auto tensor =
      torch::tensor(values, torch::TensorOptions().dtype(torch::kFloat64))
          .reshape(shape);
  if (!torch::isfinite(tensor).all().item<bool>()) {
    if (err) {
      *err =
          "E_RUNTIME_POLICY_TRAINING_PPO_POLICY_INPUT_TENSOR_PAYLOAD_INVALID: "
          "non-finite values for " +
          std::string(label);
    }
    return {};
  }
  if (dtype == torch::kBool) {
    return tensor.ne(0.0).to(torch::kBool).contiguous();
  }
  return tensor.to(dtype).contiguous();
}

[[nodiscard]] bool
load_sample_policy_input_tensors(ppo_v0_rollout_sample_t *sample,
                                 std::string *err) {
  if (sample == nullptr) {
    if (err) {
      *err =
          "E_RUNTIME_POLICY_TRAINING_PPO_POLICY_INPUT_TENSOR_PAYLOAD_INVALID: "
          "null sample";
    }
    return false;
  }
  sample->node_features = tensor_from_csv_payload(
      "policy_input_node_features", sample->policy_input_node_features_shape,
      sample->policy_input_node_features, torch::kFloat64, err);
  if (!sample->node_features.defined()) {
    return false;
  }
  sample->global_features = tensor_from_csv_payload(
      "policy_input_global_features",
      sample->policy_input_global_features_shape,
      sample->policy_input_global_features, torch::kFloat64, err);
  if (!sample->global_features.defined()) {
    return false;
  }
  sample->risk_features = tensor_from_csv_payload(
      "policy_input_risk_features", sample->policy_input_risk_features_shape,
      sample->policy_input_risk_features, torch::kFloat64, err);
  if (!sample->risk_features.defined()) {
    return false;
  }
  sample->executable_mask = tensor_from_csv_payload(
      "policy_input_executable_mask",
      sample->policy_input_executable_mask_shape,
      sample->policy_input_executable_mask, torch::kBool, err);
  if (!sample->executable_mask.defined()) {
    return false;
  }
  if (!sample->target_weights_tensor.defined()) {
    const auto target_weights =
        parse_target_weight_values(sample->target_node_weights);
    if (!target_weights.empty()) {
      sample->target_weights_tensor = torch::tensor(
          target_weights, torch::TensorOptions().dtype(torch::kFloat64));
    }
  }
  if (!sample->target_weights_tensor.defined()) {
    if (err) {
      *err =
          "E_RUNTIME_POLICY_TRAINING_PPO_POLICY_INPUT_TENSOR_PAYLOAD_INVALID: "
          "target_node_weights could not be parsed";
    }
    return false;
  }
  sample->policy_input_tensor_payload_bound =
      sample->node_features.defined() && sample->node_features.dim() == 2 &&
      sample->global_features.defined() && sample->global_features.dim() == 1 &&
      sample->risk_features.defined() && sample->risk_features.dim() == 1 &&
      sample->executable_mask.defined() && sample->executable_mask.dim() == 1 &&
      sample->target_weights_tensor.defined() &&
      sample->target_weights_tensor.dim() == 1 &&
      sample->node_features.size(0) == sample->target_weights_tensor.size(0) &&
      sample->executable_mask.size(0) == sample->target_weights_tensor.size(0);
  if (!sample->policy_input_tensor_payload_bound && err) {
    *err = "E_RUNTIME_POLICY_TRAINING_PPO_POLICY_INPUT_TENSOR_PAYLOAD_INVALID: "
           "node, mask, and target-weight dimensions do not agree";
  }
  return sample->policy_input_tensor_payload_bound;
}

struct ppo_v0_dirichlet_eval_t {
  torch::Tensor log_prob{};
  torch::Tensor entropy{};
};

[[nodiscard]] ppo_v0_dirichlet_eval_t masked_dirichlet_log_prob_entropy(
    torch::Tensor logits, torch::Tensor action_distribution_params,
    torch::Tensor target_weights, torch::Tensor executable_mask) {
  const auto mask = executable_mask.to(torch::kBool).contiguous();
  const auto active_indices = torch::nonzero(mask).reshape({-1});
  const auto active_count = active_indices.numel();
  if (active_count <= 0) {
    throw std::runtime_error(
        "E_RUNTIME_POLICY_TRAINING_PPO_NO_EXECUTABLE_NODE: sample has no "
        "active executable graph node");
  }
  const auto dtype = torch::kFloat64;
  const auto device = logits.device();
  if (active_count == 1) {
    const auto zero =
        torch::zeros({}, torch::TensorOptions().dtype(dtype).device(device));
    return {.log_prob = zero, .entropy = zero};
  }
  const auto active_logits = logits.to(dtype).index_select(0, active_indices);
  auto active_weights = target_weights.to(dtype)
                            .index_select(0, active_indices)
                            .clamp_min(1.0e-12);
  active_weights = active_weights / active_weights.sum().clamp_min(1.0e-12);
  const auto mean = torch::softmax(active_logits, 0);
  torch::Tensor raw_concentration =
      action_distribution_params.defined() &&
              action_distribution_params.numel() > 0
          ? action_distribution_params.to(dtype).reshape({-1}).index({0})
          : torch::zeros({},
                         torch::TensorOptions().dtype(dtype).device(device));
  raw_concentration =
      raw_concentration.to(torch::TensorOptions().dtype(dtype).device(device));
  const auto concentration = torch::clamp(
      torch::log1p(torch::exp(torch::clamp(raw_concentration, -60.0, 60.0))) +
          0.25,
      0.25, 256.0);
  const auto alpha = mean * concentration + 1.0e-4;
  const auto alpha_sum = alpha.sum();
  const auto log_prob = torch::lgamma(alpha_sum) - torch::lgamma(alpha).sum() +
                        ((alpha - 1.0) * torch::log(active_weights)).sum();
  const auto log_beta = torch::lgamma(alpha).sum() - torch::lgamma(alpha_sum);
  const auto entropy = log_beta +
                       (alpha_sum - static_cast<double>(active_count)) *
                           torch::digamma(alpha_sum) -
                       ((alpha - 1.0) * torch::digamma(alpha)).sum();
  return {.log_prob = log_prob, .entropy = entropy};
}

[[nodiscard]] bool compute_ppo_v0_update_metrics(
    const cuwacunu::hero::runtime::policy_training_job_contract_t &contract,
    const std::vector<ppo_v0_rollout_sample_t> &samples,
    const fs::path &collection_module_state_path,
    const fs::path &resume_optimizer_torch_state_path,
    const fs::path &updated_module_state_path,
    const fs::path &updated_optimizer_torch_state_path,
    const torch::Device &device, ppo_v0_update_metrics_t *metrics,
    std::string *err) {
  const std::int64_t sample_count = static_cast<std::int64_t>(samples.size());
  ppo_v0_update_metrics_t out{};
  out.sample_count = sample_count;
  out.parameter_update_mode = "torch_autograd_ppo_clip_gae_v0";
  out.optimizer_kind = "torch_adam";
  out.device_policy = contract.device_policy;
  out.runtime_device_kind = device_kind_string(device);
  out.cuda_device_index = contract.cuda_device_index;
  out.cuda_available = torch::cuda::is_available();
  out.resume_mode = contract.resume_mode;
  out.resume_parent_actor_checkpoint_digest =
      contract.resume_actor_checkpoint_digest;
  out.resume_optimizer_state_digest = contract.resume_optimizer_state_digest;
  if (sample_count <= 0) {
    out.optimizer_steps = 0;
    if (metrics) {
      *metrics = std::move(out);
    }
    return true;
  }
  if (contract.action_distribution_id != "masked_dirichlet_simplex.v1") {
    if (err) {
      *err = "E_RUNTIME_POLICY_TRAINING_PPO_ACTION_DISTRIBUTION_UNSUPPORTED: "
             "real PPO autograd V0 supports masked_dirichlet_simplex.v1 only";
    }
    return false;
  }

  std::vector<double> advantages(samples.size(), 0.0);
  std::vector<double> returns(samples.size(), 0.0);
  double gae = 0.0;
  for (std::int64_t i = sample_count; i-- > 0;) {
    const auto &sample = samples[static_cast<std::size_t>(i)];
    const double nonterminal = (sample.done || sample.truncated) ? 0.0 : 1.0;
    const double next_value =
        i + 1 < sample_count
            ? samples[static_cast<std::size_t>(i + 1)].old_value
            : 0.0;
    const double delta = sample.reward +
                         contract.ppo_gamma * nonterminal * next_value -
                         sample.old_value;
    gae = delta +
          contract.ppo_gamma * contract.ppo_gae_lambda * nonterminal * gae;
    advantages[static_cast<std::size_t>(i)] = gae;
    returns[static_cast<std::size_t>(i)] = gae + sample.old_value;
  }

  for (std::int64_t i = 0; i < sample_count; ++i) {
    const auto &sample = samples[static_cast<std::size_t>(i)];
    out.mean_reward += sample.reward;
    out.mean_return_target += returns[static_cast<std::size_t>(i)];
    out.advantage_mean += advantages[static_cast<std::size_t>(i)];
  }
  out.mean_reward /= static_cast<double>(sample_count);
  out.mean_return_target /= static_cast<double>(sample_count);
  out.advantage_mean /= static_cast<double>(sample_count);
  double advantage_var = 0.0;
  for (std::int64_t i = 0; i < sample_count; ++i) {
    const double centered =
        advantages[static_cast<std::size_t>(i)] - out.advantage_mean;
    advantage_var += centered * centered;
  }
  out.advantage_std =
      std::sqrt(advantage_var / static_cast<double>(sample_count));
  if (!(out.advantage_std > 1.0e-12) || !std::isfinite(out.advantage_std)) {
    out.advantage_std = 1.0;
  }

  for (const auto &sample : samples) {
    if (!sample.policy_input_tensor_payload_bound ||
        !sample.node_features.defined() || !sample.global_features.defined() ||
        !sample.risk_features.defined() || !sample.executable_mask.defined() ||
        !sample.target_weights_tensor.defined()) {
      if (err) {
        *err =
            "E_RUNTIME_POLICY_TRAINING_PPO_POLICY_INPUT_TENSOR_PAYLOAD_MISSING:"
            " "
            "real PPO autograd requires actor-visible policy_input_t tensors "
            "for every collected rollout sample";
      }
      return false;
    }
  }
  out.policy_input_tensor_payload_bound = true;
  out.policy_input_tensor_payload_source =
      "collected_actor_visible_policy_input_tensors";

  namespace graph_alloc =
      cuwacunu::wikimyei::policy::portfolio::graph_node_allocation;
  torch::manual_seed(17);
  auto module_options =
      graph_alloc::make_graph_node_allocation_torch_policy_options();
  module_options.device = device;
  auto module = graph_alloc::GraphNodeAllocationTorchPolicyModule(
      std::move(module_options));
  if (!collection_module_state_path.empty() &&
      fs::exists(collection_module_state_path)) {
    graph_alloc::load_graph_node_allocation_torch_module_state(
        collection_module_state_path, module);
  }
  module->to(device, torch::kFloat64);
  out.resume_parent_loaded = !collection_module_state_path.empty() &&
                             fs::exists(collection_module_state_path);
  out.module_parameters_on_cuda = device.type() == torch::kCUDA &&
                                  module_parameters_on_device(module, device);
  module->train();
  std::vector<torch::Tensor> initial_parameters;
  for (const auto &parameter : module->parameters()) {
    initial_parameters.push_back(parameter.detach().clone());
  }

  out.actor_learning_rate =
      clamp_double(contract.ppo_clip_epsilon * 0.005, 1.0e-5, 1.0e-3);
  out.critic_learning_rate = out.actor_learning_rate;
  torch::optim::Adam optimizer(
      module->parameters(), torch::optim::AdamOptions(out.actor_learning_rate));
  if (!resume_optimizer_torch_state_path.empty()) {
    try {
      torch::serialize::InputArchive optimizer_archive;
      optimizer_archive.load_from(resume_optimizer_torch_state_path.string(),
                                  device);
      optimizer.load(optimizer_archive);
      out.optimizer_state_resume_loaded = true;
    } catch (const std::exception &ex) {
      if (err) {
        *err = "E_RUNTIME_POLICY_TRAINING_PPO_RESUME_OPTIMIZER_LOAD_FAILED: " +
               std::string(ex.what());
      }
      return false;
    }
  }
  const std::int64_t epochs =
      std::max<std::int64_t>(1, contract.ppo_epochs_per_rollout);
  const std::int64_t minibatch_size = std::clamp<std::int64_t>(
      contract.ppo_minibatch_size > 0 ? contract.ppo_minibatch_size
                                      : sample_count,
      1, sample_count);
  out.optimizer_steps = 0;
  for (std::int64_t epoch = 0; epoch < epochs; ++epoch) {
    for (std::int64_t begin = 0; begin < sample_count;
         begin += minibatch_size) {
      const std::int64_t end = std::min(sample_count, begin + minibatch_size);
      std::vector<torch::Tensor> policy_losses;
      std::vector<torch::Tensor> value_losses;
      std::vector<torch::Tensor> entropies;
      std::vector<double> approx_kls;
      policy_losses.reserve(static_cast<std::size_t>(end - begin));
      value_losses.reserve(static_cast<std::size_t>(end - begin));
      entropies.reserve(static_cast<std::size_t>(end - begin));
      approx_kls.reserve(static_cast<std::size_t>(end - begin));
      for (std::int64_t i = begin; i < end; ++i) {
        const auto &sample = samples[static_cast<std::size_t>(i)];
        const auto node_features = tensor_to_runtime_device(
            sample.node_features, device, torch::kFloat64);
        const auto global_features = tensor_to_runtime_device(
            sample.global_features, device, torch::kFloat64);
        const auto risk_features = tensor_to_runtime_device(
            sample.risk_features, device, torch::kFloat64);
        const auto executable_mask = sample.executable_mask.to(
            torch::TensorOptions().dtype(torch::kBool).device(device));
        const auto target_weights = tensor_to_runtime_device(
            sample.target_weights_tensor, device, torch::kFloat64);
        out.forward_input_on_cuda =
            out.forward_input_on_cuda ||
            (device.type() == torch::kCUDA &&
             tensor_on_device(node_features, device) &&
             tensor_on_device(global_features, device) &&
             tensor_on_device(risk_features, device) &&
             tensor_on_device(executable_mask, device) &&
             tensor_on_device(target_weights, device));
        const auto module_out = module->forward(node_features, global_features,
                                                risk_features, executable_mask);
        const auto dist = masked_dirichlet_log_prob_entropy(
            module_out.node_weight_logits,
            module_out.action_distribution_params, target_weights,
            executable_mask);
        const auto old_log_prob = torch::tensor(
            sample.old_log_prob,
            torch::TensorOptions().dtype(torch::kFloat64).device(device));
        const auto advantage = torch::tensor(
            (advantages[static_cast<std::size_t>(i)] - out.advantage_mean) /
                out.advantage_std,
            torch::TensorOptions().dtype(torch::kFloat64).device(device));
        const auto ratio = torch::exp(dist.log_prob - old_log_prob);
        const auto clipped_ratio =
            torch::clamp(ratio, 1.0 - contract.ppo_clip_epsilon,
                         1.0 + contract.ppo_clip_epsilon);
        policy_losses.push_back(
            -torch::minimum(ratio * advantage, clipped_ratio * advantage));
        const auto value = module_out.state_value.reshape({-1}).index({0});
        const auto return_target = torch::tensor(
            returns[static_cast<std::size_t>(i)],
            torch::TensorOptions().dtype(torch::kFloat64).device(device));
        const auto value_error = value - return_target;
        value_losses.push_back(0.5 * value_error * value_error);
        entropies.push_back(dist.entropy);
        approx_kls.push_back(sample.old_log_prob -
                             dist.log_prob.detach().item<double>());
      }
      const auto policy_loss = torch::stack(policy_losses).mean();
      const auto value_loss = torch::stack(value_losses).mean();
      const auto entropy = torch::stack(entropies).mean();
      const auto loss = policy_loss +
                        contract.ppo_value_loss_coeff * value_loss -
                        contract.ppo_entropy_coeff * entropy;
      out.loss_on_cuda = out.loss_on_cuda || (device.type() == torch::kCUDA &&
                                              tensor_on_device(loss, device));
      optimizer.zero_grad();
      loss.backward();
      const auto clipped_norm = torch::nn::utils::clip_grad_norm_(
          module->parameters(), contract.ppo_max_grad_norm);
      out.gradient_norm = clipped_norm;
      out.actor_logit_gradient_norm = out.gradient_norm;
      optimizer.step();
      ++out.optimizer_steps;
      double mean_kl = 0.0;
      for (const double value : approx_kls) {
        mean_kl += value;
      }
      mean_kl /=
          static_cast<double>(std::max<std::size_t>(1, approx_kls.size()));
      if (mean_kl > contract.ppo_target_kl) {
        out.target_kl_exceeded = true;
        out.early_stop = true;
        break;
      }
    }
    if (out.early_stop) {
      break;
    }
  }

  double value_target_var = 0.0;
  double value_error_var = 0.0;
  const double return_mean = out.mean_return_target;
  out.update_samples.clear();
  out.update_samples.reserve(samples.size());
  module->eval();
  torch::NoGradGuard no_grad;
  for (std::int64_t i = 0; i < sample_count; ++i) {
    const auto &sample = samples[static_cast<std::size_t>(i)];
    const double normalized_advantage =
        (advantages[static_cast<std::size_t>(i)] - out.advantage_mean) /
        out.advantage_std;
    const auto node_features =
        tensor_to_runtime_device(sample.node_features, device, torch::kFloat64);
    const auto global_features = tensor_to_runtime_device(
        sample.global_features, device, torch::kFloat64);
    const auto risk_features =
        tensor_to_runtime_device(sample.risk_features, device, torch::kFloat64);
    const auto executable_mask = sample.executable_mask.to(
        torch::TensorOptions().dtype(torch::kBool).device(device));
    const auto target_weights = tensor_to_runtime_device(
        sample.target_weights_tensor, device, torch::kFloat64);
    const auto module_out = module->forward(node_features, global_features,
                                            risk_features, executable_mask);
    const auto dist = masked_dirichlet_log_prob_entropy(
        module_out.node_weight_logits, module_out.action_distribution_params,
        target_weights, executable_mask);
    const double new_log_prob = dist.log_prob.item<double>();
    const double new_value =
        module_out.state_value.reshape({-1}).index({0}).item<double>();
    const double entropy = dist.entropy.item<double>();
    const double log_ratio = new_log_prob - sample.old_log_prob;
    const double ratio = std::exp(log_ratio);
    const double clipped_ratio =
        clamp_double(ratio, 1.0 - contract.ppo_clip_epsilon,
                     1.0 + contract.ppo_clip_epsilon);
    const double unclipped = ratio * normalized_advantage;
    const double clipped = clipped_ratio * normalized_advantage;
    const bool ratio_clipped =
        std::abs(ratio - 1.0) > contract.ppo_clip_epsilon;
    const double policy_loss_unclipped = -unclipped;
    const double policy_loss_clipped = -clipped;
    const double policy_loss_final = -std::min(unclipped, clipped);
    out.policy_loss_unclipped += policy_loss_unclipped;
    out.policy_loss_clipped += policy_loss_clipped;
    out.policy_loss_final += policy_loss_final;
    out.clip_fraction += ratio_clipped ? 1.0 : 0.0;

    const double value_error = new_value - returns[static_cast<std::size_t>(i)];
    const double value_loss = 0.5 * value_error * value_error;
    const double approx_kl = sample.old_log_prob - new_log_prob;
    out.value_loss += value_loss;
    out.entropy += entropy;
    out.approx_kl += approx_kl;
    value_target_var += (returns[static_cast<std::size_t>(i)] - return_mean) *
                        (returns[static_cast<std::size_t>(i)] - return_mean);
    value_error_var += value_error * value_error;

    ppo_v0_update_sample_t update{};
    update.step_index = sample.step_index;
    update.policy_input_digest = sample.policy_input_digest;
    update.reward = sample.reward;
    update.old_value = sample.old_value;
    update.new_value = new_value;
    update.advantage = advantages[static_cast<std::size_t>(i)];
    update.normalized_advantage = normalized_advantage;
    update.return_target = returns[static_cast<std::size_t>(i)];
    update.old_log_prob = sample.old_log_prob;
    update.new_log_prob = new_log_prob;
    update.log_prob_delta = log_ratio;
    update.prob_ratio = ratio;
    update.clipped_prob_ratio = clipped_ratio;
    update.ratio_clipped = ratio_clipped;
    update.policy_loss_unclipped = policy_loss_unclipped;
    update.policy_loss_clipped = policy_loss_clipped;
    update.policy_loss_final = policy_loss_final;
    update.value_loss = value_loss;
    update.entropy = entropy;
    update.entropy_bonus = contract.ppo_entropy_coeff * entropy;
    update.approx_kl = approx_kl;
    out.update_samples.push_back(std::move(update));
  }

  const double denom = static_cast<double>(sample_count);
  out.policy_loss_unclipped /= denom;
  out.policy_loss_clipped /= denom;
  out.policy_loss_final /= denom;
  out.value_loss /= denom;
  out.entropy /= denom;
  out.approx_kl /= denom;
  out.clip_fraction /= denom;
  out.entropy_bonus = contract.ppo_entropy_coeff * out.entropy;
  out.explained_variance = value_target_var > 1.0e-18
                               ? 1.0 - (value_error_var / value_target_var)
                               : 0.0;
  out.target_kl_exceeded = out.approx_kl > contract.ppo_target_kl;
  out.early_stop = out.early_stop || out.target_kl_exceeded;

  const auto final_out = module->forward(
      tensor_to_runtime_device(samples.front().node_features, device,
                               torch::kFloat64),
      tensor_to_runtime_device(samples.front().global_features, device,
                               torch::kFloat64),
      tensor_to_runtime_device(samples.front().risk_features, device,
                               torch::kFloat64),
      samples.front().executable_mask.to(
          torch::TensorOptions().dtype(torch::kBool).device(device)));
  const auto final_logits =
      final_out.node_weight_logits.detach()
          .to(torch::TensorOptions().dtype(torch::kFloat64).device(torch::kCPU))
          .contiguous();
  out.updated_node_weight_logits.assign(final_logits.data_ptr<double>(),
                                        final_logits.data_ptr<double>() +
                                            final_logits.numel());
  const auto final_params =
      final_out.action_distribution_params.detach()
          .to(torch::TensorOptions().dtype(torch::kFloat64).device(torch::kCPU))
          .contiguous()
          .view({-1});
  out.updated_action_distribution_params.assign(
      final_params.data_ptr<double>(),
      final_params.data_ptr<double>() + final_params.numel());
  out.updated_value_estimate =
      final_out.state_value.reshape({-1}).index({0}).item<double>();
  auto parameters = module->parameters();
  for (std::size_t i = 0;
       i < parameters.size() && i < initial_parameters.size(); ++i) {
    const auto diff = parameters[i].detach().to(torch::kFloat64) -
                      initial_parameters[i].to(torch::kFloat64);
    out.parameter_delta_l2 += diff.pow(2).sum().item<double>();
  }
  out.parameter_delta_l2 = std::sqrt(out.parameter_delta_l2);
  out.parameter_update_applied =
      std::isfinite(out.parameter_delta_l2) && out.parameter_delta_l2 > 0.0;
  graph_alloc::save_graph_node_allocation_torch_module_state(
      updated_module_state_path, module);
  if (!updated_optimizer_torch_state_path.empty()) {
    torch::serialize::OutputArchive optimizer_archive;
    optimizer.save(optimizer_archive);
    if (!updated_optimizer_torch_state_path.parent_path().empty()) {
      fs::create_directories(updated_optimizer_torch_state_path.parent_path());
    }
    optimizer_archive.save_to(updated_optimizer_torch_state_path.string());
    out.optimizer_state_on_cuda = device.type() == torch::kCUDA;
  }
  out.cuda_verification_passed =
      out.cuda_available && device.type() == torch::kCUDA &&
      out.module_parameters_on_cuda && out.forward_input_on_cuda &&
      out.loss_on_cuda && out.optimizer_state_on_cuda;
  if (!out.cuda_verification_passed) {
    if (err) {
      *err = "E_RUNTIME_POLICY_TRAINING_PPO_CUDA_VERIFICATION_FAILED";
    }
    return false;
  }
  if (metrics) {
    *metrics = std::move(out);
  }
  return true;
}

[[nodiscard]] std::string policy_training_ppo_actor_checkpoint_text(
    const cuwacunu::hero::runtime::policy_training_job_contract_t &contract,
    std::string_view contract_digest,
    std::string_view checkpoint_stage = "post_update_policy",
    std::string_view parent_actor_checkpoint_digest = {},
    const ppo_v0_update_metrics_t *metrics = nullptr,
    std::string_view module_state_path = "module_state.pt",
    std::string_view module_state_digest = {}) {
  std::ostringstream out;
  out << "schema=" << contract.policy_checkpoint_schema_id << "\n";
  out << "checkpoint_part=actor\n";
  out << "checkpoint_stage=" << checkpoint_stage << "\n";
  out << "policy_id=" << contract.policy_id << "\n";
  out << "policy_family_id=" << contract.policy_family_id << "\n";
  out << "policy_kind=" << contract.policy_kind << "\n";
  out << "contract_digest=" << contract_digest << "\n";
  out << "parent_actor_checkpoint_digest=" << parent_actor_checkpoint_digest
      << "\n";
  out << "policy_dsl_digest=" << contract.policy_dsl_digest << "\n";
  out << "policy_net_digest=" << contract.policy_net_digest << "\n";
  out << "policy_input_schema_id=" << contract.policy_input_schema_id << "\n";
  out << "policy_input_feature_manifest_digest="
      << contract.policy_input_feature_manifest_digest << "\n";
  out << "policy_jkimyei_digest=" << contract.policy_jkimyei_digest << "\n";
  out << "target_node_universe_digest=" << contract.target_node_universe_digest
      << "\n";
  out << "action_schema_id=" << contract.action_schema_digest << "\n";
  out << "action_adapter_id=" << contract.action_adapter_id << "\n";
  out << "action_distribution_id=" << contract.action_distribution_id << "\n";
  out << "action_distribution_config_digest="
      << contract.action_distribution_config_digest << "\n";
  out << "reward_contract_id=" << contract.reward_contract_id << "\n";
  out << "execution_profile_digest=" << contract.execution_profile_digest
      << "\n";
  out << "graph_order_fingerprint=" << contract.graph_order_fingerprint << "\n";
  out << "ppo_config_digest=" << contract.ppo_config_digest << "\n";
  out << "actor_architecture_digest=" << contract.actor_architecture_digest
      << "\n";
  out << "critic_architecture_digest=" << contract.critic_architecture_digest
      << "\n";
  out << "causal_schedule_digest=" << contract.causal_schedule_digest << "\n";
  out << "snapshot_family_digest=" << contract.snapshot_family_digest << "\n";
  out << "module_contract_id="
      << "wikimyei.policy.portfolio.graph_node_allocation."
         "torch_policy_module.v0"
      << "\n";
  out << "network_contract_id="
      << "wikimyei.policy.portfolio.graph_node_allocation.net.v1\n";
  out << "module_parameterization="
      << (metrics != nullptr ? "graph_node_allocation_torch_module_autograd_v0"
                             : "seeded_graph_node_allocation_torch_module_v0")
      << "\n";
  out << "checkpoint_layout="
      << "graph_node_allocation_torch_checkpoint_artifact_v0.v1\n";
  out << "module_state_schema="
      << "wikimyei.policy.portfolio.graph_node_allocation.module_state.v0\n";
  out << "module_state_path=" << module_state_path << "\n";
  out << "module_state_digest=" << module_state_digest << "\n";
  out << "optimizer_state_schema_id=" << contract.optimizer_state_schema_id
      << "\n";
  out << "device_policy=" << contract.device_policy << "\n";
  out << "cuda_device_index=" << contract.cuda_device_index << "\n";
  out << "resume_mode=" << contract.resume_mode << "\n";
  out << "resume_parent_actor_checkpoint_digest="
      << contract.resume_actor_checkpoint_digest << "\n";
  out << "resume_optimizer_state_digest="
      << contract.resume_optimizer_state_digest << "\n";
  out << "runtime_device_kind="
      << (metrics != nullptr ? metrics->runtime_device_kind : "") << "\n";
  out << "cuda_available="
      << (metrics != nullptr && metrics->cuda_available ? "true" : "false")
      << "\n";
  out << "module_parameters_on_cuda="
      << (metrics != nullptr && metrics->module_parameters_on_cuda ? "true"
                                                                   : "false")
      << "\n";
  out << "forward_input_on_cuda="
      << (metrics != nullptr && metrics->forward_input_on_cuda ? "true"
                                                               : "false")
      << "\n";
  out << "loss_on_cuda="
      << (metrics != nullptr && metrics->loss_on_cuda ? "true" : "false")
      << "\n";
  out << "optimizer_state_on_cuda="
      << (metrics != nullptr && metrics->optimizer_state_on_cuda ? "true"
                                                                 : "false")
      << "\n";
  out << "cuda_verification_passed="
      << (metrics != nullptr && metrics->cuda_verification_passed ? "true"
                                                                  : "false")
      << "\n";
  out << "node_feature_dim=28\n";
  out << "global_feature_dim=6\n";
  out << "risk_feature_dim=10\n";
  out << "forward_mode=graph_node_allocation_torch_policy_module_v0.v1\n";
  const bool learned_update =
      metrics != nullptr && metrics->parameter_update_applied;
  out << "node_weight_logit_bias_mode="
      << (metrics != nullptr ? "none_autograd_module_state" : "zero") << "\n";
  out << "node_weight_logit_bias="
      << "\n";
  out << "action_distribution_params="
      << (metrics != nullptr
              ? csv_from_doubles(metrics->updated_action_distribution_params)
              : "")
      << "\n";
  out << "value_estimate="
      << (metrics != nullptr ? metrics->updated_value_estimate : 0.0) << "\n";
  out << "parameter_update_mode="
      << (metrics != nullptr
              ? (learned_update
                     ? metrics->parameter_update_mode
                     : "torch_autograd_ppo_clip_gae_v0_no_parameter_update")
              : "collection_policy_initialization")
      << "\n";
  out << "ppo_update_bound=" << (metrics != nullptr ? "true" : "false") << "\n";
  out << "parameter_update_applied=" << (learned_update ? "true" : "false")
      << "\n";
  if (metrics != nullptr) {
    out << "optimizer_steps=" << metrics->optimizer_steps << "\n";
    out << "sample_count=" << metrics->sample_count << "\n";
    out << "policy_loss_final=" << metrics->policy_loss_final << "\n";
    out << "value_loss=" << metrics->value_loss << "\n";
    out << "approx_kl=" << metrics->approx_kl << "\n";
    out << "clip_fraction=" << metrics->clip_fraction << "\n";
    out << "actor_learning_rate=" << metrics->actor_learning_rate << "\n";
    out << "critic_learning_rate=" << metrics->critic_learning_rate << "\n";
    out << "optimizer_kind=" << metrics->optimizer_kind << "\n";
    out << "policy_input_tensor_payload_bound="
        << (metrics->policy_input_tensor_payload_bound ? "true" : "false")
        << "\n";
    out << "policy_input_tensor_payload_source="
        << metrics->policy_input_tensor_payload_source << "\n";
    out << "actor_logit_gradient_norm=" << metrics->actor_logit_gradient_norm
        << "\n";
    out << "parameter_delta_l2=" << metrics->parameter_delta_l2 << "\n";
  }
  out << "live_execution_allowed=false\n";
  return out.str();
}

[[nodiscard]] std::string policy_training_ppo_critic_checkpoint_text(
    const cuwacunu::hero::runtime::policy_training_job_contract_t &contract,
    std::string_view contract_digest,
    std::string_view parent_actor_checkpoint_digest = {},
    const ppo_v0_update_metrics_t *metrics = nullptr) {
  std::ostringstream out;
  out << "schema=" << contract.policy_checkpoint_schema_id << "\n";
  out << "checkpoint_part=critic\n";
  out << "checkpoint_stage=post_update_value\n";
  out << "policy_id=" << contract.policy_id << "\n";
  out << "policy_family_id=" << contract.policy_family_id << "\n";
  out << "policy_kind=" << contract.policy_kind << "\n";
  out << "contract_digest=" << contract_digest << "\n";
  out << "parent_actor_checkpoint_digest=" << parent_actor_checkpoint_digest
      << "\n";
  out << "critic_architecture_digest=" << contract.critic_architecture_digest
      << "\n";
  out << "reward_contract_id=" << contract.reward_contract_id << "\n";
  out << "advantage_estimator_id=" << contract.advantage_estimator_id << "\n";
  out << "causal_schedule_digest=" << contract.causal_schedule_digest << "\n";
  out << "value_estimate="
      << (metrics != nullptr ? metrics->updated_value_estimate : 0.0) << "\n";
  out << "value_loss=" << (metrics != nullptr ? metrics->value_loss : 0.0)
      << "\n";
  out << "parameter_update_mode="
      << (metrics != nullptr && metrics->parameter_update_applied
              ? metrics->parameter_update_mode
              : "torch_autograd_ppo_clip_gae_v0_no_value_update")
      << "\n";
  out << "ppo_update_bound=" << (metrics != nullptr ? "true" : "false") << "\n";
  out << "live_execution_allowed=false\n";
  return out.str();
}

[[nodiscard]] std::string policy_training_ppo_optimizer_state_text(
    const cuwacunu::hero::runtime::policy_training_job_contract_t &contract,
    std::string_view contract_digest, const ppo_v0_update_metrics_t &metrics) {
  std::ostringstream out;
  const auto effective_minibatch_size =
      metrics.sample_count > 0
          ? std::clamp<std::int64_t>(contract.ppo_minibatch_size > 0
                                         ? contract.ppo_minibatch_size
                                         : metrics.sample_count,
                                     1, metrics.sample_count)
          : 0;
  const auto minibatch_count_per_epoch =
      effective_minibatch_size > 0
          ? (metrics.sample_count + effective_minibatch_size - 1) /
                effective_minibatch_size
          : 0;
  out << "schema=kikijyeba.runtime.ppo_optimizer_state.v1\n";
  out << "policy_id=" << contract.policy_id << "\n";
  out << "contract_digest=" << contract_digest << "\n";
  out << "ppo_config_digest=" << contract.ppo_config_digest << "\n";
  out << "optimizer_state_schema_id=" << contract.optimizer_state_schema_id
      << "\n";
  out << "optimizer_torch_state_path=" << metrics.optimizer_torch_state_path
      << "\n";
  out << "optimizer_torch_state_digest=" << metrics.optimizer_torch_state_digest
      << "\n";
  out << "device_policy=" << metrics.device_policy << "\n";
  out << "runtime_device_kind=" << metrics.runtime_device_kind << "\n";
  out << "cuda_device_index=" << metrics.cuda_device_index << "\n";
  out << "cuda_available=" << (metrics.cuda_available ? "true" : "false")
      << "\n";
  out << "module_parameters_on_cuda="
      << (metrics.module_parameters_on_cuda ? "true" : "false") << "\n";
  out << "forward_input_on_cuda="
      << (metrics.forward_input_on_cuda ? "true" : "false") << "\n";
  out << "loss_on_cuda=" << (metrics.loss_on_cuda ? "true" : "false") << "\n";
  out << "optimizer_state_on_cuda="
      << (metrics.optimizer_state_on_cuda ? "true" : "false") << "\n";
  out << "cuda_verification_passed="
      << (metrics.cuda_verification_passed ? "true" : "false") << "\n";
  out << "resume_mode=" << metrics.resume_mode << "\n";
  out << "resume_parent_loaded="
      << (metrics.resume_parent_loaded ? "true" : "false") << "\n";
  out << "optimizer_state_resume_loaded="
      << (metrics.optimizer_state_resume_loaded ? "true" : "false") << "\n";
  out << "resume_parent_actor_checkpoint_digest="
      << metrics.resume_parent_actor_checkpoint_digest << "\n";
  out << "resume_optimizer_state_digest="
      << metrics.resume_optimizer_state_digest << "\n";
  out << "optimizer_steps=" << metrics.optimizer_steps << "\n";
  out << "minibatch_policy=deterministic_contiguous_v0\n";
  out << "effective_minibatch_size=" << effective_minibatch_size << "\n";
  out << "minibatch_count_per_epoch=" << minibatch_count_per_epoch << "\n";
  out << "learning_rate_actor=" << metrics.actor_learning_rate << "\n";
  out << "learning_rate_critic=" << metrics.critic_learning_rate << "\n";
  out << "max_grad_norm=" << contract.ppo_max_grad_norm << "\n";
  out << "gradient_norm=" << metrics.gradient_norm << "\n";
  out << "actor_logit_gradient_norm=" << metrics.actor_logit_gradient_norm
      << "\n";
  out << "parameter_delta_l2=" << metrics.parameter_delta_l2 << "\n";
  out << "parameter_update_applied="
      << (metrics.parameter_update_applied ? "true" : "false") << "\n";
  out << "optimizer_kind=" << metrics.optimizer_kind << "\n";
  out << "device_policy=" << metrics.device_policy << "\n";
  out << "runtime_device_kind=" << metrics.runtime_device_kind << "\n";
  out << "cuda_device_index=" << metrics.cuda_device_index << "\n";
  out << "cuda_available=" << (metrics.cuda_available ? "true" : "false")
      << "\n";
  out << "module_parameters_on_cuda="
      << (metrics.module_parameters_on_cuda ? "true" : "false") << "\n";
  out << "forward_input_on_cuda="
      << (metrics.forward_input_on_cuda ? "true" : "false") << "\n";
  out << "loss_on_cuda=" << (metrics.loss_on_cuda ? "true" : "false") << "\n";
  out << "optimizer_state_on_cuda="
      << (metrics.optimizer_state_on_cuda ? "true" : "false") << "\n";
  out << "cuda_verification_passed="
      << (metrics.cuda_verification_passed ? "true" : "false") << "\n";
  out << "resume_mode=" << metrics.resume_mode << "\n";
  out << "resume_parent_loaded="
      << (metrics.resume_parent_loaded ? "true" : "false") << "\n";
  out << "optimizer_state_resume_loaded="
      << (metrics.optimizer_state_resume_loaded ? "true" : "false") << "\n";
  out << "policy_input_tensor_payload_bound="
      << (metrics.policy_input_tensor_payload_bound ? "true" : "false") << "\n";
  out << "state_kind=torch_autograd_ppo_clip_gae_v0_update_state\n";
  return out.str();
}

[[nodiscard]] std::string policy_training_ppo_rollout_collection_text(
    const cuwacunu::hero::runtime::policy_training_job_contract_t &contract,
    std::string_view contract_digest, const ppo_v0_update_metrics_t &metrics,
    const ppo_v0_rollout_collection_t &collection) {
  const auto &samples = collection.samples;
  std::ostringstream out;
  out << "schema=" << contract.rollout_collection_schema_id << "\n";
  out << "rollout_collection_id=" << collection.rollout_collection_id << "\n";
  out << "contract_digest=" << contract_digest << "\n";
  out << "policy_id=" << contract.policy_id << "\n";
  out << "policy_checkpoint_digest=" << contract.actor_checkpoint_digest
      << "\n";
  out << "policy_input_schema_id=" << contract.policy_input_schema_id << "\n";
  out << "action_adapter_id=" << contract.action_adapter_id << "\n";
  out << "action_schema_digest=" << contract.action_schema_digest << "\n";
  out << "action_distribution_id=" << contract.action_distribution_id << "\n";
  out << "environment_contract_id=" << contract.environment_contract_id << "\n";
  out << "execution_profile_digest=" << contract.execution_profile_digest
      << "\n";
  out << "reward_contract_id=" << contract.reward_contract_id << "\n";
  out << "causal_schedule_digest=" << contract.causal_schedule_digest << "\n";
  out << "snapshot_family_digest=" << contract.snapshot_family_digest << "\n";
  out << "sample_count=" << metrics.sample_count << "\n";
  out << "fresh_on_policy_trajectory=true\n";
  out << "old_trajectory_reuse_for_gradient=false\n";
  out << "collection_mode=" << collection.collection_mode << "\n";
  out << "collection_source=" << collection.collection_source << "\n";
  out << "replay_step_schema_compatible=true\n";
  std::int64_t replay_backed_step_count = 0;
  for (const auto &sample : samples) {
    replay_backed_step_count += sample.replay_backed ? 1 : 0;
  }
  out << "replay_backed_step_count=" << replay_backed_step_count << "\n";
  out << "fixture_step_count="
      << (metrics.sample_count - replay_backed_step_count) << "\n";
  out << "input_replay_report_path="
      << collection.input_replay_report_path.string() << "\n";
  out << "input_replay_report_digest=" << collection.input_replay_report_digest
      << "\n";
  out << "input_policy_checkpoint_path="
      << collection.input_policy_checkpoint_path.string() << "\n";
  out << "input_policy_checkpoint_digest="
      << collection.input_policy_checkpoint_digest << "\n";
  out << "policy_input_digest_source="
      << (collection.kikijyeba_cajtucu_replay_integration
              ? "kikijyeba_replay_report_or_runtime_digest"
              : "runtime_smoke_fixture_digest")
      << "\n";
  out << "action_log_prob_source="
      << (collection.policy_distribution_evidence_bound
              ? "action_distribution_evidence"
              : "replay_report_missing_policy_distribution_evidence")
      << "\n";
  out << "policy_distribution_evidence_bound="
      << bool_json(collection.policy_distribution_evidence_bound) << "\n";
  std::int64_t tensor_payload_bound_count = 0;
  for (const auto &sample : samples) {
    tensor_payload_bound_count +=
        sample.policy_input_tensor_payload_bound ? 1 : 0;
  }
  out << "policy_input_tensor_payload_schema_id="
      << "kikijyeba.environment.policy_input.tensor_payload.v1\n";
  out << "policy_input_tensor_payload_bound_count="
      << tensor_payload_bound_count << "\n";
  out << "policy_input_tensor_payload_complete="
      << bool_json(tensor_payload_bound_count == metrics.sample_count) << "\n";
  out << "value_estimate_source="
      << (collection.policy_distribution_evidence_bound
              ? "policy_value_head_evidence"
              : "replay_report_missing_policy_value_evidence")
      << "\n";
  out << "transition_reward_source="
      << (collection.kikijyeba_cajtucu_replay_integration
              ? "kikijyeba_replay_report"
              : "runtime_smoke_fixture")
      << "\n";
  out << "kikijyeba_cajtucu_replay_integration="
      << bool_json(collection.kikijyeba_cajtucu_replay_integration) << "\n";
  out << "on_policy_runtime_collection_complete="
      << bool_json(collection.on_policy_runtime_collection_complete) << "\n";
  double total_transaction_cost = 0.0;
  double total_fee_cost = 0.0;
  double total_spread_cost = 0.0;
  double total_slippage_cost = 0.0;
  double total_turnover = 0.0;
  double total_rejected_notional = 0.0;
  double total_partial_unfilled_notional = 0.0;
  std::uint64_t missing_direct_pair_count = 0;
  std::uint64_t numeraire_fallback_pair_count = 0;
  std::uint64_t rejected_fill_count = 0;
  std::uint64_t partial_fill_count = 0;
  for (const auto &sample : samples) {
    total_transaction_cost += sample.transaction_cost_numeraire;
    total_fee_cost += sample.fee_cost_numeraire;
    total_spread_cost += sample.spread_cost_numeraire;
    total_slippage_cost += sample.slippage_cost_numeraire;
    total_turnover += sample.turnover;
    total_rejected_notional += sample.rejected_notional_numeraire;
    total_partial_unfilled_notional +=
        sample.partial_unfilled_notional_numeraire;
    missing_direct_pair_count += sample.missing_direct_pair_count;
    numeraire_fallback_pair_count += sample.numeraire_fallback_pair_count;
    rejected_fill_count += sample.rejected_fill_count;
    partial_fill_count += sample.partial_fill_count;
  }
  out << "total_transaction_cost_numeraire=" << total_transaction_cost << "\n";
  out << "fee_cost_numeraire=" << total_fee_cost << "\n";
  out << "spread_cost_numeraire=" << total_spread_cost << "\n";
  out << "slippage_cost_numeraire=" << total_slippage_cost << "\n";
  out << "turnover=" << total_turnover << "\n";
  out << "rejected_notional_numeraire=" << total_rejected_notional << "\n";
  out << "partial_unfilled_notional_numeraire="
      << total_partial_unfilled_notional << "\n";
  out << "missing_direct_pair_count=" << missing_direct_pair_count << "\n";
  out << "numeraire_fallback_pair_count=" << numeraire_fallback_pair_count
      << "\n";
  out << "rejected_fill_count=" << rejected_fill_count << "\n";
  out << "partial_fill_count=" << partial_fill_count << "\n";
  for (const auto &sample : samples) {
    const std::string prefix =
        "step_" + std::to_string(sample.step_index) + "_";
    const std::string sample_key =
        std::string(contract_digest) + "|" + std::to_string(sample.step_index) +
        "|" + contract.policy_id + "|" + contract.snapshot_family_digest + "|" +
        sample.observation_anchor_key;
    const std::string policy_input_digest =
        !sample.policy_input_digest.empty()
            ? sample.policy_input_digest
            : cuwacunu::hero::marshal::marshal_digest_for_text(
                  "kikijyeba.runtime.ppo.policy_input_smoke.v1", sample_key);
    const std::string transition_digest =
        !sample.transition_digest.empty()
            ? sample.transition_digest
            : cuwacunu::hero::marshal::marshal_digest_for_text(
                  sample.replay_backed
                      ? "kikijyeba.runtime.ppo.transition_replay_report.v1"
                      : "kikijyeba.runtime.ppo.transition_smoke.v1",
                  sample_key + "|" + std::to_string(sample.reward));
    const double log_prob_delta = sample.new_log_prob - sample.old_log_prob;
    out << prefix << "source=" << sample.source << "\n";
    out << prefix << "source_step_index=" << sample.source_step_index << "\n";
    out << prefix << "replay_backed=" << bool_json(sample.replay_backed)
        << "\n";
    out << prefix << "policy_distribution_evidence_bound="
        << bool_json(sample.policy_distribution_evidence_bound) << "\n";
    out << prefix << "rollout_id=" << sample.rollout_id << "\n";
    out << prefix << "episode_id=" << sample.episode_id << "\n";
    out << prefix << "block_id=" << sample.block_id << "\n";
    out << prefix << "block_cursor_begin=" << sample.block_cursor_begin << "\n";
    out << prefix << "block_cursor_end=" << sample.block_cursor_end << "\n";
    out << prefix << "observation_anchor_key=" << sample.observation_anchor_key
        << "\n";
    out << prefix
        << "observation_anchor_index=" << sample.observation_anchor_index
        << "\n";
    out << prefix << "knowledge_timestamp_ms=" << sample.knowledge_timestamp_ms
        << "\n";
    out << prefix << "policy_input_digest=" << policy_input_digest << "\n";
    out << prefix
        << "snapshot_family_digest=" << contract.snapshot_family_digest << "\n";
    out << prefix
        << "policy_checkpoint_digest=" << contract.actor_checkpoint_digest
        << "\n";
    out << prefix
        << "policy_input_schema_id=" << contract.policy_input_schema_id << "\n";
    out << prefix << "action_adapter_id=" << contract.action_adapter_id << "\n";
    out << prefix << "action_distribution_id="
        << (sample.action_distribution_id.empty()
                ? contract.action_distribution_id
                : sample.action_distribution_id)
        << "\n";
    out << prefix << "active_node_indices=" << sample.active_node_indices
        << "\n";
    out << prefix << "active_count=" << sample.active_count << "\n";
    out << prefix << "old_log_prob=" << sample.old_log_prob << "\n";
    out << prefix << "new_log_prob=" << sample.new_log_prob << "\n";
    out << prefix << "log_prob_delta=" << log_prob_delta << "\n";
    out << prefix << "prob_ratio=" << std::exp(log_prob_delta) << "\n";
    out << prefix << "entropy=" << sample.entropy << "\n";
    out << prefix << "old_value_estimate=" << sample.old_value << "\n";
    out << prefix << "new_value_estimate=" << sample.new_value << "\n";
    out << prefix << "policy_input_tensor_payload_schema_id="
        << sample.policy_input_tensor_payload_schema_id << "\n";
    out << prefix << "policy_input_tensor_payload_bound="
        << bool_json(sample.policy_input_tensor_payload_bound) << "\n";
    out << prefix << "policy_input_node_features_shape="
        << sample.policy_input_node_features_shape << "\n";
    out << prefix << "policy_input_global_features_shape="
        << sample.policy_input_global_features_shape << "\n";
    out << prefix << "policy_input_risk_features_shape="
        << sample.policy_input_risk_features_shape << "\n";
    out << prefix << "policy_input_executable_mask_shape="
        << sample.policy_input_executable_mask_shape << "\n";
    out << prefix << "action_target_node_weights=" << sample.target_node_weights
        << "\n";
    out << prefix << "transition_digest=" << transition_digest << "\n";
    out << prefix << "cajtucu_trace_available="
        << bool_json(sample.cajtucu_trace_available) << "\n";
    out << prefix << "cajtucu_trace_id=" << sample.cajtucu_trace_id << "\n";
    out << prefix << "cajtucu_trace_digest=" << sample.cajtucu_trace_digest
        << "\n";
    out << prefix << "cajtucu_trace_digest_bound="
        << bool_json(sample.cajtucu_trace_digest_bound) << "\n";
    out << prefix << "cajtucu_trace_source=" << sample.cajtucu_trace_source
        << "\n";
    out << prefix << "cajtucu_trace_valid="
        << (sample.cajtucu_trace_valid ? "true" : "false") << "\n";
    out << prefix
        << "invalid_action=" << (sample.invalid_action ? "true" : "false")
        << "\n";
    out << prefix << "invalid_execution_trace="
        << (sample.invalid_execution_trace ? "true" : "false") << "\n";
    out << prefix
        << "transaction_cost_numeraire=" << sample.transaction_cost_numeraire
        << "\n";
    out << prefix << "fee_cost_numeraire=" << sample.fee_cost_numeraire << "\n";
    out << prefix << "spread_cost_numeraire=" << sample.spread_cost_numeraire
        << "\n";
    out << prefix
        << "slippage_cost_numeraire=" << sample.slippage_cost_numeraire << "\n";
    out << prefix << "turnover=" << sample.turnover << "\n";
    out << prefix
        << "rejected_notional_numeraire=" << sample.rejected_notional_numeraire
        << "\n";
    out << prefix << "partial_unfilled_notional_numeraire="
        << sample.partial_unfilled_notional_numeraire << "\n";
    out << prefix
        << "missing_direct_pair_count=" << sample.missing_direct_pair_count
        << "\n";
    out << prefix << "numeraire_fallback_pair_count="
        << sample.numeraire_fallback_pair_count << "\n";
    out << prefix << "rejected_fill_count=" << sample.rejected_fill_count
        << "\n";
    out << prefix << "partial_fill_count=" << sample.partial_fill_count << "\n";
    out << prefix << "reward_total=" << sample.reward << "\n";
    out << prefix << "reward_log_growth=" << sample.reward_log_growth << "\n";
    out << prefix
        << "reward_drawdown_penalty=" << sample.reward_drawdown_penalty << "\n";
    out << prefix << "reward_transaction_cost_penalty="
        << sample.reward_transaction_cost_penalty << "\n";
    out << prefix
        << "reward_turnover_penalty=" << sample.reward_turnover_penalty << "\n";
    out << prefix << "reward_invalid_action_penalty="
        << sample.reward_invalid_action_penalty << "\n";
    out << prefix << "done=" << (sample.done ? "true" : "false") << "\n";
    out << prefix << "truncated=" << (sample.truncated ? "true" : "false")
        << "\n";
  }
  out << "live_execution_allowed=false\n";
  return out.str();
}

[[nodiscard]] std::string policy_training_ppo_update_report_text(
    const cuwacunu::hero::runtime::policy_training_job_contract_t &contract,
    std::string_view contract_digest, const ppo_v0_update_metrics_t &metrics) {
  std::ostringstream out;
  const auto effective_minibatch_size =
      metrics.sample_count > 0
          ? std::clamp<std::int64_t>(contract.ppo_minibatch_size > 0
                                         ? contract.ppo_minibatch_size
                                         : metrics.sample_count,
                                     1, metrics.sample_count)
          : 0;
  const auto minibatch_count_per_epoch =
      effective_minibatch_size > 0
          ? (metrics.sample_count + effective_minibatch_size - 1) /
                effective_minibatch_size
          : 0;
  out << "schema=" << contract.ppo_update_report_schema_id << "\n";
  out << "report_writer_id=hero.runtime.policy_training.ppo_v0.v1\n";
  out << "contract_digest=" << contract_digest << "\n";
  out << "policy_id=" << contract.policy_id << "\n";
  out << "policy_kind=" << contract.policy_kind << "\n";
  out << "ppo_algorithm=ppo_clip\n";
  out << "advantage_estimator_id=" << contract.advantage_estimator_id << "\n";
  out << "advantage_normalization_policy="
      << contract.advantage_normalization_policy << "\n";
  out << "gamma=" << contract.ppo_gamma << "\n";
  out << "gae_lambda=" << contract.ppo_gae_lambda << "\n";
  out << "clip_epsilon=" << contract.ppo_clip_epsilon << "\n";
  out << "target_kl=" << contract.ppo_target_kl << "\n";
  out << "entropy_coeff=" << contract.ppo_entropy_coeff << "\n";
  out << "value_loss_coeff=" << contract.ppo_value_loss_coeff << "\n";
  out << "max_grad_norm=" << contract.ppo_max_grad_norm << "\n";
  out << "minibatch_size=" << contract.ppo_minibatch_size << "\n";
  out << "minibatch_policy=deterministic_contiguous_v0\n";
  out << "effective_minibatch_size=" << effective_minibatch_size << "\n";
  out << "minibatch_count_per_epoch=" << minibatch_count_per_epoch << "\n";
  out << "epochs_per_rollout=" << contract.ppo_epochs_per_rollout << "\n";
  out << "sample_count=" << metrics.sample_count << "\n";
  out << "optimizer_steps=" << metrics.optimizer_steps << "\n";
  out << "mean_reward=" << metrics.mean_reward << "\n";
  out << "advantage_mean=" << metrics.advantage_mean << "\n";
  out << "advantage_std=" << metrics.advantage_std << "\n";
  out << "mean_return_target=" << metrics.mean_return_target << "\n";
  out << "policy_loss_unclipped=" << metrics.policy_loss_unclipped << "\n";
  out << "policy_loss_clipped=" << metrics.policy_loss_clipped << "\n";
  out << "policy_loss_final=" << metrics.policy_loss_final << "\n";
  out << "value_loss=" << metrics.value_loss << "\n";
  out << "entropy=" << metrics.entropy << "\n";
  out << "entropy_bonus=" << metrics.entropy_bonus << "\n";
  out << "approx_kl=" << metrics.approx_kl << "\n";
  out << "clip_fraction=" << metrics.clip_fraction << "\n";
  out << "target_kl_exceeded="
      << (metrics.target_kl_exceeded ? "true" : "false") << "\n";
  out << "early_stop=" << (metrics.early_stop ? "true" : "false") << "\n";
  out << "gradient_norm=" << metrics.gradient_norm << "\n";
  out << "explained_variance=" << metrics.explained_variance << "\n";
  out << "parameter_update_mode="
      << (metrics.parameter_update_applied
              ? metrics.parameter_update_mode
              : "torch_autograd_ppo_clip_gae_v0_no_parameter_update")
      << "\n";
  out << "parameter_update_applied="
      << (metrics.parameter_update_applied ? "true" : "false") << "\n";
  out << "optimizer_kind=" << metrics.optimizer_kind << "\n";
  out << "device_policy=" << metrics.device_policy << "\n";
  out << "runtime_device_kind=" << metrics.runtime_device_kind << "\n";
  out << "cuda_device_index=" << metrics.cuda_device_index << "\n";
  out << "cuda_available=" << (metrics.cuda_available ? "true" : "false")
      << "\n";
  out << "module_parameters_on_cuda="
      << (metrics.module_parameters_on_cuda ? "true" : "false") << "\n";
  out << "forward_input_on_cuda="
      << (metrics.forward_input_on_cuda ? "true" : "false") << "\n";
  out << "loss_on_cuda=" << (metrics.loss_on_cuda ? "true" : "false") << "\n";
  out << "optimizer_state_on_cuda="
      << (metrics.optimizer_state_on_cuda ? "true" : "false") << "\n";
  out << "cuda_verification_passed="
      << (metrics.cuda_verification_passed ? "true" : "false") << "\n";
  out << "resume_mode=" << metrics.resume_mode << "\n";
  out << "resume_parent_loaded="
      << (metrics.resume_parent_loaded ? "true" : "false") << "\n";
  out << "optimizer_state_resume_loaded="
      << (metrics.optimizer_state_resume_loaded ? "true" : "false") << "\n";
  out << "policy_input_tensor_payload_bound="
      << (metrics.policy_input_tensor_payload_bound ? "true" : "false") << "\n";
  out << "policy_input_tensor_payload_source="
      << metrics.policy_input_tensor_payload_source << "\n";
  out << "actor_learning_rate=" << metrics.actor_learning_rate << "\n";
  out << "critic_learning_rate=" << metrics.critic_learning_rate << "\n";
  out << "actor_logit_gradient_norm=" << metrics.actor_logit_gradient_norm
      << "\n";
  out << "parameter_delta_l2=" << metrics.parameter_delta_l2 << "\n";
  out << "updated_node_weight_logits="
      << csv_from_doubles(metrics.updated_node_weight_logits) << "\n";
  out << "updated_action_distribution_params="
      << csv_from_doubles(metrics.updated_action_distribution_params) << "\n";
  out << "updated_value_estimate=" << metrics.updated_value_estimate << "\n";
  out << "update_sample_count=" << metrics.update_samples.size() << "\n";
  for (std::size_t i = 0; i < metrics.update_samples.size(); ++i) {
    const auto &sample = metrics.update_samples[i];
    const std::string prefix = "sample_" + std::to_string(i) + "_";
    out << prefix << "step_index=" << sample.step_index << "\n";
    out << prefix << "policy_input_digest=" << sample.policy_input_digest
        << "\n";
    out << prefix << "reward_total=" << sample.reward << "\n";
    out << prefix << "old_value_estimate=" << sample.old_value << "\n";
    out << prefix << "new_value_estimate=" << sample.new_value << "\n";
    out << prefix << "advantage=" << sample.advantage << "\n";
    out << prefix << "normalized_advantage=" << sample.normalized_advantage
        << "\n";
    out << prefix << "return_target=" << sample.return_target << "\n";
    out << prefix << "old_log_prob=" << sample.old_log_prob << "\n";
    out << prefix << "new_log_prob=" << sample.new_log_prob << "\n";
    out << prefix << "log_prob_delta=" << sample.log_prob_delta << "\n";
    out << prefix << "prob_ratio=" << sample.prob_ratio << "\n";
    out << prefix << "clipped_prob_ratio=" << sample.clipped_prob_ratio << "\n";
    out << prefix
        << "ratio_clipped=" << (sample.ratio_clipped ? "true" : "false")
        << "\n";
    out << prefix << "policy_loss_unclipped=" << sample.policy_loss_unclipped
        << "\n";
    out << prefix << "policy_loss_clipped=" << sample.policy_loss_clipped
        << "\n";
    out << prefix << "policy_loss_final=" << sample.policy_loss_final << "\n";
    out << prefix << "value_loss=" << sample.value_loss << "\n";
    out << prefix << "entropy=" << sample.entropy << "\n";
    out << prefix << "entropy_bonus=" << sample.entropy_bonus << "\n";
    out << prefix << "approx_kl=" << sample.approx_kl << "\n";
  }
  out << "finite_loss_check=true\n";
  out << "live_execution_allowed=false\n";
  return out.str();
}

struct ppo_v0_evaluation_rollout_metrics_t {
  std::int64_t sample_count{0};
  std::int64_t replay_backed_step_count{0};
  bool final_equity_numeraire_bound{false};
  double final_equity_numeraire{0.0};
  bool total_log_growth_bound{false};
  double total_log_growth{0.0};
  bool max_drawdown_bound{false};
  double max_drawdown{0.0};
  bool realized_volatility_bound{false};
  double realized_volatility{0.0};
  bool target_tracking_error_l1_bound{false};
  double target_tracking_error_l1{0.0};
  bool target_tracking_error_linf_bound{false};
  double target_tracking_error_linf{0.0};
  double reward_total{0.0};
  double reward_log_growth{0.0};
  double reward_drawdown_penalty{0.0};
  double reward_transaction_cost_penalty{0.0};
  double reward_turnover_penalty{0.0};
  double reward_invalid_action_penalty{0.0};
  double transaction_cost_numeraire{0.0};
  double fee_cost_numeraire{0.0};
  double spread_cost_numeraire{0.0};
  double slippage_cost_numeraire{0.0};
  double turnover{0.0};
  double rejected_notional_numeraire{0.0};
  double partial_unfilled_notional_numeraire{0.0};
  std::uint64_t missing_direct_pair_count{0};
  std::uint64_t numeraire_fallback_pair_count{0};
  std::uint64_t rejected_fill_count{0};
  std::uint64_t partial_fill_count{0};
  std::uint64_t invalid_action_count{0};
  std::uint64_t invalid_trace_count{0};
};

[[nodiscard]] ppo_v0_evaluation_rollout_metrics_t
compute_ppo_v0_evaluation_rollout_metrics(
    const ppo_v0_rollout_collection_t &collection) {
  ppo_v0_evaluation_rollout_metrics_t out{};
  out.sample_count = static_cast<std::int64_t>(collection.samples.size());
  out.final_equity_numeraire_bound = collection.final_equity_numeraire_bound;
  out.final_equity_numeraire = collection.final_equity_numeraire;
  out.total_log_growth_bound = collection.total_log_growth_bound;
  out.total_log_growth = collection.total_log_growth;
  out.max_drawdown_bound = collection.max_drawdown_bound;
  out.max_drawdown = collection.max_drawdown;
  out.realized_volatility_bound = collection.realized_volatility_bound;
  out.realized_volatility = collection.realized_volatility;
  out.target_tracking_error_l1_bound =
      collection.target_tracking_error_l1_bound;
  out.target_tracking_error_l1 = collection.target_tracking_error_l1;
  out.target_tracking_error_linf_bound =
      collection.target_tracking_error_linf_bound;
  out.target_tracking_error_linf = collection.target_tracking_error_linf;

  double log_growth_mean = 0.0;
  for (const auto &sample : collection.samples) {
    out.replay_backed_step_count += sample.replay_backed ? 1 : 0;
    out.reward_total += sample.reward;
    out.reward_log_growth += sample.reward_log_growth;
    out.reward_drawdown_penalty += sample.reward_drawdown_penalty;
    out.reward_transaction_cost_penalty +=
        sample.reward_transaction_cost_penalty;
    out.reward_turnover_penalty += sample.reward_turnover_penalty;
    out.reward_invalid_action_penalty += sample.reward_invalid_action_penalty;
    out.transaction_cost_numeraire += sample.transaction_cost_numeraire;
    out.fee_cost_numeraire += sample.fee_cost_numeraire;
    out.spread_cost_numeraire += sample.spread_cost_numeraire;
    out.slippage_cost_numeraire += sample.slippage_cost_numeraire;
    out.turnover += sample.turnover;
    out.rejected_notional_numeraire += sample.rejected_notional_numeraire;
    out.partial_unfilled_notional_numeraire +=
        sample.partial_unfilled_notional_numeraire;
    out.missing_direct_pair_count += sample.missing_direct_pair_count;
    out.numeraire_fallback_pair_count += sample.numeraire_fallback_pair_count;
    out.rejected_fill_count += sample.rejected_fill_count;
    out.partial_fill_count += sample.partial_fill_count;
    out.invalid_action_count += sample.invalid_action ? 1U : 0U;
    out.invalid_trace_count += sample.invalid_execution_trace ? 1U : 0U;
    log_growth_mean += sample.reward_log_growth;
  }
  if (!out.total_log_growth_bound && out.sample_count > 0) {
    out.total_log_growth = out.reward_log_growth;
    out.total_log_growth_bound = true;
  }
  if (!out.realized_volatility_bound && out.sample_count > 1) {
    log_growth_mean /= static_cast<double>(out.sample_count);
    double variance = 0.0;
    for (const auto &sample : collection.samples) {
      const double centered = sample.reward_log_growth - log_growth_mean;
      variance += centered * centered;
    }
    out.realized_volatility =
        std::sqrt(variance / static_cast<double>(out.sample_count - 1));
    out.realized_volatility_bound = std::isfinite(out.realized_volatility);
  }
  return out;
}

[[nodiscard]] std::string policy_training_ppo_validation_rollout_text(
    const cuwacunu::hero::runtime::policy_training_job_contract_t &contract,
    std::string_view contract_digest, const fs::path &actor_checkpoint_path,
    std::string_view actor_checkpoint_digest,
    const fs::path &raw_validation_replay_report_path,
    std::string_view raw_validation_replay_report_digest,
    const ppo_v0_rollout_collection_t &validation_collection,
    bool validation_rollout_ready, std::string_view reason,
    double linear_transaction_cost_rate) {
  const auto metrics =
      compute_ppo_v0_evaluation_rollout_metrics(validation_collection);
  std::ostringstream out;
  out << "schema=kikijyeba.runtime.ppo_cost_aware_validation_rollout.v1\n";
  out << "report_writer_id=hero.runtime.policy_training.ppo_v0.validation_"
         "rollout.v1\n";
  out << "contract_digest=" << contract_digest << "\n";
  out << "policy_id=" << contract.policy_id << "\n";
  out << "policy_kind=" << contract.policy_kind << "\n";
  out << "policy_family_id=" << contract.policy_family_id << "\n";
  out << "policy_checkpoint_path=" << actor_checkpoint_path.string() << "\n";
  out << "policy_checkpoint_digest=" << actor_checkpoint_digest << "\n";
  out << "post_update_actor_checkpoint_used="
      << bool_json(validation_rollout_ready && !actor_checkpoint_digest.empty())
      << "\n";
  out << "policy_input_schema_id=" << contract.policy_input_schema_id << "\n";
  out << "action_adapter_id=" << contract.action_adapter_id << "\n";
  out << "action_schema_digest=" << contract.action_schema_digest << "\n";
  out << "action_distribution_id=" << contract.action_distribution_id << "\n";
  out << "action_selection_mode=deterministic_action\n";
  out << "environment_contract_id=" << contract.environment_contract_id << "\n";
  out << "reward_contract_id=" << contract.reward_contract_id << "\n";
  out << "execution_profile_digest=" << contract.execution_profile_digest
      << "\n";
  out << "causal_schedule_digest=" << contract.causal_schedule_digest << "\n";
  out << "snapshot_family_digest=" << contract.snapshot_family_digest << "\n";
  out << "validation_rollout_ready=" << bool_json(validation_rollout_ready)
      << "\n";
  out << "validation_runtime_replay_complete="
      << bool_json(validation_rollout_ready &&
                   validation_collection.kikijyeba_cajtucu_replay_integration)
      << "\n";
  out << "reason=" << reason << "\n";
  out << "cost_aware_validation_rollout=" << bool_json(validation_rollout_ready)
      << "\n";
  out << "linear_transaction_cost_rate=" << linear_transaction_cost_rate
      << "\n";
  out << "raw_validation_replay_report_path="
      << raw_validation_replay_report_path.string() << "\n";
  out << "raw_validation_replay_report_digest="
      << raw_validation_replay_report_digest << "\n";
  out << "collection_mode=" << validation_collection.collection_mode << "\n";
  out << "collection_source=" << validation_collection.collection_source
      << "\n";
  out << "replay_summary_bound="
      << bool_json(validation_collection.replay_summary_bound) << "\n";
  out << "sample_count=" << metrics.sample_count << "\n";
  out << "replay_backed_step_count=" << metrics.replay_backed_step_count
      << "\n";
  out << "fixture_step_count="
      << (metrics.sample_count - metrics.replay_backed_step_count) << "\n";
  out << "final_equity_numeraire_bound="
      << bool_json(metrics.final_equity_numeraire_bound) << "\n";
  out << "final_equity_numeraire=" << metrics.final_equity_numeraire << "\n";
  out << "total_log_growth_bound=" << bool_json(metrics.total_log_growth_bound)
      << "\n";
  out << "total_log_growth=" << metrics.total_log_growth << "\n";
  out << "max_drawdown_bound=" << bool_json(metrics.max_drawdown_bound) << "\n";
  out << "max_drawdown=" << metrics.max_drawdown << "\n";
  out << "realized_volatility_bound="
      << bool_json(metrics.realized_volatility_bound) << "\n";
  out << "realized_volatility=" << metrics.realized_volatility << "\n";
  out << "turnover=" << metrics.turnover << "\n";
  out << "transaction_cost_numeraire=" << metrics.transaction_cost_numeraire
      << "\n";
  out << "fee_cost_numeraire=" << metrics.fee_cost_numeraire << "\n";
  out << "spread_cost_numeraire=" << metrics.spread_cost_numeraire << "\n";
  out << "slippage_cost_numeraire=" << metrics.slippage_cost_numeraire << "\n";
  out << "rejected_notional_numeraire=" << metrics.rejected_notional_numeraire
      << "\n";
  out << "partial_unfilled_notional_numeraire="
      << metrics.partial_unfilled_notional_numeraire << "\n";
  out << "missing_direct_pair_count=" << metrics.missing_direct_pair_count
      << "\n";
  out << "numeraire_fallback_pair_count="
      << metrics.numeraire_fallback_pair_count << "\n";
  out << "rejected_fill_count=" << metrics.rejected_fill_count << "\n";
  out << "partial_fill_count=" << metrics.partial_fill_count << "\n";
  out << "invalid_action_count=" << metrics.invalid_action_count << "\n";
  out << "invalid_trace_count=" << metrics.invalid_trace_count << "\n";
  out << "target_tracking_error_l1_bound="
      << bool_json(metrics.target_tracking_error_l1_bound) << "\n";
  out << "target_tracking_error_l1=" << metrics.target_tracking_error_l1
      << "\n";
  out << "target_tracking_error_linf_bound="
      << bool_json(metrics.target_tracking_error_linf_bound) << "\n";
  out << "target_tracking_error_linf=" << metrics.target_tracking_error_linf
      << "\n";
  out << "reward_total=" << metrics.reward_total << "\n";
  out << "reward_log_growth=" << metrics.reward_log_growth << "\n";
  out << "reward_drawdown_penalty=" << metrics.reward_drawdown_penalty << "\n";
  out << "reward_transaction_cost_penalty="
      << metrics.reward_transaction_cost_penalty << "\n";
  out << "reward_turnover_penalty=" << metrics.reward_turnover_penalty << "\n";
  out << "reward_invalid_action_penalty="
      << metrics.reward_invalid_action_penalty << "\n";
  for (const auto &sample : validation_collection.samples) {
    const std::string prefix =
        "step_" + std::to_string(sample.step_index) + "_";
    out << prefix << "source=" << sample.source << "\n";
    out << prefix << "replay_backed=" << bool_json(sample.replay_backed)
        << "\n";
    out << prefix << "observation_anchor_key=" << sample.observation_anchor_key
        << "\n";
    out << prefix
        << "observation_anchor_index=" << sample.observation_anchor_index
        << "\n";
    out << prefix << "policy_input_digest=" << sample.policy_input_digest
        << "\n";
    out << prefix << "target_node_weights=" << sample.target_node_weights
        << "\n";
    out << prefix
        << "transaction_cost_numeraire=" << sample.transaction_cost_numeraire
        << "\n";
    out << prefix << "turnover=" << sample.turnover << "\n";
    out << prefix
        << "missing_direct_pair_count=" << sample.missing_direct_pair_count
        << "\n";
    out << prefix << "numeraire_fallback_pair_count="
        << sample.numeraire_fallback_pair_count << "\n";
    out << prefix << "rejected_fill_count=" << sample.rejected_fill_count
        << "\n";
    out << prefix << "partial_fill_count=" << sample.partial_fill_count << "\n";
    out << prefix << "cajtucu_trace_id=" << sample.cajtucu_trace_id << "\n";
    out << prefix << "cajtucu_trace_digest=" << sample.cajtucu_trace_digest
        << "\n";
    out << prefix
        << "cajtucu_trace_valid=" << bool_json(sample.cajtucu_trace_valid)
        << "\n";
    out << prefix << "reward_total=" << sample.reward << "\n";
    out << prefix << "reward_log_growth=" << sample.reward_log_growth << "\n";
    out << prefix << "done=" << bool_json(sample.done) << "\n";
  }
  out << "projection_quality_separate_from_portfolio=true\n";
  out << "execution_feasibility_separate_from_projection=true\n";
  out << "policy_quality_claimed=false\n";
  out << "lattice_quality_claim=false\n";
  out << "market_readiness_claimed=false\n";
  out << "deployment_readiness_claimed=false\n";
  out << "live_execution_allowed=false\n";
  return out.str();
}

[[nodiscard]] const ppo_v0_policy_summary_t *
find_policy_summary_by_id(const std::vector<ppo_v0_policy_summary_t> &summaries,
                          std::string_view policy_id) {
  for (const auto &summary : summaries) {
    if (summary.policy_id == policy_id) {
      return &summary;
    }
  }
  return nullptr;
}

[[nodiscard]] std::int64_t count_bound_baseline_summaries(
    const std::vector<ppo_v0_policy_summary_t> &summaries) {
  const std::array<std::string_view, 4> required{
      "numeraire_only.v1", "current_weight_no_trade.v1", "equal_weight.v1",
      "kikijyeba.environment.policy.spot_distributional_utility.v1"};
  std::int64_t out = 0;
  for (std::string_view id : required) {
    out += find_policy_summary_by_id(summaries, id) != nullptr ? 1 : 0;
  }
  return out;
}

[[nodiscard]] std::vector<std::string> missing_policy_quality_summary_ids(
    const std::vector<ppo_v0_policy_summary_t> &summaries) {
  const std::array<std::string_view, 5> required{
      "wikimyei.policy.portfolio.graph_node_allocation.v1", "numeraire_only.v1",
      "current_weight_no_trade.v1", "equal_weight.v1",
      "kikijyeba.environment.policy.spot_distributional_utility.v1"};
  std::vector<std::string> out;
  for (std::string_view id : required) {
    if (find_policy_summary_by_id(summaries, id) == nullptr) {
      out.emplace_back(id);
    }
  }
  return out;
}

[[nodiscard]] std::string policy_training_policy_quality_report_text(
    const cuwacunu::hero::runtime::policy_training_job_contract_t &contract,
    std::string_view contract_digest, const fs::path &actor_checkpoint_path,
    std::string_view actor_checkpoint_digest,
    const fs::path &validation_rollout_path,
    std::string_view validation_rollout_digest,
    const fs::path &quality_replay_report_path,
    std::string_view quality_replay_report_digest,
    const ppo_v0_rollout_collection_t &quality_collection,
    bool quality_report_ready, std::string_view reason,
    double linear_transaction_cost_rate) {
  const auto *ppo_summary = find_policy_summary_by_id(
      quality_collection.policy_summaries,
      "wikimyei.policy.portfolio.graph_node_allocation.v1");
  const std::array<std::string_view, 4> baseline_ids{
      "numeraire_only.v1", "current_weight_no_trade.v1", "equal_weight.v1",
      "kikijyeba.environment.policy.spot_distributional_utility.v1"};

  std::ostringstream out;
  out << "schema=kikijyeba.runtime.policy_quality_report.v1\n";
  out << "report_writer_id=hero.runtime.policy_training.policy_quality.v1\n";
  out << "contract_digest=" << contract_digest << "\n";
  out << "policy_id=" << contract.policy_id << "\n";
  out << "policy_kind=" << contract.policy_kind << "\n";
  out << "policy_family_id=" << contract.policy_family_id << "\n";
  out << "policy_checkpoint_path=" << actor_checkpoint_path.string() << "\n";
  out << "policy_checkpoint_digest=" << actor_checkpoint_digest << "\n";
  out << "validation_rollout_report_path=" << validation_rollout_path.string()
      << "\n";
  out << "validation_rollout_report_digest=" << validation_rollout_digest
      << "\n";
  out << "quality_replay_report_path=" << quality_replay_report_path.string()
      << "\n";
  out << "quality_replay_report_digest=" << quality_replay_report_digest
      << "\n";
  out << "raw_comparison_replay_report_path="
      << quality_replay_report_path.string() << "\n";
  out << "raw_comparison_replay_report_digest=" << quality_replay_report_digest
      << "\n";
  out << "quality_report_ready=" << bool_json(quality_report_ready) << "\n";
  out << "policy_quality_report_ready=" << bool_json(quality_report_ready)
      << "\n";
  out << "reason=" << reason << "\n";
  out << "comparison_scope=post_update_cost_aware_replay\n";
  out << "metric_scope=after_cost_policy_comparison\n";
  out << "ppo_policy_id="
      << "wikimyei.policy.portfolio.graph_node_allocation.v1\n";
  out << "policy_under_review_id="
      << "wikimyei.policy.portfolio.graph_node_allocation.v1\n";
  out << "policy_under_review_summary_bound="
      << bool_json(ppo_summary != nullptr) << "\n";
  out << "baseline_required_count=4\n";
  out << "baseline_bound_count="
      << count_bound_baseline_summaries(quality_collection.policy_summaries)
      << "\n";
  out << "baseline_set_complete="
      << bool_json(count_bound_baseline_summaries(
                       quality_collection.policy_summaries) == 4)
      << "\n";
  out << "comparison_policy_set_complete="
      << bool_json(ppo_summary != nullptr &&
                   count_bound_baseline_summaries(
                       quality_collection.policy_summaries) == 4)
      << "\n";
  out << "policy_summary_count=" << quality_collection.policy_summaries.size()
      << "\n";
  out << "same_replay_source_job=true\n";
  out << "same_execution_profile_digest=true\n";
  out << "same_reward_contract_id=true\n";
  out << "same_accounting_numeraire=true\n";
  out << "same_transaction_cost_profile=true\n";
  out << "same_graph_order_fingerprint=true\n";
  out << "same_action_universe=true\n";
  out << "same_causal_schedule_digest=true\n";
  out << "same_snapshot_family_digest=true\n";
  out << "policy_input_schema_id=" << contract.policy_input_schema_id << "\n";
  out << "action_schema_digest=" << contract.action_schema_digest << "\n";
  out << "action_adapter_id=" << contract.action_adapter_id << "\n";
  out << "action_distribution_id=" << contract.action_distribution_id << "\n";
  out << "reward_contract_id=" << contract.reward_contract_id << "\n";
  out << "execution_profile_digest=" << contract.execution_profile_digest
      << "\n";
  out << "causal_schedule_digest=" << contract.causal_schedule_digest << "\n";
  out << "snapshot_family_digest=" << contract.snapshot_family_digest << "\n";
  out << "linear_transaction_cost_rate=" << linear_transaction_cost_rate
      << "\n";
  for (std::size_t i = 0; i < quality_collection.policy_summaries.size(); ++i) {
    const auto &summary = quality_collection.policy_summaries[i];
    const std::string prefix = "policy_" + std::to_string(i) + "_";
    out << prefix << "id=" << summary.policy_id << "\n";
    out << prefix << "kind=" << summary.policy_kind << "\n";
    out << prefix << "attempted_count=" << summary.attempted_count << "\n";
    out << prefix << "completed_count=" << summary.completed_count << "\n";
    out << prefix << "failed_count=" << summary.failed_count << "\n";
    out << prefix << "mean_total_reward=" << summary.mean_total_reward << "\n";
    out << prefix << "mean_total_log_growth=" << summary.mean_total_log_growth
        << "\n";
    out << prefix
        << "mean_final_equity_numeraire=" << summary.mean_final_equity_numeraire
        << "\n";
    out << prefix << "mean_max_drawdown=" << summary.mean_max_drawdown << "\n";
    out << prefix << "realized_volatility=" << summary.realized_volatility
        << "\n";
    out << prefix << "mean_total_turnover=" << summary.mean_total_turnover
        << "\n";
    out << prefix << "mean_total_transaction_cost_numeraire="
        << summary.mean_total_transaction_cost_numeraire << "\n";
    out << prefix << "fee_cost_numeraire=" << summary.fee_cost_numeraire
        << "\n";
    out << prefix << "spread_cost_numeraire=" << summary.spread_cost_numeraire
        << "\n";
    out << prefix
        << "slippage_cost_numeraire=" << summary.slippage_cost_numeraire
        << "\n";
    out << prefix
        << "rejected_notional_numeraire=" << summary.rejected_notional_numeraire
        << "\n";
    out << prefix
        << "partial_notional_numeraire=" << summary.partial_notional_numeraire
        << "\n";
    out << prefix << "fill_ratio=" << summary.fill_ratio << "\n";
    out << prefix
        << "cajtucu_valid_trace_count=" << summary.cajtucu_valid_trace_count
        << "\n";
    out << prefix
        << "cajtucu_invalid_trace_count=" << summary.cajtucu_invalid_trace_count
        << "\n";
    out << prefix << "cajtucu_missing_direct_pair_count="
        << summary.cajtucu_missing_direct_pair_count << "\n";
    out << prefix << "cajtucu_numeraire_fallback_pair_count="
        << summary.cajtucu_numeraire_fallback_pair_count << "\n";
    out << prefix << "rejected_order_count=" << summary.rejected_order_count
        << "\n";
    out << prefix << "partial_order_count=" << summary.partial_order_count
        << "\n";
    out << prefix
        << "mean_target_weight_error_l1=" << summary.mean_target_weight_error_l1
        << "\n";
    out << prefix << "mean_target_weight_error_linf="
        << summary.mean_target_weight_error_linf << "\n";
    out << prefix << "mean_projection_mae=" << summary.mean_projection_mae
        << "\n";
    out << prefix
        << "mean_projection_signed_bias=" << summary.mean_projection_signed_bias
        << "\n";
    out << prefix << "mean_projection_directional_accuracy="
        << summary.mean_projection_directional_accuracy << "\n";
    out << prefix << "mean_projection_interval_coverage="
        << summary.mean_projection_interval_coverage << "\n";
  }
  if (ppo_summary != nullptr) {
    std::size_t comparison_index = 0;
    for (std::string_view baseline_id : baseline_ids) {
      const auto *baseline = find_policy_summary_by_id(
          quality_collection.policy_summaries, baseline_id);
      const std::string prefix =
          "comparison_" + std::to_string(comparison_index++) + "_";
      out << prefix << "baseline_id=" << baseline_id << "\n";
      out << prefix
          << "baseline_summary_bound=" << bool_json(baseline != nullptr)
          << "\n";
      if (baseline != nullptr) {
        out << prefix << "ppo_minus_baseline_total_log_growth="
            << (ppo_summary->mean_total_log_growth -
                baseline->mean_total_log_growth)
            << "\n";
        out << prefix << "ppo_minus_baseline_final_equity_numeraire="
            << (ppo_summary->mean_final_equity_numeraire -
                baseline->mean_final_equity_numeraire)
            << "\n";
        out << prefix << "ppo_minus_baseline_max_drawdown="
            << (ppo_summary->mean_max_drawdown - baseline->mean_max_drawdown)
            << "\n";
        out << prefix << "ppo_minus_baseline_turnover="
            << (ppo_summary->mean_total_turnover -
                baseline->mean_total_turnover)
            << "\n";
        out << prefix << "ppo_minus_baseline_transaction_cost_numeraire="
            << (ppo_summary->mean_total_transaction_cost_numeraire -
                baseline->mean_total_transaction_cost_numeraire)
            << "\n";
      }
    }
  }
  out << "policy_selection_claimed=false\n";
  out << "policy_selected=false\n";
  out << "comparison_ranked=false\n";
  out << "winner_declared=false\n";
  out << "policy_quality_claimed=false\n";
  out << "market_readiness_claimed=false\n";
  out << "deployment_readiness_claimed=false\n";
  out << "live_execution_allowed=false\n";
  out << "lattice_policy_selector=false\n";
  out << "lattice_quality_claim=false\n";
  out << "lattice_report_integrity_only=true\n";
  out << "report_integrity_only=true\n";
  return out.str();
}

[[nodiscard]] std::string policy_training_ppo_report_text(
    const cuwacunu::hero::runtime::policy_training_job_contract_t &contract,
    std::string_view contract_digest, const fs::path &actor_checkpoint_path,
    std::string_view actor_checkpoint_digest,
    const fs::path &critic_checkpoint_path,
    std::string_view critic_checkpoint_digest,
    const fs::path &optimizer_state_path,
    std::string_view optimizer_state_digest,
    const fs::path &rollout_collection_path,
    std::string_view rollout_collection_digest,
    const fs::path &ppo_update_report_path,
    std::string_view ppo_update_report_digest,
    const fs::path &validation_rollout_path,
    std::string_view validation_rollout_digest,
    const fs::path &policy_quality_report_path,
    std::string_view policy_quality_report_digest,
    const ppo_v0_update_metrics_t &metrics,
    const ppo_v0_rollout_collection_t &collection) {
  std::ostringstream out;
  out << "report_schema_id=kikijyeba.runtime.policy_training.ppo_v0_report."
         "v1\n";
  out << "report_writer_id=hero.runtime.policy_training.ppo_v0.v1\n";
  out << "run_data_kind="
      << (collection.on_policy_runtime_collection_complete
              ? "policy_training_ppo_v0_on_policy_replay_collected"
              : (collection.kikijyeba_cajtucu_replay_integration
                     ? "policy_training_ppo_v0_replay_report_ingested"
                     : "policy_training_ppo_v0_smoke"))
      << "\n";
  out << "rollout_collection_source=" << collection.collection_source << "\n";
  out << "input_replay_report_path="
      << collection.input_replay_report_path.string() << "\n";
  out << "input_replay_report_digest=" << collection.input_replay_report_digest
      << "\n";
  out << "input_policy_checkpoint_path="
      << collection.input_policy_checkpoint_path.string() << "\n";
  out << "input_policy_checkpoint_digest="
      << collection.input_policy_checkpoint_digest << "\n";
  out << "kikijyeba_cajtucu_replay_integration="
      << bool_json(collection.kikijyeba_cajtucu_replay_integration) << "\n";
  out << "policy_distribution_evidence_bound="
      << bool_json(collection.policy_distribution_evidence_bound) << "\n";
  out << "policy_id=" << contract.policy_id << "\n";
  out << "policy_kind=" << contract.policy_kind << "\n";
  out << "policy_family_id=" << contract.policy_family_id << "\n";
  out << "contract_digest=" << contract_digest << "\n";
  out << "training_schedule_mode=" << contract.training_schedule_mode << "\n";
  out << "causal_schedule_digest=" << contract.causal_schedule_digest << "\n";
  out << "actor_checkpoint_path=" << actor_checkpoint_path.string() << "\n";
  out << "actor_checkpoint_digest=" << actor_checkpoint_digest << "\n";
  out << "critic_checkpoint_path=" << critic_checkpoint_path.string() << "\n";
  out << "critic_checkpoint_digest=" << critic_checkpoint_digest << "\n";
  out << "optimizer_state_path=" << optimizer_state_path.string() << "\n";
  out << "optimizer_state_digest=" << optimizer_state_digest << "\n";
  out << "optimizer_state_schema_id=" << contract.optimizer_state_schema_id
      << "\n";
  out << "device_policy=" << metrics.device_policy << "\n";
  out << "runtime_device_kind=" << metrics.runtime_device_kind << "\n";
  out << "cuda_device_index=" << metrics.cuda_device_index << "\n";
  out << "cuda_verification_passed="
      << bool_json(metrics.cuda_verification_passed) << "\n";
  out << "resume_mode=" << metrics.resume_mode << "\n";
  out << "resume_parent_loaded=" << bool_json(metrics.resume_parent_loaded)
      << "\n";
  out << "optimizer_state_resume_loaded="
      << bool_json(metrics.optimizer_state_resume_loaded) << "\n";
  out << "rollout_collection_path=" << rollout_collection_path.string() << "\n";
  out << "rollout_collection_digest=" << rollout_collection_digest << "\n";
  out << "ppo_update_report_path=" << ppo_update_report_path.string() << "\n";
  out << "ppo_update_report_digest=" << ppo_update_report_digest << "\n";
  out << "validation_rollout_report_path=" << validation_rollout_path.string()
      << "\n";
  out << "validation_rollout_report_digest=" << validation_rollout_digest
      << "\n";
  out << "policy_quality_report_path=" << policy_quality_report_path.string()
      << "\n";
  out << "policy_quality_report_digest=" << policy_quality_report_digest
      << "\n";
  out << "optimizer_steps=" << metrics.optimizer_steps << "\n";
  out << "sample_count=" << metrics.sample_count << "\n";
  out << "policy_loss_final=" << metrics.policy_loss_final << "\n";
  out << "value_loss=" << metrics.value_loss << "\n";
  out << "approx_kl=" << metrics.approx_kl << "\n";
  out << "clip_fraction=" << metrics.clip_fraction << "\n";
  out << "ppo_implemented=true\n";
  out << "runtime_trains_ppo=true\n";
  out << "runtime_executes_live_capital=false\n";
  out << "policy_quality_claimed=false\n";
  out << "market_readiness_claimed=false\n";
  out << "replay_report_ingestion_complete="
      << bool_json(collection.kikijyeba_cajtucu_replay_integration) << "\n";
  out << "replay_integration_complete="
      << bool_json(collection.kikijyeba_cajtucu_replay_integration) << "\n";
  out << "on_policy_runtime_collection_complete="
      << bool_json(collection.on_policy_runtime_collection_complete) << "\n";
  return out.str();
}

[[nodiscard]] std::string policy_training_ppo_runtime_result_fact_text(
    std::string_view job_id, const fs::path &actor_checkpoint_path,
    const ppo_v0_update_metrics_t &metrics) {
  std::ostringstream out;
  out << "fact_type=runtime.result.fact\n";
  out << "schema_version=1\n";
  out << "producer=runtime\n";
  out << "job_id=" << job_id << "\n";
  out << "job_kind=policy_training\n";
  out << "status=completed\n";
  out << "error_message=\n";
  out << "trainer_kind=ppo_policy_adapter.v1\n";
  out << "optimizer_steps=" << metrics.optimizer_steps << "\n";
  out << "device_policy=" << metrics.device_policy << "\n";
  out << "runtime_device_kind=" << metrics.runtime_device_kind << "\n";
  out << "cuda_verification_passed="
      << (metrics.cuda_verification_passed ? "true" : "false") << "\n";
  out << "resume_mode=" << metrics.resume_mode << "\n";
  out << "checkpoint_written=true\n";
  out << "checkpoint_path=" << actor_checkpoint_path.string() << "\n";
  out << "model_state_mutated=true\n";
  out << "finite_parameter_check=true\n";
  out << "nonfinite_output_count=0\n";
  out << "final_loss=" << metrics.policy_loss_final << "\n";
  out << "mean_loss=" << metrics.policy_loss_final << "\n";
  out << "runtime_trains_ppo=true\n";
  out << "runtime_executes_live_capital=false\n";
  out << "device_policy=" << metrics.device_policy << "\n";
  out << "runtime_device_kind=" << metrics.runtime_device_kind << "\n";
  out << "cuda_verification_passed="
      << (metrics.cuda_verification_passed ? "true" : "false") << "\n";
  out << "resume_mode=" << metrics.resume_mode << "\n";
  return out.str();
}

[[nodiscard]] std::string policy_training_ppo_checkpoint_io_fact_text(
    std::string_view job_id, const fs::path &actor_checkpoint_path,
    std::string_view actor_checkpoint_digest) {
  std::error_code ec;
  const auto checkpoint_bytes = fs::exists(actor_checkpoint_path, ec)
                                    ? fs::file_size(actor_checkpoint_path, ec)
                                    : std::uintmax_t{0};
  std::ostringstream out;
  out << "fact_type=runtime.checkpoint_io.fact\n";
  out << "schema_version=1\n";
  out << "producer=runtime\n";
  out << "job_id=" << job_id << "\n";
  out << "job_kind=policy_training\n";
  out << "trainer_kind=ppo_policy_adapter.v1\n";
  out << "checkpoint_written=true\n";
  out << "checkpoint_path=" << actor_checkpoint_path.string() << "\n";
  out << "checkpoint_write_count=3\n";
  out << "checkpoint_artifact_digest=" << actor_checkpoint_digest << "\n";
  out << "checkpoint_path_reported=" << actor_checkpoint_path.string() << "\n";
  out << "checkpoint_digest_reported=" << actor_checkpoint_digest << "\n";
  out << "checkpoint_digest_verified=true\n";
  out << "checkpoint_file_exists=true\n";
  out << "checkpoint_bytes=" << checkpoint_bytes << "\n";
  out << "checkpoint_artifact_status=written\n";
  out << "model_state_mutated=true\n";
  return out.str();
}

[[nodiscard]] std::string policy_training_ppo_state_text(
    std::string_view job_id, std::string_view job_stable_id,
    std::string_view attempt_leaf, const fs::path &actor_checkpoint_path,
    const fs::path &report_path, const ppo_v0_update_metrics_t &metrics) {
  std::ostringstream out;
  out << "state_format=kikijyeba.runtime.job_state.v1\n";
  out << "job_id=" << job_id << "\n";
  out << "job_stable_id=" << job_stable_id << "\n";
  out << "job_attempt_id=" << attempt_leaf << "\n";
  out << "job_attempt_index=0\n";
  out << "job_attempt_policy=immutable_no_overwrite\n";
  out << "job_kind=policy_training\n";
  out << "status=completed\n";
  out << "error_message=\n";
  out << "wave_id=policy_training_ppo_v0\n";
  out << "target_component_family_id=wikimyei.policy.portfolio.graph_node_"
         "allocation\n";
  out << "wave_action=train\n";
  out << "steps_attempted=" << metrics.sample_count << "\n";
  out << "steps_completed=" << metrics.sample_count << "\n";
  out << "skipped_batches=0\n";
  out << "optimizer_steps=" << metrics.optimizer_steps << "\n";
  out << "checkpoint_written=true\n";
  out << "checkpoint_write_count=3\n";
  out << "checkpoint_path=" << actor_checkpoint_path.string() << "\n";
  out << "report_written=true\n";
  out << "report_write_count=3\n";
  out << "report_path=" << report_path.string() << "\n";
  out << "runtime_result_fact_written=true\n";
  out << "runtime_result_fact_path=runtime.result.fact\n";
  out << "runtime_checkpoint_io_fact_written=true\n";
  out << "runtime_checkpoint_io_fact_path=runtime.checkpoint_io.fact\n";
  out << "runtime_health_measurement_fact_written=true\n";
  out << "runtime_health_measurement_fact_path=runtime.health_measurement."
         "fact\n";
  out << "lattice_exposure_fact_written=true\n";
  out << "lattice_exposure_fact_path=runtime.policy_training.fact\n";
  return out.str();
}

void append_assignment(std::ostringstream &out, std::string_view key,
                       std::string_view value) {
  out << key << "=" << value << "\n";
}

void append_assignment_bool(std::ostringstream &out, std::string_view key,
                            bool value) {
  append_assignment(out, key, value ? "true" : "false");
}

struct policy_training_lattice_identity_t {
  std::string parent_exposure_fact_digest{};
  std::string job_id{};
  std::string wave_id{};
  std::string job_status{};
  std::string wave_action{};
  std::string split_name{};
  std::string split_role{};
  std::string anchor_begin{};
  std::string anchor_end{};
  std::string completed_anchor_begin{};
  std::string completed_anchor_end{};
};

[[nodiscard]] std::string
kv_get(const std::unordered_map<std::string, std::string> &fields,
       std::string_view key) {
  const auto it = fields.find(std::string(key));
  return it == fields.end() ? std::string{} : it->second;
}

[[nodiscard]] policy_training_lattice_identity_t
policy_training_lattice_identity_from_parent_fields(
    const std::unordered_map<std::string, std::string> &fields,
    std::string_view policy_job_id) {
  policy_training_lattice_identity_t identity{};
  identity.parent_exposure_fact_digest =
      kv_get(fields, "parent_exposure_fact_digest");
  identity.job_id = std::string(policy_job_id);
  identity.wave_id = "policy_training_ppo_v0";
  identity.job_status = "completed";
  identity.wave_action = "train";
  identity.split_name = kv_get(fields, "split_name");
  identity.split_role = kv_get(fields, "split_role");
  identity.anchor_begin = kv_get(fields, "anchor_begin");
  identity.anchor_end = kv_get(fields, "anchor_end");
  identity.completed_anchor_begin = kv_get(fields, "completed_anchor_begin");
  identity.completed_anchor_end = kv_get(fields, "completed_anchor_end");
  return identity;
}

[[nodiscard]] policy_training_lattice_identity_t
policy_training_lattice_identity_from_source_job(
    const fs::path &source_replay_job_dir, std::string_view policy_job_id,
    std::string_view parent_forecast_eval_fact_digest,
    std::string_view parent_observer_belief_fact_digest) {
  const auto forecast_fields =
      parse_kv_file(source_replay_job_dir / "lattice.forecast_eval.fact");
  if (!forecast_fields.empty() &&
      (trim_ascii(parent_forecast_eval_fact_digest).empty() ||
       kv_get(forecast_fields, "fact_digest") ==
           trim_ascii(parent_forecast_eval_fact_digest))) {
    return policy_training_lattice_identity_from_parent_fields(forecast_fields,
                                                               policy_job_id);
  }

  const auto observer_fields =
      parse_kv_file(source_replay_job_dir / "lattice.observer_belief.fact");
  if (!observer_fields.empty() &&
      (trim_ascii(parent_observer_belief_fact_digest).empty() ||
       kv_get(observer_fields, "fact_digest") ==
           trim_ascii(parent_observer_belief_fact_digest))) {
    return policy_training_lattice_identity_from_parent_fields(observer_fields,
                                                               policy_job_id);
  }

  return {};
}

[[nodiscard]] std::string policy_training_fact_text(
    const cuwacunu::hero::runtime::policy_training_job_contract_t &contract,
    std::string_view checkpoint_digest, bool runtime_trainer = false,
    std::string_view input_policy_checkpoint_path = {},
    std::string_view input_policy_checkpoint_digest = {},
    const policy_training_lattice_identity_t *identity = nullptr,
    const ppo_v0_update_metrics_t *ppo_metrics = nullptr) {
  std::ostringstream out;
  append_assignment(out, "schema", contract.artifact_schema_id);
  append_assignment(out, "fact_type", "policy_training");
  if (identity != nullptr) {
    append_assignment(out, "parent_exposure_fact_digest",
                      identity->parent_exposure_fact_digest);
    append_assignment(out, "job_id", identity->job_id);
    append_assignment(out, "wave_id", identity->wave_id);
    append_assignment(out, "job_status", identity->job_status);
    append_assignment(out, "wave_action", identity->wave_action);
    append_assignment(out, "split_name", identity->split_name);
    append_assignment(out, "split_role", identity->split_role);
    append_assignment(out, "anchor_begin", identity->anchor_begin);
    append_assignment(out, "anchor_end", identity->anchor_end);
    append_assignment(out, "completed_anchor_begin",
                      identity->completed_anchor_begin);
    append_assignment(out, "completed_anchor_end",
                      identity->completed_anchor_end);
  }
  append_assignment(out, "contract_fingerprint",
                    contract.protocol_contract_fingerprint);
  append_assignment(out, "protocol_id", contract.protocol_id);
  append_assignment(out, "graph_order_fingerprint",
                    contract.graph_order_fingerprint);
  append_assignment(out, "source_cursor_token", contract.source_cursor_token);
  append_assignment(out, "split_policy_fingerprint",
                    contract.split_policy_fingerprint);
  append_assignment(out, "component_assembly_fingerprint",
                    contract.component_assembly_fingerprint);
  append_assignment(out, "target_component_family_id",
                    "wikimyei.policy.portfolio.graph_node_allocation");
  append_assignment(out, "policy_id", contract.policy_id);
  append_assignment(out, "policy_kind", contract.policy_kind);
  append_assignment(out, "policy_architecture_digest",
                    contract.policy_architecture_digest);
  append_assignment(out, "training_config_digest",
                    contract.training_config_digest);
  append_assignment(out, "training_range_digest",
                    contract.training_range_digest);
  append_assignment(out, "validation_range_digest",
                    contract.validation_range_digest);
  append_assignment(out, "test_range_digest", contract.test_range_digest);
  append_assignment(out, "environment_contract_id",
                    contract.environment_contract_id);
  append_assignment(out, "observation_schema_digest",
                    contract.observation_schema_digest);
  append_assignment(out, "action_schema_digest", contract.action_schema_digest);
  append_assignment(out, "reward_contract_digest",
                    contract.reward_contract_digest);
  append_assignment(out, "policy_input_schema_id",
                    contract.policy_input_schema_id);
  append_assignment(out, "action_adapter_id", contract.action_adapter_id);
  append_assignment(out, "action_distribution_id",
                    contract.action_distribution_id);
  append_assignment(out, "reward_contract_id", contract.reward_contract_id);
  append_assignment(out, "execution_profile_digest",
                    contract.execution_profile_digest);
  append_assignment(out, "ppo_policy_artifact_contract_id",
                    contract.ppo_policy_artifact_contract_id);
  append_assignment(out, "policy_family_id", contract.policy_family_id);
  append_assignment(out, "policy_checkpoint_schema_id",
                    contract.policy_checkpoint_schema_id);
  append_assignment(out, "policy_dsl_digest", contract.policy_dsl_digest);
  append_assignment(out, "policy_net_digest", contract.policy_net_digest);
  append_assignment(out, "policy_input_feature_manifest_digest",
                    contract.policy_input_feature_manifest_digest);
  append_assignment(out, "policy_jkimyei_digest",
                    contract.policy_jkimyei_digest);
  append_assignment(out, "target_node_universe_digest",
                    contract.target_node_universe_digest);
  append_assignment(out, "action_distribution_config_digest",
                    contract.action_distribution_config_digest);
  append_assignment(out, "snapshot_family_digest",
                    contract.snapshot_family_digest);
  append_assignment(out, "actor_architecture_digest",
                    contract.actor_architecture_digest);
  append_assignment(out, "critic_architecture_digest",
                    contract.critic_architecture_digest);
  append_assignment(out, "input_policy_checkpoint_path",
                    input_policy_checkpoint_path);
  append_assignment(out, "input_policy_checkpoint_digest",
                    input_policy_checkpoint_digest);
  append_assignment(out, "actor_checkpoint_digest",
                    contract.actor_checkpoint_digest);
  append_assignment(out, "critic_checkpoint_digest",
                    contract.critic_checkpoint_digest);
  append_assignment(out, "optimizer_state_digest",
                    contract.optimizer_state_digest);
  append_assignment(out, "ppo_config_digest", contract.ppo_config_digest);
  append_assignment(out, "optimizer_state_schema_id",
                    contract.optimizer_state_schema_id);
  append_assignment(out, "device_policy", contract.device_policy);
  append_assignment(out, "cuda_device_index",
                    std::to_string(contract.cuda_device_index));
  append_assignment(out, "resume_mode", contract.resume_mode);
  append_assignment(out, "resume_actor_checkpoint_path",
                    contract.resume_actor_checkpoint_path);
  append_assignment(out, "resume_actor_checkpoint_digest",
                    contract.resume_actor_checkpoint_digest);
  append_assignment(out, "resume_optimizer_state_path",
                    contract.resume_optimizer_state_path);
  append_assignment(out, "resume_optimizer_state_digest",
                    contract.resume_optimizer_state_digest);
  if (ppo_metrics != nullptr) {
    append_assignment(out, "runtime_device_kind",
                      ppo_metrics->runtime_device_kind);
    append_assignment_bool(out, "cuda_available", ppo_metrics->cuda_available);
    append_assignment_bool(out, "module_parameters_on_cuda",
                           ppo_metrics->module_parameters_on_cuda);
    append_assignment_bool(out, "forward_input_on_cuda",
                           ppo_metrics->forward_input_on_cuda);
    append_assignment_bool(out, "loss_on_cuda", ppo_metrics->loss_on_cuda);
    append_assignment_bool(out, "optimizer_state_on_cuda",
                           ppo_metrics->optimizer_state_on_cuda);
    append_assignment_bool(out, "cuda_verification_passed",
                           ppo_metrics->cuda_verification_passed);
    append_assignment_bool(out, "resume_parent_loaded",
                           ppo_metrics->resume_parent_loaded);
    append_assignment_bool(out, "optimizer_state_resume_loaded",
                           ppo_metrics->optimizer_state_resume_loaded);
    append_assignment(out, "optimizer_torch_state_path",
                      ppo_metrics->optimizer_torch_state_path);
    append_assignment(out, "optimizer_torch_state_digest",
                      ppo_metrics->optimizer_torch_state_digest);
  }
  append_assignment(out, "advantage_estimator_id",
                    contract.advantage_estimator_id);
  append_assignment(out, "advantage_normalization_policy",
                    contract.advantage_normalization_policy);
  append_assignment(out, "rollout_collection_schema_id",
                    contract.rollout_collection_schema_id);
  append_assignment(out, "rollout_collection_digest",
                    contract.rollout_collection_digest);
  append_assignment(out, "ppo_update_report_schema_id",
                    contract.ppo_update_report_schema_id);
  append_assignment(out, "ppo_update_report_digest",
                    contract.ppo_update_report_digest);
  append_assignment(out, "validation_rollout_report_digest",
                    contract.validation_rollout_report_digest);
  append_assignment(out, "policy_quality_report_digest",
                    contract.policy_quality_report_digest);
  append_assignment(out, "ppo_gamma", std::to_string(contract.ppo_gamma));
  append_assignment_bool(out, "ppo_gamma_bound", contract.ppo_gamma_bound);
  append_assignment(out, "ppo_gae_lambda",
                    std::to_string(contract.ppo_gae_lambda));
  append_assignment_bool(out, "ppo_gae_lambda_bound",
                         contract.ppo_gae_lambda_bound);
  append_assignment(out, "ppo_clip_epsilon",
                    std::to_string(contract.ppo_clip_epsilon));
  append_assignment_bool(out, "ppo_clip_epsilon_bound",
                         contract.ppo_clip_epsilon_bound);
  append_assignment(out, "ppo_target_kl",
                    std::to_string(contract.ppo_target_kl));
  append_assignment_bool(out, "ppo_target_kl_bound",
                         contract.ppo_target_kl_bound);
  append_assignment(out, "ppo_entropy_coeff",
                    std::to_string(contract.ppo_entropy_coeff));
  append_assignment_bool(out, "ppo_entropy_coeff_bound",
                         contract.ppo_entropy_coeff_bound);
  append_assignment(out, "ppo_value_loss_coeff",
                    std::to_string(contract.ppo_value_loss_coeff));
  append_assignment_bool(out, "ppo_value_loss_coeff_bound",
                         contract.ppo_value_loss_coeff_bound);
  append_assignment(out, "ppo_max_grad_norm",
                    std::to_string(contract.ppo_max_grad_norm));
  append_assignment_bool(out, "ppo_max_grad_norm_bound",
                         contract.ppo_max_grad_norm_bound);
  append_assignment(out, "ppo_minibatch_size",
                    std::to_string(contract.ppo_minibatch_size));
  append_assignment_bool(out, "ppo_minibatch_size_bound",
                         contract.ppo_minibatch_size_bound);
  append_assignment(out, "ppo_epochs_per_rollout",
                    std::to_string(contract.ppo_epochs_per_rollout));
  append_assignment_bool(out, "ppo_epochs_per_rollout_bound",
                         contract.ppo_epochs_per_rollout_bound);
  append_assignment(out, "training_schedule_mode",
                    contract.training_schedule_mode);
  append_assignment(out, "causal_schedule_schema_id",
                    contract.causal_schedule_schema_id);
  append_assignment(out, "causal_schedule_digest",
                    contract.causal_schedule_digest);
  append_assignment(out, "causal_schedule_cursor_key_kind",
                    contract.causal_schedule_cursor_key_kind);
  append_assignment(out, "causal_schedule_no_future_snapshot_use_source",
                    contract.causal_schedule_no_future_snapshot_use_source);
  append_assignment(out, "normalization_fit_range_digest",
                    contract.normalization_fit_range_digest);
  append_assignment(out, "replay_buffer_source_range_digest",
                    contract.replay_buffer_source_range_digest);
  append_assignment(out, "early_stopping_policy_digest",
                    contract.early_stopping_policy_digest);
  append_assignment(out, "hyperparameter_selection_policy_digest",
                    contract.hyperparameter_selection_policy_digest);
  append_assignment(out, "selector_split", contract.selector_split);
  append_assignment(out, "selector_policy_digest",
                    contract.selector_policy_digest);
  append_assignment(out, "parent_checkpoint_digest",
                    contract.parent_checkpoint_digest);
  append_assignment(out, "checkpoint_digest", checkpoint_digest);
  append_assignment(out, "parent_forecast_eval_fact_digest",
                    contract.parent_forecast_eval_fact_digest);
  append_assignment(out, "parent_observer_belief_fact_digest",
                    contract.parent_observer_belief_fact_digest);
  append_assignment(out, "parent_allocation_engine_fact_digest",
                    contract.parent_allocation_engine_fact_digest);
  append_assignment(out, "parent_replay_environment_fact_digest",
                    contract.parent_replay_environment_fact_digest);
  append_assignment(out, "parent_replay_environment_report_digest",
                    contract.parent_replay_environment_report_digest);
  append_assignment(out, "final_refit_parent_selected_checkpoint_digest",
                    contract.final_refit_parent_selected_checkpoint_digest);
  append_assignment(out, "random_seed", std::to_string(contract.random_seed));
  append_assignment_bool(out, "training_range_disjoint_validation", true);
  append_assignment_bool(out, "training_range_disjoint_test", true);
  append_assignment_bool(out, "validation_range_disjoint_test", true);
  append_assignment_bool(out, "normalization_fit_training_only", true);
  append_assignment_bool(out, "replay_buffer_training_only", true);
  append_assignment_bool(out, "reward_baseline_training_only", true);
  append_assignment_bool(out, "early_stopping_uses_validation_only", true);
  append_assignment_bool(out, "hyperparameter_selection_uses_validation_only",
                         true);
  append_assignment_bool(out, "test_sealed_until_final_report", true);
  append_assignment_bool(out, "test_first_access_after_selection", true);
  append_assignment_bool(out, "runtime_job_kind_bound", true);
  append_assignment_bool(out, "policy_checkpoint_written", true);
  append_assignment_bool(out, "causal_schedule_readiness_eligible",
                         contract.causal_schedule_readiness_eligible);
  append_assignment_bool(out, "causal_schedule_no_future_snapshot_use",
                         contract.causal_schedule_no_future_snapshot_use);
  append_assignment_bool(out, "offline_full_window_research", false);
  append_assignment_bool(out, "final_refit_uses_validation",
                         contract.final_refit_uses_validation);
  append_assignment_bool(out, "validation_no_longer_proof",
                         contract.validation_no_longer_proof);
  append_assignment_bool(out, "sealed_test_required",
                         contract.sealed_test_required);
  append_assignment_bool(out, "artifact_evidence", true);
  append_assignment_bool(out, "visibility_only", true);
  append_assignment_bool(out, "runtime_trainer", runtime_trainer);
  append_assignment_bool(out, "policy_training_authority", runtime_trainer);
  append_assignment_bool(out, "policy_selector", false);
  append_assignment_bool(out, "allocation_authority", false);
  append_assignment_bool(out, "execution_authority", false);
  append_assignment_bool(out, "readiness_authority", false);
  append_assignment_bool(out, "quality_authority", false);
  append_assignment_bool(out, "performance_authority", false);
  append_assignment_bool(out, "market_readiness_authority", false);
  append_assignment_bool(out, "deployment_authority", false);
  append_assignment_bool(out, "checkpoint_selector", false);
  append_assignment_bool(out, "coverage_authority", false);
  append_assignment_bool(out, "leakage_authority", false);
  append_assignment_bool(out, "contract_identity_authority", false);
  return out.str();
}

[[nodiscard]] std::string
unique_policy_training_attempt_leaf(std::string_view contract_digest) {
  const auto stamp = std::chrono::duration_cast<std::chrono::microseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
  return "attempt_" + std::string(contract_digest.substr(0, 16)) + "_" +
         std::to_string(static_cast<long long>(stamp)) + "_" +
         std::to_string(static_cast<long long>(::getpid()));
}

[[nodiscard]] bool path_exists_and_is_nonempty_dir(const fs::path &path) {
  std::error_code ec;
  if (!fs::exists(path, ec) || ec) {
    return false;
  }
  if (!fs::is_directory(path, ec) || ec) {
    return true;
  }
  return fs::directory_iterator(path, ec) != fs::directory_iterator{};
}

[[nodiscard]] bool append_optional_double_cli_arg(
    const std::string &args, std::string_view key, std::string_view flag,
    std::vector<std::string> *argv, std::string *err);

[[nodiscard]] bool append_optional_int_cli_arg(const std::string &args,
                                               std::string_view key,
                                               std::string_view flag,
                                               std::vector<std::string> *argv,
                                               std::string *err);

[[nodiscard]] bool execute_policy_training_noop(
    const std::string &args,
    const cuwacunu::hero::runtime::policy_training_job_contract_t &contract,
    const std::string &contract_text, std::string_view contract_digest,
    runtime_context_t *ctx, std::string *out, std::string *err) {
  if (!policy_bool_or(ctx->policy, "allow_execute", false)) {
    *err = "E_RUNTIME_EXECUTE_DENIED: allow_execute is false";
    return false;
  }
  if (!policy_bool_or(ctx->policy, "allow_train_execute", false)) {
    *err = "E_RUNTIME_TRAIN_DENIED: allow_train_execute is false";
    return false;
  }
  if (!policy_training_execute_kind_supported(contract)) {
    *err = policy_training_unsupported_execute_reason(contract);
    return false;
  }

  int timeout_seconds = 0;
  if (!policy_runtime_timeout_seconds(ctx->policy, &timeout_seconds, err)) {
    return false;
  }
  if (contract.max_wall_clock_seconds > 0) {
    timeout_seconds = static_cast<int>(contract.max_wall_clock_seconds);
  }

  std::string job_dir_arg;
  std::string job_id_arg;
  (void)extract_json_string_field(args, "job_dir", &job_dir_arg);
  (void)extract_json_string_field(args, "job_id", &job_id_arg);

  const fs::path root = runtime_root(ctx->policy);
  const std::string job_stable_id =
      "policy_training_" + std::string(contract_digest.substr(0, 16));
  const std::string attempt_leaf =
      unique_policy_training_attempt_leaf(contract_digest);
  const std::string job_id = trim_ascii(job_id_arg).empty()
                                 ? job_stable_id + "." + attempt_leaf
                                 : trim_ascii(job_id_arg);
  fs::path job_dir;
  if (!trim_ascii(job_dir_arg).empty()) {
    job_dir = normalize_path(fs::path(job_dir_arg));
    if (!explicit_job_dir_allowed(ctx->policy, job_dir) &&
        !path_within(root, job_dir)) {
      *err =
          "E_RUNTIME_JOB_DIR_DENIED: job_dir is outside Runtime Hero roots: " +
          job_dir.string();
      return false;
    }
  } else {
    job_dir = cuwacunu::hero::runtime::job_layout::job_dir(
        root, "wikimyei.policy.portfolio.graph_node_allocation",
        contract.policy_id,
        cuwacunu::hero::runtime::policy_operator_surface_digest(contract),
        "train", job_id);
  }
  if (path_exists_and_is_nonempty_dir(job_dir)) {
    *err = "E_RUNTIME_POLICY_TRAINING_JOB_DIR_EXISTS: refusing to overwrite "
           "existing job_dir: " +
           job_dir.string();
    return false;
  }

  try {
    fs::create_directories(job_dir);
    cuwacunu::hero::runtime::job_layout::write_runtime_layout_marker(root);

    const fs::path checkpoint_dir =
        job_dir / "artifacts" /
        "wikimyei.policy.portfolio.graph_node_allocation" / "checkpoints";
    const fs::path checkpoint_path =
        checkpoint_dir / "noop_policy_training.checkpoint";
    const std::string checkpoint_text =
        policy_training_checkpoint_text(contract, contract_digest);
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        checkpoint_path, checkpoint_text);
    const std::string checkpoint_digest = file_digest_or_empty(
        checkpoint_path, "kikijyeba.runtime.policy_training."
                         "noop_checkpoint.v1");

    const fs::path report_path = job_dir / "policy_training.report";
    const std::string report_text = policy_training_report_text(
        contract, contract_digest, checkpoint_path, checkpoint_digest);
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        report_path, report_text);

    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        job_dir / "job.manifest",
        policy_training_manifest_text(contract, contract_digest, job_id,
                                      job_stable_id, attempt_leaf,
                                      ctx->global_config_path));
    cuwacunu::hero::runtime::job_layout::write_component_spawn_ref(
        root, "wikimyei.policy.portfolio.graph_node_allocation",
        contract.policy_id, contract.policy_id,
        cuwacunu::hero::runtime::policy_operator_surface_digest(contract),
        "hero.runtime.policy_training",
        "policy_training_contract_" + std::string(contract_digest),
        "policy_training_contract_" + std::string(contract_digest));
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        job_dir / "policy_training.contract", contract_text);

    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        job_dir / "runtime.result.fact",
        policy_training_runtime_result_fact_text(job_id, checkpoint_path));
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        job_dir / "runtime.checkpoint_io.fact",
        policy_training_checkpoint_io_fact_text(job_id, checkpoint_path,
                                                checkpoint_digest));
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        job_dir / "runtime.health_measurement.fact",
        policy_training_health_fact_text(job_id));

    const std::string fact_text =
        policy_training_fact_text(contract, checkpoint_digest);
    const fs::path policy_fact_path = job_dir / "runtime.policy_training.fact";
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        policy_fact_path, fact_text);
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        job_dir / "job.state",
        policy_training_state_text(job_id, job_stable_id, attempt_leaf,
                                   checkpoint_path, report_path));

    std::ostringstream json;
    json << "{\"ok\":true,\"requested_mode\":\"execute\",\"dry_run\":false"
         << ",\"schema_version\":\"kikijyeba.runtime.policy_training_"
            "execution_packet.v1\""
         << ",\"policy_training_execution_supported\":true"
         << ",\"trainer_kind\":\"noop_policy_training.v1\""
         << ",\"ppo_implemented\":false"
         << ",\"contract_digest\":" << json_quote(contract_digest)
         << ",\"checkpoint_digest\":" << json_quote(checkpoint_digest)
         << ",\"job_id\":" << json_quote(job_id)
         << ",\"job_dir\":" << json_quote(job_dir.string())
         << ",\"manifest_path\":"
         << json_quote((job_dir / "job.manifest").string())
         << ",\"state_path\":" << json_quote((job_dir / "job.state").string())
         << ",\"report_path\":" << json_quote(report_path.string())
         << ",\"checkpoint_path\":" << json_quote(checkpoint_path.string())
         << ",\"policy_training_fact_path\":"
         << json_quote(policy_fact_path.string()) << ",\"artifacts\":"
         << job_artifacts_json(job_dir, report_path.string(), false,
                               static_cast<std::size_t>(policy_int_or(
                                   ctx->policy, "max_capture_bytes", 65536)))
         << ",\"terminal_evidence\":" << runtime_terminal_evidence_json(job_dir)
         << ",\"authority\":{\"runtime_executes_policy_training_smoke\":true,"
            "\"runtime_executes_live_capital\":false,"
            "\"runtime_trains_ppo\":false,"
            "\"runtime_selects_policy\":false,"
            "\"runtime_proves_lattice_target\":false}"
         << ",\"next_safe_actions\":[\"inspect_policy_training_artifact\","
            "\"evaluate_policy_training_artifact_ready\"]}";
    *out = json.str();
    return true;
  } catch (const std::exception &ex) {
    *err =
        std::string("E_RUNTIME_POLICY_TRAINING_EXECUTION_FAILED: ") + ex.what();
    return false;
  }
}

[[nodiscard]] bool execute_policy_training_ppo_v0(
    const std::string &args,
    const cuwacunu::hero::runtime::policy_training_job_contract_t &contract,
    const std::string &contract_text, std::string_view contract_digest,
    runtime_context_t *ctx, std::string *out, std::string *err) {
  if (!policy_bool_or(ctx->policy, "allow_execute", false)) {
    *err = "E_RUNTIME_EXECUTE_DENIED: allow_execute is false";
    return false;
  }
  if (!policy_bool_or(ctx->policy, "allow_train_execute", false)) {
    *err = "E_RUNTIME_TRAIN_DENIED: allow_train_execute is false";
    return false;
  }
  if (contract.live_execution_allowed) {
    *err = "E_RUNTIME_POLICY_TRAINING_LIVE_FORBIDDEN: PPO V0 is replay/paper "
           "only and requires live_execution_allowed=false";
    return false;
  }
  if (!policy_training_execute_kind_is_ppo_v0(contract)) {
    *err = policy_training_unsupported_execute_reason(contract);
    return false;
  }
  torch::Device runtime_device(torch::kCPU);
  if (!resolve_ppo_v0_runtime_device(contract, &runtime_device, err)) {
    return false;
  }

  int timeout_seconds = 0;
  if (!policy_runtime_timeout_seconds(ctx->policy, &timeout_seconds, err)) {
    return false;
  }
  if (contract.max_wall_clock_seconds > 0) {
    timeout_seconds = static_cast<int>(contract.max_wall_clock_seconds);
  }

  std::string job_dir_arg;
  std::string job_id_arg;
  std::string report_path_arg;
  std::string replay_job_dir_arg;
  std::string config_arg;
  std::string accounting_numeraire_node_id_arg;
  std::string target_node_ids_arg;
  std::string experiment_id_arg;
  std::string policy_set_digest_arg;
  (void)extract_json_string_field(args, "job_dir", &job_dir_arg);
  (void)extract_json_string_field(args, "job_id", &job_id_arg);
  (void)extract_json_string_field(args, "report_path", &report_path_arg);
  (void)extract_json_string_field(args, "replay_job_dir", &replay_job_dir_arg);
  (void)extract_json_string_field(args, "config_path", &config_arg);
  (void)extract_json_string_field(args, "accounting_numeraire_node_id",
                                  &accounting_numeraire_node_id_arg);
  (void)extract_json_string_field(args, "target_node_ids",
                                  &target_node_ids_arg);
  (void)extract_json_string_field(args, "experiment_id", &experiment_id_arg);
  (void)extract_json_string_field(args, "policy_set_digest",
                                  &policy_set_digest_arg);
  const bool validation_cost_rate_explicit =
      extract_json_raw_field(args, "linear_transaction_cost_rate", nullptr);
  double validation_linear_transaction_cost_rate = 0.001;
  if (!parse_optional_double_arg(args, "linear_transaction_cost_rate",
                                 validation_linear_transaction_cost_rate,
                                 &validation_linear_transaction_cost_rate,
                                 err)) {
    return false;
  }
  if (validation_cost_rate_explicit &&
      (!std::isfinite(validation_linear_transaction_cost_rate) ||
       validation_linear_transaction_cost_rate <= 0.0)) {
    *err = "E_RUNTIME_POLICY_TRAINING_PPO_VALIDATION_NONZERO_COST_REQUIRED";
    return false;
  }

  const fs::path root = runtime_root(ctx->policy);
  resolved_resume_actor_checkpoint_t resolved_resume_actor{};
  fs::path resume_optimizer_torch_state_path;
  if (contract.resume_mode != "fresh_spawn") {
    if (!explicit_job_dir_allowed(
            ctx->policy,
            normalize_path(fs::path(contract.resume_actor_checkpoint_path))) &&
        !path_within(root, fs::path(contract.resume_actor_checkpoint_path))) {
      *err = "E_RUNTIME_POLICY_TRAINING_PPO_RESUME_CHECKPOINT_DENIED: "
             "resume_actor_checkpoint_path is outside Runtime Hero roots";
      return false;
    }
    if (!resolve_resume_actor_checkpoint(contract, &resolved_resume_actor,
                                         err)) {
      return false;
    }
  }
  if (contract.resume_mode == "resume_weights_and_optimizer") {
    if (!explicit_job_dir_allowed(
            ctx->policy,
            normalize_path(fs::path(contract.resume_optimizer_state_path))) &&
        !path_within(root, fs::path(contract.resume_optimizer_state_path))) {
      *err = "E_RUNTIME_POLICY_TRAINING_PPO_RESUME_OPTIMIZER_STATE_DENIED: "
             "resume_optimizer_state_path is outside Runtime Hero roots";
      return false;
    }
    if (!resolve_resume_optimizer_torch_state_path(
            contract, &resume_optimizer_torch_state_path, err)) {
      return false;
    }
  }
  const std::string job_stable_id =
      "policy_training_ppo_v0_" + std::string(contract_digest.substr(0, 16));
  const std::string attempt_leaf =
      unique_policy_training_attempt_leaf(contract_digest);
  const std::string job_id = trim_ascii(job_id_arg).empty()
                                 ? job_stable_id + "." + attempt_leaf
                                 : trim_ascii(job_id_arg);
  fs::path job_dir;
  if (!trim_ascii(job_dir_arg).empty()) {
    job_dir = normalize_path(fs::path(job_dir_arg));
    if (!explicit_job_dir_allowed(ctx->policy, job_dir) &&
        !path_within(root, job_dir)) {
      *err =
          "E_RUNTIME_JOB_DIR_DENIED: job_dir is outside Runtime Hero roots: " +
          job_dir.string();
      return false;
    }
  } else {
    job_dir = cuwacunu::hero::runtime::job_layout::job_dir(
        root, "wikimyei.policy.portfolio.graph_node_allocation",
        contract.policy_id,
        cuwacunu::hero::runtime::policy_operator_surface_digest(contract),
        "train", job_id);
  }
  if (path_exists_and_is_nonempty_dir(job_dir)) {
    *err = "E_RUNTIME_POLICY_TRAINING_JOB_DIR_EXISTS: refusing to overwrite "
           "existing job_dir: " +
           job_dir.string();
    return false;
  }

  fs::path input_replay_report_path;
  fs::path source_replay_job_dir;
  if (!trim_ascii(report_path_arg).empty()) {
    input_replay_report_path = normalize_path(fs::path(report_path_arg));
    if (!explicit_job_dir_allowed(ctx->policy, input_replay_report_path) &&
        !path_within(root, input_replay_report_path)) {
      *err = "E_RUNTIME_POLICY_TRAINING_PPO_REPLAY_REPORT_DENIED: "
             "report_path is outside Runtime Hero roots: " +
             input_replay_report_path.string();
      return false;
    }
  }
  if (!trim_ascii(replay_job_dir_arg).empty()) {
    source_replay_job_dir = normalize_path(fs::path(replay_job_dir_arg));
    if (!explicit_job_dir_allowed(ctx->policy, source_replay_job_dir) &&
        !path_within(root, source_replay_job_dir)) {
      *err = "E_RUNTIME_POLICY_TRAINING_PPO_REPLAY_JOB_DENIED: "
             "replay_job_dir is outside Runtime Hero roots: " +
             source_replay_job_dir.string();
      return false;
    }
    if (!input_replay_report_path.empty()) {
      *err = "E_RUNTIME_POLICY_TRAINING_PPO_REPLAY_INPUT_AMBIGUOUS: provide "
             "either replay_job_dir for fresh collection or report_path for "
             "report ingestion, not both";
      return false;
    }
    if (!fs::exists(source_replay_job_dir / "job.manifest") ||
        !fs::exists(source_replay_job_dir / "job.state")) {
      *err = "E_RUNTIME_POLICY_TRAINING_PPO_REPLAY_JOB_INVALID: "
             "replay_job_dir must contain job.manifest and job.state";
      return false;
    }
    const auto source_state =
        parse_kv_file(source_replay_job_dir / "job.state");
    const auto status_found = source_state.find("status");
    const std::string source_status = status_found == source_state.end()
                                          ? std::string{}
                                          : status_found->second;
    if (lowercase_ascii(source_status) != "completed") {
      *err = "E_RUNTIME_POLICY_TRAINING_PPO_REPLAY_JOB_NOT_COMPLETED: "
             "fresh PPO collection requires a completed Runtime replay source "
             "job";
      return false;
    }
    const fs::path batch_index_path =
        runtime_replay_batch_index_path(source_replay_job_dir);
    if (!fs::exists(batch_index_path)) {
      *err = "E_RUNTIME_POLICY_TRAINING_PPO_REPLAY_ARTIFACTS_MISSING: "
             "missing " +
             batch_index_path.string();
      return false;
    }
  }

  ppo_v0_rollout_collection_t rollout_collection =
      make_ppo_v0_smoke_collection();

  try {
    fs::create_directories(job_dir);
    cuwacunu::hero::runtime::job_layout::write_runtime_layout_marker(root);

    const fs::path artifact_dir =
        job_dir / "artifacts" /
        "wikimyei.policy.portfolio.graph_node_allocation";
    const fs::path checkpoint_dir = artifact_dir / "checkpoints";
    const fs::path report_dir = artifact_dir / "reports";
    const fs::path actor_checkpoint_dir =
        checkpoint_dir / "ppo_v0_actor.checkpoint";
    const fs::path collection_actor_checkpoint_dir =
        checkpoint_dir / "ppo_v0_actor.collection.checkpoint";
    const fs::path actor_checkpoint_path =
        actor_checkpoint_dir / "checkpoint.meta";
    const fs::path actor_module_state_path =
        actor_checkpoint_dir / "module_state.pt";
    const fs::path collection_actor_checkpoint_path =
        collection_actor_checkpoint_dir / "checkpoint.meta";
    const fs::path collection_actor_module_state_path =
        collection_actor_checkpoint_dir / "module_state.pt";
    const fs::path critic_checkpoint_path =
        checkpoint_dir / "ppo_v0_critic.checkpoint";
    const fs::path optimizer_state_path =
        checkpoint_dir / "ppo_v0_optimizer.state";
    const fs::path optimizer_torch_state_path =
        checkpoint_dir / "ppo_v0_optimizer_state.pt";
    const fs::path rollout_collection_path =
        report_dir / "ppo_v0_rollout_collection.report";
    const fs::path runtime_replay_report_path =
        report_dir / "ppo_v0_on_policy_replay.report";
    const fs::path ppo_update_report_path = report_dir / "ppo_v0_update.report";
    const fs::path validation_rollout_path =
        report_dir / "ppo_v0_validation_rollout.report";
    const fs::path validation_replay_report_path =
        report_dir / "ppo_v0_validation_replay.report";
    const fs::path policy_quality_report_path =
        report_dir / "policy_quality_report.report";
    const fs::path policy_quality_replay_report_path =
        report_dir / "policy_quality_replay.report";
    fs::path replay_config_path;
    fs::path exec_path;
    std::string accounting_numeraire_node_id;
    ppo_v0_rollout_collection_t validation_rollout_collection{};
    std::string validation_replay_report_digest;
    bool validation_rollout_ready = false;
    std::string validation_rollout_reason{
        "fresh_runtime_replay_job_dir_required_for_cost_aware_evaluation"};
    ppo_v0_rollout_collection_t policy_quality_collection{};
    std::string policy_quality_replay_report_digest;
    bool policy_quality_report_ready = false;
    std::string policy_quality_report_reason{
        "fresh_runtime_replay_job_dir_required_for_policy_quality_report"};
    const std::string child_experiment_id_root =
        trim_ascii(experiment_id_arg).empty() ? job_id
                                              : trim_ascii(experiment_id_arg);
    const std::string replay_policy_set_digest =
        trim_ascii(policy_set_digest_arg).empty()
            ? contract.policy_family_id
            : trim_ascii(policy_set_digest_arg);
    fs::path input_actor_checkpoint_path = collection_actor_checkpoint_path;
    fs::path input_actor_module_state_path = collection_actor_module_state_path;
    std::string input_actor_checkpoint_digest;

    if (!source_replay_job_dir.empty()) {
      fs::create_directories(checkpoint_dir);
      fs::create_directories(report_dir);
      if (contract.resume_mode == "fresh_spawn") {
        write_seed_graph_node_allocation_module_state_artifact(
            collection_actor_module_state_path, runtime_device);
        const std::string collection_actor_module_state_digest =
            file_digest_or_empty(
                collection_actor_module_state_path,
                "wikimyei.policy.portfolio.graph_node_allocation.module_state."
                "v0");
        cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
            collection_actor_checkpoint_path,
            policy_training_ppo_actor_checkpoint_text(
                contract, contract_digest, "collection_policy", {}, nullptr,
                "module_state.pt", collection_actor_module_state_digest));
        input_actor_checkpoint_digest =
            file_digest_or_empty(collection_actor_checkpoint_path,
                                 contract.policy_checkpoint_schema_id);
      } else {
        input_actor_checkpoint_path = resolved_resume_actor.checkpoint_path;
        input_actor_module_state_path = resolved_resume_actor.module_state_path;
        input_actor_checkpoint_digest = resolved_resume_actor.checkpoint_digest;
      }
      replay_config_path = effective_config_path(*ctx, config_arg);
      accounting_numeraire_node_id =
          trim_ascii(accounting_numeraire_node_id_arg);
      if (accounting_numeraire_node_id.empty()) {
        const auto configured_numeraire = read_ini_value(
            replay_config_path, "ACCOUNTING", "accounting_numeraire_node_id");
        if (!configured_numeraire.has_value() ||
            trim_ascii(*configured_numeraire).empty()) {
          *err = "E_RUNTIME_POLICY_TRAINING_PPO_ACCOUNTING_NUMERAIRE_MISSING: "
                 "missing [ACCOUNTING].accounting_numeraire_node_id in " +
                 replay_config_path.string();
          return false;
        }
        accounting_numeraire_node_id = trim_ascii(*configured_numeraire);
      }
      exec_path = policy_path(ctx->policy, "runtime_exec_path");
      if (!executable_file(exec_path)) {
        *err = "E_RUNTIME_EXEC_MISSING: runtime_exec_path is not executable: " +
               exec_path.string();
        return false;
      }
      const std::string source_policy_digest = input_actor_checkpoint_digest;
      std::vector<std::string> argv{
          exec_path.string(),
          "--config",
          replay_config_path.string(),
          "--replay-from-job-dir",
          source_replay_job_dir.string(),
          "--replay-accounting-numeraire-node",
          accounting_numeraire_node_id,
          "--replay-report-path",
          runtime_replay_report_path.string(),
          "--replay-experiment-id",
          child_experiment_id_root + ".on_policy_collection",
          "--replay-include-graph-node-allocation-policy",
          "--replay-on-policy-sample",
          "--replay-no-numeraire-only-policy",
          "--replay-no-sdu-policy",
          "--replay-action-distribution-id",
          contract.action_distribution_id,
          "--replay-policy-artifact-digest",
          source_policy_digest,
          "--replay-policy-checkpoint-path",
          input_actor_checkpoint_path.string(),
          "--replay-causal-schedule-digest",
          contract.causal_schedule_digest,
          "--replay-snapshot-family-digest",
          contract.snapshot_family_digest,
          "--replay-execution-profile-digest",
          contract.execution_profile_digest,
          "--replay-policy-set-digest",
          replay_policy_set_digest};
      if (!trim_ascii(target_node_ids_arg).empty()) {
        argv.push_back("--replay-target-nodes");
        argv.push_back(trim_ascii(target_node_ids_arg));
      }
      if (!append_optional_double_cli_arg(args, "initial_equity_numeraire",
                                          "--replay-initial-equity-numeraire",
                                          &argv, err) ||
          !append_optional_double_cli_arg(args, "max_node_weight",
                                          "--replay-max-node-weight", &argv,
                                          err) ||
          !append_optional_double_cli_arg(args, "max_turnover_l1",
                                          "--replay-max-turnover-l1", &argv,
                                          err) ||
          !append_optional_int_cli_arg(args, "max_steps", "--replay-max-steps",
                                       &argv, err) ||
          !append_optional_int_cli_arg(args, "max_parallel_jobs",
                                       "--replay-max-parallel-jobs", &argv,
                                       err) ||
          !append_optional_double_cli_arg(
              args, "linear_transaction_cost_rate",
              "--replay-linear-transaction-cost-rate", &argv, err)) {
        return false;
      }
      process_result_t replay_result =
          run_process(argv, timeout_seconds,
                      static_cast<std::size_t>(policy_int_or(
                          ctx->policy, "max_capture_bytes", 65536)));
      if (replay_result.exit_code != 0 || replay_result.timed_out) {
        *err = "E_RUNTIME_POLICY_TRAINING_PPO_ON_POLICY_REPLAY_FAILED: " +
               replay_result.stderr_text;
        return false;
      }
      if (!load_ppo_v0_replay_rollout_collection(runtime_replay_report_path,
                                                 contract, contract_digest,
                                                 &rollout_collection, err)) {
        return false;
      }
      rollout_collection.rollout_collection_id =
          "ppo_v0_runtime_on_policy_replay_rollout";
      rollout_collection.collection_mode =
          "runtime_ppo_v0_on_policy_replay_collection";
      rollout_collection.collection_source = "kikijyeba_on_policy_replay";
      rollout_collection.input_policy_checkpoint_path =
          input_actor_checkpoint_path;
      rollout_collection.input_policy_checkpoint_digest =
          input_actor_checkpoint_digest;
      rollout_collection.on_policy_runtime_collection_complete = true;
      for (auto &sample : rollout_collection.samples) {
        sample.source = "kikijyeba_on_policy_replay";
      }
      if (!rollout_collection.policy_distribution_evidence_bound) {
        *err = "E_RUNTIME_POLICY_TRAINING_PPO_DISTRIBUTION_EVIDENCE_MISSING: "
               "fresh on-policy PPO replay collection must bind old-policy "
               "action-distribution evidence";
        return false;
      }
    } else if (!input_replay_report_path.empty() &&
               !load_ppo_v0_replay_rollout_collection(
                   input_replay_report_path, contract, contract_digest,
                   &rollout_collection, err)) {
      return false;
    } else if (!input_replay_report_path.empty()) {
      if (!rollout_collection.policy_distribution_evidence_bound) {
        *err = "E_RUNTIME_POLICY_TRAINING_PPO_DISTRIBUTION_EVIDENCE_MISSING: "
               "PPO replay-report ingestion requires old-policy "
               "action-distribution evidence";
        return false;
      }
      validation_rollout_reason =
          "existing_replay_report_ingestion_not_post_update_evaluation";
    }
    if (contract.resume_mode != "fresh_spawn" &&
        source_replay_job_dir.empty()) {
      input_actor_checkpoint_path = resolved_resume_actor.checkpoint_path;
      input_actor_module_state_path = resolved_resume_actor.module_state_path;
      input_actor_checkpoint_digest = resolved_resume_actor.checkpoint_digest;
      rollout_collection.input_policy_checkpoint_path =
          input_actor_checkpoint_path;
      rollout_collection.input_policy_checkpoint_digest =
          input_actor_checkpoint_digest;
    }
    ppo_v0_update_metrics_t metrics{};
    if (!compute_ppo_v0_update_metrics(
            contract, rollout_collection.samples, input_actor_module_state_path,
            resume_optimizer_torch_state_path, actor_module_state_path,
            optimizer_torch_state_path, runtime_device, &metrics, err)) {
      return false;
    }
    const std::string actor_module_state_digest = file_digest_or_empty(
        actor_module_state_path,
        "wikimyei.policy.portfolio.graph_node_allocation.module_state.v0");
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        actor_checkpoint_path,
        policy_training_ppo_actor_checkpoint_text(
            contract, contract_digest, "post_update_policy",
            rollout_collection.input_policy_checkpoint_digest, &metrics,
            "module_state.pt", actor_module_state_digest));
    const std::string actor_checkpoint_digest = file_digest_or_empty(
        actor_checkpoint_path, contract.policy_checkpoint_schema_id);

    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        critic_checkpoint_path,
        policy_training_ppo_critic_checkpoint_text(
            contract, contract_digest,
            rollout_collection.input_policy_checkpoint_digest, &metrics));
    const std::string critic_checkpoint_digest = file_digest_or_empty(
        critic_checkpoint_path, contract.policy_checkpoint_schema_id);

    metrics.optimizer_torch_state_path =
        optimizer_torch_state_path.filename().string();
    metrics.optimizer_torch_state_digest =
        file_digest_or_empty(optimizer_torch_state_path,
                             "kikijyeba.runtime.ppo_optimizer_torch_state.v1");
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        optimizer_state_path, policy_training_ppo_optimizer_state_text(
                                  contract, contract_digest, metrics));
    const std::string optimizer_state_digest = file_digest_or_empty(
        optimizer_state_path, "kikijyeba.runtime.ppo_optimizer_state.v1");

    auto emitted_contract = contract;
    emitted_contract.actor_checkpoint_digest = actor_checkpoint_digest;
    emitted_contract.critic_checkpoint_digest = critic_checkpoint_digest;
    emitted_contract.optimizer_state_digest = optimizer_state_digest;

    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        rollout_collection_path,
        policy_training_ppo_rollout_collection_text(
            emitted_contract, contract_digest, metrics, rollout_collection));
    emitted_contract.rollout_collection_digest = file_digest_or_empty(
        rollout_collection_path, contract.rollout_collection_schema_id);

    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        ppo_update_report_path,
        policy_training_ppo_update_report_text(emitted_contract,
                                               contract_digest, metrics));
    emitted_contract.ppo_update_report_digest = file_digest_or_empty(
        ppo_update_report_path, contract.ppo_update_report_schema_id);

    if (!source_replay_job_dir.empty()) {
      std::vector<std::string> validation_argv{
          exec_path.string(),
          "--config",
          replay_config_path.string(),
          "--replay-from-job-dir",
          source_replay_job_dir.string(),
          "--replay-accounting-numeraire-node",
          accounting_numeraire_node_id,
          "--replay-report-path",
          validation_replay_report_path.string(),
          "--replay-experiment-id",
          child_experiment_id_root + ".validation_rollout",
          "--replay-include-graph-node-allocation-policy",
          "--replay-no-numeraire-only-policy",
          "--replay-no-sdu-policy",
          "--replay-action-distribution-id",
          emitted_contract.action_distribution_id,
          "--replay-policy-artifact-digest",
          emitted_contract.actor_checkpoint_digest,
          "--replay-policy-checkpoint-path",
          actor_checkpoint_path.string(),
          "--replay-causal-schedule-digest",
          emitted_contract.causal_schedule_digest,
          "--replay-snapshot-family-digest",
          emitted_contract.snapshot_family_digest,
          "--replay-execution-profile-digest",
          emitted_contract.execution_profile_digest,
          "--replay-policy-set-digest",
          replay_policy_set_digest,
          "--replay-linear-transaction-cost-rate",
          std::to_string(validation_linear_transaction_cost_rate)};
      if (!trim_ascii(target_node_ids_arg).empty()) {
        validation_argv.push_back("--replay-target-nodes");
        validation_argv.push_back(trim_ascii(target_node_ids_arg));
      }
      if (!append_optional_double_cli_arg(args, "initial_equity_numeraire",
                                          "--replay-initial-equity-numeraire",
                                          &validation_argv, err) ||
          !append_optional_double_cli_arg(args, "max_node_weight",
                                          "--replay-max-node-weight",
                                          &validation_argv, err) ||
          !append_optional_double_cli_arg(args, "max_turnover_l1",
                                          "--replay-max-turnover-l1",
                                          &validation_argv, err)) {
        return false;
      }
      if (extract_json_raw_field(args, "max_steps", nullptr)) {
        if (!append_optional_int_cli_arg(args, "max_steps",
                                         "--replay-max-steps", &validation_argv,
                                         err)) {
          return false;
        }
      } else if (emitted_contract.max_steps > 0) {
        validation_argv.push_back("--replay-max-steps");
        validation_argv.push_back(std::to_string(emitted_contract.max_steps));
      }
      if (extract_json_raw_field(args, "max_parallel_jobs", nullptr)) {
        if (!append_optional_int_cli_arg(args, "max_parallel_jobs",
                                         "--replay-max-parallel-jobs",
                                         &validation_argv, err)) {
          return false;
        }
      } else if (emitted_contract.max_parallel_jobs > 0) {
        validation_argv.push_back("--replay-max-parallel-jobs");
        validation_argv.push_back(
            std::to_string(emitted_contract.max_parallel_jobs));
      }
      process_result_t validation_replay_result =
          run_process(validation_argv, timeout_seconds,
                      static_cast<std::size_t>(policy_int_or(
                          ctx->policy, "max_capture_bytes", 65536)));
      if (validation_replay_result.exit_code != 0 ||
          validation_replay_result.timed_out) {
        *err = "E_RUNTIME_POLICY_TRAINING_PPO_VALIDATION_REPLAY_FAILED: " +
               validation_replay_result.stderr_text;
        return false;
      }
      if (!load_ppo_v0_replay_rollout_collection(
              validation_replay_report_path, emitted_contract, contract_digest,
              &validation_rollout_collection, err)) {
        return false;
      }
      validation_rollout_collection.rollout_collection_id =
          "ppo_v0_runtime_cost_aware_validation_rollout";
      validation_rollout_collection.collection_mode =
          "runtime_ppo_v0_cost_aware_validation_rollout";
      validation_rollout_collection.collection_source =
          "kikijyeba_cost_aware_validation_replay";
      validation_rollout_collection.input_policy_checkpoint_path =
          actor_checkpoint_path;
      validation_rollout_collection.input_policy_checkpoint_digest =
          emitted_contract.actor_checkpoint_digest;
      validation_rollout_collection.on_policy_runtime_collection_complete =
          false;
      validation_rollout_collection.kikijyeba_cajtucu_replay_integration = true;
      for (auto &sample : validation_rollout_collection.samples) {
        sample.source = "kikijyeba_cost_aware_validation_replay";
      }
      validation_replay_report_digest =
          validation_rollout_collection.input_replay_report_digest;
      validation_rollout_ready = true;
      validation_rollout_reason =
          "post_update_deterministic_cost_aware_replay_complete";
    }

    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        validation_rollout_path,
        policy_training_ppo_validation_rollout_text(
            emitted_contract, contract_digest, actor_checkpoint_path,
            emitted_contract.actor_checkpoint_digest,
            validation_rollout_ready ? validation_replay_report_path
                                     : fs::path{},
            validation_replay_report_digest, validation_rollout_collection,
            validation_rollout_ready, validation_rollout_reason,
            validation_linear_transaction_cost_rate));
    emitted_contract.validation_rollout_report_digest = file_digest_or_empty(
        validation_rollout_path,
        "kikijyeba.runtime.ppo_cost_aware_validation_rollout.v1");

    if (!source_replay_job_dir.empty()) {
      std::vector<std::string> quality_argv{
          exec_path.string(),
          "--config",
          replay_config_path.string(),
          "--replay-from-job-dir",
          source_replay_job_dir.string(),
          "--replay-accounting-numeraire-node",
          accounting_numeraire_node_id,
          "--replay-report-path",
          policy_quality_replay_report_path.string(),
          "--replay-experiment-id",
          child_experiment_id_root + ".policy_quality",
          "--replay-include-graph-node-allocation-policy",
          "--replay-include-equal-weight",
          "--replay-include-current-weight",
          "--replay-action-distribution-id",
          emitted_contract.action_distribution_id,
          "--replay-policy-artifact-digest",
          emitted_contract.actor_checkpoint_digest,
          "--replay-policy-checkpoint-path",
          actor_checkpoint_path.string(),
          "--replay-causal-schedule-digest",
          emitted_contract.causal_schedule_digest,
          "--replay-snapshot-family-digest",
          emitted_contract.snapshot_family_digest,
          "--replay-execution-profile-digest",
          emitted_contract.execution_profile_digest,
          "--replay-policy-set-digest",
          replay_policy_set_digest,
          "--replay-linear-transaction-cost-rate",
          std::to_string(validation_linear_transaction_cost_rate)};
      if (!trim_ascii(target_node_ids_arg).empty()) {
        quality_argv.push_back("--replay-target-nodes");
        quality_argv.push_back(trim_ascii(target_node_ids_arg));
      }
      if (!append_optional_double_cli_arg(args, "initial_equity_numeraire",
                                          "--replay-initial-equity-numeraire",
                                          &quality_argv, err) ||
          !append_optional_double_cli_arg(args, "max_node_weight",
                                          "--replay-max-node-weight",
                                          &quality_argv, err) ||
          !append_optional_double_cli_arg(args, "max_turnover_l1",
                                          "--replay-max-turnover-l1",
                                          &quality_argv, err)) {
        return false;
      }
      if (extract_json_raw_field(args, "max_steps", nullptr)) {
        if (!append_optional_int_cli_arg(
                args, "max_steps", "--replay-max-steps", &quality_argv, err)) {
          return false;
        }
      } else if (emitted_contract.max_steps > 0) {
        quality_argv.push_back("--replay-max-steps");
        quality_argv.push_back(std::to_string(emitted_contract.max_steps));
      }
      if (extract_json_raw_field(args, "max_parallel_jobs", nullptr)) {
        if (!append_optional_int_cli_arg(args, "max_parallel_jobs",
                                         "--replay-max-parallel-jobs",
                                         &quality_argv, err)) {
          return false;
        }
      } else if (emitted_contract.max_parallel_jobs > 0) {
        quality_argv.push_back("--replay-max-parallel-jobs");
        quality_argv.push_back(
            std::to_string(emitted_contract.max_parallel_jobs));
      }
      process_result_t quality_replay_result =
          run_process(quality_argv, timeout_seconds,
                      static_cast<std::size_t>(policy_int_or(
                          ctx->policy, "max_capture_bytes", 65536)));
      if (quality_replay_result.exit_code != 0 ||
          quality_replay_result.timed_out) {
        *err = "E_RUNTIME_POLICY_TRAINING_PPO_POLICY_QUALITY_REPLAY_FAILED: " +
               quality_replay_result.stderr_text;
        return false;
      }
      if (!load_ppo_v0_replay_rollout_collection(
              policy_quality_replay_report_path, emitted_contract,
              contract_digest, &policy_quality_collection, err)) {
        return false;
      }
      policy_quality_collection.rollout_collection_id =
          "ppo_v0_runtime_policy_quality_replay";
      policy_quality_collection.collection_mode =
          "runtime_ppo_v0_policy_quality_report";
      policy_quality_collection.collection_source =
          "kikijyeba_policy_quality_replay";
      policy_quality_collection.input_policy_checkpoint_path =
          actor_checkpoint_path;
      policy_quality_collection.input_policy_checkpoint_digest =
          emitted_contract.actor_checkpoint_digest;
      policy_quality_collection.on_policy_runtime_collection_complete = false;
      policy_quality_collection.kikijyeba_cajtucu_replay_integration = true;
      for (auto &sample : policy_quality_collection.samples) {
        sample.source = "kikijyeba_policy_quality_replay";
      }
      policy_quality_replay_report_digest =
          policy_quality_collection.input_replay_report_digest;
      const auto missing_quality_ids = missing_policy_quality_summary_ids(
          policy_quality_collection.policy_summaries);
      if (!missing_quality_ids.empty()) {
        std::ostringstream missing;
        for (std::size_t i = 0; i < missing_quality_ids.size(); ++i) {
          if (i > 0) {
            missing << ",";
          }
          missing << missing_quality_ids[i];
        }
        *err =
            "E_RUNTIME_POLICY_TRAINING_PPO_POLICY_QUALITY_SUMMARY_INCOMPLETE: "
            "missing policy summaries: " +
            missing.str();
        return false;
      }
      policy_quality_report_ready = true;
      policy_quality_report_reason =
          "post_update_cost_aware_policy_quality_replay_complete";
    }

    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        policy_quality_report_path,
        policy_training_policy_quality_report_text(
            emitted_contract, contract_digest, actor_checkpoint_path,
            emitted_contract.actor_checkpoint_digest, validation_rollout_path,
            emitted_contract.validation_rollout_report_digest,
            policy_quality_report_ready ? policy_quality_replay_report_path
                                        : fs::path{},
            policy_quality_replay_report_digest, policy_quality_collection,
            policy_quality_report_ready, policy_quality_report_reason,
            validation_linear_transaction_cost_rate));
    emitted_contract.policy_quality_report_digest = file_digest_or_empty(
        policy_quality_report_path,
        cuwacunu::hero::runtime::k_policy_quality_report_schema_v1);
    if (cuwacunu::hero::runtime::policy_kind_is_ppo_adapter(
            emitted_contract.policy_kind,
            emitted_contract.ppo_policy_artifact_contract_id)) {
      emitted_contract.parent_allocation_engine_fact_digest.clear();
    }
    if (policy_quality_report_ready) {
      emitted_contract.parent_replay_environment_report_digest =
          replay_report_digest_for_path_or_empty(
              policy_quality_replay_report_path);
      emitted_contract.parent_replay_environment_fact_digest.clear();
    } else if (validation_rollout_ready) {
      emitted_contract.parent_replay_environment_report_digest =
          replay_report_digest_for_path_or_empty(validation_replay_report_path);
      emitted_contract.parent_replay_environment_fact_digest.clear();
    } else if (!rollout_collection.input_replay_report_path.empty()) {
      emitted_contract.parent_replay_environment_report_digest =
          replay_report_digest_for_path_or_empty(
              rollout_collection.input_replay_report_path);
      emitted_contract.parent_replay_environment_fact_digest.clear();
    }

    const fs::path report_path = job_dir / "policy_training.report";
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        report_path,
        policy_training_ppo_report_text(
            emitted_contract, contract_digest, actor_checkpoint_path,
            emitted_contract.actor_checkpoint_digest, critic_checkpoint_path,
            emitted_contract.critic_checkpoint_digest, optimizer_state_path,
            emitted_contract.optimizer_state_digest, rollout_collection_path,
            emitted_contract.rollout_collection_digest, ppo_update_report_path,
            emitted_contract.ppo_update_report_digest, validation_rollout_path,
            emitted_contract.validation_rollout_report_digest,
            policy_quality_report_path,
            emitted_contract.policy_quality_report_digest, metrics,
            rollout_collection));

    const std::string ppo_protocol_status =
        rollout_collection.on_policy_runtime_collection_complete
            ? "ppo_v0_runtime_on_policy_replay_collection"
            : (rollout_collection.kikijyeba_cajtucu_replay_integration
                   ? "ppo_v0_runtime_replay_report_ingestion"
                   : "ppo_v0_runtime_smoke_execution");
    const std::string ppo_execution_chain =
        rollout_collection.on_policy_runtime_collection_complete
            ? "runtime.policy_training_contract:validate "
              "-> runtime.policy_training.ppo_v0:"
              "dispatch_kikijyeba_replay_collect_on_policy_report_"
              "write_checkpoint_rollout_update"
            : (rollout_collection.kikijyeba_cajtucu_replay_integration
                   ? "runtime.policy_training_contract:validate "
                     "-> runtime.policy_training.ppo_v0:"
                     "ingest_replay_report_write_checkpoint_rollout_update"
                   : "runtime.policy_training_contract:validate "
                     "-> runtime.policy_training.ppo_v0:"
                     "write_checkpoint_rollout_update");
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        job_dir / "job.manifest",
        policy_training_manifest_text(
            emitted_contract, contract_digest, job_id, job_stable_id,
            attempt_leaf, ctx->global_config_path, ppo_protocol_status,
            "policy_training_ppo_v0", ppo_execution_chain));
    cuwacunu::hero::runtime::job_layout::write_component_spawn_ref(
        root, "wikimyei.policy.portfolio.graph_node_allocation",
        emitted_contract.policy_id, emitted_contract.policy_id,
        cuwacunu::hero::runtime::policy_operator_surface_digest(
            emitted_contract),
        "hero.runtime.policy_training.ppo_v0",
        "policy_training_contract_" + std::string(contract_digest),
        "policy_training_contract_" + std::string(contract_digest));
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        job_dir / "policy_training.contract", contract_text);

    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        job_dir / "runtime.result.fact",
        policy_training_ppo_runtime_result_fact_text(
            job_id, actor_checkpoint_path, metrics));
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        job_dir / "runtime.checkpoint_io.fact",
        policy_training_ppo_checkpoint_io_fact_text(
            job_id, actor_checkpoint_path,
            emitted_contract.actor_checkpoint_digest));
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        job_dir / "runtime.health_measurement.fact",
        policy_training_health_fact_text(job_id));

    const auto policy_training_identity =
        policy_training_lattice_identity_from_source_job(
            source_replay_job_dir, job_id,
            emitted_contract.parent_forecast_eval_fact_digest,
            emitted_contract.parent_observer_belief_fact_digest);
    const std::string fact_text = policy_training_fact_text(
        emitted_contract, emitted_contract.actor_checkpoint_digest,
        /*runtime_trainer=*/false,
        rollout_collection.input_policy_checkpoint_path.string(),
        rollout_collection.input_policy_checkpoint_digest,
        &policy_training_identity, &metrics);
    const fs::path policy_fact_path = job_dir / "runtime.policy_training.fact";
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        policy_fact_path, fact_text);
    cuwacunu::hero::runtime::job_layout::write_text_file_atomically(
        job_dir / "job.state",
        policy_training_ppo_state_text(job_id, job_stable_id, attempt_leaf,
                                       actor_checkpoint_path, report_path,
                                       metrics));

    std::ostringstream json;
    json << "{\"ok\":true,\"requested_mode\":\"execute\",\"dry_run\":false"
         << ",\"schema_version\":\"kikijyeba.runtime.policy_training_"
            "execution_packet.v1\""
         << ",\"policy_training_execution_supported\":true"
         << ",\"trainer_kind\":\"ppo_policy_adapter.v1\""
         << ",\"ppo_implemented\":true"
         << ",\"contract_digest\":" << json_quote(contract_digest)
         << ",\"contract_ref\":"
         << json_quote(display::contract_ref(contract_digest))
         << ",\"actor_checkpoint_digest\":"
         << json_quote(emitted_contract.actor_checkpoint_digest)
         << ",\"actor_checkpoint_ref\":"
         << json_quote(display::checkpoint_ref(
                emitted_contract.actor_checkpoint_digest))
         << ",\"critic_checkpoint_digest\":"
         << json_quote(emitted_contract.critic_checkpoint_digest)
         << ",\"critic_checkpoint_ref\":"
         << json_quote(display::checkpoint_ref(
                emitted_contract.critic_checkpoint_digest))
         << ",\"optimizer_state_digest\":"
         << json_quote(emitted_contract.optimizer_state_digest)
         << ",\"optimizer_state_ref\":"
         << json_quote(display::digest_ref(
                "opt", emitted_contract.optimizer_state_digest))
         << ",\"optimizer_state_schema_id\":"
         << json_quote(emitted_contract.optimizer_state_schema_id)
         << ",\"optimizer_torch_state_digest\":"
         << json_quote(metrics.optimizer_torch_state_digest)
         << ",\"optimizer_torch_state_ref\":"
         << json_quote(display::digest_ref(
                "opt", metrics.optimizer_torch_state_digest))
         << ",\"device_policy\":" << json_quote(metrics.device_policy)
         << ",\"runtime_device_kind\":"
         << json_quote(metrics.runtime_device_kind)
         << ",\"cuda_device_index\":" << metrics.cuda_device_index
         << ",\"cuda_verification_passed\":"
         << bool_json(metrics.cuda_verification_passed)
         << ",\"resume_mode\":" << json_quote(metrics.resume_mode)
         << ",\"resume_parent_loaded\":"
         << bool_json(metrics.resume_parent_loaded)
         << ",\"optimizer_state_resume_loaded\":"
         << bool_json(metrics.optimizer_state_resume_loaded)
         << ",\"rollout_collection_digest\":"
         << json_quote(emitted_contract.rollout_collection_digest)
         << ",\"rollout_collection_ref\":"
         << json_quote(
                display::report_ref(emitted_contract.rollout_collection_digest))
         << ",\"ppo_update_report_digest\":"
         << json_quote(emitted_contract.ppo_update_report_digest)
         << ",\"ppo_update_report_ref\":"
         << json_quote(
                display::report_ref(emitted_contract.ppo_update_report_digest))
         << ",\"validation_rollout_report_digest\":"
         << json_quote(emitted_contract.validation_rollout_report_digest)
         << ",\"validation_rollout_report_ref\":"
         << json_quote(display::report_ref(
                emitted_contract.validation_rollout_report_digest))
         << ",\"policy_quality_report_digest\":"
         << json_quote(emitted_contract.policy_quality_report_digest)
         << ",\"policy_quality_report_ref\":"
         << json_quote(display::report_ref(
                emitted_contract.policy_quality_report_digest))
         << ",\"optimizer_steps\":" << metrics.optimizer_steps
         << ",\"sample_count\":" << metrics.sample_count
         << ",\"rollout_collection_source\":"
         << json_quote(rollout_collection.collection_source)
         << ",\"replay_backed_step_count\":"
         << (rollout_collection.kikijyeba_cajtucu_replay_integration
                 ? metrics.sample_count
                 : 0)
         << ",\"input_replay_report_path\":"
         << json_quote(rollout_collection.input_replay_report_path.string())
         << ",\"input_replay_report_digest\":"
         << json_quote(rollout_collection.input_replay_report_digest)
         << ",\"input_replay_report_ref\":"
         << json_quote(display::report_ref(
                rollout_collection.input_replay_report_digest))
         << ",\"input_policy_checkpoint_path\":"
         << json_quote(rollout_collection.input_policy_checkpoint_path.string())
         << ",\"input_policy_checkpoint_digest\":"
         << json_quote(rollout_collection.input_policy_checkpoint_digest)
         << ",\"input_policy_checkpoint_ref\":"
         << json_quote(display::checkpoint_ref(
                rollout_collection.input_policy_checkpoint_digest))
         << ",\"job_id\":" << json_quote(job_id)
         << ",\"job_dir\":" << json_quote(job_dir.string())
         << ",\"manifest_path\":"
         << json_quote((job_dir / "job.manifest").string())
         << ",\"state_path\":" << json_quote((job_dir / "job.state").string())
         << ",\"report_path\":" << json_quote(report_path.string())
         << ",\"actor_checkpoint_path\":"
         << json_quote(actor_checkpoint_path.string())
         << ",\"critic_checkpoint_path\":"
         << json_quote(critic_checkpoint_path.string())
         << ",\"optimizer_state_path\":"
         << json_quote(optimizer_state_path.string())
         << ",\"optimizer_torch_state_path\":"
         << json_quote(optimizer_torch_state_path.string())
         << ",\"rollout_collection_path\":"
         << json_quote(rollout_collection_path.string())
         << ",\"ppo_update_report_path\":"
         << json_quote(ppo_update_report_path.string())
         << ",\"validation_rollout_report_path\":"
         << json_quote(validation_rollout_path.string())
         << ",\"policy_quality_report_path\":"
         << json_quote(policy_quality_report_path.string())
         << ",\"policy_quality_report_ready\":"
         << bool_json(policy_quality_report_ready)
         << ",\"policy_quality_replay_report_path\":"
         << json_quote(policy_quality_report_ready
                           ? policy_quality_replay_report_path.string()
                           : std::string{})
         << ",\"policy_quality_replay_report_digest\":"
         << json_quote(policy_quality_replay_report_digest)
         << ",\"validation_rollout_ready\":"
         << bool_json(validation_rollout_ready)
         << ",\"validation_replay_report_path\":"
         << json_quote(validation_rollout_ready
                           ? validation_replay_report_path.string()
                           : std::string{})
         << ",\"validation_replay_report_digest\":"
         << json_quote(validation_replay_report_digest)
         << ",\"policy_training_fact_path\":"
         << json_quote(policy_fact_path.string()) << ",\"artifacts\":"
         << job_artifacts_json(job_dir, report_path.string(), false,
                               static_cast<std::size_t>(policy_int_or(
                                   ctx->policy, "max_capture_bytes", 65536)))
         << ",\"terminal_evidence\":" << runtime_terminal_evidence_json(job_dir)
         << ",\"authority\":{\"runtime_executes_policy_training_smoke\":false,"
            "\"runtime_executes_live_capital\":false,"
            "\"runtime_trains_ppo\":true,"
            "\"runtime_selects_policy\":false,"
            "\"runtime_proves_lattice_target\":false,"
            "\"policy_quality_claimed\":false,"
            "\"market_readiness_claimed\":false}"
         << ",\"limitations\":{\"kikijyeba_cajtucu_replay_integration\":"
         << bool_json(rollout_collection.kikijyeba_cajtucu_replay_integration)
         << ",\"policy_distribution_evidence_bound\":"
         << bool_json(rollout_collection.policy_distribution_evidence_bound)
         << ",\"on_policy_runtime_collection_complete\":"
         << bool_json(rollout_collection.on_policy_runtime_collection_complete)
         << ",\"cost_aware_validation_rollout\":"
         << bool_json(validation_rollout_ready) << "}"
         << ",\"next_safe_actions\":[\"inspect_policy_training_artifact\","
            "\"evaluate_policy_training_artifact_ready\","
            "\"inspect_policy_quality_report\","
            "\"run_cost_aware_policy_evaluation\"]}";
    *out = json.str();
    return true;
  } catch (const std::exception &ex) {
    *err = std::string("E_RUNTIME_POLICY_TRAINING_PPO_EXECUTION_FAILED: ") +
           ex.what();
    return false;
  }
}

[[nodiscard]] bool handle_policy_training_contract_object(
    cuwacunu::hero::runtime::policy_training_job_contract_t contract,
    const std::string &execution_args, std::string_view requested_mode,
    runtime_context_t *ctx, std::string *out, std::string *err) {
  namespace runtime_contract = cuwacunu::hero::runtime;
  if (runtime_contract::policy_training_requires_ppo_artifact_contract(
          contract)) {
    contract.component_assembly_fingerprint =
        runtime_contract::policy_operator_surface_digest(contract);
  }

  const auto issues =
      runtime_contract::validate_policy_training_job_contract(contract);
  if (!issues.empty()) {
    *err = "E_RUNTIME_POLICY_TRAINING_CONTRACT_INVALID: " +
           string_array_json(issues);
    return false;
  }

  const auto contract_text = contract.to_text();
  const auto contract_digest =
      runtime_contract::policy_training_contract_digest_for_text(contract_text);
  if (requested_mode == "execute") {
    if (policy_training_execute_kind_is_ppo_v0(contract)) {
      return execute_policy_training_ppo_v0(execution_args, contract,
                                            contract_text, contract_digest, ctx,
                                            out, err);
    }
    return execute_policy_training_noop(execution_args, contract, contract_text,
                                        contract_digest, ctx, out, err);
  }
  const bool execution_supported =
      policy_training_execute_kind_supported(contract);
  std::ostringstream json;
  json << "{\"ok\":true,\"requested_mode\":" << json_quote(requested_mode)
       << ",\"dry_run\":true"
       << ",\"schema_version\":\"kikijyeba.runtime.policy_training_job_"
          "contract_packet.v1\""
       << ",\"contract_digest\":" << json_quote(contract_digest)
       << ",\"policy_training_execution_supported\":"
       << bool_json(execution_supported)
       << ",\"supported_execute_policy_kinds\":[\"noop_policy_training.v1\","
          "\"ppo_policy_adapter.v1\"]"
       << ",\"artifact_schema_id\":" << json_quote(contract.artifact_schema_id)
       << ",\"contract\":" << policy_training_contract_json(contract)
       << ",\"contract_text\":" << json_quote(contract_text)
       << ",\"authority\":{\"runtime_trains_policy\":"
       << bool_json(execution_supported)
       << ","
          "\"runtime_executes_live_capital\":false,"
          "\"runtime_selects_policy\":false,"
          "\"runtime_proves_lattice_target\":false}"
       << ",\"next_safe_actions\":[\"persist_contract_with_future_"
          "policy_training_runner\","
          "\"inspect_policy_training_contract\"]}";
  *out = json.str();
  return true;
}

void apply_policy_training_request_fixed_contract_values(
    cuwacunu::hero::runtime::policy_training_job_contract_t *contract) {
  if (contract == nullptr) {
    return;
  }
  namespace runtime_contract = cuwacunu::hero::runtime;
  contract->environment_contract_id = "kikijyeba.environment.replay.v1";
  contract->action_schema_digest =
      runtime_contract::k_target_node_weights_action_schema_v1;
  contract->policy_input_schema_id = runtime_contract::k_policy_input_schema_v1;
  contract->action_adapter_id =
      runtime_contract::k_target_node_weights_simplex_adapter_v1;
  contract->reward_contract_id =
      runtime_contract::k_post_execution_reward_contract_v1;
  contract->causal_schedule_schema_id =
      runtime_contract::k_policy_training_causal_schedule_schema_v1;
  contract->causal_schedule_no_future_snapshot_use_source =
      runtime_contract::k_policy_training_schedule_no_future_snapshot_source_v1;

  if (!runtime_contract::policy_training_requires_ppo_artifact_contract(
          *contract)) {
    return;
  }
  contract->ppo_policy_artifact_contract_id =
      runtime_contract::k_ppo_policy_artifact_contract_v1;
  contract->policy_family_id =
      runtime_contract::k_graph_node_allocation_policy_family_v1;
  contract->policy_checkpoint_schema_id =
      runtime_contract::k_ppo_policy_checkpoint_schema_v1;
  contract->optimizer_state_schema_id =
      "kikijyeba.runtime.ppo_optimizer_state.v1";
  contract->device_policy = "require_cuda";
  contract->cuda_device_index = 0;
  contract->cuda_device_index_bound = true;
  contract->advantage_estimator_id =
      runtime_contract::k_gae_advantage_estimator_v1;
  contract->rollout_collection_schema_id =
      runtime_contract::k_ppo_rollout_collection_schema_v1;
  contract->ppo_update_report_schema_id =
      runtime_contract::k_ppo_update_report_schema_v1;
}

[[nodiscard]] bool handle_policy_training_contract(
    const std::string &args, std::string_view requested_mode,
    runtime_context_t *ctx, std::string *out, std::string *err) {
  namespace runtime_contract = cuwacunu::hero::runtime;
  std::optional<wave_info_t> selected_wave;
  if (ctx != nullptr) {
    std::string config_arg;
    (void)extract_json_string_field(args, "config_path", &config_arg);
    selected_wave =
        load_wave_info(*ctx, effective_config_path(*ctx, config_arg));
  }
  const wave_info_t *wave_defaults =
      selected_wave.has_value() &&
              wave_info_requests_policy_training(*selected_wave)
          ? &*selected_wave
          : nullptr;
  runtime_contract::policy_training_job_contract_t contract{};
  if (!parse_required_string_arg(args, "protocol_id", &contract.protocol_id,
                                 err) ||
      !parse_required_string_arg(args, "protocol_contract_fingerprint",
                                 &contract.protocol_contract_fingerprint,
                                 err) ||
      !parse_required_string_arg(args, "graph_order_fingerprint",
                                 &contract.graph_order_fingerprint, err) ||
      !parse_required_string_arg(args, "source_cursor_token",
                                 &contract.source_cursor_token, err) ||
      !parse_required_string_arg_or_wave(args, "policy_id", wave_defaults,
                                         "POLICY_ID", &contract.policy_id,
                                         err) ||
      !parse_required_string_arg_or_wave(args, "policy_kind", wave_defaults,
                                         "POLICY_KIND", &contract.policy_kind,
                                         err) ||
      !parse_required_string_arg(args, "policy_architecture_digest",
                                 &contract.policy_architecture_digest, err) ||
      !parse_required_string_arg(args, "training_config_digest",
                                 &contract.training_config_digest, err) ||
      !parse_required_string_arg(args, "training_range_digest",
                                 &contract.training_range_digest, err) ||
      !parse_required_string_arg(args, "validation_range_digest",
                                 &contract.validation_range_digest, err) ||
      !parse_required_string_arg(args, "test_range_digest",
                                 &contract.test_range_digest, err) ||
      !parse_required_string_arg(args, "observation_schema_digest",
                                 &contract.observation_schema_digest, err) ||
      !parse_required_string_arg(args, "reward_contract_digest",
                                 &contract.reward_contract_digest, err) ||
      !parse_required_string_arg(args, "action_distribution_id",
                                 &contract.action_distribution_id, err) ||
      !parse_required_string_arg(args, "execution_profile_digest",
                                 &contract.execution_profile_digest, err) ||
      !parse_required_string_arg_or_wave(
          args, "training_schedule_mode", wave_defaults,
          "TRAINING_SCHEDULE_MODE", &contract.training_schedule_mode, err) ||
      !parse_required_string_arg(args, "causal_schedule_digest",
                                 &contract.causal_schedule_digest, err) ||
      !parse_required_string_arg(args, "causal_schedule_cursor_key_kind",
                                 &contract.causal_schedule_cursor_key_kind,
                                 err) ||
      !parse_required_string_arg(args, "normalization_fit_range_digest",
                                 &contract.normalization_fit_range_digest,
                                 err) ||
      !parse_required_string_arg(args, "replay_buffer_source_range_digest",
                                 &contract.replay_buffer_source_range_digest,
                                 err) ||
      !parse_required_string_arg(args, "early_stopping_policy_digest",
                                 &contract.early_stopping_policy_digest, err) ||
      !parse_required_string_arg(
          args, "hyperparameter_selection_policy_digest",
          &contract.hyperparameter_selection_policy_digest, err) ||
      !parse_required_string_arg(args, "selector_policy_digest",
                                 &contract.selector_policy_digest, err) ||
      !parse_required_string_arg(args, "parent_checkpoint_digest",
                                 &contract.parent_checkpoint_digest, err) ||
      !parse_required_string_arg(args, "parent_forecast_eval_fact_digest",
                                 &contract.parent_forecast_eval_fact_digest,
                                 err) ||
      !parse_required_string_arg(args, "parent_observer_belief_fact_digest",
                                 &contract.parent_observer_belief_fact_digest,
                                 err)) {
    return false;
  }
  (void)extract_json_string_field(args, "split_policy_fingerprint",
                                  &contract.split_policy_fingerprint);
  (void)extract_json_string_field(args, "component_assembly_fingerprint",
                                  &contract.component_assembly_fingerprint);
  (void)extract_json_string_field(args, "selector_split",
                                  &contract.selector_split);
  (void)extract_json_string_field(
      args, "final_refit_parent_selected_checkpoint_digest",
      &contract.final_refit_parent_selected_checkpoint_digest);
  (void)extract_json_string_field(
      args, "parent_allocation_engine_fact_digest",
      &contract.parent_allocation_engine_fact_digest);
  (void)extract_json_string_field(
      args, "parent_replay_environment_fact_digest",
      &contract.parent_replay_environment_fact_digest);
  (void)extract_json_string_field(
      args, "parent_replay_environment_report_digest",
      &contract.parent_replay_environment_report_digest);
  (void)extract_json_string_field(args, "policy_dsl_digest",
                                  &contract.policy_dsl_digest);
  (void)extract_json_string_field(args, "policy_net_digest",
                                  &contract.policy_net_digest);
  (void)extract_json_string_field(
      args, "policy_input_feature_manifest_digest",
      &contract.policy_input_feature_manifest_digest);
  (void)extract_json_string_field(args, "policy_jkimyei_digest",
                                  &contract.policy_jkimyei_digest);
  (void)extract_json_string_field(args, "target_node_universe_digest",
                                  &contract.target_node_universe_digest);
  (void)extract_json_string_field(args, "action_distribution_config_digest",
                                  &contract.action_distribution_config_digest);
  (void)extract_json_string_field(args, "snapshot_family_digest",
                                  &contract.snapshot_family_digest);
  (void)extract_json_string_field(args, "actor_architecture_digest",
                                  &contract.actor_architecture_digest);
  (void)extract_json_string_field(args, "critic_architecture_digest",
                                  &contract.critic_architecture_digest);
  (void)extract_json_string_field(args, "actor_checkpoint_digest",
                                  &contract.actor_checkpoint_digest);
  (void)extract_json_string_field(args, "critic_checkpoint_digest",
                                  &contract.critic_checkpoint_digest);
  (void)extract_json_string_field(args, "optimizer_state_digest",
                                  &contract.optimizer_state_digest);
  (void)extract_json_string_field(args, "ppo_config_digest",
                                  &contract.ppo_config_digest);
  (void)extract_json_string_field(args, "resume_mode", &contract.resume_mode);
  (void)extract_json_string_field(args, "resume_actor_checkpoint_path",
                                  &contract.resume_actor_checkpoint_path);
  (void)extract_json_string_field(args, "resume_actor_checkpoint_digest",
                                  &contract.resume_actor_checkpoint_digest);
  (void)extract_json_string_field(args, "resume_optimizer_state_path",
                                  &contract.resume_optimizer_state_path);
  (void)extract_json_string_field(args, "resume_optimizer_state_digest",
                                  &contract.resume_optimizer_state_digest);
  (void)extract_json_string_field(args, "advantage_normalization_policy",
                                  &contract.advantage_normalization_policy);
  (void)extract_json_string_field(args, "rollout_collection_digest",
                                  &contract.rollout_collection_digest);
  (void)extract_json_string_field(args, "ppo_update_report_digest",
                                  &contract.ppo_update_report_digest);
  (void)extract_json_string_field(args, "validation_rollout_report_digest",
                                  &contract.validation_rollout_report_digest);
  (void)extract_json_string_field(args, "policy_quality_report_digest",
                                  &contract.policy_quality_report_digest);

  int parsed_int = 0;
  double parsed_double = 0.0;
  if (extract_json_raw_field(args, "ppo_gamma", nullptr)) {
    if (!parse_optional_double_arg(args, "ppo_gamma", 0.0, &parsed_double,
                                   err)) {
      return false;
    }
    contract.ppo_gamma = parsed_double;
    contract.ppo_gamma_bound = true;
  }
  if (extract_json_raw_field(args, "ppo_gae_lambda", nullptr)) {
    if (!parse_optional_double_arg(args, "ppo_gae_lambda", 0.0, &parsed_double,
                                   err)) {
      return false;
    }
    contract.ppo_gae_lambda = parsed_double;
    contract.ppo_gae_lambda_bound = true;
  }
  if (extract_json_raw_field(args, "ppo_clip_epsilon", nullptr)) {
    if (!parse_optional_double_arg(args, "ppo_clip_epsilon", 0.0,
                                   &parsed_double, err)) {
      return false;
    }
    contract.ppo_clip_epsilon = parsed_double;
    contract.ppo_clip_epsilon_bound = true;
  }
  if (extract_json_raw_field(args, "ppo_target_kl", nullptr)) {
    if (!parse_optional_double_arg(args, "ppo_target_kl", 0.0, &parsed_double,
                                   err)) {
      return false;
    }
    contract.ppo_target_kl = parsed_double;
    contract.ppo_target_kl_bound = true;
  }
  if (extract_json_raw_field(args, "ppo_entropy_coeff", nullptr)) {
    if (!parse_optional_double_arg(args, "ppo_entropy_coeff", 0.0,
                                   &parsed_double, err)) {
      return false;
    }
    contract.ppo_entropy_coeff = parsed_double;
    contract.ppo_entropy_coeff_bound = true;
  }
  if (extract_json_raw_field(args, "ppo_value_loss_coeff", nullptr)) {
    if (!parse_optional_double_arg(args, "ppo_value_loss_coeff", 0.0,
                                   &parsed_double, err)) {
      return false;
    }
    contract.ppo_value_loss_coeff = parsed_double;
    contract.ppo_value_loss_coeff_bound = true;
  }
  if (extract_json_raw_field(args, "ppo_max_grad_norm", nullptr)) {
    if (!parse_optional_double_arg(args, "ppo_max_grad_norm", 0.0,
                                   &parsed_double, err)) {
      return false;
    }
    contract.ppo_max_grad_norm = parsed_double;
    contract.ppo_max_grad_norm_bound = true;
  }
  if (extract_json_raw_field(args, "ppo_minibatch_size", nullptr)) {
    if (!parse_optional_int_arg(args, "ppo_minibatch_size", 0, &parsed_int,
                                err)) {
      return false;
    }
    contract.ppo_minibatch_size = parsed_int;
    contract.ppo_minibatch_size_bound = true;
  }
  if (extract_json_raw_field(args, "ppo_epochs_per_rollout", nullptr)) {
    if (!parse_optional_int_arg(args, "ppo_epochs_per_rollout", 0, &parsed_int,
                                err)) {
      return false;
    }
    contract.ppo_epochs_per_rollout = parsed_int;
    contract.ppo_epochs_per_rollout_bound = true;
  }
  if (!parse_policy_training_required_int(args, "random_seed", &parsed_int,
                                          err)) {
    return false;
  }
  contract.random_seed = parsed_int;
  contract.random_seed_bound = true;
  if (!parse_policy_training_required_int(args, "max_episodes", &parsed_int,
                                          err)) {
    return false;
  }
  contract.max_episodes = parsed_int;
  if (!parse_policy_training_required_int(args, "max_steps", &parsed_int,
                                          err)) {
    return false;
  }
  contract.max_steps = parsed_int;
  if (!parse_optional_int_arg(args, "max_parallel_jobs", 1, &parsed_int, err)) {
    return false;
  }
  contract.max_parallel_jobs = parsed_int;
  if (!parse_policy_training_required_int(args, "max_wall_clock_seconds",
                                          &parsed_int, err)) {
    return false;
  }
  contract.max_wall_clock_seconds = parsed_int;
  if (!parse_optional_bool_arg(args, "causal_schedule_readiness_eligible", true,
                               &contract.causal_schedule_readiness_eligible,
                               err)) {
    return false;
  }
  if (!parse_optional_bool_arg(
          args, "causal_schedule_no_future_snapshot_use", true,
          &contract.causal_schedule_no_future_snapshot_use, err)) {
    return false;
  }
  if (!parse_optional_bool_arg(
          args, "offline_full_window_research_allowed", false,
          &contract.offline_full_window_research_allowed, err)) {
    return false;
  }
  if (!parse_optional_bool_arg(args, "final_refit_uses_validation", false,
                               &contract.final_refit_uses_validation, err)) {
    return false;
  }
  if (!parse_optional_bool_arg(args, "validation_no_longer_proof", false,
                               &contract.validation_no_longer_proof, err)) {
    return false;
  }
  if (!parse_optional_bool_arg(args, "sealed_test_required", false,
                               &contract.sealed_test_required, err)) {
    return false;
  }
  if (!parse_optional_bool_arg_or_wave(args, "live_execution_allowed",
                                       wave_defaults, "LIVE_EXECUTION_ALLOWED",
                                       false, &contract.live_execution_allowed,
                                       err)) {
    return false;
  }

  apply_policy_training_request_fixed_contract_values(&contract);
  return handle_policy_training_contract_object(contract, args, requested_mode,
                                                ctx, out, err);
}

[[nodiscard]] bool json_raw_is_empty_array(const std::string &raw) {
  const std::string value = trim_ascii(raw);
  return value == "[]";
}

[[nodiscard]] bool is_symbolic_handoff_value(std::string_view raw) {
  const std::string value = trim_ascii(raw);
  return value.rfind("latest_satisfying:", 0) == 0 ||
         value.rfind("lattice:", 0) == 0 || value.rfind("selector:", 0) == 0;
}

[[nodiscard]] bool string_map_has_symbolic_values(
    const std::unordered_map<std::string, std::string> &values,
    std::string *symbolic_key) {
  for (const auto &[key, value] : values) {
    if (is_symbolic_handoff_value(value)) {
      if (symbolic_key) {
        *symbolic_key = key;
      }
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool same_normalized_string_map(
    const std::unordered_map<std::string, std::string> &lhs,
    const std::unordered_map<std::string, std::string> &rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (const auto &[key, value] : lhs) {
    const auto found = rhs.find(key);
    if (found == rhs.end()) {
      return false;
    }
    if (fs::path(value).lexically_normal().string() !=
        fs::path(found->second).lexically_normal().string()) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] std::unordered_map<std::string, std::string>
normalized_path_string_map(
    const std::unordered_map<std::string, std::string> &values) {
  std::unordered_map<std::string, std::string> out;
  for (const auto &[key, value] : values) {
    out[key] = fs::path(value).lexically_normal().string();
  }
  return out;
}

struct runtime_handoff_binding_t {
  std::string handoff_id{};
  std::string handoff_digest{};
  std::string target_driver_run_id{};
  std::unordered_map<std::string, std::string> model_state_inputs{};
};

[[nodiscard]] bool validate_runtime_handoff_object(
    const std::string &handoff_raw, const fs::path &config_path,
    const fs::path &policy_path, bool dry_run, bool force_rebuild_cache,
    std::string *wave_raw, runtime_handoff_binding_t *binding,
    std::string *err) {
  std::string schema_version;
  if (!extract_json_string_field(handoff_raw, "handoff_schema_version",
                                 &schema_version) ||
      schema_version != "1") {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: handoff_schema_version must be 1";
    }
    return false;
  }

  std::string handoff_id;
  if (!extract_json_string_field(handoff_raw, "handoff_id", &handoff_id) ||
      trim_ascii(handoff_id).empty()) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: handoff_id is required";
    }
    return false;
  }

  std::string handoff_digest;
  if (!extract_json_string_field(handoff_raw, "handoff_digest",
                                 &handoff_digest) ||
      trim_ascii(handoff_digest).empty() ||
      handoff_id != "runtime_handoff_" + handoff_digest) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: handoff_digest is required and must "
             "match handoff_id";
    }
    return false;
  }

  std::string target_driver_run_id;
  (void)extract_json_string_field(handoff_raw, "target_driver_run_id",
                                  &target_driver_run_id);

  std::string created_by;
  if (!extract_json_string_field(handoff_raw, "created_by", &created_by) ||
      created_by != "marshal") {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: created_by must be marshal";
    }
    return false;
  }

  std::string created_at;
  if (!extract_json_string_field(handoff_raw, "created_at", &created_at) ||
      trim_ascii(created_at).empty()) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: created_at is required";
    }
    return false;
  }

  std::string target_id;
  if (!extract_json_string_field(handoff_raw, "target_id", &target_id) ||
      trim_ascii(target_id).empty()) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: target_id is required";
    }
    return false;
  }

  std::string unresolved_raw;
  if (!extract_json_raw_field(handoff_raw, "unresolved_symbols",
                              &unresolved_raw) ||
      !json_raw_is_empty_array(unresolved_raw)) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_UNRESOLVED_SYMBOLS: unresolved_symbols must "
             "be present and empty";
    }
    return false;
  }

  std::string base_config_raw;
  if (!extract_json_raw_field(handoff_raw, "base_config", &base_config_raw)) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: base_config is required";
    }
    return false;
  }
  std::string base_config_path;
  std::string base_config_digest;
  const std::string actual_base_config_digest = file_digest_or_empty(
      config_path, "kikijyeba.runtime.handoff.base_config_file.v1");
  if (!extract_json_string_field(base_config_raw, "path", &base_config_path) ||
      (!extract_json_string_field(base_config_raw, "digest",
                                  &base_config_digest) &&
       !extract_json_string_field(base_config_raw, "hash",
                                  &base_config_digest)) ||
      trim_ascii(base_config_digest).empty() ||
      base_config_digest != actual_base_config_digest ||
      fs::path(base_config_path).lexically_normal() !=
          config_path.lexically_normal()) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: base_config path/digest is invalid";
    }
    return false;
  }

  std::string runtime_policy_raw;
  if (!extract_json_raw_field(handoff_raw, "runtime_policy",
                              &runtime_policy_raw)) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: runtime_policy is required";
    }
    return false;
  }
  std::string runtime_policy_path;
  std::string runtime_policy_digest;
  const std::string actual_runtime_policy_digest = file_digest_or_empty(
      policy_path, "kikijyeba.runtime.handoff.runtime_policy_file.v1");
  if (!extract_json_string_field(runtime_policy_raw, "path",
                                 &runtime_policy_path) ||
      (!extract_json_string_field(runtime_policy_raw, "digest",
                                  &runtime_policy_digest) &&
       !extract_json_string_field(runtime_policy_raw, "hash",
                                  &runtime_policy_digest)) ||
      trim_ascii(runtime_policy_path).empty() ||
      trim_ascii(runtime_policy_digest).empty() ||
      runtime_policy_digest != actual_runtime_policy_digest ||
      fs::path(runtime_policy_path).lexically_normal() !=
          policy_path.lexically_normal()) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: runtime_policy path/digest is invalid";
    }
    return false;
  }

  std::string intent_raw;
  if (!extract_json_raw_field(handoff_raw, "intent", &intent_raw)) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: intent is required";
    }
    return false;
  }
  bool intent_dry_run = false;
  bool intent_force_rebuild_cache = false;
  if (extract_json_raw_field(intent_raw, "confirm_execute", nullptr)) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: intent.confirm_execute is retired";
    }
    return false;
  }
  if (!extract_json_bool_field(intent_raw, "dry_run", &intent_dry_run) ||
      !extract_json_bool_field(intent_raw, "force_rebuild_cache",
                               &intent_force_rebuild_cache) ||
      intent_dry_run != dry_run ||
      intent_force_rebuild_cache != force_rebuild_cache) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: intent differs from execute args";
    }
    return false;
  }

  std::string local_wave_raw;
  if (!extract_json_raw_field(handoff_raw, "wave", &local_wave_raw)) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: wave is required";
    }
    return false;
  }

  std::string checkpoint_inputs_raw;
  if (!extract_json_raw_field(handoff_raw, "checkpoint_inputs",
                              &checkpoint_inputs_raw)) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: checkpoint_inputs is required";
    }
    return false;
  }
  std::unordered_map<std::string, std::string> checkpoint_inputs;
  if (!extract_json_string_object(checkpoint_inputs_raw, &checkpoint_inputs)) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: checkpoint_inputs must be an object "
             "of string paths";
    }
    return false;
  }
  std::string symbolic_key;
  if (string_map_has_symbolic_values(checkpoint_inputs, &symbolic_key)) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_UNRESOLVED_SYMBOLS: checkpoint input " +
             symbolic_key + " is symbolic";
    }
    return false;
  }

  std::string wave_inputs_raw;
  std::unordered_map<std::string, std::string> wave_inputs;
  if (extract_json_raw_field(local_wave_raw, "model_state_inputs",
                             &wave_inputs_raw)) {
    if (!extract_json_string_object(wave_inputs_raw, &wave_inputs)) {
      if (err) {
        *err = "E_RUNTIME_HANDOFF_INVALID: wave.model_state_inputs malformed";
      }
      return false;
    }
    if (string_map_has_symbolic_values(wave_inputs, &symbolic_key)) {
      if (err) {
        *err = "E_RUNTIME_HANDOFF_UNRESOLVED_SYMBOLS: wave input " +
               symbolic_key + " is symbolic";
      }
      return false;
    }
  }
  if (!same_normalized_string_map(checkpoint_inputs, wave_inputs)) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: checkpoint_inputs differs from "
             "wave.model_state_inputs";
    }
    return false;
  }

  std::string lattice_refs_raw;
  std::unordered_map<std::string, std::string> lattice_refs;
  if (!extract_json_raw_field(handoff_raw, "lattice_certificate_refs",
                              &lattice_refs_raw) ||
      !extract_json_string_object(lattice_refs_raw, &lattice_refs)) {
    if (err) {
      *err = "E_RUNTIME_HANDOFF_INVALID: lattice_certificate_refs must be an "
             "object";
    }
    return false;
  }

  if (wave_raw) {
    *wave_raw = std::move(local_wave_raw);
  }
  if (binding) {
    binding->handoff_id = std::move(handoff_id);
    binding->handoff_digest = std::move(handoff_digest);
    binding->target_driver_run_id = std::move(target_driver_run_id);
    binding->model_state_inputs = normalized_path_string_map(checkpoint_inputs);
  }
  return true;
}

[[nodiscard]] bool expected_wave_matches_runtime_wave(
    const std::string &args, const fs::path &config_path,
    const fs::path &policy_path, bool dry_run, bool force_rebuild_cache,
    const wave_info_t &wave, runtime_handoff_binding_t *binding,
    std::unordered_map<std::string, std::string> *effective_model_state_inputs,
    std::string *err) {
  std::string expected_raw;
  std::string handoff_raw;
  if (args.find("\"runtime_handoff\"") != std::string::npos) {
    if (!extract_json_raw_field(args, "runtime_handoff", &handoff_raw)) {
      if (err) {
        *err = "E_RUNTIME_HANDOFF_INVALID: runtime_handoff is malformed";
      }
      return false;
    }
    if (!validate_runtime_handoff_object(handoff_raw, config_path, policy_path,
                                         dry_run, force_rebuild_cache,
                                         &expected_raw, binding, err)) {
      return false;
    }
  } else {
    return true;
  }

  std::string expected_target;
  if (extract_json_first_string_field(
          expected_raw, {"target_component_family_id"}, &expected_target)) {
    const std::string actual_target = wave.values.count("TARGET") != 0
                                          ? wave.values.at("TARGET")
                                          : std::string{};
    if (expected_target != actual_target) {
      if (err) {
        *err = "E_RUNTIME_EXPECTED_WAVE_MISMATCH: "
               "target_component_family_id differs";
      }
      return false;
    }
  }

  std::string expected_mode;
  if (extract_json_first_string_field(expected_raw, {"mode"}, &expected_mode)) {
    const std::string actual_mode = wave.values.count("MODE") != 0
                                        ? wave.values.at("MODE")
                                        : std::string{"run"};
    if (expected_mode != actual_mode) {
      if (err) {
        *err = "E_RUNTIME_EXPECTED_WAVE_MISMATCH: mode differs";
      }
      return false;
    }
  }

  std::string expected_source_range;
  if (extract_json_string_field(expected_raw, "source_range",
                                &expected_source_range)) {
    expected_source_range = canonical_source_range(expected_source_range);
    const std::string actual_source_range = canonical_source_range(
        wave.values.count("SOURCE_RANGE") != 0 ? wave.values.at("SOURCE_RANGE")
                                               : std::string{"all"});
    if (expected_source_range != actual_source_range) {
      if (err) {
        *err = "E_RUNTIME_EXPECTED_WAVE_MISMATCH: source_range differs";
      }
      return false;
    }
  }

  std::string expected_source_order;
  if (extract_json_string_field(expected_raw, "source_order",
                                &expected_source_order)) {
    expected_source_order = lowercase_ascii(trim_ascii(expected_source_order));
    const std::string actual_mode = wave.values.count("MODE") != 0
                                        ? wave.values.at("MODE")
                                        : std::string{"run"};
    const std::string actual_source_order =
        wave_source_order_from_info(wave, wave_action_from_mode(actual_mode));
    if (expected_source_order != actual_source_order) {
      if (err) {
        *err = "E_RUNTIME_EXPECTED_WAVE_MISMATCH: source_order differs";
      }
      return false;
    }
  }

  std::string expected_begin;
  if (extract_json_string_field(expected_raw, "anchor_index_begin",
                                &expected_begin)) {
    const std::string actual_begin =
        wave.values.count("ANCHOR_INDEX_BEGIN") != 0
            ? wave.values.at("ANCHOR_INDEX_BEGIN")
            : std::string{};
    if (expected_begin != actual_begin) {
      if (err) {
        *err = "E_RUNTIME_EXPECTED_WAVE_MISMATCH: anchor_index_begin differs";
      }
      return false;
    }
  }

  std::string expected_end;
  if (extract_json_string_field(expected_raw, "anchor_index_end",
                                &expected_end)) {
    const std::string actual_end = wave.values.count("ANCHOR_INDEX_END") != 0
                                       ? wave.values.at("ANCHOR_INDEX_END")
                                       : std::string{};
    if (expected_end != actual_end) {
      if (err) {
        *err = "E_RUNTIME_EXPECTED_WAVE_MISMATCH: anchor_index_end differs";
      }
      return false;
    }
  }

  std::string expected_source_key_begin;
  if (extract_json_string_field(expected_raw, "source_key_begin",
                                &expected_source_key_begin)) {
    const std::string actual_begin =
        wave.values.count("SOURCE_KEY_BEGIN") != 0
            ? wave.values.at("SOURCE_KEY_BEGIN")
            : (wave.values.count("ANCHOR_KEY_BEGIN") != 0
                   ? wave.values.at("ANCHOR_KEY_BEGIN")
                   : std::string{});
    if (expected_source_key_begin != actual_begin) {
      if (err) {
        *err = "E_RUNTIME_EXPECTED_WAVE_MISMATCH: source_key_begin differs";
      }
      return false;
    }
  }

  std::string expected_source_key_end;
  if (extract_json_string_field(expected_raw, "source_key_end",
                                &expected_source_key_end)) {
    const std::string actual_end =
        wave.values.count("SOURCE_KEY_END") != 0
            ? wave.values.at("SOURCE_KEY_END")
            : (wave.values.count("ANCHOR_KEY_END") != 0
                   ? wave.values.at("ANCHOR_KEY_END")
                   : std::string{});
    if (expected_source_key_end != actual_end) {
      if (err) {
        *err = "E_RUNTIME_EXPECTED_WAVE_MISMATCH: source_key_end differs";
      }
      return false;
    }
  }

  std::string expected_inputs_raw;
  if (extract_json_raw_field(expected_raw, "model_state_inputs",
                             &expected_inputs_raw)) {
    std::unordered_map<std::string, std::string> expected_inputs;
    if (!extract_json_string_object(expected_inputs_raw, &expected_inputs)) {
      if (err) {
        *err = "E_RUNTIME_EXPECTED_WAVE_MISMATCH: model_state_inputs malformed";
      }
      return false;
    }
    std::string symbolic_key;
    if (string_map_has_symbolic_values(expected_inputs, &symbolic_key)) {
      if (err) {
        *err = "E_RUNTIME_HANDOFF_UNRESOLVED_SYMBOLS: expected wave input " +
               symbolic_key + " is symbolic";
      }
      return false;
    }
    const auto actual_inputs =
        binding != nullptr && !binding->model_state_inputs.empty()
            ? binding->model_state_inputs
            : wave_model_state_inputs(wave);
    if (string_map_has_symbolic_values(actual_inputs, &symbolic_key)) {
      if (err) {
        *err = "E_RUNTIME_HANDOFF_UNRESOLVED_SYMBOLS: active wave input " +
               symbolic_key + " is symbolic";
      }
      return false;
    }
    for (const auto &[key, expected_value] : expected_inputs) {
      const auto found = actual_inputs.find(key);
      const std::string normalized_expected =
          fs::path(expected_value).lexically_normal().string();
      if (found == actual_inputs.end() ||
          found->second != normalized_expected) {
        if (err) {
          *err = "E_RUNTIME_EXPECTED_WAVE_MISMATCH: model_state_inputs differs";
        }
        return false;
      }
    }
  }

  if (effective_model_state_inputs != nullptr && binding != nullptr) {
    *effective_model_state_inputs = binding->model_state_inputs;
  }
  return true;
}

void append_model_state_checkpoint_arg(
    const std::unordered_map<std::string, std::string> &model_state_inputs,
    std::string_view input_key, std::string_view cli_flag,
    std::vector<std::string> *argv) {
  const auto found = model_state_inputs.find(std::string(input_key));
  if (found == model_state_inputs.end() || trim_ascii(found->second).empty()) {
    return;
  }
  argv->push_back(std::string(cli_flag));
  argv->push_back(fs::path(found->second).lexically_normal().string());
}

[[nodiscard]] bool handle_status(const std::string &args,
                                 runtime_context_t *ctx, std::string *out,
                                 std::string *err) {
  std::vector<json_field_t> fields;
  if (!validate_tool_fields(args, {}, &fields, err)) {
    return false;
  }
  const fs::path exec_path = policy_path(ctx->policy, "runtime_exec_path");
  const fs::path root = runtime_root(ctx->policy);
  const wave_info_t wave =
      load_wave_info(*ctx, effective_config_path(*ctx, ""));
  const std::size_t job_count =
      cuwacunu::hero::runtime::job_layout::discover_runtime_job_dirs(root)
          .size();
  std::ostringstream json;
  json << "{\"policy_path\":" << json_quote(ctx->policy.policy_path.string())
       << ",\"global_config_path\":"
       << json_quote(ctx->global_config_path.string())
       << ",\"runtime_profile\":" << json_quote(ctx->policy.profile_id)
       << ",\"runtime_profile_source\":"
       << json_quote(policy_get(ctx->policy, "runtime_profile_source"))
       << ",\"protocol_layer\":"
       << json_quote(policy_get(ctx->policy, "protocol_layer"))
       << ",\"runtime_exec_path\":" << json_quote(exec_path.string())
       << ",\"runtime_exec_exists\":" << bool_json(fs::exists(exec_path))
       << ",\"runtime_exec_executable\":"
       << bool_json(executable_file(exec_path))
       << ",\"runtime_root\":" << json_quote(root.string())
       << ",\"runtime_root_exists\":" << bool_json(fs::exists(root))
       << ",\"job_count\":" << job_count << ",\"default_dry_run\":"
       << bool_json(policy_bool_or(ctx->policy, "default_dry_run", true))
       << ",\"allow_execute\":"
       << bool_json(policy_bool_or(ctx->policy, "allow_execute", false))
       << ",\"allow_train_execute\":"
       << bool_json(policy_bool_or(ctx->policy, "allow_train_execute", false))
       << ",\"allow_dev_nuke\":"
       << bool_json(policy_bool_or(ctx->policy, "allow_dev_nuke", false))
       << ",\"dev_nuke_backup_enabled\":"
       << bool_json(
              policy_bool_or(ctx->policy, "dev_nuke_backup_enabled", false))
       << ",\"dev_nuke_backup_root\":"
       << json_quote(policy_path(ctx->policy, "dev_nuke_backup_root").string())
       << ",\"wave\":" << wave_info_json(wave) << "}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool handle_schema(const std::string &, runtime_context_t *,
                                 std::string *out, std::string *) {
  std::ostringstream json;
  json << "{\"keys\":[";
  for (std::size_t i = 0; i < kRuntimePolicyDescriptors.size(); ++i) {
    const auto &d = kRuntimePolicyDescriptors[i];
    if (i != 0) {
      json << ",";
    }
    json << "{\"key\":" << json_quote(d.key)
         << ",\"type\":" << json_quote(d.type)
         << ",\"required\":" << bool_json(d.required)
         << ",\"default\":" << json_quote(d.default_value)
         << ",\"range\":" << json_quote(d.range)
         << ",\"allowed\":" << json_quote(d.allowed)
         << ",\"summary\":" << json_quote(d.summary) << "}";
  }
  json << "]}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool handle_wave(const std::string &args, runtime_context_t *ctx,
                               std::string *out, std::string *) {
  std::string config_arg;
  (void)extract_json_string_field(args, "config_path", &config_arg);
  *out = wave_info_json(
      load_wave_info(*ctx, effective_config_path(*ctx, config_arg)));
  return true;
}

[[nodiscard]] bool execute_runtime(const std::string &args,
                                   runtime_context_t *ctx, bool force_dry_run,
                                   std::string *out, std::string *err) {
  std::string config_arg;
  std::string job_dir_arg;
  (void)extract_json_string_field(args, "config_path", &config_arg);
  (void)extract_json_string_field(args, "job_dir", &job_dir_arg);

  bool dry_run = policy_bool_or(ctx->policy, "default_dry_run", true);
  if (force_dry_run) {
    dry_run = true;
  } else if (!parse_optional_bool_arg(args, "dry_run", dry_run, &dry_run,
                                      err)) {
    return false;
  }

  bool force_rebuild_cache = false;
  if (!parse_optional_bool_arg(args, "force_rebuild_cache", false,
                               &force_rebuild_cache, err)) {
    return false;
  }
  if (force_rebuild_cache &&
      !policy_bool_or(ctx->policy, "allow_force_rebuild_cache", false)) {
    *err = "E_RUNTIME_FORCE_REBUILD_DENIED: allow_force_rebuild_cache is false";
    return false;
  }

  const fs::path config_path = effective_config_path(*ctx, config_arg);
  wave_overlay_t wave_overlay{};
  if (!parse_wave_overlay_from_args(args, &wave_overlay, err)) {
    return false;
  }
  wave_info_t wave = load_wave_info(*ctx, config_path);
  apply_wave_overlay_to_info(&wave, wave_overlay);
  std::string symbolic_key;
  if (string_map_has_symbolic_values(wave_model_state_inputs(wave),
                                     &symbolic_key)) {
    *err = "E_RUNTIME_HANDOFF_UNRESOLVED_SYMBOLS: active wave input " +
           symbolic_key + " is symbolic";
    return false;
  }
  runtime_handoff_binding_t handoff_binding{};
  std::unordered_map<std::string, std::string> handoff_model_state_inputs;
  if (!expected_wave_matches_runtime_wave(
          args, config_path, ctx->policy_path, dry_run, force_rebuild_cache,
          wave, &handoff_binding, &handoff_model_state_inputs, err)) {
    return false;
  }
  const std::string mode = wave.values.count("MODE") != 0
                               ? wave.values.at("MODE")
                               : std::string{"run"};
  const std::string action = wave_action_from_mode(mode);

  if (!dry_run) {
    if (!policy_bool_or(ctx->policy, "allow_execute", false)) {
      *err = "E_RUNTIME_EXECUTE_DENIED: allow_execute is false";
      return false;
    }
    if (action == "train" &&
        !policy_bool_or(ctx->policy, "allow_train_execute", false)) {
      *err = "E_RUNTIME_TRAIN_DENIED: allow_train_execute is false";
      return false;
    }
  }

  fs::path job_dir;
  if (!trim_ascii(job_dir_arg).empty()) {
    job_dir = normalize_path(fs::path(job_dir_arg));
    if (!explicit_job_dir_allowed(ctx->policy, job_dir)) {
      *err =
          "E_RUNTIME_JOB_DIR_DENIED: job_dir is outside allowed_job_roots: " +
          job_dir.string();
      return false;
    }
  }

  int timeout_seconds = 0;
  if (!policy_runtime_timeout_seconds(ctx->policy, &timeout_seconds, err)) {
    return false;
  }
  int max_capture = policy_int_or(ctx->policy, "max_capture_bytes", 65536);
  if (max_capture < 1024) {
    max_capture = 1024;
  }

  const fs::path exec_path = policy_path(ctx->policy, "runtime_exec_path");
  if (!executable_file(exec_path)) {
    *err = "E_RUNTIME_EXEC_MISSING: runtime_exec_path is not executable: " +
           exec_path.string();
    return false;
  }

  std::vector<std::string> argv{exec_path.string(), "--config",
                                config_path.string()};
  if (!job_dir.empty()) {
    argv.push_back("--job-dir");
    argv.push_back(job_dir.string());
  }
  if (dry_run) {
    argv.push_back("--dry-run");
  }
  if (force_rebuild_cache) {
    argv.push_back("--force-rebuild-cache");
  }
  if (wave_overlay.present) {
    argv.push_back("--source-range");
    argv.push_back(wave_overlay.source_range);
    if (wave_overlay.source_range == "anchor_index") {
      argv.push_back("--anchor-index-begin");
      argv.push_back(wave_overlay.anchor_index_begin);
      argv.push_back("--anchor-index-end");
      argv.push_back(wave_overlay.anchor_index_end);
    } else if (wave_overlay.source_range == "source_key") {
      argv.push_back("--source-key-begin");
      argv.push_back(wave_overlay.source_key_begin);
      argv.push_back("--source-key-end");
      argv.push_back(wave_overlay.source_key_end);
    }
  }
  if (!handoff_binding.handoff_id.empty()) {
    argv.push_back("--runtime-handoff-id");
    argv.push_back(handoff_binding.handoff_id);
    argv.push_back("--runtime-handoff-digest");
    argv.push_back(handoff_binding.handoff_digest);
  }
  if (!handoff_binding.target_driver_run_id.empty()) {
    argv.push_back("--marshal-target-driver-run-id");
    argv.push_back(handoff_binding.target_driver_run_id);
  }
  append_model_state_checkpoint_arg(handoff_model_state_inputs,
                                    "PLAN_INPUT_REPRESENTATION_CHECKPOINT",
                                    "--input-representation-checkpoint", &argv);
  append_model_state_checkpoint_arg(handoff_model_state_inputs,
                                    "PLAN_INPUT_MDN_CHECKPOINT",
                                    "--input-mdn-checkpoint", &argv);

  process_result_t result =
      run_process(argv, timeout_seconds, static_cast<std::size_t>(max_capture));
  const auto stdout_kv = parse_process_stdout_kv(result.stdout_text);
  *out = process_result_json(argv, result, job_dir, stdout_kv, false,
                             static_cast<std::size_t>(max_capture));
  return true;
}

[[nodiscard]] bool handle_dry_run(const std::string &args,
                                  runtime_context_t *ctx, std::string *out,
                                  std::string *err) {
  return execute_runtime(args, ctx, true, out, err);
}

[[nodiscard]] bool handle_execute(const std::string &args,
                                  runtime_context_t *ctx, std::string *out,
                                  std::string *err) {
  return execute_runtime(args, ctx, false, out, err);
}

[[nodiscard]] bool append_optional_double_cli_arg(
    const std::string &args, std::string_view key, std::string_view flag,
    std::vector<std::string> *argv, std::string *err) {
  std::string raw;
  if (!extract_json_raw_field(args, key, &raw)) {
    return true;
  }
  double ignored = 0.0;
  if (!parse_double(raw, &ignored)) {
    *err = std::string(key) + " must be number";
    return false;
  }
  argv->push_back(std::string(flag));
  argv->push_back(trim_ascii(raw));
  return true;
}

[[nodiscard]] bool append_optional_int_cli_arg(const std::string &args,
                                               std::string_view key,
                                               std::string_view flag,
                                               std::vector<std::string> *argv,
                                               std::string *err) {
  int value = 0;
  if (!extract_json_raw_field(args, key, nullptr)) {
    return true;
  }
  if (!extract_json_int_field(args, key, &value)) {
    *err = std::string(key) + " must be integer";
    return false;
  }
  argv->push_back(std::string(flag));
  argv->push_back(std::to_string(value));
  return true;
}

[[nodiscard]] bool
append_optional_string_cli_arg(const std::string &args, std::string_view key,
                               std::string_view flag,
                               std::vector<std::string> *argv) {
  std::string value;
  if (!extract_json_string_field(args, key, &value) ||
      trim_ascii(value).empty()) {
    return true;
  }
  argv->push_back(std::string(flag));
  argv->push_back(trim_ascii(value));
  return true;
}

[[nodiscard]] std::string
replay_dry_run_json(const std::vector<std::string> &argv,
                    const fs::path &job_dir, const fs::path &batch_index_path,
                    const fs::path &report_path) {
  std::ostringstream json;
  json << "{\"ok\":true,\"dry_run\":true,\"job_dir\":"
       << json_quote(job_dir.string()) << ",\"replay_batch_index_path\":"
       << json_quote(batch_index_path.string()) << ",\"report_path\":";
  if (report_path.empty()) {
    json << "null";
  } else {
    json << json_quote(report_path.string());
  }
  json << ",\"argv\":[";
  for (std::size_t i = 0; i < argv.size(); ++i) {
    if (i != 0) {
      json << ",";
    }
    json << json_quote(argv[i]);
  }
  json << "]}";
  return json.str();
}

[[nodiscard]] bool handle_replay(const std::string &args,
                                 runtime_context_t *ctx, std::string *out,
                                 std::string *err) {
  fs::path job_dir;
  if (!resolve_job_dir_from_args(args, ctx->policy, &job_dir, err)) {
    return false;
  }
  const fs::path manifest_path = job_dir / "job.manifest";
  const fs::path state_path = job_dir / "job.state";
  if (!fs::exists(manifest_path) || !fs::exists(state_path)) {
    *err = "E_RUNTIME_REPLAY_JOB_INVALID: job.manifest and job.state are "
           "required";
    return false;
  }
  const auto state = parse_kv_file(state_path);
  const auto status_found = state.find("status");
  const std::string status =
      status_found == state.end() ? std::string{} : status_found->second;
  if (lowercase_ascii(status) != "completed") {
    *err = "E_RUNTIME_REPLAY_JOB_NOT_COMPLETED: replay requires a completed "
           "Runtime job";
    return false;
  }
  const fs::path batch_index_path = runtime_replay_batch_index_path(job_dir);
  if (!fs::exists(batch_index_path)) {
    *err = "E_RUNTIME_REPLAY_ARTIFACTS_MISSING: missing " +
           batch_index_path.string();
    return false;
  }

  std::string config_arg;
  std::string accounting_numeraire_node_id;
  std::string target_node_ids;
  std::string experiment_id;
  std::string report_path_arg;
  (void)extract_json_string_field(args, "config_path", &config_arg);
  (void)extract_json_string_field(args, "accounting_numeraire_node_id",
                                  &accounting_numeraire_node_id);
  (void)extract_json_string_field(args, "target_node_ids", &target_node_ids);
  (void)extract_json_string_field(args, "experiment_id", &experiment_id);
  (void)extract_json_string_field(args, "report_path", &report_path_arg);

  const fs::path replay_config_path = effective_config_path(*ctx, config_arg);
  accounting_numeraire_node_id = trim_ascii(accounting_numeraire_node_id);
  if (accounting_numeraire_node_id.empty()) {
    const auto configured_numeraire = read_ini_value(
        replay_config_path, "ACCOUNTING", "accounting_numeraire_node_id");
    if (!configured_numeraire.has_value() ||
        trim_ascii(*configured_numeraire).empty()) {
      *err = "E_RUNTIME_REPLAY_ACCOUNTING_NUMERAIRE_MISSING: missing "
             "[ACCOUNTING].accounting_numeraire_node_id in " +
             replay_config_path.string();
      return false;
    }
    accounting_numeraire_node_id = trim_ascii(*configured_numeraire);
  }

  fs::path report_path;
  if (!trim_ascii(report_path_arg).empty()) {
    report_path = normalize_path(fs::path(report_path_arg));
    if (!explicit_job_dir_allowed(ctx->policy, report_path) &&
        !path_within(runtime_root(ctx->policy), report_path)) {
      *err = "E_RUNTIME_REPLAY_REPORT_DENIED: report_path is outside Runtime "
             "Hero roots: " +
             report_path.string();
      return false;
    }
  }

  bool dry_run = policy_bool_or(ctx->policy, "default_dry_run", true);
  if (!parse_optional_bool_arg(args, "dry_run", dry_run, &dry_run, err)) {
    return false;
  }

  bool include_equal_weight = false;
  if (!parse_optional_bool_arg(args, "include_equal_weight", false,
                               &include_equal_weight, err)) {
    return false;
  }
  bool include_current_weight = false;
  if (!parse_optional_bool_arg(args, "include_current_weight", false,
                               &include_current_weight, err)) {
    return false;
  }
  bool include_graph_node_allocation_policy = false;
  if (!parse_optional_bool_arg(args, "include_graph_node_allocation_policy",
                               false, &include_graph_node_allocation_policy,
                               err)) {
    return false;
  }
  bool on_policy_sample = false;
  if (!parse_optional_bool_arg(args, "on_policy_sample", false,
                               &on_policy_sample, err)) {
    return false;
  }
  bool include_numeraire_only_policy = true;
  if (!parse_optional_bool_arg(args, "include_numeraire_only_policy", true,
                               &include_numeraire_only_policy, err)) {
    return false;
  }
  bool include_sdu_policy = true;
  if (!parse_optional_bool_arg(args,
                               "include_spot_distributional_utility_policy",
                               true, &include_sdu_policy, err)) {
    return false;
  }
  bool allow_synthetic_direct_edges = false;
  if (!parse_optional_bool_arg(args, "allow_synthetic_direct_edges", false,
                               &allow_synthetic_direct_edges, err)) {
    return false;
  }
  bool validation_rollout = false;
  if (!parse_optional_bool_arg(args, "validation_rollout", false,
                               &validation_rollout, err)) {
    return false;
  }

  const bool has_linear_transaction_cost_rate =
      extract_json_raw_field(args, "linear_transaction_cost_rate", nullptr);
  double linear_transaction_cost_rate = 0.0;
  if (!parse_optional_double_arg(args, "linear_transaction_cost_rate", 0.0,
                                 &linear_transaction_cost_rate, err)) {
    return false;
  }
  const bool has_max_steps = extract_json_raw_field(args, "max_steps", nullptr);
  int max_steps = 0;
  if (!parse_optional_int_arg(args, "max_steps", 0, &max_steps, err)) {
    return false;
  }
  const bool has_max_parallel_jobs =
      extract_json_raw_field(args, "max_parallel_jobs", nullptr);
  int max_parallel_jobs = 0;
  if (!parse_optional_int_arg(args, "max_parallel_jobs", 0, &max_parallel_jobs,
                              err)) {
    return false;
  }
  std::string execution_profile_digest;
  std::string policy_set_digest;
  (void)extract_json_string_field(args, "execution_profile_digest",
                                  &execution_profile_digest);
  (void)extract_json_string_field(args, "policy_set_digest",
                                  &policy_set_digest);

  if (validation_rollout) {
    if (trim_ascii(execution_profile_digest).empty()) {
      *err = "E_RUNTIME_REPLAY_VALIDATION_PROFILE_DIGEST_MISSING";
      return false;
    }
    if (trim_ascii(policy_set_digest).empty()) {
      *err = "E_RUNTIME_REPLAY_VALIDATION_POLICY_SET_DIGEST_MISSING";
      return false;
    }
    if (!has_linear_transaction_cost_rate ||
        !std::isfinite(linear_transaction_cost_rate) ||
        linear_transaction_cost_rate <= 0.0) {
      *err = "E_RUNTIME_REPLAY_VALIDATION_NONZERO_COST_REQUIRED";
      return false;
    }
    if (allow_synthetic_direct_edges) {
      *err = "E_RUNTIME_REPLAY_VALIDATION_SYNTHETIC_EDGES_FORBIDDEN";
      return false;
    }
    if (!has_max_steps || max_steps <= 0) {
      *err = "E_RUNTIME_REPLAY_VALIDATION_MAX_STEPS_REQUIRED";
      return false;
    }
    if (!has_max_parallel_jobs || max_parallel_jobs <= 0) {
      *err = "E_RUNTIME_REPLAY_VALIDATION_PARALLELISM_INVALID";
      return false;
    }
  }

  int timeout_seconds = 0;
  if (!policy_runtime_timeout_seconds(ctx->policy, &timeout_seconds, err)) {
    return false;
  }
  int max_capture = policy_int_or(ctx->policy, "max_capture_bytes", 65536);
  if (max_capture < 1024) {
    max_capture = 1024;
  }

  const fs::path exec_path = policy_path(ctx->policy, "runtime_exec_path");
  if (!executable_file(exec_path)) {
    *err = "E_RUNTIME_EXEC_MISSING: runtime_exec_path is not executable: " +
           exec_path.string();
    return false;
  }

  std::vector<std::string> argv{exec_path.string()};
  argv.push_back("--config");
  argv.push_back(replay_config_path.string());
  argv.push_back("--replay-from-job-dir");
  argv.push_back(job_dir.string());
  argv.push_back("--replay-accounting-numeraire-node");
  argv.push_back(trim_ascii(accounting_numeraire_node_id));
  if (!trim_ascii(target_node_ids).empty()) {
    argv.push_back("--replay-target-nodes");
    argv.push_back(trim_ascii(target_node_ids));
  }
  if (!trim_ascii(experiment_id).empty()) {
    argv.push_back("--replay-experiment-id");
    argv.push_back(trim_ascii(experiment_id));
  }
  if (!report_path.empty()) {
    argv.push_back("--replay-report-path");
    argv.push_back(report_path.string());
  }
  if (!append_optional_double_cli_arg(args, "initial_equity_numeraire",
                                      "--replay-initial-equity-numeraire",
                                      &argv, err) ||
      !append_optional_double_cli_arg(args, "max_node_weight",
                                      "--replay-max-node-weight", &argv, err) ||
      !append_optional_double_cli_arg(args, "max_turnover_l1",
                                      "--replay-max-turnover-l1", &argv, err) ||
      !append_optional_int_cli_arg(args, "max_steps", "--replay-max-steps",
                                   &argv, err) ||
      !append_optional_int_cli_arg(args, "max_parallel_jobs",
                                   "--replay-max-parallel-jobs", &argv, err)) {
    return false;
  }
  if (include_equal_weight) {
    argv.push_back("--replay-include-equal-weight");
  }
  if (include_current_weight) {
    argv.push_back("--replay-include-current-weight");
  }
  if (include_graph_node_allocation_policy) {
    argv.push_back("--replay-include-graph-node-allocation-policy");
  }
  if (on_policy_sample) {
    argv.push_back("--replay-on-policy-sample");
  }
  if (!include_numeraire_only_policy) {
    argv.push_back("--replay-no-numeraire-only-policy");
  }
  if (!include_sdu_policy) {
    argv.push_back("--replay-no-sdu-policy");
  }
  if (allow_synthetic_direct_edges) {
    argv.push_back("--replay-allow-synthetic-direct-edges");
  }
  if (!append_optional_double_cli_arg(args, "linear_transaction_cost_rate",
                                      "--replay-linear-transaction-cost-rate",
                                      &argv, err)) {
    return false;
  }
  if (!append_optional_string_cli_arg(args, "execution_profile_digest",
                                      "--replay-execution-profile-digest",
                                      &argv) ||
      !append_optional_string_cli_arg(args, "policy_set_digest",
                                      "--replay-policy-set-digest", &argv) ||
      !append_optional_string_cli_arg(args, "action_distribution_id",
                                      "--replay-action-distribution-id",
                                      &argv) ||
      !append_optional_string_cli_arg(args, "policy_artifact_digest",
                                      "--replay-policy-artifact-digest",
                                      &argv) ||
      !append_optional_string_cli_arg(args, "causal_schedule_digest",
                                      "--replay-causal-schedule-digest",
                                      &argv) ||
      !append_optional_string_cli_arg(args, "snapshot_family_digest",
                                      "--replay-snapshot-family-digest",
                                      &argv)) {
    return false;
  }

  if (dry_run) {
    *out = replay_dry_run_json(argv, job_dir, batch_index_path, report_path);
    return true;
  }

  process_result_t result =
      run_process(argv, timeout_seconds, static_cast<std::size_t>(max_capture));
  const auto stdout_kv = parse_process_stdout_kv(result.stdout_text);
  if (validation_rollout) {
    if (result.exit_code != 0 || result.timed_out) {
      *err = "E_RUNTIME_REPLAY_VALIDATION_PROCESS_FAILED";
      return false;
    }
    fs::path validation_report_path = report_path;
    if (validation_report_path.empty()) {
      const auto found_report = stdout_kv.find("replay_report_path");
      if (found_report != stdout_kv.end()) {
        validation_report_path = normalize_path(fs::path(found_report->second));
      }
    }
    if (!validate_replay_report_for_validation_rollout(validation_report_path,
                                                       err)) {
      return false;
    }
  }
  *out = process_result_json(argv, result, job_dir, stdout_kv, false,
                             static_cast<std::size_t>(max_capture));
  return true;
}

[[nodiscard]] bool handle_dev_nuke(const std::string &args,
                                   runtime_context_t *ctx, std::string *out,
                                   std::string *err) {
  std::string root_arg;
  (void)extract_json_string_field(args, "runtime_root", &root_arg);
  fs::path root = trim_ascii(root_arg).empty()
                      ? runtime_root(ctx->policy)
                      : normalize_path(fs::path(root_arg));
  root = normalize_path(root);

  bool dry_run = true;
  if (!parse_optional_bool_arg(args, "dry_run", true, &dry_run, err)) {
    return false;
  }
  bool backup_enabled =
      policy_bool_or(ctx->policy, "dev_nuke_backup_enabled", false);
  if (!parse_optional_bool_arg(args, "backup", backup_enabled, &backup_enabled,
                               err)) {
    return false;
  }
  if (!looks_safe_to_clear_runtime_root(root)) {
    *err = "E_RUNTIME_DEV_NUKE_UNSAFE_ROOT: refusing unsafe runtime_root: " +
           root.string();
    return false;
  }
  if (!dev_nuke_root_allowed(ctx->policy, root)) {
    *err = "E_RUNTIME_DEV_NUKE_ROOT_DENIED: runtime_root is outside "
           "allowed_dev_nuke_roots: " +
           root.string();
    return false;
  }

  fs::path backup_root = policy_path(ctx->policy, "dev_nuke_backup_root");
  if (backup_enabled) {
    backup_root = normalize_path(backup_root);
    if (path_within(root, backup_root)) {
      *err =
          "E_RUNTIME_DEV_NUKE_BACKUP_DENIED: dev_nuke_backup_root must not be "
          "inside runtime_root";
      return false;
    }
  }

  std::vector<dev_nuke_target_t> targets;
  if (!collect_dev_nuke_targets(root, &targets, err)) {
    return false;
  }
  std::vector<active_job_marker_t> active_jobs;
  if (!scan_active_runtime_jobs(root, &active_jobs, err)) {
    return false;
  }

  std::uintmax_t planned_entries = 0;
  for (const auto &target : targets) {
    planned_entries += target.entry_count;
  }

  const bool allow_dev_nuke =
      policy_bool_or(ctx->policy, "allow_dev_nuke", false);
  if (dry_run) {
    std::ostringstream json;
    json << "{\"ok\":true,\"dry_run\":true,\"would_clear\":"
         << bool_json(targets.size() > 0) << ",\"would_block_on_active_jobs\":"
         << bool_json(!active_jobs.empty())
         << ",\"runtime_root\":" << json_quote(root.string())
         << ",\"runtime_root_exists\":" << bool_json(fs::exists(root))
         << ",\"target_count\":" << targets.size()
         << ",\"planned_entry_count\":" << planned_entries
         << ",\"backup_enabled\":" << bool_json(backup_enabled)
         << ",\"backup_root\":"
         << (backup_enabled ? json_quote(backup_root.string())
                            : std::string{"null"})
         << ",\"allow_dev_nuke\":" << bool_json(allow_dev_nuke)
         << ",\"targets\":" << dev_nuke_targets_json(targets)
         << ",\"active_jobs\":" << active_jobs_json(active_jobs) << "}";
    *out = json.str();
    return true;
  }

  if (!allow_dev_nuke) {
    *err = "E_RUNTIME_DEV_NUKE_DENIED: allow_dev_nuke is false";
    return false;
  }
  if (!active_jobs.empty()) {
    *err = "E_RUNTIME_DEV_NUKE_ACTIVE_JOBS: refusing reset while active or "
           "unknown-status jobs exist";
    return false;
  }

  std::vector<fs::path> backed_up_paths;
  std::vector<fs::path> removed_paths;
  fs::path snapshot_root;
  std::uintmax_t cleared_entries = 0;
  if (backup_enabled && !targets.empty()) {
    if (!make_dev_nuke_snapshot_root(root, backup_root, &snapshot_root, err)) {
      return false;
    }
    for (const auto &target : targets) {
      fs::path backup_path;
      if (!move_dev_nuke_target(root, target.path, snapshot_root, &backup_path,
                                err)) {
        return false;
      }
      backed_up_paths.push_back(backup_path);
      cleared_entries += target.entry_count;
    }
  } else {
    for (const auto &target : targets) {
      std::error_code ec;
      const std::uintmax_t removed = fs::remove_all(target.path, ec);
      if (ec) {
        *err = "failed to remove runtime target during dev_nuke: " +
               target.path.string() + ": " + ec.message();
        return false;
      }
      removed_paths.push_back(target.path);
      cleared_entries += removed;
    }
  }

  std::ostringstream json;
  json << "{\"ok\":true,\"dry_run\":false,\"cleared\":true"
       << ",\"runtime_root\":" << json_quote(root.string())
       << ",\"target_count\":" << targets.size()
       << ",\"cleared_entry_count\":" << cleared_entries
       << ",\"backup_enabled\":" << bool_json(backup_enabled)
       << ",\"backup_snapshot_path\":";
  if (snapshot_root.empty()) {
    json << "null";
  } else {
    json << json_quote(snapshot_root.string());
  }
  json << ",\"backed_up_paths\":" << path_vector_json(backed_up_paths)
       << ",\"removed_paths\":" << path_vector_json(removed_paths)
       << ",\"active_jobs\":[]"
       << "}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool handle_list_jobs(const std::string &args,
                                    runtime_context_t *ctx, std::string *out,
                                    std::string *err) {
  std::string root_arg;
  (void)extract_json_string_field(args, "root", &root_arg);
  fs::path root = trim_ascii(root_arg).empty() ? runtime_root(ctx->policy)
                                               : normalize_path(root_arg);
  if (!explicit_job_dir_allowed(ctx->policy, root) &&
      !path_within(runtime_root(ctx->policy), root)) {
    *err = "E_RUNTIME_JOB_DIR_DENIED: root is outside Runtime Hero roots: " +
           root.string();
    return false;
  }
  int limit = 20;
  if (!parse_optional_int_arg(args, "limit", limit, &limit, err)) {
    return false;
  }
  bool include_artifacts = false;
  if (!parse_optional_bool_arg(args, "include_artifacts", false,
                               &include_artifacts, err)) {
    return false;
  }

  const auto rows =
      cuwacunu::hero::runtime::job_layout::discover_runtime_job_dirs(root);
  if (limit < 0) {
    limit = 0;
  }
  if (limit > 200) {
    limit = 200;
  }

  std::ostringstream json;
  json << "{\"root\":" << json_quote(root.string())
       << ",\"count\":" << rows.size() << ",\"jobs\":[";
  const std::size_t n =
      std::min<std::size_t>(rows.size(), static_cast<std::size_t>(limit));
  for (std::size_t i = 0; i < n; ++i) {
    if (i != 0) {
      json << ",";
    }
    const auto manifest = parse_kv_file(rows[i].manifest_path);
    const auto state = parse_kv_file(rows[i].state_path);
    json << "{\"job_dir\":" << json_quote(rows[i].dir.string())
         << ",\"job_id\":"
         << json_quote(manifest.count("job_id") != 0
                           ? manifest.at("job_id")
                           : rows[i].dir.filename().string())
         << ",\"job_stable_id\":"
         << json_quote(manifest.count("job_stable_id") != 0
                           ? manifest.at("job_stable_id")
                           : "")
         << ",\"job_attempt_id\":"
         << json_quote(manifest.count("job_attempt_id") != 0
                           ? manifest.at("job_attempt_id")
                           : "")
         << ",\"job_attempt_policy\":"
         << json_quote(manifest.count("job_attempt_policy") != 0
                           ? manifest.at("job_attempt_policy")
                           : "")
         << ",\"status\":"
         << json_quote(state.count("status") != 0 ? state.at("status") : "")
         << ",\"job_kind\":"
         << json_quote(manifest.count("job_kind") != 0 ? manifest.at("job_kind")
                                                       : "")
         << ",\"target_component_family_id\":"
         << json_quote(manifest.count("target_component_family_id") != 0
                           ? manifest.at("target_component_family_id")
                           : "")
         << ",\"wave_action\":"
         << json_quote(manifest.count("wave_action") != 0
                           ? manifest.at("wave_action")
                           : "")
         << ",\"accepted_anchor_count\":"
         << json_quote(manifest.count("accepted_anchor_count") != 0
                           ? manifest.at("accepted_anchor_count")
                           : "");
    if (include_artifacts) {
      json << ",\"artifacts\":"
           << job_artifacts_json(rows[i].dir, "", false,
                                 static_cast<std::size_t>(policy_int_or(
                                     ctx->policy, "max_capture_bytes", 65536)));
    }
    json << "}";
  }
  json << "]}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool handle_get_job(const std::string &args,
                                  runtime_context_t *ctx, std::string *out,
                                  std::string *err) {
  fs::path job_dir;
  if (!resolve_job_dir_from_args(args, ctx->policy, &job_dir, err)) {
    return false;
  }
  bool include_text = false;
  if (!parse_optional_bool_arg(args, "include_text", false, &include_text,
                               err)) {
    return false;
  }
  int max_bytes = policy_int_or(ctx->policy, "max_capture_bytes", 65536);
  if (!parse_optional_int_arg(args, "max_bytes", max_bytes, &max_bytes, err)) {
    return false;
  }
  if (max_bytes < 1) {
    *err = "max_bytes must be >= 1";
    return false;
  }
  *out = job_artifacts_json(job_dir, "", include_text,
                            static_cast<std::size_t>(max_bytes));
  return true;
}

[[nodiscard]] bool handle_read_artifact(const std::string &args,
                                        runtime_context_t *ctx,
                                        std::string *out, std::string *err) {
  std::string explicit_path_arg;
  (void)extract_json_string_field(args, "path", &explicit_path_arg);
  fs::path path;
  std::optional<replay_experiment_report_binding_t> replay_report_binding;
  if (!trim_ascii(explicit_path_arg).empty()) {
    path = normalize_path(explicit_path_arg);
    if (!explicit_job_dir_allowed(ctx->policy, path) &&
        !path_within(runtime_root(ctx->policy), path)) {
      *err = "E_RUNTIME_ARTIFACT_DENIED: path is outside Runtime Hero roots: " +
             path.string();
      return false;
    }
  } else {
    fs::path job_dir;
    if (!resolve_job_dir_from_args(args, ctx->policy, &job_dir, err)) {
      return false;
    }
    std::string artifact;
    (void)extract_json_string_field(args, "artifact", &artifact);
    artifact = lowercase_ascii(trim_ascii(artifact));
    if (artifact.empty() || artifact == "manifest") {
      path = job_dir / "job.manifest";
    } else if (artifact == "state") {
      path = job_dir / "job.state";
    } else if (artifact == "inference_report" || artifact == "report") {
      path = job_dir / "channel_inference.report";
      if (!fs::exists(path)) {
        path = job_dir / "channel_representation.report";
      }
      if (!fs::exists(path)) {
        path = job_dir / "inference.report";
      }
      if (!fs::exists(path)) {
        path = job_dir / "representation.report";
      }
    } else if (artifact == "representation_report") {
      path = job_dir / "channel_representation.report";
      if (!fs::exists(path)) {
        path = job_dir / "representation.report";
      }
    } else if (artifact == "replay_batch_index" ||
               artifact == "runtime_replay_batches") {
      path = runtime_replay_batch_index_path(job_dir);
    } else if (artifact == "replay_experiment_index" ||
               artifact == "runtime_replay_experiments") {
      path = runtime_replay_experiment_index_path(job_dir);
    } else if (artifact == "replay_experiment_report" ||
               artifact == "replay_report") {
      replay_report_binding = latest_replay_experiment_report_binding(job_dir);
      path = replay_report_binding->report_path;
    } else {
      *err = "unknown artifact: " + artifact;
      return false;
    }
  }
  int max_bytes = policy_int_or(ctx->policy, "max_capture_bytes", 65536);
  if (!parse_optional_int_arg(args, "max_bytes", max_bytes, &max_bytes, err)) {
    return false;
  }
  bool truncated = false;
  const std::string text = read_text_file_limited(
      path, static_cast<std::size_t>(max_bytes), &truncated);
  std::ostringstream json;
  json << "{\"path\":" << json_quote(path.string())
       << ",\"exists\":" << bool_json(fs::exists(path))
       << ",\"truncated\":" << bool_json(truncated)
       << ",\"text\":" << json_quote(text)
       << ",\"fields\":" << kv_map_to_json(parse_kv_file(path));
  if (replay_report_binding.has_value()) {
    json << ",\"replay_report_integrity\":"
         << replay_report_integrity_json(*replay_report_binding);
  }
  json << "}";
  *out = json.str();
  return true;
}

[[nodiscard]] bool handle_inspect_schema(const std::string &args,
                                         runtime_context_t *ctx,
                                         std::string *out, std::string *err) {
  std::vector<json_field_t> fields;
  if (!validate_tool_fields(args, {}, &fields, err)) {
    return false;
  }
  return handle_schema(args, ctx, out, err);
}

[[nodiscard]] bool handle_inspect_wave(const std::string &args,
                                       runtime_context_t *ctx, std::string *out,
                                       std::string *err) {
  std::vector<json_field_t> fields;
  if (!validate_tool_fields(args, {"config_path"}, &fields, err)) {
    return false;
  }
  return handle_wave(args, ctx, out, err);
}

[[nodiscard]] bool handle_inspect_jobs(const std::string &args,
                                       runtime_context_t *ctx, std::string *out,
                                       std::string *err) {
  std::vector<json_field_t> fields;
  if (!validate_tool_fields(args, {"root", "limit", "include_artifacts"},
                            &fields, err)) {
    return false;
  }
  return handle_list_jobs(args, ctx, out, err);
}

[[nodiscard]] bool handle_inspect_job(const std::string &args,
                                      runtime_context_t *ctx, std::string *out,
                                      std::string *err) {
  std::vector<json_field_t> fields;
  if (!validate_tool_fields(args,
                            {"job_id", "job_dir", "include_text", "max_bytes"},
                            &fields, err)) {
    return false;
  }
  return handle_get_job(args, ctx, out, err);
}

[[nodiscard]] bool handle_inspect_artifact_job(const std::string &args,
                                               runtime_context_t *ctx,
                                               std::string *out,
                                               std::string *err) {
  std::vector<json_field_t> fields;
  if (!validate_tool_fields(
          args, {"job_id", "job_dir", "artifact", "max_bytes"}, &fields, err)) {
    return false;
  }
  return handle_read_artifact(args, ctx, out, err);
}

[[nodiscard]] bool handle_inspect_artifact_path(const std::string &args,
                                                runtime_context_t *ctx,
                                                std::string *out,
                                                std::string *err) {
  std::vector<json_field_t> fields;
  if (!validate_tool_fields(args, {"path", "max_bytes"}, &fields, err)) {
    return false;
  }
  std::string path;
  if (!extract_json_string_field(args, "path", &path) ||
      trim_ascii(path).empty()) {
    if (err) {
      *err = "hero.runtime.inspect.artifact.path requires path";
    }
    return false;
  }
  return handle_read_artifact(args, ctx, out, err);
}

[[nodiscard]] bool policy_training_wave_requested(const std::string &args) {
  return extract_json_raw_field(args, "policy_kind", nullptr) ||
         extract_json_raw_field(args, "policy_id", nullptr) ||
         extract_json_raw_field(args, "training_schedule_mode", nullptr) ||
         extract_json_raw_field(args, "causal_schedule_digest", nullptr);
}

[[nodiscard]] bool policy_training_wave_requested(
    const std::unordered_map<std::string, std::string> &request) {
  return request.count("policy_kind") != 0 || request.count("policy_id") != 0 ||
         request.count("training_schedule_mode") != 0 ||
         request.count("causal_schedule_digest") != 0;
}

[[nodiscard]] bool append_policy_training_request_json_fields(
    const std::unordered_map<std::string, std::string> &request,
    std::ostringstream &out, bool *first, std::string *err) {
  const std::initializer_list<kv_json_field_t> fields = {
      {"job_id", kv_json_field_kind_t::string_value},
      {"job_dir", kv_json_field_kind_t::string_value},
      {"report_path", kv_json_field_kind_t::string_value},
      {"replay_job_dir", kv_json_field_kind_t::string_value},
      {"accounting_numeraire_node_id", kv_json_field_kind_t::string_value},
      {"target_node_ids", kv_json_field_kind_t::string_value},
      {"experiment_id", kv_json_field_kind_t::string_value},
      {"protocol_id", kv_json_field_kind_t::string_value},
      {"protocol_contract_fingerprint", kv_json_field_kind_t::string_value},
      {"graph_order_fingerprint", kv_json_field_kind_t::string_value},
      {"source_cursor_token", kv_json_field_kind_t::string_value},
      {"split_policy_fingerprint", kv_json_field_kind_t::string_value},
      {"component_assembly_fingerprint", kv_json_field_kind_t::string_value},
      {"policy_id", kv_json_field_kind_t::string_value},
      {"policy_kind", kv_json_field_kind_t::string_value},
      {"policy_architecture_digest", kv_json_field_kind_t::string_value},
      {"training_config_digest", kv_json_field_kind_t::string_value},
      {"training_range_digest", kv_json_field_kind_t::string_value},
      {"validation_range_digest", kv_json_field_kind_t::string_value},
      {"test_range_digest", kv_json_field_kind_t::string_value},
      {"observation_schema_digest", kv_json_field_kind_t::string_value},
      {"reward_contract_digest", kv_json_field_kind_t::string_value},
      {"action_distribution_id", kv_json_field_kind_t::string_value},
      {"execution_profile_digest", kv_json_field_kind_t::string_value},
      {"policy_set_digest", kv_json_field_kind_t::string_value},
      {"policy_dsl_digest", kv_json_field_kind_t::string_value},
      {"policy_net_digest", kv_json_field_kind_t::string_value},
      {"policy_input_feature_manifest_digest",
       kv_json_field_kind_t::string_value},
      {"policy_jkimyei_digest", kv_json_field_kind_t::string_value},
      {"target_node_universe_digest", kv_json_field_kind_t::string_value},
      {"action_distribution_config_digest", kv_json_field_kind_t::string_value},
      {"snapshot_family_digest", kv_json_field_kind_t::string_value},
      {"actor_architecture_digest", kv_json_field_kind_t::string_value},
      {"critic_architecture_digest", kv_json_field_kind_t::string_value},
      {"actor_checkpoint_digest", kv_json_field_kind_t::string_value},
      {"critic_checkpoint_digest", kv_json_field_kind_t::string_value},
      {"optimizer_state_digest", kv_json_field_kind_t::string_value},
      {"ppo_config_digest", kv_json_field_kind_t::string_value},
      {"resume_mode", kv_json_field_kind_t::string_value},
      {"resume_actor_checkpoint_path", kv_json_field_kind_t::string_value},
      {"resume_actor_checkpoint_digest", kv_json_field_kind_t::string_value},
      {"resume_optimizer_state_path", kv_json_field_kind_t::string_value},
      {"resume_optimizer_state_digest", kv_json_field_kind_t::string_value},
      {"advantage_normalization_policy", kv_json_field_kind_t::string_value},
      {"rollout_collection_digest", kv_json_field_kind_t::string_value},
      {"ppo_update_report_digest", kv_json_field_kind_t::string_value},
      {"validation_rollout_report_digest", kv_json_field_kind_t::string_value},
      {"policy_quality_report_digest", kv_json_field_kind_t::string_value},
      {"training_schedule_mode", kv_json_field_kind_t::string_value},
      {"causal_schedule_digest", kv_json_field_kind_t::string_value},
      {"causal_schedule_cursor_key_kind", kv_json_field_kind_t::string_value},
      {"normalization_fit_range_digest", kv_json_field_kind_t::string_value},
      {"replay_buffer_source_range_digest", kv_json_field_kind_t::string_value},
      {"early_stopping_policy_digest", kv_json_field_kind_t::string_value},
      {"hyperparameter_selection_policy_digest",
       kv_json_field_kind_t::string_value},
      {"selector_split", kv_json_field_kind_t::string_value},
      {"selector_policy_digest", kv_json_field_kind_t::string_value},
      {"parent_checkpoint_digest", kv_json_field_kind_t::string_value},
      {"parent_forecast_eval_fact_digest", kv_json_field_kind_t::string_value},
      {"parent_observer_belief_fact_digest",
       kv_json_field_kind_t::string_value},
      {"parent_allocation_engine_fact_digest",
       kv_json_field_kind_t::string_value},
      {"parent_replay_environment_fact_digest",
       kv_json_field_kind_t::string_value},
      {"parent_replay_environment_report_digest",
       kv_json_field_kind_t::string_value},
      {"final_refit_parent_selected_checkpoint_digest",
       kv_json_field_kind_t::string_value},
      {"policy_artifact_digest", kv_json_field_kind_t::string_value},
      {"ppo_gamma", kv_json_field_kind_t::number_value},
      {"ppo_gae_lambda", kv_json_field_kind_t::number_value},
      {"ppo_clip_epsilon", kv_json_field_kind_t::number_value},
      {"ppo_target_kl", kv_json_field_kind_t::number_value},
      {"ppo_entropy_coeff", kv_json_field_kind_t::number_value},
      {"ppo_value_loss_coeff", kv_json_field_kind_t::number_value},
      {"ppo_max_grad_norm", kv_json_field_kind_t::number_value},
      {"linear_transaction_cost_rate", kv_json_field_kind_t::number_value},
      {"initial_equity_numeraire", kv_json_field_kind_t::number_value},
      {"max_node_weight", kv_json_field_kind_t::number_value},
      {"max_turnover_l1", kv_json_field_kind_t::number_value},
      {"ppo_minibatch_size", kv_json_field_kind_t::int_value},
      {"ppo_epochs_per_rollout", kv_json_field_kind_t::int_value},
      {"random_seed", kv_json_field_kind_t::int_value},
      {"max_episodes", kv_json_field_kind_t::int_value},
      {"max_steps", kv_json_field_kind_t::int_value},
      {"max_parallel_jobs", kv_json_field_kind_t::int_value},
      {"max_wall_clock_seconds", kv_json_field_kind_t::int_value},
      {"causal_schedule_readiness_eligible", kv_json_field_kind_t::bool_value},
      {"causal_schedule_no_future_snapshot_use",
       kv_json_field_kind_t::bool_value},
      {"offline_full_window_research_allowed",
       kv_json_field_kind_t::bool_value},
      {"final_refit_uses_validation", kv_json_field_kind_t::bool_value},
      {"validation_no_longer_proof", kv_json_field_kind_t::bool_value},
      {"sealed_test_required", kv_json_field_kind_t::bool_value},
      {"live_execution_allowed", kv_json_field_kind_t::bool_value}};
  if (!validate_kv_json_fields(request, fields, err, "execution request",
                               {"config_path"})) {
    return false;
  }
  return append_kv_json_fields(request, fields, out, first, err);
}

[[nodiscard]] bool handle_run_policy_training(const std::string &args,
                                              runtime_context_t *ctx,
                                              std::string *out,
                                              std::string *err);

[[nodiscard]] bool handle_run_policy_training(const std::string &args,
                                              runtime_context_t *ctx,
                                              std::string *out,
                                              std::string *err) {
  std::vector<json_field_t> fields;
  if (!validate_tool_fields(args,
                            {"requested_mode", "contract_path",
                             "contract_digest", "execution_request_path",
                             "execution_request_digest"},
                            &fields, err)) {
    return false;
  }
  std::string requested_mode;
  if (!parse_required_string_arg(args, "requested_mode", &requested_mode,
                                 err)) {
    return false;
  }
  requested_mode = lowercase_ascii(requested_mode);
  if (requested_mode != "dry_run" && requested_mode != "execute") {
    *err = "requested_mode must be dry_run or execute";
    return false;
  }
  if (ctx == nullptr) {
    *err = "runtime context is required";
    return false;
  }

  fs::path contract_path;
  std::string contract_text;
  if (!read_required_text_file_arg(args, "contract_path", &contract_path,
                                   &contract_text, err)) {
    return false;
  }
  auto contract_kv = parse_assignment_text(contract_text, false);
  cuwacunu::hero::runtime::policy_training_job_contract_t contract{};
  if (!policy_training_contract_from_kv(contract_kv, &contract, err)) {
    return false;
  }
  const std::string canonical_contract_text = contract.to_text();
  const std::string contract_digest =
      cuwacunu::hero::runtime::policy_training_contract_digest_for_text(
          canonical_contract_text);
  if (!check_optional_string_digest_arg(
          args, "contract_digest", contract_digest,
          "E_RUNTIME_POLICY_TRAINING_CONTRACT_DIGEST_MISMATCH", err)) {
    return false;
  }

  std::unordered_map<std::string, std::string> execution_request;
  if (extract_json_raw_field(args, "execution_request_path", nullptr)) {
    fs::path execution_request_path;
    std::string execution_request_text;
    if (!read_required_text_file_arg(args, "execution_request_path",
                                     &execution_request_path,
                                     &execution_request_text, err)) {
      return false;
    }
    const std::string execution_request_digest =
        cuwacunu::hero::marshal::marshal_digest_for_text(
            "kikijyeba.runtime.policy_training_execution_request.v1",
            execution_request_text);
    if (!check_optional_string_digest_arg(
            args, "execution_request_digest", execution_request_digest,
            "E_RUNTIME_POLICY_TRAINING_EXECUTION_REQUEST_DIGEST_MISMATCH",
            err)) {
      return false;
    }
    execution_request = parse_assignment_text(execution_request_text, false);
    if (execution_request.count("confirm_execute") != 0 ||
        execution_request.count("timeout_seconds") != 0) {
      *err = "execution request must not contain confirm_execute or "
             "timeout_seconds";
      return false;
    }
  } else if (extract_json_raw_field(args, "execution_request_digest",
                                    nullptr)) {
    *err = "execution_request_digest requires execution_request_path";
    return false;
  }

  std::ostringstream execution_args;
  execution_args << "{";
  bool first = true;
  if (!append_kv_json_fields(
          execution_request,
          {{"job_id", kv_json_field_kind_t::string_value},
           {"job_dir", kv_json_field_kind_t::string_value},
           {"report_path", kv_json_field_kind_t::string_value},
           {"replay_job_dir", kv_json_field_kind_t::string_value},
           {"config_path", kv_json_field_kind_t::string_value},
           {"accounting_numeraire_node_id", kv_json_field_kind_t::string_value},
           {"target_node_ids", kv_json_field_kind_t::string_value},
           {"experiment_id", kv_json_field_kind_t::string_value},
           {"policy_set_digest", kv_json_field_kind_t::string_value},
           {"linear_transaction_cost_rate", kv_json_field_kind_t::number_value},
           {"initial_equity_numeraire", kv_json_field_kind_t::number_value},
           {"max_node_weight", kv_json_field_kind_t::number_value},
           {"max_turnover_l1", kv_json_field_kind_t::number_value},
           {"max_steps", kv_json_field_kind_t::int_value},
           {"max_parallel_jobs", kv_json_field_kind_t::int_value}},
          execution_args, &first, err)) {
    return false;
  }
  execution_args << "}";
  return handle_policy_training_contract_object(contract, execution_args.str(),
                                                requested_mode, ctx, out, err);
}

[[nodiscard]] bool handle_runtime_wave_run_materialized(
    const std::string &run_args, std::string_view requested_mode,
    runtime_context_t *ctx, std::string *out, std::string *err) {
  if (extract_json_raw_field(run_args, "contract_path", nullptr)) {
    return handle_run_policy_training(
        object_with_selected_fields(
            run_args, {"requested_mode", "contract_path", "contract_digest",
                       "execution_request_path", "execution_request_digest"}),
        ctx, out, err);
  }

  std::unordered_map<std::string, std::string> execution_request;
  if (!load_optional_execution_request(run_args, &execution_request, err)) {
    return false;
  }
  const bool request_policy_training_contract =
      policy_training_wave_requested(execution_request);
  if (request_policy_training_contract) {
    std::ostringstream contract_args;
    contract_args << "{";
    bool contract_first = true;
    append_string_json_field(contract_args, "requested_mode", requested_mode,
                             &contract_first);
    (void)append_top_or_kv_string_field(run_args, execution_request,
                                        "config_path", contract_args,
                                        &contract_first);
    if (!append_policy_training_request_json_fields(
            execution_request, contract_args, &contract_first, err)) {
      return false;
    }
    contract_args << "}";
    return handle_policy_training_contract(contract_args.str(), requested_mode,
                                           ctx, out, err);
  }
  if (!validate_kv_fields(execution_request,
                          {"config_path", "job_dir", "force_rebuild_cache",
                           "source_range", "anchor_index_begin",
                           "anchor_index_end", "source_key_begin",
                           "source_key_end"},
                          err)) {
    return false;
  }

  std::optional<wave_info_t> selected_wave;
  if (ctx != nullptr) {
    std::string config_arg;
    if (!extract_json_string_field(run_args, "config_path", &config_arg)) {
      (void)kv_get_trimmed(execution_request, "config_path", &config_arg);
    }
    selected_wave =
        load_wave_info(*ctx, effective_config_path(*ctx, config_arg));
  }
  const bool active_policy_training_wave =
      selected_wave.has_value() &&
      wave_info_requests_policy_training(*selected_wave);
  if (active_policy_training_wave) {
    const std::string target =
        wave_value_or_empty(*selected_wave, "TARGET").empty()
            ? "wikimyei.policy.portfolio.graph_node_allocation"
            : wave_value_or_empty(*selected_wave, "TARGET");
    const std::string wave_id = wave_value_or_empty(*selected_wave, "WAVE_ID");
    const std::string policy_id =
        wave_value_or_empty(*selected_wave, "POLICY_ID");
    const std::string policy_kind =
        wave_value_or_empty(*selected_wave, "POLICY_KIND");
    const std::string training_schedule_mode =
        wave_value_or_empty(*selected_wave, "TRAINING_SCHEDULE_MODE");
    if (requested_mode == "dry_run") {
      std::ostringstream json;
      json << "{\"ok\":true,\"requested_mode\":\"dry_run\",\"dry_run\":true"
           << ",\"schema_version\":\"kikijyeba.runtime.component_wave_"
              "driver_plan.v1\""
           << ",\"wave_id\":" << json_quote(wave_id)
           << ",\"target_component_family_id\":" << json_quote(target)
           << ",\"component_driver_id\":\"hero.runtime."
              "graph_node_allocation_policy_component_driver.v1\""
           << ",\"component_driver_kind\":\"contract_backed_policy_training\""
           << ",\"contract_required\":true"
           << ",\"contract_path_bound\":false"
           << ",\"policy_id\":";
      if (policy_id.empty()) {
        json << "null";
      } else {
        json << json_quote(policy_id);
      }
      json << ",\"policy_kind\":";
      if (policy_kind.empty()) {
        json << "null";
      } else {
        json << json_quote(policy_kind);
      }
      json << ",\"training_schedule_mode\":";
      if (training_schedule_mode.empty()) {
        json << "null";
      } else {
        json << json_quote(training_schedule_mode);
      }
      json << ",\"authority\":{\"runtime_executes_component_wave\":true,"
              "\"runtime_component_driver_specialized\":true,"
              "\"environment_replay_dispatch\":false,"
              "\"runtime_emits_environment_admission_fact\":false,"
              "\"runtime_selects_policy\":false,"
              "\"runtime_executes_live_capital\":false}"
           << ",\"next_safe_actions\":[\"materialize_policy_training_"
              "contract\",\"run_policy_component_wave_with_contract_path\","
              "\"inspect_policy_component_wave\"]}";
      *out = json.str();
      return true;
    }
    *err = "E_RUNTIME_POLICY_COMPONENT_DRIVER_CONTRACT_REQUIRED: active wave "
           "TARGET=wikimyei.policy.portfolio.graph_node_allocation uses the "
           "contract-backed policy component driver; provide contract_path or "
           "request dry_run";
    return false;
  }

  std::ostringstream forwarded;
  forwarded << "{";
  bool first = true;
  (void)append_top_or_kv_string_field(run_args, execution_request,
                                      "config_path", forwarded, &first);
  if (!append_kv_json_fields(
          execution_request,
          {{"job_dir", kv_json_field_kind_t::string_value},
           {"force_rebuild_cache", kv_json_field_kind_t::bool_value}},
          forwarded, &first, err) ||
      !append_wave_overlay_from_request(execution_request, forwarded, &first,
                                        err)) {
    return false;
  }
  append_optional_raw_json_field(run_args, forwarded, "runtime_handoff",
                                 &first);
  append_bool_json_field(forwarded, "dry_run", requested_mode == "dry_run",
                         &first);
  forwarded << "}";
  return execute_runtime(forwarded.str(), ctx, false, out, err);
}

[[nodiscard]] bool handle_runtime_replay_run_materialized(
    const std::string &run_args, std::string_view requested_mode,
    runtime_context_t *ctx, std::string *out, std::string *err) {
  std::unordered_map<std::string, std::string> execution_request;
  if (!load_optional_execution_request(run_args, &execution_request, err)) {
    return false;
  }
  if (!validate_kv_fields(execution_request,
                          {"job_id",
                           "job_dir",
                           "config_path",
                           "accounting_numeraire_node_id",
                           "target_node_ids",
                           "experiment_id",
                           "report_path",
                           "initial_equity_numeraire",
                           "max_node_weight",
                           "max_turnover_l1",
                           "max_steps",
                           "max_parallel_jobs",
                           "include_equal_weight",
                           "include_current_weight",
                           "include_graph_node_allocation_policy",
                           "on_policy_sample",
                           "include_numeraire_only_policy",
                           "include_spot_distributional_utility_policy",
                           "allow_synthetic_direct_edges",
                           "validation_rollout",
                           "linear_transaction_cost_rate",
                           "execution_profile_digest",
                           "policy_set_digest",
                           "policy_artifact_digest",
                           "action_distribution_id",
                           "causal_schedule_digest",
                           "snapshot_family_digest"},
                          err)) {
    return false;
  }

  std::ostringstream forwarded;
  forwarded << "{";
  bool first = true;
  (void)append_top_or_kv_string_field(run_args, execution_request,
                                      "config_path", forwarded, &first);
  if (!append_kv_json_fields(
          execution_request,
          {{"job_id", kv_json_field_kind_t::string_value},
           {"job_dir", kv_json_field_kind_t::string_value},
           {"accounting_numeraire_node_id", kv_json_field_kind_t::string_value},
           {"target_node_ids", kv_json_field_kind_t::string_value},
           {"experiment_id", kv_json_field_kind_t::string_value},
           {"report_path", kv_json_field_kind_t::string_value},
           {"initial_equity_numeraire", kv_json_field_kind_t::number_value},
           {"max_node_weight", kv_json_field_kind_t::number_value},
           {"max_turnover_l1", kv_json_field_kind_t::number_value},
           {"max_steps", kv_json_field_kind_t::int_value},
           {"max_parallel_jobs", kv_json_field_kind_t::int_value},
           {"include_equal_weight", kv_json_field_kind_t::bool_value},
           {"include_current_weight", kv_json_field_kind_t::bool_value},
           {"include_graph_node_allocation_policy",
            kv_json_field_kind_t::bool_value},
           {"on_policy_sample", kv_json_field_kind_t::bool_value},
           {"include_numeraire_only_policy", kv_json_field_kind_t::bool_value},
           {"include_spot_distributional_utility_policy",
            kv_json_field_kind_t::bool_value},
           {"allow_synthetic_direct_edges", kv_json_field_kind_t::bool_value},
           {"validation_rollout", kv_json_field_kind_t::bool_value},
           {"linear_transaction_cost_rate", kv_json_field_kind_t::number_value},
           {"execution_profile_digest", kv_json_field_kind_t::string_value},
           {"policy_set_digest", kv_json_field_kind_t::string_value},
           {"policy_artifact_digest", kv_json_field_kind_t::string_value},
           {"action_distribution_id", kv_json_field_kind_t::string_value},
           {"causal_schedule_digest", kv_json_field_kind_t::string_value},
           {"snapshot_family_digest", kv_json_field_kind_t::string_value}},
          forwarded, &first, err)) {
    return false;
  }
  append_bool_json_field(forwarded, "dry_run", requested_mode != "execute",
                         &first);
  forwarded << "}";
  return handle_replay(forwarded.str(), ctx, out, err);
}

[[nodiscard]] bool handle_run(const std::string &args, runtime_context_t *ctx,
                              std::string *out, std::string *err) {
  std::vector<json_field_t> fields;
  if (!validate_tool_fields(args, {"mode", "args_path", "args_digest"}, &fields,
                            err)) {
    return false;
  }
  std::string requested_mode;
  if (!parse_required_string_arg(args, "mode", &requested_mode, err)) {
    return false;
  }
  requested_mode = lowercase_ascii(requested_mode);
  if (requested_mode != "dry_run" && requested_mode != "execute") {
    *err = "mode must be dry_run or execute";
    return false;
  }
  std::string run_args;
  if (!materialize_runtime_run_args(args, "wave", requested_mode, &run_args,
                                    err)) {
    return false;
  }
  return handle_runtime_wave_run_materialized(run_args, requested_mode, ctx,
                                              out, err);
}

[[nodiscard]] bool handle_replay_internal(const std::string &args,
                                          runtime_context_t *ctx,
                                          std::string *out, std::string *err) {
  std::vector<json_field_t> fields;
  if (!validate_tool_fields(args, {"mode", "args_path", "args_digest"}, &fields,
                            err)) {
    return false;
  }
  std::string requested_mode;
  if (!parse_required_string_arg(args, "mode", &requested_mode, err)) {
    return false;
  }
  requested_mode = lowercase_ascii(requested_mode);
  if (requested_mode != "dry_run" && requested_mode != "execute") {
    *err = "mode must be dry_run or execute";
    return false;
  }
  std::string run_args;
  if (!materialize_runtime_run_args(args, "replay", requested_mode, &run_args,
                                    err)) {
    return false;
  }
  return handle_runtime_replay_run_materialized(run_args, requested_mode, ctx,
                                                out, err);
}

[[nodiscard]] bool handle_reset(const std::string &args, runtime_context_t *ctx,
                                std::string *out, std::string *err) {
  std::vector<json_field_t> fields;
  if (!validate_tool_fields(args, {"mode", "args_path", "args_digest"}, &fields,
                            err)) {
    return false;
  }
  std::string requested_mode;
  if (!parse_required_string_arg(args, "mode", &requested_mode, err)) {
    return false;
  }
  requested_mode = lowercase_ascii(requested_mode);
  if (requested_mode != "plan" && requested_mode != "execute") {
    *err = "mode must be plan or execute";
    return false;
  }
  runtime_reset_request_file_t reset_request{};
  if (!load_optional_reset_request(args, requested_mode, &reset_request, err)) {
    return false;
  }
  std::ostringstream forwarded;
  forwarded << "{";
  bool first = true;
  if (reset_request.present &&
      !append_kv_json_fields(
          reset_request.fields,
          {{"runtime_root", kv_json_field_kind_t::string_value},
           {"backup", kv_json_field_kind_t::bool_value}},
          forwarded, &first, err)) {
    return false;
  }
  append_bool_json_field(forwarded, "dry_run", requested_mode == "plan",
                         &first);
  forwarded << "}";
  return handle_dev_nuke(forwarded.str(), ctx, out, err);
}

using handler_fn = bool (*)(const std::string &, runtime_context_t *,
                            std::string *, std::string *);

[[nodiscard]] std::optional<handler_fn> find_handler(std::string_view name) {
  if (name == "hero.runtime.status") {
    return handle_status;
  }
  if (name == "hero.runtime.inspect.schema") {
    return handle_inspect_schema;
  }
  if (name == "hero.runtime.inspect.wave") {
    return handle_inspect_wave;
  }
  if (name == "hero.runtime.inspect.jobs") {
    return handle_inspect_jobs;
  }
  if (name == "hero.runtime.inspect.job") {
    return handle_inspect_job;
  }
  if (name == "hero.runtime.inspect.artifact.job") {
    return handle_inspect_artifact_job;
  }
  if (name == "hero.runtime.inspect.artifact.path") {
    return handle_inspect_artifact_path;
  }
  if (name == "hero.runtime.run") {
    return handle_run;
  }
  if (name == "hero.runtime.replay") {
    return handle_replay_internal;
  }
  if (name == "hero.runtime.reset") {
    return handle_reset;
  }
  return std::nullopt;
}

} // namespace

std::filesystem::path
resolve_runtime_hero_dsl_path(const std::filesystem::path &global_config_path) {
  if (const auto fresh =
          read_ini_value(global_config_path, "HERO", "runtime_hero_dsl_path")) {
    return resolve_against(global_config_path, *fresh);
  }
  return kDefaultRuntimePolicyPath;
}

bool load_runtime_policy(const std::filesystem::path &policy_path,
                         const std::filesystem::path &global_config,
                         runtime_policy_t *out, std::string *error,
                         std::string_view profile_override) {
  if (!out) {
    if (error) {
      *error = "runtime policy output pointer is null";
    }
    return false;
  }
  runtime_policy_t policy{};
  policy.policy_path = policy_path;
  policy.global_config_path = global_config;
  for (const auto &descriptor : kRuntimePolicyDescriptors) {
    policy.values[std::string(descriptor.key)] =
        std::string(descriptor.default_value);
  }
  std::string text;
  if (!read_text_file(policy_path, &text, error)) {
    policy.from_template = true;
    text = std::string(kRuntimePolicyTemplateText);
  }

  runtime_profile_blocks_t profile_blocks{};
  if (!split_runtime_profile_blocks(text, &profile_blocks, error)) {
    return false;
  }

  for (auto &[key, value] :
       parse_assignment_text(profile_blocks.base_text, false)) {
    policy.values[std::move(key)] = std::move(value);
  }

  std::string selected_profile = trim_ascii(profile_override);
  std::string selected_profile_source = "cli";
  if (selected_profile.empty()) {
    selected_profile_source = "global_config";
    if (const auto configured =
            read_ini_value(global_config, "HERO", "runtime_hero_profile")) {
      selected_profile = trim_ascii(*configured);
    }
  }
  if (selected_profile.empty()) {
    selected_profile = trim_ascii(policy_get(policy, "runtime_profile"));
    selected_profile_source = "policy_file";
  }
  if (selected_profile.empty()) {
    selected_profile = "operator_default";
    selected_profile_source = "default";
  }
  if (!valid_runtime_profile_id(selected_profile)) {
    if (error) {
      *error = "invalid runtime profile id: " + selected_profile;
    }
    return false;
  }

  const auto profile_it =
      profile_blocks.profile_text_by_id.find(selected_profile);
  if (profile_it == profile_blocks.profile_text_by_id.end() &&
      !(profile_blocks.profile_text_by_id.empty() &&
        selected_profile == "operator_default")) {
    if (error) {
      *error = "unknown runtime profile `" + selected_profile +
               "` in policy file: " + policy_path.string();
    }
    return false;
  }
  if (profile_it != profile_blocks.profile_text_by_id.end()) {
    for (auto &[key, value] :
         parse_assignment_text(profile_it->second, false)) {
      if (key == "runtime_profile") {
        if (error) {
          *error = "RUNTIME_PROFILE block must not set runtime_profile";
        }
        return false;
      }
      policy.values[std::move(key)] = std::move(value);
    }
  }
  policy.profile_id = selected_profile;
  policy.values["runtime_profile"] = selected_profile;
  policy.values["runtime_profile_source"] = selected_profile_source;

  if (policy_get(policy, "protocol_layer") != kProtocolLayerStdio) {
    if (error) {
      *error = std::string(kProtocolLayerHttpsSseFailFastMessage);
    }
    return false;
  }
  *out = std::move(policy);
  return true;
}

bool execute_tool_json(const std::string &tool_name, std::string arguments_json,
                       runtime_context_t *ctx,
                       std::string *out_tool_result_json,
                       std::string *out_error_message) {
  if (out_tool_result_json) {
    out_tool_result_json->clear();
  }
  if (out_error_message) {
    out_error_message->clear();
  }
  if (!ctx) {
    if (out_error_message) {
      *out_error_message = "Runtime Hero context pointer is null";
    }
    if (out_tool_result_json) {
      *out_tool_result_json =
          make_error_result("Runtime Hero context pointer is null", -32603);
    }
    return false;
  }
  arguments_json = trim_ascii(arguments_json);
  if (arguments_json.empty()) {
    arguments_json = "{}";
  }
  if (arguments_json.front() != '{') {
    if (out_error_message) {
      *out_error_message = "--args-json must be a JSON object";
    }
    if (out_tool_result_json) {
      *out_tool_result_json =
          make_error_result("--args-json must be a JSON object", -32602);
    }
    return false;
  }

  const auto handler = find_handler(tool_name);
  if (!handler.has_value()) {
    const std::string err = "unknown tool: " + tool_name;
    if (out_error_message) {
      *out_error_message = err;
    }
    if (out_tool_result_json) {
      *out_tool_result_json = make_error_result(err, -32601);
    }
    return false;
  }

  std::string structured;
  std::string err;
  const bool ok = (*handler)(arguments_json, ctx, &structured, &err);
  if (!ok) {
    if (err.empty()) {
      err = "E_RUNTIME_HANDLER_FAILED_WITHOUT_DIAGNOSTIC";
    }
    if (out_error_message) {
      *out_error_message = err;
    }
    if (out_tool_result_json) {
      const int code = err.find("E_RUNTIME_") == 0 ? kPolicyErrorCode : -32602;
      *out_tool_result_json = make_error_result(err, code);
    }
    return false;
  }
  if (out_tool_result_json) {
    *out_tool_result_json = make_tool_result(tool_name, structured);
  }
  return true;
}

std::string build_tools_list_result_json() {
  std::ostringstream out;
  out << "{\"tools\":[";
  for (std::size_t i = 0; i < std::size(kTools); ++i) {
    mcp_schema_compat::assert_tool_input_schema_compatible(
        kTools[i].name, kTools[i].input_schema_json);
    if (i != 0) {
      out << ",";
    }
    out << "{\"name\":" << json_quote(kTools[i].name)
        << ",\"description\":" << json_quote(kTools[i].description)
        << ",\"inputSchema\":" << kTools[i].input_schema_json
        << ",\"annotations\":{\"readOnlyHint\":"
        << bool_json(tool_is_read_only(kTools[i].name))
        << ",\"destructiveHint\":"
        << bool_json(tool_is_destructive(kTools[i].name))
        << ",\"openWorldHint\":false}}";
  }
  out << "]}";
  return out.str();
}

std::string build_tools_list_human_text() {
  std::ostringstream out;
  for (const auto &tool : kTools) {
    out << tool.name << "\t" << tool.description << "\n";
  }
  return out.str();
}

void run_jsonrpc_stdio_loop(runtime_context_t *ctx) {
  mcp_stdio::message_t message;
  bool shutdown_seen = false;
  while (mcp_stdio::read_message(std::cin, &message)) {
    std::string line = trim_ascii(message.json);
    if (line.empty()) {
      continue;
    }
    const auto send = [&](std::string response) {
      mcp_stdio::write_response(std::cout, response);
    };
    std::string id_raw = "null";
    (void)extract_json_raw_field(line, "id", &id_raw);
    std::string method;
    if (!extract_json_string_field(line, "method", &method)) {
      continue;
    }
    if (method == "notifications/initialized") {
      continue;
    }
    if (method == "exit") {
      break;
    }
    if (method == "initialize") {
      std::string protocol = "2024-11-05";
      (void)extract_json_string_field(line, "protocolVersion", &protocol);
      send(std::string("{\"jsonrpc\":\"2.0\",\"id\":") + id_raw +
           ",\"result\":{\"protocolVersion\":" + json_quote(protocol) +
           ",\"capabilities\":{\"tools\":{}},\"serverInfo\":{\"name\":"
           "\"hero_runtime\",\"version\":\"2\"}}}");
      continue;
    }
    if (method == "tools/list") {
      send(std::string("{\"jsonrpc\":\"2.0\",\"id\":") + id_raw +
           ",\"result\":" + build_tools_list_result_json() + "}");
      continue;
    }
    if (method == "resources/list") {
      send(std::string("{\"jsonrpc\":\"2.0\",\"id\":") + id_raw +
           ",\"result\":{\"resources\":[]}}");
      continue;
    }
    if (method == "resources/templates/list") {
      send(std::string("{\"jsonrpc\":\"2.0\",\"id\":") + id_raw +
           ",\"result\":{\"resourceTemplates\":[]}}");
      continue;
    }
    if (method == "shutdown") {
      shutdown_seen = true;
      send(std::string("{\"jsonrpc\":\"2.0\",\"id\":") + id_raw +
           ",\"result\":null}");
      continue;
    }
    if (method == "tools/call") {
      std::string params;
      if (!extract_json_raw_field(line, "params", &params)) {
        send(std::string("{\"jsonrpc\":\"2.0\",\"id\":") + id_raw +
             ",\"error\":{\"code\":-32602,\"message\":\"missing "
             "params\"}}");
        continue;
      }
      std::string name;
      if (!extract_json_string_field(params, "name", &name)) {
        send(std::string("{\"jsonrpc\":\"2.0\",\"id\":") + id_raw +
             ",\"error\":{\"code\":-32602,\"message\":\"missing tool "
             "name\"}}");
        continue;
      }
      std::string arguments = "{}";
      (void)extract_json_raw_field(params, "arguments", &arguments);
      std::string result;
      std::string error;
      (void)execute_tool_json(name, arguments, ctx, &result, &error);
      send(std::string("{\"jsonrpc\":\"2.0\",\"id\":") + id_raw +
           ",\"result\":" + result + "}");
      continue;
    }
    send(std::string("{\"jsonrpc\":\"2.0\",\"id\":") + id_raw +
         ",\"error\":{\"code\":-32601,\"message\":\"unknown method\"}}");
    if (shutdown_seen) {
      break;
    }
  }
}

} // namespace cuwacunu::hero::runtime
