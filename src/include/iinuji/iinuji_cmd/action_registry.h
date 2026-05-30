#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <functional>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "iinuji/iinuji_cmd/state.h"
#include "iinuji/iinuji_cmd/system_snapshot.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline std::string command_identity_title(std::string_view command_name) {
  std::string label =
      command_name.empty() ? "cuwacunu_cmd" : std::string(command_name);
  return label + " iinuji interface";
}

inline std::array<std::string, 4>
command_identity_lines(std::string_view command_name = "cuwacunu_cmd") {
  return {{
      command_identity_title(command_name),
      "aliases: cuwacunu_cmd, cuwacunu.cmd, iinuji_cmd",
      "screens: F1 Home | F2 Workbench | F3 Runtime | F4 Lattice | F8 "
      "Shell Logs | F9 Config",
      "workbench: F2 core workspace",
  }};
}

enum class ActionId : std::uint8_t {
  ShowHome,
  HomeVisual,
  HomeSplash,
  HomeFarewell,
  ShowWorkbench,
  ShowRuntime,
  ShowLattice,
  ShowConfig,
  ShowLogs,
  ShowCurrent,
  ShowIdentity,
  ShowHomeSummary,
  ShowWorkbenchSummary,
  ShowRuntimeSummary,
  ShowLatticeSummary,
  ShowLogsSummary,
  ShowConfigSummary,
  Help,
  CloseHelp,
  HelpScrollUp,
  HelpScrollDown,
  HelpScrollLeft,
  HelpScrollRight,
  HelpScrollPageUp,
  HelpScrollPageDown,
  HelpScrollHome,
  HelpScrollEnd,
  ActionMenu,
  ActionMenuClose,
  ActionMenuSelectPrev,
  ActionMenuSelectNext,
  ActionMenuSelectPageUp,
  ActionMenuSelectPageDown,
  ActionMenuSelectHome,
  ActionMenuSelectEnd,
  Refresh,
  WorkbenchRefresh,
  RuntimeRefresh,
  LatticeRefresh,
  ConfigReload,
  ClearLogs,
  LogsScrollUp,
  LogsScrollDown,
  LogsScrollPageUp,
  LogsScrollPageDown,
  LogsScrollHome,
  LogsScrollEnd,
  ToggleLogsFollow,
  ToggleLogsTimestamp,
  ToggleLogsThread,
  ToggleLogsMetadata,
  ToggleLogsColor,
  ToggleLogsMouseCapture,
  CycleLogsSourceFilter,
  SetLogsSourceAll,
  SetLogsSourceRefresh,
  SetLogsSourceAction,
  SetLogsSourceCommand,
  SetLogsSourceShow,
  SetLogsSourceStatus,
  CycleLogsSeverityFilter,
  CycleLogsMetadataFilter,
  SetLogsSeverityDebug,
  SetLogsSeverityInfo,
  SetLogsSeverityWarning,
  SetLogsSeverityError,
  SetLogsSeverityFatal,
  SetLogsMetadataAny,
  SetLogsMetadataAnyMeta,
  SetLogsMetadataFunction,
  SetLogsMetadataPath,
  SetLogsMetadataCallsite,
  LogsSelectSettingPrev,
  LogsSelectSettingNext,
  LogsAdjustSettingPrev,
  LogsAdjustSettingNext,
  RuntimeFocusDevices,
  RuntimeFocusJobs,
  RuntimeFocusPrev,
  RuntimeFocusNext,
  RuntimeDetailNext,
  RuntimeDetailManifest,
  RuntimeDetailLog,
  RuntimeDetailTrace,
  RuntimeItemPrev,
  RuntimeItemNext,
  ConfigFiles,
  ConfigShow,
  ConfigFileShow,
  ConfigFilePrev,
  ConfigFileNext,
  ConfigFileFirst,
  ConfigFileLast,
  LatticeTargetPrev,
  LatticeTargetNext,
  LatticeTargetFirst,
  LatticeTargetLast,
  ToggleWorkspaceZoom,
  RestoreWorkspaceSplit,
  Quit,
  Exit,
};

struct Action {
  ActionId id{};
  std::string label{};
  std::vector<std::string> aliases{};
  std::function<void(CmdState &)> apply{};
};

inline std::string normalize_command(std::string command) {
  command = trim(std::move(command));
  if (!command.empty() && command.front() == ':')
    command.erase(command.begin());
  std::string compacted{};
  compacted.reserve(command.size());
  bool last_space = false;
  for (unsigned char ch : command) {
    if (std::isspace(ch)) {
      if (!last_space)
        compacted.push_back(' ');
      last_space = true;
      continue;
    }
    compacted.push_back(static_cast<char>(std::tolower(ch)));
    last_space = false;
  }
  command = trim(std::move(compacted));
  if (command.size() >= 2u && command.substr(command.size() - 2u) == "()")
    command.resize(command.size() - 2u);
  return command;
}

inline bool command_starts_with(const std::string &command,
                                const std::string &prefix) {
  return command.rfind(prefix, 0) == 0;
}

inline const std::array<std::string_view, 3> &shell_command_prefixes() {
  static constexpr std::array<std::string_view, 3> prefixes{
      "cuwacunu_cmd ", "cuwacunu.cmd ", "iinuji_cmd "};
  return prefixes;
}

inline std::optional<std::string>
payload_after_shell_command_prefix(const std::string &command) {
  for (const std::string_view prefix : shell_command_prefixes()) {
    if (command.rfind(prefix, 0) == 0)
      return command.substr(prefix.size());
  }
  return std::nullopt;
}

inline std::optional<std::string>
command_name_from_shell_command_prefix(const std::string &command) {
  for (const std::string_view prefix : shell_command_prefixes()) {
    if (command.rfind(prefix, 0) == 0)
      return std::string(prefix.substr(0, prefix.size() - 1u));
  }
  return std::nullopt;
}

inline std::string command_token_value(std::string token) {
  token = trim(std::move(token));
  const std::size_t eq = token.find('=');
  if (eq != std::string::npos)
    token = trim(token.substr(eq + 1u));
  if (token.size() >= 2u && ((token.front() == '"' && token.back() == '"') ||
                             (token.front() == '\'' && token.back() == '\''))) {
    token = token.substr(1u, token.size() - 2u);
  }
  return trim(std::move(token));
}

inline bool parse_one_based_command_index(std::string token,
                                          std::size_t &index) {
  token = command_token_value(std::move(token));
  if (command_starts_with(token, "idx")) {
    token.erase(0, 3u);
  } else if (!token.empty() && (token.front() == 'n' || token.front() == 'i' ||
                                token.front() == 'v')) {
    token.erase(token.begin());
  }
  if (token.empty())
    return false;

  std::size_t value = 0;
  for (unsigned char ch : token) {
    if (!std::isdigit(ch))
      return false;
    const std::size_t digit = static_cast<std::size_t>(ch - '0');
    if (value > (std::numeric_limits<std::size_t>::max() - digit) / 10u) {
      return false;
    }
    value = (value * 10u) + digit;
  }
  if (value == 0u)
    return false;
  index = value - 1u;
  return true;
}

inline std::string config_id_token(std::string value) {
  value = command_token_value(std::move(value));
  value = normalize_command(std::move(value));
  std::replace(value.begin(), value.end(), '\\', '/');
  while (command_starts_with(value, "./"))
    value.erase(0, 2);
  return value;
}

inline bool managed_config_path_extension(std::string extension) {
  extension = normalize_command(std::move(extension));
  static constexpr std::array<std::string_view, 6> kManagedConfigExtensions{
      {".dsl", ".bnf", ".man", ".md", ".jkimyei", ".net"}};
  return std::find(kManagedConfigExtensions.begin(),
                   kManagedConfigExtensions.end(),
                   extension) != kManagedConfigExtensions.end();
}

inline bool command_looks_like_managed_config_path(std::string_view command) {
  const std::string token = command_token_value(std::string(command));
  if (token.empty())
    return false;
  const std::filesystem::path path{token};
  return managed_config_path_extension(path.extension().string());
}

inline bool config_token_matches_candidate(const std::string &normalized_token,
                                           const std::string &candidate) {
  const std::string normalized_candidate = config_id_token(candidate);
  if (normalized_candidate.empty())
    return false;
  if (normalized_token == normalized_candidate)
    return true;
  return normalized_token.size() > normalized_candidate.size() &&
         normalized_token.compare(
             normalized_token.size() - normalized_candidate.size(),
             normalized_candidate.size(), normalized_candidate) == 0 &&
         normalized_token[normalized_token.size() -
                          normalized_candidate.size() - 1u] == '/';
}

inline std::string compact_config_id_token(std::string value) {
  value = config_id_token(std::move(value));
  std::string out{};
  out.reserve(value.size());
  for (unsigned char ch : value) {
    if (std::isalnum(ch))
      out.push_back(static_cast<char>(ch));
  }
  return out;
}

inline bool config_file_matches_id_token(const ConfigFileSummary &file,
                                         const std::string &token) {
  const std::string normalized_token = config_id_token(token);
  const std::string compact_token = compact_config_id_token(token);
  std::vector<std::string> candidates{file.relative_path, file.path.string(),
                                      file.path.filename().string(),
                                      file.path.stem().string()};
  std::error_code ec{};
  const std::filesystem::path cwd = std::filesystem::current_path(ec);
  if (!ec)
    candidates.push_back(file.path.lexically_relative(cwd).string());
  for (const auto &candidate : candidates) {
    if (config_token_matches_candidate(normalized_token, candidate))
      return true;
    if (!compact_token.empty() &&
        compact_config_id_token(candidate) == compact_token)
      return true;
  }
  return false;
}

