inline bool lattice_set_section(CmdState &st, LatticeSection section) {
  if (st.lattice.selected_section == section)
    return false;
  st.lattice.selected_section = section;
  if (section == LatticeSection::Lattice) {
    st.lattice.mode = LatticeMode::Roots;
  }
  clamp_lattice_state(st);
  if (section == LatticeSection::Components) {
    queue_lattice_selected_member_detail_if_needed(st);
  } else if (section == LatticeSection::Lattice &&
             st.lattice.mode == LatticeMode::Facts) {
    queue_lattice_selected_member_detail_if_needed(st);
  } else {
    st.lattice.detail_requeue = false;
  }
  if (section == LatticeSection::Lattice &&
      st.lattice.mode == LatticeMode::Views) {
    queue_lattice_selected_view_if_needed(st);
  } else {
    st.lattice.view_requeue = false;
  }
  return true;
}

inline bool lattice_select_next_section(CmdState &st) {
  switch (st.lattice.selected_section) {
  case LatticeSection::Overview:
    return lattice_set_section(st, LatticeSection::Components);
  case LatticeSection::Components:
    return lattice_set_section(st, LatticeSection::Lattice);
  case LatticeSection::Lattice:
    return lattice_set_section(st, LatticeSection::Overview);
  }
  return false;
}

inline bool lattice_select_prev_section(CmdState &st) {
  switch (st.lattice.selected_section) {
  case LatticeSection::Overview:
    return lattice_set_section(st, LatticeSection::Lattice);
  case LatticeSection::Components:
    return lattice_set_section(st, LatticeSection::Overview);
  case LatticeSection::Lattice:
    return lattice_set_section(st, LatticeSection::Components);
  }
  return false;
}

inline bool lattice_select_prev_view(CmdState &st) {
  if (st.lattice.views.empty() || st.lattice.selected_view_index == 0)
    return false;
  --st.lattice.selected_view_index;
  queue_lattice_selected_view_if_needed(st);
  return true;
}

inline bool lattice_select_next_view(CmdState &st) {
  if (st.lattice.views.empty() ||
      st.lattice.selected_view_index + 1 >= st.lattice.views.size()) {
    return false;
  }
  ++st.lattice.selected_view_index;
  queue_lattice_selected_view_if_needed(st);
  return true;
}

inline bool lattice_select_first_view(CmdState &st) {
  if (st.lattice.views.empty() || st.lattice.selected_view_index == 0)
    return false;
  st.lattice.selected_view_index = 0;
  queue_lattice_selected_view_if_needed(st);
  return true;
}

inline bool lattice_select_last_view(CmdState &st) {
  if (st.lattice.views.empty())
    return false;
  const std::size_t last = st.lattice.views.size() - 1;
  if (st.lattice.selected_view_index == last)
    return false;
  st.lattice.selected_view_index = last;
  queue_lattice_selected_view_if_needed(st);
  return true;
}

inline void lattice_ensure_member_identity(LatticeMemberEntry *member) {
  if (member == nullptr || member->canonical_path.empty())
    return;
  if (member->project_path.empty()) {
    member->project_path =
        lattice_project_path_for_canonical(member->canonical_path);
  }
  if (member->family.empty()) {
    member->family = lattice_family_from_canonical(member->canonical_path);
  }
  if (member->hashimyei.empty()) {
    std::string hashimyei{};
    if (lattice_has_hashimyei_suffix(member->canonical_path, &hashimyei)) {
      member->hashimyei = std::move(hashimyei);
    }
  }
  if (member->display_name.empty()) {
    member->display_name = lattice_display_name_for_member(
        member->canonical_path, member->hashimyei);
  }
}

inline LatticeFamilyGroup *
lattice_ensure_family_group(std::vector<LatticeFamilyGroup> *families,
                            std::string_view family_name) {
  if (families == nullptr)
    return nullptr;
  const std::string family = trim_copy(std::string(family_name));
  for (auto &entry : *families) {
    if (entry.family == family)
      return &entry;
  }
  families->push_back(LatticeFamilyGroup{.family = family});
  return &families->back();
}

