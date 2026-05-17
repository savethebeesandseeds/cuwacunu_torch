enum class workbench_marshal_launch_mode_t {
  ExistingObjective,
  CreateObjectiveBundle,
};

inline std::optional<workbench_marshal_launch_mode_t>
prompt_workbench_marshal_launch_mode(CmdState &st) {
  const auto objective_paths = workbench_available_objective_dsl_paths(st);
  std::vector<ui_choice_panel_option_t> options{
      ui_choice_panel_option_t{
          .label = "Launch existing objective",
          .enabled = !objective_paths.empty(),
          .disabled_status =
              "No marshal objective bundles were discovered under objective "
              "roots.",
      },
      ui_choice_panel_option_t{
          .label = "Create new objective bundle",
          .enabled = true,
      },
  };

  std::size_t selected = objective_paths.empty() ? 1u : 0u;
  bool cancelled = false;
  if (!ui_prompt_choice_panel(
          " Marshal objective ",
          "Choose an existing objective bundle to launch, or scaffold a new "
          "objective bundle under an objective root first.",
          options, &selected, &cancelled)) {
    set_workbench_status(st, "failed to collect Marshal launch mode", true);
    return std::nullopt;
  }
  if (cancelled) {
    set_workbench_status(st, "Cancelled.", false);
    return std::nullopt;
  }
  return selected == 0u
             ? workbench_marshal_launch_mode_t::ExistingObjective
             : workbench_marshal_launch_mode_t::CreateObjectiveBundle;
}

inline std::optional<std::string>
prompt_workbench_objective_root(CmdState &st) {
  const auto roots = workbench_objective_roots(st);
  if (roots.empty()) {
    set_workbench_status(st, "No objective roots are configured.", true);
    return std::nullopt;
  }
  if (roots.size() == 1u) {
    return roots.front();
  }

  std::vector<ui_choice_panel_option_t> options{};
  options.reserve(roots.size());
  for (const auto &root : roots) {
    options.push_back(ui_choice_panel_option_t{.label = root, .enabled = true});
  }

  std::size_t selected = 0u;
  bool cancelled = false;
  if (!ui_prompt_choice_panel(
          " Objective root ",
          "Choose which objective root should host the new marshal objective "
          "bundle.",
          options, &selected, &cancelled)) {
    set_workbench_status(st, "failed to collect objective root", true);
    return std::nullopt;
  }
  if (cancelled) {
    set_workbench_status(st, "Cancelled.", false);
    return std::nullopt;
  }
  return roots[std::min(selected, roots.size() - 1u)];
}

inline std::string workbench_normalize_objective_bundle_name(std::string raw) {
  raw = trim_copy(raw);
  std::string out{};
  bool last_was_separator = false;
  for (const unsigned char uch : raw) {
    const char ch = static_cast<char>(uch);
    if (std::isalnum(uch)) {
      out.push_back(static_cast<char>(std::tolower(uch)));
      last_was_separator = false;
      continue;
    }
    if (ch == '.' || ch == '_' || ch == '-') {
      if (!out.empty() && !last_was_separator) {
        out.push_back(ch);
        last_was_separator = true;
      }
      continue;
    }
    if (std::isspace(uch) || ch == '/' || ch == '\\') {
      if (!out.empty() && !last_was_separator) {
        out.push_back('.');
        last_was_separator = true;
      }
    }
  }
  while (!out.empty() &&
         (out.back() == '.' || out.back() == '_' || out.back() == '-')) {
    out.pop_back();
  }
  while (!out.empty() &&
         (out.front() == '.' || out.front() == '_' || out.front() == '-')) {
    out.erase(out.begin());
  }
  return out;
}

inline std::string workbench_build_marshal_objective_dsl(
    std::string_view objective_name, std::string_view campaign_rel_path,
    std::string_view objective_md_rel_path,
    std::string_view guidance_md_rel_path) {
  std::ostringstream oss;
  oss << "/*\n";
  oss << "  " << objective_name << ".marshal.dsl\n";
  oss << "  ===========================\n";
  oss << "  Marshal Hero entrypoint for " << objective_name << ".\n";
  oss << "\n";
  oss << "  Fresh sessions should inherit the current source objective and\n";
  oss << "  local guidance from here rather than relying on older session-local\n";
  oss << "  snapshots.\n";
  oss << "  Optional:\n";
  oss << "    marshal_codex_model:str = gpt-5.4\n";
  oss << "    marshal_codex_reasoning_effort:str = xhigh\n";
  oss << "    marshal_session_id:str = some.stable.marshal.id\n";
  oss << "*/\n";
  oss << "campaign_dsl_path:path = " << campaign_rel_path << "\n";
  oss << "objective_md_path:path = " << objective_md_rel_path << "\n";
  oss << "guidance_md_path:path = " << guidance_md_rel_path << "\n";
  oss << "objective_name:str = " << objective_name << "\n";
  return oss.str();
}

