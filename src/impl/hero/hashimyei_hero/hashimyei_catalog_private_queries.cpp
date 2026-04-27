bool hashimyei_catalog_store_t::get_run(std::string_view run_id,
                                        run_manifest_t *out,
                                        std::string *error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "run output pointer is null");
    return false;
  }
  const auto it = runs_by_id_.find(std::string(run_id));
  if (it == runs_by_id_.end()) {
    set_error(error, "run not found: " + std::string(run_id));
    return false;
  }
  *out = it->second;
  return true;
}

bool hashimyei_catalog_store_t::list_runs_by_binding(
    std::string_view contract_hashimyei, std::string_view wave_hashimyei,
    std::string_view binding_hashimyei, std::vector<run_manifest_t> *out,
    std::string *error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "run list output pointer is null");
    return false;
  }
  out->clear();
  for (const auto &[_, run] : runs_by_id_) {
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
  std::sort(out->begin(), out->end(),
            [](const run_manifest_t &a, const run_manifest_t &b) {
              if (a.started_at_ms != b.started_at_ms)
                return a.started_at_ms > b.started_at_ms;
              return a.run_id < b.run_id;
            });
  return true;
}

bool hashimyei_catalog_store_t::latest_report_fragment(
    std::string_view canonical_path, std::string_view schema,
    report_fragment_entry_t *out, std::string *error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "report_fragment output pointer is null");
    return false;
  }
  const std::string key = join_key(canonical_path, schema);
  const auto it = latest_report_fragment_by_key_.find(key);
  if (it == latest_report_fragment_by_key_.end()) {
    set_error(error, "latest report_fragment not found for key: " + key);
    return false;
  }
  const auto it_art = report_fragments_by_id_.find(it->second);
  if (it_art == report_fragments_by_id_.end()) {
    set_error(error,
              "catalog inconsistency: latest report_fragment id missing");
    return false;
  }
  *out = it_art->second;
  return true;
}

bool hashimyei_catalog_store_t::get_report_fragment(
    std::string_view report_fragment_id, report_fragment_entry_t *out,
    std::string *error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "report_fragment output pointer is null");
    return false;
  }
  const auto it = report_fragments_by_id_.find(std::string(report_fragment_id));
  if (it == report_fragments_by_id_.end()) {
    set_error(error,
              "report_fragment not found: " + std::string(report_fragment_id));
    return false;
  }
  *out = it->second;
  return true;
}

bool hashimyei_catalog_store_t::report_fragment_metrics(
    std::string_view report_fragment_id,
    std::vector<std::pair<std::string, double>> *out_numeric,
    std::vector<std::pair<std::string, std::string>> *out_text,
    std::string *error) const {
  clear_error(error);
  if (!out_numeric || !out_text) {
    set_error(error, "report_fragment metrics output pointer is null");
    return false;
  }
  out_numeric->clear();
  out_text->clear();

  const std::string id(report_fragment_id);
  const auto it_num = metrics_num_by_report_fragment_.find(id);
  if (it_num != metrics_num_by_report_fragment_.end())
    *out_numeric = it_num->second;
  const auto it_txt = metrics_txt_by_report_fragment_.find(id);
  if (it_txt != metrics_txt_by_report_fragment_.end())
    *out_text = it_txt->second;
  return true;
}

