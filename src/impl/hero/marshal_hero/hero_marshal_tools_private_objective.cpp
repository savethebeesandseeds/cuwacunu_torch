[[nodiscard]] bool tail_file_lines(const std::filesystem::path &path,
                                   std::size_t lines, std::string *out,
                                   std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "tail output pointer is null";
    return false;
  }
  *out = "";
  std::error_code ec{};
  if (!std::filesystem::exists(path, ec)) {
    if (ec) {
      if (error)
        *error = "cannot access log file: " + path.string();
      return false;
    }
    return true;
  }
  std::string text{};
  if (!cuwacunu::hero::runtime::read_text_file(path, &text, error))
    return false;
  if (lines == 0) {
    *out = text;
    return true;
  }
  std::size_t count = 0;
  std::size_t pos = text.size();
  while (pos > 0) {
    --pos;
    if (text[pos] == '\n') {
      ++count;
      if (count > lines) {
        ++pos;
        break;
      }
    }
  }
  if (count <= lines)
    pos = 0;
  *out = text.substr(pos);
  return true;
}

[[nodiscard]] bool
read_kv_file(const std::filesystem::path &path,
             std::unordered_map<std::string, std::string> *out,
             std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "kv output pointer is null";
    return false;
  }
  out->clear();
  std::string text{};
  if (!cuwacunu::hero::runtime::read_text_file(path, &text, error))
    return false;
  std::istringstream in(text);
  std::string line{};
  while (std::getline(in, line)) {
    const std::string trimmed = trim_ascii(line);
    if (trimmed.empty())
      continue;
    const std::size_t eq = trimmed.find('=');
    if (eq == std::string::npos)
      continue;
    (*out)[trim_ascii(trimmed.substr(0, eq))] =
        trim_ascii(trimmed.substr(eq + 1));
  }
  return true;
}

[[nodiscard]] std::string extract_logged_hash_value(std::string_view text,
                                                    std::string_view key) {
  const std::size_t pos = text.rfind(key);
  if (pos == std::string::npos)
    return {};
  const std::size_t start = pos + key.size();
  std::size_t end = start;
  while (end < text.size()) {
    const char c = text[end];
    if (!(std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' ||
          c == '-' || c == '.')) {
      break;
    }
    ++end;
  }
  return std::string(text.substr(start, end - start));
}

[[nodiscard]] std::vector<std::string>
unique_nonempty_strings(std::vector<std::string> values) {
  for (std::string &value : values)
    value = trim_ascii(value);
  values.erase(
      std::remove_if(values.begin(), values.end(),
                     [](const std::string &value) { return value.empty(); }),
      values.end());
  std::sort(values.begin(), values.end());
  values.erase(std::unique(values.begin(), values.end()), values.end());
  return values;
}

[[nodiscard]] std::vector<run_manifest_hint_t>
collect_run_manifest_hints(const app_context_t &app,
                           const std::vector<std::string> &binding_ids,
                           std::size_t per_binding_limit) {
  const std::filesystem::path runtime_root =
      app.defaults.marshal_root.parent_path();
  const std::filesystem::path runs_root =
      runtime_root / ".hashimyei" / "_meta" / "runs";
  std::vector<run_manifest_hint_t> out{};
  std::error_code ec{};
  if (!std::filesystem::exists(runs_root, ec) || ec)
    return out;

  for (const std::string &binding_id : binding_ids) {
    std::vector<run_manifest_hint_t> binding_rows{};
    for (const auto &it : std::filesystem::directory_iterator(runs_root, ec)) {
      if (ec)
        break;
      if (!it.is_directory(ec) || ec)
        continue;
      const std::string dirname = it.path().filename().string();
      if (dirname.find("." + binding_id + ".") == std::string::npos)
        continue;
      const std::filesystem::path manifest = it.path() / "run.manifest.v2.kv";
      if (!std::filesystem::exists(manifest, ec) || ec)
        continue;
      std::unordered_map<std::string, std::string> kv{};
      std::string ignored{};
      if (!read_kv_file(manifest, &kv, &ignored))
        continue;
      run_manifest_hint_t hint{};
      hint.path = manifest;
      hint.run_id = kv["run_id"];
      hint.binding_id = kv["wave_contract_binding.binding_id"];
      hint.contract_hash = kv["wave_contract_binding.contract.hash_sha256_hex"];
      hint.wave_hash = kv["wave_contract_binding.wave.hash_sha256_hex"];
      (void)cuwacunu::hero::runtime::parse_u64(kv["started_at_ms"],
                                               &hint.started_at_ms);
      binding_rows.push_back(std::move(hint));
    }
    std::sort(binding_rows.begin(), binding_rows.end(),
              [](const auto &lhs, const auto &rhs) {
                if (lhs.started_at_ms != rhs.started_at_ms) {
                  return lhs.started_at_ms > rhs.started_at_ms;
                }
                return lhs.path.string() > rhs.path.string();
              });
    if (binding_rows.size() > per_binding_limit) {
      binding_rows.resize(per_binding_limit);
    }
    out.insert(out.end(), binding_rows.begin(), binding_rows.end());
  }
  return out;
}

