#include "camahjucunu/dsl/tsiemene_board/tsiemene_board.h"

#include <cctype>
#include <filesystem>
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

[[nodiscard]] bool ends_with(std::string_view s, std::string_view suffix) {
  if (suffix.size() > s.size()) return false;
  return s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

[[nodiscard]] std::string sanitize_identifier(std::string value) {
  for (char& ch : value) {
    const bool alpha_num = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                           (ch >= '0' && ch <= '9');
    if (!alpha_num) ch = '_';
  }
  std::string out;
  out.reserve(value.size());
  bool last_underscore = false;
  for (const char ch : value) {
    if (ch == '_') {
      if (last_underscore) continue;
      last_underscore = true;
      out.push_back(ch);
      continue;
    }
    last_underscore = false;
    out.push_back(ch);
  }
  while (!out.empty() && out.front() == '_') out.erase(out.begin());
  while (!out.empty() && out.back() == '_') out.pop_back();
  if (out.empty()) out = "unnamed";
  if (!out.empty() && out.front() >= '0' && out.front() <= '9') {
    out.insert(out.begin(), '_');
  }
  return out;
}

[[nodiscard]] std::string derive_contract_id_from_file(const std::string& file_path,
                                                       std::size_t import_index) {
  std::string file_name = std::filesystem::path(file_path).filename().string();
  if (file_name.empty()) {
    return "contract_" + std::to_string(import_index + 1);
  }
  if (ends_with(file_name, ".board.contract.config")) {
    file_name.resize(file_name.size() - std::string(".board.contract.config").size());
  } else if (ends_with(file_name, ".contract.config")) {
    file_name.resize(file_name.size() - std::string(".contract.config").size());
  } else if (ends_with(file_name, ".config")) {
    file_name.resize(file_name.size() - std::string(".config").size());
  } else if (ends_with(file_name, ".ini")) {
    file_name.resize(file_name.size() - std::string(".ini").size());
  } else {
    file_name = std::filesystem::path(file_name).stem().string();
  }
  const std::string slug = sanitize_identifier(file_name);
  return "contract_" + slug;
}

[[nodiscard]] std::string derive_wave_import_id_from_file(const std::string& file_path,
                                                          std::size_t import_index) {
  std::string file_name = std::filesystem::path(file_path).filename().string();
  if (file_name.empty()) {
    return "wave_import_" + std::to_string(import_index + 1);
  }
  if (ends_with(file_name, ".dsl")) {
    file_name.resize(file_name.size() - std::string(".dsl").size());
  } else {
    file_name = std::filesystem::path(file_name).stem().string();
  }
  const std::string slug = sanitize_identifier(file_name);
  return "wave_import_" + slug;
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

  cuwacunu::camahjucunu::tsiemene_board_instruction_t parse() {
    using cuwacunu::camahjucunu::tsiemene_board_instruction_t;
    tsiemene_board_instruction_t out{};
    std::unordered_set<std::string> contract_ids{};
    std::unordered_set<std::string> wave_import_ids{};
    std::unordered_set<std::string> bind_ids{};
    std::unordered_set<std::string> contract_files{};
    std::unordered_set<std::string> wave_files{};

    expect_identifier("BOARD");
    expect_symbol('{');

    std::size_t contract_import_count = 0;
    std::size_t wave_import_count = 0;
    while (!peek_is_symbol('}')) {
      const token_t head = peek();
      if (head.kind != token_t::kind_e::Identifier) {
        throw std::runtime_error(
            "expected BOARD declaration at " + std::to_string(head.line) + ":" +
            std::to_string(head.col));
      }
      if (head.text == "IMPORT_CONTRACT_FILE") {
        auto c = parse_import_contract_decl(contract_import_count++);
        if (!contract_ids.insert(c.id).second) {
          throw std::runtime_error("duplicate CONTRACT id derived from import: " +
                                   c.id);
        }
        if (!contract_files.insert(c.file).second) {
          throw std::runtime_error("duplicate IMPORT_CONTRACT_FILE entry: " +
                                   c.file);
        }
        out.contracts.push_back(std::move(c));
        continue;
      }
      if (head.text == "IMPORT_WAVE_FILE") {
        auto w = parse_import_wave_decl(wave_import_count++);
        if (!wave_import_ids.insert(w.id).second) {
          throw std::runtime_error("duplicate WAVE import id derived from import: " +
                                   w.id);
        }
        if (!wave_files.insert(w.file).second) {
          throw std::runtime_error("duplicate IMPORT_WAVE_FILE entry: " + w.file);
        }
        out.waves.push_back(std::move(w));
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
      throw std::runtime_error("unknown BOARD declaration: " + head.text);
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
      throw std::runtime_error(
          "board instruction requires at least one IMPORT_CONTRACT_FILE");
    }
    if (out.waves.empty()) {
      throw std::runtime_error(
          "board instruction requires at least one IMPORT_WAVE_FILE");
    }
    if (out.binds.empty()) {
      throw std::runtime_error("board instruction requires at least one BIND");
    }

    for (const auto& bind : out.binds) {
      if (contract_ids.find(bind.contract_ref) == contract_ids.end()) {
        throw std::runtime_error("BIND '" + bind.id +
                                 "' references unknown CONTRACT id: " +
                                 bind.contract_ref);
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
      throw std::runtime_error("expected symbol '" + std::string(1, c) + "' at " +
                               std::to_string(t.line) + ":" +
                               std::to_string(t.col));
    }
  }

  token_t expect_identifier_any() {
    const token_t t = next();
    if (t.kind != token_t::kind_e::Identifier) {
      throw std::runtime_error("expected identifier at " + std::to_string(t.line) +
                               ":" + std::to_string(t.col));
    }
    return t;
  }

  void expect_identifier(const char* expected) {
    const token_t t = expect_identifier_any();
    if (t.text != expected) {
      throw std::runtime_error("expected '" + std::string(expected) + "' at " +
                               std::to_string(t.line) + ":" +
                               std::to_string(t.col) + ", got '" + t.text + "'");
    }
  }

  std::string parse_scalar_value() {
    const token_t t = next();
    if (t.kind != token_t::kind_e::Identifier &&
        t.kind != token_t::kind_e::String) {
      throw std::runtime_error("expected scalar value at " + std::to_string(t.line) +
                               ":" + std::to_string(t.col));
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

  cuwacunu::camahjucunu::tsiemene_board_contract_decl_t parse_import_contract_decl(
      std::size_t import_index) {
    using cuwacunu::camahjucunu::tsiemene_board_contract_decl_t;
    tsiemene_board_contract_decl_t out{};
    expect_identifier("IMPORT_CONTRACT_FILE");
    out.file = parse_scalar_value();
    expect_symbol(';');
    if (out.file.empty()) {
      throw std::runtime_error("IMPORT_CONTRACT_FILE missing path");
    }
    out.id = derive_contract_id_from_file(out.file, import_index);
    return out;
  }

  cuwacunu::camahjucunu::tsiemene_board_wave_decl_t parse_import_wave_decl(
      std::size_t import_index) {
    using cuwacunu::camahjucunu::tsiemene_board_wave_decl_t;
    tsiemene_board_wave_decl_t out{};
    expect_identifier("IMPORT_WAVE_FILE");
    out.file = parse_scalar_value();
    expect_symbol(';');
    if (out.file.empty()) {
      throw std::runtime_error("IMPORT_WAVE_FILE missing path");
    }
    out.id = derive_wave_import_id_from_file(out.file, import_index);
    return out;
  }

  cuwacunu::camahjucunu::tsiemene_board_bind_decl_t parse_bind_decl() {
    using cuwacunu::camahjucunu::tsiemene_board_bind_decl_t;
    tsiemene_board_bind_decl_t out{};
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
      throw std::runtime_error("unknown BIND key for '" + out.id + "': " + key.text);
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

  lexer_t lex_;
};

}  // namespace

namespace cuwacunu {
namespace camahjucunu {

namespace {

[[nodiscard]] bool has_non_ws_ascii_(std::string_view text) {
  for (const char ch : text) {
    if (!std::isspace(static_cast<unsigned char>(ch))) return true;
  }
  return false;
}

void validate_board_grammar_text_or_throw_(const std::string& grammar_text) {
  if (!has_non_ws_ascii_(grammar_text)) {
    throw std::runtime_error("tsiemene board grammar text is empty");
  }
  constexpr std::string_view kRequiredGrammarTokens[] = {
      "<board_block>",
      "<contract_import_decl>",
      "<wave_import_decl>",
      "<bind_decl>",
      "BOARD",
      "IMPORT_CONTRACT_FILE",
      "IMPORT_WAVE_FILE",
      "BIND",
      "CONTRACT",
      "WAVE",
  };
  for (const auto token : kRequiredGrammarTokens) {
    if (grammar_text.find(token) == std::string::npos) {
      throw std::runtime_error(
          "tsiemene board grammar missing required token: " +
          std::string(token));
    }
  }
}

}  // namespace

std::string tsiemene_board_instruction_t::str() const {
  std::ostringstream oss;
  oss << "tsiemene_board_instruction_t: contracts=" << contracts.size()
      << " waves=" << waves.size()
      << " binds=" << binds.size() << "\n";
  for (std::size_t i = 0; i < binds.size(); ++i) {
    const auto& b = binds[i];
    oss << "  [bind:" << i << "] id=" << b.id
        << " contract=" << b.contract_ref
        << " wave=" << b.wave_ref << "\n";
  }
  return oss.str();
}

namespace dsl {

tsiemeneBoardPipeline::tsiemeneBoardPipeline(std::string grammar_text)
    : grammar_text_(std::move(grammar_text)) {
  validate_board_grammar_text_or_throw_(grammar_text_);
}

tsiemene_board_instruction_t tsiemeneBoardPipeline::decode(
    std::string instruction) {
  std::lock_guard<std::mutex> lk(current_mutex_);
  parser_t parser(std::move(instruction));
  return parser.parse();
}

tsiemene_board_instruction_t decode_tsiemene_board_from_dsl(
    std::string grammar_text,
    std::string instruction_text) {
  tsiemeneBoardPipeline pipeline(std::move(grammar_text));
  return pipeline.decode(std::move(instruction_text));
}

}  // namespace dsl

}  // namespace camahjucunu
}  // namespace cuwacunu
