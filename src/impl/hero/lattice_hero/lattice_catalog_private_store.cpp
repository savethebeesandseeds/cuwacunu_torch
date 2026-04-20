bool lattice_catalog_store_t::open(const options_t& options, std::string* error) {
  clear_error(error);
  const std::uint64_t open_started_at_ms = now_ms_utc();
  const auto log_open_finish = [&](bool ok,
                                   std::string_view error_message = {}) {
    std::ostringstream extra;
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "catalog_path", options.catalog_path.string());
    cuwacunu::hero::mcp_observability::append_json_bool_field(
        extra, "read_only", options.read_only);
    cuwacunu::hero::mcp_observability::append_json_bool_field(
        extra, "encrypted", options.encrypted);
    cuwacunu::hero::mcp_observability::append_json_bool_field(extra, "ok", ok);
    cuwacunu::hero::mcp_observability::append_json_uint64_field(
        extra, "duration_ms", now_ms_utc() - open_started_at_ms);
    if (!error_message.empty()) {
      cuwacunu::hero::mcp_observability::append_json_string_field(
          extra, "error", error_message);
    }
    log_lattice_catalog_event("catalog_open_finish", extra.str());
  };
  {
    std::ostringstream extra;
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "catalog_path", options.catalog_path.string());
    cuwacunu::hero::mcp_observability::append_json_bool_field(
        extra, "read_only", options.read_only);
    log_lattice_catalog_event("catalog_open_begin", extra.str());
  }
  if (db_ != nullptr) {
    set_error(error, "catalog already open");
    log_open_finish(false, "catalog already open");
    return false;
  }

  options_ = options;
  if (options_.catalog_path.empty()) {
    set_error(error, "catalog_path is required");
    log_open_finish(false, error && !error->empty() ? std::string_view(*error)
                                                    : std::string_view{});
    return false;
  }

  const auto parent = options_.catalog_path.parent_path();
  if (!parent.empty()) {
    std::error_code ec;
    std::filesystem::create_directories(parent, ec);
    if (ec) {
      set_error(error,
                "cannot create catalog directory: " + parent.string());
      log_open_finish(false, error && !error->empty() ? std::string_view(*error)
                                                      : std::string_view{});
      return false;
    }
  }

  int rc = IDYDB_ERROR;
  const int open_flags = options_.read_only ? IDYDB_READONLY : IDYDB_CREATE;
  for (int attempt = 0; attempt <= kCatalogOpenBusyRetryCount; ++attempt) {
    if (options_.encrypted) {
      if (options_.passphrase.empty()) {
        set_error(error, "encrypted catalog requires passphrase");
        log_open_finish(false, error && !error->empty() ? std::string_view(*error)
                                                        : std::string_view{});
        return false;
      }
      rc = idydb_open_encrypted(options_.catalog_path.string().c_str(), &db_,
                                open_flags, options_.passphrase.c_str());
    } else {
      rc = idydb_open(options_.catalog_path.string().c_str(), &db_, open_flags);
    }

    if (rc == IDYDB_SUCCESS && db_ != nullptr) break;
    if (db_) {
      (void)idydb_close(&db_);
      db_ = nullptr;
    }

    if (rc != IDYDB_BUSY || attempt >= kCatalogOpenBusyRetryCount) break;
    std::this_thread::sleep_for(kCatalogOpenBusyRetryDelay);
  }

  if (rc != IDYDB_SUCCESS || db_ == nullptr) {
    const char* msg = db_ ? idydb_errmsg(&db_) : nullptr;
    std::string detail =
        (msg && msg[0] != '\0')
            ? std::string(msg)
            : ("idydb rc=" + std::to_string(rc) + " (" + idydb_rc_label(rc) + ")");
    if (rc == IDYDB_BUSY) {
      detail += "; catalog lock is held by another process";
    }
    set_error(error, "cannot open lattice catalog: " + detail);
    log_open_finish(false, error && !error->empty() ? std::string_view(*error)
                                                    : std::string_view{});
    if (db_) {
      (void)idydb_close(&db_);
      db_ = nullptr;
    }
    return false;
  }

  if (!ensure_catalog_header_(error)) {
    std::string close_error;
    (void)close(&close_error);
    log_open_finish(false, error && !error->empty() ? std::string_view(*error)
                                                    : std::string_view{});
    return false;
  }
  const bool indexes_ok =
      options_.runtime_only_indexes ? rebuild_runtime_indexes_(error)
                                    : rebuild_indexes(error);
  if (!indexes_ok) {
    std::string close_error;
    (void)close(&close_error);
    log_open_finish(false, error && !error->empty() ? std::string_view(*error)
                                                    : std::string_view{});
    return false;
  }
  opened_at_ms_ = now_ms_utc();
  log_open_finish(true);
  return true;
}

