[[nodiscard]] bool initialize_marshal_session_record(
    const app_context_t &app,
    const std::filesystem::path &source_marshal_objective_dsl_path,
    const marshal_start_session_overrides_t &start_overrides,
    cuwacunu::hero::marshal::marshal_session_record_t *out,
    std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "marshal session output pointer is null";
    return false;
  }
  *out = cuwacunu::hero::marshal::marshal_session_record_t{};

  std::error_code ec{};
  std::filesystem::create_directories(app.defaults.marshal_root, ec);
  if (ec) {
    if (error) {
      *error = "cannot create .marshal_hero root: " +
               app.defaults.marshal_root.string();
    }
    return false;
  }

  marshal_objective_spec_t marshal_objective_spec{};
  if (!decode_marshal_objective_snapshot(
          app, source_marshal_objective_dsl_path.string(),
          &marshal_objective_spec, error)) {
    return false;
  }
  const std::string marshal_session_id =
      trim_ascii(marshal_objective_spec.marshal_session_id).empty()
          ? cuwacunu::hero::marshal::make_marshal_session_id(
                app.defaults.marshal_root)
          : trim_ascii(marshal_objective_spec.marshal_session_id);
  if (!is_valid_marshal_session_id(marshal_session_id)) {
    if (error) {
      *error = "marshal objective marshal_session_id is invalid; use only "
               "[A-Za-z0-9._-]: " +
               marshal_session_id;
    }
    return false;
  }
  std::filesystem::path source_campaign_path{};
  if (!resolve_marshal_objective_member_source_path(
          source_marshal_objective_dsl_path,
          marshal_objective_spec.campaign_dsl_path, "campaign_dsl_path",
          &source_campaign_path, error)) {
    return false;
  }
  std::filesystem::path source_marshal_objective_md_path{};
  if (!resolve_marshal_objective_member_source_path(
          source_marshal_objective_dsl_path,
          marshal_objective_spec.objective_md_path, "objective_md_path",
          &source_marshal_objective_md_path, error)) {
    return false;
  }
  std::filesystem::path source_marshal_guidance_md_path{};
  if (!resolve_marshal_objective_member_source_path(
          source_marshal_objective_dsl_path,
          marshal_objective_spec.guidance_md_path, "guidance_md_path",
          &source_marshal_guidance_md_path, error)) {
    return false;
  }
  cuwacunu::camahjucunu::iitepi_campaign_instruction_t ignored_campaign{};
  if (!decode_campaign_snapshot(app, source_campaign_path.string(),
                                &ignored_campaign, error)) {
    return false;
  }
  {
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t>
        existing_loops{};
    std::string scan_error{};
    if (!list_marshal_sessions_reconciled(app, &existing_loops, &scan_error)) {
      if (error)
        *error = scan_error;
      return false;
    }
    const std::filesystem::path normalized_source_marshal_objective =
        source_marshal_objective_dsl_path.lexically_normal();
    for (const auto &existing : existing_loops) {
      if (cuwacunu::hero::marshal::is_marshal_session_terminal_state(
              existing.lifecycle)) {
        continue;
      }
      if (std::filesystem::path(existing.source_marshal_objective_dsl_path)
              .lexically_normal() == normalized_source_marshal_objective) {
        if (error) {
          const bool resumable_idle =
              cuwacunu::hero::marshal::marshal_session_is_review_ready(
                  existing) &&
              existing.finish_reason == "interrupted";
          *error = (resumable_idle
                        ? "another resumable marshal session already owns this "
                          "marshal objective: "
                        : "another active marshal session already owns this "
                          "marshal objective: ") +
                   existing.marshal_session_id;
        }
        return false;
      }
    }
  }
  const std::filesystem::path session_root =
      cuwacunu::hero::marshal::marshal_session_dir(app.defaults.marshal_root,
                                                   marshal_session_id);
  if (std::filesystem::exists(session_root, ec) && !ec) {
    if (error)
      *error = "marshal session already exists: " + marshal_session_id;
    return false;
  }
  std::filesystem::create_directories(session_root, ec);
  if (ec) {
    if (error)
      *error = "cannot create marshal session dir: " + session_root.string();
    return false;
  }

  const marshal_session_workspace_context_t workspace_context =
      make_marshal_session_workspace_context(app);
  const std::filesystem::path objective_root =
      source_marshal_objective_dsl_path.parent_path().lexically_normal();

  cuwacunu::hero::marshal::marshal_session_record_t loop{};
  loop.marshal_session_id = marshal_session_id;
  loop.lifecycle = "bootstrapping";
  loop.activity = "bootstrapping";
  loop.status_detail = "initializing marshal session";
  loop.work_gate = "open";
  loop.finish_reason = "none";
  loop.objective_name =
      trim_ascii(marshal_objective_spec.objective_name).empty()
          ? derive_marshal_session_objective_name(
                source_marshal_objective_dsl_path)
          : trim_ascii(marshal_objective_spec.objective_name);
  loop.global_config_path = app.global_config_path.string();
  loop.source_marshal_objective_dsl_path =
      source_marshal_objective_dsl_path.string();
  loop.source_campaign_dsl_path = source_campaign_path.string();
  loop.source_marshal_objective_md_path =
      source_marshal_objective_md_path.string();
  loop.source_marshal_guidance_md_path =
      source_marshal_guidance_md_path.string();
  loop.session_root = session_root.string();
  loop.objective_root = objective_root.string();
  loop.campaign_dsl_path = source_campaign_path.lexically_normal().string();
  loop.marshal_objective_dsl_path =
      cuwacunu::hero::marshal::marshal_session_objective_dsl_path(
          app.defaults.marshal_root, marshal_session_id)
          .string();
  loop.marshal_objective_md_path =
      cuwacunu::hero::marshal::marshal_session_objective_md_path(
          app.defaults.marshal_root, marshal_session_id)
          .string();
  loop.marshal_guidance_md_path =
      cuwacunu::hero::marshal::marshal_session_guidance_md_path(
          app.defaults.marshal_root, marshal_session_id)
          .string();
  loop.hero_marshal_dsl_path =
      cuwacunu::hero::marshal::marshal_session_hero_marshal_dsl_path(
          app.defaults.marshal_root, marshal_session_id)
          .string();
  loop.config_policy_path =
      cuwacunu::hero::marshal::marshal_session_config_policy_path(
          app.defaults.marshal_root, marshal_session_id)
          .string();
  loop.briefing_path = cuwacunu::hero::marshal::marshal_session_briefing_path(
                           app.defaults.marshal_root, marshal_session_id)
                           .string();
  loop.memory_path = cuwacunu::hero::marshal::marshal_session_memory_path(
                         app.defaults.marshal_root, marshal_session_id)
                         .string();
  loop.human_request_path =
      cuwacunu::hero::marshal::marshal_session_human_request_path(
          app.defaults.marshal_root, marshal_session_id)
          .string();
  loop.events_path = cuwacunu::hero::marshal::marshal_session_events_path(
                         app.defaults.marshal_root, marshal_session_id)
                         .string();
  loop.turns_path = cuwacunu::hero::marshal::marshal_session_turns_path(
                        app.defaults.marshal_root, marshal_session_id)
                        .string();
  loop.codex_stdout_path =
      cuwacunu::hero::marshal::marshal_session_codex_stdout_path(
          app.defaults.marshal_root, marshal_session_id)
          .string();
  loop.codex_stderr_path =
      cuwacunu::hero::marshal::marshal_session_codex_stderr_path(
          app.defaults.marshal_root, marshal_session_id)
          .string();
  {
    const std::filesystem::path resolved_codex_binary =
        resolve_marshal_codex_binary_path(
            app.defaults.repo_root, app.defaults.marshal_codex_binary.string());
    loop.resolved_marshal_codex_binary =
        resolved_codex_binary.empty()
            ? app.defaults.marshal_codex_binary.string()
            : resolved_codex_binary.string();
  }
  loop.resolved_marshal_codex_model =
      trim_ascii(start_overrides.marshal_codex_model).empty()
          ? (trim_ascii(marshal_objective_spec.marshal_codex_model).empty()
                 ? app.defaults.marshal_codex_model
                 : trim_ascii(marshal_objective_spec.marshal_codex_model))
          : trim_ascii(start_overrides.marshal_codex_model);
  loop.resolved_marshal_codex_reasoning_effort =
      trim_ascii(start_overrides.marshal_codex_reasoning_effort).empty()
          ? (trim_ascii(marshal_objective_spec.marshal_codex_reasoning_effort)
                     .empty()
                 ? app.defaults.marshal_codex_reasoning_effort
                 : trim_ascii(
                       marshal_objective_spec.marshal_codex_reasoning_effort))
          : trim_ascii(start_overrides.marshal_codex_reasoning_effort);
  loop.started_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  loop.updated_at_ms = loop.started_at_ms;
  loop.max_campaign_launches = app.defaults.marshal_max_campaign_launches;
  loop.remaining_campaign_launches = app.defaults.marshal_max_campaign_launches;
  loop.authority_scope = "objective_only";

  if (!write_marshal_session_bootstrap_files(
          workspace_context, source_marshal_objective_dsl_path,
          source_marshal_objective_md_path, source_marshal_guidance_md_path,
          loop, error)) {
    return false;
  }
  if (!write_marshal_session(app, loop, error))
    return false;
  if (!append_marshal_session_event(
          &loop, "marshal", "session.started",
          "marshal session initialized from marshal objective source", error)) {
    return false;
  }
  *out = std::move(loop);
  return true;
}

