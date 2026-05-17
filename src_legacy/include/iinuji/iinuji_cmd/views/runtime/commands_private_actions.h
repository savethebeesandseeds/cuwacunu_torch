inline RuntimeExcerptSnapshot runtime_collect_excerpt_snapshot(
    cuwacunu::hero::runtime_mcp::app_context_t runtime_app,
    const cuwacunu::hero::runtime::runtime_job_record_t &job,
    RuntimeDetailMode detail_mode) {
  RuntimeExcerptSnapshot snapshot{};
  snapshot.job_cursor = job.job_cursor;
  snapshot.detail_mode = detail_mode;
  switch (detail_mode) {
  case RuntimeDetailMode::Manifest:
    return snapshot;
  case RuntimeDetailMode::Log:
    if (!job.stdout_path.empty()) {
      runtime_assign_excerpt_from_tool(
          &runtime_app, "hero.runtime.tail_log",
          std::string("{\"job_cursor\":") + config_json_quote(job.job_cursor) +
              ",\"stream\":\"stdout\",\"lines\":10,\"include_text\":true,"
              "\"inline_line_limit\":10,\"max_bytes\":4096}",
          &snapshot.stdout_excerpt);
    }
    if (!job.stderr_path.empty()) {
      runtime_assign_excerpt_from_tool(
          &runtime_app, "hero.runtime.tail_log",
          std::string("{\"job_cursor\":") + config_json_quote(job.job_cursor) +
              ",\"stream\":\"stderr\",\"lines\":10,\"include_text\":true,"
              "\"inline_line_limit\":10,\"max_bytes\":4096}",
          &snapshot.stderr_excerpt);
    }
    return snapshot;
  case RuntimeDetailMode::Trace:
    if (!job.trace_path.empty()) {
      runtime_assign_excerpt_from_tool(
          &runtime_app, "hero.runtime.tail_trace",
          std::string("{\"job_cursor\":") + config_json_quote(job.job_cursor) +
              ",\"lines\":14,\"include_text\":true,"
              "\"inline_line_limit\":14,"
              "\"max_bytes\":4096}",
          &snapshot.trace_excerpt);
    }
    return snapshot;
  }
  return snapshot;
}

inline void refresh_runtime_job_excerpts(CmdState &st) {
  clear_runtime_job_excerpts(st);
  if (!st.runtime.job.has_value())
    return;
  const auto &job = *st.runtime.job;
  switch (st.runtime.detail_mode) {
  case RuntimeDetailMode::Manifest:
    return;
  case RuntimeDetailMode::Log:
    if (!job.stdout_path.empty()) {
      runtime_assign_excerpt_from_tool(
          st, "hero.runtime.tail_log",
          std::string("{\"job_cursor\":") + config_json_quote(job.job_cursor) +
              ",\"stream\":\"stdout\",\"lines\":10,\"include_text\":true,"
              "\"inline_line_limit\":10,\"max_bytes\":4096}",
          &st.runtime.job_stdout_excerpt);
    }
    if (!job.stderr_path.empty()) {
      runtime_assign_excerpt_from_tool(
          st, "hero.runtime.tail_log",
          std::string("{\"job_cursor\":") + config_json_quote(job.job_cursor) +
              ",\"stream\":\"stderr\",\"lines\":10,\"include_text\":true,"
              "\"inline_line_limit\":10,\"max_bytes\":4096}",
          &st.runtime.job_stderr_excerpt);
    }
    return;
  case RuntimeDetailMode::Trace:
    if (!job.trace_path.empty()) {
      runtime_assign_excerpt_from_tool(
          st, "hero.runtime.tail_trace",
          std::string("{\"job_cursor\":") + config_json_quote(job.job_cursor) +
              ",\"lines\":14,\"include_text\":true,"
              "\"inline_line_limit\":14,"
              "\"max_bytes\":4096}",
          &st.runtime.job_trace_excerpt);
    }
    return;
  }
}

inline void focus_runtime_menu(CmdState &st) {
  st.runtime.focus = kRuntimeMenuFocus;
}

inline void focus_runtime_worklist(CmdState &st) {
  st.runtime.focus = kRuntimeWorklistFocus;
}

inline void clear_runtime_drill_filters(CmdState &st) {
  st.runtime.campaign_filter_active = false;
  st.runtime.job_filter_active = false;
}

inline bool queue_runtime_excerpt_refresh(CmdState &st);
inline bool poll_runtime_async_updates(CmdState &st);

inline std::string
runtime_session_id_for_campaign(const RuntimeState &runtime,
                                std::string_view campaign_cursor);

inline std::string runtime_campaign_cursor_for_job(const RuntimeState &runtime,
                                                   std::string_view job_cursor);

inline std::vector<std::size_t>
runtime_child_campaign_indices(const CmdState &st);

inline std::vector<std::size_t>
runtime_orphan_campaign_indices(const CmdState &st) {
  std::vector<std::size_t> indices{};
  indices.reserve(st.runtime.campaigns.size());
  for (std::size_t i = 0; i < st.runtime.campaigns.size(); ++i) {
    if (runtime_session_id_for_campaign(st.runtime,
                                        st.runtime.campaigns[i].campaign_cursor)
            .empty()) {
      indices.push_back(i);
    }
  }
  return indices;
}

inline std::vector<std::size_t> runtime_orphan_job_indices(const CmdState &st) {
  std::vector<std::size_t> indices{};
  indices.reserve(st.runtime.jobs.size());
  for (std::size_t i = 0; i < st.runtime.jobs.size(); ++i) {
    if (runtime_campaign_cursor_for_job(st.runtime,
                                        st.runtime.jobs[i].job_cursor)
            .empty()) {
      indices.push_back(i);
    }
  }
  return indices;
}

inline bool runtime_has_none_session_row(const CmdState &st) {
  return !runtime_orphan_campaign_indices(st).empty() ||
         !runtime_orphan_job_indices(st).empty();
}

inline bool runtime_has_none_campaign_row(const CmdState &st) {
  return !runtime_orphan_job_indices(st).empty();
}

inline bool runtime_child_has_none_campaign_row(const CmdState &st) {
  return runtime_is_none_session_selection(st.runtime.selected_session_id) &&
         runtime_has_none_campaign_row(st);
}

inline std::size_t runtime_session_row_count(const CmdState &st) {
  return st.runtime.sessions.size() +
         (runtime_has_none_session_row(st) ? 1u : 0u);
}

inline std::size_t runtime_campaign_row_count(const CmdState &st) {
  return st.runtime.campaigns.size() +
         (runtime_has_none_campaign_row(st) ? 1u : 0u);
}

inline std::size_t runtime_child_campaign_row_count(const CmdState &st) {
  return runtime_child_campaign_indices(st).size() +
         (runtime_child_has_none_campaign_row(st) ? 1u : 0u);
}

