inline bool workbench_show_runtime_session(
    CmdState &st, std::string_view marshal_session_id, std::string status) {
  std::string session_error{};
  const auto sessions = workbench_fresh_marshal_sessions(st, &session_error);
  if (!session_error.empty()) {
    set_workbench_status(st, session_error, true);
    return false;
  }
  for (const auto &session : sessions) {
    if (session.marshal_session_id != marshal_session_id) {
      continue;
    }
    st.runtime.sessions = sessions;
    st.runtime.ok = true;
    st.runtime.error.clear();
    clear_runtime_drill_filters(st);
    st.runtime.lane = kRuntimeSessionLane;
    st.runtime.family_anchor_lane = kRuntimeSessionLane;
    rebuild_runtime_lineage_cache(st);
    select_runtime_session_and_related(st, session.marshal_session_id);
    focus_runtime_worklist(st);
    st.screen = ScreenMode::Runtime;
    (void)queue_runtime_refresh(st);
    set_runtime_status(st, std::move(status), false);
    return true;
  }
  set_workbench_status(st, "started Marshal session but could not load it in Runtime",
                       true);
  return false;
}

inline bool workbench_show_runtime_campaign(CmdState &st,
                                            std::string_view campaign_cursor,
                                            std::string status) {
  st.runtime.ok = true;
  st.runtime.error.clear();
  clear_runtime_drill_filters(st);
  st.runtime.lane = kRuntimeCampaignLane;
  st.runtime.family_anchor_lane = kRuntimeCampaignLane;
  st.runtime.selected_campaign_cursor = std::string(campaign_cursor);
  st.runtime.selected_session_id.clear();
  st.runtime.anchor_session_id.clear();
  st.runtime.selected_job_cursor.clear();
  update_runtime_selection_records(st);
  focus_runtime_worklist(st);
  st.screen = ScreenMode::Runtime;
  (void)queue_runtime_refresh(st);
  set_runtime_status(st, std::move(status), false);
  return true;
}

inline bool create_workbench_marshal_objective_bundle(CmdState &st) {
  const auto objective_root = prompt_workbench_objective_root(st);
  if (!objective_root.has_value()) {
    return true;
  }

  std::string requested_name{};
  bool cancelled = false;
  if (!ui_prompt_text_dialog(
          " Objective bundle name ",
          "Choose the new objective bundle name. Booster will normalize it "
          "into the directory and file stem used under the selected objective "
          "root.",
          &requested_name, false, false, &cancelled)) {
    set_workbench_status(st, "failed to collect objective bundle name", true);
    return true;
  }
  if (cancelled) {
    set_workbench_status(st, "Cancelled.", false);
    return true;
  }

  const std::string objective_name =
      workbench_normalize_objective_bundle_name(requested_name);
  if (objective_name.empty()) {
    set_workbench_status(
        st, "Objective bundle name must contain letters or digits.", true);
    return true;
  }

  const auto objective_root_path =
      std::filesystem::path(*objective_root).lexically_normal();
  const auto bundle_dir = objective_root_path / objective_name;
  std::error_code ec{};
  if (std::filesystem::exists(bundle_dir, ec)) {
    set_workbench_status(
        st, "Objective bundle already exists: " + bundle_dir.string(), true);
    return true;
  }

  const std::string template_objective_dsl =
      workbench_preferred_default_objective_dsl_path(st);
  const std::string template_objective_md =
      workbench_preferred_default_objective_md_path(st);
  const std::string template_guidance_md =
      workbench_preferred_default_guidance_md_path(st);
  const std::string template_campaign = workbench_preferred_default_campaign_path(st);
  const std::string objective_md_content =
      workbench_read_file_text(template_objective_md);
  const std::string guidance_md_content =
      workbench_read_file_text(template_guidance_md);
  const std::string campaign_content = workbench_read_file_text(template_campaign);

  if (template_objective_dsl.empty() || template_objective_md.empty() ||
      template_guidance_md.empty() || template_campaign.empty() ||
      objective_md_content.empty() || guidance_md_content.empty() ||
      campaign_content.empty()) {
    set_workbench_status(
        st, "Booster could not load the default marshal objective templates.",
        true);
    return true;
  }

  const std::string objective_dsl_rel =
      objective_name + "/" + objective_name + ".marshal.dsl";
  const std::string objective_md_rel =
      objective_name + "/" + objective_name + ".objective.md";
  const std::string guidance_md_rel =
      objective_name + "/" + objective_name + ".guidance.md";
  const std::string campaign_rel = objective_name + "/iitepi.campaign.dsl";

  const std::string objective_dsl_content = workbench_build_marshal_objective_dsl(
      objective_name, "iitepi.campaign.dsl", objective_name + ".objective.md",
      objective_name + ".guidance.md");

  std::vector<std::string> created_relative_paths{};
  const auto create_with_rollback =
      [&](const std::string &relative_path, const std::string &content,
          std::string *created_path) -> bool {
    if (workbench_create_objective_file(st, *objective_root, relative_path,
                                        content, created_path)) {
      created_relative_paths.push_back(relative_path);
      return true;
    }
    const std::string cleanup_error = workbench_cleanup_created_objective_bundle(
        st, *objective_root, created_relative_paths);
    if (!cleanup_error.empty()) {
      const std::string combined_status =
          st.workbench.status.empty() ? cleanup_error
                                      : (st.workbench.status + " | " + cleanup_error);
      set_workbench_status(st, combined_status, true);
    }
    return false;
  };

  if (!create_with_rollback(objective_md_rel, objective_md_content, nullptr)) {
    return true;
  }
  if (!create_with_rollback(guidance_md_rel, guidance_md_content, nullptr)) {
    return true;
  }
  if (!create_with_rollback(campaign_rel, campaign_content, nullptr)) {
    return true;
  }
  std::string created_dsl_path{};
  if (!create_with_rollback(objective_dsl_rel, objective_dsl_content,
                            &created_dsl_path)) {
    return true;
  }

  const auto bundle = workbench_resolve_marshal_bundle(created_dsl_path);
  if (!bundle.has_value()) {
    set_workbench_status(
        st, "created objective bundle but could not resolve its primary files",
        true);
    return true;
  }
  clear_workbench_marshal_launch_draft(st);
  workbench_stage_marshal_launch_draft(st, *bundle, true);
  set_workbench_status(
      st,
      "New marshal objective bundle created. Review, edit, and confirm launch.",
      false);
  return true;
}