void append_memory_note(
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::uint64_t turn_index, std::string_view note) {
  const std::string trimmed = trim_ascii(note);
  if (trimmed.empty())
    return;
  std::ostringstream out;
  out << "\n\n## Checkpoint " << turn_index << "\n\n" << trimmed << "\n";
  std::string ignored{};
  (void)cuwacunu::hero::runtime::append_text_file(loop.memory_path, out.str(),
                                                  &ignored);
}

void append_warning_text(std::string *dst, std::string_view warning) {
  if (!dst)
    return;
  const std::string trimmed = trim_ascii(warning);
  if (trimmed.empty())
    return;
  if (!dst->empty())
    dst->append("; ");
  dst->append(trimmed);
}

void append_structured_warnings(const std::string &structured_json,
                                std::string_view prefix,
                                std::vector<std::string> *out) {
  if (!out)
    return;
  std::vector<std::string> warnings{};
  if (!extract_json_string_array_field(structured_json, "warnings",
                                       &warnings)) {
    return;
  }
  for (const std::string &warning : warnings) {
    const std::string trimmed = trim_ascii(warning);
    if (trimmed.empty())
      continue;
    out->push_back(trim_ascii(std::string(prefix) + trimmed));
  }
}

void persist_marshal_session_warning_best_effort(
    const app_context_t &app,
    cuwacunu::hero::marshal::marshal_session_record_t *loop,
    std::string_view warning) {
  if (!loop)
    return;
  const std::string trimmed = trim_ascii(warning);
  if (trimmed.empty())
    return;
  loop->last_warning = trimmed;
  loop->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  std::string ignored{};
  (void)write_marshal_session(app, *loop, &ignored);
  std::ostringstream note;
  note << "\n\n## Runtime Warning\n\n" << trimmed << "\n";
  (void)cuwacunu::hero::runtime::append_text_file(loop->memory_path, note.str(),
                                                  &ignored);
}