inline std::vector<std::size_t>
runtime_child_campaign_indices(const CmdState &st) {
  if (st.runtime.selected_session_id.empty())
    return {};
  if (runtime_is_none_session_selection(st.runtime.selected_session_id)) {
    return runtime_orphan_campaign_indices(st);
  }
  const auto it = st.runtime.campaign_indices_by_session.find(
      st.runtime.selected_session_id);
  if (it == st.runtime.campaign_indices_by_session.end())
    return {};
  return it->second;
}

inline std::vector<std::size_t> runtime_child_job_indices(const CmdState &st) {
  if (st.runtime.selected_campaign_cursor.empty())
    return {};
  if (runtime_is_none_campaign_selection(st.runtime.selected_campaign_cursor)) {
    return runtime_orphan_job_indices(st);
  }
  const auto it = st.runtime.job_indices_by_campaign.find(
      st.runtime.selected_campaign_cursor);
  if (it == st.runtime.job_indices_by_campaign.end())
    return {};
  return it->second;
}

inline std::vector<std::size_t>
runtime_visible_campaign_indices(const CmdState &st) {
  std::vector<std::size_t> indices{};
  indices.reserve(st.runtime.campaigns.size());
  for (std::size_t i = 0; i < st.runtime.campaigns.size(); ++i) {
    indices.push_back(i);
  }
  return indices;
}

inline std::vector<std::size_t>
runtime_visible_job_indices(const CmdState &st) {
  std::vector<std::size_t> indices{};
  indices.reserve(st.runtime.jobs.size());
  for (std::size_t i = 0; i < st.runtime.jobs.size(); ++i) {
    indices.push_back(i);
  }
  return indices;
}

inline std::size_t runtime_lane_count(const CmdState &st, RuntimeLane lane) {
  if (runtime_is_device_lane(lane))
    return runtime_device_item_count(st);
  if (runtime_is_session_lane(lane))
    return runtime_session_row_count(st);
  if (runtime_is_campaign_lane(lane))
    return runtime_campaign_row_count(st);
  return st.runtime.jobs.size();
}

inline std::size_t current_runtime_lane_count(const CmdState &st) {
  return runtime_lane_count(st, st.runtime.lane);
}

inline bool runtime_has_worklist_items(const CmdState &st) {
  if (runtime_is_device_lane(st.runtime.lane)) {
    return runtime_device_item_count(st) != 0;
  }
  if (runtime_is_session_lane(st.runtime.lane) &&
      st.runtime.job_filter_active) {
    return !runtime_child_job_indices(st).empty();
  }
  if (runtime_is_session_lane(st.runtime.lane) &&
      st.runtime.campaign_filter_active) {
    return !runtime_child_campaign_indices(st).empty() ||
           runtime_child_has_none_campaign_row(st);
  }
  if (runtime_is_campaign_lane(st.runtime.lane) &&
      st.runtime.job_filter_active) {
    return !runtime_child_job_indices(st).empty();
  }
  return current_runtime_lane_count(st) != 0;
}

inline std::optional<std::size_t>
runtime_selected_session_index(const CmdState &st) {
  const bool has_none = runtime_has_none_session_row(st);
  if (st.runtime.sessions.empty() && !has_none)
    return std::nullopt;
  if (runtime_is_none_session_selection(st.runtime.selected_session_id)) {
    return has_none ? std::optional<std::size_t>{st.runtime.sessions.size()}
                    : std::nullopt;
  }
  if (st.runtime.selected_session_id.empty()) {
    if (!st.runtime.sessions.empty())
      return std::size_t{0};
    if (has_none)
      return std::optional<std::size_t>{0};
    return std::nullopt;
  }
  if (!st.runtime.sessions.empty()) {
    return runtime_find_session_index(st.runtime.sessions,
                                      st.runtime.selected_session_id)
        .value_or(std::size_t{0});
  }
  if (has_none)
    return std::size_t{0};
  return std::nullopt;
}

inline std::optional<std::size_t>
runtime_selected_campaign_index(const CmdState &st) {
  const auto visible = runtime_visible_campaign_indices(st);
  const bool has_none = runtime_has_none_campaign_row(st);
  if (visible.empty() && !has_none)
    return std::nullopt;
  if (runtime_is_none_campaign_selection(st.runtime.selected_campaign_cursor)) {
    return has_none ? std::optional<std::size_t>{visible.size()} : std::nullopt;
  }
  if (st.runtime.selected_campaign_cursor.empty()) {
    if (!visible.empty())
      return std::size_t{0};
    if (has_none)
      return std::optional<std::size_t>{0};
    return std::nullopt;
  }
  for (std::size_t i = 0; i < visible.size(); ++i) {
    if (st.runtime.campaigns[visible[i]].campaign_cursor ==
        st.runtime.selected_campaign_cursor) {
      return i;
    }
  }
  if (!visible.empty())
    return std::size_t{0};
  if (has_none)
    return std::optional<std::size_t>{0};
  return std::nullopt;
}

inline std::optional<std::size_t>
runtime_selected_child_campaign_index(const CmdState &st) {
  const auto visible = runtime_child_campaign_indices(st);
  const bool has_none = runtime_child_has_none_campaign_row(st);
  if (visible.empty() && !has_none)
    return std::nullopt;
  if (runtime_is_none_campaign_selection(st.runtime.selected_campaign_cursor)) {
    return has_none ? std::optional<std::size_t>{visible.size()} : std::nullopt;
  }
  if (st.runtime.selected_campaign_cursor.empty()) {
    if (!visible.empty())
      return std::size_t{0};
    if (has_none)
      return std::optional<std::size_t>{0};
    return std::nullopt;
  }
  for (std::size_t i = 0; i < visible.size(); ++i) {
    if (st.runtime.campaigns[visible[i]].campaign_cursor ==
        st.runtime.selected_campaign_cursor) {
      return i;
    }
  }
  if (!visible.empty())
    return std::size_t{0};
  if (has_none)
    return std::optional<std::size_t>{0};
  return std::nullopt;
}

inline std::optional<std::size_t>
runtime_selected_job_index(const CmdState &st) {
  const auto visible = runtime_visible_job_indices(st);
  if (visible.empty())
    return std::nullopt;
  if (st.runtime.selected_job_cursor.empty())
    return std::size_t{0};
  for (std::size_t i = 0; i < visible.size(); ++i) {
    if (st.runtime.jobs[visible[i]].job_cursor ==
        st.runtime.selected_job_cursor) {
      return i;
    }
  }
  return std::size_t{0};
}

