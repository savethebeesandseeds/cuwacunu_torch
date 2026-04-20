using hashimyei_tool_handler_t = bool (*)(app_context_t *app,
                                          const std::string &arguments_json,
                                          std::string *out_structured,
                                          std::string *out_error);

struct hashimyei_tool_descriptor_t {
  std::string_view name;
  std::string_view description;
  std::string_view input_schema_json;
  hashimyei_tool_handler_t handler;
};

[[nodiscard]] bool handle_tool_list(app_context_t *app,
                                    const std::string &arguments_json,
                                    std::string *out_structured,
                                    std::string *out_error);
[[nodiscard]] bool handle_tool_get_founding_dsl_bundle(
    app_context_t *app, const std::string &arguments_json,
    std::string *out_structured, std::string *out_error);
[[nodiscard]] bool handle_tool_get_component_manifest(
    app_context_t *app, const std::string &arguments_json,
    std::string *out_structured, std::string *out_error);
[[nodiscard]] bool handle_tool_evaluate_contract_compatibility(
    app_context_t *app, const std::string &arguments_json,
    std::string *out_structured, std::string *out_error);
[[nodiscard]] bool handle_tool_update_rank(app_context_t *app,
                                           const std::string &arguments_json,
                                           std::string *out_structured,
                                           std::string *out_error);
[[nodiscard]] bool handle_tool_reset_catalog(app_context_t *app,
                                             const std::string &arguments_json,
                                             std::string *out_structured,
                                             std::string *out_error);

constexpr hashimyei_tool_descriptor_t kHashimyeiTools[] = {
#define HERO_HASHIMYEI_TOOL(NAME, DESCRIPTION, INPUT_SCHEMA_JSON, HANDLER)     \
  hashimyei_tool_descriptor_t{NAME, DESCRIPTION, INPUT_SCHEMA_JSON, HANDLER},
#include "hero/hashimyei_hero/hero_hashimyei_tools.def"
#undef HERO_HASHIMYEI_TOOL
};

[[nodiscard]] const hashimyei_tool_descriptor_t *
find_hashimyei_tool_descriptor(std::string_view name) {
  for (const auto &descriptor : kHashimyeiTools) {
    if (descriptor.name == name)
      return &descriptor;
  }
  return nullptr;
}

[[nodiscard]] std::string tools_list_result_json() {
  std::ostringstream out;
  out << "{\"tools\":[";
  for (std::size_t i = 0; i < std::size(kHashimyeiTools); ++i) {
    if (i != 0)
      out << ",";
    const bool read_only =
        kHashimyeiTools[i].name == "hero.hashimyei.list" ||
        kHashimyeiTools[i].name == "hero.hashimyei.get_component_manifest" ||
        kHashimyeiTools[i].name ==
            "hero.hashimyei.evaluate_contract_compatibility" ||
        kHashimyeiTools[i].name == "hero.hashimyei.get_founding_dsl_bundle";
    out << "{\"name\":" << json_quote(kHashimyeiTools[i].name)
        << ",\"description\":" << json_quote(kHashimyeiTools[i].description)
        << ",\"inputSchema\":" << kHashimyeiTools[i].input_schema_json
        << ",\"annotations\":{\"readOnlyHint\":"
        << (read_only ? "true" : "false")
        << ",\"destructiveHint\":false,\"openWorldHint\":false}}";
  }
  out << "]}";
  return out.str();
}

[[nodiscard]] std::string tools_list_human_text() {
  std::ostringstream out;
  for (const auto &descriptor : kHashimyeiTools) {
    out << descriptor.name << "\t" << descriptor.description << "\n";
  }
  return out.str();
}

[[nodiscard]] std::string
make_tool_result_json(std::string_view text, std::string_view structured_json,
                      bool is_error) {
  std::ostringstream out;
  out << "{\"content\":[{\"type\":\"text\",\"text\":" << json_quote(text)
      << "}],\"structuredContent\":" << structured_json
      << ",\"isError\":" << (is_error ? "true" : "false") << "}";
  return out.str();
}

[[nodiscard]] std::string
tool_result_text_for_structured_json(const std::string &structured_json) {
  std::string summary{};
  if (extract_json_string_field(structured_json, "summary", &summary) &&
      !summary.empty()) {
    return summary;
  }
  return "ok";
}

[[nodiscard]] bool tool_requires_catalog_sync(std::string_view tool_name) {
  return tool_name != "hero.hashimyei.reset_catalog" &&
         tool_name != "hero.hashimyei.list" &&
         tool_name != "hero.hashimyei.get_component_manifest" &&
         tool_name != "hero.hashimyei.evaluate_contract_compatibility" &&
         tool_name != "hero.hashimyei.get_founding_dsl_bundle";
}

[[nodiscard]] bool tool_requires_catalog_preopen(std::string_view tool_name) {
  return tool_name != "hero.hashimyei.reset_catalog" &&
         tool_name != "hero.hashimyei.list" &&
         tool_name != "hero.hashimyei.get_component_manifest" &&
         tool_name != "hero.hashimyei.evaluate_contract_compatibility" &&
         tool_name != "hero.hashimyei.get_founding_dsl_bundle";
}

struct store_discovery_snapshot_t {
  std::vector<cuwacunu::hero::hashimyei::component_state_t> components{};
  std::vector<cuwacunu::hero::hashimyei::report_fragment_entry_t>
      report_fragments{};
};

[[nodiscard]] bool load_component_state_from_manifest_path(
    const std::filesystem::path &path,
    cuwacunu::hero::hashimyei::component_state_t *out, std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (!out) {
    if (out_error)
      *out_error = "component output pointer is null";
    return false;
  }
  *out = cuwacunu::hero::hashimyei::component_state_t{};

  cuwacunu::hero::hashimyei::component_manifest_t manifest{};
  if (!cuwacunu::hero::hashimyei::load_component_manifest(path, &manifest,
                                                          out_error)) {
    if (out_error != nullptr &&
        *out_error == "pre-hard-cut component manifest is not supported after "
                      "the hard cut") {
      out_error->clear();
      return true;
    }
    return false;
  }

  std::string payload{};
  if (!read_text_file(path, &payload, out_error))
    return false;

  std::string payload_sha{};
  if (!sha256_hex_bytes(payload, &payload_sha)) {
    if (out_error) {
      *out_error = "cannot compute component manifest sha256: " + path.string();
    }
    return false;
  }

  out->component_id =
      cuwacunu::hero::hashimyei::compute_component_manifest_id(manifest);
  out->ts_ms = manifest.updated_at_ms != 0 ? manifest.updated_at_ms
                                           : manifest.created_at_ms;
  out->manifest_path =
      canonicalized(path).value_or(path.lexically_normal()).string();
  out->report_fragment_sha256 = std::move(payload_sha);
  out->manifest = std::move(manifest);
  return true;
}