[[nodiscard]] bool error_mentions_timeout(std::string_view text) {
  return lowercase_copy(text).find("timed out") != std::string::npos;
}

[[nodiscard]] std::string build_resume_degraded_warning(
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::uint64_t checkpoint_index, std::string_view failure_kind,
    std::string_view detail) {
  std::ostringstream out;
  out << "codex resume degraded: checkpoint=" << checkpoint_index
      << " kind=" << trim_ascii(failure_kind) << " fallback=fresh_checkpoint";
  const std::string prior_session_id = trim_ascii(loop.current_thread_id);
  if (!prior_session_id.empty()) {
    out << " prior_session_id=" << prior_session_id;
  }
  if (error_mentions_timeout(detail)) {
    out << " resume_timeout=true";
  }
  const std::string trimmed_detail = trim_ascii(detail);
  if (!trimmed_detail.empty()) {
    out << " detail=" << trimmed_detail;
  }
  out << "; stderr_log=" << marshal_session_codex_stderr_path(loop).string()
      << "; stdout_log=" << marshal_session_codex_stdout_path(loop).string();
  return out.str();
}

[[nodiscard]] std::filesystem::path marshal_session_resume_prompt_path_for(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::uint64_t checkpoint_index) {
  return cuwacunu::hero::marshal::marshal_session_checkpoints_dir(
             app.defaults.marshal_root, loop.marshal_session_id) /
         ("resume_prompt." +
          cuwacunu::hero::marshal::format_marshal_index(
              static_cast<std::size_t>(checkpoint_index)) +
          ".md");
}