inline bool lattice_target_matches_id_token(const std::string &target_id,
                                            const std::string &token) {
  const std::string normalized_token = config_id_token(token);
  const std::string compact_token = compact_config_id_token(token);
  const std::filesystem::path target_path{target_id};
  const std::vector<std::string> candidates{
      target_id, target_path.filename().string(), target_path.stem().string()};
  for (const auto &candidate : candidates) {
    if (config_id_token(candidate) == normalized_token)
      return true;
    if (!compact_token.empty() &&
        compact_config_id_token(candidate) == compact_token)
      return true;
  }
  return false;
}

inline bool select_config_file_by_index(CmdState &state, std::size_t index) {
  state.screen = ScreenMode::Config;
  if (state.config.files.empty() || index >= state.config.files.size()) {
    state.status = "config file index out of range";
    return true;
  }
  state.config.selected_file = index;
  state.status = selected_config_file_status(state);
  return true;
}

inline bool select_config_file_by_index_token(CmdState &state,
                                              const std::string &token) {
  std::size_t index = 0;
  if (!parse_one_based_command_index(token, index)) {
    state.screen = ScreenMode::Config;
    state.status = "config file index invalid";
    return true;
  }
  return select_config_file_by_index(state, index);
}

inline bool select_config_file_by_id_token(CmdState &state,
                                           const std::string &token) {
  state.screen = ScreenMode::Config;
  if (trim(token).empty()) {
    state.status = "config file id missing";
    return true;
  }
  for (std::size_t i = 0; i < state.config.files.size(); ++i) {
    if (!config_file_matches_id_token(state.config.files[i], token))
      continue;
    state.config.selected_file = i;
    state.status = selected_config_file_status(state);
    return true;
  }
  state.status = "config file id not found";
  return true;
}

inline bool select_config_file_by_token(CmdState &state,
                                        const std::string &token) {
  std::size_t index = 0;
  if (parse_one_based_command_index(token, index))
    return select_config_file_by_index(state, index);
  return select_config_file_by_id_token(state, token);
}

inline bool select_lattice_target_by_index(CmdState &state, std::size_t index) {
  state.screen = ScreenMode::Lattice;
  if (state.lattice.target_ids.empty() ||
      index >= state.lattice.target_ids.size()) {
    state.status = "lattice target index out of range";
    return true;
  }
  state.lattice.selected_target = index;
  state.status = "lattice target " + std::to_string(index + 1u) + "/" +
                 std::to_string(state.lattice.target_ids.size());
  return true;
}

inline bool select_lattice_target_by_index_token(CmdState &state,
                                                 const std::string &token) {
  std::size_t index = 0;
  if (!parse_one_based_command_index(token, index)) {
    state.screen = ScreenMode::Lattice;
    state.status = "lattice target index invalid";
    return true;
  }
  return select_lattice_target_by_index(state, index);
}

inline bool select_lattice_target_by_id_token(CmdState &state,
                                              const std::string &token) {
  state.screen = ScreenMode::Lattice;
  if (trim(token).empty()) {
    state.status = "lattice target id missing";
    return true;
  }
  for (std::size_t i = 0; i < state.lattice.target_ids.size(); ++i) {
    if (!lattice_target_matches_id_token(state.lattice.target_ids[i], token))
      continue;
    state.lattice.selected_target = i;
    state.status = "lattice target " + state.lattice.target_ids[i];
    return true;
  }
  state.status = "lattice target id not found";
  return true;
}

inline bool select_lattice_target_by_token(CmdState &state,
                                           const std::string &token) {
  std::size_t index = 0;
  if (parse_one_based_command_index(token, index))
    return select_lattice_target_by_index(state, index);
  return select_lattice_target_by_id_token(state, token);
}

inline std::string show_screen_hint_line() {
  return "hint=F2 workbench | F3 runtime | F4 lattice | F8 shell logs | F9 "
         "config";
}

inline const RuntimeJobSummary *
show_selected_runtime_job(const CmdState &state) {
  if (state.runtime.jobs.empty() ||
      state.runtime.selected_job >= state.runtime.jobs.size())
    return nullptr;
  return &state.runtime.jobs[state.runtime.selected_job];
}

inline void append_show_current_screen_logs(CmdState &state) {
  switch (state.screen) {
  case ScreenMode::Home:
    append_log(state, "show", "screen=home");
    append_log(state, "show", show_screen_hint_line());
    append_log(state, "show", "site=waajacu.com");
    state.status = "show home";
    break;
  case ScreenMode::Workbench:
    append_log(state, "show", "screen=workbench");
    append_log(state, "show", "status=empty");
    append_log(state, "show", "workbench=empty");
    state.status = "show workbench";
    break;
  case ScreenMode::Runtime: {
    append_log(state, "show", "screen=runtime");
    append_log(state, "show",
               "focus=" + runtime_focus_label(state.runtime.focus));
    append_log(state, "show",
               "detail_mode=" +
                   runtime_detail_mode_label(state.runtime.detail_mode));
    append_log(
        state, "show",
        "jobs=" + std::to_string(state.runtime.jobs.size()) +
            " gpus=" + std::to_string(state.runtime.device.gpus.size()) +
            " samples=" + std::to_string(state.runtime.device_history.size()));
    if (show_selected_runtime_job(state) != nullptr)
      append_log(state, "show",
                 "selected_job=" + show_selected_runtime_job(state)->job_id);
    state.status = "show runtime";
    break;
  }
  case ScreenMode::Lattice:
    append_log(state, "show", "screen=lattice");
    append_log(state, "show",
               "targets=" + std::to_string(state.lattice.target_ids.size()));
    append_log(
        state, "show",
        "exposure_facts=" + std::to_string(state.lattice.exposure_fact_count) +
            " checkpoints=" +
            std::to_string(state.lattice.checkpoint_fact_count));
    if (selected_lattice_target_id(state) != nullptr)
      append_log(state, "show",
                 "selected_target=" + *selected_lattice_target_id(state));
    state.status = "show lattice";
    break;
  case ScreenMode::Logs:
    append_log(state, "show", "screen=shell_logs");
    append_log(state, "show", "entries=" + std::to_string(state.log.size()));
    append_log(
        state, "show",
        "source=" + logs_source_filter_label(state.logs.source_filter) +
            " level=" + logs_severity_filter_label(state.logs.severity_filter));
    append_log(state, "show", "hint=iinuji.logs.clear()");
    state.status = "show shell logs";
    break;
  case ScreenMode::Config:
    append_log(state, "show", "screen=config");
    append_log(state, "show",
               "files=" + std::to_string(state.config.files.size()));
    if (selected_config_file(state) != nullptr) {
      append_log(state, "show",
                 "relative=" + selected_config_file(state)->relative_path);
      append_log(state, "show",
                 "path=" + selected_config_file(state)->path.string());
    }
    state.status = "show config";
    break;
  }
}

inline void append_home_splash_logs(CmdState &state, bool farewell) {
  append_log(state, "show", farewell ? "splash=farewell" : "splash=bootstrap");
  append_log(state, "show",
             "loading_logo=" + state.home_visual.loading_logo_path.string());
  append_log(state, "show",
             "closing_logo=" + state.home_visual.closing_logo_path.string());
  append_log(state, "show",
             "home_animation=" +
                 state.home_visual.home_animation_path.string());
  append_log(
      state, "show",
      "animation_frames=" + std::to_string(state.home_visual.animation_frames) +
          " dynamic=" + (state.home_visual.animation_dynamic ? "yes" : "no"));
  if (!state.home_visual.loading_logo_error.empty())
    append_log(state, "show",
               "loading_logo_error=" + state.home_visual.loading_logo_error);
  if (!state.home_visual.home_animation_error.empty())
    append_log(state, "show",
               "home_animation_error=" +
                   state.home_visual.home_animation_error);
  state.status = farewell ? "farewell splash" : "bootstrap splash";
}

inline bool command_tail_after_prefix(const std::string &command,
                                      const std::string &prefix,
                                      std::string &tail) {
  if (!command_starts_with(command, prefix))
    return false;
  tail = trim(command.substr(prefix.size()));
  return true;
}

inline bool command_call_payload(const std::string &command,
                                 const std::string &call,
                                 std::string &payload) {
  const std::string prefix = call + "(";
  if (!command_starts_with(command, prefix) || command.back() != ')')
    return false;
  payload = command_token_value(
      command.substr(prefix.size(), command.size() - prefix.size() - 1u));
  return true;
}

class ActionRegistry {
public:
  void add(Action action) {
    const std::size_t index = actions_.size();
    for (const auto &alias : action.aliases)
      alias_index_[normalize_command(alias)] = index;
    actions_.push_back(std::move(action));
  }

  bool dispatch(ActionId id, CmdState &state) const {
    for (const auto &action : actions_) {
      if (action.id != id)
        continue;
      if (id == ActionId::ActionMenu) {
        open_action_menu(state);
        append_log(state, "action", action.label);
        return true;
      }
      close_choice_menu(state);
      close_content_popup(state);
      action.apply(state);
      append_log(state, "action", action.label);
      return true;
    }
    return false;
  }

  bool dispatch(std::string command, CmdState &state) const {
    command = normalize_command(std::move(command));
    if (command.empty())
      return false;
    if (const auto shell_result =
            dispatch_shell_utility_command(command, state);
        shell_result.has_value()) {
      append_log(state, "command", command);
      return *shell_result;
    }
    if (dispatch_action_menu_command(command, state)) {
      append_log(state, "command", command);
      return true;
    }
    const auto it = alias_index_.find(command);
    if (it == alias_index_.end()) {
      if (dispatch_pattern(command, state)) {
        append_log(state, "command", command);
        return true;
      }
      state.status = "unknown command: " + command;
      append_log(state, "command", state.status);
      return false;
    }
    const auto &action = actions_[it->second];
    if (action.id == ActionId::ActionMenu) {
      open_action_menu(state);
      append_log(state, "command", command);
      return true;
    }
    close_choice_menu(state);
    close_content_popup(state);
    action.apply(state);
    append_log(state, "command", command);
    return true;
  }