inline bool workbench_create_objective_file(CmdState &st,
                                            const std::string &objective_root,
                                            const std::string &relative_path,
                                            const std::string &content,
                                            std::string *created_path) {
  std::ostringstream args;
  args << "{\"objective_root\":" << config_json_quote(objective_root)
       << ",\"path\":" << config_json_quote(relative_path)
       << ",\"content\":" << config_json_quote(content) << "}";
  cmd_json_value_t structured{};
  std::string error{};
  if (!workbench_invoke_tool(st, "hero.config.objective.create", args.str(),
                             &structured, &error)) {
    set_workbench_status(st, error, true);
    return false;
  }
  if (created_path != nullptr) {
    const auto fallback =
        (std::filesystem::path(objective_root) / relative_path)
            .lexically_normal()
            .string();
    *created_path = cmd_json_string(cmd_json_field(&structured, "path"),
                                    fallback);
  }
  return true;
}

inline bool workbench_delete_objective_file(CmdState &st,
                                            const std::string &objective_root,
                                            const std::string &relative_path,
                                            std::string *error) {
  std::ostringstream args;
  args << "{\"objective_root\":" << config_json_quote(objective_root)
       << ",\"path\":" << config_json_quote(relative_path) << "}";
  cmd_json_value_t structured{};
  return workbench_invoke_tool(st, "hero.config.objective.delete", args.str(),
                               &structured, error);
}

inline std::string workbench_cleanup_created_objective_bundle(
    CmdState &st, const std::string &objective_root,
    const std::vector<std::string> &created_relative_paths) {
  std::vector<std::string> failures{};
  for (auto it = created_relative_paths.rbegin();
       it != created_relative_paths.rend(); ++it) {
    std::string error{};
    if (!workbench_delete_objective_file(st, objective_root, *it, &error) &&
        !error.empty()) {
      failures.push_back(*it + ": " + error);
    }
  }
  if (failures.empty()) {
    return {};
  }
  std::ostringstream oss;
  oss << "cleanup failed for ";
  for (std::size_t i = 0; i < failures.size(); ++i) {
    if (i != 0u) {
      oss << " | ";
    }
    oss << failures[i];
  }
  return oss.str();
}

inline void clear_workbench_marshal_launch_draft(CmdState &st) {
  st.workbench.marshal_launch = WorkbenchMarshalLaunchDraft{};
}

inline void clear_workbench_campaign_launch_draft(CmdState &st) {
  st.workbench.campaign_launch = WorkbenchCampaignLaunchDraft{};
}

inline void clear_workbench_editor_return(CmdState &st) {
  st.workbench.editor_return = WorkbenchEditorReturnState{};
}