[[nodiscard]] bool write_marshal_resume_prompt(
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::uint64_t checkpoint_index, const std::filesystem::path &prompt_path,
    std::string *error) {
  if (error)
    error->clear();
  if (prompt_path.empty()) {
    if (error)
      *error = "resume prompt path is empty";
    return false;
  }

  const std::filesystem::path latest_input_path =
      marshal_session_latest_input_checkpoint_path(loop);
  std::string input_checkpoint_text{};
  {
    std::string ignored{};
    (void)cuwacunu::hero::runtime::read_text_file(
        latest_input_path, &input_checkpoint_text, &ignored);
  }

  std::string checkpoint_kind{};
  (void)extract_json_string_field(input_checkpoint_text, "checkpoint_kind",
                                  &checkpoint_kind);
  std::string operator_message_json{};
  (void)extract_json_object_field(input_checkpoint_text, "operator_message",
                                  &operator_message_json);
  std::string run_plan_progress_json{};
  (void)extract_json_object_field(input_checkpoint_text, "run_plan_progress",
                                  &run_plan_progress_json);
  std::string campaign_json{};
  (void)extract_json_object_field(input_checkpoint_text, "campaign",
                                  &campaign_json);

  std::ostringstream out;
  out << "You are resuming an existing Marshal Hero Codex thread.\n\n"
      << "Use the living thread context you already have. Do not reread or "
         "restuff the full Marshal briefing, memory, objective bundle, or raw "
         "runtime logs unless this delta explicitly proves continuity is "
         "insufficient.\n\n"
      << "This resume prompt is intentionally small. Treat safe points as "
         "commit gates; conversation and planning continuity should stay "
         "natural inside this Codex thread.\n\n"
      << "Return only JSON matching the Marshal decision contract already "
         "established for this session.\n\n"
      << "Session delta:\n"
      << "- marshal_session_id: " << loop.marshal_session_id << "\n"
      << "- checkpoint_index: " << checkpoint_index << "\n"
      << "- checkpoint_kind: " << trim_ascii(checkpoint_kind) << "\n"
      << "- current_thread_id: " << trim_ascii(loop.current_thread_id) << "\n"
      << "- lifecycle: " << loop.lifecycle << "\n"
      << "- work_gate: " << loop.work_gate << "\n"
      << "- activity: " << loop.activity << "\n"
      << "- campaign_status: " << loop.campaign_status << "\n"
      << "- campaign_cursor: " << trim_ascii(loop.campaign_cursor) << "\n"
      << "- last_codex_action: " << trim_ascii(loop.last_codex_action) << "\n"
      << "- latest_input_checkpoint_path: " << latest_input_path.string()
      << "\n"
      << "- memory_path: " << loop.memory_path << "\n"
      << "- briefing_path: " << loop.briefing_path
      << " (bootstrap reference only; do not reread by default)\n\n";

  if (!trim_ascii(operator_message_json).empty()) {
    out << "Operator message delta:\n" << operator_message_json << "\n\n";
  }
  if (!trim_ascii(run_plan_progress_json).empty()) {
    out << "Run-plan progress delta:\n" << run_plan_progress_json << "\n\n";
  }
  if (!trim_ascii(campaign_json).empty()) {
    out << "Campaign delta:\n" << campaign_json << "\n\n";
  }

  if (!trim_ascii(input_checkpoint_text).empty()) {
    out << "Latest input checkpoint excerpt (bounded, not full history):\n"
        << bounded_text_excerpt(input_checkpoint_text,
                                kResumePromptInputCheckpointBytesCap)
        << "\n\n";
  }
  std::error_code ec{};
  std::filesystem::create_directories(prompt_path.parent_path(), ec);
  if (ec) {
    if (error)
      *error = "cannot create resume prompt directory: " +
               prompt_path.parent_path().string();
    return false;
  }
  return cuwacunu::hero::runtime::write_text_file_atomic(prompt_path, out.str(),
                                                         error);
}