inline LatticeProjectGroup *
lattice_ensure_project_group(LatticeFamilyGroup *family,
                             std::string_view project_path) {
  if (family == nullptr)
    return nullptr;
  const std::string project = trim_copy(std::string(project_path));
  for (auto &entry : family->projects) {
    if (entry.project_path == project)
      return &entry;
  }
  family->projects.push_back(
      LatticeProjectGroup{.project_path = project, .family = family->family});
  return &family->projects.back();
}

inline LatticeMemberEntry *
lattice_ensure_member_entry(std::vector<LatticeFamilyGroup> *families,
                            std::string_view canonical_path) {
  if (families == nullptr)
    return nullptr;
  const std::string canonical =
      normalize_lattice_canonical_path(canonical_path);
  const std::string family_name = lattice_family_from_canonical(canonical);
  const std::string project_path =
      lattice_project_path_for_canonical(canonical);
  auto *family = lattice_ensure_family_group(families, family_name);
  auto *project = lattice_ensure_project_group(family, project_path);
  if (project == nullptr)
    return nullptr;
  for (auto &member : project->members) {
    if (member.canonical_path == canonical)
      return &member;
  }
  project->members.push_back(LatticeMemberEntry{
      .canonical_path = canonical,
      .project_path = project_path,
      .family = family_name,
  });
  lattice_ensure_member_identity(&project->members.back());
  return &project->members.back();
}

inline void
lattice_apply_hashimyei_ref_to_member(LatticeMemberEntry *member,
                                      const lattice_hashimyei_ref_t &ref) {
  if (member == nullptr)
    return;
  if (member->canonical_path.empty())
    member->canonical_path = ref.canonical_path;
  if (member->hashimyei.empty())
    member->hashimyei = ref.hashimyei;
  member->in_hashimyei_catalog = true;
  member->component_count =
      std::max(member->component_count, ref.component_count);
  member->report_fragment_count =
      std::max(member->report_fragment_count, ref.report_fragment_count);
  lattice_ensure_member_identity(member);
}

inline void
lattice_apply_latest_fact_summary_to_member(LatticeMemberEntry *member,
                                            const LatticeFactSummary &fact) {
  if (member == nullptr)
    return;
  member->has_fact = true;
  member->fact = fact;
  if (member->canonical_path.empty())
    member->canonical_path = fact.canonical_path;
  member->report_fragment_count =
      std::max(member->report_fragment_count, fact.fragment_count);
  lattice_ensure_member_identity(member);
}

inline void lattice_merge_member_entry(LatticeMemberEntry *dst,
                                       const LatticeMemberEntry &src) {
  if (dst == nullptr)
    return;
  if (dst->canonical_path.empty())
    dst->canonical_path = src.canonical_path;
  if (dst->project_path.empty())
    dst->project_path = src.project_path;
  if (dst->family.empty())
    dst->family = src.family;
  if (dst->display_name.empty())
    dst->display_name = src.display_name;
  if (dst->hashimyei.empty())
    dst->hashimyei = src.hashimyei;
  if (dst->contract_hash.empty())
    dst->contract_hash = src.contract_hash;
  if (dst->component_compatibility_sha256_hex.empty())
    dst->component_compatibility_sha256_hex =
        src.component_compatibility_sha256_hex;
  if (dst->lineage_state.empty() || dst->lineage_state == "unknown" ||
      dst->lineage_state == "fact-only") {
    if (!src.lineage_state.empty())
      dst->lineage_state = src.lineage_state;
  }
  dst->in_hashimyei_catalog =
      dst->in_hashimyei_catalog || src.in_hashimyei_catalog;
  const bool had_component = dst->has_component;
  dst->has_component = had_component || src.has_component;
  if (src.has_component &&
      (!had_component || src.component.ts_ms >= dst->component.ts_ms)) {
    dst->component = src.component;
  }
  dst->has_fact = dst->has_fact || src.has_fact;
  if (src.fact.fragment_count > dst->fact.fragment_count) {
    dst->fact.fragment_count = src.fact.fragment_count;
  }
  if (src.fact.available_context_count > dst->fact.available_context_count) {
    dst->fact.available_context_count = src.fact.available_context_count;
  }
  if (src.fact.latest_ts_ms >= dst->fact.latest_ts_ms) {
    if (!src.fact.latest_wave_cursor.empty()) {
      dst->fact.latest_wave_cursor = src.fact.latest_wave_cursor;
    }
    dst->fact.latest_ts_ms = src.fact.latest_ts_ms;
  }
  lattice_merge_string_values(&dst->fact.wave_cursors, src.fact.wave_cursors);
  lattice_merge_string_values(&dst->fact.binding_ids, src.fact.binding_ids);
  lattice_merge_string_values(&dst->fact.semantic_taxa, src.fact.semantic_taxa);
  lattice_merge_string_values(&dst->fact.source_runtime_cursors,
                              src.fact.source_runtime_cursors);
  if (src.family_rank.has_value() &&
      (!dst->family_rank.has_value() || *src.family_rank < *dst->family_rank)) {
    dst->family_rank = src.family_rank;
  }
  dst->detail_loaded = dst->detail_loaded || src.detail_loaded;
  lattice_ensure_member_identity(dst);
}

