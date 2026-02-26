/* bnf_types.h */
#pragma once
#include <memory>
#include <string>
#include <cctype>
#include <vector>
#include <ostream>
#include <variant>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include "piaabo/dutils.h"

namespace cuwacunu {
namespace piaabo {
namespace bnf {

/* language descriptions */
/**
  * @brief Represents a Unit in DSL.
 */
struct ProductionUnit {
  /**
   * @brief Enumerates the types of BNF units.
   */
  enum class Type {
    Terminal,             // Represents a terminal string enclosed in quotes or literals e.g. [  "A"    "ABC"    'ABC'    A     ABC    1  123   "123"   ]
    NonTerminal,          // Represents a non-terminal symbol enclosed in angle brackets e.g. <example>
    Optional,             // Indicates a optional NonTerminal e.g. <example> ::= [<item>];
    Repetition,           // Indicates a zero or more elements NonTerminal repetition e.g. <example> ::= {<item>} ;
    Punctuation,          // Represents a punctuation in BNF e.g. [  ';'     '|'    '::='  ]
    EndOfFile,            // Indicates the end of the input stream 
    Undetermined          // Represents an empty or invalid unit
  } type = Type::Undetermined; // Default initialization

  std::string lexeme = "";  // The textual representation of the unit.
  int line = 1;             // The line number where the unit appears.
  int column = 1;           // The column number where the unit starts.

  /**
   * @brief Constructs a ProductionUnit with specified type, lexeme, line, and column.
   * @param type    The type of the unit.
   * @param lexeme  The textual representation of the unit.
   * @param line    The line number where the unit appears.
   * @param column  The column number where the unit starts.
   */
  ProductionUnit(Type type = Type::Undetermined, const std::string& lexeme = "", int line = 1, int column = 1)
    : type(type), lexeme(lexeme), line(line), column(column) {}
  
  /* convert to std::string */
  std::string str(bool versbose = false) const;
};

/**
 * @brief Represents a production rule alternative in DSL.
 */
struct ProductionAlternative {
  std::string lhs = "";

  /**
   * @brief Enumerates the types of BNF units.
   */
  enum class Type {
    Single,   // Unique units  <example> ::= [<option>] ; 
    Sequence, // Right-hand Sequence of (5) units <example> ::= <hour> ":" <minutes> "." <seconds> ;
    Unknown
  } type = Type::Unknown;

  /**
   * @brief Enumerates flags for production alternatives.
   */
  enum class Flags {
    None       = 0,       // b0000
    Recursion  = 1 << 0,  // b0001  // Contains Recursion units to self e.g. <example> ::= "A" | "B" <example> ; ----> BBBBBA
    Optional   = 1 << 1,  // b0010  // Contains Optional units e.g. <example> ::= [<item>] ;
    Repetition = 1 << 2,  // b0100  // Contains Repetition units e.g. <example> ::= [<item>] ;
  } flags = Flags::None;

  std::variant<ProductionUnit, std::vector<ProductionUnit>> content;

  ProductionAlternative(std::string dlhs, const ProductionUnit& dunit, Flags dflags = Flags::None)
    : lhs(dlhs), type(Type::Single), flags(dflags), content(dunit) {}

  ProductionAlternative(std::string dlhs, const std::vector<ProductionUnit>& dunits, Flags dflags = Flags::None)
    : lhs(dlhs), type(Type::Sequence), flags(dflags), content(dunits) {}
  
  /* convert to std::string */
  std::string str(bool versbose = false) const;
};

/* ProductionAlternative Flags Bitwise */
ProductionAlternative::Flags  operator  |   (ProductionAlternative::Flags lhs, ProductionAlternative::Flags rhs);
ProductionAlternative::Flags& operator  |=  (ProductionAlternative::Flags& lhs, ProductionAlternative::Flags rhs);
ProductionAlternative::Flags  operator  &   (ProductionAlternative::Flags lhs, ProductionAlternative::Flags rhs);
ProductionAlternative::Flags& operator  &=  (ProductionAlternative::Flags& lhs, ProductionAlternative::Flags rhs);
ProductionAlternative::Flags  operator  ~   (ProductionAlternative::Flags lhs);

/**
 * @brief Represents a production rule in DSL.
 *    a production is a line in the grammar file
 */
struct ProductionRule {
  std::string lhs;                          /* Left-hand side of the production. */
  std::vector<ProductionAlternative> rhs;   /* Right-hand side alternatives. */

  std::string str(bool versbose = false) const;
};


/**
 * @brief Represents a production rule in DSL.
 *    a production is a line in the grammar file
 */
struct ProductionGrammar {
  std::vector<ProductionRule> rules;

  ProductionRule& getRule(const ProductionUnit unit);
  ProductionRule& getRule(const std::string& lhs);
  ProductionRule& getRule(size_t lhs_index);
  
  std::string str(int indentLevel = 0) const;
};

} /* namespace bnf */
} /* namespace piaabo */
} /* namespace cuwacunu */
