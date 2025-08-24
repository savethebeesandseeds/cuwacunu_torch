/* BNF_grammar_lexer.cpp */
#include "camahjucunu/BNF/BNF_instruction_lexer.h"

namespace cuwacunu {
namespace camahjucunu {
namespace BNF {
  
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

} /* namespace BNF */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