inline bool launch_workbench_marshal(CmdState &st) {
  if (!st.workbench.marshal_launch.active) {
    const auto launch_mode = prompt_workbench_marshal_launch_mode(st);
    if (!launch_mode.has_value()) {
      return true;
    }
    if (*launch_mode == workbench_marshal_launch_mode_t::CreateObjectiveBundle) {
      (void)create_workbench_marshal_objective_bundle(st);
      if (!st.workbench.marshal_launch.active) {
        return true;
      }
    } else {
      const auto selection = prompt_workbench_marshal_objective_choice(st);
      if (!selection.has_value()) {
        return true;
      }
      const auto bundle = workbench_resolve_marshal_bundle(*selection);
      if (!bundle.has_value()) {
        set_workbench_status(
            st, "selected objective bundle is missing primary launch files", true);
        return true;
      }
      clear_workbench_marshal_launch_draft(st);
      workbench_stage_marshal_launch_draft(st, *bundle, false);
    }
  }

  while (st.workbench.marshal_launch.active) {
    const auto action = prompt_workbench_marshal_prepare_action(st);
    if (!action.has_value()) {
      return true;
    }

    switch (*action) {
    case workbench_marshal_prepare_action_t::EditMarshalDsl:
      return workbench_open_config_editor_for_path(
          st, st.workbench.marshal_launch.objective_dsl_path,
          "editing marshal.dsl | Enter inserts newline | Esc opens save/discard "
          "| return to Booster to continue launch",
          true, workbench_booster_action_kind_t::LaunchMarshal,
          "Returned to Booster. Review edits and confirm launch.");
    case workbench_marshal_prepare_action_t::EditObjectiveMd:
      return workbench_open_config_editor_for_path(
          st, st.workbench.marshal_launch.objective_md_path,
          "editing objective.md | Enter inserts newline | Esc opens save/discard "
          "| return to Booster to continue launch",
          true, workbench_booster_action_kind_t::LaunchMarshal,
          "Returned to Booster. Review edits and confirm launch.");
    case workbench_marshal_prepare_action_t::EditGuidanceMd:
      return workbench_open_config_editor_for_path(
          st, st.workbench.marshal_launch.guidance_md_path,
          "editing guidance.md | Enter inserts newline | Esc opens save/discard "
          "| return to Booster to continue launch",
          true, workbench_booster_action_kind_t::LaunchMarshal,
          "Returned to Booster. Review edits and confirm launch.");
    case workbench_marshal_prepare_action_t::EditCampaignDsl:
      return workbench_open_config_editor_for_path(
          st, st.workbench.marshal_launch.campaign_path,
          "editing campaign.dsl | Enter inserts newline | Esc opens save/discard "
          "| return to Booster to continue launch",
          true, workbench_booster_action_kind_t::LaunchMarshal,
          "Returned to Booster. Review edits and confirm launch.");
    case workbench_marshal_prepare_action_t::ChooseModel: {
      const auto model = prompt_workbench_marshal_model_override(st);
      if (!model.has_value()) {
        continue;
      }
      st.workbench.marshal_launch.marshal_model = *model;
      continue;
    }
    case workbench_marshal_prepare_action_t::ChooseReasoning: {
      const auto reasoning = prompt_workbench_marshal_reasoning_override(st);
      if (!reasoning.has_value()) {
        continue;
      }
      st.workbench.marshal_launch.marshal_reasoning_effort = *reasoning;
      continue;
    }
    case workbench_marshal_prepare_action_t::Cancel:
      clear_workbench_marshal_launch_draft(st);
      set_workbench_status(st, "Cancelled Marshal launch.", false);
      return true;
    case workbench_marshal_prepare_action_t::Start:
      break;
    }

    std::ostringstream confirm;
    confirm << "Start Marshal from objective "
            << st.workbench.marshal_launch.objective_name << "?\n\n";
    confirm << "Model: "
            << workbench_marshal_model_label(
                   st, st.workbench.marshal_launch.marshal_model)
            << " ("
            << (st.workbench.marshal_launch.marshal_model.empty()
                    ? "objective/default"
                    : "launch override")
            << ")"
            << "\n";
    confirm << "Reasoning: "
            << workbench_marshal_reasoning_label(
                   st, st.workbench.marshal_launch.marshal_reasoning_effort)
            << " ("
            << (st.workbench.marshal_launch.marshal_reasoning_effort.empty()
                    ? "objective/default"
                    : "launch override")
            << ")"
            << "\n";
    confirm << "Objective DSL: "
            << st.workbench.marshal_launch.objective_dsl_path;
    const auto confirmed = prompt_workbench_yes_no(
        st, " Confirm Marshal launch ", confirm.str());
    if (!confirmed.has_value()) {
      continue;
    }
    if (!*confirmed) {
      continue;
    }

    set_workbench_status(
        st,
        "Starting Marshal session from " +
            st.workbench.marshal_launch.objective_name + "...",
        false);
    st.workbench.marshal_launch.launch_pending = true;
    st.workbench.marshal_launch.launch_status =
        "Starting Marshal session from " +
        st.workbench.marshal_launch.objective_name + "...";
    if (st.repaint_now) {
      st.repaint_now();
    }

    std::ostringstream args;
    args << "{\"marshal_objective_dsl_path\":"
         << config_json_quote(st.workbench.marshal_launch.objective_dsl_path);
    if (!st.workbench.marshal_launch.marshal_model.empty()) {
      args << ",\"marshal_codex_model\":"
           << config_json_quote(st.workbench.marshal_launch.marshal_model);
    }
    if (!st.workbench.marshal_launch.marshal_reasoning_effort.empty()) {
      args << ",\"marshal_codex_reasoning_effort\":"
           << config_json_quote(
                  st.workbench.marshal_launch.marshal_reasoning_effort);
    }
    args << "}";

    cmd_json_value_t structured{};
    std::string error{};
    bool ok = false;
    ui_run_blocking_dialog(
        " Starting Marshal ",
        "Starting Marshal session from the prepared objective bundle.",
        [&]() {
          ok = workbench_invoke_tool(st, "hero.marshal.start_session",
                                     args.str(), &structured, &error);
        });
    if (!ok) {
      st.workbench.marshal_launch.launch_pending = false;
      st.workbench.marshal_launch.launch_status.clear();
      set_workbench_status(st, error, true);
      return true;
    }

    st.workbench.marshal_launch.launch_pending = false;
    st.workbench.marshal_launch.launch_status.clear();
    refresh_workbench_booster_views(st, false);
    const std::string session_id =
        cmd_json_string(cmd_json_field(&structured, "marshal_session_id"));
    const std::string objective_name =
        st.workbench.marshal_launch.objective_name;
    clear_workbench_marshal_launch_draft(st);
    if (!session_id.empty()) {
      return workbench_show_runtime_session(
          st, session_id,
          "Marshal session " + session_id + " is starting from " +
              objective_name);
    }
    set_workbench_status(st,
                         "Started Marshal session from " + objective_name + ".",
                         false);
    return true;
  }
  return true;
}

