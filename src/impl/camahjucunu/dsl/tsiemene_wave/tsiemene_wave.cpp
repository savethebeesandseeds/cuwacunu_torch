/* tsiemene_wave.cpp */
#include "camahjucunu/dsl/tsiemene_wave/tsiemene_wave.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_set>
#include <utility>

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

  cuwacunu::camahjucunu::tsiemene_wave_set_t parse() {
    using cuwacunu::camahjucunu::tsiemene_wave_set_t;
    tsiemene_wave_set_t out{};
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

  cuwacunu::camahjucunu::tsiemene_wave_wikimyei_decl_t parse_wikimyei_block() {
    using cuwacunu::camahjucunu::tsiemene_wave_wikimyei_decl_t;
    tsiemene_wave_wikimyei_decl_t out{};
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

  cuwacunu::camahjucunu::tsiemene_wave_source_decl_t parse_source_block() {
    using cuwacunu::camahjucunu::tsiemene_wave_source_decl_t;
    tsiemene_wave_source_decl_t out{};
    expect_identifier("SOURCE");
    out.source_path = expect_identifier_any().text;
    expect_symbol('{');

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
    return out;
  }

  cuwacunu::camahjucunu::tsiemene_wave_t parse_wave() {
    using cuwacunu::camahjucunu::tsiemene_wave_t;
    tsiemene_wave_t out{};
    expect_identifier("WAVE");
    out.name = expect_identifier_any().text;
    expect_symbol('{');

    std::unordered_set<std::string> seen_wikimyei_paths;
    std::unordered_set<std::string> seen_source_paths;
    bool has_mode = false;
    bool has_sampler = false;
    bool has_epochs = false;
    bool has_batch_size = false;

    while (!peek_is_symbol('}')) {
      const token_t head = peek();
      if (head.kind != token_t::kind_e::Identifier) {
        throw std::runtime_error("expected wave statement at " +
                                 std::to_string(head.line) + ":" +
                                 std::to_string(head.col));
      }
      if (head.text == "MODE") {
        out.mode = parse_assignment_value("MODE");
        out.mode = lower_ascii_copy(out.mode);
        if (out.mode != "train" && out.mode != "run") {
          throw std::runtime_error("WAVE '" + out.name +
                                   "' invalid MODE: " + out.mode);
        }
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
    if (out.wikimyeis.empty()) {
      throw std::runtime_error("WAVE '" + out.name +
                               "' must declare at least one WIKIMYEI block");
    }
    if (out.sources.empty()) {
      throw std::runtime_error("WAVE '" + out.name +
                               "' must declare at least one SOURCE block");
    }

    if (out.mode == "run") {
      for (const auto& w : out.wikimyeis) {
        if (w.train) {
          throw std::runtime_error(
              "WAVE '" + out.name +
              "' MODE=run forbids WIKIMYEI TRAIN=true (PATH '" + w.wikimyei_path + "')");
        }
      }
    } else {
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
            "' MODE=train requires at least one WIKIMYEI TRAIN=true");
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
    throw std::runtime_error("tsiemene wave grammar text is empty");
  }
  constexpr std::string_view kRequiredGrammarTokens[] = {
      "<wave>",
      "WAVE",
      "WIKIMYEI",
      "SOURCE",
      "PATH",
      "MODE",
      "SAMPLER",
      "EPOCHS",
      "BATCH_SIZE",
      "MAX_BATCHES_PER_EPOCH",
  };
  for (const auto token : kRequiredGrammarTokens) {
    if (grammar_text.find(token) == std::string::npos) {
      throw std::runtime_error(
          "tsiemene wave grammar missing required token: " + std::string(token));
    }
  }
}

} /* namespace */

std::string tsiemene_wave_set_t::str() const {
  std::ostringstream oss;
  oss << "tsiemene_wave_set_t: waves=" << waves.size() << "\n";
  for (std::size_t i = 0; i < waves.size(); ++i) {
    const auto& p = waves[i];
    oss << "  [" << i << "] name=" << p.name
        << " mode=" << p.mode
        << " sampler=" << p.sampler
        << " epochs=" << p.epochs
        << " batch_size=" << p.batch_size
        << " max_batches_per_epoch=" << p.max_batches_per_epoch
        << " wikimyeis=" << p.wikimyeis.size()
        << " sources=" << p.sources.size() << "\n";
  }
  return oss.str();
}

namespace dsl {

tsiemeneWavePipeline::tsiemeneWavePipeline(std::string grammar_text)
    : grammar_text_(std::move(grammar_text)) {
  validate_wave_grammar_text_or_throw_(grammar_text_);
}

tsiemene_wave_set_t tsiemeneWavePipeline::decode(std::string instruction) {
  std::lock_guard<std::mutex> lk(current_mutex_);
  parser_t parser(std::move(instruction));
  return parser.parse();
}

tsiemene_wave_set_t decode_tsiemene_wave_from_dsl(
    std::string grammar_text,
    std::string instruction_text) {
  tsiemeneWavePipeline pipeline(std::move(grammar_text));
  return pipeline.decode(std::move(instruction_text));
}

} /* namespace dsl */

} /* namespace camahjucunu */
} /* namespace cuwacunu */