inline void workbench_apply_booster_config_write_scope(ConfigState *config) {
  if (config == nullptr) {
    return;
  }
  constexpr const char *kBoosterBackupRoot = "/cuwacunu/.backups/hero.config";
  constexpr const char *kBoosterOptimBackupRoot =
      "/cuwacunu/.backups/hero.config.optim";
  std::vector<std::string> write_roots{};
  auto push_root = [&](const std::string &root) {
    const std::string trimmed = trim_copy(root);
    if (!trimmed.empty()) {
      write_roots.push_back(trimmed);
    }
  };
  for (const auto &root : config->objective_roots) {
    push_root(root);
  }
  for (const auto &root : config->temp_roots) {
    push_root(root);
  }
  if (write_roots.empty()) {
    push_root("/cuwacunu/src/config/instructions/objectives");
    push_root("/cuwacunu/src/config/instructions/temp");
  }
  push_root(kBoosterBackupRoot);
  push_root(kBoosterOptimBackupRoot);
  std::vector<std::string> unique{};
  std::set<std::string> seen{};
  for (const auto &root : write_roots) {
    if (seen.insert(root).second) {
      unique.push_back(root);
    }
  }
  config->write_policy_path =
      (std::filesystem::temp_directory_path() /
       "iinuji_cmd.workbench.booster.config.hero.dsl")
          .string();
  config->using_session_policy = true;
  config->policy_write_allowed = true;
  config->write_roots = std::move(unique);
  {
    const auto join_csv = [](const std::vector<std::string> &roots) {
      std::ostringstream oss;
      for (std::size_t i = 0; i < roots.size(); ++i) {
        if (i != 0u) {
          oss << ",";
        }
        oss << roots[i];
      }
      return oss.str();
    };
    std::vector<std::string> default_roots{};
    std::vector<std::string> objective_roots{};
    std::vector<std::string> temp_roots{};
    const auto collect_roots = [](const std::vector<std::string> &source,
                                  std::vector<std::string> *out) {
      for (const auto &root : source) {
        const std::string trimmed = trim_copy(root);
        if (!trimmed.empty()) {
          out->push_back(trimmed);
        }
      }
    };
    collect_roots(config->default_roots, &default_roots);
    collect_roots(config->objective_roots, &objective_roots);
    collect_roots(config->temp_roots, &temp_roots);
    if (default_roots.empty()) {
      default_roots.push_back("/cuwacunu/src/config/instructions/defaults");
    }
    if (objective_roots.empty()) {
      objective_roots.push_back("/cuwacunu/src/config/instructions/objectives");
    }
    if (temp_roots.empty()) {
      temp_roots.push_back("/cuwacunu/src/config/instructions/temp");
    }
    std::ofstream out(config->write_policy_path);
    if (out) {
      out << "protocol_layer[STDIO|HTTPS/SSE]:str = STDIO\n";
      out << "config_scope_root:str = /cuwacunu/src/config\n";
      out << "allow_local_read:bool = true\n";
      out << "allow_local_write:bool = true\n";
      out << "default_roots:str = " << join_csv(default_roots) << "\n";
      out << "objective_roots:str = " << join_csv(objective_roots) << "\n";
      out << "temp_roots:str = " << join_csv(temp_roots) << "\n";
      out << "allowed_extensions:str = .dsl,.md\n";
      out << "write_roots:str = " << join_csv(config->write_roots) << "\n";
      out << "backup_enabled:bool = true\n";
      out << "backup_dir:str = " << kBoosterBackupRoot << "\n";
      out << "backup_max_entries(1,+inf):int = 20\n";
      out << "optim_backup_enabled:bool = true\n";
      out << "optim_backup_dir:str = " << kBoosterOptimBackupRoot << "\n";
      out << "optim_backup_max_entries(1,+inf):int = 20\n";
      out << "dev_nuke_reset_backup_enabled:bool = true\n";
    }
  }
  config_apply_write_scope(config);
}

inline std::string workbench_normalized_path_string(const std::string &value) {
  return std::filesystem::path(value).lexically_normal().string();
}

inline bool workbench_path_is_under_root(const std::string &path,
                                         const std::string &root) {
  const std::string normalized_path = workbench_normalized_path_string(path);
  std::string normalized_root = workbench_normalized_path_string(root);
  if (normalized_root.empty()) {
    return false;
  }
  if (normalized_path == normalized_root) {
    return true;
  }
  if (normalized_root.back() != '/' && normalized_root.back() != '\\') {
    normalized_root.push_back('/');
  }
  return normalized_path.rfind(normalized_root, 0u) == 0u;
}

inline bool workbench_path_is_booster_writable(const CmdState &st,
                                               const std::string &path) {
  for (const auto &root : st.config.objective_roots) {
    if (workbench_path_is_under_root(path, root)) {
      return true;
    }
  }
  for (const auto &root : st.config.temp_roots) {
    if (workbench_path_is_under_root(path, root)) {
      return true;
    }
  }
  return false;
}

struct workbench_config_file_target_t {
  ConfigFileFamily family{ConfigFileFamily::Defaults};
  std::string root_path{};
  std::string path{};
  std::string relative_path{};
};

