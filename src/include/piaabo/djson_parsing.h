/*
 * JSON Parser in C++
 *
 * This code implements a JSON parser using only the C++ standard library.
 * It supports parsing of all standard JSON data types, including:
 * - Objects (`{ }`)
 * - Arrays (`[ ]`)
 * - Strings
 * - Numbers (integers and floating-point numbers)
 * - Booleans (`true` and `false`)
 * - Null (`null`)
 *
 * The parser handles nested objects and arrays, escape sequences in strings,
 * Unicode characters, and numbers with fractional and exponential parts.
 *
 * Key Features:
 *
 * 1. **JsonValue Structure**:
 *  - Represents any JSON value.
 *  - Uses `std::shared_ptr` for automatic memory management of objects and arrays.
 *  - Contains fields for each possible JSON value type.
 *
 * 2. **JsonParser Class**:
 *  - Parses a JSON string into a `JsonValue` object.
 *  - Implements recursive descent parsing techniques.
 *  - Provides detailed error messages with position information.
 *
 * 3. **Test Suite**:
 *  - A comprehensive set of tests to verify the correctness of the parser.
 *  - Tests cover various scenarios, including nested structures, empty objects and arrays,
 *    escape sequences, Unicode characters, numbers in different formats, booleans, nulls,
 *    and error handling for invalid JSON inputs.
 *
 * 4. **Usage**:
 *  - The parser can be used to parse JSON strings into `JsonValue` objects.
 *  - Parsed data can be accessed through the fields of `JsonValue` and its contained objects and arrays.
 *
 * Implementation Details:
 *
 * - **Parsing Functions**:
 *   - `parseValue()`: Determines the type of the next value and calls the appropriate parsing function.
 *   - `parseObject()`: Parses JSON objects.
 *   - `parseArray()`: Parses JSON arrays.
 *   - `parseString()`: Parses JSON strings, handling escape sequences and Unicode characters.
 *   - `parseNumber()`: Parses JSON numbers, supporting integers and floating-point numbers with optional exponents.
 *   - `parseLiteral()`: Parses the literals `true`, `false`, and `null`.
 *
 * - **Error Handling**:
 log_secure_fatal` exceptions with descriptive messages when encountering invalid syntax or unexpected characters.
 *   - Error messages include the position in the input string where the error occurred.
 *
 * - **Memory Management**:
 *   - Uses `std::shared_ptr` to manage dynamically allocated memory for objects and arrays.
 *   - Eliminates the need for manual memory management and prevents memory leaks and double frees.
 *
 * - **Unicode Support**:
 *   - Supports parsing of Unicode escape sequences (e.g., `\uXXXX`) in strings.
 *   - Handles code points in the Basic Multilingual Plane.
 *
 * - **Testing**:
 *   - The test suite uses `assert` statements to verify the correctness of the parser.
 *   - Tests are designed to cover a wide range of valid and invalid JSON inputs.
 *
 * How to Use:
 *
 * - **Parsing a JSON String**:
 *   ```cpp
 *   std::string jsonString = R"({"name": "Alice", "age": 30})";
 *   JsonParser parser(jsonString);
 *   JsonValue root = parser.parse();
 *   ```
 *
 * - **Accessing Parsed Data**:
 *   ```cpp
 *   if (root.type == JsonValueType::OBJECT) {
 *     auto& obj = *root.objectValue;
 *     std::string name = obj["name"].stringValue;
 *     double age = obj["age"].numberValue;
 *   }
 *   ```
 *
 * - **Running Tests**:
 *   - Compile and run the test suite to verify the parser's functionality.
 *   - The test suite is included in the `runTests()` function.
 *
 * Compilation Instructions:
 *
 * - Use a C++11 or later compiler.
 * - Example:
 *   ```bash
 *   g++ -std=c++11 json_parser.cpp -o json_parser
 *   ```
 *
 * - Run the compiled program:
 *   ```bash
 *   ./json_parser
 *   ```
 *
 * This implementation provides a solid foundation for parsing JSON data in C++ applications
 * without relying on external libraries. It demonstrates the use of modern C++ techniques,
 * such as smart pointers and recursive descent parsing, to build a robust and efficient JSON parser.
 */

