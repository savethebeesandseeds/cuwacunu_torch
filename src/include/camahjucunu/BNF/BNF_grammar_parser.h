/* BNF_grammar_parser.h */
#pragma once
#include "camahjucunu/BNF/BNF_types.h"
#include "camahjucunu/BNF/BNF_grammar_lexer.h"

namespace cuwacunu {
namespace camahjucunu {
namespace BNF {

class GrammarParser {
private:
  GrammarLexer& lexer;
  ProductionUnit currentUnit;
  ProductionGrammar grammar;
public:
  GrammarParser(GrammarLexer& lexer)
    : lexer(lexer), currentUnit(), grammar() {
      lexer.reset();
    }

  const ProductionGrammar& getGrammar() const;
  void parseGrammar();

private:
  void advanceUnit();
  ProductionRule parseNextProductionRule();
  ProductionAlternative parseProductionAlternative(std::string lhs_lexeme, const std::vector<ProductionUnit>& rhs_units);

};

/* --- --- --- --- --- --- --- --- --- --- --- --- */
/*                    UTILS                        */
/* --- --- --- --- --- --- --- --- --- --- --- --- */

void verityGrammar(const ProductionGrammar& dgrammar);

} /* namespace BNF */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
