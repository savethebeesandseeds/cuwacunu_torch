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
  size_t initial_pos = iLexer.getPosition();

  /* try to match all alternatives */
  for (const ProductionAlternative& alternative : rule.rhs) {
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
  std::vector<ProductionUnit> units;
  std::vector<ASTNodePtr> children;

  /* emplace the units */
  switch (alt.type) {
    case ProductionAlternative::Type::Single:
      // return parse_ProductionUnit(std::get<ProductionUnit>(alt.content));
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


  return std::make_unique<IntermediaryNode>(alt, std::move(children));
}

ASTNodePtr InstructionParser::parse_ProductionUnit(const ProductionUnit& unit) {

  switch (unit.type) {
    
    case ProductionUnit::Type::Terminal:
      return parse_TerminalNode(unit);
    
    case ProductionUnit::Type::NonTerminal:
      return parse_ProductionRule(grammar.getRule(unit.lexeme));
    
    case ProductionUnit::Type::Optional: {
      std::string inner_lexeme = unit.lexeme.substr(1, unit.lexeme.size() - 2); /* remove the outter brachets [] */
      ASTNodePtr child = parse_ProductionUnit(
        ProductionUnit(
          ProductionUnit::Type::NonTerminal, inner_lexeme, unit.line, unit.column
        )
      );
      /* validate */
      if (child == nullptr) {
        /* since this is an optional unit on the abscense case we retirn a node with no children */
        return std::make_unique<IntermediaryNode>(unit);
      }

      return child;

      // std::vector<ASTNodePtr> children;
      // children.push_back(std::move(child));
      // return std::make_unique<IntermediaryNode>(unit, std::move(children));
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
  std::string lexeme = unit.lexeme;

  /* remove quotes */
  if ((lexeme.front() == '"' && lexeme.back() == '"') ||
      (lexeme.front() == '\'' && lexeme.back() == '\'')) {
    lexeme = lexeme.substr(1, lexeme.size() - 2);
  }

  /* parse terminal */
  for (char ch : lexeme) {
    if (iLexer.isAtEnd() || iLexer.peek() != ch) {
      /* Match failed */
      iLexer.setPosition(initial_pos);
      return nullptr;
    }
    iLexer.advance();
  }
  

  return std::make_unique<TerminalNode>(unit);
}

} /* namespace BNF */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
