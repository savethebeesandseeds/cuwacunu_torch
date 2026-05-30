[[nodiscard]] std::string pending_request_row_to_json(
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  std::ostringstream out;
  out << "{"
      << "\"marshal_session_id\":" << json_quote(session.marshal_session_id)
      << ","
      << "\"objective_name\":" << json_quote(session.objective_name) << ","
      << "\"operator_state\":"
      << json_quote(std::string(
             operator_session_state_label(operator_session_state(session))))
      << ","
      << "\"operator_state_detail\":"
      << json_quote(operator_session_state_detail(session, false)) << ","
      << "\"operator_action_hint\":"
      << json_quote(operator_session_action_hint(session)) << ","
      << "\"phase\":" << json_quote(session.lifecycle) << ","
      << "\"status_detail\":" << json_quote(session.status_detail) << ","
      << "\"work_gate\":" << json_quote(session.work_gate) << ","
      << "\"request_kind\":"
      << json_quote(std::string(human_request_kind_label(session.work_gate)))
      << ","
      << "\"checkpoint_count\":" << session.checkpoint_count << ","
      << "\"started_at_ms\":" << session.started_at_ms << ","
      << "\"updated_at_ms\":" << session.updated_at_ms << ","
      << "\"human_request_path\":" << json_quote(session.human_request_path)
      << ","
      << "\"governance_resolution_path\":"
      << json_quote(
             cuwacunu::hero::marshal::
                 marshal_session_human_governance_resolution_latest_path(
                     std::filesystem::path(session.session_root).parent_path(),
                     session.marshal_session_id)
                     .string())
      << ",\"governance_resolution_sig_path\":"
      << json_quote(
             cuwacunu::hero::marshal::
                 marshal_session_human_governance_resolution_latest_sig_path(
                     std::filesystem::path(session.session_root).parent_path(),
                     session.marshal_session_id)
                     .string())
      << ",\"clarification_answer_path\":"
      << json_quote(
             cuwacunu::hero::marshal::
                 marshal_session_human_clarification_answer_latest_path(
                     std::filesystem::path(session.session_root).parent_path(),
                     session.marshal_session_id)
                     .string())
      << ",\"request_excerpt\":" << json_quote(request_excerpt(session)) << "}";
  return out.str();
}

[[nodiscard]] std::string pending_summary_row_to_json(
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  const std::optional<std::uint64_t> elapsed_ms =
      marshal_session_elapsed_ms(session);
  std::ostringstream out;
  out << "{"
      << "\"marshal_session_id\":" << json_quote(session.marshal_session_id)
      << ","
      << "\"objective_name\":" << json_quote(session.objective_name) << ","
      << "\"operator_state\":"
      << json_quote(std::string(
             operator_session_state_label(operator_session_state(session))))
      << ","
      << "\"operator_state_detail\":"
      << json_quote(operator_session_state_detail(session, true)) << ","
      << "\"operator_action_hint\":"
      << json_quote(operator_session_action_hint(session)) << ","
      << "\"lifecycle\":" << json_quote(session.lifecycle) << ","
      << "\"work_gate\":" << json_quote(session.work_gate) << ","
      << "\"activity\":" << json_quote(session.activity) << ","
      << "\"campaign_status\":" << json_quote(session.campaign_status) << ","
      << "\"campaign_cursor\":" << json_quote(session.campaign_cursor) << ","
      << "\"current_thread_id\":" << json_quote(session.current_thread_id)
      << ","
      << "\"codex_continuity\":"
      << json_quote(session.codex_continuity) << ","
      << "\"status_detail\":" << json_quote(session.status_detail) << ","
      << "\"finish_reason\":" << json_quote(session.finish_reason) << ","
      << "\"checkpoint_count\":" << session.checkpoint_count << ","
      << "\"launch_count\":" << session.launch_count << ","
      << "\"started_at_ms\":" << session.started_at_ms << ","
      << "\"updated_at_ms\":" << session.updated_at_ms << ","
      << "\"finished_at_ms\":"
      << (session.finished_at_ms.has_value()
              ? std::to_string(*session.finished_at_ms)
              : "null")
      << ","
      << "\"duration_ms\":"
      << (elapsed_ms.has_value() ? std::to_string(*elapsed_ms) : "null") << ","
      << "\"max_campaign_launches\":" << session.max_campaign_launches << ","
      << "\"remaining_campaign_launches\":"
      << session.remaining_campaign_launches << ","
      << "\"effort_summary_text\":"
      << json_quote(build_summary_effort_text(session)) << ","
      << "\"summary_path\":"
      << json_quote(summary_path_for_session(session).string()) << ","
      << "\"summary_ack_path\":"
      << json_quote(summary_ack_path_for_session(session).string()) << ","
      << "\"summary_ack_sig_path\":"
      << json_quote(summary_ack_sig_path_for_session(session).string()) << ","
      << "\"summary_excerpt\":" << json_quote(summary_excerpt(session)) << "}";
  return out.str();
}