bool lattice_catalog_store_t::close(std::string* error) {
  clear_error(error);
  if (!db_) return true;
  const std::uint64_t close_started_at_ms = now_ms_utc();
  const std::uint64_t opened_at_ms = opened_at_ms_;
  {
    std::ostringstream extra;
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "catalog_path", options_.catalog_path.string());
    log_lattice_catalog_event("catalog_close_begin", extra.str());
  }
  if (idydb_close(&db_) != IDYDB_DONE) {
    const char* msg = idydb_errmsg(&db_);
    set_error(error, "idydb_close failed: " +
                         std::string(msg ? msg : "<no error message>"));
    std::ostringstream extra;
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "catalog_path", options_.catalog_path.string());
    cuwacunu::hero::mcp_observability::append_json_bool_field(extra, "ok",
                                                              false);
    cuwacunu::hero::mcp_observability::append_json_uint64_field(
        extra, "duration_ms", now_ms_utc() - close_started_at_ms);
    if (opened_at_ms != 0) {
      cuwacunu::hero::mcp_observability::append_json_uint64_field(
          extra, "catalog_hold_ms", now_ms_utc() - opened_at_ms);
    }
    if (error && !error->empty()) {
      cuwacunu::hero::mcp_observability::append_json_string_field(
          extra, "error", *error);
    }
    log_lattice_catalog_event("catalog_close_finish", extra.str());
    return false;
  }
  db_ = nullptr;
  opened_at_ms_ = 0;
  next_row_hint_ = 0;
  cells_by_id_.clear();
  cell_id_by_coord_profile_.clear();
  trials_by_cell_.clear();
  report_by_trial_id_.clear();
  projection_num_by_cell_.clear();
  projection_txt_by_cell_.clear();
  runtime_runs_by_id_.clear();
  runtime_run_ids_.clear();
  runtime_components_by_id_.clear();
  runtime_report_fragments_by_id_.clear();
  runtime_latest_report_fragment_by_key_.clear();
  runtime_report_fragment_ids_by_canonical_.clear();
  runtime_fact_summaries_by_canonical_.clear();
  runtime_dependency_files_by_run_id_.clear();
  runtime_report_fragment_ids_by_wave_cursor_.clear();
  runtime_ledger_.clear();
  runtime_ledger_path_by_id_.clear();
  explicit_family_rank_by_scope_.clear();
  {
    const std::uint64_t closed_at_ms = now_ms_utc();
    std::ostringstream extra;
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "catalog_path", options_.catalog_path.string());
    cuwacunu::hero::mcp_observability::append_json_bool_field(extra, "ok", true);
    cuwacunu::hero::mcp_observability::append_json_uint64_field(
        extra, "duration_ms", closed_at_ms - close_started_at_ms);
    if (opened_at_ms != 0) {
      cuwacunu::hero::mcp_observability::append_json_uint64_field(
          extra, "catalog_hold_ms", closed_at_ms - opened_at_ms);
    }
    log_lattice_catalog_event("catalog_close_finish", extra.str());
  }
  return true;
}

