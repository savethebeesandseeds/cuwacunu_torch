#pragma once

#include <cctype>
#include <string>
#include <string_view>
#include <vector>

namespace cuwacunu {
namespace camahjucunu {

struct wave_contract_binding_variable_t {
  std::string name{};
  std::string value{};
};

struct wave_contract_binding_decl_t {
  std::string id{};
  std::string contract_ref{};
  std::string wave_ref{};
  std::vector<wave_contract_binding_variable_t> variables{};
};

[[nodiscard]] inline std::string wave_contract_binding_trim_ascii(
    std::string_view in) {
  std::size_t b = 0;
  while (b < in.size() &&
         std::isspace(static_cast<unsigned char>(in[b])) != 0) {
    ++b;
  }
  std::size_t e = in.size();
  while (e > b && std::isspace(static_cast<unsigned char>(in[e - 1])) != 0) {
    --e;
  }
  return std::string(in.substr(b, e - b));
}

[[nodiscard]] inline bool is_wave_contract_binding_variable_name(
    std::string_view name) {
  if (name.size() < 3) return false;
  if (name[0] != '_' || name[1] != '_') return false;
  for (std::size_t i = 2; i < name.size(); ++i) {
    const char c = name[i];
    const bool alpha = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    const bool digit = (c >= '0' && c <= '9');
    const bool symbol = c == '_' || c == '.' || c == '-';
    if (!(alpha || digit || symbol)) return false;
  }
  return true;
}

[[nodiscard]] inline const wave_contract_binding_variable_t*
find_wave_contract_binding_variable(
    const wave_contract_binding_decl_t& binding, std::string_view name) {
  for (const auto& variable : binding.variables) {
    if (variable.name == name) return &variable;
  }
  return nullptr;
}

[[nodiscard]] inline bool append_wave_contract_binding_variable(
    wave_contract_binding_decl_t* binding, std::string name, std::string value,
    std::string* error = nullptr) {
  if (error) error->clear();
  if (!binding) {
    if (error) *error = "missing wave-contract binding destination";
    return false;
  }
  name = wave_contract_binding_trim_ascii(name);
  if (!is_wave_contract_binding_variable_name(name)) {
    if (error) {
      *error = "invalid binding variable name '" + name +
               "' (variables must start with '__')";
    }
    return false;
  }
  if (find_wave_contract_binding_variable(*binding, name) != nullptr) {
    if (error) *error = "duplicate binding variable: " + name;
    return false;
  }
  binding->variables.push_back(
      wave_contract_binding_variable_t{std::move(name), std::move(value)});
  return true;
}

namespace detail {

struct wave_contract_binding_placeholder_t {
  std::string variable_name{};
  std::string default_value{};
  bool has_default{false};
};

[[nodiscard]] inline bool parse_wave_contract_binding_placeholder(
    std::string_view body, wave_contract_binding_placeholder_t* out,
    std::string* error = nullptr) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "missing placeholder destination";
    return false;
  }
  out->variable_name.clear();
  out->default_value.clear();
  out->has_default = false;

  const std::string trimmed = wave_contract_binding_trim_ascii(body);
  if (trimmed.empty()) {
    if (error) *error = "empty binding variable placeholder";
    return false;
  }

  std::size_t pos = 0;
  while (pos < trimmed.size() &&
         std::isspace(static_cast<unsigned char>(trimmed[pos])) != 0) {
    ++pos;
  }
  const std::size_t name_begin = pos;
  while (pos < trimmed.size()) {
    const char c = trimmed[pos];
    const bool alpha = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    const bool digit = (c >= '0' && c <= '9');
    const bool symbol = c == '_' || c == '.' || c == '-';
    if (!(alpha || digit || symbol)) break;
    ++pos;
  }
  const std::string variable_name =
      trimmed.substr(name_begin, pos - name_begin);
  if (!is_wave_contract_binding_variable_name(variable_name)) {
    if (error) {
      *error = "invalid binding placeholder variable '" + variable_name +
               "' (variables must start with '__')";
    }
    return false;
  }

  while (pos < trimmed.size() &&
         std::isspace(static_cast<unsigned char>(trimmed[pos])) != 0) {
    ++pos;
  }
  if (pos == trimmed.size()) {
    out->variable_name = variable_name;
    return true;
  }
  if (trimmed[pos] != '?') {
    if (error) {
      *error = "invalid binding placeholder syntax after variable '" +
               variable_name + "'";
    }
    return false;
  }
  ++pos;
  const std::string default_value =
      wave_contract_binding_trim_ascii(trimmed.substr(pos));
  if (default_value.empty()) {
    if (error) {
      *error = "binding placeholder default is empty for variable '" +
               variable_name + "'";
    }
    return false;
  }

  out->variable_name = variable_name;
  out->default_value = default_value;
  out->has_default = true;
  return true;
}

}  // namespace detail

[[nodiscard]] inline bool resolve_wave_contract_binding_variables_in_text(
    std::string_view text, const wave_contract_binding_decl_t& binding,
    std::string* out_text, std::string* error = nullptr) {
  if (error) error->clear();
  if (!out_text) {
    if (error) *error = "missing destination for resolved binding text";
    return false;
  }
  out_text->clear();
  out_text->reserve(text.size());

  bool in_block_comment = false;
  bool in_line_comment = false;
  for (std::size_t i = 0; i < text.size();) {
    const char c = text[i];
    const char next = (i + 1 < text.size()) ? text[i + 1] : '\0';

    if (in_block_comment) {
      out_text->push_back(c);
      if (c == '*' && next == '/') {
        out_text->push_back(next);
        i += 2;
        in_block_comment = false;
      } else {
        ++i;
      }
      continue;
    }
    if (in_line_comment) {
      out_text->push_back(c);
      ++i;
      if (c == '\n') in_line_comment = false;
      continue;
    }

    if (c == '/' && next == '*') {
      out_text->push_back(c);
      out_text->push_back(next);
      i += 2;
      in_block_comment = true;
      continue;
    }
    if (c == '/' && next == '/') {
      out_text->push_back(c);
      out_text->push_back(next);
      i += 2;
      in_line_comment = true;
      continue;
    }
    if (c == '#') {
      out_text->push_back(c);
      ++i;
      in_line_comment = true;
      continue;
    }

    if (c != '%') {
      out_text->push_back(c);
      ++i;
      continue;
    }

    const std::size_t close = text.find('%', i + 1);
    if (close == std::string_view::npos) {
      if (error) *error = "unterminated binding variable placeholder";
      return false;
    }

    detail::wave_contract_binding_placeholder_t placeholder{};
    std::string parse_error{};
    if (!detail::parse_wave_contract_binding_placeholder(
            text.substr(i + 1, close - i - 1), &placeholder, &parse_error)) {
      if (error) *error = parse_error;
      return false;
    }

    const auto* variable =
        find_wave_contract_binding_variable(binding, placeholder.variable_name);
    if (variable != nullptr) {
      out_text->append(variable->value);
    } else if (placeholder.has_default) {
      out_text->append(placeholder.default_value);
    } else {
      if (error) {
        *error = "missing binding variable '" + placeholder.variable_name +
                 "' and no default was provided";
      }
      return false;
    }
    i = close + 1;
  }
  return true;
}

}  // namespace camahjucunu
}  // namespace cuwacunu
