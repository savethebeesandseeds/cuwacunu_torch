
namespace cuwacunu {
namespace hero {
namespace lattice_mcp {

std::filesystem::path resolve_lattice_hero_dsl_path(
    const std::filesystem::path& global_config_path) {
  return ::resolve_lattice_hero_dsl_path(global_config_path);
}

bool load_wave_runtime_defaults(const std::filesystem::path& hero_dsl_path,
                                const std::filesystem::path& global_config_path,
                                wave_runtime_defaults_t* out,
                                std::string* error) {
  return ::load_wave_runtime_defaults(hero_dsl_path, global_config_path, out,
                                      error);
}

bool execute_tool_json(const std::string& tool_name, std::string arguments_json,
                       app_context_t* app,
                       std::string* out_tool_result_json,
                       std::string* out_error_message) {
  if (out_error_message) out_error_message->clear();
  if (!app || !out_tool_result_json || !out_error_message) return false;
  const bool catalog_already_open = app->catalog.opened();
  const bool needs_preopen = tool_requires_catalog_preopen(tool_name);
  const bool opened_here = !catalog_already_open && needs_preopen;
  const std::uint64_t tool_started_at_ms = now_ms_utc();
  std::uint64_t catalog_hold_started_at_ms = 0;
  log_lattice_tool_begin(app, tool_name, catalog_already_open);
  if (needs_preopen && !::open_lattice_catalog_if_needed(app, out_error_message)) {
    *out_error_message = "catalog open failed: " + *out_error_message;
    log_lattice_tool_end(app, tool_name, catalog_already_open, false, false,
                         now_ms_utc() - tool_started_at_ms, 0,
                         *out_error_message);
    return false;
  }
  if (opened_here) {
    catalog_hold_started_at_ms = now_ms_utc();
  }
  const bool ok = ::handle_tool_call(app, tool_name, arguments_json,
                                     out_tool_result_json, out_error_message);
  bool catalog_closed = false;
  if (opened_here && app->close_catalog_after_execute) {
    std::string close_error;
    if (!::close_lattice_catalog_if_open(app, &close_error)) {
      if (!out_error_message->empty()) out_error_message->append("; ");
      out_error_message->append("catalog close failed: " + close_error);
      log_lattice_tool_end(
          app, tool_name, catalog_already_open, false, false,
          now_ms_utc() - tool_started_at_ms,
          catalog_hold_started_at_ms == 0 ? 0
                                          : now_ms_utc() -
                                                catalog_hold_started_at_ms,
          *out_error_message);
      return false;
    }
    catalog_closed = true;
  }
  const std::uint64_t finished_at_ms = now_ms_utc();
  log_lattice_tool_end(
      app, tool_name, catalog_already_open, ok, catalog_closed,
      finished_at_ms - tool_started_at_ms,
      catalog_hold_started_at_ms == 0 ? 0
                                      : finished_at_ms -
                                            catalog_hold_started_at_ms,
      out_error_message->empty() ? std::string_view{}
                                 : std::string_view(*out_error_message));
  return ok;
}

bool tool_result_is_error(std::string_view tool_result_json) {
  const std::string json(tool_result_json);
  bool is_error = false;
  return ::extract_json_bool_field(json, "isError", &is_error) && is_error;
}

std::string build_tools_list_result_json() {
  return ::build_tools_list_result_json();
}

std::string build_tools_list_human_text() {
  return ::build_tools_list_human_text();
}

void prepare_jsonrpc_stdio_server_output() {
  if (g_protocol_stdout_fd >= 0) return;
  std::fflush(stdout);
  g_protocol_stdout_fd = ::dup(STDOUT_FILENO);
  if (g_protocol_stdout_fd < 0) return;
  (void)::dup2(STDERR_FILENO, STDOUT_FILENO);
}

void write_cli_stdout(std::string_view payload) { ::write_cli_stdout(payload); }

void run_jsonrpc_stdio_loop(app_context_t* app) {
  ::run_jsonrpc_stdio_loop_impl(app);
}

}  // namespace lattice_mcp
}  // namespace hero
}  // namespace cuwacunu
