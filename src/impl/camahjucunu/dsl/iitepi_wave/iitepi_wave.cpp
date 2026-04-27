/* iitepi_wave.cpp */
#include "camahjucunu/dsl/iitepi_wave/iitepi_wave.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "camahjucunu/dsl/canonical_path/canonical_path.h"
#include "hero/hashimyei_hero/hashimyei_identity.h"
#include "tsiemene/tsi.type.registry.h"

namespace {

struct token_t {
  enum class kind_e { Identifier, String, Symbol, End };
  kind_e kind{kind_e::End};
  std::string text{};
  std::size_t line{1};
  std::size_t col{1};
};

class lexer_t {
public:
  explicit lexer_t(std::string src) : src_(std::move(src)) {}

  token_t peek() {
    if (!has_peek_) {
      peek_tok_ = next_impl();
      has_peek_ = true;
    }
    return peek_tok_;
  }

  token_t next() {
    if (has_peek_) {
      has_peek_ = false;
      return peek_tok_;
    }
    return next_impl();
  }

private:
  static bool is_symbol_char(char c) {
    return c == '{' || c == '}' || c == '=' || c == ':' || c == ';' ||
           c == '<' || c == '>' || c == '|' || c == '+' || c == '^' || c == ',';
  }

  bool eof() const { return pos_ >= src_.size(); }
  char curr() const { return eof() ? '\0' : src_[pos_]; }
  char next_char() const {
    return (pos_ + 1 < src_.size()) ? src_[pos_ + 1] : '\0';
  }

  void advance() {
    if (eof())
      return;
    if (src_[pos_] == '\n') {
      ++line_;
      col_ = 1;
    } else {
      ++col_;
    }
    ++pos_;
  }

  void skip_line_comment() {
    while (!eof() && curr() != '\n')
      advance();
  }

  void skip_block_comment() {
    advance();
    advance();
    while (!eof()) {
      if (curr() == '*' && next_char() == '/') {
        advance();
        advance();
        return;
      }
      advance();
    }
  }

  void skip_ignorable() {
    for (;;) {
      if (eof())
        return;
      if (std::isspace(static_cast<unsigned char>(curr()))) {
        advance();
        continue;
      }
      if (curr() == '/' && next_char() == '*') {
        skip_block_comment();
        continue;
      }
      if (curr() == '/' && next_char() == '/') {
        skip_line_comment();
        continue;
      }
      if (curr() == '#') {
        skip_line_comment();
        continue;
      }
      return;
    }
  }

  token_t parse_string_token() {
    const std::size_t line = line_;
    const std::size_t col = col_;
    std::string out;
    advance();
    while (!eof()) {
      const char c = curr();
      if (c == '"') {
        advance();
        return token_t{token_t::kind_e::String, std::move(out), line, col};
      }
      if (c == '\\') {
        advance();
        if (eof())
          break;
        const char esc = curr();
        switch (esc) {
        case 'n':
          out.push_back('\n');
          break;
        case 't':
          out.push_back('\t');
          break;
        case 'r':
          out.push_back('\r');
          break;
        case '\\':
          out.push_back('\\');
          break;
        case '"':
          out.push_back('"');
          break;
        default:
          out.push_back(esc);
          break;
        }
        advance();
        continue;
      }
      out.push_back(c);
      advance();
    }
    throw std::runtime_error("unterminated string literal");
  }

  token_t parse_identifier_token() {
    const std::size_t line = line_;
    const std::size_t col = col_;
    std::string out;
    while (!eof()) {
      const char c = curr();
      if (std::isspace(static_cast<unsigned char>(c)) || is_symbol_char(c))
        break;
      if (c == '/' && next_char() == '*')
        break;
      if (c == '/' && next_char() == '/')
        break;
      if (c == '#')
        break;
      out.push_back(c);
      advance();
    }
    return token_t{token_t::kind_e::Identifier, std::move(out), line, col};
  }

  token_t next_impl() {
    skip_ignorable();
    if (eof())
      return token_t{token_t::kind_e::End, "", line_, col_};

    const std::size_t line = line_;
    const std::size_t col = col_;
    const char c = curr();

    if (is_symbol_char(c)) {
      std::string s(1, c);
      advance();
      return token_t{token_t::kind_e::Symbol, std::move(s), line, col};
    }
    if (c == '"')
      return parse_string_token();
    return parse_identifier_token();
  }

  std::string src_;
  std::size_t pos_{0};
  std::size_t line_{1};
  std::size_t col_{1};
  bool has_peek_{false};
  token_t peek_tok_{};
};

class parser_t {
public:
  explicit parser_t(std::string input) : lex_(std::move(input)) {}

  cuwacunu::camahjucunu::iitepi_wave_set_t parse() {
    using cuwacunu::camahjucunu::iitepi_wave_set_t;
    iitepi_wave_set_t out{};
    std::unordered_set<std::string> wave_names;
    while (!peek_is_end()) {
      auto wave = parse_wave();
      if (!wave_names.insert(wave.name).second) {
        throw std::runtime_error("duplicate WAVE name: " + wave.name);
      }
      out.waves.push_back(std::move(wave));
    }
    if (out.waves.empty()) {
      throw std::runtime_error("wave set has no WAVE blocks");
    }
    return out;
  }

private:
  struct block_header_t {
    std::string binding_id{};
    std::string legacy_path{};
  };

  struct source_runtime_presence_t {
    bool has_runtime_instrument_signature{false};
    bool has_from{false};
    bool has_to{false};
    bool has_workers{false};
    bool has_force_rebuild_cache{false};
    bool has_range_warn_batches{false};
  };

  struct probe_runtime_presence_t {
    bool has_training_window{false};
    bool has_report_policy{false};
    bool has_objective{false};
  };

