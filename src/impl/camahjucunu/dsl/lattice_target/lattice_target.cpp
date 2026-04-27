#include "camahjucunu/dsl/lattice_target/lattice_target.h"

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

[[nodiscard]] bool is_valid_id(std::string_view value) {
  if (value.empty())
    return false;
  for (const char ch : value) {
    const bool alpha_num = (ch >= 'a' && ch <= 'z') ||
                           (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9');
    const bool symbol = ch == '_' || ch == '.' || ch == '-';
    if (!(alpha_num || symbol))
      return false;
  }
  return true;
}

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
    std::string out{};
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
    std::string out{};
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

  std::string src_{};
  std::size_t pos_{0};
  std::size_t line_{1};
  std::size_t col_{1};
  bool has_peek_{false};
  token_t peek_tok_{};
};

class parser_t {
public:
  explicit parser_t(std::string input) : lex_(std::move(input)) {}

  cuwacunu::camahjucunu::lattice_target_instruction_t parse() {
    using cuwacunu::camahjucunu::lattice_target_instruction_t;
    lattice_target_instruction_t out{};
    std::unordered_set<std::string> contract_ids{};
    std::unordered_set<std::string> contract_files{};
    std::unordered_set<std::string> target_ids{};

    expect_identifier("LATTICE_TARGET");
    out.id = parse_id_value("LATTICE_TARGET id");
    expect_symbol('{');

    while (!peek_is_symbol('}')) {
      const token_t head = peek();
      if (head.kind != token_t::kind_e::Identifier) {
        throw std::runtime_error("expected LATTICE_TARGET declaration at " +
                                 location(head));
      }
      if (head.text == "IMPORT_CONTRACT") {
        auto contract = parse_contract_import_decl();
        if (!contract_ids.insert(contract.id).second) {
          throw std::runtime_error("duplicate imported CONTRACT id: " +
                                   contract.id);
        }
        if (!contract_files.insert(contract.file).second) {
          throw std::runtime_error("duplicate imported CONTRACT path: " +
                                   contract.file);
        }
        out.contracts.push_back(std::move(contract));
        continue;
      }
      if (head.text == "TARGET") {
        auto target = parse_target_decl();
        if (!target_ids.insert(target.id).second) {
          throw std::runtime_error("duplicate TARGET id: " + target.id);
        }
        out.targets.push_back(std::move(target));
        continue;
      }
      throw std::runtime_error("unknown LATTICE_TARGET declaration: " +
                               head.text);
    }
    expect_symbol('}');
    if (peek_is_symbol(';'))
      expect_symbol(';');
    if (!peek_is_end()) {
      const token_t tail = peek();
      throw std::runtime_error("unexpected trailing tokens at " +
                               location(tail));
    }

    if (out.contracts.empty()) {
      throw std::runtime_error(
          "lattice target requires at least one IMPORT_CONTRACT");
    }
    if (out.targets.empty())
      throw std::runtime_error("lattice target requires at least one TARGET");
    for (const auto &target : out.targets) {
      if (contract_ids.find(target.contract_ref) == contract_ids.end()) {
        throw std::runtime_error(
            "TARGET '" + target.id +
            "' references unknown CONTRACT id: " + target.contract_ref);
      }
    }
    return out;
  }

private:
  token_t peek() { return lex_.peek(); }
  token_t next() { return lex_.next(); }

  static std::string location(const token_t &t) {
    return std::to_string(t.line) + ":" + std::to_string(t.col);
  }

  bool peek_is_end() { return peek().kind == token_t::kind_e::End; }

  bool peek_is_symbol(char c) {
    const token_t t = peek();
    return t.kind == token_t::kind_e::Symbol && t.text.size() == 1 &&
           t.text[0] == c;
  }

  void expect_symbol(char c) {
    const token_t t = next();
    if (!(t.kind == token_t::kind_e::Symbol && t.text.size() == 1 &&
          t.text[0] == c)) {
      throw std::runtime_error("expected symbol '" + std::string(1, c) +
                               "' at " + location(t));
    }
  }

