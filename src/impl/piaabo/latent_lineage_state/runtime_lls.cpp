#include "piaabo/latent_lineage_state/runtime_lls.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state.h"
#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state_lhs.h"
#include "piaabo/dfiles.h"

namespace {

using entry_t = cuwacunu::piaabo::latent_lineage_state::runtime_lls_entry_t;
using document_t = cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t;

[[nodiscard]] std::string trim_ascii_copy(std::string_view text) {
  return cuwacunu::camahjucunu::dsl::trim_ascii_ws_copy(text);
}

void clear_error(std::string* error) {
  if (error) error->clear();
}

void set_error(std::string* error, std::string message) {
  if (error) *error = std::move(message);
}

[[nodiscard]] bool is_ascii_letter(char ch) {
  return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
}

[[nodiscard]] bool is_ascii_digit(char ch) {
  return ch >= '0' && ch <= '9';
}

[[nodiscard]] bool is_identifier(std::string_view text) {
  if (text.empty()) return false;
  if (!(is_ascii_letter(text.front()) || text.front() == '_')) return false;
  for (const char ch : text.substr(1)) {
    if (is_ascii_letter(ch) || is_ascii_digit(ch) || ch == '_' || ch == '.' ||
        ch == '-') {
      continue;
    }
    return false;
  }
  return true;
}

[[nodiscard]] bool is_supported_declared_type(std::string_view text) {
  const std::string value = trim_ascii_copy(text);
  return value == "str" || value == "bool" || value == "int" ||
         value == "uint" ||
         value == "double";
}

[[nodiscard]] bool is_runtime_symbol_char(char ch) {
  switch (ch) {
    case '.':
    case ',':
    case ':':
    case '-':
    case '_':
    case '\\':
    case '/':
    case '[':
    case ']':
    case '(':
    case ')':
    case '{':
    case '}':
    case '|':
    case '\'':
    case '"':
    case '+':
    case '*':
    case '?':
    case '!':
    case '<':
    case '>':
    case '=':
    case '@':
      return true;
    default:
      return false;
  }
}

[[nodiscard]] bool is_runtime_value_char(char ch) {
  return is_ascii_letter(ch) || is_ascii_digit(ch) || ch == ' ' || ch == '\t' ||
         is_runtime_symbol_char(ch);
}

[[nodiscard]] bool is_runtime_range_domain_char(char ch) {
  if (is_ascii_letter(ch) || is_ascii_digit(ch) || ch == ' ' || ch == '\t') {
    return true;
  }
  switch (ch) {
    case '.':
    case ',':
    case ':':
    case '-':
    case '_':
    case '\\':
    case '/':
    case '[':
    case ']':
    case '{':
    case '}':
    case '|':
    case '\'':
    case '"':
    case '+':
    case '*':
    case '?':
    case '!':
    case '<':
    case '>':
    case '=':
    case '@':
      return true;
    default:
      return false;
  }
}

[[nodiscard]] bool is_runtime_enum_domain_char(char ch) {
  if (is_ascii_letter(ch) || is_ascii_digit(ch) || ch == ' ' || ch == '\t') {
    return true;
  }
  switch (ch) {
    case '.':
    case ',':
    case ':':
    case '-':
    case '_':
    case '\\':
    case '/':
    case '(':
    case ')':
    case '{':
    case '}':
    case '|':
    case '\'':
    case '"':
    case '+':
    case '*':
    case '?':
    case '!':
    case '<':
    case '>':
    case '=':
    case '@':
      return true;
    default:
      return false;
  }
}

[[nodiscard]] const char* find_runtime_comment_delimiter(std::string_view text) {
  for (std::size_t i = 0; i + 1 < text.size(); ++i) {
    if (text[i] == '/' && text[i + 1] == '*') return "/*";
    if (text[i] == '*' && text[i + 1] == '/') return "*/";
  }
  return nullptr;
}

[[nodiscard]] bool validate_runtime_raw_value_text(std::string_view key,
                                                   std::string_view value,
                                                   std::string* error) {
  clear_error(error);
  if (value.find('\n') != std::string_view::npos ||
      value.find('\r') != std::string_view::npos) {
    set_error(error,
              "runtime .lls value for key '" + std::string(key) +
                  "' must be single-line");
    return false;
  }
  if (const char* marker = find_runtime_comment_delimiter(value); marker != nullptr) {
    set_error(error,
              "runtime .lls value for key '" + std::string(key) +
                  "' must not contain comment delimiter " + marker);
    return false;
  }
  for (const char ch : value) {
    if (is_runtime_value_char(ch)) continue;
    set_error(error,
              "runtime .lls value for key '" + std::string(key) +
                  "' contains non-canonical character");
    return false;
  }
  return true;
}

[[nodiscard]] bool is_supported_domain(std::string_view text) {
  const std::string value = trim_ascii_copy(text);
  if (value.empty()) return true;
  if (value.size() < 2) return false;
  if (value.find('\n') != std::string::npos || value.find('\r') != std::string::npos) {
    return false;
  }
  if (find_runtime_comment_delimiter(value) != nullptr) return false;
  if (value.front() == '(' && value.back() == ')') {
    for (std::size_t i = 1; i + 1 < value.size(); ++i) {
      if (!is_runtime_range_domain_char(value[i])) return false;
    }
    return true;
  }
  if (value.front() == '[' && value.back() == ']') {
    for (std::size_t i = 1; i + 1 < value.size(); ++i) {
      if (!is_runtime_enum_domain_char(value[i])) return false;
    }
    return true;
  }
  return false;
}

[[nodiscard]] std::string lower_ascii_copy(std::string_view text) {
  return cuwacunu::camahjucunu::dsl::lowercase_ascii_copy(text);
}

[[nodiscard]] bool parse_u64_ascii(std::string_view text, std::uint64_t* out) {
  if (!out) return false;
  const std::string value = trim_ascii_copy(text);
  if (value.empty()) return false;
  std::uint64_t parsed = 0;
  const auto r =
      std::from_chars(value.data(), value.data() + value.size(), parsed);
  if (r.ec != std::errc{} || r.ptr != value.data() + value.size()) return false;
  *out = parsed;
  return true;
}

[[nodiscard]] std::string infer_runtime_declared_type_from_value(
    std::string_view value) {
  const std::string token = trim_ascii_copy(value);
  if (token.empty()) return "str";
  bool as_bool = false;
  if (cuwacunu::camahjucunu::dsl::parse_bool_ascii(token, &as_bool)) {
    return "bool";
  }
  std::int64_t as_i64 = 0;
  if (cuwacunu::camahjucunu::dsl::parse_i64_ascii(token, &as_i64)) return "int";
  std::uint64_t as_u64 = 0;
  if (parse_u64_ascii(token, &as_u64)) return "uint";
  double as_f64 = 0.0;
  if (cuwacunu::camahjucunu::dsl::parse_double_ascii(token, &as_f64)) {
    return "double";
  }
  return "str";
}

[[nodiscard]] std::string block_comments_only(std::string_view text,
                                              std::string* error) {
  clear_error(error);
  std::string out;
  out.reserve(text.size());

  bool in_block_comment = false;

  for (std::size_t i = 0; i < text.size();) {
    const char c = text[i];
    const char next = (i + 1 < text.size()) ? text[i + 1] : '\0';

    if (in_block_comment) {
      if (c == '\n') out.push_back('\n');
      if (c == '\r') out.push_back('\r');
      if (c == '*' && next == '/') {
        in_block_comment = false;
        i += 2;
      } else {
        ++i;
      }
      continue;
    }

    if (c == '/' && next == '*') {
      in_block_comment = true;
      i += 2;
      continue;
    }

    out.push_back(c);
    ++i;
  }

  if (in_block_comment) {
    set_error(error, "unterminated /* ... */ comment in runtime .lls payload");
    return {};
  }
  return out;
}

[[nodiscard]] bool contains_forbidden_legacy_comment_markers(
    std::string_view text,
    std::string* error) {
  clear_error(error);
  for (std::size_t i = 0; i < text.size(); ++i) {
    const char c = text[i];
    if (c == '#') {
      set_error(error, "runtime .lls accepts only /* ... */ comments; found '#'");
      return true;
    }
    if (c == ';') {
      set_error(error, "runtime .lls accepts only /* ... */ comments; found ';'");
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool is_schema_key(std::string_view key) {
  const std::string trimmed = trim_ascii_copy(key);
  if (trimmed == "schema") return true;
  return trimmed.size() > 7 &&
         trimmed.compare(trimmed.size() - 7, 7, ".schema") == 0;
}

[[nodiscard]] bool is_versioned_schema_value(std::string_view value) {
  const std::string token = trim_ascii_copy(value);
  const std::size_t version_pos = token.rfind(".v");
  if (version_pos == std::string::npos || version_pos + 2 >= token.size()) {
    return false;
  }
  if (!is_identifier(std::string_view(token).substr(0, version_pos))) {
    return false;
  }
  for (std::size_t i = version_pos + 2; i < token.size(); ++i) {
    if (!is_ascii_digit(token[i])) return false;
  }
  return true;
}

[[nodiscard]] int canonical_key_rank(std::string_view key) {
  const std::string k = trim_ascii_copy(key);
  if (is_schema_key(k)) return 0;
  if (k == "report_kind") return 1;
  if (k == "tsi_type") return 2;
  if (k == "canonical_path") return 3;
  if (k == "hashimyei") return 4;
  if (k == "contract_hash") return 5;
  if (k == "wave_hash") return 6;
  if (k == "binding_id") return 7;
  if (k == "run_id") return 8;
  return 100;
}

[[nodiscard]] std::string canonical_value_for_type(const entry_t& entry,
                                                   std::string* error) {
  clear_error(error);
  const std::string type = trim_ascii_copy(entry.declared_type);
  const std::string lowered = lower_ascii_copy(type);
  const std::string raw_value = trim_ascii_copy(entry.value);

  if (lowered == "str") {
    if (!validate_runtime_raw_value_text(entry.key, raw_value, error)) {
      return {};
    }
    return raw_value;
  }
  if (lowered == "bool") {
    bool parsed = false;
    if (!cuwacunu::camahjucunu::dsl::parse_bool_ascii(raw_value, &parsed)) {
      set_error(error, "invalid bool value for key '" + entry.key + "'");
      return {};
    }
    return parsed ? "true" : "false";
  }
  if (lowered == "int") {
    std::int64_t parsed = 0;
    if (!cuwacunu::camahjucunu::dsl::parse_i64_ascii(raw_value, &parsed)) {
      set_error(error, "invalid int value for key '" + entry.key + "'");
      return {};
    }
    return std::to_string(parsed);
  }
  if (lowered == "uint") {
    std::uint64_t parsed = 0;
    if (!parse_u64_ascii(raw_value, &parsed)) {
      set_error(error, "invalid uint value for key '" + entry.key + "'");
      return {};
    }
    return std::to_string(parsed);
  }
  if (lowered == "double") {
    double parsed = 0.0;
    if (!cuwacunu::camahjucunu::dsl::parse_double_ascii(raw_value, &parsed)) {
      set_error(error, "invalid double value for key '" + entry.key + "'");
      return {};
    }
    if (!std::isfinite(parsed)) {
      set_error(error, "non-finite double value for key '" + entry.key + "'");
      return {};
    }
    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss << std::setprecision(12) << parsed;
    return oss.str();
  }

  set_error(error, "unsupported declared_type for key '" + entry.key + "'");
  return {};
}

[[nodiscard]] bool validate_entries(const std::vector<entry_t>& entries,
                                    bool require_schema_first,
                                    std::string* error) {
  clear_error(error);
  if (entries.empty()) {
    set_error(error, "runtime .lls payload must contain at least one assignment");
    return false;
  }

  std::unordered_set<std::string> seen_keys{};
  bool seen_schema = false;
  bool first_entry_is_schema = false;

  for (std::size_t i = 0; i < entries.size(); ++i) {
    const entry_t& entry = entries[i];
    const std::string key = trim_ascii_copy(entry.key);
    const std::string type = trim_ascii_copy(entry.declared_type);
    const std::string domain = trim_ascii_copy(entry.declared_domain);

    if (key.empty()) {
      set_error(error, "runtime .lls entry has empty key");
      return false;
    }
    if (!is_identifier(key)) {
      set_error(error, "runtime .lls key is not a valid identifier: " + key);
      return false;
    }
    if (type.empty()) {
      set_error(error, "runtime .lls entry '" + key + "' is missing declared_type");
      return false;
    }
    if (!is_supported_declared_type(type)) {
      set_error(error, "runtime .lls entry '" + key +
                           "' uses unsupported declared_type: " + type);
      return false;
    }
    if (!is_supported_domain(domain)) {
      set_error(error, "runtime .lls entry '" + key +
                           "' uses invalid declared_domain: " + domain);
      return false;
    }

    if (i == 0) {
      first_entry_is_schema = is_schema_key(key);
    }

    if (!seen_keys.insert(key).second) {
      set_error(error, "runtime .lls payload contains duplicate key: " + key);
      return false;
    }

    if (is_schema_key(key)) {
      if (seen_schema) {
        set_error(error, "runtime .lls payload contains duplicate schema entry");
        return false;
      }
      seen_schema = true;
      if (lower_ascii_copy(type) != "str") {
        set_error(error, "runtime .lls schema entry must use declared_type str");
        return false;
      }
      const std::string schema_value = trim_ascii_copy(entry.value);
      if (schema_value.empty()) {
        set_error(error, "runtime .lls schema value must be non-empty");
        return false;
      }
      if (!is_versioned_schema_value(schema_value)) {
        set_error(error,
                  "runtime .lls schema value must use .v<integer> suffix");
        return false;
      }
    }

    std::string value_error;
    (void)canonical_value_for_type(
        {key, domain, type, entry.value}, &value_error);
    if (!value_error.empty()) {
      set_error(error, std::move(value_error));
      return false;
    }
  }

  if (!seen_schema) {
    set_error(error, "runtime .lls payload is missing schema entry");
    return false;
  }
  if (require_schema_first && !first_entry_is_schema) {
    set_error(error,
              "runtime .lls payload must declare schema on the first assignment");
    return false;
  }
  return true;
}

[[nodiscard]] std::vector<entry_t> to_runtime_entries(
    const cuwacunu::camahjucunu::latent_lineage_state_instruction_t& decoded) {
  std::vector<entry_t> out;
  out.reserve(decoded.entries.size());
  for (const auto& entry : decoded.entries) {
    out.push_back(
        {.key = entry.key,
         .declared_domain = entry.declared_domain,
         .declared_type = entry.declared_type,
         .value = entry.value});
  }
  return out;
}

[[nodiscard]] bool parse_runtime_entries_strict(std::string_view text,
                                                std::vector<entry_t>* out,
                                                std::string* error) {
  clear_error(error);
  if (!out) {
    set_error(error, "runtime .lls parse output pointer is null");
    return false;
  }
  out->clear();

  std::string preflight_error;
  std::string stripped = block_comments_only(text, &preflight_error);
  if (!preflight_error.empty()) {
    set_error(error, std::move(preflight_error));
    return false;
  }

  std::string comment_error;
  if (contains_forbidden_legacy_comment_markers(stripped, &comment_error)) {
    set_error(error, std::move(comment_error));
    return false;
  }

  try {
    const auto decoded =
        cuwacunu::camahjucunu::dsl::decode_latent_lineage_state_from_dsl(
            cuwacunu::piaabo::latent_lineage_state::runtime_lls_grammar_text(),
            stripped);
    std::vector<entry_t> parsed = to_runtime_entries(decoded);
    if (!validate_entries(parsed, true, error)) return false;
    *out = std::move(parsed);
    return true;
  } catch (const std::exception& e) {
    set_error(error, std::string("runtime .lls parse failure: ") + e.what());
    return false;
  }
}

[[nodiscard]] std::vector<entry_t> canonicalized_entries(const document_t& document) {
  std::vector<entry_t> entries = document.entries;
  for (auto& entry : entries) {
    entry.key = trim_ascii_copy(entry.key);
    entry.declared_type = trim_ascii_copy(entry.declared_type);
    entry.declared_domain = trim_ascii_copy(entry.declared_domain);
    entry.value = trim_ascii_copy(entry.value);

    if (entry.declared_type.empty()) {
      entry.declared_type = infer_runtime_declared_type_from_value(entry.value);
    }
    if (entry.declared_domain.empty()) {
      entry.declared_domain =
          cuwacunu::camahjucunu::dsl::default_domain_for_declared_type(
              entry.declared_type);
    }
  }

  std::sort(entries.begin(), entries.end(), [](const entry_t& a, const entry_t& b) {
    const int ra = canonical_key_rank(a.key);
    const int rb = canonical_key_rank(b.key);
    if (ra != rb) return ra < rb;
    return a.key < b.key;
  });
  return entries;
}

}  // namespace

namespace cuwacunu {
namespace piaabo {
namespace latent_lineage_state {

std::string runtime_lls_grammar_text() {
  return cuwacunu::piaabo::dfiles::readFileToString(
      std::string(kRuntimeLlsGrammarPath));
}

bool validate_runtime_lls_text(std::string_view text, std::string* error) {
  std::vector<entry_t> entries{};
  return parse_runtime_entries_strict(text, &entries, error);
}

bool parse_runtime_lls_text(std::string_view text,
                            runtime_lls_document_t* out,
                            std::string* error) {
  clear_error(error);
  if (!out) {
    set_error(error, "runtime .lls document output pointer is null");
    return false;
  }
  *out = runtime_lls_document_t{};

  std::vector<entry_t> entries{};
  if (!parse_runtime_entries_strict(text, &entries, error)) return false;
  runtime_lls_document_t parsed{};
  parsed.entries = std::move(entries);
  out->entries = canonicalized_entries(parsed);
  return true;
}

bool validate_runtime_lls_document(const runtime_lls_document_t& document,
                                   std::string* error) {
  return validate_entries(canonicalized_entries(document), false, error);
}

bool runtime_lls_document_to_kv_map(
    const runtime_lls_document_t& document,
    std::unordered_map<std::string, std::string>* out,
    std::string* error) {
  clear_error(error);
  if (!out) {
    set_error(error, "runtime .lls kv output pointer is null");
    return false;
  }
  out->clear();

  const std::vector<entry_t> entries = canonicalized_entries(document);
  if (!validate_entries(entries, false, error)) return false;

  for (const auto& entry : entries) {
    std::string value_error;
    const std::string value = canonical_value_for_type(entry, &value_error);
    if (!value_error.empty()) {
      set_error(error, std::move(value_error));
      out->clear();
      return false;
    }
    (*out)[entry.key] = value;
  }
  return true;
}

std::string emit_runtime_lls_canonical(const runtime_lls_document_t& document) {
  std::string error;
  const std::vector<entry_t> entries = canonicalized_entries(document);
  if (!validate_entries(entries, false, &error)) {
    throw std::runtime_error(error);
  }

  std::ostringstream out;
  for (const auto& entry : entries) {
    std::string value_error;
    const std::string value = canonical_value_for_type(entry, &value_error);
    if (!value_error.empty()) {
      throw std::runtime_error(value_error);
    }

    out << entry.key;
    if (!entry.declared_domain.empty()) out << entry.declared_domain;
    out << ":" << entry.declared_type << " = " << value << "\n";
  }
  return out.str();
}

runtime_lls_entry_t make_runtime_lls_entry(std::string key,
                                           std::string value,
                                           std::string declared_type,
                                           std::string declared_domain) {
  return {
      .key = std::move(key),
      .declared_domain = std::move(declared_domain),
      .declared_type = std::move(declared_type),
      .value = std::move(value),
  };
}

runtime_lls_entry_t make_runtime_lls_string_entry(std::string key,
                                                  std::string value) {
  return make_runtime_lls_entry(std::move(key), std::move(value), "str");
}

runtime_lls_entry_t make_runtime_lls_bool_entry(std::string key, bool value) {
  return make_runtime_lls_entry(std::move(key), value ? "true" : "false", "bool");
}

runtime_lls_entry_t make_runtime_lls_int_entry(std::string key,
                                               std::int64_t value,
                                               std::string declared_domain) {
  return make_runtime_lls_entry(
      std::move(key), std::to_string(value), "int", std::move(declared_domain));
}

runtime_lls_entry_t make_runtime_lls_uint_entry(std::string key,
                                                std::uint64_t value,
                                                std::string declared_domain) {
  return make_runtime_lls_entry(
      std::move(key), std::to_string(value), "uint", std::move(declared_domain));
}

runtime_lls_entry_t make_runtime_lls_double_entry(std::string key,
                                                  double value,
                                                  std::string declared_domain) {
  std::ostringstream oss;
  oss.setf(std::ios::fixed);
  oss << std::setprecision(12) << value;
  return make_runtime_lls_entry(
      std::move(key), oss.str(), "double", std::move(declared_domain));
}

}  // namespace latent_lineage_state
}  // namespace piaabo
}  // namespace cuwacunu
