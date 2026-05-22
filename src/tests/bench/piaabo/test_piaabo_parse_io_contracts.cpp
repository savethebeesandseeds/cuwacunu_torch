#include "piaabo/io/files.h"
#include "piaabo/parse/bnf/grammar_lexer.h"
#include "piaabo/parse/bnf/grammar_parser.h"
#include "piaabo/parse/bnf/instruction_lexer.h"
#include "piaabo/parse/bnf/instruction_parser.h"
#include "piaabo/parse/json/json_parsing.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

namespace bnf = cuwacunu::piaabo::parse::bnf;
namespace io = cuwacunu::piaabo::io;
namespace json = cuwacunu::piaabo::parse::json;

namespace {

struct csv_row_t {
  int id{};
  float value{};

  static csv_row_t from_csv(const std::string &line, char delimiter,
                            size_t line_number = 0) {
    const std::size_t comma = line.find(delimiter);
    if (comma == std::string::npos) {
      throw std::runtime_error("missing delimiter at line " +
                               std::to_string(line_number));
    }

    std::size_t used_id = 0;
    std::size_t used_value = 0;
    csv_row_t out{};
    out.id = std::stoi(line.substr(0, comma), &used_id, 10);
    out.value = std::stof(line.substr(comma + 1), &used_value);
    if (used_id != comma || used_value != line.size() - comma - 1) {
      throw std::runtime_error("trailing characters at line " +
                               std::to_string(line_number));
    }
    return out;
  }

  static csv_row_t from_binary(const char *data) {
    csv_row_t out{};
    std::memcpy(&out, data, sizeof(out));
    return out;
  }
};

void write_text(const std::string &path, const std::string &text) {
  std::ofstream out(path, std::ios::out | std::ios::trunc);
  assert(out && "expected writable test file");
  out << text;
}

bool file_exists(const std::string &path) {
  std::ifstream in(path);
  return static_cast<bool>(in);
}

template <typename Fn> void expect_throw(Fn &&fn) {
  bool threw = false;
  try {
    fn();
  } catch (const std::exception &) {
    threw = true;
  }
  assert(threw);
}

void test_json_validity_is_strict() {
  assert(json::json_fast_validity_check(
      R"({"id":"abc","ok":true,"items":[1,false,null,{"x":-2.5e3}]})"));
  assert(json::json_fast_validity_check(R"([1,2,{"nested":"yes"}])"));

  assert(!json::json_fast_validity_check(R"({"id" "missing-colon"})"));
  assert(!json::json_fast_validity_check(R"({"id": tru})"));
  assert(!json::json_fast_validity_check(R"({"id": 01})"));
  assert(!json::json_fast_validity_check(R"({"id":"abc"} trailing)"));
  assert(!json::json_fast_validity_check(R"("scalar")"));
}

void test_bnf_repetition_allows_zero_items() {
  const std::string grammar_text = R"(
<instruction> ::= "a" {<letter>} "z" ;
<letter>      ::= "b" ;
)";

  bnf::GrammarLexer grammar_lexer(grammar_text);
  bnf::GrammarParser grammar_parser(grammar_lexer);
  grammar_parser.parseGrammar();
  bnf::ProductionGrammar grammar = grammar_parser.getGrammar();

  bnf::InstructionLexer instruction_lexer;
  bnf::InstructionParser parser(instruction_lexer, grammar);
  assert(parser.parse_Instruction("az") != nullptr);
  assert(parser.parse_Instruction("abbbz") != nullptr);
  expect_throw([&] { parser.parse_Instruction("abx"); });
}

void test_csv_conversion_fails_without_partial_publish() {
  const std::string prefix =
      "/tmp/cuwacunu_piaabo_csv_contract_" + std::to_string(getpid());
  const std::string csv_path = prefix + ".csv";
  const std::string bin_path = prefix + ".bin";
  const std::string tmp_path = bin_path + ".tmp";

  std::remove(csv_path.c_str());
  std::remove(bin_path.c_str());
  std::remove(tmp_path.c_str());

  write_text(csv_path, "1,1.5\n2,2.5\n");
  io::csvFile_to_binary<csv_row_t>(csv_path, bin_path, 1);
  const auto first_rows = io::binaryFile_to_vector<csv_row_t>(bin_path, 1);
  assert(first_rows.size() == 2);
  assert(first_rows[0].id == 1);
  assert(first_rows[1].id == 2);

  write_text(csv_path, "3,3.5\nbad-line\n4,4.5\n");
  expect_throw(
      [&] { io::csvFile_to_binary<csv_row_t>(csv_path, bin_path, 1); });
  assert(!file_exists(tmp_path));

  const auto preserved_rows = io::binaryFile_to_vector<csv_row_t>(bin_path, 1);
  assert(preserved_rows.size() == 2);
  assert(preserved_rows[0].id == 1);
  assert(preserved_rows[1].id == 2);

  std::remove(csv_path.c_str());
  std::remove(bin_path.c_str());
  std::remove(tmp_path.c_str());
}

} // namespace

int main() {
  test_json_validity_is_strict();
  test_bnf_repetition_allows_zero_items();
  test_csv_conversion_fails_without_partial_publish();
  return 0;
}