inline std::optional<std::size_t>
runtime_selected_child_job_index(const CmdState &st) {
  const auto visible = runtime_child_job_indices(st);
  if (visible.empty())
    return std::nullopt;
  if (st.runtime.selected_job_cursor.empty())
    return std::size_t{0};
  for (std::size_t i = 0; i < visible.size(); ++i) {
    if (st.runtime.jobs[visible[i]].job_cursor ==
        st.runtime.selected_job_cursor) {
      return i;
    }
  }
  return std::size_t{0};
}

inline std::optional<std::size_t>
runtime_current_lane_selected_index(const CmdState &st) {
  if (runtime_is_device_lane(st.runtime.lane)) {
    return runtime_selected_device_index(st);
  }
  if (runtime_is_session_lane(st.runtime.lane)) {
    if (st.runtime.job_filter_active) {
      return runtime_selected_child_job_index(st);
    }
    if (st.runtime.campaign_filter_active) {
      return runtime_selected_child_campaign_index(st);
    }
    return runtime_selected_session_index(st);
  }
  if (runtime_is_campaign_lane(st.runtime.lane)) {
    if (st.runtime.job_filter_active) {
      return runtime_selected_child_job_index(st);
    }
    return runtime_selected_campaign_index(st);
  }
  return runtime_selected_job_index(st);
}

inline std::string
runtime_parent_campaign_cursor_from_job_cursor(std::string_view job_cursor) {
  const std::string trimmed = cuwacunu::hero::runtime::trim_ascii(job_cursor);
  const std::size_t marker =
      trimmed.find(cuwacunu::hero::runtime::kRuntimeJobCursorChildMarker);
  if (marker == std::string::npos || marker == 0)
    return {};
  return trimmed.substr(0, marker);
}

inline bool runtime_session_links_campaign(
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::string_view campaign_cursor) {
  if (campaign_cursor.empty())
    return false;
  if (session.campaign_cursor == campaign_cursor)
    return true;
  return std::find(
             session.campaign_cursors.begin(), session.campaign_cursors.end(),
             std::string(campaign_cursor)) != session.campaign_cursors.end();
}

inline std::string
runtime_session_id_for_campaign(const RuntimeState &runtime,
                                std::string_view campaign_cursor) {
  if (campaign_cursor.empty())
    return {};
  if (const auto it =
          runtime.campaign_session_by_cursor.find(std::string(campaign_cursor));
      it != runtime.campaign_session_by_cursor.end()) {
    return it->second;
  }
  const auto campaign =
      find_runtime_campaign_by_cursor(runtime.campaigns, campaign_cursor);
  if (campaign.has_value() && !campaign->marshal_session_id.empty()) {
    return campaign->marshal_session_id;
  }
  for (const auto &session : runtime.sessions) {
    if (runtime_session_links_campaign(session, campaign_cursor)) {
      return session.marshal_session_id;
    }
  }
  return {};
}

inline std::string
runtime_campaign_cursor_for_job(const RuntimeState &runtime,
                                std::string_view job_cursor) {
  if (const auto it =
          runtime.job_campaign_by_cursor.find(std::string(job_cursor));
      it != runtime.job_campaign_by_cursor.end()) {
    return it->second;
  }
  std::string campaign_cursor =
      runtime_parent_campaign_cursor_from_job_cursor(job_cursor);
  if (!campaign_cursor.empty())
    return campaign_cursor;
  for (const auto &campaign : runtime.campaigns) {
    if (std::find(campaign.job_cursors.begin(), campaign.job_cursors.end(),
                  std::string(job_cursor)) != campaign.job_cursors.end()) {
      return campaign.campaign_cursor;
    }
  }
  return {};
}

inline std::string runtime_session_id_for_job(const RuntimeState &runtime,
                                              std::string_view job_cursor) {
  if (const auto it =
          runtime.job_session_by_cursor.find(std::string(job_cursor));
      it != runtime.job_session_by_cursor.end()) {
    return it->second;
  }
  return runtime_session_id_for_campaign(
      runtime, runtime_campaign_cursor_for_job(runtime, job_cursor));
}

inline void rebuild_runtime_lineage_cache(CmdState &st) {
  st.runtime.campaign_session_by_cursor.clear();
  st.runtime.job_campaign_by_cursor.clear();
  st.runtime.job_session_by_cursor.clear();
  st.runtime.campaign_indices_by_session.clear();
  st.runtime.job_indices_by_campaign.clear();
  st.runtime.job_indices_by_session.clear();
  st.runtime.runner_job_index_by_pid.clear();
  st.runtime.target_job_index_by_pid.clear();

  for (std::size_t i = 0; i < st.runtime.campaigns.size(); ++i) {
    const auto &campaign = st.runtime.campaigns[i];
    std::string marshal_session_id = campaign.marshal_session_id;
    if (marshal_session_id.empty()) {
      for (const auto &session : st.runtime.sessions) {
        if (runtime_session_links_campaign(session, campaign.campaign_cursor)) {
          marshal_session_id = session.marshal_session_id;
          break;
        }
      }
    }
    if (!marshal_session_id.empty()) {
      st.runtime.campaign_session_by_cursor.emplace(campaign.campaign_cursor,
                                                    marshal_session_id);
      st.runtime.campaign_indices_by_session[marshal_session_id].push_back(i);
    }
  }

  for (std::size_t i = 0; i < st.runtime.jobs.size(); ++i) {
    const auto &job = st.runtime.jobs[i];
    if (job.runner_pid.has_value()) {
      st.runtime.runner_job_index_by_pid.emplace(*job.runner_pid, i);
    }
    if (job.target_pid.has_value()) {
      st.runtime.target_job_index_by_pid.emplace(*job.target_pid, i);
    }
    std::string campaign_cursor =
        runtime_parent_campaign_cursor_from_job_cursor(job.job_cursor);
    if (campaign_cursor.empty()) {
      for (const auto &campaign : st.runtime.campaigns) {
        if (std::find(campaign.job_cursors.begin(), campaign.job_cursors.end(),
                      job.job_cursor) != campaign.job_cursors.end()) {
          campaign_cursor = campaign.campaign_cursor;
          break;
        }
      }
    }
    if (campaign_cursor.empty()) {
      continue;
    }

    st.runtime.job_campaign_by_cursor.emplace(job.job_cursor, campaign_cursor);
    st.runtime.job_indices_by_campaign[campaign_cursor].push_back(i);

    const auto session_it =
        st.runtime.campaign_session_by_cursor.find(campaign_cursor);
    if (session_it != st.runtime.campaign_session_by_cursor.end() &&
        !session_it->second.empty()) {
      st.runtime.job_session_by_cursor.emplace(job.job_cursor,
                                               session_it->second);
      st.runtime.job_indices_by_session[session_it->second].push_back(i);
    }
  }
}

