bool lattice_catalog_store_t::rebuild_indexes(std::string *error) {
  clear_error(error);
  if (!db_) {
    set_error(error, "catalog is not open");
    return false;
  }

  cells_by_id_.clear();
  cell_id_by_coord_profile_.clear();
  trials_by_cell_.clear();
  report_by_trial_id_.clear();
  projection_num_by_cell_.clear();
  projection_txt_by_cell_.clear();
  runtime_runs_by_id_.clear();
  runtime_run_ids_.clear();
  runtime_report_fragments_by_id_.clear();
  runtime_latest_report_fragment_by_key_.clear();
  runtime_report_fragment_ids_by_canonical_.clear();
  runtime_fact_summaries_by_canonical_.clear();
  runtime_dependency_files_by_run_id_.clear();
  runtime_report_fragment_ids_by_wave_cursor_.clear();
  runtime_ledger_.clear();
  runtime_ledger_path_by_id_.clear();
  runtime_components_by_id_.clear();
  explicit_family_rank_by_scope_.clear();

  std::unordered_map<std::string, std::uint64_t> family_rank_row_ts_by_scope{};
  std::unordered_map<std::string, std::string> family_rank_row_id_by_scope{};

  db::query::query_spec_t q{};
  q.select_columns = {kColRecordKind};
  std::vector<idydb_column_row_sizing> rows{};
  if (!db::query::select_rows(&db_, q, &rows, error))
    return false;
  next_row_hint_ = rows.empty() ? 1 : (rows.back() + 1);

  for (const auto row : rows) {
    const std::string kind = as_text_or_empty(&db_, kColRecordKind, row);
    if (kind == cuwacunu::hero::schema::kRecordKindCELL) {
      wave_cell_t cell{};
      cell.cell_id = as_text_or_empty(&db_, kColCellId, row);
      if (cell.cell_id.empty()) {
        cell.cell_id = as_text_or_empty(&db_, kColRecordId, row);
      }
      cell.coord.contract_hash = as_text_or_empty(&db_, kColContractHash, row);
      cell.coord.wave_hash = as_text_or_empty(&db_, kColWaveHash, row);
      const std::string profile_id = as_text_or_empty(&db_, kColProfileId, row);
      const std::string profile_json =
          as_text_or_empty(&db_, kColExecutionProfileJson, row);
      cell.execution_profile = profile_from_json(profile_json);
      cell.state = as_text_or_empty(&db_, kColStateTxt, row);

      if (const auto n = as_num_or_empty(&db_, kColMetricNum, row);
          n.has_value() && n.value() >= 0.0) {
        cell.trial_count = static_cast<std::size_t>(n.value());
      }
      cell.report.report_schema = as_text_or_empty(&db_, kColTextA, row);
      cell.report.report_sha256 = as_text_or_empty(&db_, kColTextB, row);

      if (const std::string pv =
              as_text_or_empty(&db_, kColProjectionVersion, row);
          !pv.empty()) {
        std::uint64_t parsed = 0;
        if (parse_u64(pv, &parsed)) {
          cell.projection_version =
              static_cast<std::uint32_t>(std::min<std::uint64_t>(
                  parsed, std::numeric_limits<std::uint32_t>::max()));
        }
      }

      if (const std::string ts = as_text_or_empty(&db_, kColTsMs, row);
          !ts.empty()) {
        std::uint64_t parsed = 0;
        if (parse_u64(ts, &parsed)) {
          cell.updated_at_ms = parsed;
          if (cell.created_at_ms == 0)
            cell.created_at_ms = parsed;
        }
      }

      const std::string payload = as_text_or_empty(&db_, kColPayload, row);
      if (!payload.empty()) {
        lattice_cell_report_t decoded{};
        std::string ignored;
        if (decode_cell_report_payload(payload, &decoded, &ignored)) {
          cell.report = std::move(decoded);
        }
      }

      const std::string map_cell_id = cell.cell_id;
      const std::string map_contract_hash = cell.coord.contract_hash;
      const std::string map_wave_hash = cell.coord.wave_hash;

      if (const auto it = cells_by_id_.find(cell.cell_id);
          it != cells_by_id_.end()) {
        wave_cell_t merged = cell;
        merged.created_at_ms =
            (it->second.created_at_ms == 0)
                ? cell.created_at_ms
                : std::min(it->second.created_at_ms, cell.created_at_ms);
        if (merged.updated_at_ms < it->second.updated_at_ms) {
          merged.updated_at_ms = it->second.updated_at_ms;
          merged.last_trial_id = it->second.last_trial_id;
        }
        cells_by_id_[cell.cell_id] = std::move(merged);
      } else {
        cells_by_id_[cell.cell_id] = std::move(cell);
      }

      if (!map_contract_hash.empty() && !map_wave_hash.empty() &&
          !profile_id.empty()) {
        const std::string key =
            coord_profile_key_(map_contract_hash, map_wave_hash, profile_id);
        cell_id_by_coord_profile_[key] = map_cell_id;
      }
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindTRIAL) {
      wave_trial_t trial{};
      trial.trial_id = as_text_or_empty(&db_, kColRecordId, row);
      trial.cell_id = as_text_or_empty(&db_, kColCellId, row);
      const std::string started = as_text_or_empty(&db_, kColStartedAtMs, row);
      const std::string finished =
          as_text_or_empty(&db_, kColFinishedAtMs, row);
      (void)parse_u64(started, &trial.started_at_ms);
      (void)parse_u64(finished, &trial.finished_at_ms);
      trial.ok = as_text_or_empty(&db_, kColOkTxt, row) == "1";
      trial.error = as_text_or_empty(&db_, kColTextA, row);
      trial.state_snapshot_id = as_text_or_empty(&db_, kColTextB, row);
      trial.campaign_hash = as_text_or_empty(&db_, kColCampaignHash, row);
      trial.run_id = as_text_or_empty(&db_, kColRunId, row);
      (void)parse_u64(as_text_or_empty(&db_, kColTotalSteps, row),
                      &trial.total_steps);
      const std::string payload = as_text_or_empty(&db_, kColPayload, row);
      if (!trial.trial_id.empty() && !payload.empty()) {
        lattice_cell_report_t decoded{};
        std::string ignored{};
        if (decode_cell_report_payload(payload, &decoded, &ignored)) {
          report_by_trial_id_[trial.trial_id] = std::move(decoded);
        }
      }
      if (!trial.cell_id.empty()) {
        trials_by_cell_[trial.cell_id].push_back(std::move(trial));
      }
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindPROJECTION_NUM) {
      const std::string cell_id = as_text_or_empty(&db_, kColCellId, row);
      const std::string key = as_text_or_empty(&db_, kColProjectionKey, row);
      const auto value = as_num_or_empty(&db_, kColProjectionNum, row);
      if (!cell_id.empty() && !key.empty() && value.has_value()) {
        projection_num_by_cell_[cell_id][key] = value.value();
      }
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindPROJECTION_TXT) {
      const std::string cell_id = as_text_or_empty(&db_, kColCellId, row);
      const std::string key = as_text_or_empty(&db_, kColProjectionKey, row);
      const std::string value = as_text_or_empty(&db_, kColProjectionTxt, row);
      if (!cell_id.empty() && !key.empty()) {
        projection_txt_by_cell_[cell_id][key] = value;
      }
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindFAMILY_RANK) {
      const std::string payload = as_text_or_empty(&db_, kColPayload, row);
      if (!payload.empty()) {
        std::unordered_map<std::string, std::string> kv{};
        runtime_lls_document_t ignored_document{};
        std::string parse_error{};
        if (parse_runtime_lls_payload(payload, &kv, &ignored_document,
                                      &parse_error)) {
          cuwacunu::hero::family_rank::state_t rank_state{};
          if (cuwacunu::hero::family_rank::parse_state_from_kv(kv, &rank_state,
                                                               &parse_error)) {
            std::uint64_t row_ts = rank_state.updated_at_ms;
            if (row_ts == 0) {
              (void)parse_u64(as_text_or_empty(&db_, kColTsMs, row), &row_ts);
            }
            const std::string scope_key =
                cuwacunu::hero::family_rank::scope_key(
                    rank_state.family,
                    rank_state.component_compatibility_sha256_hex);
            const std::string row_id =
                as_text_or_empty(&db_, kColRecordId, row);
            const auto current_ts_it =
                family_rank_row_ts_by_scope.find(scope_key);
            const bool should_replace =
                current_ts_it == family_rank_row_ts_by_scope.end() ||
                row_ts > current_ts_it->second ||
                (row_ts == current_ts_it->second &&
                 row_id > family_rank_row_id_by_scope[scope_key]);
            if (should_replace) {
              family_rank_row_ts_by_scope[scope_key] = row_ts;
              family_rank_row_id_by_scope[scope_key] = row_id;
              explicit_family_rank_by_scope_[scope_key] = std::move(rank_state);
            }
          }
        }
      }
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindRUNTIME_LEDGER) {
      const std::string report_fragment_id =
          as_text_or_empty(&db_, kColRecordId, row);
      if (!report_fragment_id.empty()) {
        runtime_ledger_.insert(report_fragment_id);
        runtime_ledger_path_by_id_[report_fragment_id] =
            as_text_or_empty(&db_, kColTextA, row);
        if (const auto it_fragment =
                runtime_report_fragments_by_id_.find(report_fragment_id);
            it_fragment != runtime_report_fragments_by_id_.end() &&
            it_fragment->second.path.empty()) {
          it_fragment->second.path =
              runtime_ledger_path_by_id_[report_fragment_id];
        }
      }
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindRUNTIME_RUN) {
      const std::string run_id = as_text_or_empty(&db_, kColRecordId, row);
      if (!run_id.empty())
        runtime_run_ids_.insert(run_id);
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindRUNTIME_COMPONENT) {
      cuwacunu::hero::hashimyei::component_state_t component{};
      component.component_id = as_text_or_empty(&db_, kColRecordId, row);
      component.manifest_path = as_text_or_empty(&db_, kColTextB, row);
      component.report_fragment_sha256 =
          as_text_or_empty(&db_, kColCampaignHash, row);
      (void)parse_u64(as_text_or_empty(&db_, kColTsMs, row), &component.ts_ms);

      const std::string payload = as_text_or_empty(&db_, kColPayload, row);
      if (!payload.empty()) {
        std::unordered_map<std::string, std::string> kv{};
        if (cuwacunu::hero::hashimyei::parse_latent_lineage_state_payload(
                payload, &kv)) {
          std::string parse_error{};
          cuwacunu::hero::hashimyei::component_manifest_t manifest{};
          if (cuwacunu::hero::hashimyei::parse_component_manifest_kv(
                  kv, &manifest, &parse_error)) {
            component.manifest = std::move(manifest);
          }
        }
      }
      if (component.manifest.canonical_path.empty()) {
        component.manifest.canonical_path =
            as_text_or_empty(&db_, kColTextA, row);
      }
      if (component.manifest.contract_identity.hash_sha256_hex.empty()) {
        component.manifest.contract_identity.hash_sha256_hex =
            as_text_or_empty(&db_, kColContractHash, row);
      }
      if (component.manifest.hashimyei_identity.name.empty()) {
        component.manifest.hashimyei_identity.name =
            as_text_or_empty(&db_, kColStateTxt, row);
      }
      if (component.manifest.family.empty()) {
        component.manifest.family =
            runtime_family_from_canonical(component.manifest.canonical_path);
      }
      if (component.manifest.created_at_ms == 0) {
        component.manifest.created_at_ms = component.ts_ms;
      }
      if (component.manifest.updated_at_ms == 0) {
        component.manifest.updated_at_ms = component.ts_ms;
      }
      if (!component.component_id.empty()) {
        runtime_components_by_id_[component.component_id] = component;
      }
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindRUNTIME_REPORT_FRAGMENT) {
      runtime_report_fragment_t fragment{};
      fragment.report_fragment_id = as_text_or_empty(&db_, kColRecordId, row);
      if (fragment.report_fragment_id.empty())
        continue;
      fragment.canonical_path =
          trim_ascii(as_text_or_empty(&db_, kColTextA, row));
      fragment.hashimyei = as_text_or_empty(&db_, kColTextB, row);
      fragment.contract_hash = as_text_or_empty(&db_, kColContractHash, row);
      fragment.schema = as_text_or_empty(&db_, kColStateTxt, row);
      fragment.report_fragment_sha256 = fragment.report_fragment_id;
      if (const auto it_path =
              runtime_ledger_path_by_id_.find(fragment.report_fragment_id);
          it_path != runtime_ledger_path_by_id_.end()) {
        fragment.path = it_path->second;
      }
      fragment.payload_json = as_text_or_empty(&db_, kColPayload, row);
      (void)parse_u64(as_text_or_empty(&db_, kColTsMs, row), &fragment.ts_ms);

      std::unordered_map<std::string, std::string> kv{};
      runtime_lls_document_t ignored_document{};
      std::string parse_error{};
      if (!fragment.payload_json.empty() &&
          parse_runtime_lls_payload(fragment.payload_json, &kv,
                                    &ignored_document, &parse_error)) {
        if (fragment.canonical_path.empty()) {
          fragment.canonical_path = trim_ascii(kv["canonical_path"]);
        }
        if (fragment.hashimyei.empty())
          fragment.hashimyei = kv["hashimyei"];
        if (fragment.contract_hash.empty()) {
          fragment.contract_hash = kv["contract_hash"];
        }
        if (fragment.component_compatibility_sha256_hex.empty()) {
          fragment.component_compatibility_sha256_hex =
              trim_ascii(kv["component_compatibility_sha256_hex"]);
        }
        if (fragment.schema.empty())
          fragment.schema = kv["schema"];
        populate_runtime_report_fragment_header_fields_(kv, &fragment);
      }
      if (fragment.intersection_cursor.empty() && fragment.wave_cursor != 0 &&
          !fragment.canonical_path.empty()) {
        fragment.intersection_cursor = build_intersection_cursor(
            fragment.canonical_path, fragment.wave_cursor);
      }
      fragment.family = runtime_family_from_canonical(fragment.canonical_path);
      index_runtime_report_fragment_(
          fragment, &runtime_report_fragments_by_id_,
          &runtime_latest_report_fragment_by_key_,
          &runtime_report_fragment_ids_by_canonical_,
          &runtime_fact_summaries_by_canonical_,
          &runtime_report_fragment_ids_by_wave_cursor_);
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindRUNTIME_REPORT)
      continue;
  }

  for (auto &[cell_id, cell] : cells_by_id_) {
    auto it_trials = trials_by_cell_.find(cell_id);
    if (it_trials != trials_by_cell_.end()) {
      auto &trials = it_trials->second;
      std::sort(trials.begin(), trials.end(),
                [](const wave_trial_t &a, const wave_trial_t &b) {
                  if (a.started_at_ms != b.started_at_ms) {
                    return a.started_at_ms < b.started_at_ms;
                  }
                  return a.trial_id < b.trial_id;
                });
      cell.trial_count = trials.size();
      if (!trials.empty()) {
        cell.last_trial_id = trials.back().trial_id;
        cell.state = trials.back().ok ? "ready" : "error";
        if (cell.updated_at_ms < trials.back().finished_at_ms) {
          cell.updated_at_ms = trials.back().finished_at_ms;
        }
        if (cell.created_at_ms == 0) {
          cell.created_at_ms = trials.front().started_at_ms;
        }
      }
    }
  }

  refresh_runtime_report_fragment_family_ranks_(
      &runtime_report_fragments_by_id_, runtime_components_by_id_,
      explicit_family_rank_by_scope_);

  return true;
}