bool hashimyei_catalog_store_t::list_report_fragments(
    std::string_view canonical_path, std::string_view schema, std::size_t limit,
    std::size_t offset, bool newest_first,
    std::vector<report_fragment_entry_t> *out, std::string *error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "report_fragment list output pointer is null");
    return false;
  }
  out->clear();

  const std::string cp(canonical_path);
  const std::string sc(schema);
  for (const auto &[_, art] : report_fragments_by_id_) {
    if (!cp.empty() && art.canonical_path != cp)
      continue;
    if (!sc.empty() && art.schema != sc)
      continue;
    out->push_back(art);
  }

  std::sort(out->begin(), out->end(),
            [newest_first](const report_fragment_entry_t &a,
                           const report_fragment_entry_t &b) {
              if (a.ts_ms != b.ts_ms) {
                return newest_first ? (a.ts_ms > b.ts_ms) : (a.ts_ms < b.ts_ms);
              }
              return newest_first
                         ? (a.report_fragment_id > b.report_fragment_id)
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

bool hashimyei_catalog_store_t::resolve_component(
    std::string_view canonical_path, std::string_view hashimyei,
    component_state_t *out, std::string *error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "component output pointer is null");
    return false;
  }
  *out = component_state_t{};

  const std::string canonical =
      normalize_hashimyei_canonical_path_or_same(canonical_path);
  const std::string hash = normalize_hashimyei_name_or_same(hashimyei);

  if (!canonical.empty() && !hash.empty()) {
    component_state_t best{};
    bool found = false;
    for (const auto &[_, component] : components_by_id_) {
      if (component.manifest.canonical_path != canonical)
        continue;
      if (component.manifest.hashimyei_identity.name != hash)
        continue;
      if (!found || component.ts_ms > best.ts_ms ||
          (component.ts_ms == best.ts_ms &&
           component.component_id > best.component_id)) {
        best = component;
        found = true;
      }
    }
    if (!found) {
      set_error(error, "component not found for canonical_path/hashimyei");
      return false;
    }
    *out = std::move(best);
    return true;
  }

  if (!canonical.empty()) {
    const auto try_latest_component = [&](const std::string &key) -> bool {
      const auto it = latest_component_by_canonical_.find(key);
      if (it == latest_component_by_canonical_.end())
        return false;
      const auto it_comp = components_by_id_.find(it->second);
      if (it_comp == components_by_id_.end())
        return false;
      *out = it_comp->second;
      return true;
    };
    const auto try_active_component = [&](const std::string &key) -> bool {
      const auto it_active = active_component_by_canonical_.find(key);
      if (it_active == active_component_by_canonical_.end())
        return false;
      const auto it_comp = components_by_id_.find(it_active->second);
      if (it_comp == components_by_id_.end())
        return false;
      *out = it_comp->second;
      return true;
    };
    if (try_active_component(canonical))
      return true;
    const std::string hash_tail = maybe_hashimyei_from_canonical(canonical);
    if (!hash_tail.empty()) {
      // Explicit hash suffix means exact component identity lookup.
      if (try_latest_component(canonical))
        return true;
      set_error(error, "component not found for canonical_path: " + canonical);
      return false;
    }

    if (try_latest_component(canonical))
      return true;

    set_error(error, "component not found for canonical_path: " + canonical);
    return false;
  }

  if (!hash.empty()) {
    const auto it = latest_component_by_hashimyei_.find(hash);
    if (it == latest_component_by_hashimyei_.end()) {
      set_error(error, "component not found for hashimyei: " + hash);
      return false;
    }
    const auto it_comp = components_by_id_.find(it->second);
    if (it_comp == components_by_id_.end()) {
      set_error(error, "catalog inconsistency: latest component id missing");
      return false;
    }
    *out = it_comp->second;
    return true;
  }

  set_error(error, "resolve_component requires canonical_path or hashimyei");
  return false;
}