  static std::string lower_ascii_copy(std::string s) {
    for (char &c : s) {
      c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
  }

  static std::string trim_ascii_copy(std::string s) {
    std::size_t b = 0;
    while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b])))
      ++b;
    std::size_t e = s.size();
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])))
      --e;
    return s.substr(b, e - b);
  }

  static bool has_non_ws_ascii(const std::string &s) {
    for (char ch : s) {
      if (!std::isspace(static_cast<unsigned char>(ch)))
        return true;
    }
    return false;
  }

  static bool parse_bool_token(const std::string &value, bool *out) {
    if (!out)
      return false;
    const std::string v = lower_ascii_copy(value);
    if (v == "true" || v == "1" || v == "yes" || v == "on") {
      *out = true;
      return true;
    }
    if (v == "false" || v == "0" || v == "no" || v == "off") {
      *out = false;
      return true;
    }
    return false;
  }

  static bool parse_sampler_token(const std::string &value, std::string *out) {
    if (!out)
      return false;
    const std::string v = lower_ascii_copy(value);
    if (v == "sequential" || v == "sequentialsampler") {
      *out = "sequential";
      return true;
    }
    if (v == "random" || v == "randomsampler") {
      *out = "random";
      return true;
    }
    return false;
  }

  static bool parse_determinism_policy_token(const std::string &value,
                                             std::string *out) {
    if (!out)
      return false;
    const std::string v = lower_ascii_copy(trim_ascii_copy(value));
    if (v == "deterministic" || v == "det") {
      *out = "deterministic";
      return true;
    }
    if (v == "non_deterministic" || v == "nondeterministic" ||
        v == "non-deterministic" || v == "stochastic") {
      *out = "non_deterministic";
      return true;
    }
    return false;
  }

  static bool parse_u64_token(const std::string &value, std::uint64_t *out) {
    if (!out)
      return false;
    std::uint64_t x = 0;
    const char *b = value.data();
    const char *e = value.data() + value.size();
    const auto r = std::from_chars(b, e, x);
    if (r.ec != std::errc{} || r.ptr != e)
      return false;
    *out = x;
    return true;
  }

  static bool parse_probe_training_window_token(
      const std::string &value,
      cuwacunu::camahjucunu::iitepi_wave_evaluation_training_window_e *out) {
    if (!out)
      return false;
    const std::string v = lower_ascii_copy(value);
    if (v == "incoming_batch") {
      *out = cuwacunu::camahjucunu::iitepi_wave_evaluation_training_window_e::
          IncomingBatch;
      return true;
    }
    return false;
  }

  static bool parse_probe_report_policy_token(
      const std::string &value,
      cuwacunu::camahjucunu::iitepi_wave_evaluation_report_policy_e *out) {
    if (!out)
      return false;
    const std::string v = lower_ascii_copy(value);
    if (v == "epoch_end_log") {
      *out = cuwacunu::camahjucunu::iitepi_wave_evaluation_report_policy_e::
          EpochEndLog;
      return true;
    }
    return false;
  }

  static bool parse_probe_objective_token(
      const std::string &value,
      cuwacunu::camahjucunu::iitepi_wave_evaluation_objective_e *out) {
    if (!out)
      return false;
    const std::string v = lower_ascii_copy(value);
    if (v == "future_target_feature_indices_nll") {
      *out = cuwacunu::camahjucunu::iitepi_wave_evaluation_objective_e::
          FutureTargetFeatureIndicesNll;
      return true;
    }
    return false;
  }

  static bool derive_family_and_hash_from_path(const std::string &path,
                                               std::string *out_family,
                                               std::string *out_hash) {
    const auto parsed =
        cuwacunu::camahjucunu::decode_canonical_path(trim_ascii_copy(path));
    if (!parsed.ok || parsed.path_kind !=
                          cuwacunu::camahjucunu::canonical_path_kind_e::Node) {
      return false;
    }
    if (out_family && out_family->empty()) {
      const auto type_id =
          tsiemene::parse_tsi_type_id(parsed.canonical_identity);
      *out_family = type_id.has_value()
                        ? std::string(tsiemene::tsi_type_token(*type_id))
                        : parsed.canonical_identity;
    }
    if (out_hash && out_hash->empty()) {
      *out_hash = parsed.hashimyei;
    }
    return true;
  }

  static std::string compose_node_path(const std::string &family,
                                       const std::string &hashimyei) {
    if (family.empty())
      return {};
    if (hashimyei.empty())
      return family;
    return family + "." + hashimyei;
  }

  static bool canonicalize_family_token(std::string family,
                                        std::string *out_family,
                                        std::string *error) {
    if (!out_family)
      return false;
    family = trim_ascii_copy(std::move(family));
    if (!has_non_ws_ascii(family)) {
      if (error)
        *error = "family token is empty";
      return false;
    }

    const auto type_id = tsiemene::parse_tsi_type_id(family);
    if (!type_id.has_value()) {
      if (error)
        *error = "unsupported component family token: " + family;
      return false;
    }

    *out_family = std::string(tsiemene::tsi_type_token(*type_id));
    return true;
  }

  static bool finalize_component_identity(const char *component_kind,
                                          std::string *family,
                                          std::string *runtime_path) {
    if (!family || !runtime_path)
      return false;

    *family = trim_ascii_copy(*family);
    *runtime_path = trim_ascii_copy(*runtime_path);

    std::string path_family{};
    std::string path_hash{};
    if (!runtime_path->empty()) {
      if (!derive_family_and_hash_from_path(*runtime_path, &path_family,
                                            &path_hash)) {
        throw std::runtime_error(
            std::string(component_kind) +
            " invalid PATH/FAMILY payload: " + *runtime_path);
      }
    }

    if (!family->empty()) {
      std::string family_error;
      std::string canonical_family;
      if (!canonicalize_family_token(*family, &canonical_family,
                                     &family_error)) {
        throw std::runtime_error(std::string(component_kind) +
                                 " invalid FAMILY: " + family_error);
      }
      *family = canonical_family;
    }

    if (family->empty() && !path_family.empty()) {
      *family = path_family;
    }

    if (family->empty() && runtime_path->empty()) {
      throw std::runtime_error(std::string(component_kind) +
                               " missing required FAMILY assignment");
    }

    if (!family->empty()) {
      const auto type_id = tsiemene::parse_tsi_type_id(*family);
      if (!type_id.has_value()) {
        throw std::runtime_error(std::string(component_kind) +
                                 " has unsupported FAMILY: " + *family);
      }

      if (!path_family.empty() && path_family != *family) {
        throw std::runtime_error(std::string(component_kind) +
                                 " PATH/FAMILY mismatch: FAMILY=" + *family +
                                 " PATH=" + *runtime_path);
      }

      const auto policy = tsiemene::tsi_type_instance_policy(*type_id);
      if (policy == tsiemene::TsiInstancePolicy::HashimyeiInstances) {
        *runtime_path = compose_node_path(*family, path_hash);
        return true;
      }
      *runtime_path = compose_node_path(*family, {});
      return true;
    }
    return false;
  }

  token_t peek() { return lex_.peek(); }
  token_t next() { return lex_.next(); }

  bool peek_is_end() { return peek().kind == token_t::kind_e::End; }

  bool peek_is_symbol(char c) {
    const token_t t = peek();
    return t.kind == token_t::kind_e::Symbol && t.text.size() == 1 &&
           t.text[0] == c;
  }

  bool try_consume_symbol(char c) {
    if (!peek_is_symbol(c))
      return false;
    (void)next();
    return true;
  }

  void expect_symbol(char c) {
    const token_t t = next();
    if (!(t.kind == token_t::kind_e::Symbol && t.text.size() == 1 &&
          t.text[0] == c)) {
      throw std::runtime_error("expected symbol '" + std::string(1, c) +
                               "' at " + std::to_string(t.line) + ":" +
                               std::to_string(t.col));
    }
  }

  token_t expect_identifier_any() {
    const token_t t = next();
    if (t.kind != token_t::kind_e::Identifier) {
      throw std::runtime_error("expected identifier at " +
                               std::to_string(t.line) + ":" +
                               std::to_string(t.col));
    }
    return t;
  }

  void expect_identifier(const char *expected) {
    const token_t t = expect_identifier_any();
    if (t.text != expected) {
      throw std::runtime_error("expected '" + std::string(expected) + "' at " +
                               std::to_string(t.line) + ":" +
                               std::to_string(t.col) + ", got '" + t.text +
                               "'");
    }
  }

  void expect_assignment_delim() {
    const token_t t = next();
    if (!(t.kind == token_t::kind_e::Symbol &&
          (t.text == ":" || t.text == "="))) {
      throw std::runtime_error("expected assignment delimiter ':' or '=' at " +
                               std::to_string(t.line) + ":" +
                               std::to_string(t.col));
    }
  }

  std::string parse_scalar_value() {
    const token_t t = next();
    if (t.kind != token_t::kind_e::Identifier &&
        t.kind != token_t::kind_e::String) {
      throw std::runtime_error("expected scalar value at " +
                               std::to_string(t.line) + ":" +
                               std::to_string(t.col));
    }
    return t.text;
  }

  std::string parse_assignment_value(const char *key) {
    if (!(peek_is_symbol(':') || peek_is_symbol('='))) {
      expect_identifier(key);
    }
    expect_assignment_delim();
    std::string value = parse_scalar_value();
    expect_symbol(';');
    return value;
  }

  std::string parse_assignment_expression(const char *key) {
    expect_identifier(key);
    expect_assignment_delim();
    std::ostringstream oss;
    bool seen_value = false;
    for (;;) {
      const token_t t = next();
      if (t.kind == token_t::kind_e::Symbol && t.text == ";")
        break;
      if (t.kind != token_t::kind_e::Identifier &&
          t.kind != token_t::kind_e::String &&
          t.kind != token_t::kind_e::Symbol) {
        throw std::runtime_error(
            "expected scalar expression token for '" + std::string(key) +
            "' at " + std::to_string(t.line) + ":" + std::to_string(t.col));
      }
      if (seen_value)
        oss << ' ';
      oss << t.text;
      seen_value = true;
    }
    if (!seen_value) {
      throw std::runtime_error("assignment '" + std::string(key) +
                               "' requires a value");
    }
    return trim_ascii_copy(oss.str());
  }

  cuwacunu::camahjucunu::instrument_signature_t
  parse_runtime_instrument_signature_block(const std::string &scope_label) {
    cuwacunu::camahjucunu::instrument_signature_t out{};
    std::unordered_set<std::string> seen_fields{};
    expect_assignment_delim();
    expect_symbol('{');
    while (!peek_is_symbol('}')) {
      const token_t key = expect_identifier_any();
      std::string field_error{};
      const std::string value = parse_assignment_value(key.text.c_str());
      if (!cuwacunu::camahjucunu::instrument_signature_set_field(
              &out, key.text, value, &field_error)) {
        throw std::runtime_error(
            scope_label +
            " invalid RUNTIME_INSTRUMENT_SIGNATURE: " + field_error);
      }
      if (!seen_fields.insert(key.text).second) {
        throw std::runtime_error(
            scope_label +
            " duplicate RUNTIME_INSTRUMENT_SIGNATURE field: " + key.text);
      }
    }
    expect_symbol('}');
    expect_symbol(';');

    if (seen_fields.size() != 6U) {
      throw std::runtime_error(
          scope_label +
          " RUNTIME_INSTRUMENT_SIGNATURE must declare SYMBOL, RECORD_TYPE, "
          "MARKET_TYPE, VENUE, BASE_ASSET, and QUOTE_ASSET");
    }
    std::string validation_error{};
    if (!cuwacunu::camahjucunu::instrument_signature_validate(
            out, /*allow_any=*/false,
            scope_label + " RUNTIME_INSTRUMENT_SIGNATURE", &validation_error)) {
      throw std::runtime_error(validation_error);
    }
    return out;
  }

  block_header_t parse_component_header(const char *keyword) {
    expect_identifier(keyword);
    (void)try_consume_symbol(':');

    block_header_t out{};
    if (try_consume_symbol('<')) {
      out.binding_id = trim_ascii_copy(expect_identifier_any().text);
      if (!has_non_ws_ascii(out.binding_id)) {
        throw std::runtime_error(std::string(keyword) +
                                 " binding id cannot be empty");
      }
      expect_symbol('>');
    } else if (peek().kind == token_t::kind_e::Identifier) {
      out.legacy_path = trim_ascii_copy(expect_identifier_any().text);
    }

    expect_symbol('{');
    return out;
  }

  bool parse_source_settings_statement(
      const token_t &key, cuwacunu::camahjucunu::iitepi_wave_source_decl_t *out,
      source_runtime_presence_t *presence, std::string_view scope_label) {
    if (!out || !presence)
      return false;
    if (key.text == "WORKERS") {
      const std::string value = parse_assignment_value("WORKERS");
      if (!parse_u64_token(value, &out->workers)) {
        throw std::runtime_error(std::string(scope_label) +
                                 " invalid WORKERS: " + value);
      }
      presence->has_workers = true;
      return true;
    }
    if (key.text == "FORCE_REBUILD_CACHE") {
      const std::string value = parse_assignment_value("FORCE_REBUILD_CACHE");
      bool parsed = false;
      if (!parse_bool_token(value, &parsed)) {
        throw std::runtime_error(std::string(scope_label) +
                                 " invalid FORCE_REBUILD_CACHE: " + value);
      }
      out->force_rebuild_cache = parsed;
      presence->has_force_rebuild_cache = true;
      return true;
    }
    if (key.text == "RANGE_WARN_BATCHES") {
      const std::string value = parse_assignment_value("RANGE_WARN_BATCHES");
      if (!parse_u64_token(value, &out->range_warn_batches) ||
          out->range_warn_batches == 0) {
        throw std::runtime_error(std::string(scope_label) +
                                 " invalid RANGE_WARN_BATCHES: " + value);
      }
      presence->has_range_warn_batches = true;
      return true;
    }
    return false;
  }

  void parse_source_settings_block(
      cuwacunu::camahjucunu::iitepi_wave_source_decl_t *out,
      source_runtime_presence_t *presence, const std::string &source_identity) {
    if (!out || !presence)
      return;
    expect_assignment_delim();
    expect_symbol('{');
    while (!peek_is_symbol('}')) {
      const token_t key = expect_identifier_any();
      if (!parse_source_settings_statement(
              key, out, presence,
              ("SOURCE '" + source_identity + "' SETTINGS").c_str())) {
        throw std::runtime_error("unknown SOURCE SETTINGS key for '" +
                                 source_identity + "': " + key.text);
      }
    }
    expect_symbol('}');
    expect_symbol(';');
  }

  bool parse_source_runtime_statement(
      const token_t &key, cuwacunu::camahjucunu::iitepi_wave_source_decl_t *out,
      source_runtime_presence_t *presence, std::string_view scope_label) {
    if (!out || !presence)
      return false;

    if (key.text == "SYMBOL") {
      throw std::runtime_error(
          std::string(scope_label) +
          " uses removed SYMBOL field; use RUNTIME_INSTRUMENT_SIGNATURE "
          "with explicit SYMBOL, RECORD_TYPE, MARKET_TYPE, VENUE, BASE_ASSET, "
          "and QUOTE_ASSET");
    }
    if (key.text == "RUNTIME_INSTRUMENT_SIGNATURE") {
      out->runtime_instrument_signature =
          parse_runtime_instrument_signature_block(std::string(scope_label));
      presence->has_runtime_instrument_signature = true;
      return true;
    }
    if (key.text == "FROM") {
      out->from = parse_assignment_value("FROM");
      presence->has_from = true;
      return true;
    }
    if (key.text == "TO") {
      out->to = parse_assignment_value("TO");
      presence->has_to = true;
      return true;
    }
    return false;
  }

  void parse_source_runtime_block(
      cuwacunu::camahjucunu::iitepi_wave_source_decl_t *out,
      source_runtime_presence_t *presence, const std::string &source_identity) {
    if (!out || !presence)
      return;
    expect_assignment_delim();
    expect_symbol('{');
    while (!peek_is_symbol('}')) {
      const token_t key = expect_identifier_any();
      if (!parse_source_runtime_statement(
              key, out, presence,
              ("SOURCE '" + source_identity + "' RUNTIME").c_str())) {
        throw std::runtime_error("unknown SOURCE RUNTIME key for '" +
                                 source_identity + "': " + key.text);
      }
    }
    expect_symbol('}');
    expect_symbol(';');
  }

  bool parse_probe_runtime_statement(
      const token_t &key, cuwacunu::camahjucunu::iitepi_wave_probe_decl_t *out,
      probe_runtime_presence_t *presence, std::string_view scope_label) {
    using cuwacunu::camahjucunu::iitepi_wave_evaluation_objective_e;
    using cuwacunu::camahjucunu::iitepi_wave_evaluation_report_policy_e;
    using cuwacunu::camahjucunu::iitepi_wave_evaluation_training_window_e;

    if (!out || !presence)
      return false;

    if (key.text == "TRAINING_WINDOW") {
      const std::string value = parse_assignment_value("TRAINING_WINDOW");
      iitepi_wave_evaluation_training_window_e parsed{};
      if (!parse_probe_training_window_token(value, &parsed)) {
        throw std::runtime_error(std::string(scope_label) +
                                 " invalid TRAINING_WINDOW: " + value);
      }
      out->policy.training_window = parsed;
      presence->has_training_window = true;
      return true;
    }
    if (key.text == "REPORT_POLICY") {
      const std::string value = parse_assignment_value("REPORT_POLICY");
      iitepi_wave_evaluation_report_policy_e parsed{};
      if (!parse_probe_report_policy_token(value, &parsed)) {
        throw std::runtime_error(std::string(scope_label) +
                                 " invalid REPORT_POLICY: " + value);
      }
      out->policy.report_policy = parsed;
      presence->has_report_policy = true;
      return true;
    }
    if (key.text == "OBJECTIVE") {
      const std::string value = parse_assignment_value("OBJECTIVE");
      iitepi_wave_evaluation_objective_e parsed{};
      if (!parse_probe_objective_token(value, &parsed)) {
        throw std::runtime_error(std::string(scope_label) +
                                 " invalid OBJECTIVE: " + value);
      }
      out->policy.objective = parsed;
      presence->has_objective = true;
      return true;
    }
    return false;
  }

  void parse_probe_runtime_block(
      cuwacunu::camahjucunu::iitepi_wave_probe_decl_t *out,
      probe_runtime_presence_t *presence, const std::string &probe_identity) {
    if (!out || !presence)
      return;
    expect_assignment_delim();
    expect_symbol('{');
    while (!peek_is_symbol('}')) {
      const token_t key = expect_identifier_any();
      if (!parse_probe_runtime_statement(
              key, out, presence,
              ("PROBE '" + probe_identity + "' RUNTIME").c_str())) {
        throw std::runtime_error("unknown PROBE RUNTIME key for '" +
                                 probe_identity + "': " + key.text);
      }
    }
    expect_symbol('}');
    expect_symbol(';');
  }

  void parse_wikimyei_jkimyei_block(
      cuwacunu::camahjucunu::iitepi_wave_wikimyei_decl_t *out,
      const std::string &wikimyei_identity) {
    if (!out)
      return;
    expect_assignment_delim();
    expect_symbol('{');

    bool has_halt_train = false;

    while (!peek_is_symbol('}')) {
      const token_t key = expect_identifier_any();
      if (key.text == "HALT_TRAIN") {
        const std::string value = parse_assignment_value("HALT_TRAIN");
        bool parsed = false;
        if (!parse_bool_token(value, &parsed)) {
          throw std::runtime_error("WIKIMYEI '" + wikimyei_identity +
                                   "' invalid JKIMYEI.HALT_TRAIN: " + value);
        }
        out->halt_train = parsed;
        out->train = !parsed;
        out->has_train = true;
        has_halt_train = true;
      } else if (key.text == "PROFILE_ID") {
        out->profile_id = parse_assignment_value("PROFILE_ID");
      } else {
        throw std::runtime_error("WIKIMYEI '" + wikimyei_identity +
                                 "' unknown JKIMYEI key: " + key.text);
      }
    }

    expect_symbol('}');
    expect_symbol(';');

    if (!has_halt_train) {
      throw std::runtime_error("WIKIMYEI '" + wikimyei_identity +
                               "' JKIMYEI missing HALT_TRAIN");
    }
  }

  cuwacunu::camahjucunu::iitepi_wave_wikimyei_decl_t parse_wikimyei_block() {
    using cuwacunu::camahjucunu::iitepi_wave_wikimyei_decl_t;

    const auto header = parse_component_header("WIKIMYEI");

    iitepi_wave_wikimyei_decl_t out{};
    out.binding_id = header.binding_id;
    out.wikimyei_path = header.legacy_path;
    bool has_jkimyei = false;

    while (!peek_is_symbol('}')) {
      const token_t key = expect_identifier_any();
      if (key.text == "FAMILY") {
        out.family = parse_assignment_value("FAMILY");
      } else if (key.text == "HASHIMYEI") {
        throw std::runtime_error("WIKIMYEI '" +
                                 (has_non_ws_ascii(out.binding_id)
                                      ? out.binding_id
                                      : out.wikimyei_path) +
                                 "' uses legacy HASHIMYEI; concrete "
                                 "hashimyeis are derived from contract "
                                 "component content");
      } else if (key.text == "PATH") {
        out.wikimyei_path = parse_assignment_value("PATH");
      } else if (key.text == "JKIMYEI") {
        const std::string identity = has_non_ws_ascii(out.binding_id)
                                         ? out.binding_id
                                         : out.wikimyei_path;
        parse_wikimyei_jkimyei_block(&out, identity);
        has_jkimyei = true;
      } else {
        throw std::runtime_error("unknown WIKIMYEI key: " + key.text);
      }
    }

    expect_symbol('}');
    expect_symbol(';');

    (void)finalize_component_identity("WIKIMYEI", &out.family,
                                      &out.wikimyei_path);

    if (!has_non_ws_ascii(out.binding_id)) {
      throw std::runtime_error("WIKIMYEI '" + out.wikimyei_path +
                               "' missing required binding id (<...>)");
    }
    if (!has_jkimyei) {
      throw std::runtime_error("WIKIMYEI '" + out.wikimyei_path +
                               "' missing required JKIMYEI block");
    }
    if (!out.has_train) {
      throw std::runtime_error("WIKIMYEI '" + out.wikimyei_path +
                               "' missing required JKIMYEI.HALT_TRAIN policy");
    }

    return out;
  }

  cuwacunu::camahjucunu::iitepi_wave_source_decl_t parse_source_block() {
    using cuwacunu::camahjucunu::iitepi_wave_source_decl_t;

    const auto header = parse_component_header("SOURCE");

    iitepi_wave_source_decl_t out{};
    out.binding_id = header.binding_id;
    out.source_path = header.legacy_path;

    source_runtime_presence_t presence{};

    while (!peek_is_symbol('}')) {
      const token_t key = expect_identifier_any();
      if (key.text == "FAMILY") {
        out.family = parse_assignment_value("FAMILY");
      } else if (key.text == "PATH") {
        out.source_path = parse_assignment_value("PATH");
      } else if (key.text == "SETTINGS") {
        const std::string identity =
            has_non_ws_ascii(out.binding_id) ? out.binding_id : out.source_path;
        parse_source_settings_block(&out, &presence, identity);
      } else if (key.text == "RUNTIME") {
        const std::string identity =
            has_non_ws_ascii(out.binding_id) ? out.binding_id : out.source_path;
        parse_source_runtime_block(&out, &presence, identity);
      } else {
        throw std::runtime_error("unknown SOURCE key: " + key.text);
      }
    }

    expect_symbol('}');
    expect_symbol(';');

    (void)finalize_component_identity("SOURCE", &out.family, &out.source_path);

    if (!has_non_ws_ascii(out.binding_id)) {
      throw std::runtime_error("SOURCE '" + out.source_path +
                               "' missing required binding id (<...>)");
    }
    if (!presence.has_runtime_instrument_signature ||
        !has_non_ws_ascii(out.runtime_instrument_signature.symbol)) {
      throw std::runtime_error("SOURCE '" + out.source_path +
                               "' missing required "
                               "RUNTIME.RUNTIME_INSTRUMENT_SIGNATURE");
    }
    if (!presence.has_from || !presence.has_to || !has_non_ws_ascii(out.from) ||
        !has_non_ws_ascii(out.to)) {
      throw std::runtime_error("SOURCE '" + out.source_path +
                               "' requires both RUNTIME.FROM and RUNTIME.TO");
    }
    if (!presence.has_workers) {
      throw std::runtime_error("SOURCE '" + out.source_path +
                               "' missing required SETTINGS.WORKERS");
    }
    if (!presence.has_force_rebuild_cache) {
      throw std::runtime_error(
          "SOURCE '" + out.source_path +
          "' missing required SETTINGS.FORCE_REBUILD_CACHE");
    }
    if (!presence.has_range_warn_batches || out.range_warn_batches == 0) {
      throw std::runtime_error(
          "SOURCE '" + out.source_path +
          "' missing required SETTINGS.RANGE_WARN_BATCHES");
    }
    return out;
  }

  cuwacunu::camahjucunu::iitepi_wave_probe_decl_t parse_probe_block() {
    using cuwacunu::camahjucunu::iitepi_wave_probe_decl_t;

    const auto header = parse_component_header("PROBE");

    iitepi_wave_probe_decl_t out{};
    out.binding_id = header.binding_id;
    out.probe_path = header.legacy_path;

    probe_runtime_presence_t runtime_presence{};

    while (!peek_is_symbol('}')) {
      const token_t key = expect_identifier_any();
      if (key.text == "FAMILY") {
        out.family = parse_assignment_value("FAMILY");
      } else if (key.text == "HASHIMYEI") {
        throw std::runtime_error("PROBE '" +
                                 (has_non_ws_ascii(out.binding_id)
                                      ? out.binding_id
                                      : out.probe_path) +
                                 "' uses legacy HASHIMYEI; concrete "
                                 "hashimyeis are derived from contract "
                                 "component content");
      } else if (key.text == "PATH") {
        out.probe_path = parse_assignment_value("PATH");
      } else if (key.text == "RUNTIME") {
        const std::string identity =
            has_non_ws_ascii(out.binding_id) ? out.binding_id : out.probe_path;
        parse_probe_runtime_block(&out, &runtime_presence, identity);
      } else if (parse_probe_runtime_statement(key, &out, &runtime_presence,
                                               "PROBE")) {
        continue;
      } else {
        throw std::runtime_error("unknown PROBE key: " + key.text);
      }
    }

    expect_symbol('}');
    expect_symbol(';');

    (void)finalize_component_identity("PROBE", &out.family, &out.probe_path);

    if (!has_non_ws_ascii(out.binding_id)) {
      throw std::runtime_error("PROBE '" + out.probe_path +
                               "' missing required binding id (<...>)");
    }

    out.has_runtime = runtime_presence.has_training_window ||
                      runtime_presence.has_report_policy ||
                      runtime_presence.has_objective;

    return out;
  }

  cuwacunu::camahjucunu::iitepi_wave_sink_decl_t parse_sink_block() {
    using cuwacunu::camahjucunu::iitepi_wave_sink_decl_t;

    const auto header = parse_component_header("SINK");

    iitepi_wave_sink_decl_t out{};
    out.binding_id = header.binding_id;
    out.sink_path = header.legacy_path;

    while (!peek_is_symbol('}')) {
      const token_t key = expect_identifier_any();
      if (key.text == "FAMILY") {
        out.family = parse_assignment_value("FAMILY");
      } else if (key.text == "PATH") {
        out.sink_path = parse_assignment_value("PATH");
      } else {
        throw std::runtime_error("unknown SINK key: " + key.text);
      }
    }

    expect_symbol('}');
    expect_symbol(';');

    (void)finalize_component_identity("SINK", &out.family, &out.sink_path);

    if (!has_non_ws_ascii(out.binding_id)) {
      throw std::runtime_error("SINK '" + out.sink_path +
                               "' missing required binding id (<...>)");
    }

    return out;
  }

  cuwacunu::camahjucunu::iitepi_wave_t parse_wave() {
    using cuwacunu::camahjucunu::iitepi_wave_t;

    iitepi_wave_t out{};
    expect_identifier("WAVE");
    out.name = expect_identifier_any().text;
    expect_symbol('{');

    std::unordered_set<std::string> seen_binding_ids{};
    std::unordered_set<std::string> seen_wikimyei_paths{};
    std::unordered_set<std::string> seen_source_paths{};
    std::unordered_set<std::string> seen_probe_paths{};
    std::unordered_set<std::string> seen_sink_paths{};

    bool has_mode = false;
    bool has_circuit = false;
    bool has_sampler = false;
    bool has_epochs = false;
    bool has_batch_size = false;
    bool has_max_batches_per_epoch = false;

    while (!peek_is_symbol('}')) {
      const token_t head = peek();
      if (head.kind != token_t::kind_e::Identifier) {
        throw std::runtime_error("expected wave statement at " +
                                 std::to_string(head.line) + ":" +
                                 std::to_string(head.col));
      }

      if (head.text == "MODE") {
        const std::string mode_expression = parse_assignment_expression("MODE");
        std::string mode_error;
        if (!cuwacunu::camahjucunu::parse_iitepi_wave_mode_flags(
                mode_expression, &out.mode_flags, nullptr, &mode_error)) {
          throw std::runtime_error("WAVE '" + out.name +
                                   "' invalid MODE: " + mode_error);
        }
        out.mode = lower_ascii_copy(trim_ascii_copy(mode_expression));
        has_mode = true;
        continue;
      }

      if (head.text == "CIRCUIT") {
        if (has_circuit) {
          throw std::runtime_error("WAVE '" + out.name +
                                   "' duplicate CIRCUIT assignment");
        }
        out.circuit_name = trim_ascii_copy(parse_assignment_value("CIRCUIT"));
        if (!has_non_ws_ascii(out.circuit_name)) {
          throw std::runtime_error("WAVE '" + out.name +
                                   "' invalid CIRCUIT: empty selector");
        }
        has_circuit = true;
        continue;
      }

      if (head.text == "SAMPLER") {
        const std::string value = parse_assignment_value("SAMPLER");
        if (!parse_sampler_token(value, &out.sampler)) {
          throw std::runtime_error("WAVE '" + out.name +
                                   "' invalid SAMPLER: " + value);
        }
        has_sampler = true;
        continue;
      }

      if (head.text == "DETERMINISM") {
        const std::string value = parse_assignment_value("DETERMINISM");
        if (!parse_determinism_policy_token(value, &out.determinism_policy)) {
          throw std::runtime_error("WAVE '" + out.name +
                                   "' invalid DETERMINISM: " + value);
        }
        continue;
      }

      if (head.text == "EPOCHS") {
        const std::string value = parse_assignment_value("EPOCHS");
        if (!parse_u64_token(value, &out.epochs) || out.epochs == 0) {
          throw std::runtime_error("WAVE '" + out.name +
                                   "' invalid EPOCHS: " + value);
        }
        has_epochs = true;
        continue;
      }

      if (head.text == "BATCH_SIZE") {
        const std::string value = parse_assignment_value("BATCH_SIZE");
        if (!parse_u64_token(value, &out.batch_size) || out.batch_size == 0) {
          throw std::runtime_error("WAVE '" + out.name +
                                   "' invalid BATCH_SIZE: " + value);
        }
        has_batch_size = true;
        continue;
      }

      if (head.text == "MAX_BATCHES_PER_EPOCH") {
        const std::string value =
            parse_assignment_value("MAX_BATCHES_PER_EPOCH");
        if (!parse_u64_token(value, &out.max_batches_per_epoch) ||
            out.max_batches_per_epoch == 0) {
          throw std::runtime_error("WAVE '" + out.name +
                                   "' invalid MAX_BATCHES_PER_EPOCH: " + value);
        }
        has_max_batches_per_epoch = true;
        continue;
      }

      if (head.text == "WIKIMYEI") {
        auto w = parse_wikimyei_block();
        if (!seen_binding_ids.insert(w.binding_id).second) {
          throw std::runtime_error(
              "WAVE '" + out.name +
              "' duplicate component binding id: " + w.binding_id);
        }
        if (!seen_wikimyei_paths.insert(w.wikimyei_path).second) {
          throw std::runtime_error(
              "WAVE '" + out.name +
              "' duplicate WIKIMYEI PATH: " + w.wikimyei_path);
        }
        out.wikimyeis.push_back(std::move(w));
        continue;
      }

      if (head.text == "SOURCE") {
        auto s = parse_source_block();
        if (!seen_binding_ids.insert(s.binding_id).second) {
          throw std::runtime_error(
              "WAVE '" + out.name +
              "' duplicate component binding id: " + s.binding_id);
        }
        if (!seen_source_paths.insert(s.source_path).second) {
          throw std::runtime_error("WAVE '" + out.name +
                                   "' duplicate SOURCE PATH: " + s.source_path);
        }
        out.sources.push_back(std::move(s));
        continue;
      }

      if (head.text == "PROBE") {
        auto p = parse_probe_block();
        if (!seen_binding_ids.insert(p.binding_id).second) {
          throw std::runtime_error(
              "WAVE '" + out.name +
              "' duplicate component binding id: " + p.binding_id);
        }
        if (!seen_probe_paths.insert(p.probe_path).second) {
          throw std::runtime_error("WAVE '" + out.name +
                                   "' duplicate PROBE PATH: " + p.probe_path);
        }
        out.probes.push_back(std::move(p));
        continue;
      }

      if (head.text == "SINK") {
        auto s = parse_sink_block();
        if (!seen_binding_ids.insert(s.binding_id).second) {
          throw std::runtime_error(
              "WAVE '" + out.name +
              "' duplicate component binding id: " + s.binding_id);
        }
        if (!seen_sink_paths.insert(s.sink_path).second) {
          throw std::runtime_error("WAVE '" + out.name +
                                   "' duplicate SINK PATH: " + s.sink_path);
        }
        out.sinks.push_back(std::move(s));
        continue;
      }

      throw std::runtime_error("WAVE '" + out.name +
                               "' unknown statement: " + head.text);
    }

    expect_symbol('}');

    if (!has_mode) {
      throw std::runtime_error("WAVE '" + out.name +
                               "' missing MODE assignment");
    }
    if (!has_epochs) {
      throw std::runtime_error("WAVE '" + out.name +
                               "' missing EPOCHS assignment");
    }
    if (!has_batch_size) {
      throw std::runtime_error("WAVE '" + out.name +
                               "' missing BATCH_SIZE assignment");
    }
    if (!has_max_batches_per_epoch) {
      throw std::runtime_error("WAVE '" + out.name +
                               "' missing MAX_BATCHES_PER_EPOCH assignment");
    }
    if (out.wikimyeis.empty()) {
      throw std::runtime_error("WAVE '" + out.name +
                               "' must declare at least one WIKIMYEI block");
    }
    if (out.sources.empty()) {
      throw std::runtime_error("WAVE '" + out.name +
                               "' must declare at least one SOURCE block");
    }

    if (!has_sampler) {
      throw std::runtime_error("WAVE '" + out.name +
                               "' missing SAMPLER assignment");
    }

    const std::uint64_t run_flag =
        cuwacunu::camahjucunu::iitepi_wave_mode_value(
            cuwacunu::camahjucunu::iitepi_wave_mode_flag_e::Run);
    const std::uint64_t train_flag =
        cuwacunu::camahjucunu::iitepi_wave_mode_value(
            cuwacunu::camahjucunu::iitepi_wave_mode_flag_e::Train);

    const bool run_enabled = (out.mode_flags & run_flag) != 0;
    const bool train_enabled = (out.mode_flags & train_flag) != 0;

    if (!run_enabled && !train_enabled) {
      throw std::runtime_error("WAVE '" + out.name +
                               "' MODE must enable run or train");
    }

    if (run_enabled && !train_enabled) {
      for (const auto &w : out.wikimyeis) {
        if (w.train) {
          throw std::runtime_error(
              "WAVE '" + out.name +
              "' MODE without train bit forbids JKIMYEI.HALT_TRAIN=false ('" +
              w.binding_id + "')");
        }
      }
    }

    if (train_enabled) {
      bool has_train_true = false;
      for (const auto &w : out.wikimyeis) {
        if (w.train) {
          has_train_true = true;
          break;
        }
      }
      if (!has_train_true) {
        throw std::runtime_error("WAVE '" + out.name +
                                 "' MODE with train bit requires at least one "
                                 "WIKIMYEI with JKIMYEI.HALT_TRAIN=false");
      }
    }

    return out;
  }

  lexer_t lex_;
};

} // namespace

