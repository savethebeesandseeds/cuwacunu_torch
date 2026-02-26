/* bnf_grammar_parser.cpp */
#include "piaabo/bnf_compat/grammar_parser.h"

RUNTIME_WARNING("(bnf_grammar_parser.cpp)[] guard printing the errors with secure methods \n");
RUNTIME_WARNING("(bnf_grammar_parser.cpp)[] could use better grammar verification \n");


namespace cuwacunu {
namespace piaabo {
namespace bnf {

/* - - - - - - - - - - - - */
/*         Utils           */
/* - - - - - - - - - - - - */

std::string print_listof_units(const std::vector<ProductionUnit>& units) {
  std::ostringstream oss;
  oss << units;
  return oss.str();
}

/* - - - - - - - - - - - - */
/*         Checks          */
/* - - - - - - - - - - - - */

bool check_symb_present(const std::vector<ProductionUnit>& units, cuwacunu::piaabo::bnf::ProductionUnit::Type check_type, std::string check_sym) {
  /* determine */
  for (const auto& unit : units) {
    if (unit.type == check_type && unit.lexeme == check_sym) {
      return true;
    }
  }
  /* not found */
  return false;
}

bool check_includes_type(const std::vector<ProductionUnit>& units, cuwacunu::piaabo::bnf::ProductionUnit::Type check_type) {
  /* determine */
  for (const auto& unit : units) {
    if (unit.type == check_type) {
      return true;
    }
  }
  /* not found */
  return false;
}


static inline bool check_is_informationUnit(const ProductionUnit& unit) {
  switch (unit.type) {
    case ProductionUnit::Type::Terminal:
    case ProductionUnit::Type::NonTerminal:
    case ProductionUnit::Type::Optional:
    case ProductionUnit::Type::Repetition:
      return true;
    default:
      return false;
  }
}

/* - - - - - - - - - - - - */
/*      Validations        */
/* - - - - - - - - - - - - */

void validate_is_Instruction(ProductionUnit unit) {
  if(unit.type != ProductionUnit::Type::NonTerminal || unit.lexeme != "<instruction>") {
    throw std::runtime_error(
          "Grammar Syntax Error: Initial ProductionRule must start with '<instruction> ::= ' found instead '" + unit.lexeme +
          "' at line " + std::to_string(unit.line) +
          ", column " + std::to_string(unit.column)
    );
  }
}

void validate_isNot_ProductionOperator(ProductionUnit unit) {
  if (unit.type == ProductionUnit::Type::Punctuation && unit.lexeme == "::=") {
    throw std::runtime_error(
      "Grammar Syntax Error: Unexpected ProductionOperator, found '" + unit.lexeme +
      "' at line " + std::to_string(unit.line) +
      ", column " + std::to_string(unit.column)
    );
  }
}

void validate_is_ProductionOperator(ProductionUnit unit) {
  if (unit.type != ProductionUnit::Type::Punctuation || unit.lexeme != "::=") {
    throw std::runtime_error(
      "Grammar Syntax Error: Expected '::=' after left-hand size non-terminal '" + unit.lexeme +
      "' at line " + std::to_string(unit.line) +
      ", column " + std::to_string(unit.column)
    );
  }
}

void validate_is_InformationUnit(ProductionUnit unit) {
  if ( ! check_is_informationUnit(unit)) {
    throw std::runtime_error(
      "Grammar Syntax Error: Expected \"Terminal\", '<NonTerminal>', {<Repetition>} or [<Optional>] unit after ProductionOperator ::= '" + unit.lexeme + 
      "' at line " + std::to_string(unit.line) +
      ", column " + std::to_string(unit.column)
    );
  }
}

void validate_isNot_Nonterminal(ProductionUnit unit) {
  if (unit.type != ProductionUnit::Type::NonTerminal) {
    throw std::runtime_error(
      "Grammar Syntax Error: Expected a non-terminal on the left-hand side '" + unit.lexeme + 
      "' at line " + std::to_string(unit.line) +
      ", column " + std::to_string(unit.column)
    );
  }
}

void validate_is_semicolon(ProductionUnit unit) {
  if (unit.type != ProductionUnit::Type::Punctuation || unit.lexeme != ";") {
    throw std::runtime_error(
      "Grammar Syntax Error: Expected ';' at the end of each production (each line) for right-hand side '" + unit.lexeme + 
      "' at line " + std::to_string(unit.line) +
      ", column " + std::to_string(unit.column)
    );
  }
}


/* - - - - - - - - - - - - */
/*       GrammarParser         */
/* - - - - - - - - - - - - */

const ProductionGrammar& GrammarParser::getGrammar() const {
  return grammar;
}

void GrammarParser::advanceUnit() {
  currentUnit = lexer.getNextUnit();
}

void GrammarParser::parseGrammar() {
  /* first step */
  lexer.reset();
  advanceUnit();

  /* validate first production */
  validate_is_Instruction(currentUnit);

  /* parse all the productions until the eand of file is reached */
  while (currentUnit.type != ProductionUnit::Type::EndOfFile) {
    grammar.rules.push_back(
      std::move(parseNextProductionRule())
    );
  }

  /* final steps */
  verityGrammar(getGrammar());
}

/* 
 * @brief Parses a single production rule from the BNF input.
 * 
 * This function processes a line in the BNF grammar, extracting the left-hand side (LHS)
 * non-terminal and its corresponding right-hand side (RHS) alternatives. It ensures
 * that the syntax adheres to the expected DSL format and populates the grammar with the
 * parsed production rules.
 */
ProductionRule GrammarParser::parseNextProductionRule() {
  /* Initialize a new ProductionRule instance to store the current rule */
  ProductionRule rule;

  /* left-hand side of a rule should be a NonTerminal */
  validate_isNot_Nonterminal(currentUnit);

  /* Assign the lexeme of the current unit to the left-hand side (lhs) of the rule. */
  rule.lhs = currentUnit.lexeme;
      
  /* Advance to the next unit, which should be the production operator '::='. */
  advanceUnit();

  validate_is_ProductionOperator(currentUnit);

  /* Consume the production operator '::=' */
  advanceUnit();
  
  /* 
   * `rhs_units_alt` is a vector of vectors, where each inner vector represents a sequence of units
   * for a specific alternative in the production rule.
   *
   * **Example:**
   * Consider the following DSL ProductionRule:
   *     <Expression> ::= <Term> "+" <Expression> | <Term> ;
   * 
   * In this case, `rhs_units_alt` would be structured as follows:
   * [
   *   [ <Term>, "+", <Expression> ], // units in ProductionUnit[0]
   *   [ <Term> ]                     // units in ProductionUnit[1]
   * ]
   */
  std::vector<std::vector<ProductionUnit>> rhs_units_alt;
  rhs_units_alt.emplace_back();
  int alternative_index = 0;

  /* 
   * The loop processes each unit in the RHS, handling multiple alternatives separated by '|'.
   */
  do {
    
    if (currentUnit.type == ProductionUnit::Type::Punctuation && currentUnit.lexeme == "|") { 
      /* Initialize a new unit list for the next alternative. */
      
      alternative_index++;        /* increment index */
      rhs_units_alt.emplace_back();  /* empty next */

      /* Consume the alternative separator operator '|' */
      advanceUnit();
    }

    /* new alternative */

    validate_is_InformationUnit(currentUnit); /* terminal, non-terminal or optional*/

    rhs_units_alt[alternative_index].push_back(currentUnit);
    
    advanceUnit();
    
    validate_isNot_ProductionOperator(currentUnit);

    if(lexer.isAtEnd()) {
      break;
    }

  } while (currentUnit.type != ProductionUnit::Type::Punctuation || currentUnit.lexeme != ";"); // Continue until the end of the production rule is reached.

  /* Validate that the production rule ends with a semicolon ';' */
  validate_is_semicolon(currentUnit);

  /* Consume the terminating semicolon ';' */
  advanceUnit();

  /* Parse each RHS alternative and add it to the production rule. */
  for (const auto& units_in_alternative : rhs_units_alt) {
    /* Add the fully parsed production rule to the grammar */
    rule.rhs.push_back(
      parseProductionAlternative(rule.lhs, units_in_alternative)
    );
  }

  return rule;
}

ProductionAlternative GrammarParser::parseProductionAlternative(std::string lhs_lexeme, const std::vector<ProductionUnit>& rhs_units) {

  /* validate its not empty */
  if(rhs_units.size() == 0) {
    throw std::runtime_error(
      "Grammar Syntax Error: Not undestood empty right-hand side alternative at line " + std::to_string(currentUnit.line)
    );
  }

  /* 
   * Alternative is a Sequence of Units
   */
  if(rhs_units.size() > 1) {
    /* initialize */
    std::vector<ProductionUnit> dunits;
    ProductionAlternative::Flags dflags = ProductionAlternative::Flags::None;

    /* parse units */
    for (auto unit : rhs_units) {
      if(check_is_informationUnit(unit)) {
        /* push back unit */
        dunits.push_back(unit);
        
        /* determine flags */
        auto is_rec = [&]()->bool {
          if (unit.type == ProductionUnit::Type::NonTerminal) return unit.lexeme == lhs_lexeme;
          if (unit.type == ProductionUnit::Type::Optional || unit.type == ProductionUnit::Type::Repetition) {
            if (unit.lexeme.size() >= 3 && (unit.lexeme.front()=='[' || unit.lexeme.front()=='{')) {
              const std::string inner = unit.lexeme.substr(1, unit.lexeme.size()-2);
              return inner == lhs_lexeme;
            }
          }
          return false;
        }();
        if (is_rec) dflags |= ProductionAlternative::Flags::Recursion;
        dflags |= unit.type == ProductionUnit::Type::Optional ? ProductionAlternative::Flags::Optional : ProductionAlternative::Flags::None;
        dflags |= unit.type == ProductionUnit::Type::Repetition ? ProductionAlternative::Flags::Repetition : ProductionAlternative::Flags::None;
      }
    }
    
    /* validate alternative */
    if(dunits.size() == 0) {
      throw std::runtime_error(
        "Grammar Syntax Error: Empty information on right-hand side alternative : ..." + 
          print_listof_units(rhs_units) + "... at line " + std::to_string(currentUnit.line)
      );
    }

    /* return Alternative as a Sequence (specified as dunits is a std::vector) */
    return ProductionAlternative(lhs_lexeme, dunits, dflags);
  }

  /* 
   * Alternative is a Single Unit
   */

  /* in this point we expect only to be one unit (Terminal or NonTerminal) */
  if(rhs_units.size() != 1) {
    throw std::runtime_error(
      "Grammar Syntax Error: Not undestood right-hand side alternative : ..." + 
        print_listof_units(rhs_units) + "... at line " + std::to_string(currentUnit.line)
    );
  }

  ProductionUnit dunit = rhs_units[0];
  ProductionAlternative::Flags dflags = ProductionAlternative::Flags::None;

  /* check the one element contains information */
  if( ! check_is_informationUnit(dunit)) {
    throw std::runtime_error(
      "Grammar Syntax Error: Right-hand side alternative has no information unit : ..." + 
        print_listof_units(rhs_units) + "... at line " + std::to_string(currentUnit.line)
    );
  }

  /* determine flags*/
  dflags |= lhs_lexeme == dunit.lexeme ? ProductionAlternative::Flags::Recursion : ProductionAlternative::Flags::None;
  if (dunit.type == ProductionUnit::Type::Optional || dunit.type == ProductionUnit::Type::Repetition) {
    if (dunit.lexeme.size() >= 3 && (dunit.lexeme.front()=='[' || dunit.lexeme.front()=='{')) {
      const std::string inner = dunit.lexeme.substr(1, dunit.lexeme.size()-2);
      if (inner == lhs_lexeme) dflags |= ProductionAlternative::Flags::Recursion;
    }
  }
  dflags |= dunit.type == ProductionUnit::Type::Optional ? ProductionAlternative::Flags::Optional : ProductionAlternative::Flags::None;
  dflags |= dunit.type == ProductionUnit::Type::Repetition ? ProductionAlternative::Flags::Repetition : ProductionAlternative::Flags::None;

  /* ProductionAlternative::Type::Single with ProductionAlternative::Flags::Recursion generates infinite recursion */
  if((dflags & ProductionAlternative::Flags::Recursion) == ProductionAlternative::Flags::Recursion) {
    throw std::runtime_error(
      "Grammar Syntax Error: Infinite recursion found at Right-hand side alternative, TIP: recursion alternative must add information ..." + 
        print_listof_units(rhs_units) + "... at line " + std::to_string(currentUnit.line)
    );
  }

  return ProductionAlternative(lhs_lexeme, dunit, dflags);
}

/* --- --- --- --- --- --- --- --- --- --- --- --- */
/*                    UTILS                        */
/* --- --- --- --- --- --- --- --- --- --- --- --- */
void verityGrammar(const ProductionGrammar& dgrammar) {
  int line_number = 0;
  std::vector<std::string> lhs_check_for_duplicates;

  for(const auto& rule : dgrammar.rules) { /* all ProductionRule in the ProductionGrammar */
    line_number++;
    lhs_check_for_duplicates.push_back(rule.lhs);
    for(const auto& alternative : rule.rhs) { /* all ProductionAlternative on the ProductionRule */
      
      /* ProductionUnits in the ProductionAlternative */
      
      /* ProductionAlternative::Type::Single with ProductionAlternative::Flags::Recursion generates infinite recursion */
      if( alternative.type == ProductionAlternative::Type::Single && (alternative.flags & ProductionAlternative::Flags::Recursion) == ProductionAlternative::Flags::Recursion ) {
        throw std::runtime_error(
          "Grammar Syntax Error: Infinite recursion found on validation at Right-hand side single alternative, TIP: recursion alternative must add information " 
            + rule.lhs + " at line " + std::to_string(line_number)
        );
      }
      
      /* ProductionAlternative::Type::Sequence */
      if(alternative.type == ProductionAlternative::Type::Sequence) {
        for (const auto& unit : std::get<std::vector<ProductionUnit>>(alternative.content)) {
          validate_is_InformationUnit(unit);
        }
      } 
      
      /* ProductionAlternative::Type::Single */
      else if(alternative.type == ProductionAlternative::Type::Single) {
        ProductionUnit unit = std::get<ProductionUnit>(alternative.content);
        validate_is_InformationUnit(unit);
      } 
      
      /* Default, Unknown */
      else {
        throw std::runtime_error(
          "Grammar Syntax Error: Unable to parse ProductionAlternative of Type::Unknown, on Rule: " 
            + rule.lhs + " at line: " + std::to_string(line_number)
        );
      }
    }
  }

  /* check for duplicates (no two ProductionRules should have the same left-hand side) */
  std::sort(lhs_check_for_duplicates.begin(), lhs_check_for_duplicates.end());
  if (std::distance(std::unique(
        lhs_check_for_duplicates.begin(), 
        lhs_check_for_duplicates.end()), 
      lhs_check_for_duplicates.end()) > 0) {
    /* lhs duplicates found */
    throw std::runtime_error(
      "Grammar Syntax Error: Duplicated elements found in Grammar, please review the grammar file"
    );
  }
}

} /* namespace bnf */
} /* namespace piaabo */
} /* namespace cuwacunu */