inline std::string latest_runtime_campaign_cursor_for_session(
    const RuntimeState &runtime, std::string_view marshal_session_id) {
  const auto session =
      find_runtime_session_by_id(runtime.sessions, marshal_session_id);
  if (session.has_value()) {
    if (!session->campaign_cursor.empty() &&
        runtime_find_campaign_index(runtime.campaigns, session->campaign_cursor)
            .has_value()) {
      return session->campaign_cursor;
    }
    for (auto it = session->campaign_cursors.rbegin();
         it != session->campaign_cursors.rend(); ++it) {
      if (runtime_find_campaign_index(runtime.campaigns, *it).has_value()) {
        return *it;
      }
    }
  }
  for (const auto &campaign : runtime.campaigns) {
    if (runtime_session_id_for_campaign(runtime, campaign.campaign_cursor) ==
        marshal_session_id) {
      return campaign.campaign_cursor;
    }
  }
  return {};
}

inline std::string latest_runtime_job_cursor_for_campaign(
    const RuntimeState &runtime,
    const cuwacunu::hero::runtime::runtime_campaign_record_t &campaign) {
  for (auto it = campaign.job_cursors.rbegin();
       it != campaign.job_cursors.rend(); ++it) {
    if (runtime_find_job_index(runtime.jobs, *it).has_value())
      return *it;
  }
  for (const auto &job : runtime.jobs) {
    if (runtime_parent_campaign_cursor_from_job_cursor(job.job_cursor) ==
        campaign.campaign_cursor) {
      return job.job_cursor;
    }
  }
  return {};
}

inline std::string
latest_runtime_job_cursor_for_campaign(const RuntimeState &runtime,
                                       std::string_view campaign_cursor) {
  const auto campaign =
      find_runtime_campaign_by_cursor(runtime.campaigns, campaign_cursor);
  if (!campaign.has_value())
    return {};
  return latest_runtime_job_cursor_for_campaign(runtime, *campaign);
}

inline std::string latest_runtime_orphan_campaign_cursor(const CmdState &st) {
  const auto orphan_campaigns = runtime_orphan_campaign_indices(st);
  if (orphan_campaigns.empty())
    return {};
  return st.runtime.campaigns[orphan_campaigns.front()].campaign_cursor;
}

inline std::string latest_runtime_orphan_job_cursor(const CmdState &st) {
  const auto orphan_jobs = runtime_orphan_job_indices(st);
  if (orphan_jobs.empty())
    return {};
  return st.runtime.jobs[orphan_jobs.front()].job_cursor;
}

inline void clear_runtime_inventory_selection(CmdState &st) {
  st.runtime.anchor_session_id.clear();
  st.runtime.family_anchor_lane = st.runtime.lane;
  st.runtime.selected_session_id.clear();
  st.runtime.selected_campaign_cursor.clear();
  st.runtime.selected_job_cursor.clear();
  st.runtime.session.reset();
  st.runtime.campaign.reset();
  st.runtime.job.reset();
  clear_runtime_job_excerpts(st);
}

inline void update_runtime_selection_records(CmdState &st) {
  st.runtime.session =
      st.runtime.selected_session_id.empty()
          ? std::nullopt
          : find_runtime_session_by_id(st.runtime.sessions,
                                       st.runtime.selected_session_id);
  st.runtime.campaign =
      st.runtime.selected_campaign_cursor.empty()
          ? std::nullopt
          : find_runtime_campaign_by_cursor(
                st.runtime.campaigns, st.runtime.selected_campaign_cursor);
  st.runtime.job = st.runtime.selected_job_cursor.empty()
                       ? std::nullopt
                       : find_runtime_job_by_cursor(
                             st.runtime.jobs, st.runtime.selected_job_cursor);

  if (st.runtime.session.has_value()) {
    st.runtime.anchor_session_id = st.runtime.session->marshal_session_id;
    return;
  }
  if (st.runtime.campaign.has_value()) {
    st.runtime.anchor_session_id = st.runtime.campaign->marshal_session_id;
    return;
  }
  st.runtime.anchor_session_id.clear();
}

inline bool select_prev_runtime_lane(CmdState &st) {
  const std::size_t idx = runtime_lane_index(st.runtime.lane);
  if (idx == 0)
    return false;
  st.runtime.lane = runtime_lane_from_index(idx - 1);
  clear_runtime_drill_filters(st);
  return true;
}

inline bool select_next_runtime_lane(CmdState &st) {
  const std::size_t idx = runtime_lane_index(st.runtime.lane);
  if (idx >= 3)
    return false;
  st.runtime.lane = runtime_lane_from_index(idx + 1);
  clear_runtime_drill_filters(st);
  return true;
}

inline void
select_runtime_session_and_related(CmdState &st,
                                   std::string_view marshal_session_id) {
  if (runtime_is_none_session_selection(marshal_session_id)) {
    st.runtime.family_anchor_lane = kRuntimeSessionLane;
    st.runtime.selected_session_id = std::string(kRuntimeNoneSessionSelection);
    st.runtime.anchor_session_id.clear();
    st.runtime.selected_campaign_cursor =
        latest_runtime_orphan_campaign_cursor(st);
    if (st.runtime.selected_campaign_cursor.empty() &&
        runtime_has_none_campaign_row(st)) {
      st.runtime.selected_campaign_cursor =
          std::string(kRuntimeNoneCampaignSelection);
    }
    if (runtime_is_none_campaign_selection(
            st.runtime.selected_campaign_cursor)) {
      st.runtime.selected_job_cursor = latest_runtime_orphan_job_cursor(st);
    } else {
      st.runtime.selected_job_cursor = latest_runtime_job_cursor_for_campaign(
          st.runtime, st.runtime.selected_campaign_cursor);
    }
    update_runtime_selection_records(st);
    (void)queue_runtime_excerpt_refresh(st);
    return;
  }
  const auto session_index =
      runtime_find_session_index(st.runtime.sessions, marshal_session_id);
  if (!session_index.has_value()) {
    clear_runtime_inventory_selection(st);
    update_runtime_selection_records(st);
    return;
  }
  st.runtime.family_anchor_lane = kRuntimeSessionLane;
  st.runtime.selected_session_id =
      st.runtime.sessions[*session_index].marshal_session_id;
  st.runtime.anchor_session_id = st.runtime.selected_session_id;
  st.runtime.selected_campaign_cursor =
      latest_runtime_campaign_cursor_for_session(
          st.runtime, st.runtime.selected_session_id);
  st.runtime.selected_job_cursor = latest_runtime_job_cursor_for_campaign(
      st.runtime, st.runtime.selected_campaign_cursor);
  update_runtime_selection_records(st);
  (void)queue_runtime_excerpt_refresh(st);
}

