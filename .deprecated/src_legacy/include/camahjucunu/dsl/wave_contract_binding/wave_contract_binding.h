#pragma once

#include <cctype>
#include <string>
#include <string_view>
#include <vector>

namespace cuwacunu {
namespace camahjucunu {

struct dsl_variable_t {
  std::string name{};
  std::string value{};
};

struct dsl_variable_scope_t {
  std::vector<dsl_variable_t> variables{};
};

using wave_contract_binding_variable_t = dsl_variable_t;

struct wave_contract_binding_decl_t {
  std::string id{};
  std::string contract_ref{};
  std::string wave_ref{};
  std::vector<wave_contract_binding_variable_t> variables{};
};

[[nodiscard]] inline std::string dsl_variable_trim_ascii(std::string_view in) {
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

[[nodiscard]] inline std::string wave_contract_binding_trim_ascii(
    std::string_view in) {
  return dsl_variable_trim_ascii(in);
}

[[nodiscard]] inline bool is_dsl_variable_name(std::string_view name) {
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

[[nodiscard]] inline bool is_wave_contract_binding_variable_name(
    std::string_view name) {
  return is_dsl_variable_name(name);
}

[[nodiscard]] inline const dsl_variable_t* find_dsl_variable(
    const std::vector<dsl_variable_t>& variables, std::string_view name) {
  for (const auto& variable : variables) {
    if (variable.name == name) return &variable;
  }
  return nullptr;
}

[[nodiscard]] inline const wave_contract_binding_variable_t*
find_wave_contract_binding_variable(
    const wave_contract_binding_decl_t& binding, std::string_view name) {
  return find_dsl_variable(binding.variables, name);
}

[[nodiscard]] inline bool append_dsl_variable(
    std::vector<dsl_variable_t>* variables, std::string name, std::string value,
    std::string* error = nullptr) {
  if (error) error->clear();
  if (!variables) {
    if (error) *error = "missing DSL variable destination";
    return false;
  }
  name = dsl_variable_trim_ascii(name);
  if (!is_dsl_variable_name(name)) {
    if (error) {
      *error = "invalid DSL variable name '" + name +
               "' (variables must start with '__')";
    }
    return false;
  }
  if (find_dsl_variable(*variables, name) != nullptr) {
    if (error) *error = "duplicate DSL variable: " + name;
    return false;
  }
  variables->push_back(dsl_variable_t{std::move(name), std::move(value)});
  return true;
}

[[nodiscard]] inline bool append_wave_contract_binding_variable(
    wave_contract_binding_decl_t* binding, std::string name, std::string value,
    std::string* error = nullptr) {
  if (error) error->clear();
  if (!binding) {
    if (error) *error = "missing wave-contract binding destination";
    return false;
  }
  return append_dsl_variable(&binding->variables, std::move(name), std::move(value),
                             error);
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

  const std::string trimmed = dsl_variable_trim_ascii(body);
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
  if (!is_dsl_variable_name(variable_name)) {
    if (error) {
      *error = "invalid DSL placeholder variable '" + variable_name +
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
      dsl_variable_trim_ascii(trimmed.substr(pos));
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

[[nodiscard]] inline bool resolve_dsl_variables_in_text(
    std::string_view text, const std::vector<dsl_variable_t>& variables,
    std::string* out_text, std::string* error = nullptr);

[[nodiscard]] inline bool resolve_wave_contract_binding_variables_in_text(
    std::string_view text, const wave_contract_binding_decl_t& binding,
    std::string* out_text, std::string* error = nullptr) {
  return resolve_dsl_variables_in_text(text, binding.variables, out_text, error);
}

[[nodiscard]] inline bool resolve_dsl_variables_in_text(
    std::string_view text, const std::vector<dsl_variable_t>& variables,
    std::string* out_text, std::string* error) {
  if (error) error->clear();
  if (!out_text) {
    if (error) *error = "missing destination for resolved DSL text";
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
      if (error) *error = "unterminated DSL variable placeholder";
      return false;
    }

    detail::wave_contract_binding_placeholder_t placeholder{};
    std::string parse_error{};
    if (!detail::parse_wave_contract_binding_placeholder(
            text.substr(i + 1, close - i - 1), &placeholder, &parse_error)) {
      if (error) *error = parse_error;
      return false;
    }

    const auto* variable = find_dsl_variable(variables, placeholder.variable_name);
    if (variable != nullptr) {
      out_text->append(variable->value);
    } else if (placeholder.has_default) {
      out_text->append(placeholder.default_value);
    } else {
      if (error) {
        *error = "missing DSL variable '" + placeholder.variable_name +
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