bool lattice_catalog_store_t::ensure_catalog_header_(std::string* error) {
  clear_error(error);
  if (!db_) {
    set_error(error, "catalog is not open");
    return false;
  }

  const std::string fast_row_kind = as_text_or_empty(&db_, kColRecordKind, 1);
  const std::string fast_row_id = as_text_or_empty(&db_, kColRecordId, 1);
  if (fast_row_kind == cuwacunu::hero::schema::kRecordKindHEADER &&
      fast_row_id == "catalog_schema_version") {
    const std::string schema =
        trim_ascii(as_text_or_empty(&db_, kColTextA, 1));
    const std::string version =
        trim_ascii(as_text_or_empty(&db_, kColProjectionVersion, 1));
    if (schema != "lattice.catalog.v2" || version != "2") {
      set_error(error,
                "unsupported lattice catalog schema '" + schema +
                    "' (version '" + version + "'); expected strict v2");
      return false;
    }
    return true;
  }

  idydb_filter_term t_kind{};
  t_kind.column = kColRecordKind;
  t_kind.type = IDYDB_CHAR;
  t_kind.op = IDYDB_FILTER_OP_EQ;
  t_kind.value.s = cuwacunu::hero::schema::kRecordKindHEADER;

  idydb_filter_term t_id{};
  t_id.column = kColRecordId;
  t_id.type = IDYDB_CHAR;
  t_id.op = IDYDB_FILTER_OP_EQ;
  t_id.value.s = "catalog_schema_version";

  bool exists = false;
  if (!db::query::exists_row(&db_, {t_kind, t_id}, &exists, error)) return false;
  if (exists) {
    const idydb_column_row_sizing next = idydb_column_next_row(&db_, kColRecordKind);
    for (idydb_column_row_sizing row = 1; row < next; ++row) {
      const std::string row_kind = as_text_or_empty(&db_, kColRecordKind, row);
      const std::string row_id = as_text_or_empty(&db_, kColRecordId, row);
      if (row_kind != cuwacunu::hero::schema::kRecordKindHEADER ||
          row_id != "catalog_schema_version") {
        continue;
      }
      const std::string schema =
          trim_ascii(as_text_or_empty(&db_, kColTextA, row));
      const std::string version =
          trim_ascii(as_text_or_empty(&db_, kColProjectionVersion, row));
      if (schema != "lattice.catalog.v2" || version != "2") {
        set_error(error,
                  "unsupported lattice catalog schema '" + schema +
                      "' (version '" + version + "'); expected strict v2");
        return false;
      }
      return true;
    }
    set_error(error, "catalog_schema_version header row exists but cannot be read");
    return false;
  }

  return append_row_(cuwacunu::hero::schema::kRecordKindHEADER,
                     "catalog_schema_version", "", "", "", "", "",
                     "schema", std::numeric_limits<double>::quiet_NaN(),
                     "lattice.catalog.v2", "", "2",
                     std::to_string(now_ms_utc()), "{}", "",
                     std::numeric_limits<double>::quiet_NaN(), "", "", "", "",
                     "", "", "", "", "", error);
}