  const std::vector<Action> &actions() const { return actions_; }

  void open_action_menu(CmdState &state) const {
    state.help_view = false;
    state.action_menu_open = true;
    close_choice_menu(state);
    close_content_popup(state);
    state.action_menu_scroll_y = 0;
    if (const auto index = contextual_action_menu_index(state);
        index.has_value()) {
      state.action_menu_selected = *index;
    }
    state.status = "action menu";
  }

private:
  std::optional<bool> dispatch_shell_utility_command(const std::string &command,
                                                     CmdState &state) const {
    const std::optional<std::string> rest_value =
        payload_after_shell_command_prefix(command);
    if (!rest_value.has_value())
      return std::nullopt;

    if (const auto command_name =
            command_name_from_shell_command_prefix(command);
        command_name.has_value())
      state.command_name = *command_name;

    const std::string rest = trim(*rest_value);
    const auto strip_snapshot_suffix = [](std::string value) {
      value = trim(std::move(value));
      static constexpr std::string_view kSnapshotSuffix{" --snapshot"};
      if (value.size() >= kSnapshotSuffix.size() &&
          value.substr(value.size() - kSnapshotSuffix.size()) ==
              kSnapshotSuffix) {
        value.resize(value.size() - kSnapshotSuffix.size());
      }
      return trim(std::move(value));
    };
    const auto strip_snapshot_prefix = [](std::string value) {
      value = trim(std::move(value));
      static constexpr std::string_view kSnapshotPrefix{"--snapshot "};
      if (value.rfind(kSnapshotPrefix, 0) == 0)
        value.erase(0, kSnapshotPrefix.size());
      return trim(std::move(value));
    };
    const auto strip_snapshot_wrapper = [&](std::string value) {
      return strip_snapshot_prefix(strip_snapshot_suffix(std::move(value)));
    };
    const auto dispatch_payload = [&](std::string payload) {
      payload = command_token_value(strip_snapshot_suffix(std::move(payload)));
      if (payload.empty()) {
        state.status = "unknown command: " + command;
        return false;
      }
      const bool ok = dispatch(std::move(payload), state);
      if (!ok)
        state.status = "unknown command: " + command;
      return ok;
    };
    const auto dispatch_payload_then = [&](std::string payload, ActionId id) {
      payload = command_token_value(strip_snapshot_suffix(std::move(payload)));
      if (!payload.empty() && !dispatch(std::move(payload), state)) {
        state.status = "unknown command: " + command;
        return false;
      }
      return dispatch(id, state);
    };
    const std::string rest_without_snapshot_prefix =
        strip_snapshot_prefix(rest);
    const auto payload_after_from =
        [](const std::string &source,
           std::string_view prefix) -> std::optional<std::string> {
      const std::string prefix_text{prefix};
      if (source.rfind(prefix_text, 0) != 0)
        return std::nullopt;
      return trim(source.substr(prefix_text.size()));
    };
    const auto payload_after =
        [&](std::string_view prefix) -> std::optional<std::string> {
      if (const auto payload = payload_after_from(rest, prefix);
          payload.has_value())
        return payload;
      if (rest_without_snapshot_prefix != rest)
        return payload_after_from(rest_without_snapshot_prefix, prefix);
      return std::nullopt;
    };
    const std::string utility_rest = strip_snapshot_wrapper(rest);

    if (utility_rest == "--menu")
      return dispatch(ActionId::Help, state);
    if (utility_rest == "--help" || utility_rest == "-h")
      return dispatch(ActionId::Help, state);
    if (utility_rest == "--version" || utility_rest == "-v")
      return dispatch(ActionId::ShowIdentity, state);
    if (utility_rest == "--actions" || utility_rest == "--palette" ||
        utility_rest == "--commands" || utility_rest == "--catalog")
      return dispatch(ActionId::ActionMenu, state);
    if (rest == "--snapshot")
      return dispatch(ActionId::ShowCurrent, state);
    if (utility_rest == "--zoom" || utility_rest == "--full")
      return dispatch(ActionId::ToggleWorkspaceZoom, state);
    if (utility_rest == "--visual" || utility_rest == "--home-visual" ||
        utility_rest == "--image" || utility_rest == "--animation" ||
        utility_rest == "--waajacamaya")
      return dispatch(ActionId::HomeVisual, state);
    if (utility_rest == "--splash" || utility_rest == "--splash=bootstrap" ||
        utility_rest == "--splash=boot" || utility_rest == "--splash=loading" ||
        utility_rest == "--splash=load" ||
        utility_rest == "--splash bootstrap" ||
        utility_rest == "--splash boot" || utility_rest == "--splash loading" ||
        utility_rest == "--splash load" || utility_rest == "--bootstrap" ||
        utility_rest == "--boot")
      return dispatch(ActionId::HomeSplash, state);
    if (utility_rest == "--farewell" || utility_rest == "--closing" ||
        utility_rest == "--splash=farewell" ||
        utility_rest == "--splash=closing" ||
        utility_rest == "--splash=close" ||
        utility_rest == "--splash=good luck" ||
        utility_rest == "--splash=good-luck" ||
        utility_rest == "--splash=good_luck" ||
        utility_rest == "--splash=goodluck" ||
        utility_rest == "--splash farewell" ||
        utility_rest == "--splash closing" ||
        utility_rest == "--splash close" ||
        utility_rest == "--splash good luck" ||
        utility_rest == "--splash good-luck" ||
        utility_rest == "--splash good_luck" ||
        utility_rest == "--splash goodluck")
      return dispatch(ActionId::HomeFarewell, state);

    if (const auto payload = payload_after("--run="); payload.has_value())
      return dispatch_payload(*payload);
    if (const auto payload = payload_after("--run "); payload.has_value())
      return dispatch_payload(*payload);
    if (const auto payload = payload_after("--command="); payload.has_value())
      return dispatch_payload(*payload);
    if (const auto payload = payload_after("--command "); payload.has_value())
      return dispatch_payload(*payload);

    if (const auto payload = payload_after("--snapshot --run ");
        payload.has_value())
      return dispatch_payload(*payload);
    if (const auto payload = payload_after("--snapshot --command ");
        payload.has_value())
      return dispatch_payload(*payload);
    if (const auto payload = payload_after("--screen="); payload.has_value())
      return dispatch_payload(*payload);
    if (const auto payload = payload_after("--screen "); payload.has_value())
      return dispatch_payload(*payload);
    if (const auto payload = payload_after("--snapshot --screen ");
        payload.has_value())
      return dispatch_payload(*payload);
    if (const auto payload = payload_after("--snapshot --screen=");
        payload.has_value())
      return dispatch_payload(*payload);
    if (const auto payload = payload_after("--menu --run ");
        payload.has_value())
      return dispatch_payload_then(*payload, ActionId::Help);
    if (const auto payload = payload_after("--menu --command ");
        payload.has_value())
      return dispatch_payload_then(*payload, ActionId::Help);
    if (const auto payload = payload_after("--actions --run ");
        payload.has_value())
      return dispatch_payload_then(*payload, ActionId::ActionMenu);
    if (const auto payload = payload_after("--actions --command ");
        payload.has_value())
      return dispatch_payload_then(*payload, ActionId::ActionMenu);
    if (const auto payload = payload_after("--palette --run ");
        payload.has_value())
      return dispatch_payload_then(*payload, ActionId::ActionMenu);
    if (const auto payload = payload_after("--palette --command ");
        payload.has_value())
      return dispatch_payload_then(*payload, ActionId::ActionMenu);
    if (const auto payload = payload_after("--commands --run ");
        payload.has_value())
      return dispatch_payload_then(*payload, ActionId::ActionMenu);
    if (const auto payload = payload_after("--commands --command ");
        payload.has_value())
      return dispatch_payload_then(*payload, ActionId::ActionMenu);
    if (const auto payload = payload_after("--catalog --run ");
        payload.has_value())
      return dispatch_payload_then(*payload, ActionId::ActionMenu);
    if (const auto payload = payload_after("--catalog --command ");
        payload.has_value())
      return dispatch_payload_then(*payload, ActionId::ActionMenu);

    if (!rest.empty() && rest.front() != '-')
      return dispatch_payload(rest);

    state.status = "unknown command: " + command;
    return false;
  }

  std::optional<std::size_t> action_index(ActionId id) const {
    for (std::size_t i = 0; i < actions_.size(); ++i) {
      if (actions_[i].id == id)
        return i;
    }
    return std::nullopt;
  }