[[nodiscard]] bool
scan_store_discovery_snapshot(app_context_t *app, bool include_report_fragments,
                              store_discovery_snapshot_t *out,
                              std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (!app || !out) {
    if (out_error)
      *out_error = "missing store discovery context";
    return false;
  }
  out->components.clear();
  out->report_fragments.clear();

  const std::filesystem::path store_root = app->store_root;
  if (store_root.empty()) {
    if (out_error)
      *out_error = "store_root is empty";
    return false;
  }

  const std::filesystem::path normalized_store_root =
      store_root.lexically_normal();
  const std::filesystem::path normalized_catalog_root =
      app->catalog_options.catalog_path.parent_path().empty()
          ? std::filesystem::path{}
          : app->catalog_options.catalog_path.parent_path().lexically_normal();
  const std::filesystem::path family_rank_root =
      store_root / "hero" / "family_rank";
  const std::filesystem::path normalized_family_rank_root =
      family_rank_root.lexically_normal();

  std::unordered_map<std::string, cuwacunu::hero::family_rank::state_t>
      family_rank_by_scope{};
  std::unordered_map<std::string, std::uint64_t> family_rank_ts_by_scope{};
  std::unordered_map<std::string, std::string> family_rank_path_by_scope{};

  std::error_code ec{};
  for (std::filesystem::recursive_directory_iterator it(store_root, ec), end;
       it != end; it.increment(ec)) {
    if (ec) {
      if (out_error)
        *out_error = "failed scanning hashimyei store";
      return false;
    }

    std::error_code entry_ec{};
    const auto symlink_state = it->symlink_status(entry_ec);
    if (entry_ec) {
      if (out_error)
        *out_error = "failed reading hashimyei store entry status";
      return false;
    }
    if (std::filesystem::is_symlink(symlink_state))
      continue;
    if (!it->is_regular_file(entry_ec)) {
      if (entry_ec) {
        if (out_error)
          *out_error = "failed reading hashimyei store entry type";
        return false;
      }
      continue;
    }

    const std::filesystem::path entry_path = it->path();
    const std::filesystem::path normalized_entry =
        entry_path.lexically_normal();
    if (!path_is_within(normalized_store_root, normalized_entry))
      continue;
    if (!normalized_catalog_root.empty() &&
        path_is_within(normalized_catalog_root, normalized_entry)) {
      continue;
    }

    if (entry_path.filename() ==
        cuwacunu::hashimyei::kComponentManifestFilenameV2) {
      cuwacunu::hero::hashimyei::component_state_t component{};
      if (!load_component_state_from_manifest_path(entry_path, &component,
                                                   out_error)) {
        return false;
      }
      if (component.component_id.empty())
        continue;
      out->components.push_back(std::move(component));
      continue;
    }

    if (entry_path.extension() != ".lls")
      continue;

    std::string payload{};
    if (!read_text_file(entry_path, &payload, out_error))
      return false;
    if (payload.empty())
      continue;

    std::unordered_map<std::string, std::string> kv{};
    std::string parse_error{};
    if (!cuwacunu::hero::hashimyei::parse_latent_lineage_state_payload(payload,
                                                                       &kv)) {
      if (out_error) {
        *out_error = "cannot parse runtime payload: " + entry_path.string();
      }
      return false;
    }

    const std::string schema = trim_ascii(kv["schema"]);
    if (schema.empty())
      continue;

    if (path_is_within(normalized_family_rank_root, normalized_entry)) {
      cuwacunu::hero::family_rank::state_t rank_state{};
      if (!cuwacunu::hero::family_rank::parse_state_from_kv(kv, &rank_state,
                                                            &parse_error)) {
        if (out_error) {
          *out_error = "family rank parse failure for " + entry_path.string() +
                       ": " + parse_error;
        }
        return false;
      }

      const std::string scope_key = cuwacunu::hero::family_rank::scope_key(
          rank_state.family, rank_state.dock_hash);
      const std::uint64_t row_ts = rank_state.updated_at_ms;
      const std::string row_path = normalized_entry.string();
      const auto current_ts_it = family_rank_ts_by_scope.find(scope_key);
      const bool should_replace =
          current_ts_it == family_rank_ts_by_scope.end() ||
          row_ts > current_ts_it->second ||
          (row_ts == current_ts_it->second &&
           row_path > family_rank_path_by_scope[scope_key]);
      if (should_replace) {
        family_rank_ts_by_scope[scope_key] = row_ts;
        family_rank_path_by_scope[scope_key] = row_path;
        family_rank_by_scope[scope_key] = std::move(rank_state);
      }
      continue;
    }

    if (!include_report_fragments)
      continue;

    cuwacunu::hero::hashimyei::report_fragment_entry_t fragment{};
    fragment.schema = schema;
    fragment.path = normalized_entry.string();
    fragment.payload_json = payload;

    cuwacunu::piaabo::latent_lineage_state::runtime_report_header_t header{};
    if (cuwacunu::piaabo::latent_lineage_state::
            parse_runtime_report_header_from_kv(kv, &header, nullptr)) {
      fragment.semantic_taxon = trim_ascii(header.semantic_taxon);
      fragment.report_canonical_path =
          trim_ascii(header.context.canonical_path);
      fragment.binding_id = trim_ascii(header.context.binding_id);
      fragment.source_runtime_cursor =
          trim_ascii(header.context.source_runtime_cursor);
      if (header.context.has_wave_cursor) {
        fragment.has_wave_cursor = true;
        fragment.wave_cursor = header.context.wave_cursor;
      }
    }

    fragment.source_label = trim_ascii(kv["source_label"]);
    fragment.canonical_path =
        cuwacunu::hashimyei::normalize_hashimyei_canonical_path(
            trim_ascii(kv["canonical_path"]));
    if (fragment.canonical_path.empty()) {
      fragment.canonical_path =
          cuwacunu::hashimyei::normalize_hashimyei_canonical_path(
              fragment.report_canonical_path);
    }
    fragment.hashimyei = trim_ascii(kv["hashimyei"]);
    cuwacunu::hashimyei::normalize_hex_hash_name_inplace(&fragment.hashimyei);
    if (fragment.hashimyei.empty()) {
      fragment.hashimyei =
          maybe_hashimyei_from_canonical(fragment.canonical_path);
    }
    if (fragment.canonical_path.empty() && fragment.hashimyei.empty())
      continue;

    std::string payload_sha{};
    if (!sha256_hex_bytes(payload, &payload_sha)) {
      if (out_error) {
        *out_error =
            "cannot compute report fragment sha256: " + entry_path.string();
      }
      return false;
    }
    fragment.report_fragment_id = payload_sha;
    fragment.report_fragment_sha256 = std::move(payload_sha);

    std::error_code ts_ec{};
    const auto fts = std::filesystem::last_write_time(entry_path, ts_ec);
    if (!ts_ec) {
      const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          fts.time_since_epoch())
                          .count();
      if (ms > 0)
        fragment.ts_ms = static_cast<std::uint64_t>(ms);
    }

    out->report_fragments.push_back(std::move(fragment));
  }

  for (auto &component : out->components) {
    const std::string dock_hash =
        trim_ascii(component.manifest.docking_signature_sha256_hex);
    if (dock_hash.empty()) continue;
    const std::string scope_key = cuwacunu::hero::family_rank::scope_key(
        component.manifest.family, dock_hash);
    const auto it = family_rank_by_scope.find(scope_key);
    if (it == family_rank_by_scope.end())
      continue;
    component.family_rank = cuwacunu::hero::family_rank::rank_for_hashimyei(
        it->second, component.manifest.hashimyei_identity.name);
  }
  return true;
}