inline void
select_runtime_campaign_and_related(CmdState &st,
                                    std::string_view campaign_cursor) {
  const bool preserve_none_session_context =
      runtime_is_session_lane(st.runtime.lane) &&
      (st.runtime.campaign_filter_active || st.runtime.job_filter_active) &&
      runtime_is_none_session_selection(st.runtime.selected_session_id);
  const auto campaign_index =
      runtime_find_campaign_index(st.runtime.campaigns, campaign_cursor);
  if (!campaign_index.has_value()) {
    clear_runtime_inventory_selection(st);
    update_runtime_selection_records(st);
    return;
  }
  const auto &campaign = st.runtime.campaigns[*campaign_index];
  st.runtime.family_anchor_lane = kRuntimeCampaignLane;
  st.runtime.selected_campaign_cursor = campaign.campaign_cursor;
  st.runtime.selected_session_id =
      runtime_session_id_for_campaign(st.runtime, campaign.campaign_cursor);
  if (st.runtime.selected_session_id.empty() && preserve_none_session_context) {
    st.runtime.selected_session_id = std::string(kRuntimeNoneSessionSelection);
  }
  st.runtime.anchor_session_id = st.runtime.selected_session_id;
  st.runtime.selected_job_cursor =
      latest_runtime_job_cursor_for_campaign(st.runtime, campaign);
  update_runtime_selection_records(st);
  (void)queue_runtime_excerpt_refresh(st);
}

inline void select_runtime_none_campaign_and_related(
    CmdState &st, std::string_view session_selection_for_context = {}) {
  st.runtime.family_anchor_lane = kRuntimeCampaignLane;
  st.runtime.selected_campaign_cursor =
      std::string(kRuntimeNoneCampaignSelection);
  st.runtime.selected_session_id = std::string(session_selection_for_context);
  st.runtime.anchor_session_id.clear();
  st.runtime.selected_job_cursor = latest_runtime_orphan_job_cursor(st);
  update_runtime_selection_records(st);
  (void)queue_runtime_excerpt_refresh(st);
}

inline void select_runtime_job_and_related(CmdState &st,
                                           std::string_view job_cursor) {
  const bool preserve_none_session_context =
      runtime_is_session_lane(st.runtime.lane) &&
      st.runtime.job_filter_active &&
      runtime_is_none_session_selection(st.runtime.selected_session_id);
  const bool preserve_none_campaign_context =
      ((runtime_is_session_lane(st.runtime.lane) &&
        st.runtime.job_filter_active) ||
       (runtime_is_campaign_lane(st.runtime.lane) &&
        st.runtime.job_filter_active)) &&
      runtime_is_none_campaign_selection(st.runtime.selected_campaign_cursor);
  const auto job_index = runtime_find_job_index(st.runtime.jobs, job_cursor);
  if (!job_index.has_value()) {
    clear_runtime_inventory_selection(st);
    update_runtime_selection_records(st);
    return;
  }
  const auto &job = st.runtime.jobs[*job_index];
  st.runtime.family_anchor_lane = kRuntimeJobLane;
  st.runtime.selected_job_cursor = job.job_cursor;
  st.runtime.selected_campaign_cursor =
      runtime_campaign_cursor_for_job(st.runtime, job.job_cursor);
  if (st.runtime.selected_campaign_cursor.empty() &&
      preserve_none_campaign_context) {
    st.runtime.selected_campaign_cursor =
        std::string(kRuntimeNoneCampaignSelection);
  }
  st.runtime.selected_session_id =
      runtime_session_id_for_job(st.runtime, job.job_cursor);
  if (st.runtime.selected_session_id.empty() && preserve_none_session_context) {
    st.runtime.selected_session_id = std::string(kRuntimeNoneSessionSelection);
  }
  st.runtime.anchor_session_id = st.runtime.selected_session_id;
  update_runtime_selection_records(st);
  (void)queue_runtime_excerpt_refresh(st);
}

inline bool runtime_select_session_row_at_index(CmdState &st,
                                                std::size_t index) {
  if (index < st.runtime.sessions.size()) {
    select_runtime_session_and_related(
        st, st.runtime.sessions[index].marshal_session_id);
    return true;
  }
  if (runtime_has_none_session_row(st) && index == st.runtime.sessions.size()) {
    select_runtime_session_and_related(st, kRuntimeNoneSessionSelection);
    return true;
  }
  return false;
}

inline bool runtime_select_campaign_row_at_index(CmdState &st,
                                                 std::size_t index) {
  const auto visible = runtime_visible_campaign_indices(st);
  if (index < visible.size()) {
    select_runtime_campaign_and_related(
        st, st.runtime.campaigns[visible[index]].campaign_cursor);
    return true;
  }
  if (runtime_has_none_campaign_row(st) && index == visible.size()) {
    select_runtime_none_campaign_and_related(st);
    return true;
  }
  return false;
}

inline bool runtime_select_child_campaign_row_at_index(CmdState &st,
                                                       std::size_t index) {
  const auto visible = runtime_child_campaign_indices(st);
  if (index < visible.size()) {
    select_runtime_campaign_and_related(
        st, st.runtime.campaigns[visible[index]].campaign_cursor);
    return true;
  }
  if (runtime_child_has_none_campaign_row(st) && index == visible.size()) {
    select_runtime_none_campaign_and_related(st,
                                             st.runtime.selected_session_id);
    return true;
  }
  return false;
}

inline bool select_prev_runtime_item(CmdState &st) {
  if (runtime_is_device_lane(st.runtime.lane)) {
    const std::size_t idx = runtime_selected_device_index(st).value_or(0);
    if (idx == 0)
      return false;
    st.runtime.selected_device_index = idx - 1;
    return true;
  }
  if (runtime_is_session_lane(st.runtime.lane)) {
    if (st.runtime.job_filter_active) {
      const auto visible = runtime_child_job_indices(st);
      if (visible.empty())
        return false;
      const std::size_t idx = runtime_selected_child_job_index(st).value_or(0);
      if (idx == 0)
        return false;
      select_runtime_job_and_related(
          st, st.runtime.jobs[visible[idx - 1]].job_cursor);
      return true;
    }
    if (st.runtime.campaign_filter_active) {
      const std::size_t total = runtime_child_campaign_row_count(st);
      if (total == 0)
        return false;
      const std::size_t idx =
          runtime_selected_child_campaign_index(st).value_or(0);
      if (idx == 0)
        return false;
      return runtime_select_child_campaign_row_at_index(st, idx - 1);
    }
    const std::size_t total = runtime_session_row_count(st);
    if (total == 0)
      return false;
    const std::size_t idx = runtime_selected_session_index(st).value_or(0);
    if (idx == 0)
      return false;
    return runtime_select_session_row_at_index(st, idx - 1);
  }
  if (runtime_is_campaign_lane(st.runtime.lane)) {
    if (st.runtime.job_filter_active) {
      const auto visible = runtime_child_job_indices(st);
      if (visible.empty())
        return false;
      const std::size_t idx = runtime_selected_child_job_index(st).value_or(0);
      if (idx == 0)
        return false;
      select_runtime_job_and_related(
          st, st.runtime.jobs[visible[idx - 1]].job_cursor);
      return true;
    }
    const std::size_t total = runtime_campaign_row_count(st);
    if (total == 0)
      return false;
    const std::size_t idx = runtime_selected_campaign_index(st).value_or(0);
    if (idx == 0)
      return false;
    return runtime_select_campaign_row_at_index(st, idx - 1);
  }
  const auto visible = runtime_visible_job_indices(st);
  if (visible.empty())
    return false;
  const std::size_t idx = runtime_selected_job_index(st).value_or(0);
  if (idx == 0)
    return false;
  select_runtime_job_and_related(st,
                                 st.runtime.jobs[visible[idx - 1]].job_cursor);
  return true;
}

