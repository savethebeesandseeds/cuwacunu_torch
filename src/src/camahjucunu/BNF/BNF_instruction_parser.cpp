/* BNF_instruction_parser.cpp */
#include "camahjucunu/BNF/BNF_instruction_parser.h"


RUNTIME_WARNING("(BNF_instruction_parser.cpp)[parse_ProductionRule] parse all alternatives in parse_ProductionRule, instead of breaking when there is a match, multiple matching altenatives gives good debuging information \n");
RUNTIME_WARNING("(BNF_instruction_parser.cpp)[parse_ProductionAlternative] ProductionAlternative::Flags are not used\n");

namespace cuwacunu {
namespace camahjucunu {
namespace BNF {

/* - - - - - - - - - - - - */
/*     InstructionParser   */
/* - - - - - - - - - - - - */

ASTNodePtr InstructionParser::parse_Instruction(const std::string& instruction_input) {
  log_warn("waka: parse_Instruction \n");
  
  /* initialize */
  const std::string lhs_instruction = "<instruction>";
  
  iLexer.setInput(instruction_input);
  iLexer.reset();

  /* parse Instruction Rule */
  ASTNodePtr root_node = parse_ProductionRule(grammar.getRule(lhs_instruction));

  /* validate */
  if (root_node == nullptr || !iLexer.isAtEnd()) {
    throw std::runtime_error(
      "Parsing failed: could not parse instruction: " + instruction_input
    );
  }

  /* return on success */
  std::vector<ASTNodePtr> nodes;
  nodes.push_back(std::move(root_node));
  return std::make_unique<RootNode>(lhs_instruction, std::move(nodes));
}

/* --- --- --- --- --- parse types --- --- --- --- --- ---  */

ASTNodePtr InstructionParser::parse_ProductionRule(const ProductionRule& rule) {
  log_warn("waka: parse_ProductionRule : %s\n", rule.str().c_str());
  size_t initial_pos = iLexer.getPosition();

  /* try to match all alternatives */
  for (const ProductionAlternative& alternative : rule.rhs) {
    log_warn("waka: parse_ProductionRule : \t\t next alternative: %s \n",  alternative.str().c_str());
    /* reset the lexer to validate alternative match */
    iLexer.setPosition(initial_pos);
    ASTNodePtr node = parse_ProductionAlternative(alternative);

    /* validate HINT : we could see all alternatives and determine what to choose later */
    if (node != nullptr) { 
      return node; 
    }
  }

  /* none of the alternatives matched */
  iLexer.setPosition(initial_pos);
  return nullptr;
}

ASTNodePtr InstructionParser::parse_ProductionAlternative(const ProductionAlternative& alt) {
  log_warn("waka: parse_ProductionAlternative : %s\n", alt.str().c_str());
  std::vector<ProductionUnit> units;
  std::vector<ASTNodePtr> children;

  /* emplace the units */
  switch (alt.type) {
    case ProductionAlternative::Type::Single:
      units.push_back(std::get<ProductionUnit>(alt.content));
      break;
    case ProductionAlternative::Type::Sequence:
      units.insert(units.end(), std::get<std::vector<ProductionUnit>>(alt.content).begin(), std::get<std::vector<ProductionUnit>>(alt.content).end());
      break;
    case ProductionAlternative::Type::Unknown:
    default:
      throw std::runtime_error(
        "Instruction Parsing Error: Unexpected ProductionAlternative of unknown type."
      );
  }

  if(units.empty()) {
    return nullptr;
  }

  /* parse the individual units */
  for (const ProductionUnit& unit : units) {
    log_warn("waka: parse_ProductionAlternative \t\t next unit: %s \n", unit.str().c_str());
    size_t initial_pos = iLexer.getPosition();

    /* parse unit */
    ASTNodePtr child = parse_ProductionUnit(unit);
    
    /* validate */
    if (child == nullptr) {
      /* one of the units did not match, alternative failure */
      iLexer.setPosition(initial_pos);
      return nullptr;
    }

    /* append child */
    children.push_back(std::move(child));
  }


  log_info("waka dao [1] alt: %s\n", alt.str().c_str());
  return std::make_unique<IntermediaryNode>(alt, std::move(children));
}

ASTNodePtr InstructionParser::parse_ProductionUnit(const ProductionUnit& unit) {

  log_warn("waka: parse_ProductionUnit : %s\n", unit.str().c_str());
  switch (unit.type) {
    
    case ProductionUnit::Type::Terminal:
      return parse_TerminalNode(unit);
    
    case ProductionUnit::Type::NonTerminal:
      return parse_ProductionRule(grammar.getRule(unit.lexeme));
    
    case ProductionUnit::Type::Optional: {
      log_info("waka OPTIONAL ------------------------------------------ \n");
      std::string inner_lexeme = unit.lexeme.substr(1, unit.lexeme.size() - 2); /* remove the outter brachets [] */
      ASTNodePtr child = parse_ProductionUnit(
        ProductionUnit(
          ProductionUnit::Type::NonTerminal, inner_lexeme, unit.line, unit.column
        )
      );
      /* validate */
      if (child == nullptr) {
          log_info("waka OPTIONAL --------- child != nullptr \n");
        /* since this is an optional unit on the abscense case we retirn a node with no children */
        log_info("waka dao [2]\n");
        return std::make_unique<IntermediaryNode>(unit);
      }

      std::vector<ASTNodePtr> children;
      children.push_back(std::move(child));
      log_info("waka dao [3] unit: %s\n", unit.str().c_str());
      return std::make_unique<IntermediaryNode>(unit, std::move(children));
    }

    case ProductionUnit::Type::Repetition: {
        std::string inner_lexeme = unit.lexeme.substr(1, unit.lexeme.size() - 2); // remove the outer brackets {}
        std::vector<ASTNodePtr> children;
        ASTNodePtr child;

        do {
          child = parse_ProductionUnit(
            ProductionUnit(ProductionUnit::Type::NonTerminal, inner_lexeme, unit.line, unit.column)
          );
          
          if (child == nullptr) {
            break;
          }
          children.push_back(std::move(child)); // Move the unique_ptr into the vector

        } while (true);

        /* return the node, note children might be empty for the zero case */
        log_info("waka dao [4]\n");
        return std::make_unique<IntermediaryNode>(unit, std::move(children));
    }
    case ProductionUnit::Type::Punctuation:
    case ProductionUnit::Type::EndOfFile:
    case ProductionUnit::Type::Unknown:
    default:
      throw std::runtime_error(
        "Instruction Parsing Error: Unexpected ProductionUnit type: " + unit.str()
      );
  }
}

ASTNodePtr InstructionParser::parse_TerminalNode(const ProductionUnit& unit) {
  
  size_t initial_pos = iLexer.getPosition();
  log_warn("waka: parse_TerminalNode: %s, initial_pos: %ld \n", unit.str().c_str(), initial_pos);
  std::string lexeme = unit.lexeme;

  /* remove quotes */
  if ((lexeme.front() == '"' && lexeme.back() == '"') ||
      (lexeme.front() == '\'' && lexeme.back() == '\'')) {
    lexeme = lexeme.substr(1, lexeme.size() - 2);
  }

  /* parse terminal */
  for (char ch : lexeme) {
    log_warn("waka \n");
    if (iLexer.isAtEnd() || iLexer.peek() != ch) {
      log_warn("waka: parse_TerminalNode: %s, initial_pos: %ld loop on letter: %c != %c : \t FAILURE \n", unit.str().c_str(), initial_pos, iLexer.peek(), ch);
      /* Match failed */
      iLexer.setPosition(initial_pos);
      return nullptr;
    }
    iLexer.advance();
  }
  
  log_warn("waka: parse_TerminalNode: %s \t\t\t\t\t\t SUCCESS \n", unit.str().c_str());

  return std::make_unique<TerminalNode>(unit);
}

} /* namespace BNF */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