[[nodiscard]] bool collect_all_sessions(
    const app_context_t &app,
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t> *out,
    std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "sessions output pointer is null";
    return false;
  }
  out->clear();
  return cuwacunu::hero::marshal::scan_marshal_session_records(
      app.defaults.marshal_root, out, error);
}

[[nodiscard]] bool collect_human_operator_inbox(const app_context_t &app,
                                                human_operator_inbox_t *out,
                                                bool sync_markers,
                                                std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "operator inbox output pointer is null";
    return false;
  }
  out->actionable_requests.clear();
  out->unacknowledged_summaries.clear();
  out->all_sessions.clear();

  std::vector<cuwacunu::hero::marshal::marshal_session_record_t> sessions{};
  if (!cuwacunu::hero::marshal::scan_marshal_session_records(
          app.defaults.marshal_root, &sessions, error)) {
    return false;
  }
  out->all_sessions = sessions;
  for (const auto &session : sessions) {
    if (session.lifecycle == "live" &&
        session.work_gate != "open" &&
        is_human_request_work_gate(session.work_gate)) {
      out->actionable_requests.push_back(session);
      continue;
    }
    if (!cuwacunu::hero::marshal::is_marshal_session_summary_state(
            session.lifecycle, session.activity)) {
      continue;
    }
    const std::filesystem::path summary_path =
        summary_path_for_session(session);
    if (!std::filesystem::exists(summary_path) ||
        !std::filesystem::is_regular_file(summary_path)) {
      continue;
    }
    std::string ack_error{};
    if (!summary_ack_matches_current_summary(session, &ack_error)) {
      out->unacknowledged_summaries.push_back(session);
    }
  }

  if (!sync_markers)
    return true;
  return write_human_pending_marker_counts(
      app.defaults.marshal_root, out->actionable_requests.size(),
      out->unacknowledged_summaries.size(), error);
}

[[nodiscard]] bool collect_pending_requests(
    const app_context_t &app,
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t> *out,
    std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "pending requests output pointer is null";
    return false;
  }
  human_operator_inbox_t inbox{};
  if (!collect_human_operator_inbox(app, &inbox, true, error))
    return false;
  *out = std::move(inbox.actionable_requests);
  return true;
}

[[nodiscard]] bool collect_pending_reviews(
    const app_context_t &app,
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t> *out,
    std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "pending reviews output pointer is null";
    return false;
  }
  human_operator_inbox_t inbox{};
  if (!collect_human_operator_inbox(app, &inbox, true, error))
    return false;
  *out = std::move(inbox.unacknowledged_summaries);
  return true;
}