inline void
lattice_recompute_rollups(std::vector<LatticeFamilyGroup> *families) {
  if (families == nullptr)
    return;
  for (auto &family : *families) {
    family.member_count = 0;
    family.active_member_count = 0;
    family.fragment_count = 0;
    family.available_context_count = 0;
    family.latest_ts_ms = 0;
    family.wave_cursors.clear();
    family.binding_ids.clear();
    family.semantic_taxa.clear();
    family.contract_hashes.clear();

    for (auto &project : family.projects) {
      std::sort(project.members.begin(), project.members.end(),
                [](const auto &a, const auto &b) {
                  return a.canonical_path < b.canonical_path;
                });
      project.fragment_count = 0;
      project.available_context_count = 0;
      project.latest_ts_ms = 0;
      project.wave_cursors.clear();
      project.binding_ids.clear();
      project.semantic_taxa.clear();
      project.source_runtime_cursors.clear();
      project.contract_hashes.clear();

      for (const auto &member : project.members) {
        ++family.member_count;
        if (lattice_member_lineage_state_text(member) == "active") {
          ++family.active_member_count;
        }
        project.fragment_count += member.fact.fragment_count;
        project.available_context_count += member.fact.available_context_count;
        project.latest_ts_ms =
            std::max(project.latest_ts_ms, lattice_member_latest_ts_ms(member));
        lattice_merge_string_values(&project.wave_cursors,
                                    member.fact.wave_cursors);
        lattice_merge_string_values(&project.binding_ids,
                                    member.fact.binding_ids);
        lattice_merge_string_values(&project.semantic_taxa,
                                    member.fact.semantic_taxa);
        lattice_merge_string_values(&project.source_runtime_cursors,
                                    member.fact.source_runtime_cursors);
        lattice_append_unique_value(&project.contract_hashes,
                                    lattice_contract_hash_for_member(member));
      }

      family.fragment_count += project.fragment_count;
      family.available_context_count += project.available_context_count;
      family.latest_ts_ms = std::max(family.latest_ts_ms, project.latest_ts_ms);
      lattice_merge_string_values(&family.wave_cursors, project.wave_cursors);
      lattice_merge_string_values(&family.binding_ids, project.binding_ids);
      lattice_merge_string_values(&family.semantic_taxa, project.semantic_taxa);
      lattice_merge_string_values(&family.contract_hashes,
                                  project.contract_hashes);
    }

    std::sort(family.projects.begin(), family.projects.end(),
              [](const auto &a, const auto &b) {
                return a.project_path < b.project_path;
              });
  }
  std::sort(families->begin(), families->end(),
            [](const auto &a, const auto &b) { return a.family < b.family; });
}