[[nodiscard]] bool resolve_component_from_store_snapshot(
    const store_discovery_snapshot_t &snapshot, std::string_view canonical_path,
    std::string_view hashimyei,
    cuwacunu::hero::hashimyei::component_state_t *out, std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (!out) {
    if (out_error)
      *out_error = "component output pointer is null";
    return false;
  }
  *out = cuwacunu::hero::hashimyei::component_state_t{};

  const std::string canonical =
      cuwacunu::hashimyei::normalize_hashimyei_canonical_path(
          trim_ascii(canonical_path));
  std::string hash = trim_ascii(hashimyei);
  cuwacunu::hashimyei::normalize_hex_hash_name_inplace(&hash);

  const auto is_newer = [](const auto &candidate, const auto &current) {
    return candidate.ts_ms > current.ts_ms ||
           (candidate.ts_ms == current.ts_ms &&
            candidate.component_id > current.component_id);
  };

  if (!canonical.empty() && !hash.empty()) {
    bool found = false;
    cuwacunu::hero::hashimyei::component_state_t best{};
    for (const auto &component : snapshot.components) {
      const bool canonical_match =
          component.manifest.canonical_path == canonical ||
          component.manifest.family == canonical;
      if (!canonical_match)
        continue;
      if (component.manifest.hashimyei_identity.name != hash)
        continue;
      if (!found || is_newer(component, best)) {
        best = component;
        found = true;
      }
    }
    if (!found) {
      if (out_error) {
        *out_error = "component not found for canonical_path/hashimyei";
      }
      return false;
    }
    *out = std::move(best);
    return true;
  }

  if (!canonical.empty()) {
    const std::string canonical_hash_tail =
        maybe_hashimyei_from_canonical(canonical);
    bool found = false;
    cuwacunu::hero::hashimyei::component_state_t best{};

    if (!canonical_hash_tail.empty()) {
      for (const auto &component : snapshot.components) {
        if (component.manifest.canonical_path != canonical)
          continue;
        if (!found || is_newer(component, best)) {
          best = component;
          found = true;
        }
      }
    } else {
      for (const auto &component : snapshot.components) {
        if (component.manifest.family != canonical)
          continue;
        if (trim_ascii(component.manifest.lineage_state) != "active")
          continue;
        if (!found || is_newer(component, best)) {
          best = component;
          found = true;
        }
      }
      if (!found) {
        for (const auto &component : snapshot.components) {
          if (component.manifest.canonical_path != canonical)
            continue;
          if (!found || is_newer(component, best)) {
            best = component;
            found = true;
          }
        }
      }
    }

    if (!found) {
      if (out_error) {
        *out_error = "component not found for canonical_path: " + canonical;
      }
      return false;
    }
    *out = std::move(best);
    return true;
  }

  if (!hash.empty()) {
    bool found = false;
    cuwacunu::hero::hashimyei::component_state_t best{};
    for (const auto &component : snapshot.components) {
      if (component.manifest.hashimyei_identity.name != hash)
        continue;
      if (!found || is_newer(component, best)) {
        best = component;
        found = true;
      }
    }
    if (!found) {
      if (out_error)
        *out_error = "component not found for hashimyei: " + hash;
      return false;
    }
    *out = std::move(best);
    return true;
  }

  if (out_error) {
    *out_error = "resolve_component requires canonical_path or hashimyei";
  }
  return false;
}

struct resolved_contract_snapshot_t {
  std::string contract_hash{};
  std::string contract_dsl_path{};
  std::string contract_source{};
  std::string docking_signature_sha256_hex{};
  std::shared_ptr<const cuwacunu::iitepi::contract_record_t> snapshot{};
};

[[nodiscard]] std::filesystem::path
effective_global_config_path(const app_context_t *app) {
  if (app != nullptr && !app->global_config_path.empty()) {
    return app->global_config_path;
  }
  return std::filesystem::path(kDefaultGlobalConfigPath);
}

[[nodiscard]] std::filesystem::path
resolve_contract_dsl_input_path(const app_context_t *app,
                                std::string_view raw_path) {
  const std::string path_text = trim_ascii(raw_path);
  if (path_text.empty())
    return {};

  const std::filesystem::path input_path(path_text);
  if (input_path.is_absolute()) {
    return input_path.lexically_normal();
  }

  std::error_code ec{};
  if (std::filesystem::exists(input_path, ec)) {
    return canonicalized(input_path).value_or(input_path.lexically_normal());
  }

  const auto global_config_path = effective_global_config_path(app);
  const std::filesystem::path config_relative =
      global_config_path.parent_path() / input_path;
  ec.clear();
  if (std::filesystem::exists(config_relative, ec)) {
    return canonicalized(config_relative)
        .value_or(config_relative.lexically_normal());
  }

  return config_relative.lexically_normal();
}

[[nodiscard]] bool ensure_iitepi_config_loaded(const app_context_t *app,
                                               std::string *out_error) {
  if (out_error)
    out_error->clear();
  const std::filesystem::path global_config_path =
      effective_global_config_path(app);
  if (global_config_path.empty()) {
    if (out_error)
      *out_error = "global_config_path is empty";
    return false;
  }

  std::error_code ec{};
  if (!std::filesystem::exists(global_config_path, ec) ||
      !std::filesystem::is_regular_file(global_config_path, ec)) {
    if (out_error) {
      *out_error =
          "global config file not found: " + global_config_path.string();
    }
    return false;
  }

  try {
    cuwacunu::iitepi::config_space_t::change_config_file(
        global_config_path.string().c_str());
    cuwacunu::iitepi::config_space_t::update_config();
  } catch (const std::exception &e) {
    if (out_error) {
      *out_error = "failed to load iitepi global config '" +
                   global_config_path.string() + "': " + e.what();
    }
    return false;
  } catch (...) {
    if (out_error) {
      *out_error = "failed to load iitepi global config '" +
                   global_config_path.string() + "'";
    }
    return false;
  }

  return true;
}