  token_t expect_identifier_any() {
    const token_t t = next();
    if (t.kind != token_t::kind_e::Identifier) {
      throw std::runtime_error("expected identifier at " + location(t));
    }
    return t;
  }

  void expect_identifier(const char *expected) {
    const token_t t = expect_identifier_any();
    if (t.text != expected) {
      throw std::runtime_error("expected '" + std::string(expected) + "' at " +
                               location(t) + ", got '" + t.text + "'");
    }
  }

  std::string parse_scalar_value() {
    const token_t t = next();
    if (t.kind != token_t::kind_e::Identifier &&
        t.kind != token_t::kind_e::String) {
      throw std::runtime_error("expected scalar value at " + location(t));
    }
    return t.text;
  }

  std::string parse_id_value(const char *label) {
    const std::string value = parse_scalar_value();
    if (!is_valid_id(value)) {
      throw std::runtime_error(std::string(label) +
                               " must use [A-Za-z0-9_.-]+: " + value);
    }
    return value;
  }

  std::uint64_t parse_positive_u64_value(const char *label) {
    const std::string text = parse_scalar_value();
    std::uint64_t value = 0;
    const auto *begin = text.data();
    const auto *end = text.data() + text.size();
    const auto result = std::from_chars(begin, end, value);
    if (result.ec != std::errc{} || result.ptr != end || value == 0) {
      throw std::runtime_error(std::string(label) +
                               " must be a positive integer: " + text);
    }
    return value;
  }

  bool parse_bool_value(const char *label) {
    const std::string text = parse_scalar_value();
    if (text == "true")
      return true;
    if (text == "false")
      return false;
    throw std::runtime_error(std::string(label) +
                             " must be true or false: " + text);
  }

  std::string parse_assignment_scalar(const char *key) {
    expect_identifier(key);
    expect_symbol('=');
    std::string value = parse_scalar_value();
    expect_symbol(';');
    return value;
  }

  cuwacunu::camahjucunu::lattice_target_contract_decl_t
  parse_contract_import_decl() {
    cuwacunu::camahjucunu::lattice_target_contract_decl_t out{};
    expect_identifier("IMPORT_CONTRACT");
    out.file = parse_scalar_value();
    if (out.file.empty())
      throw std::runtime_error("IMPORT_CONTRACT missing path");
    expect_identifier("AS");
    out.id = parse_id_value("CONTRACT import id");
    expect_symbol(';');
    return out;
  }

