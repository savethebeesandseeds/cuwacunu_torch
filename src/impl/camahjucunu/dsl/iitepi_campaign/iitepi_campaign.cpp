#include "camahjucunu/dsl/iitepi_campaign/iitepi_campaign.h"

#include <cctype>
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

[[nodiscard]] bool is_valid_import_id(std::string_view value) {
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
    return c == '{' || c == '}' || c == '=' || c == ';' || c == ',';
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

  cuwacunu::camahjucunu::iitepi_campaign_instruction_t parse() {
    using cuwacunu::camahjucunu::iitepi_campaign_instruction_t;
    iitepi_campaign_instruction_t out{};
    std::unordered_set<std::string> contract_ids{};
    std::unordered_set<std::string> wave_import_ids{};
    std::unordered_set<std::string> bind_ids{};
    std::unordered_set<std::string> contract_files{};
    bool saw_marshal_objective = false;

    expect_identifier("CAMPAIGN");
    expect_symbol('{');

    while (!peek_is_symbol('}')) {
      const token_t head = peek();
      if (head.kind != token_t::kind_e::Identifier) {
        throw std::runtime_error("expected CAMPAIGN declaration at " +
                                 std::to_string(head.line) + ":" +
                                 std::to_string(head.col));
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
      if (head.text == "FROM") {
        auto wave = parse_wave_import_decl();
        if (!wave_import_ids.insert(wave.id).second) {
          throw std::runtime_error("duplicate imported WAVE id: " + wave.id);
        }
        out.waves.push_back(std::move(wave));
        continue;
      }
      if (head.text == "MARSHAL") {
        if (saw_marshal_objective) {
          throw std::runtime_error("duplicate MARSHAL entry in CAMPAIGN block");
        }
        out.marshal_objective_file = parse_marshal_decl();
        saw_marshal_objective = true;
        continue;
      }
      if (head.text == "BIND") {
        auto b = parse_bind_decl();
        if (!bind_ids.insert(b.id).second) {
          throw std::runtime_error("duplicate BIND id: " + b.id);
        }
        out.binds.push_back(std::move(b));
        continue;
      }
      if (head.text == "RUN") {
        auto run = parse_run_decl();
        out.runs.push_back(std::move(run));
        continue;
      }
      throw std::runtime_error("unknown CAMPAIGN declaration: " + head.text);
    }
    expect_symbol('}');
    if (peek_is_symbol(';')) {
      expect_symbol(';');
    }
    if (!peek_is_end()) {
      const token_t tail = peek();
      throw std::runtime_error("unexpected trailing tokens at " +
                               std::to_string(tail.line) + ":" +
                               std::to_string(tail.col));
    }

    if (out.contracts.empty()) {
      throw std::runtime_error("iitepi campaign instruction requires at least "
                               "one IMPORT_CONTRACT ... AS ...");
    }
    if (out.waves.empty()) {
      throw std::runtime_error("iitepi campaign instruction requires at least "
                               "one FROM ... IMPORT_WAVE");
    }
    if (out.binds.empty()) {
      throw std::runtime_error(
          "iitepi campaign instruction requires at least one BIND");
    }
    if (out.runs.empty()) {
      throw std::runtime_error(
          "iitepi campaign instruction requires at least one RUN");
    }

    for (const auto &bind : out.binds) {
      if (contract_ids.find(bind.contract_ref) == contract_ids.end()) {
        throw std::runtime_error(
            "BIND '" + bind.id +
            "' references unknown CONTRACT id: " + bind.contract_ref);
      }
      if (wave_import_ids.find(bind.wave_ref) == wave_import_ids.end()) {
        throw std::runtime_error(
            "BIND '" + bind.id +
            "' references unknown WAVE id: " + bind.wave_ref);
      }
    }
    for (const auto &run : out.runs) {
      if (bind_ids.find(run.bind_ref) == bind_ids.end()) {
        throw std::runtime_error("RUN references unknown BIND id: " +
                                 run.bind_ref);
      }
    }

    return out;
  }

private:
  token_t peek() { return lex_.peek(); }
  token_t next() { return lex_.next(); }

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

  std::string parse_import_id_value(const char *import_kind) {
    const std::string value = parse_scalar_value();
    if (!is_valid_import_id(value)) {
      throw std::runtime_error(std::string(import_kind) +
                               " import id must use [A-Za-z0-9_.-]+: " + value);
    }
    return value;
  }

  std::string parse_assignment_value(const char *key) {
    expect_identifier(key);
    expect_symbol('=');
    std::string value = parse_scalar_value();
    expect_symbol(';');
    return value;
  }

  cuwacunu::camahjucunu::iitepi_campaign_contract_decl_t
  parse_contract_import_decl() {
    using cuwacunu::camahjucunu::iitepi_campaign_contract_decl_t;
    iitepi_campaign_contract_decl_t out{};
    expect_identifier("IMPORT_CONTRACT");
    out.file = parse_scalar_value();
    if (out.file.empty()) {
      throw std::runtime_error("IMPORT_CONTRACT missing path");
    }
    expect_identifier("AS");
    out.id = parse_import_id_value("CONTRACT");
    expect_symbol(';');
    return out;
  }

  cuwacunu::camahjucunu::iitepi_campaign_wave_decl_t parse_wave_import_decl() {
    using cuwacunu::camahjucunu::iitepi_campaign_wave_decl_t;
    iitepi_campaign_wave_decl_t out{};
    expect_identifier("FROM");
    out.file = parse_scalar_value();
    if (out.file.empty()) {
      throw std::runtime_error("FROM import missing path");
    }
    expect_identifier("IMPORT_WAVE");
    out.id = parse_import_id_value("WAVE");
    expect_symbol(';');
    return out;
  }

  std::string parse_marshal_decl() {
    expect_identifier("MARSHAL");
    std::string value = parse_scalar_value();
    expect_symbol(';');
    if (value.empty()) {
      throw std::runtime_error("MARSHAL missing objective path");
    }
    return value;
  }

  cuwacunu::camahjucunu::iitepi_campaign_bind_decl_t parse_bind_decl() {
    using cuwacunu::camahjucunu::iitepi_campaign_bind_decl_t;
    iitepi_campaign_bind_decl_t out{};
    expect_identifier("BIND");
    out.id = expect_identifier_any().text;
    expect_symbol('{');

    bool has_contract = false;
    bool has_wave = false;
    while (!peek_is_symbol('}')) {
      const token_t key = peek();
      if (key.kind != token_t::kind_e::Identifier) {
        throw std::runtime_error("expected BIND key at " +
                                 std::to_string(key.line) + ":" +
                                 std::to_string(key.col));
      }
      if (key.text == "CONTRACT") {
        out.contract_ref = parse_assignment_value("CONTRACT");
        has_contract = true;
        continue;
      }
      if (key.text == "WAVE") {
        out.wave_ref = parse_assignment_value("WAVE");
        has_wave = true;
        continue;
      }
      if (cuwacunu::camahjucunu::is_wave_contract_binding_variable_name(
              key.text)) {
        std::string variable_error{};
        if (!cuwacunu::camahjucunu::append_wave_contract_binding_variable(
                &out, key.text, parse_assignment_value(key.text.c_str()),
                &variable_error)) {
          throw std::runtime_error("BIND '" + out.id + "' " + variable_error);
        }
        continue;
      }
      throw std::runtime_error("unknown BIND key for '" + out.id +
                               "': " + key.text);
    }
    expect_symbol('}');
    expect_symbol(';');

    if (!has_contract || out.contract_ref.empty()) {
      throw std::runtime_error("BIND '" + out.id + "' missing CONTRACT");
    }
    if (!has_wave || out.wave_ref.empty()) {
      throw std::runtime_error("BIND '" + out.id + "' missing WAVE");
    }
    return out;
  }

  cuwacunu::camahjucunu::iitepi_campaign_run_decl_t parse_run_decl() {
    using cuwacunu::camahjucunu::iitepi_campaign_run_decl_t;
    iitepi_campaign_run_decl_t out{};
    expect_identifier("RUN");
    out.bind_ref = expect_identifier_any().text;
    expect_symbol(';');
    if (out.bind_ref.empty()) {
      throw std::runtime_error("RUN missing bind reference");
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

void validate_iitepi_campaign_grammar_text_or_throw_(
    const std::string &grammar_text) {
  if (!has_non_ws_ascii_(grammar_text)) {
    throw std::runtime_error("iitepi campaign grammar text is empty");
  }
  constexpr std::string_view kRequiredGrammarTokens[] = {
      "<instruction>",
      "<contract_import_decl>",
      "<wave_import_decl>",
      "<marshal_decl>",
      "<bind_decl>",
      "<run_decl>",
      "CAMPAIGN",
      "FROM",
      "IMPORT_CONTRACT",
      "AS",
      "IMPORT_WAVE",
      "MARSHAL",
      "BIND",
      "RUN",
      "CONTRACT",
      "WAVE",
  };
  for (const auto token : kRequiredGrammarTokens) {
    if (grammar_text.find(token) == std::string::npos) {
      throw std::runtime_error(
          "iitepi campaign grammar missing required token: " +
          std::string(token));
    }
  }
}

} // namespace

std::string iitepi_campaign_instruction_t::str() const {
  std::ostringstream oss;
  oss << "iitepi_campaign_instruction_t: contracts=" << contracts.size()
      << " waves=" << waves.size() << " binds=" << binds.size()
      << " runs=" << runs.size() << "\n";
  if (!marshal_objective_file.empty()) {
    oss << "  [marshal] objective_file=" << marshal_objective_file << "\n";
  }
  for (std::size_t i = 0; i < runs.size(); ++i) {
    const auto &run = runs[i];
    oss << "  [run:" << i << "] bind_ref=" << run.bind_ref << "\n";
  }
  for (std::size_t i = 0; i < binds.size(); ++i) {
    const auto &bind = binds[i];
    oss << "  [bind:" << i << "] id=" << bind.id
        << " contract=" << bind.contract_ref << " wave=" << bind.wave_ref
        << " variables=" << bind.variables.size() << "\n";
    for (std::size_t j = 0; j < bind.variables.size(); ++j) {
      oss << "    [var:" << j << "] " << bind.variables[j].name << "="
          << bind.variables[j].value << "\n";
    }
  }
  return oss.str();
}

namespace dsl {

iitepiCampaignPipeline::iitepiCampaignPipeline(std::string grammar_text)
    : grammar_text_(std::move(grammar_text)) {
  validate_iitepi_campaign_grammar_text_or_throw_(grammar_text_);
}

iitepi_campaign_instruction_t
iitepiCampaignPipeline::decode(std::string instruction) {
  std::lock_guard<std::mutex> lk(current_mutex_);
  parser_t parser(std::move(instruction));
  return parser.parse();
}

iitepi_campaign_instruction_t
decode_iitepi_campaign_from_dsl(std::string grammar_text,
                                std::string instruction_text) {
  iitepiCampaignPipeline pipeline(std::move(grammar_text));
  return pipeline.decode(std::move(instruction_text));
}

} // namespace dsl

} // namespace camahjucunu
} // namespace cuwacunu
