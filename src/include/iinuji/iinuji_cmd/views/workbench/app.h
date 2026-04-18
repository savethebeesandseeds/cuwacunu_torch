#pragma once

#include <cctype>
#include <filesystem>
#include <fstream>
#include <memory>
#include <ncursesw/ncurses.h>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "iinuji/iinuji_cmd/views/workbench/commands.h"
#include "iinuji/iinuji_cmd/views/common/action_menu.h"
#include "iinuji/iinuji_cmd/views/config/commands.h"
#include "iinuji/iinuji_cmd/views/lattice/commands.h"
#include "iinuji/iinuji_cmd/views/runtime/commands.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline void refresh_hero_shell_views(CmdState &st) {
  // Workbench actions should return control to the operator quickly. Refresh the
  // Workbench screen immediately, and let the other screens refresh when they are
  // focused again instead of forcing large runtime/lattice scans inline.
  (void)queue_workbench_refresh(st);
}

inline void refresh_workbench_booster_views(CmdState &st,
                                            bool refresh_config = false) {
  (void)queue_workbench_refresh(st);
  (void)queue_runtime_refresh(st);
  if (refresh_config) {
    (void)queue_config_refresh(st);
  }
}

inline bool handle_workbench_key(CmdState &st, int ch);

inline bool workbench_invoke_config_tool(CmdState &st,
                                         const std::string &tool_name,
                                         const std::string &arguments_json,
                                         cmd_json_value_t *out_structured,
                                         std::string *error) {
  const std::string global_path =
      cuwacunu::iitepi::config_space_t::config_file_path;
  const std::string policy_path =
      hero_config_path_from_global_key(global_path, "config_hero_dsl_filename");
  if (policy_path.empty()) {
    if (error) {
      *error = "missing [REAL_HERO].config_hero_dsl_filename";
    }
    return false;
  }

  const auto collect_roots = [&](const std::vector<std::string> &source,
                                 std::vector<std::string> *out) {
    for (const auto &root : source) {
      const std::string trimmed = trim_copy(root);
      if (!trimmed.empty()) {
        out->push_back(trimmed);
      }
    }
  };
  const auto dedupe_roots = [](std::vector<std::string> *roots) {
    std::vector<std::string> unique{};
    std::set<std::string> seen{};
    for (const auto &root : *roots) {
      if (seen.insert(root).second) {
        unique.push_back(root);
      }
    }
    *roots = std::move(unique);
  };
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

  std::string effective_policy_path = policy_path;
  if (tool_name.rfind("hero.config.objective.", 0u) == 0u ||
      tool_name.rfind("hero.config.temp.", 0u) == 0u) {
    const std::string backup_root = "/cuwacunu/.backups/hero.config";
    const std::string optim_backup_root = "/cuwacunu/.backups/hero.config.optim";
    std::vector<std::string> default_roots{};
    std::vector<std::string> objective_roots{};
    std::vector<std::string> temp_roots{};
    std::vector<std::string> write_roots{};
    collect_roots(st.config.default_roots, &default_roots);
    collect_roots(st.config.objective_roots, &objective_roots);
    collect_roots(st.config.temp_roots, &temp_roots);
    collect_roots(objective_roots, &write_roots);
    collect_roots(temp_roots, &write_roots);
    if (default_roots.empty()) {
      default_roots.push_back("/cuwacunu/src/config/instructions/defaults");
    }
    if (objective_roots.empty()) {
      objective_roots.push_back("/cuwacunu/src/config/instructions/objectives");
      write_roots.push_back(objective_roots.back());
    }
    if (temp_roots.empty()) {
      temp_roots.push_back("/cuwacunu/src/config/instructions/temp");
      write_roots.push_back(temp_roots.back());
    }
    write_roots.push_back(backup_root);
    write_roots.push_back(optim_backup_root);
    dedupe_roots(&default_roots);
    dedupe_roots(&objective_roots);
    dedupe_roots(&temp_roots);
    dedupe_roots(&write_roots);

    const std::filesystem::path booster_policy_path =
        std::filesystem::temp_directory_path() /
        "iinuji_cmd.workbench.booster.config.hero.dsl";
    std::ostringstream policy;
    policy << "protocol_layer[STDIO|HTTPS/SSE]:str = STDIO\n";
    policy << "config_scope_root:str = /cuwacunu/src/config\n";
    policy << "allow_local_read:bool = true\n";
    policy << "allow_local_write:bool = true\n";
    policy << "default_roots:str = " << join_csv(default_roots) << "\n";
    policy << "objective_roots:str = " << join_csv(objective_roots) << "\n";
    policy << "temp_roots:str = " << join_csv(temp_roots) << "\n";
    policy << "allowed_extensions:str = .dsl,.md\n";
    policy << "write_roots:str = " << join_csv(write_roots) << "\n";
    policy << "backup_enabled:bool = true\n";
    policy << "backup_dir:str = " << backup_root << "\n";
    policy << "backup_max_entries(1,+inf):int = 20\n";
    policy << "optim_backup_enabled:bool = true\n";
    policy << "optim_backup_dir:str = " << optim_backup_root << "\n";
    policy << "optim_backup_max_entries(1,+inf):int = 20\n";
    policy << "dev_nuke_reset_backup_enabled:bool = true\n";

    std::ofstream out(booster_policy_path);
    if (!out) {
      if (error) {
        *error = "failed to materialize Booster config write policy: " +
                 booster_policy_path.string();
      }
      return false;
    }
    out << policy.str();
    out.close();
    effective_policy_path = booster_policy_path.string();
  }

  cuwacunu::hero::mcp::hero_config_store_t store(effective_policy_path,
                                                 global_path);
  std::string load_error{};
  if (!store.load(&load_error)) {
    if (error) {
      *error = "failed to load config policy: " + load_error;
    }
    return false;
  }
  return config_invoke_tool(tool_name, arguments_json, &store, out_structured,
                            error);
}

