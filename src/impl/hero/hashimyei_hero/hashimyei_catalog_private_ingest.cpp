bool hashimyei_catalog_store_t::ingest_component_manifest_file_(
    const std::filesystem::path &path, std::string *error) {
  clear_error(error);
  if (!std::filesystem::exists(path) ||
      !std::filesystem::is_regular_file(path)) {
    set_error(error, "component manifest path is not a regular file");
    return false;
  }

  component_manifest_t manifest{};
  if (!load_component_manifest(path, &manifest, error))
    return false;

  std::string report_fragment_sha;
  if (!sha256_hex_file(path, &report_fragment_sha, error))
    return false;
  bool already = false;
  if (!ledger_contains_(report_fragment_sha, &already, error))
    return false;
  if (already)
    return true;

  std::filesystem::path cp = path;
  if (const auto can = canonicalized(path); can.has_value())
    cp = *can;
  std::string payload;
  std::string read_error;
  if (!read_text_file(path, &payload, &read_error)) {
    set_error(error, "cannot read component manifest payload: " + read_error);
    return false;
  }

  std::uint64_t ts_ms = manifest.updated_at_ms != 0 ? manifest.updated_at_ms
                                                    : manifest.created_at_ms;
  if (ts_ms == 0)
    ts_ms = now_ms_utc();

  const std::string component_id = compute_component_manifest_id(manifest);
  if (!append_row_(cuwacunu::hero::schema::kRecordKindCOMPONENT, component_id,
                   "", manifest.canonical_path,
                   manifest.hashimyei_identity.name, manifest.schema,
                   manifest.family, std::numeric_limits<double>::quiet_NaN(),
                   manifest.lineage_state, report_fragment_sha, cp.string(),
                   std::to_string(ts_ms), payload, error)) {
    return false;
  }

  const std::string source_payload =
      "{\"component_edge_kind\":\"founding_dsl_source\"}";
  if (!append_row_(cuwacunu::hero::schema::kRecordKindCOMPONENT_LINEAGE,
                   component_id + "|founding_dsl_source", "",
                   manifest.canonical_path, manifest.hashimyei_identity.name,
                   manifest.schema, manifest.founding_dsl_source_path,
                   std::numeric_limits<double>::quiet_NaN(),
                   manifest.founding_dsl_source_sha256_hex, "", cp.string(),
                   std::to_string(ts_ms), source_payload, error)) {
    return false;
  }

  const std::filesystem::path store_root =
      store_root_from_catalog_path(options_.catalog_path);
  founding_dsl_bundle_manifest_t bundle_manifest{};
  std::string bundle_error{};
  if (read_founding_dsl_bundle_manifest(store_root, manifest.canonical_path,
                                        component_id, &bundle_manifest,
                                        &bundle_error)) {
    const std::string bundle_payload =
        std::string("{\"component_edge_kind\":\"founding_dsl_bundle\","
                    "\"founding_dsl_bundle_aggregate_sha256_hex\":\"") +
        bundle_manifest.founding_dsl_bundle_aggregate_sha256_hex + "\"}";
    if (!append_row_(
            cuwacunu::hero::schema::kRecordKindCOMPONENT_LINEAGE,
            component_id + "|founding_dsl_bundle", "", manifest.canonical_path,
            manifest.hashimyei_identity.name, manifest.schema,
            founding_dsl_bundle_manifest_path(
                store_root, manifest.canonical_path, component_id)
                .string(),
            std::numeric_limits<double>::quiet_NaN(),
            bundle_manifest.founding_dsl_bundle_aggregate_sha256_hex, "",
            cp.string(), std::to_string(ts_ms), bundle_payload, error)) {
      return false;
    }
  }

  if (!append_row_(cuwacunu::hero::schema::kRecordKindCOMPONENT_REVISION,
                   component_id + "|revision", "", manifest.canonical_path,
                   manifest.hashimyei_identity.name, manifest.schema,
                   manifest.family, std::numeric_limits<double>::quiet_NaN(),
                   manifest.parent_identity.has_value()
                       ? std::string_view(manifest.parent_identity->name)
                       : std::string_view{},
                   "", cp.string(), std::to_string(ts_ms),
                   manifest.revision_reason, error)) {
    return false;
  }

  if (trim_ascii(manifest.lineage_state) == "active") {
    const std::string contract_hash =
        cuwacunu::hero::hashimyei::contract_hash_from_identity(
            manifest.contract_identity);
    const std::string pointer_key = component_active_pointer_key(
        manifest.canonical_path, manifest.family,
        manifest.hashimyei_identity.name, contract_hash);
    std::ostringstream pointer_payload;
    pointer_payload << "canonical_path=" << manifest.canonical_path << "\n";
    pointer_payload << "family=" << manifest.family << "\n";
    pointer_payload << "component_tag=" << manifest.component_tag << "\n";
    pointer_payload << "contract_hash=" << contract_hash << "\n";
    pointer_payload << "active_hashimyei=" << manifest.hashimyei_identity.name
                    << "\n";
    pointer_payload << "component_id=" << component_id << "\n";
    pointer_payload << "docking_signature_sha256_hex="
                    << manifest.docking_signature_sha256_hex << "\n";
    pointer_payload << "component_compatibility_sha256_hex="
                    << manifest.component_compatibility_sha256_hex << "\n";
    pointer_payload << "parent_hashimyei="
                    << (manifest.parent_identity.has_value()
                            ? std::string_view(manifest.parent_identity->name)
                            : std::string_view{})
                    << "\n";
    pointer_payload << "revision_reason=" << manifest.revision_reason << "\n";
    pointer_payload << "founding_revision_id=" << manifest.founding_revision_id
                    << "\n";
    if (!append_row_(cuwacunu::hero::schema::kRecordKindCOMPONENT_ACTIVE,
                     pointer_key, "", manifest.canonical_path,
                     manifest.hashimyei_identity.name,
                     cuwacunu::hashimyei::kComponentActivePointerSchemaV2,
                     manifest.family, std::numeric_limits<double>::quiet_NaN(),
                     manifest.founding_revision_id, "", cp.string(),
                     std::to_string(ts_ms), pointer_payload.str(), error)) {
      return false;
    }
  }

  if (!append_ledger_(report_fragment_sha, cp.string(), error))
    return false;

  component_state_t component{};
  component.component_id = component_id;
  component.ts_ms = ts_ms;
  component.manifest_path = cp.string();
  component.report_fragment_sha256 = report_fragment_sha;
  component.manifest = manifest;
  observe_component_(component);

  if (trim_ascii(manifest.lineage_state) == "active") {
    const std::string contract_hash =
        cuwacunu::hero::hashimyei::contract_hash_from_identity(
            manifest.contract_identity);
    const std::string pointer_key = component_active_pointer_key(
        manifest.canonical_path, manifest.family,
        manifest.hashimyei_identity.name, contract_hash);
    active_hashimyei_by_key_[pointer_key] = manifest.hashimyei_identity.name;
  }
  return true;
}

