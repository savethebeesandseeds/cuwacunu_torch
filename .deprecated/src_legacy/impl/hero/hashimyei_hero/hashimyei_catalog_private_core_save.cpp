bool save_component_manifest(const std::filesystem::path &store_root,
                             const component_manifest_t &manifest,
                             std::filesystem::path *out_path,
                             std::string *error) {
  clear_error(error);
  component_manifest_t normalized = manifest;
  normalize_component_manifest_inplace(&normalized);
  std::string validate_error;
  if (!validate_component_manifest(normalized, &validate_error)) {
    set_error(error, "component manifest invalid: " + validate_error);
    return false;
  }

  const std::string component_id = compute_component_manifest_id(normalized);
  if (trim_ascii(component_id).empty() || component_id == "component.invalid") {
    set_error(error, "component manifest id is invalid");
    return false;
  }

  std::error_code ec;
  const auto dir = component_manifest_directory(
      store_root, normalized.canonical_path, component_id);
  std::filesystem::create_directories(dir, ec);
  if (ec) {
    set_error(error,
              "cannot create component manifest directory: " + dir.string());
    return false;
  }

  const auto target = component_manifest_path(
      store_root, normalized.canonical_path, component_id);
  const auto tmp = target.string() + ".tmp";
  std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
  if (!out) {
    set_error(error, "cannot write component manifest temporary file: " + tmp);
    return false;
  }
  out << component_manifest_payload(normalized);
  out.flush();
  if (!out) {
    set_error(error, "cannot flush component manifest temporary file: " + tmp);
    return false;
  }
  out.close();

  std::filesystem::rename(tmp, target, ec);
  if (ec) {
    std::filesystem::remove(tmp, ec);
    set_error(error, "cannot rename component manifest into place: " +
                         target.string());
    return false;
  }
  if (out_path)
    *out_path = target;
  return true;
}

bool save_run_manifest(const std::filesystem::path &store_root,
                       const run_manifest_t &manifest,
                       std::filesystem::path *out_path, std::string *error) {
  clear_error(error);
  run_manifest_t normalized = manifest;
  normalize_run_manifest_inplace(&normalized);
  if (normalized.schema != cuwacunu::hashimyei::kRunManifestSchemaV2) {
    set_error(error, std::string("run manifest schema must be ") +
                         cuwacunu::hashimyei::kRunManifestSchemaV2);
    return false;
  }
  if (normalized.run_id.empty()) {
    set_error(error, "run manifest requires run_id");
    return false;
  }
  std::string identity_error;
  if (!cuwacunu::hashimyei::validate_hashimyei(normalized.campaign_identity,
                                               &identity_error)) {
    set_error(error,
              "run manifest campaign_identity invalid: " + identity_error);
    return false;
  }
  if (!validate_wave_contract_binding(normalized.wave_contract_binding,
                                      &identity_error)) {
    set_error(error,
              "run manifest wave_contract_binding invalid: " + identity_error);
    return false;
  }

  std::error_code ec;
  const auto dir = run_manifest_directory(store_root, normalized.run_id);
  std::filesystem::create_directories(dir, ec);
  if (ec) {
    set_error(error, "cannot create run manifest directory: " + dir.string());
    return false;
  }

  const auto target = run_manifest_path(store_root, normalized.run_id);
  const auto tmp = target.string() + ".tmp";
  std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
  if (!out) {
    set_error(error, "cannot write run manifest temporary file: " + tmp);
    return false;
  }
  out << run_manifest_payload(normalized);
  out.flush();
  if (!out) {
    set_error(error, "cannot flush run manifest temporary file: " + tmp);
    return false;
  }
  out.close();

  std::filesystem::rename(tmp, target, ec);
  if (ec) {
    std::filesystem::remove(tmp, ec);
    set_error(error,
              "cannot rename run manifest into place: " + target.string());
    return false;
  }
  if (out_path)
    *out_path = target;
  return true;
}

hashimyei_catalog_store_t::~hashimyei_catalog_store_t() {
  std::string ignored;
  (void)close(&ignored);
}