bool hashimyei_catalog_store_t::list_components(
    std::string_view canonical_path, std::string_view hashimyei,
    std::size_t limit, std::size_t offset, bool newest_first,
    std::vector<component_state_t> *out, std::string *error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "component list output pointer is null");
    return false;
  }
  out->clear();
  const std::string canonical =
      normalize_hashimyei_canonical_path_or_same(canonical_path);
  const std::string hash = normalize_hashimyei_name_or_same(hashimyei);

  for (const auto &[_, component] : components_by_id_) {
    if (!canonical.empty() && component.manifest.canonical_path != canonical)
      continue;
    if (!hash.empty() && component.manifest.hashimyei_identity.name != hash) {
      continue;
    }
    out->push_back(component);
  }

  std::sort(
      out->begin(), out->end(),
      [newest_first](const component_state_t &a, const component_state_t &b) {
        if (a.ts_ms != b.ts_ms) {
          return newest_first ? (a.ts_ms > b.ts_ms) : (a.ts_ms < b.ts_ms);
        }
        return newest_first ? (a.component_id > b.component_id)
                            : (a.component_id < b.component_id);
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

bool hashimyei_catalog_store_t::list_component_heads(
    std::size_t limit, std::size_t offset, bool newest_first,
    std::vector<component_state_t> *out, std::string *error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "component head list output pointer is null");
    return false;
  }
  out->clear();

  std::unordered_map<std::string, std::string> selected_by_canonical{};
  selected_by_canonical.reserve(latest_component_by_canonical_.size() +
                                active_component_by_canonical_.size());
  for (const auto &[canonical_path, component_id] :
       latest_component_by_canonical_) {
    selected_by_canonical[canonical_path] = component_id;
  }
  for (const auto &[canonical_path, component_id] :
       active_component_by_canonical_) {
    selected_by_canonical[canonical_path] = component_id;
  }

  std::unordered_set<std::string> emitted_component_ids{};
  out->reserve(selected_by_canonical.size());
  for (const auto &[_, component_id] : selected_by_canonical) {
    if (!emitted_component_ids.insert(component_id).second)
      continue;
    const auto it = components_by_id_.find(component_id);
    if (it == components_by_id_.end())
      continue;
    out->push_back(it->second);
  }

  std::sort(
      out->begin(), out->end(),
      [newest_first](const component_state_t &a, const component_state_t &b) {
        if (a.ts_ms != b.ts_ms) {
          return newest_first ? (a.ts_ms > b.ts_ms) : (a.ts_ms < b.ts_ms);
        }
        return newest_first ? (a.component_id > b.component_id)
                            : (a.component_id < b.component_id);
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

bool hashimyei_catalog_store_t::resolve_active_hashimyei(
    std::string_view canonical_path, std::string_view family,
    std::string_view contract_hash, std::string *out_hashimyei,
    std::string *error) const {
  clear_error(error);
  if (!out_hashimyei) {
    set_error(error, "active hashimyei output pointer is null");
    return false;
  }
  out_hashimyei->clear();

  const std::string canonical =
      normalize_hashimyei_canonical_path_or_same(canonical_path);
  const std::string fam = normalize_hashimyei_canonical_path_or_same(family);
  const std::string contract = trim_ascii(contract_hash);
  if (canonical.empty()) {
    set_error(error, "canonical_path is required");
    return false;
  }
  if (contract.empty()) {
    set_error(error, "contract_hash is required");
    return false;
  }
  const std::string canonical_hash_tail =
      maybe_hashimyei_from_canonical(canonical);
  std::string canonical_lookup = canonical;
  if (!canonical_hash_tail.empty()) {
    canonical_lookup =
        component_family_from_parts(canonical, canonical_hash_tail);
  }

  if (!fam.empty()) {
    const std::string key = component_active_pointer_key(
        canonical_lookup, fam, canonical_hash_tail, contract);
    const auto it = active_hashimyei_by_key_.find(key);
    if (it == active_hashimyei_by_key_.end()) {
      set_error(
          error,
          "active hashimyei not found for canonical_path/family/contract");
      return false;
    }
    *out_hashimyei = it->second;
    return true;
  }

  std::string best_key{};
  std::uint64_t best_ts = 0;
  bool found = false;
  for (const auto &[key, value] : active_hashimyei_by_key_) {
    std::string key_canonical{};
    std::string key_family{};
    std::string key_contract{};
    if (!parse_component_active_pointer_key(key, &key_canonical, &key_family,
                                            &key_contract)) {
      continue;
    }
    if (key_canonical != canonical_lookup)
      continue;
    if (key_contract != contract)
      continue;
    std::uint64_t ts = 0;
    if (const auto it_comp = active_component_by_key_.find(key);
        it_comp != active_component_by_key_.end()) {
      if (const auto it_state = components_by_id_.find(it_comp->second);
          it_state != components_by_id_.end()) {
        ts = it_state->second.ts_ms;
      }
    }
    if (!found || ts > best_ts || (ts == best_ts && key > best_key)) {
      best_key = key;
      best_ts = ts;
      *out_hashimyei = value;
      found = true;
    }
  }
  if (!found) {
    set_error(error, "active hashimyei not found for canonical_path");
    return false;
  }
  return true;
}

bool hashimyei_catalog_store_t::get_explicit_family_rank(
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

bool hashimyei_catalog_store_t::get_family_rank(
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

bool hashimyei_catalog_store_t::resolve_ranked_hashimyei(
    std::string_view family,
    std::string_view component_compatibility_sha256_hex, std::uint64_t rank,
    std::string *out_hashimyei, std::string *error) const {
  clear_error(error);
  if (!out_hashimyei) {
    set_error(error, "ranked hashimyei output pointer is null");
    return false;
  }
  out_hashimyei->clear();

  cuwacunu::hero::family_rank::state_t rank_state{};
  if (!get_family_rank(family, component_compatibility_sha256_hex, &rank_state,
                       error))
    return false;
  for (const auto &assignment : rank_state.assignments) {
    if (assignment.rank != rank)
      continue;
    *out_hashimyei = assignment.hashimyei;
    return true;
  }

  set_error(error, "family rank not found for requested rank");
  return false;
}

bool hashimyei_catalog_store_t::register_component_manifest(
    const component_manifest_t &manifest_in, std::string *out_component_id,
    bool *out_inserted, std::string *error) {
  clear_error(error);
  if (out_component_id)
    out_component_id->clear();
  if (out_inserted)
    *out_inserted = false;
  if (!db_) {
    set_error(error, "catalog is not open");
    return false;
  }

  component_manifest_t manifest = manifest_in;
  if (trim_ascii(manifest.schema).empty()) {
    manifest.schema = cuwacunu::hashimyei::kComponentManifestSchemaV2;
  }
  if (!ensure_identity_allocated_(&manifest.hashimyei_identity, error))
    return false;
  if (!ensure_identity_allocated_(&manifest.contract_identity, error))
    return false;
  const std::string contract_hash =
      cuwacunu::hero::hashimyei::contract_hash_from_identity(
          manifest.contract_identity);
  if (trim_ascii(manifest.docking_signature_sha256_hex).empty() &&
      !trim_ascii(contract_hash).empty()) {
    const auto contract_snapshot =
        cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash);
    manifest.docking_signature_sha256_hex =
        contract_snapshot->signature.docking_signature_sha256_hex;
  }
  if (manifest.parent_identity.has_value() &&
      !manifest.parent_identity->name.empty()) {
    std::string parent_error;
    if (!cuwacunu::hashimyei::validate_hashimyei(*manifest.parent_identity,
                                                 &parent_error)) {
      set_error(error, "parent_identity invalid: " + parent_error);
      return false;
    }
  }
  if (manifest.created_at_ms == 0)
    manifest.created_at_ms = now_ms_utc();
  if (manifest.updated_at_ms == 0)
    manifest.updated_at_ms = manifest.created_at_ms;
  if (!validate_component_manifest(manifest, error))
    return false;

  if (!manifest.hashimyei_identity.name.empty()) {
    const auto existing_it =
        latest_component_by_hashimyei_.find(manifest.hashimyei_identity.name);
    if (existing_it != latest_component_by_hashimyei_.end()) {
      const auto component_it = components_by_id_.find(existing_it->second);
      if (component_it != components_by_id_.end() &&
          !same_contract_identity_(
              component_it->second.manifest.contract_identity,
              manifest.contract_identity)) {
        set_error(error, "component hashimyei is contract-scoped and cannot be "
                         "reused across "
                         "contracts: " +
                             manifest.hashimyei_identity.name);
        return false;
      }
    }
  }

  const std::string payload = component_manifest_payload(manifest);
  std::string report_fragment_sha;
  if (!sha256_hex_bytes(payload, &report_fragment_sha)) {
    set_error(error, "failed to compute component manifest payload sha256");
    return false;
  }

  const std::string component_id = compute_component_manifest_id(manifest);
  if (out_component_id)
    *out_component_id = component_id;
  const std::filesystem::path store_root =
      store_root_from_catalog_path(options_.catalog_path);
  std::filesystem::path manifest_path = component_manifest_path(
      store_root, manifest.canonical_path, component_id);
  if (!std::filesystem::exists(manifest_path)) {
    if (!save_component_manifest(store_root, manifest, &manifest_path, error)) {
      return false;
    }
  }

  bool already = false;
  if (!ledger_contains_(report_fragment_sha, &already, error))
    return false;
  if (already)
    return true;

  const std::uint64_t ts_ms =
      manifest.updated_at_ms != 0
          ? manifest.updated_at_ms
          : (manifest.created_at_ms != 0 ? manifest.created_at_ms
                                         : now_ms_utc());

  const std::string manifest_path_s = manifest_path.string();
  const bool is_active = trim_ascii(manifest.lineage_state) == "active";
  std::string active_pointer_key{};

  if (!append_row_(cuwacunu::hero::schema::kRecordKindCOMPONENT, component_id,
                   "", manifest.canonical_path,
                   manifest.hashimyei_identity.name, manifest.schema,
                   manifest.family, std::numeric_limits<double>::quiet_NaN(),
                   manifest.lineage_state, report_fragment_sha, manifest_path_s,
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
                   manifest.founding_dsl_source_sha256_hex, "", manifest_path_s,
                   std::to_string(ts_ms), source_payload, error)) {
    return false;
  }
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
            manifest_path_s, std::to_string(ts_ms), bundle_payload, error)) {
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
                   "", manifest_path_s, std::to_string(ts_ms),
                   manifest.revision_reason, error)) {
    return false;
  }

  if (is_active) {
    const std::string contract_hash =
        cuwacunu::hero::hashimyei::contract_hash_from_identity(
            manifest.contract_identity);
    active_pointer_key = component_active_pointer_key(
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
                     active_pointer_key, "", manifest.canonical_path,
                     manifest.hashimyei_identity.name,
                     cuwacunu::hashimyei::kComponentActivePointerSchemaV2,
                     manifest.family, std::numeric_limits<double>::quiet_NaN(),
                     manifest.founding_revision_id, "", manifest_path_s,
                     std::to_string(ts_ms), pointer_payload.str(), error)) {
      return false;
    }
  }

  if (!append_ledger_(report_fragment_sha, manifest_path_s, error))
    return false;

  component_state_t component{};
  component.component_id = component_id;
  component.ts_ms = ts_ms;
  component.manifest_path = manifest_path_s;
  component.report_fragment_sha256 = report_fragment_sha;
  component.manifest = manifest;
  observe_component_(component);
  if (is_active && !active_pointer_key.empty()) {
    active_hashimyei_by_key_[active_pointer_key] =
        manifest.hashimyei_identity.name;
    refresh_active_component_views_();
  }
  refresh_component_family_ranks_();

  if (out_inserted)
    *out_inserted = true;
  return true;
}

bool hashimyei_catalog_store_t::latest_report_fragment_snapshot(
    std::string_view canonical_path, report_fragment_snapshot_t *out,
    std::string *error) const {
  clear_error(error);
  if (!out) {
    set_error(error, "latest report_fragment snapshot output pointer is null");
    return false;
  }
  *out = report_fragment_snapshot_t{};

  const std::string cp(canonical_path);
  report_fragment_entry_t best{};
  bool found = false;
  for (const auto &[_, art] : report_fragments_by_id_) {
    if (art.canonical_path != cp)
      continue;
    if (!found || art.ts_ms > best.ts_ms ||
        (art.ts_ms == best.ts_ms &&
         art.report_fragment_id > best.report_fragment_id)) {
      best = art;
      found = true;
    }
  }
  if (!found) {
    set_error(error, "latest report_fragment snapshot not found");
    return false;
  }

  out->report_fragment = best;
  (void)report_fragment_metrics(best.report_fragment_id, &out->numeric_metrics,
                                &out->text_metrics, nullptr);
  return true;
}

} // namespace hashimyei
} // namespace hero
} // namespace cuwacunu