bool lattice_catalog_store_t::append_row_(
    std::string_view record_kind, std::string_view record_id,
    std::string_view cell_id, std::string_view contract_hash,
    std::string_view wave_hash, std::string_view profile_id,
    std::string_view execution_profile_json, std::string_view state_txt,
    double metric_num, std::string_view text_a, std::string_view text_b,
    std::string_view projection_version, std::string_view ts_ms,
    std::string_view payload_json, std::string_view projection_key,
    double projection_num, std::string_view projection_txt,
    std::string_view projection_key_aux,
    std::string_view projection_txt_aux, std::string_view started_at_ms,
    std::string_view finished_at_ms, std::string_view ok_txt,
    std::string_view total_steps, std::string_view campaign_hash,
    std::string_view run_id, std::string* error) {
  clear_error(error);
  if (!db_) {
    set_error(error, "catalog is not open");
    return false;
  }
  if (!cuwacunu::hero::schema::is_known_record_kind(record_kind)) {
    set_error(error, "unknown catalog record_kind: " + std::string(record_kind));
    return false;
  }

  const idydb_column_row_sizing row =
      buffer_rows_
          ? (buffered_row_start_ != 0
                 ? static_cast<idydb_column_row_sizing>(
                       buffered_row_start_ + buffered_rows_.size())
                 : ((buffered_row_start_ =
                         (next_row_hint_ != 0)
                             ? next_row_hint_
                             : idydb_column_next_row(&db_, kColRecordKind)),
                    buffered_row_start_))
          : ((next_row_hint_ != 0) ? next_row_hint_
                                   : idydb_column_next_row(&db_, kColRecordKind));
  if (row == 0) {
    set_error(error, "failed to resolve next row");
    return false;
  }
  if (buffer_rows_) {
    buffered_row_t buffered{};
    buffered.record_kind = std::string(record_kind);
    buffered.record_id = std::string(record_id);
    buffered.cell_id = std::string(cell_id);
    buffered.contract_hash = std::string(contract_hash);
    buffered.wave_hash = std::string(wave_hash);
    buffered.profile_id = std::string(profile_id);
    buffered.execution_profile_json = std::string(execution_profile_json);
    buffered.state_txt = std::string(state_txt);
    buffered.metric_num = metric_num;
    buffered.has_metric_num = std::isfinite(metric_num);
    buffered.text_a = std::string(text_a);
    buffered.text_b = std::string(text_b);
    buffered.projection_version = std::string(projection_version);
    buffered.ts_ms = std::string(ts_ms);
    buffered.payload_json = std::string(payload_json);
    buffered.projection_key = std::string(projection_key);
    buffered.projection_num = projection_num;
    buffered.has_projection_num = std::isfinite(projection_num);
    buffered.projection_txt = std::string(projection_txt);
    buffered.projection_key_aux = std::string(projection_key_aux);
    buffered.projection_txt_aux = std::string(projection_txt_aux);
    buffered.started_at_ms = std::string(started_at_ms);
    buffered.finished_at_ms = std::string(finished_at_ms);
    buffered.ok_txt = std::string(ok_txt);
    buffered.total_steps = std::string(total_steps);
    buffered.campaign_hash = std::string(campaign_hash);
    buffered.run_id = std::string(run_id);
    buffered_rows_.push_back(std::move(buffered));
    next_row_hint_ = row + 1;
    return true;
  }

  const auto insert_text_if_present =
      [&](idydb_column_row_sizing column, std::string_view value) {
        return value.empty() || insert_text(&db_, column, row, value, error);
      };
  const auto insert_num_if_present =
      [&](idydb_column_row_sizing column, double value) {
        return std::isnan(value) || insert_num(&db_, column, row, value, error);
      };
  if (!insert_text_if_present(kColRecordKind, record_kind)) return false;
  if (!insert_text_if_present(kColRecordId, record_id)) return false;
  if (!insert_text_if_present(kColCellId, cell_id)) return false;
  if (!insert_text_if_present(kColContractHash, contract_hash)) return false;
  if (!insert_text_if_present(kColWaveHash, wave_hash)) return false;
  if (!insert_text_if_present(kColProfileId, profile_id)) return false;
  if (!insert_text_if_present(kColExecutionProfileJson, execution_profile_json)) {
    return false;
  }
  if (!insert_text_if_present(kColStateTxt, state_txt)) return false;
  if (!insert_num_if_present(kColMetricNum, metric_num)) return false;
  if (!insert_text_if_present(kColTextA, text_a)) return false;
  if (!insert_text_if_present(kColTextB, text_b)) return false;
  if (!insert_text_if_present(kColProjectionVersion, projection_version)) {
    return false;
  }
  if (!insert_text_if_present(kColTsMs, ts_ms)) return false;
  if (!insert_text_if_present(kColPayload, payload_json)) return false;
  if (!insert_text_if_present(kColProjectionKey, projection_key)) {
    return false;
  }
  if (!insert_num_if_present(kColProjectionNum, projection_num)) {
    return false;
  }
  if (!insert_text_if_present(kColProjectionTxt, projection_txt)) {
    return false;
  }
  if (!insert_text_if_present(kColProjectionKeyAux, projection_key_aux)) {
    return false;
  }
  if (!insert_text_if_present(kColProjectionTxtAux, projection_txt_aux)) {
    return false;
  }
  if (!insert_text_if_present(kColStartedAtMs, started_at_ms)) return false;
  if (!insert_text_if_present(kColFinishedAtMs, finished_at_ms)) {
    return false;
  }
  if (!insert_text_if_present(kColOkTxt, ok_txt)) return false;
  if (!insert_text_if_present(kColTotalSteps, total_steps)) return false;
  if (!insert_text_if_present(kColCampaignHash, campaign_hash)) return false;
  if (!insert_text_if_present(kColRunId, run_id)) return false;
  next_row_hint_ = row + 1;
  return true;
}