inline bool launch_workbench_campaign(CmdState &st) {
  if (!st.workbench.campaign_launch.active) {
    const auto selection = resolve_workbench_booster_launch_source(
        st, "campaign.dsl", workbench_available_campaign_paths(st),
        workbench_preferred_default_campaign_path(st), " Temp campaign DSL ",
        "Booster can create a temp campaign.dsl from the default before "
        "launch. Edit the suggested temp-relative path if you want a "
        "different name.",
        workbench_suggest_temp_file_name("booster.campaign", ".campaign.dsl"));
    if (!selection.has_value()) {
      return true;
    }
    clear_workbench_campaign_launch_draft(st);
    workbench_stage_campaign_launch_draft(st, *selection);
    if (selection->created_temp) {
      return workbench_open_config_editor_for_path(
          st, selection->path,
          "temp campaign created | editor focus | Ctrl+S saves through Config "
          "Hero | return to Booster to review and launch it",
          true, workbench_booster_action_kind_t::LaunchCampaign,
          "Returned to Booster. Review the campaign and confirm launch.");
    }
  }

  while (st.workbench.campaign_launch.active) {
    const auto action = prompt_workbench_campaign_prepare_action(st);
    if (!action.has_value()) {
      return true;
    }

    switch (*action) {
    case workbench_campaign_prepare_action_t::EditCampaignDsl:
      return workbench_open_config_editor_for_path(
          st, st.workbench.campaign_launch.campaign_path,
          "editing campaign.dsl | Enter inserts newline | Esc opens "
          "save/discard | return to Booster to continue launch",
          true, workbench_booster_action_kind_t::LaunchCampaign,
          "Returned to Booster. Review the campaign and confirm launch.");
    case workbench_campaign_prepare_action_t::ChooseBinding: {
      const auto binding_id = prompt_workbench_campaign_binding_id(
          st, st.workbench.campaign_launch.campaign_path);
      if (!binding_id.has_value()) {
        continue;
      }
      st.workbench.campaign_launch.binding_id = *binding_id;
      continue;
    }
    case workbench_campaign_prepare_action_t::ToggleResetRuntimeState:
      st.workbench.campaign_launch.reset_runtime_state =
          !st.workbench.campaign_launch.reset_runtime_state;
      set_workbench_status(
          st,
          st.workbench.campaign_launch.reset_runtime_state
              ? "Runtime Hero will reset runtime state before this campaign "
                "launch."
              : "Runtime Hero will keep the current runtime state for this "
                "campaign launch.",
          false);
      continue;
    case workbench_campaign_prepare_action_t::Cancel:
      clear_workbench_campaign_launch_draft(st);
      set_workbench_status(st, "Cancelled.", false);
      return true;
    case workbench_campaign_prepare_action_t::Start:
      break;
    }

    std::ostringstream confirm_body;
    confirm_body << "Start Runtime campaign from this draft?\n\n";
    confirm_body << "Campaign DSL: "
                 << st.workbench.campaign_launch.campaign_path << "\n";
    confirm_body << "Binding: "
                 << workbench_campaign_binding_label(
                        st.workbench.campaign_launch.binding_id)
                 << "\n";
    confirm_body << "Reset runtime state: "
                 << workbench_campaign_reset_label(
                        st.workbench.campaign_launch);
    const auto confirmed = prompt_workbench_yes_no(
        st, " Confirm campaign launch ", confirm_body.str());
    if (!confirmed.has_value()) {
      continue;
    }
    if (!*confirmed) {
      continue;
    }

    set_workbench_status(
        st, "Starting Runtime campaign from " +
                st.workbench.campaign_launch.campaign_path + "...",
        false);

    std::ostringstream args;
    args << "{\"campaign_dsl_path\":"
         << config_json_quote(st.workbench.campaign_launch.campaign_path);
    if (!st.workbench.campaign_launch.binding_id.empty()) {
      args << ",\"binding_id\":"
           << config_json_quote(st.workbench.campaign_launch.binding_id);
    }
    if (st.workbench.campaign_launch.reset_runtime_state) {
      args << ",\"reset_runtime_state\":true";
    }
    args << "}";

    cmd_json_value_t structured{};
    std::string error{};
    bool ok = false;
    ui_run_blocking_dialog(
        " Starting campaign ",
        "Starting Runtime campaign from the selected draft.",
        [&]() {
          ok = workbench_invoke_tool(st, "hero.runtime.start_campaign",
                                     args.str(), &structured, &error);
        });
    if (!ok) {
      set_workbench_status(st, error, true);
      return true;
    }

    const std::string launched_path = st.workbench.campaign_launch.campaign_path;
    const bool created_temp = st.workbench.campaign_launch.created_temp;
    refresh_workbench_booster_views(st, created_temp);
    const std::string campaign_cursor =
        cmd_json_string(cmd_json_field(&structured, "campaign_cursor"));
    clear_workbench_campaign_launch_draft(st);
    if (!campaign_cursor.empty()) {
      return workbench_show_runtime_campaign(
          st, campaign_cursor,
          "Runtime campaign " + campaign_cursor + " is starting from " +
              launched_path);
    }
    set_workbench_status(st,
                         "Started Runtime campaign from " + launched_path + ".",
                         false);
    return true;
  }
  return true;
}

