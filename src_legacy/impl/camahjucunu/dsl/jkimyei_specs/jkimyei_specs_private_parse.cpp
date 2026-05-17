/* jkimyei_specs.cpp */
#include "camahjucunu/dsl/jkimyei_specs/jkimyei_specs.h"
#include "jkimyei/specs/jkimyei_schema.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

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
    return c == '{' || c == '}' || c == '[' || c == ']' || c == ':' || c == ',' || c == '|';
  }

  bool eof() const { return pos_ >= src_.size(); }

  char curr() const { return eof() ? '\0' : src_[pos_]; }

  char next_char() const { return (pos_ + 1 < src_.size()) ? src_[pos_ + 1] : '\0'; }

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

struct kv_list_t {
  std::vector<std::pair<std::string, std::string>> entries;
};

struct named_kv_block_t {
  std::string name{};
  kv_list_t kv{};
  bool present{false};
};

struct curve_t {
  std::string name{};
  kv_list_t kv{};
};

struct profile_t {
  std::string name{};
  named_kv_block_t optimizer{};
  named_kv_block_t lr_scheduler{};
  named_kv_block_t loss{};

  kv_list_t component_params{};
  kv_list_t numerics{};
  kv_list_t gradient{};
  kv_list_t checkpoint{};
  kv_list_t metrics{};
  kv_list_t data_ref{};
  std::vector<curve_t> augmentations{};

  bool component_params_present{false};
  bool numerics_present{false};
  bool gradient_present{false};
  bool checkpoint_present{false};
  bool metrics_present{false};
  bool data_ref_present{false};
  bool augmentations_present{false};
};

struct component_t {
  std::string canonical_type{};
  std::string id{};
  std::vector<profile_t> profiles{};
  std::string active_profile{};
};

struct document_t {
  std::string version{};
  std::vector<component_t> components{};
};

constexpr const char* kAugmentationCurveKey = "time_warp_curve";
constexpr const char* kLegacyAugmentationCurveKey = "name";

[[nodiscard]] std::string canonicalize_augmentation_curve_key(std::string key) {
  if (key == kLegacyAugmentationCurveKey) return std::string(kAugmentationCurveKey);
  return key;
}

[[nodiscard]] kv_list_t canonicalize_augmentation_curve_kv(
    const kv_list_t& kv,
    const std::string& context) {
  struct seen_key_t {
    std::size_t index;
    std::string raw_key;
  };

  kv_list_t out{};
  std::unordered_map<std::string, seen_key_t> seen_keys{};
  for (const auto& [raw_key, value] : kv.entries) {
    const std::string key = canonicalize_augmentation_curve_key(raw_key);
    const auto seen_it = seen_keys.find(key);
    if (seen_it != seen_keys.end()) {
      if (seen_it->second.raw_key == raw_key) {
        throw std::runtime_error(cuwacunu::piaabo::string_format(
            "%s repeats key '%s'",
            context.c_str(),
            raw_key.c_str()));
      }
      const auto& prior_value = out.entries[seen_it->second.index].second;
      if (prior_value != value) {
        throw std::runtime_error(cuwacunu::piaabo::string_format(
            "%s defines conflicting values for '%s' via aliases '%s' and '%s'",
            context.c_str(),
            key.c_str(),
            seen_it->second.raw_key.c_str(),
            raw_key.c_str()));
      }
      continue;
    }
    seen_keys.emplace(key, seen_key_t{out.entries.size(), raw_key});
    out.entries.emplace_back(key, value);
  }
  return out;
}

class parser_t {
 public:
  explicit parser_t(std::string input) : lex_(std::move(input)) {}

