bool hashimyei_catalog_store_t::open(const options_t &options,
                                     std::string *error) {
  clear_error(error);
  const std::uint64_t open_started_at_ms = now_ms_utc();
  const auto log_open_finish = [&](bool ok,
                                   std::string_view error_message = {}) {
    std::ostringstream extra;
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "catalog_path", options.catalog_path.string());
    cuwacunu::hero::mcp_observability::append_json_bool_field(
        extra, "encrypted", options.encrypted);
    cuwacunu::hero::mcp_observability::append_json_bool_field(extra, "ok", ok);
    cuwacunu::hero::mcp_observability::append_json_uint64_field(
        extra, "duration_ms", now_ms_utc() - open_started_at_ms);
    if (!error_message.empty()) {
      cuwacunu::hero::mcp_observability::append_json_string_field(
          extra, "error", error_message);
    }
    log_hashimyei_catalog_event("catalog_open_finish", extra.str());
  };
  {
    std::ostringstream extra;
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "catalog_path", options.catalog_path.string());
    cuwacunu::hero::mcp_observability::append_json_bool_field(
        extra, "encrypted", options.encrypted);
    log_hashimyei_catalog_event("catalog_open_begin", extra.str());
  }
  if (db_ != nullptr) {
    set_error(error, "catalog already open");
    log_open_finish(false, "catalog already open");
    return false;
  }

  std::error_code ec;
  const auto parent = options.catalog_path.parent_path();
  if (!parent.empty()) {
    std::filesystem::create_directories(parent, ec);
    if (ec) {
      set_error(error, "cannot create catalog directory: " + parent.string());
      log_open_finish(false, error && !error->empty() ? std::string_view(*error)
                                                      : std::string_view{});
      return false;
    }
  }

  const auto remove_catalog_for_recovery = [&](std::string *out_error) -> bool {
    if (out_error)
      out_error->clear();
    std::error_code rm_ec{};
    std::filesystem::remove(options.catalog_path, rm_ec);
    if (rm_ec && std::filesystem::exists(options.catalog_path)) {
      set_error(out_error, "cannot remove damaged catalog: " +
                               options.catalog_path.string());
      return false;
    }
    return true;
  };

  for (int recovery_attempt = 0; recovery_attempt < 2; ++recovery_attempt) {
    int rc = IDYDB_ERROR;
    for (int attempt = 0; attempt <= kCatalogOpenBusyRetryCount; ++attempt) {
      if (options.encrypted) {
        if (options.passphrase.empty()) {
          set_error(error, "encrypted catalog requires passphrase");
          log_open_finish(false, error && !error->empty()
                                     ? std::string_view(*error)
                                     : std::string_view{});
          return false;
        }
        rc = idydb_open_encrypted(options.catalog_path.string().c_str(), &db_,
                                  IDYDB_CREATE, options.passphrase.c_str());
      } else {
        rc = idydb_open(options.catalog_path.string().c_str(), &db_,
                        IDYDB_CREATE);
      }

      if (rc == IDYDB_SUCCESS && db_ != nullptr)
        break;
      if (db_) {
        (void)idydb_close(&db_);
        db_ = nullptr;
      }

      if (rc != IDYDB_BUSY || attempt >= kCatalogOpenBusyRetryCount)
        break;
      std::this_thread::sleep_for(kCatalogOpenBusyRetryDelay);
    }
    if (rc != IDYDB_SUCCESS || db_ == nullptr) {
      const char *msg = db_ ? idydb_errmsg(&db_) : nullptr;
      std::string detail = (msg && msg[0] != '\0')
                               ? std::string(msg)
                               : ("idydb rc=" + std::to_string(rc) + " (" +
                                  idydb_rc_label(rc) + ")");
      if (rc == IDYDB_BUSY) {
        detail += "; catalog lock is held by another process";
      }
      if (db_) {
        (void)idydb_close(&db_);
        db_ = nullptr;
      }
      if (recovery_attempt == 0 && remove_catalog_for_recovery(nullptr)) {
        continue;
      }
      set_error(error, "cannot open catalog: " + detail);
      log_open_finish(false, error && !error->empty() ? std::string_view(*error)
                                                      : std::string_view{});
      return false;
    }

    options_ = options;
    if (ensure_catalog_header_(error) && rebuild_indexes(error)) {
      opened_at_ms_ = now_ms_utc();
      log_open_finish(true);
      return true;
    }

    std::string ignored;
    (void)close(&ignored);
    if (recovery_attempt == 0 && remove_catalog_for_recovery(nullptr)) {
      continue;
    }
    log_open_finish(false, error && !error->empty() ? std::string_view(*error)
                                                    : std::string_view{});
    return false;
  }

  set_error(error, "cannot open catalog after recovery attempts");
  log_open_finish(false, error && !error->empty() ? std::string_view(*error)
                                                  : std::string_view{});
  return false;
}

