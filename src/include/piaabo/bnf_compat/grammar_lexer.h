/* bnf_grammar_lexer.h */
#pragma once
#include "piaabo/bnf_compat/parser_types.h"

namespace cuwacunu {
namespace piaabo {
namespace bnf {

/**
 * @brief Overloads the stream insertion operator for ProductionUnit::Type.
 * @param os The output stream.
 * @param type The Type to output.
 * @return The output stream.
 */
std::ostream& operator<<(std::ostream& os, const ProductionUnit::Type& type);
std::ostream& operator<<(std::ostream& os, const ProductionUnit& Unit);
std::ostream& operator<<(std::ostream& os, const std::vector<ProductionUnit>& vec);

/**
 * @brief Lexer class for Unitizing BNF input.
 */
class GrammarLexer {
public:
  /**
   * @brief Constructs a GrammarLexer with the given input string.
   * @param input The BNF input string to Unitize.
   */
  GrammarLexer(const std::string& input)
    : input(input), pos(0), line(1), column(1) {
      reset();
    }

  /**
   * @brief Retrieves the next Unit from the input.
   * @return The next ProductionUnit.
   */
  void reset();

  /**
   * @brief Retrieves the next Unit from the input.
   * @return The next ProductionUnit.
   */
  ProductionUnit getNextUnit();

  /**
   * @brief Checks if the lexer has reached the end of the input.
   * @return True if at end, false otherwise.
   */
  bool isAtEnd() const;
  
private:
  std::string input; ///< The BNF input string.
  size_t pos;        ///< Current position in the input string.
  int line;          ///< Current line number.
  int column;        ///< Current column number.

  /**
   * @brief Retrieves the current position in the input.
   * @return The current position index.
   */
  size_t getPosition() const;

  /**
   * @brief Sets the current position in the input.
   * @param position The new position index.
   */
  void setPosition(size_t position);

  /**
   * @brief Peeks at the current character without consuming it.
   * @return The current character or '\0' if at end.
   */
  char peek();

  /**
   * @brief Advances to the next character and returns it.
   * @return The consumed character or '\0' if at end.
   */
  char advance();

  /**
   * @brief Skips whitespace characters.
   */
  void skipWhitespace();

  /**
   * @brief Parses a non-terminal Unit.
   * @return The parsed ProductionUnit representing a non-terminal.
   */
  ProductionUnit parseNonTerminal();

  /**
   * @brief Parses a terminal Unit.
   * @return The parsed ProductionUnit representing a terminal.
   */
  ProductionUnit parseTerminal();

  /**
   * @brief Parses a optional Unit.
   * @return The parsed ProductionUnit representing a optional.
   */
  ProductionUnit parseOptional();

  /**
   * @brief Parses a repetition Unit.
   * @return The parsed ProductionUnit representing a repetition.
   */
  ProductionUnit parseRepetition();

  /**
   * @brief Parses a symbol Unit.
   * @return The parsed ProductionUnit representing a symbol.
   */
  ProductionUnit parsePunctuation();

  /**
   * @brief Updates line and column numbers based on the consumed character.
   * @param ch The character that was consumed.
   */
  void updatePosition(char ch);
};

} /* namespace bnf */
} /* namespace piaabo */
} /* namespace cuwacunu */