bool lattice_catalog_store_t::rebuild_runtime_indexes_(std::string *error) {
  clear_error(error);
  if (!db_) {
    set_error(error, "catalog is not open");
    return false;
  }

  cells_by_id_.clear();
  cell_id_by_coord_profile_.clear();
  trials_by_cell_.clear();
  report_by_trial_id_.clear();
  projection_num_by_cell_.clear();
  projection_txt_by_cell_.clear();
  runtime_report_fragments_by_id_.clear();
  runtime_latest_report_fragment_by_key_.clear();
  runtime_report_fragment_ids_by_canonical_.clear();
  runtime_fact_summaries_by_canonical_.clear();
  runtime_dependency_files_by_run_id_.clear();
  runtime_report_fragment_ids_by_wave_cursor_.clear();
  runtime_ledger_.clear();
  runtime_ledger_path_by_id_.clear();
  runtime_components_by_id_.clear();
  runtime_runs_by_id_.clear();
  explicit_family_rank_by_scope_.clear();

  std::unordered_map<std::string, std::uint64_t> family_rank_row_ts_by_scope{};
  std::unordered_map<std::string, std::string> family_rank_row_id_by_scope{};

  const idydb_column_row_sizing next =
      idydb_column_next_row(&db_, kColRecordKind);
  const idydb_column_row_sizing next_record_id =
      idydb_column_next_row(&db_, kColRecordId);
  const idydb_column_row_sizing next_contract_hash =
      idydb_column_next_row(&db_, kColContractHash);
  const idydb_column_row_sizing next_state_txt =
      idydb_column_next_row(&db_, kColStateTxt);
  const idydb_column_row_sizing next_text_a =
      idydb_column_next_row(&db_, kColTextA);
  const idydb_column_row_sizing next_text_b =
      idydb_column_next_row(&db_, kColTextB);
  const idydb_column_row_sizing next_ts_ms =
      idydb_column_next_row(&db_, kColTsMs);
  const idydb_column_row_sizing next_payload =
      idydb_column_next_row(&db_, kColPayload);
  for (idydb_column_row_sizing row = 1; row < next; ++row) {
    const std::string kind = as_text_or_empty(&db_, kColRecordKind, row);
    if (kind.empty())
      continue;

    if (kind == cuwacunu::hero::schema::kRecordKindFAMILY_RANK) {
      const std::string payload = (row < next_payload)
                                      ? as_text_or_empty(&db_, kColPayload, row)
                                      : std::string{};
      if (payload.empty())
        continue;
      std::unordered_map<std::string, std::string> kv{};
      runtime_lls_document_t ignored_document{};
      std::string parse_error{};
      if (!parse_runtime_lls_payload(payload, &kv, &ignored_document,
                                     &parse_error)) {
        continue;
      }
      cuwacunu::hero::family_rank::state_t rank_state{};
      if (!cuwacunu::hero::family_rank::parse_state_from_kv(kv, &rank_state,
                                                            &parse_error)) {
        continue;
      }
      std::uint64_t row_ts = rank_state.updated_at_ms;
      if (row_ts == 0 && row < next_ts_ms) {
        (void)parse_u64(as_text_or_empty(&db_, kColTsMs, row), &row_ts);
      }
      const std::string scope_key = cuwacunu::hero::family_rank::scope_key(
          rank_state.family, rank_state.component_compatibility_sha256_hex);
      const std::string row_id = (row < next_record_id)
                                     ? as_text_or_empty(&db_, kColRecordId, row)
                                     : std::string{};
      const auto current_ts_it = family_rank_row_ts_by_scope.find(scope_key);
      const bool should_replace =
          current_ts_it == family_rank_row_ts_by_scope.end() ||
          row_ts > current_ts_it->second ||
          (row_ts == current_ts_it->second &&
           row_id > family_rank_row_id_by_scope[scope_key]);
      if (!should_replace)
        continue;
      family_rank_row_ts_by_scope[scope_key] = row_ts;
      family_rank_row_id_by_scope[scope_key] = row_id;
      explicit_family_rank_by_scope_[scope_key] = std::move(rank_state);
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindRUNTIME_LEDGER) {
      if (row >= next_record_id)
        continue;
      const std::string report_fragment_id =
          as_text_or_empty(&db_, kColRecordId, row);
      if (report_fragment_id.empty())
        continue;
      runtime_ledger_.insert(report_fragment_id);
      if (row < next_text_a) {
        runtime_ledger_path_by_id_[report_fragment_id] =
            as_text_or_empty(&db_, kColTextA, row);
      }
      if (const auto it_fragment =
              runtime_report_fragments_by_id_.find(report_fragment_id);
          it_fragment != runtime_report_fragments_by_id_.end() &&
          it_fragment->second.path.empty()) {
        it_fragment->second.path =
            runtime_ledger_path_by_id_[report_fragment_id];
      }
      continue;
    }

    if (kind != cuwacunu::hero::schema::kRecordKindRUNTIME_REPORT_FRAGMENT) {
      continue;
    }

    runtime_report_fragment_t fragment{};
    if (row >= next_record_id)
      continue;
    fragment.report_fragment_id = as_text_or_empty(&db_, kColRecordId, row);
    if (fragment.report_fragment_id.empty())
      continue;
    fragment.report_fragment_sha256 = fragment.report_fragment_id;
    if (const auto it_path =
            runtime_ledger_path_by_id_.find(fragment.report_fragment_id);
        it_path != runtime_ledger_path_by_id_.end()) {
      fragment.path = it_path->second;
    }
    if (row < next_payload) {
      fragment.payload_json = as_text_or_empty(&db_, kColPayload, row);
    }
    if (row < next_ts_ms) {
      (void)parse_u64(as_text_or_empty(&db_, kColTsMs, row), &fragment.ts_ms);
    }

    std::unordered_map<std::string, std::string> kv{};
    runtime_lls_document_t ignored_document{};
    std::string parse_error{};
    if (!fragment.payload_json.empty() &&
        parse_runtime_lls_payload(fragment.payload_json, &kv, &ignored_document,
                                  &parse_error)) {
      if (fragment.canonical_path.empty()) {
        if (const auto it = kv.find("canonical_path"); it != kv.end()) {
          fragment.canonical_path = trim_ascii(it->second);
        }
      }
      if (fragment.hashimyei.empty()) {
        if (const auto it = kv.find("hashimyei"); it != kv.end()) {
          fragment.hashimyei = it->second;
        }
      }
      if (fragment.contract_hash.empty()) {
        if (const auto it = kv.find("contract_hash"); it != kv.end()) {
          fragment.contract_hash = it->second;
        }
      }
      if (fragment.component_compatibility_sha256_hex.empty()) {
        if (const auto it = kv.find("component_compatibility_sha256_hex");
            it != kv.end()) {
          fragment.component_compatibility_sha256_hex = trim_ascii(it->second);
        }
      }
      if (fragment.schema.empty()) {
        if (const auto it = kv.find("schema"); it != kv.end()) {
          fragment.schema = it->second;
        }
      }
      populate_runtime_report_fragment_header_fields_(kv, &fragment);
    }
    if (fragment.canonical_path.empty()) {
      if (row < next_text_a) {
        fragment.canonical_path =
            trim_ascii(as_text_or_empty(&db_, kColTextA, row));
      }
    }
    if (fragment.hashimyei.empty()) {
      if (row < next_text_b) {
        fragment.hashimyei = as_text_or_empty(&db_, kColTextB, row);
      }
    }
    if (fragment.contract_hash.empty()) {
      if (row < next_contract_hash) {
        fragment.contract_hash = as_text_or_empty(&db_, kColContractHash, row);
      }
    }
    if (fragment.schema.empty()) {
      if (row < next_state_txt) {
        fragment.schema = as_text_or_empty(&db_, kColStateTxt, row);
      }
    }
    if (fragment.intersection_cursor.empty() && fragment.wave_cursor != 0 &&
        !fragment.canonical_path.empty()) {
      fragment.intersection_cursor = build_intersection_cursor(
          fragment.canonical_path, fragment.wave_cursor);
    }
    fragment.family = runtime_family_from_canonical(fragment.canonical_path);
    index_runtime_report_fragment_(
        fragment, &runtime_report_fragments_by_id_,
        &runtime_latest_report_fragment_by_key_,
        &runtime_report_fragment_ids_by_canonical_,
        &runtime_fact_summaries_by_canonical_,
        &runtime_report_fragment_ids_by_wave_cursor_);
  }

  refresh_runtime_report_fragment_family_ranks_(
      &runtime_report_fragments_by_id_, runtime_components_by_id_,
      explicit_family_rank_by_scope_);
  return true;
}

