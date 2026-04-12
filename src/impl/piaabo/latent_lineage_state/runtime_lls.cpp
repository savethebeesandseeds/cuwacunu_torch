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
#include "camahjucunu/db/idydb.h"
#include "piaabo/dfiles.h"
#include "piaabo/latent_lineage_state/report_taxonomy.h"

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

[[nodiscard]] bool is_known_runtime_report_semantic_taxon(
    std::string_view semantic_taxon) {
  using namespace cuwacunu::piaabo::latent_lineage_state::report_taxonomy;
  return semantic_taxon == kSourceData ||
         semantic_taxon == kEmbeddingData ||
         semantic_taxon == kEmbeddingEvaluation ||
         semantic_taxon == kEmbeddingNetwork;
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
  const bool interval_domain =
      (value.front() == '(' || value.front() == '[') &&
      (value.back() == ')' || value.back() == ']') &&
      value.find(',') != std::string::npos;
  if (interval_domain) {
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
  if (k == "semantic_taxon") return 1;
  if (k == "canonical_path") return 2;
  if (k == "binding_id") return 3;
  if (k == "wave_cursor") return 4;
  if (k == "source_runtime_cursor") return 5;
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
    static cuwacunu::camahjucunu::dsl::latentLineageStatePipeline pipeline(
        cuwacunu::piaabo::latent_lineage_state::runtime_lls_grammar_text());
    const auto decoded = pipeline.decode(stripped);
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
  static const std::string grammar_text =
      cuwacunu::piaabo::dfiles::readFileToString(
          std::string(kRuntimeLlsGrammarPath));
  return grammar_text;
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

bool parse_runtime_lls_text_fast_to_kv_map(
    std::string_view text,
    std::unordered_map<std::string, std::string>* out,
    std::string* error) {
  clear_error(error);
  if (!out) {
    set_error(error, "runtime .lls kv output pointer is null");
    return false;
  }
  out->clear();

  bool saw_schema = false;
  std::size_t cursor = 0;
  while (cursor < text.size()) {
    std::size_t line_end = text.find('\n', cursor);
    if (line_end == std::string_view::npos) line_end = text.size();
    std::string_view line = text.substr(cursor, line_end - cursor);
    if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
    const std::string trimmed = trim_ascii_copy(line);
    if (!trimmed.empty() && trimmed.front() != '#') {
      const std::size_t eq = trimmed.find('=');
      if (eq == std::string_view::npos || eq == 0) {
        set_error(error, "runtime .lls parse failure: invalid key/value line");
        out->clear();
        return false;
      }
      const std::string key =
          cuwacunu::camahjucunu::dsl::extract_latent_lineage_state_lhs_key(
              trim_ascii_copy(trimmed.substr(0, eq)));
      if (key.empty()) {
        set_error(error, "runtime .lls parse failure: invalid lhs key");
        out->clear();
        return false;
      }
      std::string value = trim_ascii_copy(trimmed.substr(eq + 1));
      (*out)[key] = value;
      if (key == "schema" && !value.empty()) saw_schema = true;
    }
    if (line_end == text.size()) break;
    cursor = line_end + 1;
  }

  if (!saw_schema) {
    set_error(error, "persisted runtime .lls requires top-level schema key");
    out->clear();
    return false;
  }
  return true;
}

std::string normalize_runtime_report_semantic_taxon(std::string_view semantic_taxon,
                                                    std::string_view schema) {
  (void)schema;
  return trim_ascii_copy(semantic_taxon);
}

void append_runtime_report_header_entries(runtime_lls_document_t* document,
                                          const runtime_report_header_t& header) {
  if (!document) return;
  const std::string semantic_taxon =
      normalize_runtime_report_semantic_taxon(header.semantic_taxon);
  if (!semantic_taxon.empty()) {
    if (!is_known_runtime_report_semantic_taxon(semantic_taxon)) {
      throw std::invalid_argument("unknown runtime report semantic_taxon: " +
                                  semantic_taxon);
    }
    document->entries.push_back(make_runtime_lls_string_entry("semantic_taxon",
                                                              semantic_taxon));
  }
  if (!header.context.canonical_path.empty()) {
    document->entries.push_back(
        make_runtime_lls_string_entry("canonical_path",
                                      header.context.canonical_path));
  }
  if (!header.context.binding_id.empty()) {
    document->entries.push_back(
        make_runtime_lls_string_entry("binding_id", header.context.binding_id));
  }
  if (header.context.has_wave_cursor) {
    document->entries.push_back(make_runtime_lls_uint_entry(
        "wave_cursor", header.context.wave_cursor, "[0,+inf)"));
  }
  if (!header.context.source_runtime_cursor.empty()) {
    document->entries.push_back(make_runtime_lls_string_entry(
        "source_runtime_cursor", header.context.source_runtime_cursor));
  }
}

bool parse_runtime_report_header_from_kv(
    const std::unordered_map<std::string, std::string>& kv,
    runtime_report_header_t* out,
    std::string* error) {
  clear_error(error);
  if (!out) {
    set_error(error, "runtime report header output pointer is null");
    return false;
  }
  *out = runtime_report_header_t{};

  const auto lookup = [&](std::string_view key) -> std::string {
    const auto it = kv.find(std::string(key));
    if (it == kv.end()) return {};
    return trim_ascii_copy(it->second);
  };

  const std::string schema = lookup("schema");
  out->semantic_taxon =
      normalize_runtime_report_semantic_taxon(lookup("semantic_taxon"), schema);
  if (!out->semantic_taxon.empty() &&
      !is_known_runtime_report_semantic_taxon(out->semantic_taxon)) {
    set_error(error, "unknown runtime report semantic_taxon: " +
                         out->semantic_taxon);
    *out = runtime_report_header_t{};
    return false;
  }
  out->context.canonical_path = lookup("canonical_path");
  out->context.binding_id = lookup("binding_id");
  out->context.source_runtime_cursor = lookup("source_runtime_cursor");
  std::uint64_t parsed_wave_cursor = 0;
  if (parse_u64_ascii(lookup("wave_cursor"), &parsed_wave_cursor)) {
    out->context.has_wave_cursor = true;
    out->context.wave_cursor = parsed_wave_cursor;
  }

  return true;
}

bool pack_runtime_wave_cursor(std::uint64_t run_id,
                              std::uint64_t episode,
                              std::uint64_t batch,
                              std::uint64_t* out) {
  if (!out) return false;
  const db::wave_cursor::layout_t layout{
      .run_bits = 41,
      .episode_bits = 11,
      .batch_bits = 11,
  };
  const db::wave_cursor::parts_t parts{
      .run_id = run_id,
      .episode_k = episode,
      .batch_j = batch,
  };
  return db::wave_cursor::pack(layout, parts, out);
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
