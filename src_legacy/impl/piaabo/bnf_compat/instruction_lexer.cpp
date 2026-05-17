/* bnf_grammar_lexer.cpp */
#include "piaabo/bnf_compat/instruction_lexer.h"

namespace cuwacunu {
namespace piaabo {
namespace bnf {
  
void InstructionLexer::reset() {
  setPosition(0);
}

char InstructionLexer::peek() {
  return isAtEnd() ? '\0' : input[pos];
}

char InstructionLexer::advance() {
  if (isAtEnd()) {
    return '\0';
  }
  return input[pos++];
}

char InstructionLexer::advance(size_t delta) {
  char ch = '\0';
  if (!isAtEnd()) {
    ch = input[pos];
  }
  pos += delta;
  if (pos > input.length()) {
    pos = input.length();
  }
  return ch;
}

bool InstructionLexer::isAtEnd() const {
  return pos >= input.length();
}

size_t InstructionLexer::getPosition() const {
  return pos;
}

void InstructionLexer::setPosition(size_t position) {
  pos = position;
}

std::string InstructionLexer::getInput() const {
  return input;
}

void InstructionLexer::setInput(std::string dinput) {
  input = dinput;
  reset();
}

void InstructionLexer::skipWhitespace() {
  while (!isAtEnd() && std::isspace(static_cast<unsigned char>(peek()))) {
    advance();
  }
}

} /* namespace bnf */
} /* namespace piaabo */
} /* namespace cuwacunu */