inline bool select_next_runtime_item(CmdState &st) {
  if (runtime_is_device_lane(st.runtime.lane)) {
    const std::size_t idx = runtime_selected_device_index(st).value_or(0);
    const std::size_t total = runtime_device_item_count(st);
    if (idx + 1 >= total)
      return false;
    st.runtime.selected_device_index = idx + 1;
    return true;
  }
  if (runtime_is_session_lane(st.runtime.lane)) {
    if (st.runtime.job_filter_active) {
      const auto visible = runtime_child_job_indices(st);
      if (visible.empty())
        return false;
      const std::size_t idx = runtime_selected_child_job_index(st).value_or(0);
      if (idx + 1 >= visible.size())
        return false;
      select_runtime_job_and_related(
          st, st.runtime.jobs[visible[idx + 1]].job_cursor);
      return true;
    }
    if (st.runtime.campaign_filter_active) {
      const std::size_t total = runtime_child_campaign_row_count(st);
      if (total == 0)
        return false;
      const std::size_t idx =
          runtime_selected_child_campaign_index(st).value_or(0);
      if (idx + 1 >= total)
        return false;
      return runtime_select_child_campaign_row_at_index(st, idx + 1);
    }
    const std::size_t total = runtime_session_row_count(st);
    if (total == 0)
      return false;
    const std::size_t idx = runtime_selected_session_index(st).value_or(0);
    if (idx + 1 >= total)
      return false;
    return runtime_select_session_row_at_index(st, idx + 1);
  }
  if (runtime_is_campaign_lane(st.runtime.lane)) {
    if (st.runtime.job_filter_active) {
      const auto visible = runtime_child_job_indices(st);
      if (visible.empty())
        return false;
      const std::size_t idx = runtime_selected_child_job_index(st).value_or(0);
      if (idx + 1 >= visible.size())
        return false;
      select_runtime_job_and_related(
          st, st.runtime.jobs[visible[idx + 1]].job_cursor);
      return true;
    }
    const std::size_t total = runtime_campaign_row_count(st);
    if (total == 0)
      return false;
    const std::size_t idx = runtime_selected_campaign_index(st).value_or(0);
    if (idx + 1 >= total)
      return false;
    return runtime_select_campaign_row_at_index(st, idx + 1);
  }
  const auto visible = runtime_visible_job_indices(st);
  if (visible.empty())
    return false;
  const std::size_t idx = runtime_selected_job_index(st).value_or(0);
  if (idx + 1 >= visible.size())
    return false;
  select_runtime_job_and_related(st,
                                 st.runtime.jobs[visible[idx + 1]].job_cursor);
  return true;
}

inline std::string runtime_detail_mode_label(RuntimeDetailMode mode) {
  switch (mode) {
  case RuntimeDetailMode::Manifest:
    return "manifest";
  case RuntimeDetailMode::Log:
    return "log";
  case RuntimeDetailMode::Trace:
    return "trace";
  }
  return "manifest";
}

inline void cycle_runtime_detail_mode(CmdState &st) {
  switch (st.runtime.detail_mode) {
  case RuntimeDetailMode::Manifest:
    st.runtime.detail_mode = RuntimeDetailMode::Log;
    break;
  case RuntimeDetailMode::Log:
    st.runtime.detail_mode = RuntimeDetailMode::Trace;
    break;
  case RuntimeDetailMode::Trace:
    st.runtime.detail_mode = RuntimeDetailMode::Manifest;
    break;
  }
  (void)queue_runtime_excerpt_refresh(st);
}

inline void set_runtime_status(CmdState &st, std::string status,
                               bool is_error) {
  st.runtime.status = std::move(status);
  st.runtime.status_is_error = is_error;
  if (is_error) {
    st.runtime.ok = false;
    st.runtime.error = st.runtime.status;
  } else {
    st.runtime.error.clear();
  }
}

inline RuntimeRefreshSnapshot runtime_collect_refresh_snapshot(
    cuwacunu::hero::runtime_mcp::app_context_t runtime_app, bool marshal_ok,
    cuwacunu::hero::marshal_mcp::app_context_t marshal_app,
    std::string marshal_error) {
  RuntimeRefreshSnapshot snapshot{};
  if (runtime_app.defaults.campaigns_root.empty()) {
    snapshot.error = "Runtime campaigns_root is not configured.";
    return snapshot;
  }

  if (marshal_ok) {
    if (!runtime_load_sessions_via_hero(marshal_ok, marshal_error, &marshal_app,
                                        &snapshot.sessions,
                                        &snapshot.session_list_error)) {
      snapshot.sessions.clear();
    }
  } else if (!marshal_error.empty()) {
    snapshot.session_list_error = std::move(marshal_error);
  }

  std::string campaign_error{};
  if (!runtime_load_campaigns_via_hero(&runtime_app, &snapshot.campaigns,
                                       &campaign_error)) {
    snapshot.error = campaign_error.empty() ? "failed to load runtime campaigns"
                                            : campaign_error;
    return snapshot;
  }

  if (!runtime_load_jobs_via_hero(&runtime_app, &snapshot.jobs,
                                  &snapshot.job_list_error)) {
    snapshot.jobs.clear();
  }

  snapshot.ok = true;
  return snapshot;
}

