bool lattice_catalog_store_t::ingest_runtime_report_fragments(
    const std::filesystem::path &store_root, std::string *error) {
  clear_error(error);
  if (!db_) {
    set_error(error, "catalog is not open");
    return false;
  }

  const std::filesystem::path normalized_store_root =
      store_root.lexically_normal();
  struct buffered_rows_guard_t {
    lattice_catalog_store_t *self{nullptr};
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
  std::uint64_t run_manifest_files = 0;
  std::uint64_t component_manifest_files = 0;
  std::uint64_t report_fragment_files = 0;
  std::uint64_t run_manifest_ms = 0;
  std::uint64_t component_manifest_ms = 0;
  std::uint64_t report_fragment_ms = 0;
  std::uint64_t flush_ms = 0;
  std::uint64_t family_rank_refresh_ms = 0;
  std::uint64_t runs_root_scan_ms = 0;
  std::uint64_t store_root_scan_ms = 0;
  std::error_code ec;

  const auto runs_root = cuwacunu::hashimyei::runs_root(store_root);
  if (std::filesystem::exists(runs_root, ec) &&
      std::filesystem::is_directory(runs_root, ec)) {
    const std::uint64_t phase_started_at_ms = now_ms_utc();
    for (std::filesystem::recursive_directory_iterator it(runs_root, ec), end;
         it != end; it.increment(ec)) {
      if (ec) {
        set_error(error, "failed scanning runs root");
        return false;
      }
      std::error_code entry_ec;
      const auto symlink_state = it->symlink_status(entry_ec);
      if (entry_ec) {
        set_error(error, "failed reading runs entry status");
        return false;
      }
      if (std::filesystem::is_symlink(symlink_state))
        continue;
      if (!it->is_regular_file(entry_ec)) {
        if (entry_ec) {
          set_error(error, "failed reading runs entry type");
          return false;
        }
        continue;
      }
      const auto normalized_entry = it->path().lexically_normal();
      if (!path_is_within(normalized_store_root, normalized_entry))
        continue;
      if (it->path().filename() != cuwacunu::hashimyei::kRunManifestFilenameV2)
        continue;
      const std::uint64_t ingest_started_file_ms = now_ms_utc();
      if (!ingest_runtime_run_manifest_file_(it->path(), error))
        return false;
      ++run_manifest_files;
      run_manifest_ms += now_ms_utc() - ingest_started_file_ms;
    }
    runs_root_scan_ms = now_ms_utc() - phase_started_at_ms;
  }

  const std::uint64_t store_scan_started_at_ms = now_ms_utc();
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
    const auto normalized_entry = it->path().lexically_normal();
    if (!path_is_within(normalized_store_root, normalized_entry))
      continue;

    const auto p = it->path();
    if (p == options_.catalog_path)
      continue;
    if (p.filename() == cuwacunu::hashimyei::kRunManifestFilenameV2)
      continue;
    if (p.filename() == cuwacunu::hashimyei::kComponentManifestFilenameV2) {
      const std::uint64_t ingest_started_file_ms = now_ms_utc();
      if (!ingest_runtime_component_manifest_file_(p, error))
        return false;
      ++component_manifest_files;
      component_manifest_ms += now_ms_utc() - ingest_started_file_ms;
      continue;
    }

    const auto ext = p.extension().string();
    if (ext != ".lls")
      continue;
    const std::uint64_t ingest_started_file_ms = now_ms_utc();
    if (!ingest_runtime_report_fragment_file_(p, error))
      return false;
    ++report_fragment_files;
    report_fragment_ms += now_ms_utc() - ingest_started_file_ms;
  }
  store_root_scan_ms = now_ms_utc() - store_scan_started_at_ms;

  buffer_rows_ = false;
  {
    const std::uint64_t phase_started_at_ms = now_ms_utc();
    if (!flush_buffered_rows_(error))
      return false;
    flush_ms = now_ms_utc() - phase_started_at_ms;
  }
  // Ingest already updates the runtime in-memory indexes incrementally as rows
  // are discovered. The remaining deferred fixups are component compatibility
  // enrichment from runtime component manifests plus rank propagation, because
  // family-rank artifacts may arrive before their matching fragments.
  {
    const std::uint64_t phase_started_at_ms = now_ms_utc();
    refresh_runtime_report_fragment_family_ranks_(
        &runtime_report_fragments_by_id_, runtime_components_by_id_,
        explicit_family_rank_by_scope_);
    family_rank_refresh_ms = now_ms_utc() - phase_started_at_ms;
  }
  std::ostringstream extra;
  cuwacunu::hero::mcp_observability::append_json_string_field(
      extra, "store_root", store_root.string());
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
  cuwacunu::hero::mcp_observability::append_json_uint64_field(
      extra, "runs_root_scan_ms", runs_root_scan_ms);
  cuwacunu::hero::mcp_observability::append_json_uint64_field(
      extra, "store_root_scan_ms", store_root_scan_ms);
  cuwacunu::hero::mcp_observability::append_json_uint64_field(extra, "flush_ms",
                                                              flush_ms);
  cuwacunu::hero::mcp_observability::append_json_uint64_field(
      extra, "family_rank_refresh_ms", family_rank_refresh_ms);
  cuwacunu::hero::mcp_observability::append_json_uint64_field(
      extra, "duration_ms", now_ms_utc() - ingest_started_at_ms);
  log_lattice_catalog_event("ingest_summary", extra.str());
  return true;
}