bool lattice_catalog_store_t::resolve_cell(
    const wave_cell_coord_t &coord, const wave_execution_profile_t &profile,
    wave_cell_t *out, std::string *error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "output cell pointer is null");
    return false;
  }
  *out = wave_cell_t{};

  const std::string expected_cell_id =
      compute_cell_id(coord.contract_hash, coord.wave_hash, profile);
  if (const auto it = cells_by_id_.find(expected_cell_id);
      it != cells_by_id_.end()) {
    *out = it->second;
    return true;
  }

  const std::string profile_id = compute_profile_id(profile);
  const std::string key =
      coord_profile_key_(coord.contract_hash, coord.wave_hash, profile_id);
  const auto it_id = cell_id_by_coord_profile_.find(key);
  if (it_id == cell_id_by_coord_profile_.end()) {
    set_error(error, "cell not found for coordinate + execution_profile");
    return false;
  }
  const auto it = cells_by_id_.find(it_id->second);
  if (it == cells_by_id_.end()) {
    set_error(error, "cell id map points to missing cell");
    return false;
  }
  *out = it->second;
  return true;
}

bool lattice_catalog_store_t::get_cell(std::string_view cell_id,
                                       wave_cell_t *out,
                                       std::string *error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "output cell pointer is null");
    return false;
  }
  *out = wave_cell_t{};
  const auto it = cells_by_id_.find(std::string(cell_id));
  if (it == cells_by_id_.end()) {
    set_error(error, "cell not found: " + std::string(cell_id));
    return false;
  }
  *out = it->second;
  return true;
}

