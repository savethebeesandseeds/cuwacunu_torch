/* djson_parsing.cpp */
#include "piaabo/djson_parsing.h"

RUNTIME_WARNING("(djson_parsing.cpp)[] Trowing errors instead of fatal logs would allow for error catching (but then be aware to prevent terminal inyection).\n");
RUNTIME_WARNING("(djson_parsing.cpp)[] Error cases are well defined, but better error messages are required.\n");

namespace cuwacunu {
namespace piaabo {

namespace {

inline void skipWhitespaceView(const std::string& s, size_t& idx) {
  while (idx < s.size() && std::isspace(static_cast<unsigned char>(s[idx]))) {
    ++idx;
  }
}

inline bool isHexDigit(char ch) {
  return (ch >= '0' && ch <= '9') ||
         (ch >= 'a' && ch <= 'f') ||
         (ch >= 'A' && ch <= 'F');
}

inline uint8_t hexValue(char ch) {
  if (ch >= '0' && ch <= '9') return static_cast<uint8_t>(ch - '0');
  if (ch >= 'a' && ch <= 'f') return static_cast<uint8_t>(10 + (ch - 'a'));
  return static_cast<uint8_t>(10 + (ch - 'A'));
}

inline void appendUtf8CodePoint(uint32_t codePoint, std::string& out) {
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
    return;
  }
}

inline bool parseHex4(const std::string& s, size_t& idx, uint16_t& codeUnit) {
  if (idx + 4 > s.size()) return false;
  uint16_t value = 0;
  for (int i = 0; i < 4; ++i) {
    const char ch = s[idx++];
    if (!isHexDigit(ch)) return false;
    value = static_cast<uint16_t>((value << 4) | hexValue(ch));
  }
  codeUnit = value;
  return true;
}

inline bool parseJsonStringToken(const std::string& s, size_t& idx, std::string& out) {
  if (idx >= s.size() || s[idx] != '"') return false;
  ++idx;
  out.clear();

  while (idx < s.size()) {
    const char ch = s[idx++];
    if (ch == '"') return true;

    if (ch == '\\') {
      if (idx >= s.size()) return false;
      const char next = s[idx++];
      switch (next) {
        case '"': out += '"'; break;
        case '\\': out += '\\'; break;
        case '/': out += '/'; break;
        case 'b': out += '\b'; break;
        case 'f': out += '\f'; break;
        case 'n': out += '\n'; break;
        case 'r': out += '\r'; break;
        case 't': out += '\t'; break;
        case 'u': {
          uint16_t first = 0;
          if (!parseHex4(s, idx, first)) return false;
          uint32_t codePoint = first;
          if (first >= 0xD800 && first <= 0xDBFF) {
            if (idx + 2 > s.size() || s[idx] != '\\' || s[idx + 1] != 'u') return false;
            idx += 2;
            uint16_t second = 0;
            if (!parseHex4(s, idx, second)) return false;
            if (second < 0xDC00 || second > 0xDFFF) return false;
            codePoint = 0x10000u + (((static_cast<uint32_t>(first) - 0xD800u) << 10)
                                    | (static_cast<uint32_t>(second) - 0xDC00u));
          } else if (first >= 0xDC00 && first <= 0xDFFF) {
            return false;
          }
          appendUtf8CodePoint(codePoint, out);
          break;
        }
        default:
          return false;
      }
      continue;
    }

    if (static_cast<unsigned char>(ch) < 0x20) return false;
    out += ch;
  }

  return false;
}

inline bool skipJsonValue(const std::string& s, size_t& idx) {
  if (idx >= s.size()) return false;

  if (s[idx] == '"') {
    std::string tmp;
    return parseJsonStringToken(s, idx, tmp);
  }

  if (s[idx] == '{' || s[idx] == '[') {
    std::vector<char> stack;
    stack.reserve(16);
    stack.push_back(s[idx]);
    ++idx;

    bool in_string = false;
    bool escape = false;
    while (idx < s.size()) {
      const char ch = s[idx++];
      if (in_string) {
        if (escape) {
          escape = false;
          continue;
        }
        if (ch == '\\') {
          escape = true;
          continue;
        }
        if (ch == '"') {
          in_string = false;
          continue;
        }
        if (static_cast<unsigned char>(ch) < 0x20) return false;
        continue;
      }

      if (ch == '"') {
        in_string = true;
        continue;
      }
      if (ch == '{' || ch == '[') {
        stack.push_back(ch);
        continue;
      }
      if (ch == '}') {
        if (stack.empty() || stack.back() != '{') return false;
        stack.pop_back();
      } else if (ch == ']') {
        if (stack.empty() || stack.back() != '[') return false;
        stack.pop_back();
      }

      if (stack.empty()) return true;
    }
    return false;
  }

  size_t start = idx;
  while (idx < s.size()) {
    const char ch = s[idx];
    if (ch == ',' || ch == '}' || ch == ']' ||
        std::isspace(static_cast<unsigned char>(ch))) {
      break;
    }
    ++idx;
  }
  return idx > start;
}

} // namespace

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

std::string extract_json_string_value(const std::string& json_str, const std::string& key, const std::string& nullcase) {
  size_t idx = 0;
  skipWhitespaceView(json_str, idx);
  if (idx >= json_str.size() || json_str[idx] != '{') return nullcase;
  ++idx;

  while (idx < json_str.size()) {
    skipWhitespaceView(json_str, idx);
    if (idx < json_str.size() && json_str[idx] == '}') return nullcase;

    std::string current_key;
    if (!parseJsonStringToken(json_str, idx, current_key)) return nullcase;

    skipWhitespaceView(json_str, idx);
    if (idx >= json_str.size() || json_str[idx] != ':') return nullcase;
    ++idx;
    skipWhitespaceView(json_str, idx);

    if (current_key == key) {
      std::string value;
      if (!parseJsonStringToken(json_str, idx, value)) return nullcase;
      return value;
    }

    if (!skipJsonValue(json_str, idx)) return nullcase;

    skipWhitespaceView(json_str, idx);
    if (idx < json_str.size() && json_str[idx] == ',') {
      ++idx;
      continue;
    }
    if (idx < json_str.size() && json_str[idx] == '}') return nullcase;
    return nullcase;
  }

  return nullcase;
}

bool json_fast_validity_check(const std::string& json_str) {
  size_t idx = 0;
  skipWhitespaceView(json_str, idx);
  if (idx >= json_str.size()) return false;
  if (json_str[idx] != '{' && json_str[idx] != '[') return false;

  std::vector<char> bracket_stack;
  bracket_stack.reserve(16);
  bracket_stack.push_back(json_str[idx]);
  ++idx;

  bool escape = false;
  bool in_string = false;

  for (; idx < json_str.size(); ++idx) {
    const char ch = json_str[idx];
    if (in_string) {
      if (escape) {
        escape = false;
        continue;
      }
      if (ch == '\\') {
        escape = true;
        continue;
      }
      if (ch == '"') {
        in_string = false;
        continue;
      }
      if (static_cast<unsigned char>(ch) < 0x20) return false;
      continue;
    }

    if (ch == '"') {
      in_string = true;
      continue;
    }

    switch (ch) {
      case '{':
      case '[':
        bracket_stack.push_back(ch);
        break;
      case '}':
        if (bracket_stack.empty() || bracket_stack.back() != '{') return false;
        bracket_stack.pop_back();
        break;
      case ']':
        if (bracket_stack.empty() || bracket_stack.back() != '[') return false;
        bracket_stack.pop_back();
        break;
      default:
        break;
    }

    if (bracket_stack.empty()) {
      ++idx;
      for (; idx < json_str.size(); ++idx) {
        if (!std::isspace(static_cast<unsigned char>(json_str[idx]))) return false;
      }
      return true;
    }
  }

  return false;
}

} /* namespace piaabo */
} /* namespace cuwacunu */