inline bool workbench_invoke_tool(CmdState &st, const std::string &tool_name,
                                  const std::string &arguments_json,
                                  cmd_json_value_t *out_structured,
                                  std::string *error) {
  if (tool_name.rfind("hero.marshal.", 0u) == 0u) {
    return runtime_invoke_marshal_tool(st, tool_name, arguments_json,
                                       out_structured, error);
  }
  if (tool_name.rfind("hero.runtime.", 0u) == 0u) {
    return runtime_invoke_runtime_tool(st, tool_name, arguments_json,
                                       out_structured, error);
  }
  if (tool_name.rfind("hero.config.", 0u) == 0u) {
    return workbench_invoke_config_tool(st, tool_name, arguments_json,
                                        out_structured, error);
  }

  if (error) {
    *error = "unsupported Booster tool: " + tool_name;
  }
  return false;
}

enum class workbench_booster_source_mode_t {
  Default,
  Existing,
  TempFromDefault,
};

struct workbench_booster_source_selection_t {
  std::string path{};
  bool created_temp{false};
};

struct workbench_choice_value_t {
  std::string label{};
  std::string value{};
  bool enabled{true};
  std::string disabled_status{};
};

inline std::optional<workbench_booster_source_mode_t>
prompt_workbench_booster_source_mode(CmdState &st,
                                     const std::string &resource_label,
                                     bool has_default, bool has_existing) {
  std::vector<ui_choice_panel_option_t> options{
      ui_choice_panel_option_t{
          .label = "Use default " + resource_label,
          .enabled = has_default,
          .disabled_status =
              "No default " + resource_label + " is currently discoverable.",
      },
      ui_choice_panel_option_t{
          .label = "Choose existing " + resource_label,
          .enabled = has_existing,
          .disabled_status =
              "No existing " + resource_label + " files were discovered.",
      },
      ui_choice_panel_option_t{
          .label = "Create temp " + resource_label + " from default",
          .enabled = has_default,
          .disabled_status =
              "A default " + resource_label + " is required for temp copy.",
      },
  };

  std::size_t selected = has_default ? 0u : (has_existing ? 1u : 0u);
  bool cancelled = false;
  if (!ui_prompt_choice_panel(
          " Booster source ",
          "Choose how Booster should prepare the " + resource_label +
              " before launch.",
          options, &selected, &cancelled)) {
    set_workbench_status(st, "failed to collect Booster launch source", true);
    return std::nullopt;
  }
  if (cancelled) {
    set_workbench_status(st, "Cancelled.", false);
    return std::nullopt;
  }

  if (selected == 0u) {
    return workbench_booster_source_mode_t::Default;
  }
  if (selected == 1u) {
    return workbench_booster_source_mode_t::Existing;
  }
  return workbench_booster_source_mode_t::TempFromDefault;
}

inline std::optional<std::string>
prompt_workbench_booster_path_choice(CmdState &st, const std::string &title,
                                     const std::string &body,
                                     const std::vector<std::string> &paths,
                                     const std::string &preferred_path) {
  if (paths.empty()) {
    set_workbench_status(st, "No launchable files were discovered.", true);
    return std::nullopt;
  }
  std::vector<ui_choice_panel_option_t> options{};
  options.reserve(paths.size());
  std::size_t selected = 0u;
  for (std::size_t i = 0; i < paths.size(); ++i) {
    options.push_back(
        ui_choice_panel_option_t{.label = paths[i], .enabled = true});
    if (!preferred_path.empty() && paths[i] == preferred_path) {
      selected = i;
    }
  }

  bool cancelled = false;
  if (!ui_prompt_choice_panel(title, body, options, &selected, &cancelled)) {
    set_workbench_status(st, "failed to collect Booster file selection", true);
    return std::nullopt;
  }
  if (cancelled) {
    set_workbench_status(st, "Cancelled.", false);
    return std::nullopt;
  }
  return paths[std::min(selected, paths.size() - 1u)];
}

