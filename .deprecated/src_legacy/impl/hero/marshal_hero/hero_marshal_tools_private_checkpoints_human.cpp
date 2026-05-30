[[nodiscard]] std::filesystem::path mutation_checkpoint_path_for(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::uint64_t checkpoint_index) {
  return cuwacunu::hero::marshal::marshal_session_mutation_checkpoint_path(
      app.defaults.marshal_root, loop.marshal_session_id, checkpoint_index);
}

[[nodiscard]] bool has_recorded_mutation_checkpoint(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::uint64_t checkpoint_index, std::filesystem::path *out_path = nullptr) {
  const std::filesystem::path mutation_path =
      mutation_checkpoint_path_for(app, loop, checkpoint_index);
  if (out_path)
    *out_path = mutation_path;
  std::error_code ec{};
  return std::filesystem::exists(mutation_path, ec) && !ec;
}

[[nodiscard]] bool write_mutation_checkpoint_summary(
    const app_context_t &app,
    cuwacunu::hero::marshal::marshal_session_record_t *loop,
    std::uint64_t checkpoint_index,
    const objective_hash_snapshot_t &before_snapshot,
    const objective_hash_snapshot_t &after_snapshot, bool *out_materialized,
    std::string *error) {
  if (error)
    error->clear();
  if (out_materialized)
    *out_materialized = false;
  if (!loop) {
    if (error)
      *error = "mutation checkpoint loop pointer is null";
    return false;
  }

  struct mutation_row_t {
    std::string path{};
    std::string change_kind{};
    std::string before_sha256_hex{};
    std::string after_sha256_hex{};
  };

  std::vector<mutation_row_t> rows{};
  auto before_it = before_snapshot.begin();
  auto after_it = after_snapshot.begin();
  while (before_it != before_snapshot.end() ||
         after_it != after_snapshot.end()) {
    if (after_it == after_snapshot.end() ||
        (before_it != before_snapshot.end() &&
         before_it->first < after_it->first)) {
      rows.push_back(
          mutation_row_t{before_it->first, "deleted", before_it->second, ""});
      ++before_it;
      continue;
    }
    if (before_it == before_snapshot.end() ||
        after_it->first < before_it->first) {
      rows.push_back(
          mutation_row_t{after_it->first, "created", "", after_it->second});
      ++after_it;
      continue;
    }
    if (before_it->second != after_it->second) {
      rows.push_back(mutation_row_t{before_it->first, "updated",
                                    before_it->second, after_it->second});
    }
    ++before_it;
    ++after_it;
  }

  const std::filesystem::path mutation_path =
      mutation_checkpoint_path_for(app, *loop, checkpoint_index);
  const std::filesystem::path latest_mutation_path =
      cuwacunu::hero::marshal::marshal_session_latest_mutation_checkpoint_path(
          app.defaults.marshal_root, loop->marshal_session_id);

  if (rows.empty()) {
    if (has_recorded_mutation_checkpoint(app, *loop, checkpoint_index)) {
      loop->last_mutation_checkpoint_path = mutation_path.string();
      std::string mutation_text{};
      std::string ignored{};
      if (cuwacunu::hero::runtime::read_text_file(mutation_path, &mutation_text,
                                                  &ignored) &&
          !mutation_text.empty()) {
        (void)cuwacunu::hero::runtime::write_text_file_atomic(
            latest_mutation_path, mutation_text, &ignored);
      }
    }
    return true;
  }

  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(kMutationCheckpointSchemaV3) << ","
      << "\"marshal_session_id\":" << json_quote(loop->marshal_session_id)
      << ","
      << "\"checkpoint_index\":" << checkpoint_index << ","
      << "\"objective_root\":" << json_quote(loop->objective_root) << ","
      << "\"changed_file_count\":" << rows.size() << ","
      << "\"files\":[";
  for (std::size_t i = 0; i < rows.size(); ++i) {
    if (i != 0)
      out << ",";
    out << "{"
        << "\"path\":" << json_quote(rows[i].path) << ","
        << "\"change_kind\":" << json_quote(rows[i].change_kind) << ","
        << "\"before_sha256_hex\":" << json_quote(rows[i].before_sha256_hex)
        << ","
        << "\"after_sha256_hex\":" << json_quote(rows[i].after_sha256_hex)
        << "}";
  }
  out << "]}";

  if (!cuwacunu::hero::runtime::write_text_file_atomic(mutation_path, out.str(),
                                                       error)) {
    return false;
  }
  if (!cuwacunu::hero::runtime::write_text_file_atomic(latest_mutation_path,
                                                       out.str(), error)) {
    return false;
  }
  loop->last_mutation_checkpoint_path = mutation_path.string();
  if (out_materialized)
    *out_materialized = true;
  return true;
}