inline void
lattice_merge_family_snapshots(std::vector<LatticeFamilyGroup> *base,
                               const std::vector<LatticeFamilyGroup> &overlay) {
  if (base == nullptr)
    return;
  for (const auto &overlay_family : overlay) {
    for (const auto &overlay_project : overlay_family.projects) {
      for (const auto &overlay_member : overlay_project.members) {
        auto *member =
            lattice_ensure_member_entry(base, overlay_member.canonical_path);
        if (member == nullptr)
          continue;
        lattice_merge_member_entry(member, overlay_member);
      }
    }
  }
  lattice_recompute_rollups(base);
}

inline bool lattice_restore_selection(
    CmdState &st, const lattice_refresh_selection_key_t &previous_selection) {
  if (st.lattice.rows.empty())
    return false;
  for (std::size_t i = 0; i < st.lattice.rows.size(); ++i) {
    const auto &row = st.lattice.rows[i];
    if (previous_selection.kind != row.kind)
      continue;
    const auto &family = st.lattice.families[row.family_index];
    if (row.kind == LatticeRowKind::Family &&
        family.family == previous_selection.family) {
      st.lattice.selected_row = i;
      return true;
    }
    if (row.kind == LatticeRowKind::Project) {
      const auto &project = family.projects[row.project_index];
      if (project.project_path == previous_selection.project) {
        st.lattice.selected_row = i;
        return true;
      }
    }
    if (row.kind == LatticeRowKind::Member) {
      const auto &project = family.projects[row.project_index];
      const auto &member = project.members[row.member_index];
      if (member.canonical_path == previous_selection.canonical_path) {
        st.lattice.selected_row = i;
        return true;
      }
    }
  }
  return false;
}

inline LatticeRefreshSnapshot lattice_collect_refresh_snapshot(
    const cuwacunu::hero::lattice_mcp::wave_runtime_defaults_t
        &lattice_defaults,
    const cuwacunu::hero::hashimyei_mcp::hashimyei_runtime_defaults_t
        &hashimyei_defaults,
    LatticeRefreshMode mode) {
  LatticeRefreshSnapshot snapshot{};
  std::string error{};
  cuwacunu::hero::lattice_mcp::app_context_t lattice_app{};
  lattice_app.store_root = lattice_defaults.store_root;
  lattice_app.lattice_catalog_path = lattice_defaults.catalog_path;
  lattice_app.hashimyei_catalog_path = hashimyei_defaults.catalog_path;
  lattice_app.close_catalog_after_execute = true;

  cuwacunu::hero::hashimyei_mcp::app_context_t hashimyei_app{};
  hashimyei_app.store_root = hashimyei_defaults.store_root;
  hashimyei_app.lattice_catalog_path = lattice_defaults.catalog_path;
  hashimyei_app.catalog_options.catalog_path = hashimyei_defaults.catalog_path;
  hashimyei_app.catalog_options.encrypted = false;
  hashimyei_app.close_catalog_after_execute = true;

  if (mode == LatticeRefreshMode::SyncStore) {
    std::string hashimyei_result{};
    if (!cuwacunu::hero::hashimyei_mcp::execute_tool_json(
            "hero.hashimyei.reset_catalog", "{\"reingest\":true}",
            &hashimyei_app, &hashimyei_result, &error)) {
      snapshot.error = std::move(error);
      return snapshot;
    }
    lattice_json_value_t hashimyei_structured{};
    if (!lattice_parse_tool_structured_content(hashimyei_result,
                                               &hashimyei_structured, &error)) {
      snapshot.error = std::move(error);
      return snapshot;
    }
    lattice_json_value_t ignored{};
    if (!lattice_invoke_tool(&lattice_app, "hero.lattice.refresh",
                             "{\"reingest\":true}", &ignored, &error)) {
      snapshot.error = std::move(error);
      return snapshot;
    }
  }

  std::string views_error{};
  const bool views_ok = lattice_load_view_definitions(
      &lattice_app, &snapshot.views, &views_error);
  (void)views_ok;

  std::vector<lattice_hashimyei_ref_t> hashimyei_refs{};
  std::string hashimyei_error{};
  const bool hashimyei_ok = lattice_load_hashimyei_refs(
      &hashimyei_app, &hashimyei_refs, &hashimyei_error);

  std::vector<LatticeFactSummary> facts{};
  std::string facts_error{};
  const bool facts_ok =
      lattice_load_latest_fact_summaries(&lattice_app, &facts, &facts_error);

  if (!hashimyei_ok && !facts_ok) {
    snapshot.error = !facts_error.empty()
                         ? facts_error
                         : (!hashimyei_error.empty()
                                ? hashimyei_error
                                : std::string("lattice refresh failed"));
    return snapshot;
  }

  snapshot.discovered_component_count = 0;
  snapshot.discovered_report_manifest_count = 0;
  for (const auto &ref : hashimyei_refs) {
    snapshot.discovered_component_count += ref.component_count;
    snapshot.discovered_report_manifest_count += ref.report_fragment_count;
    auto *member =
        lattice_ensure_member_entry(&snapshot.families, ref.canonical_path);
    if (member == nullptr)
      continue;
    lattice_apply_hashimyei_ref_to_member(member, ref);
  }
  for (const auto &fact : facts) {
    auto *member =
        lattice_ensure_member_entry(&snapshot.families, fact.canonical_path);
    if (member == nullptr)
      continue;
    lattice_apply_latest_fact_summary_to_member(member, fact);
  }

  if (facts.empty() && !hashimyei_refs.empty()) {
    snapshot.used_hashimyei_fallback = true;
  } else if (!facts.empty() && !hashimyei_refs.empty()) {
    snapshot.used_hashimyei_enrichment = true;
  }
  lattice_recompute_rollups(&snapshot.families);
  snapshot.ok = true;
  return snapshot;
}

