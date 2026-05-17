/* bnf_grammar_parser.h */
#pragma once
#include <stack>
#include "piaabo/parse/bnf/ast.h"
#include "piaabo/parse/bnf/parser_types.h"
#include "piaabo/parse/bnf/grammar_lexer.h"
#include "piaabo/parse/bnf/instruction_lexer.h"
#include "piaabo/parse/bnf/grammar_parser.h"

namespace cuwacunu {
namespace piaabo {
namespace parse {
namespace bnf {

class InstructionParser {
private:
  InstructionLexer iLexer;
  ProductionGrammar grammar;
  std::stack<std::string> parsing_error_stack;
  std::stack<std::string> parsing_success_stack;
  size_t failure_position;
public:
  InstructionParser(InstructionLexer& iLexer, ProductionGrammar& grammar)
    : iLexer(iLexer), grammar(grammar), parsing_error_stack(), parsing_success_stack() {
      iLexer.reset();
  }

  ASTNodePtr parse_Instruction(const std::string& instruction_input); /* left-hand side of <instruction> */

private:
  ASTNodePtr parse_ProductionRule(const ProductionRule& rule);
  ASTNodePtr parse_ProductionAlternative(const ProductionAlternative& alt);
  ASTNodePtr parse_ProductionUnit(const ProductionAlternative& alt, const ProductionUnit& unit);
  
  ASTNodePtr parse_TerminalNode(const std::string& lhs, const ProductionUnit& unit);
};

} /* namespace bnf */
} /* namespace parse */
} /* namespace piaabo */
} /* namespace cuwacunu */