inline std::optional<std::string> prompt_workbench_choice_value(
    CmdState &st, const std::string &title, const std::string &body,
    const std::vector<workbench_choice_value_t> &choices,
    std::size_t default_index = 0u) {
  if (choices.empty()) {
    set_workbench_status(st, "No choices are available.", true);
    return std::nullopt;
  }
  std::vector<ui_choice_panel_option_t> options{};
  options.reserve(choices.size());
  bool has_enabled = false;
  std::size_t first_enabled = 0u;
  for (std::size_t i = 0; i < choices.size(); ++i) {
    options.push_back(ui_choice_panel_option_t{
        .label = choices[i].label,
        .enabled = choices[i].enabled,
        .disabled_status = choices[i].disabled_status,
    });
    if (choices[i].enabled && !has_enabled) {
      has_enabled = true;
      first_enabled = i;
    }
  }
  if (!has_enabled) {
    set_workbench_status(st, "No selectable choices are currently available.",
                         true);
    return std::nullopt;
  }
  std::size_t selected = std::min(default_index, choices.size() - 1u);
  if (!choices[selected].enabled) {
    selected = first_enabled;
  }
  bool cancelled = false;
  if (!ui_prompt_choice_panel(title, body, options, &selected, &cancelled)) {
    set_workbench_status(st, "failed to collect Booster choice", true);
    return std::nullopt;
  }
  if (cancelled) {
    set_workbench_status(st, "Cancelled.", false);
    return std::nullopt;
  }
  return choices[std::min(selected, choices.size() - 1u)].value;
}

inline std::optional<std::string> workbench_create_temp_copy_from_default(
    CmdState &st, const std::string &source_path, const std::string &title,
    const std::string &body, const std::string &suggested_relative_path) {
  if (source_path.empty()) {
    set_workbench_status(st, "No default file is available for temp copy.",
                         true);
    return std::nullopt;
  }
  const std::string source_text = workbench_read_file_text(source_path);
  if (source_text.empty()) {
    set_workbench_status(st, "failed to read default file for temp copy", true);
    return std::nullopt;
  }

  std::string relative_path = suggested_relative_path;
  bool cancelled = false;
  if (!ui_prompt_text_dialog(title, body, &relative_path, false, false,
                             &cancelled)) {
    set_workbench_status(st, "failed to collect temp file path", true);
    return std::nullopt;
  }
  if (cancelled) {
    set_workbench_status(st, "Cancelled.", false);
    return std::nullopt;
  }
  relative_path = trim_copy(relative_path);
  if (relative_path.empty()) {
    set_workbench_status(st, "Temp file path cannot be empty.", true);
    return std::nullopt;
  }

  const auto target_path =
      workbench_temp_absolute_path(st, relative_path).lexically_normal();
  std::ostringstream args;
  args << "{\"path\":" << config_json_quote(target_path.string())
       << ",\"content\":" << config_json_quote(source_text) << "}";
  cmd_json_value_t structured{};
  std::string error{};
  if (!workbench_invoke_tool(st, "hero.config.temp.create", args.str(),
                             &structured, &error)) {
    set_workbench_status(st, error, true);
    return std::nullopt;
  }

  refresh_workbench_booster_views(st, true);
  return cmd_json_string(cmd_json_field(&structured, "path"),
                         target_path.string());
}

inline std::optional<workbench_booster_source_selection_t>
resolve_workbench_booster_launch_source(
    CmdState &st, const std::string &resource_label,
    const std::vector<std::string> &paths, const std::string &default_path,
    const std::string &temp_dialog_title, const std::string &temp_dialog_body,
    const std::string &suggested_relative_path) {
  const auto source_mode = prompt_workbench_booster_source_mode(
      st, resource_label, !default_path.empty(), !paths.empty());
  if (!source_mode.has_value()) {
    return std::nullopt;
  }

  if (*source_mode == workbench_booster_source_mode_t::Default) {
    return workbench_booster_source_selection_t{
        .path = default_path,
        .created_temp = false,
    };
  }

  if (*source_mode == workbench_booster_source_mode_t::Existing) {
    const auto chosen = prompt_workbench_booster_path_choice(
        st, " Select file ",
        "Choose the " + resource_label + " Booster should launch.", paths,
        default_path);
    if (!chosen.has_value()) {
      return std::nullopt;
    }
    return workbench_booster_source_selection_t{
        .path = *chosen,
        .created_temp = false,
    };
  }

  const auto temp_path = workbench_create_temp_copy_from_default(
      st, default_path, temp_dialog_title, temp_dialog_body,
      suggested_relative_path);
  if (!temp_path.has_value()) {
    return std::nullopt;
  }
  return workbench_booster_source_selection_t{
      .path = *temp_path,
      .created_temp = true,
  };
}

inline bool prompt_workbench_optional_text(CmdState &st,
                                           const std::string &title,
                                           const std::string &body,
                                           std::string *value) {
  bool cancelled = false;
  if (!ui_prompt_text_dialog(title, body, value, false, true, &cancelled)) {
    set_workbench_status(st, "failed to collect Booster input", true);
    return false;
  }
  if (cancelled) {
    set_workbench_status(st, "Cancelled.", false);
    return false;
  }
  *value = trim_copy(*value);
  return true;
}

