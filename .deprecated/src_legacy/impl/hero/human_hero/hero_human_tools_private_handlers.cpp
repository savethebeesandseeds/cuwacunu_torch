[[nodiscard]] bool handle_tool_list_requests(app_context_t *app,
                                             const std::string &arguments_json,
                                             std::string *out_structured,
                                             std::string *out_error);
[[nodiscard]] bool handle_tool_list_summaries(app_context_t *app,
                                              const std::string &arguments_json,
                                              std::string *out_structured,
                                              std::string *out_error);
[[nodiscard]] bool handle_tool_get_request(app_context_t *app,
                                           const std::string &arguments_json,
                                           std::string *out_structured,
                                           std::string *out_error);
[[nodiscard]] bool handle_tool_get_summary(app_context_t *app,
                                           const std::string &arguments_json,
                                           std::string *out_structured,
                                           std::string *out_error);
[[nodiscard]] bool handle_tool_answer_request(app_context_t *app,
                                              const std::string &arguments_json,
                                              std::string *out_structured,
                                              std::string *out_error);
[[nodiscard]] bool handle_tool_resolve_governance(
    app_context_t *app, const std::string &arguments_json,
    std::string *out_structured, std::string *out_error);
[[nodiscard]] bool handle_tool_ack_summary(app_context_t *app,
                                           const std::string &arguments_json,
                                           std::string *out_structured,
                                           std::string *out_error);
[[nodiscard]] bool handle_tool_archive_summary(app_context_t *app,
                                               const std::string &arguments_json,
                                               std::string *out_structured,
                                               std::string *out_error);

#define HERO_HUMAN_TOOL(NAME, DESCRIPTION, INPUT_SCHEMA_JSON, HANDLER)         \
  {NAME, DESCRIPTION, INPUT_SCHEMA_JSON, HANDLER},
constexpr human_tool_descriptor_t kHumanTools[] = {
#include "hero/human_hero/hero_human_tools.def"
};
#undef HERO_HUMAN_TOOL

[[nodiscard]] const human_tool_descriptor_t *
find_human_tool_descriptor(std::string_view tool_name) {
  for (const auto &tool : kHumanTools) {
    if (tool.name == tool_name)
      return &tool;
  }
  return nullptr;
}

[[nodiscard]] bool handle_tool_list_requests(app_context_t *app,
                                             const std::string &arguments_json,
                                             std::string *out_structured,
                                             std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::size_t limit = 0;
  std::size_t offset = 0;
  bool newest_first = true;
  (void)extract_json_size_field(arguments_json, "limit", &limit);
  (void)extract_json_size_field(arguments_json, "offset", &offset);
  (void)extract_json_bool_field(arguments_json, "newest_first", &newest_first);

  std::vector<cuwacunu::hero::marshal::marshal_session_record_t> sessions{};
  if (!collect_pending_requests(*app, &sessions, out_error))
    return false;
  std::sort(sessions.begin(), sessions.end(),
            [newest_first](const auto &a, const auto &b) {
              if (a.updated_at_ms != b.updated_at_ms) {
                return newest_first ? (a.updated_at_ms > b.updated_at_ms)
                                    : (a.updated_at_ms < b.updated_at_ms);
              }
              return newest_first
                         ? (a.marshal_session_id > b.marshal_session_id)
                         : (a.marshal_session_id < b.marshal_session_id);
            });
  const std::size_t total = sessions.size();
  const std::size_t off = std::min(offset, sessions.size());
  std::size_t count = limit;
  if (count == 0)
    count = sessions.size() - off;
  count = std::min(count, sessions.size() - off);

  std::ostringstream rows;
  rows << "[";
  for (std::size_t i = 0; i < count; ++i) {
    if (i != 0)
      rows << ",";
    rows << pending_request_row_to_json(sessions[off + i]);
  }
  rows << "]";

  *out_structured =
      "{\"marshal_session_id\":\"\",\"count\":" + std::to_string(count) +
      ",\"total\":" + std::to_string(total) + ",\"requests\":" + rows.str() +
      "}";
  return true;
}

