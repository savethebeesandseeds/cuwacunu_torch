#pragma once

#include <array>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_map>

#include "camahjucunu/BNF/implementations/canonical_path/canonical_path.h"

#include "iinuji/iinuji_cmd/commands/iinuji.paths.h"
#include "iinuji/iinuji_cmd/commands/iinuji.screen.h"
#include "iinuji/iinuji_cmd/commands/iinuji.state.flow.h"
#include "iinuji/iinuji_cmd/views/board/commands.h"
#include "iinuji/iinuji_cmd/views/config/commands.h"
#include "iinuji/iinuji_cmd/views/data/commands.h"
#include "iinuji/iinuji_cmd/views/tsiemene/commands.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

struct IinujiPathHandlers {
  using CallHandlerId = canonical_paths::CallId;

  CmdState& state;
  IinujiScreen screen{state};
  IinujiStateFlow state_flow{state};

  [[nodiscard]] static const std::unordered_map<std::string_view, CallHandlerId>& call_handlers() {
    return canonical_paths::call_map();
  }

  template <std::size_t N>
  [[nodiscard]] static bool is_call_segments_prefix(const cuwacunu::camahjucunu::canonical_path_t& path,
                                                    const std::array<std::string_view, N>& segs) {
    if (path.path_kind != cuwacunu::camahjucunu::canonical_path_kind_e::Call) return false;
    if (path.segments.size() < N) return false;
    for (std::size_t i = 0; i < N; ++i) {
      if (path.segments[i] != segs[i]) return false;
    }
    return true;
  }

  template <std::size_t N>
  [[nodiscard]] static bool is_call_segments(const cuwacunu::camahjucunu::canonical_path_t& path,
                                             const std::array<std::string_view, N>& segs) {
    return is_call_segments_prefix(path, segs) && path.segments.size() == N;
  }

  template <std::size_t N>
  [[nodiscard]] static bool is_call_with_optional_tail_atom(const cuwacunu::camahjucunu::canonical_path_t& path,
                                                            const std::array<std::string_view, N>& segs) {
    if (!is_call_segments_prefix(path, segs)) return false;
    if (path.segments.size() == N) return true;
    return path.segments.size() == (N + 1) && path.args.empty();
  }

  [[nodiscard]] static bool is_data_plot_call(const cuwacunu::camahjucunu::canonical_path_t& path) {
    static constexpr std::array<std::string_view, 4> kSegs = {
        "iinuji", "view", "data", "plot"
    };
    return is_call_segments(path, kSegs);
  }

  [[nodiscard]] static bool is_board_select_index_call(const cuwacunu::camahjucunu::canonical_path_t& path) {
    static constexpr std::array<std::string_view, 4> kSegs = {
        "iinuji", "board", "select", "index"
    };
    return is_call_with_optional_tail_atom(path, kSegs);
  }

  [[nodiscard]] static bool is_tsi_tab_index_call(const cuwacunu::camahjucunu::canonical_path_t& path) {
    static constexpr std::array<std::string_view, 4> kSegs = {
        "iinuji", "tsi", "tab", "index"
    };
    return is_call_with_optional_tail_atom(path, kSegs);
  }

  [[nodiscard]] static bool is_tsi_tab_id_call(const cuwacunu::camahjucunu::canonical_path_t& path) {
    static constexpr std::array<std::string_view, 4> kSegs = {
        "iinuji", "tsi", "tab", "id"
    };
    return is_call_with_optional_tail_atom(path, kSegs);
  }

  [[nodiscard]] static bool is_config_tab_index_call(const cuwacunu::camahjucunu::canonical_path_t& path) {
    static constexpr std::array<std::string_view, 4> kSegs = {
        "iinuji", "config", "tab", "index"
    };
    return is_call_with_optional_tail_atom(path, kSegs);
  }

  [[nodiscard]] static bool is_config_tab_id_call(const cuwacunu::camahjucunu::canonical_path_t& path) {
    static constexpr std::array<std::string_view, 4> kSegs = {
        "iinuji", "config", "tab", "id"
    };
    return is_call_with_optional_tail_atom(path, kSegs);
  }

  [[nodiscard]] static bool is_data_x_call(const cuwacunu::camahjucunu::canonical_path_t& path) {
    static constexpr std::array<std::string_view, 3> kSegs = {
        "iinuji", "data", "x"
    };
    return is_call_segments(path, kSegs);
  }

  [[nodiscard]] static bool is_data_mask_call(const cuwacunu::camahjucunu::canonical_path_t& path) {
    static constexpr std::array<std::string_view, 3> kSegs = {
        "iinuji", "data", "mask"
    };
    return is_call_segments(path, kSegs);
  }

  [[nodiscard]] static bool is_data_ch_index_call(const cuwacunu::camahjucunu::canonical_path_t& path) {
    static constexpr std::array<std::string_view, 4> kSegs = {
        "iinuji", "data", "ch", "index"
    };
    return is_call_with_optional_tail_atom(path, kSegs);
  }

  [[nodiscard]] static bool is_data_sample_index_call(const cuwacunu::camahjucunu::canonical_path_t& path) {
    static constexpr std::array<std::string_view, 4> kSegs = {
        "iinuji", "data", "sample", "index"
    };
    return is_call_with_optional_tail_atom(path, kSegs);
  }

  [[nodiscard]] static bool is_data_dim_index_call(const cuwacunu::camahjucunu::canonical_path_t& path) {
    static constexpr std::array<std::string_view, 4> kSegs = {
        "iinuji", "data", "dim", "index"
    };
    return is_call_with_optional_tail_atom(path, kSegs);
  }

  [[nodiscard]] static bool is_data_dim_id_call(const cuwacunu::camahjucunu::canonical_path_t& path) {
    static constexpr std::array<std::string_view, 4> kSegs = {
        "iinuji", "data", "dim", "id"
    };
    return is_call_with_optional_tail_atom(path, kSegs);
  }