  document_t parse() {
    document_t doc{};

    expect_identifier("JKSPEC");
    doc.version = parse_scalar_value();

    if (peek_is_end()) {
      throw std::runtime_error("JKSPEC requires at least one COMPONENT block");
    }

    std::unordered_set<std::string> component_canonical_types;
    while (!peek_is_end()) {
      if (!peek_is_identifier("COMPONENT")) {
        const token_t bad = peek();
        throw std::runtime_error(cuwacunu::piaabo::string_format(
            "unexpected token '%s' at JKSPEC root %zu:%zu; expected COMPONENT",
            bad.text.c_str(),
            bad.line,
            bad.col));
      }
      component_t component = parse_component();
      if (!component_canonical_types.insert(component.canonical_type).second) {
        throw std::runtime_error(cuwacunu::piaabo::string_format(
            "duplicate COMPONENT canonical type '%s'",
            component.canonical_type.c_str()));
      }
      doc.components.push_back(std::move(component));
    }
    if (doc.components.empty()) {
      throw std::runtime_error("JKSPEC requires at least one COMPONENT block");
    }
    return doc;
  }

 private:
  static std::string lower_ascii_copy(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
  }

  static std::string trim_ascii_copy(std::string s) {
    auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };
    while (!s.empty() && is_space(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    while (!s.empty() && is_space(static_cast<unsigned char>(s.back()))) s.pop_back();
    return s;
  }

  static std::string normalize_key_token(std::string key) {
    while (!key.empty() && key.front() == '.') key.erase(key.begin());
    return key;
  }

  static std::string join_csv(const std::vector<std::string>& vals) {
    std::ostringstream oss;
    for (std::size_t i = 0; i < vals.size(); ++i) {
      if (i > 0) oss << ',';
      oss << vals[i];
    }
    return oss.str();
  }

  token_t peek() { return lex_.peek(); }

  token_t next() { return lex_.next(); }

  bool peek_is_end() { return peek().kind == token_t::kind_e::End; }

  bool peek_is_symbol(char c) {
    const token_t t = peek();
    return t.kind == token_t::kind_e::Symbol && t.text.size() == 1 && t.text[0] == c;
  }

  void expect_symbol(char c) {
    token_t t = next();
    if (!(t.kind == token_t::kind_e::Symbol && t.text.size() == 1 && t.text[0] == c)) {
      throw std::runtime_error(
          cuwacunu::piaabo::string_format("expected symbol '%c' at %zu:%zu", c, t.line, t.col));
    }
  }

  bool try_consume_symbol(char c) {
    if (!peek_is_symbol(c)) return false;
    (void)next();
    return true;
  }

  static bool equals_ascii_ci(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
      const char ca = static_cast<char>(std::tolower(static_cast<unsigned char>(a[i])));
      const char cb = static_cast<char>(std::tolower(static_cast<unsigned char>(b[i])));
      if (ca != cb) return false;
    }
    return true;
  }

  bool peek_is_identifier(const char* expected) {
    const token_t t = peek();
    return t.kind == token_t::kind_e::Identifier && t.text == expected;
  }

  token_t expect_identifier_any() {
    token_t t = next();
    if (t.kind != token_t::kind_e::Identifier) {
      throw std::runtime_error(cuwacunu::piaabo::string_format(
          "expected identifier at %zu:%zu", t.line, t.col));
    }
    return t;
  }

  void expect_identifier(const char* expected) {
    token_t t = expect_identifier_any();
    if (t.text != expected) {
      throw std::runtime_error(cuwacunu::piaabo::string_format(
          "expected '%s' at %zu:%zu, got '%s'",
          expected,
          t.line,
          t.col,
          t.text.c_str()));
    }
  }

  void consume_identifier(const char* expected) { expect_identifier(expected); }

  std::string expect_string_literal() {
    token_t t = next();
    if (t.kind != token_t::kind_e::String) {
      throw std::runtime_error(cuwacunu::piaabo::string_format(
          "expected string literal at %zu:%zu", t.line, t.col));
    }
    return t.text;
  }

  std::string parse_scalar_value() {
    token_t t = next();
    if (t.kind == token_t::kind_e::String) return t.text;
    if (t.kind == token_t::kind_e::Identifier) {
      if (equals_ascii_ci(t.text, "true")) return "true";
      if (equals_ascii_ci(t.text, "false")) return "false";
      return t.text;
    }
    throw std::runtime_error(cuwacunu::piaabo::string_format(
        "expected scalar value at %zu:%zu", t.line, t.col));
  }