bool lattice_catalog_store_t::list_trials(std::string_view cell_id,
                                          std::size_t limit, std::size_t offset,
                                          bool newest_first,
                                          std::vector<wave_trial_t> *out,
                                          std::string *error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "trials output pointer is null");
    return false;
  }
  out->clear();

  const auto it = trials_by_cell_.find(std::string(cell_id));
  if (it == trials_by_cell_.end())
    return true;

  std::vector<wave_trial_t> trials = it->second;
  if (newest_first) {
    std::reverse(trials.begin(), trials.end());
  }

  const std::size_t begin = std::min(offset, trials.size());
  std::size_t end = trials.size();
  if (limit != 0)
    end = std::min(end, begin + limit);
  out->assign(trials.begin() + static_cast<std::ptrdiff_t>(begin),
              trials.begin() + static_cast<std::ptrdiff_t>(end));
  return true;
}

bool lattice_catalog_store_t::query_matrix(const matrix_query_t &query,
                                           std::vector<wave_cell_t> *out,
                                           std::string *error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "matrix output pointer is null");
    return false;
  }
  out->clear();

  std::vector<wave_cell_t> matches{};
  matches.reserve(cells_by_id_.size());

  for (const auto &[_, cell] : cells_by_id_) {
    if (!query.contract_hash.empty() &&
        cell.coord.contract_hash != query.contract_hash) {
      continue;
    }
    if (!query.wave_hash.empty() && cell.coord.wave_hash != query.wave_hash) {
      continue;
    }

    bool matched = true;
    if (const auto it_projection = projection_num_by_cell_.find(cell.cell_id);
        !query.projection_num_eq.empty()) {
      if (it_projection == projection_num_by_cell_.end()) {
        matched = false;
      } else {
        for (const auto &[k, v] : query.projection_num_eq) {
          const auto it = it_projection->second.find(k);
          if (it == it_projection->second.end() ||
              !is_numeric_close(it->second, v)) {
            matched = false;
            break;
          }
        }
      }
    }
    if (!matched)
      continue;

    if (const auto it_projection = projection_txt_by_cell_.find(cell.cell_id);
        !query.projection_txt_eq.empty()) {
      if (it_projection == projection_txt_by_cell_.end()) {
        matched = false;
      } else {
        for (const auto &[k, v] : query.projection_txt_eq) {
          const auto it = it_projection->second.find(k);
          if (it == it_projection->second.end() || it->second != v) {
            matched = false;
            break;
          }
        }
      }
    }
    if (!matched)
      continue;

    const auto it_trials = trials_by_cell_.find(cell.cell_id);
    const std::vector<wave_trial_t> *trials =
        (it_trials == trials_by_cell_.end()) ? nullptr : &it_trials->second;

    const wave_trial_t *chosen_trial = nullptr;
    if (trials && !trials->empty()) {
      if (query.latest_success_only) {
        for (auto it = trials->rbegin(); it != trials->rend(); ++it) {
          if (it->ok) {
            chosen_trial = &(*it);
            break;
          }
        }
      } else {
        chosen_trial = &trials->back();
      }
    } else if (query.latest_success_only) {
      continue;
    }

    if (!query.state_snapshot_id.empty()) {
      if (!chosen_trial ||
          chosen_trial->state_snapshot_id != query.state_snapshot_id) {
        continue;
      }
    }

    wave_cell_t selected = cell;
    if (chosen_trial) {
      selected.last_trial_id = chosen_trial->trial_id;
      selected.state = chosen_trial->ok ? "ready" : "error";
      selected.updated_at_ms =
          std::max(selected.updated_at_ms, chosen_trial->finished_at_ms);
      const auto it_report = report_by_trial_id_.find(chosen_trial->trial_id);
      if (it_report != report_by_trial_id_.end()) {
        selected.report = it_report->second;
      }
    }

    matches.push_back(std::move(selected));
  }

  std::sort(matches.begin(), matches.end(),
            [&](const wave_cell_t &a, const wave_cell_t &b) {
              if (a.updated_at_ms != b.updated_at_ms) {
                return query.newest_first ? (a.updated_at_ms > b.updated_at_ms)
                                          : (a.updated_at_ms < b.updated_at_ms);
              }
              return query.newest_first ? (a.cell_id > b.cell_id)
                                        : (a.cell_id < b.cell_id);
            });

  const std::size_t begin = std::min(query.offset, matches.size());
  std::size_t end = matches.size();
  if (query.limit != 0)
    end = std::min(end, begin + query.limit);

  out->assign(matches.begin() + static_cast<std::ptrdiff_t>(begin),
              matches.begin() + static_cast<std::ptrdiff_t>(end));
  return true;
}