[[nodiscard]] bool build_request_answer_and_resume(
    app_context_t *app,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::string answer, std::string *out_structured, std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (!app || !out_structured || !out_error)
    return false;
  answer = trim_ascii(answer);
  if (answer.empty()) {
    *out_error = "human clarification answer requires a non-empty answer";
    return false;
  }

  const auto answer_path =
      cuwacunu::hero::marshal::marshal_session_human_clarification_answer_path(
          app->defaults.marshal_root, session.marshal_session_id,
          session.checkpoint_count);
  const auto latest_answer_path = cuwacunu::hero::marshal::
      marshal_session_human_clarification_answer_latest_path(
          app->defaults.marshal_root, session.marshal_session_id);
  const std::string answer_json =
      std::string("{\"schema\":") +
      json_quote(kHumanClarificationAnswerSchemaV3) +
      ",\"marshal_session_id\":" + json_quote(session.marshal_session_id) +
      ",\"checkpoint_index\":" + std::to_string(session.checkpoint_count) +
      ",\"answer\":" + json_quote(answer) + "}";
  if (!cuwacunu::hero::runtime::write_text_file_atomic(answer_path, answer_json,
                                                       out_error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          latest_answer_path, answer_json, out_error)) {
    return false;
  }

  std::string marshal_structured{};
  const std::string resume_args =
      "{\"marshal_session_id\":" + json_quote(session.marshal_session_id) +
      ",\"clarification_answer_path\":" + json_quote(answer_path.string()) +
      "}";
  if (!call_marshal_tool(*app, "hero.marshal.resume_session", resume_args,
                         &marshal_structured, out_error)) {
    return false;
  }
  sync_human_pending_markers_best_effort(*app);

  *out_structured =
      "{\"marshal_session_id\":" + json_quote(session.marshal_session_id) +
      ",\"clarification_answer_path\":" + json_quote(answer_path.string()) +
      ",\"marshal\":" + marshal_structured + "}";
  return true;
}