inline std::optional<std::string>
prompt_workbench_campaign_binding_id(CmdState &st,
                                     const std::string &campaign_path) {
  const auto binding_ids = workbench_campaign_binding_ids(campaign_path);
  if (binding_ids.empty()) {
    return std::string{};
  }

  std::vector<ui_choice_panel_option_t> options{};
  options.reserve(binding_ids.size() + 1u);
  options.push_back(ui_choice_panel_option_t{.label = "Use default RUN plan",
                                             .enabled = true});
  for (const auto &binding_id : binding_ids) {
    options.push_back(
        ui_choice_panel_option_t{.label = "Launch binding " + binding_id,
                                 .enabled = true});
  }

  std::size_t selected = 0u;
  bool cancelled = false;
  if (!ui_prompt_choice_panel(
          " Campaign binding ",
          "Select the launch target inside the chosen campaign.dsl. The "
          "default RUN plan keeps the campaign file in control.",
          options, &selected, &cancelled)) {
    set_workbench_status(st, "failed to collect campaign binding selection",
                         true);
    return std::nullopt;
  }
  if (cancelled) {
    set_workbench_status(st, "Cancelled.", false);
    return std::nullopt;
  }
  if (selected == 0u) {
    return std::string{};
  }
  return binding_ids[std::min(selected - 1u, binding_ids.size() - 1u)];
}

inline std::optional<bool>
prompt_workbench_yes_no(CmdState &st, const std::string &title,
                        const std::string &body) {
  bool confirmed = false;
  bool cancelled = false;
  if (!ui_prompt_yes_no_dialog(title, body, false, &confirmed, &cancelled)) {
    set_workbench_status(st, "failed to collect Booster confirmation", true);
    return std::nullopt;
  }
  if (cancelled) {
    set_workbench_status(st, "Cancelled.", false);
    return std::nullopt;
  }
  return confirmed;
}

inline std::vector<cuwacunu::hero::marshal::marshal_session_record_t>
workbench_fresh_marshal_sessions(CmdState &st, std::string *error) {
  std::vector<cuwacunu::hero::marshal::marshal_session_record_t> sessions{};
  if (!runtime_load_sessions_via_hero(st, &sessions, error)) {
    return {};
  }
  return sessions;
}

inline std::optional<cuwacunu::hero::marshal::marshal_session_record_t>
workbench_owner_session_for_objective(
    const std::vector<cuwacunu::hero::marshal::marshal_session_record_t> &sessions,
    const std::string &objective_path) {
  const auto normalized = std::filesystem::path(objective_path).lexically_normal();
  for (const auto &session : sessions) {
    if (cuwacunu::hero::marshal::is_marshal_session_terminal_state(
            session.lifecycle)) {
      continue;
    }
    const std::string owned_path =
        session.source_marshal_objective_dsl_path.empty()
            ? session.marshal_objective_dsl_path
            : session.source_marshal_objective_dsl_path;
    if (owned_path.empty()) {
      continue;
    }
    if (std::filesystem::path(owned_path).lexically_normal() == normalized) {
      return session;
    }
  }
  return std::nullopt;
}

inline bool workbench_show_runtime_session_owner(
    CmdState &st,
    const std::vector<cuwacunu::hero::marshal::marshal_session_record_t> &sessions,
    const cuwacunu::hero::marshal::marshal_session_record_t &owner) {
  st.runtime.sessions = sessions;
  st.runtime.ok = true;
  st.runtime.error.clear();
  clear_runtime_drill_filters(st);
  st.runtime.lane = kRuntimeSessionLane;
  st.runtime.family_anchor_lane = kRuntimeSessionLane;
  rebuild_runtime_lineage_cache(st);
  select_runtime_session_and_related(st, owner.marshal_session_id);
  focus_runtime_worklist(st);
  st.screen = ScreenMode::Runtime;
  (void)queue_runtime_refresh(st);
  set_runtime_status(
      st,
      ((owner.lifecycle == "live" && owner.activity == "review" &&
        owner.finish_reason == "interrupted")
           ? "Selected objective already belongs to resumable session "
           : "Selected objective already belongs to active session ") +
          owner.marshal_session_id,
      false);
  return true;
}

inline std::optional<std::string>
prompt_workbench_marshal_objective_choice(CmdState &st) {
  const auto objective_paths = workbench_available_objective_dsl_paths(st);
  if (objective_paths.empty()) {
    set_workbench_status(st, "No marshal objective bundles were discovered.",
                         true);
    return std::nullopt;
  }

  std::string session_error{};
  const auto sessions = workbench_fresh_marshal_sessions(st, &session_error);
  if (!session_error.empty()) {
    set_workbench_status(st, session_error, true);
    return std::nullopt;
  }

  std::vector<workbench_choice_value_t> choices{};
  choices.reserve(objective_paths.size());
  std::optional<cuwacunu::hero::marshal::marshal_session_record_t> lone_owner{};
  std::size_t disabled_count = 0u;
  for (const auto &path : objective_paths) {
    const auto owner = workbench_owner_session_for_objective(sessions, path);
    workbench_choice_value_t choice{};
    choice.label = workbench_objective_display_name(path);
    choice.value = path;
    if (owner.has_value()) {
      const bool resumable_review =
          owner->lifecycle == "live" && owner->activity == "review" &&
          owner->finish_reason == "interrupted";
      choice.enabled = false;
      choice.disabled_status =
          std::string(resumable_review ? "Owned by resumable session "
                                     : "Owned by active session ") +
          owner->marshal_session_id;
      ++disabled_count;
      if (!lone_owner.has_value()) {
        lone_owner = *owner;
      }
    }
    choices.push_back(std::move(choice));
  }

  if (disabled_count == choices.size()) {
    if (lone_owner.has_value()) {
      return workbench_show_runtime_session_owner(st, sessions, *lone_owner)
                 ? std::nullopt
                 : std::nullopt;
    }
    set_workbench_status(
        st, "All discovered marshal objectives are already owned by existing "
            "sessions.",
        false);
    return std::nullopt;
  }

  return prompt_workbench_choice_value(
      st, " Select objective ",
      "Choose the marshal objective bundle Booster should launch. Owned "
      "objectives are disabled so they cannot fail late at launch time.",
      choices);
}