[[nodiscard]] std::string build_lattice_recommendations_json(
    const std::vector<std::string> &contract_hash_candidates) {
  std::ostringstream view_queries_json;
  view_queries_json << "[";
  for (std::size_t i = 0; i < contract_hash_candidates.size(); ++i) {
    if (i != 0)
      view_queries_json << ",";
    view_queries_json
        << "{"
        << "\"tool\":\"hero.lattice.get_view\","
        << "\"arguments\":{"
        << "\"view_kind\":\"family_evaluation_report\","
        << "\"canonical_path\":" << json_quote("<family canonical_path>")
        << ",\"contract_hash\":" << json_quote(contract_hash_candidates[i])
        << "},"
        << "\"when_to_use\":"
        << json_quote("Use for semantic family-level evaluation evidence "
                      "once you know the family canonical_path selector.")
        << ",\"notes\":"
        << json_quote("family_evaluation_report requires a family "
                      "canonical_path selector plus contract_hash. If the "
                      "family selector is unknown, discover it with "
                      "hero.lattice.list_facts first.")
        << "}";
  }
  view_queries_json << "]";

  std::ostringstream out;
  out << "{"
      << "\"preferred_evidence_order\":[" << json_quote("input_checkpoint")
      << "," << json_quote("hero.lattice.get_view_or_get_fact") << ","
      << json_quote("hero.runtime_operational_debugging") << ","
      << json_quote("direct_file_reads_as_fallback") << "],"
      << "\"semantic_preference\":"
      << json_quote(
             "Prefer Lattice facts/views when judging report quality or "
             "comparing semantic evidence. Use Runtime tails primarily for "
             "operational debugging such as launch failures, missing logs, or "
             "abnormal traces.")
      << ",\"fact_discovery_queries\":["
      << "{"
      << "\"tool\":\"hero.lattice.list_views\","
      << "\"arguments\":{},"
      << "\"when_to_use\":"
      << json_quote(
             "Use when you need to confirm available derived view kinds and "
             "their selectors.")
      << "},"
      << "{"
      << "\"tool\":\"hero.lattice.list_facts\","
      << "\"arguments\":{},"
      << "\"when_to_use\":"
      << json_quote(
             "Use when you need to discover canonical_path selectors available "
             "in the current catalog before choosing one concrete fact/view "
             "query.")
      << "},"
      << "{"
      << "\"tool\":\"hero.lattice.get_fact\","
      << "\"arguments\":{\"canonical_path\":"
      << json_quote("<component canonical_path>") << "},"
      << "\"when_to_use\":"
      << json_quote(
             "Use after you know a component canonical_path and want one "
             "assembled fact bundle rather than raw report fragments.")
      << "}"
      << "],"
      << "\"family_evaluation_report_queries\":" << view_queries_json.str()
      << ",\"contract_hash_candidates\":"
      << json_array_from_strings(contract_hash_candidates)
      << ",\"selector_guidance\":"
      << json_quote(
             "For family_evaluation_report, canonical_path should be the "
             "family "
             "selector without a hashimyei suffix. If you do not already know "
             "that selector from the objective, use hero.lattice.list_facts to "
             "discover it.")
      << "}";
  return out.str();
}

