// src/tests/bench/camahjucunu/bnf/test_bnf_iinuji_renderings.cpp
#include <iostream>
#include <string>

#include "piaabo/dconfig.h"
#include "camahjucunu/BNF/BNF_AST.h"
#include "camahjucunu/BNF/BNF_grammar_lexer.h"
#include "camahjucunu/BNF/BNF_grammar_parser.h"
#include "camahjucunu/BNF/BNF_instruction_lexer.h"
#include "camahjucunu/BNF/BNF_instruction_parser.h"

using namespace cuwacunu::camahjucunu::BNF;

int main() {
const std::string INPUT = 
R"(SCREEN F+7
  arg ARG2 path Arg1.DATALOADER presented_by TABLE COLUMNS "A,B,C"
    TRIGGERS ON_ENTER "LOAD" ON_CLICK SELECT ENDTRIGGERS
  PANEL P1 plot at 0 0 6 4 z 1 scale 1 bind ARG2
    draw CURVE D Y
    draw MDN_BAND Y Y_STD
  ENDPANEL
  PANEL T1 text at 0 4 6 2
    draw TEXT CONTENT "HELLO WORLD"
  ENDPANEL
ENDSCREEN
)";


  // read the configuration
  const char* config_folder = "/cuwacunu/src/config/";
  cuwacunu::piaabo::dconfig::config_space_t::change_config_file(config_folder);
  cuwacunu::piaabo::dconfig::config_space_t::update_config();

  std::string LANGUAGE = cuwacunu::piaabo::dconfig::config_space_t::iinuji_renderings_bnf();

  // Parse the grammar
  GrammarLexer glex(LANGUAGE);
  GrammarParser gpar(glex);
  gpar.parseGrammar();
  const ProductionGrammar& g = gpar.getGrammar();

  // Parse the instruction according to that grammar
  InstructionLexer ilex(INPUT);
  InstructionParser ipar(ilex, const_cast<ProductionGrammar&>(g)); // API requires non-const
  ASTNodePtr ast = ipar.parse_Instruction(INPUT);

  // Show results
  std::cout << "== Parsed ProductionGrammar ==\n";
  std::cout << g.str(0) << "\n";
  std::cout << "== AST ==\n";
  printAST(ast.get(), /*verbose=*/true, 0, std::cout);
  std::cout << std::endl;

  return 0;
}