[[nodiscard]] bool handle_tool_get_request(app_context_t *app,
                                           const std::string &arguments_json,
                                           std::string *out_structured,
                                           std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string marshal_session_id{};
  (void)extract_json_string_field(arguments_json, "marshal_session_id",
                                  &marshal_session_id);
  marshal_session_id = trim_ascii(marshal_session_id);
  if (marshal_session_id.empty()) {
    *out_error = "get_request requires arguments.marshal_session_id";
    return false;
  }

  cuwacunu::hero::marshal::marshal_session_record_t session{};
  if (!cuwacunu::hero::marshal::read_marshal_session_record(
          app->defaults.marshal_root, marshal_session_id, &session,
          out_error)) {
    return false;
  }
  if (session.lifecycle != "live" ||
      !is_human_request_work_gate(session.work_gate)) {
    *out_error = "session is not currently paused for a human request";
    return false;
  }

  std::string request_text{};
  if (!cuwacunu::hero::runtime::read_text_file(session.human_request_path,
                                               &request_text, out_error)) {
    return false;
  }
  *out_structured =
      "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
      ",\"request\":" + pending_request_row_to_json(session) +
      ",\"request_text\":" + json_quote(request_text) + "}";
  return true;
}

[[nodiscard]] bool handle_tool_list_summaries(app_context_t *app,
                                              const std::string &arguments_json,
                                              std::string *out_structured,
                                              std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::size_t limit = 0;
  std::size_t offset = 0;
  bool newest_first = true;
  (void)extract_json_size_field(arguments_json, "limit", &limit);
  (void)extract_json_size_field(arguments_json, "offset", &offset);
  (void)extract_json_bool_field(arguments_json, "newest_first", &newest_first);

  std::vector<cuwacunu::hero::marshal::marshal_session_record_t> sessions{};
  if (!collect_pending_reviews(*app, &sessions, out_error))
    return false;
  std::sort(sessions.begin(), sessions.end(),
            [newest_first](const auto &a, const auto &b) {
              if (a.updated_at_ms != b.updated_at_ms) {
                return newest_first ? (a.updated_at_ms > b.updated_at_ms)
                                    : (a.updated_at_ms < b.updated_at_ms);
              }
              return newest_first
                         ? (a.marshal_session_id > b.marshal_session_id)
                         : (a.marshal_session_id < b.marshal_session_id);
            });
  const std::size_t total = sessions.size();
  const std::size_t off = std::min(offset, sessions.size());
  std::size_t count = limit;
  if (count == 0)
    count = sessions.size() - off;
  count = std::min(count, sessions.size() - off);

  std::ostringstream rows;
  rows << "[";
  for (std::size_t i = 0; i < count; ++i) {
    if (i != 0)
      rows << ",";
    rows << pending_summary_row_to_json(sessions[off + i]);
  }
  rows << "]";

  *out_structured =
      "{\"marshal_session_id\":\"\",\"count\":" + std::to_string(count) +
      ",\"total\":" + std::to_string(total) + ",\"summaries\":" + rows.str() +
      "}";
  return true;
}

[[nodiscard]] bool handle_tool_get_summary(app_context_t *app,
                                           const std::string &arguments_json,
                                           std::string *out_structured,
                                           std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string marshal_session_id{};
  (void)extract_json_string_field(arguments_json, "marshal_session_id",
                                  &marshal_session_id);
  marshal_session_id = trim_ascii(marshal_session_id);
  if (marshal_session_id.empty()) {
    *out_error = "get_summary requires arguments.marshal_session_id";
    return false;
  }

  cuwacunu::hero::marshal::marshal_session_record_t session{};
  if (!cuwacunu::hero::marshal::read_marshal_session_record(
          app->defaults.marshal_root, marshal_session_id, &session,
          out_error)) {
    return false;
  }
  if (!cuwacunu::hero::marshal::is_marshal_session_summary_state(
          session)) {
    *out_error = "session does not currently expose an informational summary";
    return false;
  }

  std::string summary_text{};
  if (!read_summary_text_for_session(session, &summary_text, out_error)) {
    return false;
  }
  *out_structured =
      "{\"marshal_session_id\":" + json_quote(marshal_session_id) +
      ",\"summary\":" + pending_summary_row_to_json(session) +
      ",\"summary_text\":" + json_quote(summary_text) + "}";
  return true;
}

