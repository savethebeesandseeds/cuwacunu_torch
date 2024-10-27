/* BNF_instruction_parser.cpp */
#include "camahjucunu/BNF/BNF_instruction_parser.h"

namespace cuwacunu {
namespace camahjucunu {
namespace BNF {

/* - - - - - - - - - - - - */
/*     InstructionParser   */
/* - - - - - - - - - - - - */

ASTNodePtr InstructionParser::parse_Instruction(const std::string& instruction_input) {
  log_warn("waka: parse_Instruction \n");
  const std::string lhs_instruction = "<instruction>";
  const ProductionRule& instruction_rule = grammar.getRule(lhs_instruction);

  // Set the lexer input to the instruction_input
  iLexer.setInput(instruction_input);
  iLexer.reset();

  ASTNodePtr root_node = parse_ProductionRule(instruction_rule);

  if (root_node == nullptr || !iLexer.isAtEnd()) {
    // Parsing failed or not all input was consumed
    throw std::runtime_error("Parsing failed: could not parse instruction: " + instruction_input);
  }

  std::vector<ASTNodePtr> nodes;
  nodes.push_back(std::move(root_node));
  return std::make_unique<RootNode>(lhs_instruction, std::move(nodes));
}

/* --- --- --- --- --- parse types --- --- --- --- --- ---  */

ASTNodePtr InstructionParser::parse_ProductionRule(const ProductionRule& rule) {
  log_warn("waka: parse_ProductionRule : %s\n", rule.str().c_str());
  size_t initial_pos = iLexer.getPosition();

  for (const ProductionAlternative& alternative : rule.rhs) {
    log_warn("waka: parse_ProductionRule loop\n");
    iLexer.setPosition(initial_pos); // Reset lexer to the start position for each alternative
    ASTNodePtr node = parse_ProductionAlternative(alternative);
    if (node != nullptr) {
      // Successfully parsed this alternative
      return node;
    }
  }

  // None of the alternatives matched
  log_warn("waka: parse_ProductionRule finished on failure\n");
  iLexer.setPosition(initial_pos);
  return nullptr;
}

ASTNodePtr InstructionParser::parse_ProductionAlternative(const ProductionAlternative& alt) {
  log_warn("waka: parse_ProductionAlternative : %s\n", alt.str().c_str());
  size_t initial_pos = iLexer.getPosition();
  std::vector<ASTNodePtr> children;

  switch (alt.type) {
    case ProductionAlternative::Type::Unknown:
    log_warn("waka: parse_ProductionAlternative \t ProductionAlternative::Type::Unknown \n");
    default:
      throw std::runtime_error(
        "Instruction Parsing Error: Unexpected ProductionAlternative of unknown type."
      );

    case ProductionAlternative::Type::Single: {
      const ProductionUnit& unit = std::get<ProductionUnit>(alt.content);
      ASTNodePtr child = parse_ProductionUnit(unit);
      if (child != nullptr) {
        log_warn("waka: parse_ProductionAlternative SINGLE finshed on success \n");
        children.push_back(std::move(child));
        return std::make_unique<IntermediaryNode>(alt, std::move(children));
      } else {
        log_warn("waka: parse_ProductionAlternative SINGLE finshed on failure \n");
        iLexer.setPosition(initial_pos);
        return nullptr;
      }
    }

    case ProductionAlternative::Type::Sequence: {
      log_warn("waka: parse_ProductionAlternative \t ProductionAlternative::Type::Sequence \n");
      const std::vector<ProductionUnit>& units = std::get<std::vector<ProductionUnit>>(alt.content);
      for (const ProductionUnit& unit : units) {

        log_warn("waka: parse_ProductionAlternative \t\t next unit: %s \n", unit.str().c_str());
        ASTNodePtr child = parse_ProductionUnit(unit);
        if (child != nullptr) {
          log_warn("waka: parse_ProductionAlternative SEQUENCE finished on success\n");
          children.push_back(std::move(child));
          initial_pos = iLexer.getPosition();
        } else {
          log_warn("waka: parse_ProductionAlternative SEQUENCE finished on failure\n");
          iLexer.setPosition(initial_pos);
          return nullptr;
        }
      }
      return std::make_unique<IntermediaryNode>(alt, std::move(children));
    }
  }
}

ASTNodePtr InstructionParser::parse_ProductionUnit(const ProductionUnit& unit) {
  log_warn("waka: parse_ProductionUnit : %s\n", unit.str().c_str());
  switch (unit.type) {
    case ProductionUnit::Type::Terminal:
    log_warn("waka: parse_ProductionUnit : ProductionUnit::Type::Terminal \n");
      return parse_TerminalNode(unit);

    case ProductionUnit::Type::Optional: {
      log_warn("waka: parse_ProductionUnit : ProductionUnit::Type::Optional \n");
      // Optional: extract the inner content
      // The lexeme is something like "[<item>]"
      // We need to remove the '[' and ']' and get the inner lexeme
      std::string inner_lexeme = unit.lexeme.substr(1, unit.lexeme.size() - 2);

      // Now create a ProductionUnit of type NonTerminal with this inner_lexeme
      ProductionUnit inner_unit(ProductionUnit::Type::NonTerminal, inner_lexeme, unit.line, unit.column);

      // Try to parse the inner unit
      ASTNodePtr child = parse_ProductionUnit(inner_unit);
      if (child != nullptr) {
        // Successfully parsed the optional content
        return std::make_unique<OptionalNode>(unit, std::move(child));
      } else {
        // Optional, return a node indicating the absence
        return std::make_unique<OptionalNode>(unit, nullptr);
      }
    }

    case ProductionUnit::Type::NonTerminal: {
      log_warn("waka: parse_ProductionUnit : ProductionUnit::Type::NonTerminal \n");
      // NonTerminal: parse according to its production rule
      const ProductionRule& rule = grammar.getRule(unit.lexeme);
      ASTNodePtr node = parse_ProductionRule(rule);
      if (node != nullptr) {
        return node;
      } else {
        // Parsing failed
        return nullptr;
      }
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

  // If the lexeme starts and ends with quotes, remove them for comparison
  if ((lexeme.front() == '"' && lexeme.back() == '"') ||
      (lexeme.front() == '\'' && lexeme.back() == '\'')) {
    lexeme = lexeme.substr(1, lexeme.size() - 2);
  }

  for (char ch : lexeme) {
    if (iLexer.isAtEnd() || iLexer.peek() != ch) {
      log_warn("waka: parse_TerminalNode: %s, initial_pos: %ld loop on letter: %c != %c : \t FAILURE \n", unit.str().c_str(), initial_pos, iLexer.peek(), ch);
      // Match failed
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
