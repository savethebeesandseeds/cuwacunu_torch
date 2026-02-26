#pragma once

// Included inside struct IinujiPathHandlers.

template <class PushInfo>
bool dispatch_logs_setting_cursor_delta(int delta, PushInfo&& push_info) const {
  if (logs_settings_count() > 0) {
    if (delta < 0) {
      state.logs.selected_setting =
          (state.logs.selected_setting + logs_settings_count() - 1u) % logs_settings_count();
      push_info("logs.settings.cursor=prev");
    } else {
      state.logs.selected_setting =
          (state.logs.selected_setting + 1u) % logs_settings_count();
      push_info("logs.settings.cursor=next");
    }
  } else {
    state.logs.selected_setting = 0;
    push_info(delta < 0 ? "logs.settings.cursor=prev" : "logs.settings.cursor=next");
  }
  screen.logs();
  return true;
}

template <class PushInfo, class PushWarn, class PushErr, class AppendLog>
bool dispatch_logs_call(CallHandlerId call_id,
                        PushInfo&& push_info,
                        PushWarn&& push_warn,
                        PushErr&& push_err,
                        AppendLog&& append_log) const {
  (void)push_warn;
  (void)push_err;
  (void)append_log;
  switch (call_id) {
    case CallHandlerId::LogsClear:
      cuwacunu::piaabo::dlog_clear_buffer();
      screen.logs();
      push_info("logs cleared");
      return true;
    case CallHandlerId::LogsScrollUp:
      screen.logs();
      state.logs.pending_scroll_y = saturating_add_signed(state.logs.pending_scroll_y, -6);
      state.logs.auto_follow = false;
      push_info("logs scroll=up");
      return true;
    case CallHandlerId::LogsScrollDown:
      screen.logs();
      state.logs.pending_scroll_y = saturating_add_signed(state.logs.pending_scroll_y, +6);
      state.logs.auto_follow = false;
      push_info("logs scroll=down");
      return true;
    case CallHandlerId::LogsScrollPageUp:
      screen.logs();
      state.logs.pending_scroll_y = saturating_add_signed(state.logs.pending_scroll_y, -20);
      state.logs.auto_follow = false;
      push_info("logs scroll=page-up");
      return true;
    case CallHandlerId::LogsScrollPageDown:
      screen.logs();
      state.logs.pending_scroll_y = saturating_add_signed(state.logs.pending_scroll_y, +20);
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
      return dispatch_logs_setting_cursor_delta(-1, push_info);
    case CallHandlerId::LogsSettingsSelectNext:
      return dispatch_logs_setting_cursor_delta(+1, push_info);
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
    default:
      return false;
  }
}