[[nodiscard]] bool persist_attempted_checkpoint_state(
    const app_context_t &app,
    cuwacunu::hero::marshal::marshal_session_record_t *loop,
    std::uint64_t checkpoint_index,
    const marshal_checkpoint_decision_t &decision,
    std::string_view checkpoint_warning, std::string *error) {
  if (error)
    error->clear();
  if (!loop) {
    if (error)
      *error = "checkpoint bookkeeping loop pointer is null";
    return false;
  }

  loop->checkpoint_count = std::max(loop->checkpoint_count, checkpoint_index);
  loop->last_codex_action = decision.intent;
  loop->last_intent_checkpoint_path =
      cuwacunu::hero::marshal::marshal_session_intent_checkpoint_path(
          app.defaults.marshal_root, loop->marshal_session_id, checkpoint_index)
          .string();
  loop->last_warning.clear();
  append_warning_text(&loop->last_warning, checkpoint_warning);
  loop->updated_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  loop->finished_at_ms.reset();
  if (!write_marshal_session(app, *loop, error))
    return false;
  append_memory_note(*loop, checkpoint_index, decision.memory_note);
  return true;
}

[[nodiscard]] bool park_session_idle_after_checkpoint_failure(
    const app_context_t &app,
    cuwacunu::hero::marshal::marshal_session_record_t *loop,
    std::uint64_t checkpoint_index, std::string_view failure_detail,
    std::string *error) {
  if (error)
    error->clear();
  if (!loop) {
    if (error)
      *error = "checkpoint failure loop pointer is null";
    return false;
  }

  const std::string trimmed_detail = trim_ascii(failure_detail);
  std::ostringstream detail;
  detail << "planning checkpoint " << checkpoint_index
         << " failed after intent capture";
  if (!trimmed_detail.empty()) {
    detail << ": " << trimmed_detail;
  }
  detail << "; session parked for review so the operator can message it after "
            "adjusting objective files or tooling";

  loop->lifecycle = "live";
  loop->activity = "review";
  loop->status_detail = detail.str();
  loop->work_gate = "open";
  loop->finish_reason = "failed";
  loop->finished_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  loop->updated_at_ms = *loop->finished_at_ms;
  clear_marshal_session_campaign_linkage(loop);
  append_warning_text(&loop->last_warning,
                      "checkpoint parked for review after intent capture; use "
                      "message_session once the underlying issue is addressed");
  if (!write_marshal_session(app, *loop, error))
    return false;
  std::string ignored{};
  (void)append_marshal_session_event(&loop, "checkpoint.failed",
                                     loop->status_detail, &ignored);
  return true;
}

