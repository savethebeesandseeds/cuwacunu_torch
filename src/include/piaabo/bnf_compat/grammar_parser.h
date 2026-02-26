/* bnf_grammar_parser.h */
#pragma once
#include "piaabo/bnf_compat/parser_types.h"
#include "piaabo/bnf_compat/grammar_lexer.h"

namespace cuwacunu {
namespace piaabo {
namespace bnf {

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

} /* namespace bnf */
} /* namespace piaabo */
} /* namespace cuwacunu */