inline bool lattice_apply_refresh_snapshot(CmdState &st,
                                           LatticeRefreshSnapshot snapshot) {
  if (!snapshot.ok) {
    return lattice_handle_refresh_failure(st, snapshot.error);
  }

  const lattice_refresh_selection_key_t previous_selection =
      capture_lattice_selection_key(st);
  st.lattice.families = std::move(snapshot.families);
  st.lattice.views = std::move(snapshot.views);
  st.lattice.used_hashimyei_fallback = snapshot.used_hashimyei_fallback;
  st.lattice.used_hashimyei_enrichment = snapshot.used_hashimyei_enrichment;
  st.lattice.discovered_component_count = snapshot.discovered_component_count;
  st.lattice.discovered_report_manifest_count =
      snapshot.discovered_report_manifest_count;
  st.lattice.view_snapshot = LatticeViewTransportSnapshot{};
  rebuild_lattice_visible_rows(st);
  rebuild_lattice_component_rows(st);
  if (!lattice_restore_selection(st, previous_selection)) {
    clamp_lattice_state(st);
  }
  if (st.lattice.selected_section == LatticeSection::Components ||
      (st.lattice.selected_section == LatticeSection::Lattice &&
       st.lattice.mode == LatticeMode::Facts)) {
    queue_lattice_selected_member_detail_if_needed(st);
  }
  if (st.lattice.selected_section == LatticeSection::Lattice &&
      st.lattice.mode == LatticeMode::Views) {
    queue_lattice_selected_view_if_needed(st);
  }

  st.lattice.ok = true;
  st.lattice.error.clear();
  if (st.lattice.rows.empty()) {
    if (st.lattice.used_hashimyei_fallback ||
        st.lattice.used_hashimyei_enrichment) {
      set_lattice_status(
          st,
          "Showing Hashimyei components; no lattice facts are available yet.",
          false);
    } else {
      set_lattice_status(
          st, "Lattice is ready but no components or lattice facts exist yet.",
          false);
    }
  } else {
    if (st.lattice.used_hashimyei_fallback) {
      set_lattice_status(
          st,
          "Showing Hashimyei components while lattice facts are still empty.",
          false);
    } else if (st.lattice.used_hashimyei_enrichment) {
      set_lattice_status(st,
                         "Showing lattice facts with Hashimyei identity "
                         "enrichment for uncovered rows.",
                         false);
    } else {
      set_lattice_status(st, {}, false);
    }
  }
  log_dbg("[iinuji:lattice] refresh_apply mode=%s families=%zu rows=%zu "
          "views=%zu fallback=%d enrichment=%d\n",
          lattice_refresh_mode_name(st.lattice.refresh_mode),
          st.lattice.families.size(), st.lattice.rows.size(),
          st.lattice.views.size(), st.lattice.used_hashimyei_fallback ? 1 : 0,
          st.lattice.used_hashimyei_enrichment ? 1 : 0);
  return true;
}