inline bool runtime_apply_refresh_snapshot(CmdState &st,
                                           RuntimeRefreshSnapshot snapshot) {
  if (!snapshot.ok) {
    set_runtime_status(st,
                       snapshot.error.empty()
                           ? "failed to load runtime inventory"
                           : snapshot.error,
                       true);
    return false;
  }

  const std::string previous_session_id = st.runtime.selected_session_id;
  const std::string previous_campaign_cursor =
      st.runtime.selected_campaign_cursor;
  const std::string previous_job_cursor = st.runtime.selected_job_cursor;
  const RuntimeLane previous_family_anchor_lane = st.runtime.family_anchor_lane;

  st.runtime.sessions = std::move(snapshot.sessions);
  st.runtime.campaigns = std::move(snapshot.campaigns);
  st.runtime.jobs = std::move(snapshot.jobs);
  rebuild_runtime_lineage_cache(st);
  clear_runtime_inventory_selection(st);
  update_runtime_selection_records(st);
  clear_runtime_job_excerpts(st);

  bool restored = false;
  const auto restore_previous_for_lane = [&](RuntimeLane lane) -> bool {
    if (runtime_is_session_lane(lane)) {
      if (runtime_is_none_session_selection(previous_session_id) &&
          runtime_has_none_session_row(st)) {
        select_runtime_session_and_related(st, kRuntimeNoneSessionSelection);
        return true;
      }
      if (!previous_session_id.empty() &&
          runtime_find_session_index(st.runtime.sessions, previous_session_id)
              .has_value()) {
        select_runtime_session_and_related(st, previous_session_id);
        return true;
      }
      return false;
    }
    if (runtime_is_campaign_lane(lane)) {
      if (runtime_is_none_campaign_selection(previous_campaign_cursor)) {
        if (runtime_is_none_session_selection(previous_session_id) &&
            runtime_has_none_session_row(st)) {
          select_runtime_session_and_related(st, kRuntimeNoneSessionSelection);
          if (runtime_child_has_none_campaign_row(st)) {
            select_runtime_none_campaign_and_related(
                st, std::string(kRuntimeNoneSessionSelection));
            return true;
          }
        }
        if (runtime_has_none_campaign_row(st)) {
          select_runtime_none_campaign_and_related(st);
          return true;
        }
      }
      if (!previous_campaign_cursor.empty() &&
          runtime_find_campaign_index(st.runtime.campaigns,
                                      previous_campaign_cursor)
              .has_value()) {
        if (runtime_is_none_session_selection(previous_session_id) &&
            runtime_has_none_session_row(st) &&
            runtime_session_id_for_campaign(st.runtime,
                                            previous_campaign_cursor)
                .empty()) {
          select_runtime_session_and_related(st, kRuntimeNoneSessionSelection);
        }
        select_runtime_campaign_and_related(st, previous_campaign_cursor);
        return true;
      }
      return false;
    }
    if (!previous_job_cursor.empty() &&
        runtime_find_job_index(st.runtime.jobs, previous_job_cursor)
            .has_value()) {
      const std::string parent_campaign_cursor =
          runtime_campaign_cursor_for_job(st.runtime, previous_job_cursor);
      const std::string parent_session_id =
          runtime_session_id_for_job(st.runtime, previous_job_cursor);
      if (runtime_is_none_session_selection(previous_session_id) &&
          runtime_has_none_session_row(st) && parent_session_id.empty()) {
        select_runtime_session_and_related(st, kRuntimeNoneSessionSelection);
      }
      if (runtime_is_none_campaign_selection(previous_campaign_cursor) &&
          runtime_has_none_campaign_row(st) && parent_campaign_cursor.empty()) {
        select_runtime_none_campaign_and_related(
            st, runtime_is_none_session_selection(previous_session_id)
                    ? std::string(kRuntimeNoneSessionSelection)
                    : std::string{});
      }
      select_runtime_job_and_related(st, previous_job_cursor);
      return true;
    }
    return false;
  };

  restored = restore_previous_for_lane(previous_family_anchor_lane);
  if (!restored)
    restored = restore_previous_for_lane(kRuntimeJobLane);
  if (!restored)
    restored = restore_previous_for_lane(kRuntimeCampaignLane);
  if (!restored)
    restored = restore_previous_for_lane(kRuntimeSessionLane);

  if (!restored) {
    const auto anchor_session = selected_operator_session_record(st);
    if (anchor_session.has_value() &&
        runtime_find_session_index(st.runtime.sessions,
                                   anchor_session->marshal_session_id)
            .has_value()) {
      select_runtime_session_and_related(st,
                                         anchor_session->marshal_session_id);
      restored = true;
    }
  }
  if (!restored && !st.runtime.sessions.empty()) {
    select_runtime_session_and_related(
        st, st.runtime.sessions.front().marshal_session_id);
    restored = true;
  }
  if (!restored && runtime_has_none_session_row(st)) {
    select_runtime_session_and_related(st, kRuntimeNoneSessionSelection);
    restored = true;
  }
  if (!restored && !st.runtime.campaigns.empty()) {
    select_runtime_campaign_and_related(
        st, st.runtime.campaigns.front().campaign_cursor);
    restored = true;
  }
  if (!restored && runtime_has_none_campaign_row(st)) {
    select_runtime_none_campaign_and_related(st);
    restored = true;
  }
  if (!restored && !st.runtime.jobs.empty()) {
    select_runtime_job_and_related(st, st.runtime.jobs.front().job_cursor);
    restored = true;
  }
  if (!restored) {
    clear_runtime_inventory_selection(st);
    update_runtime_selection_records(st);
  }

  st.runtime.ok = true;
  st.runtime.error.clear();
  if (!snapshot.session_list_error.empty() ||
      !snapshot.job_list_error.empty()) {
    std::string warning = "Runtime inventory partially loaded.";
    if (!snapshot.session_list_error.empty()) {
      warning += " sessions: " + snapshot.session_list_error + ".";
    }
    if (!snapshot.job_list_error.empty()) {
      warning += " jobs: " + snapshot.job_list_error + ".";
    }
    set_runtime_status(st, warning, false);
  } else {
    set_runtime_status(st, {}, false);
  }
  return true;
}

inline bool queue_runtime_refresh(CmdState &st) {
  if (st.runtime.refresh_pending) {
    st.runtime.refresh_requeue = true;
    return false;
  }
  st.runtime.refresh_pending = true;
  st.runtime.refresh_requeue = false;
  auto runtime_app = st.runtime.app;
  auto marshal_app = st.runtime.marshal_app;
  const bool marshal_ok = st.runtime.marshal_ok;
  const std::string marshal_error = st.runtime.marshal_error;
  st.runtime.refresh_future =
      std::async(std::launch::async,
                 [runtime_app = std::move(runtime_app), marshal_ok,
                  marshal_app = std::move(marshal_app),
                  marshal_error]() mutable -> RuntimeRefreshSnapshot {
                   return runtime_collect_refresh_snapshot(
                       std::move(runtime_app), marshal_ok,
                       std::move(marshal_app), marshal_error);
                 });
  return true;
}

