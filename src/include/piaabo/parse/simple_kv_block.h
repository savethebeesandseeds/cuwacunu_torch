// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace cuwacunu::piaabo::parse::simple_kv {

struct block_t {
  std::string name{};
  std::unordered_map<std::string, std::string> values{};
};

[[nodiscard]] inline std::string trim(std::string value) {
  const auto not_space = [](unsigned char c) { return !std::isspace(c); };
  value.erase(value.begin(),
              std::find_if(value.begin(), value.end(), not_space));
  value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(),
              value.end());
  return value;
}

[[nodiscard]] inline std::string uppercase(std::string value) {
  std::transform(
      value.begin(), value.end(), value.begin(),
      [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
  return value;
}

[[nodiscard]] inline std::string lowercase(std::string value) {
  std::transform(
      value.begin(), value.end(), value.begin(),
      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}

[[nodiscard]] inline std::string strip_comments(const std::string &text) {
  std::string out;
  out.reserve(text.size());
  for (std::size_t i = 0; i < text.size();) {
    if (i + 1 < text.size() && text[i] == '/' && text[i + 1] == '*') {
      i += 2;
      while (i + 1 < text.size() && !(text[i] == '*' && text[i + 1] == '/')) {
        ++i;
      }
      i = (i + 1 < text.size()) ? i + 2 : text.size();
      continue;
    }
    if (i + 1 < text.size() && text[i] == '/' && text[i + 1] == '/') {
      while (i < text.size() && text[i] != '\n') {
        ++i;
      }
      continue;
    }
    if (text[i] == '#') {
      while (i < text.size() && text[i] != '\n') {
        ++i;
      }
      continue;
    }
    out.push_back(text[i++]);
  }
  return out;
}

[[nodiscard]] inline std::vector<block_t>
parse_blocks(const std::string &text) {
  const auto clean = strip_comments(text);
  std::vector<block_t> blocks;
  std::size_t pos = 0;
  while (pos < clean.size()) {
    while (pos < clean.size() &&
           (std::isspace(static_cast<unsigned char>(clean[pos])) ||
            clean[pos] == ';')) {
      ++pos;
    }
    if (pos >= clean.size()) {
      break;
    }
    const std::size_t name_begin = pos;
    while (pos < clean.size() &&
           (std::isalnum(static_cast<unsigned char>(clean[pos])) ||
            clean[pos] == '_' || clean[pos] == '.')) {
      ++pos;
    }
    auto name = uppercase(trim(clean.substr(name_begin, pos - name_begin)));
    if (name.empty()) {
      throw std::runtime_error("[simple_kv] expected block name");
    }
    while (pos < clean.size() &&
           std::isspace(static_cast<unsigned char>(clean[pos]))) {
      ++pos;
    }
    if (pos >= clean.size() || clean[pos] != '{') {
      throw std::runtime_error("[simple_kv] expected '{' after block " + name);
    }
    ++pos;
    std::size_t body_begin = pos;
    int depth = 1;
    while (pos < clean.size() && depth > 0) {
      if (clean[pos] == '{') {
        ++depth;
      } else if (clean[pos] == '}') {
        --depth;
        if (depth == 0) {
          break;
        }
      }
      ++pos;
    }
    if (depth != 0) {
      throw std::runtime_error("[simple_kv] unclosed block " + name);
    }
    const auto body = clean.substr(body_begin, pos - body_begin);
    ++pos;

    block_t block{};
    block.name = std::move(name);
    std::size_t cur = 0;
    while (cur < body.size()) {
      while (cur < body.size() &&
             (std::isspace(static_cast<unsigned char>(body[cur])) ||
              body[cur] == ';')) {
        ++cur;
      }
      if (cur >= body.size()) {
        break;
      }
      const std::size_t key_begin = cur;
      while (cur < body.size() &&
             (std::isalnum(static_cast<unsigned char>(body[cur])) ||
              body[cur] == '_' || body[cur] == '.')) {
        ++cur;
      }
      auto key = uppercase(trim(body.substr(key_begin, cur - key_begin)));
      while (cur < body.size() &&
             std::isspace(static_cast<unsigned char>(body[cur]))) {
        ++cur;
      }
      if (cur >= body.size() || body[cur] != '=') {
        throw std::runtime_error("[simple_kv] expected '=' after key " + key);
      }
      ++cur;
      const std::size_t value_begin = cur;
      while (cur < body.size() && body[cur] != ';') {
        ++cur;
      }
      auto value = trim(body.substr(value_begin, cur - value_begin));
      if (key.empty()) {
        throw std::runtime_error("[simple_kv] empty key in block " +
                                 block.name);
      }
      if (!block.values.emplace(key, value).second) {
        throw std::runtime_error("[simple_kv] duplicate key " + key +
                                 " in block " + block.name);
      }
      if (cur < body.size() && body[cur] == ';') {
        ++cur;
      }
    }
    blocks.push_back(std::move(block));
  }
  return blocks;
}

[[nodiscard]] inline const block_t &single_block(const std::string &text,
                                                 const std::string &name) {
  static thread_local std::vector<block_t> blocks;
  blocks = parse_blocks(text);
  const auto target = uppercase(name);
  const block_t *found = nullptr;
  for (const auto &block : blocks) {
    if (block.name == target) {
      if (found != nullptr) {
        throw std::runtime_error("[simple_kv] duplicate block " + target);
      }
      found = &block;
    }
  }
  if (found == nullptr) {
    throw std::runtime_error("[simple_kv] missing block " + target);
  }
  return *found;
}

[[nodiscard]] inline std::string required(const block_t &block,
                                          const std::string &key) {
  const auto target = uppercase(key);
  const auto it = block.values.find(target);
  if (it == block.values.end() || trim(it->second).empty()) {
    throw std::runtime_error("[simple_kv] missing required key " + target +
                             " in block " + block.name);
  }
  return trim(it->second);
}

[[nodiscard]] inline std::string optional(const block_t &block,
                                          const std::string &key,
                                          std::string fallback = {}) {
  const auto target = uppercase(key);
  const auto it = block.values.find(target);
  if (it == block.values.end()) {
    return fallback;
  }
  auto value = trim(it->second);
  return value.empty() ? fallback : value;
}

[[nodiscard]] inline bool parse_bool(std::string value) {
  value = lowercase(trim(value));
  if (value == "true" || value == "yes" || value == "1") {
    return true;
  }
  if (value == "false" || value == "no" || value == "0") {
    return false;
  }
  throw std::runtime_error("[simple_kv] invalid bool value: " + value);
}

[[nodiscard]] inline int64_t parse_i64(const std::string &value) {
  std::size_t used = 0;
  const auto trimmed = trim(value);
  const auto out = std::stoll(trimmed, &used, 10);
  if (used != trimmed.size()) {
    throw std::runtime_error("[simple_kv] invalid int64 value: " + trimmed);
  }
  return out;
}

[[nodiscard]] inline double parse_double(const std::string &value) {
  std::size_t used = 0;
  const auto trimmed = trim(value);
  const auto out = std::stod(trimmed, &used);
  if (used != trimmed.size()) {
    throw std::runtime_error("[simple_kv] invalid double value: " + trimmed);
  }
  return out;
}

[[nodiscard]] inline std::vector<std::string>
parse_list(const std::string &value) {
  std::vector<std::string> out;
  std::size_t cur = 0;
  while (cur <= value.size()) {
    const auto next = value.find(',', cur);
    auto item = trim(value.substr(
        cur, next == std::string::npos ? std::string::npos : next - cur));
    if (!item.empty()) {
      out.push_back(std::move(item));
    }
    if (next == std::string::npos) {
      break;
    }
    cur = next + 1;
  }
  return out;
}

[[nodiscard]] inline std::vector<int64_t>
parse_i64_list(const std::string &value) {
  std::vector<int64_t> out;
  for (const auto &item : parse_list(value)) {
    out.push_back(parse_i64(item));
  }
  return out;
}

} // namespace cuwacunu::piaabo::parse::simple_kv
