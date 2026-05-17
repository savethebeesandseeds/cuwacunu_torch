/* bnf_grammar_parser.h */
#pragma once
#include "piaabo/parse/bnf/parser_types.h"
#include "piaabo/parse/bnf/grammar_lexer.h"

namespace cuwacunu {
namespace piaabo {
namespace parse {
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
} /* namespace parse */
} /* namespace piaabo */
} /* namespace cuwacunu */