[[nodiscard]] bool rewrite_campaign_imports_absolute(
    const std::filesystem::path &source_campaign_path,
    std::string_view input_text, std::string *out_text, std::string *error) {
  if (error)
    error->clear();
  if (!out_text) {
    if (error)
      *error = "campaign snapshot output pointer is null";
    return false;
  }
  *out_text = "";

  bool in_block_comment = false;
  std::ostringstream out;
  std::size_t pos = 0;
  bool first = true;
  while (pos <= input_text.size()) {
    const std::size_t end = input_text.find('\n', pos);
    std::string line = (end == std::string::npos)
                           ? std::string(input_text.substr(pos))
                           : std::string(input_text.substr(pos, end - pos));

    const auto rewrite_import = [&](std::string_view token,
                                    const std::string &current_line) {
      std::size_t token_pos = current_line.find(token);
      if (token_pos == std::string::npos)
        return current_line;
      const std::size_t quote_begin =
          current_line.find('"', token_pos + token.size());
      if (quote_begin == std::string::npos)
        return current_line;
      const std::size_t quote_end = current_line.find('"', quote_begin + 1);
      if (quote_end == std::string::npos)
        return current_line;
      const std::filesystem::path raw_path(
          current_line.substr(quote_begin + 1, quote_end - quote_begin - 1));
      if (raw_path.is_absolute())
        return current_line;
      const std::filesystem::path rewritten =
          (source_campaign_path.parent_path() / raw_path).lexically_normal();
      return current_line.substr(0, quote_begin + 1) + rewritten.string() +
             current_line.substr(quote_end);
    };

    if (!in_block_comment) {
      line = rewrite_import("IMPORT_CONTRACT", line);
      line = rewrite_import("FROM", line);
      line = rewrite_import("IMPORT_CONTRACT_FILE", line);
      line = rewrite_import("IMPORT_WAVE_FILE", line);
      line = rewrite_import("MARSHAL", line);
    }
    if (!first)
      out << "\n";
    first = false;
    out << line;
    in_block_comment = cuwacunu::hero::wave_contract_binding_runtime::detail::
        next_block_comment_state(line, in_block_comment);
    if (end == std::string::npos)
      break;
    pos = end + 1;
  }
  *out_text = out.str();
  return true;
}

[[nodiscard]] bool decode_campaign_snapshot(
    const app_context_t &app, const std::string &campaign_dsl_path,
    cuwacunu::camahjucunu::iitepi_campaign_instruction_t *out_instruction,
    std::string *error) {
  if (error)
    error->clear();
  if (!out_instruction) {
    if (error)
      *error = "campaign decode output pointer is null";
    return false;
  }
  std::string grammar_text{};
  if (!cuwacunu::hero::runtime::read_text_file(
          app.defaults.campaign_grammar_path, &grammar_text, error)) {
    return false;
  }
  std::string campaign_text{};
  if (!cuwacunu::hero::runtime::read_text_file(campaign_dsl_path,
                                               &campaign_text, error)) {
    return false;
  }
  try {
    *out_instruction =
        cuwacunu::camahjucunu::dsl::decode_iitepi_campaign_from_dsl(
            grammar_text, campaign_text);
  } catch (const std::exception &e) {
    if (error) {
      *error = "failed to decode campaign DSL snapshot '" + campaign_dsl_path +
               "': " + e.what();
    }
    return false;
  }
  return true;
}

[[nodiscard]] std::size_t completed_run_count_for_campaign(
    const cuwacunu::hero::runtime::runtime_campaign_record_t &campaign) {
  if (trim_ascii(campaign.state) == "exited")
    return campaign.run_bind_ids.size();
  if (!campaign.current_run_index.has_value())
    return 0;
  return std::min<std::size_t>(
      static_cast<std::size_t>(*campaign.current_run_index),
      campaign.run_bind_ids.size());
}

[[nodiscard]] std::size_t attempted_run_count_for_campaign(
    const cuwacunu::hero::runtime::runtime_campaign_record_t &campaign) {
  if (trim_ascii(campaign.state) == "exited")
    return campaign.run_bind_ids.size();
  if (!campaign.current_run_index.has_value())
    return 0;
  return std::min<std::size_t>(
      static_cast<std::size_t>(*campaign.current_run_index) + 1,
      campaign.run_bind_ids.size());
}