namespace cuwacunu {
namespace camahjucunu {

namespace {

[[nodiscard]] bool has_non_ws_ascii_(std::string_view text) {
  for (const char ch : text) {
    if (!std::isspace(static_cast<unsigned char>(ch)))
      return true;
  }
  return false;
}

void validate_wave_grammar_text_or_throw_(const std::string &grammar_text) {
  if (!has_non_ws_ascii_(grammar_text)) {
    throw std::runtime_error("iitepi wave grammar text is empty");
  }

  constexpr std::string_view kRequiredGrammarTokens[] = {
      "<wave>",
      "WAVE",
      "WIKIMYEI",
      "SOURCE",
      "PROBE",
      "SINK",
      "FAMILY",
      "RUNTIME",
      "RUNTIME_INSTRUMENT_SIGNATURE",
      "SYMBOL",
      "RECORD_TYPE",
      "MARKET_TYPE",
      "VENUE",
      "BASE_ASSET",
      "QUOTE_ASSET",
      "SETTINGS",
      "JKIMYEI",
      "HALT_TRAIN",
      "PROFILE_ID",
      "TRAINING_WINDOW",
      "REPORT_POLICY",
      "OBJECTIVE",
      "MODE",
      "CIRCUIT",
      "DETERMINISM",
      "SAMPLER",
      "EPOCHS",
      "BATCH_SIZE",
      "MAX_BATCHES_PER_EPOCH",
      "WORKERS",
      "FORCE_REBUILD_CACHE",
      "RANGE_WARN_BATCHES",
  };

  for (const auto token : kRequiredGrammarTokens) {
    if (grammar_text.find(token) == std::string::npos) {
      throw std::runtime_error("iitepi wave grammar missing required token: " +
                               std::string(token));
    }
  }
}

} /* namespace */

std::string iitepi_wave_set_t::str() const {
  std::ostringstream oss;
  oss << "iitepi_wave_set_t: waves=" << waves.size() << "\n";
  for (std::size_t i = 0; i < waves.size(); ++i) {
    const auto &p = waves[i];
    const bool has_source = !p.sources.empty();
    const auto &src0 = has_source
                           ? p.sources.front()
                           : cuwacunu::camahjucunu::iitepi_wave_source_decl_t{};
    oss << "  [" << i << "] name=" << p.name << " circuit=" << p.circuit_name
        << " mode=" << p.mode << " mode_flags=0x" << std::hex << p.mode_flags
        << std::dec << " determinism=" << p.determinism_policy
        << " sampler=" << p.sampler << " epochs=" << p.epochs
        << " batch_size=" << p.batch_size
        << " max_batches_per_epoch=" << p.max_batches_per_epoch
        << " source0(binding_id=" << src0.binding_id
        << ",path=" << src0.source_path << ",workers=" << src0.workers
        << ",force_rebuild_cache="
        << (src0.force_rebuild_cache ? "true" : "false")
        << ",range_warn_batches=" << src0.range_warn_batches << ")"
        << " wikimyeis=" << p.wikimyeis.size()
        << " sources=" << p.sources.size() << " probes=" << p.probes.size()
        << " sinks=" << p.sinks.size() << "\n";
  }
  return oss.str();
}

namespace dsl {

iitepiWavePipeline::iitepiWavePipeline(std::string grammar_text)
    : grammar_text_(std::move(grammar_text)) {
  validate_wave_grammar_text_or_throw_(grammar_text_);
}

iitepi_wave_set_t iitepiWavePipeline::decode(std::string instruction) {
  std::lock_guard<std::mutex> lk(current_mutex_);
  parser_t parser(std::move(instruction));
  return parser.parse();
}

iitepi_wave_set_t decode_iitepi_wave_from_dsl(std::string grammar_text,
                                              std::string instruction_text) {
  iitepiWavePipeline pipeline(std::move(grammar_text));
  return pipeline.decode(std::move(instruction_text));
}

} /* namespace dsl */

} /* namespace camahjucunu */
} /* namespace cuwacunu */
