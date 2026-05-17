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
#include <cerrno>
#include <cstdlib>
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

  JsonValue(ObjectType&& obj) : type(JsonValueType::OBJECT), objectValue(std::make_shared<ObjectType>(std::move(obj))) {}

  JsonValue(const ArrayType& arr) : type(JsonValueType::ARRAY), arrayValue(std::make_shared<ArrayType>(arr)) {}

  JsonValue(ArrayType&& arr) : type(JsonValueType::ARRAY), arrayValue(std::make_shared<ArrayType>(std::move(arr))) {}

  JsonValue(const std::string& str) : type(JsonValueType::STRING), stringValue(str) {}

  JsonValue(std::string&& str) : type(JsonValueType::STRING), stringValue(std::move(str)) {}

  JsonValue(double num) : type(JsonValueType::NUMBER), numberValue(num) {}

  JsonValue(bool b) : type(JsonValueType::BOOLEAN), boolValue(b) {}

  JsonValue(JsonValueType nullType) : type(nullType), numberValue(0), boolValue(false) {}
};

class JsonParser {
public:
  explicit JsonParser(const std::string& input)
    : json(input), idx(0), length(json.length()) {}

  explicit JsonParser(std::string&& input)
    : json(std::move(input)), idx(0), length(json.length()) {}

  JsonValue parse() {
    skipWhitespace();
    JsonValue value = parseValue();
    skipWhitespace();
    if (idx != length) {
      std::stringstream ss;
      ss << "Extra characters after parsing JSON at position " << idx;
      log_secure_fatal("%s", ss.str().c_str());
    }
    return value;
  }

private:
  std::string json;
  size_t idx;
  size_t length;

  static bool isHexDigit(char ch) {
    return (ch >= '0' && ch <= '9') ||
           (ch >= 'a' && ch <= 'f') ||
           (ch >= 'A' && ch <= 'F');
  }

  static uint8_t hexValue(char ch) {
    if (ch >= '0' && ch <= '9') return static_cast<uint8_t>(ch - '0');
    if (ch >= 'a' && ch <= 'f') return static_cast<uint8_t>(10 + (ch - 'a'));
    return static_cast<uint8_t>(10 + (ch - 'A'));
  }