[[nodiscard]] bool resolve_requested_contract_snapshot(
    app_context_t *app, std::string_view requested_contract_hash,
    std::string_view requested_contract_dsl_path,
    resolved_contract_snapshot_t *out, std::string *out_error) {
  if (out_error)
    out_error->clear();
  if (!out) {
    if (out_error)
      *out_error = "requested contract output pointer is null";
    return false;
  }
  *out = resolved_contract_snapshot_t{};

  std::string contract_hash =
      lowercase_copy(trim_ascii(requested_contract_hash));
  std::string contract_path = trim_ascii(requested_contract_dsl_path);
  if (contract_hash.empty() && contract_path.empty()) {
    if (out_error) {
      *out_error = "evaluate_contract_compatibility requires "
                   "arguments.contract_dsl_path "
                   "or arguments.contract_hash";
    }
    return false;
  }

  if (!contract_path.empty()) {
    const auto global_config_path = effective_global_config_path(app);
    const std::filesystem::path resolved_path =
        resolve_contract_dsl_input_path(app, contract_path);
    contract_path = resolved_path.string();

    std::string validate_error{};
    if (!cuwacunu::iitepi::contract_space_t::validate_contract_file_isolated(
            contract_path, global_config_path.string(), &validate_error)) {
      if (out_error) {
        *out_error =
            "requested contract_dsl_path is invalid: " + validate_error;
      }
      return false;
    }
    if (!ensure_iitepi_config_loaded(app, out_error))
      return false;

    const std::string registered_hash = lowercase_copy(
        cuwacunu::iitepi::contract_space_t::register_contract_file(
            contract_path));
    if (registered_hash.empty()) {
      if (out_error) {
        *out_error =
            "requested contract_dsl_path produced an empty contract hash";
      }
      return false;
    }
    if (!contract_hash.empty() && contract_hash != registered_hash) {
      if (out_error) {
        *out_error = "arguments.contract_hash does not match the hash of "
                     "arguments.contract_dsl_path";
      }
      return false;
    }
    contract_hash = registered_hash;
    out->contract_source = "contract_dsl_path";
  } else {
    if (!cuwacunu::iitepi::contract_space_t::has_contract(contract_hash)) {
      if (out_error) {
        *out_error =
            "requested contract_hash is not registered in this process; "
            "provide arguments.contract_dsl_path to evaluate an arbitrary "
            "contract";
      }
      return false;
    }
    out->contract_source = "registered_contract_hash";
  }

  const auto snapshot =
      cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash);
  if (!snapshot) {
    if (out_error) {
      *out_error = "requested contract snapshot could not be resolved";
    }
    return false;
  }

  const std::string docking_signature =
      lowercase_copy(snapshot->signature.docking_signature_sha256_hex);
  if (docking_signature.empty()) {
    if (out_error) {
      *out_error =
          "requested contract snapshot is missing docking_signature_sha256_hex";
    }
    return false;
  }

  out->contract_hash = contract_hash;
  out->contract_dsl_path = !contract_path.empty()
                               ? contract_path
                               : (!snapshot->config_file_path_canonical.empty()
                                      ? snapshot->config_file_path_canonical
                                      : snapshot->config_file_path);
  out->docking_signature_sha256_hex = docking_signature;
  out->snapshot = snapshot;
  return true;
}

[[nodiscard]] bool validate_component_manifest_public_docking(
    const cuwacunu::hero::hashimyei::component_manifest_t &manifest,
    std::string_view requested_contract_hash,
    std::string_view expected_docking_signature, std::string *out_error) {
  if (out_error)
    out_error->clear();

  const std::string expected_signature =
      lowercase_copy(std::string(expected_docking_signature));
  if (expected_signature.empty()) {
    if (out_error) {
      *out_error = "expected docking_signature_sha256_hex is empty";
    }
    return false;
  }

  const std::string actual_signature =
      lowercase_copy(manifest.docking_signature_sha256_hex);
  if (actual_signature.empty()) {
    if (out_error) {
      *out_error = "component manifest is missing "
                   "docking_signature_sha256_hex";
    }
    return false;
  }

  if (actual_signature != expected_signature) {
    if (out_error) {
      *out_error =
          "component manifest docking signature mismatch: canonical_path=" +
          manifest.canonical_path + " component_docking=" + actual_signature +
          " requested_docking=" + expected_signature + " component_contract=" +
          cuwacunu::hero::hashimyei::contract_hash_from_identity(
              manifest.contract_identity) +
          " requested_contract=" + std::string(requested_contract_hash);
    }
    return false;
  }

  return true;
}

[[nodiscard]] std::string
contract_compatibility_summary(bool compatible, bool founding_contract_match) {
  if (compatible) {
    if (founding_contract_match) {
      return "Component is dock-compatible with the requested contract";
    }
    return "Component is dock-compatible with the requested contract even "
           "though the founding contract differs";
  }
  return "Component is not dock-compatible with the requested contract";
}

[[nodiscard]] std::string contract_compatibility_details(
    const cuwacunu::hero::hashimyei::component_state_t &component,
    const resolved_contract_snapshot_t &requested_contract, bool compatible,
    bool founding_contract_match, std::string_view compatibility_reason) {
  const std::string component_contract_hash =
      cuwacunu::hero::hashimyei::contract_hash_from_identity(
          component.manifest.contract_identity);
  std::ostringstream out;
  if (compatible) {
    out << "requested docking_signature_sha256_hex matches the component "
           "manifest. Compatibility keys on DOCK/docking_signature, so "
           "ASSEMBLY and founding contract provenance may differ.";
    if (founding_contract_match) {
      out << " founding_contract_match=true";
    } else {
      out << " component_contract=" << component_contract_hash
          << " requested_contract=" << requested_contract.contract_hash;
    }
    return out.str();
  }

  out << compatibility_reason;
  out << " Compatibility is evaluated by DOCK/docking_signature only; the "
         "founding contract remains provenance and does not override a dock "
         "mismatch.";
  return out.str();
}