  std::string parse_value() {
    if (try_consume_symbol('[')) {
      std::vector<std::string> vals;
      if (!try_consume_symbol(']')) {
        for (;;) {
          vals.push_back(parse_scalar_value());
          if (try_consume_symbol(']')) break;
          expect_symbol(',');
        }
      }
      return join_csv(vals);
    }
    return parse_scalar_value();
  }

  kv_list_t parse_kv_block() {
    kv_list_t kv{};
    std::unordered_set<std::string> seen_keys;
    expect_symbol('{');
    while (!try_consume_symbol('}')) {
      const token_t key_tok = expect_identifier_any();
      const std::string key = normalize_key_token(key_tok.text);
      if (key.empty()) {
        throw std::runtime_error(cuwacunu::piaabo::string_format(
            "invalid empty key at %zu:%zu",
            key_tok.line,
            key_tok.col));
      }
      if (!seen_keys.insert(key).second) {
        throw std::runtime_error(cuwacunu::piaabo::string_format(
            "duplicate key '%s' at %zu:%zu",
            key.c_str(),
            key_tok.line,
            key_tok.col));
      }
      expect_symbol(':');
      const std::string value = parse_value();
      kv.entries.emplace_back(key, value);
    }
    return kv;
  }

  kv_list_t parse_kv_profile_section(const std::string& profile_name,
                                     const std::string& section_name) {
    kv_list_t kv{};
    std::unordered_set<std::string> seen_keys;
    while (!peek_is_symbol('[') && !peek_is_symbol('}')) {
      const token_t key_tok = expect_identifier_any();
      const std::string key = normalize_key_token(key_tok.text);
      if (key.empty()) {
        throw std::runtime_error(cuwacunu::piaabo::string_format(
            "invalid empty key in [%s] for PROFILE '%s' at %zu:%zu",
            section_name.c_str(),
            profile_name.c_str(),
            key_tok.line,
            key_tok.col));
      }
      if (!seen_keys.insert(key).second) {
        throw std::runtime_error(cuwacunu::piaabo::string_format(
            "duplicate key '%s' in [%s] for PROFILE '%s' at %zu:%zu",
            key.c_str(),
            section_name.c_str(),
            profile_name.c_str(),
            key_tok.line,
            key_tok.col));
      }
      expect_symbol(':');
      kv.entries.emplace_back(key, parse_value());
    }
    return kv;
  }

  std::string extract_required_type(kv_list_t* kv,
                                    const std::string& section_name,
                                    const std::string& profile_name) {
    if (!kv) throw std::runtime_error("extract_required_type called with null kv");
    for (auto it = kv->entries.begin(); it != kv->entries.end(); ++it) {
      if (it->first == "type") {
        const std::string out = it->second;
        kv->entries.erase(it);
        if (out.empty()) {
          throw std::runtime_error(cuwacunu::piaabo::string_format(
              "[%s] has empty .type in PROFILE '%s'",
              section_name.c_str(),
              profile_name.c_str()));
        }
        return out;
      }
    }
    throw std::runtime_error(cuwacunu::piaabo::string_format(
        "missing .type in [%s] for PROFILE '%s'",
        section_name.c_str(),
        profile_name.c_str()));
  }