[[nodiscard]] bool collect_marshal_run_plan_progress(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &loop,
    const cuwacunu::camahjucunu::iitepi_campaign_instruction_t &instruction,
    const cuwacunu::hero::runtime::runtime_campaign_record_t *campaign_hint,
    marshal_run_plan_progress_t *out, std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "run-plan progress output pointer is null";
    return false;
  }
  *out = marshal_run_plan_progress_t{};

  out->declared_bind_ids.reserve(instruction.binds.size());
  for (const auto &bind : instruction.binds) {
    const std::string bind_id = trim_ascii(bind.id);
    if (!bind_id.empty())
      out->declared_bind_ids.push_back(bind_id);
  }
  out->default_run_bind_ids.reserve(instruction.runs.size());
  for (const auto &run : instruction.runs) {
    const std::string bind_id = trim_ascii(run.bind_ref);
    if (!bind_id.empty())
      out->default_run_bind_ids.push_back(bind_id);
  }

  const std::filesystem::path normalized_campaign_source =
      std::filesystem::path(loop.campaign_dsl_path).lexically_normal();
  for (const std::string &campaign_cursor : loop.campaign_cursors) {
    cuwacunu::hero::runtime::runtime_campaign_record_t campaign{};
    if (campaign_hint != nullptr &&
        trim_ascii(campaign_hint->campaign_cursor) ==
            trim_ascii(campaign_cursor)) {
      campaign = *campaign_hint;
    } else if (!read_runtime_campaign_direct(app, campaign_cursor, &campaign,
                                             error)) {
      return false;
    }
    if (std::filesystem::path(campaign.source_campaign_dsl_path)
            .lexically_normal() != normalized_campaign_source) {
      continue;
    }

    const std::size_t attempted_count =
        attempted_run_count_for_campaign(campaign);
    const std::size_t completed_count =
        completed_run_count_for_campaign(campaign);
    for (std::size_t i = 0; i < attempted_count; ++i) {
      const std::string bind_id = trim_ascii(campaign.run_bind_ids[i]);
      if (!bind_id.empty())
        out->attempted_run_bind_ids.push_back(bind_id);
    }
    if (!out->ordered_prefix_valid)
      continue;
    for (std::size_t i = 0; i < completed_count; ++i) {
      const std::string bind_id = trim_ascii(campaign.run_bind_ids[i]);
      if (bind_id.empty())
        continue;
      if (out->completed_run_bind_ids.size() >=
          out->default_run_bind_ids.size()) {
        out->ordered_prefix_valid = false;
        break;
      }
      if (bind_id !=
          out->default_run_bind_ids[out->completed_run_bind_ids.size()]) {
        out->ordered_prefix_valid = false;
        break;
      }
      out->completed_run_bind_ids.push_back(bind_id);
    }
  }

  if (out->ordered_prefix_valid) {
    for (std::size_t i = out->completed_run_bind_ids.size();
         i < out->default_run_bind_ids.size(); ++i) {
      out->pending_run_bind_ids.push_back(out->default_run_bind_ids[i]);
    }
    if (!out->pending_run_bind_ids.empty()) {
      out->next_pending_bind_id = out->pending_run_bind_ids.front();
    }
  } else {
    out->pending_run_bind_ids = out->default_run_bind_ids;
  }
  return true;
}

[[nodiscard]] std::string
marshal_run_plan_progress_to_json(const marshal_run_plan_progress_t &progress) {
  std::ostringstream out;
  out << "{"
      << "\"declared_bind_ids\":"
      << json_array_from_strings(progress.declared_bind_ids)
      << ",\"default_run_bind_ids\":"
      << json_array_from_strings(progress.default_run_bind_ids)
      << ",\"completed_run_bind_ids\":"
      << json_array_from_strings(progress.completed_run_bind_ids)
      << ",\"attempted_run_bind_ids\":"
      << json_array_from_strings(progress.attempted_run_bind_ids)
      << ",\"pending_run_bind_ids\":"
      << json_array_from_strings(progress.pending_run_bind_ids)
      << ",\"ordered_prefix_valid\":"
      << bool_json(progress.ordered_prefix_valid)
      << ",\"next_pending_bind_id\":";
  if (trim_ascii(progress.next_pending_bind_id).empty()) {
    out << "null";
  } else {
    out << json_quote(progress.next_pending_bind_id);
  }
  out << "}";
  return out.str();
}

