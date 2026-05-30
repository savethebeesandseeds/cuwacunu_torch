inline void queue_lattice_selected_member_detail_if_needed(CmdState &st);
inline void queue_lattice_selected_view_if_needed(CmdState &st);

inline bool select_prev_lattice_row(CmdState &st) {
  if (st.lattice.rows.empty() || st.lattice.selected_row == 0)
    return false;
  --st.lattice.selected_row;
  queue_lattice_selected_member_detail_if_needed(st);
  return true;
}

inline bool select_next_lattice_row(CmdState &st) {
  if (st.lattice.rows.empty() ||
      st.lattice.selected_row + 1 >= st.lattice.rows.size()) {
    return false;
  }
  ++st.lattice.selected_row;
  queue_lattice_selected_member_detail_if_needed(st);
  return true;
}

inline bool select_first_lattice_row(CmdState &st) {
  if (st.lattice.rows.empty() || st.lattice.selected_row == 0)
    return false;
  st.lattice.selected_row = 0;
  queue_lattice_selected_member_detail_if_needed(st);
  return true;
}

inline bool select_last_lattice_row(CmdState &st) {
  if (st.lattice.rows.empty())
    return false;
  const std::size_t last = st.lattice.rows.size() - 1;
  if (st.lattice.selected_row == last)
    return false;
  st.lattice.selected_row = last;
  queue_lattice_selected_member_detail_if_needed(st);
  return true;
}