  cuwacunu::camahjucunu::lattice_target_decl_t parse_target_decl() {
    using cuwacunu::camahjucunu::instrument_signature_validate;
    using cuwacunu::camahjucunu::lattice_target_decl_t;
    lattice_target_decl_t out{};
    expect_identifier("TARGET");
    out.id = parse_id_value("TARGET id");
    expect_symbol('{');

    bool has_contract = false;
    bool has_components = false;
    bool has_source = false;
    bool has_effort = false;
    bool has_require = false;

    while (!peek_is_symbol('}')) {
      const token_t key = peek();
      if (key.kind != token_t::kind_e::Identifier) {
        throw std::runtime_error("expected TARGET key at " + location(key));
      }
      if (key.text == "CONTRACT") {
        if (has_contract) {
          throw std::runtime_error("TARGET '" + out.id +
                                   "' has duplicate CONTRACT");
        }
        out.contract_ref = parse_assignment_scalar("CONTRACT");
        if (!is_valid_id(out.contract_ref)) {
          throw std::runtime_error(
              "TARGET '" + out.id +
              "' CONTRACT ref is not an id: " + out.contract_ref);
        }
        has_contract = true;
        continue;
      }
      if (key.text == "COMPONENT_TARGET") {
        if (has_components) {
          throw std::runtime_error("TARGET '" + out.id +
                                   "' has duplicate COMPONENT_TARGET");
        }
        out.component_targets = parse_component_target_block(out.id);
        has_components = true;
        continue;
      }
      if (key.text == "SOURCE") {
        if (has_source)
          throw std::runtime_error("TARGET '" + out.id +
                                   "' has duplicate SOURCE");
        out.source = parse_source_block(out.id);
        has_source = true;
        continue;
      }
      if (key.text == "EFFORT") {
        if (has_effort)
          throw std::runtime_error("TARGET '" + out.id +
                                   "' has duplicate EFFORT");
        out.effort = parse_effort_block(out.id);
        has_effort = true;
        continue;
      }
      if (key.text == "REQUIRE") {
        if (has_require)
          throw std::runtime_error("TARGET '" + out.id +
                                   "' has duplicate REQUIRE");
        out.require = parse_require_block(out.id);
        has_require = true;
        continue;
      }
      throw std::runtime_error("unknown TARGET key for '" + out.id +
                               "': " + key.text);
    }
    expect_symbol('}');
    if (peek_is_symbol(';'))
      expect_symbol(';');

    if (!has_contract || out.contract_ref.empty()) {
      throw std::runtime_error("TARGET '" + out.id + "' missing CONTRACT");
    }
    if (!has_components || out.component_targets.empty()) {
      throw std::runtime_error("TARGET '" + out.id +
                               "' missing COMPONENT_TARGET");
    }
    if (!has_source)
      throw std::runtime_error("TARGET '" + out.id + "' missing SOURCE");
    if (!has_effort)
      throw std::runtime_error("TARGET '" + out.id + "' missing EFFORT");
    if (!has_require)
      throw std::runtime_error("TARGET '" + out.id + "' missing REQUIRE");

    std::string signature_error{};
    if (!instrument_signature_validate(out.source.signature, false,
                                       "TARGET SOURCE <" + out.id + ">",
                                       &signature_error)) {
      throw std::runtime_error(signature_error);
    }
    return out;
  }

  std::vector<cuwacunu::camahjucunu::lattice_target_component_decl_t>
  parse_component_target_block(std::string_view target_id) {
    using cuwacunu::camahjucunu::lattice_target_component_decl_t;
    std::vector<lattice_target_component_decl_t> out{};
    std::unordered_set<std::string> slots{};
    expect_identifier("COMPONENT_TARGET");
    expect_symbol('{');
    while (!peek_is_symbol('}')) {
      lattice_target_component_decl_t component{};
      component.slot = parse_id_value("COMPONENT_TARGET slot");
      if (!slots.insert(component.slot).second) {
        throw std::runtime_error(
            "TARGET '" + std::string(target_id) +
            "' duplicate COMPONENT_TARGET slot: " + component.slot);
      }
      expect_symbol('=');
      expect_identifier("TAG");
      component.tag = parse_id_value("COMPONENT_TARGET TAG");
      expect_symbol(';');
      out.push_back(std::move(component));
    }
    expect_symbol('}');
    if (peek_is_symbol(';'))
      expect_symbol(';');
    if (out.empty()) {
      throw std::runtime_error("TARGET '" + std::string(target_id) +
                               "' COMPONENT_TARGET is empty");
    }
    return out;
  }