bool lattice_catalog_store_t::list_runtime_runs_by_binding(
    std::string_view contract_hashimyei, std::string_view wave_hashimyei,
    std::string_view binding_hashimyei,
    std::vector<cuwacunu::hero::hashimyei::run_manifest_t> *out,
    std::string *error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "run list output pointer is null");
    return false;
  }
  out->clear();
  for (const auto &[_, run] : runtime_runs_by_id_) {
    if (!contract_hashimyei.empty() &&
        run.wave_contract_binding.contract.name != contract_hashimyei) {
      continue;
    }
    if (!wave_hashimyei.empty() &&
        run.wave_contract_binding.wave.name != wave_hashimyei) {
      continue;
    }
    if (!binding_hashimyei.empty() &&
        run.wave_contract_binding.identity.name != binding_hashimyei) {
      continue;
    }
    out->push_back(run);
  }
  std::sort(out->begin(), out->end(), [](const auto &a, const auto &b) {
    if (a.started_at_ms != b.started_at_ms)
      return a.started_at_ms > b.started_at_ms;
    return a.run_id < b.run_id;
  });
  return true;
}

bool lattice_catalog_store_t::ingest_runtime_artifact(
    const std::filesystem::path &path, std::string *error) {
  clear_error(error);
  if (!db_) {
    set_error(error, "catalog is not open");
    return false;
  }

  std::error_code ec;
  if (!std::filesystem::exists(path, ec) ||
      !std::filesystem::is_regular_file(path, ec)) {
    set_error(error,
              "runtime artifact path is missing or not a regular file: " +
                  path.string());
    return false;
  }

  if (path.filename() == cuwacunu::hashimyei::kRunManifestFilenameV2) {
    return ingest_runtime_run_manifest_file_(path, error);
  }
  if (path.filename() == cuwacunu::hashimyei::kComponentManifestFilenameV2) {
    return ingest_runtime_component_manifest_file_(path, error);
  }
  if (path.extension() == ".lls") {
    return ingest_runtime_report_fragment_file_(path, error);
  }

  set_error(error, "unsupported runtime artifact path: " + path.string());
  return false;
}

bool lattice_catalog_store_t::list_runtime_report_fragments(
    std::string_view canonical_path, std::string_view schema, std::size_t limit,
    std::size_t offset, bool newest_first,
    std::vector<runtime_report_fragment_t> *out, std::string *error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "report_fragment list output pointer is null");
    return false;
  }
  out->clear();

  const std::string cp = normalize_source_hashimyei_cursor(canonical_path);
  const std::string sc(schema);
  for (const auto &[_, fragment] : runtime_report_fragments_by_id_) {
    if (!runtime_hashimyei_cursor_matches_fragment(cp, fragment))
      continue;
    if (!sc.empty() && fragment.schema != sc)
      continue;
    out->push_back(fragment);
  }

  std::sort(
      out->begin(), out->end(), [newest_first](const auto &a, const auto &b) {
        if (a.ts_ms != b.ts_ms) {
          return newest_first ? (a.ts_ms > b.ts_ms) : (a.ts_ms < b.ts_ms);
        }
        return newest_first ? (a.report_fragment_id > b.report_fragment_id)
                            : (a.report_fragment_id < b.report_fragment_id);
      });

  const std::size_t off = std::min(offset, out->size());
  std::size_t count = limit;
  if (count == 0)
    count = out->size() - off;
  count = std::min(count, out->size() - off);
  out->assign(out->begin() + static_cast<std::ptrdiff_t>(off),
              out->begin() + static_cast<std::ptrdiff_t>(off + count));
  return true;
}

bool lattice_catalog_store_t::list_runtime_fact_summaries(
    std::size_t limit, std::size_t offset, bool newest_first,
    std::vector<runtime_fact_summary_t> *out, std::string *error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "fact summary list output pointer is null");
    return false;
  }
  out->clear();

  out->reserve(runtime_fact_summaries_by_canonical_.size());
  for (const auto &[_, summary] : runtime_fact_summaries_by_canonical_) {
    runtime_fact_summary_t row = summary;
    finalize_runtime_fact_summary_(&row);
    out->push_back(std::move(row));
  }

  std::sort(out->begin(), out->end(),
            [newest_first](const runtime_fact_summary_t &a,
                           const runtime_fact_summary_t &b) {
              if (a.latest_ts_ms != b.latest_ts_ms) {
                return newest_first ? (a.latest_ts_ms > b.latest_ts_ms)
                                    : (a.latest_ts_ms < b.latest_ts_ms);
              }
              return newest_first ? (a.canonical_path < b.canonical_path)
                                  : (a.canonical_path > b.canonical_path);
            });

  const std::size_t off = std::min(offset, out->size());
  std::size_t count = limit;
  if (count == 0)
    count = out->size() - off;
  count = std::min(count, out->size() - off);
  out->assign(out->begin() + static_cast<std::ptrdiff_t>(off),
              out->begin() + static_cast<std::ptrdiff_t>(off + count));
  return true;
}

