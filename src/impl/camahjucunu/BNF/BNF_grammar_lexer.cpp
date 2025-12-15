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
  while (!isAtEnd()) {
    // 1) Skip whitespace (spaces, tabs, newlines, etc.)
    while (!isAtEnd() && std::isspace(static_cast<unsigned char>(peek()))) {
      advance();
    }

    // 2) If we are at the start of a line and see ';', skip the whole line as a comment.
    //
    //    - We use `column == 1` to detect that this ';' is the first character on the line.
    //    - This ensures we don't accidentally treat the terminating ';' in a production
    //      as a comment, because those will be after other tokens (column > 1).
    if (!isAtEnd() && peek() == ';' && column == 1) {
      // Consume characters until end-of-line or EOF
      while (!isAtEnd() && peek() != '\n') {
        advance();
      }
      // If there's a newline, consume it too so that the next token starts
      // correctly on the following line.
      if (!isAtEnd() && peek() == '\n') {
        advance();
      }

      // After skipping a comment line, loop again to skip any additional
      // whitespace or comment lines.
      continue;
    }

    // If we get here, we are not looking at a comment line; break out.
    break;
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
  
  const int start_line=line, start_col=column;

  advance(); // '['

  if (peek()!='<') throw std::runtime_error("... Optionals should enclose Non-Terminals [<example>] ...");

  // read the nonterminal <...>
  do { lexeme += advance(); if (isAtEnd()) throw std::runtime_error("... Unterminated optional ..."); } while (peek()!='>');
  
  lexeme += advance(); // '>'
  
  if (peek()!=']') throw std::runtime_error("... Missing closing ']' for optional ...");
  
  advance(); // ']'
  
  return ProductionUnit(ProductionUnit::Type::Optional, "[" + lexeme + "]", start_line, start_col);
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
  
  const int start_line=line, start_col=column;

  advance(); // '{'

  if (peek()!='<') throw std::runtime_error(std::string("... Repetitions should enclose Non-Terminals {<example>} ... ") + std::string("(line:") + std::to_string(line) + std::string(", column") + std::to_string(column) + ").");

  do { lexeme += advance(); if (isAtEnd()) throw std::runtime_error("... Unterminated repetition ..."); } while (peek()!='>');

  lexeme += advance(); // '>'

  if (peek()!='}') throw std::runtime_error("... Missing closing '}' for repetition ...");

  advance(); // '}'
  
  return ProductionUnit(ProductionUnit::Type::Repetition, "{" + lexeme + "}", start_line, start_col);
}

/**
 * @brief Parses a terminal unit.
 * @return The parsed ProductionUnit representing a terminal.
 */
ProductionUnit GrammarLexer::parseTerminal() {
  std::string lexeme;

  // 1) Literal terminal without quotes: [A-Za-z0-9_.]+
  if (peek() != '\"' && peek() != '\'') {
    while (!isAtEnd() &&
           (std::isalnum(static_cast<unsigned char>(peek())) ||
            peek() == '_' || peek() == '.')) {
      lexeme += advance();
    }
    return ProductionUnit(ProductionUnit::Type::Terminal, lexeme, line, column);
  }

  // 2) Quoted literal: "..." or '...'
  const char quote = advance(); // opening quote character (' or ")
  const int start_line = line;
  const int start_col  = column;

  while (!isAtEnd()) {
    char ch = peek();

    // Closing quote must match the opening quote
    if (ch == quote) {
      advance(); // consume closing quote

      std::string total_lexeme;
      total_lexeme += quote;
      total_lexeme += lexeme;
      total_lexeme += quote;

      return ProductionUnit(ProductionUnit::Type::Terminal,
                            total_lexeme,
                            start_line,
                            start_col);
    }

    // Escape sequence: keep backslash + next char verbatim.
    // They will be interpreted later by InstructionParser::unescape().
    if (ch == '\\') {
      lexeme += advance();             // '\'
      if (!isAtEnd()) {
        lexeme += advance();           // escaped char, e.g. '"', '\', 'n', ...
      }
      continue;
    }

    // Normal character
    lexeme += advance();
  }

  // If we get here, the terminal never closed.
  throw std::runtime_error(
    "Grammar Syntax Error: Unterminated terminal starting at line " +
    std::to_string(start_line) + ", column " + std::to_string(start_col)
  );
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
