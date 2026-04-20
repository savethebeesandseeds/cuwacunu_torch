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

[[nodiscard]] std::string marshal_decision_schema_json() {
  return "{\"type\":\"object\",\"properties\":{"
         "\"intent\":{\"type\":\"string\",\"enum\":[\"reply_only\","
         "\"interrupt_campaign\",\"launch_campaign\","
         "\"pause_for_clarification\",\"request_governance\",\"complete\","
         "\"terminate\"]},"
         "\"reply_text\":{\"type\":\"string\"},"
         "\"launch\":{\"type\":[\"object\",\"null\"],\"properties\":{"
         "\"mode\":{\"type\":\"string\",\"enum\":[\"run_plan\",\"binding\"]},"
         "\"binding_id\":{\"type\":[\"string\",\"null\"]},"
         "\"reset_runtime_state\":{\"type\":\"boolean\"},"
         "\"requires_objective_mutation\":{\"type\":\"boolean\"}"
         "},\"required\":[\"mode\",\"binding_id\",\"reset_runtime_state\","
         "\"requires_objective_mutation\"],\"additionalProperties\":false},"
         "\"clarification_request\":{\"type\":[\"object\",\"null\"],"
         "\"properties\":{"
         "\"request\":{\"type\":\"string\"}"
         "},\"required\":[\"request\"],\"additionalProperties\":false},"
         "\"governance\":{\"type\":[\"object\",\"null\"],\"properties\":{"
         "\"kind\":{\"type\":\"string\",\"enum\":[\"authority_expansion\","
         "\"launch_budget_expansion\",\"policy_decision\"]},"
         "\"request\":{\"type\":\"string\"},"
         "\"delta\":{\"type\":[\"object\",\"null\"],\"properties\":{"
         "\"allow_default_write\":{\"type\":\"boolean\"},"
         "\"additional_campaign_launches\":{\"type\":\"integer\"}"
         "},\"required\":[\"allow_default_write\",\"additional_campaign_"
         "launches\"],\"additionalProperties\":false}"
         "},\"required\":[\"kind\",\"request\",\"delta\"],"
         "\"additionalProperties\":false},"
         "\"reason\":{\"type\":\"string\"},"
         "\"memory_note\":{\"type\":\"string\"}"
         "},\"required\":[\"intent\",\"launch\",\"clarification_request\","
         "\"governance\",\"reason\",\"memory_note\",\"reply_text\"],"
         "\"additionalProperties\":false}";
}

[[nodiscard]] std::string bool_text(bool value) {
  return value ? "true" : "false";
}

[[nodiscard]] std::string
describe_marshal_binary_state(const std::filesystem::path &binary_path,
                              std::uint64_t session_started_at_ms) {
  std::ostringstream out;
  out << "path=" << binary_path.string();

  std::error_code exists_ec{};
  const bool exists = std::filesystem::exists(binary_path, exists_ec);
  out << " exists=" << bool_text(exists && !exists_ec);
  if (exists_ec || !exists) {
    out << " executable=unknown rebuilt_after_session_start=unknown";
    return out.str();
  }

  out << " executable=" << bool_text(::access(binary_path.c_str(), X_OK) == 0);

  std::error_code time_ec{};
  const auto mtime = std::filesystem::last_write_time(binary_path, time_ec);
  if (time_ec) {
    out << " rebuilt_after_session_start=unknown";
    return out.str();
  }

  const std::uint64_t now_ms = cuwacunu::hero::runtime::now_ms_utc();
  bool rebuilt_after_session_start = false;
  if (session_started_at_ms != 0 && session_started_at_ms <= now_ms) {
    const auto session_age =
        std::chrono::milliseconds(now_ms - session_started_at_ms);
    const auto session_start_on_file_clock =
        decltype(mtime)::clock::now() - session_age;
    rebuilt_after_session_start = mtime > session_start_on_file_clock;
  }
  out << " rebuilt_after_session_start="
      << bool_text(rebuilt_after_session_start);
  return out.str();
}

[[nodiscard]] std::string build_missing_mutation_diagnostic(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::uint64_t checkpoint_index) {
  std::ostringstream out;
  std::filesystem::path mutation_path{};
  const bool mutation_exists = has_recorded_mutation_checkpoint(
      app, loop, checkpoint_index, &mutation_path);
  out << "launch.requires_objective_mutation=true but no same-checkpoint "
         "objective mutation was materialized and no recorded mutation "
         "checkpoint is available; "
         "mutation_checkpoint_path="
      << mutation_path.string() << " exists=" << bool_text(mutation_exists)
      << "; config_hero_binary("
      << describe_marshal_binary_state(app.defaults.config_hero_binary,
                                       loop.started_at_ms)
      << "); inspect codex stderr "
      << cuwacunu::hero::marshal::marshal_session_codex_stderr_path(
             app.defaults.marshal_root, loop.marshal_session_id)
             .string()
      << " and codex stdout "
      << cuwacunu::hero::marshal::marshal_session_codex_stdout_path(
             app.defaults.marshal_root, loop.marshal_session_id)
             .string();
  return out.str();
}

