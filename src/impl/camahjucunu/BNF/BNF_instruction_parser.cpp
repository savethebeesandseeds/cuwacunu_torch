/* BNF_instruction_parser.cpp */
#include "camahjucunu/BNF/BNF_instruction_parser.h"

RUNTIME_WARNING("(BNF_instruction_parser.cpp)[] overall the methods in this file can be faster\n");
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
  failure_position = 0; /* safe init */

  /* reset the stacks */
  while (!parsing_error_stack.empty())  { parsing_error_stack.pop();   };
  while (!parsing_success_stack.empty()){ parsing_success_stack.pop(); };

  /* parse Instruction Rule */
  ASTNodePtr root_node = parse_ProductionRule(grammar.getRule(lhs_instruction));

  /* validate */
  if (root_node == nullptr || !iLexer.isAtEnd()) {
    size_t count = 0;
    size_t max_success_stack = 50;
    /* print the report in case of failure */
    std::ostringstream err_oss, scss_oss;
    while (!parsing_error_stack.empty()) { err_oss << parsing_error_stack.top() << "\n"; parsing_error_stack.pop(); }
    if(parsing_success_stack.size() > max_success_stack) { scss_oss << "\t\t ...truncated to size " << max_success_stack << "...\n";}
    while (!parsing_success_stack.empty() && ++count < max_success_stack) { scss_oss << parsing_success_stack.top() << "\n"; parsing_success_stack.pop(); }
    throw std::runtime_error(
      "Parsing failed: could not parse instruction: " + 
      std::string(ANSI_COLOR_Dim_Green)  + instruction_input.substr(0, failure_position) + std::string(ANSI_COLOR_RESET) + 
      std::string(ANSI_COLOR_Bright_Red) + instruction_input.substr(failure_position, 1) + std::string(ANSI_COLOR_RESET) + 
      std::string(ANSI_COLOR_Dim_Yellow) + instruction_input.substr(failure_position +1) + std::string(ANSI_COLOR_RESET) + 
      " \n\t Production Failures Stack: \n" + err_oss.str() + 
      " \n\t Production Success Stack: \n" + scss_oss.str() + "\n"
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
  std::vector<std::pair<ASTNodePtr, size_t>> matches;

  /* try to match all alternatives */
  for (const ProductionAlternative& alternative : rule.rhs) {

    /* reset the lexer to validate alternative match */
    iLexer.setPosition(initial_pos);
    ASTNodePtr node = parse_ProductionAlternative(alternative);
    
    /* validate HINT : we could see all alternatives and determine what to choose later */
    if (node != nullptr) {
      /* Append to success: store the node and the new lexer position */
      matches.emplace_back(std::move(node), iLexer.getPosition());
    }
  }

  /* Determine if there was a success */
  if (!matches.empty()) {
    if(matches.size() > 1) {
      /* push the problem to the stack */
      parsing_error_stack.push(
        cuwacunu::piaabo::string_format("        : --- --- : >> %sMultiple Alternatives%s [%ld]: found for rule %s", ANSI_COLOR_Yellow, ANSI_COLOR_RESET, matches.size(), rule.str(false).c_str())
      );
    }
    /* Find the match with the longest consumed input */
    auto best_match_iter = std::max_element( matches.begin(), matches.end(),
      [&](const std::pair<ASTNodePtr, size_t>& a, const std::pair<ASTNodePtr, size_t>& b) -> bool { return a.second < b.second; }
    );
    
    /* Advance the to the position after the best match */
    iLexer.setPosition(best_match_iter->second);

    /* push the success to the stack */
    parsing_success_stack.push(
      cuwacunu::piaabo::string_format("        :        : --- --- >> parsed %sparse_ProductionRule%s : %s", ANSI_COLOR_Bright_Green, ANSI_COLOR_RESET, best_match_iter->first->str(true).c_str())
    );

    /* Return the AST node corresponding to the best match */
    return std::move(best_match_iter->first);
  }

  /* push the problem to the stack */
  parsing_error_stack.push(
    cuwacunu::piaabo::string_format("        : --- --- : >> %sUnable%s to parse %sRule%s: %s", ANSI_COLOR_Bright_Red, ANSI_COLOR_RESET, ANSI_COLOR_Cyan, ANSI_COLOR_RESET, rule.str(false).c_str())
  );
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
      units.push_back(std::get<ProductionUnit>(alt.content));
      break;
    case ProductionAlternative::Type::Sequence:
      units.insert(units.end(), std::get<std::vector<ProductionUnit>>(alt.content).begin(), std::get<std::vector<ProductionUnit>>(alt.content).end());
      break;
    case ProductionAlternative::Type::Unknown:
    default:
      throw std::runtime_error(
        "Instruction Parsing Error: Unexpected ProductionAlternative of unknown type." + alt.str()
      );
  }

  /* validate */
  if(units.empty()) {
    return nullptr;
  }

  /* parse the individual units */
  for (const ProductionUnit& unit : units) {
    size_t initial_pos = iLexer.getPosition();

    /* parse unit */
    ASTNodePtr parsedChild = parse_ProductionUnit(alt, unit);
    
    /* validate */
    if (parsedChild == nullptr) {
      /* one of the units did not match, alternative failure */
      iLexer.setPosition(initial_pos);
      return nullptr;
    }

    /* Determine how to push the children */
    if (unit.type == ProductionUnit::Type::Repetition) {
      for (auto& child : dynamic_cast<IntermediaryNode*>(parsedChild.get())->children) {
        children.emplace_back(std::move(child));
      }
    } else {
      children.emplace_back(std::move(parsedChild));
    }
  }

  /* if there is only one */
  if (alt.type == ProductionAlternative::Type::Single && units[0].type == ProductionUnit::Type::Terminal) {
    return std::move(children[0]);
  }

  /* if it was a sequence of units */
  return std::make_unique<IntermediaryNode>(alt, std::move(children));
}