  std::optional<std::size_t>
  contextual_action_menu_index(const CmdState &state) const {
    switch (state.screen) {
    case ScreenMode::Home:
      return action_index(ActionId::HomeVisual);
    case ScreenMode::Workbench:
      return action_index(ActionId::ShowRuntime);
    case ScreenMode::Runtime:
      if (state.runtime.focus == RuntimeFocus::Jobs) {
        switch (state.runtime.detail_mode) {
        case RuntimeDetailMode::Manifest:
          return action_index(ActionId::RuntimeDetailLog);
        case RuntimeDetailMode::Log:
          return action_index(ActionId::RuntimeDetailTrace);
        case RuntimeDetailMode::Trace:
          return action_index(ActionId::RuntimeDetailManifest);
        }
      }
      return action_index(ActionId::RuntimeFocusDevices);
    case ScreenMode::Lattice:
      return action_index(ActionId::ShowLatticeSummary);
    case ScreenMode::Logs:
      switch (state.logs.selected_setting) {
      case 0:
        return action_index(ActionId::CycleLogsSourceFilter);
      case 1:
        return action_index(ActionId::CycleLogsSeverityFilter);
      case 2:
        return action_index(ActionId::CycleLogsMetadataFilter);
      case 3:
        return action_index(ActionId::ToggleLogsTimestamp);
      case 4:
        return action_index(ActionId::ToggleLogsThread);
      case 5:
        return action_index(ActionId::ToggleLogsMetadata);
      case 6:
        return action_index(ActionId::ToggleLogsColor);
      case 7:
        return action_index(ActionId::ToggleLogsFollow);
      case 8:
        return action_index(ActionId::ToggleLogsMouseCapture);
      default:
        return action_index(ActionId::CycleLogsSourceFilter);
      }
    case ScreenMode::Config:
      return action_index(ActionId::ConfigFileShow);
    }
    return std::nullopt;
  }

  bool dispatch_pattern(const std::string &command, CmdState &state) const {
    static const std::string kConfigIndexPrefix{"iinuji.config.file.index."};
    static const std::string kConfigIdPrefix{"iinuji.config.file.id."};
    static const std::string kLatticeIndexPrefix{
        "iinuji.lattice.target.index."};
    static const std::string kLatticeIdPrefix{"iinuji.lattice.target.id."};

    std::string token{};
    if (command_call_payload(command, "iinuji.config.file.index", token)) {
      return select_config_file_by_index_token(state, token);
    }

    if (command_call_payload(command, "iinuji.config.file.id", token)) {
      return select_config_file_by_id_token(state, token);
    }

    if (command_call_payload(command, "iinuji.lattice.target.index", token)) {
      return select_lattice_target_by_index_token(state, token);
    }

    if (command_call_payload(command, "iinuji.lattice.target.id", token)) {
      return select_lattice_target_by_id_token(state, token);
    }

    if (command_starts_with(command, kConfigIndexPrefix)) {
      return select_config_file_by_index_token(
          state, command.substr(kConfigIndexPrefix.size()));
    }

    if (command_starts_with(command, kConfigIdPrefix)) {
      return select_config_file_by_id_token(
          state, command.substr(kConfigIdPrefix.size()));
    }

    if (command_starts_with(command, kLatticeIndexPrefix)) {
      return select_lattice_target_by_index_token(
          state, command.substr(kLatticeIndexPrefix.size()));
    }

    if (command_starts_with(command, kLatticeIdPrefix)) {
      return select_lattice_target_by_id_token(
          state, command.substr(kLatticeIdPrefix.size()));
    }

    if (command_tail_after_prefix(command, "config file index ", token) ||
        command_tail_after_prefix(command, "config index ", token) ||
        command_tail_after_prefix(command, "file index ", token)) {
      return select_config_file_by_index_token(state, token);
    }

    if (command_tail_after_prefix(command, "config file id ", token) ||
        command_tail_after_prefix(command, "config id ", token) ||
        command_tail_after_prefix(command, "file id ", token)) {
      return select_config_file_by_id_token(state, token);
    }

    if (command_tail_after_prefix(command, "config file ", token) ||
        command_tail_after_prefix(command, "file ", token) ||
        command_tail_after_prefix(command, "config ", token)) {
      return select_config_file_by_token(state, token);
    }

    if (command_looks_like_managed_config_path(command))
      return select_config_file_by_token(state, command);

    if (command_tail_after_prefix(command, "lattice target index ", token) ||
        command_tail_after_prefix(command, "lattice index ", token) ||
        command_tail_after_prefix(command, "target index ", token)) {
      return select_lattice_target_by_index_token(state, token);
    }

    if (command_tail_after_prefix(command, "lattice target id ", token) ||
        command_tail_after_prefix(command, "lattice id ", token) ||
        command_tail_after_prefix(command, "target id ", token)) {
      return select_lattice_target_by_id_token(state, token);
    }

    if (command_tail_after_prefix(command, "lattice target ", token) ||
        command_tail_after_prefix(command, "target ", token) ||
        command_tail_after_prefix(command, "lattice ", token)) {
      return select_lattice_target_by_token(state, token);
    }

    return false;
  }

  bool dispatch_action_menu_command(const std::string &command,
                                    CmdState &state) const {
    if (command != "iinuji.actions.run" &&
        command != "iinuji.actions.run.selected" &&
        command != "iinuji.commands.run" &&
        command != "iinuji.commands.run.selected" && command != "actions run" &&
        command != "action run" && command != "run action" &&
        command != "commands run" && command != "palette run" &&
        command != "run palette") {
      return false;
    }
    if (actions_.empty()) {
      state.status = "action menu empty";
      return true;
    }
    if (!state.action_menu_open) {
      open_action_menu(state);
      return true;
    }
    const std::size_t selected =
        std::min(state.action_menu_selected, actions_.size() - 1u);
    const ActionId selected_id = actions_[selected].id;
    state.action_menu_open = false;
    state.action_menu_scroll_y = 0;
    return dispatch(selected_id, state);
  }

  std::vector<Action> actions_{};
  std::unordered_map<std::string, std::size_t> alias_index_{};
};

