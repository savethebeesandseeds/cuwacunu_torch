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