  profile_t parse_profile() {
    consume_identifier("PROFILE");
    profile_t p{};
    p.name = expect_string_literal();
    expect_symbol('{');

    while (!try_consume_symbol('}')) {
      if (peek_is_symbol('[')) {
        expect_symbol('[');
        const token_t section_tok = expect_identifier_any();
        const std::string section_name = section_tok.text;
        expect_symbol(']');

        if (section_name == "OPTIMIZER") {
          if (p.optimizer.present) {
            throw std::runtime_error(cuwacunu::piaabo::string_format(
                "duplicate OPTIMIZER block in PROFILE '%s'",
                p.name.c_str()));
          }
          p.optimizer.kv = parse_kv_profile_section(p.name, section_name);
          p.optimizer.name = extract_required_type(&p.optimizer.kv, section_name, p.name);
          p.optimizer.present = true;
          continue;
        }
        if (section_name == "LR_SCHEDULER") {
          if (p.lr_scheduler.present) {
            throw std::runtime_error(cuwacunu::piaabo::string_format(
                "duplicate LR_SCHEDULER block in PROFILE '%s'",
                p.name.c_str()));
          }
          p.lr_scheduler.kv = parse_kv_profile_section(p.name, section_name);
          p.lr_scheduler.name = extract_required_type(&p.lr_scheduler.kv, section_name, p.name);
          p.lr_scheduler.present = true;
          continue;
        }
        if (section_name == "LOSS") {
          if (p.loss.present) {
            throw std::runtime_error(cuwacunu::piaabo::string_format(
                "duplicate LOSS block in PROFILE '%s'",
                p.name.c_str()));
          }
          p.loss.kv = parse_kv_profile_section(p.name, section_name);
          p.loss.name = extract_required_type(&p.loss.kv, section_name, p.name);
          p.loss.present = true;
          continue;
        }
        if (section_name == "COMPONENT_PARAMS") {
          if (p.component_params_present) {
            throw std::runtime_error(cuwacunu::piaabo::string_format(
                "duplicate COMPONENT_PARAMS block in PROFILE '%s'",
                p.name.c_str()));
          }
          p.component_params = parse_kv_profile_section(p.name, section_name);
          p.component_params_present = true;
          continue;
        }
        if (section_name == "NUMERICS") {
          if (p.numerics_present) {
            throw std::runtime_error(cuwacunu::piaabo::string_format(
                "duplicate NUMERICS block in PROFILE '%s'",
                p.name.c_str()));
          }
          p.numerics = parse_kv_profile_section(p.name, section_name);
          p.numerics_present = true;
          continue;
        }
        if (section_name == "GRADIENT") {
          if (p.gradient_present) {
            throw std::runtime_error(cuwacunu::piaabo::string_format(
                "duplicate GRADIENT block in PROFILE '%s'",
                p.name.c_str()));
          }
          p.gradient = parse_kv_profile_section(p.name, section_name);
          p.gradient_present = true;
          continue;
        }
        if (section_name == "CHECKPOINT") {
          if (p.checkpoint_present) {
            throw std::runtime_error(cuwacunu::piaabo::string_format(
                "duplicate CHECKPOINT block in PROFILE '%s'",
                p.name.c_str()));
          }
          p.checkpoint = parse_kv_profile_section(p.name, section_name);
          p.checkpoint_present = true;
          continue;
        }
        if (section_name == "METRICS") {
          if (p.metrics_present) {
            throw std::runtime_error(cuwacunu::piaabo::string_format(
                "duplicate METRICS block in PROFILE '%s'",
                p.name.c_str()));
          }
          p.metrics = parse_kv_profile_section(p.name, section_name);
          p.metrics_present = true;
          continue;
        }
        if (section_name == "DATA_REF") {
          if (p.data_ref_present) {
            throw std::runtime_error(cuwacunu::piaabo::string_format(
                "duplicate DATA_REF block in PROFILE '%s'",
                p.name.c_str()));
          }
          p.data_ref = parse_kv_profile_section(p.name, section_name);
          p.data_ref_present = true;
          continue;
        }
        if (section_name == "AUGMENTATIONS") {
          if (p.augmentations_present) {
            throw std::runtime_error(cuwacunu::piaabo::string_format(
                "duplicate AUGMENTATIONS block in PROFILE '%s'",
                p.name.c_str()));
          }
          if (try_consume_symbol('{')) {
            if (peek_is_identifier("CURVE")) {
              p.augmentations = parse_augmentations_curve_blocks(p.name);
            } else {
              p.augmentations = parse_augmentations_ascii_table(p.name, /*expect_block_end=*/true);
            }
          } else {
            if (peek_is_identifier("CURVE")) {
              throw std::runtime_error(cuwacunu::piaabo::string_format(
                  "legacy CURVE augmentation format requires braces in PROFILE '%s'",
                  p.name.c_str()));
            }
            p.augmentations = parse_augmentations_ascii_table(p.name, /*expect_block_end=*/false);
          }
          p.augmentations_present = true;
          continue;
        }

        throw std::runtime_error(cuwacunu::piaabo::string_format(
            "unexpected section '[%s]' in PROFILE '%s' at %zu:%zu",
            section_name.c_str(),
            p.name.c_str(),
            section_tok.line,
            section_tok.col));
      }

      if (peek_is_identifier("OPTIMIZER")) {
        if (p.optimizer.present) {
          throw std::runtime_error(cuwacunu::piaabo::string_format(
              "duplicate OPTIMIZER block in PROFILE '%s'",
              p.name.c_str()));
        }
        consume_identifier("OPTIMIZER");
        p.optimizer.name = expect_string_literal();
        p.optimizer.kv = parse_kv_block();
        p.optimizer.present = true;
        continue;
      }
      if (peek_is_identifier("LR_SCHEDULER")) {
        if (p.lr_scheduler.present) {
          throw std::runtime_error(cuwacunu::piaabo::string_format(
              "duplicate LR_SCHEDULER block in PROFILE '%s'",
              p.name.c_str()));
        }
        consume_identifier("LR_SCHEDULER");
        p.lr_scheduler.name = expect_string_literal();
        p.lr_scheduler.kv = parse_kv_block();
        p.lr_scheduler.present = true;
        continue;
      }
      if (peek_is_identifier("LOSS")) {
        if (p.loss.present) {
          throw std::runtime_error(cuwacunu::piaabo::string_format(
              "duplicate LOSS block in PROFILE '%s'",
              p.name.c_str()));
        }
        consume_identifier("LOSS");
        p.loss.name = expect_string_literal();
        p.loss.kv = parse_kv_block();
        p.loss.present = true;
        continue;
      }
      if (peek_is_identifier("COMPONENT_PARAMS")) {
        if (p.component_params_present) {
          throw std::runtime_error(cuwacunu::piaabo::string_format(
              "duplicate COMPONENT_PARAMS block in PROFILE '%s'",
              p.name.c_str()));
        }
        consume_identifier("COMPONENT_PARAMS");
        p.component_params = parse_kv_block();
        p.component_params_present = true;
        continue;
      }
      if (peek_is_identifier("NUMERICS")) {
        if (p.numerics_present) {
          throw std::runtime_error(cuwacunu::piaabo::string_format(
              "duplicate NUMERICS block in PROFILE '%s'",
              p.name.c_str()));
        }
        consume_identifier("NUMERICS");
        p.numerics = parse_kv_block();
        p.numerics_present = true;
        continue;
      }
      if (peek_is_identifier("GRADIENT")) {
        if (p.gradient_present) {
          throw std::runtime_error(cuwacunu::piaabo::string_format(
              "duplicate GRADIENT block in PROFILE '%s'",
              p.name.c_str()));
        }
        consume_identifier("GRADIENT");
        p.gradient = parse_kv_block();
        p.gradient_present = true;
        continue;
      }
      if (peek_is_identifier("CHECKPOINT")) {
        if (p.checkpoint_present) {
          throw std::runtime_error(cuwacunu::piaabo::string_format(
              "duplicate CHECKPOINT block in PROFILE '%s'",
              p.name.c_str()));
        }
        consume_identifier("CHECKPOINT");
        p.checkpoint = parse_kv_block();
        p.checkpoint_present = true;
        continue;
      }
      if (peek_is_identifier("METRICS")) {
        if (p.metrics_present) {
          throw std::runtime_error(cuwacunu::piaabo::string_format(
              "duplicate METRICS block in PROFILE '%s'",
              p.name.c_str()));
        }
        consume_identifier("METRICS");
        p.metrics = parse_kv_block();
        p.metrics_present = true;
        continue;
      }
      if (peek_is_identifier("DATA_REF")) {
        if (p.data_ref_present) {
          throw std::runtime_error(cuwacunu::piaabo::string_format(
              "duplicate DATA_REF block in PROFILE '%s'",
              p.name.c_str()));
        }
        consume_identifier("DATA_REF");
        p.data_ref = parse_kv_block();
        p.data_ref_present = true;
        continue;
      }
      if (peek_is_identifier("AUGMENTATIONS")) {
        if (p.augmentations_present) {
          throw std::runtime_error(cuwacunu::piaabo::string_format(
              "duplicate AUGMENTATIONS block in PROFILE '%s'",
              p.name.c_str()));
        }
        p.augmentations = parse_augmentations(p.name);
        p.augmentations_present = true;
        continue;
      }

      const token_t bad = next();
      throw std::runtime_error(cuwacunu::piaabo::string_format(
          "unexpected token '%s' in PROFILE '%s' at %zu:%zu",
          bad.text.c_str(),
          p.name.c_str(),
          bad.line,
          bad.col));
    }

    return p;
  }