[[nodiscard]] bool validate_marshal_checkpoint_intent_action(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::uint64_t checkpoint_index,
    const marshal_checkpoint_decision_t &decision,
    bool objective_mutation_materialized, std::string *error) {
  if (error)
    error->clear();
  if (decision.intent == "reply_only") {
    return true;
  }
  if (decision.intent == "interrupt_campaign") {
    if (!cuwacunu::hero::marshal::marshal_session_has_active_campaign(loop)) {
      if (error) {
        *error = "interrupt_campaign requires an active runtime campaign";
      }
      return false;
    }
    return true;
  }
  if (cuwacunu::hero::marshal::marshal_session_has_active_campaign(loop)) {
    if (error) {
      *error = "active runtime campaigns must use reply_only or "
               "interrupt_campaign from operator_message checkpoints";
    }
    return false;
  }
  if (decision.intent == "launch_campaign") {
    const bool mutation_checkpoint_exists =
        has_recorded_mutation_checkpoint(app, loop, checkpoint_index);
    if (decision.launch_requires_objective_mutation &&
        !objective_mutation_materialized && !mutation_checkpoint_exists) {
      if (error) {
        *error = build_missing_mutation_diagnostic(app, loop, checkpoint_index);
      }
      return false;
    }
    if (decision.launch_mode == "run_plan") {
      return true;
    }
    if (decision.launch_mode == "binding") {
      cuwacunu::camahjucunu::iitepi_campaign_instruction_t instruction{};
      if (!decode_campaign_snapshot(app, loop.campaign_dsl_path, &instruction,
                                    error)) {
        return false;
      }
      const auto bind_it =
          std::find_if(instruction.binds.begin(), instruction.binds.end(),
                       [&](const auto &bind) {
                         return bind.id == decision.launch_binding_id;
                       });
      if (bind_it == instruction.binds.end()) {
        if (error) {
          *error = "launch outcome binding_id is not declared in campaign: " +
                   decision.launch_binding_id;
        }
        return false;
      }
      return true;
    }
    if (error) {
      *error = "unsupported launch.mode: " + decision.launch_mode;
    }
    return false;
  }
  if (decision.intent == "pause_for_clarification") {
    if (trim_ascii(decision.clarification_request).empty()) {
      if (error) {
        *error =
            "pause_for_clarification requires clarification_request.request";
      }
      return false;
    }
    return true;
  }
  if (decision.intent == "request_governance") {
    if (decision.governance_kind != "authority_expansion" &&
        decision.governance_kind != "launch_budget_expansion" &&
        decision.governance_kind != "policy_decision") {
      if (error) {
        *error = "unsupported governance.kind: " + decision.governance_kind;
      }
      return false;
    }
    if (trim_ascii(decision.governance_request).empty()) {
      if (error)
        *error = "request_governance requires governance.request";
      return false;
    }
    if (decision.governance_kind == "authority_expansion" &&
        !decision.governance_allow_default_write) {
      if (error) {
        *error = "authority_expansion must request allow_default_write";
      }
      return false;
    }
    if (decision.governance_kind == "launch_budget_expansion" &&
        decision.governance_additional_campaign_launches == 0) {
      if (error) {
        *error =
            "launch_budget_expansion requires a positive campaign-launch delta";
      }
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool
parse_marshal_checkpoint_decision(const std::string &json,
                                  marshal_checkpoint_decision_t *out,
                                  std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "decision output pointer is null";
    return false;
  }
  *out = marshal_checkpoint_decision_t{};
  if (!extract_json_string_field(json, "reason", &out->reason)) {
    if (error)
      *error = "intent JSON missing required reason";
    return false;
  }
  if (!extract_json_string_field(json, "reply_text", &out->reply_text)) {
    if (error)
      *error = "intent JSON missing required reply_text";
    return false;
  }
  if (!extract_json_string_field(json, "intent", &out->intent)) {
    if (error)
      *error = "intent JSON missing required intent";
    return false;
  }
  std::string launch_json{};
  if (extract_json_object_field(json, "launch", &launch_json)) {
    (void)extract_json_string_field(launch_json, "mode", &out->launch_mode);
    (void)extract_json_string_field(launch_json, "binding_id",
                                    &out->launch_binding_id);
    if (extract_json_field_raw(launch_json, "reset_runtime_state", nullptr) &&
        !extract_json_bool_field(launch_json, "reset_runtime_state",
                                 &out->launch_reset_runtime_state)) {
      if (error) {
        *error = "launch.reset_runtime_state must be boolean when present";
      }
      return false;
    }
    if (extract_json_field_raw(launch_json, "requires_objective_mutation",
                               nullptr) &&
        !extract_json_bool_field(launch_json, "requires_objective_mutation",
                                 &out->launch_requires_objective_mutation)) {
      if (error) {
        *error =
            "launch.requires_objective_mutation must be boolean when present";
      }
      return false;
    }
  }
  (void)extract_json_string_field(json, "memory_note", &out->memory_note);
  std::string clarification_request_json{};
  if (extract_json_object_field(json, "clarification_request",
                                &clarification_request_json)) {
    (void)extract_json_string_field(clarification_request_json, "request",
                                    &out->clarification_request);
  }
  std::string governance_json{};
  if (extract_json_object_field(json, "governance", &governance_json)) {
    (void)extract_json_string_field(governance_json, "kind",
                                    &out->governance_kind);
    (void)extract_json_string_field(governance_json, "request",
                                    &out->governance_request);
    std::string delta_json{};
    if (extract_json_object_field(governance_json, "delta", &delta_json)) {
      (void)extract_json_bool_field(delta_json, "allow_default_write",
                                    &out->governance_allow_default_write);
      (void)extract_json_u64_field(
          delta_json, "additional_campaign_launches",
          &out->governance_additional_campaign_launches);
    }
  }
  out->intent = trim_ascii(out->intent);
  out->launch_mode = trim_ascii(out->launch_mode);
  out->launch_binding_id = trim_ascii(out->launch_binding_id);
  out->clarification_request = trim_ascii(out->clarification_request);
  out->governance_kind = trim_ascii(out->governance_kind);
  out->governance_request = trim_ascii(out->governance_request);
  out->memory_note = trim_ascii(out->memory_note);
  out->reply_text = trim_ascii(out->reply_text);
  if (out->intent != "reply_only" && out->intent != "interrupt_campaign" &&
      out->intent != "launch_campaign" &&
      out->intent != "pause_for_clarification" &&
      out->intent != "request_governance" && out->intent != "complete" &&
      out->intent != "terminate") {
    if (error)
      *error = "unsupported intent: " + out->intent;
    return false;
  }
  if (out->intent == "launch_campaign" && out->launch_mode.empty()) {
    if (error)
      *error = "launch_campaign requires launch.mode";
    return false;
  }
  if (!out->launch_mode.empty() && out->launch_mode != "run_plan" &&
      out->launch_mode != "binding") {
    if (error)
      *error = "unsupported launch.mode: " + out->launch_mode;
    return false;
  }
  if (out->launch_mode == "binding" && out->launch_binding_id.empty()) {
    if (error)
      *error = "launch.mode=binding requires launch.binding_id";
    return false;
  }
  if (out->intent != "launch_campaign") {
    out->launch_mode.clear();
    out->launch_binding_id.clear();
    out->launch_reset_runtime_state = false;
    out->launch_requires_objective_mutation = false;
  }
  if (out->intent != "pause_for_clarification") {
    out->clarification_request.clear();
  }
  if (out->intent != "request_governance") {
    out->governance_kind.clear();
    out->governance_request.clear();
    out->governance_allow_default_write = false;
    out->governance_additional_campaign_launches = 0;
  } else if (out->governance_kind.empty()) {
    if (error)
      *error = "request_governance requires governance.kind";
    return false;
  }
  return true;
}

[[nodiscard]] bool rewrite_marshal_session_briefing(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::string *error) {
  if (error)
    error->clear();
  std::string objective_dsl_text{};
  if (!cuwacunu::hero::runtime::read_text_file(loop.marshal_objective_dsl_path,
                                               &objective_dsl_text, error)) {
    return false;
  }
  std::string objective_md_text{};
  if (!cuwacunu::hero::runtime::read_text_file(loop.marshal_objective_md_path,
                                               &objective_md_text, error)) {
    return false;
  }
  std::string guidance_md_text{};
  if (!cuwacunu::hero::runtime::read_text_file(loop.marshal_guidance_md_path,
                                               &guidance_md_text, error)) {
    return false;
  }
  std::string hero_marshal_dsl_text{};
  if (!cuwacunu::hero::runtime::read_text_file(loop.hero_marshal_dsl_path,
                                               &hero_marshal_dsl_text, error)) {
    return false;
  }
  std::string memory_text{};
  {
    std::string ignored{};
    (void)cuwacunu::hero::runtime::read_text_file(loop.memory_path,
                                                  &memory_text, &ignored);
  }
  std::string input_checkpoint_text{};
  {
    std::string ignored{};
    (void)cuwacunu::hero::runtime::read_text_file(
        cuwacunu::hero::marshal::marshal_session_latest_input_checkpoint_path(
            std::filesystem::path(loop.session_root).parent_path(),
            loop.marshal_session_id),
        &input_checkpoint_text, &ignored);
  }
  std::string checkpoint_kind{};
  std::string prior_input_checkpoint_text{};
  std::string human_request_text{};
  std::string human_resolution_text{};
  if (!trim_ascii(input_checkpoint_text).empty()) {
    (void)extract_json_string_field(input_checkpoint_text, "checkpoint_kind",
                                    &checkpoint_kind);
    if (trim_ascii(checkpoint_kind) == "governance_followup") {
      std::string prior_input_checkpoint_path{};
      if (extract_json_string_field(input_checkpoint_text,
                                    "prior_input_checkpoint_path",
                                    &prior_input_checkpoint_path) &&
          !trim_ascii(prior_input_checkpoint_path).empty()) {
        std::string ignored{};
        (void)cuwacunu::hero::runtime::read_text_file(
            prior_input_checkpoint_path, &prior_input_checkpoint_text,
            &ignored);
      }
      std::string human_resolution_path{};
      if (extract_json_string_field(input_checkpoint_text,
                                    "governance_resolution_path",
                                    &human_resolution_path) &&
          !trim_ascii(human_resolution_path).empty()) {
        std::string ignored{};
        (void)cuwacunu::hero::runtime::read_text_file(
            human_resolution_path, &human_resolution_text, &ignored);
      }
    }
  }
  {
    std::string ignored{};
    (void)cuwacunu::hero::runtime::read_text_file(
        loop.human_request_path, &human_request_text, &ignored);
  }
  std::string campaign_text{};
  {
    std::string ignored{};
    (void)cuwacunu::hero::runtime::read_text_file(loop.campaign_dsl_path,
                                                  &campaign_text, &ignored);
  }
  objective_bundle_snapshot_t objective_bundle_snapshot{};
  {
    std::string ignored{};
    (void)collect_objective_bundle_snapshot(
        std::filesystem::path(loop.objective_root), &objective_bundle_snapshot,
        &ignored);
  }
  std::string objective_campaign_relative_path =
      std::filesystem::path(loop.campaign_dsl_path).filename().string();
  {
    const std::filesystem::path relative_path =
        std::filesystem::path(loop.campaign_dsl_path)
            .lexically_relative(std::filesystem::path(loop.objective_root));
    if (!relative_path.empty() && !relative_path.is_absolute()) {
      const std::string relative_text = relative_path.string();
      if (!relative_text.empty() && relative_text.rfind("..", 0) != 0) {
        objective_campaign_relative_path = relative_text;
      }
    }
  }
  std::ostringstream out;
  out << "You are planning inside a Marshal Hero session.\n\n"
      << "Sovereignty:\n"
      << "- Marshal Hero owns the session ledger and autonomous checkpoint "
         "orchestration.\n"
      << "- Runtime Hero executes campaigns only.\n"
      << "- Config Hero is the only writer for objective/default file "
         "changes.\n"
      << "- Hashimyei and Lattice are read-only evidence surfaces in this "
         "session.\n\n"
      << "Primary files:\n"
      << "- Marshal objective DSL: " << loop.marshal_objective_dsl_path << "\n"
      << "- Marshal objective markdown: " << loop.marshal_objective_md_path
      << "\n"
      << "- Marshal guidance markdown: " << loop.marshal_guidance_md_path
      << "\n"
      << "- Marshal session settings DSL: " << loop.hero_marshal_dsl_path
      << "\n"
      << "- Marshal Hero session manifest: "
      << cuwacunu::hero::marshal::marshal_session_manifest_path(
             app.defaults.marshal_root, loop.marshal_session_id)
             .string()
      << "\n"
      << "- Config Hero policy: " << loop.config_policy_path << "\n"
      << "- Memory: " << loop.memory_path << "\n"
      << "- Latest input checkpoint: "
      << marshal_session_latest_input_checkpoint_path(loop).string() << "\n"
      << "- Human request artifact: " << loop.human_request_path << "\n"
      << "- Mutable objective root: " << loop.objective_root << "\n"
      << "- Objective campaign file for hero.config.objective.*: "
      << objective_campaign_relative_path << "\n\n"
      << "The input checkpoint includes checkpoint_kind = bootstrap, "
         "postcampaign, governance_followup, clarification_followup, or "
         "operator_message.\n"
      << "Bootstrap launch guidance:\n"
      << "- If checkpoint_kind = bootstrap and the input checkpoint already "
         "provides concrete default_run_bind_ids plus a concrete "
         "objective-local campaign, prefer intent=launch_campaign with "
         "launch.mode=run_plan unless a real blocker or required "
         "same-checkpoint mutation is evident. Marshal now advances only the "
         "next pending RUN bind for launch.mode=run_plan and then replans "
         "before later RUN steps.\n"
      << "- Do not spend bootstrap tool calls on broad Lattice discovery when "
         "no new campaign evidence exists yet.\n\n"
      << "Interpret the authored markdown in this order:\n"
      << "- objective markdown = what the session is trying to achieve\n"
      << "- guidance markdown = authored boundaries plus advisory heuristics; "
         "prefer stronger evidence when the guidance is not a hard rule\n\n"
      << "Available MCP tools in this planning checkpoint:\n"
      << "- hero.config.default.list\n"
      << "- hero.config.default.read\n"
      << "- hero.config.default.create\n"
      << "- hero.config.default.replace\n"
      << "- hero.config.default.delete\n"
      << "- hero.config.temp.list\n"
      << "- hero.config.temp.read\n"
      << "- hero.config.temp.create\n"
      << "- hero.config.temp.replace\n"
      << "- hero.config.temp.delete\n"
      << "- hero.config.objective.list\n"
      << "- hero.config.objective.read\n"
      << "- hero.config.objective.create\n"
      << "- hero.config.objective.replace\n"
      << "- hero.config.objective.delete\n"
      << "- hero.runtime.get_campaign\n"
      << "- hero.runtime.get_job\n"
      << "- hero.runtime.list_jobs\n"
      << "- hero.runtime.tail_log\n"
      << "- hero.runtime.tail_trace\n"
      << "- hero.lattice.list_facts\n"
      << "- hero.lattice.get_fact\n"
      << "- hero.lattice.list_views\n"
      << "- hero.lattice.get_view\n\n"
      << "Rules:\n"
      << "1. Work in read-only shell mode. Do not edit files directly.\n"
      << "2. Do not use hero.hashimyei.* tools in this planning checkpoint; "
         "prefer input-checkpoint evidence plus hero.lattice.* queries.\n"
      << "3. Use Config Hero objective.read/create/replace/delete for "
         "truth-source objective files under objective_root.\n"
      << "4. Use Config Hero temp.read/create/replace/delete for temporary "
         "instruction files under temp_roots when scratch authored artifacts "
         "help planning.\n"
      << "5. Use Config Hero default.read/create/replace/delete only when a "
         "shared default truly needs to change.\n"
      << "6. Pass objective_root=" << loop.objective_root
      << " to those Config Hero tools.\n"
      << "7. Use hero.config.objective.list when you need the exact relative "
         "path under objective_root; do not assume generic names like "
         "campaign.dsl.\n"
      << "8. In this session, the objective-local campaign file path for "
         "hero.config.objective.* is "
      << objective_campaign_relative_path << ".\n"
      << "9. Prefer whole-file replace with expected_sha256 from the prior "
         "read.\n"
      << "10. Never mutate files outside the configured objective/default/temp "
         "roots.\n"
      << "11. Prefer the input checkpoint first. On bootstrap checkpoints with "
         "concrete default_run_bind_ids and no fresh campaign evidence, prefer "
         "immediate launch over exploratory evidence gathering.\n"
      << "11b. Read run_plan_progress first when present. For "
         "launch.mode=run_plan, Marshal will launch only "
         "run_plan_progress.next_pending_bind_id and then return for another "
         "planning checkpoint after that bind's campaign terminates.\n"
      << "12. Use hero.lattice.list_views/list_facts/get_view/get_fact mainly "
         "after at least one campaign has produced evidence, or when debugging "
         "a specific preexisting report; family_evaluation_report requires a "
         "family canonical_path plus contract_hash.\n"
      << "13. Use Runtime get/tail tools mainly for operational debugging such "
         "as launch failures, missing logs, or abnormal traces.\n"
      << "13b. Treat raw Runtime logs as artifacts, not session context. "
         "Prefer "
         "Lattice facts/views and Runtime paths/status summaries; if you must "
         "tail logs, request a narrow bounded excerpt and do not repeat the "
         "same "
         "raw output across turns.\n"
      << "14. Shell exec is unavailable in this environment. Prefer the "
         "embedded input checkpoint, memory, and objective bundle snapshot "
         "below; use MCP tools instead of shell reads.\n"
      << "15. If you change any objective, temp, or campaign DSL, describe the "
         "actual "
         "changes in memory_note.\n"
      << "16. Request governance only for authority_expansion, "
         "launch_budget_expansion, or policy_decision.\n"
      << "17. When checkpoint_kind = governance_followup, use the prior input "
         "checkpoint as the operational evidence base and the verified "
         "governance resolution as operator context.\n"
      << "18. A verified governance resolution with resolution_kind = grant or "
         "clarify authorizes more reasoning; it does not directly choose the "
         "next launch.\n"
      << "19. Use the embedded objective bundle snapshot below before spending "
         "tool calls rediscovering editable files.\n"
      << "20. Return only JSON matching the provided output schema.\n"
      << "21. intent=complete means the objective is satisfied for now and the "
         "session should park review-ready until a future operator message "
         "arrives.\n"
      << "22. For checkpoint_kind=operator_message, reply_text must be the "
         "direct operator-facing answer for this turn. For non-operator "
         "checkpoints, reply_text should be the empty string.\n"
      << "23. When a runtime campaign is active, use reply_only to answer or "
         "update future strategy without changing the active run, or use "
         "interrupt_campaign when the current run must stop before the next "
         "plan is committed.\n"
      << "24. When returning intent=complete or intent=terminate, make "
         "reason a concise report of affairs for the operator: state the "
         "objective outcome, the evidence or campaigns considered, any "
         "truth-source changes made, and the remaining risks or next steps. "
         "Do not make the operator read debug files for the core "
         "conclusion.\n\n"
      << "Intent contract:\n"
      << "- intent = reply_only | interrupt_campaign | launch_campaign | "
         "pause_for_clarification | request_governance | complete | "
         "terminate\n"
      << "- reply_text = direct operator answer for operator_message "
         "checkpoints, otherwise empty string\n"
      << "- reason = concise operator-facing explanation; for complete or "
         "terminate, this is the short report of affairs persisted into "
         "human/summary.latest.md\n"
      << "- intent=reply_only means answer now and keep the current Marshal "
         "state unchanged aside from any same-turn objective edits\n"
      << "- intent=interrupt_campaign means request a stop for the retained "
         "active runtime campaign and let Marshal resume ordinary "
         "post-campaign "
         "replanning after that campaign becomes terminal\n"
      << "- intent=complete parks the session as idle rather than hard-closing "
         "it\n"
      << "- launch.mode = run_plan | binding when intent=launch_campaign; "
         "run_plan advances only the next pending RUN bind from the current "
         "objective-local campaign sequence\n"
      << "- launch.binding_id required when launch.mode=binding\n"
      << "- launch.reset_runtime_state must be boolean when "
         "intent=launch_campaign\n"
      << "- launch.requires_objective_mutation must be boolean when "
         "intent=launch_campaign; set it true when the launch depends on "
         "same-checkpoint objective-root file edits\n"
      << "- clarification_request.request must be non-empty when "
         "intent=pause_for_clarification\n"
      << "- governance.kind = authority_expansion | launch_budget_expansion | "
         "policy_decision when intent=request_governance\n"
      << "- governance.request must be non-empty when "
         "intent=request_governance\n"
      << "- governance.delta carries the requested authority/budget change "
         "when applicable\n\n"
      << "Repo root: " << app.defaults.repo_root.string() << "\n";
  if (!trim_ascii(objective_dsl_text).empty()) {
    out << "\nMarshal objective DSL contents:\n" << objective_dsl_text;
    if (objective_dsl_text.back() != '\n')
      out << "\n";
  }
  if (!trim_ascii(objective_md_text).empty()) {
    out << "\nMarshal objective markdown contents:\n" << objective_md_text;
    if (objective_md_text.back() != '\n')
      out << "\n";
  }
  if (!trim_ascii(guidance_md_text).empty()) {
    out << "\nMarshal guidance markdown contents:\n" << guidance_md_text;
    if (guidance_md_text.back() != '\n')
      out << "\n";
  }
  if (!trim_ascii(hero_marshal_dsl_text).empty()) {
    out << "\nMarshal session settings DSL contents:\n"
        << hero_marshal_dsl_text;
    if (hero_marshal_dsl_text.back() != '\n')
      out << "\n";
  }
  if (!trim_ascii(memory_text).empty()) {
    out << "\nMemory contents:\n" << memory_text;
    if (memory_text.back() != '\n')
      out << "\n";
  }
  if (!trim_ascii(input_checkpoint_text).empty()) {
    out << "\nInput checkpoint contents:\n" << input_checkpoint_text;
    if (input_checkpoint_text.back() != '\n')
      out << "\n";
  }
  if (!trim_ascii(prior_input_checkpoint_text).empty()) {
    out << "\nPrior input checkpoint contents:\n"
        << prior_input_checkpoint_text;
    if (prior_input_checkpoint_text.back() != '\n')
      out << "\n";
  }
  if (!trim_ascii(human_request_text).empty()) {
    out << "\nHuman request contents:\n" << human_request_text;
    if (human_request_text.back() != '\n')
      out << "\n";
  }
  if (!trim_ascii(human_resolution_text).empty()) {
    out << "\nVerified human resolution contents:\n" << human_resolution_text;
    if (human_resolution_text.back() != '\n')
      out << "\n";
  }
  if (!trim_ascii(campaign_text).empty()) {
    out << "\nObjective campaign DSL contents ("
        << objective_campaign_relative_path << "):\n"
        << campaign_text;
    if (campaign_text.back() != '\n')
      out << "\n";
  }
  if (!objective_bundle_snapshot.entries.empty()) {
    out << "\nObjective-local mutable bundle index:\n";
    for (const auto &entry : objective_bundle_snapshot.entries) {
      out << "- " << entry.relative_path << " (" << entry.text.size()
          << " bytes)\n";
    }
    if (objective_bundle_snapshot.truncated) {
      out << "- ... truncated after "
          << objective_bundle_snapshot.embedded_bytes
          << " embedded bytes; omitted files: "
          << objective_bundle_snapshot.omitted_files << "\n";
    }
    for (const auto &entry : objective_bundle_snapshot.entries) {
      out << "\nObjective-local file contents (" << entry.relative_path
          << "):\n"
          << entry.text;
      if (!entry.text.empty() && entry.text.back() != '\n')
        out << "\n";
    }
  }
  return cuwacunu::hero::runtime::write_text_file_atomic(loop.briefing_path,
                                                         out.str(), error);
}

[[nodiscard]] bool run_command_with_stdio_and_timeout(
    const std::vector<std::string> &argv,
    const std::filesystem::path &stdin_path,
    const std::filesystem::path &stdout_path,
    const std::filesystem::path &stderr_path,
    const std::filesystem::path *working_dir,
    const std::vector<std::pair<std::string, std::string>> *env_overrides,
    std::size_t timeout_sec, const std::filesystem::path *pid_path,
    int *out_exit_code, std::string *error) {
  if (error)
    error->clear();
  if (argv.empty()) {
    if (error)
      *error = "command argv is empty";
    return false;
  }
  if (out_exit_code)
    *out_exit_code = -1;
  if (pid_path != nullptr && !pid_path->empty())
    remove_file_noexcept(*pid_path);

  std::error_code ec{};
  if (!std::filesystem::exists(stdin_path, ec) ||
      !std::filesystem::is_regular_file(stdin_path, ec)) {
    if (error)
      *error = "command stdin path does not exist: " + stdin_path.string();
    return false;
  }
  if (!stdout_path.empty()) {
    const int stdout_probe =
        ::open(stdout_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (stdout_probe < 0) {
      if (error) {
        *error = "cannot open stdout path for command: " + stdout_path.string();
      }
      return false;
    }
    (void)::close(stdout_probe);
  }
  if (stderr_path.empty()) {
    if (error)
      *error = "command stderr path is empty";
    return false;
  }
  const int stderr_probe =
      ::open(stderr_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (stderr_probe < 0) {
    if (error)
      *error = "cannot open stderr path for command: " + stderr_path.string();
    return false;
  }
  (void)::close(stderr_probe);

  int pipe_fds[2]{-1, -1};
#ifdef O_CLOEXEC
  if (::pipe2(pipe_fds, O_CLOEXEC) != 0) {
    if (error)
      *error = "pipe2 failed for command execution";
    return false;
  }
#else
  if (::pipe(pipe_fds) != 0) {
    if (error)
      *error = "pipe failed for command execution";
    return false;
  }
  (void)::fcntl(pipe_fds[0], F_SETFD, FD_CLOEXEC);
  (void)::fcntl(pipe_fds[1], F_SETFD, FD_CLOEXEC);
#endif

  const pid_t child = ::fork();
  if (child < 0) {
    (void)::close(pipe_fds[0]);
    (void)::close(pipe_fds[1]);
    if (error)
      *error = "fork failed for command execution";
    return false;
  }
  if (child == 0) {
    (void)::close(pipe_fds[0]);
    (void)::setpgid(0, 0);
    if (working_dir != nullptr && !working_dir->empty() &&
        ::chdir(working_dir->c_str()) != 0) {
      write_command_child_failure_noexcept(
          pipe_fds[1], command_child_failure_stage_t::kChdir, errno);
      _exit(125);
    }
    if (env_overrides != nullptr) {
      for (const auto &[key, value] : *env_overrides) {
        if (key.empty())
          continue;
        if (::setenv(key.c_str(), value.c_str(), 1) != 0) {
          write_command_child_failure_noexcept(
              pipe_fds[1], command_child_failure_stage_t::kSetenv, errno);
          _exit(125);
        }
      }
    }
    const int stdin_fd = ::open(stdin_path.c_str(), O_RDONLY);
    if (stdin_fd < 0) {
      write_command_child_failure_noexcept(
          pipe_fds[1], command_child_failure_stage_t::kStdinOpen, errno);
      _exit(126);
    }
    (void)::dup2(stdin_fd, STDIN_FILENO);
    if (stdin_fd > STDERR_FILENO)
      (void)::close(stdin_fd);
    const char *stdout_target =
        stdout_path.empty() ? "/dev/null" : stdout_path.c_str();
    const int stdout_fd =
        ::open(stdout_target, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (stdout_fd < 0) {
      write_command_child_failure_noexcept(
          pipe_fds[1], command_child_failure_stage_t::kStdoutOpen, errno);
      _exit(126);
    }
    (void)::dup2(stdout_fd, STDOUT_FILENO);
    if (stdout_fd > STDERR_FILENO)
      (void)::close(stdout_fd);
    const int stderr_fd =
        ::open(stderr_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (stderr_fd < 0) {
      write_command_child_failure_noexcept(
          pipe_fds[1], command_child_failure_stage_t::kStderrOpen, errno);
      _exit(126);
    }
    (void)::dup2(stderr_fd, STDERR_FILENO);
    if (stderr_fd > STDERR_FILENO)
      (void)::close(stderr_fd);
    std::vector<char *> exec_argv{};
    exec_argv.reserve(argv.size() + 1);
    for (const std::string &arg : argv) {
      exec_argv.push_back(const_cast<char *>(arg.c_str()));
    }
    exec_argv.push_back(nullptr);
    ::execvp(exec_argv[0], exec_argv.data());
    write_command_child_failure_noexcept(
        pipe_fds[1], command_child_failure_stage_t::kExecvp, errno);
    _exit(127);
  }
  (void)::close(pipe_fds[1]);

  if (pid_path != nullptr && !pid_path->empty()) {
    std::string pid_error{};
    if (!cuwacunu::hero::runtime::write_text_file_atomic(
            *pid_path, std::to_string(static_cast<long long>(child)),
            &pid_error)) {
      (void)::kill(-child, SIGKILL);
      (void)::kill(child, SIGKILL);
      (void)::waitpid(child, nullptr, 0);
      (void)::close(pipe_fds[0]);
      if (error) {
        *error = "cannot persist checkpoint pid path " + pid_path->string() +
                 ": " + pid_error;
      }
      return false;
    }
  }

  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(timeout_sec);
  int status = 0;
  for (;;) {
    const pid_t waited = ::waitpid(child, &status, WNOHANG);
    if (waited == child)
      break;
    if (waited < 0 && errno != EINTR) {
      if (error)
        *error = "waitpid failed for command execution";
      (void)::kill(-child, SIGKILL);
      (void)::waitpid(child, nullptr, 0);
      (void)::close(pipe_fds[0]);
      if (pid_path != nullptr && !pid_path->empty())
        remove_file_noexcept(*pid_path);
      return false;
    }
    if (std::chrono::steady_clock::now() >= deadline) {
      (void)::kill(-child, SIGKILL);
      (void)::kill(child, SIGKILL);
      (void)::waitpid(child, nullptr, 0);
      if (error)
        *error = "command timed out";
      (void)::close(pipe_fds[0]);
      if (pid_path != nullptr && !pid_path->empty())
        remove_file_noexcept(*pid_path);
      return false;
    }
    ::usleep(100 * 1000);
  }

  if (pid_path != nullptr && !pid_path->empty())
    remove_file_noexcept(*pid_path);
  if (WIFEXITED(status) && out_exit_code)
    *out_exit_code = WEXITSTATUS(status);

  command_child_failure_t child_failure{};
  const ssize_t failure_bytes =
      ::read(pipe_fds[0], &child_failure, sizeof(child_failure));
  (void)::close(pipe_fds[0]);
  if (failure_bytes == static_cast<ssize_t>(sizeof(child_failure)) &&
      child_failure.err != 0 &&
      child_failure.stage != command_child_failure_stage_t::kNone) {
    if (error) {
      *error = describe_command_child_failure(child_failure, argv, working_dir);
    }
    return false;
  }

  if (WIFEXITED(status)) {
    return true;
  }
  if (WIFSIGNALED(status)) {
    if (out_exit_code)
      *out_exit_code = 128 + WTERMSIG(status);
    if (error)
      *error = "command terminated by signal";
    return false;
  }
  if (error)
    *error = "command ended in unknown state";
  return false;
}

[[nodiscard]] bool build_marshal_postcampaign_input_checkpoint_json(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    const cuwacunu::hero::runtime::runtime_campaign_record_t &campaign,
    std::uint64_t checkpoint_index, std::string *out_json, std::string *error) {
  if (error)
    error->clear();
  if (!out_json) {
    if (error)
      *error = "postcampaign input checkpoint output pointer is null";
    return false;
  }
  out_json->clear();

  cuwacunu::camahjucunu::iitepi_campaign_instruction_t instruction{};
  if (!decode_campaign_snapshot(app, loop.campaign_dsl_path, &instruction,
                                error)) {
    return false;
  }
  marshal_session_workspace_context_t workspace_context{};
  if (!load_marshal_session_workspace_context(app, loop, &workspace_context,
                                              error)) {
    return false;
  }

  std::vector<std::string> declared_bind_ids{};
  declared_bind_ids.reserve(instruction.binds.size());
  for (const auto &bind : instruction.binds)
    declared_bind_ids.push_back(bind.id);
  std::vector<std::string> default_run_bind_ids{};
  default_run_bind_ids.reserve(instruction.runs.size());
  for (const auto &run : instruction.runs) {
    default_run_bind_ids.push_back(run.bind_ref);
  }
  marshal_run_plan_progress_t run_plan_progress{};
  if (!collect_marshal_run_plan_progress(app, loop, instruction, &campaign,
                                         &run_plan_progress, error)) {
    return false;
  }

  struct job_checkpoint_row_t {
    cuwacunu::hero::runtime::runtime_job_record_t record{};
    cuwacunu::hero::runtime::runtime_job_observation_t observation{};
    std::string stdout_tail{};
    std::string stderr_tail{};
    std::string trace_tail{};
    std::string contract_hash{};
    std::string wave_hash{};
  };
  std::vector<job_checkpoint_row_t> jobs{};
  jobs.reserve(campaign.job_cursors.size());
  for (const std::string &job_cursor : campaign.job_cursors) {
    cuwacunu::hero::runtime::runtime_job_record_t job{};
    if (!read_runtime_job_direct(app, job_cursor, &job, error))
      return false;
    job_checkpoint_row_t row{};
    row.record = job;
    row.observation = cuwacunu::hero::runtime::observe_runtime_job(job);
    (void)tail_file_lines(
        job.stdout_path,
        std::min<std::size_t>(40, workspace_context.tail_default_lines),
        &row.stdout_tail, nullptr);
    (void)tail_file_lines(
        job.stderr_path,
        std::min<std::size_t>(20, workspace_context.tail_default_lines),
        &row.stderr_tail, nullptr);
    (void)tail_file_lines(
        runtime_job_trace_path_for_record(app, job),
        std::min<std::size_t>(40, workspace_context.tail_default_lines),
        &row.trace_tail, nullptr);
    row.contract_hash =
        extract_logged_hash_value(row.stdout_tail, "contract_hash=");
    row.wave_hash = extract_logged_hash_value(row.stdout_tail, "wave_hash=");
    row.stdout_tail = bounded_sanitized_artifact_excerpt(
        row.stdout_tail, kMarshalCheckpointTailExcerptBytesCap);
    row.stderr_tail = bounded_sanitized_artifact_excerpt(
        row.stderr_tail, kMarshalCheckpointTailExcerptBytesCap);
    row.trace_tail = bounded_sanitized_artifact_excerpt(
        row.trace_tail, kMarshalCheckpointTailExcerptBytesCap);
    jobs.push_back(std::move(row));
  }

  const auto run_manifest_hints =
      collect_run_manifest_hints(app, declared_bind_ids, 2);
  std::vector<std::string> contract_hash_candidates{};
  for (const auto &row : jobs)
    contract_hash_candidates.push_back(row.contract_hash);
  for (const auto &hint : run_manifest_hints) {
    contract_hash_candidates.push_back(hint.contract_hash);
  }
  contract_hash_candidates =
      unique_nonempty_strings(std::move(contract_hash_candidates));

  std::ostringstream jobs_json;
  jobs_json << "[";
  for (std::size_t i = 0; i < jobs.size(); ++i) {
    if (i != 0)
      jobs_json << ",";
    jobs_json << "{"
              << "\"job\":"
              << runtime_job_summary_to_json(jobs[i].record,
                                             jobs[i].observation)
              << ",\"job_compact\":true"
              << ",\"stdout_tail\":" << json_quote(jobs[i].stdout_tail)
              << ",\"stderr_tail\":" << json_quote(jobs[i].stderr_tail)
              << ",\"trace_tail\":" << json_quote(jobs[i].trace_tail)
              << ",\"tail_excerpt_max_bytes\":"
              << kMarshalCheckpointTailExcerptBytesCap
              << ",\"contract_hash\":" << json_quote(jobs[i].contract_hash)
              << ",\"wave_hash\":" << json_quote(jobs[i].wave_hash) << "}";
  }
  jobs_json << "]";

  std::ostringstream manifests_json;
  manifests_json << "[";
  for (std::size_t i = 0; i < run_manifest_hints.size(); ++i) {
    if (i != 0)
      manifests_json << ",";
    manifests_json
        << "{"
        << "\"path\":" << json_quote(run_manifest_hints[i].path.string())
        << ",\"run_id\":" << json_quote(run_manifest_hints[i].run_id)
        << ",\"binding_id\":" << json_quote(run_manifest_hints[i].binding_id)
        << ",\"contract_hash\":"
        << json_quote(run_manifest_hints[i].contract_hash)
        << ",\"wave_hash\":" << json_quote(run_manifest_hints[i].wave_hash)
        << ",\"started_at_ms\":" << run_manifest_hints[i].started_at_ms << "}";
  }
  manifests_json << "]";

  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(kInputCheckpointSchemaV3) << ","
      << "\"checkpoint_kind\":\"postcampaign\","
      << "\"checkpoint_index\":" << checkpoint_index
      << ",\"session\":" << marshal_session_to_json(loop)
      << ",\"campaign\":" << runtime_campaign_to_json(campaign)
      << ",\"declared_bind_ids\":" << json_array_from_strings(declared_bind_ids)
      << ",\"default_run_bind_ids\":"
      << json_array_from_strings(default_run_bind_ids)
      << ",\"run_plan_progress\":"
      << marshal_run_plan_progress_to_json(run_plan_progress)
      << ",\"jobs\":" << jobs_json.str()
      << ",\"run_manifest_hints\":" << manifests_json.str()
      << ",\"lattice_recommendations\":"
      << build_lattice_recommendations_json(contract_hash_candidates)
      << ",\"memory_path\":" << json_quote(loop.memory_path)
      << ",\"human_request_path\":" << json_quote(loop.human_request_path)
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

[[nodiscard]] bool build_marshal_bootstrap_input_checkpoint_json(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    std::uint64_t checkpoint_index, std::string *out_json, std::string *error) {
  if (error)
    error->clear();
  if (!out_json) {
    if (error)
      *error = "bootstrap input checkpoint output pointer is null";
    return false;
  }
  out_json->clear();

  cuwacunu::camahjucunu::iitepi_campaign_instruction_t instruction{};
  if (!decode_campaign_snapshot(app, loop.campaign_dsl_path, &instruction,
                                error)) {
    return false;
  }

  std::vector<std::string> declared_bind_ids{};
  declared_bind_ids.reserve(instruction.binds.size());
  for (const auto &bind : instruction.binds)
    declared_bind_ids.push_back(bind.id);
  std::vector<std::string> default_run_bind_ids{};
  default_run_bind_ids.reserve(instruction.runs.size());
  for (const auto &run : instruction.runs) {
    default_run_bind_ids.push_back(run.bind_ref);
  }
  marshal_run_plan_progress_t run_plan_progress{};
  if (!collect_marshal_run_plan_progress(app, loop, instruction, nullptr,
                                         &run_plan_progress, error)) {
    return false;
  }

  const auto run_manifest_hints =
      collect_run_manifest_hints(app, declared_bind_ids, 2);
  std::vector<std::string> contract_hash_candidates{};
  for (const auto &hint : run_manifest_hints) {
    contract_hash_candidates.push_back(hint.contract_hash);
  }
  contract_hash_candidates =
      unique_nonempty_strings(std::move(contract_hash_candidates));

  std::ostringstream manifests_json;
  manifests_json << "[";
  for (std::size_t i = 0; i < run_manifest_hints.size(); ++i) {
    if (i != 0)
      manifests_json << ",";
    manifests_json
        << "{"
        << "\"path\":" << json_quote(run_manifest_hints[i].path.string())
        << ",\"run_id\":" << json_quote(run_manifest_hints[i].run_id)
        << ",\"binding_id\":" << json_quote(run_manifest_hints[i].binding_id)
        << ",\"contract_hash\":"
        << json_quote(run_manifest_hints[i].contract_hash)
        << ",\"wave_hash\":" << json_quote(run_manifest_hints[i].wave_hash)
        << ",\"started_at_ms\":" << run_manifest_hints[i].started_at_ms << "}";
  }
  manifests_json << "]";

  std::ostringstream out;
  out << "{"
      << "\"schema\":" << json_quote(kInputCheckpointSchemaV3) << ","
      << "\"checkpoint_kind\":\"bootstrap\","
      << "\"checkpoint_index\":" << checkpoint_index
      << ",\"session\":" << marshal_session_to_json(loop)
      << ",\"campaign\":null"
      << ",\"declared_bind_ids\":" << json_array_from_strings(declared_bind_ids)
      << ",\"default_run_bind_ids\":"
      << json_array_from_strings(default_run_bind_ids)
      << ",\"run_plan_progress\":"
      << marshal_run_plan_progress_to_json(run_plan_progress) << ",\"jobs\":[]"
      << ",\"run_manifest_hints\":" << manifests_json.str()
      << ",\"lattice_recommendations\":"
      << build_lattice_recommendations_json(contract_hash_candidates)
      << ",\"memory_path\":" << json_quote(loop.memory_path)
      << ",\"human_request_path\":" << json_quote(loop.human_request_path)
      << ",\"marshal_objective_dsl_path\":"
      << json_quote(loop.marshal_objective_dsl_path)
      << ",\"marshal_objective_md_path\":"
      << json_quote(loop.marshal_objective_md_path)
      << ",\"marshal_guidance_md_path\":"
      << json_quote(loop.marshal_guidance_md_path)
      << ",\"config_policy_path\":" << json_quote(loop.config_policy_path)
      << ",\"briefing_path\":" << json_quote(loop.briefing_path)
      << ",\"mutable_objective_root\":" << json_quote(loop.objective_root)
      << ",\"objective_campaign_dsl_path\":"
      << json_quote(loop.campaign_dsl_path)
      << ",\"objective_root\":" << json_quote(loop.objective_root)
      << ",\"campaigns_root\":"
      << json_quote(campaigns_root_for_app(app).string())
      << ",\"marshal_root\":" << json_quote(app.defaults.marshal_root.string())
      << ",\"launch_context\":{\"initial_launch\":true}"
      << "}";
  *out_json = out.str();
  return true;
}