[[nodiscard]] bool handle_tool_list(app_context_t *app,
                                    const std::string &arguments_json,
                                    std::string *out_structured,
                                    std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  bool include_non_hashimyei = false;
  std::string canonical_path_prefix;
  std::size_t limit = 0;
  std::size_t offset = 0;
  (void)extract_json_bool_field(arguments_json, "include_non_hashimyei",
                                &include_non_hashimyei);
  (void)extract_json_string_field(arguments_json, "canonical_path_prefix",
                                  &canonical_path_prefix);
  (void)extract_json_size_field(arguments_json, "limit", &limit);
  (void)extract_json_size_field(arguments_json, "offset", &offset);

  struct ref_rollup_t {
    std::string canonical_path{};
    std::string hashimyei{};
    std::size_t report_fragment_count{0};
    std::size_t component_count{0};
  };
  std::unordered_map<std::string, ref_rollup_t> by_ref{};

  const auto add_entry = [&](std::string hash,
                             const std::string &canonical_path,
                             const bool from_component) {
    hash = trim_ascii(hash);
    std::string canonical_ref = trim_ascii(canonical_path);
    if (hash.empty())
      hash = maybe_hashimyei_from_canonical(canonical_ref);
    if (!canonical_path_prefix.empty() &&
        canonical_ref.rfind(canonical_path_prefix, 0) != 0) {
      return;
    }
    if (hash.empty() && !include_non_hashimyei)
      return;
    if (!hash.empty() && !canonical_ref.empty()) {
      const std::string suffix = "." + hash;
      if (canonical_ref.size() <= suffix.size() ||
          canonical_ref.compare(canonical_ref.size() - suffix.size(),
                                suffix.size(), suffix) != 0) {
        canonical_ref += suffix;
      }
    }
    if (canonical_ref.empty()) {
      canonical_ref = hash;
    }
    if (canonical_ref.empty())
      return;

    auto it = by_ref.find(canonical_ref);
    if (it == by_ref.end()) {
      it = by_ref
               .emplace(canonical_ref,
                        ref_rollup_t{
                            .canonical_path = canonical_ref,
                            .hashimyei = hash.empty()
                                             ? maybe_hashimyei_from_canonical(
                                                   canonical_ref)
                                             : hash,
                            .report_fragment_count = 0,
                            .component_count = 0,
                        })
               .first;
    }
    if (from_component) {
      ++it->second.component_count;
    } else {
      ++it->second.report_fragment_count;
    }
  };

  store_discovery_snapshot_t snapshot{};
  if (!scan_store_discovery_snapshot(app, true, &snapshot, out_error))
    return false;

  const auto &components = snapshot.components;
  for (const auto &c : components) {
    add_entry(c.manifest.hashimyei_identity.name, c.manifest.canonical_path,
              true);
  }

  const auto &report_fragments = snapshot.report_fragments;
  for (const auto &fragment : report_fragments) {
    add_entry(fragment.hashimyei, fragment.canonical_path, false);
  }

  std::vector<ref_rollup_t> rows{};
  rows.reserve(by_ref.size());
  for (const auto &[_, value] : by_ref) {
    rows.push_back(value);
  }

  std::sort(rows.begin(), rows.end(),
            [](const ref_rollup_t &a, const ref_rollup_t &b) {
              return a.canonical_path < b.canonical_path;
            });

  const std::size_t total = rows.size();
  const std::size_t off = std::min(offset, rows.size());
  std::size_t count = limit;
  if (count == 0)
    count = rows.size() - off;
  count = std::min(count, rows.size() - off);
  const auto begin = rows.begin() + static_cast<std::ptrdiff_t>(off);
  const auto end = begin + static_cast<std::ptrdiff_t>(count);
  rows.assign(begin, end);

  std::ostringstream rows_json;
  rows_json << "[";
  for (std::size_t i = 0; i < rows.size(); ++i) {
    if (i != 0)
      rows_json << ",";
    rows_json << "{\"canonical_path\":" << json_quote(rows[i].canonical_path)
              << ",\"hashimyei\":" << json_quote(rows[i].hashimyei)
              << ",\"report_fragment_count\":" << rows[i].report_fragment_count
              << ",\"component_count\":" << rows[i].component_count << "}";
  }
  rows_json << "]";

  std::ostringstream out;
  out << "{\"canonical_path\":\"\""
      << ",\"count\":" << rows.size() << ",\"total\":" << total
      << ",\"include_non_hashimyei\":"
      << (include_non_hashimyei ? "true" : "false")
      << ",\"hashimyeis\":" << rows_json.str() << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_get_founding_dsl_bundle(
    app_context_t *app, const std::string &arguments_json,
    std::string *out_structured, std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string canonical_path;
  std::string hashimyei;
  (void)extract_json_string_field(arguments_json, "canonical_path",
                                  &canonical_path);
  (void)extract_json_string_field(arguments_json, "hashimyei", &hashimyei);
  if (canonical_path.empty() && hashimyei.empty()) {
    *out_error = "get_founding_dsl_bundle requires arguments.canonical_path or "
                 "arguments.hashimyei";
    return false;
  }

  bool include_content = false;
  (void)extract_json_bool_field(arguments_json, "include_content",
                                &include_content);

  store_discovery_snapshot_t snapshot{};
  if (!scan_store_discovery_snapshot(app, false, &snapshot, out_error))
    return false;

  cuwacunu::hero::hashimyei::component_state_t component{};
  if (!resolve_component_from_store_snapshot(
          snapshot, canonical_path, hashimyei, &component, out_error)) {
    return false;
  }

  const std::string component_id =
      component.component_id.empty()
          ? cuwacunu::hero::hashimyei::compute_component_manifest_id(
                component.manifest)
          : component.component_id;
  const auto bundle_dir =
      cuwacunu::hero::hashimyei::founding_dsl_bundle_directory(
          app->store_root, component.manifest.canonical_path, component_id);
  const auto bundle_manifest_path =
      cuwacunu::hero::hashimyei::founding_dsl_bundle_manifest_path(
          app->store_root, component.manifest.canonical_path, component_id);

  cuwacunu::hero::hashimyei::founding_dsl_bundle_manifest_t bundle_manifest{};
  if (!cuwacunu::hero::hashimyei::read_founding_dsl_bundle_manifest(
          app->store_root, component.manifest.canonical_path, component_id,
          &bundle_manifest, out_error)) {
    return false;
  }
  if (!bundle_manifest.component_id.empty() &&
      bundle_manifest.component_id != component_id) {
    *out_error = "founding DSL bundle manifest component_id mismatch: " +
                 bundle_manifest.component_id + " != " + component_id;
    return false;
  }
  if (!bundle_manifest.canonical_path.empty() &&
      bundle_manifest.canonical_path != component.manifest.canonical_path) {
    *out_error = "founding DSL bundle manifest canonical_path mismatch: " +
                 bundle_manifest.canonical_path +
                 " != " + component.manifest.canonical_path;
    return false;
  }
  if (!bundle_manifest.hashimyei_name.empty() &&
      bundle_manifest.hashimyei_name !=
          component.manifest.hashimyei_identity.name) {
    *out_error = "founding DSL bundle manifest hashimyei_name mismatch: " +
                 bundle_manifest.hashimyei_name +
                 " != " + component.manifest.hashimyei_identity.name;
    return false;
  }
  if (bundle_manifest.founding_dsl_bundle_aggregate_sha256_hex.empty()) {
    *out_error = "founding DSL bundle manifest missing "
                 "founding_dsl_bundle_aggregate_sha256_hex";
    return false;
  }

  std::ostringstream files_json;
  files_json << "[";
  bool all_files_match = true;
  std::string bundle_sha_actual{};

  auto verified_manifest = bundle_manifest;
  for (std::size_t i = 0; i < bundle_manifest.files.size(); ++i) {
    if (i != 0)
      files_json << ",";
    const auto &file = bundle_manifest.files[i];
    const std::filesystem::path stored_path =
        bundle_dir / file.snapshot_relpath;
    std::string dsl_text{};
    if (!read_text_file(stored_path, &dsl_text, out_error)) {
      *out_error = "cannot read founding DSL bundle snapshot '" +
                   stored_path.string() + "': " + *out_error;
      return false;
    }

    std::string dsl_sha_actual{};
    if (!sha256_hex_bytes(dsl_text, &dsl_sha_actual)) {
      *out_error = "cannot compute sha256 for founding DSL bundle snapshot: " +
                   stored_path.string();
      return false;
    }

    const bool dsl_sha_match =
        lowercase_copy(dsl_sha_actual) == lowercase_copy(file.sha256_hex);
    all_files_match = all_files_match && dsl_sha_match;
    verified_manifest.files[i].sha256_hex = dsl_sha_actual;

    files_json << "{"
               << "\"source_path\":" << json_quote(file.source_path) << ","
               << "\"stored_path\":" << json_quote(stored_path.string()) << ","
               << "\"snapshot_relpath\":" << json_quote(file.snapshot_relpath)
               << ",\"sha256_expected\":" << json_quote(file.sha256_hex)
               << ",\"sha256_actual\":" << json_quote(dsl_sha_actual) << ","
               << "\"sha256_match\":" << (dsl_sha_match ? "true" : "false");
    if (include_content) {
      files_json << ",\"text\":" << json_quote(dsl_text);
    }
    files_json << "}";
  }
  bundle_sha_actual =
      cuwacunu::hero::hashimyei::compute_founding_dsl_bundle_aggregate_sha256(
          verified_manifest);
  if (bundle_sha_actual.empty()) {
    *out_error = "cannot compute founding DSL bundle aggregate sha256";
    return false;
  }
  const bool bundle_sha_match =
      lowercase_copy(bundle_sha_actual) ==
      lowercase_copy(bundle_manifest.founding_dsl_bundle_aggregate_sha256_hex);
  all_files_match = all_files_match && bundle_sha_match;
  files_json << "]";

  std::ostringstream out;
  out << "{\"canonical_path\":" << json_quote(component.manifest.canonical_path)
      << ",\"hashimyei\":"
      << json_quote(component.manifest.hashimyei_identity.name)
      << ",\"component_id\":" << json_quote(component_id)
      << ",\"founding_dsl_source_path\":"
      << json_quote(component.manifest.founding_dsl_source_path)
      << ",\"founding_dsl_source_sha256_expected\":"
      << json_quote(component.manifest.founding_dsl_source_sha256_hex)
      << ",\"founding_dsl_bundle_manifest_path\":"
      << json_quote(bundle_manifest_path.string())
      << ",\"founding_dsl_bundle_directory\":"
      << json_quote(bundle_dir.string())
      << ",\"founding_dsl_bundle_aggregate_sha256_expected\":"
      << json_quote(bundle_manifest.founding_dsl_bundle_aggregate_sha256_hex)
      << ",\"founding_dsl_bundle_aggregate_sha256_actual\":"
      << json_quote(bundle_sha_actual) << ",\"founding_dsl_bundle_consistent\":"
      << (all_files_match ? "true" : "false")
      << ",\"founding_dsl_bundle_files\":" << files_json.str()
      << ",\"component\":" << component_state_to_json(component);
  out << "}";

  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_get_component_manifest(
    app_context_t *app, const std::string &arguments_json,
    std::string *out_structured, std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string canonical_path{};
  std::string hashimyei{};
  (void)extract_json_string_field(arguments_json, "canonical_path",
                                  &canonical_path);
  (void)extract_json_string_field(arguments_json, "hashimyei", &hashimyei);
  if (canonical_path.empty() && hashimyei.empty()) {
    *out_error = "get_component_manifest requires arguments.canonical_path or "
                 "arguments.hashimyei";
    return false;
  }

  store_discovery_snapshot_t snapshot{};
  if (!scan_store_discovery_snapshot(app, false, &snapshot, out_error))
    return false;

  cuwacunu::hero::hashimyei::component_state_t component{};
  if (!resolve_component_from_store_snapshot(
          snapshot, canonical_path, hashimyei, &component, out_error)) {
    return false;
  }

  std::ostringstream out;
  out << "{\"canonical_path\":" << json_quote(component.manifest.canonical_path)
      << ",\"hashimyei\":"
      << json_quote(component.manifest.hashimyei_identity.name)
      << ",\"component\":" << component_state_to_json(component) << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_evaluate_contract_compatibility(
    app_context_t *app, const std::string &arguments_json,
    std::string *out_structured, std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string canonical_path{};
  std::string hashimyei{};
  std::string contract_hash{};
  std::string contract_dsl_path{};
  (void)extract_json_string_field(arguments_json, "canonical_path",
                                  &canonical_path);
  (void)extract_json_string_field(arguments_json, "hashimyei", &hashimyei);
  (void)extract_json_string_field(arguments_json, "contract_hash",
                                  &contract_hash);
  (void)extract_json_string_field(arguments_json, "contract_dsl_path",
                                  &contract_dsl_path);
  if (canonical_path.empty() && hashimyei.empty()) {
    *out_error = "evaluate_contract_compatibility requires "
                 "arguments.canonical_path or arguments.hashimyei";
    return false;
  }

  store_discovery_snapshot_t snapshot{};
  if (!scan_store_discovery_snapshot(app, false, &snapshot, out_error)) {
    return false;
  }

  cuwacunu::hero::hashimyei::component_state_t component{};
  if (!resolve_component_from_store_snapshot(
          snapshot, canonical_path, hashimyei, &component, out_error)) {
    return false;
  }

  resolved_contract_snapshot_t requested_contract{};
  if (!resolve_requested_contract_snapshot(app, contract_hash,
                                           contract_dsl_path,
                                           &requested_contract, out_error)) {
    return false;
  }

  std::string compatibility_reason{};
  const bool compatible = validate_component_manifest_public_docking(
      component.manifest, requested_contract.contract_hash,
      requested_contract.docking_signature_sha256_hex, &compatibility_reason);
  const std::string component_contract_hash =
      cuwacunu::hero::hashimyei::contract_hash_from_identity(
          component.manifest.contract_identity);
  const std::string component_docking_signature =
      lowercase_copy(component.manifest.docking_signature_sha256_hex);
  const bool founding_contract_match =
      lowercase_copy(component_contract_hash) ==
      requested_contract.contract_hash;
  const std::string summary =
      contract_compatibility_summary(compatible, founding_contract_match);
  const std::string details = contract_compatibility_details(
      component, requested_contract, compatible, founding_contract_match,
      compatibility_reason);

  std::ostringstream out;
  out << "{"
      << "\"summary\":" << json_quote(summary) << ","
      << "\"details\":" << json_quote(details) << ","
      << "\"compatible\":" << (compatible ? "true" : "false") << ","
      << "\"compatibility_basis\":\"dock_only\","
      << "\"canonical_path\":" << json_quote(component.manifest.canonical_path)
      << ","
      << "\"family\":" << json_quote(component.manifest.family) << ","
      << "\"hashimyei\":"
      << json_quote(component.manifest.hashimyei_identity.name) << ","
      << "\"lineage_state\":" << json_quote(component.manifest.lineage_state)
      << ","
      << "\"requested_contract_hash\":"
      << json_quote(requested_contract.contract_hash) << ","
      << "\"requested_contract_dsl_path\":"
      << (requested_contract.contract_dsl_path.empty()
              ? "null"
              : json_quote(requested_contract.contract_dsl_path))
      << ",\"requested_contract_source\":"
      << json_quote(requested_contract.contract_source) << ","
      << "\"requested_docking_signature_sha256_hex\":"
      << json_quote(requested_contract.docking_signature_sha256_hex) << ","
      << "\"component_contract_hash\":" << json_quote(component_contract_hash)
      << ","
      << "\"component_docking_signature_sha256_hex\":"
      << json_quote(component_docking_signature) << ","
      << "\"founding_contract_match\":"
      << (founding_contract_match ? "true" : "false") << ","
      << "\"docking_signature_match\":" << (compatible ? "true" : "false")
      << ","
      << "\"comparison_reason\":" << json_quote(compatibility_reason) << ","
      << "\"component\":" << component_state_to_json(component) << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_update_rank(app_context_t *app,
                                           const std::string &arguments_json,
                                           std::string *out_structured,
                                           std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  std::string family{};
  std::string dock_hash{};
  std::string source_view_kind{};
  std::string source_view_transport_sha256{};
  std::vector<std::string> ordered_hashimyeis{};
  (void)extract_json_string_field(arguments_json, "family", &family);
  (void)extract_json_string_field(arguments_json, "dock_hash", &dock_hash);
  (void)extract_json_string_field(arguments_json, "source_view_kind",
                                  &source_view_kind);
  (void)extract_json_string_field(arguments_json,
                                  "source_view_transport_sha256",
                                  &source_view_transport_sha256);
  (void)extract_json_string_array_field(arguments_json, "ordered_hashimyeis",
                                        &ordered_hashimyeis);
  family = trim_ascii(family);
  dock_hash = trim_ascii(dock_hash);
  source_view_kind = trim_ascii(source_view_kind);
  source_view_transport_sha256 = trim_ascii(source_view_transport_sha256);
  if (family.empty()) {
    *out_error = "update_rank requires arguments.family";
    return false;
  }
  if (dock_hash.empty()) {
    *out_error = "update_rank requires arguments.dock_hash";
    return false;
  }
  if (ordered_hashimyeis.empty()) {
    *out_error = "update_rank requires arguments.ordered_hashimyeis";
    return false;
  }
  if (source_view_kind.empty())
    source_view_kind = "family_evaluation_report";

  std::unordered_set<std::string> requested_hashimyeis{};
  for (auto &hashimyei : ordered_hashimyeis) {
    hashimyei = trim_ascii(hashimyei);
    if (hashimyei.empty()) {
      *out_error = "ordered_hashimyeis cannot contain empty values";
      return false;
    }
    if (!requested_hashimyeis.insert(hashimyei).second) {
      *out_error = "ordered_hashimyeis contains duplicates";
      return false;
    }
  }

  std::vector<cuwacunu::hero::hashimyei::component_state_t> components{};
  if (!app->catalog.list_components("", "", 0, 0, true, &components,
                                    out_error)) {
    return false;
  }

  std::unordered_map<std::string, cuwacunu::hero::hashimyei::component_state_t>
      latest_by_hashimyei{};
  for (const auto &component : components) {
    if (component.manifest.family != family)
      continue;
    if (trim_ascii(component.manifest.docking_signature_sha256_hex) !=
        dock_hash) {
      continue;
    }
    const std::string hashimyei = component.manifest.hashimyei_identity.name;
    if (hashimyei.empty())
      continue;
    const auto it = latest_by_hashimyei.find(hashimyei);
    if (it == latest_by_hashimyei.end() || component.ts_ms > it->second.ts_ms ||
        (component.ts_ms == it->second.ts_ms &&
         component.component_id > it->second.component_id)) {
      latest_by_hashimyei[hashimyei] = component;
    }
  }
  if (latest_by_hashimyei.empty()) {
    *out_error = "family has no components in the catalog for dock_hash=" +
                 dock_hash + ": " + family;
    return false;
  }
  if (ordered_hashimyeis.size() != latest_by_hashimyei.size()) {
    *out_error = "ordered_hashimyeis must enumerate every current family "
                 "member exactly once";
    return false;
  }
  for (const auto &[hashimyei, _] : latest_by_hashimyei) {
    if (requested_hashimyeis.find(hashimyei) == requested_hashimyeis.end()) {
      *out_error = "ordered_hashimyeis is missing family member: " + hashimyei;
      return false;
    }
  }

  cuwacunu::hero::family_rank::state_t before_state{};
  const bool has_before_rank = app->catalog.get_explicit_family_rank(
      family, dock_hash, &before_state, nullptr);

  cuwacunu::hero::family_rank::state_t desired_state{};
  desired_state.family = family;
  desired_state.dock_hash = dock_hash;
  desired_state.source_view_kind = source_view_kind;
  desired_state.source_view_transport_sha256 = source_view_transport_sha256;
  desired_state.updated_at_ms = now_ms_utc();
  desired_state.assignments.reserve(ordered_hashimyeis.size());
  for (std::size_t i = 0; i < ordered_hashimyeis.size(); ++i) {
    const auto it = latest_by_hashimyei.find(ordered_hashimyeis[i]);
    if (it == latest_by_hashimyei.end()) {
      *out_error = "ordered_hashimyeis contains unknown family member";
      return false;
    }
    desired_state.assignments.push_back(
        cuwacunu::hero::family_rank::assignment_t{
            .rank = static_cast<std::uint64_t>(i),
            .hashimyei = ordered_hashimyeis[i],
            .canonical_path = it->second.manifest.canonical_path,
            .component_id = it->second.component_id,
        });
  }

  const auto state_matches =
      [&](const cuwacunu::hero::family_rank::state_t &lhs,
          const cuwacunu::hero::family_rank::state_t &rhs) {
        return cuwacunu::hero::family_rank::ordering_matches(lhs, rhs);
      };

  const std::filesystem::path rank_file =
      family_rank_file_path(app->store_root, family, dock_hash);
  const bool changed =
      !has_before_rank || !state_matches(before_state, desired_state);
  if (changed) {
    const std::string payload =
        cuwacunu::hero::family_rank::emit_state_payload(desired_state);
    if (!write_text_file_atomic(rank_file, payload, out_error))
      return false;
    if (!app->catalog.ingest_filesystem(app->store_root, out_error))
      return false;
  }

  bool synchronized_lattice = false;
  std::string lattice_catalog_error{};
  if (app->lattice_catalog_path.empty()) {
    *out_error = "lattice_catalog_path is empty";
    return false;
  }
  const std::filesystem::path lattice_catalog_path = app->lattice_catalog_path;
  cuwacunu::hero::wave::lattice_catalog_store_t lattice_catalog{};
  cuwacunu::hero::wave::lattice_catalog_store_t::options_t lattice_options{};
  lattice_options.catalog_path = lattice_catalog_path;
  lattice_options.encrypted = false;
  lattice_options.projection_version = 2;
  if (!lattice_catalog.open(lattice_options, &lattice_catalog_error)) {
    *out_error = "lattice sync open failed: " + lattice_catalog_error;
    return false;
  }
  if (!lattice_catalog.ingest_runtime_artifact(rank_file,
                                               &lattice_catalog_error)) {
    *out_error = "lattice sync ingest failed: " + lattice_catalog_error;
    return false;
  }
  synchronized_lattice = true;
  (void)lattice_catalog.close(nullptr);

  cuwacunu::hero::family_rank::state_t after_state{};
  if (!app->catalog.get_family_rank(family, dock_hash, &after_state,
                                    out_error)) {
    return false;
  }

  std::ostringstream out;
  out << "{\"family\":" << json_quote(family)
      << ",\"dock_hash\":" << json_quote(dock_hash)
      << ",\"changed\":" << (changed ? "true" : "false")
      << ",\"rank_file\":" << json_quote(rank_file.string())
      << ",\"lattice_catalog_path\":"
      << json_quote(lattice_catalog_path.string())
      << ",\"synchronized_lattice\":"
      << (synchronized_lattice ? "true" : "false")
      << ",\"component_count\":" << latest_by_hashimyei.size() << ",\"before\":"
      << (has_before_rank ? family_rank_state_to_json(before_state) : "null")
      << ",\"after\":" << family_rank_state_to_json(after_state) << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_reset_catalog(app_context_t *app,
                                             const std::string &arguments_json,
                                             std::string *out_structured,
                                             std::string *out_error) {
  if (!app || !out_structured || !out_error)
    return false;
  out_error->clear();

  bool reingest = true;
  (void)extract_json_bool_field(arguments_json, "reingest", &reingest);
  if (!reset_catalog(app, reingest, out_error))
    return false;

  std::vector<cuwacunu::hero::hashimyei::component_state_t> components{};
  if (!app->catalog.list_components("", "", 0, 0, true, &components,
                                    out_error)) {
    return false;
  }

  std::ostringstream out;
  out << "{\"canonical_path\":\"\""
      << ",\"catalog_path\":"
      << json_quote(app->catalog_options.catalog_path.string())
      << ",\"reingest\":" << (reingest ? "true" : "false")
      << ",\"component_count\":" << components.size() << "}";
  *out_structured = out.str();
  return true;
}

[[nodiscard]] bool handle_tool_call(app_context_t *app,
                                    const std::string &tool_name,
                                    const std::string &arguments_json,
                                    std::string *out_result_json,
                                    std::string *out_error_json) {
  if (!app || !out_result_json || !out_error_json)
    return false;
  out_result_json->clear();
  out_error_json->clear();

  const auto *descriptor = find_hashimyei_tool_descriptor(tool_name);
  if (!descriptor) {
    *out_error_json = "unknown tool: " + tool_name;
    return false;
  }

  std::string structured;
  std::string err;
  if (tool_requires_catalog_sync(tool_name)) {
    std::string sync_error;
    if (!ensure_hashimyei_catalog_synced_to_store(app, false, &sync_error)) {
      const std::string err_ingest = "catalog sync failed: " + sync_error;
      const std::string fallback =
          "{\"canonical_path\":\"\",\"error\":" + json_quote(err_ingest) + "}";
      *out_result_json = make_tool_result_json(err_ingest, fallback, true);
      return true;
    }
  }

  const bool ok = descriptor->handler(app, arguments_json, &structured, &err);

  if (!ok) {
    const std::string fallback =
        "{\"canonical_path\":\"\",\"error\":" + json_quote(err) + "}";
    *out_result_json = make_tool_result_json(err, fallback, true);
    return true;
  }

  *out_result_json = make_tool_result_json(
      tool_result_text_for_structured_json(structured), structured, false);
  return true;
}

[[nodiscard]] std::string initialize_result_json() {
  std::ostringstream out;
  out << "{"
      << "\"protocolVersion\":" << json_quote(kProtocolVersion) << ","
      << "\"serverInfo\":{\"name\":" << json_quote(kServerName)
      << ",\"version\":" << json_quote(kServerVersion) << "},"
      << "\"capabilities\":{\"tools\":{\"listChanged\":false}},"
      << "\"instructions\":" << json_quote(kInitializeInstructions) << "}";
  return out.str();
}

void run_jsonrpc_stdio_loop(app_context_t *app) {
  std::string request;
  bool shutdown_requested = false;
  while (true) {
    if (!wait_for_stdio_activity())
      return;

    bool used_content_length = false;
    if (!read_next_jsonrpc_message(&std::cin, &request, &used_content_length)) {
      return;
    }
    if (used_content_length)
      g_jsonrpc_use_content_length_framing = true;

    std::string method;
    if (!extract_json_string_field(request, "method", &method)) {
      write_jsonrpc_error("null", -32600, "missing method");
      continue;
    }

    if (method == "exit")
      return;

    std::string id_json = "null";
    const bool has_id = extract_json_id_field(request, &id_json);
    if (method.rfind("notifications/", 0) == 0) {
      continue;
    }
    if (!has_id) {
      write_jsonrpc_error("null", -32700, "invalid request id");
      continue;
    }
    if (method == "shutdown") {
      write_jsonrpc_result(id_json, "{}");
      shutdown_requested = true;
      continue;
    }
    if (shutdown_requested) {
      write_jsonrpc_error(id_json, -32000, "server is shutting down");
      continue;
    }
    if (method == "initialize") {
      write_jsonrpc_result(id_json, initialize_result_json());
      continue;
    }
    if (method == "ping") {
      write_jsonrpc_result(id_json, "{\"ok\":true}");
      continue;
    }
    if (method == "tools/list") {
      write_jsonrpc_result(id_json, tools_list_result_json());
      continue;
    }
    if (method == "resources/list") {
      write_jsonrpc_result(id_json, "{\"resources\":[]}");
      continue;
    }
    if (method == "resources/templates/list") {
      write_jsonrpc_result(id_json, "{\"resourceTemplates\":[]}");
      continue;
    }
    if (method != "tools/call") {
      write_jsonrpc_error(id_json, -32601, "method not found");
      continue;
    }

    std::string params;
    if (!extract_json_object_field(request, "params", &params)) {
      write_jsonrpc_error(id_json, -32602, "tools/call requires params object");
      continue;
    }

    std::string name;
    if (!extract_json_string_field(params, "name", &name) || name.empty()) {
      write_jsonrpc_error(id_json, -32602, "tools/call requires params.name");
      continue;
    }

    std::string arguments = "{}";
    std::string extracted_args;
    if (extract_json_object_field(params, "arguments", &extracted_args)) {
      arguments = extracted_args;
    }

    const bool catalog_already_open = app->catalog.opened();
    const std::uint64_t tool_started_at_ms = now_ms_utc();
    std::uint64_t catalog_hold_started_at_ms = 0;
    log_hashimyei_tool_begin(app, name, catalog_already_open);

    const bool needs_preopen = tool_requires_catalog_preopen(name);
    if (!catalog_already_open && needs_preopen) {
      std::string open_error;
      if (!app->catalog.open(app->catalog_options, &open_error)) {
        const std::string err = "catalog open failed: " + open_error;
        log_hashimyei_tool_end(app, name, catalog_already_open, false, false,
                               now_ms_utc() - tool_started_at_ms, 0, err);
        const std::string fallback =
            "{\"canonical_path\":\"\",\"error\":" + json_quote(err) + "}";
        write_jsonrpc_result(id_json,
                             make_tool_result_json(err, fallback, true));
        continue;
      }
      catalog_hold_started_at_ms = now_ms_utc();
    }

    std::string tool_result;
    std::string tool_error;
    const bool ok =
        handle_tool_call(app, name, arguments, &tool_result, &tool_error);

    // Release the catalog lock after every request so idle stdio servers do
    // not block other clients from opening the catalog.
    std::string close_error;
    if (!close_catalog_if_open(app, &close_error)) {
      const std::string err = "catalog close failed: " + close_error;
      log_hashimyei_tool_end(app, name, catalog_already_open, false, false,
                             now_ms_utc() - tool_started_at_ms,
                             catalog_hold_started_at_ms == 0
                                 ? 0
                                 : now_ms_utc() - catalog_hold_started_at_ms,
                             err);
      write_jsonrpc_error(id_json, -32603,
                          "catalog close failed: " + close_error);
      continue;
    }

    const std::uint64_t finished_at_ms = now_ms_utc();
    log_hashimyei_tool_end(app, name, catalog_already_open, ok, true,
                           finished_at_ms - tool_started_at_ms,
                           catalog_hold_started_at_ms == 0
                               ? 0
                               : finished_at_ms - catalog_hold_started_at_ms,
                           ok ? std::string_view{}
                              : std::string_view(tool_error));
    if (!ok) {
      write_jsonrpc_error(id_json, -32601, tool_error);
      continue;
    }

    write_jsonrpc_result(id_json, tool_result);
  }
}

} // namespace