inline bool select_lattice_parent_row(CmdState &st) {
  const auto *row = selected_lattice_row_ptr(st);
  if (!row)
    return false;
  if (row->kind == LatticeRowKind::Member) {
    for (std::size_t i = st.lattice.selected_row; i-- > 0;) {
      const auto &candidate = st.lattice.rows[i];
      if (candidate.kind == LatticeRowKind::Project &&
          candidate.family_index == row->family_index &&
          candidate.project_index == row->project_index) {
        st.lattice.selected_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
    }
    return false;
  }
  if (row->kind == LatticeRowKind::Project) {
    for (std::size_t i = st.lattice.selected_row; i-- > 0;) {
      const auto &candidate = st.lattice.rows[i];
      if (candidate.kind == LatticeRowKind::Family &&
          candidate.family_index == row->family_index) {
        st.lattice.selected_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
    }
  }
  return false;
}

inline bool select_lattice_child_row(CmdState &st) {
  const auto *row = selected_lattice_row_ptr(st);
  if (!row)
    return false;
  if (row->kind == LatticeRowKind::Family) {
    for (std::size_t i = st.lattice.selected_row + 1;
         i < st.lattice.rows.size(); ++i) {
      const auto &candidate = st.lattice.rows[i];
      if (candidate.kind == LatticeRowKind::Project &&
          candidate.family_index == row->family_index) {
        st.lattice.selected_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
      if (candidate.family_index != row->family_index)
        break;
    }
    return false;
  }
  if (row->kind == LatticeRowKind::Project) {
    for (std::size_t i = st.lattice.selected_row + 1;
         i < st.lattice.rows.size(); ++i) {
      const auto &candidate = st.lattice.rows[i];
      if (candidate.kind == LatticeRowKind::Member &&
          candidate.family_index == row->family_index &&
          candidate.project_index == row->project_index) {
        st.lattice.selected_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
      if (candidate.family_index != row->family_index ||
          candidate.project_index != row->project_index) {
        break;
      }
    }
  }
  return false;
}

inline bool select_prev_lattice_component_row(CmdState &st) {
  const auto *row = selected_component_row_ptr(st);
  if (!row)
    return false;
  if (row->kind == LatticeComponentRowKind::Root) {
    for (std::size_t i = st.lattice.selected_component_row; i-- > 0;) {
      if (st.lattice.component_rows[i].kind == LatticeComponentRowKind::Root) {
        st.lattice.selected_component_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
    }
    return false;
  }
  if (row->kind == LatticeComponentRowKind::Category) {
    for (std::size_t i = st.lattice.selected_component_row; i-- > 0;) {
      const auto &candidate = st.lattice.component_rows[i];
      if (candidate.root_index != row->root_index)
        break;
      if (candidate.kind == LatticeComponentRowKind::Category) {
        st.lattice.selected_component_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
    }
    return false;
  }
  for (std::size_t i = st.lattice.selected_component_row; i-- > 0;) {
    const auto &candidate = st.lattice.component_rows[i];
    if (candidate.root_index != row->root_index ||
        candidate.category_index != row->category_index) {
      break;
    }
    if (candidate.kind == LatticeComponentRowKind::Member) {
      st.lattice.selected_component_row = i;
      queue_lattice_selected_member_detail_if_needed(st);
      return true;
    }
  }
  return false;
}

inline bool select_next_lattice_component_row(CmdState &st) {
  const auto *row = selected_component_row_ptr(st);
  if (!row)
    return false;
  if (row->kind == LatticeComponentRowKind::Root) {
    for (std::size_t i = st.lattice.selected_component_row + 1;
         i < st.lattice.component_rows.size(); ++i) {
      if (st.lattice.component_rows[i].kind == LatticeComponentRowKind::Root) {
        st.lattice.selected_component_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
    }
    return false;
  }
  if (row->kind == LatticeComponentRowKind::Category) {
    for (std::size_t i = st.lattice.selected_component_row + 1;
         i < st.lattice.component_rows.size(); ++i) {
      const auto &candidate = st.lattice.component_rows[i];
      if (candidate.root_index != row->root_index)
        break;
      if (candidate.kind == LatticeComponentRowKind::Category) {
        st.lattice.selected_component_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
    }
    return false;
  }
  for (std::size_t i = st.lattice.selected_component_row + 1;
       i < st.lattice.component_rows.size(); ++i) {
    const auto &candidate = st.lattice.component_rows[i];
    if (candidate.root_index != row->root_index ||
        candidate.category_index != row->category_index) {
      break;
    }
    if (candidate.kind == LatticeComponentRowKind::Member) {
      st.lattice.selected_component_row = i;
      queue_lattice_selected_member_detail_if_needed(st);
      return true;
    }
  }
  return false;
}

inline bool select_first_lattice_component_row(CmdState &st) {
  const auto *row = selected_component_row_ptr(st);
  if (!row)
    return false;
  if (row->kind == LatticeComponentRowKind::Root) {
    for (std::size_t i = 0; i < st.lattice.component_rows.size(); ++i) {
      if (st.lattice.component_rows[i].kind == LatticeComponentRowKind::Root) {
        if (st.lattice.selected_component_row == i)
          return false;
        st.lattice.selected_component_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
    }
    return false;
  }
  if (row->kind == LatticeComponentRowKind::Category) {
    for (std::size_t i = 0; i < st.lattice.component_rows.size(); ++i) {
      const auto &candidate = st.lattice.component_rows[i];
      if (candidate.root_index != row->root_index)
        continue;
      if (candidate.kind == LatticeComponentRowKind::Category) {
        if (st.lattice.selected_component_row == i)
          return false;
        st.lattice.selected_component_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
    }
    return false;
  }
  for (std::size_t i = 0; i < st.lattice.component_rows.size(); ++i) {
    const auto &candidate = st.lattice.component_rows[i];
    if (candidate.root_index != row->root_index ||
        candidate.category_index != row->category_index) {
      continue;
    }
    if (candidate.kind == LatticeComponentRowKind::Member) {
      if (st.lattice.selected_component_row == i)
        return false;
      st.lattice.selected_component_row = i;
      queue_lattice_selected_member_detail_if_needed(st);
      return true;
    }
  }
  return false;
}

inline bool select_last_lattice_component_row(CmdState &st) {
  const auto *row = selected_component_row_ptr(st);
  if (!row)
    return false;
  if (row->kind == LatticeComponentRowKind::Root) {
    for (std::size_t i = st.lattice.component_rows.size(); i-- > 0;) {
      if (st.lattice.component_rows[i].kind == LatticeComponentRowKind::Root) {
        if (st.lattice.selected_component_row == i)
          return false;
        st.lattice.selected_component_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
    }
    return false;
  }
  if (row->kind == LatticeComponentRowKind::Category) {
    for (std::size_t i = st.lattice.component_rows.size(); i-- > 0;) {
      const auto &candidate = st.lattice.component_rows[i];
      if (candidate.root_index != row->root_index)
        continue;
      if (candidate.kind == LatticeComponentRowKind::Category) {
        if (st.lattice.selected_component_row == i)
          return false;
        st.lattice.selected_component_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
    }
    return false;
  }
  for (std::size_t i = st.lattice.component_rows.size(); i-- > 0;) {
    const auto &candidate = st.lattice.component_rows[i];
    if (candidate.root_index != row->root_index ||
        candidate.category_index != row->category_index) {
      continue;
    }
    if (candidate.kind == LatticeComponentRowKind::Member) {
      if (st.lattice.selected_component_row == i)
        return false;
      st.lattice.selected_component_row = i;
      queue_lattice_selected_member_detail_if_needed(st);
      return true;
    }
  }
  return false;
}

inline bool select_lattice_component_parent_row(CmdState &st) {
  const auto *row = selected_component_row_ptr(st);
  if (!row)
    return false;
  if (row->kind == LatticeComponentRowKind::Member) {
    for (std::size_t i = st.lattice.selected_component_row; i-- > 0;) {
      const auto &candidate = st.lattice.component_rows[i];
      if (candidate.kind == LatticeComponentRowKind::Category &&
          candidate.root_index == row->root_index &&
          candidate.category_index == row->category_index) {
        st.lattice.selected_component_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
    }
    return false;
  }
  if (row->kind == LatticeComponentRowKind::Category) {
    for (std::size_t i = st.lattice.selected_component_row; i-- > 0;) {
      const auto &candidate = st.lattice.component_rows[i];
      if (candidate.kind == LatticeComponentRowKind::Root &&
          candidate.root_index == row->root_index) {
        st.lattice.selected_component_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
    }
  }
  return false;
}

inline bool select_lattice_component_child_row(CmdState &st) {
  const auto *row = selected_component_row_ptr(st);
  if (!row)
    return false;
  if (row->kind == LatticeComponentRowKind::Root) {
    for (std::size_t i = st.lattice.selected_component_row + 1;
         i < st.lattice.component_rows.size(); ++i) {
      const auto &candidate = st.lattice.component_rows[i];
      if (candidate.kind == LatticeComponentRowKind::Category &&
          candidate.root_index == row->root_index) {
        st.lattice.selected_component_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
      if (candidate.root_index != row->root_index)
        break;
    }
    return false;
  }
  if (row->kind == LatticeComponentRowKind::Category) {
    for (std::size_t i = st.lattice.selected_component_row + 1;
         i < st.lattice.component_rows.size(); ++i) {
      const auto &candidate = st.lattice.component_rows[i];
      if (candidate.kind == LatticeComponentRowKind::Member &&
          candidate.root_index == row->root_index &&
          candidate.category_index == row->category_index) {
        st.lattice.selected_component_row = i;
        queue_lattice_selected_member_detail_if_needed(st);
        return true;
      }
      if (candidate.root_index != row->root_index ||
          candidate.category_index != row->category_index) {
        break;
      }
    }
  }
  return false;
}

inline bool select_prev_lattice_mode_root(CmdState &st) {
  if (st.lattice.selected_mode_row == 0)
    return false;
  --st.lattice.selected_mode_row;
  return true;
}

inline bool select_next_lattice_mode_root(CmdState &st) {
  if (st.lattice.selected_mode_row >= 1)
    return false;
  ++st.lattice.selected_mode_row;
  return true;
}

inline bool lattice_enter_selected_mode(CmdState &st) {
  if (st.lattice.selected_section != LatticeSection::Lattice ||
      st.lattice.mode != LatticeMode::Roots) {
    return false;
  }
  st.lattice.mode = selected_lattice_root_mode(st);
  if (st.lattice.mode == LatticeMode::Facts) {
    queue_lattice_selected_member_detail_if_needed(st);
  } else if (st.lattice.mode == LatticeMode::Views) {
    queue_lattice_selected_view_if_needed(st);
  }
  return true;
}

inline bool lattice_exit_mode_to_roots(CmdState &st) {
  if (st.lattice.selected_section != LatticeSection::Lattice ||
      st.lattice.mode == LatticeMode::Roots) {
    return false;
  }
  st.lattice.selected_mode_row = st.lattice.mode == LatticeMode::Views ? 1 : 0;
  st.lattice.mode = LatticeMode::Roots;
  st.lattice.view_requeue = false;
  return true;
}

inline std::size_t
lattice_count_active_members(const std::vector<LatticeMemberEntry> &members) {
  return static_cast<std::size_t>(
      std::count_if(members.begin(), members.end(), [](const auto &member) {
        return lattice_member_lineage_state_text(member) == "active";
      }));
}

struct lattice_refresh_selection_key_t {
  LatticeRowKind kind{LatticeRowKind::Family};
  std::string family{};
  std::string project{};
  std::string canonical_path{};
};

inline lattice_refresh_selection_key_t
capture_lattice_selection_key(const CmdState &st) {
  lattice_refresh_selection_key_t key{};
  if (const auto *row = selected_lattice_row_ptr(st); row != nullptr) {
    key.kind = row->kind;
    if (const auto *family = selected_lattice_family_ptr(st);
        family != nullptr) {
      key.family = family->family;
    }
    if (const auto *project = selected_lattice_project_ptr(st);
        project != nullptr) {
      key.project = project->project_path;
    }
    if (const auto *member = selected_lattice_member_ptr(st);
        member != nullptr) {
      key.canonical_path = member->canonical_path;
    }
  }
  return key;
}

inline bool lattice_has_cached_snapshot(const CmdState &st) {
  return !st.lattice.families.empty() || !st.lattice.component_rows.empty() ||
         !st.lattice.views.empty();
}

inline std::string lattice_loading_status_text(LatticeRefreshMode mode) {
  if (mode == LatticeRefreshMode::SyncStore) {
    return "Refreshing hero.lattice and hero.hashimyei catalogs; full reingest "
           "can take about a minute...";
  }
  return "Loading lattice components, facts, and views...";
}

inline const char *lattice_refresh_mode_name(LatticeRefreshMode mode) {
  return mode == LatticeRefreshMode::SyncStore ? "sync_store" : "snapshot";
}

inline bool lattice_is_refresh_pending(const CmdState &st) {
  return st.lattice.refresh_pending;
}

inline bool lattice_is_visibly_loading(const CmdState &st) {
  return st.lattice.refresh_pending && !st.lattice.refresh_timed_out;
}

inline void queue_lattice_refresh(CmdState &st, LatticeRefreshMode mode,
                                  std::string status = {});

inline bool lattice_handle_refresh_failure(CmdState &st,
                                           std::string_view error) {
  if (lattice_has_cached_snapshot(st)) {
    set_lattice_status(
        st,
        "Lattice refresh skipped; showing cached lineage snapshot: " +
            std::string(error),
        false);
    return true;
  }
  st.lattice.families.clear();
  st.lattice.component_roots.clear();
  st.lattice.rows.clear();
  st.lattice.component_rows.clear();
  st.lattice.selected_row = 0;
  st.lattice.selected_component_row = 0;
  set_lattice_status(st, std::string(error), true);
  return false;
}

inline void lattice_ensure_member_identity(LatticeMemberEntry *member);
inline bool poll_lattice_async_updates(CmdState &st);
inline void queue_lattice_selected_view_if_needed(CmdState &st);

inline void queue_lattice_selected_member_detail_if_needed(CmdState &st) {
  if (st.lattice.selected_section == LatticeSection::Overview ||
      (st.lattice.selected_section == LatticeSection::Lattice &&
       st.lattice.mode != LatticeMode::Facts)) {
    st.lattice.detail_requeue = false;
    return;
  }
  const auto *member = selected_lattice_detail_member_ptr(st);
  if (member == nullptr || member->detail_loaded) {
    st.lattice.detail_requeue = false;
    return;
  }
  if (st.lattice.detail_refresh_pending) {
    st.lattice.detail_requeue = true;
    return;
  }
  st.lattice.detail_refresh_pending = true;
  st.lattice.detail_requeue = false;
  const auto lattice_defaults = st.lattice.lattice_defaults;
  const auto hashimyei_defaults = st.lattice.hashimyei_defaults;
  const auto member_copy = *member;
  st.lattice.detail_future = std::async(
      std::launch::async,
      [lattice_defaults, hashimyei_defaults,
       member_copy]() mutable -> LatticeMemberDetailSnapshot {
        LatticeMemberDetailSnapshot snapshot{};
        snapshot.member = member_copy;
        cuwacunu::hero::lattice_mcp::app_context_t lattice_app{};
        lattice_app.store_root = lattice_defaults.store_root;
        lattice_app.lattice_catalog_path = lattice_defaults.catalog_path;
        lattice_app.hashimyei_catalog_path = hashimyei_defaults.catalog_path;
        lattice_app.close_catalog_after_execute = true;

        cuwacunu::hero::hashimyei_mcp::app_context_t hashimyei_app{};
        hashimyei_app.store_root = hashimyei_defaults.store_root;
        hashimyei_app.lattice_catalog_path = lattice_defaults.catalog_path;
        hashimyei_app.catalog_options.catalog_path =
            hashimyei_defaults.catalog_path;
        hashimyei_app.catalog_options.encrypted = false;
        hashimyei_app.close_catalog_after_execute = true;

        bool component_ok = false;
        bool facts_ok = false;
        bool fact_bundle_ok = false;
        std::string first_error{};

        if (!member_copy.canonical_path.empty()) {
          std::string component_result_json{};
          std::string component_error{};
          const std::string component_args =
              std::string("{\"canonical_path\":") +
              config_json_quote(member_copy.canonical_path) + "}";
          if (cuwacunu::hero::hashimyei_mcp::execute_tool_json(
                  "hero.hashimyei.get_component_manifest", component_args,
                  &hashimyei_app, &component_result_json, &component_error)) {
            lattice_json_value_t structured{};
            if (lattice_parse_tool_structured_content(
                    component_result_json, &structured, &component_error)) {
              snapshot.member.has_component = true;
              snapshot.member.component_count =
                  std::max<std::size_t>(snapshot.member.component_count, 1);
              snapshot.member.hashimyei = lattice_json_string(
                  lattice_json_field(&structured, "hashimyei"),
                  snapshot.member.hashimyei);
              snapshot.member.canonical_path = lattice_json_string(
                  lattice_json_field(&structured, "canonical_path"),
                  snapshot.member.canonical_path);
              if (const auto *component =
                      lattice_json_field(&structured, "component");
                  component != nullptr) {
                snapshot.member.component.component_id = lattice_json_string(
                    lattice_json_field(component, "component_id"));
                snapshot.member.component.ts_ms =
                    lattice_json_u64(lattice_json_field(component, "ts_ms"));
                snapshot.member.component.manifest_path = lattice_json_string(
                    lattice_json_field(component, "manifest_path"));
                snapshot.member.component.family_rank =
                    lattice_json_optional_u64(
                        lattice_json_field(component, "family_rank"));
                if (const auto *manifest =
                        lattice_json_field(component, "manifest");
                    manifest != nullptr) {
                  snapshot.member.family = lattice_json_string(
                      lattice_json_field(manifest, "family"),
                      snapshot.member.family);
                  snapshot.member.lineage_state = lattice_json_string(
                      lattice_json_field(manifest, "lineage_state"),
                      snapshot.member.lineage_state);
                  snapshot.member.component.manifest.canonical_path =
                      lattice_json_string(
                          lattice_json_field(manifest, "canonical_path"));
                  snapshot.member.component.manifest.family =
                      lattice_json_string(
                          lattice_json_field(manifest, "family"));
                  snapshot.member.component.manifest.lineage_state =
                      lattice_json_string(
                          lattice_json_field(manifest, "lineage_state"));
                  snapshot.member.component.manifest.replaced_by =
                      lattice_json_string(
                          lattice_json_field(manifest, "replaced_by"));
                  snapshot.member.component.manifest
                      .docking_signature_sha256_hex =
                      lattice_json_string(lattice_json_field(
                          manifest, "docking_signature_sha256_hex"));
                  snapshot.member.component.manifest
                      .component_compatibility_sha256_hex =
                      lattice_json_string(lattice_json_field(
                          manifest, "component_compatibility_sha256_hex"));
                  snapshot.member.component_compatibility_sha256_hex =
                      lattice_json_string(
                          lattice_json_field(
                              manifest, "component_compatibility_sha256_hex"),
                          snapshot.member.component_compatibility_sha256_hex);
                  if (const auto *hashimyei_identity =
                          lattice_json_field(manifest, "hashimyei_identity");
                      hashimyei_identity != nullptr) {
                    snapshot.member.component.manifest.hashimyei_identity.name =
                        lattice_json_string(
                            lattice_json_field(hashimyei_identity, "name"));
                  }
                  if (const auto *contract_identity =
                          lattice_json_field(manifest, "contract_identity");
                      contract_identity != nullptr) {
                    snapshot.member.component.manifest.contract_identity.name =
                        lattice_json_string(
                            lattice_json_field(contract_identity, "name"));
                    snapshot.member.contract_hash =
                        snapshot.member.component.manifest.contract_identity
                            .name;
                  }
                  if (const auto *parent_identity =
                          lattice_json_field(manifest, "parent_identity");
                      parent_identity != nullptr &&
                      parent_identity->type == lattice_json_type_t::OBJECT) {
                    snapshot.member.component.manifest.parent_identity =
                        cuwacunu::hashimyei::hashimyei_t{};
                    snapshot.member.component.manifest.parent_identity->name =
                        lattice_json_string(
                            lattice_json_field(parent_identity, "name"));
                  }
                }
              }
              component_ok = true;
            }
          }
          if (!component_ok && first_error.empty() &&
              !component_error.empty()) {
            first_error = component_error;
          }

          lattice_json_value_t facts_structured{};
          std::string facts_error{};
          const std::string facts_args =
              std::string("{\"canonical_path\":") +
              config_json_quote(member_copy.canonical_path) + "}";
          if (lattice_invoke_tool(&lattice_app, "hero.lattice.list_facts",
                                  facts_args, &facts_structured,
                                  &facts_error)) {
            if (const auto *facts =
                    lattice_json_field(&facts_structured, "facts");
                facts != nullptr && facts->type == lattice_json_type_t::ARRAY &&
                facts->arrayValue) {
              snapshot.member.fact.wave_cursors.clear();
              snapshot.member.fact.binding_ids.clear();
              snapshot.member.fact.semantic_taxa.clear();
              snapshot.member.fact.source_runtime_cursors.clear();
              snapshot.member.fact.available_context_count =
                  facts->arrayValue->size();
              std::uint64_t latest_ts_ms = snapshot.member.fact.latest_ts_ms;
              std::string latest_wave_cursor =
                  snapshot.member.fact.latest_wave_cursor;
              std::size_t latest_fragment_count =
                  snapshot.member.fact.fragment_count;
              for (const auto &entry : *facts->arrayValue) {
                const std::string wave_cursor = lattice_json_string(
                    lattice_json_field(&entry, "wave_cursor"));
                const std::uint64_t latest_ts = lattice_json_u64(
                    lattice_json_field(&entry, "latest_ts_ms"));
                lattice_append_unique_value(&snapshot.member.fact.wave_cursors,
                                            wave_cursor);
                lattice_merge_string_values(
                    &snapshot.member.fact.binding_ids,
                    lattice_json_string_array(
                        lattice_json_field(&entry, "binding_ids")));
                lattice_merge_string_values(
                    &snapshot.member.fact.semantic_taxa,
                    lattice_json_string_array(
                        lattice_json_field(&entry, "semantic_taxa")));
                lattice_merge_string_values(
                    &snapshot.member.fact.source_runtime_cursors,
                    lattice_json_string_array(
                        lattice_json_field(&entry, "source_runtime_cursors")));
                if (latest_ts >= latest_ts_ms) {
                  latest_ts_ms = latest_ts;
                  latest_wave_cursor = wave_cursor;
                  latest_fragment_count = lattice_json_u64(
                      lattice_json_field(&entry, "fragment_count"),
                      latest_fragment_count);
                }
              }
              snapshot.member.has_fact =
                  snapshot.member.has_fact || !facts->arrayValue->empty();
              snapshot.member.fact.latest_ts_ms = latest_ts_ms;
              snapshot.member.fact.latest_wave_cursor = latest_wave_cursor;
              snapshot.member.fact.fragment_count = latest_fragment_count;
              snapshot.member.report_fragment_count = std::max(
                  snapshot.member.report_fragment_count, latest_fragment_count);
              facts_ok = true;
            }
          }
          if (!facts_ok && first_error.empty() && !facts_error.empty()) {
            first_error = facts_error;
          }

          if (snapshot.member.has_fact) {
            lattice_json_value_t fact_structured{};
            std::string fact_error{};
            if (lattice_invoke_tool(&lattice_app, "hero.lattice.get_fact",
                                    facts_args, &fact_structured,
                                    &fact_error)) {
              snapshot.member.latest_fact_lls = lattice_json_string(
                  lattice_json_field(&fact_structured, "fact_lls"));
              snapshot.member.fact.latest_wave_cursor = lattice_json_string(
                  lattice_json_field(&fact_structured, "wave_cursor"),
                  snapshot.member.fact.latest_wave_cursor);
              snapshot.member.fact.latest_ts_ms = lattice_json_u64(
                  lattice_json_field(&fact_structured, "latest_ts_ms"),
                  snapshot.member.fact.latest_ts_ms);
              snapshot.member.fact.fragment_count = lattice_json_u64(
                  lattice_json_field(&fact_structured, "fragment_count"),
                  snapshot.member.fact.fragment_count);
              lattice_merge_string_values(
                  &snapshot.member.fact.binding_ids,
                  lattice_json_string_array(
                      lattice_json_field(&fact_structured, "binding_ids")));
              lattice_merge_string_values(
                  &snapshot.member.fact.semantic_taxa,
                  lattice_json_string_array(
                      lattice_json_field(&fact_structured, "semantic_taxa")));
              lattice_merge_string_values(
                  &snapshot.member.fact.source_runtime_cursors,
                  lattice_json_string_array(lattice_json_field(
                      &fact_structured, "source_runtime_cursors")));
              fact_bundle_ok = true;
            }
            if (!fact_bundle_ok && first_error.empty() && !fact_error.empty()) {
              first_error = fact_error;
            }
          }
        }

        lattice_ensure_member_identity(&snapshot.member);
        snapshot.member.detail_loaded = true;
        snapshot.ok = component_ok || facts_ok || fact_bundle_ok ||
                      (!snapshot.member.canonical_path.empty());
        if (!snapshot.ok && !first_error.empty()) {
          snapshot.error = std::move(first_error);
        }
        return snapshot;
      });
}

inline LatticeMemberEntry *
lattice_find_member_by_canonical_path_mutable(CmdState &st,
                                              std::string_view canonical_path) {
  for (auto &family : st.lattice.families) {
    for (auto &project : family.projects) {
      for (auto &member : project.members) {
        if (member.canonical_path == canonical_path)
          return &member;
      }
    }
  }
  return nullptr;
}

inline bool lattice_apply_member_detail_snapshot(
    CmdState &st, const LatticeMemberDetailSnapshot &snapshot) {
  if (!snapshot.ok) {
    set_lattice_status(
        st,
        "Lattice member detail unavailable; showing summary view: " +
            snapshot.error,
        false);
    return false;
  }
  auto *member = lattice_find_member_by_canonical_path_mutable(
      st, snapshot.member.canonical_path);
  if (member == nullptr)
    return false;
  *member = snapshot.member;
  return true;
}

inline bool lattice_apply_view_transport_snapshot(
    CmdState &st, const LatticeViewTransportSnapshot &snapshot) {
  st.lattice.view_snapshot = snapshot;
  return true;
}

inline void queue_lattice_selected_view_if_needed(CmdState &st) {
  if (st.lattice.selected_section != LatticeSection::Lattice ||
      st.lattice.mode != LatticeMode::Views) {
    st.lattice.view_requeue = false;
    return;
  }
  const auto *view = selected_lattice_view_ptr(st);
  const auto *member = selected_lattice_member_ptr(st);
  if (view == nullptr) {
    st.lattice.view_requeue = false;
    st.lattice.view_snapshot = LatticeViewTransportSnapshot{};
    return;
  }
  if (member == nullptr) {
    st.lattice.view_requeue = false;
    st.lattice.view_snapshot = LatticeViewTransportSnapshot{
        .ok = false,
        .error =
            "Select a fact member in Lattice/Facts to materialize this view.",
        .view_kind = view->view_kind,
    };
    return;
  }
  if (!member->detail_loaded) {
    queue_lattice_selected_member_detail_if_needed(st);
    if (st.lattice.detail_refresh_pending) {
      st.lattice.view_requeue = true;
      return;
    }
  }
  if (st.lattice.view_refresh_pending) {
    st.lattice.view_requeue = true;
    return;
  }

  const std::string canonical_path = member->canonical_path;
  const std::string contract_hash = member->contract_hash;
  const std::string component_compatibility_sha256_hex =
      lattice_component_compatibility_sha256_hex_for_member(*member);
  const std::string wave_cursor = member->fact.latest_wave_cursor;
  const bool contract_scoped_view =
      view->view_kind != "family_evaluation_report";
  if (st.lattice.view_snapshot.view_kind == view->view_kind &&
      st.lattice.view_snapshot.canonical_path == canonical_path &&
      (!contract_scoped_view ||
       st.lattice.view_snapshot.contract_hash == contract_hash) &&
      st.lattice.view_snapshot.component_compatibility_sha256_hex ==
          component_compatibility_sha256_hex &&
      st.lattice.view_snapshot.wave_cursor == wave_cursor &&
      (st.lattice.view_snapshot.ok ||
       !st.lattice.view_snapshot.error.empty())) {
    st.lattice.view_requeue = false;
    return;
  }

  st.lattice.view_refresh_pending = true;
  st.lattice.view_requeue = false;
  const auto lattice_defaults = st.lattice.lattice_defaults;
  const auto hashimyei_defaults = st.lattice.hashimyei_defaults;
  const auto view_copy = *view;
  const auto member_copy = *member;
  st.lattice.view_future = std::async(
      std::launch::async,
      [lattice_defaults, hashimyei_defaults, view_copy,
       member_copy]() mutable -> LatticeViewTransportSnapshot {
        LatticeViewTransportSnapshot snapshot{};
        snapshot.view_kind = view_copy.view_kind;
        snapshot.canonical_path = member_copy.canonical_path;
        snapshot.contract_hash = member_copy.contract_hash;
        snapshot.component_compatibility_sha256_hex =
            lattice_component_compatibility_sha256_hex_for_member(member_copy);
        snapshot.wave_cursor = member_copy.fact.latest_wave_cursor;

        if (view_copy.view_kind == "family_evaluation_report") {
          if (snapshot.canonical_path.empty() ||
              snapshot.component_compatibility_sha256_hex.empty()) {
            snapshot.error = "family_evaluation_report needs canonical_path + "
                             "component_compatibility_sha256_hex";
            return snapshot;
          }
        } else if (view_copy.view_kind == "entropic_capacity_comparison") {
          if (snapshot.wave_cursor.empty()) {
            snapshot.error = "entropic_capacity_comparison needs wave_cursor";
            return snapshot;
          }
        }

        cuwacunu::hero::lattice_mcp::app_context_t lattice_app{};
        lattice_app.store_root = lattice_defaults.store_root;
        lattice_app.lattice_catalog_path = lattice_defaults.catalog_path;
        lattice_app.hashimyei_catalog_path = hashimyei_defaults.catalog_path;
        lattice_app.close_catalog_after_execute = true;

        std::ostringstream args;
        args << "{\"view_kind\":" << config_json_quote(view_copy.view_kind);
        if (!snapshot.canonical_path.empty()) {
          args << ",\"canonical_path\":"
               << config_json_quote(snapshot.canonical_path);
        }
        if (view_copy.view_kind == "family_evaluation_report" &&
            !snapshot.component_compatibility_sha256_hex.empty()) {
          args << ",\"component_compatibility_sha256_hex\":"
               << config_json_quote(
                      snapshot.component_compatibility_sha256_hex);
        } else if (!snapshot.contract_hash.empty()) {
          args << ",\"contract_hash\":"
               << config_json_quote(snapshot.contract_hash);
        }
        if (!snapshot.wave_cursor.empty()) {
          args << ",\"wave_cursor\":"
               << config_json_quote(snapshot.wave_cursor);
        }
        args << "}";

        lattice_json_value_t structured{};
        std::string error{};
        if (!lattice_invoke_tool(&lattice_app, "hero.lattice.get_view",
                                 args.str(), &structured, &error)) {
          snapshot.error = std::move(error);
          return snapshot;
        }
        snapshot.canonical_path = lattice_json_string(
            lattice_json_field(&structured, "canonical_path"),
            snapshot.canonical_path);
        snapshot.contract_hash = lattice_json_string(
            lattice_json_field(&structured, "contract_hash"),
            snapshot.contract_hash);
        snapshot.component_compatibility_sha256_hex = lattice_json_string(
            lattice_json_field(&structured,
                               "component_compatibility_sha256_hex"),
            snapshot.component_compatibility_sha256_hex);
        snapshot.wave_cursor =
            lattice_json_string(lattice_json_field(&structured, "wave_cursor"),
                                snapshot.wave_cursor);
        snapshot.match_count =
            lattice_json_u64(lattice_json_field(&structured, "match_count"));
        snapshot.ambiguity_count = lattice_json_u64(
            lattice_json_field(&structured, "ambiguity_count"));
        snapshot.view_lls =
            lattice_json_string(lattice_json_field(&structured, "view_lls"));
        snapshot.ok = true;
        return snapshot;
      });
}