inline std::optional<workbench_config_file_target_t>
workbench_resolve_config_file_target(const CmdState &st,
                                     const std::string &path) {
  const std::filesystem::path normalized =
      std::filesystem::path(path).lexically_normal();

  auto resolve_from_roots =
      [&](const std::vector<std::string> &roots,
          ConfigFileFamily family) -> std::optional<workbench_config_file_target_t> {
    std::optional<workbench_config_file_target_t> best{};
    std::size_t best_len = 0u;
    for (const auto &root : roots) {
      if (!workbench_path_is_under_root(normalized.string(), root)) {
        continue;
      }
      const std::filesystem::path root_path =
          std::filesystem::path(root).lexically_normal();
      std::error_code ec{};
      const std::string relative =
          std::filesystem::relative(normalized, root_path, ec).string();
      if (ec || relative.empty()) {
        continue;
      }
      const std::size_t root_len = root_path.string().size();
      if (!best.has_value() || root_len > best_len) {
        best = workbench_config_file_target_t{
            .family = family,
            .root_path = root_path.string(),
            .path = normalized.string(),
            .relative_path = relative,
        };
        best_len = root_len;
      }
    }
    return best;
  };

  if (const auto objective =
          resolve_from_roots(st.config.objective_roots,
                             ConfigFileFamily::Objectives);
      objective.has_value()) {
    return objective;
  }
  if (const auto temp =
          resolve_from_roots(st.config.temp_roots, ConfigFileFamily::Temp);
      temp.has_value()) {
    return temp;
  }
  if (const auto defaults =
          resolve_from_roots(st.config.default_roots,
                             ConfigFileFamily::Defaults);
      defaults.has_value()) {
    return defaults;
  }
  return std::nullopt;
}

inline bool workbench_open_config_editor_for_path(CmdState &st,
                                                  const std::string &path,
                                                  std::string editor_status,
                                                  bool booster_write_scope = false,
                                                  std::optional<workbench_booster_action_kind_t>
                                                      return_to_booster_action =
                                                          std::nullopt,
                                                  std::string return_status = {}) {
  const auto target = workbench_resolve_config_file_target(st, path);
  if (!target.has_value()) {
    set_workbench_status(
        st, "Booster could not resolve this file inside Config roots: " + path,
        true);
    return false;
  }

  ConfigState refreshed = st.config;
  refreshed.ok = true;
  refreshed.error.clear();
  refreshed.allowed_extensions = refreshed.allowed_extensions.empty()
                                     ? ".dsl,.md"
                                     : refreshed.allowed_extensions;
  refreshed.async = std::make_shared<ConfigAsyncState>();
  if (refreshed.default_roots.empty()) {
    refreshed.default_roots.push_back(
        "/cuwacunu/src/config/instructions/defaults");
  }
  if (refreshed.objective_roots.empty()) {
    refreshed.objective_roots.push_back(
        "/cuwacunu/src/config/instructions/objectives");
  }
  if (refreshed.temp_roots.empty()) {
    refreshed.temp_roots.push_back("/cuwacunu/src/config/instructions/temp");
  }
  if (booster_write_scope) {
    workbench_apply_booster_config_write_scope(&refreshed);
  }

  std::shared_ptr<ConfigState> catalog_snapshot{};
  if (st.config.transient_single_file_view && st.config.catalog_snapshot) {
    catalog_snapshot = st.config.catalog_snapshot;
  } else if (!st.config.files.empty()) {
    catalog_snapshot = std::make_shared<ConfigState>(st.config);
    catalog_snapshot->transient_single_file_view = false;
    catalog_snapshot->catalog_snapshot.reset();
    catalog_snapshot->editor.reset();
    catalog_snapshot->editor_focus = false;
  }

  cuwacunu::hero::mcp::hero_config_store_t store(
      config_effective_write_policy_path(refreshed),
      cuwacunu::iitepi::config_space_t::config_file_path);
  std::string store_error{};
  if (!store.load(&store_error)) {
    set_workbench_status(st, "failed to load Config policy: " + store_error,
                         true);
    return false;
  }

  auto idx = config_file_index_by_path(refreshed, target->path, target->family);
  if (!idx.has_value()) {
    refreshed.files.push_back(make_config_file_entry(
        target->family, target->root_path, target->path, target->relative_path,
        true, refreshed.policy_write_allowed));
    idx = refreshed.files.size() - 1u;
  }
  auto &file = refreshed.files[*idx];
  file.family = target->family;
  file.root_path = target->root_path;
  file.path = target->path;
  file.relative_path = target->relative_path;
  file.request_root_path = target->root_path;
  file.request_path = target->relative_path;
  file.replace_supported = true;
  file.write_allowed = refreshed.policy_write_allowed;
  config_refresh_entry_identity(&file);
  if (!config_fill_file_payload(&store, &file)) {
    set_workbench_status(
        st,
        file.error.empty() ? "failed to load Config file for editing"
                           : file.error,
        true);
    return false;
  }

  refreshed.selected_file = *idx;
  refreshed.selected_family = target->family;
  refreshed.browse_focus = ConfigBrowseFocus::Files;
  refreshed.transient_single_file_view = return_to_booster_action.has_value();
  refreshed.catalog_snapshot = std::move(catalog_snapshot);
  sync_config_editor_from_selection(refreshed);

  st.config = std::move(refreshed);
  st.screen = ScreenMode::Config;
  (void)open_selected_config_file(st);
  clear_workbench_editor_return(st);
  if (return_to_booster_action.has_value()) {
    st.workbench.editor_return.active = true;
    st.workbench.editor_return.booster_action = *return_to_booster_action;
    st.workbench.editor_return.status =
        return_status.empty()
            ? "Returned to Booster. Review edits and continue."
            : std::move(return_status);
  }
  if (editor_status.empty()) {
    editor_status =
        "editor focus | Ctrl+S saves through Config Hero";
  }
  config_set_status(st.config, std::move(editor_status), false);
  return true;
}