[[nodiscard]] bool materialize_human_request(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::uint64_t turn_index, const marshal_checkpoint_decision_t &decision,
    std::string *error) {
  if (error)
    error->clear();
  std::ostringstream out;
  out << "# Human Request\n\n"
      << "Schema: " << kHumanRequestSchemaV3 << "\n"
      << "Marshal ID: " << loop.marshal_session_id << "\n"
      << "Checkpoint: " << turn_index << "\n"
      << "Pause Kind: "
      << (decision.intent == "pause_for_clarification" ? "clarification"
                                                       : "governance")
      << "\n";
  if (decision.intent == "pause_for_clarification") {
    out << "Clarification Request:\n"
        << decision.clarification_request << "\n\n";
  } else {
    out << "Governance Kind: " << decision.governance_kind << "\n"
        << "Governance Request:\n"
        << decision.governance_request << "\n\n";
  }
  out << "Current Authority Scope: " << loop.authority_scope << "\n"
      << "Remaining Campaign Launches: " << loop.remaining_campaign_launches
      << "\n\n"
      << "Reason:\n"
      << decision.reason << "\n\n"
      << "Last Intent: " << loop.last_codex_action << "\n\n";
  if (decision.intent == "request_governance" &&
      decision.governance_kind == "authority_expansion") {
    out << "Requested authority delta:\n"
        << "- allow_default_write: "
        << (decision.governance_allow_default_write ? "true" : "false")
        << "\n\n";
  } else if (decision.intent == "request_governance" &&
             decision.governance_kind == "launch_budget_expansion") {
    out << "Requested launch budget delta:\n"
        << "- additional_campaign_launches: "
        << decision.governance_additional_campaign_launches << "\n\n";
  }
  out << "Key files:\n"
      << "- Session manifest: "
      << cuwacunu::hero::marshal::marshal_session_manifest_path(
             app.defaults.marshal_root, loop.marshal_session_id)
             .string()
      << "\n"
      << "- Latest input checkpoint: "
      << cuwacunu::hero::marshal::marshal_session_latest_input_checkpoint_path(
             app.defaults.marshal_root, loop.marshal_session_id)
             .string()
      << "\n"
      << "- Latest intent checkpoint: "
      << cuwacunu::hero::marshal::marshal_session_latest_intent_checkpoint_path(
             app.defaults.marshal_root, loop.marshal_session_id)
             .string()
      << "\n"
      << "- Codex stdout: " << loop.codex_stdout_path << "\n"
      << "- Codex stderr: " << loop.codex_stderr_path << "\n"
      << "- Memory: " << loop.memory_path << "\n"
      << "- Events: " << loop.events_path << "\n";
  return cuwacunu::hero::runtime::write_text_file_atomic(
      loop.human_request_path, out.str(), error);
}

