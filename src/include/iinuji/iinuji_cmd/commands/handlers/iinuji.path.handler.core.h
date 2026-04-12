#pragma once

// Included inside struct IinujiPathHandlers.

template <class AppendLog>
void dispatch_show_logs(AppendLog &&append_log) const {
  append_log("screen=shell_logs", "show", "#d8d8ff");
  append_log(
      "dlogs.buffer=" + std::to_string(cuwacunu::piaabo::dlog_buffer_size()) +
          "/" + std::to_string(cuwacunu::piaabo::dlog_buffer_capacity()),
      "show", "#d8d8ff");
  append_log("hint=" + std::string(canonical_paths::kLogsClear), "show",
             "#d8d8ff");
}

template <class PushInfo, class PushWarn, class PushErr, class AppendLog>
bool dispatch_core_call(CallHandlerId call_id, PushInfo &&push_info,
                        PushWarn &&push_warn, PushErr &&push_err,
                        AppendLog &&append_log) const {
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
  case CallHandlerId::ScreenHuman:
    screen.human();
    focus_human_menu(state);
    if (state.human.inbox.all_sessions.empty() &&
        !state.human.refresh_pending) {
      set_human_status(state, "Loading Human cockpit...", false);
      (void)queue_human_refresh(state);
    }
    push_info("screen=human");
    return true;
  case CallHandlerId::ScreenRuntime:
    screen.runtime();
    if (state.runtime.sessions.empty() && state.runtime.campaigns.empty() &&
        state.runtime.jobs.empty() && !state.runtime.refresh_pending) {
      set_runtime_status(state, "Loading runtime inventory...", false);
      (void)queue_runtime_refresh(state);
    }
    push_info("screen=runtime");
    return true;
  case CallHandlerId::ScreenLattice:
    screen.lattice();
    if (state.lattice.rows.empty() && !lattice_is_refresh_pending(state)) {
      queue_lattice_refresh(
          state, LatticeRefreshMode::Snapshot,
          "Loading lattice snapshot... press r for a full sync.");
    } else if (!lattice_is_refresh_pending(state)) {
      set_lattice_status(
          state, "Showing cached lattice snapshot. Press r to sync store.",
          false);
    }
    push_info("screen=lattice");
    return true;
  case CallHandlerId::ScreenLogs:
    screen.logs();
    push_info("screen=shell_logs");
    return true;
  case CallHandlerId::ScreenConfig:
    screen.config();
    if (state.config.files.empty() &&
        !config_is_refresh_pending(state.config) &&
        !state.config.editor_focus) {
      config_set_status(state.config, "Loading config view...", false);
      (void)queue_config_refresh(state);
    }
    push_info("screen=config");
    return true;
  case CallHandlerId::RefreshAll:
    state_flow.reload_shell();
    push_info("hero shell refresh queued");
    return true;
  case CallHandlerId::HumanRefresh:
    set_human_status(state, "Loading Human cockpit...", false);
    state_flow.reload_human();
    push_info("human cockpit refresh queued");
    return true;
  case CallHandlerId::RuntimeRefresh:
    set_runtime_status(state, "Loading runtime inventory...", false);
    state_flow.reload_runtime();
    push_info("runtime screen refresh queued");
    return true;
  case CallHandlerId::LatticeRefresh:
    queue_lattice_refresh(state, LatticeRefreshMode::SyncStore,
                          "Syncing lattice store...");
    push_info("lattice refresh queued");
    return true;
  case CallHandlerId::ConfigReload:
    config_set_status(state.config, "Loading config view...", false);
    state_flow.reload_config();
    push_info("config reload queued");
    return true;
  case CallHandlerId::HumanViewInbox:
    set_human_view(state, kHumanInboxView);
    focus_human_menu(state);
    screen.human();
    push_info("human view=inbox");
    return true;
  case CallHandlerId::HumanViewLive:
    set_human_view(state, kHumanLiveView);
    focus_human_menu(state);
    screen.human();
    push_info("human view=live redirected to inbox");
    return true;
  case CallHandlerId::HumanViewHistory:
    set_human_view(state, kHumanHistoryView);
    focus_human_menu(state);
    screen.human();
    push_info("human view=history redirected to inbox");
    return true;
  case CallHandlerId::HumanViewOverview:
    set_human_view(state, kHumanOverviewView);
    focus_human_menu(state);
    screen.human();
    push_info("human view=inbox (overview compatibility)");
    return true;
  case CallHandlerId::HumanViewRequests:
    set_human_view(state, kHumanRequestsView);
    focus_human_menu(state);
    screen.human();
    push_info("human view=inbox (requests compatibility)");
    return true;
  case CallHandlerId::HumanViewReviews:
    set_human_view(state, kHumanReviewsView);
    focus_human_menu(state);
    screen.human();
    push_info("human view=inbox (reviews compatibility)");
    return true;
  case CallHandlerId::HumanFilterNext:
    cycle_human_phase_filter(state);
    screen.human();
    push_warn("human phase filter is deprecated; Marshal is inbox-only and "
              "Runtime owns session inventory");
    return true;
  case CallHandlerId::RuntimeDetailNext:
    cycle_runtime_detail_mode(state);
    screen.runtime();
    push_info("runtime detail=" +
              runtime_detail_mode_label(state.runtime.detail_mode));
    return true;
  case CallHandlerId::RuntimeRowPrev:
    if (!select_prev_runtime_lane(state)) {
      push_warn("runtime lane is already at the first lane");
    }
    screen.runtime();
    push_info("runtime lane=" + runtime_lane_label(state.runtime.lane));
    return true;
  case CallHandlerId::RuntimeRowNext:
    if (!select_next_runtime_lane(state)) {
      push_warn("runtime lane is already at the last lane");
    }
    screen.runtime();
    push_info("runtime lane=" + runtime_lane_label(state.runtime.lane));
    return true;
  case CallHandlerId::RuntimeItemPrev:
    if (!select_prev_runtime_item(state)) {
      push_warn("runtime item is already at the oldest visible entry");
    } else {
      push_info("runtime " + runtime_row_kind_label(state.runtime.lane) +
                "=prev");
    }
    screen.runtime();
    return true;
  case CallHandlerId::RuntimeItemNext:
    if (!select_next_runtime_item(state)) {
      push_warn("runtime item is already at the newest visible entry");
    } else {
      push_info("runtime " + runtime_row_kind_label(state.runtime.lane) +
                "=next");
    }
    screen.runtime();
    return true;
  case CallHandlerId::ShowCurrent:
    if (state.screen == ScreenMode::Home) {
      append_log("screen=home", "show", "#d8ffd8");
      append_log("hint=F2 Marshal | F3 Runtime | F4 Lattice | F8 Shell Logs "
                 "| F9 Config",
                 "show", "#d8ffd8");
      append_log("site=waajacu.com", "show", "#d8ffd8");
      return true;
    }
    if (state.screen == ScreenMode::Human) {
      append_human_show_lines(state, append_log);
      return true;
    }
    if (state.screen == ScreenMode::Runtime) {
      append_runtime_show_lines(state, append_log);
      return true;
    }
    if (state.screen == ScreenMode::Lattice) {
      append_lattice_show_lines(state, append_log);
      return true;
    }
    if (state.screen == ScreenMode::ShellLogs) {
      dispatch_show_logs(append_log);
      return true;
    }
    handle_config_show(state, push_warn, append_log);
    return true;
  case CallHandlerId::ShowHuman:
    append_human_show_lines(state, append_log);
    return true;
  case CallHandlerId::ShowRuntime:
    append_runtime_show_lines(state, append_log);
    return true;
  case CallHandlerId::ShowLattice:
    append_lattice_show_lines(state, append_log);
    return true;
  case CallHandlerId::ShowLogs:
    dispatch_show_logs(append_log);
    return true;
  case CallHandlerId::ShowConfig:
  case CallHandlerId::ConfigShow:
  case CallHandlerId::ConfigFileShow:
    handle_config_show(state, push_warn, append_log);
    return true;
  default:
    return false;
  }
}