ASTNodePtr InstructionParser::parse_ProductionUnit(const ProductionAlternative& alt, const ProductionUnit& unit) {

  switch (unit.type) {
    
    case ProductionUnit::Type::Terminal:
      return parse_TerminalNode(alt.lhs, unit);
    
    case ProductionUnit::Type::NonTerminal:
      return parse_ProductionRule(grammar.getRule(unit.lexeme));
    
    case ProductionUnit::Type::Optional: {
      std::string inner_lexeme = unit.lexeme.substr(1, unit.lexeme.size() - 2); /* remove the outter brachets [] */
      ASTNodePtr child = parse_ProductionUnit(alt, 
        ProductionUnit(
          ProductionUnit::Type::NonTerminal, inner_lexeme, unit.line, unit.column
        )
      );
      /* validate */
      if (child == nullptr) {
        /* since this is an optional unit on the abscense case we retirn a node with no children */
        return std::make_unique<TerminalNode>(alt.lhs);
      }

      return child;
    }

    case ProductionUnit::Type::Repetition: {
        std::string inner_lexeme = unit.lexeme.substr(1, unit.lexeme.size() - 2); /* remove the outer brackets {} */
        std::vector<ASTNodePtr> children;
        ASTNodePtr child;

        do {
          child = parse_ProductionRule(grammar.getRule(inner_lexeme));

          if (child == nullptr) {
            break;
          }
          
          children.push_back(std::move(child));
        } while (true);

        /* validate */
        if(children.empty()){
          return nullptr;
        }

        /* return the node, note children might be empty for the zero case */
        return std::make_unique<IntermediaryNode>(alt, std::move(children));
    }
    case ProductionUnit::Type::Punctuation:
    case ProductionUnit::Type::EndOfFile:
    case ProductionUnit::Type::Undetermined:
    default:
      throw std::runtime_error(
        "Instruction Parsing Error: Unexpected ProductionUnit type: " + unit.str()
      );
  }
}


std::string unescape(const std::string& str) {
  std::string result;
  for (size_t i = 0; i < str.size(); ++i) {
    if (str[i] == '\\' && i + 1 < str.size()) {
      switch (str[i + 1]) {
        case 'n': result += '\n'; ++i; break;
        case 'r': result += '\r'; ++i; break;
        case 't': result += '\t'; ++i; break;
        case '\\': result += '\\'; ++i; break;
        case '"': result += '"'; ++i; break;
        case '\'': result += '\''; ++i; break;
        default:
          // If unknown escape sequence, include both characters as-is
          result += '\\';
          result += str[i + 1];
          ++i;
          break;
      }
    } else {
      result += str[i];
    }
  }
  return result;
}

std::string scape(const char ch) {
  std::string result;
  switch (ch) {
    case '\n': result += "\\n"; break;
    case '\r': result += "\\r"; break;
    case '\t': result += "\\t"; break;
    case '"':  result += "\\\""; break;
    // case '\\': result += "\\\\"; break;
    case '\'': result += "\\\'"; break;
    default:
      result += ch;
      break;
  }
  return result;
}
std::string scape(const std::string& str) {
  std::string result;
  for (char ch : str) {
    result += scape(ch);
  }
  return result;
}


ASTNodePtr InstructionParser::parse_TerminalNode(const std::string& lhs, const ProductionUnit& unit) {
  size_t initial_pos = iLexer.getPosition();
  std::string lexeme = unit.lexeme;

  /* remove quotes */
  if ((lexeme.front() == '"' && lexeme.back() == '"') ||
      (lexeme.front() == '\'' && lexeme.back() == '\'')) {
    lexeme = lexeme.substr(1, lexeme.size() - 2);
  }

  /* handle escape sequences */
  lexeme = unescape(lexeme);

  /* parse terminal */
  for (char ch : lexeme) {
    if (iLexer.isAtEnd() || iLexer.peek() != ch) {
      /* push the error to the stack */
      parsing_error_stack.push(
        cuwacunu::piaabo::string_format("        :        : --- --- >> Unable to parse %sTerminal Node%s : %s :  trying to match terminal: \"%s\" for character \'%s\' having lexer at character: \'%s\'", ANSI_COLOR_Bright_Red, ANSI_COLOR_RESET, unit.str(true).c_str(), lexeme.c_str(), scape(ch).c_str(), scape(iLexer.peek()).c_str())
      );
      /* save the failure position */
      failure_position = iLexer.getPosition();
      /* Match failed */
      iLexer.setPosition(initial_pos);
      return nullptr;
    }
    iLexer.advance();
  }
  
  /* reset the error stack (to avoid it growing to large) */
  while (!parsing_error_stack.empty()) { parsing_error_stack.pop(); };

  return std::make_unique<TerminalNode>(lhs, unit);
}

} /* namespace BNF */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
