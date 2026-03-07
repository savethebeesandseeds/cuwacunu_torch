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

#include "hashimyei/hashimyei_identity.h"

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
    return c == '{' || c == '}' || c == '=' || c == ';';
  }

  bool eof() const { return pos_ >= src_.size(); }
  char curr() const { return eof() ? '\0' : src_[pos_]; }
  char next_char() const {
    return (pos_ + 1 < src_.size()) ? src_[pos_ + 1] : '\0';
  }

  void advance() {
    if (eof()) return;
    if (src_[pos_] == '\n') {
      ++line_;
      col_ = 1;
    } else {
      ++col_;
    }
    ++pos_;
  }

  void skip_line_comment() {
    while (!eof() && curr() != '\n') advance();
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
      if (eof()) return;
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
        if (eof()) break;
        const char esc = curr();
        switch (esc) {
          case 'n': out.push_back('\n'); break;
          case 't': out.push_back('\t'); break;
          case 'r': out.push_back('\r'); break;
          case '\\': out.push_back('\\'); break;
          case '"': out.push_back('"'); break;
          default: out.push_back(esc); break;
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
      if (std::isspace(static_cast<unsigned char>(c)) || is_symbol_char(c)) break;
      if (c == '/' && next_char() == '*') break;
      if (c == '/' && next_char() == '/') break;
      if (c == '#') break;
      out.push_back(c);
      advance();
    }
    return token_t{token_t::kind_e::Identifier, std::move(out), line, col};
  }

  token_t next_impl() {
    skip_ignorable();
    if (eof()) return token_t{token_t::kind_e::End, "", line_, col_};

    const std::size_t line = line_;
    const std::size_t col = col_;
    const char c = curr();

    if (is_symbol_char(c)) {
      std::string s(1, c);
      advance();
      return token_t{token_t::kind_e::Symbol, std::move(s), line, col};
    }
    if (c == '"') return parse_string_token();
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
  static std::string lower_ascii_copy(std::string s) {
    for (char& c : s) {
      c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
  }

  static std::string trim_ascii_copy(std::string s) {
    std::size_t b = 0;
    while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    std::size_t e = s.size();
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
  }

  static bool parse_bool_token(const std::string& value, bool* out) {
    if (!out) return false;
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

  static bool parse_sampler_token(const std::string& value, std::string* out) {
    if (!out) return false;
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

  static bool parse_u64_token(const std::string& value, std::uint64_t* out) {
    if (!out) return false;
    std::uint64_t x = 0;
    const char* b = value.data();
    const char* e = value.data() + value.size();
    const auto r = std::from_chars(b, e, x);
    if (r.ec != std::errc{} || r.ptr != e) return false;
    *out = x;
    return true;
  }

  static bool parse_probe_training_window_token(
      const std::string& value,
      cuwacunu::camahjucunu::iitepi_wave_probe_training_window_e* out) {
    if (!out) return false;
    const std::string v = lower_ascii_copy(value);
    if (v == "incoming_batch") {
      *out = cuwacunu::camahjucunu::iitepi_wave_probe_training_window_e::
          IncomingBatch;
      return true;
    }
    return false;
  }

  static bool parse_probe_report_policy_token(
      const std::string& value,
      cuwacunu::camahjucunu::iitepi_wave_probe_report_policy_e* out) {
    if (!out) return false;
    const std::string v = lower_ascii_copy(value);
    if (v == "epoch_end_log") {
      *out =
          cuwacunu::camahjucunu::iitepi_wave_probe_report_policy_e::EpochEndLog;
      return true;
    }
    return false;
  }

  static bool parse_probe_objective_token(
      const std::string& value,
      cuwacunu::camahjucunu::iitepi_wave_probe_objective_e* out) {
    if (!out) return false;
    const std::string v = lower_ascii_copy(value);
    if (v == "future_target_dims_nll") {
      *out = cuwacunu::camahjucunu::iitepi_wave_probe_objective_e::
          FutureTargetDimsNll;
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
    if (!peek_is_symbol(c)) return false;
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

  void expect_identifier(const char* expected) {
    const token_t t = expect_identifier_any();
    if (t.text != expected) {
      throw std::runtime_error("expected '" + std::string(expected) + "' at " +
                               std::to_string(t.line) + ":" +
                               std::to_string(t.col) + ", got '" + t.text +
                               "'");
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

  std::string parse_assignment_value(const char* key) {
    expect_identifier(key);
    expect_symbol('=');
    std::string value = parse_scalar_value();
    expect_symbol(';');
    return value;
  }

  std::string parse_assignment_expression(const char* key) {
    expect_identifier(key);
    expect_symbol('=');
    std::ostringstream oss;
    bool seen_value = false;
    for (;;) {
      const token_t t = next();
      if (t.kind == token_t::kind_e::Symbol && t.text == ";") break;
      if (t.kind != token_t::kind_e::Identifier &&
          t.kind != token_t::kind_e::String) {
        throw std::runtime_error("expected scalar expression token for '" +
                                 std::string(key) + "' at " +
                                 std::to_string(t.line) + ":" +
                                 std::to_string(t.col));
      }
      if (seen_value) oss << ' ';
      oss << t.text;
      seen_value = true;
    }
    if (!seen_value) {
      throw std::runtime_error("assignment '" + std::string(key) +
                               "' requires a value");
    }
    return trim_ascii_copy(oss.str());
  }

  cuwacunu::camahjucunu::iitepi_wave_wikimyei_decl_t parse_wikimyei_block() {
    using cuwacunu::camahjucunu::iitepi_wave_wikimyei_decl_t;
    iitepi_wave_wikimyei_decl_t out{};
    expect_identifier("WIKIMYEI");
    out.wikimyei_path = expect_identifier_any().text;
    expect_symbol('{');

    while (!peek_is_symbol('}')) {
      const token_t key = expect_identifier_any();
      if (key.text == "PATH") {
        expect_symbol('=');
        out.wikimyei_path = parse_scalar_value();
        expect_symbol(';');
      } else if (key.text == "TRAIN") {
        expect_symbol('=');
        const std::string train_value = parse_scalar_value();
        expect_symbol(';');
        bool parsed = false;
        if (!parse_bool_token(train_value, &parsed)) {
          throw std::runtime_error("invalid WIKIMYEI TRAIN value for PATH '" +
                                   out.wikimyei_path + "': " + train_value);
        }
        out.train = parsed;
        out.has_train = true;
      } else if (key.text == "PROFILE_ID") {
        expect_symbol('=');
        out.profile_id = parse_scalar_value();
        expect_symbol(';');
      } else {
        throw std::runtime_error("unknown WIKIMYEI key for PATH '" + out.wikimyei_path +
                                 "': " + key.text);
      }
    }

    expect_symbol('}');
    expect_symbol(';');

    if (!out.has_train) {
      throw std::runtime_error("WIKIMYEI '" + out.wikimyei_path +
                               "' missing required TRAIN assignment");
    }
    if (out.wikimyei_path.empty()) {
      throw std::runtime_error("WIKIMYEI missing required PATH assignment");
    }
    if (out.profile_id.empty()) {
      throw std::runtime_error("WIKIMYEI '" + out.wikimyei_path +
                               "' missing required PROFILE_ID assignment");
    }
    return out;
  }

  cuwacunu::camahjucunu::iitepi_wave_source_decl_t parse_source_block() {
    using cuwacunu::camahjucunu::iitepi_wave_source_decl_t;
    iitepi_wave_source_decl_t out{};
    expect_identifier("SOURCE");
    out.source_path = expect_identifier_any().text;
    expect_symbol('{');

    bool has_workers = false;
    bool has_force_rebuild_cache = false;
    bool has_range_warn_batches = false;
    bool has_sources_dsl_file = false;
    bool has_channels_dsl_file = false;

    while (!peek_is_symbol('}')) {
      const token_t key = expect_identifier_any();
      if (key.text == "PATH") {
        expect_symbol('=');
        out.source_path = parse_scalar_value();
        expect_symbol(';');
      } else if (key.text == "SYMBOL") {
        expect_symbol('=');
        out.symbol = parse_scalar_value();
        expect_symbol(';');
      } else if (key.text == "FROM") {
        expect_symbol('=');
        out.from = parse_scalar_value();
        expect_symbol(';');
      } else if (key.text == "TO") {
        expect_symbol('=');
        out.to = parse_scalar_value();
        expect_symbol(';');
      } else if (key.text == "WORKERS") {
        expect_symbol('=');
        const std::string value = parse_scalar_value();
        expect_symbol(';');
        if (!parse_u64_token(value, &out.workers)) {
          throw std::runtime_error("invalid SOURCE WORKERS for PATH '" +
                                   out.source_path + "': " + value);
        }
        has_workers = true;
      } else if (key.text == "FORCE_REBUILD_CACHE") {
        expect_symbol('=');
        const std::string value = parse_scalar_value();
        expect_symbol(';');
        bool parsed = false;
        if (!parse_bool_token(value, &parsed)) {
          throw std::runtime_error(
              "invalid SOURCE FORCE_REBUILD_CACHE for PATH '" +
              out.source_path + "': " + value);
        }
        out.force_rebuild_cache = parsed;
        has_force_rebuild_cache = true;
      } else if (key.text == "RANGE_WARN_BATCHES") {
        expect_symbol('=');
        const std::string value = parse_scalar_value();
        expect_symbol(';');
        if (!parse_u64_token(value, &out.range_warn_batches) ||
            out.range_warn_batches == 0) {
          throw std::runtime_error(
              "invalid SOURCE RANGE_WARN_BATCHES for PATH '" +
              out.source_path + "': " + value);
        }
        has_range_warn_batches = true;
      } else if (key.text == "SOURCES_DSL_FILE") {
        expect_symbol('=');
        out.sources_dsl_file = parse_scalar_value();
        expect_symbol(';');
        has_sources_dsl_file = true;
      } else if (key.text == "CHANNELS_DSL_FILE") {
        expect_symbol('=');
        out.channels_dsl_file = parse_scalar_value();
        expect_symbol(';');
        has_channels_dsl_file = true;
      } else {
        throw std::runtime_error("unknown SOURCE key for PATH '" + out.source_path +
                                 "': " + key.text);
      }
    }

    expect_symbol('}');
    expect_symbol(';');

    if (out.symbol.empty()) {
      throw std::runtime_error("SOURCE '" + out.source_path +
                               "' missing required SYMBOL assignment");
    }
    if (out.source_path.empty()) {
      throw std::runtime_error("SOURCE missing required PATH assignment");
    }
    if (out.from.empty() || out.to.empty()) {
      throw std::runtime_error("SOURCE '" + out.source_path +
                               "' requires both FROM and TO");
    }
    if (!has_workers) {
      throw std::runtime_error("SOURCE '" + out.source_path +
                               "' missing required WORKERS assignment");
    }
    if (!has_force_rebuild_cache) {
      throw std::runtime_error(
          "SOURCE '" + out.source_path +
          "' missing required FORCE_REBUILD_CACHE assignment");
    }
    if (!has_range_warn_batches) {
      throw std::runtime_error(
          "SOURCE '" + out.source_path +
          "' missing required RANGE_WARN_BATCHES assignment");
    }
    const auto has_non_ws_ascii = [](const std::string& s) {
      for (const char ch : s) {
        if (!std::isspace(static_cast<unsigned char>(ch))) return true;
      }
      return false;
    };
    if (!has_sources_dsl_file || !has_non_ws_ascii(out.sources_dsl_file)) {
      throw std::runtime_error(
          "SOURCE '" + out.source_path +
          "' missing required SOURCES_DSL_FILE assignment");
    }
    if (!has_channels_dsl_file || !has_non_ws_ascii(out.channels_dsl_file)) {
      throw std::runtime_error(
          "SOURCE '" + out.source_path +
          "' missing required CHANNELS_DSL_FILE assignment");
    }
    return out;
  }

  cuwacunu::camahjucunu::iitepi_wave_probe_decl_t parse_probe_block() {
    using cuwacunu::camahjucunu::iitepi_wave_probe_decl_t;
    using cuwacunu::camahjucunu::iitepi_wave_probe_objective_e;
    using cuwacunu::camahjucunu::iitepi_wave_probe_report_policy_e;
    using cuwacunu::camahjucunu::iitepi_wave_probe_training_window_e;

    iitepi_wave_probe_decl_t out{};
    expect_identifier("PROBE");
    out.probe_path = expect_identifier_any().text;
    expect_symbol('{');

    bool has_training_window = false;
    bool has_report_policy = false;
    bool has_objective = false;

    while (!peek_is_symbol('}')) {
      const token_t key = expect_identifier_any();
      if (key.text == "PATH") {
        expect_symbol('=');
        out.probe_path = parse_scalar_value();
        expect_symbol(';');
      } else if (key.text == "TRAINING_WINDOW") {
        expect_symbol('=');
        const std::string value = parse_scalar_value();
        expect_symbol(';');
        iitepi_wave_probe_training_window_e parsed{};
        if (!parse_probe_training_window_token(value, &parsed)) {
          throw std::runtime_error(
              "invalid PROBE TRAINING_WINDOW for PATH '" + out.probe_path +
              "': " + value + " (expected incoming_batch)");
        }
        out.policy.training_window = parsed;
        has_training_window = true;
      } else if (key.text == "REPORT_POLICY") {
        expect_symbol('=');
        const std::string value = parse_scalar_value();
        expect_symbol(';');
        iitepi_wave_probe_report_policy_e parsed{};
        if (!parse_probe_report_policy_token(value, &parsed)) {
          throw std::runtime_error("invalid PROBE REPORT_POLICY for PATH '" +
                                   out.probe_path + "': " + value +
                                   " (expected epoch_end_log)");
        }
        out.policy.report_policy = parsed;
        has_report_policy = true;
      } else if (key.text == "OBJECTIVE") {
        expect_symbol('=');
        const std::string value = parse_scalar_value();
        expect_symbol(';');
        iitepi_wave_probe_objective_e parsed{};
        if (!parse_probe_objective_token(value, &parsed)) {
          throw std::runtime_error("invalid PROBE OBJECTIVE for PATH '" +
                                   out.probe_path + "': " + value +
                                   " (expected future_target_dims_nll)");
        }
        out.policy.objective = parsed;
        has_objective = true;
      } else {
        throw std::runtime_error("unknown PROBE key for PATH '" +
                                 out.probe_path + "': " + key.text);
      }
    }

    expect_symbol('}');
    expect_symbol(';');

    if (out.probe_path.empty()) {
      throw std::runtime_error("PROBE missing required PATH assignment");
    }
    constexpr std::string_view kProbePrefix =
        "tsi.probe.representation.transfer_matrix_evaluation.";
    if (out.probe_path.size() <= kProbePrefix.size() ||
        out.probe_path.rfind(kProbePrefix, 0) != 0) {
      throw std::runtime_error(
          "PROBE PATH must be "
          "'tsi.probe.representation.transfer_matrix_evaluation.0x<hex>', got '" +
          out.probe_path + "'");
    }
    const std::string hash = out.probe_path.substr(kProbePrefix.size());
    if (!cuwacunu::hashimyei::is_hex_hash_name(hash)) {
      throw std::runtime_error("PROBE PATH invalid hashimyei suffix: " + hash +
                               " (expected 0x<hex>)");
    }
    if (!has_training_window) {
      throw std::runtime_error("PROBE '" + out.probe_path +
                               "' missing required TRAINING_WINDOW assignment");
    }
    if (!has_report_policy) {
      throw std::runtime_error("PROBE '" + out.probe_path +
                               "' missing required REPORT_POLICY assignment");
    }
    if (!has_objective) {
      throw std::runtime_error("PROBE '" + out.probe_path +
                               "' missing required OBJECTIVE assignment");
    }
    return out;
  }

  cuwacunu::camahjucunu::iitepi_wave_t parse_wave() {
    using cuwacunu::camahjucunu::iitepi_wave_t;
    iitepi_wave_t out{};
    expect_identifier("WAVE");
    out.name = expect_identifier_any().text;
    expect_symbol('{');

    std::unordered_set<std::string> seen_wikimyei_paths;
    std::unordered_set<std::string> seen_source_paths;
    std::unordered_set<std::string> seen_probe_paths;
    bool has_mode = false;
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
                mode_expression,
                &out.mode_flags,
                nullptr,
                &mode_error)) {
          throw std::runtime_error("WAVE '" + out.name +
                                   "' invalid MODE: " + mode_error);
        }
        out.mode = lower_ascii_copy(trim_ascii_copy(mode_expression));
        has_mode = true;
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
          throw std::runtime_error(
              "WAVE '" + out.name +
              "' invalid MAX_BATCHES_PER_EPOCH: " + value);
        }
        has_max_batches_per_epoch = true;
        continue;
      }
      if (head.text == "WIKIMYEI") {
        auto w = parse_wikimyei_block();
        if (!seen_wikimyei_paths.insert(w.wikimyei_path).second) {
          throw std::runtime_error("WAVE '" + out.name +
                                   "' duplicate WIKIMYEI PATH: " + w.wikimyei_path);
        }
        out.wikimyeis.push_back(std::move(w));
        continue;
      }
      if (head.text == "SOURCE") {
        auto s = parse_source_block();
        if (!seen_source_paths.insert(s.source_path).second) {
          throw std::runtime_error("WAVE '" + out.name +
                                   "' duplicate SOURCE PATH: " + s.source_path);
        }
        out.sources.push_back(std::move(s));
        continue;
      }
      if (head.text == "PROBE") {
        auto p = parse_probe_block();
        if (!seen_probe_paths.insert(p.probe_path).second) {
          throw std::runtime_error("WAVE '" + out.name +
                                   "' duplicate PROBE PATH: " + p.probe_path);
        }
        out.probes.push_back(std::move(p));
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
    if (!has_sampler) {
      throw std::runtime_error("WAVE '" + out.name +
                               "' missing SAMPLER assignment");
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

    const std::uint64_t run_flag = cuwacunu::camahjucunu::iitepi_wave_mode_value(
        cuwacunu::camahjucunu::iitepi_wave_mode_flag_e::Run);
    const std::uint64_t train_flag = cuwacunu::camahjucunu::iitepi_wave_mode_value(
        cuwacunu::camahjucunu::iitepi_wave_mode_flag_e::Train);
    const bool run_enabled = (out.mode_flags & run_flag) != 0;
    const bool train_enabled = (out.mode_flags & train_flag) != 0;
    if (!run_enabled && !train_enabled) {
      throw std::runtime_error(
          "WAVE '" + out.name +
          "' MODE must enable at least one execution bit: run or train");
    }
    if (run_enabled && !train_enabled) {
      for (const auto& w : out.wikimyeis) {
        if (w.train) {
          throw std::runtime_error(
              "WAVE '" + out.name +
              "' MODE=run forbids WIKIMYEI TRAIN=true (PATH '" + w.wikimyei_path + "')");
        }
      }
    }
    if (train_enabled) {
      bool has_train_true = false;
      for (const auto& w : out.wikimyeis) {
        if (w.train) {
          has_train_true = true;
          break;
        }
      }
      if (!has_train_true) {
        throw std::runtime_error(
            "WAVE '" + out.name +
            "' MODE with train bit requires at least one WIKIMYEI TRAIN=true");
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
    if (!std::isspace(static_cast<unsigned char>(ch))) return true;
  }
  return false;
}

void validate_wave_grammar_text_or_throw_(const std::string& grammar_text) {
  if (!has_non_ws_ascii_(grammar_text)) {
    throw std::runtime_error("iitepi wave grammar text is empty");
  }
  constexpr std::string_view kRequiredGrammarTokens[] = {
      "<wave>",
      "WAVE",
      "WIKIMYEI",
      "SOURCE",
      "PROBE",
      "PATH",
      "TRAINING_WINDOW",
      "REPORT_POLICY",
      "OBJECTIVE",
      "MODE",
      "SAMPLER",
      "EPOCHS",
      "BATCH_SIZE",
      "MAX_BATCHES_PER_EPOCH",
      "WORKERS",
      "FORCE_REBUILD_CACHE",
      "RANGE_WARN_BATCHES",
      "SOURCES_DSL_FILE",
      "CHANNELS_DSL_FILE",
  };
  for (const auto token : kRequiredGrammarTokens) {
    if (grammar_text.find(token) == std::string::npos) {
      throw std::runtime_error(
          "iitepi wave grammar missing required token: " + std::string(token));
    }
  }
}

} /* namespace */

std::string iitepi_wave_set_t::str() const {
  std::ostringstream oss;
  oss << "iitepi_wave_set_t: waves=" << waves.size() << "\n";
  for (std::size_t i = 0; i < waves.size(); ++i) {
    const auto& p = waves[i];
    const bool has_source = !p.sources.empty();
    const auto& src0 = has_source ? p.sources.front() : cuwacunu::camahjucunu::iitepi_wave_source_decl_t{};
    oss << "  [" << i << "] name=" << p.name
        << " mode=" << p.mode
        << " mode_flags=0x" << std::hex << p.mode_flags << std::dec
        << " sampler=" << p.sampler
        << " epochs=" << p.epochs
        << " batch_size=" << p.batch_size
        << " max_batches_per_epoch=" << p.max_batches_per_epoch
        << " source0_dataloader(workers=" << src0.workers
        << ",force_rebuild_cache=" << (src0.force_rebuild_cache ? "true" : "false")
        << ",range_warn_batches=" << src0.range_warn_batches
        << ",sources=" << src0.sources_dsl_file
        << ",channels=" << src0.channels_dsl_file
        << ")"
        << " wikimyeis=" << p.wikimyeis.size()
        << " sources=" << p.sources.size()
        << " probes=" << p.probes.size() << "\n";
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

iitepi_wave_set_t decode_iitepi_wave_from_dsl(
    std::string grammar_text,
    std::string instruction_text) {
  iitepiWavePipeline pipeline(std::move(grammar_text));
  return pipeline.decode(std::move(instruction_text));
}

} /* namespace dsl */

} /* namespace camahjucunu */
} /* namespace cuwacunu */