inline std::vector<std::string>
workbench_known_marshal_models(const CmdState &st) {
  std::vector<std::string> models{};
  const auto push_unique = [&](const std::string &value) {
    if (value.empty()) {
      return;
    }
    for (const auto &existing : models) {
      if (existing == value) {
        return;
      }
    }
    models.push_back(value);
  };
  push_unique(st.runtime.marshal_app.defaults.marshal_codex_model);
  for (const auto &value : std::vector<std::string>{
           "gpt-5.4",
           "gpt-5.4-mini",
           "gpt-5.2",
           "gpt-5.2-codex",
           "gpt-5.1-codex-max",
           "gpt-5.1-codex-mini",
           "gpt-5.3-codex",
           "gpt-5.3-codex-spark",
       }) {
    push_unique(value);
  }
  return models;
}

inline std::optional<std::string>
prompt_workbench_marshal_model_override(CmdState &st) {
  const std::string configured_default =
      st.runtime.marshal_app.defaults.marshal_codex_model.empty()
          ? std::string("gpt-5.3-codex-spark")
          : st.runtime.marshal_app.defaults.marshal_codex_model;
  std::vector<workbench_choice_value_t> choices{};
  choices.push_back(workbench_choice_value_t{
      .label = "Use configured default (" + configured_default + ")",
      .value = "",
  });
  for (const auto &model : workbench_known_marshal_models(st)) {
    choices.push_back(workbench_choice_value_t{
        .label = model,
        .value = model,
    });
  }
  choices.push_back(workbench_choice_value_t{
      .label = "Custom model...",
      .value = "__custom__",
  });

  const auto selected = prompt_workbench_choice_value(
      st, " Marshal model ",
      "Choose the Codex model for this Marshal session. The configured default "
      "stays future-friendly, while Custom lets you type a newer slug.",
      choices);
  if (!selected.has_value()) {
    return std::nullopt;
  }
  if (*selected != "__custom__") {
    return *selected;
  }
  std::string custom{};
  if (!prompt_workbench_optional_text(
          st, " Custom marshal model ",
          "Enter the Marshal Codex model slug to override for this session.",
          &custom)) {
    return std::nullopt;
  }
  return custom;
}

inline std::optional<std::string>
prompt_workbench_marshal_reasoning_override(CmdState &st) {
  const std::string configured_default =
      st.runtime.marshal_app.defaults.marshal_codex_reasoning_effort.empty()
          ? std::string("xhigh")
          : st.runtime.marshal_app.defaults.marshal_codex_reasoning_effort;
  std::vector<workbench_choice_value_t> choices{
      workbench_choice_value_t{
          .label = "Use configured default (" + configured_default + ")",
          .value = "",
      },
      workbench_choice_value_t{.label = "low", .value = "low"},
      workbench_choice_value_t{.label = "medium", .value = "medium"},
      workbench_choice_value_t{.label = "high", .value = "high"},
      workbench_choice_value_t{.label = "xhigh", .value = "xhigh"},
      workbench_choice_value_t{.label = "Custom reasoning effort...",
                               .value = "__custom__"},
  };
  const auto selected = prompt_workbench_choice_value(
      st, " Marshal reasoning ",
      "Choose the Codex reasoning effort for this Marshal session.",
      choices);
  if (!selected.has_value()) {
    return std::nullopt;
  }
  if (*selected != "__custom__") {
    return *selected;
  }
  std::string custom{};
  if (!prompt_workbench_optional_text(
          st, " Custom marshal reasoning ",
          "Enter the reasoning effort override for this Marshal session.",
          &custom)) {
    return std::nullopt;
  }
  return custom;
}

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
  return st.runtime.marshal_app.defaults.marshal_codex_model.empty()
             ? std::string("gpt-5.3-codex-spark")
             : st.runtime.marshal_app.defaults.marshal_codex_model;
}