bool hashimyei_catalog_store_t::close(std::string *error) {
  clear_error(error);
  if (!db_)
    return true;
  const std::uint64_t close_started_at_ms = now_ms_utc();
  const std::uint64_t opened_at_ms = opened_at_ms_;
  {
    std::ostringstream extra;
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "catalog_path", options_.catalog_path.string());
    log_hashimyei_catalog_event("catalog_close_begin", extra.str());
  }
  const int rc = idydb_close(&db_);
  db_ = nullptr;
  if (rc != IDYDB_DONE) {
    opened_at_ms_ = 0;
    set_error(error, "idydb_close failed");
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
    log_hashimyei_catalog_event("catalog_close_finish", extra.str());
    return false;
  }
  opened_at_ms_ = 0;
  next_row_hint_ = 0;
  runs_by_id_.clear();
  run_ids_.clear();
  ledger_report_fragment_ids_.clear();
  report_fragments_by_id_.clear();
  latest_report_fragment_by_key_.clear();
  metrics_num_by_report_fragment_.clear();
  metrics_txt_by_report_fragment_.clear();
  components_by_id_.clear();
  latest_component_by_canonical_.clear();
  latest_component_by_hashimyei_.clear();
  active_hashimyei_by_key_.clear();
  active_component_by_key_.clear();
  active_component_by_canonical_.clear();
  dependency_files_by_run_id_.clear();
  kind_counters_.clear();
  hash_identity_by_kind_sha_.clear();
  explicit_family_rank_by_scope_.clear();
  {
    const std::uint64_t closed_at_ms = now_ms_utc();
    std::ostringstream extra;
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "catalog_path", options_.catalog_path.string());
    cuwacunu::hero::mcp_observability::append_json_bool_field(extra, "ok",
                                                              true);
    cuwacunu::hero::mcp_observability::append_json_uint64_field(
        extra, "duration_ms", closed_at_ms - close_started_at_ms);
    if (opened_at_ms != 0) {
      cuwacunu::hero::mcp_observability::append_json_uint64_field(
          extra, "catalog_hold_ms", closed_at_ms - opened_at_ms);
    }
    log_hashimyei_catalog_event("catalog_close_finish", extra.str());
  }
  return true;
}