bool lattice_catalog_store_t::get_cell_report(std::string_view cell_id,
                                              lattice_cell_report_t *out,
                                              std::string *error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "cell-report output pointer is null");
    return false;
  }
  *out = lattice_cell_report_t{};

  wave_cell_t cell{};
  if (!get_cell(cell_id, &cell, error))
    return false;
  *out = cell.report;
  return true;
}

bool lattice_catalog_store_t::runtime_ledger_contains_(
    std::string_view report_fragment_id, bool *out_exists, std::string *error) {
  clear_error(error);
  if (!out_exists) {
    set_error(error, "runtime ledger output pointer is null");
    return false;
  }
  *out_exists = false;
  const std::string id(report_fragment_id);
  if (id.empty())
    return true;
  if (runtime_ledger_.count(id) != 0) {
    *out_exists = true;
    return true;
  }
  return true;
}

bool lattice_catalog_store_t::append_runtime_ledger_(
    std::string_view report_fragment_id, std::string_view path,
    std::string *error) {
  if (!append_row_(cuwacunu::hero::schema::kRecordKindRUNTIME_LEDGER,
                   report_fragment_id, "", "", "", "", "", "",
                   std::numeric_limits<double>::quiet_NaN(), path, "", "2",
                   std::to_string(now_ms_utc()), "{}", "",
                   std::numeric_limits<double>::quiet_NaN(), "", "", "", "", "",
                   "", "", "", "", error)) {
    return false;
  }
  runtime_ledger_.insert(std::string(report_fragment_id));
  runtime_ledger_path_by_id_[std::string(report_fragment_id)] =
      std::string(path);
  if (const auto it_fragment =
          runtime_report_fragments_by_id_.find(std::string(report_fragment_id));
      it_fragment != runtime_report_fragments_by_id_.end() &&
      it_fragment->second.path.empty()) {
    it_fragment->second.path = std::string(path);
  }
  return true;
}