  static void appendUtf8CodePoint(uint32_t codePoint, std::string& out) {
    if (codePoint <= 0x7F) {
      out += static_cast<char>(codePoint);
    } else if (codePoint <= 0x7FF) {
      out += static_cast<char>(0xC0 | ((codePoint >> 6) & 0x1F));
      out += static_cast<char>(0x80 | (codePoint & 0x3F));
    } else if (codePoint <= 0xFFFF) {
      out += static_cast<char>(0xE0 | ((codePoint >> 12) & 0x0F));
      out += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
      out += static_cast<char>(0x80 | (codePoint & 0x3F));
    } else if (codePoint <= 0x10FFFF) {
      out += static_cast<char>(0xF0 | ((codePoint >> 18) & 0x07));
      out += static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F));
      out += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
      out += static_cast<char>(0x80 | (codePoint & 0x3F));
    } else {
      log_secure_fatal("Invalid Unicode code point");
    }
  }

  bool startsWithLiteral(const char* literal) const {
    const size_t literal_len = std::strlen(literal);
    if (idx + literal_len > length) return false;
    return std::memcmp(json.data() + idx, literal, literal_len) == 0;
  }

  void skipWhitespace() {
    while (idx < length && std::isspace(static_cast<unsigned char>(json[idx]))) {
      idx++;
    }
  }

  char peek() const {
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
      log_secure_fatal("%s", ss.str().c_str());
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
    } else if (ch == '-' || std::isdigit(static_cast<unsigned char>(ch))) {
      return JsonValue(parseNumber());
    } else if (startsWithLiteral("true") ||
               startsWithLiteral("false") ||
               startsWithLiteral("null")) {
      return parseLiteral();
    } else {
      std::stringstream ss;
      ss << "Invalid value at position " << idx;
      log_secure_fatal("%s", ss.str().c_str());
    }
    return JsonValue(JsonValueType::NULL_TYPE);
  }

  JsonValue parseObject() {
    expect('{');
    skipWhitespace();
    ObjectType object;
    if (peek() == '}') {
      advance();  // Skip '}'
      return JsonValue(std::move(object));
    }
    while (true) {
      skipWhitespace();
      if (peek() != '"') {
        std::stringstream ss;
        ss << "Expected '\"' at position " << idx;
        log_secure_fatal("%s", ss.str().c_str());
      }
      std::string key = parseString();
      skipWhitespace();
      expect(':');
      skipWhitespace();
      JsonValue value = parseValue();
      object.insert_or_assign(std::move(key), std::move(value));
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
        log_secure_fatal("%s", ss.str().c_str());
      }
    }
    return JsonValue(std::move(object));
  }

  JsonValue parseArray() {
    expect('[');
    skipWhitespace();
    ArrayType array;
    if (peek() == ']') {
      advance();  // Skip ']'
      return JsonValue(std::move(array));
    }
    while (true) {
      skipWhitespace();
      JsonValue value = parseValue();
      array.push_back(std::move(value));
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
        log_secure_fatal("%s", ss.str().c_str());
      }
    }
    return JsonValue(std::move(array));
  }

  std::string parseString() {
    expect('"');
    std::string result;
    result.reserve(16);
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
            log_secure_fatal("%s", ss.str().c_str());
        }
      } else if (ch == '"') {
        return result;
      } else {
        if (static_cast<unsigned char>(ch) < 0x20) {
          std::stringstream ss;
          ss << "Invalid control character in string at position " << idx - 1;
          log_secure_fatal("%s", ss.str().c_str());
        }
        result += ch;
      }
    }
    log_secure_fatal("Unterminated JSON string");
    return {};
  }

  std::string parseUnicodeEscape() {
    auto parseHex4CodeUnit = [this]() -> uint16_t {
      if (idx + 4 > length) {
        log_secure_fatal("Invalid Unicode escape sequence");
      }
      uint16_t code_unit = 0;
      for (int i = 0; i < 4; ++i) {
        char ch = json[idx++];
        if (!isHexDigit(ch)) {
          log_secure_fatal("Invalid Unicode escape sequence");
        }
        code_unit = static_cast<uint16_t>((code_unit << 4) | hexValue(ch));
      }
      return code_unit;
    };

    const uint16_t first = parseHex4CodeUnit();
    uint32_t codePoint = first;

    if (first >= 0xD800 && first <= 0xDBFF) {
      if (idx + 2 > length || json[idx] != '\\' || json[idx + 1] != 'u') {
        log_secure_fatal("Missing low surrogate pair in Unicode escape");
      }
      idx += 2;
      const uint16_t second = parseHex4CodeUnit();
      if (second < 0xDC00 || second > 0xDFFF) {
        log_secure_fatal("Invalid low surrogate in Unicode escape");
      }
      codePoint = 0x10000u + (((static_cast<uint32_t>(first) - 0xD800u) << 10)
                              | (static_cast<uint32_t>(second) - 0xDC00u));
    } else if (first >= 0xDC00 && first <= 0xDFFF) {
      log_secure_fatal("Unexpected low surrogate in Unicode escape");
    }

    std::string utf8Char;
    appendUtf8CodePoint(codePoint, utf8Char);
    return utf8Char;
  }

  double parseNumber() {
    size_t startIdx = idx;
    if (peek() == '-') {
      advance();
    }
    if (peek() == '0') {
      advance();
      if (std::isdigit(static_cast<unsigned char>(peek()))) {
        std::stringstream ss;
        ss << "Invalid number with leading zeros at position " << idx;
        log_secure_fatal("%s", ss.str().c_str());
      }
    } else if (std::isdigit(static_cast<unsigned char>(peek()))) {
      while (std::isdigit(static_cast<unsigned char>(peek()))) {
        advance();
      }
    } else {
      std::stringstream ss;
      ss << "Invalid number at position " << idx;
      log_secure_fatal("%s", ss.str().c_str());
    }
    if (peek() == '.') {
      advance();
      if (!std::isdigit(static_cast<unsigned char>(peek()))) {
        std::stringstream ss;
        ss << "Invalid fractional part in number at position " << idx;
        log_secure_fatal("%s", ss.str().c_str());
      }
      while (std::isdigit(static_cast<unsigned char>(peek()))) {
        advance();
      }
    }
    if (peek() == 'e' || peek() == 'E') {
      advance();
      if (peek() == '+' || peek() == '-') {
        advance();
      }
      if (!std::isdigit(static_cast<unsigned char>(peek()))) {
        std::stringstream ss;
        ss << "Invalid exponent in number at position " << idx;
        log_secure_fatal("%s", ss.str().c_str());
      }
      while (std::isdigit(static_cast<unsigned char>(peek()))) {
        advance();
      }
    }

    const char* begin_ptr = json.c_str() + startIdx;
    char* end_ptr = nullptr;
    errno = 0;
    const double parsed = std::strtod(begin_ptr, &end_ptr);
    if (end_ptr != json.c_str() + idx || errno == ERANGE) {
      std::string numStr = json.substr(startIdx, idx - startIdx);
      std::stringstream ss;
      ss << "Invalid number format: " << numStr;
      log_secure_fatal("%s", ss.str().c_str());
    }
    return parsed;
  }

  JsonValue parseLiteral() {
    if (startsWithLiteral("true")) {
      idx += 4;
      return JsonValue(true);
    } else if (startsWithLiteral("false")) {
      idx += 5;
      return JsonValue(false);
    } else if (startsWithLiteral("null")) {
      idx += 4;
      return JsonValue(JsonValueType::NULL_TYPE);
    } else {
      std::stringstream ss;
      ss << "Invalid literal at position " << idx;
      log_secure_fatal("%s", ss.str().c_str());
    }
    return JsonValue(JsonValueType::NULL_TYPE);
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
 * **Note:** This function scans only the top-level object fields. It handles whitespace and escaped
 * string content, but intentionally does not search recursively inside nested objects/arrays.
 *
 * @param json_str The JSON-formatted string to search within.
 * @param key The key whose associated string value needs to be extracted.
 * @param nullcase The default string to return if the key is not found or the value is not a string.
 *
 * @return A `std::string` containing the value associated with the specified key.
 *         Returns `nullcase` if the key is not found or if the value is not a string.
 *
 */
std::string extract_json_string_value(const std::string& json_str, const std::string& key, const std::string& nullcase = "NULL");

/**
 * @brief Performs a lightweight structural validation of a JSON string.
 *
 * This function validates whether the provided buffer contains exactly one complete
 * top-level JSON object or array (with optional surrounding whitespace), while
 * handling escaped characters inside strings.
 * 
 * **Note:** This is intentionally lightweight and should be treated as a framing gate,
 * not a full JSON validator.
 *
 * @param json_str The JSON string to validate.
 * @return `true` if the JSON string is structurally valid based on bracket and quote balancing.
 * @return `false` if there are unbalanced brackets/braces, mismatched quotes, or improper nesting.
 */
bool json_fast_validity_check(const std::string& json_str);

} /* namespace piaabo */
} /* namespace cuwacunu */
