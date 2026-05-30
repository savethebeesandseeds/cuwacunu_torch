// bnf_types.cpp
#include "piaabo/bnf_compat/parser_types.h"

namespace cuwacunu {
namespace piaabo {
namespace bnf {

std::string ProductionUnit::str(bool verbose) const {
  std::ostringstream stream;
  switch (type) {
    case ProductionUnit::Type::Terminal:
      if(verbose) { stream << ANSI_COLOR_Dim_Cyan << "Terminal:" << ANSI_COLOR_RESET << " "; }
      stream << lexeme << " ";
      break;
    case ProductionUnit::Type::NonTerminal:
      if(verbose) { stream << ANSI_COLOR_Dim_Cyan << "NonTerminal:" << ANSI_COLOR_RESET << " "; }
      stream << lexeme << " ";
      break;
    case ProductionUnit::Type::Optional:
      if(verbose) { stream << ANSI_COLOR_Dim_Cyan << "Optional:" << ANSI_COLOR_RESET << " "; }
      stream << lexeme << " ";
      break;
    case ProductionUnit::Type::Repetition:
      if(verbose) { stream << ANSI_COLOR_Dim_Cyan << "Repetition:" << ANSI_COLOR_RESET << " "; }
      stream << lexeme << " ";
      break;
    case ProductionUnit::Type::Punctuation:
      if(verbose) { stream << ANSI_COLOR_Dim_Cyan << "Punctuation:" << ANSI_COLOR_RESET << " "; }
      stream << lexeme << " ";
      break;
    case ProductionUnit::Type::EndOfFile:
      if(verbose) { stream << ANSI_COLOR_Dim_Cyan << "EndOfFile:" << ANSI_COLOR_RESET << " "; }
      stream << lexeme << " ";
      break;
    case ProductionUnit::Type::Undetermined:
      if(verbose) { stream << ANSI_COLOR_Dim_Cyan << "Undetermined:" << ANSI_COLOR_RESET << " "; }
      stream << "null" << " ";
      break;
    default:
      log_secure_fatal("Undetermined ProductionUnit: [%d] with lexeme: %s\n", static_cast<int>(type), lexeme.c_str());
      break;
  }
  return stream.str();
}

std::string ProductionAlternative::str(bool verbose) const {
  std::ostringstream stream;
  switch (type) {
    case ProductionAlternative::Type::Single:
      if(verbose) { stream << ANSI_COLOR_Dim_Cyan << "Single:" << ANSI_COLOR_RESET << " "; }
      stream << std::get<ProductionUnit>(content).str(verbose) << " ";
      break;
    case ProductionAlternative::Type::Sequence:
      if(verbose) { stream << ANSI_COLOR_Dim_Cyan << "Sequence:" << ANSI_COLOR_RESET << " "; }
      for (size_t i = 0; i < std::get<std::vector<ProductionUnit>>(content).size(); ++i) {
        stream << std::get<std::vector<ProductionUnit>>(content)[i].str(verbose);
        stream << " ";
      }
      break;
    case ProductionAlternative::Type::Unknown:
      stream << ANSI_COLOR_Dim_Cyan << "Unknown!" << ANSI_COLOR_RESET << " ";
      break;
    default:
      log_secure_fatal("Unknown ProductionAlternative: [%d]\n", static_cast<int>(type));
      break;
  }
  return stream.str();
}

std::string ProductionRule::str(bool verbose) const {
  std::ostringstream stream;
  stream << lhs << " ::= ";
  for (size_t i = 0; i < rhs.size(); ++i) {
    stream << rhs[i].str(verbose);
    if(i < rhs.size() - 1) {
      stream << " | ";
    }
  }
  stream << " ; ";
  return stream.str();
}


/* ProductionAlternative Flags Bitwise OR */
ProductionAlternative::Flags operator|(ProductionAlternative::Flags lhs, ProductionAlternative::Flags rhs) {
  using underlying = std::underlying_type<ProductionAlternative::Flags>::type;
  return static_cast<ProductionAlternative::Flags>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
}

/* ProductionAlternative Flags Bitwise OR assignment */
ProductionAlternative::Flags& operator|=(ProductionAlternative::Flags& lhs, ProductionAlternative::Flags rhs) {
    lhs = lhs | rhs;
    return lhs;
}

/* ProductionAlternative Flags Bitwise AND */
ProductionAlternative::Flags operator&(ProductionAlternative::Flags lhs, ProductionAlternative::Flags rhs) {
  using underlying = std::underlying_type<ProductionAlternative::Flags>::type;
  return static_cast<ProductionAlternative::Flags>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
}

/* ProductionAlternative Flags Bitwise AND assignment */
ProductionAlternative::Flags& operator&=(ProductionAlternative::Flags& lhs, ProductionAlternative::Flags rhs) {
  lhs = lhs & rhs;
  return lhs;
}

/* ProductionAlternative Flags Bitwise NOT */
ProductionAlternative::Flags operator~(ProductionAlternative::Flags lhs) {
  using underlying = std::underlying_type<ProductionAlternative::Flags>::type;
  return static_cast<ProductionAlternative::Flags>(~static_cast<underlying>(lhs));
}

ProductionRule& ProductionGrammar::getRule(const std::string& lhs) {
  for (auto& rule : rules) {
    if (rule.lhs == lhs) {
      return rule;
    }
  }
  /* If not found, throw an exception or handle it as needed */
  throw std::invalid_argument("No production rule found with lhs: " + lhs);
}

ProductionRule& ProductionGrammar::getRule(const ProductionUnit unit) {
  if(unit.type == ProductionUnit::Type::Optional) {
    /* remove the surrounding brackets of optional */
    if (!unit.lexeme.empty() && unit.lexeme.front() == '[' && unit.lexeme.back() == ']') {
      return getRule(unit.lexeme.substr(1, unit.lexeme.size() - 2));
    }
  }
  return getRule(unit.lexeme);
}

ProductionRule& ProductionGrammar::getRule(size_t lhs_index) {
  if (lhs_index < rules.size()) {
    return rules[lhs_index];
  }
  /* If index is out of bounds, throw an exception or handle it as needed */
  throw std::out_of_range("Rule index out of range: " + std::to_string(lhs_index));
}

std::string ProductionGrammar::str(int indentLevel) const {
  std::ostringstream stream;
  /* Define the number of spaces per indentation level */
  const int spacesPerIndent = 4;
   /* Generate the indentation string based on indentLevel */
  std::string indent(spacesPerIndent * indentLevel, ' ');
   /* Iterate through each non-terminal and its rule */
  for (const auto& rule : rules) {
    stream << indent << " Rule : " << rule.lhs << "\n";
     /* Iterate through each alternative in the rule's RHS */
    for (const auto& alternative : rule.rhs) {
      /* */
      stream << indent << "\t Alternative: ";
      /* Iterate through each unit in the alternative */
      if(alternative.type == ProductionAlternative::Type::Sequence) {
        for (const auto& unit : std::get<std::vector<ProductionUnit>>(alternative.content)) {
          stream << unit.str() << " "; /* Add space between units */
        }
      } else if(alternative.type == ProductionAlternative::Type::Single) {
        stream << std::get<ProductionUnit>(alternative.content).str() << " "; /* Add space between units */
      } else {
        stream << "\t Unknown! ";
      }
      stream << "\n";
    }
  }
  return stream.str();
}

} // namespace bnf
} // namespace piaabo
} // namespace cuwacunu