bool lattice_catalog_store_t::ingest_runtime_run_manifest_file_(
    const std::filesystem::path &path, std::string *error) {
  clear_error(error);
  cuwacunu::hero::hashimyei::run_manifest_t m{};
  if (!cuwacunu::hero::hashimyei::load_run_manifest(path, &m, error))
    return false;

  runtime_runs_by_id_[m.run_id] = m;
  runtime_dependency_files_by_run_id_[m.run_id] = m.dependency_files;
  if (runtime_run_ids_.count(m.run_id) != 0)
    return true;

  std::filesystem::path cp = path;
  if (const auto can = canonicalized(path); can.has_value())
    cp = *can;
  std::string payload{};
  if (!read_text_file(path, &payload, error))
    return false;
  std::string manifest_sha{};
  if (!sha256_hex_file(cp, &manifest_sha, error))
    return false;

  if (!append_row_(cuwacunu::hero::schema::kRecordKindRUNTIME_RUN, m.run_id, "",
                   m.wave_contract_binding.contract.name,
                   m.wave_contract_binding.wave.name,
                   m.wave_contract_binding.identity.name, "", m.schema,
                   static_cast<double>(m.started_at_ms),
                   m.wave_contract_binding.binding_id, m.campaign_identity.name,
                   "2", std::to_string(m.started_at_ms), payload, "",
                   std::numeric_limits<double>::quiet_NaN(), "", "", "",
                   std::to_string(m.started_at_ms), "", "", "", manifest_sha,
                   m.run_id, error)) {
    return false;
  }
  for (const auto &d : m.dependency_files) {
    const std::string rec_id = m.run_id + "|" + d.canonical_path;
    if (!append_row_(cuwacunu::hero::schema::kRecordKindRUNTIME_DEPENDENCY,
                     rec_id, "", "", "", "", "", d.canonical_path,
                     std::numeric_limits<double>::quiet_NaN(), d.sha256_hex, "",
                     "2", std::to_string(m.started_at_ms), "{}", "",
                     std::numeric_limits<double>::quiet_NaN(), "", "", "", "",
                     "", "", "", "", m.run_id, error)) {
      return false;
    }
  }
  runtime_run_ids_.insert(m.run_id);
  return true;
}