void hashimyei_catalog_store_t::populate_metrics_from_kv_(
    std::string_view report_fragment_id,
    const std::unordered_map<std::string, std::string> &kv) {
  if (report_fragment_id.empty())
    return;
  auto &numeric =
      metrics_num_by_report_fragment_[std::string(report_fragment_id)];
  auto &text = metrics_txt_by_report_fragment_[std::string(report_fragment_id)];
  numeric.clear();
  text.clear();
  for (const auto &[k, v] : kv) {
    if (k == "schema" || k == "canonical_path" || k == "binding_id" ||
        k == "source_runtime_cursor" || k == "wave_cursor" ||
        k == "semantic_taxon" || k == "component_instance_name" ||
        k == "report_event" || k == "source_label") {
      continue;
    }

    double num = 0.0;
    if (parse_double_token(v, &num)) {
      numeric.push_back({k, num});
    } else {
      text.push_back({k, v});
    }
  }
}

bool hashimyei_catalog_store_t::ingest_report_fragment_file_(
    const std::filesystem::path &path, std::string *error) {
  clear_error(error);
  if (!std::filesystem::exists(path) ||
      !std::filesystem::is_regular_file(path)) {
    return true;
  }
  if (path.extension() != ".lls")
    return true;

  std::string payload;
  if (!read_text_file(path, &payload, nullptr)) {
    return true;
  }
  if (payload.empty())
    return true;

  std::unordered_map<std::string, std::string> kv;
  std::string parse_error;
  if (!parse_runtime_lls_payload(payload, &kv, nullptr, &parse_error)) {
    set_error(error, "runtime .lls parse failure for " + path.string() + ": " +
                         parse_error);
    return false;
  }
  const std::string schema = kv["schema"];
  if (!is_known_schema(schema))
    return true;

  std::string report_fragment_sha;
  if (!sha256_hex_file(path, &report_fragment_sha, error))
    return false;
  bool already = false;
  if (!ledger_contains_(report_fragment_sha, &already, error))
    return false;
  if (already)
    return true;

  std::filesystem::path cp = path;
  if (const auto can = canonicalized(path); can.has_value())
    cp = *can;

  if (is_family_rank_schema(schema)) {
    cuwacunu::hero::family_rank::state_t rank_state{};
    if (!cuwacunu::hero::family_rank::parse_state_from_kv(kv, &rank_state,
                                                          error)) {
      set_error(error, "family rank payload parse failure for " +
                           path.string() + ": " +
                           (error ? *error : std::string{}));
      return false;
    }
    std::uint64_t ts_ms = rank_state.updated_at_ms;
    if (ts_ms == 0)
      ts_ms = now_ms_utc();
    const std::string record_id = family_rank_record_id(
        rank_state.family, rank_state.component_compatibility_sha256_hex,
        report_fragment_sha);
    if (!append_row_(cuwacunu::hero::schema::kRecordKindFAMILY_RANK, record_id,
                     "", rank_state.family, "", rank_state.schema,
                     rank_state.component_compatibility_sha256_hex,
                     static_cast<double>(rank_state.assignments.size()),
                     rank_state.source_view_kind, report_fragment_sha,
                     cp.string(), std::to_string(ts_ms), payload, error)) {
      return false;
    }
    if (!append_ledger_(report_fragment_sha, cp.string(), error))
      return false;
    explicit_family_rank_by_scope_[cuwacunu::hero::family_rank::scope_key(
        rank_state.family, rank_state.component_compatibility_sha256_hex)] =
        std::move(rank_state);
    return true;
  }

  cuwacunu::piaabo::latent_lineage_state::runtime_report_header_t header{};
  if (!cuwacunu::piaabo::latent_lineage_state::
          parse_runtime_report_header_from_kv(kv, &header, error)) {
    set_error(error,
              "runtime report_fragment header parse failure: " + path.string());
    return false;
  }
  const std::string report_canonical_path =
      trim_ascii(header.context.canonical_path);
  if (report_canonical_path.empty()) {
    set_error(error, "runtime report_fragment missing canonical_path: " +
                         path.string());
    return false;
  }
  std::string canonical_path = trim_ascii(kv["canonical_path"]);
  if (canonical_path.empty())
    canonical_path = report_canonical_path;

  std::string hashimyei = kv["hashimyei"];
  if (hashimyei.empty())
    hashimyei = maybe_hashimyei_from_canonical(canonical_path);

  std::string contract_hash = kv["contract_hash"];
  if (contract_hash.empty())
    contract_hash = kv["runtime.contract_hash"];
  if (contract_hash.empty())
    contract_hash = contract_hash_from_report_fragment_path(path);

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

  if (!append_row_(cuwacunu::hero::schema::kRecordKindREPORT_FRAGMENT,
                   report_fragment_sha, "", canonical_path, hashimyei, schema,
                   "", std::numeric_limits<double>::quiet_NaN(), "",
                   report_fragment_sha, cp.string(), std::to_string(ts_ms),
                   payload, error)) {
    return false;
  }
  report_fragment_entry_t entry{};
  entry.report_fragment_id = report_fragment_sha;
  entry.canonical_path = canonical_path;
  entry.hashimyei = hashimyei;
  entry.schema = schema;
  entry.report_fragment_sha256 = report_fragment_sha;
  entry.path = cp.string();
  entry.ts_ms = ts_ms;
  entry.payload_json = payload;
  populate_runtime_report_entry_header_fields_(kv, &entry);
  report_fragments_by_id_[entry.report_fragment_id] = entry;
  {
    const std::string key = join_key(entry.canonical_path, entry.schema);
    auto it = latest_report_fragment_by_key_.find(key);
    if (it == latest_report_fragment_by_key_.end()) {
      latest_report_fragment_by_key_[key] = entry.report_fragment_id;
    } else {
      const auto old_it = report_fragments_by_id_.find(it->second);
      if (old_it == report_fragments_by_id_.end() ||
          entry.ts_ms > old_it->second.ts_ms ||
          (entry.ts_ms == old_it->second.ts_ms &&
           entry.report_fragment_id > old_it->second.report_fragment_id)) {
        it->second = entry.report_fragment_id;
      }
    }
  }
  populate_metrics_from_kv_(report_fragment_sha, kv);
  return append_ledger_(report_fragment_sha, cp.string(), error);
}