/* djson_parsing.h */
#pragma once

#include <iostream>
#include <string>
#include <cctype>
#include <stdexcept>
#include <sstream>
#include <memory>
#include <cassert>
#include <cstring>
#include <map>
#include <vector>
#include <stack>

#include "piaabo/dutils.h"

namespace cuwacunu {
namespace piaabo {

enum class JsonValueType {
  OBJECT,
  ARRAY,
  STRING,
  NUMBER,
  BOOLEAN,
  NULL_TYPE
};

struct JsonValue;  // Forward declaration

using ObjectType = std::map<std::string, JsonValue>;
using ArrayType = std::vector<JsonValue>;

struct JsonValue {
  JsonValueType type;
  std::shared_ptr<ObjectType> objectValue;
  std::shared_ptr<ArrayType> arrayValue;
  std::string stringValue;
  double numberValue;
  bool boolValue;

  // Constructors
  JsonValue() : type(JsonValueType::NULL_TYPE), numberValue(0), boolValue(false) {}

  JsonValue(const ObjectType& obj) : type(JsonValueType::OBJECT), objectValue(std::make_shared<ObjectType>(obj)) {}

  JsonValue(const ArrayType& arr) : type(JsonValueType::ARRAY), arrayValue(std::make_shared<ArrayType>(arr)) {}

  JsonValue(const std::string& str) : type(JsonValueType::STRING), stringValue(str) {}

  JsonValue(double num) : type(JsonValueType::NUMBER), numberValue(num) {}

  JsonValue(bool b) : type(JsonValueType::BOOLEAN), boolValue(b) {}

  JsonValue(JsonValueType nullType) : type(nullType), numberValue(0), boolValue(false) {}
};

class JsonParser {
public:
  JsonParser(const std::string& input)
    : json(input), idx(0), length(input.length()) {}

  JsonValue parse() {
    skipWhitespace();
    JsonValue value = parseValue();
    skipWhitespace();
    if (idx != length) {
      log_secure_fatal("Extra characters after parsing JSON");
    }
    return value;
  }

private:
  std::string json;
  size_t idx;
  size_t length;

  void skipWhitespace() {
    while (idx < length && std::isspace(json[idx])) {
      idx++;
    }
  }

  char peek() {
    if (idx < length) {
      return json[idx];
    }
    return '\0';
  }

  char advance() {
    if (idx < length) {
      return json[idx++];
    }
    return '\0';
  }

  void expect(char ch) {
    if (advance() != ch) {
      std::stringstream ss;
      ss << "Expected '" << ch << "' at position " << idx - 1;
      log_secure_fatal(ss.str().c_str());
    }
  }

  JsonValue parseValue() {
    skipWhitespace();
    char ch = peek();
    if (ch == '{') {
      return parseObject();
    } else if (ch == '[') {
      return parseArray();
    } else if (ch == '"') {
      return JsonValue(parseString());
    } else if (ch == '-' || std::isdigit(ch)) {
      return JsonValue(parseNumber());
    } else if (std::strncmp(&json[idx], "true", 4) == 0 ||
           std::strncmp(&json[idx], "false", 5) == 0 ||
           std::strncmp(&json[idx], "null", 4) == 0) {
      return parseLiteral();
    } else {
      std::stringstream ss;
      ss << "Invalid value at position " << idx;
      log_secure_fatal(ss.str().c_str());
    }
  }

