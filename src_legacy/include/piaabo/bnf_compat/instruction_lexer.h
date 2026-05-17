/* bnf_grammar_lexer.h */
#pragma once
#include "piaabo/bnf_compat/parser_types.h"

namespace cuwacunu {
namespace piaabo {
namespace bnf {

class InstructionLexer {
private:
  std::string input;
  size_t pos;
public:
  InstructionLexer(const std::string& input = "")
    : input(input), pos(0) {}
public:
  void reset();
  char peek();
  char advance();
  char advance(size_t delta);
  bool isAtEnd() const;
  size_t getPosition() const;
  void setPosition(size_t position);
  void setInput(std::string dinput);
  std::string getInput() const;
  void skipWhitespace();
};

} /* namespace bnf */
} /* namespace piaabo */
} /* namespace cuwacunu */