bool lattice_catalog_store_t::ingest_runtime_component_manifest_file_(
    const std::filesystem::path &path, std::string *error) {
  clear_error(error);
  cuwacunu::hero::hashimyei::component_manifest_t manifest{};
  if (!cuwacunu::hero::hashimyei::load_component_manifest(path, &manifest,
                                                          error)) {
    return false;
  }

  std::string payload{};
  if (!read_text_file(path, &payload, error))
    return false;
  std::string manifest_sha{};
  if (!sha256_hex_file(path, &manifest_sha, error))
    return false;

  runtime_components_by_id_
      [cuwacunu::hero::hashimyei::compute_component_manifest_id(manifest)] =
          cuwacunu::hero::hashimyei::component_state_t{
              .component_id =
                  cuwacunu::hero::hashimyei::compute_component_manifest_id(
                      manifest),
              .ts_ms = manifest.updated_at_ms != 0 ? manifest.updated_at_ms
                                                   : manifest.created_at_ms,
              .manifest_path = path.string(),
              .report_fragment_sha256 = manifest_sha,
              .family_rank = std::nullopt,
              .manifest = manifest,
          };

  bool already = false;
  if (!runtime_ledger_contains_(manifest_sha, &already, error))
    return false;
  if (already)
    return true;

  std::filesystem::path cp = path;
  if (const auto can = canonicalized(path); can.has_value())
    cp = *can;

  const std::string component_id =
      cuwacunu::hero::hashimyei::compute_component_manifest_id(manifest);
  const std::uint64_t ts_ms =
      manifest.updated_at_ms != 0
          ? manifest.updated_at_ms
          : (manifest.created_at_ms != 0 ? manifest.created_at_ms
                                         : now_ms_utc());
  if (!append_row_(cuwacunu::hero::schema::kRecordKindRUNTIME_COMPONENT,
                   component_id, "",
                   cuwacunu::hero::hashimyei::contract_hash_from_identity(
                       manifest.contract_identity),
                   "", "", "", manifest.hashimyei_identity.name,
                   std::numeric_limits<double>::quiet_NaN(),
                   manifest.canonical_path, cp.string(), "2",
                   std::to_string(ts_ms), payload, "",
                   std::numeric_limits<double>::quiet_NaN(), "", "", "", "", "",
                   "", "", manifest_sha, "", error)) {
    return false;
  }
  return append_runtime_ledger_(manifest_sha, cp.string(), error);
}