[[nodiscard]] bool decode_marshal_objective_snapshot(
    const app_context_t &app, const std::string &marshal_objective_dsl_path,
    marshal_objective_spec_t *out_spec, std::string *error) {
  if (error)
    error->clear();
  if (!out_spec) {
    if (error)
      *error = "marshal objective decode output pointer is null";
    return false;
  }
  *out_spec = marshal_objective_spec_t{};

  std::string grammar_text{};
  if (!cuwacunu::hero::runtime::read_text_file(
          app.defaults.marshal_objective_grammar_path, &grammar_text, error)) {
    return false;
  }
  std::string objective_text{};
  if (!cuwacunu::hero::runtime::read_text_file(marshal_objective_dsl_path,
                                               &objective_text, error)) {
    return false;
  }

  try {
    const auto decoded =
        cuwacunu::camahjucunu::dsl::decode_marshal_objective_from_dsl(
            grammar_text, objective_text);
    out_spec->campaign_dsl_path = trim_ascii(decoded.campaign_dsl_path);
    out_spec->objective_md_path = trim_ascii(decoded.objective_md_path);
    out_spec->guidance_md_path = trim_ascii(decoded.guidance_md_path);
    out_spec->objective_name = trim_ascii(decoded.objective_name);
    out_spec->marshal_codex_model = trim_ascii(decoded.marshal_codex_model);
    if (!trim_ascii(decoded.marshal_codex_reasoning_effort).empty() &&
        !normalize_codex_reasoning_effort(
            decoded.marshal_codex_reasoning_effort,
            &out_spec->marshal_codex_reasoning_effort)) {
      if (error) {
        *error = "failed to decode marshal objective DSL '" +
                 marshal_objective_dsl_path +
                 "': marshal_codex_reasoning_effort must be one of: low, "
                 "medium, high, xhigh";
      }
      return false;
    }
    out_spec->marshal_session_id = trim_ascii(decoded.marshal_session_id);
  } catch (const std::exception &e) {
    if (error) {
      *error = "failed to decode marshal objective DSL '" +
               marshal_objective_dsl_path + "': " + e.what();
    }
    return false;
  }

  if (out_spec->campaign_dsl_path.empty()) {
    if (error) {
      *error = "marshal objective '" + marshal_objective_dsl_path +
               "' is missing required campaign_dsl_path";
    }
    return false;
  }
  if (out_spec->objective_md_path.empty()) {
    if (error) {
      *error = "marshal objective '" + marshal_objective_dsl_path +
               "' is missing required objective_md_path";
    }
    return false;
  }
  if (out_spec->guidance_md_path.empty()) {
    if (error) {
      *error = "marshal objective '" + marshal_objective_dsl_path +
               "' is missing required guidance_md_path";
    }
    return false;
  }
  return true;
}

[[nodiscard]] std::filesystem::path
resolve_requested_marshal_objective_source_path(
    const app_context_t &app, std::string requested_marshal_objective_dsl_path,
    std::string *error) {
  if (error)
    error->clear();
  requested_marshal_objective_dsl_path =
      trim_ascii(std::move(requested_marshal_objective_dsl_path));
  const std::filesystem::path source_marshal_objective_dsl_path =
      requested_marshal_objective_dsl_path.empty()
          ? resolve_default_marshal_objective_dsl_path(app.global_config_path,
                                                       error)
          : std::filesystem::path(resolve_path_from_base_folder(
                app.global_config_path.parent_path().string(),
                requested_marshal_objective_dsl_path));
  if (source_marshal_objective_dsl_path.empty())
    return {};

  std::error_code ec{};
  if (!std::filesystem::exists(source_marshal_objective_dsl_path, ec) ||
      !std::filesystem::is_regular_file(source_marshal_objective_dsl_path,
                                        ec)) {
    if (error) {
      *error = "marshal objective DSL does not exist: " +
               source_marshal_objective_dsl_path.string();
    }
    return {};
  }
  if (lowercase_copy(source_marshal_objective_dsl_path.extension().string()) !=
      ".dsl") {
    if (error) {
      *error = "marshal objective path must target a .dsl file: " +
               source_marshal_objective_dsl_path.string();
    }
    return {};
  }
  return source_marshal_objective_dsl_path;
}