void append_human_resolution_note(
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    const cuwacunu::hero::human::human_resolution_record_t &resolution,
    const std::filesystem::path &response_path,
    const std::filesystem::path &signature_path) {
  std::ostringstream out;
  out << "\n\n## Governance Resolution to Checkpoint "
      << resolution.checkpoint_index << "\n\n"
      << "- operator_id: " << resolution.operator_id << "\n"
      << "- resolved_at_ms: " << resolution.resolved_at_ms << "\n"
      << "- resolution_kind: " << resolution.resolution_kind << "\n"
      << "- governance_kind: " << resolution.governance_kind << "\n"
      << "- resolution_path: " << response_path.string() << "\n"
      << "- resolution_sig_path: " << signature_path.string() << "\n";
  if (resolution.resolution_kind == "grant") {
    out << "- grant.allow_default_write: "
        << (resolution.grant_allow_default_write ? "true" : "false") << "\n"
        << "- grant.additional_campaign_launches: "
        << resolution.grant_additional_campaign_launches << "\n";
  }
  out << "\nReason:\n" << trim_ascii(resolution.reason) << "\n";
  std::string ignored{};
  (void)cuwacunu::hero::runtime::append_text_file(loop.memory_path, out.str(),
                                                  &ignored);
}

[[nodiscard]] bool build_marshal_governance_followup_input_checkpoint_json(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::uint64_t checkpoint_index,
    const std::filesystem::path &prior_input_checkpoint_path,
    const std::filesystem::path &response_path,
    const std::filesystem::path &response_sig_path,
    const cuwacunu::hero::human::human_resolution_record_t &response,
    std::string *out_json, std::string *error) {
  if (error)
    error->clear();
  if (!out_json) {
    if (error)
      *error = "governance followup checkpoint output pointer is null";
    return false;
  }
  out_json->clear();

  cuwacunu::camahjucunu::iitepi_campaign_instruction_t instruction{};
  if (!decode_campaign_snapshot(app, loop.campaign_dsl_path, &instruction,
                                error)) {
    return false;
  }
  marshal_run_plan_progress_t run_plan_progress{};
  if (!collect_marshal_run_plan_progress(app, loop, instruction, nullptr,
                                         &run_plan_progress, error)) {
    return false;
  }

  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(kInputCheckpointSchemaV3) << ","
      << "\"checkpoint_kind\":\"governance_followup\","
      << "\"checkpoint_index\":" << checkpoint_index
      << ",\"session\":" << marshal_session_to_json(loop)
      << ",\"run_plan_progress\":"
      << marshal_run_plan_progress_to_json(run_plan_progress)
      << ",\"prior_input_checkpoint_path\":"
      << json_quote(prior_input_checkpoint_path.string())
      << ",\"human_request_path\":" << json_quote(loop.human_request_path)
      << ",\"governance_resolution_path\":"
      << json_quote(response_path.string())
      << ",\"governance_resolution_sig_path\":"
      << json_quote(response_sig_path.string())
      << ",\"verified_governance_resolution\":{"
      << "\"operator_id\":" << json_quote(response.operator_id) << ","
      << "\"resolved_at_ms\":" << response.resolved_at_ms << ","
      << "\"resolution_kind\":" << json_quote(response.resolution_kind) << ","
      << "\"governance_kind\":" << json_quote(response.governance_kind) << ","
      << "\"reason\":" << json_quote(response.reason) << ","
      << "\"grant_delta\":{"
      << "\"allow_default_write\":"
      << bool_json(response.grant_allow_default_write) << ","
      << "\"additional_campaign_launches\":"
      << response.grant_additional_campaign_launches << "}"
      << "},\"memory_path\":" << json_quote(loop.memory_path)
      << ",\"marshal_objective_dsl_path\":"
      << json_quote(loop.marshal_objective_dsl_path)
      << ",\"marshal_objective_md_path\":"
      << json_quote(loop.marshal_objective_md_path)
      << ",\"marshal_guidance_md_path\":"
      << json_quote(loop.marshal_guidance_md_path)
      << ",\"hero_marshal_dsl_path\":" << json_quote(loop.hero_marshal_dsl_path)
      << ",\"config_policy_path\":" << json_quote(loop.config_policy_path)
      << ",\"briefing_path\":" << json_quote(loop.briefing_path)
      << ",\"mutable_objective_root\":" << json_quote(loop.objective_root)
      << ",\"objective_campaign_dsl_path\":"
      << json_quote(loop.campaign_dsl_path)
      << ",\"objective_root\":" << json_quote(loop.objective_root)
      << ",\"campaigns_root\":"
      << json_quote(campaigns_root_for_app(app).string())
      << ",\"marshal_root\":" << json_quote(app.defaults.marshal_root.string())
      << "}";
  *out_json = out.str();
  return true;
}

