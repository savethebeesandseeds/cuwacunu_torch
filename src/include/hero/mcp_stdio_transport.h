#pragma once

#include <cctype>
#include <cstddef>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace cuwacunu::hero::mcp_stdio {

struct message_t {
  std::string json;
};

[[nodiscard]] inline std::string trim_ascii(std::string_view in) {
  std::size_t begin = 0;
  while (begin < in.size() &&
         std::isspace(static_cast<unsigned char>(in[begin])) != 0) {
    ++begin;
  }
  std::size_t end = in.size();
  while (end > begin &&
         std::isspace(static_cast<unsigned char>(in[end - 1])) != 0) {
    --end;
  }
  return std::string(in.substr(begin, end - begin));
}

[[nodiscard]] inline bool iequals_ascii(std::string_view lhs,
                                        std::string_view rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    const auto l = static_cast<unsigned char>(lhs[i]);
    const auto r = static_cast<unsigned char>(rhs[i]);
    if (std::tolower(l) != std::tolower(r)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] inline bool parse_content_length_header(std::string_view line,
                                                      std::size_t *out) {
  const std::size_t colon = line.find(':');
  if (colon == std::string_view::npos) {
    return false;
  }
  const std::string key = trim_ascii(line.substr(0, colon));
  if (!iequals_ascii(key, "Content-Length")) {
    return false;
  }
  const std::string value = trim_ascii(line.substr(colon + 1));
  if (value.empty()) {
    throw std::runtime_error("Content-Length header has empty value");
  }
  std::size_t consumed = 0;
  const auto length = static_cast<std::size_t>(std::stoull(value, &consumed));
  if (consumed != value.size()) {
    throw std::runtime_error("Content-Length header has non-numeric value");
  }
  if (out != nullptr) {
    *out = length;
  }
  return true;
}

[[nodiscard]] inline bool read_message(std::istream &in, message_t *out) {
  std::string line;
  while (std::getline(in, line)) {
    const std::string trimmed = trim_ascii(line);
    if (trimmed.empty()) {
      continue;
    }

    std::size_t content_length = 0;
    if (!parse_content_length_header(trimmed, &content_length)) {
      return false;
    }

    while (std::getline(in, line)) {
      const std::string header = trim_ascii(line);
      if (header.empty()) {
        break;
      }
      std::size_t replacement_length = 0;
      if (parse_content_length_header(header, &replacement_length)) {
        content_length = replacement_length;
      }
    }

    std::string body(content_length, '\0');
    in.read(body.data(), static_cast<std::streamsize>(content_length));
    if (static_cast<std::size_t>(in.gcount()) != content_length) {
      return false;
    }
    out->json = std::move(body);
    return true;
  }
  return false;
}

inline void write_response(std::ostream &out, std::string_view json) {
  out << "Content-Length: " << json.size() << "\r\n\r\n" << json;
  out.flush();
}

} // namespace cuwacunu::hero::mcp_stdio
