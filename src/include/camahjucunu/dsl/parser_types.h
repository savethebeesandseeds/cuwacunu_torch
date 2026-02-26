#pragma once

#include "piaabo/bnf_compat/ast.h"
#include "piaabo/bnf_compat/grammar_lexer.h"
#include "piaabo/bnf_compat/grammar_parser.h"
#include "piaabo/bnf_compat/instruction_lexer.h"
#include "piaabo/bnf_compat/instruction_parser.h"
#include "piaabo/bnf_compat/visitor.h"

namespace cuwacunu {
namespace camahjucunu {
namespace dsl {

namespace parser_core = ::cuwacunu::piaabo::bnf;

using parser_core::ASTNode;
using parser_core::ASTNodePtr;
using parser_core::ASTVisitor;
using parser_core::GrammarLexer;
using parser_core::GrammarParser;
using parser_core::InstructionLexer;
using parser_core::InstructionParser;
using parser_core::IntermediaryNode;
using parser_core::printAST;
using parser_core::ProductionGrammar;
using parser_core::ProductionUnit;
using parser_core::RootNode;
using parser_core::TerminalNode;
using parser_core::VisitorContext;

}  // namespace dsl
}  // namespace camahjucunu
}  // namespace cuwacunu