[[nodiscard]] bool build_resolution_and_apply(
    app_context_t *app,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::string resolution_kind, std::string reason,
    std::string *out_structured, std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (!app || !out_structured || !out_error)
    return false;

  resolution_kind = trim_ascii(resolution_kind);
  reason = trim_ascii(reason);
  if (resolution_kind != "grant" && resolution_kind != "deny" &&
      resolution_kind != "clarify" && resolution_kind != "terminate") {
    *out_error =
        "human resolution_kind must be grant, deny, clarify, or terminate";
    return false;
  }
  if (reason.empty()) {
    *out_error = "human resolution requires a non-empty reason";
    return false;
  }
  if (app->defaults.operator_id.empty() ||
      app->defaults.operator_id == "CHANGE_ME_OPERATOR") {
    *out_error = "Human Hero operator_id is not configured; update "
                 "default.hero.human.dsl";
    return false;
  }
  if (app->defaults.operator_signing_ssh_identity.empty()) {
    *out_error = "Human Hero operator_signing_ssh_identity is not configured";
    return false;
  }

  std::string request_sha256_hex{};
  if (!cuwacunu::hero::human::sha256_hex_file(session.human_request_path,
                                              &request_sha256_hex, out_error)) {
    return false;
  }

  const std::filesystem::path latest_intent_path =
      cuwacunu::hero::marshal::marshal_session_latest_intent_checkpoint_path(
          app->defaults.marshal_root, session.marshal_session_id);
  std::string latest_intent_json{};
  if (!cuwacunu::hero::runtime::read_text_file(
          latest_intent_path, &latest_intent_json, out_error)) {
    return false;
  }
  std::string intent{};
  (void)extract_json_string_field(latest_intent_json, "intent", &intent);
  intent = trim_ascii(intent);
  if (intent != "request_governance") {
    *out_error = "latest pending intent is not a governance request";
    return false;
  }
  std::string governance_json{};
  if (!extract_json_object_field(latest_intent_json, "governance",
                                 &governance_json)) {
    *out_error = "latest intent is missing governance object";
    return false;
  }
  std::string governance_kind{};
  (void)extract_json_string_field(governance_json, "kind", &governance_kind);
  governance_kind = trim_ascii(governance_kind);
  if (governance_kind.empty()) {
    *out_error = "latest intent governance is missing kind";
    return false;
  }

  std::string delta_json{};
  (void)extract_json_object_field(governance_json, "delta", &delta_json);
  human_resolution_record_t resolution{};
  resolution.marshal_session_id = session.marshal_session_id;
  resolution.checkpoint_index = session.checkpoint_count;
  resolution.request_sha256_hex = request_sha256_hex;
  resolution.operator_id = app->defaults.operator_id;
  resolution.resolved_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  resolution.resolution_kind = resolution_kind;
  resolution.governance_kind = governance_kind;
  resolution.reason = reason;
  resolution.signer_public_key_fingerprint_sha256_hex = std::string(64, '0');
  if (resolution_kind == "grant") {
    (void)extract_json_bool_field(delta_json, "allow_default_write",
                                  &resolution.grant_allow_default_write);
    (void)extract_json_u64_field(
        delta_json, "additional_campaign_launches",
        &resolution.grant_additional_campaign_launches);
  }

  std::string signature_hex{};
  std::string fingerprint_hex{};
  std::string provisional_json =
      cuwacunu::hero::human::human_resolution_to_json(resolution);
  if (!cuwacunu::hero::human::sign_human_attested_json(
          app->defaults.operator_signing_ssh_identity, provisional_json,
          &signature_hex, &fingerprint_hex, out_error)) {
    return false;
  }
  resolution.signer_public_key_fingerprint_sha256_hex = fingerprint_hex;
  const std::string resolution_json =
      cuwacunu::hero::human::human_resolution_to_json(resolution);
  if (!cuwacunu::hero::human::sign_human_attested_json(
          app->defaults.operator_signing_ssh_identity, resolution_json,
          &signature_hex, &fingerprint_hex, out_error)) {
    return false;
  }
  if (fingerprint_hex != resolution.signer_public_key_fingerprint_sha256_hex) {
    *out_error = "human resolution fingerprint changed during signing";
    return false;
  }

  std::error_code ec{};
  const auto response_dir =
      cuwacunu::hero::marshal::marshal_session_human_governance_resolutions_dir(
          app->defaults.marshal_root, session.marshal_session_id);
  std::filesystem::create_directories(response_dir, ec);
  if (ec) {
    *out_error =
        "cannot create human resolution directory: " + response_dir.string();
    return false;
  }
  const auto response_path =
      cuwacunu::hero::marshal::marshal_session_human_governance_resolution_path(
          app->defaults.marshal_root, session.marshal_session_id,
          session.checkpoint_count);
  const auto response_sig_path = cuwacunu::hero::marshal::
      marshal_session_human_governance_resolution_sig_path(
          app->defaults.marshal_root, session.marshal_session_id,
          session.checkpoint_count);
  const auto latest_path = cuwacunu::hero::marshal::
      marshal_session_human_governance_resolution_latest_path(
          app->defaults.marshal_root, session.marshal_session_id);
  const auto latest_sig_path = cuwacunu::hero::marshal::
      marshal_session_human_governance_resolution_latest_sig_path(
          app->defaults.marshal_root, session.marshal_session_id);

  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          response_path, resolution_json, out_error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          response_sig_path, signature_hex + "\n", out_error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          latest_path, resolution_json, out_error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          latest_sig_path, signature_hex + "\n", out_error)) {
    return false;
  }

  std::string marshal_structured{};
  std::string resume_args =
      "{\"marshal_session_id\":" + json_quote(session.marshal_session_id) +
      ",\"governance_resolution_path\":" + json_quote(response_path.string()) +
      ",\"governance_resolution_sig_path\":" +
      json_quote(response_sig_path.string()) + "}";
  if (!call_marshal_tool(*app, "hero.marshal.resume_session", resume_args,
                         &marshal_structured, out_error)) {
    return false;
  }
  sync_human_pending_markers_best_effort(*app);

  *out_structured =
      "{\"marshal_session_id\":" + json_quote(session.marshal_session_id) +
      ",\"governance_resolution_path\":" + json_quote(response_path.string()) +
      ",\"governance_resolution_sig_path\":" +
      json_quote(response_sig_path.string()) +
      ",\"fingerprint_sha256_hex\":" + json_quote(fingerprint_hex) +
      ",\"marshal\":" + marshal_structured + "}";
  return true;
}