inline bool
refresh_lattice_state(CmdState &st,
                      LatticeRefreshMode mode = LatticeRefreshMode::Snapshot) {
  log_dbg("[iinuji:lattice] refresh_immediate_begin mode=%s\n",
          lattice_refresh_mode_name(mode));
  st.lattice.refresh_pending = false;
  st.lattice.refresh_started_at_ms = 0;
  st.lattice.refresh_timed_out = false;
  st.lattice.detail_refresh_pending = false;
  st.lattice.view_refresh_pending = false;
  st.lattice.refresh_requeue = false;
  st.lattice.detail_requeue = false;
  st.lattice.view_requeue = false;
  st.lattice.refresh_mode = LatticeRefreshMode::Snapshot;

  std::string error{};
  if (!ensure_lattice_defaults_loaded(st, &error)) {
    st.lattice.families.clear();
    st.lattice.component_roots.clear();
    st.lattice.rows.clear();
    st.lattice.component_rows.clear();
    st.lattice.selected_row = 0;
    st.lattice.selected_component_row = 0;
    st.lattice.refresh_started_at_ms = 0;
    st.lattice.refresh_timed_out = false;
    set_lattice_status(st, error, true);
    log_warn(
        "[iinuji:lattice] refresh_immediate_defaults_failed mode=%s error=%s\n",
        lattice_refresh_mode_name(mode), error.c_str());
    return false;
  }
  const bool ok = lattice_apply_refresh_snapshot(
      st,
      lattice_collect_refresh_snapshot(st.lattice.lattice_defaults,
                                       st.lattice.hashimyei_defaults, mode));
  log_dbg("[iinuji:lattice] refresh_immediate_finish mode=%s ok=%d rows=%zu "
          "views=%zu\n",
          lattice_refresh_mode_name(mode), ok ? 1 : 0, st.lattice.rows.size(),
          st.lattice.views.size());
  return ok;
}

