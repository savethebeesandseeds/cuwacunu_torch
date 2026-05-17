[[nodiscard]] inline bool
write_runtime_text_file_atomic(const std::filesystem::path &target,
                               std::string_view content,
                               std::string *error = nullptr) {
  if (error)
    error->clear();
  if (target.empty()) {
    if (error)
      *error = "target path is empty";
    return false;
  }
  std::error_code ec{};
  if (target.has_parent_path()) {
    std::filesystem::create_directories(target.parent_path(), ec);
    if (ec) {
      if (error) {
        *error =
            "cannot create parent directory: " + target.parent_path().string();
      }
      return false;
    }
  }
  const auto tmp = target.string() + ".tmp";
  std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
  if (!out) {
    if (error)
      *error = "cannot open temporary file: " + tmp;
    return false;
  }
  out << content;
  out.flush();
  if (!out.good()) {
    if (error)
      *error = "cannot write temporary file: " + tmp;
    return false;
  }
  out.close();
  std::filesystem::rename(tmp, target, ec);
  if (ec) {
    std::filesystem::remove(tmp, ec);
    if (error)
      *error = "cannot replace target file: " + target.string();
    return false;
  }
  return true;
}

[[nodiscard]] inline std::string
sanitize_bundle_snapshot_filename(std::string_view filename) {
  std::string out{};
  out.reserve(filename.size());
  for (const char ch : filename) {
    const bool ok = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                    (ch >= '0' && ch <= '9') || ch == '.' || ch == '_' ||
                    ch == '-';
    out.push_back(ok ? ch : '_');
  }
  if (out.empty())
    out = "dsl";
  return out;
}

[[nodiscard]] inline bool
runtime_node_canonical_type(const Tsi &node, std::string *out) noexcept;

[[nodiscard]] inline bool resolve_runtime_wikimyei_binding(
    const RuntimeBindingContract &c, const TsiWikimyei &wik,
    RuntimeBindingContract::WikimyeiBindingIdentity *out,
    std::string *error = nullptr) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "missing wikimyei binding output";
    return false;
  }

  const std::string binding_id = std::string(wik.instance_name());
  if (binding_id.empty()) {
    if (error)
      *error = "wikimyei node has empty binding id";
    return false;
  }
  std::string canonical_type{};
  if (!runtime_node_canonical_type(wik, &canonical_type) ||
      canonical_type.empty()) {
    if (error) {
      *error = "cannot resolve canonical runtime type for wikimyei node '" +
               binding_id + "'";
    }
    return false;
  }

  const auto *binding = c.spec.find_wikimyei_binding(binding_id);
  if (!binding) {
    if (error) {
      *error = "missing explicit wikimyei binding entry for node '" +
               binding_id + "' type '" + canonical_type + "'";
    }
    return false;
  }
  *out = *binding;

  if (out->binding_id != binding_id) {
    if (error) {
      *error = "wikimyei binding id mismatch for node '" + binding_id +
               "': spec=" + out->binding_id;
    }
    return false;
  }
  if (out->canonical_type.empty()) {
    if (error) {
      *error = "wikimyei binding for node '" + binding_id +
               "' is missing canonical type";
    }
    return false;
  }
  if (out->canonical_type != canonical_type) {
    if (error) {
      *error = "wikimyei binding type mismatch for node '" + binding_id +
               "': spec=" + out->canonical_type + " runtime=" + canonical_type;
    }
    return false;
  }

  const auto type_id = parse_tsi_type_id(out->canonical_type);
  if (type_id.has_value() &&
      tsi_type_instance_policy(*type_id) ==
          TsiInstancePolicy::HashimyeiInstances &&
      out->hashimyei.empty()) {
    if (error) {
      *error = "wikimyei binding for node '" + binding_id +
               "' is missing required hashimyei";
    }
    return false;
  }
  return true;
}

[[nodiscard]] inline const RuntimeBindingContract::ComponentDslFingerprint *
find_runtime_component_dsl_fingerprint(
    const RuntimeBindingContract &c,
    const RuntimeBindingContract::WikimyeiBindingIdentity &binding) noexcept {
  for (const auto &fp : c.spec.component_dsl_fingerprints) {
    if (fp.binding_id == binding.binding_id) {
      return &fp;
    }
  }
  return nullptr;
}

[[nodiscard]] inline std::optional<
    cuwacunu::hero::hashimyei::component_manifest_t>