[[nodiscard]] bool finalize_session_summary(
    app_context_t *app,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::string note, std::string disposition, std::string *out_structured,
    std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (!app || !out_structured || !out_error)
    return false;
  note = trim_ascii(note);
  disposition = lowercase_copy(trim_ascii(disposition));
  if (disposition != "acknowledged" && disposition != "dismissed" &&
      disposition != "archived") {
    *out_error = "invalid summary disposition";
    return false;
  }
  if (note.empty()) {
    *out_error = "review acknowledgment requires a non-empty note";
    return false;
  }
  if (disposition == "dismissed")
    disposition = "acknowledged";

  if (!cuwacunu::hero::marshal::is_marshal_session_summary_state(
          session)) {
    *out_error = "session does not currently expose an informational summary";
    return false;
  }
  if (app->defaults.operator_id.empty() ||
      app->defaults.operator_id == "CHANGE_ME_OPERATOR") {
    *out_error = "Human Hero operator_id is not configured; update "
                 "default.hero.human.dsl";
    return false;
  }
  if (app->defaults.operator_signing_ssh_identity.empty()) {
    *out_error = "Human Hero operator_signing_ssh_identity is not configured";
    return false;
  }

  const std::filesystem::path summary_path = summary_path_for_session(session);
  std::string summary_sha256_hex{};
  if (!cuwacunu::hero::human::sha256_hex_file(summary_path, &summary_sha256_hex,
                                              out_error)) {
    return false;
  }

  const std::uint64_t acknowledged_at_ms =
      cuwacunu::hero::runtime::now_ms_utc();
  std::string ack_json =
      std::string("{\"schema\":") + json_quote(kHumanSummaryAckSchemaV3) +
      ",\"marshal_session_id\":" + json_quote(session.marshal_session_id) +
      ",\"phase\":" + json_quote(session.lifecycle) +
      ",\"finish_reason\":" + json_quote(session.finish_reason) +
      ",\"summary_sha256_hex\":" + json_quote(summary_sha256_hex) +
      ",\"operator_id\":" + json_quote(app->defaults.operator_id) +
      ",\"acknowledged_at_ms\":" + std::to_string(acknowledged_at_ms) +
      ",\"disposition\":" + json_quote(disposition) +
      ",\"note\":" + json_quote(note) +
      ",\"signer_public_key_fingerprint_sha256_hex\":" +
      json_quote(std::string(64, '0')) + "}";

  std::string signature_hex{};
  std::string fingerprint_hex{};
  if (!cuwacunu::hero::human::sign_human_attested_json(
          app->defaults.operator_signing_ssh_identity, ack_json, &signature_hex,
          &fingerprint_hex, out_error)) {
    return false;
  }
  if (fingerprint_hex.size() != 64) {
    *out_error = "summary ack signing returned invalid fingerprint length";
    return false;
  }
  ack_json =
      std::string("{\"schema\":") + json_quote(kHumanSummaryAckSchemaV3) +
      ",\"marshal_session_id\":" + json_quote(session.marshal_session_id) +
      ",\"phase\":" + json_quote(session.lifecycle) +
      ",\"finish_reason\":" + json_quote(session.finish_reason) +
      ",\"summary_sha256_hex\":" + json_quote(summary_sha256_hex) +
      ",\"operator_id\":" + json_quote(app->defaults.operator_id) +
      ",\"acknowledged_at_ms\":" + std::to_string(acknowledged_at_ms) +
      ",\"disposition\":" + json_quote(disposition) +
      ",\"note\":" + json_quote(note) +
      ",\"signer_public_key_fingerprint_sha256_hex\":" +
      json_quote(fingerprint_hex) + "}";

  const std::filesystem::path ack_path = summary_ack_path_for_session(session);
  const std::filesystem::path ack_sig_path =
      summary_ack_sig_path_for_session(session);
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          ack_path, ack_json + "\n", out_error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(
          ack_sig_path, signature_hex + "\n", out_error)) {
    return false;
  }
  sync_human_pending_markers_best_effort(*app);

  *out_structured =
      "{\"marshal_session_id\":" + json_quote(session.marshal_session_id) +
      ",\"summary_path\":" + json_quote(summary_path.string()) +
      ",\"summary_ack_path\":" + json_quote(ack_path.string()) +
      ",\"summary_ack_sig_path\":" + json_quote(ack_sig_path.string()) +
      ",\"disposition\":" + json_quote(disposition) +
      ",\"note\":" + json_quote(note) +
      ",\"fingerprint_sha256_hex\":" + json_quote(fingerprint_hex) + "}";
  return true;
}