[[nodiscard]] bool parse_human_clarification_answer_json(
    const std::string &json, std::string_view expected_marshal_session_id,
    std::uint64_t expected_checkpoint_index, std::string *out_answer,
    std::string *error) {
  if (error)
    error->clear();
  if (!out_answer) {
    if (error)
      *error = "clarification answer output pointer is null";
    return false;
  }
  out_answer->clear();
  std::string schema{};
  (void)extract_json_string_field(json, "schema", &schema);
  if (!trim_ascii(schema).empty() &&
      trim_ascii(schema) != std::string(kHumanClarificationAnswerSchemaV3)) {
    if (error) {
      *error = "unsupported clarification answer schema: " + trim_ascii(schema);
    }
    return false;
  }
  std::string marshal_session_id{};
  if (!extract_json_string_field(json, "marshal_session_id",
                                 &marshal_session_id) ||
      trim_ascii(marshal_session_id).empty()) {
    if (error)
      *error = "clarification answer is missing marshal_session_id";
    return false;
  }
  const std::string expected_session_id =
      trim_ascii(expected_marshal_session_id);
  marshal_session_id = trim_ascii(marshal_session_id);
  if (marshal_session_id != expected_session_id) {
    if (error) {
      *error = "clarification answer marshal_session_id does not match paused "
               "session";
    }
    return false;
  }
  std::uint64_t checkpoint_index = 0;
  if (!extract_json_u64_field(json, "checkpoint_index", &checkpoint_index) ||
      checkpoint_index == 0) {
    if (error)
      *error = "clarification answer is missing checkpoint_index";
    return false;
  }
  if (checkpoint_index != expected_checkpoint_index) {
    if (error) {
      *error = "clarification answer checkpoint_index does not match paused "
               "checkpoint";
    }
    return false;
  }
  if (!extract_json_string_field(json, "answer", out_answer) ||
      trim_ascii(*out_answer).empty()) {
    if (error)
      *error = "clarification answer is missing non-empty answer";
    return false;
  }
  *out_answer = trim_ascii(*out_answer);
  return true;
}

[[nodiscard]] bool build_marshal_clarification_followup_checkpoint_json(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::uint64_t checkpoint_index,
    const std::filesystem::path &prior_input_checkpoint_path,
    const std::filesystem::path &answer_path, std::string_view answer,
    std::string *out_json, std::string *error) {
  if (error)
    error->clear();
  if (!out_json) {
    if (error) {
      *error = "clarification followup checkpoint output pointer is null";
    }
    return false;
  }
  out_json->clear();

  cuwacunu::camahjucunu::iitepi_campaign_instruction_t instruction{};
  if (!decode_campaign_snapshot(app, loop.campaign_dsl_path, &instruction,
                                error)) {
    return false;
  }
  marshal_run_plan_progress_t run_plan_progress{};
  if (!collect_marshal_run_plan_progress(app, loop, instruction, nullptr,
                                         &run_plan_progress, error)) {
    return false;
  }

  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(kInputCheckpointSchemaV3) << ","
      << "\"checkpoint_kind\":\"clarification_followup\","
      << "\"checkpoint_index\":" << checkpoint_index
      << ",\"session\":" << marshal_session_to_json(loop)
      << ",\"run_plan_progress\":"
      << marshal_run_plan_progress_to_json(run_plan_progress)
      << ",\"prior_input_checkpoint_path\":"
      << json_quote(prior_input_checkpoint_path.string())
      << ",\"human_request_path\":" << json_quote(loop.human_request_path)
      << ",\"clarification_answer_path\":" << json_quote(answer_path.string())
      << ",\"clarification_answer\":{\"answer\":" << json_quote(answer) << "}"
      << ",\"memory_path\":" << json_quote(loop.memory_path)
      << ",\"marshal_objective_dsl_path\":"
      << json_quote(loop.marshal_objective_dsl_path)
      << ",\"marshal_objective_md_path\":"
      << json_quote(loop.marshal_objective_md_path)
      << ",\"marshal_guidance_md_path\":"
      << json_quote(loop.marshal_guidance_md_path)
      << ",\"hero_marshal_dsl_path\":" << json_quote(loop.hero_marshal_dsl_path)
      << ",\"config_policy_path\":" << json_quote(loop.config_policy_path)
      << ",\"briefing_path\":" << json_quote(loop.briefing_path)
      << ",\"mutable_objective_root\":" << json_quote(loop.objective_root)
      << ",\"objective_campaign_dsl_path\":"
      << json_quote(loop.campaign_dsl_path)
      << ",\"objective_root\":" << json_quote(loop.objective_root)
      << ",\"campaigns_root\":"
      << json_quote(campaigns_root_for_app(app).string())
      << ",\"marshal_root\":" << json_quote(app.defaults.marshal_root.string())
      << "}";
  *out_json = out.str();
  return true;
}