build_runtime_component_manifest(const RuntimeBindingContract &c,
                                 const TsiWikimyei &wik,
                                 std::string *error = nullptr) {
  if (error)
    error->clear();
  RuntimeBindingContract::WikimyeiBindingIdentity binding{};
  if (!resolve_runtime_wikimyei_binding(c, wik, &binding, error)) {
    return std::nullopt;
  }

  std::string normalized_hashimyei{};
  if (!cuwacunu::hashimyei::normalize_hex_hash_name(binding.hashimyei,
                                                    &normalized_hashimyei)) {
    if (error) {
      *error = "wikimyei hashimyei is not a valid 0x... ordinal for node '" +
               binding.binding_id + "'";
    }
    return std::nullopt;
  }

  const std::string family = binding.canonical_type;
  if (family.empty()) {
    if (error)
      *error = "wikimyei family is empty";
    return std::nullopt;
  }

  const RuntimeBindingContract::ComponentDslFingerprint *selected_fp =
      find_runtime_component_dsl_fingerprint(c, binding);
  if (!selected_fp) {
    if (error) {
      *error = "missing component DSL fingerprint for runtime bootstrap "
               "binding: " +
               binding.binding_id;
    }
    return std::nullopt;
  }
  if (selected_fp->tsi_type != family) {
    if (error) {
      *error = "component DSL fingerprint type mismatch for binding '" +
               binding.binding_id + "': fingerprint=" + selected_fp->tsi_type +
               " runtime=" + family;
    }
    return std::nullopt;
  }
  std::string normalized_fp_hashimyei{};
  if (!cuwacunu::hashimyei::normalize_hex_hash_name(selected_fp->hashimyei,
                                                    &normalized_fp_hashimyei) ||
      normalized_fp_hashimyei != normalized_hashimyei) {
    if (error) {
      *error = "component DSL fingerprint hashimyei mismatch for binding '" +
               binding.binding_id + "'";
    }
    return std::nullopt;
  }

  const auto contract_snapshot =
      cuwacunu::iitepi::contract_space_t::contract_itself(c.spec.contract_hash);
  if (!contract_snapshot) {
    if (error)
      *error = "missing contract snapshot for runtime component manifest";
    return std::nullopt;
  }
  if (selected_fp->component_compatibility_sha256_hex.empty()) {
    if (error) {
      *error = "missing realized component compatibility signature for "
               "wikimyei node '" +
               binding.binding_id + "'";
    }
    return std::nullopt;
  }

  std::uint64_t component_ordinal = 0;
  if (!cuwacunu::hashimyei::parse_hex_hash_name_ordinal(normalized_hashimyei,
                                                        &component_ordinal)) {
    if (error) {
      *error = "wikimyei hashimyei is not a valid 0x... ordinal";
    }
    return std::nullopt;
  }
  const std::string contract_hash = lowercase_copy(c.spec.contract_hash);
  const std::uint64_t contract_ordinal =
      cuwacunu::hashimyei::fnv1a64(contract_hash) & 0xffffull;

  cuwacunu::hero::hashimyei::component_manifest_t manifest{};
  manifest.schema = cuwacunu::hashimyei::kComponentManifestSchemaV2;
  manifest.canonical_path = family + "." + normalized_hashimyei;
  manifest.family = family;
  manifest.component_tag = selected_fp->component_tag;
  manifest.hashimyei_identity = cuwacunu::hashimyei::make_identity(
      cuwacunu::hashimyei::hashimyei_kind_e::TSIEMENE, component_ordinal);
  manifest.contract_identity = cuwacunu::hashimyei::make_identity(
      cuwacunu::hashimyei::hashimyei_kind_e::CONTRACT, contract_ordinal,
      contract_hash);
  manifest.parent_identity = std::nullopt;
  manifest.revision_reason = "runtime_bootstrap";
  manifest.founding_revision_id = "runtime_bootstrap:" + contract_hash;
  manifest.founding_dsl_source_path =
      compact_runtime_source_path_label(selected_fp->tsi_dsl_path);
  manifest.founding_dsl_source_sha256_hex = lowercase_copy(
      selected_fp->tsi_dsl_sha256_hex.empty()
          ? cuwacunu::iitepi::contract_space_t::sha256_hex_for_file(
                selected_fp->tsi_dsl_path)
          : selected_fp->tsi_dsl_sha256_hex);
  manifest.docking_signature_sha256_hex =
      lowercase_copy(contract_snapshot->signature.docking_signature_sha256_hex);
  manifest.component_compatibility_sha256_hex =
      lowercase_copy(selected_fp->component_compatibility_sha256_hex);
  manifest.instrument_signature = selected_fp->instrument_signature;
  manifest.lineage_state = "active";
  manifest.replaced_by.clear();
  manifest.created_at_ms = now_ms_utc_runtime_binding();
  manifest.updated_at_ms = manifest.created_at_ms;

  std::string validate_error{};
  if (!cuwacunu::hero::hashimyei::validate_component_manifest(
          manifest, &validate_error)) {
    if (error) {
      *error = "runtime component manifest invalid: " + validate_error;
    }
    return std::nullopt;
  }
  return manifest;
}