struct workbench_marshal_bundle_t {
  std::string objective_name{};
  std::string objective_dsl_path{};
  std::string objective_md_path{};
  std::string guidance_md_path{};
  std::string campaign_path{};
  std::string marshal_codex_model{};
  std::string marshal_codex_reasoning_effort{};
};

inline std::string workbench_extract_marshal_dsl_value(std::string_view text,
                                                       std::string_view key) {
  std::istringstream in{std::string(text)};
  std::string line{};
  while (std::getline(in, line)) {
    const std::size_t eq = line.find('=');
    if (eq == std::string::npos) {
      continue;
    }
    std::string lhs = trim_copy(line.substr(0, eq));
    const std::size_t domain = lhs.find(':');
    if (domain != std::string::npos) {
      lhs.resize(domain);
    }
    const std::size_t paren = lhs.find('(');
    if (paren != std::string::npos) {
      lhs.resize(paren);
    }
    lhs = trim_copy(lhs);
    if (lhs != key) {
      continue;
    }
    return trim_copy(line.substr(eq + 1));
  }
  return {};
}

inline std::optional<workbench_marshal_bundle_t>
workbench_resolve_marshal_bundle(const std::string &objective_dsl_path) {
  const std::string content = workbench_read_file_text(objective_dsl_path);
  if (content.empty()) {
    return std::nullopt;
  }
  const std::filesystem::path dsl_path(objective_dsl_path);
  const std::filesystem::path base_dir = dsl_path.parent_path();
  const auto resolve_field = [&](std::string_view key) {
    const std::string raw = workbench_extract_marshal_dsl_value(content, key);
    if (raw.empty()) {
      return std::string{};
    }
    const std::filesystem::path raw_path(raw);
    return (raw_path.is_absolute() ? raw_path : (base_dir / raw_path))
        .lexically_normal()
        .string();
  };

  workbench_marshal_bundle_t bundle{};
  bundle.objective_name = workbench_extract_marshal_dsl_value(content, "objective_name");
  if (bundle.objective_name.empty()) {
    bundle.objective_name = workbench_objective_display_name(objective_dsl_path);
  }
  bundle.objective_dsl_path = dsl_path.lexically_normal().string();
  bundle.objective_md_path = resolve_field("objective_md_path");
  bundle.guidance_md_path = resolve_field("guidance_md_path");
  bundle.campaign_path = resolve_field("campaign_dsl_path");
  bundle.marshal_codex_model =
      workbench_extract_marshal_dsl_value(content, "marshal_codex_model");
  bundle.marshal_codex_reasoning_effort = workbench_extract_marshal_dsl_value(
      content, "marshal_codex_reasoning_effort");
  if (bundle.objective_md_path.empty() || bundle.guidance_md_path.empty() ||
      bundle.campaign_path.empty()) {
    return std::nullopt;
  }
  return bundle;
}