  static bool is_dash_only_token(const std::string& token) {
    if (token.empty()) return false;
    for (char c : token) {
      if (c != '-') return false;
    }
    return true;
  }

  static bool is_frame_token(const token_t& tok) {
    if (tok.kind != token_t::kind_e::Identifier || tok.text.empty()) return false;
    bool has_dash = false;
    for (char c : tok.text) {
      if (c == '-') {
        has_dash = true;
        continue;
      }
      if (c == '/' || c == '\\') continue;
      return false;
    }
    return has_dash;
  }

  std::vector<std::string> parse_table_row_cells() {
    std::vector<std::string> cells{};
    expect_symbol('|');
    while (true) {
      const token_t cell_start = peek();
      if (cell_start.kind == token_t::kind_e::Symbol && cell_start.text == "|") {
        throw std::runtime_error(cuwacunu::piaabo::string_format(
            "empty AUGMENTATIONS table cell at %zu:%zu",
            cell_start.line,
            cell_start.col));
      }
      std::vector<std::string> pieces{};
      while (true) {
        const token_t tok = peek();
        if (tok.kind == token_t::kind_e::End) {
          throw std::runtime_error("unterminated AUGMENTATIONS table row");
        }
        if (tok.kind == token_t::kind_e::Symbol && tok.text == "|") break;
        pieces.push_back(next().text);
      }
      std::ostringstream cell_builder;
      for (std::size_t i = 0; i < pieces.size(); ++i) {
        if (i > 0) cell_builder << ' ';
        cell_builder << pieces[i];
      }
      std::string cell = trim_ascii_copy(cell_builder.str());
      if (equals_ascii_ci(cell, "true")) cell = "true";
      if (equals_ascii_ci(cell, "false")) cell = "false";
      cells.push_back(std::move(cell));
      expect_symbol('|');

      const token_t next_tok = peek();
      if (next_tok.kind == token_t::kind_e::Symbol &&
          (next_tok.text == "|" || next_tok.text == "}" || next_tok.text == "[")) {
        break;
      }
      if (next_tok.kind == token_t::kind_e::End || is_frame_token(next_tok)) {
        break;
      }
    }
    return cells;
  }