inline ActionRegistry create_default_actions() {
  ActionRegistry registry{};
  auto screen_action = [&](ActionId id, ScreenMode screen,
                           std::vector<std::string> aliases) {
    registry.add(Action{
        .id = id,
        .label = "switch " + screen_key_label(screen) + " " +
                 screen_badge_label(screen),
        .aliases = std::move(aliases),
        .apply =
            [screen](CmdState &state) {
              state.screen = screen;
              if (screen == ScreenMode::Home)
                state.home_visual.presentation = HomePresentationMode::Showcase;
              state.help_view = false;
              state.action_menu_open = false;
              state.status = screen_status_label(screen);
            },
    });
  };
  auto show_screen_action = [&](ActionId id, ScreenMode screen,
                                std::vector<std::string> aliases) {
    registry.add(Action{
        .id = id,
        .label = "show " + screen_key_label(screen) + " " +
                 screen_badge_label(screen) + " summary",
        .aliases = std::move(aliases),
        .apply =
            [screen](CmdState &state) {
              state.screen = screen;
              if (screen == ScreenMode::Home)
                state.home_visual.presentation = HomePresentationMode::Showcase;
              state.help_view = false;
              state.action_menu_open = false;
              append_show_current_screen_logs(state);
            },
    });
  };
  auto logs_level_action = [&](ActionId id, LogsSeverityFilter filter,
                               std::vector<std::string> aliases) {
    registry.add(Action{
        .id = id,
        .label = "logs level " + logs_severity_filter_label(filter),
        .aliases = std::move(aliases),
        .apply =
            [filter](CmdState &state) {
              state.logs.severity_filter = filter;
              state.screen = ScreenMode::Logs;
              state.status = "logs.level=" + logs_severity_filter_label(filter);
            },
    });
  };
  auto logs_source_filter_action = [&](ActionId id, LogsSourceFilter filter,
                                       std::vector<std::string> aliases) {
    registry.add(Action{
        .id = id,
        .label = "logs source " + logs_source_filter_label(filter),
        .aliases = std::move(aliases),
        .apply =
            [filter](CmdState &state) {
              state.logs.source_filter = filter;
              state.screen = ScreenMode::Logs;
              state.status = "logs.source=" + logs_source_filter_label(filter);
            },
    });
  };
  auto logs_metadata_filter_action = [&](ActionId id, LogsMetadataFilter filter,
                                         std::vector<std::string> aliases) {
    registry.add(Action{
        .id = id,
        .label = "logs metadata filter " + logs_metadata_filter_label(filter),
        .aliases = std::move(aliases),
        .apply =
            [filter](CmdState &state) {
              state.logs.metadata_filter = filter;
              state.screen = ScreenMode::Logs;
              state.status =
                  "logs.metadata_filter=" + logs_metadata_filter_label(filter);
            },
    });
  };
  auto help_scroll_action = [&](ActionId id, std::string label,
                                std::vector<std::string> aliases,
                                std::string status, int dy, int dx,
                                bool home = false, bool end = false) {
    registry.add(Action{
        .id = id,
        .label = std::move(label),
        .aliases = std::move(aliases),
        .apply =
            [status = std::move(status), dy, dx, home, end](CmdState &state) {
              state.help_view = true;
              state.action_menu_open = false;
              close_content_popup(state);
              if (home) {
                state.help_scroll_y = 0;
                state.help_scroll_x = 0;
              } else if (end) {
                state.help_scroll_y = std::numeric_limits<int>::max();
              } else {
                state.help_scroll_y = std::max(0, state.help_scroll_y + dy);
                state.help_scroll_x = std::max(0, state.help_scroll_x + dx);
              }
              state.status = status;
            },
    });
  };
  auto logs_scroll_action =
      [&](ActionId id, std::string label, std::vector<std::string> aliases,
          std::string status, int dy, bool home = false, bool end = false) {
        registry.add(Action{
            .id = id,
            .label = std::move(label),
            .aliases = std::move(aliases),
            .apply =
                [status = std::move(status), dy, home, end](CmdState &state) {
                  state.screen = ScreenMode::Logs;
                  if (home) {
                    state.logs.scroll_y = 0;
                    state.logs.scroll_x = 0;
                  } else if (end) {
                    state.logs.scroll_y = std::numeric_limits<int>::max();
                  } else {
                    state.logs.scroll_y = std::max(0, state.logs.scroll_y + dy);
                  }
                  if (home || (!end && dy != 0))
                    state.logs.auto_follow = false;
                  if (end)
                    state.logs.auto_follow = true;
                  state.status = status;
                },
        });
      };

  screen_action(ActionId::ShowHome, ScreenMode::Home,
                {"home", "1", "f1", "iinuji.screen.home()"});
  registry.add(Action{
      .id = ActionId::HomeVisual,
      .label = "show Home visual showcase (waajacamaya)",
      .aliases = {"visual",
                  "image",
                  "animation",
                  "waajacamaya",
                  "home visual",
                  "home-visual",
                  "home_visual",
                  "home image",
                  "home-image",
                  "home_image",
                  "home animation",
                  "home-animation",
                  "home_animation",
                  "iinuji.home.visual()",
                  "iinuji.home.animation()",
                  "cuwacunu_cmd --visual",
                  "cuwacunu_cmd --home-visual",
                  "cuwacunu_cmd --image",
                  "cuwacunu_cmd --animation",
                  "cuwacunu_cmd --waajacamaya",
                  "cuwacunu.cmd --visual",
                  "cuwacunu.cmd --home-visual",
                  "cuwacunu.cmd --image",
                  "cuwacunu.cmd --animation",
                  "cuwacunu.cmd --waajacamaya",
                  "iinuji_cmd --visual",
                  "iinuji_cmd --home-visual",
                  "iinuji_cmd --image",
                  "iinuji_cmd --animation",
                  "iinuji_cmd --waajacamaya"},
      .apply =
          [](CmdState &state) {
            state.screen = ScreenMode::Home;
            state.home_visual.presentation = HomePresentationMode::Showcase;
            state.help_view = false;
            state.action_menu_open = false;
            state.status = "home visual";
          },
  });
  registry.add(Action{
      .id = ActionId::HomeSplash,
      .label = "show Home bootstrap splash (loading)",
      .aliases = {"iinuji.home.splash()",
                  "splash",
                  "bootstrap",
                  "boot",
                  "loading",
                  "load",
                  "show splash",
                  "show bootstrap",
                  "home splash",
                  "bootstrap splash",
                  "iinuji.splash()",
                  "cuwacunu_cmd --splash",
                  "cuwacunu_cmd --splash=bootstrap",
                  "cuwacunu_cmd --splash=boot",
                  "cuwacunu_cmd --splash=loading",
                  "cuwacunu_cmd --splash=load",
                  "cuwacunu_cmd --splash bootstrap",
                  "cuwacunu_cmd --splash boot",
                  "cuwacunu_cmd --splash loading",
                  "cuwacunu_cmd --splash load",
                  "cuwacunu_cmd --bootstrap",
                  "cuwacunu_cmd --boot",
                  "cuwacunu.cmd --splash",
                  "cuwacunu.cmd --splash=bootstrap",
                  "cuwacunu.cmd --splash=boot",
                  "cuwacunu.cmd --splash=loading",
                  "cuwacunu.cmd --splash=load",
                  "cuwacunu.cmd --splash bootstrap",
                  "cuwacunu.cmd --splash boot",
                  "cuwacunu.cmd --splash loading",
                  "cuwacunu.cmd --splash load",
                  "cuwacunu.cmd --bootstrap",
                  "cuwacunu.cmd --boot",
                  "iinuji_cmd --splash",
                  "iinuji_cmd --splash=bootstrap",
                  "iinuji_cmd --splash=boot",
                  "iinuji_cmd --splash=loading",
                  "iinuji_cmd --splash=load",
                  "iinuji_cmd --splash bootstrap",
                  "iinuji_cmd --splash boot",
                  "iinuji_cmd --splash loading",
                  "iinuji_cmd --splash load",
                  "iinuji_cmd --bootstrap",
                  "iinuji_cmd --boot"},
      .apply =
          [](CmdState &state) {
            state.screen = ScreenMode::Home;
            state.home_visual.presentation =
                HomePresentationMode::BootstrapSplash;
            state.help_view = false;
            state.action_menu_open = false;
            append_home_splash_logs(state, false);
          },
  });
  registry.add(Action{
      .id = ActionId::HomeFarewell,
      .label = "show Home farewell splash (Good luck)",
      .aliases = {"iinuji.home.farewell()",
                  "farewell",
                  "closing",
                  "show farewell",
                  "show closing",
                  "home farewell",
                  "good luck",
                  "show good luck",
                  "farewell splash",
                  "closing splash",
                  "good luck splash",
                  "iinuji.farewell()",
                  "cuwacunu_cmd --farewell",
                  "cuwacunu_cmd --splash=farewell",
                  "cuwacunu_cmd --splash=closing",
                  "cuwacunu_cmd --splash=close",
                  "cuwacunu_cmd --splash=good luck",
                  "cuwacunu_cmd --splash=good-luck",
                  "cuwacunu_cmd --splash=good_luck",
                  "cuwacunu_cmd --splash=goodluck",
                  "cuwacunu_cmd --splash farewell",
                  "cuwacunu_cmd --splash closing",
                  "cuwacunu_cmd --splash close",
                  "cuwacunu_cmd --splash good luck",
                  "cuwacunu_cmd --splash good-luck",
                  "cuwacunu_cmd --splash good_luck",
                  "cuwacunu_cmd --splash goodluck",
                  "cuwacunu_cmd --closing",
                  "cuwacunu.cmd --farewell",
                  "cuwacunu.cmd --splash=farewell",
                  "cuwacunu.cmd --splash=closing",
                  "cuwacunu.cmd --splash=close",
                  "cuwacunu.cmd --splash=good luck",
                  "cuwacunu.cmd --splash=good-luck",
                  "cuwacunu.cmd --splash=good_luck",
                  "cuwacunu.cmd --splash=goodluck",
                  "cuwacunu.cmd --splash farewell",
                  "cuwacunu.cmd --splash closing",
                  "cuwacunu.cmd --splash close",
                  "cuwacunu.cmd --splash good luck",
                  "cuwacunu.cmd --splash good-luck",
                  "cuwacunu.cmd --splash good_luck",
                  "cuwacunu.cmd --splash goodluck",
                  "cuwacunu.cmd --closing",
                  "iinuji_cmd --farewell",
                  "iinuji_cmd --splash=farewell",
                  "iinuji_cmd --splash=closing",
                  "iinuji_cmd --splash=close",
                  "iinuji_cmd --splash=good luck",
                  "iinuji_cmd --splash=good-luck",
                  "iinuji_cmd --splash=good_luck",
                  "iinuji_cmd --splash=goodluck",
                  "iinuji_cmd --splash farewell",
                  "iinuji_cmd --splash closing",
                  "iinuji_cmd --splash close",
                  "iinuji_cmd --splash good luck",
                  "iinuji_cmd --splash good-luck",
                  "iinuji_cmd --splash good_luck",
                  "iinuji_cmd --splash goodluck",
                  "iinuji_cmd --closing"},
      .apply =
          [](CmdState &state) {
            state.screen = ScreenMode::Home;
            state.home_visual.presentation =
                HomePresentationMode::FarewellSplash;
            state.help_view = false;
            state.action_menu_open = false;
            append_home_splash_logs(state, true);
          },
  });
  screen_action(ActionId::ShowWorkbench, ScreenMode::Workbench,
                {"workbench", "work", "w", "marshal", "inbox", "human", "2",
                 "f2", "iinuji.screen.workbench()", "iinuji.screen.marshal()",
                 "iinuji.screen.inbox()", "iinuji.screen.human()"});
  screen_action(ActionId::ShowRuntime, ScreenMode::Runtime,
                {"runtime", "run", "rt", "3", "f3", "iinuji.screen.runtime()"});
  screen_action(ActionId::ShowLattice, ScreenMode::Lattice,
                {"lattice", "lat", "proof", "4", "f4",
                 "iinuji.screen.lattice()", "lattice targets", "targets"});
  screen_action(ActionId::ShowLogs, ScreenMode::Logs,
                {"logs", "log", "shell", "shelllogs", "shell_logs", "events",
                 "shell logs", "shell log", "shell-logs", "shell-log", "8",
                 "f8", "iinuji.screen.logs()"});
  screen_action(ActionId::ShowConfig, ScreenMode::Config,
                {"config", "cfg", "9", "f9", "iinuji.screen.config()"});
  registry.add(Action{
      .id = ActionId::ShowCurrent,
      .label = "show current screen",
      .aliases = {"show", "iinuji.show()"},
      .apply =
          [](CmdState &state) {
            state.help_view = false;
            state.action_menu_open = false;
            append_show_current_screen_logs(state);
          },
  });
  registry.add(Action{
      .id = ActionId::ShowIdentity,
      .label = "show command identity",
      .aliases = {"iinuji.version()", "iinuji.about()", "version", "about",
                  "identity", "cuwacunu_cmd --version", "cuwacunu_cmd -v",
                  "cuwacunu.cmd --version", "cuwacunu.cmd -v",
                  "iinuji_cmd --version", "iinuji_cmd -v"},
      .apply =
          [](CmdState &state) {
            state.help_view = false;
            state.action_menu_open = false;
            state.screen = ScreenMode::Logs;
            for (const std::string &line :
                 command_identity_lines(state.command_name))
              append_log(state, "show", line);
            state.status = "version";
          },
  });
  show_screen_action(ActionId::ShowHomeSummary, ScreenMode::Home,
                     {"iinuji.show.home()", "show home", "home show"});
  show_screen_action(ActionId::ShowWorkbenchSummary, ScreenMode::Workbench,
                     {"iinuji.show.workbench()", "show workbench",
                      "workbench show", "show marshal", "marshal show",
                      "show inbox", "inbox show", "show human", "human show",
                      "iinuji.show.marshal()", "iinuji.show.inbox()",
                      "iinuji.show.human()"});
  show_screen_action(ActionId::ShowRuntimeSummary, ScreenMode::Runtime,
                     {"iinuji.show.runtime()", "show runtime", "runtime show"});
  show_screen_action(ActionId::ShowLatticeSummary, ScreenMode::Lattice,
                     {"iinuji.show.lattice()", "show lattice", "show target",
                      "show proof", "show lattice target", "show lattice proof",
                      "show selected target", "show selected target proof",
                      "target proof", "lattice proof", "lattice show",
                      "target show", "proof show"});
  show_screen_action(ActionId::ShowLogsSummary, ScreenMode::Logs,
                     {"iinuji.show.logs()", "show logs", "logs show",
                      "show shell logs", "shell logs show"});
  show_screen_action(ActionId::ShowConfigSummary, ScreenMode::Config,
                     {"iinuji.show.config()", "show config"});
  registry.add(Action{
      .id = ActionId::Help,
      .label = "open help",
      .aliases = {"help", "h", "menu", "?", "iinuji.help()",
                  "cuwacunu_cmd --help", "cuwacunu_cmd -h",
                  "cuwacunu_cmd --menu", "cuwacunu.cmd --help",
                  "cuwacunu.cmd -h", "cuwacunu.cmd --menu", "iinuji_cmd --help",
                  "iinuji_cmd -h", "iinuji_cmd --menu"},
      .apply =
          [](CmdState &state) {
            state.help_view = true;
            state.action_menu_open = false;
            close_content_popup(state);
            state.help_scroll_y = 0;
            state.help_scroll_x = 0;
            state.status = "help overlay=open (Esc or click [x] to close)";
          },
  });
  registry.add(Action{
      .id = ActionId::CloseHelp,
      .label = "close help",
      .aliases = {"close help", "help close", "iinuji.help.close()"},
      .apply =
          [](CmdState &state) {
            state.help_view = false;
            state.status = "help overlay=closed";
          },
  });
  help_scroll_action(ActionId::HelpScrollUp, "help scroll up",
                     {"help up", "menu up", "iinuji.help.scroll.up()"},
                     "help scroll=up", -3, 0);
  help_scroll_action(ActionId::HelpScrollDown, "help scroll down",
                     {"help down", "menu down", "iinuji.help.scroll.down()"},
                     "help scroll=down", 3, 0);
  help_scroll_action(ActionId::HelpScrollLeft, "help scroll left",
                     {"help left", "menu left", "iinuji.help.scroll.left()"},
                     "help scroll=left", 0, -16);
  help_scroll_action(ActionId::HelpScrollRight, "help scroll right",
                     {"help right", "menu right", "iinuji.help.scroll.right()"},
                     "help scroll=right", 0, 16);
  help_scroll_action(
      ActionId::HelpScrollPageUp, "help scroll page up",
      {"help page up", "menu page up", "iinuji.help.scroll.page.up()"},
      "help scroll=page-up", -20, 0);
  help_scroll_action(
      ActionId::HelpScrollPageDown, "help scroll page down",
      {"help page down", "menu page down", "iinuji.help.scroll.page.down()"},
      "help scroll=page-down", 20, 0);
  help_scroll_action(ActionId::HelpScrollHome, "help scroll home",
                     {"help home", "menu home", "iinuji.help.scroll.home()"},
                     "help scroll=home", 0, 0, true);
  help_scroll_action(ActionId::HelpScrollEnd, "help scroll end",
                     {"help end", "menu end", "iinuji.help.scroll.end()"},
                     "help scroll=end", 0, 0, false, true);
  registry.add(Action{
      .id = ActionId::ActionMenu,
      .label = "action menu",
      .aliases = {"a", "actions", "action menu", "palette", "commands",
                  "iinuji.actions()", "iinuji.commands()",
                  "cuwacunu_cmd --actions", "cuwacunu_cmd --palette",
                  "cuwacunu_cmd --commands", "cuwacunu_cmd --catalog",
                  "cuwacunu.cmd --actions", "cuwacunu.cmd --palette",
                  "cuwacunu.cmd --commands", "cuwacunu.cmd --catalog",
                  "iinuji_cmd --actions", "iinuji_cmd --palette",
                  "iinuji_cmd --commands", "iinuji_cmd --catalog"},
      .apply =
          [](CmdState &state) {
            state.help_view = false;
            state.action_menu_open = true;
            close_content_popup(state);
            state.action_menu_scroll_y = 0;
            state.status = "action menu";
          },
  });
  registry.add(Action{
      .id = ActionId::ActionMenuClose,
      .label = "close action menu",
      .aliases = {"actions close", "action menu close", "commands close",
                  "palette close", "close actions", "close palette",
                  "close commands", "iinuji.actions.close()",
                  "iinuji.commands.close()"},
      .apply =
          [](CmdState &state) {
            state.action_menu_open = false;
            state.action_menu_scroll_y = 0;
            state.status = screen_label(state.screen);
          },
  });
  registry.add(Action{
      .id = ActionId::ActionMenuSelectPrev,
      .label = "action menu previous",
      .aliases = {"actions previous", "actions prev", "action menu previous",
                  "actions up", "palette up", "commands up",
                  "iinuji.actions.select.prev()",
                  "iinuji.commands.select.prev()"},
      .apply =
          [](CmdState &state) { select_relative_action_menu_item(state, -1); },
  });
  registry.add(Action{
      .id = ActionId::ActionMenuSelectNext,
      .label = "action menu next",
      .aliases = {"actions next", "action menu next", "actions down",
                  "palette down", "commands down",
                  "iinuji.actions.select.next()",
                  "iinuji.commands.select.next()"},
      .apply =
          [](CmdState &state) { select_relative_action_menu_item(state, 1); },
  });
  registry.add(Action{
      .id = ActionId::ActionMenuSelectPageUp,
      .label = "action menu page up",
      .aliases = {"actions page up", "action menu page up", "palette page up",
                  "commands page up", "iinuji.actions.select.page.up()",
                  "iinuji.commands.select.page.up()"},
      .apply =
          [](CmdState &state) { select_relative_action_menu_item(state, -8); },
  });
  registry.add(Action{
      .id = ActionId::ActionMenuSelectPageDown,
      .label = "action menu page down",
      .aliases = {"actions page down", "action menu page down",
                  "palette page down", "commands page down",
                  "iinuji.actions.select.page.down()",
                  "iinuji.commands.select.page.down()"},
      .apply =
          [](CmdState &state) { select_relative_action_menu_item(state, 8); },
  });
  registry.add(Action{
      .id = ActionId::ActionMenuSelectHome,
      .label = "action menu first",
      .aliases = {"actions first", "actions home", "action menu first",
                  "palette home", "commands home",
                  "iinuji.actions.select.home()",
                  "iinuji.commands.select.home()"},
      .apply = [](CmdState &state) { select_first_action_menu_item(state); },
  });
  registry.add(Action{
      .id = ActionId::ActionMenuSelectEnd,
      .label = "action menu last",
      .aliases = {"actions last", "actions end", "action menu last",
                  "palette end", "commands end", "iinuji.actions.select.end()",
                  "iinuji.commands.select.end()"},
      .apply = [](CmdState &state) { select_last_action_menu_item(state); },
  });
  logs_scroll_action(ActionId::LogsScrollUp, "logs scroll up",
                     {"logs up", "log up", "iinuji.logs.scroll.up()"},
                     "logs scroll=up", -6);
  logs_scroll_action(ActionId::LogsScrollDown, "logs scroll down",
                     {"logs down", "log down", "iinuji.logs.scroll.down()"},
                     "logs scroll=down", 6);
  logs_scroll_action(
      ActionId::LogsScrollPageUp, "logs scroll page up",
      {"logs page up", "log page up", "iinuji.logs.scroll.page.up()"},
      "logs scroll=page-up", -20);
  logs_scroll_action(
      ActionId::LogsScrollPageDown, "logs scroll page down",
      {"logs page down", "log page down", "iinuji.logs.scroll.page.down()"},
      "logs scroll=page-down", 20);
  logs_scroll_action(ActionId::LogsScrollHome, "logs scroll home",
                     {"logs home", "log home", "iinuji.logs.scroll.home()"},
                     "logs scroll=home", 0, true);
  logs_scroll_action(ActionId::LogsScrollEnd, "logs scroll end",
                     {"logs end", "log end", "iinuji.logs.scroll.end()"},
                     "logs scroll=end", 0, false, true);
  registry.add(Action{
      .id = ActionId::Refresh,
      .label = "refresh",
      .aliases = {"refresh", "reload", "r", "iinuji.refresh()"},
      .apply =
          [](CmdState &state) {
            refresh_snapshots(state);
            state.status = "hero shell refresh queued";
          },
  });
  registry.add(Action{
      .id = ActionId::WorkbenchRefresh,
      .label = "workbench refresh",
      .aliases = {"workbench refresh", "refresh workbench",
                  "iinuji.workbench.refresh()"},
      .apply =
          [](CmdState &state) {
            refresh_snapshots(state);
            state.screen = ScreenMode::Workbench;
            state.status = "workbench refresh queued";
          },
  });
  registry.add(Action{
      .id = ActionId::RuntimeRefresh,
      .label = "runtime refresh",
      .aliases = {"runtime refresh", "refresh runtime",
                  "iinuji.runtime.refresh()"},
      .apply =
          [](CmdState &state) {
            refresh_snapshots(state);
            state.screen = ScreenMode::Runtime;
            state.status = "runtime screen refresh queued";
          },
  });
  registry.add(Action{
      .id = ActionId::LatticeRefresh,
      .label = "lattice refresh",
      .aliases = {"lattice refresh", "refresh lattice",
                  "iinuji.lattice.refresh()"},
      .apply =
          [](CmdState &state) {
            refresh_snapshots(state);
            state.screen = ScreenMode::Lattice;
            state.status = "lattice refresh queued";
          },
  });
  registry.add(Action{
      .id = ActionId::ConfigReload,
      .label = "config reload",
      .aliases = {"config reload", "reload config", "refresh config",
                  "iinuji.config.reload()"},
      .apply =
          [](CmdState &state) {
            refresh_snapshots(state);
            state.screen = ScreenMode::Config;
            state.status = "config reload queued";
          },
  });
  registry.add(Action{
      .id = ActionId::ClearLogs,
      .label = "clear logs",
      .aliases = {"clear logs", "logs clear", "clearlogs", "clear",
                  "iinuji.logs.clear()"},
      .apply =
          [](CmdState &state) {
            state.log.clear();
            state.screen = ScreenMode::Logs;
            state.status = "logs cleared";
          },
  });
  registry.add(Action{
      .id = ActionId::ToggleLogsFollow,
      .label = "toggle logs follow",
      .aliases = {"logs follow", "follow", "toggle follow",
                  "iinuji.logs.settings.follow.toggle()"},
      .apply = [](CmdState &state) { toggle_logs_follow(state); },
  });
  registry.add(Action{
      .id = ActionId::ToggleLogsTimestamp,
      .label = "toggle logs timestamp",
      .aliases = {"logs time", "logs timestamp", "timestamp", "time",
                  "iinuji.logs.settings.date.toggle()"},
      .apply = [](CmdState &state) { toggle_logs_timestamp(state); },
  });
  registry.add(Action{
      .id = ActionId::ToggleLogsThread,
      .label = "toggle logs thread",
      .aliases = {"logs thread", "thread", "toggle thread",
                  "iinuji.logs.settings.thread.toggle()"},
      .apply = [](CmdState &state) { toggle_logs_thread(state); },
  });
  registry.add(Action{
      .id = ActionId::ToggleLogsMetadata,
      .label = "toggle logs metadata",
      .aliases = {"logs metadata", "metadata", "toggle metadata",
                  "iinuji.logs.settings.metadata.toggle()"},
      .apply = [](CmdState &state) { toggle_logs_metadata(state); },
  });
  registry.add(Action{
      .id = ActionId::ToggleLogsColor,
      .label = "toggle logs color",
      .aliases = {"logs color", "color", "toggle color",
                  "iinuji.logs.settings.color.toggle()"},
      .apply = [](CmdState &state) { toggle_logs_color(state); },
  });
  registry.add(Action{
      .id = ActionId::ToggleLogsMouseCapture,
      .label = "toggle logs mouse capture",
      .aliases = {"logs mouse", "mouse", "mouse capture", "toggle mouse",
                  "iinuji.logs.settings.mouse.capture.toggle()"},
      .apply = [](CmdState &state) { toggle_logs_mouse_capture(state); },
  });
  registry.add(Action{
      .id = ActionId::CycleLogsSourceFilter,
      .label = "cycle logs source filter",
      .aliases = {"logs filter", "filter", "source filter", "logs source",
                  "iinuji.logs.settings.source.next()"},
      .apply = [](CmdState &state) { cycle_logs_source_filter(state, 1); },
  });
  logs_source_filter_action(ActionId::SetLogsSourceAll, LogsSourceFilter::All,
                            {"logs source any", "logs source all", "source any",
                             "source all", "iinuji.logs.settings.source.any()",
                             "iinuji.logs.settings.source.all()"});
  logs_source_filter_action(ActionId::SetLogsSourceRefresh,
                            LogsSourceFilter::Refresh,
                            {"logs source refresh", "source refresh",
                             "iinuji.logs.settings.source.refresh()"});
  logs_source_filter_action(ActionId::SetLogsSourceAction,
                            LogsSourceFilter::Action,
                            {"logs source action", "source action",
                             "iinuji.logs.settings.source.action()"});
  logs_source_filter_action(ActionId::SetLogsSourceCommand,
                            LogsSourceFilter::Command,
                            {"logs source command", "source command",
                             "iinuji.logs.settings.source.command()"});
  logs_source_filter_action(ActionId::SetLogsSourceShow, LogsSourceFilter::Show,
                            {"logs source show", "source show", "show source",
                             "iinuji.logs.settings.source.show()"});
  logs_source_filter_action(
      ActionId::SetLogsSourceStatus, LogsSourceFilter::Status,
      {"logs source status", "source status", "status source",
       "iinuji.logs.settings.source.status()"});
  registry.add(Action{
      .id = ActionId::CycleLogsSeverityFilter,
      .label = "cycle logs severity filter",
      .aliases = {"logs level", "logs severity", "severity", "level",
                  "iinuji.logs.settings.level.next()"},
      .apply = [](CmdState &state) { cycle_logs_severity_filter(state, 1); },
  });
  registry.add(Action{
      .id = ActionId::CycleLogsMetadataFilter,
      .label = "cycle logs metadata filter",
      .aliases = {"logs meta", "logs metadata filter", "metadata filter",
                  "meta filter", "iinuji.logs.settings.metadata.filter.next()"},
      .apply = [](CmdState &state) { cycle_logs_metadata_filter(state, 1); },
  });
  logs_level_action(
      ActionId::SetLogsSeverityDebug, LogsSeverityFilter::DebugOrHigher,
      {"logs debug", "level debug", "iinuji.logs.settings.level.debug()"});
  logs_level_action(
      ActionId::SetLogsSeverityInfo, LogsSeverityFilter::InfoOrHigher,
      {"logs info", "level info", "iinuji.logs.settings.level.info()"});
  logs_level_action(ActionId::SetLogsSeverityWarning,
                    LogsSeverityFilter::WarningOrHigher,
                    {"logs warning", "logs warn", "level warning", "level warn",
                     "iinuji.logs.settings.level.warning()"});
  logs_level_action(
      ActionId::SetLogsSeverityError, LogsSeverityFilter::ErrorOrHigher,
      {"logs error", "level error", "iinuji.logs.settings.level.error()"});
  logs_level_action(
      ActionId::SetLogsSeverityFatal, LogsSeverityFilter::FatalOnly,
      {"logs fatal", "level fatal", "iinuji.logs.settings.level.fatal()"});
  logs_metadata_filter_action(ActionId::SetLogsMetadataAny,
                              LogsMetadataFilter::Any,
                              {"logs meta any", "metadata filter any",
                               "iinuji.logs.settings.metadata.filter.any()"});
  logs_metadata_filter_action(
      ActionId::SetLogsMetadataAnyMeta, LogsMetadataFilter::WithAnyMetadata,
      {"logs meta any_meta", "logs meta meta", "metadata filter any_meta",
       "metadata filter meta",
       "iinuji.logs.settings.metadata.filter.any_meta()"});
  logs_metadata_filter_action(
      ActionId::SetLogsMetadataFunction, LogsMetadataFilter::WithFunction,
      {"logs meta function", "logs meta fn", "metadata filter function",
       "metadata filter fn",
       "iinuji.logs.settings.metadata.filter.function()"});
  logs_metadata_filter_action(ActionId::SetLogsMetadataPath,
                              LogsMetadataFilter::WithPath,
                              {"logs meta path", "metadata filter path",
                               "iinuji.logs.settings.metadata.filter.path()"});
  logs_metadata_filter_action(
      ActionId::SetLogsMetadataCallsite, LogsMetadataFilter::WithCallsite,
      {"logs meta callsite", "metadata filter callsite",
       "iinuji.logs.settings.metadata.filter.callsite()"});
  registry.add(Action{
      .id = ActionId::LogsSelectSettingPrev,
      .label = "logs setting previous",
      .aliases = {"logs setting previous", "logs setting prev",
                  "iinuji.logs.settings.select.prev()"},
      .apply =
          [](CmdState &state) {
            state.screen = ScreenMode::Logs;
            select_relative_logs_setting(state, -1);
          },
  });
  registry.add(Action{
      .id = ActionId::LogsSelectSettingNext,
      .label = "logs setting next",
      .aliases = {"logs setting next", "iinuji.logs.settings.select.next()"},
      .apply =
          [](CmdState &state) {
            state.screen = ScreenMode::Logs;
            select_relative_logs_setting(state, 1);
          },
  });
  registry.add(Action{
      .id = ActionId::LogsAdjustSettingPrev,
      .label = "logs setting adjust previous",
      .aliases = {"logs setting left", "logs setting change previous",
                  "logs setting change prev", "logs setting adjust previous",
                  "logs setting adjust prev",
                  "iinuji.logs.settings.change.prev()",
                  "iinuji.logs.settings.adjust.prev()"},
      .apply = [](CmdState &state) { adjust_selected_logs_setting(state, -1); },
  });
  registry.add(Action{
      .id = ActionId::LogsAdjustSettingNext,
      .label = "logs setting adjust next",
      .aliases = {"logs setting right", "logs setting change next",
                  "logs setting adjust next",
                  "iinuji.logs.settings.change.next()",
                  "iinuji.logs.settings.adjust.next()"},
      .apply = [](CmdState &state) { adjust_selected_logs_setting(state, 1); },
  });
  registry.add(Action{
      .id = ActionId::RuntimeFocusDevices,
      .label = "runtime focus devices",
      .aliases = {"runtime devices", "devices", "iinuji.runtime.devices()"},
      .apply =
          [](CmdState &state) {
            state.screen = ScreenMode::Runtime;
            set_runtime_focus(state, RuntimeFocus::Devices);
          },
  });
  registry.add(Action{
      .id = ActionId::RuntimeFocusJobs,
      .label = "runtime focus jobs",
      .aliases = {"runtime jobs", "jobs", "iinuji.runtime.jobs()"},
      .apply =
          [](CmdState &state) {
            state.screen = ScreenMode::Runtime;
            set_runtime_focus(state, RuntimeFocus::Jobs);
          },
  });
  registry.add(Action{
      .id = ActionId::RuntimeFocusPrev,
      .label = "runtime row previous",
      .aliases = {"runtime row previous", "runtime row prev",
                  "runtime lane previous", "runtime lane prev", "runtime left",
                  "iinuji.runtime.row.prev()"},
      .apply =
          [](CmdState &state) {
            state.screen = ScreenMode::Runtime;
            select_relative_runtime_focus(state, -1);
          },
  });
  registry.add(Action{
      .id = ActionId::RuntimeFocusNext,
      .label = "runtime row next",
      .aliases = {"runtime row next", "runtime lane next", "runtime right",
                  "iinuji.runtime.row.next()"},
      .apply =
          [](CmdState &state) {
            state.screen = ScreenMode::Runtime;
            select_relative_runtime_focus(state, 1);
          },
  });
  registry.add(Action{
      .id = ActionId::RuntimeDetailNext,
      .label = "runtime detail next",
      .aliases = {"runtime detail", "runtime detail next", "detail next",
                  "iinuji.runtime.detail.next()"},
      .apply =
          [](CmdState &state) {
            state.screen = ScreenMode::Runtime;
            cycle_runtime_detail_mode(state);
          },
  });
  auto runtime_detail_action = [&](ActionId id, RuntimeDetailMode mode,
                                   std::vector<std::string> aliases) {
    registry.add(Action{
        .id = id,
        .label = "runtime detail " + runtime_detail_mode_label(mode),
        .aliases = std::move(aliases),
        .apply =
            [mode](CmdState &state) {
              state.screen = ScreenMode::Runtime;
              set_runtime_detail_mode(state, mode);
            },
    });
  };
  runtime_detail_action(
      ActionId::RuntimeDetailManifest, RuntimeDetailMode::Manifest,
      {"runtime detail manifest", "detail manifest", "runtime manifest",
       "iinuji.runtime.detail.manifest()"});
  runtime_detail_action(ActionId::RuntimeDetailLog, RuntimeDetailMode::Log,
                        {"runtime detail log", "detail log", "runtime log",
                         "iinuji.runtime.detail.log()"});
  runtime_detail_action(ActionId::RuntimeDetailTrace, RuntimeDetailMode::Trace,
                        {"runtime detail trace", "detail trace",
                         "runtime trace", "iinuji.runtime.detail.trace()"});
  registry.add(Action{
      .id = ActionId::RuntimeItemPrev,
      .label = "runtime item previous",
      .aliases = {"runtime item previous", "runtime item prev",
                  "runtime item up", "runtime up", "device up", "job up",
                  "iinuji.runtime.item.prev()"},
      .apply =
          [](CmdState &state) {
            state.screen = ScreenMode::Runtime;
            select_relative_runtime_item(state, -1);
          },
  });
  registry.add(Action{
      .id = ActionId::RuntimeItemNext,
      .label = "runtime item next",
      .aliases = {"runtime item next", "runtime item down", "runtime down",
                  "device down", "job down", "iinuji.runtime.item.next()"},
      .apply =
          [](CmdState &state) {
            state.screen = ScreenMode::Runtime;
            select_relative_runtime_item(state, 1);
          },
  });
  registry.add(Action{
      .id = ActionId::ConfigFiles,
      .label = "config files",
      .aliases = {"files", "config files", "show files",
                  "iinuji.config.files()"},
      .apply =
          [](CmdState &state) {
            state.screen = ScreenMode::Config;
            state.config.file_editor_open = false;
            state.status = "config files";
          },
  });
  registry.add(Action{
      .id = ActionId::ConfigShow,
      .label = "config show selected",
      .aliases = {"show config", "config show", "show selected config",
                  "iinuji.config.show()"},
      .apply =
          [](CmdState &state) {
            state.screen = ScreenMode::Config;
            state.config.file_editor_open = false;
            append_show_current_screen_logs(state);
          },
  });
  registry.add(Action{
      .id = ActionId::ConfigFileShow,
      .label = "config file show selected",
      .aliases = {"show file", "file show", "config file show",
                  "iinuji.config.file.show()"},
      .apply =
          [](CmdState &state) {
            state.screen = ScreenMode::Config;
            state.config.file_editor_open =
                selected_config_file(state) != nullptr;
            append_show_current_screen_logs(state);
          },
  });
  registry.add(Action{
      .id = ActionId::ConfigFilePrev,
      .label = "config file previous",
      .aliases = {"config file previous", "config file prev", "file previous",
                  "file prev", "config previous", "config prev", "config up",
                  "file up", "iinuji.config.file.prev()"},
      .apply =
          [](CmdState &state) {
            state.screen = ScreenMode::Config;
            select_relative_config_file(state, -1);
          },
  });
  registry.add(Action{
      .id = ActionId::ConfigFileNext,
      .label = "config file next",
      .aliases = {"config file next", "file next", "config next", "config down",
                  "file down", "iinuji.config.file.next()"},
      .apply =
          [](CmdState &state) {
            state.screen = ScreenMode::Config;
            select_relative_config_file(state, 1);
          },
  });
  registry.add(Action{
      .id = ActionId::ConfigFileFirst,
      .label = "config file first",
      .aliases = {"config file first", "config file home", "file first",
                  "file home", "config first", "config home",
                  "iinuji.config.file.first()"},
      .apply =
          [](CmdState &state) {
            state.screen = ScreenMode::Config;
            select_first_config_file(state);
          },
  });
  registry.add(Action{
      .id = ActionId::ConfigFileLast,
      .label = "config file last",
      .aliases = {"config file last", "config file end", "file last",
                  "file end", "config last", "config end",
                  "iinuji.config.file.last()"},
      .apply =
          [](CmdState &state) {
            state.screen = ScreenMode::Config;
            select_last_config_file(state);
          },
  });
  registry.add(Action{
      .id = ActionId::LatticeTargetPrev,
      .label = "lattice target previous",
      .aliases = {"lattice target previous", "lattice target prev",
                  "target previous", "target prev", "lattice previous",
                  "lattice prev", "lattice up", "target up",
                  "iinuji.lattice.target.prev()"},
      .apply =
          [](CmdState &state) {
            state.screen = ScreenMode::Lattice;
            select_relative_lattice_target(state, -1);
          },
  });
  registry.add(Action{
      .id = ActionId::LatticeTargetNext,
      .label = "lattice target next",
      .aliases = {"lattice target next", "target next", "lattice next",
                  "lattice down", "target down",
                  "iinuji.lattice.target.next()"},
      .apply =
          [](CmdState &state) {
            state.screen = ScreenMode::Lattice;
            select_relative_lattice_target(state, 1);
          },
  });
  registry.add(Action{
      .id = ActionId::LatticeTargetFirst,
      .label = "lattice target first",
      .aliases = {"lattice target first", "lattice target home", "target first",
                  "target home", "lattice first", "lattice home",
                  "iinuji.lattice.target.first()"},
      .apply =
          [](CmdState &state) {
            state.screen = ScreenMode::Lattice;
            select_first_lattice_target(state);
          },
  });
  registry.add(Action{
      .id = ActionId::LatticeTargetLast,
      .label = "lattice target last",
      .aliases = {"lattice target last", "lattice target end", "target last",
                  "target end", "lattice last", "lattice end",
                  "iinuji.lattice.target.last()"},
      .apply =
          [](CmdState &state) {
            state.screen = ScreenMode::Lattice;
            select_last_lattice_target(state);
          },
  });
  registry.add(Action{
      .id = ActionId::ToggleWorkspaceZoom,
      .label = "toggle workspace zoom",
      .aliases = {"zoom", "full", "fullscreen", "toggle zoom",
                  "cuwacunu.cmd --zoom", "cuwacunu.cmd --full",
                  "cuwacunu_cmd --zoom", "cuwacunu_cmd --full",
                  "iinuji_cmd --zoom", "iinuji_cmd --full",
                  "iinuji.workspace.zoom.toggle()"},
      .apply =
          [](CmdState &state) {
            if (!workspace_toggle_current_screen_zoom(state)) {
              state.status =
                  screen_label(state.screen) + " has no zoomable panel";
              return;
            }
            state.status = workspace_is_current_screen_zoomed(state)
                               ? "workspace full"
                               : "workspace split";
          },
  });
  registry.add(Action{
      .id = ActionId::RestoreWorkspaceSplit,
      .label = "restore workspace split",
      .aliases = {"split", "restore split", "split view", "unzoom",
                  "iinuji.workspace.split()"},
      .apply =
          [](CmdState &state) {
            if (workspace_restore_current_screen_split(state))
              state.status = "workspace split";
            else
              state.status = "workspace already split";
          },
  });
  registry.add(Action{
      .id = ActionId::Quit,
      .label = "quit",
      .aliases = {"quit", "q", "iinuji.quit()"},
      .apply =
          [](CmdState &state) {
            state.quit = true;
            state.status = "quit";
          },
  });
  registry.add(Action{
      .id = ActionId::Exit,
      .label = "exit",
      .aliases = {"exit", "x", "iinuji.exit()"},
      .apply =
          [](CmdState &state) {
            state.quit = true;
            state.status = "exit";
          },
  });
  return registry;
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
