#pragma once

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace cuwacunu::hero::mcp_schema_compat {

struct mcp_schema_compat_issue_t {
  std::string tool_name{};
  std::string schema_path{};
  std::string construct{};
  std::string message{};
};

namespace detail {

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

inline void skip_ws(std::string_view s, std::size_t *idx) {
  while (*idx < s.size() &&
         std::isspace(static_cast<unsigned char>(s[*idx])) != 0) {
    ++(*idx);
  }
}

[[nodiscard]] inline bool
parse_json_string(std::string_view s, std::size_t *idx, std::string *out) {
  if (*idx >= s.size() || s[*idx] != '"') {
    return false;
  }
  ++(*idx);
  std::string value;
  while (*idx < s.size()) {
    const char c = s[*idx];
    ++(*idx);
    if (c == '"') {
      if (out) {
        *out = std::move(value);
      }
      return true;
    }
    if (c == '\\') {
      if (*idx >= s.size()) {
        return false;
      }
      value.push_back(s[*idx]);
      ++(*idx);
      continue;
    }
    value.push_back(c);
  }
  return false;
}

[[nodiscard]] inline bool skip_json_value(std::string_view s,
                                          std::size_t *idx) {
  skip_ws(s, idx);
  if (*idx >= s.size()) {
    return false;
  }
  if (s[*idx] == '"') {
    return parse_json_string(s, idx, nullptr);
  }
  if (s[*idx] == '{' || s[*idx] == '[') {
    const char open = s[*idx];
    const char close = open == '{' ? '}' : ']';
    int depth = 1;
    ++(*idx);
    while (*idx < s.size()) {
      if (s[*idx] == '"') {
        if (!parse_json_string(s, idx, nullptr)) {
          return false;
        }
        continue;
      }
      if (s[*idx] == open) {
        ++depth;
      } else if (s[*idx] == close) {
        --depth;
        if (depth == 0) {
          ++(*idx);
          return true;
        }
      }
      ++(*idx);
    }
    return false;
  }
  while (*idx < s.size() && s[*idx] != ',' && s[*idx] != '}' &&
         s[*idx] != ']') {
    ++(*idx);
  }
  return true;
}

struct root_member_t {
  std::string key{};
  std::string value{};
};

[[nodiscard]] inline bool
parse_root_members(std::string_view schema, std::vector<root_member_t> *members,
                   std::string *error) {
  std::size_t idx = 0;
  skip_ws(schema, &idx);
  if (idx >= schema.size() || schema[idx] != '{') {
    if (error) {
      *error = "inputSchema root is not a JSON object";
    }
    return false;
  }
  ++idx;
  std::vector<root_member_t> parsed;
  while (idx < schema.size()) {
    skip_ws(schema, &idx);
    if (idx < schema.size() && schema[idx] == '}') {
      ++idx;
      skip_ws(schema, &idx);
      if (idx != schema.size()) {
        if (error) {
          *error = "trailing data after inputSchema root object";
        }
        return false;
      }
      if (members) {
        *members = std::move(parsed);
      }
      return true;
    }
    std::string key;
    if (!parse_json_string(schema, &idx, &key)) {
      if (error) {
        *error = "expected root object string key";
      }
      return false;
    }
    skip_ws(schema, &idx);
    if (idx >= schema.size() || schema[idx] != ':') {
      if (error) {
        *error = "expected ':' after root object key";
      }
      return false;
    }
    ++idx;
    const std::size_t value_begin = idx;
    if (!skip_json_value(schema, &idx)) {
      if (error) {
        *error = "invalid JSON value for root object key " + key;
      }
      return false;
    }
    parsed.push_back(root_member_t{
        .key = std::move(key),
        .value = std::string(schema.substr(value_begin, idx - value_begin))});
    skip_ws(schema, &idx);
    if (idx < schema.size() && schema[idx] == ',') {
      ++idx;
      continue;
    }
    if (idx < schema.size() && schema[idx] == '}') {
      continue;
    }
    if (error) {
      *error = "expected ',' or '}' in root object";
    }
    return false;
  }
  if (error) {
    *error = "unterminated inputSchema root object";
  }
  return false;
}

[[nodiscard]] inline bool json_string_value_equals(std::string_view raw,
                                                   std::string_view expected) {
  std::size_t idx = 0;
  skip_ws(raw, &idx);
  std::string parsed;
  if (!parse_json_string(raw, &idx, &parsed)) {
    return false;
  }
  skip_ws(raw, &idx);
  return idx == raw.size() && parsed == expected;
}

[[nodiscard]] inline bool forbidden_root_construct(std::string_view key) {
  static constexpr std::string_view kForbidden[] = {"oneOf", "anyOf", "allOf",
                                                    "enum", "not"};
  return std::find(std::begin(kForbidden), std::end(kForbidden), key) !=
         std::end(kForbidden);
}

} // namespace detail

[[nodiscard]] inline std::vector<mcp_schema_compat_issue_t>
validate_tool_input_schema(std::string_view tool_name,
                           std::string_view input_schema_json) {
  std::vector<mcp_schema_compat_issue_t> issues;
  std::vector<detail::root_member_t> members;
  std::string parse_error;
  if (!detail::parse_root_members(input_schema_json, &members, &parse_error)) {
    issues.push_back(
        mcp_schema_compat_issue_t{.tool_name = std::string(tool_name),
                                  .schema_path = "inputSchema",
                                  .construct = "malformed_json",
                                  .message = parse_error});
    return issues;
  }

  bool type_seen = false;
  bool type_is_object = false;
  for (const auto &member : members) {
    if (detail::forbidden_root_construct(member.key)) {
      issues.push_back(mcp_schema_compat_issue_t{
          .tool_name = std::string(tool_name),
          .schema_path = "inputSchema." + member.key,
          .construct = member.key,
          .message =
              "top-level JSON Schema construct is not MCP-harness compatible"});
    }
    if (member.key == "type") {
      type_seen = true;
      type_is_object = detail::json_string_value_equals(member.value, "object");
    }
  }
  if (!type_seen) {
    issues.push_back(mcp_schema_compat_issue_t{
        .tool_name = std::string(tool_name),
        .schema_path = "inputSchema.type",
        .construct = "missing_type",
        .message = "tool parameter schema must declare top-level type object"});
  } else if (!type_is_object) {
    issues.push_back(mcp_schema_compat_issue_t{
        .tool_name = std::string(tool_name),
        .schema_path = "inputSchema.type",
        .construct = "type",
        .message = "tool parameter schema top-level type must be object"});
  }
  return issues;
}

[[nodiscard]] inline std::string
format_issue(const mcp_schema_compat_issue_t &issue) {
  std::ostringstream out;
  out << "tool=" << issue.tool_name << " schema_path=" << issue.schema_path
      << " construct=" << issue.construct << " message=" << issue.message;
  return out.str();
}

inline void
assert_tool_input_schema_compatible(std::string_view tool_name,
                                    std::string_view input_schema_json) {
  const auto issues = validate_tool_input_schema(tool_name, input_schema_json);
  if (!issues.empty()) {
    throw std::runtime_error("MCP schema compatibility violation: " +
                             format_issue(issues.front()));
  }
}

} // namespace cuwacunu::hero::mcp_schema_compat