bool hashimyei_catalog_store_t::acquire_ingest_lock_(
    const std::filesystem::path &store_root, ingest_lock_t *lock,
    std::string *error) {
  clear_error(error);
  const std::uint64_t acquire_started_at_ms = now_ms_utc();
  if (!lock) {
    set_error(error, "lock pointer is null");
    std::ostringstream extra;
    cuwacunu::hero::mcp_observability::append_json_uint64_field(
        extra, "wait_ms", now_ms_utc() - acquire_started_at_ms);
    if (error && !error->empty()) {
      cuwacunu::hero::mcp_observability::append_json_string_field(
          extra, "error", *error);
    }
    log_hashimyei_catalog_event("ingest_lock_failed", extra.str());
    return false;
  }
  lock->path.clear();
  lock->handle = nullptr;
  lock->acquired_at_ms = 0;
  std::error_code ec;
  const auto lock_dir = cuwacunu::hashimyei::catalog_directory(store_root);
  std::filesystem::create_directories(lock_dir, ec);
  if (ec) {
    set_error(error, "cannot create lock directory: " + lock_dir.string());
    std::ostringstream extra;
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "lock_dir", lock_dir.string());
    cuwacunu::hero::mcp_observability::append_json_uint64_field(
        extra, "wait_ms", now_ms_utc() - acquire_started_at_ms);
    if (error && !error->empty()) {
      cuwacunu::hero::mcp_observability::append_json_string_field(
          extra, "error", *error);
    }
    log_hashimyei_catalog_event("ingest_lock_failed", extra.str());
    return false;
  }
  const auto lock_path = lock_dir / ".ingest.lock";
  idydb_named_lock_options options{};
  options.shared = false;
  options.create_parent_directories = true;
  options.retry_count = 0;
  options.retry_delay_ms = 0;
  options.owner_label = "hashimyei_catalog_ingest";
  const std::string lock_path_string = lock_path.string();
  const int rc = idydb_named_lock_acquire(lock_path_string.c_str(),
                                          &lock->handle, &options);
  if (rc != IDYDB_SUCCESS) {
    std::ostringstream extra;
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "lock_path", lock_path_string);
    cuwacunu::hero::mcp_observability::append_json_uint64_field(
        extra, "wait_ms", now_ms_utc() - acquire_started_at_ms);
    cuwacunu::hero::mcp_observability::append_json_int_field(extra, "idydb_rc",
                                                             rc);
    if (rc == IDYDB_BUSY) {
      char owner[1024] = {0};
      std::string owner_suffix{};
      if (idydb_named_lock_describe_owner(lock_path_string.c_str(), owner,
                                          sizeof(owner)) == IDYDB_SUCCESS) {
        const std::string compact_owner = compact_lock_owner_description(owner);
        if (!compact_owner.empty()) {
          owner_suffix = " owner={" + compact_owner + "}";
          cuwacunu::hero::mcp_observability::append_json_string_field(
              extra, "owner", compact_owner);
        }
      }
      set_error(error, "ingest lock already held: " + lock_path.string() +
                           owner_suffix);
      if (error && !error->empty()) {
        cuwacunu::hero::mcp_observability::append_json_string_field(
            extra, "error", *error);
      }
      log_hashimyei_catalog_event("ingest_lock_busy", extra.str());
    } else {
      set_error(error, "cannot acquire ingest lock: " + lock_path.string() +
                           " (idydb rc=" + std::to_string(rc) + " " +
                           idydb_rc_label(rc) + ")");
      if (error && !error->empty()) {
        cuwacunu::hero::mcp_observability::append_json_string_field(
            extra, "error", *error);
      }
      log_hashimyei_catalog_event("ingest_lock_failed", extra.str());
    }
    return false;
  }
  lock->path = lock_path;
  lock->acquired_at_ms = now_ms_utc();
  {
    std::ostringstream extra;
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "lock_path", lock_path.string());
    cuwacunu::hero::mcp_observability::append_json_uint64_field(
        extra, "wait_ms", lock->acquired_at_ms - acquire_started_at_ms);
    log_hashimyei_catalog_event("ingest_lock_acquired", extra.str());
  }
  return true;
}