[[nodiscard]] bool build_marshal_message_input_checkpoint_json(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::uint64_t checkpoint_index,
    const std::filesystem::path &prior_input_checkpoint_path,
    const cuwacunu::hero::marshal::marshal_session_record_t::operator_message_t
        &message,
    std::string *out_json, std::string *error) {
  if (error)
    error->clear();
  if (!out_json) {
    if (error)
      *error = "message checkpoint output pointer is null";
    return false;
  }
  out_json->clear();

  const std::string trimmed_message = trim_ascii(message.text);
  if (trimmed_message.empty()) {
    if (error)
      *error = "message checkpoint requires non-empty message";
    return false;
  }

  cuwacunu::camahjucunu::iitepi_campaign_instruction_t campaign_instruction{};
  if (!decode_campaign_snapshot(app, loop.campaign_dsl_path,
                                &campaign_instruction, error)) {
    return false;
  }
  marshal_run_plan_progress_t run_plan_progress{};
  if (!collect_marshal_run_plan_progress(app, loop, campaign_instruction,
                                         nullptr, &run_plan_progress, error)) {
    return false;
  }

  std::string campaign_json{"null"};
  std::ostringstream jobs_json;
  jobs_json << "[]";
  const std::string campaign_cursor = trim_ascii(loop.campaign_cursor);
  if (!campaign_cursor.empty()) {
    cuwacunu::hero::runtime::runtime_campaign_record_t campaign{};
    std::string campaign_error{};
    if (read_runtime_campaign_direct(app, campaign_cursor, &campaign,
                                     &campaign_error)) {
      campaign_json = runtime_campaign_to_json(campaign);
      std::ostringstream current_jobs_json;
      current_jobs_json << "[";
      bool first_job = true;
      for (const std::string &job_cursor : campaign.job_cursors) {
        cuwacunu::hero::runtime::runtime_job_record_t job{};
        if (!read_runtime_job_direct(app, job_cursor, &job, &campaign_error))
          continue;
        if (!first_job)
          current_jobs_json << ",";
        first_job = false;
        current_jobs_json << runtime_job_summary_to_json(
            job, cuwacunu::hero::runtime::observe_runtime_job(job));
      }
      current_jobs_json << "]";
      jobs_json.str("");
      jobs_json.clear();
      jobs_json << current_jobs_json.str();
    }
  }

  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(kInputCheckpointSchemaV3) << ","
      << "\"checkpoint_kind\":\"operator_message\","
      << "\"checkpoint_index\":" << checkpoint_index
      << ",\"session\":" << marshal_session_to_json(loop)
      << ",\"campaign\":" << campaign_json << ",\"jobs\":" << jobs_json.str()
      << ",\"run_plan_progress\":"
      << marshal_run_plan_progress_to_json(run_plan_progress)
      << ",\"prior_input_checkpoint_path\":"
      << json_quote(prior_input_checkpoint_path.string())
      << ",\"operator_message\":{\"message_id\":"
      << json_quote(message.message_id)
      << ",\"text\":" << json_quote(trimmed_message)
      << ",\"delivery_mode\":" << json_quote(message.delivery_mode) << "}"
      << ",\"memory_path\":" << json_quote(loop.memory_path)
      << ",\"marshal_objective_dsl_path\":"
      << json_quote(loop.marshal_objective_dsl_path)
      << ",\"marshal_objective_md_path\":"
      << json_quote(loop.marshal_objective_md_path)
      << ",\"marshal_guidance_md_path\":"
      << json_quote(loop.marshal_guidance_md_path)
      << ",\"hero_marshal_dsl_path\":" << json_quote(loop.hero_marshal_dsl_path)
      << ",\"config_policy_path\":" << json_quote(loop.config_policy_path)
      << ",\"briefing_path\":" << json_quote(loop.briefing_path)
      << ",\"mutable_objective_root\":" << json_quote(loop.objective_root)
      << ",\"objective_campaign_dsl_path\":"
      << json_quote(loop.campaign_dsl_path)
      << ",\"objective_root\":" << json_quote(loop.objective_root)
      << ",\"campaigns_root\":"
      << json_quote(campaigns_root_for_app(app).string())
      << ",\"marshal_root\":" << json_quote(app.defaults.marshal_root.string())
      << "}";
  *out_json = out.str();
  return true;
}