[[nodiscard]] bool resolve_marshal_objective_member_source_path(
    const std::filesystem::path &source_marshal_objective_dsl_path,
    std::string_view raw_path, std::string_view field_name,
    std::filesystem::path *out_path, std::string *error) {
  if (error)
    error->clear();
  if (!out_path) {
    if (error)
      *error = "marshal objective member output pointer is null";
    return false;
  }
  out_path->clear();
  const std::string trimmed = trim_ascii(raw_path);
  if (trimmed.empty()) {
    if (error) {
      *error = "marshal objective '" +
               source_marshal_objective_dsl_path.string() +
               "' is missing required " + std::string(field_name);
    }
    return false;
  }

  std::filesystem::path resolved(trimmed);
  if (!resolved.is_absolute()) {
    resolved = (source_marshal_objective_dsl_path.parent_path() / resolved)
                   .lexically_normal();
  }
  const bool inside_objective_bundle =
      path_is_within(source_marshal_objective_dsl_path.parent_path(), resolved);
  const bool allow_shared_guidance =
      field_name == "guidance_md_path" &&
      path_is_within(source_marshal_objective_dsl_path.parent_path()
                         .parent_path()
                         .parent_path(),
                     resolved);
  if (!inside_objective_bundle && !allow_shared_guidance) {
    if (error) {
      *error = "marshal objective field " + std::string(field_name) +
               " escapes its objective bundle: " + resolved.string();
    }
    return false;
  }
  std::error_code ec{};
  if (!std::filesystem::exists(resolved, ec) ||
      !std::filesystem::is_regular_file(resolved, ec)) {
    if (error) {
      *error = "marshal objective field " + std::string(field_name) +
               " does not exist: " + resolved.string();
    }
    return false;
  }
  *out_path = resolved;
  return true;
}

[[nodiscard]] marshal_session_workspace_context_t
make_marshal_session_workspace_context(const app_context_t &app) {
  marshal_session_workspace_context_t context{};
  context.global_config_path = app.global_config_path;
  context.self_binary_path = app.self_binary_path;
  context.marshal_root = app.defaults.marshal_root;
  context.repo_root = app.defaults.repo_root;
  context.config_scope_root = app.defaults.config_scope_root;
  context.runtime_hero_binary = app.defaults.runtime_hero_binary;
  context.config_hero_binary = app.defaults.config_hero_binary;
  context.lattice_hero_binary = app.defaults.lattice_hero_binary;
  context.human_hero_binary = app.defaults.human_hero_binary;
  context.human_operator_identities = app.defaults.human_operator_identities;
  context.marshal_codex_binary = app.defaults.marshal_codex_binary;
  context.tail_default_lines = app.defaults.tail_default_lines;
  context.poll_interval_ms = app.defaults.poll_interval_ms;
  context.marshal_codex_timeout_sec = app.defaults.marshal_codex_timeout_sec;
  return context;
}

[[nodiscard]] bool load_marshal_session_workspace_context(
    const app_context_t &app,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    marshal_session_workspace_context_t *out, std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error) {
      *error = "marshal session workspace context output pointer is null";
    }
    return false;
  }
  *out = make_marshal_session_workspace_context(app);

  const std::filesystem::path session_root =
      trim_ascii(session.session_root).empty()
          ? cuwacunu::hero::marshal::marshal_session_dir(
                app.defaults.marshal_root, session.marshal_session_id)
          : std::filesystem::path(session.session_root);
  if (!session_root.empty()) {
    out->marshal_root = session_root.parent_path();
  }

  const std::filesystem::path hero_marshal_dsl_path =
      trim_ascii(session.hero_marshal_dsl_path).empty()
          ? cuwacunu::hero::marshal::marshal_session_hero_marshal_dsl_path(
                out->marshal_root, session.marshal_session_id)
          : std::filesystem::path(session.hero_marshal_dsl_path);
  std::error_code ec{};
  if (!std::filesystem::exists(hero_marshal_dsl_path, ec) ||
      !std::filesystem::is_regular_file(hero_marshal_dsl_path, ec)) {
    if (error) {
      *error = "marshal session is missing hero.marshal.dsl: " +
               hero_marshal_dsl_path.string();
    }
    return false;
  }

  marshal_defaults_t resolved{};
  if (!load_marshal_defaults(hero_marshal_dsl_path, app.global_config_path,
                             &resolved, error)) {
    return false;
  }
  out->repo_root = resolved.repo_root;
  out->config_scope_root = resolved.config_scope_root;
  out->runtime_hero_binary = resolved.runtime_hero_binary;
  out->config_hero_binary = resolved.config_hero_binary;
  out->lattice_hero_binary = resolved.lattice_hero_binary;
  out->human_hero_binary = resolved.human_hero_binary;
  out->human_operator_identities = resolved.human_operator_identities;
  out->marshal_codex_binary = resolved.marshal_codex_binary;
  out->tail_default_lines = resolved.tail_default_lines;
  out->poll_interval_ms = resolved.poll_interval_ms;
  out->marshal_codex_timeout_sec = resolved.marshal_codex_timeout_sec;
  return true;
}