  std::vector<curve_t> parse_augmentations_curve_blocks(const std::string& profile_name) {
    std::unordered_set<std::string> curve_names;
    std::vector<curve_t> curves{};

    while (!try_consume_symbol('}')) {
      consume_identifier("CURVE");
      curve_t curve{};
      curve.name = expect_string_literal();
      if (!curve_names.insert(curve.name).second) {
        throw std::runtime_error(cuwacunu::piaabo::string_format(
            "duplicate CURVE '%s' in PROFILE '%s' AUGMENTATIONS",
            curve.name.c_str(),
            profile_name.c_str()));
      }
      curve.kv = parse_kv_block();
      curves.push_back(std::move(curve));
    }

    return curves;
  }

  std::vector<curve_t> parse_augmentations_ascii_table(const std::string& profile_name,
                                                       bool expect_block_end) {
    static const std::vector<std::string> kExpectedHeader = {
        kAugmentationCurveKey,
        "active",
        "curve_param",
        "noise_scale",
        "smoothing_kernel_size",
        "point_drop_prob",
        "value_jitter_std",
        "time_mask_band_frac",
        "channel_dropout_prob",
        "comment",
    };

    while (!peek_is_symbol('|')) {
      if (peek_is_symbol('}')) {
        throw std::runtime_error(cuwacunu::piaabo::string_format(
            "empty AUGMENTATIONS table in PROFILE '%s'",
            profile_name.c_str()));
      }
      const token_t tok = next();
      if (!is_frame_token(tok)) {
        throw std::runtime_error(cuwacunu::piaabo::string_format(
            "unexpected token '%s' before AUGMENTATIONS table header in PROFILE '%s'",
            tok.text.c_str(),
            profile_name.c_str()));
      }
    }

    const std::vector<std::string> first_row = parse_table_row_cells();
    std::vector<std::string> canonical_first_row = first_row;
    if (!canonical_first_row.empty()) {
      canonical_first_row[0] =
          canonicalize_augmentation_curve_key(canonical_first_row[0]);
    }

    auto finish_table = [&]() {
      while (is_frame_token(peek())) (void)next();
      if (expect_block_end) {
        if (!peek_is_symbol('}')) {
          const token_t bad = peek();
          throw std::runtime_error(cuwacunu::piaabo::string_format(
              "unexpected token '%s' after AUGMENTATIONS table rows in PROFILE '%s'",
              bad.text.c_str(),
              profile_name.c_str()));
        }
        expect_symbol('}');
      }
    };

    if (canonical_first_row == kExpectedHeader) {
      std::unordered_set<std::string> curve_names;
      std::vector<curve_t> curves{};
      while (peek_is_symbol('|')) {
        const std::vector<std::string> row = parse_table_row_cells();
        if (row.size() == 1 && is_dash_only_token(row.front())) continue;
        if (row.size() != kExpectedHeader.size()) {
          throw std::runtime_error(cuwacunu::piaabo::string_format(
              "AUGMENTATIONS table row has %zu cells, expected %zu in PROFILE '%s'",
              row.size(),
              kExpectedHeader.size(),
              profile_name.c_str()));
        }

        curve_t curve{};
        curve.name = row[0];
        if (!curve_names.insert(curve.name).second) {
          throw std::runtime_error(cuwacunu::piaabo::string_format(
              "duplicate AUGMENTATIONS row name '%s' in PROFILE '%s'",
              curve.name.c_str(),
              profile_name.c_str()));
        }
        for (std::size_t i = 0; i < kExpectedHeader.size(); ++i) {
          curve.kv.entries.emplace_back(kExpectedHeader[i], row[i]);
        }
        curves.push_back(std::move(curve));
      }
      finish_table();
      if (curves.empty()) {
        throw std::runtime_error(cuwacunu::piaabo::string_format(
            "AUGMENTATIONS table requires at least one row in PROFILE '%s'",
            profile_name.c_str()));
      }
      return curves;
    }

    if (canonical_first_row.empty() ||
        canonical_first_row[0] != kAugmentationCurveKey ||
        canonical_first_row.size() < 2) {
      std::ostringstream expected;
      std::ostringstream got;
      for (std::size_t i = 0; i < kExpectedHeader.size(); ++i) {
        if (i > 0) expected << ", ";
        expected << kExpectedHeader[i];
      }
      for (std::size_t i = 0; i < first_row.size(); ++i) {
        if (i > 0) got << ", ";
        got << first_row[i];
      }
      throw std::runtime_error(cuwacunu::piaabo::string_format(
          "AUGMENTATIONS table header mismatch in PROFILE '%s'. expected row-header=[%s] or transposed first-row starting with '%s' (legacy alias '%s' also accepted), got=[%s]",
          profile_name.c_str(),
          expected.str().c_str(),
          kAugmentationCurveKey,
          kLegacyAugmentationCurveKey,
          got.str().c_str()));
    }

    std::vector<curve_t> curves(canonical_first_row.size() - 1);
    std::vector<std::unordered_set<std::string>> seen_curve_keys(
        canonical_first_row.size() - 1);
    std::unordered_set<std::string> curve_names;
    for (std::size_t i = 1; i < canonical_first_row.size(); ++i) {
      curves[i - 1].name = canonical_first_row[i];
      if (curves[i - 1].name.empty()) {
        throw std::runtime_error(cuwacunu::piaabo::string_format(
            "empty augmentation name in transposed AUGMENTATIONS table of PROFILE '%s'",
            profile_name.c_str()));
      }
      if (!curve_names.insert(curves[i - 1].name).second) {
        throw std::runtime_error(cuwacunu::piaabo::string_format(
            "duplicate AUGMENTATIONS row name '%s' in PROFILE '%s'",
            curves[i - 1].name.c_str(),
            profile_name.c_str()));
      }
    }

    while (peek_is_symbol('|')) {
      const std::vector<std::string> row = parse_table_row_cells();
      if (row.size() == 1 && is_dash_only_token(row.front())) continue;
      if (row.size() != canonical_first_row.size()) {
        throw std::runtime_error(cuwacunu::piaabo::string_format(
            "transposed AUGMENTATIONS row has %zu cells, expected %zu in PROFILE '%s'",
            row.size(),
            canonical_first_row.size(),
            profile_name.c_str()));
      }
      const std::string key = canonicalize_augmentation_curve_key(
          normalize_key_token(trim_ascii_copy(row[0])));
      if (key.empty()) {
        throw std::runtime_error(cuwacunu::piaabo::string_format(
            "empty transposed AUGMENTATIONS field name in PROFILE '%s'",
            profile_name.c_str()));
      }
      if (key == kAugmentationCurveKey) {
        for (std::size_t i = 1; i < row.size(); ++i) {
          if (row[i] != curves[i - 1].name) {
            throw std::runtime_error(cuwacunu::piaabo::string_format(
                "transposed AUGMENTATIONS %s row mismatch for column %zu in PROFILE '%s'",
                kAugmentationCurveKey,
                i,
                profile_name.c_str()));
          }
        }
        continue;
      }
      for (std::size_t i = 1; i < row.size(); ++i) {
        if (!seen_curve_keys[i - 1].insert(key).second) {
          throw std::runtime_error(cuwacunu::piaabo::string_format(
              "duplicate transposed AUGMENTATIONS field '%s' for curve '%s' in PROFILE '%s'",
              key.c_str(),
              curves[i - 1].name.c_str(),
              profile_name.c_str()));
        }
        curves[i - 1].kv.entries.emplace_back(key, row[i]);
      }
    }

    finish_table();
    if (curves.empty()) {
      throw std::runtime_error(cuwacunu::piaabo::string_format(
          "AUGMENTATIONS table requires at least one row in PROFILE '%s'",
          profile_name.c_str()));
    }
    return curves;
  }