[[nodiscard]] bool handle_tool_answer_request(app_context_t *app,
                                              const std::string &arguments_json,
                                              std::string *out_structured,
                                              std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string marshal_session_id{};
  std::string answer{};
  (void)extract_json_string_field(arguments_json, "marshal_session_id",
                                  &marshal_session_id);
  (void)extract_json_string_field(arguments_json, "answer", &answer);
  marshal_session_id = trim_ascii(marshal_session_id);
  if (marshal_session_id.empty()) {
    *out_error = "answer_request requires arguments.marshal_session_id";
    return false;
  }
  if (trim_ascii(answer).empty()) {
    *out_error = "answer_request requires arguments.answer";
    return false;
  }

  cuwacunu::hero::marshal::marshal_session_record_t session{};
  if (!cuwacunu::hero::marshal::read_marshal_session_record(
          app->defaults.marshal_root, marshal_session_id, &session,
          out_error)) {
    return false;
  }
  if (session.lifecycle != "live" ||
      !is_clarification_work_gate(session.work_gate)) {
    *out_error = "session is not currently paused for clarification";
    return false;
  }
  return build_request_answer_and_resume(app, session, answer, out_structured,
                                         out_error);
}

[[nodiscard]] bool handle_tool_resolve_governance(
    app_context_t *app, const std::string &arguments_json,
    std::string *out_structured, std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string marshal_session_id{};
  std::string resolution_kind{};
  std::string reason{};
  (void)extract_json_string_field(arguments_json, "marshal_session_id",
                                  &marshal_session_id);
  (void)extract_json_string_field(arguments_json, "resolution_kind",
                                  &resolution_kind);
  (void)extract_json_string_field(arguments_json, "reason", &reason);
  marshal_session_id = trim_ascii(marshal_session_id);
  if (marshal_session_id.empty()) {
    *out_error = "resolve_governance requires arguments.marshal_session_id";
    return false;
  }
  if (trim_ascii(resolution_kind).empty()) {
    *out_error = "resolve_governance requires arguments.resolution_kind";
    return false;
  }

  cuwacunu::hero::marshal::marshal_session_record_t session{};
  if (!cuwacunu::hero::marshal::read_marshal_session_record(
          app->defaults.marshal_root, marshal_session_id, &session,
          out_error)) {
    return false;
  }
  if (session.lifecycle != "live" || session.work_gate != "governance") {
    *out_error = "session is not currently paused for governance";
    return false;
  }
  return build_resolution_and_apply(app, session, resolution_kind, reason,
                                    out_structured, out_error);
}

[[nodiscard]] bool handle_tool_ack_summary(app_context_t *app,
                                           const std::string &arguments_json,
                                           std::string *out_structured,
                                           std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string marshal_session_id{};
  std::string note{};
  (void)extract_json_string_field(arguments_json, "marshal_session_id",
                                  &marshal_session_id);
  (void)extract_json_string_field(arguments_json, "note", &note);
  marshal_session_id = trim_ascii(marshal_session_id);
  if (marshal_session_id.empty()) {
    *out_error = "ack_summary requires arguments.marshal_session_id";
    return false;
  }
  if (trim_ascii(note).empty()) {
    *out_error = "ack_summary requires arguments.note";
    return false;
  }

  cuwacunu::hero::marshal::marshal_session_record_t session{};
  if (!cuwacunu::hero::marshal::read_marshal_session_record(
          app->defaults.marshal_root, marshal_session_id, &session,
          out_error)) {
    return false;
  }
  return acknowledge_session_summary(app, session, note, out_structured,
                                     out_error);
}

[[nodiscard]] bool handle_tool_archive_summary(
    app_context_t *app, const std::string &arguments_json,
    std::string *out_structured, std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string marshal_session_id{};
  std::string note{};
  (void)extract_json_string_field(arguments_json, "marshal_session_id",
                                  &marshal_session_id);
  (void)extract_json_string_field(arguments_json, "note", &note);
  marshal_session_id = trim_ascii(marshal_session_id);
  if (marshal_session_id.empty()) {
    *out_error = "archive_summary requires arguments.marshal_session_id";
    return false;
  }
  if (trim_ascii(note).empty()) {
    *out_error = "archive_summary requires arguments.note";
    return false;
  }

  cuwacunu::hero::marshal::marshal_session_record_t session{};
  if (!cuwacunu::hero::marshal::read_marshal_session_record(
          app->defaults.marshal_root, marshal_session_id, &session,
          out_error)) {
    return false;
  }
  return archive_session_summary(app, session, note, out_structured,
                                 out_error);
}
