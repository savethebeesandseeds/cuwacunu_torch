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
  push_unique(st.workbench.marshal_launch.objective_marshal_model);
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
      !st.workbench.marshal_launch.objective_marshal_model.empty()
          ? st.workbench.marshal_launch.objective_marshal_model
          : (st.runtime.marshal_app.defaults.marshal_codex_model.empty()
                 ? std::string("gpt-5.3-codex-spark")
                 : st.runtime.marshal_app.defaults.marshal_codex_model);
  std::vector<workbench_choice_value_t> choices{};
  choices.push_back(workbench_choice_value_t{
      .label = "Use objective/default (" + configured_default + ")",
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
      "Choose an optional Codex model override for this Marshal session. "
      "Use objective/default clears the override and lets the marshal "
      "objective DSL or default Marshal DSL decide.",
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
      !st.workbench.marshal_launch.objective_marshal_reasoning_effort.empty()
          ? st.workbench.marshal_launch.objective_marshal_reasoning_effort
          : (st.runtime.marshal_app.defaults.marshal_codex_reasoning_effort
                     .empty()
                 ? std::string("xhigh")
                 : st.runtime.marshal_app.defaults
                       .marshal_codex_reasoning_effort);
  std::vector<workbench_choice_value_t> choices{
      workbench_choice_value_t{
          .label = "Use objective/default (" + configured_default + ")",
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
      "Choose an optional Codex reasoning override for this Marshal session. "
      "Use objective/default clears the override and lets the marshal "
      "objective DSL or default Marshal DSL decide.",
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

