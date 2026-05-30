#pragma once

// Included inside struct IinujiPathHandlers.

template <class PushInfo>
bool dispatch_logs_setting_cursor_delta(int delta, PushInfo&& push_info) const {
  if (logs_settings_count() > 0) {
    if (delta < 0) {
      state.shell_logs.selected_setting =
          (state.shell_logs.selected_setting + logs_settings_count() - 1u) %
          logs_settings_count();
      push_info("logs.settings.cursor=prev");
    } else {
      state.shell_logs.selected_setting =
          (state.shell_logs.selected_setting + 1u) % logs_settings_count();
      push_info("logs.settings.cursor=next");
    }
  } else {
    state.shell_logs.selected_setting = 0;
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
      state.shell_logs.pending_scroll_y =
          saturating_add_signed(state.shell_logs.pending_scroll_y, -6);
      state.shell_logs.auto_follow = false;
      push_info("logs scroll=up");
      return true;
    case CallHandlerId::LogsScrollDown:
      screen.logs();
      state.shell_logs.pending_scroll_y =
          saturating_add_signed(state.shell_logs.pending_scroll_y, +6);
      state.shell_logs.auto_follow = false;
      push_info("logs scroll=down");
      return true;
    case CallHandlerId::LogsScrollPageUp:
      screen.logs();
      state.shell_logs.pending_scroll_y =
          saturating_add_signed(state.shell_logs.pending_scroll_y, -20);
      state.shell_logs.auto_follow = false;
      push_info("logs scroll=page-up");
      return true;
    case CallHandlerId::LogsScrollPageDown:
      screen.logs();
      state.shell_logs.pending_scroll_y =
          saturating_add_signed(state.shell_logs.pending_scroll_y, +20);
      state.shell_logs.auto_follow = false;
      push_info("logs scroll=page-down");
      return true;
    case CallHandlerId::LogsScrollHome:
      screen.logs();
      state.shell_logs.pending_scroll_y = 0;
      state.shell_logs.pending_scroll_x = 0;
      state.shell_logs.pending_jump_end = false;
      state.shell_logs.pending_jump_home = true;
      state.shell_logs.auto_follow = false;
      push_info("logs scroll=home");
      return true;
    case CallHandlerId::LogsScrollEnd:
      screen.logs();
      state.shell_logs.pending_scroll_y = 0;
      state.shell_logs.pending_scroll_x = 0;
      state.shell_logs.pending_jump_home = false;
      state.shell_logs.pending_jump_end = true;
      state.shell_logs.auto_follow = true;
      push_info("logs scroll=end");
      return true;
    case CallHandlerId::LogsSettingsLevelDebug:
      state.shell_logs.level_filter = LogsLevelFilter::DebugOrHigher;
      screen.logs();
      push_info("logs.level=DEBUG+");
      return true;
    case CallHandlerId::LogsSettingsLevelInfo:
      state.shell_logs.level_filter = LogsLevelFilter::InfoOrHigher;
      screen.logs();
      push_info("logs.level=INFO+");
      return true;
    case CallHandlerId::LogsSettingsLevelWarning:
      state.shell_logs.level_filter = LogsLevelFilter::WarningOrHigher;
      screen.logs();
      push_info("logs.level=WARNING+");
      return true;
    case CallHandlerId::LogsSettingsLevelError:
      state.shell_logs.level_filter = LogsLevelFilter::ErrorOrHigher;
      screen.logs();
      push_info("logs.level=ERROR+");
      return true;
    case CallHandlerId::LogsSettingsLevelFatal:
      state.shell_logs.level_filter = LogsLevelFilter::FatalOnly;
      screen.logs();
      push_info("logs.level=FATAL");
      return true;
    case CallHandlerId::LogsSettingsSelectPrev:
      return dispatch_logs_setting_cursor_delta(-1, push_info);
    case CallHandlerId::LogsSettingsSelectNext:
      return dispatch_logs_setting_cursor_delta(+1, push_info);
    case CallHandlerId::LogsSettingsDateToggle:
      state.shell_logs.show_date = !state.shell_logs.show_date;
      screen.logs();
      push_info(std::string("logs.date=") +
                (state.shell_logs.show_date ? "on" : "off"));
      return true;
    case CallHandlerId::LogsSettingsThreadToggle:
      state.shell_logs.show_thread = !state.shell_logs.show_thread;
      screen.logs();
      push_info(std::string("logs.thread=") +
                (state.shell_logs.show_thread ? "on" : "off"));
      return true;
    case CallHandlerId::LogsSettingsMetadataToggle:
      state.shell_logs.show_metadata = !state.shell_logs.show_metadata;
      screen.logs();
      push_info(std::string("logs.metadata=") +
                (state.shell_logs.show_metadata ? "on" : "off"));
      return true;
    case CallHandlerId::LogsSettingsMetadataFilterAny:
      state.shell_logs.metadata_filter = LogsMetadataFilter::Any;
      screen.logs();
      push_info("logs.metadata_filter=ANY");
      return true;
    case CallHandlerId::LogsSettingsMetadataFilterAnyMeta:
      state.shell_logs.metadata_filter = LogsMetadataFilter::WithAnyMetadata;
      screen.logs();
      push_info("logs.metadata_filter=META+");
      return true;
    case CallHandlerId::LogsSettingsMetadataFilterFunction:
      state.shell_logs.metadata_filter = LogsMetadataFilter::WithFunction;
      screen.logs();
      push_info("logs.metadata_filter=FN+");
      return true;
    case CallHandlerId::LogsSettingsMetadataFilterPath:
      state.shell_logs.metadata_filter = LogsMetadataFilter::WithPath;
      screen.logs();
      push_info("logs.metadata_filter=PATH+");
      return true;
    case CallHandlerId::LogsSettingsMetadataFilterCallsite:
      state.shell_logs.metadata_filter = LogsMetadataFilter::WithCallsite;
      screen.logs();
      push_info("logs.metadata_filter=CALLSITE+");
      return true;
    case CallHandlerId::LogsSettingsColorToggle:
      state.shell_logs.show_color = !state.shell_logs.show_color;
      screen.logs();
      push_info(std::string("logs.color=") +
                (state.shell_logs.show_color ? "on" : "off"));
      return true;
    case CallHandlerId::LogsSettingsFollowToggle:
      state.shell_logs.auto_follow = !state.shell_logs.auto_follow;
      screen.logs();
      push_info(std::string("logs.follow=") +
                (state.shell_logs.auto_follow ? "on" : "off"));
      return true;
    case CallHandlerId::LogsSettingsMouseCaptureToggle:
      state.shell_logs.mouse_capture = !state.shell_logs.mouse_capture;
      screen.logs();
      push_info(std::string("logs.mouse_capture=") +
                (state.shell_logs.mouse_capture ? "on" : "off"));
      return true;
    default:
      return false;
  }
}