inline void queue_lattice_refresh(CmdState &st, LatticeRefreshMode mode,
                                  std::string status) {
  std::string error{};
  if (!ensure_lattice_defaults_loaded(st, &error)) {
    st.lattice.families.clear();
    st.lattice.component_roots.clear();
    st.lattice.rows.clear();
    st.lattice.component_rows.clear();
    st.lattice.selected_row = 0;
    st.lattice.selected_component_row = 0;
    st.lattice.refresh_started_at_ms = 0;
    st.lattice.refresh_timed_out = false;
    set_lattice_status(st, error, true);
    log_warn(
        "[iinuji:lattice] refresh_queue_defaults_failed mode=%s error=%s\n",
        lattice_refresh_mode_name(mode), error.c_str());
    return;
  }
  if (st.lattice.refresh_pending) {
    if (st.lattice.refresh_mode == LatticeRefreshMode::SyncStore &&
        mode == LatticeRefreshMode::SyncStore) {
      set_lattice_status(st,
                         "Catalog refresh already running in the background; "
                         "full reingest can take about a minute.",
                         false);
      log_dbg("[iinuji:lattice] refresh_queue_duplicate_sync_ignored\n");
      return;
    }
    if (st.lattice.refresh_mode == LatticeRefreshMode::SyncStore &&
        mode == LatticeRefreshMode::Snapshot) {
      log_dbg(
          "[iinuji:lattice] refresh_queue_ignored current=%s requested=%s\n",
          lattice_refresh_mode_name(st.lattice.refresh_mode),
          lattice_refresh_mode_name(mode));
      return;
    }
    st.lattice.refresh_requeue = true;
    st.lattice.refresh_requeue_mode = mode;
    st.lattice.refresh_mode = mode;
    if (st.lattice.refresh_timed_out) {
      set_lattice_status(
          st,
          "Lattice refresh exceeded 3s; retry queued behind the stalled "
          "attempt. Press r again after it clears, or restart after runtime "
          "resets.",
          true);
    } else {
      set_lattice_status(st,
                         status.empty() ? lattice_loading_status_text(mode)
                                        : std::move(status),
                         false);
    }
    log_dbg("[iinuji:lattice] refresh_requeued current=%s requested=%s "
            "timed_out=%d\n",
            lattice_refresh_mode_name(st.lattice.refresh_mode),
            lattice_refresh_mode_name(mode),
            st.lattice.refresh_timed_out ? 1 : 0);
    return;
  }
  st.lattice.refresh_pending = true;
  st.lattice.refresh_requeue = false;
  st.lattice.refresh_requeue_mode = LatticeRefreshMode::Snapshot;
  st.lattice.view_requeue = false;
  st.lattice.refresh_mode = mode;
  st.lattice.refresh_started_at_ms = lattice_now_ms();
  st.lattice.refresh_timed_out = false;
  set_lattice_status(st,
                     status.empty() ? lattice_loading_status_text(mode)
                                    : std::move(status),
                     false);
  const auto lattice_defaults = st.lattice.lattice_defaults;
  const auto hashimyei_defaults = st.lattice.hashimyei_defaults;
  st.lattice.refresh_future =
      std::async(std::launch::async,
                 [lattice_defaults, hashimyei_defaults,
                  mode]() mutable -> LatticeRefreshSnapshot {
                   return lattice_collect_refresh_snapshot(
                       lattice_defaults, hashimyei_defaults, mode);
                 });
  log_dbg("[iinuji:lattice] refresh_async_started mode=%s cached_rows=%zu "
          "cached_views=%zu\n",
          lattice_refresh_mode_name(mode), st.lattice.rows.size(),
          st.lattice.views.size());
}

