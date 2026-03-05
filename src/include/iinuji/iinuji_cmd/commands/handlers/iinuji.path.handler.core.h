#pragma once

// Included inside struct IinujiPathHandlers.

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
  append_log("hint=" + std::string(canonical_paths::kLogsClear), "show", "#d8d8ff");
}

template <class PushInfo, class PushWarn, class PushErr, class AppendLog>
bool dispatch_core_call(CallHandlerId call_id,
                        PushInfo&& push_info,
                        PushWarn&& push_warn,
                        PushErr&& push_err,
                        AppendLog&& append_log) const {
  switch (call_id) {
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
      state.help_scroll_y = saturating_add_non_negative(state.help_scroll_y, -3);
      push_info("help scroll=up");
      return true;
    case CallHandlerId::HelpScrollDown:
      state.help_view = true;
      state.help_scroll_y = saturating_add_non_negative(state.help_scroll_y, +3);
      push_info("help scroll=down");
      return true;
    case CallHandlerId::HelpScrollLeft:
      state.help_view = true;
      state.help_scroll_x = saturating_add_non_negative(state.help_scroll_x, -16);
      push_info("help scroll=left");
      return true;
    case CallHandlerId::HelpScrollRight:
      state.help_view = true;
      state.help_scroll_x = saturating_add_non_negative(state.help_scroll_x, +16);
      push_info("help scroll=right");
      return true;
    case CallHandlerId::HelpScrollPageUp:
      state.help_view = true;
      state.help_scroll_y = saturating_add_non_negative(state.help_scroll_y, -20);
      push_info("help scroll=page-up");
      return true;
    case CallHandlerId::HelpScrollPageDown:
      state.help_view = true;
      state.help_scroll_y = saturating_add_non_negative(state.help_scroll_y, +20);
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
    case CallHandlerId::ScreenTraining:
    case CallHandlerId::ViewTraining:
      screen.training();
      push_info("screen=training");
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
      if (state.screen == ScreenMode::Training) {
        handle_training_show(state, push_warn, append_log);
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
    case CallHandlerId::ShowTraining:
      handle_training_show(state, push_warn, append_log);
      return true;
    case CallHandlerId::ShowTsi:
      handle_tsi_show(state, push_warn, append_log);
      return true;
    case CallHandlerId::ShowConfig:
    case CallHandlerId::ConfigShow:
    case CallHandlerId::ConfigTabShow:
      handle_config_show(state, push_warn, append_log);
      return true;
    default:
      return false;
  }
}