  JsonValue parseObject() {
    expect('{');
    skipWhitespace();
    ObjectType object;
    if (peek() == '}') {
      advance();  // Skip '}'
      return JsonValue(object);
    }
    while (true) {
      skipWhitespace();
      if (peek() != '"') {
        std::stringstream ss;
        ss << "Expected '\"' at position " << idx;
        log_secure_fatal(ss.str().c_str());
      }
      std::string key = parseString();
      skipWhitespace();
      expect(':');
      skipWhitespace();
      JsonValue value = parseValue();
      object[key] = value;
      skipWhitespace();
      char ch = peek();
      if (ch == ',') {
        advance();  // Skip ','
        continue;
      } else if (ch == '}') {
        advance();  // Skip '}'
        break;
      } else {
        std::stringstream ss;
        ss << "Expected ',' or '}' at position " << idx;
        log_secure_fatal(ss.str().c_str());
      }
    }
    return JsonValue(object);
  }

  JsonValue parseArray() {
    expect('[');
    skipWhitespace();
    ArrayType array;
    if (peek() == ']') {
      advance();  // Skip ']'
      return JsonValue(array);
    }
    while (true) {
      skipWhitespace();
      JsonValue value = parseValue();
      array.push_back(value);
      skipWhitespace();
      char ch = peek();
      if (ch == ',') {
        advance();  // Skip ','
        continue;
      } else if (ch == ']') {
        advance();  // Skip ']'
        break;
      } else {
        std::stringstream ss;
        ss << "Expected ',' or ']' at position " << idx;
        log_secure_fatal(ss.str().c_str());
      }
    }
    return JsonValue(array);
  }

  std::string parseString() {
    expect('"');
    std::string result;
    while (idx < length) {
      char ch = advance();
      if (ch == '\\') {
        if (idx >= length) {
          log_secure_fatal("Invalid escape sequence");
        }
        char nextChar = advance();
        switch (nextChar) {
          case '"': result += '"'; break;
          case '\\': result += '\\'; break;
          case '/': result += '/'; break;
          case 'b': result += '\b'; break;
          case 'f': result += '\f'; break;
          case 'n': result += '\n'; break;
          case 'r': result += '\r'; break;
          case 't': result += '\t'; break;
          case 'u':
            result += parseUnicodeEscape();
            break;
          default:
            std::stringstream ss;
            ss << "Invalid escape character '\\" << nextChar << "' at position " << idx - 1;
            log_secure_fatal(ss.str().c_str());
        }
      } else if (ch == '"') {
        break;
      } else {
        result += ch;
      }
    }
    return result;
  }

  std::string parseUnicodeEscape() {
    if (idx + 4 > length) {
      log_secure_fatal("Invalid Unicode escape sequence");
    }
    std::string hexStr = json.substr(idx, 4);
    idx += 4;
    char16_t codePoint = static_cast<char16_t>(std::stoi(hexStr, nullptr, 16));
    // For simplicity, we'll handle code points in the Basic Multilingual Plane
    // Extend this if surrogate pairs need to be handled
    std::string utf8Char;
    if (codePoint <= 0x7F) {
      utf8Char += static_cast<char>(codePoint);
    } else if (codePoint <= 0x7FF) {
      utf8Char += static_cast<char>(0xC0 | ((codePoint >> 6) & 0x1F));
      utf8Char += static_cast<char>(0x80 | (codePoint & 0x3F));
    } else {
      utf8Char += static_cast<char>(0xE0 | ((codePoint >> 12) & 0x0F));
      utf8Char += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
      utf8Char += static_cast<char>(0x80 | (codePoint & 0x3F));
    }
    return utf8Char;
  }