[[nodiscard]] std::filesystem::path
runtime_tool_io_dir(const app_context_t &app) {
  return app.defaults.marshal_root / ".runtime_io";
}

[[nodiscard]] std::filesystem::path
human_tool_io_dir(const app_context_t &app) {
  return app.defaults.marshal_root / ".human_io";
}

[[nodiscard]] std::string make_tool_io_basename_(std::string_view prefix,
                                                 std::string_view stem) {
  const std::uint64_t started_at_ms = cuwacunu::hero::runtime::now_ms_utc();
  const std::uint64_t salt = static_cast<std::uint64_t>(
      std::chrono::steady_clock::now().time_since_epoch().count());
  return std::string(prefix) + "." +
         cuwacunu::hero::runtime::base36_lower_u64(started_at_ms) + "." +
         cuwacunu::hero::runtime::base36_lower_u64(
             static_cast<std::uint64_t>(::getpid())) +
         "." + cuwacunu::hero::runtime::base36_lower_u64(salt).substr(0, 6) +
         "." + std::string(stem);
}

[[nodiscard]] std::filesystem::path
make_runtime_tool_io_path(const app_context_t &app, std::string_view stem) {
  return runtime_tool_io_dir(app) / make_tool_io_basename_("rt", stem);
}

[[nodiscard]] std::filesystem::path
make_human_tool_io_path(const app_context_t &app, std::string_view stem) {
  return human_tool_io_dir(app) / make_tool_io_basename_("h", stem);
}

void cleanup_runtime_tool_io(const std::filesystem::path &stdin_path,
                             const std::filesystem::path &stdout_path,
                             const std::filesystem::path &stderr_path) {
  std::error_code ec{};
  (void)std::filesystem::remove(stdin_path, ec);
  (void)std::filesystem::remove(stdout_path, ec);
  (void)std::filesystem::remove(stderr_path, ec);
}

void cleanup_human_tool_io(const std::filesystem::path &stdin_path,
                           const std::filesystem::path &stdout_path,
                           const std::filesystem::path &stderr_path) {
  std::error_code ec{};
  (void)std::filesystem::remove(stdin_path, ec);
  (void)std::filesystem::remove(stdout_path, ec);
  (void)std::filesystem::remove(stderr_path, ec);
}