inline bool queue_runtime_excerpt_refresh(CmdState &st) {
  clear_runtime_job_excerpts(st);
  if (!st.runtime.job.has_value() ||
      st.runtime.detail_mode == RuntimeDetailMode::Manifest) {
    st.runtime.excerpt_requeue = false;
    return false;
  }
  if (st.runtime.excerpt_pending) {
    st.runtime.excerpt_requeue = true;
    return false;
  }
  st.runtime.excerpt_pending = true;
  st.runtime.excerpt_requeue = false;
  auto runtime_app = st.runtime.app;
  const auto job = *st.runtime.job;
  const RuntimeDetailMode detail_mode = st.runtime.detail_mode;
  st.runtime.excerpt_future =
      std::async(std::launch::async,
                 [runtime_app = std::move(runtime_app), job,
                  detail_mode]() mutable -> RuntimeExcerptSnapshot {
                   return runtime_collect_excerpt_snapshot(
                       std::move(runtime_app), job, detail_mode);
                 });
  return true;
}

inline bool poll_runtime_async_updates(CmdState &st) {
  bool dirty = false;
  if (st.runtime.refresh_pending && st.runtime.refresh_future.valid() &&
      st.runtime.refresh_future.wait_for(std::chrono::milliseconds(0)) ==
          std::future_status::ready) {
    RuntimeRefreshSnapshot snapshot = st.runtime.refresh_future.get();
    st.runtime.refresh_pending = false;
    dirty = runtime_apply_refresh_snapshot(st, std::move(snapshot)) || dirty;
    if (st.runtime.refresh_requeue) {
      st.runtime.refresh_requeue = false;
      (void)queue_runtime_refresh(st);
    }
  }
  if (st.runtime.excerpt_pending && st.runtime.excerpt_future.valid() &&
      st.runtime.excerpt_future.wait_for(std::chrono::milliseconds(0)) ==
          std::future_status::ready) {
    RuntimeExcerptSnapshot snapshot = st.runtime.excerpt_future.get();
    st.runtime.excerpt_pending = false;
    bool applied = false;
    if (st.runtime.job.has_value() &&
        st.runtime.job->job_cursor == snapshot.job_cursor &&
        st.runtime.detail_mode == snapshot.detail_mode) {
      st.runtime.job_stdout_excerpt = std::move(snapshot.stdout_excerpt);
      st.runtime.job_stderr_excerpt = std::move(snapshot.stderr_excerpt);
      st.runtime.job_trace_excerpt = std::move(snapshot.trace_excerpt);
      applied = true;
    }
    if (st.runtime.excerpt_requeue || !applied) {
      st.runtime.excerpt_requeue = false;
      (void)queue_runtime_excerpt_refresh(st);
    }
    dirty = applied || dirty;
  }
  if (st.runtime.device_pending && st.runtime.device_future.valid() &&
      st.runtime.device_future.wait_for(std::chrono::milliseconds(0)) ==
          std::future_status::ready) {
    RuntimeDeviceSnapshot snapshot = st.runtime.device_future.get();
    st.runtime.device_pending = false;
    const bool applied = runtime_apply_device_snapshot(st, std::move(snapshot));
    if (st.runtime.device_requeue) {
      st.runtime.device_requeue = false;
      (void)queue_runtime_device_refresh(st);
    }
    dirty = applied || dirty;
  }
  return dirty;
}

inline bool refresh_runtime_state(CmdState &st) {
  const bool applied = runtime_apply_refresh_snapshot(
      st, runtime_collect_refresh_snapshot(
              st.runtime.app, st.runtime.marshal_ok, st.runtime.marshal_app,
              st.runtime.marshal_error));
  st.runtime.device_last_refresh_ms = runtime_monotonic_now_ms();
  (void)runtime_apply_device_snapshot(st, runtime_collect_device_snapshot());
  return applied;
}

template <class AppendLog>
inline void append_runtime_show_lines(const CmdState &st,
                                      AppendLog &&append_log) {
  append_log("screen=runtime", "show", "#d8d8ff");
  append_log("detail_mode=" + runtime_detail_mode_label(st.runtime.detail_mode),
             "show", "#d8d8ff");
  append_log("lane=" + runtime_lane_label(st.runtime.lane) +
                 " focus=" + runtime_focus_label(st.runtime.focus),
             "show", "#d8d8ff");
  if (st.runtime.refresh_pending) {
    append_log("runtime_refresh=background", "show", "#d8d8ff");
  }
  if (st.runtime.excerpt_pending) {
    append_log("runtime_excerpt=background", "show", "#d8d8ff");
  }
  if (!st.runtime.anchor_session_id.empty()) {
    append_log("anchor_session_id=" + st.runtime.anchor_session_id, "show",
               "#d8d8ff");
  }
  append_log("sessions=" + std::to_string(st.runtime.sessions.size()) +
                 " campaigns=" + std::to_string(st.runtime.campaigns.size()) +
                 " jobs=" + std::to_string(st.runtime.jobs.size()),
             "show", "#d8d8ff");
  std::size_t detached_campaigns = 0;
  for (const auto &campaign : st.runtime.campaigns) {
    if (runtime_session_id_for_campaign(st.runtime, campaign.campaign_cursor)
            .empty()) {
      ++detached_campaigns;
    }
  }
  std::size_t orphan_jobs = 0;
  std::size_t sessionless_jobs = 0;
  for (const auto &job : st.runtime.jobs) {
    const std::string campaign_cursor =
        runtime_campaign_cursor_for_job(st.runtime, job.job_cursor);
    const std::string marshal_session_id =
        runtime_session_id_for_job(st.runtime, job.job_cursor);
    if (campaign_cursor.empty()) {
      ++orphan_jobs;
    } else if (marshal_session_id.empty()) {
      ++sessionless_jobs;
    }
  }
  append_log("detached_campaigns=" + std::to_string(detached_campaigns) +
                 " orphan_jobs=" + std::to_string(orphan_jobs) +
                 " sessionless_jobs=" + std::to_string(sessionless_jobs),
             "show", "#d8d8ff");
  if (!st.runtime.selected_session_id.empty()) {
    append_log("selected_session_id=" + st.runtime.selected_session_id, "show",
               "#d8d8ff");
  }
  if (!st.runtime.selected_campaign_cursor.empty()) {
    append_log("selected_campaign=" + st.runtime.selected_campaign_cursor,
               "show", "#d8d8ff");
  }
  if (!st.runtime.selected_job_cursor.empty()) {
    append_log("selected_job=" + st.runtime.selected_job_cursor, "show",
               "#d8d8ff");
  }
  if (const auto idx = runtime_current_lane_selected_index(st);
      idx.has_value()) {
    append_log("lane_selection=" + std::to_string(*idx + 1) + "/" +
                   std::to_string(current_runtime_lane_count(st)),
               "show", "#d8d8ff");
  }
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