void hashimyei_catalog_store_t::release_ingest_lock_(ingest_lock_t *lock) {
  if (!lock)
    return;
  const std::filesystem::path lock_path = lock->path;
  const std::uint64_t acquired_at_ms = lock->acquired_at_ms;
  (void)idydb_named_lock_release(&lock->handle);
  if (!lock_path.empty()) {
    std::ostringstream extra;
    cuwacunu::hero::mcp_observability::append_json_string_field(
        extra, "lock_path", lock_path.string());
    if (acquired_at_ms != 0) {
      cuwacunu::hero::mcp_observability::append_json_uint64_field(
          extra, "hold_ms", now_ms_utc() - acquired_at_ms);
    }
    log_hashimyei_catalog_event("ingest_lock_released", extra.str());
  }
  lock->path.clear();
  lock->acquired_at_ms = 0;
}

bool hashimyei_catalog_store_t::ingest_filesystem(
    const std::filesystem::path &store_root, std::string *error) {
  clear_error(error);
  if (!db_) {
    set_error(error, "catalog is not open");
    return false;
  }

  ingest_lock_t lock{};
  if (!acquire_ingest_lock_(store_root, &lock, error))
    return false;
  struct ingest_lock_guard_t {
    hashimyei_catalog_store_t *self{nullptr};
    ingest_lock_t *lock{nullptr};
    ~ingest_lock_guard_t() {
      if (self && lock)
        self->release_ingest_lock_(lock);
    }
  } lock_guard{this, &lock};
  const std::filesystem::path normalized_store_root =
      store_root.lexically_normal();
  const std::filesystem::path normalized_catalog_root =
      lock.path.parent_path().lexically_normal();
  struct buffered_rows_guard_t {
    hashimyei_catalog_store_t *self{nullptr};
    ~buffered_rows_guard_t() {
      if (!self)
        return;
      self->buffer_rows_ = false;
      self->buffered_row_start_ = 0;
      self->buffered_rows_.clear();
    }
  } buffered_rows_guard{this};
  buffer_rows_ = true;
  buffered_row_start_ = (next_row_hint_ != 0)
                            ? next_row_hint_
                            : idydb_column_next_row(&db_, kColRecordKind);
  buffered_rows_.clear();
  const std::uint64_t ingest_started_at_ms = now_ms_utc();
  std::uint64_t regular_files_scanned = 0;
  std::uint64_t run_manifest_files = 0;
  std::uint64_t component_manifest_files = 0;
  std::uint64_t report_fragment_files = 0;
  std::uint64_t run_manifest_ms = 0;
  std::uint64_t component_manifest_ms = 0;
  std::uint64_t report_fragment_ms = 0;
  std::uint64_t flush_ms = 0;
  std::uint64_t refresh_active_ms = 0;
  std::uint64_t refresh_family_rank_ms = 0;

  std::error_code ec;
  for (std::filesystem::recursive_directory_iterator it(store_root, ec), end;
       it != end; it.increment(ec)) {
    if (ec) {
      set_error(error, "failed scanning store root");
      return false;
    }
    std::error_code entry_ec;
    const auto symlink_state = it->symlink_status(entry_ec);
    if (entry_ec) {
      set_error(error, "failed reading store entry status");
      return false;
    }
    if (std::filesystem::is_symlink(symlink_state))
      continue;
    if (!it->is_regular_file(entry_ec)) {
      if (entry_ec) {
        set_error(error, "failed reading store entry type");
        return false;
      }
      continue;
    }
    ++regular_files_scanned;
    const auto normalized_entry = it->path().lexically_normal();
    if (!path_is_within(normalized_store_root, normalized_entry))
      continue;
    if (path_is_within(normalized_catalog_root, normalized_entry)) {
      continue;
    }

    const auto p = it->path();
    if (p.filename() == "run.manifest.v1.kv") {
      set_error(error,
                "unsupported v1 run manifest filename run.manifest.v1.kv");
      return false;
    }
    if (p.filename() == "component.manifest.v1.kv") {
      set_error(error, "unsupported v1 component manifest filename "
                       "component.manifest.v1.kv");
      return false;
    }
    if (p.filename() == cuwacunu::hashimyei::kRunManifestFilenameV2) {
      const std::uint64_t phase_started_at_ms = now_ms_utc();
      if (!ingest_run_manifest_file_(p, error)) {
        return false;
      }
      ++run_manifest_files;
      run_manifest_ms += now_ms_utc() - phase_started_at_ms;
      continue;
    }
    if (p.filename() == cuwacunu::hashimyei::kComponentManifestFilenameV2) {
      const std::uint64_t phase_started_at_ms = now_ms_utc();
      if (!ingest_component_manifest_file_(p, error)) {
        return false;
      }
      ++component_manifest_files;
      component_manifest_ms += now_ms_utc() - phase_started_at_ms;
      continue;
    }
    if (p.extension() != ".lls")
      continue;
    const std::uint64_t phase_started_at_ms = now_ms_utc();
    if (!ingest_report_fragment_file_(p, error)) {
      return false;
    }
    ++report_fragment_files;
    report_fragment_ms += now_ms_utc() - phase_started_at_ms;
  }

  buffer_rows_ = false;
  {
    const std::uint64_t phase_started_at_ms = now_ms_utc();
    if (!flush_buffered_rows_(error))
      return false;
    flush_ms = now_ms_utc() - phase_started_at_ms;
  }
  {
    const std::uint64_t phase_started_at_ms = now_ms_utc();
    refresh_active_component_views_();
    refresh_active_ms = now_ms_utc() - phase_started_at_ms;
  }
  {
    const std::uint64_t phase_started_at_ms = now_ms_utc();
    refresh_component_family_ranks_();
    refresh_family_rank_ms = now_ms_utc() - phase_started_at_ms;
  }
  std::ostringstream extra;
  cuwacunu::hero::mcp_observability::append_json_string_field(
      extra, "store_root", store_root.string());
  cuwacunu::hero::mcp_observability::append_json_uint64_field(
      extra, "regular_files_scanned", regular_files_scanned);
  cuwacunu::hero::mcp_observability::append_json_uint64_field(
      extra, "run_manifest_files", run_manifest_files);
  cuwacunu::hero::mcp_observability::append_json_uint64_field(
      extra, "component_manifest_files", component_manifest_files);
  cuwacunu::hero::mcp_observability::append_json_uint64_field(
      extra, "report_fragment_files", report_fragment_files);
  cuwacunu::hero::mcp_observability::append_json_uint64_field(
      extra, "run_manifest_ms", run_manifest_ms);
  cuwacunu::hero::mcp_observability::append_json_uint64_field(
      extra, "component_manifest_ms", component_manifest_ms);
  cuwacunu::hero::mcp_observability::append_json_uint64_field(
      extra, "report_fragment_ms", report_fragment_ms);
  cuwacunu::hero::mcp_observability::append_json_uint64_field(extra, "flush_ms",
                                                              flush_ms);
  cuwacunu::hero::mcp_observability::append_json_uint64_field(
      extra, "refresh_active_ms", refresh_active_ms);
  cuwacunu::hero::mcp_observability::append_json_uint64_field(
      extra, "refresh_family_rank_ms", refresh_family_rank_ms);
  cuwacunu::hero::mcp_observability::append_json_uint64_field(
      extra, "duration_ms", now_ms_utc() - ingest_started_at_ms);
  log_hashimyei_catalog_event("ingest_summary", extra.str());
  return true;
}