bool hashimyei_catalog_store_t::ensure_catalog_header_(std::string *error) {
  clear_error(error);
  if (!db_) {
    set_error(error, "catalog is not open");
    return false;
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
  if (!db::query::exists_row(&db_, {t_kind, t_id}, &exists, error))
    return false;
  if (exists) {
    const idydb_column_row_sizing next =
        idydb_column_next_row(&db_, kColRecordKind);
    for (idydb_column_row_sizing row = 1; row < next; ++row) {
      const std::string row_kind = as_text_or_empty(&db_, kColRecordKind, row);
      const std::string row_id = as_text_or_empty(&db_, kColRecordId, row);
      if (row_kind != cuwacunu::hero::schema::kRecordKindHEADER ||
          row_id != "catalog_schema_version") {
        continue;
      }
      const std::string version =
          trim_ascii(as_text_or_empty(&db_, kColMetricTxt, row));
      if (version != "2") {
        set_error(error, "unsupported catalog schema version '" + version +
                             "'; expected strict v2");
        return false;
      }
      return true;
    }
  }

  return append_row_(
      cuwacunu::hero::schema::kRecordKindHEADER, "catalog_schema_version", "",
      "", "", cuwacunu::hashimyei::kCatalogSchemaV2, "catalog_schema_version",
      std::numeric_limits<double>::quiet_NaN(), "2", "",
      options_.catalog_path.string(), std::to_string(now_ms_utc()), "{}",
      error);
}

bool hashimyei_catalog_store_t::append_row_(
    std::string_view record_kind, std::string_view record_id,
    std::string_view run_id, std::string_view canonical_path,
    std::string_view hashimyei, std::string_view schema,
    std::string_view metric_key, double metric_num, std::string_view metric_txt,
    std::string_view report_fragment_sha256, std::string_view path,
    std::string_view ts_ms, std::string_view payload_json, std::string *error) {
  clear_error(error);
  if (!db_) {
    set_error(error, "catalog is not open");
    return false;
  }
  if (!cuwacunu::hero::schema::is_known_record_kind(record_kind)) {
    set_error(error,
              "unknown catalog record_kind: " + std::string(record_kind));
    return false;
  }
  const idydb_column_row_sizing row =
      buffer_rows_
          ? (buffered_row_start_ != 0
                 ? static_cast<idydb_column_row_sizing>(buffered_row_start_ +
                                                        buffered_rows_.size())
                 : ((buffered_row_start_ =
                         (next_row_hint_ != 0)
                             ? next_row_hint_
                             : idydb_column_next_row(&db_, kColRecordKind)),
                    buffered_row_start_))
          : ((next_row_hint_ != 0)
                 ? next_row_hint_
                 : idydb_column_next_row(&db_, kColRecordKind));
  if (row == 0) {
    set_error(error, "failed to resolve next row");
    return false;
  }
  const std::string canonical_key =
      normalize_hashimyei_canonical_path_or_same(canonical_path);
  const std::string hash_key = normalize_hashimyei_name_or_same(hashimyei);
  if (buffer_rows_) {
    buffered_row_t buffered{};
    buffered.record_kind = std::string(record_kind);
    buffered.record_id = std::string(record_id);
    buffered.run_id = std::string(run_id);
    buffered.canonical_path = canonical_key;
    buffered.hashimyei = hash_key;
    buffered.schema = std::string(schema);
    buffered.metric_key = std::string(metric_key);
    buffered.metric_num = metric_num;
    buffered.has_metric_num = std::isfinite(metric_num);
    buffered.metric_txt = std::string(metric_txt);
    buffered.report_fragment_sha256 = std::string(report_fragment_sha256);
    buffered.path = std::string(path);
    buffered.ts_ms = std::string(ts_ms);
    buffered.payload_json = std::string(payload_json);
    buffered_rows_.push_back(std::move(buffered));
    next_row_hint_ = row + 1;
    return true;
  }
  const auto insert_text_if_present = [&](idydb_column_row_sizing column,
                                          std::string_view value) {
    return value.empty() || insert_text(&db_, column, row, value, error);
  };
  const auto insert_num_if_present = [&](idydb_column_row_sizing column,
                                         double value) {
    return std::isnan(value) || insert_num(&db_, column, row, value, error);
  };
  if (!insert_text_if_present(kColRecordKind, record_kind))
    return false;
  if (!insert_text_if_present(kColRecordId, record_id))
    return false;
  if (!insert_text_if_present(kColRunId, run_id))
    return false;
  if (!insert_text_if_present(kColCanonicalPath, canonical_key))
    return false;
  if (!insert_text_if_present(kColHashimyei, hash_key))
    return false;
  if (!insert_text_if_present(kColSchema, schema))
    return false;
  if (!insert_text_if_present(kColMetricKey, metric_key))
    return false;
  if (!insert_num_if_present(kColMetricNum, metric_num))
    return false;
  if (!insert_text_if_present(kColMetricTxt, metric_txt))
    return false;
  if (!insert_text_if_present(kColReportFragmentSha256, report_fragment_sha256))
    return false;
  if (!insert_text_if_present(kColPath, path))
    return false;
  if (!insert_text_if_present(kColTsMs, ts_ms))
    return false;
  if (!insert_text_if_present(kColPayload, payload_json))
    return false;
  next_row_hint_ = row + 1;
  return true;
}

bool hashimyei_catalog_store_t::flush_buffered_rows_(std::string *error) {
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
      (buffered_row_start_ != 0)
          ? buffered_row_start_
          : ((next_row_hint_ != 0)
                 ? static_cast<idydb_column_row_sizing>(next_row_hint_ -
                                                        buffered_rows_.size())
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
    cells.reserve(12 + buffered_rows_.size() * 13);
    std::vector<std::string> owned_header_text;
    owned_header_text.reserve(12);
    const auto append_header_text = [&](idydb_column_row_sizing column) {
      const std::string value = as_text_or_empty(&db_, column, 1);
      if (value.empty())
        return;
      owned_header_text.push_back(value);
      idydb_bulk_cell cell{};
      cell.column = column;
      cell.row = 1;
      cell.type = IDYDB_CHAR;
      cell.value.s = owned_header_text.back().c_str();
      cells.push_back(cell);
    };
    const auto append_text_cell = [&](idydb_column_row_sizing column,
                                      idydb_column_row_sizing row,
                                      const std::string &value) {
      if (value.empty())
        return;
      idydb_bulk_cell cell{};
      cell.column = column;
      cell.row = row;
      cell.type = IDYDB_CHAR;
      cell.value.s = value.c_str();
      cells.push_back(cell);
    };
    const auto append_num_cell = [&](idydb_column_row_sizing column,
                                     idydb_column_row_sizing row, double value,
                                     bool present) {
      if (!present)
        return;
      idydb_bulk_cell cell{};
      cell.column = column;
      cell.row = row;
      cell.type = IDYDB_FLOAT;
      cell.value.f = static_cast<float>(value);
      cells.push_back(cell);
    };

    append_header_text(kColRecordKind);
    append_header_text(kColRecordId);
    append_header_text(kColRunId);
    append_header_text(kColCanonicalPath);
    append_header_text(kColHashimyei);
    append_header_text(kColSchema);
    append_header_text(kColMetricKey);
    append_header_text(kColMetricTxt);
    append_header_text(kColReportFragmentSha256);
    append_header_text(kColPath);
    append_header_text(kColTsMs);
    append_header_text(kColPayload);

    for (std::size_t i = 0; i < buffered_rows_.size(); ++i) {
      const auto row = row_for(i);
      const auto &buffered = buffered_rows_[i];
      append_text_cell(kColRecordKind, row, buffered.record_kind);
      append_text_cell(kColRecordId, row, buffered.record_id);
      append_text_cell(kColRunId, row, buffered.run_id);
      append_text_cell(kColCanonicalPath, row, buffered.canonical_path);
      append_text_cell(kColHashimyei, row, buffered.hashimyei);
      append_text_cell(kColSchema, row, buffered.schema);
      append_text_cell(kColMetricKey, row, buffered.metric_key);
      append_num_cell(kColMetricNum, row, buffered.metric_num,
                      buffered.has_metric_num);
      append_text_cell(kColMetricTxt, row, buffered.metric_txt);
      append_text_cell(kColReportFragmentSha256, row,
                       buffered.report_fragment_sha256);
      append_text_cell(kColPath, row, buffered.path);
      append_text_cell(kColTsMs, row, buffered.ts_ms);
      append_text_cell(kColPayload, row, buffered.payload_json);
    }

    const int rc = idydb_replace_with_cells(&db_, cells.data(), cells.size());
    if (rc != IDYDB_SUCCESS) {
      const char *msg = idydb_errmsg(&db_);
      set_error(error, "idydb_replace_with_cells failed: " +
                           std::string(msg ? msg : "<no error message>"));
      return false;
    }
    next_row_hint_ =
        static_cast<idydb_column_row_sizing>(start_row + buffered_rows_.size());
    buffered_rows_.clear();
    buffered_row_start_ = 0;
    return true;
  }
  const auto flush_text_column = [&](idydb_column_row_sizing column,
                                     auto accessor) -> bool {
    for (std::size_t i = 0; i < buffered_rows_.size(); ++i) {
      const auto &value = accessor(buffered_rows_[i]);
      if (!value.empty() &&
          !insert_text(&db_, column, row_for(i), value, error)) {
        return false;
      }
    }
    return true;
  };
  const auto flush_num_column = [&](idydb_column_row_sizing column,
                                    auto accessor,
                                    auto present_accessor) -> bool {
    for (std::size_t i = 0; i < buffered_rows_.size(); ++i) {
      if (!present_accessor(buffered_rows_[i]))
        continue;
      if (!insert_num(&db_, column, row_for(i), accessor(buffered_rows_[i]),
                      error)) {
        return false;
      }
    }
    return true;
  };

  if (!flush_text_column(kColRecordKind,
                         [](const buffered_row_t &row) -> const std::string & {
                           return row.record_kind;
                         })) {
    return false;
  }
  if (!flush_text_column(kColRecordId,
                         [](const buffered_row_t &row) -> const std::string & {
                           return row.record_id;
                         })) {
    return false;
  }
  if (!flush_text_column(kColRunId,
                         [](const buffered_row_t &row) -> const std::string & {
                           return row.run_id;
                         })) {
    return false;
  }
  if (!flush_text_column(kColCanonicalPath,
                         [](const buffered_row_t &row) -> const std::string & {
                           return row.canonical_path;
                         })) {
    return false;
  }
  if (!flush_text_column(kColHashimyei,
                         [](const buffered_row_t &row) -> const std::string & {
                           return row.hashimyei;
                         })) {
    return false;
  }
  if (!flush_text_column(kColSchema,
                         [](const buffered_row_t &row) -> const std::string & {
                           return row.schema;
                         })) {
    return false;
  }
  if (!flush_text_column(kColMetricKey,
                         [](const buffered_row_t &row) -> const std::string & {
                           return row.metric_key;
                         })) {
    return false;
  }
  if (!flush_num_column(
          kColMetricNum,
          [](const buffered_row_t &row) { return row.metric_num; },
          [](const buffered_row_t &row) { return row.has_metric_num; })) {
    return false;
  }
  if (!flush_text_column(kColMetricTxt,
                         [](const buffered_row_t &row) -> const std::string & {
                           return row.metric_txt;
                         })) {
    return false;
  }
  if (!flush_text_column(kColReportFragmentSha256,
                         [](const buffered_row_t &row) -> const std::string & {
                           return row.report_fragment_sha256;
                         })) {
    return false;
  }
  if (!flush_text_column(kColPath,
                         [](const buffered_row_t &row) -> const std::string & {
                           return row.path;
                         })) {
    return false;
  }
  if (!flush_text_column(kColTsMs,
                         [](const buffered_row_t &row) -> const std::string & {
                           return row.ts_ms;
                         })) {
    return false;
  }
  if (!flush_text_column(kColPayload,
                         [](const buffered_row_t &row) -> const std::string & {
                           return row.payload_json;
                         })) {
    return false;
  }

  buffered_rows_.clear();
  buffered_row_start_ = 0;
  return true;
}

bool hashimyei_catalog_store_t::ledger_contains_(
    std::string_view report_fragment_sha256, bool *out_exists,
    std::string *error) {
  clear_error(error);
  if (!out_exists) {
    set_error(error, "out_exists is null");
    return false;
  }
  *out_exists = false;
  if (!db_) {
    set_error(error, "catalog is not open");
    return false;
  }
  if (report_fragment_sha256.empty())
    return true;
  *out_exists = ledger_report_fragment_ids_.count(
                    std::string(report_fragment_sha256)) != 0;
  return true;
}

bool hashimyei_catalog_store_t::append_ledger_(
    std::string_view report_fragment_sha256, std::string_view path,
    std::string *error) {
  if (!append_row_(cuwacunu::hero::schema::kRecordKindLEDGER,
                   std::string(report_fragment_sha256), "", "", "", "",
                   "ingest_version",
                   static_cast<double>(options_.ingest_version), "",
                   report_fragment_sha256, path, std::to_string(now_ms_utc()),
                   "{}", error)) {
    return false;
  }
  ledger_report_fragment_ids_.insert(std::string(report_fragment_sha256));
  return true;
}

bool hashimyei_catalog_store_t::append_kind_counter_(
    cuwacunu::hashimyei::hashimyei_kind_e kind, std::uint64_t next_value,
    std::string *error) {
  return append_row_(cuwacunu::hero::schema::kRecordKindCOUNTER,
                     cuwacunu::hashimyei::hashimyei_kind_to_string(kind), "",
                     "", "", cuwacunu::hashimyei::kCounterSchemaV2,
                     "next_ordinal", static_cast<double>(next_value),
                     std::to_string(next_value), "", "",
                     std::to_string(now_ms_utc()), "{}", error);
}

bool hashimyei_catalog_store_t::reserve_next_ordinal_(
    cuwacunu::hashimyei::hashimyei_kind_e kind, std::uint64_t *out_ordinal,
    std::string *error) {
  clear_error(error);
  if (!out_ordinal) {
    set_error(error, "out_ordinal is null");
    return false;
  }
  const int key = static_cast<int>(kind);
  const auto it = kind_counters_.find(key);
  const std::uint64_t next = (it == kind_counters_.end()) ? 0 : it->second;
  const std::uint64_t next_value = next + 1;
  *out_ordinal = next;
  kind_counters_[key] = next_value;
  if (!append_kind_counter_(kind, next_value, error))
    return false;
  return true;
}

bool hashimyei_catalog_store_t::ensure_identity_allocated_(
    cuwacunu::hashimyei::hashimyei_t *identity, std::string *error) {
  clear_error(error);
  if (!identity) {
    set_error(error, "identity pointer is null");
    return false;
  }
  if (identity->schema.empty())
    identity->schema = cuwacunu::hashimyei::kIdentitySchemaV2;
  const bool requires_sha =
      cuwacunu::hashimyei::hashimyei_kind_requires_sha256(identity->kind);

  if (requires_sha && identity->hash_sha256_hex.empty()) {
    set_error(error,
              "identity.hash_sha256_hex is required for this identity kind");
    return false;
  }

  const std::string identity_key =
      requires_sha ? kind_sha_key(identity->kind, identity->hash_sha256_hex)
                   : std::string{};
  if (!identity_key.empty()) {
    const auto it_known = hash_identity_by_kind_sha_.find(identity_key);
    if (it_known != hash_identity_by_kind_sha_.end()) {
      const auto &known = it_known->second;
      if (identity->name.empty())
        identity->name = known.name;
      if (identity->ordinal == 0)
        identity->ordinal = known.ordinal;
      if (!identity->name.empty() && identity->name != known.name) {
        set_error(error, "identity.name conflicts with existing hashimyei "
                         "alias for kind/hash");
        return false;
      }
      if (identity->ordinal != 0 && identity->ordinal != known.ordinal) {
        set_error(error, "identity.ordinal conflicts with existing hashimyei "
                         "alias for kind/hash");
        return false;
      }
    }
  }

  if (identity->name.empty()) {
    std::uint64_t ordinal = identity->ordinal;
    if (ordinal == 0) {
      if (!reserve_next_ordinal_(identity->kind, &ordinal, error))
        return false;
    }
    identity->ordinal = ordinal;
    identity->name = cuwacunu::hashimyei::make_hex_hash_name(ordinal);
  } else {
    std::uint64_t parsed = 0;
    if (!cuwacunu::hashimyei::parse_hex_hash_name_ordinal(identity->name,
                                                          &parsed)) {
      set_error(error, "identity.name is not a valid 0x... hex ordinal");
      return false;
    }
    if (identity->ordinal == 0)
      identity->ordinal = parsed;
    if (identity->ordinal != parsed) {
      set_error(error, "identity.ordinal does not match identity.name");
      return false;
    }
  }

  std::string validate_error;
  if (!cuwacunu::hashimyei::validate_hashimyei(*identity, &validate_error)) {
    set_error(error, "identity validation failed: " + validate_error);
    return false;
  }

  if (!identity_key.empty()) {
    const auto it_known = hash_identity_by_kind_sha_.find(identity_key);
    if (it_known == hash_identity_by_kind_sha_.end()) {
      hash_identity_by_kind_sha_.emplace(identity_key, *identity);
    } else if (it_known->second.name != identity->name ||
               it_known->second.ordinal != identity->ordinal) {
      set_error(error, "identity alias collision for kind/hash");
      return false;
    }
  }

  const int key = static_cast<int>(identity->kind);
  const std::uint64_t observed_next = identity->ordinal + 1;
  const auto it = kind_counters_.find(key);
  if (it == kind_counters_.end() || observed_next > it->second) {
    kind_counters_[key] = observed_next;
    if (!append_kind_counter_(identity->kind, observed_next, error))
      return false;
  }
  return true;
}

void hashimyei_catalog_store_t::observe_identity_(
    const cuwacunu::hashimyei::hashimyei_t &identity) {
  if (identity.name.empty())
    return;

  const int key = static_cast<int>(identity.kind);
  const std::uint64_t observed_next = identity.ordinal + 1;
  const auto it_counter = kind_counters_.find(key);
  if (it_counter == kind_counters_.end() ||
      observed_next > it_counter->second) {
    kind_counters_[key] = observed_next;
  }

  if (!cuwacunu::hashimyei::hashimyei_kind_requires_sha256(identity.kind) ||
      identity.hash_sha256_hex.empty()) {
    return;
  }

  const std::string identity_key =
      kind_sha_key(identity.kind, identity.hash_sha256_hex);
  const auto it_known = hash_identity_by_kind_sha_.find(identity_key);
  if (it_known == hash_identity_by_kind_sha_.end() ||
      identity.ordinal < it_known->second.ordinal) {
    hash_identity_by_kind_sha_[identity_key] = identity;
  }
}

void hashimyei_catalog_store_t::observe_component_(
    const component_state_t &component) {
  if (component.component_id.empty())
    return;

  observe_identity_(component.manifest.hashimyei_identity);
  if (component.manifest.parent_identity.has_value()) {
    observe_identity_(*component.manifest.parent_identity);
  }
  observe_identity_(component.manifest.contract_identity);

  components_by_id_[component.component_id] = component;

  const auto pick_latest =
      [&](std::unordered_map<std::string, std::string> *map,
          const std::string &key) {
        if (!map || key.empty())
          return;
        auto it = map->find(key);
        if (it == map->end()) {
          (*map)[key] = component.component_id;
          return;
        }
        const auto old_it = components_by_id_.find(it->second);
        if (old_it == components_by_id_.end()) {
          it->second = component.component_id;
          return;
        }
        if (component.ts_ms > old_it->second.ts_ms ||
            (component.ts_ms == old_it->second.ts_ms &&
             component.component_id > old_it->second.component_id)) {
          it->second = component.component_id;
        }
      };

  pick_latest(&latest_component_by_canonical_,
              component.manifest.canonical_path);
  if (!component.manifest.hashimyei_identity.name.empty()) {
    pick_latest(&latest_component_by_hashimyei_,
                component.manifest.hashimyei_identity.name);
  }
}

void hashimyei_catalog_store_t::refresh_active_component_views_() {
  active_component_by_key_.clear();
  active_component_by_canonical_.clear();

  for (const auto &[pointer_key, pointer_hashimyei] :
       active_hashimyei_by_key_) {
    std::string canonical_path{};
    std::string family{};
    std::string contract_hash{};
    if (!parse_component_active_pointer_key(pointer_key, &canonical_path,
                                            &family, &contract_hash)) {
      continue;
    }

    component_state_t best{};
    bool found = false;
    for (const auto &[_, component] : components_by_id_) {
      const std::string component_contract_hash =
          cuwacunu::hero::hashimyei::contract_hash_from_identity(
              component.manifest.contract_identity);
      const std::string component_key = component_active_pointer_key(
          component.manifest.canonical_path, component.manifest.family,
          component.manifest.hashimyei_identity.name, component_contract_hash);
      if (component_key != pointer_key)
        continue;
      if (!family.empty() && component.manifest.family != family)
        continue;
      if (!contract_hash.empty() && component_contract_hash != contract_hash) {
        continue;
      }
      if (component.manifest.hashimyei_identity.name != pointer_hashimyei) {
        continue;
      }
      if (!found || component.ts_ms > best.ts_ms ||
          (component.ts_ms == best.ts_ms &&
           component.component_id > best.component_id)) {
        best = component;
        found = true;
      }
    }
    if (!found)
      continue;
    active_component_by_key_[pointer_key] = best.component_id;
    active_component_by_canonical_[canonical_path] = best.component_id;
    active_component_by_canonical_[best.manifest.canonical_path] =
        best.component_id;
  }
}

void hashimyei_catalog_store_t::refresh_component_family_ranks_() {
  for (auto &[_, component] : components_by_id_) {
    component.family_rank.reset();
    const std::string component_compatibility_sha256_hex =
        trim_ascii(component.manifest.component_compatibility_sha256_hex);
    if (component_compatibility_sha256_hex.empty())
      continue;
    const std::string scope_key = cuwacunu::hero::family_rank::scope_key(
        component.manifest.family, component_compatibility_sha256_hex);
    const auto it_rank = explicit_family_rank_by_scope_.find(scope_key);
    if (it_rank == explicit_family_rank_by_scope_.end())
      continue;
    component.family_rank = cuwacunu::hero::family_rank::rank_for_hashimyei(
        it_rank->second, component.manifest.hashimyei_identity.name);
  }
}

bool hashimyei_catalog_store_t::ingest_run_manifest_file_(
    const std::filesystem::path &path, std::string *error) {
  clear_error(error);
  run_manifest_t m{};
  if (!load_run_manifest(path, &m, error))
    return false;

  if (run_ids_.count(m.run_id) == 0) {
    std::filesystem::path cp = path;
    if (const auto can = canonicalized(path); can.has_value())
      cp = *can;
    std::string manifest_sha;
    if (!sha256_hex_file(cp, &manifest_sha, error))
      return false;
    if (!append_row_(
            cuwacunu::hero::schema::kRecordKindRUN, m.run_id, m.run_id, "", "",
            m.schema, "", std::numeric_limits<double>::quiet_NaN(),
            m.wave_contract_binding.identity.name, manifest_sha, cp.string(),
            std::to_string(m.started_at_ms), run_manifest_payload(m), error)) {
      return false;
    }
    for (const auto &d : m.dependency_files) {
      const std::string rec_id = m.run_id + "|" + d.canonical_path;
      if (!append_row_(cuwacunu::hero::schema::kRecordKindRUN_DEPENDENCY,
                       rec_id, m.run_id, "", "", m.schema, d.canonical_path,
                       std::numeric_limits<double>::quiet_NaN(), d.sha256_hex,
                       "", cp.string(), std::to_string(m.started_at_ms), "{}",
                       error)) {
        return false;
      }
    }
    run_ids_.insert(m.run_id);
  }
  dependency_files_by_run_id_[m.run_id] = m.dependency_files;
  observe_identity_(m.campaign_identity);
  observe_identity_(m.wave_contract_binding.identity);
  observe_identity_(m.wave_contract_binding.contract);
  observe_identity_(m.wave_contract_binding.wave);
  runs_by_id_[m.run_id] = std::move(m);
  return true;
}