[[nodiscard]] bool load_verified_human_resolution(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    const std::filesystem::path &response_path,
    const std::filesystem::path &signature_path,
    cuwacunu::hero::human::human_resolution_record_t *out_response,
    std::string *out_signature_hex, std::string *error) {
  if (error)
    error->clear();
  if (!out_response || !out_signature_hex) {
    if (error)
      *error = "human resolution outputs are null";
    return false;
  }
  *out_response = cuwacunu::hero::human::human_resolution_record_t{};
  out_signature_hex->clear();

  marshal_session_workspace_context_t workspace_context{};
  if (!load_marshal_session_workspace_context(app, loop, &workspace_context,
                                              error)) {
    return false;
  }
  if (workspace_context.human_operator_identities.empty()) {
    if (error) {
      *error = "Marshal Hero defaults missing human_operator_identities; "
               "cannot verify human resolution";
    }
    return false;
  }

  std::string response_text{};
  if (!cuwacunu::hero::runtime::read_text_file(response_path, &response_text,
                                               error)) {
    return false;
  }
  std::string signature_hex{};
  if (!cuwacunu::hero::runtime::read_text_file(signature_path, &signature_hex,
                                               error)) {
    return false;
  }
  signature_hex = trim_ascii(signature_hex);

  cuwacunu::hero::human::human_resolution_record_t response{};
  if (!cuwacunu::hero::human::parse_human_resolution_json(response_text,
                                                          &response, error)) {
    return false;
  }

  std::string verified_fingerprint{};
  if (!cuwacunu::hero::human::verify_human_attested_json_signature(
          workspace_context.human_operator_identities, response.operator_id,
          response_text, signature_hex, &verified_fingerprint, error)) {
    return false;
  }
  if (response.signer_public_key_fingerprint_sha256_hex !=
      verified_fingerprint) {
    if (error) {
      *error = "human response fingerprint does not match verified public key "
               "fingerprint";
    }
    return false;
  }
  if (response.marshal_session_id != loop.marshal_session_id) {
    if (error)
      *error = "governance resolution marshal_session_id does not match target "
               "session";
    return false;
  }
  if (response.checkpoint_index != loop.checkpoint_count) {
    if (error) {
      *error = "governance resolution checkpoint_index does not match pending "
               "checkpoint";
    }
    return false;
  }
  std::string request_sha256_hex{};
  if (!cuwacunu::hero::human::sha256_hex_file(loop.human_request_path,
                                              &request_sha256_hex, error)) {
    return false;
  }
  if (response.request_sha256_hex != request_sha256_hex) {
    if (error) {
      *error = "governance resolution request_sha256_hex does not match "
               "current request artifact";
    }
    return false;
  }
  *out_response = std::move(response);
  *out_signature_hex = std::move(signature_hex);
  return true;
}

