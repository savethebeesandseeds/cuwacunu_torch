// SPDX-License-Identifier: MIT
#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "hero/marshal_hero/marshal_session.h"

namespace cuwacunu {
namespace hero {
namespace human_mcp {

struct human_defaults_t {
  std::filesystem::path marshal_root{};
  std::filesystem::path marshal_hero_binary{};
  std::string operator_id{};
  std::filesystem::path operator_signing_ssh_identity{};
};

struct app_context_t {
  std::filesystem::path global_config_path{};
  std::filesystem::path hero_config_path{};
  std::filesystem::path self_binary_path{};
  human_defaults_t defaults{};
};

struct interactive_resolution_input_t {
  std::string resolution_kind{};
  std::string reason{};
};

struct prompt_choice_option_t {
  std::string label{};
  bool enabled{true};
};

struct human_operator_inbox_t {
  std::vector<cuwacunu::hero::marshal::marshal_session_record_t> all_sessions{};
  std::vector<cuwacunu::hero::marshal::marshal_session_record_t>
      actionable_requests{};
  std::vector<cuwacunu::hero::marshal::marshal_session_record_t>
      unacknowledged_summaries{};
};

enum class operator_console_view_t { sessions, requests, summaries };

enum class operator_console_phase_filter_t {
  all,
  active,
  running_campaign,
  paused,
  idle,
  finished
};

enum class operator_session_state_t {
  unknown,
  running,
  waiting_clarification,
  waiting_governance,
  operator_paused,
  review,
  done,
};

[[nodiscard]] std::filesystem::path
resolve_human_hero_dsl_path(const std::filesystem::path &global_config_path);
[[nodiscard]] bool
load_human_defaults(const std::filesystem::path &hero_dsl_path,
                    const std::filesystem::path &global_config_path,
                    human_defaults_t *out, std::string *error);
[[nodiscard]] std::filesystem::path current_executable_path();

[[nodiscard]] bool execute_tool_json(const std::string &tool_name,
                                     std::string arguments_json,
                                     app_context_t *app,
                                     std::string *out_tool_result_json,
                                     std::string *out_error_message);
[[nodiscard]] bool tool_result_is_error(std::string_view tool_result_json);

[[nodiscard]] std::string build_tools_list_result_json();
[[nodiscard]] std::string build_tools_list_human_text();

[[nodiscard]] bool collect_human_operator_inbox(const app_context_t &app,
                                                human_operator_inbox_t *out,
                                                bool sync_markers,
                                                std::string *error);
void sort_sessions_newest_first(
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t> *rows);
void filter_sessions_for_console(
    const std::vector<cuwacunu::hero::marshal::marshal_session_record_t>
        &all_sessions,
    operator_console_phase_filter_t filter,
    std::vector<cuwacunu::hero::marshal::marshal_session_record_t> *out);
[[nodiscard]] std::optional<std::size_t> find_session_index_by_id(
    const std::vector<cuwacunu::hero::marshal::marshal_session_record_t>
        &sessions,
    std::string_view marshal_session_id);
void select_session_by_id(
    const std::vector<cuwacunu::hero::marshal::marshal_session_record_t>
        &sessions,
    std::string_view marshal_session_id, std::size_t *selected);
void cycle_operator_console_view(operator_console_view_t *view);
void cycle_operator_console_phase_filter(
    operator_console_phase_filter_t *filter);
[[nodiscard]] std::string_view
operator_console_view_label(operator_console_view_t view);
[[nodiscard]] std::string_view
operator_console_phase_filter_label(operator_console_phase_filter_t filter);
[[nodiscard]] bool is_clarification_pause_kind(std::string_view pause_kind);
[[nodiscard]] bool is_human_request_pause_kind(std::string_view pause_kind);
[[nodiscard]] std::string_view
human_request_kind_label(std::string_view pause_kind);
[[nodiscard]] operator_session_state_t operator_session_state(
    const cuwacunu::hero::marshal::marshal_session_record_t &session);
[[nodiscard]] std::string_view
operator_session_state_label(operator_session_state_t state);
[[nodiscard]] std::string operator_session_state_detail(
    const cuwacunu::hero::marshal::marshal_session_record_t &session);
[[nodiscard]] std::string operator_session_action_hint(
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    bool has_pending_review = true);
[[nodiscard]] std::string build_summary_effort_text(
    const cuwacunu::hero::marshal::marshal_session_record_t &session);
[[nodiscard]] std::string session_state_badge(
    const cuwacunu::hero::marshal::marshal_session_record_t &session);
[[nodiscard]] bool prompt_text_dialog(const std::string &title,
                                      const std::string &label,
                                      std::string *out_value, bool secret,
                                      bool allow_empty, bool *cancelled);
[[nodiscard]] bool prompt_choice_dialog(const std::string &title,
                                        const std::string &prompt,
                                        const std::vector<std::string> &options,
                                        std::size_t *out_index,
                                        bool *cancelled);
[[nodiscard]] bool
prompt_choice_dialog(const std::string &title, const std::string &prompt,
                     const std::vector<prompt_choice_option_t> &options,
                     std::size_t *out_index, bool *cancelled);
[[nodiscard]] bool prompt_yes_no_dialog(const std::string &title,
                                        const std::string &prompt,
                                        bool default_yes, bool *out_value,
                                        bool *cancelled);
[[nodiscard]] bool read_request_text_for_session(
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::string *out, std::string *error);
[[nodiscard]] bool read_summary_text_for_session(
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::string *out, std::string *error);
[[nodiscard]] std::string build_session_console_detail_text(
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    bool has_pending_review);
[[nodiscard]] bool collect_interactive_resolution_inputs(
    app_context_t *app,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    bool stop_direct, interactive_resolution_input_t *out, bool *cancelled,
    std::string *error);
[[nodiscard]] bool build_request_answer_and_resume(
    app_context_t *app,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::string answer, std::string *out_structured, std::string *out_error);
[[nodiscard]] bool build_resolution_and_apply(
    app_context_t *app,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::string resolution_kind, std::string reason,
    std::string *out_structured, std::string *out_error);
[[nodiscard]] bool acknowledge_session_summary(
    app_context_t *app,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::string note, std::string *out_structured, std::string *out_error);
[[nodiscard]] bool pause_marshal_session(
    app_context_t *app,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    bool force, std::string *out_structured, std::string *out_error);
[[nodiscard]] bool resume_marshal_session(
    app_context_t *app,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::string *out_structured, std::string *out_error);
[[nodiscard]] bool continue_marshal_session(
    app_context_t *app,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::string instruction, std::string *out_structured,
    std::string *out_error);
[[nodiscard]] bool terminate_marshal_session(
    app_context_t *app,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    bool force, std::string *out_structured, std::string *out_error);

[[nodiscard]] bool run_interactive_operator_console(app_context_t *app,
                                                    std::string *error);

void run_jsonrpc_stdio_loop(app_context_t *app);

} // namespace human_mcp
} // namespace hero
} // namespace cuwacunu