  [[nodiscard]] static bool parse_view_bool(std::string value, bool* out, bool* toggle) {
    if (!out || !toggle) return false;
    *toggle = false;
    value = to_lower_copy(value);
    if (value == "toggle") {
      *toggle = true;
      return true;
    }
    if (value == "on" || value == "true" || value == "1") {
      *out = true;
      return true;
    }
    if (value == "off" || value == "false" || value == "0") {
      *out = false;
      return true;
    }
    return false;
  }

  [[nodiscard]] static bool get_arg_value(const cuwacunu::camahjucunu::canonical_path_t& path,
                                          std::string_view key,
                                          std::string* out) {
    if (!out) return false;
    for (const auto& arg : path.args) {
      if (arg.key == key) {
        *out = arg.value;
        return true;
      }
    }
    return false;
  }

  [[nodiscard]] static bool parse_positive_arg(const cuwacunu::camahjucunu::canonical_path_t& path,
                                               std::size_t* out) {
    if (!out) return false;
    std::string raw;
    if (get_arg_value(path, "n", &raw) || get_arg_value(path, "index", &raw) || get_arg_value(path, "value", &raw)) {
      return parse_positive_index(raw, out);
    }
    return false;
  }

  [[nodiscard]] static bool parse_string_arg(const cuwacunu::camahjucunu::canonical_path_t& path,
                                             std::string* out) {
    if (!out) return false;
    return get_arg_value(path, "value", out) || get_arg_value(path, "id", out);
  }

  [[nodiscard]] static bool parse_positive_arg_or_tail(const cuwacunu::camahjucunu::canonical_path_t& path,
                                                       std::size_t prefix_size,
                                                       std::size_t* out) {
    if (parse_positive_arg(path, out)) return true;
    if (!out) return false;
    if (!path.args.empty() || path.segments.size() != prefix_size + 1) return false;
    return canonical_path_tokens::parse_index_atom(path.segments.back(), out);
  }

  [[nodiscard]] static bool parse_string_arg_or_tail(const cuwacunu::camahjucunu::canonical_path_t& path,
                                                     std::size_t prefix_size,
                                                     std::string* out) {
    if (parse_string_arg(path, out)) return true;
    if (!out) return false;
    if (!path.args.empty() || path.segments.size() != prefix_size + 1) return false;
    *out = path.segments.back();
    return !out->empty();
  }

  [[nodiscard]] static int saturating_add_non_negative(int base, int delta) {
    using Limits = std::numeric_limits<int>;
    long long next = static_cast<long long>(base) + static_cast<long long>(delta);
    if (next < 0) return 0;
    if (next > static_cast<long long>(Limits::max())) return Limits::max();
    return static_cast<int>(next);
  }

