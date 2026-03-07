#include "cuwacunu/hero/hero_config_commands.h"

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string_view>
#include <vector>

#include "HERO/hero_config/hero.config.h"
#include "piaabo/https_compat/curl_toolkit/openai_responses_api.h"

namespace {

constexpr const char* kDefaultUserAgent = "hero-config-mcp/0.1";

[[nodiscard]] std::string trim_ascii(std::string_view in) {
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

[[nodiscard]] std::string lowercase_copy(std::string_view in) {
  std::string out(in);
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return out;
}

[[nodiscard]] std::string field_escape(std::string_view in) {
  std::string out;
  out.reserve(in.size() + 8);
  for (const char c : in) {
    if (c == '\t') {
      out += "\\t";
    } else if (c == '\n') {
      out += "\\n";
    } else if (c == '\r') {
      out += "\\r";
    } else {
      out.push_back(c);
    }
  }
  return out;
}

void emit_ok(std::string_view command, std::string_view message) {
  std::cout << "ok\t" << command << '\t' << field_escape(message) << '\n';
}

void emit_err(std::string_view command, std::string_view message) {
  std::cout << "err\t" << command << '\t' << field_escape(message) << '\n';
}

void emit_data(std::string_view key, std::string_view value) {
  std::cout << "data\t" << field_escape(key) << '\t' << field_escape(value)
            << '\n';
}

void emit_end() {
  std::cout << "end\n";
  std::cout.flush();
}

[[nodiscard]] std::string json_escape(std::string_view in) {
  std::ostringstream out;
  for (const unsigned char c : in) {
    switch (c) {
      case '\"':
        out << "\\\"";
        break;
      case '\\':
        out << "\\\\";
        break;
      case '\b':
        out << "\\b";
        break;
      case '\f':
        out << "\\f";
        break;
      case '\n':
        out << "\\n";
        break;
      case '\r':
        out << "\\r";
        break;
      case '\t':
        out << "\\t";
        break;
      default:
        if (c < 0x20) {
          out << "\\u00";
          const char* hex = "0123456789abcdef";
          out << hex[(c >> 4) & 0x0F] << hex[c & 0x0F];
        } else {
          out << static_cast<char>(c);
        }
        break;
    }
  }
  return out.str();
}

[[nodiscard]] std::string json_quote(std::string_view in) {
  return "\"" + json_escape(in) + "\"";
}

[[nodiscard]] bool parse_json_string_at(const std::string& json, std::size_t pos,
                                        std::string* out,
                                        std::size_t* end_pos) {
  if (pos >= json.size() || json[pos] != '"') return false;
  ++pos;
  std::string value;
  while (pos < json.size()) {
    const char c = json[pos++];
    if (c == '"') {
      if (out) *out = value;
      if (end_pos) *end_pos = pos;
      return true;
    }
    if (c != '\\') {
      value.push_back(c);
      continue;
    }
    if (pos >= json.size()) return false;
    const char esc = json[pos++];
    switch (esc) {
      case '"':
      case '\\':
      case '/':
        value.push_back(esc);
        break;
      case 'b':
        value.push_back('\b');
        break;
      case 'f':
        value.push_back('\f');
        break;
      case 'n':
        value.push_back('\n');
        break;
      case 'r':
        value.push_back('\r');
        break;
      case 't':
        value.push_back('\t');
        break;
      default:
        value.push_back(esc);
        break;
    }
  }
  return false;
}

[[nodiscard]] bool extract_json_string_field(const std::string& json,
                                             const std::string& key,
                                             std::string* out) {
  const std::string needle = "\"" + key + "\"";
  std::size_t pos = json.find(needle);
  if (pos == std::string::npos) return false;
  pos = json.find(':', pos + needle.size());
  if (pos == std::string::npos) return false;
  ++pos;
  while (pos < json.size() &&
         std::isspace(static_cast<unsigned char>(json[pos])) != 0) {
    ++pos;
  }
  return parse_json_string_at(json, pos, out, nullptr);
}

[[nodiscard]] bool extract_json_id_field(const std::string& json,
                                         std::string* out_id_json) {
  const std::string needle = "\"id\"";
  std::size_t pos = json.find(needle);
  if (pos == std::string::npos) {
    if (out_id_json) *out_id_json = "null";
    return true;
  }
  pos = json.find(':', pos + needle.size());
  if (pos == std::string::npos) return false;
  ++pos;
  while (pos < json.size() &&
         std::isspace(static_cast<unsigned char>(json[pos])) != 0) {
    ++pos;
  }
  if (pos >= json.size()) return false;

  if (json[pos] == '"') {
    std::string parsed;
    std::size_t end_pos = pos;
    if (!parse_json_string_at(json, pos, &parsed, &end_pos)) return false;
    if (out_id_json) *out_id_json = json_quote(parsed);
    return true;
  }

  std::size_t end = pos;
  while (end < json.size()) {
    const char c = json[end];
    if (c == ',' || c == '}' || c == ']' ||
        std::isspace(static_cast<unsigned char>(c)) != 0) {
      break;
    }
    ++end;
  }
  if (end <= pos) return false;
  if (out_id_json) *out_id_json = json.substr(pos, end - pos);
  return true;
}

void emit_jsonrpc_result(std::string_view id_json,
                         std::string_view result_object_json) {
  std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_json
            << ",\"result\":" << result_object_json << "}\n";
  std::cout.flush();
}

void emit_jsonrpc_error(std::string_view id_json, int code,
                        std::string_view message) {
  std::cout << "{\"jsonrpc\":\"2.0\",\"id\":" << id_json
            << ",\"error\":{\"code\":" << code
            << ",\"message\":" << json_quote(message) << "}}\n";
  std::cout.flush();
}

[[nodiscard]] std::string bool_json(bool v) { return v ? "true" : "false"; }

[[nodiscard]] bool parse_bool_value(std::string_view in, bool* out) {
  const std::string v = lowercase_copy(trim_ascii(in));
  if (v == "true" || v == "1" || v == "yes" || v == "y" || v == "on") {
    if (out) *out = true;
    return true;
  }
  if (v == "false" || v == "0" || v == "no" || v == "n" || v == "off") {
    if (out) *out = false;
    return true;
  }
  return false;
}

[[nodiscard]] bool parse_int64_value(std::string_view in, int64_t* out) {
  const std::string v = trim_ascii(in);
  if (v.empty()) return false;
  int64_t parsed = 0;
  const auto [ptr, ec] =
      std::from_chars(v.data(), v.data() + v.size(), parsed);
  if (ec != std::errc() || ptr != v.data() + v.size()) return false;
  if (out) *out = parsed;
  return true;
}

[[nodiscard]] bool parse_double_value(std::string_view in, double* out) {
  const std::string v = trim_ascii(in);
  if (v.empty()) return false;
  char* end = nullptr;
  errno = 0;
  const double parsed = std::strtod(v.c_str(), &end);
  if (errno != 0 || end == nullptr || *end != '\0') return false;
  if (out) *out = parsed;
  return true;
}

[[nodiscard]] bool run_llm_command(
    std::string_view command, std::string_view prompt,
    const cuwacunu::hero::mcp::hero_config_store_t& store, std::string* out_body,
    std::string* out_error) {
  const std::string mode = lowercase_copy(store.get_or_default("mode"));
  try {
    cuwacunu::hero::config::validate_backend_mode_or_throw(mode);
  } catch (const std::exception& e) {
    if (out_error) *out_error = e.what();
    return false;
  }

  if (lowercase_copy(store.get_or_default("transport")) != "curl") {
    if (out_error) *out_error = "transport must be curl in openai mode";
    return false;
  }

  const std::string auth_env = store.get_or_default("auth_token_env");
  const char* token = std::getenv(auth_env.c_str());
  if (token == nullptr || trim_ascii(token).empty()) {
    if (out_error) {
      *out_error =
          "missing OpenAI token in environment variable: " + auth_env +
          " (export it before running ask/fix)";
    }
    return false;
  }

  const std::string model = trim_ascii(store.get_or_default("model"));
  const std::string endpoint = trim_ascii(store.get_or_default("endpoint"));
  const std::string reasoning_effort =
      trim_ascii(store.get_or_default("reasoning_effort"));

  int64_t timeout_ms = 30000;
  int64_t connect_timeout_ms = 10000;
  int64_t retry_max_attempts = 3;
  int64_t retry_backoff_ms = 700;
  int64_t max_output_tokens = 1400;
  double temperature = 0.20;
  double top_p = 1.00;
  bool verify_tls = true;
  (void)parse_int64_value(store.get_or_default("timeout_ms"), &timeout_ms);
  (void)parse_int64_value(store.get_or_default("connect_timeout_ms"),
                          &connect_timeout_ms);
  (void)parse_int64_value(store.get_or_default("retry_max_attempts"),
                          &retry_max_attempts);
  (void)parse_int64_value(store.get_or_default("retry_backoff_ms"),
                          &retry_backoff_ms);
  (void)parse_int64_value(store.get_or_default("max_output_tokens"),
                          &max_output_tokens);
  (void)parse_double_value(store.get_or_default("temperature"), &temperature);
  (void)parse_double_value(store.get_or_default("top_p"), &top_p);
  (void)parse_bool_value(store.get_or_default("verify_tls"), &verify_tls);

  cuwacunu::piaabo::curl::openai_responses_request_t request{};
  request.endpoint = endpoint;
  request.bearer_token = token;
  request.model = model;
  request.reasoning_effort = reasoning_effort;
  request.system_prompt =
      (command == "fix")
          ? "You are HERO config fixer. Propose deterministic config edits "
            "and explain risk checks."
          : "You are HERO config analyst. Analyze config and provide concise, "
            "systematic guidance.";
  request.user_prompt = std::string(prompt);
  request.temperature = temperature;
  request.top_p = top_p;
  request.max_output_tokens = max_output_tokens;
  request.user_agent = kDefaultUserAgent;
  request.timeout_ms = timeout_ms;
  request.connect_timeout_ms = connect_timeout_ms;
  request.retry_max_attempts = retry_max_attempts;
  request.retry_backoff_ms = retry_backoff_ms;
  request.verify_tls = verify_tls;

  cuwacunu::piaabo::curl::openai_responses_result_t result;
  if (!cuwacunu::piaabo::curl::post_openai_responses(request, &result)) {
    if (out_error) {
      std::ostringstream ss;
      ss << result.error;
      if (!result.body.empty()) ss << " | body: " << result.body;
      *out_error = ss.str();
    }
    return false;
  }
  if (out_body) *out_body = std::move(result.body);
  return true;
}

void print_help() {
  emit_ok("help", "available commands");
  emit_data("help", "ping");
  emit_data("help", "status");
  emit_data("help", "profiles");
  emit_data("help", "targets");
  emit_data("help", "schema");
  emit_data("help", "show");
  emit_data("help", "get <key>");
  emit_data("help", "set <key> <value>");
  emit_data("help", "validate");
  emit_data("help", "save");
  emit_data("help", "reload");
  emit_data("help", "ask <prompt>");
  emit_data("help", "fix <prompt>");
  emit_data("help", "quit");
  emit_end();
}

[[nodiscard]] std::string build_status_result(
    const cuwacunu::hero::mcp::hero_config_store_t& store) {
  const auto errors = store.validate();
  std::ostringstream out;
  out << "{";
  out << "\"valid\":" << bool_json(errors.empty()) << ",";
  out << "\"config_path\":" << json_quote(store.config_path()) << ",";
  out << "\"dirty\":" << bool_json(store.dirty()) << ",";
  out << "\"from_template\":" << bool_json(store.from_template()) << ",";
  out << "\"mode\":" << json_quote(store.get_or_default("mode")) << ",";
  out << "\"protocol_layer\":"
      << json_quote(store.get_or_default("protocol_layer")) << ",";
  out << "\"transport\":" << json_quote(store.get_or_default("transport")) << ",";
  out << "\"endpoint\":" << json_quote(store.get_or_default("endpoint")) << ",";
  out << "\"model\":" << json_quote(store.get_or_default("model")) << ",";
  out << "\"backup_enabled\":"
      << json_quote(store.get_or_default("backup_enabled")) << ",";
  out << "\"backup_dir\":" << json_quote(store.get_or_default("backup_dir"))
      << ",";
  out << "\"backup_max_entries\":"
      << json_quote(store.get_or_default("backup_max_entries")) << ",";
  out << "\"errors\":[";
  for (std::size_t i = 0; i < errors.size(); ++i) {
    if (i != 0) out << ",";
    out << json_quote(errors[i]);
  }
  out << "]}";
  return out.str();
}

[[nodiscard]] std::string build_tools_list_result() {
  return R"JSON({"tools":[{"name":"hero.status","description":"Runtime status and validation snapshot"},{"name":"hero.schema","description":"Runtime key schema"},{"name":"hero.show","description":"Current key-value entries"},{"name":"hero.get","description":"Get one key (arguments: key)"},{"name":"hero.set","description":"Set one key (arguments: key,value)"},{"name":"hero.validate","description":"Validate runtime config"},{"name":"hero.save","description":"Save runtime config to disk"},{"name":"hero.reload","description":"Reload runtime config from disk"},{"name":"hero.ask","description":"Ask model for config analysis (arguments: prompt)"},{"name":"hero.fix","description":"Ask model for config fix guidance (arguments: prompt)"}]})JSON";
}

[[nodiscard]] bool dispatch_tool_jsonrpc(
    const std::string& tool_name, const std::string& request_json,
    cuwacunu::hero::mcp::hero_config_store_t* store, std::string* out_result_json,
    int* out_error_code, std::string* out_error_message) {
  if (out_error_code) *out_error_code = -32603;
  if (out_error_message) *out_error_message = "internal error";

  if (tool_name == "hero.status") {
    if (out_result_json) *out_result_json = build_status_result(*store);
    return true;
  }
  if (tool_name == "hero.schema") {
    std::ostringstream out;
    out << "{\"keys\":[";
    for (std::size_t i = 0; i < cuwacunu::hero::config::kRuntimeKeyDescriptors.size();
         ++i) {
      const auto& d = cuwacunu::hero::config::kRuntimeKeyDescriptors[i];
      if (i != 0) out << ",";
      out << "{"
          << "\"key\":" << json_quote(d.key) << ","
          << "\"type\":" << json_quote(d.type) << ","
          << "\"required\":" << bool_json(d.required) << ","
          << "\"default\":" << json_quote(d.default_value) << ","
          << "\"range\":" << json_quote(d.range) << ","
          << "\"allowed\":" << json_quote(d.allowed) << "}";
    }
    out << "]}";
    if (out_result_json) *out_result_json = out.str();
    return true;
  }
  if (tool_name == "hero.show") {
    const auto entries = store->entries_snapshot();
    std::ostringstream out;
    out << "{\"entries\":[";
    for (std::size_t i = 0; i < entries.size(); ++i) {
      if (i != 0) out << ",";
      out << "{"
          << "\"key\":" << json_quote(entries[i].key) << ","
          << "\"type\":" << json_quote(entries[i].declared_type) << ","
          << "\"value\":" << json_quote(entries[i].value) << "}";
    }
    out << "]}";
    if (out_result_json) *out_result_json = out.str();
    return true;
  }
  if (tool_name == "hero.get") {
    std::string key;
    if (!extract_json_string_field(request_json, "key", &key) || key.empty()) {
      if (out_error_code) *out_error_code = -32602;
      if (out_error_message) *out_error_message = "hero.get requires argument key";
      return false;
    }
    const auto value = store->get_value(key);
    if (!value.has_value()) {
      if (out_error_code) *out_error_code = -32602;
      if (out_error_message) *out_error_message = "key not found: " + key;
      return false;
    }
    if (out_result_json) {
      *out_result_json = "{\"key\":" + json_quote(key) + ",\"value\":" +
                         json_quote(*value) + "}";
    }
    return true;
  }
  if (tool_name == "hero.set") {
    std::string key;
    std::string value;
    if (!extract_json_string_field(request_json, "key", &key) || key.empty()) {
      if (out_error_code) *out_error_code = -32602;
      if (out_error_message) *out_error_message = "hero.set requires argument key";
      return false;
    }
    if (!extract_json_string_field(request_json, "value", &value)) {
      if (out_error_code) *out_error_code = -32602;
      if (out_error_message) *out_error_message = "hero.set requires argument value";
      return false;
    }
    std::string err;
    if (!store->set_value(key, value, &err)) {
      if (out_error_code) *out_error_code = -32602;
      if (out_error_message) *out_error_message = err;
      return false;
    }
    if (out_result_json) {
      *out_result_json = "{\"updated\":true,\"key\":" + json_quote(key) +
                         ",\"value\":" + json_quote(value) + "}";
    }
    return true;
  }
  if (tool_name == "hero.validate") {
    const auto errors = store->validate();
    std::ostringstream out;
    out << "{\"valid\":" << bool_json(errors.empty()) << ",\"errors\":[";
    for (std::size_t i = 0; i < errors.size(); ++i) {
      if (i != 0) out << ",";
      out << json_quote(errors[i]);
    }
    out << "]}";
    if (out_result_json) *out_result_json = out.str();
    return true;
  }
  if (tool_name == "hero.save") {
    std::string err;
    if (!store->save(&err)) {
      if (out_error_code) *out_error_code = -32603;
      if (out_error_message) *out_error_message = err;
      return false;
    }
    if (out_result_json) {
      *out_result_json =
          "{\"saved\":true,\"path\":" + json_quote(store->config_path()) + "}";
    }
    return true;
  }
  if (tool_name == "hero.reload") {
    std::string err;
    if (!store->load(&err)) {
      if (out_error_code) *out_error_code = -32603;
      if (out_error_message) *out_error_message = err;
      return false;
    }
    if (out_result_json) {
      *out_result_json = "{\"reloaded\":true,\"path\":" +
                         json_quote(store->config_path()) + "}";
    }
    return true;
  }
  if (tool_name == "hero.ask" || tool_name == "hero.fix") {
    std::string prompt;
    if (!extract_json_string_field(request_json, "prompt", &prompt) ||
        prompt.empty()) {
      if (out_error_code) *out_error_code = -32602;
      if (out_error_message) *out_error_message = tool_name + " requires argument prompt";
      return false;
    }
    const auto errors = store->validate();
    if (!errors.empty()) {
      if (out_error_code) *out_error_code = -32602;
      if (out_error_message) {
        *out_error_message = "config invalid; run hero.validate first";
      }
      return false;
    }
    std::string body;
    std::string err;
    const std::string verb = (tool_name == "hero.ask") ? "ask" : "fix";
    if (!run_llm_command(verb, prompt, *store, &body, &err)) {
      if (out_error_code) *out_error_code = -32603;
      if (out_error_message) *out_error_message = err;
      return false;
    }
    if (out_result_json) {
      std::ostringstream out;
      out << "{\"ok\":true,\"bytes\":" << body.size()
          << ",\"payload\":" << json_quote(body) << "}";
      *out_result_json = out.str();
    }
    return true;
  }

  if (out_error_code) *out_error_code = -32601;
  if (out_error_message) *out_error_message = "unknown tool: " + tool_name;
  return false;
}

}  // namespace

namespace cuwacunu {
namespace hero {
namespace mcp {

void emit_startup_banner(const hero_config_store_t& store) {
  emit_ok("startup", "hero_config_mcp ready; run help for commands");
  emit_data("config_path", store.config_path());
  emit_data("from_template", store.from_template() ? "true" : "false");
  emit_end();
}

bool execute_command_line(std::string line, hero_config_store_t* store,
                          bool* should_exit) {
  line = trim_ascii(line);
  if (line.empty()) return true;

  std::string command;
  std::string args;
  {
    const std::size_t space = line.find_first_of(" \t");
    if (space == std::string::npos) {
      command = lowercase_copy(line);
    } else {
      command = lowercase_copy(line.substr(0, space));
      args = trim_ascii(line.substr(space + 1));
    }
  }

  if (command == "quit" || command == "exit") {
    emit_ok(command, "bye");
    emit_end();
    if (should_exit) *should_exit = true;
    return true;
  }
  if (command == "help") {
    print_help();
    return true;
  }
  if (command == "ping") {
    emit_ok(command, "pong");
    emit_end();
    return true;
  }
  if (command == "status") {
    const auto errors = store->validate();
    emit_ok(command, errors.empty() ? "valid" : "invalid");
    emit_data("config_path", store->config_path());
    emit_data("dirty", store->dirty() ? "true" : "false");
    emit_data("from_template", store->from_template() ? "true" : "false");
    emit_data("mode", store->get_or_default("mode"));
    emit_data("protocol_layer", store->get_or_default("protocol_layer"));
    emit_data("transport", store->get_or_default("transport"));
    emit_data("endpoint", store->get_or_default("endpoint"));
    emit_data("model", store->get_or_default("model"));
    emit_data("backup_enabled", store->get_or_default("backup_enabled"));
    emit_data("backup_dir", store->get_or_default("backup_dir"));
    emit_data("backup_max_entries", store->get_or_default("backup_max_entries"));
    emit_data("error_count", std::to_string(errors.size()));
    emit_end();
    return true;
  }
  if (command == "profiles") {
    emit_ok(command, "profile manifest");
    for (const auto& p : cuwacunu::hero::config::kProfileDescriptors) {
      emit_data(std::string("profile.") + std::string(p.profile_name),
                std::string("model=") + std::string(p.model) + ",summary=" +
                    std::string(p.summary));
    }
    emit_end();
    return true;
  }
  if (command == "targets") {
    emit_ok(command, "future learning targets");
    for (const auto& t : cuwacunu::hero::config::kFutureLearningTargets) {
      emit_data(std::string(t.include_path), std::string(t.summary));
    }
    emit_end();
    return true;
  }
  if (command == "schema") {
    emit_ok(command, "runtime key schema");
    for (const auto& d : cuwacunu::hero::config::kRuntimeKeyDescriptors) {
      std::ostringstream row;
      row << "type=" << d.type << ",required=" << (d.required ? "true" : "false")
          << ",default=" << d.default_value << ",range=" << d.range
          << ",allowed=" << d.allowed;
      emit_data(d.key, row.str());
    }
    emit_end();
    return true;
  }
  if (command == "show") {
    emit_ok(command, "current values");
    for (const auto& e : store->entries_snapshot()) {
      emit_data(e.key, e.value + " (type=" + e.declared_type + ")");
    }
    emit_end();
    return true;
  }
  if (command == "get") {
    if (args.empty()) {
      emit_err(command, "usage: get <key>");
      emit_end();
      return false;
    }
    const auto value = store->get_value(args);
    if (!value.has_value()) {
      emit_err(command, "key not found: " + args);
      emit_end();
      return false;
    }
    emit_ok(command, "value");
    emit_data(args, *value);
    emit_end();
    return true;
  }
  if (command == "set") {
    const std::size_t split = args.find_first_of(" \t");
    if (args.empty() || split == std::string::npos) {
      emit_err(command, "usage: set <key> <value>");
      emit_end();
      return false;
    }
    const std::string key = trim_ascii(args.substr(0, split));
    std::string value = trim_ascii(args.substr(split + 1));
    if (value.size() >= 2 &&
        ((value.front() == '"' && value.back() == '"') ||
         (value.front() == '\'' && value.back() == '\''))) {
      value = value.substr(1, value.size() - 2);
    }
    std::string err;
    if (!store->set_value(key, value, &err)) {
      emit_err(command, err);
      emit_end();
      return false;
    }
    emit_ok(command, "updated");
    emit_data(key, value);
    emit_end();
    return true;
  }
  if (command == "validate") {
    const auto errors = store->validate();
    if (errors.empty()) {
      emit_ok(command, "valid");
    } else {
      emit_err(command, "invalid");
      for (const auto& e : errors) emit_data("error", e);
    }
    emit_end();
    return errors.empty();
  }
  if (command == "save") {
    std::string err;
    if (!store->save(&err)) {
      emit_err(command, err);
      emit_end();
      return false;
    }
    emit_ok(command, "saved");
    emit_data("path", store->config_path());
    emit_end();
    return true;
  }
  if (command == "reload") {
    std::string err;
    if (!store->load(&err)) {
      emit_err(command, err);
      emit_end();
      return false;
    }
    emit_ok(command, "reloaded");
    emit_data("path", store->config_path());
    emit_end();
    return true;
  }
  if (command == "ask" || command == "fix") {
    if (args.empty()) {
      emit_err(command, "usage: ask <prompt> | fix <prompt>");
      emit_end();
      return false;
    }
    const auto errors = store->validate();
    if (!errors.empty()) {
      emit_err(command, "config invalid; run validate and fix issues first");
      for (const auto& e : errors) emit_data("error", e);
      emit_end();
      return false;
    }

    std::string body;
    std::string err;
    if (!run_llm_command(command, args, *store, &body, &err)) {
      emit_err(command, err);
      emit_end();
      return false;
    }

    emit_ok(command, "model response");
    emit_data("bytes", std::to_string(body.size()));
    std::cout << "payload_begin\n";
    std::cout << body;
    if (!body.empty() && body.back() != '\n') std::cout << '\n';
    std::cout << "payload_end\n";
    emit_end();
    return true;
  }

  emit_err(command, "unknown command; run help");
  emit_end();
  return false;
}

void run_jsonrpc_stdio_loop(hero_config_store_t* store) {
  std::string line;
  while (std::getline(std::cin, line)) {
    const std::string trimmed = trim_ascii(line);
    if (trimmed.empty()) continue;

    std::string id_json;
    if (!extract_json_id_field(trimmed, &id_json)) {
      emit_jsonrpc_error("null", -32700, "invalid request: unable to parse id");
      continue;
    }

    std::string method;
    if (!extract_json_string_field(trimmed, "method", &method) ||
        method.empty()) {
      emit_jsonrpc_error(id_json, -32600,
                         "invalid request: missing method");
      continue;
    }

    if (method == "initialize") {
      emit_jsonrpc_result(
          id_json,
          "{\"name\":\"hero_config_mcp\",\"protocol\":\"jsonrpc-stdio\","
          "\"protocol_layer\":\"STDIO\","
          "\"capabilities\":{\"tools\":true}}");
      continue;
    }
    if (method == "ping") {
      emit_jsonrpc_result(id_json, "{\"pong\":true}");
      continue;
    }
    if (method == "tools/list") {
      emit_jsonrpc_result(id_json, build_tools_list_result());
      continue;
    }

    if (method == "tools/call" || method.rfind("hero.", 0) == 0) {
      std::string tool_name = method;
      if (method == "tools/call") {
        if (!extract_json_string_field(trimmed, "name", &tool_name) ||
            tool_name.empty()) {
          emit_jsonrpc_error(id_json, -32602,
                             "tools/call requires params.name");
          continue;
        }
      }

      std::string result_json;
      int error_code = -32603;
      std::string error_message;
      if (!dispatch_tool_jsonrpc(tool_name, trimmed, store, &result_json,
                                 &error_code, &error_message)) {
        emit_jsonrpc_error(id_json, error_code, error_message);
        continue;
      }
      emit_jsonrpc_result(id_json, result_json);
      continue;
    }

    emit_jsonrpc_error(id_json, -32601, "method not found: " + method);
  }
}

}  // namespace mcp
}  // namespace hero
}  // namespace cuwacunu