bool lattice_catalog_store_t::get_runtime_fact_summary(
    std::string_view canonical_path, runtime_fact_summary_t *out,
    std::string *error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "fact summary output pointer is null");
    return false;
  }
  *out = runtime_fact_summary_t{};

  const std::string cp = normalize_source_hashimyei_cursor(canonical_path);
  if (cp.empty()) {
    set_error(error, "canonical_path is required");
    return false;
  }

  const auto it = runtime_fact_summaries_by_canonical_.find(cp);
  if (it == runtime_fact_summaries_by_canonical_.end()) {
    set_error(error, "fact summary not found for canonical_path");
    return false;
  }

  *out = it->second;
  finalize_runtime_fact_summary_(out);
  return true;
}

bool lattice_catalog_store_t::latest_runtime_report_fragment(
    std::string_view canonical_path, std::string_view schema,
    runtime_report_fragment_t *out, std::string *error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "report_fragment output pointer is null");
    return false;
  }
  *out = runtime_report_fragment_t{};

  std::vector<runtime_report_fragment_t> rows{};
  if (!list_runtime_report_fragments(canonical_path, schema, 1, 0, true, &rows,
                                     error)) {
    return false;
  }
  if (rows.empty()) {
    set_error(error, "report_fragment not found for canonical_path/schema");
    return false;
  }
  *out = std::move(rows.front());
  return true;
}

bool lattice_catalog_store_t::get_runtime_report_fragment(
    std::string_view report_fragment_id, runtime_report_fragment_t *out,
    std::string *error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "report_fragment output pointer is null");
    return false;
  }
  *out = runtime_report_fragment_t{};

  const auto it =
      runtime_report_fragments_by_id_.find(std::string(report_fragment_id));
  if (it != runtime_report_fragments_by_id_.end()) {
    *out = it->second;
    return true;
  }

  set_error(error,
            "report_fragment not found: " + std::string(report_fragment_id));
  return false;
}

bool lattice_catalog_store_t::list_runtime_report_schemas(
    std::string_view canonical_path, std::vector<std::string> *out,
    std::string *error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "schema list output pointer is null");
    return false;
  }
  out->clear();

  std::vector<runtime_report_fragment_t> rows{};
  if (!list_runtime_report_fragments(canonical_path, "", 0, 0, true, &rows,
                                     error)) {
    return false;
  }

  std::unordered_set<std::string> seen{};
  for (const auto &row : rows) {
    if (row.schema.empty())
      continue;
    if (!seen.insert(row.schema).second)
      continue;
    out->push_back(row.schema);
  }
  std::sort(out->begin(), out->end());
  return true;
}

bool lattice_catalog_store_t::get_explicit_family_rank(
    std::string_view family,
    std::string_view component_compatibility_sha256_hex,
    cuwacunu::hero::family_rank::state_t *out, std::string *error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "family rank output pointer is null");
    return false;
  }
  *out = cuwacunu::hero::family_rank::state_t{};

  const std::string family_key = trim_ascii(family);
  const std::string component_compatibility_key =
      trim_ascii(component_compatibility_sha256_hex);
  if (family_key.empty() || component_compatibility_key.empty()) {
    set_error(error,
              "family and component_compatibility_sha256_hex are required");
    return false;
  }
  const std::string scope_key = cuwacunu::hero::family_rank::scope_key(
      family_key, component_compatibility_key);
  const auto it = explicit_family_rank_by_scope_.find(scope_key);
  if (it == explicit_family_rank_by_scope_.end()) {
    set_error(error, "explicit family rank not found: " + scope_key);
    return false;
  }
  *out = it->second;
  return true;
}

bool lattice_catalog_store_t::get_family_rank(
    std::string_view family,
    std::string_view component_compatibility_sha256_hex,
    cuwacunu::hero::family_rank::state_t *out, std::string *error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "family rank output pointer is null");
    return false;
  }
  *out = cuwacunu::hero::family_rank::state_t{};

  const std::string family_key = trim_ascii(family);
  const std::string component_compatibility_key =
      trim_ascii(component_compatibility_sha256_hex);
  if (family_key.empty() || component_compatibility_key.empty()) {
    set_error(error,
              "family and component_compatibility_sha256_hex are required");
    return false;
  }
  const std::string scope_key = cuwacunu::hero::family_rank::scope_key(
      family_key, component_compatibility_key);
  const auto it = explicit_family_rank_by_scope_.find(scope_key);
  if (it == explicit_family_rank_by_scope_.end()) {
    set_error(error, "family rank not found: " + scope_key);
    return false;
  }
  *out = it->second;
  return true;
}