  std::vector<curve_t> parse_augmentations(const std::string& profile_name) {
    consume_identifier("AUGMENTATIONS");
    expect_symbol('{');
    if (peek_is_identifier("CURVE")) {
      return parse_augmentations_curve_blocks(profile_name);
    }
    return parse_augmentations_ascii_table(profile_name, /*expect_block_end=*/true);
  }

  component_t parse_component() {
    consume_identifier("COMPONENT");

    component_t c{};
    c.canonical_type = expect_string_literal();
    c.id = c.canonical_type;
    expect_symbol('{');
    bool active_profile_set = false;
    std::unordered_set<std::string> profile_names;
    while (!try_consume_symbol('}')) {
      if (peek_is_identifier("PROFILE")) {
        profile_t profile = parse_profile();
        if (!profile_names.insert(profile.name).second) {
          throw std::runtime_error(cuwacunu::piaabo::string_format(
              "duplicate PROFILE '%s' in COMPONENT '%s'",
              profile.name.c_str(),
              c.id.c_str()));
        }
        c.profiles.push_back(std::move(profile));
        continue;
      }
      if (peek_is_identifier("ACTIVE_PROFILE")) {
        if (active_profile_set) {
          throw std::runtime_error(cuwacunu::piaabo::string_format(
              "duplicate ACTIVE_PROFILE in COMPONENT '%s'",
              c.id.c_str()));
        }
        consume_identifier("ACTIVE_PROFILE");
        expect_symbol(':');
        c.active_profile = parse_scalar_value();
        active_profile_set = true;
        continue;
      }

      const token_t bad = next();
      throw std::runtime_error(cuwacunu::piaabo::string_format(
          "unexpected token '%s' in COMPONENT '%s' at %zu:%zu",
          bad.text.c_str(),
          c.id.c_str(),
          bad.line,
          bad.col));
    }

    if (c.profiles.empty()) {
      throw std::runtime_error("COMPONENT has no PROFILE blocks");
    }
    if (c.active_profile.empty()) {
      throw std::runtime_error("COMPONENT missing ACTIVE_PROFILE (no silent defaults allowed)");
    }

    return c;
  }

  lexer_t lex_;
};