[[nodiscard]] bool
call_runtime_tool(const app_context_t &app,
                  const std::filesystem::path &runtime_binary,
                  const std::string &tool_name, std::string arguments_json,
                  std::string *out_structured, std::string *error) {
  if (error)
    error->clear();
  if (!out_structured) {
    if (error)
      *error = "runtime structured output pointer is null";
    return false;
  }
  *out_structured = "";

  std::error_code ec{};
  std::filesystem::create_directories(runtime_tool_io_dir(app), ec);
  if (ec) {
    if (error) {
      *error = "cannot create runtime tool io dir: " +
               runtime_tool_io_dir(app).string();
    }
    return false;
  }

  const std::filesystem::path stdin_path =
      make_runtime_tool_io_path(app, "stdin.json");
  const std::filesystem::path stdout_path =
      make_runtime_tool_io_path(app, "stdout.json");
  const std::filesystem::path stderr_path =
      make_runtime_tool_io_path(app, "stderr.log");
  if (!cuwacunu::hero::runtime::write_text_file_atomic(stdin_path, "", error)) {
    return false;
  }

  const std::vector<std::string> argv{runtime_binary.string(),
                                      "--global-config",
                                      app.global_config_path.string(),
                                      "--tool",
                                      tool_name,
                                      "--args-json",
                                      arguments_json};
  int exit_code = -1;
  std::string invoke_error{};
  const bool invoked = run_command_with_stdio_and_timeout(
      argv, stdin_path, stdout_path, stderr_path, nullptr, nullptr,
      kRuntimeToolTimeoutSec, nullptr, &exit_code, &invoke_error);

  std::string stdout_text{};
  std::string stderr_text{};
  std::string ignored{};
  (void)cuwacunu::hero::runtime::read_text_file(stdout_path, &stdout_text,
                                                &ignored);
  (void)cuwacunu::hero::runtime::read_text_file(stderr_path, &stderr_text,
                                                &ignored);
  cleanup_runtime_tool_io(stdin_path, stdout_path, stderr_path);

  if (!invoked && stdout_text.empty()) {
    if (error) {
      *error = !trim_ascii(stderr_text).empty() ? trim_ascii(stderr_text)
                                                : invoke_error;
    }
    return false;
  }
  if (stdout_text.empty()) {
    if (error) {
      *error = !trim_ascii(stderr_text).empty()
                   ? trim_ascii(stderr_text)
                   : ("runtime tool call produced no stdout: " + tool_name);
    }
    return false;
  }
  if (!tool_result_structured_content(stdout_text, out_structured, error)) {
    return false;
  }
  if (exit_code != 0) {
    if (error) {
      *error = !trim_ascii(stderr_text).empty()
                   ? trim_ascii(stderr_text)
                   : ("runtime tool exited non-zero: " + tool_name);
    }
    return false;
  }
  return true;
}

[[nodiscard]] bool call_human_tool(const app_context_t &app,
                                   const std::string &tool_name,
                                   std::string arguments_json,
                                   std::string *out_structured,
                                   std::string *error) {
  if (error)
    error->clear();
  if (!out_structured) {
    if (error)
      *error = "human structured output pointer is null";
    return false;
  }
  *out_structured = "";

  std::error_code ec{};
  std::filesystem::create_directories(human_tool_io_dir(app), ec);
  if (ec) {
    if (error) {
      *error =
          "cannot create human tool io dir: " + human_tool_io_dir(app).string();
    }
    return false;
  }

  const std::filesystem::path stdin_path =
      make_human_tool_io_path(app, "stdin.json");
  const std::filesystem::path stdout_path =
      make_human_tool_io_path(app, "stdout.json");
  const std::filesystem::path stderr_path =
      make_human_tool_io_path(app, "stderr.log");
  if (!cuwacunu::hero::runtime::write_text_file_atomic(stdin_path, "", error)) {
    return false;
  }

  const std::vector<std::string> argv{app.defaults.human_hero_binary.string(),
                                      "--global-config",
                                      app.global_config_path.string(),
                                      "--tool",
                                      tool_name,
                                      "--args-json",
                                      arguments_json};
  int exit_code = -1;
  std::string invoke_error{};
  const bool invoked = run_command_with_stdio_and_timeout(
      argv, stdin_path, stdout_path, stderr_path, nullptr, nullptr,
      kHumanToolTimeoutSec, nullptr, &exit_code, &invoke_error);

  std::string stdout_text{};
  std::string stderr_text{};
  std::string ignored{};
  (void)cuwacunu::hero::runtime::read_text_file(stdout_path, &stdout_text,
                                                &ignored);
  (void)cuwacunu::hero::runtime::read_text_file(stderr_path, &stderr_text,
                                                &ignored);
  cleanup_human_tool_io(stdin_path, stdout_path, stderr_path);

  if (!invoked && stdout_text.empty()) {
    if (error) {
      *error = !trim_ascii(stderr_text).empty() ? trim_ascii(stderr_text)
                                                : invoke_error;
    }
    return false;
  }
  if (stdout_text.empty()) {
    if (error) {
      *error = !trim_ascii(stderr_text).empty()
                   ? trim_ascii(stderr_text)
                   : ("human tool call produced no stdout: " + tool_name);
    }
    return false;
  }
  if (!tool_result_structured_content(stdout_text, out_structured, error)) {
    return false;
  }
  if (exit_code != 0) {
    if (error) {
      *error = !trim_ascii(stderr_text).empty()
                   ? trim_ascii(stderr_text)
                   : ("human tool exited non-zero: " + tool_name);
    }
    return false;
  }
  return true;
}