bool lattice_catalog_store_t::get_runtime_view_lls(
    std::string_view view_kind, std::string_view intersection_cursor,
    std::uint64_t wave_cursor, bool use_wave_cursor,
    std::string_view contract_hash,
    std::string_view component_compatibility_sha256_hex,
    runtime_view_report_t *out, std::string *error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "runtime view output pointer is null");
    return false;
  }
  *out = runtime_view_report_t{};

  const std::string view_kind_token = trim_ascii(view_kind);
  const std::string intersection_cursor_token = trim_ascii(intersection_cursor);
  const std::string contract_hash_token = trim_ascii(contract_hash);
  const std::string component_compatibility_sha256_hex_token =
      trim_ascii(component_compatibility_sha256_hex);
  if (view_kind_token.empty()) {
    set_error(error, "view_kind is empty");
    return false;
  }
  if (view_kind_token != kEntropicCapacityComparisonViewKind &&
      view_kind_token != kFamilyEvaluationReportViewKind) {
    set_error(error, "unsupported runtime view_kind: " + view_kind_token);
    return false;
  }
  std::string selector_canonical_path{};
  if (!intersection_cursor_token.empty()) {
    if (view_kind_token == kFamilyEvaluationReportViewKind &&
        !use_wave_cursor &&
        intersection_cursor_token.find('|') == std::string::npos) {
      selector_canonical_path =
          normalize_source_hashimyei_cursor(intersection_cursor_token);
      out->selector_hashimyei_cursor = selector_canonical_path;
    } else {
      runtime_intersection_cursor_t parsed_intersection{};
      std::string parse_error{};
      if (!lattice_catalog_store_t::parse_runtime_intersection_cursor(
              intersection_cursor_token, &parsed_intersection, &parse_error)) {
        set_error(error, parse_error);
        return false;
      }
      if (use_wave_cursor && parsed_intersection.wave_cursor != wave_cursor) {
        set_error(error, "wave_cursor and intersection_cursor select different "
                         "wave_cursor values");
        return false;
      }
      wave_cursor = parsed_intersection.wave_cursor;
      use_wave_cursor = true;
      selector_canonical_path = parsed_intersection.hashimyei_cursor;
      out->selector_hashimyei_cursor = selector_canonical_path;
      out->selector_intersection_cursor = intersection_cursor_token;
    }
  }

  out->view_kind = view_kind_token;
  out->contract_hash = contract_hash_token;
  out->component_compatibility_sha256_hex =
      component_compatibility_sha256_hex_token;
  out->wave_cursor = wave_cursor;
  out->has_wave_cursor = use_wave_cursor;

  if (view_kind_token == kFamilyEvaluationReportViewKind) {
    if (component_compatibility_sha256_hex_token.empty()) {
      set_error(error, "family_evaluation_report requires "
                       "component_compatibility_sha256_hex");
      return false;
    }
    const std::string family_selector =
        runtime_family_from_canonical(selector_canonical_path);
    if (family_selector.empty()) {
      set_error(
          error,
          "family_evaluation_report requires a family canonical_path selector");
      return false;
    }
    if (!maybe_hashimyei_from_canonical(family_selector).empty()) {
      set_error(error, "family_evaluation_report selector must be a family "
                       "token without hashimyei suffix");
      return false;
    }

    out->canonical_path = std::string(kFamilyEvaluationReportViewCanonicalPath);
    out->selector_hashimyei_cursor = family_selector;

    std::vector<runtime_report_fragment_t> family_rows{};
    collect_runtime_report_fragment_candidates_(
        runtime_report_fragments_by_id_,
        runtime_report_fragment_ids_by_wave_cursor_, family_selector, "",
        wave_cursor, use_wave_cursor, &family_rows);

    cuwacunu::hero::family_rank::state_t current_rank_state{};
    const bool has_current_rank_state = get_family_rank(
        family_selector, component_compatibility_sha256_hex_token,
        &current_rank_state, nullptr);

    struct family_candidate_t {
      std::string hashimyei{};
      std::string canonical_path{};
      std::optional<std::uint64_t> current_rank{};
      std::uint64_t selected_wave_cursor{0};
      std::uint64_t selected_bundle_ts_ms{0};
      std::vector<runtime_report_fragment_t> fragments{};
    };

    struct bundle_t {
      std::uint64_t wave_cursor{0};
      std::uint64_t latest_ts_ms{0};
      std::vector<runtime_report_fragment_t> fragments{};
    };

    std::unordered_map<std::string, std::unordered_map<std::string, bundle_t>>
        bundles_by_hash{};
    for (const auto &row : family_rows) {
      if (trim_ascii(row.component_compatibility_sha256_hex) !=
          component_compatibility_sha256_hex_token)
        continue;
      const std::string hash =
          row.hashimyei.empty()
              ? maybe_hashimyei_from_canonical(row.canonical_path)
              : row.hashimyei;
      if (hash.empty())
        continue;
      const std::string bundle_key = std::to_string(row.wave_cursor);
      auto &bundle = bundles_by_hash[hash][bundle_key];
      bundle.wave_cursor = row.wave_cursor;
      bundle.latest_ts_ms = std::max(bundle.latest_ts_ms, row.ts_ms);
      bundle.fragments.push_back(row);
    }

    std::vector<family_candidate_t> candidates{};
    candidates.reserve(bundles_by_hash.size());
    for (auto &[hash, bundles] : bundles_by_hash) {
      family_candidate_t candidate{};
      candidate.hashimyei = hash;
      if (has_current_rank_state) {
        candidate.current_rank =
            cuwacunu::hero::family_rank::rank_for_hashimyei(current_rank_state,
                                                            hash);
      }

      bundle_t *selected_bundle = nullptr;
      for (auto &[_, bundle] : bundles) {
        if (!selected_bundle) {
          selected_bundle = &bundle;
          continue;
        }
        if (bundle.wave_cursor != selected_bundle->wave_cursor) {
          if (bundle.wave_cursor > selected_bundle->wave_cursor) {
            selected_bundle = &bundle;
          }
          continue;
        }
        if (bundle.latest_ts_ms != selected_bundle->latest_ts_ms) {
          if (bundle.latest_ts_ms > selected_bundle->latest_ts_ms) {
            selected_bundle = &bundle;
          }
          continue;
        }
        if (bundle.fragments.size() > selected_bundle->fragments.size()) {
          selected_bundle = &bundle;
        }
      }
      if (!selected_bundle)
        continue;
      candidate.selected_wave_cursor = selected_bundle->wave_cursor;
      candidate.selected_bundle_ts_ms = selected_bundle->latest_ts_ms;
      candidate.fragments = std::move(selected_bundle->fragments);
      if (!candidate.fragments.empty()) {
        candidate.canonical_path =
            candidate.fragments.front().canonical_path.empty()
                ? (family_selector + "." + hash)
                : candidate.fragments.front().canonical_path;
      }
      std::sort(candidate.fragments.begin(), candidate.fragments.end(),
                [](const runtime_report_fragment_t &a,
                   const runtime_report_fragment_t &b) {
                  if (a.schema != b.schema)
                    return a.schema < b.schema;
                  if (a.ts_ms != b.ts_ms)
                    return a.ts_ms > b.ts_ms;
                  return a.report_fragment_id > b.report_fragment_id;
                });
      candidates.push_back(std::move(candidate));
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const family_candidate_t &a, const family_candidate_t &b) {
                if (a.current_rank.has_value() != b.current_rank.has_value()) {
                  return a.current_rank.has_value();
                }
                if (a.current_rank.has_value() && b.current_rank.has_value() &&
                    a.current_rank != b.current_rank) {
                  return *a.current_rank < *b.current_rank;
                }
                return a.hashimyei < b.hashimyei;
              });

    out->match_count = candidates.size();
    out->ambiguity_count = 0;

    std::ostringstream view{};
    view << "/* synthetic view transport: "
         << kFamilyEvaluationReportViewTransportSchema << " */\n";
    append_view_line(&view, "report_transport_schema",
                     kFamilyEvaluationReportViewTransportSchema);
    append_view_line(&view, "view_kind", out->view_kind);
    append_view_line(&view, "canonical_path", out->canonical_path);
    append_view_line(&view, "family", family_selector);
    if (!out->selector_intersection_cursor.empty()) {
      append_view_line(&view, "selector_intersection_cursor",
                       out->selector_intersection_cursor);
    }
    append_view_line(&view, "selector_hashimyei_cursor",
                     out->selector_hashimyei_cursor);
    append_view_line(&view, "component_compatibility_sha256_hex",
                     out->component_compatibility_sha256_hex);
    append_view_line(&view, "selection_mode",
                     out->has_wave_cursor ? "historical" : "latest");
    if (out->has_wave_cursor) {
      append_view_u64_line(&view, "wave_cursor", out->wave_cursor);
      append_view_line(&view, "wave_cursor_view",
                       format_runtime_wave_cursor(out->wave_cursor));
    }
    if (has_current_rank_state) {
      append_view_size_line(&view, "current_rank.assignment_count",
                            current_rank_state.assignments.size());
      if (!current_rank_state.source_view_kind.empty()) {
        append_view_line(&view, "current_rank.source_view_kind",
                         current_rank_state.source_view_kind);
      }
      if (!current_rank_state.source_view_transport_sha256.empty()) {
        append_view_line(&view, "current_rank.source_view_transport_sha256",
                         current_rank_state.source_view_transport_sha256);
      }
      if (!current_rank_state.component_compatibility_sha256_hex.empty()) {
        append_view_line(&view,
                         "current_rank.component_compatibility_sha256_hex",
                         current_rank_state.component_compatibility_sha256_hex);
      }
    }
    append_view_size_line(&view, "candidate_count", candidates.size());
    for (std::size_t i = 0; i < candidates.size(); ++i) {
      const auto &candidate = candidates[i];
      const std::string prefix = "candidate." + std::to_string(i + 1);
      append_view_line(&view, prefix + ".hashimyei", candidate.hashimyei);
      append_view_line(&view, prefix + ".canonical_path",
                       candidate.canonical_path);
      if (candidate.current_rank.has_value()) {
        append_view_u64_line(&view, prefix + ".current_rank",
                             *candidate.current_rank);
      }
      append_view_u64_line(&view, prefix + ".bundle_wave_cursor",
                           candidate.selected_wave_cursor);
      append_view_line(
          &view, prefix + ".bundle_wave_cursor_view",
          format_runtime_wave_cursor(candidate.selected_wave_cursor));
      append_view_u64_line(&view, prefix + ".bundle_ts_ms",
                           candidate.selected_bundle_ts_ms);
      append_view_size_line(&view, prefix + ".fragment_count",
                            candidate.fragments.size());
      for (std::size_t j = 0; j < candidate.fragments.size(); ++j) {
        const auto &fragment = candidate.fragments[j];
        const std::string fragment_prefix =
            prefix + ".fragment." + std::to_string(j + 1);
        append_view_line(&view, fragment_prefix + ".report_fragment_id",
                         fragment.report_fragment_id);
        append_view_line(&view, fragment_prefix + ".canonical_path",
                         fragment.canonical_path);
        if (!fragment.semantic_taxon.empty()) {
          append_view_line(&view, fragment_prefix + ".semantic_taxon",
                           fragment.semantic_taxon);
        }
        append_view_line(&view, fragment_prefix + ".schema", fragment.schema);
        append_view_u64_line(&view, fragment_prefix + ".ts_ms", fragment.ts_ms);
        if (!fragment.component_compatibility_sha256_hex.empty()) {
          append_view_line(
              &view, fragment_prefix + ".component_compatibility_sha256_hex",
              fragment.component_compatibility_sha256_hex);
        }
        if (!fragment.contract_hash.empty()) {
          append_view_line(&view, fragment_prefix + ".contract_hash",
                           fragment.contract_hash);
        }
        std::unordered_map<std::string, std::string> kv{};
        runtime_lls_document_t ignored_document{};
        std::string parse_error{};
        if (!fragment.payload_json.empty() &&
            parse_runtime_lls_payload(fragment.payload_json, &kv,
                                      &ignored_document, &parse_error)) {
          std::vector<std::pair<std::string, std::string>> keys{};
          keys.reserve(kv.size());
          for (const auto &[key, value] : kv) {
            keys.push_back({key, value});
          }
          std::sort(keys.begin(), keys.end(), [](const auto &a, const auto &b) {
            return a.first < b.first;
          });
          for (const auto &[key, value] : keys) {
            append_view_line(&view, fragment_prefix + ".kv." + key, value);
          }
        } else if (!parse_error.empty()) {
          append_view_line(&view, fragment_prefix + ".parse_error",
                           parse_error);
        }
      }
    }
    append_view_size_line(&view, "match_count", out->match_count);
    append_view_size_line(&view, "ambiguity_count", out->ambiguity_count);
    out->view_lls = view.str();
    return true;
  }

  if (!use_wave_cursor) {
    set_error(error,
              "view selector requires wave_cursor or intersection_cursor");
    return false;
  }

  std::string source_selector_canonical_path = "tsi.source.dataloader";
  std::string network_selector_canonical_path =
      "tsi.wikimyei.representation.encoding.vicreg";
  if (!selector_canonical_path.empty()) {
    if (runtime_hashimyei_cursor_matches(source_selector_canonical_path,
                                         selector_canonical_path)) {
      source_selector_canonical_path = selector_canonical_path;
    } else if (runtime_hashimyei_cursor_matches(network_selector_canonical_path,
                                                selector_canonical_path)) {
      network_selector_canonical_path = selector_canonical_path;
    } else {
      set_error(
          error,
          "intersection_cursor canonical_path is not supported for view_kind=" +
              view_kind_token);
      return false;
    }
  }

  out->canonical_path =
      std::string(kEntropicCapacityComparisonViewCanonicalPath);

  std::vector<runtime_report_fragment_t> source_rows{};
  std::vector<runtime_report_fragment_t> network_rows{};
  const auto collect_semantic_candidates =
      [&](std::string_view selector_canonical, std::string_view semantic_taxon,
          std::vector<runtime_report_fragment_t> *out_rows) {
        if (!out_rows)
          return;
        out_rows->clear();
        const std::string normalized_taxon =
            cuwacunu::piaabo::latent_lineage_state::
                normalize_runtime_report_semantic_taxon(semantic_taxon);
        for (const auto &[_, fragment] : runtime_report_fragments_by_id_) {
          if (use_wave_cursor && fragment.wave_cursor != wave_cursor)
            continue;
          const std::string fragment_taxon = cuwacunu::piaabo::
              latent_lineage_state::normalize_runtime_report_semantic_taxon(
                  fragment.semantic_taxon, fragment.schema);
          if (fragment_taxon != normalized_taxon)
            continue;
          if (normalized_taxon == cuwacunu::piaabo::latent_lineage_state::
                                      report_taxonomy::kSourceData &&
              fragment.schema != "piaabo.torch_compat.data_analytics.v2") {
            continue;
          }
          if (normalized_taxon == cuwacunu::piaabo::latent_lineage_state::
                                      report_taxonomy::kEmbeddingNetwork &&
              fragment.schema != "piaabo.torch_compat.network_analytics.v5") {
            continue;
          }
          if (!selector_canonical.empty()) {
            const std::string selector = trim_ascii(selector_canonical);
            if (!runtime_hashimyei_cursor_matches_fragment(selector,
                                                           fragment)) {
              continue;
            }
          }
          out_rows->push_back(fragment);
        }
      };
  collect_semantic_candidates(
      source_selector_canonical_path,
      cuwacunu::piaabo::latent_lineage_state::report_taxonomy::kSourceData,
      &source_rows);
  collect_semantic_candidates(network_selector_canonical_path,
                              cuwacunu::piaabo::latent_lineage_state::
                                  report_taxonomy::kEmbeddingNetwork,
                              &network_rows);

  std::unordered_map<std::uint64_t,
                     std::vector<entropic_capacity_view_candidate_t>>
      sources_by_wave{};
  std::unordered_map<std::uint64_t,
                     std::vector<entropic_capacity_view_candidate_t>>
      networks_by_wave{};
  auto append_candidate =
      [&](const runtime_report_fragment_t &row,
          std::unordered_map<std::uint64_t,
                             std::vector<entropic_capacity_view_candidate_t>>
              *out_groups) {
        if (!out_groups)
          return;
        entropic_capacity_view_candidate_t candidate{};
        if (!build_entropic_capacity_view_candidate(row, &candidate))
          return;
        if (use_wave_cursor && candidate.wave_cursor != wave_cursor)
          return;
        if (!contract_hash_token.empty() &&
            candidate.contract_hash != contract_hash_token) {
          return;
        }
        (*out_groups)[candidate.wave_cursor].push_back(std::move(candidate));
      };
  for (const auto &row : source_rows)
    append_candidate(row, &sources_by_wave);
  for (const auto &row : network_rows)
    append_candidate(row, &networks_by_wave);

  std::vector<std::uint64_t> group_wave_cursors{};
  group_wave_cursors.reserve(sources_by_wave.size() + networks_by_wave.size());
  for (const auto &[group_wave_cursor, _] : sources_by_wave) {
    if (const auto it = networks_by_wave.find(group_wave_cursor);
        it != networks_by_wave.end() && !it->second.empty()) {
      group_wave_cursors.push_back(group_wave_cursor);
    }
  }
  std::sort(group_wave_cursors.begin(), group_wave_cursors.end());
  group_wave_cursors.erase(
      std::unique(group_wave_cursors.begin(), group_wave_cursors.end()),
      group_wave_cursors.end());

  std::ostringstream view{};
  view << "/* synthetic view transport: "
       << kEntropicCapacityComparisonViewTransportSchema << " */\n";
  append_view_line(&view, "report_transport_schema",
                   kEntropicCapacityComparisonViewTransportSchema);
  append_view_line(&view, "view_kind", out->view_kind);
  append_view_line(&view, "canonical_path", out->canonical_path);
  if (!out->selector_intersection_cursor.empty()) {
    append_view_line(&view, "selector_intersection_cursor",
                     out->selector_intersection_cursor);
  }
  if (!out->selector_hashimyei_cursor.empty()) {
    append_view_line(&view, "selector_hashimyei_cursor",
                     out->selector_hashimyei_cursor);
  }
  if (!out->contract_hash.empty()) {
    append_view_line(&view, "contract_hash", out->contract_hash);
  }
  if (out->has_wave_cursor) {
    append_view_u64_line(&view, "wave_cursor", out->wave_cursor);
    append_view_line(&view, "wave_cursor_view",
                     format_runtime_wave_cursor(out->wave_cursor));
  }

  std::size_t block_index = 0;
  for (const std::uint64_t group_wave_cursor : group_wave_cursors) {
    const auto &sources = sources_by_wave[group_wave_cursor];
    const auto &networks = networks_by_wave[group_wave_cursor];
    if (sources.empty() || networks.empty())
      continue;

    ++block_index;
    view << "/* view block " << block_index << " */\n";
    append_view_u64_line(
        &view, "view_block." + std::to_string(block_index) + ".wave_cursor",
        group_wave_cursor);
    append_view_line(&view,
                     "view_block." + std::to_string(block_index) +
                         ".wave_cursor_view",
                     format_runtime_wave_cursor(group_wave_cursor));

    if (sources.size() == 1 && networks.size() == 1) {
      entropic_capacity_view_summary_t comparison{};
      std::string summarize_error{};
      if (!summarize_entropic_capacity_view_from_payloads(
              sources.front().fragment->payload_json,
              networks.front().fragment->payload_json, &comparison,
              &summarize_error)) {
        set_error(
            error,
            "failed to resolve entropic_capacity_comparison view for selector "
            "wave_cursor=" +
                std::to_string(group_wave_cursor) + ": " + summarize_error);
        return false;
      }

      ++out->match_count;
      append_view_line(&view,
                       "view_block." + std::to_string(block_index) + ".kind",
                       "comparison");
      append_view_line(&view,
                       "view_block." + std::to_string(block_index) +
                           ".source.report_fragment_id",
                       sources.front().fragment->report_fragment_id);
      append_view_line(&view,
                       "view_block." + std::to_string(block_index) +
                           ".source.canonical_path",
                       sources.front().fragment->canonical_path);
      if (!sources.front().fragment->source_label.empty()) {
        append_view_line(&view,
                         "view_block." + std::to_string(block_index) +
                             ".source.source_label",
                         sources.front().fragment->source_label);
      }
      if (!sources.front().fragment->semantic_taxon.empty()) {
        append_view_line(&view,
                         "view_block." + std::to_string(block_index) +
                             ".source.semantic_taxon",
                         sources.front().fragment->semantic_taxon);
      }
      if (!sources.front().fragment->binding_id.empty()) {
        append_view_line(&view,
                         "view_block." + std::to_string(block_index) +
                             ".source.binding_id",
                         sources.front().fragment->binding_id);
      }
      if (!sources.front().fragment->source_runtime_cursor.empty()) {
        append_view_line(&view,
                         "view_block." + std::to_string(block_index) +
                             ".source.source_runtime_cursor",
                         sources.front().fragment->source_runtime_cursor);
      }
      append_view_line(
          &view, "view_block." + std::to_string(block_index) + ".source.schema",
          sources.front().fragment->schema);
      if (!sources.front().contract_hash.empty()) {
        append_view_line(&view,
                         "view_block." + std::to_string(block_index) +
                             ".source.contract_hash",
                         sources.front().contract_hash);
      }
      append_view_line(&view,
                       "view_block." + std::to_string(block_index) +
                           ".network.report_fragment_id",
                       networks.front().fragment->report_fragment_id);
      append_view_line(&view,
                       "view_block." + std::to_string(block_index) +
                           ".network.canonical_path",
                       networks.front().fragment->canonical_path);
      if (!networks.front().fragment->semantic_taxon.empty()) {
        append_view_line(&view,
                         "view_block." + std::to_string(block_index) +
                             ".network.semantic_taxon",
                         networks.front().fragment->semantic_taxon);
      }
      if (!networks.front().fragment->binding_id.empty()) {
        append_view_line(&view,
                         "view_block." + std::to_string(block_index) +
                             ".network.binding_id",
                         networks.front().fragment->binding_id);
      }
      if (!networks.front().fragment->source_runtime_cursor.empty()) {
        append_view_line(&view,
                         "view_block." + std::to_string(block_index) +
                             ".network.source_runtime_cursor",
                         networks.front().fragment->source_runtime_cursor);
      }
      append_view_line(&view,
                       "view_block." + std::to_string(block_index) +
                           ".network.schema",
                       networks.front().fragment->schema);
      if (!networks.front().contract_hash.empty()) {
        append_view_line(&view,
                         "view_block." + std::to_string(block_index) +
                             ".network.contract_hash",
                         networks.front().contract_hash);
      }
      append_view_double_line(&view,
                              "view_block." + std::to_string(block_index) +
                                  ".source_entropic_load",
                              comparison.source_entropic_load);
      append_view_double_line(&view,
                              "view_block." + std::to_string(block_index) +
                                  ".network_entropic_capacity",
                              comparison.network_entropic_capacity);
      append_view_double_line(&view,
                              "view_block." + std::to_string(block_index) +
                                  ".capacity_margin",
                              comparison.capacity_margin);
      append_view_double_line(&view,
                              "view_block." + std::to_string(block_index) +
                                  ".capacity_ratio",
                              comparison.capacity_ratio);
      append_view_line(&view,
                       "view_block." + std::to_string(block_index) +
                           ".capacity_regime",
                       comparison.capacity_regime);
      continue;
    }

    ++out->ambiguity_count;
    append_view_line(&view,
                     "view_block." + std::to_string(block_index) + ".kind",
                     "ambiguity");
    append_view_size_line(
        &view, "view_block." + std::to_string(block_index) + ".source_count",
        sources.size());
    append_view_size_line(
        &view, "view_block." + std::to_string(block_index) + ".network_count",
        networks.size());
    for (std::size_t i = 0; i < sources.size(); ++i) {
      const auto &source = sources[i];
      const std::string prefix = "view_block." + std::to_string(block_index) +
                                 ".source." + std::to_string(i + 1);
      append_view_line(&view, prefix + ".report_fragment_id",
                       source.fragment->report_fragment_id);
      append_view_line(&view, prefix + ".canonical_path",
                       source.fragment->canonical_path);
      if (!source.fragment->source_label.empty()) {
        append_view_line(&view, prefix + ".source_label",
                         source.fragment->source_label);
      }
      if (!source.fragment->semantic_taxon.empty()) {
        append_view_line(&view, prefix + ".semantic_taxon",
                         source.fragment->semantic_taxon);
      }
      if (!source.fragment->binding_id.empty()) {
        append_view_line(&view, prefix + ".binding_id",
                         source.fragment->binding_id);
      }
      if (!source.fragment->source_runtime_cursor.empty()) {
        append_view_line(&view, prefix + ".source_runtime_cursor",
                         source.fragment->source_runtime_cursor);
      }
      append_view_line(&view, prefix + ".schema", source.fragment->schema);
      if (!source.contract_hash.empty()) {
        append_view_line(&view, prefix + ".contract_hash",
                         source.contract_hash);
      }
    }
    for (std::size_t i = 0; i < networks.size(); ++i) {
      const auto &network = networks[i];
      const std::string prefix = "view_block." + std::to_string(block_index) +
                                 ".network." + std::to_string(i + 1);
      append_view_line(&view, prefix + ".report_fragment_id",
                       network.fragment->report_fragment_id);
      append_view_line(&view, prefix + ".canonical_path",
                       network.fragment->canonical_path);
      if (!network.fragment->semantic_taxon.empty()) {
        append_view_line(&view, prefix + ".semantic_taxon",
                         network.fragment->semantic_taxon);
      }
      if (!network.fragment->binding_id.empty()) {
        append_view_line(&view, prefix + ".binding_id",
                         network.fragment->binding_id);
      }
      if (!network.fragment->source_runtime_cursor.empty()) {
        append_view_line(&view, prefix + ".source_runtime_cursor",
                         network.fragment->source_runtime_cursor);
      }
      append_view_line(&view, prefix + ".schema", network.fragment->schema);
      if (!network.contract_hash.empty()) {
        append_view_line(&view, prefix + ".contract_hash",
                         network.contract_hash);
      }
    }
  }

  append_view_size_line(&view, "match_count", out->match_count);
  append_view_size_line(&view, "ambiguity_count", out->ambiguity_count);
  out->view_lls = view.str();
  return true;
}

} // namespace wave
} // namespace hero
} // namespace cuwacunu
