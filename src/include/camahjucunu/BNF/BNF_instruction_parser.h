/* BNF_grammar_parser.h */
#pragma once
#include "camahjucunu/BNF/BNF_AST.h"
#include "camahjucunu/BNF/BNF_types.h"
#include "camahjucunu/BNF/BNF_grammar_lexer.h"
#include "camahjucunu/BNF/BNF_instruction_lexer.h"
#include "camahjucunu/BNF/BNF_grammar_parser.h"

namespace cuwacunu {
namespace camahjucunu {
namespace BNF {

class InstructionParser {
private:
  InstructionLexer iLexer;
  ProductionGrammar grammar;
public:
  InstructionParser(InstructionLexer& iLexer, ProductionGrammar& grammar)
    : iLexer(iLexer), grammar(grammar) {
      iLexer.reset();
  }

  ASTNodePtr parse_Instruction(const std::string& instruction_input); /* left-hand side of <instruction> */

private:
  ASTNodePtr parse_ProductionRule(const ProductionRule& rule);
  ASTNodePtr parse_ProductionAlternative(const ProductionAlternative& alt);
  ASTNodePtr parse_ProductionUnit(const ProductionUnit& unit);
  
  ASTNodePtr parse_TerminalNode(const ProductionUnit& unit);
};

} /* namespace BNF */
} /* namespace camahjucunu */
} /* namespace cuwacunu */