  cuwacunu::camahjucunu::lattice_target_source_decl_t
  parse_source_block(std::string_view target_id) {
    using cuwacunu::camahjucunu::instrument_signature_set_field;
    using cuwacunu::camahjucunu::lattice_target_source_decl_t;
    lattice_target_source_decl_t out{};
    std::unordered_set<std::string> seen{};
    bool saw_train = false;
    bool saw_validate = false;
    bool saw_test = false;

    expect_identifier("SOURCE");
    expect_symbol('{');
    while (!peek_is_symbol('}')) {
      const token_t key = expect_identifier_any();
      if (!seen.insert(key.text).second) {
        throw std::runtime_error("TARGET '" + std::string(target_id) +
                                 "' duplicate SOURCE key: " + key.text);
      }
      expect_symbol('=');
      std::string value = parse_scalar_value();
      expect_symbol(';');

      if (key.text == "TRAIN") {
        out.train_window = std::move(value);
        saw_train = true;
        continue;
      }
      if (key.text == "VALIDATE") {
        out.validate_window = std::move(value);
        saw_validate = true;
        continue;
      }
      if (key.text == "TEST") {
        out.test_window = std::move(value);
        saw_test = true;
        continue;
      }

      std::string field_error{};
      if (!instrument_signature_set_field(&out.signature, key.text,
                                          std::move(value), &field_error)) {
        throw std::runtime_error("TARGET '" + std::string(target_id) +
                                 "' SOURCE " + field_error);
      }
    }
    expect_symbol('}');
    if (peek_is_symbol(';'))
      expect_symbol(';');
    if (!saw_train || out.train_window.empty())
      throw std::runtime_error("TARGET '" + std::string(target_id) +
                               "' SOURCE missing TRAIN");
    if (!saw_validate || out.validate_window.empty())
      throw std::runtime_error("TARGET '" + std::string(target_id) +
                               "' SOURCE missing VALIDATE");
    if (!saw_test || out.test_window.empty())
      throw std::runtime_error("TARGET '" + std::string(target_id) +
                               "' SOURCE missing TEST");
    return out;
  }

  cuwacunu::camahjucunu::lattice_target_effort_decl_t
  parse_effort_block(std::string_view target_id) {
    using cuwacunu::camahjucunu::lattice_target_effort_decl_t;
    lattice_target_effort_decl_t out{};
    std::unordered_set<std::string> seen{};
    bool saw_epochs = false;
    bool saw_batch_size = false;
    bool saw_max_batches = false;

    expect_identifier("EFFORT");
    expect_symbol('{');
    while (!peek_is_symbol('}')) {
      const token_t key = expect_identifier_any();
      if (!seen.insert(key.text).second) {
        throw std::runtime_error("TARGET '" + std::string(target_id) +
                                 "' duplicate EFFORT key: " + key.text);
      }
      expect_symbol('=');
      if (key.text == "EPOCHS") {
        out.epochs = parse_positive_u64_value("EFFORT.EPOCHS");
        saw_epochs = true;
      } else if (key.text == "BATCH_SIZE") {
        out.batch_size = parse_positive_u64_value("EFFORT.BATCH_SIZE");
        saw_batch_size = true;
      } else if (key.text == "MAX_BATCHES_PER_EPOCH") {
        out.max_batches_per_epoch =
            parse_positive_u64_value("EFFORT.MAX_BATCHES_PER_EPOCH");
        saw_max_batches = true;
      } else {
        throw std::runtime_error("TARGET '" + std::string(target_id) +
                                 "' unknown EFFORT key: " + key.text);
      }
      expect_symbol(';');
    }
    expect_symbol('}');
    if (peek_is_symbol(';'))
      expect_symbol(';');
    if (!saw_epochs || !saw_batch_size || !saw_max_batches) {
      throw std::runtime_error("TARGET '" + std::string(target_id) +
                               "' EFFORT requires EPOCHS, BATCH_SIZE, and "
                               "MAX_BATCHES_PER_EPOCH");
    }
    return out;
  }