[[nodiscard]] bool
collect_objective_hash_snapshot(const std::filesystem::path &objective_root,
                                objective_hash_snapshot_t *out,
                                std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "objective hash snapshot output pointer is null";
    return false;
  }
  out->clear();
  if (objective_root.empty()) {
    if (error)
      *error = "objective root is empty";
    return false;
  }

  std::error_code ec{};
  if (!std::filesystem::exists(objective_root, ec) ||
      !std::filesystem::is_directory(objective_root, ec)) {
    if (error) {
      *error = "objective root does not exist or is not a directory: " +
               objective_root.string();
    }
    return false;
  }

  std::vector<std::filesystem::path> files{};
  for (std::filesystem::recursive_directory_iterator it(objective_root, ec),
       end;
       it != end; it.increment(ec)) {
    if (ec) {
      if (error) {
        *error = "cannot walk objective root: " + objective_root.string();
      }
      return false;
    }
    if (!it->is_regular_file(ec) || ec) {
      ec.clear();
      continue;
    }
    files.push_back(it->path().lexically_normal());
  }
  std::sort(files.begin(), files.end());

  for (const auto &file : files) {
    const std::filesystem::path relative =
        file.lexically_relative(objective_root);
    if (relative.empty())
      continue;
    std::string sha256_hex{};
    if (!cuwacunu::hero::human::sha256_hex_file(file, &sha256_hex, error)) {
      return false;
    }
    (*out)[relative.generic_string()] = std::move(sha256_hex);
  }
  return true;
}

[[nodiscard]] bool
collect_objective_bundle_snapshot(const std::filesystem::path &objective_root,
                                  objective_bundle_snapshot_t *out_snapshot,
                                  std::string *error) {
  if (error)
    error->clear();
  if (!out_snapshot) {
    if (error)
      *error = "objective bundle snapshot output pointer is null";
    return false;
  }
  *out_snapshot = objective_bundle_snapshot_t{};
  if (objective_root.empty()) {
    if (error)
      *error = "objective root is empty";
    return false;
  }

  std::error_code ec{};
  if (!std::filesystem::exists(objective_root, ec) ||
      !std::filesystem::is_directory(objective_root, ec)) {
    if (error) {
      *error = "objective root does not exist or is not a directory: " +
               objective_root.string();
    }
    return false;
  }

  std::vector<std::filesystem::path> files{};
  for (std::filesystem::recursive_directory_iterator it(objective_root, ec),
       end;
       it != end; it.increment(ec)) {
    if (ec) {
      if (error) {
        *error = "cannot walk objective root: " + objective_root.string();
      }
      return false;
    }
    if (!it->is_regular_file(ec) || ec) {
      ec.clear();
      continue;
    }
    files.push_back(it->path().lexically_normal());
  }
  std::sort(files.begin(), files.end());

  for (const auto &file : files) {
    const std::filesystem::path relative =
        file.lexically_relative(objective_root);
    if (relative.empty())
      continue;
    const std::string relative_text = relative.generic_string();
    if (!should_embed_objective_bundle_relative_path(relative_text))
      continue;

    std::string file_text{};
    if (!cuwacunu::hero::runtime::read_text_file(file, &file_text, error)) {
      return false;
    }
    const std::size_t file_bytes = file_text.size();
    if (!out_snapshot->entries.empty() &&
        out_snapshot->embedded_bytes + file_bytes >
            kEmbeddedObjectiveBundleBytesCap) {
      out_snapshot->truncated = true;
      ++out_snapshot->omitted_files;
      continue;
    }
    out_snapshot->embedded_bytes += file_bytes;
    out_snapshot->entries.push_back(
        objective_bundle_entry_t{relative_text, std::move(file_text)});
  }
  return true;
}