inline void workbench_stage_marshal_launch_draft(
    CmdState &st, const workbench_marshal_bundle_t &bundle,
    bool created_new_bundle = false) {
  st.workbench.marshal_launch.active = true;
  st.workbench.marshal_launch.objective_name = bundle.objective_name;
  st.workbench.marshal_launch.objective_dsl_path = bundle.objective_dsl_path;
  st.workbench.marshal_launch.objective_md_path = bundle.objective_md_path;
  st.workbench.marshal_launch.guidance_md_path = bundle.guidance_md_path;
  st.workbench.marshal_launch.campaign_path = bundle.campaign_path;
  st.workbench.marshal_launch.objective_marshal_model =
      bundle.marshal_codex_model;
  st.workbench.marshal_launch.objective_marshal_reasoning_effort =
      bundle.marshal_codex_reasoning_effort;
  st.workbench.marshal_launch.marshal_model.clear();
  st.workbench.marshal_launch.marshal_reasoning_effort.clear();
  st.workbench.marshal_launch.created_new_bundle = created_new_bundle;
}

inline void workbench_stage_campaign_launch_draft(
    CmdState &st, const workbench_booster_source_selection_t &selection) {
  st.workbench.campaign_launch.active = true;
  st.workbench.campaign_launch.campaign_path = selection.path;
  st.workbench.campaign_launch.binding_id.clear();
  st.workbench.campaign_launch.reset_runtime_state = false;
  st.workbench.campaign_launch.created_temp = selection.created_temp;
  st.workbench.campaign_launch.path_is_writable =
      workbench_path_is_booster_writable(st, selection.path);
}

inline std::string workbench_marshal_model_label(const CmdState &st,
                                                 const std::string &value) {
  if (!value.empty()) {
    return value;
  }
  if (!st.workbench.marshal_launch.objective_marshal_model.empty()) {
    return st.workbench.marshal_launch.objective_marshal_model;
  }
  return st.runtime.marshal_app.defaults.marshal_codex_model.empty()
             ? std::string("gpt-5.3-codex-spark")
             : st.runtime.marshal_app.defaults.marshal_codex_model;
}

inline std::string workbench_marshal_reasoning_label(const CmdState &st,
                                                     const std::string &value) {
  if (!value.empty()) {
    return value;
  }
  if (!st.workbench.marshal_launch.objective_marshal_reasoning_effort.empty()) {
    return st.workbench.marshal_launch.objective_marshal_reasoning_effort;
  }
  return st.runtime.marshal_app.defaults.marshal_codex_reasoning_effort.empty()
             ? std::string("xhigh")
             : st.runtime.marshal_app.defaults.marshal_codex_reasoning_effort;
}

enum class workbench_marshal_prepare_action_t : std::uint8_t {
  Start = 0,
  EditMarshalDsl = 1,
  EditObjectiveMd = 2,
  EditGuidanceMd = 3,
  EditCampaignDsl = 4,
  ChooseModel = 5,
  ChooseReasoning = 6,
  Cancel = 7,
};

inline std::string workbench_campaign_binding_label(const std::string &value) {
  return value.empty() ? "default RUN plan" : ("binding " + value);
}

inline std::string
workbench_campaign_reset_label(const WorkbenchCampaignLaunchDraft &draft) {
  return draft.reset_runtime_state ? "enabled" : "disabled";
}

enum class workbench_campaign_prepare_action_t : std::uint8_t {
  Start = 0,
  EditCampaignDsl = 1,
  ChooseBinding = 2,
  ToggleResetRuntimeState = 3,
  Cancel = 4,
};