  double parseNumber() {
    size_t startIdx = idx;
    if (peek() == '-') {
      advance();
    }
    if (peek() == '0') {
      advance();
    } else if (std::isdigit(peek())) {
      while (std::isdigit(peek())) {
        advance();
      }
    } else {
      std::stringstream ss;
      ss << "Invalid number at position " << idx;
      log_secure_fatal(ss.str().c_str());
    }
    if (peek() == '.') {
      advance();
      if (!std::isdigit(peek())) {
        std::stringstream ss;
        ss << "Invalid fractional part in number at position " << idx;
        log_secure_fatal(ss.str().c_str());
      }
      while (std::isdigit(peek())) {
        advance();
      }
    }
    if (peek() == 'e' || peek() == 'E') {
      advance();
      if (peek() == '+' || peek() == '-') {
        advance();
      }
      if (!std::isdigit(peek())) {
        std::stringstream ss;
        ss << "Invalid exponent in number at position " << idx;
        log_secure_fatal(ss.str().c_str());
      }
      while (std::isdigit(peek())) {
        advance();
      }
    }
    std::string numStr = json.substr(startIdx, idx - startIdx);
    try {
      return std::stod(numStr);
    } catch (...) {
      std::stringstream ss;
      ss << "Invalid number format: " << numStr;
      log_secure_fatal(ss.str().c_str());
    }
  }

  JsonValue parseLiteral() {
    if (std::strncmp(&json[idx], "true", 4) == 0) {
      idx += 4;
      return JsonValue(true);
    } else if (std::strncmp(&json[idx], "false", 5) == 0) {
      idx += 5;
      return JsonValue(false);
    } else if (std::strncmp(&json[idx], "null", 4) == 0) {
      idx += 4;
      return JsonValue(JsonValueType::NULL_TYPE);
    } else {
      std::stringstream ss;
      ss << "Invalid literal at position " << idx;
      log_secure_fatal(ss.str().c_str());
    }
  }
};

/**
 * @brief Catches to the current identation to display a Json struct
 *
 * @param indent The current indentation level for formatting the output.
 */
void printIndent(int indent);

/**
 * @brief Recursively prints a JsonValue in a formatted JSON structure.
 *
 * This function outputs the given JsonValue to the standard output with proper indentation,
 * handling different JSON types such as objects, arrays, strings, numbers, booleans, and nulls.
 *
 * @param value  The JsonValue to be printed.
 * @param indent The current indentation level for formatting the output.
 */
void printJsonValue(const JsonValue& value, int indent = 0);

/**
 * @brief Extracts the string value associated with a specified key from a JSON string.
 *
 * This function searches for a given key within a JSON-formatted string and retrieves its corresponding
 * string value. If the key is not found or the value is not a string, the function returns a default
 * value provided by the caller.
 *
 * **Note:** This function performs a simple and lightweight extraction by searching for the key pattern
 * and parsing the subsequent string value. It does not perform comprehensive JSON parsing and may not
 * handle all edge cases, such as nested objects, arrays, or complex escape sequences within strings.
 * Use this function for straightforward JSON structures where performance is critical and full parsing
 * is unnecessary.
 *
 * @param json_str The JSON-formatted string to search within.
 * @param key The key whose associated string value needs to be extracted.
 * @param nullcase The default string to return if the key is not found or the value is not a string.
 *
 * @return A `std::string` containing the value associated with the specified key.
 *         Returns `nullcase` if the key is not found or if the value is not a string.
 *
 */
std::string extract_json_string_value(const std::string& json_str, const std::string& key, const std::string nullcase = "NULL");

/**
 * @brief Performs a lightweight structural validation of a JSON string.
 *
 * This function checks whether the provided JSON string has balanced and properly
 * nested curly braces `{}`, square brackets `[]`, and correctly paired quotation marks `"` 
 * while handling escaped characters. It ignores numeric characters to enhance performance.
 * 
 * **Note:** This validation is not comprehensive and does not verify JSON syntax
 * details such as key-value pair formatting, comma placement, or data types.
 * It is intended for quick preliminary checks where full JSON parsing is unnecessary.
 *
 * @param json_str The JSON string to validate.
 * @return `true` if the JSON string is structurally valid based on bracket and quote balancing.
 * @return `false` if there are unbalanced brackets/braces, mismatched quotes, or improper nesting.
 */
bool json_fast_validity_check(const std::string& json_str);

} /* namespace piaabo */
} /* namespace cuwacunu */