bool lattice_catalog_store_t::flush_buffered_rows_(std::string* error) {
  clear_error(error);
  if (!db_) {
    set_error(error, "catalog is not open");
    return false;
  }
  if (buffered_rows_.empty()) {
    buffered_row_start_ = 0;
    return true;
  }
  const idydb_column_row_sizing start_row =
      (buffered_row_start_ != 0) ? buffered_row_start_
                                 : ((next_row_hint_ != 0)
                                        ? static_cast<idydb_column_row_sizing>(
                                              next_row_hint_ - buffered_rows_.size())
                                        : idydb_column_next_row(&db_, kColRecordKind));
  if (start_row == 0) {
    set_error(error, "failed to resolve buffered row start");
    return false;
  }
  const auto row_for = [&](std::size_t index) {
    return static_cast<idydb_column_row_sizing>(start_row + index);
  };
  const idydb_column_row_sizing existing_next_row =
      idydb_column_next_row(&db_, kColRecordKind);
  if (start_row == 2 && existing_next_row == 2) {
    std::vector<idydb_bulk_cell> cells;
    cells.reserve(23 + buffered_rows_.size() * 25);
    std::vector<std::string> owned_header_text;
    owned_header_text.reserve(23);
    const auto append_header_text = [&](idydb_column_row_sizing column) {
      const std::string value = as_text_or_empty(&db_, column, 1);
      if (value.empty()) return;
      idydb_bulk_cell cell{};
      owned_header_text.push_back(value);
      cell.column = column;
      cell.row = 1;
      cell.type = IDYDB_CHAR;
      cell.value.s = owned_header_text.back().c_str();
      cells.push_back(cell);
    };
    const auto append_text_cell = [&](idydb_column_row_sizing column,
                                      idydb_column_row_sizing row,
                                      const std::string& value) {
      if (value.empty()) return;
      idydb_bulk_cell cell{};
      cell.column = column;
      cell.row = row;
      cell.type = IDYDB_CHAR;
      cell.value.s = value.c_str();
      cells.push_back(cell);
    };
    const auto append_num_cell = [&](idydb_column_row_sizing column,
                                     idydb_column_row_sizing row,
                                     double value, bool present) {
      if (!present) return;
      idydb_bulk_cell cell{};
      cell.column = column;
      cell.row = row;
      cell.type = IDYDB_FLOAT;
      cell.value.f = static_cast<float>(value);
      cells.push_back(cell);
    };

    append_header_text(kColRecordKind);
    append_header_text(kColRecordId);
    append_header_text(kColCellId);
    append_header_text(kColContractHash);
    append_header_text(kColWaveHash);
    append_header_text(kColProfileId);
    append_header_text(kColExecutionProfileJson);
    append_header_text(kColStateTxt);
    append_header_text(kColTextA);
    append_header_text(kColTextB);
    append_header_text(kColProjectionVersion);
    append_header_text(kColTsMs);
    append_header_text(kColPayload);
    append_header_text(kColProjectionKey);
    append_header_text(kColProjectionTxt);
    append_header_text(kColProjectionKeyAux);
    append_header_text(kColProjectionTxtAux);
    append_header_text(kColStartedAtMs);
    append_header_text(kColFinishedAtMs);
    append_header_text(kColOkTxt);
    append_header_text(kColTotalSteps);
    append_header_text(kColCampaignHash);
    append_header_text(kColRunId);

    for (std::size_t i = 0; i < buffered_rows_.size(); ++i) {
      const auto row = row_for(i);
      const auto& buffered = buffered_rows_[i];
      append_text_cell(kColRecordKind, row, buffered.record_kind);
      append_text_cell(kColRecordId, row, buffered.record_id);
      append_text_cell(kColCellId, row, buffered.cell_id);
      append_text_cell(kColContractHash, row, buffered.contract_hash);
      append_text_cell(kColWaveHash, row, buffered.wave_hash);
      append_text_cell(kColProfileId, row, buffered.profile_id);
      append_text_cell(kColExecutionProfileJson, row,
                       buffered.execution_profile_json);
      append_text_cell(kColStateTxt, row, buffered.state_txt);
      append_num_cell(kColMetricNum, row, buffered.metric_num,
                      buffered.has_metric_num);
      append_text_cell(kColTextA, row, buffered.text_a);
      append_text_cell(kColTextB, row, buffered.text_b);
      append_text_cell(kColProjectionVersion, row,
                       buffered.projection_version);
      append_text_cell(kColTsMs, row, buffered.ts_ms);
      append_text_cell(kColPayload, row, buffered.payload_json);
      append_text_cell(kColProjectionKey, row, buffered.projection_key);
      append_num_cell(kColProjectionNum, row, buffered.projection_num,
                      buffered.has_projection_num);
      append_text_cell(kColProjectionTxt, row, buffered.projection_txt);
      append_text_cell(kColProjectionKeyAux, row,
                       buffered.projection_key_aux);
      append_text_cell(kColProjectionTxtAux, row,
                       buffered.projection_txt_aux);
      append_text_cell(kColStartedAtMs, row, buffered.started_at_ms);
      append_text_cell(kColFinishedAtMs, row, buffered.finished_at_ms);
      append_text_cell(kColOkTxt, row, buffered.ok_txt);
      append_text_cell(kColTotalSteps, row, buffered.total_steps);
      append_text_cell(kColCampaignHash, row, buffered.campaign_hash);
      append_text_cell(kColRunId, row, buffered.run_id);
    }

    const int rc =
        idydb_replace_with_cells(&db_, cells.data(), cells.size());
    if (rc != IDYDB_SUCCESS) {
      const char* msg = idydb_errmsg(&db_);
      set_error(error, "idydb_replace_with_cells failed: " +
                           std::string(msg ? msg : "<no error message>"));
      return false;
    }
    next_row_hint_ = static_cast<idydb_column_row_sizing>(
        start_row + buffered_rows_.size());
    buffered_rows_.clear();
    buffered_row_start_ = 0;
    return true;
  }
  const auto flush_text_column =
      [&](idydb_column_row_sizing column, auto accessor) -> bool {
    for (std::size_t i = 0; i < buffered_rows_.size(); ++i) {
      const auto& value = accessor(buffered_rows_[i]);
      if (!value.empty() && !insert_text(&db_, column, row_for(i), value, error)) {
        return false;
      }
    }
    return true;
  };
  const auto flush_num_column =
      [&](idydb_column_row_sizing column, auto accessor,
          auto present_accessor) -> bool {
    for (std::size_t i = 0; i < buffered_rows_.size(); ++i) {
      if (!present_accessor(buffered_rows_[i])) continue;
      if (!insert_num(&db_, column, row_for(i), accessor(buffered_rows_[i]), error)) {
        return false;
      }
    }
    return true;
  };

  if (!flush_text_column(kColRecordKind,
                         [](const buffered_row_t& row) -> const std::string& {
                           return row.record_kind;
                         })) {
    return false;
  }
  if (!flush_text_column(kColRecordId,
                         [](const buffered_row_t& row) -> const std::string& {
                           return row.record_id;
                         })) {
    return false;
  }
  if (!flush_text_column(kColCellId,
                         [](const buffered_row_t& row) -> const std::string& {
                           return row.cell_id;
                         })) {
    return false;
  }
  if (!flush_text_column(kColContractHash,
                         [](const buffered_row_t& row) -> const std::string& {
                           return row.contract_hash;
                         })) {
    return false;
  }
  if (!flush_text_column(kColWaveHash,
                         [](const buffered_row_t& row) -> const std::string& {
                           return row.wave_hash;
                         })) {
    return false;
  }
  if (!flush_text_column(kColProfileId,
                         [](const buffered_row_t& row) -> const std::string& {
                           return row.profile_id;
                         })) {
    return false;
  }
  if (!flush_text_column(
          kColExecutionProfileJson,
          [](const buffered_row_t& row) -> const std::string& {
            return row.execution_profile_json;
          })) {
    return false;
  }
  if (!flush_text_column(kColStateTxt,
                         [](const buffered_row_t& row) -> const std::string& {
                           return row.state_txt;
                         })) {
    return false;
  }
  if (!flush_num_column(
          kColMetricNum,
          [](const buffered_row_t& row) { return row.metric_num; },
          [](const buffered_row_t& row) { return row.has_metric_num; })) {
    return false;
  }
  if (!flush_text_column(kColTextA,
                         [](const buffered_row_t& row) -> const std::string& {
                           return row.text_a;
                         })) {
    return false;
  }
  if (!flush_text_column(kColTextB,
                         [](const buffered_row_t& row) -> const std::string& {
                           return row.text_b;
                         })) {
    return false;
  }
  if (!flush_text_column(
          kColProjectionVersion,
          [](const buffered_row_t& row) -> const std::string& {
            return row.projection_version;
          })) {
    return false;
  }
  if (!flush_text_column(kColTsMs,
                         [](const buffered_row_t& row) -> const std::string& {
                           return row.ts_ms;
                         })) {
    return false;
  }
  if (!flush_text_column(kColPayload,
                         [](const buffered_row_t& row) -> const std::string& {
                           return row.payload_json;
                         })) {
    return false;
  }
  if (!flush_text_column(
          kColProjectionKey,
          [](const buffered_row_t& row) -> const std::string& {
            return row.projection_key;
          })) {
    return false;
  }
  if (!flush_num_column(
          kColProjectionNum,
          [](const buffered_row_t& row) { return row.projection_num; },
          [](const buffered_row_t& row) { return row.has_projection_num; })) {
    return false;
  }
  if (!flush_text_column(
          kColProjectionTxt,
          [](const buffered_row_t& row) -> const std::string& {
            return row.projection_txt;
          })) {
    return false;
  }
  if (!flush_text_column(
          kColProjectionKeyAux,
          [](const buffered_row_t& row) -> const std::string& {
            return row.projection_key_aux;
          })) {
    return false;
  }
  if (!flush_text_column(
          kColProjectionTxtAux,
          [](const buffered_row_t& row) -> const std::string& {
            return row.projection_txt_aux;
          })) {
    return false;
  }
  if (!flush_text_column(
          kColStartedAtMs,
          [](const buffered_row_t& row) -> const std::string& {
            return row.started_at_ms;
          })) {
    return false;
  }
  if (!flush_text_column(
          kColFinishedAtMs,
          [](const buffered_row_t& row) -> const std::string& {
            return row.finished_at_ms;
          })) {
    return false;
  }
  if (!flush_text_column(kColOkTxt,
                         [](const buffered_row_t& row) -> const std::string& {
                           return row.ok_txt;
                         })) {
    return false;
  }
  if (!flush_text_column(
          kColTotalSteps,
          [](const buffered_row_t& row) -> const std::string& {
            return row.total_steps;
          })) {
    return false;
  }
  if (!flush_text_column(
          kColCampaignHash,
          [](const buffered_row_t& row) -> const std::string& {
            return row.campaign_hash;
          })) {
    return false;
  }
  if (!flush_text_column(kColRunId,
                         [](const buffered_row_t& row) -> const std::string& {
                           return row.run_id;
                         })) {
    return false;
  }

  buffered_rows_.clear();
  buffered_row_start_ = 0;
  return true;
}