inline std::optional<workbench_marshal_prepare_action_t>
prompt_workbench_marshal_prepare_action(CmdState &st) {
  const auto &draft = st.workbench.marshal_launch;
  const bool has_model_override = !draft.marshal_model.empty();
  const bool has_reasoning_override = !draft.marshal_reasoning_effort.empty();
  std::ostringstream body;
  body << "Objective: " << draft.objective_name << "\n";
  body << "Marshal model: "
       << workbench_marshal_model_label(st, draft.marshal_model) << " ("
       << (has_model_override ? "launch override" : "objective/default")
       << ")\n";
  body << "Reasoning: "
       << workbench_marshal_reasoning_label(st, draft.marshal_reasoning_effort)
       << " ("
       << (has_reasoning_override ? "launch override" : "objective/default")
       << ")\n";
  body << "Objective DSL: " << draft.objective_dsl_path << "\n";
  body << "Objective MD: " << draft.objective_md_path << "\n";
  body << "Guidance MD: " << draft.guidance_md_path << "\n";
  body << "Campaign DSL: " << draft.campaign_path << "\n";
  if (draft.created_new_bundle) {
    body << "\nNew objective bundle scaffolded from defaults.";
  }

  std::vector<ui_choice_panel_option_t> options{
      ui_choice_panel_option_t{.label = "Start Marshal", .enabled = true},
      ui_choice_panel_option_t{.label = "Edit marshal.dsl",
                               .enabled = !draft.objective_dsl_path.empty(),
                               .disabled_status = "marshal.dsl is unavailable"},
      ui_choice_panel_option_t{.label = "Edit objective.md",
                               .enabled = !draft.objective_md_path.empty(),
                               .disabled_status = "objective.md is unavailable"},
      ui_choice_panel_option_t{.label = "Edit guidance.md",
                               .enabled = !draft.guidance_md_path.empty(),
                               .disabled_status = "guidance.md is unavailable"},
      ui_choice_panel_option_t{.label = "Edit campaign.dsl",
                               .enabled = !draft.campaign_path.empty(),
                               .disabled_status = "campaign.dsl is unavailable"},
      ui_choice_panel_option_t{.label = "Choose model override",
                               .enabled = true},
      ui_choice_panel_option_t{.label = "Choose reasoning override",
                               .enabled = true},
      ui_choice_panel_option_t{.label = "Cancel launch", .enabled = true},
  };

  std::size_t selected = 0u;
  bool cancelled = false;
  if (!ui_prompt_choice_panel(" Marshal launch ", body.str(), options, &selected,
                              &cancelled)) {
    set_workbench_status(st, "failed to collect Marshal launch preparation",
                         true);
    return std::nullopt;
  }
  if (cancelled) {
    set_workbench_status(st, "Cancelled.", false);
    return std::nullopt;
  }
  return static_cast<workbench_marshal_prepare_action_t>(selected);
}

inline std::optional<workbench_campaign_prepare_action_t>
prompt_workbench_campaign_prepare_action(CmdState &st) {
  const auto &draft = st.workbench.campaign_launch;
  std::ostringstream body;
  body << "Campaign DSL: " << draft.campaign_path << "\n";
  body << "Binding: " << workbench_campaign_binding_label(draft.binding_id)
       << "\n";
  body << "Runtime reset: "
       << workbench_campaign_reset_label(draft) << "\n";
  if (draft.created_temp) {
    body << "\nTemp campaign created from default.";
  }
  if (!draft.path_is_writable) {
    body << "\nThis source campaign is shared/read-only through Booster. "
            "Choose a temp campaign if you want to edit before launch.";
  }
  body << "\nWhen runtime reset is enabled, Runtime Hero clears runtime state "
          "before starting this campaign.";

  std::vector<ui_choice_panel_option_t> options{
      ui_choice_panel_option_t{.label = "Start campaign", .enabled = true},
      ui_choice_panel_option_t{
          .label = "Edit campaign.dsl",
          .enabled = draft.path_is_writable,
          .disabled_status =
              "Shared/default campaign paths are read-only here. Choose a "
              "temp campaign if you want to edit before launch.",
      },
      ui_choice_panel_option_t{.label = "Choose binding target",
                               .enabled = true},
      ui_choice_panel_option_t{
          .label = std::string("Runtime reset: ") +
                   workbench_campaign_reset_label(draft),
          .enabled = true,
      },
      ui_choice_panel_option_t{.label = "Cancel launch", .enabled = true},
  };

  std::size_t selected = 0u;
  bool cancelled = false;
  if (!ui_prompt_choice_panel(" Campaign launch ", body.str(), options,
                              &selected, &cancelled)) {
    set_workbench_status(st, "failed to collect campaign launch preparation",
                         true);
    return std::nullopt;
  }
  if (cancelled) {
    set_workbench_status(st, "Cancelled.", false);
    return std::nullopt;
  }
  return static_cast<workbench_campaign_prepare_action_t>(selected);
}