inline bool run_workbench_dev_nuke_reset(CmdState &st) {
  std::string confirmation{"DEV_NUKE_RESET"};
  bool cancelled = false;
  if (!ui_prompt_text_dialog(
          " Dev Nuke confirmation ",
          "Danger zone. Type DEV_NUKE_RESET exactly to continue with the "
          "runtime reset and Config Hero reload.",
          &confirmation, false, false, &cancelled)) {
    set_workbench_status(st, "failed to collect dev nuke confirmation", true);
    return true;
  }
  if (cancelled) {
    set_workbench_status(st, "Cancelled.", false);
    return true;
  }
  confirmation = trim_copy(confirmation);
  if (confirmation != "DEV_NUKE_RESET") {
    set_workbench_status(st, "Confirmation phrase did not match.", true);
    return true;
  }

  const auto confirmed = prompt_workbench_yes_no(
      st, " Confirm dev nuke ",
      "Proceed with dev_nuke_reset? This can snapshot and then clear runtime "
      "dump folders before reloading Config Hero state.");
  if (!confirmed.has_value()) {
    return true;
  }
  if (!*confirmed) {
    set_workbench_status(st, "Cancelled.", false);
    return true;
  }

  cmd_json_value_t structured{};
  std::string error{};
  bool ok = false;
  ui_run_blocking_dialog(
      " Dev nuke reset ",
      "Running dev_nuke_reset and reloading Config Hero state.",
      [&]() {
        ok = workbench_invoke_tool(st, "hero.config.dev_nuke_reset", "{}",
                                   &structured, &error);
      });
  if (!ok) {
    set_workbench_status(st, error, true);
    return true;
  }

  refresh_workbench_booster_views(st, true);
  set_workbench_status(st, "Completed dev_nuke_reset.", false);
  return true;
}

inline bool dispatch_workbench_booster_action(CmdState &st,
                                              workbench_booster_action_kind_t action) {
  switch (action) {
  case workbench_booster_action_kind_t::LaunchMarshal:
    return launch_workbench_marshal(st);
  case workbench_booster_action_kind_t::LaunchCampaign:
    return launch_workbench_campaign(st);
  case workbench_booster_action_kind_t::DevNukeReset:
    return run_workbench_dev_nuke_reset(st);
  }
  return false;
}