bool lattice_catalog_store_t::record_cell_projection_(std::string_view cell_id,
                                                    const wave_projection_t& projection,
                                                    std::uint64_t ts_ms,
                                                    std::string* error) {
  const std::string ts = std::to_string(ts_ms);
  const std::string pv = std::to_string(projection.projection_version);
  std::string projection_lls = trim_ascii(projection.projection_lls);
  if (projection_lls.empty()) {
    projection_lls = build_projection_lls_payload(projection);
  }
  for (const auto& [k, v] : projection.projection_num) {
    const std::string rec_id = std::string(cell_id) + "|projection_num|" + k;
    if (!append_row_(cuwacunu::hero::schema::kRecordKindPROJECTION_NUM, rec_id, cell_id, "", "", "", "", "",
                     std::numeric_limits<double>::quiet_NaN(), "", "", pv, ts,
                     "", k, v, "", "", "", "", "", "", "", "", "",
                     error)) {
      return false;
    }
  }
  for (const auto& [k, v] : projection.projection_txt) {
    const std::string rec_id = std::string(cell_id) + "|projection_txt|" + k;
    if (!append_row_(cuwacunu::hero::schema::kRecordKindPROJECTION_TXT, rec_id,
                     cell_id, "", "", "", "", "",
                     std::numeric_limits<double>::quiet_NaN(), "", "", pv, ts,
                     "", k, std::numeric_limits<double>::quiet_NaN(), v, "", "",
                     "", "", "", "", "", "", error)) {
      return false;
    }
  }

  const std::string rec_id = std::string(cell_id) + "|projection_meta";
  if (!append_row_(cuwacunu::hero::schema::kRecordKindPROJECTION_META, rec_id, cell_id, "", "", "", "", "",
                   std::numeric_limits<double>::quiet_NaN(),
                   projection.projector_build_id, "", pv, ts, projection_lls, "",
                   std::numeric_limits<double>::quiet_NaN(), "", "", "", "",
                   "", "", "", "", "", error)) {
    return false;
  }
  return true;
}

