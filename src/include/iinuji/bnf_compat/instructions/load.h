#pragma once
#include <string>

#include "piaabo/dconfig.h"
#include "camahjucunu/BNF/BNF_grammar_lexer.h"
#include "camahjucunu/BNF/BNF_grammar_parser.h"
#include "camahjucunu/BNF/BNF_instruction_lexer.h"
#include "camahjucunu/BNF/BNF_instruction_parser.h"
#include "camahjucunu/BNF/BNF_AST.h"

#include "camahjucunu/BNF/implementations/iinuji_renderings/iinuji_renderings.h"

namespace cuwacunu::iinuji {

namespace BNF = cuwacunu::camahjucunu::BNF;

inline cuwacunu::camahjucunu::iinuji_renderings_instruction_t
load_instruction_from_config()
{
  std::string LANGUAGE = cuwacunu::piaabo::dconfig::config_space_t::iinuji_renderings_bnf();
  std::string INPUT    = cuwacunu::piaabo::dconfig::config_space_t::iinuji_renderings_instruction();

  BNF::GrammarLexer glex(LANGUAGE);
  BNF::GrammarParser gparser(glex);
  gparser.parseGrammar();
  BNF::ProductionGrammar grammar = gparser.getGrammar();

  BNF::InstructionLexer ilex(INPUT);
  BNF::InstructionParser iparser(ilex, grammar);
  BNF::ASTNodePtr root = iparser.parse_Instruction(INPUT);

  cuwacunu::camahjucunu::iinuji_renderings_decoder_t decoder;
  return decoder.decode(root.get());
}

} // namespace cuwacunu::iinuji