  [[nodiscard]] static int saturating_add_signed(int base, int delta) {
    using Limits = std::numeric_limits<int>;
    long long next = static_cast<long long>(base) + static_cast<long long>(delta);
    if (next < static_cast<long long>(Limits::min())) return Limits::min();
    if (next > static_cast<long long>(Limits::max())) return Limits::max();
    return static_cast<int>(next);
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch_text(const std::string& raw,
                     PushInfo&& push_info,
                     PushWarn&& push_warn,
                     PushErr&& push_err) const {
    auto append_noop = [](const std::string&, const std::string&, const std::string&) {};
    return dispatch_text(raw, push_info, push_warn, push_err, append_noop);
  }

  template <class PushInfo, class PushWarn, class PushErr, class AppendLog>
  bool dispatch_text(const std::string& raw,
                     PushInfo&& push_info,
                     PushWarn&& push_warn,
                     PushErr&& push_err,
                     AppendLog&& append_log) const {
    if (raw.rfind("iinuji.", 0) != 0) return false;

    std::string normalized = raw;
    // UX shorthand: allow argumentless canonical calls without "()".
    if (normalized.find('(') == std::string::npos &&
        normalized.find('@') == std::string::npos) {
      normalized += "()";
    }

    auto path = cuwacunu::camahjucunu::decode_canonical_path(normalized);
    if (!path.ok) {
      push_err("invalid iinuji path: " + path.error);
      return true;
    }
    std::string verror;
    if (!cuwacunu::camahjucunu::validate_canonical_path(path, &verror)) {
      push_err("invalid iinuji path: " + verror);
      return true;
    }
    if (dispatch(path, push_info, push_warn, push_err, append_log)) return true;

    push_err("unsupported iinuji call: " + path.canonical_identity);
    return true;
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch_canonical_text(const std::string& canonical_raw,
                               PushInfo&& push_info,
                               PushWarn&& push_warn,
                               PushErr&& push_err) const {
    auto append_noop = [](const std::string&, const std::string&, const std::string&) {};
    return dispatch_canonical_text(canonical_raw, push_info, push_warn, push_err, append_noop);
  }

  template <class PushInfo, class PushWarn, class PushErr, class AppendLog>
  bool dispatch_canonical_text(const std::string& canonical_raw,
                               PushInfo&& push_info,
                               PushWarn&& push_warn,
                               PushErr&& push_err,
                               AppendLog&& append_log) const {
    if (canonical_raw.rfind("iinuji.", 0) != 0) {
      push_err("internal canonical call must start with iinuji.: " + canonical_raw);
      return false;
    }

    auto path = cuwacunu::camahjucunu::decode_canonical_path(canonical_raw);
    if (!path.ok) {
      push_err("invalid canonical iinuji path: " + path.error);
      return false;
    }
    std::string verror;
    if (!cuwacunu::camahjucunu::validate_canonical_path(path, &verror)) {
      push_err("invalid canonical iinuji path: " + verror);
      return false;
    }
    if (path.path_kind != cuwacunu::camahjucunu::canonical_path_kind_e::Call) {
      push_err("internal canonical call must be a call path: " + path.canonical);
      return false;
    }
    if (path.canonical != canonical_raw) {
      push_err("internal canonical call not exact: " + canonical_raw + " -> " + path.canonical);
      return false;
    }
    if (dispatch(path, push_info, push_warn, push_err, append_log)) return true;

    push_err("unsupported canonical iinuji call: " + path.canonical_identity);
    return false;
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch(const cuwacunu::camahjucunu::canonical_path_t& path,
                PushInfo&& push_info,
                PushWarn&& push_warn,
                PushErr&& push_err) const {
    auto append_noop = [](const std::string&, const std::string&, const std::string&) {};
    return dispatch(path, push_info, push_warn, push_err, append_noop);
  }

  template <class PushInfo, class PushWarn, class PushErr, class AppendLog>
  bool dispatch(const cuwacunu::camahjucunu::canonical_path_t& path,
                PushInfo&& push_info,
                PushWarn&& push_warn,
                PushErr&& push_err,
                AppendLog&& append_log) const {
    if (path.path_kind != cuwacunu::camahjucunu::canonical_path_kind_e::Call) {
      push_err("iinuji terminal supports call paths only: " + path.canonical);
      return true;
    }

    if (is_data_plot_call(path)) return dispatch_data_plot_call(path, push_info, push_err);
    if (is_board_select_index_call(path)) return dispatch_board_select_index(path, push_info, push_warn, push_err);
    if (is_tsi_tab_index_call(path)) return dispatch_tsi_tab_index(path, push_info, push_warn, push_err);
    if (is_tsi_tab_id_call(path)) return dispatch_tsi_tab_id(path, push_info, push_warn, push_err);
    if (is_config_tab_index_call(path)) return dispatch_config_tab_index(path, push_info, push_warn, push_err);
    if (is_config_tab_id_call(path)) return dispatch_config_tab_id(path, push_info, push_warn, push_err);
    if (is_data_x_call(path)) return dispatch_data_x(path, push_info, push_err);
    if (is_data_mask_call(path)) return dispatch_data_mask(path, push_info, push_err);
    if (is_data_ch_index_call(path)) return dispatch_data_ch_index(path, push_info, push_warn, push_err);
    if (is_data_sample_index_call(path)) return dispatch_data_sample_index(path, push_info, push_warn, push_err);
    if (is_data_dim_index_call(path)) return dispatch_data_dim_index(path, push_info, push_warn, push_err);
    if (is_data_dim_id_call(path)) return dispatch_data_dim_id(path, push_info, push_warn, push_err);

    const auto it = call_handlers().find(path.canonical_identity);
    if (it == call_handlers().end()) return false;

    switch (it->second) {
      case CallHandlerId::HelpOpen:
        state.help_view = true;
        state.help_scroll_y = 0;
        state.help_scroll_x = 0;
        push_info("help overlay=open (Esc or click [x] to close)");
        return true;
      case CallHandlerId::HelpClose:
        state.help_view = false;
        push_info("help overlay=closed");
        return true;
      case CallHandlerId::HelpScrollUp:
        state.help_view = true;
        state.help_scroll_y = saturating_add_non_negative(state.help_scroll_y, -1);
        push_info("help scroll=up");
        return true;
      case CallHandlerId::HelpScrollDown:
        state.help_view = true;
        state.help_scroll_y = saturating_add_non_negative(state.help_scroll_y, +1);
        push_info("help scroll=down");
        return true;
      case CallHandlerId::HelpScrollLeft:
        state.help_view = true;
        state.help_scroll_x = saturating_add_non_negative(state.help_scroll_x, -10);
        push_info("help scroll=left");
        return true;
      case CallHandlerId::HelpScrollRight:
        state.help_view = true;
        state.help_scroll_x = saturating_add_non_negative(state.help_scroll_x, +10);
        push_info("help scroll=right");
        return true;
      case CallHandlerId::HelpScrollPageUp:
        state.help_view = true;
        state.help_scroll_y = saturating_add_non_negative(state.help_scroll_y, -10);
        push_info("help scroll=page-up");
        return true;
      case CallHandlerId::HelpScrollPageDown:
        state.help_view = true;
        state.help_scroll_y = saturating_add_non_negative(state.help_scroll_y, +10);
        push_info("help scroll=page-down");
        return true;
      case CallHandlerId::HelpScrollHome:
        state.help_view = true;
        state.help_scroll_y = 0;
        state.help_scroll_x = 0;
        push_info("help scroll=home");
        return true;
      case CallHandlerId::HelpScrollEnd:
        state.help_view = true;
        state.help_scroll_y = std::numeric_limits<int>::max();
        push_info("help scroll=end");
        return true;
      case CallHandlerId::AppQuit:
      case CallHandlerId::AppExit:
        state.running = false;
        push_info("application=exit");
        return true;
      case CallHandlerId::ScreenHome:
        screen.home();
        push_info("screen=home");
        return true;
      case CallHandlerId::ScreenBoard:
        screen.board();
        push_info("screen=board");
        return true;
      case CallHandlerId::ScreenLogs:
        screen.logs();
        push_info("screen=logs");
        return true;
      case CallHandlerId::ScreenTsi:
        screen.tsi();
        push_info("screen=tsi");
        return true;
      case CallHandlerId::ScreenData:
      case CallHandlerId::ViewData:
        screen.data();
        push_info("screen=data");
        return true;
      case CallHandlerId::ScreenConfig:
        screen.config();
        push_info("screen=config");
        return true;
      case CallHandlerId::ViewTsi:
        screen.tsi();
        push_info("screen=tsi");
        return true;
      case CallHandlerId::RefreshAll:
      case CallHandlerId::ReloadConfig:
      case CallHandlerId::ConfigReload:
        state_flow.reload_config_and_board();
        if (!state.config.ok) push_err("reload config failed: " + state.config.error);
        else push_info("config reloaded: tabs=" + std::to_string(state.config.tabs.size()));
        if (!state.board.ok) push_err("board reload after config failed: " + state.board.error);
        else push_info("board reloaded");
        if (!state.data.ok) push_err("data reload after config failed: " + state.data.error);
        else push_info("data reloaded");
        return true;
      case CallHandlerId::ReloadBoard:
        state_flow.reload_board();
        if (state.board.ok) push_info("board reloaded");
        else push_err("reload board failed: " + state.board.error);
        return true;
      case CallHandlerId::ReloadData:
      case CallHandlerId::DataReload:
        state_flow.reload_data();
        if (state.data.ok) push_info("data reloaded");
        else push_err("reload data failed: " + state.data.error);
        return true;
      case CallHandlerId::ShowCurrent:
        if (state.screen == ScreenMode::Home) {
          dispatch_show_home(append_log);
          return true;
        }
        if (state.screen == ScreenMode::Logs) {
          dispatch_show_logs(append_log);
          return true;
        }
        if (state.screen == ScreenMode::Config) {
          handle_config_show(state, push_warn, append_log);
          return true;
        }
        if (state.screen == ScreenMode::Tsiemene) {
          handle_tsi_show(state, push_warn, append_log);
          return true;
        }
        if (state.screen == ScreenMode::Data) {
          handle_data_show(state, append_log);
          return true;
        }
        handle_board_show(state, push_warn, push_err, append_log);
        return true;
      case CallHandlerId::ShowHome:
        dispatch_show_home(append_log);
        return true;
      case CallHandlerId::ShowBoard:
        handle_board_show(state, push_warn, push_err, append_log);
        return true;
      case CallHandlerId::ShowLogs:
        dispatch_show_logs(append_log);
        return true;
      case CallHandlerId::ShowData:
        handle_data_show(state, append_log);
        return true;
      case CallHandlerId::ShowTsi:
        handle_tsi_show(state, push_warn, append_log);
        return true;
      case CallHandlerId::ShowConfig:
      case CallHandlerId::ConfigShow:
      case CallHandlerId::ConfigTabShow:
        handle_config_show(state, push_warn, append_log);
        return true;
      case CallHandlerId::LogsClear:
        cuwacunu::piaabo::dlog_clear_buffer();
        screen.logs();
        push_info("logs cleared");
        return true;
      case CallHandlerId::LogsScrollUp:
        screen.logs();
        state.logs.pending_scroll_y = saturating_add_signed(state.logs.pending_scroll_y, -3);
        state.logs.auto_follow = false;
        push_info("logs scroll=up");
        return true;
      case CallHandlerId::LogsScrollDown:
        screen.logs();
        state.logs.pending_scroll_y = saturating_add_signed(state.logs.pending_scroll_y, +3);
        state.logs.auto_follow = false;
        push_info("logs scroll=down");
        return true;
      case CallHandlerId::LogsScrollPageUp:
        screen.logs();
        state.logs.pending_scroll_y = saturating_add_signed(state.logs.pending_scroll_y, -10);
        state.logs.auto_follow = false;
        push_info("logs scroll=page-up");
        return true;
      case CallHandlerId::LogsScrollPageDown:
        screen.logs();
        state.logs.pending_scroll_y = saturating_add_signed(state.logs.pending_scroll_y, +10);
        state.logs.auto_follow = false;
        push_info("logs scroll=page-down");
        return true;
      case CallHandlerId::LogsScrollHome:
        screen.logs();
        state.logs.pending_scroll_y = 0;
        state.logs.pending_scroll_x = 0;
        state.logs.pending_jump_end = false;
        state.logs.pending_jump_home = true;
        state.logs.auto_follow = false;
        push_info("logs scroll=home");
        return true;
      case CallHandlerId::LogsScrollEnd:
        screen.logs();
        state.logs.pending_scroll_y = 0;
        state.logs.pending_scroll_x = 0;
        state.logs.pending_jump_home = false;
        state.logs.pending_jump_end = true;
        state.logs.auto_follow = true;
        push_info("logs scroll=end");
        return true;
      case CallHandlerId::BoardList:
        if (!state.board.ok) {
          push_err("board invalid: " + state.board.error);
          return true;
        }
        if (state.board.board.circuits.empty()) {
          push_warn("no circuits");
          return true;
        }
        for (std::size_t i = 0; i < state.board.board.circuits.size(); ++i) {
          const auto& c = state.board.board.circuits[i];
          append_log("[" + std::to_string(i + 1) + "] " + c.name, "list", "#d0d0d0");
        }
        return true;
      case CallHandlerId::BoardSelectNext:
        if (!select_next_board_circuit(state)) {
          push_warn("no circuits");
          return true;
        }
        screen.board();
        push_info("selected circuit=" + std::to_string(state.board.selected_circuit + 1));
        return true;
      case CallHandlerId::BoardSelectPrev:
        if (!select_prev_board_circuit(state)) {
          push_warn("no circuits");
          return true;
        }
        screen.board();
        push_info("selected circuit=" + std::to_string(state.board.selected_circuit + 1));
        return true;
      case CallHandlerId::TsiTabs: {
        const auto& docs = tsi_node_docs();
        if (docs.empty()) {
          push_warn("no tsi tabs");
          return true;
        }
        for (std::size_t i = 0; i < docs.size(); ++i) {
          append_log("[" + std::to_string(i + 1) + "] " + docs[i].id, "tsi.tabs", "#d0d0d0");
        }
        screen.tsi();
        return true;
      }
      case CallHandlerId::TsiTabNext:
        select_next_tsi_tab(state);
        screen.tsi();
        push_info("selected tsi tab=" + std::to_string(state.tsiemene.selected_tab + 1));
        return true;
      case CallHandlerId::TsiTabPrev:
        select_prev_tsi_tab(state);
        screen.tsi();
        push_info("selected tsi tab=" + std::to_string(state.tsiemene.selected_tab + 1));
        return true;
      case CallHandlerId::ConfigTabs:
        if (!config_has_tabs(state)) {
          push_warn("no config tabs");
          return true;
        }
        for (std::size_t i = 0; i < state.config.tabs.size(); ++i) {
          const auto& tab = state.config.tabs[i];
          append_log(
              "[" + std::to_string(i + 1) + "] " + tab.id + (tab.ok ? "" : " (err)"),
              "tabs",
              "#d0d0d0");
        }
        screen.config();
        return true;
      case CallHandlerId::ConfigTabNext:
        if (!config_has_tabs(state)) {
          push_warn("no config tabs");
          return true;
        }
        select_next_tab(state);
        screen.config();
        push_info("selected tab=" + std::to_string(state.config.selected_tab + 1));
        return true;
      case CallHandlerId::ConfigTabPrev:
        if (!config_has_tabs(state)) {
          push_warn("no config tabs");
          return true;
        }
        select_prev_tab(state);
        screen.config();
        push_info("selected tab=" + std::to_string(state.config.selected_tab + 1));
        return true;
      case CallHandlerId::DataChannels:
        if (!data_has_channels(state)) {
          push_warn("no data channels");
          return true;
        }
        for (std::size_t i = 0; i < state.data.channels.size(); ++i) {
          const auto& c = state.data.channels[i];
          append_log(
              "[" + std::to_string(i + 1) + "] " + c.interval + "/" + c.record_type +
                  " seq=" + std::to_string(c.seq_length) +
                  " fut=" + std::to_string(c.future_seq_length),
              "data.channels",
              "#d0d0d0");
        }
        screen.data();
        return true;
      case CallHandlerId::DataPlotOn:
        screen.data();
        state.data.plot_view = true;
        push_info("data plotview=on");
        return true;
      case CallHandlerId::DataPlotOff:
        screen.data();
        state.data.plot_view = false;
        push_info("data plotview=off");
        return true;
      case CallHandlerId::DataPlotToggle:
        screen.data();
        state.data.plot_view = !state.data.plot_view;
        push_info(std::string("data plotview=") + (state.data.plot_view ? "on" : "off"));
        return true;
      case CallHandlerId::DataPlotModeSeq:
        screen.data();
        state.data.plot_mode = DataPlotMode::SeqLength;
        clamp_data_plot_mode(state);
        push_info("data plot=seq");
        return true;
      case CallHandlerId::DataPlotModeFuture:
        screen.data();
        state.data.plot_mode = DataPlotMode::FutureSeqLength;
        clamp_data_plot_mode(state);
        push_info("data plot=future");
        return true;
      case CallHandlerId::DataPlotModeWeight:
        screen.data();
        state.data.plot_mode = DataPlotMode::ChannelWeight;
        clamp_data_plot_mode(state);
        push_info("data plot=weight");
        return true;
      case CallHandlerId::DataPlotModeNorm:
        screen.data();
        state.data.plot_mode = DataPlotMode::NormWindow;
        clamp_data_plot_mode(state);
        push_info("data plot=norm");
        return true;
      case CallHandlerId::DataPlotModeBytes:
        screen.data();
        state.data.plot_mode = DataPlotMode::FileBytes;
        clamp_data_plot_mode(state);
        push_info("data plot=bytes");
        return true;
      case CallHandlerId::DataAxisToggle:
        screen.data();
        state.data.plot_x_axis = next_data_plot_x_axis(state.data.plot_x_axis);
        clamp_data_plot_x_axis(state);
        push_info("data x=" + data_plot_x_axis_token(state.data.plot_x_axis));
        return true;
      case CallHandlerId::DataAxisIdx:
        screen.data();
        state.data.plot_x_axis = DataPlotXAxis::Index;
        clamp_data_plot_x_axis(state);
        push_info("data x=idx");
        return true;
      case CallHandlerId::DataAxisKey:
        screen.data();
        state.data.plot_x_axis = DataPlotXAxis::KeyValue;
        clamp_data_plot_x_axis(state);
        push_info("data x=key");
        return true;
      case CallHandlerId::DataMaskOn:
        screen.data();
        state.data.plot_mask_overlay = true;
        push_info("data mask=on");
        return true;
      case CallHandlerId::DataMaskOff:
        screen.data();
        state.data.plot_mask_overlay = false;
        push_info("data mask=off");
        return true;
      case CallHandlerId::DataMaskToggle:
        screen.data();
        state.data.plot_mask_overlay = !state.data.plot_mask_overlay;
        push_info(std::string("data mask=") + (state.data.plot_mask_overlay ? "on" : "off"));
        return true;
      case CallHandlerId::DataChNext:
        if (!data_has_channels(state)) {
          push_warn("no data channels");
          return true;
        }
        select_next_data_channel(state);
        screen.data();
        push_info("selected data channel=" + std::to_string(state.data.selected_channel + 1));
        return true;
      case CallHandlerId::DataChPrev:
        if (!data_has_channels(state)) {
          push_warn("no data channels");
          return true;
        }
        select_prev_data_channel(state);
        screen.data();
        push_info("selected data channel=" + std::to_string(state.data.selected_channel + 1));
        return true;
      case CallHandlerId::DataSampleNext:
        if (state.data.plot_sample_count == 0) {
          push_warn("no data samples loaded");
          return true;
        }
        select_next_data_sample(state);
        screen.data();
        push_info("selected data sample=" + std::to_string(state.data.plot_sample_index + 1));
        return true;
      case CallHandlerId::DataSamplePrev:
        if (state.data.plot_sample_count == 0) {
          push_warn("no data samples loaded");
          return true;
        }
        select_prev_data_sample(state);
        screen.data();
        push_info("selected data sample=" + std::to_string(state.data.plot_sample_index + 1));
        return true;
      case CallHandlerId::DataSampleRandom:
      case CallHandlerId::DataSampleRand:
        if (state.data.plot_sample_count == 0) {
          push_warn("no data samples loaded");
          return true;
        }
        select_random_data_sample(state);
        screen.data();
        push_info("selected data sample=" + std::to_string(state.data.plot_sample_index + 1));
        return true;
      case CallHandlerId::DataDimNext:
        if (state.data.plot_D == 0) {
          push_warn("no tensor dims available");
          return true;
        }
        select_next_data_dim(state);
        screen.data();
        push_info("selected data dim=" + std::to_string(state.data.plot_feature_dim + 1));
        return true;
      case CallHandlerId::DataDimPrev:
        if (state.data.plot_D == 0) {
          push_warn("no tensor dims available");
          return true;
        }
        select_prev_data_dim(state);
        screen.data();
        push_info("selected data dim=" + std::to_string(state.data.plot_feature_dim + 1));
        return true;
      case CallHandlerId::DataFocusNext: {
        const std::size_t count = data_nav_focus_count();
        if (count > 0) {
          const auto idx = static_cast<std::size_t>(state.data.nav_focus);
          state.data.nav_focus = static_cast<DataNavFocus>((idx + 1u) % count);
        }
        clamp_data_nav_focus(state);
        screen.data();
        push_info("data.focus=" + data_nav_focus_name(state.data.nav_focus));
        return true;
      }
      case CallHandlerId::DataFocusPrev: {
        const std::size_t count = data_nav_focus_count();
        if (count > 0) {
          const auto idx = static_cast<std::size_t>(state.data.nav_focus);
          state.data.nav_focus = static_cast<DataNavFocus>((idx + count - 1u) % count);
        }
        clamp_data_nav_focus(state);
        screen.data();
        push_info("data.focus=" + data_nav_focus_name(state.data.nav_focus));
        return true;
      }
      case CallHandlerId::DataFocusChannel:
        state.data.nav_focus = DataNavFocus::Channel;
        clamp_data_nav_focus(state);
        screen.data();
        push_info("data.focus=" + data_nav_focus_name(state.data.nav_focus));
        return true;
      case CallHandlerId::DataFocusSample:
        state.data.nav_focus = DataNavFocus::Sample;
        clamp_data_nav_focus(state);
        screen.data();
        push_info("data.focus=" + data_nav_focus_name(state.data.nav_focus));
        return true;
      case CallHandlerId::DataFocusDim:
        state.data.nav_focus = DataNavFocus::Dim;
        clamp_data_nav_focus(state);
        screen.data();
        push_info("data.focus=" + data_nav_focus_name(state.data.nav_focus));
        return true;
      case CallHandlerId::DataFocusPlot:
        state.data.nav_focus = DataNavFocus::PlotMode;
        clamp_data_nav_focus(state);
        screen.data();
        push_info("data.focus=" + data_nav_focus_name(state.data.nav_focus));
        return true;
      case CallHandlerId::DataFocusXAxis:
        state.data.nav_focus = DataNavFocus::XAxis;
        clamp_data_nav_focus(state);
        screen.data();
        push_info("data.focus=" + data_nav_focus_name(state.data.nav_focus));
        return true;
      case CallHandlerId::DataFocusMask:
        state.data.nav_focus = DataNavFocus::Mask;
        clamp_data_nav_focus(state);
        screen.data();
        push_info("data.focus=" + data_nav_focus_name(state.data.nav_focus));
        return true;
      case CallHandlerId::LogsSettingsLevelDebug:
        state.logs.level_filter = LogsLevelFilter::DebugOrHigher;
        screen.logs();
        push_info("logs.level=DEBUG+");
        return true;
      case CallHandlerId::LogsSettingsLevelInfo:
        state.logs.level_filter = LogsLevelFilter::InfoOrHigher;
        screen.logs();
        push_info("logs.level=INFO+");
        return true;
      case CallHandlerId::LogsSettingsLevelWarning:
        state.logs.level_filter = LogsLevelFilter::WarningOrHigher;
        screen.logs();
        push_info("logs.level=WARNING+");
        return true;
      case CallHandlerId::LogsSettingsLevelError:
        state.logs.level_filter = LogsLevelFilter::ErrorOrHigher;
        screen.logs();
        push_info("logs.level=ERROR+");
        return true;
      case CallHandlerId::LogsSettingsLevelFatal:
        state.logs.level_filter = LogsLevelFilter::FatalOnly;
        screen.logs();
        push_info("logs.level=FATAL");
        return true;
      case CallHandlerId::LogsSettingsSelectPrev:
        if (logs_settings_count() > 0) {
          state.logs.selected_setting =
              (state.logs.selected_setting + logs_settings_count() - 1u) % logs_settings_count();
        } else {
          state.logs.selected_setting = 0;
        }
        screen.logs();
        push_info("logs.settings.cursor=prev");
        return true;
      case CallHandlerId::LogsSettingsSelectNext:
        if (logs_settings_count() > 0) {
          state.logs.selected_setting =
              (state.logs.selected_setting + 1u) % logs_settings_count();
        } else {
          state.logs.selected_setting = 0;
        }
        screen.logs();
        push_info("logs.settings.cursor=next");
        return true;
      case CallHandlerId::LogsSettingsDateToggle:
        state.logs.show_date = !state.logs.show_date;
        screen.logs();
        push_info(std::string("logs.date=") + (state.logs.show_date ? "on" : "off"));
        return true;
      case CallHandlerId::LogsSettingsThreadToggle:
        state.logs.show_thread = !state.logs.show_thread;
        screen.logs();
        push_info(std::string("logs.thread=") + (state.logs.show_thread ? "on" : "off"));
        return true;
      case CallHandlerId::LogsSettingsColorToggle:
        state.logs.show_color = !state.logs.show_color;
        screen.logs();
        push_info(std::string("logs.color=") + (state.logs.show_color ? "on" : "off"));
        return true;
      case CallHandlerId::LogsSettingsFollowToggle:
        state.logs.auto_follow = !state.logs.auto_follow;
        screen.logs();
        push_info(std::string("logs.follow=") + (state.logs.auto_follow ? "on" : "off"));
        return true;
      case CallHandlerId::LogsSettingsMouseCaptureToggle:
        state.logs.mouse_capture = !state.logs.mouse_capture;
        screen.logs();
        push_info(std::string("logs.mouse_capture=") + (state.logs.mouse_capture ? "on" : "off"));
        return true;
    }

    push_warn("unhandled iinuji call");
    return true;
  }

  template <class AppendLog>
  void dispatch_show_home(AppendLog&& append_log) const {
    append_log("screen=home", "show", "#d8d8ff");
    append_log(
        "board.circuits=" + std::to_string(state.board.board.circuits.size()) +
            " config.tabs=" + std::to_string(state.config.tabs.size()),
        "show",
        "#d8d8ff");
    append_log(
        "dlogs.buffer=" + std::to_string(cuwacunu::piaabo::dlog_buffer_size()) + "/" +
            std::to_string(cuwacunu::piaabo::dlog_buffer_capacity()),
        "show",
        "#d8d8ff");
  }

  template <class AppendLog>
  void dispatch_show_logs(AppendLog&& append_log) const {
    append_log("screen=logs", "show", "#d8d8ff");
    append_log(
        "dlogs.buffer=" + std::to_string(cuwacunu::piaabo::dlog_buffer_size()) + "/" +
            std::to_string(cuwacunu::piaabo::dlog_buffer_capacity()),
        "show",
        "#d8d8ff");
    append_log("hint=iinuji.logs.clear()", "show", "#d8d8ff");
  }

  template <class PushInfo, class PushErr>
  bool dispatch_data_plot_call(const cuwacunu::camahjucunu::canonical_path_t& path,
                               PushInfo&& push_info,
                               PushErr&& push_err) const {
    screen.data();

    if (path.args.empty()) {
      state.data.plot_view = true;
      push_info("data plotview=on");
      return true;
    }

    bool touched_mode = false;
    bool touched_view = false;
    for (const auto& arg : path.args) {
      if (arg.key == "mode") {
        const auto mode = parse_data_plot_mode_token(arg.value);
        if (!mode.has_value()) {
          push_err("invalid plot mode in iinuji call: " + arg.value);
          return true;
        }
        state.data.plot_mode = *mode;
        clamp_data_plot_mode(state);
        touched_mode = true;
        continue;
      }
      if (arg.key == "view") {
        bool view_value = state.data.plot_view;
        bool toggle = false;
        if (!parse_view_bool(arg.value, &view_value, &toggle)) {
          push_err("invalid plot view value in iinuji call: " + arg.value);
          return true;
        }
        if (toggle) state.data.plot_view = !state.data.plot_view;
        else state.data.plot_view = view_value;
        touched_view = true;
        continue;
      }
      push_err("unsupported plot arg in iinuji call: " + arg.key);
      return true;
    }

    if (touched_mode) push_info("data plot=" + data_plot_mode_token(state.data.plot_mode));
    if (touched_view) push_info(std::string("data plotview=") + (state.data.plot_view ? "on" : "off"));
    if (!touched_mode && !touched_view) push_info("data plot call applied");
    return true;
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch_board_select_index(const cuwacunu::camahjucunu::canonical_path_t& path,
                                   PushInfo&& push_info,
                                   PushWarn&& push_warn,
                                   PushErr&& push_err) const {
    std::size_t idx1 = 0;
    if (!parse_positive_arg_or_tail(path, 4, &idx1)) {
      push_err("usage: iinuji.board.select.index.n1()");
      return true;
    }
    if (!board_has_circuits(state)) {
      push_warn("no circuits");
      return true;
    }
    if (idx1 > state.board.board.circuits.size()) {
      push_err("circuit out of range");
      return true;
    }
    state.board.selected_circuit = idx1 - 1;
    screen.board();
    push_info("selected circuit=" + std::to_string(state.board.selected_circuit + 1));
    return true;
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch_tsi_tab_index(const cuwacunu::camahjucunu::canonical_path_t& path,
                              PushInfo&& push_info,
                              PushWarn&& push_warn,
                              PushErr&& push_err) const {
    const std::size_t n = tsi_tab_count();
    if (n == 0) {
      push_warn("no tsi tabs");
      return true;
    }

    std::size_t idx1 = 0;
    if (!parse_positive_arg_or_tail(path, 4, &idx1)) {
      push_err("usage: iinuji.tsi.tab.index.n1()");
      return true;
    }
    if (idx1 == 0 || idx1 > n) {
      push_err("tsi tab out of range");
      return true;
    }
    state.tsiemene.selected_tab = idx1 - 1;
    screen.tsi();
    push_info("selected tsi tab=" + std::to_string(state.tsiemene.selected_tab + 1));
    return true;
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch_tsi_tab_id(const cuwacunu::camahjucunu::canonical_path_t& path,
                           PushInfo&& push_info,
                           PushWarn&& push_warn,
                           PushErr&& push_err) const {
    const std::size_t n = tsi_tab_count();
    if (n == 0) {
      push_warn("no tsi tabs");
      return true;
    }

    std::string id;
    if (!parse_string_arg_or_tail(path, 4, &id) || id.empty()) {
      push_err("usage: iinuji.tsi.tab.id.<token>()");
      return true;
    }
    if (!select_tsi_tab_by_token(state, id)) {
      push_err("tsi tab not found");
      return true;
    }
    screen.tsi();
    push_info("selected tsi tab=" + std::to_string(state.tsiemene.selected_tab + 1));
    return true;
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch_config_tab_index(const cuwacunu::camahjucunu::canonical_path_t& path,
                                 PushInfo&& push_info,
                                 PushWarn&& push_warn,
                                 PushErr&& push_err) const {
    if (!config_has_tabs(state)) {
      push_warn("no config tabs");
      return true;
    }

    std::size_t idx1 = 0;
    if (!parse_positive_arg_or_tail(path, 4, &idx1)) {
      push_err("usage: iinuji.config.tab.index.n1()");
      return true;
    }
    if (idx1 == 0 || idx1 > state.config.tabs.size()) {
      push_err("config tab out of range");
      return true;
    }
    state.config.selected_tab = idx1 - 1;
    screen.config();
    push_info("selected tab=" + std::to_string(state.config.selected_tab + 1));
    return true;
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch_config_tab_id(const cuwacunu::camahjucunu::canonical_path_t& path,
                              PushInfo&& push_info,
                              PushWarn&& push_warn,
                              PushErr&& push_err) const {
    if (!config_has_tabs(state)) {
      push_warn("no config tabs");
      return true;
    }

    std::string id;
    if (!parse_string_arg_or_tail(path, 4, &id) || id.empty()) {
      push_err("usage: iinuji.config.tab.id.<token>()");
      return true;
    }
    if (!select_tab_by_token(state, id)) {
      push_err("tab not found");
      return true;
    }
    screen.config();
    push_info("selected tab=" + std::to_string(state.config.selected_tab + 1));
    return true;
  }

  template <class PushInfo, class PushErr>
  bool dispatch_data_x(const cuwacunu::camahjucunu::canonical_path_t& path,
                       PushInfo&& push_info,
                       PushErr&& push_err) const {
    std::string axis_raw;
    if (!get_arg_value(path, "axis", &axis_raw) &&
        !get_arg_value(path, "x", &axis_raw) &&
        !get_arg_value(path, "value", &axis_raw)) {
      axis_raw = "toggle";
    }
    const std::string axis = to_lower_copy(axis_raw);
    if (axis.empty() || axis == "toggle") {
      state.data.plot_x_axis = next_data_plot_x_axis(state.data.plot_x_axis);
    } else {
      const auto parsed_axis = parse_data_plot_x_axis_token(axis);
      if (!parsed_axis.has_value()) {
        push_err("usage: iinuji.data.x(axis=idx|key|toggle)");
        return true;
      }
      state.data.plot_x_axis = *parsed_axis;
    }
    clamp_data_plot_x_axis(state);
    screen.data();
    push_info("data x=" + data_plot_x_axis_token(state.data.plot_x_axis));
    return true;
  }

  template <class PushInfo, class PushErr>
  bool dispatch_data_mask(const cuwacunu::camahjucunu::canonical_path_t& path,
                          PushInfo&& push_info,
                          PushErr&& push_err) const {
    std::string view_raw;
    if (!get_arg_value(path, "view", &view_raw) &&
        !get_arg_value(path, "value", &view_raw)) {
      view_raw = "toggle";
    }
    bool view_value = state.data.plot_mask_overlay;
    bool toggle = false;
    if (!parse_view_bool(view_raw, &view_value, &toggle)) {
      push_err("usage: iinuji.data.mask(view=on|off|toggle)");
      return true;
    }
    if (toggle) state.data.plot_mask_overlay = !state.data.plot_mask_overlay;
    else state.data.plot_mask_overlay = view_value;
    screen.data();
    push_info(std::string("data mask=") + (state.data.plot_mask_overlay ? "on" : "off"));
    return true;
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch_data_ch_index(const cuwacunu::camahjucunu::canonical_path_t& path,
                              PushInfo&& push_info,
                              PushWarn&& push_warn,
                              PushErr&& push_err) const {
    if (!data_has_channels(state)) {
      push_warn("no data channels");
      return true;
    }
    std::size_t idx1 = 0;
    if (!parse_positive_arg_or_tail(path, 4, &idx1)) {
      push_err("usage: iinuji.data.ch.index.n1()");
      return true;
    }
    if (idx1 == 0 || idx1 > state.data.channels.size()) {
      push_err("data channel not found");
      return true;
    }
    state.data.selected_channel = idx1 - 1;
    screen.data();
    push_info("selected data channel=" + std::to_string(state.data.selected_channel + 1));
    return true;
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch_data_sample_index(const cuwacunu::camahjucunu::canonical_path_t& path,
                                  PushInfo&& push_info,
                                  PushWarn&& push_warn,
                                  PushErr&& push_err) const {
    if (state.data.plot_sample_count == 0) {
      push_warn("no data samples loaded");
      return true;
    }
    std::size_t idx1 = 0;
    if (!parse_positive_arg_or_tail(path, 4, &idx1)) {
      push_err("usage: iinuji.data.sample.index.n1()");
      return true;
    }
    if (idx1 == 0 || idx1 > state.data.plot_sample_count) {
      push_err("sample out of range");
      return true;
    }
    state.data.plot_sample_index = idx1 - 1;
    screen.data();
    push_info("selected data sample=" + std::to_string(state.data.plot_sample_index + 1));
    return true;
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch_data_dim_index(const cuwacunu::camahjucunu::canonical_path_t& path,
                               PushInfo&& push_info,
                               PushWarn&& push_warn,
                               PushErr&& push_err) const {
    if (state.data.plot_D == 0) {
      push_warn("no tensor dims available");
      return true;
    }
    std::size_t idx1 = 0;
    if (!parse_positive_arg_or_tail(path, 4, &idx1)) {
      push_err("usage: iinuji.data.dim.index.n1()");
      return true;
    }
    if (idx1 == 0 || idx1 > state.data.plot_D) {
      push_err("dim out of range");
      return true;
    }
    state.data.plot_feature_dim = idx1 - 1;
    screen.data();
    push_info("selected data dim=" + std::to_string(state.data.plot_feature_dim + 1));
    return true;
  }

  template <class PushInfo, class PushWarn, class PushErr>
  bool dispatch_data_dim_id(const cuwacunu::camahjucunu::canonical_path_t& path,
                            PushInfo&& push_info,
                            PushWarn&& push_warn,
                            PushErr&& push_err) const {
    if (state.data.plot_D == 0) {
      push_warn("no tensor dims available");
      return true;
    }
    std::string token;
    if (!parse_string_arg_or_tail(path, 4, &token) || token.empty()) {
      push_err("usage: iinuji.data.dim.id.<token>()");
      return true;
    }
    if (!select_data_dim_by_token(state, token)) {
      push_err("dim out of range");
      return true;
    }
    screen.data();
    push_info("selected data dim=" + std::to_string(state.data.plot_feature_dim + 1));
    return true;
  }
};

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