bool hashimyei_catalog_store_t::rebuild_indexes(std::string *error) {
  clear_error(error);
  if (!db_) {
    set_error(error, "catalog is not open");
    return false;
  }

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

  const auto bump_counter_from_identity =
      [&](const cuwacunu::hashimyei::hashimyei_t &identity) {
        const int key = static_cast<int>(identity.kind);
        const std::uint64_t next = identity.ordinal + 1;
        auto it = kind_counters_.find(key);
        if (it == kind_counters_.end() || next > it->second) {
          kind_counters_[key] = next;
        }
      };
  const auto index_hash_identity =
      [&](const cuwacunu::hashimyei::hashimyei_t &identity) {
        if (!cuwacunu::hashimyei::hashimyei_kind_requires_sha256(identity.kind))
          return;
        if (identity.name.empty() || identity.hash_sha256_hex.empty())
          return;
        const std::string key =
            kind_sha_key(identity.kind, identity.hash_sha256_hex);
        const auto it = hash_identity_by_kind_sha_.find(key);
        if (it == hash_identity_by_kind_sha_.end()) {
          hash_identity_by_kind_sha_.emplace(key, identity);
          return;
        }
        // Keep the smallest ordinal when conflicting aliases are observed.
        if (identity.ordinal < it->second.ordinal) {
          it->second = identity;
        }
      };

  std::unordered_map<std::string, std::uint64_t> family_rank_row_ts_by_scope{};
  std::unordered_map<std::string, std::string> family_rank_row_id_by_scope{};

  const idydb_column_row_sizing next =
      idydb_column_next_row(&db_, kColRecordKind);
  next_row_hint_ = next;
  for (idydb_column_row_sizing row = 1; row < next; ++row) {
    const std::string kind = as_text_or_empty(&db_, kColRecordKind, row);
    if (kind.empty())
      continue;

    if (kind == cuwacunu::hero::schema::kRecordKindCOUNTER) {
      const std::string kind_txt = as_text_or_empty(&db_, kColRecordId, row);
      cuwacunu::hashimyei::hashimyei_kind_e parsed_kind{};
      if (!cuwacunu::hashimyei::parse_hashimyei_kind(kind_txt, &parsed_kind)) {
        continue;
      }
      std::uint64_t next_ordinal = 0;
      if (!parse_u64(as_text_or_empty(&db_, kColMetricTxt, row),
                     &next_ordinal)) {
        const auto metric_num = as_numeric_or_null(&db_, kColMetricNum, row);
        if (metric_num.has_value() && *metric_num >= 0.0) {
          next_ordinal = static_cast<std::uint64_t>(*metric_num);
        }
      }
      kind_counters_[static_cast<int>(parsed_kind)] = next_ordinal;
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindRUN) {
      run_manifest_t run{};
      const std::string payload = as_text_or_empty(&db_, kColPayload, row);
      if (!payload.empty()) {
        std::unordered_map<std::string, std::string> kv;
        (void)parse_latent_lineage_state_payload(payload, &kv);
        run.schema = kv.count("schema") != 0 ? trim_ascii(kv.at("schema"))
                                             : std::string{};
        run.run_id = kv["run_id"];
        (void)parse_u64(kv["started_at_ms"], &run.started_at_ms);
        std::string parse_error;
        if (!parse_identity_kv(kv, "campaign_identity", false,
                               &run.campaign_identity, &parse_error)) {
          run.campaign_identity = cuwacunu::hashimyei::hashimyei_t{};
        }
        if (!parse_binding_kv(kv, "wave_contract_binding",
                              &run.wave_contract_binding, &parse_error)) {
          run.wave_contract_binding = wave_contract_binding_t{};
        }
        run.sampler = kv["sampler"];
        run.record_type = kv["record_type"];
        run.seed = kv["seed"];
        run.device = kv["device"];
        run.dtype = kv["dtype"];

        std::uint64_t dependency_count = 0;
        if (parse_u64(kv["dependency_count"], &dependency_count)) {
          run.dependency_files.reserve(
              static_cast<std::size_t>(dependency_count));
          for (std::uint64_t i = 0; i < dependency_count; ++i) {
            std::ostringstream pfx;
            pfx << "dependency_" << std::setw(4) << std::setfill('0') << i
                << "_";
            dependency_file_t d{};
            d.canonical_path = kv[pfx.str() + "path"];
            d.sha256_hex = kv[pfx.str() + "sha256"];
            if (!d.canonical_path.empty() || !d.sha256_hex.empty()) {
              run.dependency_files.push_back(std::move(d));
            }
          }
        }

        std::uint64_t component_count = 0;
        if (parse_u64(kv["component_count"], &component_count)) {
          run.components.reserve(static_cast<std::size_t>(component_count));
          for (std::uint64_t i = 0; i < component_count; ++i) {
            std::ostringstream pfx;
            pfx << "component_" << std::setw(4) << std::setfill('0') << i
                << "_";
            component_instance_t c{};
            c.canonical_path = kv[pfx.str() + "canonical_path"];
            c.family = kv[pfx.str() + "family"];
            c.hashimyei = kv[pfx.str() + "hashimyei"];
            if (!c.canonical_path.empty() || !c.family.empty() ||
                !c.hashimyei.empty()) {
              run.components.push_back(std::move(c));
            }
          }
        }
      }
      if (run.run_id.empty())
        run.run_id = as_text_or_empty(&db_, kColRunId, row);
      if (run.schema.empty())
        run.schema = as_text_or_empty(&db_, kColSchema, row);
      if (run.started_at_ms == 0) {
        std::uint64_t parsed = 0;
        if (parse_u64(as_text_or_empty(&db_, kColTsMs, row), &parsed))
          run.started_at_ms = parsed;
      }
      normalize_run_manifest_inplace(&run);
      if (!run.run_id.empty()) {
        run_ids_.insert(run.run_id);
        bump_counter_from_identity(run.campaign_identity);
        bump_counter_from_identity(run.wave_contract_binding.identity);
        bump_counter_from_identity(run.wave_contract_binding.contract);
        bump_counter_from_identity(run.wave_contract_binding.wave);
        index_hash_identity(run.wave_contract_binding.identity);
        index_hash_identity(run.wave_contract_binding.contract);
        index_hash_identity(run.wave_contract_binding.wave);
        runs_by_id_[run.run_id] = std::move(run);
      }
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindLEDGER) {
      const std::string report_fragment_id =
          as_text_or_empty(&db_, kColReportFragmentSha256, row).empty()
              ? as_text_or_empty(&db_, kColRecordId, row)
              : as_text_or_empty(&db_, kColReportFragmentSha256, row);
      if (!report_fragment_id.empty()) {
        ledger_report_fragment_ids_.insert(report_fragment_id);
      }
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindRUN_DEPENDENCY) {
      const std::string run_id = as_text_or_empty(&db_, kColRunId, row);
      const std::string dep_path = as_text_or_empty(&db_, kColMetricKey, row);
      const std::string dep_sha = as_text_or_empty(&db_, kColMetricTxt, row);
      if (!run_id.empty() && !dep_path.empty()) {
        dependency_files_by_run_id_[run_id].push_back({dep_path, dep_sha});
      }
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindCOMPONENT) {
      component_state_t component{};
      component.component_id = as_text_or_empty(&db_, kColRecordId, row);
      component.manifest_path = as_text_or_empty(&db_, kColPath, row);
      component.report_fragment_sha256 =
          as_text_or_empty(&db_, kColReportFragmentSha256, row);
      (void)parse_u64(as_text_or_empty(&db_, kColTsMs, row), &component.ts_ms);

      component.manifest.schema = as_text_or_empty(&db_, kColSchema, row);
      component.manifest.canonical_path =
          normalize_hashimyei_canonical_path_or_same(
              as_text_or_empty(&db_, kColCanonicalPath, row));
      const std::string hashimyei_name = normalize_hashimyei_name_or_same(
          as_text_or_empty(&db_, kColHashimyei, row));
      std::uint64_t parsed_ordinal = 0;
      if (cuwacunu::hashimyei::parse_hex_hash_name_ordinal(hashimyei_name,
                                                           &parsed_ordinal)) {
        component.manifest.hashimyei_identity.schema =
            cuwacunu::hashimyei::kIdentitySchemaV2;
        component.manifest.hashimyei_identity.kind =
            cuwacunu::hashimyei::hashimyei_kind_e::TSIEMENE;
        component.manifest.hashimyei_identity.name = hashimyei_name;
        component.manifest.hashimyei_identity.ordinal = parsed_ordinal;
      }
      component.manifest.family = component_family_from_parts(
          component.manifest.canonical_path,
          component.manifest.hashimyei_identity.name);
      const std::string stored_family =
          as_text_or_empty(&db_, kColMetricKey, row);
      if (!stored_family.empty()) {
        component.manifest.family = stored_family;
      }
      component.manifest.lineage_state =
          as_text_or_empty(&db_, kColMetricTxt, row);

      const std::string payload = as_text_or_empty(&db_, kColPayload, row);
      if (!payload.empty()) {
        std::unordered_map<std::string, std::string> kv;
        (void)parse_latent_lineage_state_payload(payload, &kv);
        component_manifest_t parsed_manifest{};
        std::string ignored_error;
        if (parse_component_manifest_kv(kv, &parsed_manifest, &ignored_error)) {
          component.manifest = std::move(parsed_manifest);
        }
      }

      if (component.manifest.schema.empty()) {
        component.manifest.schema =
            cuwacunu::hashimyei::kComponentManifestSchemaV2;
      }
      if (component.manifest.canonical_path.empty()) {
        component.manifest.canonical_path =
            normalize_hashimyei_canonical_path_or_same(
                as_text_or_empty(&db_, kColCanonicalPath, row));
      }
      if (component.manifest.hashimyei_identity.name.empty()) {
        const std::string fallback_name = normalize_hashimyei_name_or_same(
            as_text_or_empty(&db_, kColHashimyei, row));
        if (cuwacunu::hashimyei::parse_hex_hash_name_ordinal(fallback_name,
                                                             &parsed_ordinal)) {
          component.manifest.hashimyei_identity.schema =
              cuwacunu::hashimyei::kIdentitySchemaV2;
          component.manifest.hashimyei_identity.kind =
              cuwacunu::hashimyei::hashimyei_kind_e::TSIEMENE;
          component.manifest.hashimyei_identity.name = fallback_name;
          component.manifest.hashimyei_identity.ordinal = parsed_ordinal;
        }
      }
      if (component.manifest.family.empty()) {
        component.manifest.family = component_family_from_parts(
            component.manifest.canonical_path,
            component.manifest.hashimyei_identity.name);
      }
      if (component.manifest.lineage_state.empty()) {
        component.manifest.lineage_state =
            as_text_or_empty(&db_, kColMetricTxt, row);
      }
      if (component.manifest.updated_at_ms == 0) {
        component.manifest.updated_at_ms = component.ts_ms;
      }
      if (component.manifest.created_at_ms == 0) {
        component.manifest.created_at_ms = component.ts_ms;
      }
      normalize_component_manifest_inplace(&component.manifest);

      if (!component.component_id.empty()) {
        bump_counter_from_identity(component.manifest.hashimyei_identity);
        if (component.manifest.parent_identity.has_value()) {
          bump_counter_from_identity(*component.manifest.parent_identity);
        }
        bump_counter_from_identity(component.manifest.contract_identity);
        index_hash_identity(component.manifest.contract_identity);
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
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindCOMPONENT_REVISION) {
      const std::string component_id =
          as_text_or_empty(&db_, kColRecordId, row);
      const std::string canonical_path =
          as_text_or_empty(&db_, kColCanonicalPath, row);
      const std::string hashimyei = as_text_or_empty(&db_, kColHashimyei, row);
      const std::string family = as_text_or_empty(&db_, kColMetricKey, row);
      const std::string parent_hashimyei =
          as_text_or_empty(&db_, kColMetricTxt, row);
      const std::string revision_reason =
          as_text_or_empty(&db_, kColPayload, row);
      if (!component_id.empty()) {
        auto it_component = components_by_id_.find(component_id);
        if (it_component != components_by_id_.end()) {
          if (!parent_hashimyei.empty()) {
            std::uint64_t parent_ordinal = 0;
            if (cuwacunu::hashimyei::parse_hex_hash_name_ordinal(
                    parent_hashimyei, &parent_ordinal)) {
              cuwacunu::hashimyei::hashimyei_t parent{};
              parent.schema = cuwacunu::hashimyei::kIdentitySchemaV2;
              parent.kind = cuwacunu::hashimyei::hashimyei_kind_e::TSIEMENE;
              parent.name = parent_hashimyei;
              parent.ordinal = parent_ordinal;
              it_component->second.manifest.parent_identity = parent;
            }
          }
          if (!revision_reason.empty()) {
            it_component->second.manifest.revision_reason = revision_reason;
          }
          if (!family.empty())
            it_component->second.manifest.family = family;
        } else if (!canonical_path.empty()) {
          component_state_t placeholder{};
          placeholder.component_id = component_id;
          placeholder.manifest.canonical_path = canonical_path;
          std::uint64_t ord = 0;
          if (cuwacunu::hashimyei::parse_hex_hash_name_ordinal(hashimyei,
                                                               &ord)) {
            placeholder.manifest.hashimyei_identity.schema =
                cuwacunu::hashimyei::kIdentitySchemaV2;
            placeholder.manifest.hashimyei_identity.kind =
                cuwacunu::hashimyei::hashimyei_kind_e::TSIEMENE;
            placeholder.manifest.hashimyei_identity.name =
                normalize_hashimyei_name_or_same(hashimyei);
            placeholder.manifest.hashimyei_identity.ordinal = ord;
          }
          if (!parent_hashimyei.empty()) {
            std::uint64_t parent_ord = 0;
            if (cuwacunu::hashimyei::parse_hex_hash_name_ordinal(
                    parent_hashimyei, &parent_ord)) {
              cuwacunu::hashimyei::hashimyei_t parent{};
              parent.schema = cuwacunu::hashimyei::kIdentitySchemaV2;
              parent.kind = cuwacunu::hashimyei::hashimyei_kind_e::TSIEMENE;
              parent.name = normalize_hashimyei_name_or_same(parent_hashimyei);
              parent.ordinal = parent_ord;
              placeholder.manifest.parent_identity = parent;
            }
          }
          placeholder.manifest.revision_reason =
              revision_reason.empty() ? "initial" : revision_reason;
          placeholder.manifest.family =
              family.empty()
                  ? component_family_from_parts(canonical_path, hashimyei)
                  : family;
          normalize_component_manifest_inplace(&placeholder.manifest);
          components_by_id_[component_id] = std::move(placeholder);
        }
      }
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindCOMPONENT_ACTIVE) {
      const std::string canonical_path =
          normalize_hashimyei_canonical_path_or_same(
              as_text_or_empty(&db_, kColCanonicalPath, row));
      std::string family = normalize_hashimyei_canonical_path_or_same(
          as_text_or_empty(&db_, kColMetricKey, row));
      const std::string hashimyei = normalize_hashimyei_name_or_same(
          as_text_or_empty(&db_, kColHashimyei, row));
      if (family.empty()) {
        family = component_family_from_parts(canonical_path, hashimyei);
      }
      std::string contract_hash{};
      const std::string payload = as_text_or_empty(&db_, kColPayload, row);
      if (!payload.empty()) {
        std::unordered_map<std::string, std::string> kv{};
        (void)parse_latent_lineage_state_payload(payload, &kv);
        contract_hash = trim_ascii(kv["contract_hash"]);
      }
      if (!canonical_path.empty() && !family.empty()) {
        const std::string key = component_active_pointer_key(
            canonical_path, family, hashimyei, contract_hash);
        active_hashimyei_by_key_[key] = hashimyei;
      }
      continue;
    }

    if (kind == cuwacunu::hero::schema::kRecordKindFAMILY_RANK) {
      const std::string payload = as_text_or_empty(&db_, kColPayload, row);
      if (!payload.empty()) {
        std::unordered_map<std::string, std::string> kv;
        std::string parse_error{};
        if (parse_runtime_lls_payload(payload, &kv, nullptr, &parse_error)) {
          cuwacunu::hero::family_rank::state_t rank_state{};
          if (cuwacunu::hero::family_rank::parse_state_from_kv(kv, &rank_state,
                                                               &parse_error)) {
            const std::string scope_key =
                cuwacunu::hero::family_rank::scope_key(
                    rank_state.family,
                    rank_state.component_compatibility_sha256_hex);
            std::uint64_t row_ts = 0;
            if (rank_state.updated_at_ms != 0) {
              row_ts = rank_state.updated_at_ms;
            } else {
              (void)parse_u64(as_text_or_empty(&db_, kColTsMs, row), &row_ts);
            }
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

    if (kind == cuwacunu::hero::schema::kRecordKindREPORT_FRAGMENT) {
      report_fragment_entry_t e{};
      e.report_fragment_id = as_text_or_empty(&db_, kColRecordId, row);
      e.canonical_path = normalize_hashimyei_canonical_path_or_same(
          as_text_or_empty(&db_, kColCanonicalPath, row));
      e.hashimyei = normalize_hashimyei_name_or_same(
          as_text_or_empty(&db_, kColHashimyei, row));
      e.schema = as_text_or_empty(&db_, kColSchema, row);
      e.report_fragment_sha256 =
          as_text_or_empty(&db_, kColReportFragmentSha256, row);
      e.path = as_text_or_empty(&db_, kColPath, row);
      e.payload_json = as_text_or_empty(&db_, kColPayload, row);
      (void)parse_u64(as_text_or_empty(&db_, kColTsMs, row), &e.ts_ms);
      if (!e.payload_json.empty()) {
        std::unordered_map<std::string, std::string> kv{};
        std::string parse_error{};
        if (parse_runtime_lls_payload(e.payload_json, &kv, nullptr,
                                      &parse_error)) {
          populate_runtime_report_entry_header_fields_(kv, &e);
          populate_metrics_from_kv_(e.report_fragment_id, kv);
          if (e.canonical_path.empty()) {
            e.canonical_path = normalize_hashimyei_canonical_path_or_same(
                kv["canonical_path"]);
          }
          if (e.hashimyei.empty()) {
            e.hashimyei = normalize_hashimyei_name_or_same(kv["hashimyei"]);
          }
          if (e.schema.empty())
            e.schema = kv["schema"];
        }
      }

      if (!e.report_fragment_id.empty()) {
        report_fragments_by_id_[e.report_fragment_id] = e;
        const std::string key = join_key(e.canonical_path, e.schema);
        auto it = latest_report_fragment_by_key_.find(key);
        if (it == latest_report_fragment_by_key_.end()) {
          latest_report_fragment_by_key_[key] = e.report_fragment_id;
        } else {
          const auto old_it = report_fragments_by_id_.find(it->second);
          if (old_it == report_fragments_by_id_.end() ||
              e.ts_ms > old_it->second.ts_ms ||
              (e.ts_ms == old_it->second.ts_ms &&
               e.report_fragment_id > old_it->second.report_fragment_id)) {
            it->second = e.report_fragment_id;
          }
        }
      }
      continue;
    }
  }

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
      if (component.manifest.hashimyei_identity.name != pointer_hashimyei)
        continue;
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

  return true;
}