inline bool poll_lattice_async_updates(CmdState &st) {
  bool dirty = false;
  if (st.lattice.refresh_pending && !st.lattice.refresh_timed_out &&
      st.lattice.refresh_started_at_ms > 0) {
    const std::uint64_t elapsed_ms =
        lattice_now_ms() - st.lattice.refresh_started_at_ms;
    if (st.lattice.refresh_mode == LatticeRefreshMode::Snapshot &&
        elapsed_ms >= kLatticeRefreshUiTimeoutMs) {
      st.lattice.refresh_timed_out = true;
      set_lattice_status(
          st,
          "Lattice refresh exceeded 3s and looks stuck. Waiting in the "
          "background; keep using cached data, or press r to queue one retry.",
          true);
      log_warn(
          "[iinuji:lattice] refresh_async_timeout mode=%s elapsed_ms=%llu\n",
          lattice_refresh_mode_name(st.lattice.refresh_mode),
          static_cast<unsigned long long>(elapsed_ms));
      dirty = true;
    }
  }
  if (st.lattice.refresh_pending && st.lattice.refresh_future.valid() &&
      st.lattice.refresh_future.wait_for(std::chrono::milliseconds(0)) ==
          std::future_status::ready) {
    const std::uint64_t elapsed_ms =
        st.lattice.refresh_started_at_ms > 0
            ? lattice_now_ms() - st.lattice.refresh_started_at_ms
            : 0;
    LatticeRefreshSnapshot snapshot = st.lattice.refresh_future.get();
    st.lattice.refresh_pending = false;
    st.lattice.refresh_started_at_ms = 0;
    st.lattice.refresh_timed_out = false;
    if (snapshot.ok) {
      log_dbg("[iinuji:lattice] refresh_async_complete mode=%s elapsed_ms=%llu "
              "requeue=%d\n",
              lattice_refresh_mode_name(st.lattice.refresh_mode),
              static_cast<unsigned long long>(elapsed_ms),
              st.lattice.refresh_requeue ? 1 : 0);
    } else {
      log_warn("[iinuji:lattice] refresh_async_failed mode=%s elapsed_ms=%llu "
               "error=%s\n",
               lattice_refresh_mode_name(st.lattice.refresh_mode),
               static_cast<unsigned long long>(elapsed_ms),
               snapshot.error.c_str());
    }
    dirty = lattice_apply_refresh_snapshot(st, std::move(snapshot)) || dirty;
    if (st.lattice.refresh_requeue) {
      const auto mode = st.lattice.refresh_requeue_mode;
      st.lattice.refresh_requeue = false;
      st.lattice.refresh_requeue_mode = LatticeRefreshMode::Snapshot;
      log_dbg("[iinuji:lattice] refresh_async_requeue_dispatch mode=%s\n",
              lattice_refresh_mode_name(mode));
      queue_lattice_refresh(st, mode);
    }
  }
  if (st.lattice.detail_refresh_pending && st.lattice.detail_future.valid() &&
      st.lattice.detail_future.wait_for(std::chrono::milliseconds(0)) ==
          std::future_status::ready) {
    LatticeMemberDetailSnapshot snapshot = st.lattice.detail_future.get();
    st.lattice.detail_refresh_pending = false;
    dirty = lattice_apply_member_detail_snapshot(st, snapshot) || dirty;
    if (st.lattice.detail_requeue) {
      st.lattice.detail_requeue = false;
      queue_lattice_selected_member_detail_if_needed(st);
    }
    if ((st.lattice.selected_section == LatticeSection::Lattice &&
         st.lattice.mode == LatticeMode::Views) ||
        st.lattice.view_requeue) {
      queue_lattice_selected_view_if_needed(st);
    }
  }
  if (st.lattice.view_refresh_pending && st.lattice.view_future.valid() &&
      st.lattice.view_future.wait_for(std::chrono::milliseconds(0)) ==
          std::future_status::ready) {
    LatticeViewTransportSnapshot snapshot = st.lattice.view_future.get();
    st.lattice.view_refresh_pending = false;
    dirty = lattice_apply_view_transport_snapshot(st, snapshot) || dirty;
    if (st.lattice.view_requeue) {
      st.lattice.view_requeue = false;
      queue_lattice_selected_view_if_needed(st);
    }
  }
  return dirty;
}

template <class AppendLog>
inline void append_lattice_show_lines(const CmdState &st,
                                      AppendLog &&append_log) {
  append_log("screen=lattice", "show", "#d8d8ff");
  append_log("section=" + lattice_section_label(st.lattice.selected_section),
             "show", "#d8d8ff");
  append_log("mode=" + lattice_mode_label(st.lattice.mode), "show", "#d8d8ff");
  append_log("families=" + std::to_string(st.lattice.families.size()) +
                 " projects=" + std::to_string(count_lattice_projects(st)) +
                 " members=" + std::to_string(count_lattice_members(st)),
             "show", "#d8d8ff");
  std::size_t fragments = 0;
  for (const auto &family : st.lattice.families)
    fragments += family.fragment_count;
  append_log("report_fragments=" + std::to_string(fragments), "show",
             "#d8d8ff");
  if (const auto *row = selected_lattice_row_ptr(st); row != nullptr) {
    append_log("selection.kind=" + lattice_row_kind_label(row->kind), "show",
               "#d8d8ff");
  }
  if (const auto *family = selected_lattice_family_ptr(st); family != nullptr) {
    append_log("selection.family=" + family->family, "show", "#d8d8ff");
  }
  if (const auto *project = selected_lattice_project_ptr(st);
      project != nullptr) {
    append_log("selection.project=" + project->project_path, "show", "#d8d8ff");
  }
  if (const auto *member = selected_lattice_member_ptr(st); member != nullptr) {
    append_log("selection.member=" + member->canonical_path, "show", "#d8d8ff");
  }
  if (const auto *view = selected_lattice_view_ptr(st); view != nullptr) {
    append_log("selection.view=" + view->view_kind, "show", "#d8d8ff");
  }
  if (const auto *component = selected_component_member_ptr(st);
      component != nullptr) {
    append_log("selection.component=" + component->canonical_path, "show",
               "#d8d8ff");
  }
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