bool lattice_catalog_store_t::ingest_runtime_report_fragment_file_(
    const std::filesystem::path &path, std::string *error) {
  clear_error(error);
  if (!std::filesystem::exists(path) ||
      !std::filesystem::is_regular_file(path)) {
    return true;
  }

  std::string payload;
  if (!read_text_file(path, &payload, nullptr))
    return true;
  if (payload.empty())
    return true;

  std::unordered_map<std::string, std::string> kv;
  if (!parse_runtime_lls_payload(payload, &kv, nullptr, error)) {
    set_error(error, "runtime .lls parse failure for " + path.string() + ": " +
                         (error ? *error : std::string{}));
    return false;
  }
  const std::string schema = kv["schema"];
  const bool is_family_rank_artifact =
      path.filename() == "family.rank.latest.lls" ||
      path.string().find("/family_rank/") != std::string::npos;
  if (is_family_rank_artifact || is_family_rank_schema(schema)) {
    cuwacunu::hero::family_rank::state_t rank_state{};
    if (!cuwacunu::hero::family_rank::parse_state_from_kv(kv, &rank_state,
                                                          error)) {
      set_error(error, "family rank payload parse failure for " +
                           path.string() + ": " +
                           (error ? *error : std::string{}));
      return false;
    }
    std::string artifact_sha256{};
    if (!sha256_hex_file(path, &artifact_sha256, error))
      return false;
    bool already = false;
    if (!runtime_ledger_contains_(artifact_sha256, &already, error))
      return false;
    if (already) {
      explicit_family_rank_by_scope_[cuwacunu::hero::family_rank::scope_key(
          rank_state.family, rank_state.component_compatibility_sha256_hex)] =
          rank_state;
      refresh_runtime_report_fragment_family_ranks_(
          &runtime_report_fragments_by_id_, runtime_components_by_id_,
          explicit_family_rank_by_scope_, rank_state.family,
          rank_state.component_compatibility_sha256_hex);
      return true;
    }
    std::filesystem::path cp = path;
    if (const auto can = canonicalized(path); can.has_value())
      cp = *can;
    const std::string canonical_file_path = cp.string();
    const std::uint64_t ts_ms =
        rank_state.updated_at_ms != 0 ? rank_state.updated_at_ms : now_ms_utc();
    if (!append_row_(
            cuwacunu::hero::schema::kRecordKindFAMILY_RANK,
            family_rank_record_id(rank_state.family,
                                  rank_state.component_compatibility_sha256_hex,
                                  artifact_sha256),
            "", rank_state.component_compatibility_sha256_hex, "", "", "",
            rank_state.family,
            static_cast<double>(rank_state.assignments.size()),
            rank_state.schema, "", "2", std::to_string(ts_ms), payload, "",
            std::numeric_limits<double>::quiet_NaN(), "", "", "", "", "", "",
            "", "", "", error)) {
      return false;
    }
    explicit_family_rank_by_scope_[cuwacunu::hero::family_rank::scope_key(
        rank_state.family, rank_state.component_compatibility_sha256_hex)] =
        rank_state;
    refresh_runtime_report_fragment_family_ranks_(
        &runtime_report_fragments_by_id_, runtime_components_by_id_,
        explicit_family_rank_by_scope_, rank_state.family,
        rank_state.component_compatibility_sha256_hex);
    return append_runtime_ledger_(artifact_sha256, canonical_file_path, error);
  }
  if (!is_known_runtime_schema(schema))
    return true;

  std::string report_fragment_sha;
  if (!sha256_hex_file(path, &report_fragment_sha, error))
    return false;

  cuwacunu::piaabo::latent_lineage_state::runtime_report_header_t header{};
  if (!cuwacunu::piaabo::latent_lineage_state::
          parse_runtime_report_header_from_kv(kv, &header, error)) {
    set_error(error,
              "runtime report_fragment header parse failure: " + path.string());
    return false;
  }

  const std::string report_canonical_path = normalize_source_hashimyei_cursor(
      trim_ascii(header.context.canonical_path));
  if (report_canonical_path.empty()) {
    set_error(error, "runtime report_fragment missing canonical_path: " +
                         path.string());
    return false;
  }
  std::string canonical_path =
      normalize_source_hashimyei_cursor(trim_ascii(kv["canonical_path"]));
  if (canonical_path.empty())
    canonical_path = report_canonical_path;

  std::string hashimyei = kv["hashimyei"];
  if (hashimyei.empty())
    hashimyei = maybe_hashimyei_from_canonical(canonical_path);

  std::string contract_hash = kv["contract_hash"];
  if (contract_hash.empty())
    contract_hash = contract_hash_from_report_fragment_path(path);
  const std::string component_compatibility_sha256_hex =
      trim_ascii(kv["component_compatibility_sha256_hex"]);

  std::uint64_t ts_ms = now_ms_utc();
  {
    std::error_code ec;
    const auto fts = std::filesystem::last_write_time(path, ec);
    if (!ec) {
      const auto d = fts.time_since_epoch();
      const auto ms =
          std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
      if (ms > 0)
        ts_ms = static_cast<std::uint64_t>(ms);
    }
  }

  if (!header.context.has_wave_cursor) {
    set_error(error,
              "runtime report_fragment missing wave_cursor: " + path.string());
    return false;
  }
  const std::uint64_t wave_cursor = header.context.wave_cursor;

  std::filesystem::path cp = path;
  if (const auto can = canonicalized(path); can.has_value())
    cp = *can;
  const std::string canonical_file_path = cp.string();

  runtime_report_fragment_t fragment{};
  fragment.report_fragment_id = report_fragment_sha;
  fragment.canonical_path = canonical_path;
  fragment.family = runtime_family_from_canonical(canonical_path);
  fragment.hashimyei = hashimyei;
  fragment.contract_hash = contract_hash;
  fragment.component_compatibility_sha256_hex =
      component_compatibility_sha256_hex;
  fragment.schema = schema;
  fragment.report_fragment_sha256 = report_fragment_sha;
  fragment.path = canonical_file_path;
  fragment.ts_ms = ts_ms;
  fragment.wave_cursor = wave_cursor;
  fragment.intersection_cursor =
      build_intersection_cursor(canonical_path, wave_cursor);
  fragment.payload_json = payload;
  populate_runtime_report_fragment_header_fields_(kv, &fragment);
  index_runtime_report_fragment_(fragment, &runtime_report_fragments_by_id_,
                                 &runtime_latest_report_fragment_by_key_,
                                 &runtime_report_fragment_ids_by_canonical_,
                                 &runtime_fact_summaries_by_canonical_,
                                 &runtime_report_fragment_ids_by_wave_cursor_);

  bool already = false;
  if (!runtime_ledger_contains_(report_fragment_sha, &already, error))
    return false;
  if (already)
    return true;

  if (!append_row_(cuwacunu::hero::schema::kRecordKindRUNTIME_REPORT_FRAGMENT,
                   report_fragment_sha, "", contract_hash, "", "", "", schema,
                   std::numeric_limits<double>::quiet_NaN(), canonical_path,
                   hashimyei, "2", std::to_string(ts_ms), payload, "",
                   std::numeric_limits<double>::quiet_NaN(), "", "", "", "", "",
                   "", "", "", "", error)) {
    return false;
  }
  return append_runtime_ledger_(report_fragment_sha, canonical_file_path,
                                error);
}
