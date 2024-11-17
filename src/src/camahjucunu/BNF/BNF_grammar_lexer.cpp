/* BNF_grammar_lexer.cpp */
#include "camahjucunu/BNF/BNF_grammar_lexer.h"

RUNTIME_WARNING("(BNF_grammar_lexer.cpp)[] guard printing the errors with secure methods \n");

namespace cuwacunu {
namespace camahjucunu {
namespace BNF {

/**
 * @brief Overloads the stream insertion operator for ProductionUnit::Type.
 * @param stream The output stream.
 * @param type The Type to output.
 * @return The output stream.
 */
std::ostream& operator<<(std::ostream& stream, const ProductionUnit::Type& type) {
  switch (type) {
    case ProductionUnit::Type::Punctuation: stream << "Punctuation"; break;
    case ProductionUnit::Type::Terminal: stream << "Terminal"; break;
    case ProductionUnit::Type::NonTerminal: stream << "NonTerminal"; break;
    case ProductionUnit::Type::Optional: stream << "Optional"; break;
    case ProductionUnit::Type::Repetition: stream << "Repetition"; break;
    case ProductionUnit::Type::EndOfFile: stream << "EndOfFile"; break;
    case ProductionUnit::Type::Undetermined: stream << "Undetermined"; break;
    default: stream << "Invalid ProductionUnit::Type"; break; /* Handle unexpected cases */
  }
  return stream;
}
std::ostream& operator<<(std::ostream& stream, const ProductionUnit& unit) {
  stream << unit.lexeme << "("  << unit.type << ")";
  return stream;
}
std::ostream& operator<<(std::ostream& stream, const std::vector<ProductionUnit>& vec) {
  stream << "{";
  for (size_t i = 0; i < vec.size(); ++i) {
    stream << "\"" << vec[i] << "\"";
    if (i != vec.size() - 1) {
      stream << ", ";  /* Add a comma between elements, but not after the last one */
    }
  }
  stream << "}";
  return stream;
}

/* - - - - - - - - - - - - */
/*      GrammarLexer       */
/* - - - - - - - - - - - - */

/**
 * @brief Resets the Lexer to the starting position
 * @return void.
 */
void GrammarLexer::reset() {
  setPosition(0);
  line = 1;
  column = 1;
}

/**
 * @brief Checks if the lexer has reached the end of the input.
 * @return True if at end, false otherwise.
 */
bool GrammarLexer::isAtEnd() const {
  return pos >= input.length();
}

/**
 * @brief Peeks at the current character without consuming it.
 * @return The current character or '\0' if at end.
 */
char GrammarLexer::peek() {
  return isAtEnd() ? '\0' : input[pos];
}

/**
 * @brief Advances to the next character and returns it.
 * @return The consumed character or '\0' if at end.
 */
char GrammarLexer::advance() {
  if (isAtEnd()) return '\0';
  char currentChar = input[pos++];
  updatePosition(currentChar); /* Update line and column */
  return currentChar;
}

/**
 * @brief Skips whitespace characters.
 */
void GrammarLexer::skipWhitespace() {
  while (!isAtEnd() && std::isspace(peek())) {
    advance();
  }
}

/**
 * @brief Retrieves the next unit from the input.
 * @return The next ProductionUnit.
 */
ProductionUnit GrammarLexer::getNextUnit() {
  skipWhitespace();

  if (isAtEnd()) {
    return ProductionUnit(ProductionUnit::Type::EndOfFile, "", line, column);
  }

  char nextChar = peek();

  if (nextChar == '<') {
    return parseNonTerminal();
  } else if (nextChar == '[') {
    return parseOptional();
  } else if (nextChar == '{') {
    return parseRepetition();
  } else if (nextChar == '"' || std::isalpha(nextChar) || std::isdigit(nextChar)) {
    return parseTerminal();
  } else if (std::ispunct(nextChar)) {
    return parsePunctuation();
  } else {
    /* Unknown character */
    std::string unknownChar(1, advance());
    std::string errorMsg = "Unknown character '" + unknownChar + "'";
    throw std::runtime_error("Grammar Syntax Error: Error at line " + 
          std::to_string(line) + ", column " + std::to_string(column) + ": " + errorMsg);
  }
}

/**
 * @brief Parses a non-terminal unit.
 * @return The parsed ProductionUnit representing a non-terminal.
 */
ProductionUnit GrammarLexer::parseNonTerminal() {
  std::string lexeme;
  if(peek() != '<') {
    throw std::runtime_error("Grammar Syntax Error: Non-terminals should be enclosed in <>, found unexpected non-terminal at line " +
          std::to_string(line) + ", column " + std::to_string(column));
  }
  lexeme += advance(); /* Consume '<' */

  while (!isAtEnd() && peek() != '>') {
    char ch = advance();
    lexeme += ch;
  }

  if (isAtEnd()) {
    throw std::runtime_error("Grammar Syntax Error: Unterminated non-terminal at line " +
          std::to_string(line) + ", column " + std::to_string(column));
  }
  
  lexeme += advance(); /* Consume '>' */

  return ProductionUnit(ProductionUnit::Type::NonTerminal, lexeme, line, column);
}

/**
 * @brief Parses a optional unit.
 * @return The parsed ProductionUnit representing a optional.
 */
ProductionUnit GrammarLexer::parseOptional() {
  std::string lexeme;
  if(peek() != '[') {
    throw std::runtime_error("Grammar Syntax Error: Optionals should be enclosed in [], found unexpected optional at line " +
          std::to_string(line) + ", column " + std::to_string(column));
  }
  
  advance(); /* Advance '[' */

  if(peek() != '<') {
    throw std::runtime_error("Grammar Syntax Error: Optionals should enclose Non-Terminals [<example>], found unexpected optional enclosing terminal at line " +
          std::to_string(line) + ", column " + std::to_string(column));
  }

  while (!isAtEnd() && peek() != ']') {
    char ch = advance();
    lexeme += ch;
  }

  if (isAtEnd()) {
    throw std::runtime_error("Grammar Syntax Error: Unterminated optional at line " +
          std::to_string(line) + ", column " + std::to_string(column));
  }

  if(input[pos] == '>') {
    throw std::runtime_error("Grammar Syntax Error: Optionals should enclose Non-Terminals [<example>], found Non-Terminal without closing bracket, unexpected syntax at line " +
        std::to_string(line) + ", column " + std::to_string(column));
  }

  advance(); /* Advance ']' */

  return ProductionUnit(ProductionUnit::Type::Optional,  "[" + lexeme + "]", line, column);
}


/**
 * @brief Parses a Repetition unit.
 * @return The parsed ProductionUnit representing a Repetition.
 */
ProductionUnit GrammarLexer::parseRepetition() {
  std::string lexeme;
  if(peek() != '{') {
    throw std::runtime_error("Grammar Syntax Error: Repetitions should be enclosed in {}, found unexpected Repetition at line " +
          std::to_string(line) + ", column " + std::to_string(column));
  }
  
  advance(); /* Advance '{' */

  if(peek() != '<') {
    throw std::runtime_error("Grammar Syntax Error: Repetitions should enclose Non-Terminals {<example>}, found unexpected Repetition enclosing terminal at line " +
          std::to_string(line) + ", column " + std::to_string(column));
  }

  while (!isAtEnd() && peek() != '}') {
    char ch = advance();
    lexeme += ch;
  }

  if (isAtEnd()) {
    throw std::runtime_error("Grammar Syntax Error: Unterminated Repetition at line " +
          std::to_string(line) + ", column " + std::to_string(column));
  }

  if(input[pos] == '>') {
    throw std::runtime_error("Grammar Syntax Error: Repetitions should enclose Non-Terminals {<example>}, found Non-Terminal without closing bracket, unexpected syntax at line " +
        std::to_string(line) + ", column " + std::to_string(column));
  }

  advance(); /* Advance '}' */

  return ProductionUnit(ProductionUnit::Type::Repetition,  "{" + lexeme + "}", line, column);
}

/**
 * @brief Parses a terminal unit.
 * @return The parsed ProductionUnit representing a terminal.
 */
ProductionUnit GrammarLexer::parseTerminal() {
  std::string lexeme;
  
  /* literal terminal without quotes */
  if (peek() != '\"' && peek() != '\'') {
    /* advance the alphanumeric block */
    while (!isAtEnd() && (std::isalnum(peek()) || std::isdigit(peek()) || peek() == '_' || peek() == '.' || peek() == ' ')) {
      lexeme += advance();
    }
    

    return ProductionUnit(ProductionUnit::Type::Terminal, lexeme, line, column);
  }

  advance(); /* Consume the opening quote */

  /* literal enclosed by quotes */
  do {
    /* validate */
    if (isAtEnd()) {
      throw std::runtime_error("Grammar Syntax Error: Unterminated terminal (ends with a backslash) at line " +
            std::to_string(line) + ", column " + std::to_string(column));
    }
    
    if (peek() == '\"' || peek() == '\'') {
      std::string total_lexeme = peek() == '\"' ? "\"" + lexeme + "\"" : "\'" + lexeme + "\'";
      advance(); /* Consume the closing quote */
      return ProductionUnit(ProductionUnit::Type::Terminal, total_lexeme, line, column);
    }
    lexeme += advance();
  } while (!isAtEnd());

  /* Unterminated terminal  */
  throw std::runtime_error("Grammar Syntax Error: Unterminated terminal \"" + lexeme + "\" at line " +
        std::to_string(line) + ", column " + std::to_string(column));
}

/**
 * @brief Parses a punctuation unit.
 * @return The parsed ProductionUnit representing a punctuation.
 */
ProductionUnit GrammarLexer::parsePunctuation() {
  /* parse production operator */
  if (peek() == ':') {
    advance();
    if (peek() == ':') {
      advance(); /* Consume second ':' */
      if (peek() == '=') {
        advance(); /* Consume '=' */
        return ProductionUnit(ProductionUnit::Type::Punctuation, "::=", line, column);
      }
      throw std::runtime_error("Grammar Syntax Error: Invalid character after '::' (expected '::=') at line " +
            std::to_string(line) + ", column " + std::to_string(column));
    }
    throw std::runtime_error("Grammar Syntax Error: Invalid character after ':' (expected '::=') at line " +
          std::to_string(line) + ", column " + std::to_string(column));
  }
  /* parse other cases */
  char ch = advance();
  std::string lexeme(1, ch);
  /* Handle '.' which could be part of '...' (unsupported) */
  if (ch == '.') {
    if (peek() == '.' && pos + 1 < input.length() && input[pos + 1] == '.') {
      advance(); /* Consume second '.' */
      advance(); /* Consume third '.' */
      throw std::runtime_error("Grammar Syntax Error: Expression \"...\" is not supported in this implementation of BNF, found at line " +
            std::to_string(line) + ", column " + std::to_string(column));
    }
  }
  /* Handle other single-character symbols */
  const std::string validPunctuations = ";|"; /* ::= is also a valid punctuation */
  if (validPunctuations.find(ch) != std::string::npos) {
    return ProductionUnit(ProductionUnit::Type::Punctuation, lexeme, line, column);
  }
  /* If the symbol is not recognized, throw an exception */
  throw std::runtime_error("Grammar Syntax Error: Unsupported character: '" + std::string(1, ch) + "' at line " +
        std::to_string(line) + ", column " + std::to_string(column));
}

/**
 * @brief Retrieves the current position in the input.
 * @return The current position index.
 */
size_t GrammarLexer::getPosition() const {
  return pos;
}

/**
 * @brief Sets the current position in the input.
 * @param position The new position index.
 */
void GrammarLexer::setPosition(size_t position) {
  pos = position;
  /* Optionally, recalculate line and column based on position */
  /* This can be complex and may require additional logic */
}

/**
 * @brief Updates line and column numbers based on the consumed character.
 * @param ch The character that was consumed.
 */
void GrammarLexer::updatePosition(char ch) {
  if (ch == '\n') {
    line++;
    column = 1;
  } else {
    column++;
  }
}

} /* namespace BNF */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