[[nodiscard]] inline bool
persist_runtime_component_lineage(const RuntimeBindingContract &c,
                                  const TsiWikimyei &wik,
                                  std::string *error = nullptr) {
  if (error)
    error->clear();
  const auto manifest_opt = build_runtime_component_manifest(c, wik, error);
  if (!manifest_opt.has_value())
    return false;
  const auto &manifest = *manifest_opt;

  const auto store_root = cuwacunu::hashimyei::store_root();
  const std::string component_id =
      cuwacunu::hero::hashimyei::compute_component_manifest_id(manifest);
  if (!cuwacunu::hero::hashimyei::save_component_manifest(store_root, manifest,
                                                          nullptr, error)) {
    return false;
  }

  const auto contract_snapshot =
      cuwacunu::iitepi::contract_space_t::contract_itself(c.spec.contract_hash);
  if (!contract_snapshot) {
    if (error)
      *error = "missing contract snapshot for founding DSL bundle";
    return false;
  }

  std::vector<std::filesystem::path> source_paths{};
  std::unordered_set<std::string> seen{};
  const auto add_source_path = [&](std::string_view raw_path) {
    const std::string trimmed = std::string(raw_path);
    if (trimmed.empty())
      return;
    const std::filesystem::path p(trimmed);
    const std::string key = p.lexically_normal().string();
    if (!seen.insert(key).second)
      return;
    source_paths.push_back(p);
  };

  add_source_path(contract_snapshot->config_file_path_canonical);
  for (const auto &fp : c.spec.component_dsl_fingerprints) {
    add_source_path(fp.tsi_dsl_path);
  }
  for (const auto &surface : contract_snapshot->docking_signature.surfaces) {
    add_source_path(surface.canonical_path);
  }
  for (const auto &entry : contract_snapshot->signature.module_dsl_entries) {
    add_source_path(entry.module_dsl_path);
  }

  const auto bundle_dir =
      cuwacunu::hero::hashimyei::founding_dsl_bundle_directory(
          store_root, manifest.canonical_path, component_id);
  std::error_code ec{};
  std::filesystem::create_directories(bundle_dir, ec);
  if (ec) {
    if (error) {
      *error =
          "cannot create founding DSL bundle directory: " + bundle_dir.string();
    }
    return false;
  }

  cuwacunu::hero::hashimyei::founding_dsl_bundle_manifest_t bundle_manifest{};
  bundle_manifest.component_id = component_id;
  bundle_manifest.canonical_path = manifest.canonical_path;
  bundle_manifest.hashimyei_name = manifest.hashimyei_identity.name;
  bundle_manifest.files.reserve(source_paths.size());

  for (std::size_t i = 0; i < source_paths.size(); ++i) {
    const auto &source_path = source_paths[i];
    std::ifstream in(source_path, std::ios::binary);
    if (!in) {
      if (error) {
        *error = "cannot read founding DSL source: " + source_path.string();
      }
      return false;
    }
    std::ostringstream payload;
    payload << in.rdbuf();
    if (!in.good() && !in.eof()) {
      if (error) {
        *error = "cannot read founding DSL source: " + source_path.string();
      }
      return false;
    }
    std::ostringstream name;
    name << std::setfill('0') << std::setw(4) << i << "_"
         << sanitize_bundle_snapshot_filename(source_path.filename().string());
    const auto snapshot_path = bundle_dir / name.str();
    if (!write_runtime_text_file_atomic(snapshot_path, payload.str(), error)) {
      return false;
    }
    bundle_manifest.files.push_back(
        cuwacunu::hero::hashimyei::founding_dsl_bundle_manifest_file_t{
            .source_path =
                compact_runtime_source_path_label(source_path.string()),
            .snapshot_relpath = name.str(),
            .sha256_hex = lowercase_copy(
                cuwacunu::iitepi::contract_space_t::sha256_hex_for_file(
                    snapshot_path.string())),
        });
  }
  bundle_manifest.founding_dsl_bundle_aggregate_sha256_hex =
      cuwacunu::hero::hashimyei::compute_founding_dsl_bundle_aggregate_sha256(
          bundle_manifest);
  if (bundle_manifest.founding_dsl_bundle_aggregate_sha256_hex.empty()) {
    if (error)
      *error = "cannot compute founding DSL bundle aggregate digest";
    return false;
  }

  return cuwacunu::hero::hashimyei::write_founding_dsl_bundle_manifest(
      store_root, bundle_manifest, error);
}