[[nodiscard]] bool acknowledge_session_summary(
    app_context_t *app,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::string note, std::string *out_structured, std::string *out_error) {
  return finalize_session_summary(app, session, std::move(note), "acknowledged",
                                  out_structured, out_error);
}

[[nodiscard]] bool archive_session_summary(
    app_context_t *app,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::string note, std::string *out_structured, std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (!app || !out_structured || !out_error)
    return false;

  std::string marshal_structured{};
  const std::string args = std::string("{\"marshal_session_id\":") +
                           json_quote(session.marshal_session_id) + "}";
  if (!call_marshal_tool(*app, "hero.marshal.archive_session", args,
                         &marshal_structured, out_error)) {
    return false;
  }

  cuwacunu::hero::marshal::marshal_session_record_t archived_session{};
  std::string refresh_error{};
  if (!cuwacunu::hero::marshal::read_marshal_session_record(
          app->defaults.marshal_root, session.marshal_session_id,
          &archived_session, &refresh_error)) {
    *out_error =
        "session archived but failed to reread it for summary finalization: " +
        refresh_error;
    return false;
  }

  std::string summary_structured{};
  std::string summary_error{};
  if (!finalize_session_summary(app, archived_session, std::move(note),
                                "archived", &summary_structured,
                                &summary_error)) {
    *out_error =
        "session archived but failed to finalize signed archive summary: " +
        summary_error;
    return false;
  }

  sync_human_pending_markers_best_effort(*app);
  *out_structured =
      "{\"marshal_session_id\":" + json_quote(session.marshal_session_id) +
      ",\"summary\":" + summary_structured +
      ",\"marshal\":" + marshal_structured + "}";
  return true;
}

[[nodiscard]] bool pause_marshal_session(
    app_context_t *app,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    bool force, std::string *out_structured, std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (!app || !out_structured || !out_error)
    return false;

  std::string marshal_structured{};
  const std::string args = std::string("{\"marshal_session_id\":") +
                           json_quote(session.marshal_session_id) +
                           ",\"force\":" + bool_json(force) + "}";
  if (!call_marshal_tool(*app, "hero.marshal.pause_session", args,
                         &marshal_structured, out_error)) {
    return false;
  }
  sync_human_pending_markers_best_effort(*app);
  *out_structured =
      "{\"marshal_session_id\":" + json_quote(session.marshal_session_id) +
      ",\"force\":" + bool_json(force) + ",\"marshal\":" + marshal_structured +
      "}";
  return true;
}

[[nodiscard]] bool resume_marshal_session(
    app_context_t *app,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::string *out_structured, std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (!app || !out_structured || !out_error)
    return false;

  std::string marshal_structured{};
  const std::string args = std::string("{\"marshal_session_id\":") +
                           json_quote(session.marshal_session_id) + "}";
  if (!call_marshal_tool(*app, "hero.marshal.resume_session", args,
                         &marshal_structured, out_error)) {
    return false;
  }
  sync_human_pending_markers_best_effort(*app);
  *out_structured =
      "{\"marshal_session_id\":" + json_quote(session.marshal_session_id) +
      ",\"marshal\":" + marshal_structured + "}";
  return true;
}