inline std::string workbench_marshal_reasoning_label(const CmdState &st,
                                                     const std::string &value) {
  if (!value.empty()) {
    return value;
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
  std::ostringstream body;
  body << "Objective: " << draft.objective_name << "\n";
  body << "Marshal model: "
       << workbench_marshal_model_label(st, draft.marshal_model) << "\n";
  body << "Reasoning: "
       << workbench_marshal_reasoning_label(st, draft.marshal_reasoning_effort)
       << "\n";
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
      ui_choice_panel_option_t{.label = "Choose model", .enabled = true},
      ui_choice_panel_option_t{.label = "Choose reasoning", .enabled = true},
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
            << "\n";
    confirm << "Reasoning: "
            << workbench_marshal_reasoning_label(
                   st, st.workbench.marshal_launch.marshal_reasoning_effort)
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

inline bool message_workbench_session(
    CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  std::string message{};
  bool cancelled = false;
  if (!ui_prompt_text_dialog(
          " Message session ",
          "Provide the operator message Marshal Hero should receive. "
          "Review-ready sessions wake immediately; other live sessions try "
          "direct thread delivery when continuity is available, otherwise "
          "they queue for the next safe point.",
          &message, false, false, &cancelled)) {
    set_workbench_status(st, "failed to collect operator message", true);
    return true;
  }
  if (cancelled) {
    set_workbench_status(st, "Cancelled.", false);
    return true;
  }
  std::string structured{};
  std::string error{};
  bool ok = false;
  ui_run_blocking_dialog(
      " Sending message ",
      "Dispatching operator message to Marshal Hero.",
      [&]() {
        ok = cuwacunu::hero::human_mcp::message_marshal_session(
            &st.workbench.app, session, message, &structured, &error);
      });
  if (!ok) {
    set_workbench_status(st, error, true);
    return true;
  }
  refresh_hero_shell_views(st);
  std::string status = "Delivered message to session.";
  bool status_is_error = false;
  try {
    const cmd_json_value_t parsed =
        cuwacunu::piaabo::JsonParser(structured).parse();
    const std::string delivery =
        cmd_json_string(cmd_json_field(&parsed, "delivery"));
    const std::string reply_text =
        cmd_json_string(cmd_json_field(&parsed, "reply_text"));
    const std::string warning =
        cmd_json_string(cmd_json_field(&parsed, "warning"));
    const std::string session_activity =
        cmd_json_string(cmd_json_field(&parsed, "session_activity"));
    const auto compact = [](std::string text) {
      text = trim_copy(text);
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
    if (delivery == "failed") {
      status_is_error = true;
      status = compact(warning.empty() ? "Marshal message delivery failed."
                                       : warning);
    } else if (delivery == "delivered" && !trim_copy(reply_text).empty()) {
      status = "Marshal: " + compact(reply_text);
    } else if (delivery == "queued" &&
               session_activity == "handling_message") {
      status = "Queued message for live thread delivery.";
    } else if (delivery == "queued") {
      status = "Queued message for the next safe point.";
    }
  } catch (...) {
  }
  set_workbench_status(st, std::move(status), status_is_error);
  return true;
}

inline bool answer_workbench_request(
    CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  std::string answer{};
  bool cancelled = false;
  if (!ui_prompt_text_dialog(
          " Answer request ",
          "Provide the clarification answer that Marshal Hero should continue "
          "with.",
          &answer, false, false, &cancelled)) {
    set_workbench_status(st, "failed to collect request answer", true);
    return true;
  }
  if (cancelled) {
    set_workbench_status(st, "Cancelled.", false);
    return true;
  }
  std::string structured{};
  std::string error{};
  bool ok = false;
  ui_run_blocking_dialog(
      " Applying answer ",
      "Submitting clarification answer to Marshal Hero.",
      [&]() {
        ok = cuwacunu::hero::human_mcp::build_request_answer_and_resume(
            &st.workbench.app, session, answer, &structured, &error);
      });
  if (!ok) {
    set_workbench_status(st, error, true);
    return true;
  }
  refresh_hero_shell_views(st);
  set_workbench_status(st, "Applied clarification answer.", false);
  return true;
}

inline bool collect_workbench_resolution_input(
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    bool stop_direct,
    cuwacunu::hero::human_mcp::interactive_resolution_input_t *out,
    bool *cancelled) {
  if (cancelled)
    *cancelled = false;
  if (out == nullptr)
    return false;
  *out = cuwacunu::hero::human_mcp::interactive_resolution_input_t{};

  if (stop_direct) {
    out->resolution_kind = "terminate";
  } else {
    std::vector<ui_choice_panel_option_t> options{
        ui_choice_panel_option_t{.label = "Grant requested governance",
                                 .enabled = true},
        ui_choice_panel_option_t{
            .label = "Clarify objective or policy without widening authority",
            .enabled = true},
        ui_choice_panel_option_t{.label = "Deny requested governance",
                                 .enabled = true},
        ui_choice_panel_option_t{.label = "Terminate the session",
                                 .enabled = true},
    };
    std::size_t resolution_idx = 0u;
    if (!ui_prompt_choice_panel(
            " Resolution ",
            "Choose how to answer this pending governance request.", options,
            &resolution_idx, cancelled)) {
      return false;
    }
    if (cancelled && *cancelled)
      return true;
    if (resolution_idx == 0u) {
      out->resolution_kind = "grant";
    } else if (resolution_idx == 1u) {
      out->resolution_kind = "clarify";
    } else if (resolution_idx == 2u) {
      out->resolution_kind = "deny";
    } else {
      out->resolution_kind = "terminate";
    }
  }

  const std::string prompt_title =
      (out->resolution_kind == "grant")     ? " Grant rationale "
      : (out->resolution_kind == "clarify") ? " Clarification "
      : (out->resolution_kind == "deny")    ? " Denial rationale "
                                            : " Termination rationale ";
  const std::string prompt_body =
      (out->resolution_kind == "grant")
          ? "Why grant this governance request? This reason is shown to "
            "Marshal Hero."
      : (out->resolution_kind == "clarify")
          ? "What clarification should Marshal Hero incorporate on the next "
            "planning checkpoint?"
      : (out->resolution_kind == "deny")
          ? "Why deny this governance request? This reason is shown to Marshal "
            "Hero."
          : "Why terminate this session? This reason is shown to Marshal Hero.";
  if (!ui_prompt_text_dialog(prompt_title, prompt_body, &out->reason, false,
                             false, cancelled)) {
    return false;
  }
  if (cancelled && *cancelled)
    return true;

  (void)session;
  return true;
}

inline bool apply_workbench_request_resolution(
    CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    bool stop_direct) {
  cuwacunu::hero::human_mcp::interactive_resolution_input_t input{};
  bool cancelled = false;
  if (!collect_workbench_resolution_input(session, stop_direct, &input,
                                      &cancelled)) {
    set_workbench_status(st, "failed to collect governance resolution input", true);
    return true;
  }
  if (cancelled) {
    set_workbench_status(st, "Cancelled.", false);
    return true;
  }
  std::string structured{};
  std::string error{};
  bool ok = false;
  ui_run_blocking_dialog(
      " Applying resolution ",
      "Submitting signed governance resolution to Marshal Hero.",
      [&]() {
        ok = cuwacunu::hero::human_mcp::build_resolution_and_apply(
            &st.workbench.app, session, input.resolution_kind, input.reason,
            &structured, &error);
      });
  if (!ok) {
    set_workbench_status(st, error, true);
    return true;
  }
  refresh_hero_shell_views(st);
  set_workbench_status(st, "Applied signed governance resolution.", false);
  return true;
}

inline bool acknowledge_workbench_review(
    CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  std::string note{};
  bool cancelled = false;
  if (!ui_prompt_text_dialog(
          " Acknowledge review ",
          "This does not continue the session. It records a signed "
          "acknowledgment and clears the review report from the inbox after "
          "review. Provide a required acknowledgment message:",
          &note, false, false, &cancelled)) {
    set_workbench_status(st, "failed to collect review acknowledgment note", true);
    return true;
  }
  if (cancelled) {
    set_workbench_status(st, "Cancelled.", false);
    return true;
  }
  std::string structured{};
  std::string error{};
  bool ok = false;
  ui_run_blocking_dialog(
      " Acknowledging review ",
      "Recording the signed review acknowledgment.",
      [&]() {
        ok = cuwacunu::hero::human_mcp::acknowledge_session_summary(
            &st.workbench.app, session, note, &structured, &error);
      });
  if (!ok) {
    set_workbench_status(st, error, true);
    return true;
  }
  refresh_hero_shell_views(st);
  set_workbench_status(st, "Acknowledged review report.", false);
  return true;
}

inline bool archive_workbench_review(
    CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  std::string note{};
  bool cancelled = false;
  if (!ui_prompt_text_dialog(
          " Archive review ",
          "This is final for this session. It records a signed archive "
          "message, marks the Marshal terminal, and releases the objective so "
          "Booster can launch a fresh Marshal for it. Provide a required "
          "archive message:",
          &note, false, false, &cancelled)) {
    set_workbench_status(st, "failed to collect review archive note", true);
    return true;
  }
  if (cancelled) {
    set_workbench_status(st, "Cancelled.", false);
    return true;
  }
  std::string structured{};
  std::string error{};
  bool ok = false;
  ui_run_blocking_dialog(
      " Archiving review ",
      "Recording archive note and releasing the objective bundle.",
      [&]() {
        ok = cuwacunu::hero::human_mcp::archive_session_summary(
            &st.workbench.app, session, note, &structured, &error);
      });
  if (!ok) {
    set_workbench_status(st, error, true);
    return true;
  }
  refresh_hero_shell_views(st);
  set_workbench_status(st, "Archived review and released objective.", false);
  return true;
}

inline bool
dispatch_workbench_popup_action(CmdState &st,
                                  const workbench_popup_action_t &action) {
  switch (action.kind) {
  case workbench_popup_action_kind_t::PauseSession: {
    const auto selected = selected_operator_visible_session_record(st);
    if (!selected.has_value())
      return false;
    std::string structured{};
    std::string error{};
    bool ok = false;
    ui_run_blocking_dialog(
        " Pausing session ",
        "Sending pause request to Marshal Hero.",
        [&]() {
          ok = cuwacunu::hero::human_mcp::pause_marshal_session(
              &st.workbench.app, *selected, false, &structured, &error);
        });
    if (!ok) {
      set_workbench_status(st, error, true);
      return true;
    }
    refresh_hero_shell_views(st);
    set_workbench_status(st, "Paused selected session.", false);
    return true;
  }
  case workbench_popup_action_kind_t::ResumeOperatorPause: {
    const auto selected = selected_operator_visible_session_record(st);
    if (!selected.has_value())
      return false;
    std::string structured{};
    std::string error{};
    bool ok = false;
    ui_run_blocking_dialog(
        " Resuming session ",
        "Sending resume request to Marshal Hero.",
        [&]() {
          ok = cuwacunu::hero::human_mcp::resume_marshal_session(
              &st.workbench.app, *selected, &structured, &error);
        });
    if (!ok) {
      set_workbench_status(st, error, true);
      return true;
    }
    refresh_hero_shell_views(st);
    set_workbench_status(st, "Resumed operator-paused session.", false);
    return true;
  }
  case workbench_popup_action_kind_t::MessageSession: {
    const auto selected = selected_operator_visible_session_record(st);
    if (!selected.has_value())
      return false;
    return message_workbench_session(st, *selected);
  }
  case workbench_popup_action_kind_t::TerminateSession: {
    const auto selected = selected_operator_visible_session_record(st);
    if (!selected.has_value())
      return false;
    bool confirmed = false;
    bool cancelled = false;
    if (!ui_prompt_yes_no_dialog(
            " Terminate session ",
            "Terminate the selected session? This action is final.", false,
            &confirmed, &cancelled)) {
      set_workbench_status(st, "failed to collect session termination confirmation",
                       true);
      return true;
    }
    if (cancelled || !confirmed) {
      set_workbench_status(st, "Cancelled.", false);
      return true;
    }
    std::string structured{};
    std::string error{};
    bool ok = false;
    ui_run_blocking_dialog(
        " Terminating session ",
        "Sending terminate request to Marshal Hero.",
        [&]() {
          ok = cuwacunu::hero::human_mcp::terminate_marshal_session(
              &st.workbench.app, *selected, false, &structured, &error);
        });
    if (!ok) {
      set_workbench_status(st, error, true);
      return true;
    }
    refresh_hero_shell_views(st);
    set_workbench_status(st, "Terminated selected session.", false);
    return true;
  }
  case workbench_popup_action_kind_t::AnswerRequest: {
    const auto selected = selected_workbench_request_record(st);
    if (!selected.has_value())
      return false;
    return answer_workbench_request(st, *selected);
  }
  case workbench_popup_action_kind_t::ResolveRequest: {
    const auto selected = selected_workbench_request_record(st);
    if (!selected.has_value())
      return false;
    return apply_workbench_request_resolution(st, *selected, false);
  }
  case workbench_popup_action_kind_t::TerminateGovernanceRequest: {
    const auto selected = selected_workbench_request_record(st);
    if (!selected.has_value())
      return false;
    return apply_workbench_request_resolution(st, *selected, true);
  }
  case workbench_popup_action_kind_t::AcknowledgeReview: {
    const auto selected = selected_workbench_review_record(st);
    if (!selected.has_value())
      return false;
    return acknowledge_workbench_review(st, *selected);
  }
  case workbench_popup_action_kind_t::ArchiveReview: {
    const auto selected = selected_workbench_review_record(st);
    if (!selected.has_value())
      return false;
    return archive_workbench_review(st, *selected);
  }
  }
  return false;
}

inline bool open_workbench_action_menu(CmdState &st) {
  const auto actions = workbench_popup_actions(st);
  if (actions.empty()) {
    set_workbench_status(st, "No actions available for the selected row.", false);
    return true;
  }

  std::size_t selected = 0u;
  bool cancelled = false;
  if (!prompt_action_choice_panel("Workbench Actions",
                                  "Select workbench action.", actions,
                                  &selected, &cancelled)) {
    set_workbench_status(st, "failed to open action menu", true);
    return true;
  }
  if (cancelled) {
    set_workbench_status(st, {}, false);
    return true;
  }
  const auto &action = selected_action_or_last(actions, selected);
  if (!action.enabled) {
    set_workbench_status(st,
                     action.disabled_status.empty()
                         ? "Selected action is currently unavailable."
                         : action.disabled_status,
                     false);
    return true;
  }
  return dispatch_workbench_popup_action(st, action);
}

inline bool handle_workbench_key(CmdState &st, int ch) {
  if (st.screen != ScreenMode::Workbench)
    return false;
  if ((ch == 27 || ch == KEY_LEFT || ch == 'h') &&
      workbench_is_worklist_focus(st.workbench.focus)) {
    focus_workbench_navigation(st);
    return true;
  }
  if (workbench_is_navigation_focus(st.workbench.focus)) {
    if (ch == KEY_UP || ch == 'k') {
      return select_prev_workbench_section(st);
    }
    if (ch == KEY_DOWN || ch == 'j') {
      return select_next_workbench_section(st);
    }
    if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
      focus_workbench_worklist(st);
      return true;
    }
    if (ch == KEY_RIGHT || ch == 'l') {
      focus_workbench_worklist(st);
      return true;
    }
    return false;
  }

  if (workbench_is_booster_section(st.workbench.section)) {
    if (ch == KEY_UP || ch == 'k') {
      return select_prev_workbench_booster_action(st);
    }
    if (ch == KEY_DOWN || ch == 'j') {
      return select_next_workbench_booster_action(st);
    }
    if (ch == '\n' || ch == '\r' || ch == KEY_ENTER || ch == 'a') {
      return dispatch_workbench_booster_action(
          st, selected_workbench_booster_action(st));
    }
    return false;
  }

  if (ch == KEY_UP || ch == 'k')
    return select_prev_workbench_row(st);
  if (ch == KEY_DOWN || ch == 'j')
    return select_next_workbench_row(st);
  if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
    return open_workbench_action_menu(st);
  }
  return false;
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