  cuwacunu::camahjucunu::lattice_target_require_decl_t
  parse_require_block(std::string_view target_id) {
    using cuwacunu::camahjucunu::lattice_target_require_decl_t;
    lattice_target_require_decl_t out{};
    std::unordered_set<std::string> seen{};
    bool saw_trained = false;
    bool saw_validated = false;
    bool saw_tested = false;

    expect_identifier("REQUIRE");
    expect_symbol('{');
    while (!peek_is_symbol('}')) {
      const token_t key = expect_identifier_any();
      if (!seen.insert(key.text).second) {
        throw std::runtime_error("TARGET '" + std::string(target_id) +
                                 "' duplicate REQUIRE key: " + key.text);
      }
      expect_symbol('=');
      if (key.text == "TRAINED") {
        out.trained = parse_bool_value("REQUIRE.TRAINED");
        saw_trained = true;
      } else if (key.text == "VALIDATED") {
        out.validated = parse_bool_value("REQUIRE.VALIDATED");
        saw_validated = true;
      } else if (key.text == "TESTED") {
        out.tested = parse_bool_value("REQUIRE.TESTED");
        saw_tested = true;
      } else {
        throw std::runtime_error("TARGET '" + std::string(target_id) +
                                 "' unknown REQUIRE key: " + key.text);
      }
      expect_symbol(';');
    }
    expect_symbol('}');
    if (peek_is_symbol(';'))
      expect_symbol(';');
    if (!saw_trained || !saw_validated || !saw_tested) {
      throw std::runtime_error("TARGET '" + std::string(target_id) +
                               "' REQUIRE needs TRAINED, VALIDATED, TESTED");
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
    if (std::isspace(static_cast<unsigned char>(ch)) == 0)
      return true;
  }
  return false;
}

void validate_lattice_target_grammar_text_or_throw_(
    const std::string &grammar_text) {
  if (!has_non_ws_ascii_(grammar_text))
    throw std::runtime_error("lattice target grammar text is empty");
  constexpr std::string_view kRequiredGrammarTokens[] = {
      "<instruction>",    "<contract_import_decl>",
      "<target_decl>",    "<component_target_block>",
      "<source_block>",   "<effort_block>",
      "<require_block>",  "LATTICE_TARGET",
      "IMPORT_CONTRACT",  "TARGET",
      "COMPONENT_TARGET", "SOURCE",
      "EFFORT",           "REQUIRE",
  };
  for (const auto token : kRequiredGrammarTokens) {
    if (grammar_text.find(token) == std::string::npos) {
      throw std::runtime_error(
          "lattice target grammar missing required token: " +
          std::string(token));
    }
  }
}

} // namespace

std::vector<lattice_target_obligation_t>
lattice_target_instruction_t::obligations() const {
  std::vector<lattice_target_obligation_t> out{};
  for (const auto &target : targets) {
    for (const auto &component : target.component_targets) {
      lattice_target_obligation_t obligation{};
      obligation.target_id = target.id;
      obligation.contract_ref = target.contract_ref;
      obligation.component_slot = component.slot;
      obligation.component_tag = component.tag;
      obligation.source_signature = target.source.signature;
      obligation.train_window = target.source.train_window;
      obligation.validate_window = target.source.validate_window;
      obligation.test_window = target.source.test_window;
      obligation.epochs = target.effort.epochs;
      obligation.batch_size = target.effort.batch_size;
      obligation.max_batches_per_epoch = target.effort.max_batches_per_epoch;
      obligation.require_trained = target.require.trained;
      obligation.require_validated = target.require.validated;
      obligation.require_tested = target.require.tested;
      out.push_back(std::move(obligation));
    }
  }
  return out;
}

std::string lattice_target_instruction_t::str() const {
  std::ostringstream oss;
  oss << "lattice_target_instruction_t: id=" << id
      << " contracts=" << contracts.size() << " targets=" << targets.size()
      << " obligations=" << obligations().size() << "\n";
  for (std::size_t i = 0; i < targets.size(); ++i) {
    const auto &target = targets[i];
    oss << "  [target:" << i << "] id=" << target.id
        << " contract=" << target.contract_ref
        << " components=" << target.component_targets.size()
        << " source=" << target.source.signature.symbol << "\n";
  }
  return oss.str();
}

namespace dsl {

latticeTargetPipeline::latticeTargetPipeline(std::string grammar_text)
    : grammar_text_(std::move(grammar_text)) {
  validate_lattice_target_grammar_text_or_throw_(grammar_text_);
}

lattice_target_instruction_t
latticeTargetPipeline::decode(std::string instruction) {
  std::lock_guard<std::mutex> lk(current_mutex_);
  parser_t parser(std::move(instruction));
  return parser.parse();
}

lattice_target_instruction_t
decode_lattice_target_from_dsl(std::string grammar_text,
                               std::string instruction_text) {
  latticeTargetPipeline pipeline(std::move(grammar_text));
  return pipeline.decode(std::move(instruction_text));
}

} // namespace dsl

} // namespace camahjucunu
} // namespace cuwacunu