[[nodiscard]] bool message_marshal_session(
    app_context_t *app,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::string message, std::string *out_structured,
    std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (!app || !out_structured || !out_error)
    return false;
  message = trim_ascii(message);
  if (message.empty()) {
    *out_error = "message_session requires a non-empty message";
    return false;
  }

  std::string marshal_structured{};
  const std::string args = std::string("{\"marshal_session_id\":") +
                           json_quote(session.marshal_session_id) +
                           ",\"message\":" + json_quote(message) + "}";
  if (!call_marshal_tool(*app, "hero.marshal.message_session", args,
                         &marshal_structured, out_error)) {
    return false;
  }
  sync_human_pending_markers_best_effort(*app);
  std::string delivery{};
  std::string reply_text{};
  std::string warning{};
  std::string marshal_session_json{};
  std::string session_activity{};
  (void)extract_json_string_field(marshal_structured, "delivery", &delivery);
  (void)extract_json_string_field(marshal_structured, "reply_text",
                                  &reply_text);
  (void)extract_json_string_field(marshal_structured, "warning", &warning);
  if (extract_json_object_field(marshal_structured, "session",
                                &marshal_session_json)) {
    (void)extract_json_string_field(marshal_session_json, "activity",
                                    &session_activity);
  }
  *out_structured =
      "{\"marshal_session_id\":" + json_quote(session.marshal_session_id) +
      ",\"message\":" + json_quote(message) +
      ",\"delivery\":" + json_quote(delivery) +
      ",\"reply_text\":" + json_quote(reply_text) +
      ",\"warning\":" + json_quote(warning) +
      ",\"session_activity\":" + json_quote(session_activity) +
      ",\"marshal\":" + marshal_structured + "}";
  return true;
}

[[nodiscard]] std::string summarize_message_session_structured(
    const std::string &structured, bool refreshing, bool *out_is_error) {
  if (out_is_error)
    *out_is_error = false;
  std::string delivery{};
  std::string reply_text{};
  std::string warning{};
  (void)extract_json_string_field(structured, "delivery", &delivery);
  (void)extract_json_string_field(structured, "reply_text", &reply_text);
  (void)extract_json_string_field(structured, "warning", &warning);
  const auto compact = [](std::string text) {
    text = trim_ascii(text);
    for (char &ch : text) {
      if (ch == '\n' || ch == '\r' || ch == '\t')
        ch = ' ';
    }
    constexpr std::size_t kMax = 180;
    if (text.size() > kMax) {
      text = text.substr(0, kMax - 3);
      text += "...";
    }
    return text;
  };
  const std::string suffix = refreshing ? " Refreshing..." : "";
  if (delivery == "failed") {
    if (out_is_error)
      *out_is_error = true;
    return compact(warning.empty() ? "Marshal message delivery failed."
                                   : warning);
  }
  if (delivery == "delivered" && !trim_ascii(reply_text).empty()) {
    return "Marshal: " + compact(reply_text) + suffix;
  }
  if (delivery == "queued") {
    return "Marshal is processing the message." + suffix;
  }
  return "Delivered message to the session." + suffix;
}

[[nodiscard]] bool terminate_marshal_session(
    app_context_t *app,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    bool force, std::string *out_structured, std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (!app || !out_structured || !out_error)
    return false;

  std::string marshal_structured{};
  const std::string args = std::string("{\"marshal_session_id\":") +
                           json_quote(session.marshal_session_id) +
                           ",\"force\":" + bool_json(force) + "}";
  if (!call_marshal_tool(*app, "hero.marshal.terminate_session", args,
                         &marshal_structured, out_error)) {
    return false;
  }
  sync_human_pending_markers_best_effort(*app);
  *out_structured =
      "{\"marshal_session_id\":" + json_quote(session.marshal_session_id) +
      ",\"force\":" + bool_json(force) + ",\"marshal\":" + marshal_structured +
      "}";
  return true;
}
