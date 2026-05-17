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

