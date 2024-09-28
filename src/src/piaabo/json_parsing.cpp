#include "piaabo/json_parsing.h"

namespace cuwacunu {
namespace piaabo {

void printIndent(int indent) {
  for (int i = 0; i < indent; ++i) {
    std::cout << "  ";
  }
}

void printJsonValue(const JsonValue& value, int indent) {
  switch (value.type) {
    case JsonValueType::OBJECT: {
      std::cout << "{\n";
      for (auto it = value.objectValue->begin(); it != value.objectValue->end(); ++it) {
        printIndent(indent + 1);
        std::cout << '"' << it->first << "\": ";
        printJsonValue(it->second, indent + 1);
        if (std::next(it) != value.objectValue->end()) {
          std::cout << ",";
        }
        std::cout << "\n";
      }
      printIndent(indent);
      std::cout << "}";
      break;
    }
    case JsonValueType::ARRAY: {
      std::cout << "[\n";
      for (size_t i = 0; i < value.arrayValue->size(); ++i) {
        printIndent(indent + 1);
        printJsonValue((*value.arrayValue)[i], indent + 1);
        if (i != value.arrayValue->size() - 1) {
          std::cout << ",";
        }
        std::cout << "\n";
      }
      printIndent(indent);
      std::cout << "]";
      break;
    }
    case JsonValueType::STRING:
      std::cout << '"' << value.stringValue << '"';
      break;
    case JsonValueType::NUMBER:
      std::cout << value.numberValue;
      break;
    case JsonValueType::BOOLEAN:
      std::cout << (value.boolValue ? "true" : "false");
      break;
    case JsonValueType::NULL_TYPE:
      std::cout << "null";
      break;
  }
}

std::string extract_json_string_value(const std::string& json_str, const std::string& key, const std::string nullcase) {
  std::string key_pattern = "\"" + key + "\":";
  size_t key_pos = json_str.find(key_pattern);
  if (key_pos == std::string::npos) {
    /* Key not found */
    return nullcase;
  }
  size_t value_pos = key_pos + key_pattern.length();
  /* Skip any whitespace */
  while (value_pos < json_str.length() && std::isspace(json_str[value_pos])) {
    value_pos++;
  }
  if (value_pos >= json_str.length() || json_str[value_pos] != '\"') {
    /* Value is not a string */
    return nullcase;
  }
  value_pos++; /* Skip the opening quote */
  std::string value;
  bool escape = false;
  while (value_pos < json_str.length()) {
    char c = json_str[value_pos];
    if (escape) {
      /* Handle escape sequences */
      value += c;
      escape = false;
    } else if (c == '\\') {
      escape = true;
    } else if (c == '\"') {
      /* End of string */
      break;
    } else {
      value += c;
    }
    value_pos++;
  }
  return value;
}

bool json_fast_validity_check(const std::string& json_str) {
  std::stack<char> bracket_stack;
  bool escape = false;
  bool in_string = false;

  for (char ch : json_str) {
    /* skip excaped characters */
    if (escape)     { escape = false; continue; }
    if (ch == '\\') { escape = true;  continue; }
    /* skip chunks inside strings */
    if (ch == '\"') { in_string = !in_string; continue; }
    if (in_string)  { continue; }
    /* skip numeric characters */
    if (std::isdigit(static_cast<unsigned char>(ch))) { continue; }

    switch (ch) {
      case '{':
      case '[':
        bracket_stack.push(ch);
        break;
      case '}':
        if (bracket_stack.empty() || bracket_stack.top() != '{') { return false; }
        bracket_stack.pop();
        break;
      case ']':
        if (bracket_stack.empty() || bracket_stack.top() != '[') { return false; }
        bracket_stack.pop();
        break;
      default:
        break;
    }
  }

  return !in_string && bracket_stack.empty();
}

} /* namespace piaabo */
} /* namespace cuwacunu */