std::string lattice_catalog_store_t::coord_profile_key_(
    std::string_view contract_hash,
    std::string_view wave_hash,
    std::string_view profile_id) {
  std::string out;
  out.reserve(contract_hash.size() + wave_hash.size() + profile_id.size() + 2);
  out.append(contract_hash);
  out.push_back('|');
  out.append(wave_hash);
  out.push_back('|');
  out.append(profile_id);
  return out;
}

bool lattice_catalog_store_t::record_trial(const wave_cell_coord_t& coord,
                                           const wave_execution_profile_t& profile,
                                           const wave_trial_t& trial,
                                           const lattice_cell_report_t& report,
                                           const wave_projection_t& projection,
                                           wave_cell_t* out_cell,
                                           std::string* error) {
  clear_error(error);
  if (!db_) {
    set_error(error, "catalog is not open");
    return false;
  }

  const std::string profile_id = compute_profile_id(profile);
  const std::string cell_id = compute_cell_id(coord.contract_hash, coord.wave_hash,
                                              profile);
  const std::string coord_key = coord_profile_key_(
      coord.contract_hash, coord.wave_hash, profile_id);

  const std::uint64_t ts_now = now_ms_utc();
  wave_cell_t cell{};
  if (const auto it = cells_by_id_.find(cell_id); it != cells_by_id_.end()) {
    cell = it->second;
  } else {
    cell.cell_id = cell_id;
    cell.coord = coord;
    cell.execution_profile = profile;
    cell.created_at_ms = ts_now;
    cell.projection_version = projection.projection_version;
  }

  wave_trial_t stored_trial = trial;
  if (stored_trial.trial_id.empty()) {
    std::string payload = cell_id + "|trial|" + std::to_string(ts_now);
    if (!sha256_hex_bytes(payload, &stored_trial.trial_id, error)) return false;
  }
  stored_trial.cell_id = cell_id;
  if (stored_trial.started_at_ms == 0) stored_trial.started_at_ms = ts_now;
  if (stored_trial.finished_at_ms == 0) {
    stored_trial.finished_at_ms = std::max(stored_trial.started_at_ms, ts_now);
  }

  std::string report_payload;
  if (!encode_cell_report_payload(report, &report_payload, error)) {
    return false;
  }

  cell.execution_profile = profile;
  cell.state = stored_trial.ok ? "ready" : "error";
  cell.trial_count = cell.trial_count + 1;
  cell.last_trial_id = stored_trial.trial_id;
  cell.report = report;
  cell.projection_version = projection.projection_version;
  cell.updated_at_ms = std::max(stored_trial.finished_at_ms, ts_now);

  const std::string execution_profile_json =
      canonical_execution_profile_json(profile);
  const std::string pv = std::to_string(cell.projection_version);

  if (!append_row_(cuwacunu::hero::schema::kRecordKindTRIAL, stored_trial.trial_id, cell_id, coord.contract_hash,
                   coord.wave_hash, profile_id, execution_profile_json,
                   stored_trial.ok ? "ok" : "error",
                   static_cast<double>(stored_trial.total_steps),
                   stored_trial.error, stored_trial.state_snapshot_id, pv,
                   std::to_string(stored_trial.started_at_ms), report_payload, "",
                   std::numeric_limits<double>::quiet_NaN(), "", "", "",
                   std::to_string(stored_trial.started_at_ms),
                   std::to_string(stored_trial.finished_at_ms),
                   stored_trial.ok ? "1" : "0",
                   std::to_string(stored_trial.total_steps),
                   stored_trial.campaign_hash,
                   stored_trial.run_id, error)) {
    return false;
  }

  if (!append_row_(cuwacunu::hero::schema::kRecordKindCELL, cell_id, cell_id, coord.contract_hash, coord.wave_hash,
                   profile_id, execution_profile_json, cell.state,
                   static_cast<double>(cell.trial_count),
                   report.report_schema,
                   report.report_sha256,
                   pv, std::to_string(cell.updated_at_ms), report_payload, "",
                   std::numeric_limits<double>::quiet_NaN(), "", "", "", "",
                   "", "", "", "", "", error)) {
    return false;
  }

  if (!record_cell_projection_(cell_id, projection, cell.updated_at_ms, error)) {
    return false;
  }

  cells_by_id_[cell_id] = cell;
  cell_id_by_coord_profile_[coord_key] = cell_id;
  trials_by_cell_[cell_id].push_back(stored_trial);
  report_by_trial_id_[stored_trial.trial_id] = report;

  auto& projection_num = projection_num_by_cell_[cell_id];
  for (const auto& [k, v] : projection.projection_num) {
    projection_num[k] = v;
  }
  auto& projection_txt = projection_txt_by_cell_[cell_id];
  for (const auto& [k, v] : projection.projection_txt) {
    projection_txt[k] = v;
  }

  if (out_cell) *out_cell = std::move(cell);
  return true;
}

